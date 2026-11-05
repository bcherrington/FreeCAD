// SPDX-License-Identifier: LGPL-2.1-or-later
#include <algorithm>
#include <atomic>
#include <future>
#include <stdexcept>
#include <thread>

#include <Mod/Sketcher/App/SketcherTrace.h>
#include "SketcherTestHelpers.h"

namespace T = Sketcher::Trace;
namespace
{
std::atomic<std::uint64_t> ticks;
std::uint64_t clockTick() noexcept
{
    return ticks.fetch_add(1);
}
struct Snapshot
{
    std::uint64_t operation;
    std::uint64_t parent;
    std::uint32_t dropped;
    std::string document;
    std::string sketch;
    std::vector<T::Record> records;
    std::string json;
};
struct Capture
{
    std::vector<Snapshot> snapshots;
    static void sink(const T::Context& c, void* user)
    {
        auto& self = *static_cast<Capture*>(user);
        const auto count = std::min<std::size_t>(c.used.load(), c.capacity);
        self.snapshots.push_back(
            {c.operation,
             c.parentOperation,
             c.dropped.load(),
             c.document,
             c.sketch,
             {c.records.begin(), c.records.begin() + count},
             T::serialize(c)}
        );
    }
    T::Hooks hooks(T::Launch launch = T::Launch::Default)
    {
        return {true, clockTick, sink, this, launch};
    }
};
const T::Record* find(const Snapshot& snapshot, T::Phase phase)
{
    auto it = std::find_if(
        snapshot.records.begin(),
        snapshot.records.end(),
        [phase](const auto& record) { return record.phase == phase; }
    );
    return it == snapshot.records.end() ? nullptr : &*it;
}
void closed(const Snapshot& snapshot)
{
    ASSERT_FALSE(snapshot.records.empty());
    for (const auto& record : snapshot.records) {
        EXPECT_TRUE(record.closed);
        EXPECT_LE(record.start, record.end);
        if (record.parent) {
            ASSERT_LE(record.parent, snapshot.records.size());
            const auto& parent = snapshot.records[record.parent - 1];
            EXPECT_LE(parent.start, record.start);
            EXPECT_LE(record.end, parent.end);
        }
    }
}
}  // namespace

TEST(SketcherTrace, disabledDoesNotReadClockAllocateContextOrEmit)
{
    Capture capture;
    auto hooks = capture.hooks();
    hooks.enabled = false;
    T::TestHooks injection(hooks);
    auto initial = ticks.load();
    {
        T::Operation root("test", "disabled");
        T::Span setup(T::Phase::Setup);
        T::Diagnosis diagnosis;
        EXPECT_FALSE(root);
        EXPECT_EQ(T::current().context, nullptr);
        EXPECT_EQ(T::diagnosisCounters(), nullptr);
    }
    EXPECT_EQ(ticks.load(), initial);
    EXPECT_TRUE(capture.snapshots.empty());
}

TEST(SketcherTrace, nestedSpansAndExplicitFailureClose)
{
    Capture capture;
    auto hooks = capture.hooks();
    T::TestHooks injection(hooks);
    {
        T::Operation root("execute", "document_recompute");
        root.identify("Doc\"name", "Sketch");
        T::Span setup(T::Phase::Setup);
        setup.result(T::Outcome::Rejected, -3);
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& snapshot = capture.snapshots.front();
    closed(snapshot);
    EXPECT_EQ(snapshot.records.size(), 2);
    EXPECT_EQ(snapshot.records[1].outcome, T::Outcome::Rejected);
    EXPECT_EQ(snapshot.records[1].status, -3);
    EXPECT_NE(snapshot.json.find("Doc\\\"name"), std::string::npos);
    EXPECT_NE(snapshot.json.find("\"status\":-3"), std::string::npos);
    EXPECT_EQ(T::current().context, nullptr);
}

TEST(SketcherTrace, unwindingPreservesModelException)
{
    Capture capture;
    auto hooks = capture.hooks();
    T::TestHooks injection(hooks);
    EXPECT_THROW(
        {
            T::Operation root("test", "exception");
            T::Span child(T::Phase::Setup);
            throw std::runtime_error("model exception");
        },
        std::runtime_error
    );
    ASSERT_EQ(capture.snapshots.size(), 1);
    closed(capture.snapshots.front());
    for (const auto& record : capture.snapshots.front().records) {
        EXPECT_EQ(record.outcome, T::Outcome::Exception);
    }
    EXPECT_EQ(T::current().context, nullptr);
}

TEST(SketcherTrace, overflowIsExplicitAndRootStillCloses)
{
    Capture capture;
    auto hooks = capture.hooks();
    hooks.capacity = 2;
    T::TestHooks injection(hooks);
    {
        T::Operation root("test", "overflow");
        for (int i = 0; i < 8; ++i) {
            T::Span child(T::Phase::Setup);
        }
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    EXPECT_EQ(s.dropped, 7);
    EXPECT_EQ(s.records.size(), 2);
    EXPECT_NE(s.json.find("\"complete\":false"), std::string::npos);
}

TEST(SketcherTrace, allocationAndSinkFailuresDoNotEscape)
{
    Capture capture;
    auto hooks = capture.hooks();
    hooks.failAllocation = true;
    {
        T::TestHooks injection(hooks);
        auto before = ticks.load();
        T::Operation root("test", "allocation_failure");
        EXPECT_FALSE(root);
        EXPECT_EQ(ticks.load(), before);
        // A failed root must not restart tracing half way through its operation.
        auto recovered = hooks;
        recovered.failAllocation = false;
        T::TestHooks recovery(recovered);
        T::Operation child("test", "nested_after_failure");
        EXPECT_FALSE(child);
        EXPECT_EQ(T::current().context, nullptr);
    }
    EXPECT_TRUE(capture.snapshots.empty());
    hooks.failAllocation = false;
    hooks.sink = [](const T::Context&, void*) {
        throw std::bad_alloc();
    };
    T::TestHooks injection(hooks);
    EXPECT_NO_THROW({ T::Operation root("test", "serialization_failure"); });
    EXPECT_EQ(T::current().context, nullptr);
}

TEST(SketcherTrace, nestedOwnersAndDisabledFailureRestoreParent)
{
    Capture capture;
    auto hooks = capture.hooks();
    T::TestHooks injection(hooks);
    int doc, first, second;
    {
        T::Operation parent("execute", "document_recompute", &doc, &first);
        parent.identify("Document", "First");
        {
            T::Operation nested("solve", "explicit_solve", &doc, &second);
            nested.identify("Document", "Second");
        }
        auto failed = hooks;
        failed.failAllocation = true;
        {
            T::TestHooks fail(failed);
            T::Operation nested("solve", "failed", &doc, &second);
            EXPECT_EQ(T::current().context, nullptr);
        }
        EXPECT_NE(T::current().context, nullptr);
        T::Span child(T::Phase::Shape);
    }
    ASSERT_EQ(capture.snapshots.size(), 2);
    EXPECT_EQ(capture.snapshots[0].parent, capture.snapshots[1].operation);
    EXPECT_EQ(capture.snapshots[0].sketch, "Second");
    EXPECT_EQ(capture.snapshots[1].sketch, "First");
    EXPECT_EQ(capture.snapshots[1].dropped, 1);
    for (const auto& s : capture.snapshots) {
        closed(s);
    }
}

TEST(SketcherTrace, overlappingWorkerIntervalsUseSeparateLane)
{
    Capture capture;
    auto hooks = capture.hooks(T::Launch::Async);
    T::TestHooks injection(hooks);
    std::promise<void> started, release, done;
    auto began = started.get_future();
    auto released = release.get_future();
    auto finished = done.get_future();
    {
        T::Operation root("execute", "document_recompute");
        T::Diagnosis diagnosis;
        auto future = T::async([&] {
            {
                T::Span qr(T::Phase::QRJ);
                started.set_value();
                released.wait();
                T::qrRank(false, 1);
            }
            done.set_value();
        });
        began.wait();
        {
            T::Span qr(T::Phase::QRJT);
            release.set_value();
            finished.wait();
            T::qrRank(true, 1);
        }
        T::wait(future);
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    auto j = find(s, T::Phase::QRJ);
    auto jt = find(s, T::Phase::QRJT);
    auto join = find(s, T::Phase::Join);
    ASSERT_NE(j, nullptr);
    ASSERT_NE(jt, nullptr);
    ASSERT_NE(join, nullptr);
    EXPECT_NE(j->lane, jt->lane);
    EXPECT_FALSE(j->callerThread);
    EXPECT_LT(j->start, jt->start);
    EXPECT_LT(jt->start, j->end);
    EXPECT_LT(j->end, jt->end);
    auto d = find(s, T::Phase::Diagnosis);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->counters.rankJ, 1);
    EXPECT_EQ(d->counters.rankJT, 1);
}

TEST(SketcherTrace, deferredTaskRunsInsideJoinOnCaller)
{
    Capture capture;
    auto hooks = capture.hooks(T::Launch::Deferred);
    T::TestHooks injection(hooks);
    bool ran = false;
    {
        T::Operation root("execute", "document_recompute");
        T::Diagnosis diagnosis;
        auto future = T::async([&] {
            T::Span qr(T::Phase::QRJ);
            ran = true;
        });
        EXPECT_FALSE(ran);
        T::wait(future);
        EXPECT_TRUE(ran);
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    auto j = find(s, T::Phase::QRJ);
    auto join = find(s, T::Phase::Join);
    ASSERT_NE(j, nullptr);
    ASSERT_NE(join, nullptr);
    EXPECT_TRUE(j->callerThread);
    EXPECT_NE(j->lane, join->lane);
    EXPECT_LT(join->start, j->start);
    EXPECT_LT(j->end, join->end);
}

TEST(SketcherTrace, workerExceptionIsRecordedWithoutChangingWaitSemantics)
{
    Capture capture;
    auto hooks = capture.hooks(T::Launch::Async);
    T::TestHooks injection(hooks);
    {
        T::Operation root("execute", "document_recompute");
        T::Diagnosis diagnosis;
        auto future = T::async([] {
            T::Span qr(T::Phase::QRJ);
            throw std::runtime_error("worker");
        });
        EXPECT_NO_THROW(T::wait(future));
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    auto j = find(s, T::Phase::QRJ);
    auto d = find(s, T::Phase::Diagnosis);
    ASSERT_NE(j, nullptr);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(j->outcome, T::Outcome::Exception);
    EXPECT_EQ(d->counters.rankJ, -1);
    EXPECT_EQ(d->counters.stateJ, T::RankState::Exception);
    EXPECT_NE(s.json.find("\"rank_j\":null"), std::string::npos);
}

TEST(SketcherTrace, futureDestructionPrecedesExceptionalOwnerTeardown)
{
    Capture capture;
    auto hooks = capture.hooks(T::Launch::Async);
    T::TestHooks injection(hooks);
    EXPECT_THROW(
        {
            T::Operation root("execute", "document_recompute");
            T::Diagnosis diagnosis;
            auto future = T::async([] {
                T::Span qr(T::Phase::QRJ);
                T::qrRank(false, 0);
            });
            throw std::runtime_error("owner");
        },
        std::runtime_error
    );
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    EXPECT_EQ(s.records.front().outcome, T::Outcome::Exception);
    auto d = find(s, T::Phase::Diagnosis);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->counters.rankJ, 0);
    EXPECT_EQ(d->counters.stateJ, T::RankState::Computed);
}

TEST(SketcherTrace, argumentCopyCountMatchesOriginalAsync)
{
    struct Payload
    {
        std::atomic<int>* copies;
        explicit Payload(std::atomic<int>& c)
            : copies(&c)
        {}
        Payload(const Payload& other)
            : copies(other.copies)
        {
            ++*copies;
        }
        Payload(Payload&& other) noexcept
            : copies(other.copies)
        {}
    };
    std::atomic<int> copies {};
    Payload payload(copies);
    auto fn = [](const Payload&) {
        T::Span qr(T::Phase::QRJ);
    };
    {
        auto future = std::async(fn, payload);
        future.wait();
    }
    int baseline = copies.exchange(0);
    Capture capture;
    auto hooks = capture.hooks(T::Launch::Async);
    T::TestHooks injection(hooks);
    {
        T::Operation root("test", "copies");
        auto future = T::async(fn, payload);
        T::wait(future);
    }
    EXPECT_EQ(copies.load(), baseline);
    EXPECT_EQ(baseline, 1);
}

TEST(SketcherTrace, independentThreadsDoNotShareOperationState)
{
    Capture a, b;
    auto run = [](Capture& capture) {
        auto hooks = capture.hooks();
        T::TestHooks injection(hooks);
        T::Operation root("test", "thread");
        T::Span setup(T::Phase::Setup);
    };
    std::thread first(run, std::ref(a));
    std::thread second(run, std::ref(b));
    first.join();
    second.join();
    ASSERT_EQ(a.snapshots.size(), 1);
    ASSERT_EQ(b.snapshots.size(), 1);
    EXPECT_NE(a.snapshots[0].operation, b.snapshots[0].operation);
    EXPECT_EQ(a.snapshots[0].parent, 0);
    EXPECT_EQ(b.snapshots[0].parent, 0);
    closed(a.snapshots[0]);
    closed(b.snapshots[0]);
}

TEST_F(SketchObjectTest, traceKnownLineCountersAndExistingObserverBehaviour)
{
    auto* sketch = getObject();
    Part::GeomLineSegment line;
    SketcherTestHelpers::setupLineSegment(line);
    sketch->addGeometry(&line);
    auto* horizontal = new Sketcher::Constraint;
    horizontal->Type = Sketcher::Horizontal;
    horizontal->setGeoId(0, 0);
    sketch->Constraints.setValues({horizontal});
    sketch->getDocument()->recompute();
    const auto dof = sketch->getLastDoF();
    Capture capture;
    auto hooks = capture.hooks(T::Launch::Async);
    {
        T::TestHooks injection(hooks);
        sketch->touch();
        sketch->getDocument()->recompute();
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    EXPECT_EQ(s.sketch, sketch->getNameInDocument());
    EXPECT_EQ(s.document, sketch->getDocument()->getName());
    int diagnoses = 0, setups = 0;
    for (const auto& r : s.records) {
        if (r.phase == T::Phase::Setup) {
            ++setups;
        }
        if (r.phase != T::Phase::Diagnosis) {
            continue;
        }
        ++diagnoses;
        EXPECT_STREQ(r.reason, "document_recompute");
        const auto& c = r.counters;
        EXPECT_EQ(c.inputDriving, 1);
        EXPECT_EQ(c.equations, 1);
        EXPECT_EQ(c.unknowns, 4);
        EXPECT_EQ(c.diagnosticEquations, 1);
        EXPECT_EQ(c.diagnosticUnknowns, 4);
        EXPECT_EQ(c.rows, 1);
        EXPECT_EQ(c.columns, 4);
        EXPECT_EQ(c.bytes, 32);
        EXPECT_EQ(c.effectiveRows, 1);
        EXPECT_EQ(c.effectiveColumns, 4);
        EXPECT_EQ(c.method, T::Method::Dense);
        EXPECT_EQ(c.rankJ, 1);
        EXPECT_EQ(c.rankJT, 1);
        EXPECT_EQ(c.stateJ, T::RankState::Computed);
        EXPECT_EQ(c.stateJT, T::RankState::Computed);
    }
    EXPECT_EQ(diagnoses, 1);
    EXPECT_EQ(setups, 1);
    EXPECT_EQ(sketch->getLastDoF(), dof);
    EXPECT_GE(sketch->getLastSolveTime(), 0.0);
    EXPECT_FLOAT_EQ(sketch->getLastSolveTime(), sketch->getSolvedSketch().getSolveTime());
}

TEST_F(SketchObjectTest, traceDrivenOnlyDoesNotInventRankZero)
{
    auto* sketch = getObject();
    Part::GeomCircle circle;
    SketcherTestHelpers::setupCircle(circle);
    sketch->addGeometry(&circle);
    auto* radius = new Sketcher::Constraint;
    radius->Type = Sketcher::Radius;
    radius->setGeoId(0, 0);
    radius->setValue(3);
    radius->isDriving = false;
    sketch->Constraints.setValues({radius});
    sketch->getDocument()->recompute();
    Capture capture;
    auto hooks = capture.hooks();
    {
        T::TestHooks injection(hooks);
        sketch->touch();
        sketch->getDocument()->recompute();
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    bool seen = false;
    for (const auto& r : s.records) {
        if (r.phase == T::Phase::Diagnosis) {
            seen = true;
            EXPECT_EQ(r.counters.inputDriving, 0);
            EXPECT_EQ(r.counters.diagnosticEquations, 0);
            EXPECT_EQ(r.counters.rows, 0);
            EXPECT_EQ(r.counters.columns, 0);
            EXPECT_EQ(r.counters.bytes, 0);
            EXPECT_EQ(r.counters.rankJ, -1);
            EXPECT_EQ(r.counters.rankJT, -1);
            EXPECT_EQ(r.counters.stateJ, T::RankState::NotRun);
            EXPECT_EQ(r.counters.stateJT, T::RankState::NotRun);
        }
    }
    EXPECT_TRUE(seen);
    EXPECT_EQ(find(s, T::Phase::QRCompute), nullptr);
}

TEST_F(SketchObjectTest, traceRedundancyIsSeparateFromNumericalSolve)
{
    auto* sketch = getObject();
    Part::GeomLineSegment line;
    SketcherTestHelpers::setupLineSegment(line);
    sketch->addGeometry(&line);
    std::vector<Sketcher::Constraint*> constraints;
    for (int i = 0; i < 2; ++i) {
        auto* c = new Sketcher::Constraint;
        c->Type = Sketcher::Horizontal;
        c->setGeoId(0, 0);
        constraints.push_back(c);
    }
    sketch->Constraints.setValues(std::move(constraints));
    Capture capture;
    auto hooks = capture.hooks();
    {
        T::TestHooks injection(hooks);
        sketch->touch();
        sketch->getDocument()->recompute();
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    EXPECT_NE(find(s, T::Phase::RedundantAnalysis), nullptr);
    EXPECT_NE(find(s, T::Phase::RedundantSolve), nullptr);
    EXPECT_NE(find(s, T::Phase::NumericalSolve), nullptr);
    EXPECT_EQ(s.records.front().outcome, T::Outcome::RecoverableError);
    EXPECT_EQ(sketch->getLastRedundant(), (std::vector<int> {2}));
}

TEST_F(SketchObjectTest, traceSparseQRUsesActualSelectionAndConversion)
{
#ifdef EIGEN_SPARSEQR_COMPATIBLE
    Part::GeomLineSegment line;
    SketcherTestHelpers::setupLineSegment(line);
    Sketcher::Constraint horizontal;
    horizontal.Type = Sketcher::Horizontal;
    horizontal.setGeoId(0, 0);
    Sketcher::Sketch solver;
    solver.setSketchAutoAlgo(false);
    solver.setQRAlgorithm(GCS::EigenSparseQR);
    Capture capture;
    auto hooks = capture.hooks(T::Launch::Deferred);
    {
        T::TestHooks injection(hooks);
        EXPECT_EQ(solver.setUpSketch({&line}, {&horizontal}, 0), 3);
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    auto diagnosis = find(s, T::Phase::Diagnosis);
    ASSERT_NE(diagnosis, nullptr);
    EXPECT_EQ(diagnosis->counters.method, T::Method::Sparse);
    EXPECT_EQ(diagnosis->counters.rankJ, 1);
    EXPECT_EQ(diagnosis->counters.rankJT, 1);
    EXPECT_NE(find(s, T::Phase::SparseConversion), nullptr);
    EXPECT_EQ(solver.getQRAlgorithm(), GCS::EigenSparseQR);
#else
    GTEST_SKIP() << "Sparse QR unavailable with this Eigen version";
#endif
}

TEST_F(SketchObjectTest, traceMalformedFailureClosesBeforeWriteback)
{
    auto* sketch = getObject();
    Part::GeomLineSegment line;
    SketcherTestHelpers::setupLineSegment(line);
    sketch->addGeometry(&line);
    auto* radius = new Sketcher::Constraint;
    radius->Type = Sketcher::Radius;
    radius->setGeoId(0, 0);
    radius->setValue(5);
    sketch->Constraints.setValues({radius});
    Capture capture;
    auto hooks = capture.hooks();
    {
        T::TestHooks injection(hooks);
        sketch->touch();
        sketch->getDocument()->recompute();
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    EXPECT_EQ(s.records.front().outcome, T::Outcome::RecoverableError);
    EXPECT_EQ(find(s, T::Phase::GeometryWriteback), nullptr);
    EXPECT_EQ(find(s, T::Phase::Shape), nullptr);
    EXPECT_EQ(sketch->getLastMalformedConstraints(), (std::vector<int> {1}));
}

TEST_F(SketchObjectTest, traceBlockPostAnalysisKeepsDistinctDiagnosisOrdinals)
{
    auto* sketch = getObject();
    Part::GeomLineSegment a, b;
    a.setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(4, 0, 0));
    b.setPoints(Base::Vector3d(4, 0, 0), Base::Vector3d(8, 1, 0));
    sketch->addGeometry(&a);
    sketch->addGeometry(&b);
    auto* block = new Sketcher::Constraint;
    block->Type = Sketcher::Block;
    block->setGeoId(0, 0);
    auto* coincidence = new Sketcher::Constraint;
    coincidence->Type = Sketcher::Coincident;
    coincidence->setGeoId(0, 0);
    coincidence->setPosId(0, Sketcher::PointPos::end);
    coincidence->setGeoId(1, 1);
    coincidence->setPosId(1, Sketcher::PointPos::start);
    sketch->addConstraint(std::unique_ptr<Sketcher::Constraint>(block));
    sketch->addConstraint(std::unique_ptr<Sketcher::Constraint>(coincidence));
    Capture capture;
    auto hooks = capture.hooks();
    {
        T::TestHooks injection(hooks);
        sketch->touch();
        sketch->getDocument()->recompute();
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    std::uint32_t ordinal = 0;
    int setups = 0, diagnoses = 0;
    bool blockReason = false;
    for (const auto& r : s.records) {
        if (r.phase == T::Phase::Setup) {
            ++setups;
        }
        if (r.phase != T::Phase::Diagnosis) {
            continue;
        }
        ++diagnoses;
        EXPECT_EQ(r.diagnosisOrdinal, ++ordinal);
        blockReason = blockReason || std::string(r.reason) == "block_post_analysis";
    }
    EXPECT_EQ(setups, 1);
    EXPECT_GT(diagnoses, setups);
    EXPECT_TRUE(blockReason);
    EXPECT_EQ(sketch->getLastDoF(), 2);
}

TEST(SketcherTrace, payloadOverflowIsUnknownAndNeverWraps)
{
    Capture capture;
    auto hooks = capture.hooks();
    T::TestHooks injection(hooks);
    {
        T::Operation root("test", "overflow");
        T::Diagnosis diagnosis;
        T::matrixCounters(1, 2, std::numeric_limits<std::int64_t>::max(), 2);
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    auto d = find(capture.snapshots[0], T::Phase::Diagnosis);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->counters.bytes, -1);
}

TEST(SketcherTrace, deferredFutureDestructionDoesNotExecuteDiscardedTask)
{
    Capture capture;
    auto hooks = capture.hooks(T::Launch::Deferred);
    T::TestHooks injection(hooks);
    bool ran = false;
    {
        T::Operation root("test", "discard_deferred");
        auto future = T::async([&] { ran = true; });
    }
    EXPECT_FALSE(ran);
    ASSERT_EQ(capture.snapshots.size(), 1);
    const auto& s = capture.snapshots.front();
    closed(s);
    auto join = find(s, T::Phase::Join);
    ASSERT_NE(join, nullptr);
    EXPECT_STREQ(join->reason, "future_destruction");
}

TEST_F(SketchObjectTest, traceMoveAndUndoUseActualEntryReasons)
{
    auto* sketch = getObject();
    auto* document = sketch->getDocument();
    Part::GeomLineSegment line;
    SketcherTestHelpers::setupLineSegment(line);
    sketch->addGeometry(&line);
    document->recompute();
    Capture capture;
    auto hooks = capture.hooks();
    {
        T::TestHooks injection(hooks);
        EXPECT_EQ(
            sketch->moveGeometry(0, Sketcher::PointPos::start, Base::Vector3d(2, 3, 0), false, true),
            Sketcher::SketchSolveStatus::Success
        );
    }
    ASSERT_EQ(capture.snapshots.size(), 1);
    EXPECT_STREQ(capture.snapshots.front().records.front().entry, "move");
    EXPECT_STREQ(capture.snapshots.front().records.front().reason, "geometry_move");
    closed(capture.snapshots.front());
    document->openTransaction("trace transaction");
    auto horizontal = std::make_unique<Sketcher::Constraint>();
    horizontal->Type = Sketcher::Horizontal;
    horizontal->setGeoId(0, 0);
    sketch->addConstraint(std::move(horizontal));
    document->commitTransaction();
    capture.snapshots.clear();
    {
        T::TestHooks injection(hooks);
        document->undo();
    }
    bool sawUndo = false;
    for (const auto& s : capture.snapshots) {
        closed(s);
        if (std::string(s.records.front().entry) == "undo_redo") {
            sawUndo = true;
            EXPECT_STREQ(s.records.front().reason, "transaction_replay");
        }
    }
    EXPECT_TRUE(sawUndo);
    EXPECT_EQ(sketch->Constraints.getSize(), 0);
}
