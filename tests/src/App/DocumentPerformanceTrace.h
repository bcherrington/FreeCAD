// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <chrono>
#include <cstdint>
#include <FCGlobal.h>

namespace App::DocumentPerformanceTrace
{
struct Counters
{
    std::uint64_t saveNanoseconds = 0, saveCalls = 0, saveCompleted = 0;
    std::uint64_t saveFailed = 0, saveEmpty = 0, saveActive = 0;
    std::uint64_t mapNanoseconds = 0, mapCalls = 0, mapCompleted = 0;
    std::uint64_t mapFailed = 0, mapActive = 0, mapRecords = 0;
    std::uint64_t childRecords = 0, tokenBuffers = 0, tokenBytes = 0;
};
AppExport bool begin() noexcept;
AppExport bool end() noexcept;
AppExport Counters counters() noexcept;
AppExport void record(bool child = false) noexcept;
AppExport void tokens(std::uint64_t bytes) noexcept;

class AppExport Scope
{
public:
    enum class Kind
    {
        Save,
        Map
    };
    explicit Scope(Kind kind) noexcept;
    ~Scope() noexcept;
    void complete(bool empty = false) noexcept;
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    void finish(bool completed, bool empty) noexcept;
    Kind kind;
    bool enabled;
    std::chrono::steady_clock::time_point started;
};
}  // namespace App::DocumentPerformanceTrace
extern "C" {
AppExport bool fc_document_perf_begin() noexcept;
AppExport bool fc_document_perf_end() noexcept;
AppExport const char* fc_document_perf_snapshot() noexcept;
}
