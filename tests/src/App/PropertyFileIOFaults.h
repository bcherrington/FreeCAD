// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Linux test-only, thread-local faults for one exact disposable output file.
// The linked shim delegates every other call unchanged to libc.
extern "C" void fcIncludedFileFault(const char* path, int mode);
extern "C" int fcIncludedFileFaultHits();
