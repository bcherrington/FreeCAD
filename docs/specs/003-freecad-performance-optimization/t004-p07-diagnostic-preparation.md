# T004.2 telemetry and P07 diagnostic preparation

Date: 2026-11-08. Status: implementation preparation; **not performance acceptance**.
Owner: local-developer. Task: T004.2 — Record comparable image conversion/lifecycle timings
and remaining bottlenecks under D001. T004.2 remains in progress.

The completed T004.1 work is committed in root `b045e76f49` and docs `6e6fac3`.
P01 and P07 remain unaccepted under the original D001 criteria. P07 remains the
retained candidate. No new FreeCAD benchmark observation was collected here.

## Shared host and process telemetry

`tests/src/Gui/benchmark_host_telemetry.py` wraps the entire future diagnostic
controller. It collects a 30-second host baseline once, then samples at one-second
intervals throughout the bounded controller execution. Its CLI fixes the
controller deadline at 1200 seconds after launch; existing per-worker 60-second
and stream limits still belong to the diagnostic controller. The sidecar has a
16 MiB limit, exclusive creation and mode 0600. A successful command exit alone
is not completion: require a final `session_finished` with `cleanup_complete`.

Samples contain monotonic/realtime timestamps, raw aggregate CPU counters,
load averages, available/swap/dirty memory, CPU/memory/I/O pressure, swap-page and
major-fault counters, per-device disk activity, kernel CPU frequency estimates,
governors, platform power profile and thermal summaries where available.
Unsupported sources are explicit, never interpreted as zero. Disk counters
exclude partitions but include stacked devices; do not sum across devices.
Frequency and temperature observations do not prove absence of throttling.

Process observations follow only the owned controller's descendants. An exact
executable device/inode match identifies the supplied FreeCAD executables;
unreadable executable identity is unknown. Counters include cumulative CPU time,
RSS and logical/physical I/O, with owned namespace PIDs/start identities for later
worker correlation. One-second sampling can miss short-lived processes, and
sampled RSS is not a substitute for the existing per-worker peak RSS measurement.
No unrelated process names, arguments, paths or identities are collected.

The wrapper requires an isolated, single-threaded Linux process with no existing
children. A temporary subreaper adopts orphan descendants, including workers
that create their own sessions. Cleanup discovers owned descendants, opens
pidfds, revalidates ancestry/start identity, signals through pidfds, escalates
TERM to KILL within fixed grace periods, reaps children and restores the previous
subreaper setting. Ownership failure aborts collection. Numeric PID/group signals
are not used. SIGTERM to the CLI follows this cleanup path.

Example structure for the future frozen controller (placeholder paths):

```sh
python3 tests/src/Gui/benchmark_host_telemetry.py \
  --output HOST_TRACE.jsonl \
  --native-executable CHANGE_BUILD/bin/FreeCAD \
  --native-executable BASELINE_BUILD/bin/FreeCAD \
  -- python3 FROZEN_DIAGNOSTIC_CONTROLLER.py
```

Freeze the exact wrapper source and executable identities with the controller.
The wrapper deliberately does not change acceptance thresholds or discard samples
based on observed load. Use timestamped intervals to examine correlation; load
correlation alone does not establish the cause of an individual slow observation.

## Verification and limits

Fifteen focused Python tests pass with ResourceWarning treated as an error:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -W error::ResourceWarning -m unittest \
  discover -s tests/src/Gui -p test_benchmark_host_telemetry.py
```

Coverage includes parent/start identity changes, pidfd revalidation, unknown
executable identity, missing counters, exact CPU/PSI interpretation, exclusive
private output, timeout, fast exit, cleanup failure terminal evidence and a
TERM-resistant separate-session descendant after its controller exits. The latter
must disappear from `/proc`, including reaping. Tests launch Python surrogates,
not FreeCAD.

`t004-telemetry-qualification.json` records the final 30-second baseline smoke,
source/raw-trace hashes, sample cadence, availability and observer wall/CPU cost.
Earlier smoke attempts are preserved under `build/spec003-t004-p07-diagnostic/`.
Their implementation versions preceded review corrections and are not the final
qualification. Collector overhead is measured, not assumed negligible. This
smoke qualifies telemetry mechanics; it cannot explain the previous BIM p95 or
predict the next FreeCAD result.

Independent measurement/privacy/operations review by `p01_instrumentation_review`
identified cleanup, fast-exit, unknown-identity, counter-unit and terminal-evidence
issues. These were corrected and covered by focused tests; final review disposition
is recorded with the qualification summary.

## Next execution boundary

The approved fixed 16-worker exploratory plan in `t004-execution-plan.md` remains
unchanged. Decode/conversion/native-frame/scheduling interval instrumentation,
exact shared eager/lazy source construction, isolated optimized builds, native
qualification and the fixed diagnostic have **not** run. P04/P05 conversion
comparisons also remain pending. Neither task completion nor a P01 exception is
implied by this preparation.

Historical pre-cleanup space check: existing qualified change/baseline trees occupied 3,418,275,840 and
3,418,423,296 bytes respectively; only 6,110,867,456 bytes were available. Independent copies
exceeded free space before compilation, caches and margin.

The user's subsequent cleanup/reuse request resolved this space blocker on
2026-11-08. [The retirement record](storage-retirement-20261108.json) records
16.54 GiB reclaimed from redundant compiler caches and obsolete T020 outputs.
All campaign evidence/source refs and the P01 seed pair survived verified. T020
native replay/live auditing now requires reconstruction and new qualification;
old results and receipts remain unchanged. Follow the
[storage runbook](../../runbooks/freecad-benchmark-storage.md) to reuse/requalify
builds and retire obsolete outputs without accumulating another full copy per
task. Never share writable hard-linked outputs. No benchmark host policy or user
application was changed.
