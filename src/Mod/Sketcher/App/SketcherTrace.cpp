// SPDX-License-Identifier: LGPL-2.1-or-later
#include "SketcherTrace.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <mutex>
#include <new>

#include <Base/Console.h>
#include <nlohmann/json.hpp>

namespace Sketcher::Trace
{
namespace
{
thread_local Binding binding;
thread_local const Hooks* hooks {};
thread_local std::size_t operationDepth {};
thread_local std::size_t suppressedAt {};
std::atomic<std::uint64_t> nextOperation {1};
std::recursive_mutex emissionMutex;

std::uint64_t now() noexcept
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()
    )
        .count();
}
const char* outcomeName(Outcome outcome)
{
    switch (outcome) {
        case Outcome::Success:
            return "success";
        case Outcome::Rejected:
            return "rejected";
        case Outcome::RecoverableError:
            return "recoverable_error";
        case Outcome::Exception:
            return "exception";
        case Outcome::NotRun:
            return "not_run";
    }
    return "exception";
}
const char* rankName(RankState state)
{
    switch (state) {
        case RankState::NotRun:
            return "not_run";
        case RankState::Computed:
            return "computed";
        case RankState::Exception:
            return "exception";
    }
    return "unavailable";
}
void emit(const Context& context) noexcept
{
    try {
        if (context.sink) {
            context.sink(context, context.user);
        }
        else {
            auto output = serialize(context);
            std::lock_guard lock(emissionMutex);
            Base::Console().log("{}", output);
        }
    }
    catch (...) {
        // Diagnostics must never replace a modelling result/exception.
    }
}
}  // namespace

Binding& current() noexcept
{
    return binding;
}
bool enabled() noexcept
{
    if (hooks) {
        return hooks->enabled;
    }
    static const bool value = [] {
        const char* setting = std::getenv("FREECAD_SKETCHER_TRACE");
        return setting && std::strcmp(setting, "1") == 0;
    }();
    return value;
}
const char* phaseName(Phase phase) noexcept
{
    switch (phase) {
        case Phase::Operation:
            return "operation";
        case Phase::ExternalGeometry:
            return "external_geometry";
        case Phase::GeometryAcceptance:
            return "geometry_acceptance";
        case Phase::Setup:
            return "setup";
        case Phase::Construction:
            return "setup_construction";
        case Phase::Diagnosis:
            return "diagnosis";
        case Phase::Jacobian:
            return "jacobian";
        case Phase::Dispatch:
            return "qr_dispatch";
        case Phase::Join:
            return "qr_join";
        case Phase::QRJ:
            return "qr_j";
        case Phase::QRJT:
            return "qr_jt";
        case Phase::QRCompute:
            return "qr_compute";
        case Phase::SparseConversion:
            return "sparse_conversion";
        case Phase::RedundantAnalysis:
            return "redundant_analysis";
        case Phase::RedundantSolve:
            return "redundant_solve";
        case Phase::NumericalSolve:
            return "numerical_solve";
        case Phase::SolveAttempt:
            return "solve_attempt";
        case Phase::GeometryWriteback:
            return "geometry_writeback";
        case Phase::Shape:
            return "shape";
    }
    return "unknown";
}
TestHooks::TestHooks(const Hooks& value) noexcept
    : previous(hooks)
{
    hooks = &value;
}
TestHooks::~TestHooks()
{
    hooks = previous;
}

Span::Span(Phase phase, const char* reason, const char* entry) noexcept
{
    begin(phase, reason, entry);
}
void Span::begin(Phase phase, const char* reason, const char* entry) noexcept
{
    previous = binding;
    if (!binding.context) {
        return;
    }
    auto& context = *binding.context;
    auto index = context.used.fetch_add(1, std::memory_order_relaxed);
    if (index >= context.capacity) {
        context.dropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    id = index + 1;
    exceptions = std::uncaught_exceptions();
    auto& record = context.records[index];
    record.phase = phase;
    record.reason = std::strcmp(reason, "nested") == 0 && binding.parent
        ? context.records[binding.parent - 1].reason
        : reason;
    record.entry = entry;
    record.parent = binding.parent;
    record.lane = binding.lane;
    record.callerThread = std::this_thread::get_id() == context.caller;
    record.start = context.clock() - context.origin;
    binding.parent = id;
}
Span::~Span()
{
    finish();
}
void Span::finish() noexcept
{
    if (!id) {
        return;
    }
    auto* value = record();
    if (std::uncaught_exceptions() > exceptions) {
        value->outcome = Outcome::Exception;
        if (value->phase == Phase::QRJ || value->phase == Phase::QRJT) {
            if (auto* counters = diagnosisCounters()) {
                (value->phase == Phase::QRJ ? counters->stateJ
                                            : counters->stateJT) = RankState::Exception;
            }
        }
    }
    value->end = binding.context->clock() - binding.context->origin;
    value->closed = true;
    binding = previous;
    id = 0;
}
Record* Span::record() noexcept
{
    return id ? &previous.context->records[id - 1] : nullptr;
}
void Span::result(Outcome outcome, std::int64_t status) noexcept
{
    if (auto* value = record()) {
        value->outcome = outcome;
        value->status = status;
        value->hasStatus = status != std::numeric_limits<std::int64_t>::min();
    }
}
void Span::inputDriving(std::int64_t count) noexcept
{
    if (id) {
        binding.inputDriving = count;
        binding.context->inputDriving = count;
    }
}
Operation::Operation(const char* entry, const char* reason, const void* document, const void* owner) noexcept
    : outer(binding)
{
    ++operationDepth;
    if (suppressedAt) {
        return;
    }
    if (binding.context
        && ((!owner && !document)
            || (binding.context->owner == owner && binding.context->documentObject == document))) {
        // These entry points are also called internally. Only a new root is
        // an explicit solve/additional setup; nested calls retain their caller.
        if (binding.parent
            && (std::strcmp(entry, "solve") == 0 || std::strcmp(entry, "setup") == 0
                || std::strcmp(entry, "additional_setup") == 0)) {
            reason = binding.context->records[binding.parent - 1].reason;
        }
        begin(Phase::Operation, reason, entry);
        return;
    }
    binding = {};
    auto droppedChild = [this] {
        suppressedAt = operationDepth;
        if (outer.context) {
            outer.context->dropped.fetch_add(1, std::memory_order_relaxed);
        }
    };
    if (!enabled()) {
        droppedChild();
        return;
    }
    if (hooks && hooks->failAllocation) {
        droppedChild();
        return;
    }
    owned.reset(new (std::nothrow) Context);
    if (!owned) {
        droppedChild();
        return;
    }
    owned->clock = hooks && hooks->clock ? hooks->clock : now;
    owned->sink = hooks ? hooks->sink : nullptr;
    owned->user = hooks ? hooks->user : nullptr;
    owned->launch = hooks ? hooks->launch : Launch::Default;
    owned->capacity = hooks ? std::clamp(hooks->capacity, std::size_t(1), Context::Capacity)
                            : Context::Capacity;
    owned->operation = nextOperation.fetch_add(1, std::memory_order_relaxed);
    owned->parentOperation = outer.context ? outer.context->operation : 0;
    owned->owner = owner;
    owned->documentObject = document;
    owned->caller = std::this_thread::get_id();
    owned->origin = owned->clock();
    binding = {owned.get()};
    begin(Phase::Operation, reason, entry);
}
Operation::~Operation()
{
    finish();
    if (owned) {
        emit(*owned);
    }
    binding = outer;
    if (--operationDepth < suppressedAt) {
        suppressedAt = 0;
    }
}
void Operation::identify(const char* document, const char* sketch) noexcept
{
    if (!owned) {
        return;
    }
    try {
        owned->document = document ? document : "";
        owned->sketch = sketch ? sketch : "";
    }
    catch (...) {
        owned->dropped.fetch_add(1, std::memory_order_relaxed);
    }
}
Diagnosis::Diagnosis() noexcept
    : Span(
          Phase::Diagnosis,
          binding.context && binding.parent ? binding.context->records[binding.parent - 1].reason
                                            : "initial"
      )
{
    if (auto* value = record()) {
        value->diagnosisOrdinal = ++binding.context->nextDiagnosis;
        value->counters.inputDriving = binding.inputDriving >= 0 ? binding.inputDriving
                                                                 : binding.context->inputDriving;
        binding.diagnosis = id;
    }
}
Counters* diagnosisCounters() noexcept
{
    return binding.context && binding.diagnosis
        ? &binding.context->records[binding.diagnosis - 1].counters
        : nullptr;
}
void matrixCounters(
    std::int64_t equations,
    std::int64_t unknowns,
    std::int64_t rows,
    std::int64_t columns
) noexcept
{
    if (auto* c = diagnosisCounters()) {
        c->diagnosticEquations = equations;
        c->diagnosticUnknowns = unknowns;
        c->rows = rows;
        c->columns = columns;
        c->effectiveRows = equations;
        c->effectiveColumns = unknowns;
        if (columns == 0
            || rows <= std::numeric_limits<std::int64_t>::max()
                    / static_cast<std::int64_t>(sizeof(double)) / columns) {
            c->bytes = rows * columns * static_cast<std::int64_t>(sizeof(double));
        }
    }
}
void qrRank(bool transposed, std::int64_t rank) noexcept
{
    if (auto* c = diagnosisCounters()) {
        (transposed ? c->rankJT : c->rankJ) = rank;
        (transposed ? c->stateJT : c->stateJ) = RankState::Computed;
    }
}
Worker worker() noexcept
{
    Worker result {binding};
    if (binding.context) {
        result.binding.lane = binding.context->nextLane.fetch_add(1, std::memory_order_relaxed);
        result.launch = binding.context->launch;
    }
    return result;
}
WorkerBinding::WorkerBinding(Worker value) noexcept
    : previous(binding)
{
    binding = value.binding;
}
WorkerBinding::~WorkerBinding()
{
    binding = previous;
}

std::string serialize(const Context& context)
{
    using Json = nlohmann::json;
    auto nullable = [](std::int64_t value) -> Json {
        return value < 0 ? Json(nullptr) : Json(value);
    };
    std::string output;
    const auto used = std::min<std::size_t>(context.used.load(), context.capacity);
    bool complete = context.dropped.load() == 0;
    for (std::size_t i = 0; i < used; ++i) {
        complete = complete && context.records[i].closed;
    }
    for (std::size_t i = 0; i < used; ++i) {
        const auto& r = context.records[i];
        Json item = {
            {"schema", "sketcher-phase-v1"},
            {"operation_id", context.operation},
            {"parent_operation_id",
             context.parentOperation ? Json(context.parentOperation) : Json(nullptr)},
            {"span_id", i + 1},
            {"parent_span_id", r.parent ? Json(r.parent) : Json(nullptr)},
            {"document", context.document},
            {"sketch", context.sketch},
            {"entry", r.entry},
            {"reason", r.reason},
            {"phase", phaseName(r.phase)},
            {"lane", r.lane},
            {"caller_thread", r.callerThread},
            {"start_ns", r.start},
            {"end_ns", r.closed ? Json(r.end) : Json(nullptr)},
            {"elapsed_ns", r.closed ? Json(r.end - r.start) : Json(nullptr)},
            {"outcome", outcomeName(r.outcome)},
            {"status", r.hasStatus ? Json(r.status) : Json(nullptr)},
            {"complete", complete},
            {"dropped_records", context.dropped.load()}
        };
        if (r.phase == Phase::Diagnosis) {
            const auto& c = r.counters;
            item["diagnosis_ordinal"] = r.diagnosisOrdinal;
            item["counters"] = {
                {"input_active_driving_constraints", nullable(c.inputDriving)},
                {"solver_equations_pre_filter", nullable(c.equations)},
                {"unknowns_pre_driven_filter", nullable(c.unknowns)},
                {"diagnostic_equations", nullable(c.diagnosticEquations)},
                {"diagnostic_unknowns", nullable(c.diagnosticUnknowns)},
                {"allocated_rows", nullable(c.rows)},
                {"allocated_columns", nullable(c.columns)},
                {"allocated_payload_bytes", nullable(c.bytes)},
                {"effective_rows", nullable(c.effectiveRows)},
                {"effective_columns", nullable(c.effectiveColumns)},
                {"qr_method",
                 c.method == Method::Unknown ? Json(nullptr)
                                             : Json(c.method == Method::Dense ? "dense" : "sparse")},
                {"rank_j", nullable(c.rankJ)},
                {"rank_jt", nullable(c.rankJT)},
                {"rank_j_status", rankName(c.stateJ)},
                {"rank_jt_status", rankName(c.stateJT)}
            };
        }
        output += "SketcherPhase " + item.dump() + '\n';
    }
    return output;
}
}  // namespace Sketcher::Trace
