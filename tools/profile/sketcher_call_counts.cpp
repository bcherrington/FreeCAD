// SPDX-License-Identifier: LGPL-2.1-or-later

// Linux/ELF diagnostic interposer for baseline measurements. Build with the
// project's compiler and preload only into a task-owned FreeCADCmd process.
// No FreeCAD source, public API, or process-wide production settings are changed.
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <mutex>

namespace
{
struct Counts
{
    const void* object {};
    std::array<char, 128> name {};
    std::uint64_t executes {};
    std::uint64_t setups {};
};

std::array<Counts, 2048> counts;
std::mutex countsMutex;
std::atomic<bool> active {false};
bool invalid {};
thread_local const void* owner {};
void* sketcherHandle {};
void* appHandle {};

template<typename Function>
Function resolve(const char* symbol)
{
    // Python extension modules can be RTLD_LOCAL. Bind their exact loaded paths
    // instead of relying on RTLD_NEXT's global symbol search.
    auto handle = std::strncmp(symbol, "_ZNK3App", 8) == 0 ? appHandle : sketcherHandle;
    auto function = reinterpret_cast<Function>(handle ? dlsym(handle, symbol) : nullptr);
    if (!function) {
        std::fprintf(stderr, "sketcher counter: cannot resolve %s\n", symbol);
        std::_Exit(70);
    }
    return function;
}

struct OwnerScope
{
    const void* previous {owner};
    explicit OwnerScope(const void* object)
    {
        owner = object;
    }
    ~OwnerScope()
    {
        owner = previous;
    }
};

void record(bool execution)
{
    if (!active.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard lock(countsMutex);
    if (!owner) {
        invalid = true;
        return;
    }
    using GetName = const char* (*)(const void*);
    static auto getName = resolve<GetName>("_ZNK3App14DocumentObject17getNameInDocumentEv");
    const char* name = getName(owner);
    if (!name || std::strlen(name) >= counts.front().name.size()) {
        invalid = true;
        return;
    }
    for (auto& row : counts) {
        if (!row.object || row.object == owner) {
            if (!row.object || std::strcmp(row.name.data(), name) != 0) {
                row = {};
                row.object = owner;
                std::strcpy(row.name.data(), name);
            }
            execution ? ++row.executes : ++row.setups;
            return;
        }
    }
    invalid = true;
}
}  // namespace

extern "C" int skperf_bind(const char* sketcherPath, const char* appPath)
{
    sketcherHandle = dlopen(sketcherPath, RTLD_NOW | RTLD_NOLOAD);
    appHandle = dlopen(appPath, RTLD_NOW | RTLD_NOLOAD);
    return sketcherHandle && appHandle ? 1 : 0;
}

extern "C" void skperf_begin()
{
    std::lock_guard lock(countsMutex);
    for (auto& row : counts) {
        row = {};
    }
    invalid = false;
    active.store(true, std::memory_order_relaxed);
}

extern "C" int skperf_end()
{
    active.store(false, std::memory_order_relaxed);
    std::lock_guard lock(countsMutex);
    return invalid ? 0 : 1;
}

extern "C" int skperf_row(
    std::size_t index,
    char* name,
    std::size_t capacity,
    std::uint64_t* executes,
    std::uint64_t* setups
)
{
    std::lock_guard lock(countsMutex);
    if (index >= counts.size() || !counts[index].object) {
        return 0;
    }
    const auto& row = counts[index];
    if (capacity <= std::strlen(row.name.data())) {
        return -1;
    }
    std::strcpy(name, row.name.data());
    *executes = row.executes;
    *setups = row.setups;
    return 1;
}

extern "C" void* countedExecute(void* self) asm("_ZN8Sketcher12SketchObject7executeEv");
extern "C" void* countedExecute(void* self)
{
    static auto original = resolve<void* (*)(void*)>("_ZN8Sketcher12SketchObject7executeEv");
    OwnerScope scope(self);
    record(true);
    return original(self);
}

extern "C" int countedSolve(void* self, bool update) asm("_ZN8Sketcher12SketchObject5solveEb");
extern "C" int countedSolve(void* self, bool update)
{
    static auto original = resolve<int (*)(void*, bool)>("_ZN8Sketcher12SketchObject5solveEb");
    OwnerScope scope(self);
    return original(self, update);
}

extern "C" int countedObjectSetup(void* self) asm("_ZN8Sketcher12SketchObject11setUpSketchEv");
extern "C" int countedObjectSetup(void* self)
{
    static auto original = resolve<int (*)(void*)>("_ZN8Sketcher12SketchObject11setUpSketchEv");
    OwnerScope scope(self);
    return original(self);
}

extern "C" int countedSetup(void*, const void*, const void*, int) asm(
    "_ZN8Sketcher6Sketch11setUpSketchERKSt6vectorIPN4Part8GeometryESaIS4_EERKS1_IPNS_"
    "10ConstraintESaISA_EEi"
);
extern "C" int countedSetup(void* self, const void* geometry, const void* constraints, int external)
{
    using Function = int (*)(void*, const void*, const void*, int);
    static auto original = resolve<Function>("_ZN8Sketcher6Sketch11setUpSketchERKSt6vectorIPN4Part8"
                                             "GeometryESaIS4_EERKS1_IPNS_10ConstraintESaISA_EEi");
    record(false);
    return original(self, geometry, constraints, external);
}
