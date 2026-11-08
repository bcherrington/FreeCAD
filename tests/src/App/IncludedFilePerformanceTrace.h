// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <chrono>
#include <cstdint>
#include <FCGlobal.h>

namespace App::IncludedFilePerformanceTrace
{
// Developer-only counters, confined to the thread opening the document. No
// names, paths or payload data are retained. Snapshot outside the open timer.
struct Counters
{
    std::uint64_t restoreNanoseconds = 0;
    std::uint64_t completed = 0;
    std::uint64_t failed = 0;
    std::uint64_t reused = 0;
    std::uint64_t active = 0;
};

AppExport bool begin() noexcept;
AppExport bool end() noexcept;
AppExport Counters counters() noexcept;

class AppExport Restore
{
public:
    Restore() noexcept;
    ~Restore() noexcept;
    Restore(const Restore&) = delete;
    Restore& operator=(const Restore&) = delete;
    void complete() noexcept;
    void reuse() noexcept;

private:
    enum class Outcome
    {
        Failed,
        Completed,
        Reused
    };
    void finish(Outcome outcome) noexcept;
    bool enabled;
    std::chrono::steady_clock::time_point started;
};
}  // namespace App::IncludedFilePerformanceTrace

extern "C" {
AppExport bool fc_included_file_trace_begin() noexcept;
AppExport bool fc_included_file_trace_end() noexcept;
AppExport const char* fc_included_file_trace_snapshot() noexcept;
}
