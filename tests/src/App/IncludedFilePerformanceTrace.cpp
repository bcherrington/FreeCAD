// SPDX-License-Identifier: LGPL-2.1-or-later
#include "IncludedFilePerformanceTrace.h"

#include <string>

namespace App::IncludedFilePerformanceTrace
{
namespace
{
thread_local Counters state;
thread_local bool recording = false;
}  // namespace

bool begin() noexcept
{
    if (recording || state.active) {
        return false;
    }
    state = {};
    recording = true;
    return true;
}

bool end() noexcept
{
    if (!recording || state.active) {
        return false;
    }
    recording = false;
    return true;
}

Counters counters() noexcept
{
    return state;
}

Restore::Restore() noexcept
    : enabled(recording)
{
    if (enabled) {
        started = std::chrono::steady_clock::now();
        ++state.active;
    }
}

Restore::~Restore() noexcept
{
    finish(Outcome::Failed);
}

void Restore::finish(Outcome outcome) noexcept
{
    if (!enabled) {
        return;
    }
    const auto finished = std::chrono::steady_clock::now();
    state.restoreNanoseconds
        += std::chrono::duration_cast<std::chrono::nanoseconds>(finished - started).count();
    --state.active;
    enabled = false;
    switch (outcome) {
        case Outcome::Completed:
            ++state.completed;
            break;
        case Outcome::Reused:
            ++state.reused;
            break;
        case Outcome::Failed:
            ++state.failed;
            break;
    }
}

void Restore::complete() noexcept
{
    finish(Outcome::Completed);
}

void Restore::reuse() noexcept
{
    finish(Outcome::Reused);
}
}  // namespace App::IncludedFilePerformanceTrace

extern "C" bool fc_included_file_trace_begin() noexcept
{
    return App::IncludedFilePerformanceTrace::begin();
}

extern "C" bool fc_included_file_trace_end() noexcept
{
    return App::IncludedFilePerformanceTrace::end();
}

extern "C" const char* fc_included_file_trace_snapshot() noexcept
{
    try {
        const auto state = App::IncludedFilePerformanceTrace::counters();
        thread_local std::string result;
        result = "{\"restore_nanoseconds\":" + std::to_string(state.restoreNanoseconds)
            + ",\"completed\":" + std::to_string(state.completed) + ",\"failed\":"
            + std::to_string(state.failed) + ",\"reused\":" + std::to_string(state.reused)
            + ",\"active\":" + std::to_string(state.active) + "}";
        return result.c_str();
    }
    catch (...) {
        return nullptr;
    }
}
