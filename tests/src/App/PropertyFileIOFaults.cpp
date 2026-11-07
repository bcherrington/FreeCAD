// SPDX-License-Identifier: LGPL-2.1-or-later
// Test-only Linux libc interposition; never linked into FreeCAD production targets.
#include "PropertyFileIOFaults.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <limits.h>
#include <sys/uio.h>
#include <unistd.h>

namespace
{
struct Fault
{
    char path[PATH_MAX] {};
    int mode {};
    int hits {};
};
thread_local Fault fault;

bool matches(int fd, int mode)
{
    if (fault.mode != mode || fd < 0) {
        return false;
    }
    const int savedErrno = errno;
    char link[64] {};
    char path[PATH_MAX] {};
    std::snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
    const auto length = ::readlink(link, path, sizeof(path) - 1);
    const bool match = length >= 0 && std::strcmp(path, fault.path) == 0;
    errno = savedErrno;
    if (match) {
        ++fault.hits;
    }
    return match;
}
}  // namespace

extern "C" void fcIncludedFileFault(const char* path, int mode)
{
    fault.mode = 0;
    fault.hits = 0;
    std::snprintf(fault.path, sizeof(fault.path), "%s", path ? path : "");
    fault.mode = mode;
}

extern "C" int fcIncludedFileFaultHits()
{
    return fault.hits;
}

extern "C" ssize_t write(int fd, const void* data, size_t size)
{
    static auto real = reinterpret_cast<decltype(&write)>(dlsym(RTLD_NEXT, "write"));
    if (size && matches(fd, 1)) {
        errno = EIO;
        return -1;
    }
    return real(fd, data, size);
}

extern "C" ssize_t writev(int fd, const struct iovec* data, int count)
{
    static auto real = reinterpret_cast<decltype(&writev)>(dlsym(RTLD_NEXT, "writev"));
    if (count && matches(fd, 1)) {
        errno = EIO;
        return -1;
    }
    return real(fd, data, count);
}

extern "C" size_t fwrite(const void* data, size_t size, size_t count, FILE* file)
{
    static auto real = reinterpret_cast<decltype(&fwrite)>(dlsym(RTLD_NEXT, "fwrite"));
    if (size && count && matches(fileno(file), 1)) {
        errno = EIO;
        return 0;
    }
    return real(data, size, count, file);
}

extern "C" int fclose(FILE* file)
{
    static auto real = reinterpret_cast<decltype(&fclose)>(dlsym(RTLD_NEXT, "fclose"));
    const bool fail = matches(fileno(file), 2);
    const int result = real(file);  // Always release the actual descriptor.
    if (fail) {
        errno = EIO;
        return EOF;
    }
    return result;
}
