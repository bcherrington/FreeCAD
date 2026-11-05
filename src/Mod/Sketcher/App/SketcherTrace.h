// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Private diagnostic implementation shared by Sketcher and planegcs. Not a modelling API.
#include <array>
#include <atomic>
#include <cstdint>
#include <future>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <Mod/Sketcher/SketcherGlobal.h>

namespace Sketcher::Trace
{
enum class Phase
{
    Operation,
    ExternalGeometry,
    GeometryAcceptance,
    Setup,
    Construction,
    Diagnosis,
    Jacobian,
    Dispatch,
    Join,
    QRJ,
    QRJT,
    QRCompute,
    SparseConversion,
    RedundantAnalysis,
    RedundantSolve,
    NumericalSolve,
    SolveAttempt,
    GeometryWriteback,
    Shape
};
enum class Outcome
{
    Success,
    Rejected,
    RecoverableError,
    Exception,
    NotRun
};
enum class Method
{
    Unknown,
    Dense,
    Sparse
};
enum class RankState
{
    NotRun,
    Computed,
    Exception
};
enum class Launch
{
    Default,
    Async,
    Deferred
};

struct Counters
{
    std::int64_t inputDriving {-1};
    std::int64_t equations {-1};
    std::int64_t unknowns {-1};
    std::int64_t diagnosticEquations {-1};
    std::int64_t diagnosticUnknowns {-1};
    std::int64_t rows {-1};
    std::int64_t columns {-1};
    std::int64_t bytes {-1};
    std::int64_t effectiveRows {-1};
    std::int64_t effectiveColumns {-1};
    std::int64_t rankJ {-1};
    std::int64_t rankJT {-1};
    Method method {Method::Unknown};
    RankState stateJ {RankState::NotRun};
    RankState stateJT {RankState::NotRun};
};
struct Record
{
    Phase phase {Phase::Operation};
    Outcome outcome {Outcome::Success};
    const char* entry {"phase"};
    const char* reason {"nested"};
    std::uint32_t parent {};
    std::uint32_t lane {};
    std::uint32_t diagnosisOrdinal {};
    std::uint64_t start {};
    std::uint64_t end {};
    std::int64_t status {};
    bool hasStatus {};
    bool callerThread {};
    bool closed {};
    Counters counters;
};
struct Context;
using Clock = std::uint64_t (*)() noexcept;
using Sink = void (*)(const Context&, void*);

// Thread-local test injection. Production always uses the immutable environment,
// steady_clock, default async policy, real allocator and Console sink.
struct Hooks
{
    bool enabled {};
    Clock clock {};
    Sink sink {};
    void* user {};
    Launch launch {Launch::Default};
    std::size_t capacity {4096};
    bool failAllocation {};
};
struct Context
{
    static constexpr std::size_t Capacity = 4096;
    std::array<Record, Capacity> records;
    std::atomic<std::uint32_t> used {0};
    std::atomic<std::uint32_t> dropped {0};
    std::atomic<std::uint32_t> nextLane {1};
    std::uint32_t nextDiagnosis {};
    std::int64_t inputDriving {-1};
    std::uint64_t operation {};
    std::uint64_t parentOperation {};
    std::uint64_t origin {};
    const void* owner {};
    const void* documentObject {};
    std::string document;
    std::string sketch;
    std::thread::id caller;
    Clock clock {};
    Sink sink {};
    void* user {};
    Launch launch {Launch::Default};
    std::size_t capacity {Capacity};
};
struct Binding
{
    Context* context {};
    std::uint32_t parent {};
    std::uint32_t lane {};
    std::uint32_t diagnosis {};
    std::int64_t inputDriving {-1};
};
SketcherExport Binding& current() noexcept;
SketcherExport bool enabled() noexcept;
SketcherExport const char* phaseName(Phase phase) noexcept;
SketcherExport std::string serialize(const Context& context);

class SketcherExport TestHooks
{
public:
    explicit TestHooks(const Hooks& hooks) noexcept;
    ~TestHooks();
    TestHooks(const TestHooks&) = delete;
    TestHooks& operator=(const TestHooks&) = delete;

private:
    const Hooks* previous;
};

class SketcherExport Span
{
public:
    explicit Span(Phase phase, const char* reason = "nested", const char* entry = "phase") noexcept;
    ~Span();
    Span(const Span&) = delete;
    Span& operator=(const Span&) = delete;
    void finish() noexcept;
    void result(Outcome outcome, std::int64_t status = std::numeric_limits<std::int64_t>::min()) noexcept;
    void inputDriving(std::int64_t count) noexcept;
    template<typename Range>
    void inputConstraints(const Range& constraints) noexcept
    {
        if (!id) {
            return;
        }
        std::int64_t count = 0;
        for (const auto* constraint : constraints) {
            count += constraint->isActive && constraint->isDriving;
        }
        inputDriving(count);
    }
    Record* record() noexcept;
    explicit operator bool() const noexcept
    {
        return id != 0;
    }

protected:
    Span() noexcept = default;
    void begin(Phase phase, const char* reason, const char* entry) noexcept;
    Binding previous;
    std::uint32_t id {};
    int exceptions {};
};

class SketcherExport Operation: public Span
{
public:
    Operation(
        const char* entry,
        const char* reason,
        const void* document = nullptr,
        const void* owner = nullptr
    ) noexcept;
    ~Operation();
    void identify(const char* document, const char* sketch) noexcept;

private:
    std::unique_ptr<Context> owned;
    Binding outer;
};

class SketcherExport Diagnosis: public Span
{
public:
    Diagnosis() noexcept;
};
SketcherExport Counters* diagnosisCounters() noexcept;
SketcherExport void matrixCounters(
    std::int64_t equations,
    std::int64_t unknowns,
    std::int64_t rows,
    std::int64_t columns
) noexcept;
SketcherExport void qrRank(bool transposed, std::int64_t rank) noexcept;

struct Worker
{
    Binding binding;
    Launch launch {Launch::Default};
};
SketcherExport Worker worker() noexcept;
class SketcherExport WorkerBinding
{
public:
    explicit WorkerBinding(Worker worker) noexcept;
    ~WorkerBinding();

private:
    Binding previous;
};

// Releasing the same shared state preserves future destruction semantics:
// async work joins, unstarted deferred work is discarded (never forced by wait).
template<typename Result>
class Future
{
public:
    Future(std::future<Result>&& value) noexcept
        : value(std::move(value))
    {}
    Future(Future&&) = default;
    Future(const Future&) = delete;
    ~Future()
    {
        if (!value.valid()) {
            return;
        }
        Span release(Phase::Join, "future_destruction");
        value = std::future<Result>();
    }
    void wait()
    {
        value.wait();
    }

private:
    std::future<Result> value;
};

// std::async retains the same decayed payload arguments/copies and no-policy
// overload in production. The wrapper adds only the small trace binding.
template<typename Function, typename... Args>
auto async(Function&& function, Args&&... args) -> Future<std::invoke_result_t<Function, Args...>>
{
    auto taskWorker = worker();
    Span dispatch(Phase::Dispatch);
    if (!taskWorker.binding.context) {
        return std::async(std::forward<Function>(function), std::forward<Args>(args)...);
    }
    auto task = [taskWorker, fn = std::forward<Function>(function)](auto&&... values) mutable {
        WorkerBinding binding(taskWorker);
        return std::invoke(fn, std::forward<decltype(values)>(values)...);
    };
    // Non-default policies are only injected by native tests.
    if (taskWorker.launch == Launch::Async) {
        return std::async(std::launch::async, std::move(task), std::forward<Args>(args)...);
    }
    if (taskWorker.launch == Launch::Deferred) {
        return std::async(std::launch::deferred, std::move(task), std::forward<Args>(args)...);
    }
    return std::async(std::move(task), std::forward<Args>(args)...);
}
template<typename Future>
void wait(Future& future)
{
    Span join(Phase::Join);
    future.wait();
}
}  // namespace Sketcher::Trace
