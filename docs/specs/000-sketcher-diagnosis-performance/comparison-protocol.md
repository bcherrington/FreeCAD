---
title: T005 frozen comparison execution protocol
doc_type: spec
artifact_type: verification
status: archived
owner: local-developer
last_reviewed: 2026-11-05
---

# T005 Original Collection Protocol (Historical)

This records the original strict protocol actually used to collect the retained
samples. The user subsequently approved indicative timing evidence in
[timing-evidence-decision.md](timing-evidence-decision.md). The confidence/budget
rules below are historical, not current completion requirements. Do not alter the
original results or imply those former gates passed.

Compare baseline `6a17625f55` (no phase tracing), instrumentation-only `9bc0eb91ff`
(tracing disabled, two setups) and candidate `77ca0b6d94` (tracing disabled, one
ordinary setup). All use the configured Clang 21.1.8 Release `-O3 -DNDEBUG`
compile/link commands, unchanged dependencies and generated wrappers. The two
historical Sketcher modules are reconstructed from detached source worktrees in
isolated object directories, with two compiler jobs. All 36 baseline translation units (including generated MOC) are built against
the corresponding headers; tracing adds one translation unit, for 37. Candidate uses the existing Pixi Release build.
Exact commands, module hashes, source identities and unchanged shared library
hashes are retained privately. No working source revision is switched.

Use the same native counter and frozen private model/hash in [workloads.json](workloads.json).
All nine runnable workloads retain T001's mutation/recompute/open boundaries,
precursor creation, undo/discard/restoration and process/cache definitions. Do not
substitute a different historical-symmetry workload. Warm-loaded and persistent
cases use one initialized process per configuration and case; fresh loads use a
new process for each sample, with startup outside the open timer. Existing Log
sinks and detailed tracing are disabled consistently. Validate native vectors on
every sample against the manifest and candidate expectations. Compare each
matched result's state across all three configurations. GUI/state checks and
counter calibration run separately from acceptance measurements.

Each case/batch has two untimed warmups per configuration followed by ten matched
triples. Even pair indices run baseline, instrumentation, candidate; odd indices
reverse that order. Thus each two-configuration comparison alternates AB then BA.
The controller waits until validation, commit/undo, restoration and discard finish
before starting the next configuration. One timed operation is active at a time.
A failed/mismatched run is retained and stopped; it cannot contribute acceptance.
A replacement starts a fresh case under the same ordering/reset contract.

The host uses powersave frequency policy with turbo enabled and other applications
running. To reduce migration noise observed in T001's wide A/A intervals, the
controller and all workers have the same task-local affinity to physical cores
4 and 5 (logical CPUs 4,5). No host governor, turbo, thread-launch policy or other
process's affinity is changed. Record affinity, machine identity and load averages.
Builds and GUI checks finish before collecting acceptance data. This controls
migration; it does not establish an idle host or eliminate thermal/background noise.

Record baseline/candidate latency and baseline/instrumentation tracing-off overhead,
plus instrumentation/candidate attribution. Report medians, MAD and complete-pair
10,000-resample bootstrap percentile 95% intervals with seed 576001. Recompute
allowances from each resample's baseline median. Percentiles use the sorted sample
at floor(p * (N-1)). Do not independently resample configurations.

After the first batch, repeat only cases needing an inconclusive regression,
overhead or still-required speedup decision, with at most two additional batches.
Retain every batch and assess the pooled complete pairs as well as each batch.
Never drop an unfavourable batch or increase the sample bound. A remaining overlap
is inconclusive and cannot pass acceptance. A failed budget receives investigation
and explicit disposition. Acceptance targets are unchanged: at least one eligible
diagnosis workload's speedup interval must lie wholly at or above 10%; all selected
regression and tracing-off-overhead intervals must satisfy their respective gates.
