#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Bounded Linux host/owned-process telemetry; never an acceptance assessor.

Wrap the entire diagnostic controller, not each worker, so the 30-second host
baseline happens once. No command arguments, process names or paths are logged.
Native processes are identified by executable device/inode, not window focus.
"""

import argparse
import ctypes
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import time

SCHEMA = "freecad-benchmark-host-telemetry/v1"


class OwnershipError(ValueError):
    """Ownership failure must abort, rather than become a missing observation."""


def process_stat(text):
    # comm can contain spaces and parentheses. Remaining fields start at field 3.
    fields = text[text.rindex(")") + 2 :].split()
    return {
        "parent": int(fields[1]),
        "start_ticks": int(fields[19]),
        "cpu_ticks": int(fields[11]) + int(fields[12]),
        "rss_pages": int(fields[21]),
    }


def pressure(text):
    return {
        fields[0]: {
            key: int(value) if key == "total" else float(value)
            for key, value in (v.split("=") for v in fields[1:])
        }
        for fields in (line.split() for line in text.splitlines())
    }


class OwnedSession:
    """Linux subreaper plus pidfds: orphan/session changes never widen authority.

    Require no existing children or other threads. All children subsequently
    adopted by this standalone wrapper then originate from its sole command.
    Never use numeric PID/group signals, even after a controller has exited.
    """

    def __init__(self, proc=Path("/proc")):
        self.proc = proc
        self.handles = {}
        self.previous = ctypes.c_int()
        self.libc = ctypes.CDLL(None, use_errno=True)

    def children(self, pid):
        result = set()
        try:
            for task in (self.proc / str(pid) / "task").iterdir():
                try:
                    result.update(int(v) for v in (task / "children").read_text().split())
                except FileNotFoundError:
                    pass
        except FileNotFoundError:
            pass
        return result

    def start(self):
        if not hasattr(os, "pidfd_open") or not hasattr(signal, "pidfd_send_signal"):
            raise RuntimeError("Linux pidfd support is required")
        if len(list((self.proc / str(os.getpid()) / "task").iterdir())) != 1 or self.children(
            os.getpid()
        ):
            raise OwnershipError("Telemetry requires an isolated process with no existing children")
        if (
            self.libc.prctl(37, ctypes.byref(self.previous), 0, 0, 0) != 0
            or self.libc.prctl(36, 1, 0, 0, 0) != 0
        ):
            raise OSError("Cannot enable owned-child subreaper")

    def remember(self, pid, parent):
        before = process_stat((self.proc / str(pid) / "stat").read_text())
        if before["parent"] != parent:
            raise OwnershipError("Cleanup child ancestry changed")
        key = pid, before["start_ticks"]
        if key not in self.handles:
            handle = os.pidfd_open(pid)
            try:
                after = process_stat((self.proc / str(pid) / "stat").read_text())
                if after["start_ticks"] != before["start_ticks"] or after["parent"] != parent:
                    raise OwnershipError("Cleanup process identity changed")
            except BaseException:
                os.close(handle)
                raise
            self.handles[key] = handle

    def discover(self):
        pending = [(pid, os.getpid()) for pid in self.children(os.getpid())]
        seen = set()
        while pending:
            pid, parent = pending.pop()
            if pid in seen:
                continue
            seen.add(pid)
            try:
                self.remember(pid, parent)
                pending.extend((p, pid) for p in self.children(pid))
            except (FileNotFoundError, ProcessLookupError, OwnershipError):
                # Exit/reparent races are revisited via direct adopted children.
                continue

    def cleanup(self):
        reaped = {}
        for sig, grace in ((signal.SIGTERM, 2), (signal.SIGKILL, 2)):
            sent = set()
            deadline = time.monotonic() + grace
            while True:
                self.discover()
                for key, handle in self.handles.items():
                    if key not in sent:
                        try:
                            signal.pidfd_send_signal(handle, sig)
                        except ProcessLookupError:
                            pass
                        sent.add(key)
                while True:
                    try:
                        pid, status = os.waitpid(-1, os.WNOHANG)
                        if pid == 0:
                            break
                        reaped[pid] = os.waitstatus_to_exitcode(status)
                    except ChildProcessError:
                        return reaped
                if time.monotonic() >= deadline:
                    break
                time.sleep(0.02)
        raise RuntimeError("Owned subprocesses did not stop within cleanup bounds")

    def close(self):
        error = None
        try:
            for handle in self.handles.values():
                try:
                    os.close(handle)
                except OSError as failure:
                    error = failure
        finally:
            if self.libc.prctl(36, self.previous.value, 0, 0, 0) != 0:
                raise OSError("Cannot restore child-subreaper state")
        if error is not None:
            raise error


class Collector:
    def __init__(self, native_executables=(), proc=Path("/proc"), sys=Path("/sys")):
        self.proc = proc
        self.sys = sys
        self.native = {(p.stat().st_dev, p.stat().st_ino) for p in native_executables}
        self.root = None
        self.serials = {}
        self.ticks = os.sysconf("SC_CLK_TCK")
        self.page = os.sysconf("SC_PAGE_SIZE")

    def attach(self, child):
        try:
            stat = process_stat((self.proc / str(child.pid) / "stat").read_text())
        except FileNotFoundError:
            if child.poll() is not None:
                return False
            raise
        if stat["parent"] != os.getpid():
            raise OwnershipError("Telemetry root must be a directly owned child")
        self.root = child, stat["start_ticks"]
        return True

    def snapshot(self):
        started = time.monotonic_ns()
        cpu_started = time.process_time_ns()
        unavailable = []

        def optional(label, read):
            try:
                return read()
            except OwnershipError:
                raise
            except (OSError, ValueError, KeyError, IndexError):
                unavailable.append(label)
                return None

        def read_mem():
            values = {}
            for line in (self.proc / "meminfo").read_text().splitlines():
                name, value = line.split(":", 1)
                if name in {
                    "MemTotal",
                    "MemAvailable",
                    "SwapTotal",
                    "SwapFree",
                    "Dirty",
                    "Writeback",
                }:
                    values[name + "_bytes"] = int(value.split()[0]) * 1024
            if "MemAvailable_bytes" not in values:
                raise ValueError("No available memory counter")
            return values

        def read_cpu():
            fields = (self.proc / "stat").read_text().splitlines()[0].split()
            if fields[0] != "cpu" or len(fields) < 9:
                raise ValueError("No aggregate CPU counter")
            # guest/guest_nice are already included in user/nice; omit them.
            return dict(
                zip(
                    ("user", "nice", "system", "idle", "iowait", "irq", "softirq", "steal"),
                    map(int, fields[1:9]),
                )
            )

        def read_disk():
            devices = []
            for line in (self.proc / "diskstats").read_text().splitlines():
                fields = line.split()
                if len(fields) < 14:
                    continue
                device = f"{int(fields[0])}:{int(fields[1])}"
                entry = self.sys / "dev/block" / device
                if not entry.exists() or (entry / "partition").exists():
                    continue
                devices.append(
                    {
                        "device_number": device,
                        "read_sectors_512": int(fields[5]),
                        "write_sectors_512": int(fields[9]),
                        "in_flight": int(fields[11]),
                        "io_ms": int(fields[12]),
                        "weighted_io_ms": int(fields[13]),
                    }
                )
            return devices

        def summarize(paths, divisor=1):
            values = []
            for path in paths:
                value = optional("sensor_read", lambda: float(path.read_text()) / divisor)
                if value is not None and math.isfinite(value):
                    values.append(value)
            return (
                {
                    "count": len(values),
                    "min": min(values),
                    "max": max(values),
                    "mean": sum(values) / len(values),
                }
                if values
                else None
            )

        result = {
            "event": "sample",
            "monotonic_ns": started,
            "realtime_ns": time.time_ns(),
            "cpu_ticks": optional("cpu", read_cpu),
            "load_average": optional(
                "load", lambda: [float(v) for v in (self.proc / "loadavg").read_text().split()[:3]]
            ),
            "online_cpus": optional(
                "online_cpus", lambda: (self.sys / "devices/system/cpu/online").read_text().strip()
            ),
            "power_profile": optional(
                "power_profile",
                lambda: (self.sys / "firmware/acpi/platform_profile").read_text().strip(),
            ),
            "frequency_governors": optional(
                "frequency_governors",
                lambda: sorted(
                    {
                        p.read_text().strip()
                        for p in self.sys.glob(
                            "devices/system/cpu/cpufreq/policy*/scaling_governor"
                        )
                    }
                )
                or None,
            ),
            "memory": optional("memory", read_mem),
            "pressure": {
                kind: optional(
                    "pressure_" + kind,
                    lambda k=kind: pressure((self.proc / "pressure" / k).read_text()),
                )
                for kind in ("cpu", "memory", "io")
            },
            "vm_counters": optional(
                "vmstat",
                lambda: {
                    k: int(v)
                    for k, v in (
                        line.split() for line in (self.proc / "vmstat").read_text().splitlines()
                    )
                    if k in {"pswpin", "pswpout", "pgmajfault"}
                },
            ),
            "disk_devices": optional("disk", read_disk),
            "frequency_khz": summarize(
                self.sys.glob("devices/system/cpu/cpufreq/policy*/scaling_cur_freq")
            ),
            "thermal_celsius": summarize(self.sys.glob("class/thermal/thermal_zone*/temp"), 1000),
            "owned_processes": optional("owned_processes", self.processes),
        }
        if result["frequency_khz"] is None:
            unavailable.append("frequency")
        if result["thermal_celsius"] is None:
            unavailable.append("thermal")
        result["unavailable"] = sorted(set(unavailable))
        result["collection_duration_ns"] = time.monotonic_ns() - started
        result["collection_cpu_ns"] = time.process_time_ns() - cpu_started
        return result

    def processes(self):
        if self.root is None or self.root[0].poll() is not None:
            return []
        child, root_start = self.root
        pending = [(child.pid, None)]
        seen = set()
        result = []
        while pending:
            pid, expected_parent = pending.pop()
            if pid in seen:
                continue
            if len(seen) >= 256:
                raise OwnershipError("Owned process inventory bound exceeded")
            seen.add(pid)
            directory = self.proc / str(pid)
            try:
                before = process_stat((directory / "stat").read_text())
                if (pid == child.pid and before["start_ticks"] != root_start) or (
                    expected_parent is not None and before["parent"] != expected_parent
                ):
                    raise OwnershipError("Owned process identity changed")
                try:
                    executable = (directory / "exe").stat()
                    native = (executable.st_dev, executable.st_ino) in self.native
                except OSError:
                    native = None
                try:
                    namespace_pids = next(
                        [int(v) for v in line.split()[1:]]
                        for line in (directory / "status").read_text().splitlines()
                        if line.startswith("NSpid:")
                    )
                except (OSError, StopIteration):
                    namespace_pids = None
                try:
                    io = {
                        k.rstrip(":"): int(v)
                        for k, v in (
                            line.split() for line in (directory / "io").read_text().splitlines()
                        )
                        if k in {"read_bytes:", "write_bytes:", "rchar:", "wchar:"}
                    }
                except OSError:
                    io = None
                descendants = []
                for task in (directory / "task").iterdir():
                    descendants.extend(int(v) for v in (task / "children").read_text().split())
                after = process_stat((directory / "stat").read_text())
                if (
                    after["start_ticks"] != before["start_ticks"]
                    or after["parent"] != before["parent"]
                ):
                    raise OwnershipError("Owned process identity changed")
            except FileNotFoundError:
                # An owned child may exit between reads. No final-counter claim.
                continue
            identity = pid, before["start_ticks"]
            serial = self.serials.setdefault(identity, len(self.serials) + 1)
            result.append(
                {
                    "process_serial": serial,
                    "namespace_pids": namespace_pids,
                    "start_ticks": before["start_ticks"],
                    "native_freecad": native,
                    "cpu_time_ns": before["cpu_ticks"] * 1_000_000_000 // self.ticks,
                    "rss_bytes": before["rss_pages"] * self.page,
                    "io": io,
                }
            )
            pending.extend((descendant, pid) for descendant in descendants)
        return sorted(result, key=lambda row: row["process_serial"])


def run(command, destination, native_executables=(), baseline_seconds=30, timeout_seconds=1200):
    """Exclusive bounded sidecar; isolated single-threaded controller process only."""
    if not command or not 0 <= baseline_seconds <= 30 or not 0 < timeout_seconds <= 1200:
        raise ValueError("Invalid bounded telemetry session")
    collector = Collector(native_executables)
    child = None
    written = 0
    with destination.open("xb") as stream:
        os.fchmod(stream.fileno(), 0o600)

        def emit(row):
            nonlocal written
            encoded = (json.dumps(row, sort_keys=True, allow_nan=False) + "\n").encode()
            written += len(encoded)
            if written > 16 * 1024 * 1024:
                raise ValueError("Telemetry output bound exceeded")
            stream.write(encoded)
            stream.flush()

        emit(
            {
                "event": "session",
                "schema": SCHEMA,
                "acceptance_eligible": False,
                "counter_units": {
                    "cpu_ticks": "clock ticks",
                    "pressure_total": "microseconds",
                    "pressure_avg": "percent",
                    "pswpin": "pages",
                    "pswpout": "pages",
                    "pgmajfault": "events",
                },
                "interval_seconds": 1,
                "baseline_seconds": baseline_seconds,
                "timeout_seconds": timeout_seconds,
                "clock_ticks_per_second": collector.ticks,
                "page_bytes": collector.page,
                "native_executable_count": len(collector.native),
                "disk_policy": "whole devices, including stacked devices; never sum across devices",
                "process_policy": "owned subtree; exited and sub-second processes may be absent",
                "frequency_policy": "kernel scaling_cur_freq; not a guarantee of instantaneous hardware frequency",
                "clock": time.get_clock_info("monotonic").implementation,
            }
        )
        ownership = OwnedSession()
        ownership_started = False
        try:
            ownership.start()
            ownership_started = True
            start = time.monotonic()
            next_sample = start
            launch_at = start + baseline_seconds
            deadline = launch_at + timeout_seconds
            while True:
                now = time.monotonic()
                if now >= deadline:
                    raise TimeoutError("Telemetry session exceeded its deadline")
                if child is None and now >= launch_at:
                    child = subprocess.Popen(
                        command, stdin=subprocess.DEVNULL, start_new_session=True
                    )
                    attached = collector.attach(child)
                    emit(
                        {
                            "event": "command_started",
                            "monotonic_ns": time.monotonic_ns(),
                            "root_telemetry_available": attached,
                        }
                    )
                emit(collector.snapshot())
                if child is not None and child.poll() is not None:
                    emit(
                        {
                            "event": "command_exited",
                            "monotonic_ns": time.monotonic_ns(),
                            "returncode": child.returncode,
                        }
                    )
                    break
                next_sample += 1
                # Do not burst catch-up samples after suspension or slow reads.
                if next_sample < time.monotonic():
                    emit({"event": "sampling_gap", "monotonic_ns": time.monotonic_ns()})
                    next_sample = time.monotonic() + 1
                time.sleep(max(0, min(next_sample, deadline) - time.monotonic()))
        except BaseException:
            try:
                emit({"event": "session_aborted", "monotonic_ns": time.monotonic_ns()})
            except (OSError, ValueError):
                pass  # Preserve the original failure when the sidecar is full.
            raise
        finally:
            try:
                try:
                    if child is not None:
                        reaped = ownership.cleanup()
                        if child.returncode is None:
                            child.returncode = reaped.get(child.pid)
                            child.wait(timeout=0)
                finally:
                    if ownership_started:
                        ownership.close()
            except BaseException:
                try:
                    emit({"event": "cleanup_failed", "monotonic_ns": time.monotonic_ns()})
                except (OSError, ValueError):
                    pass
                raise
        emit(
            {
                "event": "session_finished",
                "monotonic_ns": time.monotonic_ns(),
                "returncode": child.returncode,
                "cleanup_complete": True,
            }
        )
        return child.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--native-executable", type=Path, action="append", required=True)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command

    def terminate(signum, _frame):
        raise SystemExit(128 + signum)

    signal.signal(signal.SIGTERM, terminate)
    return run(command, args.output, args.native_executable)


if __name__ == "__main__":
    raise SystemExit(main())
