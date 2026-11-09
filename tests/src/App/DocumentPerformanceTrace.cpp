// SPDX-License-Identifier: LGPL-2.1-or-later
#include "DocumentPerformanceTrace.h"
#include <string>

namespace App::DocumentPerformanceTrace
{
namespace
{
thread_local Counters state;
thread_local bool recording = false;
}  // namespace
bool begin() noexcept
{
    if (recording || state.saveActive || state.mapActive) {
        return false;
    }
    state = {};
    recording = true;
    return true;
}
bool end() noexcept
{
    if (!recording || state.saveActive || state.mapActive) {
        return false;
    }
    recording = false;
    return true;
}
Counters counters() noexcept
{
    return state;
}
void record(bool child) noexcept
{
    if (recording && state.mapActive) {
        if (child) {
            ++state.childRecords;
        }
        else {
            ++state.mapRecords;
        }
    }
}
void tokens(std::uint64_t bytes) noexcept
{
    if (recording && state.mapActive) {
        ++state.tokenBuffers;
        state.tokenBytes += bytes;
    }
}
Scope::Scope(Kind kind) noexcept
    : kind(kind)
    , enabled(recording)
{
    if (!enabled) {
        return;
    }
    started = std::chrono::steady_clock::now();
    if (kind == Kind::Save) {
        ++state.saveCalls;
        ++state.saveActive;
    }
    else {
        ++state.mapCalls;
        ++state.mapActive;
    }
}
Scope::~Scope() noexcept
{
    finish(false, false);
}
void Scope::complete(bool empty) noexcept
{
    finish(true, empty);
}
void Scope::finish(bool completed, bool empty) noexcept
{
    if (!enabled) {
        return;
    }
    enabled = false;
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::steady_clock::now() - started
    )
                             .count();
    if (kind == Kind::Save) {
        if (--state.saveActive == 0) {
            state.saveNanoseconds += elapsed;
        }
        if (completed) {
            ++state.saveCompleted;
        }
        else {
            ++state.saveFailed;
        }
        if (empty) {
            ++state.saveEmpty;
        }
    }
    else {
        if (--state.mapActive == 0) {
            state.mapNanoseconds += elapsed;
        }
        if (completed) {
            ++state.mapCompleted;
        }
        else {
            ++state.mapFailed;
        }
    }
}
}  // namespace App::DocumentPerformanceTrace
extern "C" bool fc_document_perf_begin() noexcept
{
    return App::DocumentPerformanceTrace::begin();
}
extern "C" bool fc_document_perf_end() noexcept
{
    return App::DocumentPerformanceTrace::end();
}
extern "C" const char* fc_document_perf_snapshot() noexcept
{
    try {
        const auto c = App::DocumentPerformanceTrace::counters();
        thread_local std::string result;
        result = "{";
#define FIELD(name, member) result += "\"" name "\":" + std::to_string(c.member) + ",";
        FIELD("save_nanoseconds", saveNanoseconds)
        FIELD("save_calls", saveCalls)
        FIELD("save_completed", saveCompleted)
        FIELD("save_failed", saveFailed)
        FIELD("save_empty", saveEmpty)
        FIELD("save_active", saveActive)
        FIELD("map_nanoseconds", mapNanoseconds)
        FIELD("map_calls", mapCalls)
        FIELD("map_completed", mapCompleted)
        FIELD("map_failed", mapFailed)
        FIELD("map_active", mapActive)
        FIELD("map_records", mapRecords)
        FIELD("child_records", childRecords)
        FIELD("token_buffers", tokenBuffers)
        FIELD("token_bytes", tokenBytes)
#undef FIELD
        result.back() = '}';
        return result.c_str();
    }
    catch (...) {
        return nullptr;
    }
}
