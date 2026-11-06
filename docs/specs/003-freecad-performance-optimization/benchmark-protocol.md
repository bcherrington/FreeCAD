---
title: Spec 003 predeclared comparative benchmark protocol
doc_type: spec
artifact_type: benchmark-protocol
status: approved-for-planning
owner: local-developer
last_reviewed: 2026-11-06
---

# Benchmark Protocol

## Decision And Scope

T001 resolves D001 as delegated technical selection under the user's instruction
“complete T001”, recorded by Codex on 2026-11-06 for local-developer. This is the
Phase 1 planning contract, not independent maintainer approval or runtime
acceptance. The numeric inventory in [benchmark-plan.json](benchmark-plan.json)
is authoritative; this document defines its endpoints and execution semantics.
[benchmark-runner.py](benchmark-runner.py) checks identities, emits the schedule
and collects one bounded comparison/fixture/context cell. T020/T004 deliver the
actual FreeCAD probes, qualification receipts and statistical assessment.

The immutable [T019 manifest](validation-manifest.md) remains the initial Debug
correctness candidate. Performance uses a separate, matched RelWithDebInfo
profile: `conda-linux-rel-with-deb-info`, normal `-O2 -g -DNDEBUG`, no sanitizer,
the same locked dependencies/package identities. Record effective compile/link
commands and flags, CPU/RAM, OS/kernel, power policy, competing load, display,
Qt/Coin/OCCT and renderer/driver identities. GUI uses native X11, a 1280×720
physical-pixel viewport, DPR 1, 8-sample MSAA and compact mode disabled, with
identical isolated preferences/workbench/camera/visibility and hashed config.
An unsupported profile is a blocker requiring a new pre-sample decision.
Sanitizer correctness results never serve as optimized performance samples.

## Revision And Build Contract

The intake is root `4e96936bb453c23bbfd29af711955b060cdf8b77`, docs
`f52a5c068c01b4f6755b5c62296f0ff760d0b74d`; its `src` tree is
`5d04131f65f14d62edf08d265bc470c6d701f7e9`, identical to T019. These are
composition anchors, not proof of a freshly built binary. Existing unproven
binaries remain ineligible. Exact full before/after optimization commits,
alternate historical duplicates and source-file allowlists are in the JSON. The performance-delta SHA-256 is over
the raw `git diff --binary <baseline> <change>` bytes, including its final newline.

| ID | Optimization | Baseline origin → optimized origin (abbreviated navigation only) | Owner |
| --- | --- | --- | --- |
| P01 | Buffered included-file restore | 770015798f → a93b54fb63 | T004.1 |
| P02 | Buffered included-file save | a93b54fb63 → eff963284d | T004.3 |
| P03 | Element-map parser | 0e40792710 → e093a1f66b | T004.3 |
| P04 | Direct 32-bit image conversion | 57a9b795e8 → f7227a6621 | T004.2 |
| P05 | Qt fallback image conversion | fa8659635e → 7252af06fb | T004.2 |
| P06 | Defer image loads until restored | e093a1f66b → cdf2a84f4b | T020 |
| P07 | Lazy original-resolution textures | cdf2a84f4b → c1722e4b2c | T020 |
| P08 | Restore-time tree status coalescing | c1722e4b2c → 00dd158de8 | T004.3 |

Historical trees contain known safety/lifecycle defects and are **source lineage
only**. After Phase 2 qualification, construct both arms from the same qualified
common base, varying only the individual performance algorithm from the listed
source delta. Keep all safety, calibration, parsing, identity, archive and signal
lifetime fixes identical. Review the complete delta, not just file membership;
exclude unrelated source/tests/docs differences. No production toggle is assumed.
If isolation would remove a safety guarantee or cannot be implemented faithfully,
stop that comparison and revise the protocol/route the optimization explicitly.
A combined application timing cannot establish each optimization's contribution.

Before any acceptance sample, freeze exact generated baseline/change commits,
common-base commit, performance-delta hash, clean worktree/submodule receipts,
fixture and worker source hashes, fresh binary/library hashes, build receipts,
configuration/context and correctness receipts in an execution JSON. The exact
future arm commits cannot exist before the safety corrections; the construction
rule and exact origin pairs are fixed now. T020/T004 must record this execution
freeze before sampling, without changing fixtures/thresholds after seeing results.
Use isolated worktrees/build directories with the same preset/environment. Normal
correctness builds use `pixi run build -j 2`; matched isolated performance builds
may use `pixi run --locked cmake --preset conda-linux-rel-with-deb-info` and
`pixi run --locked cmake --build <isolated-build-dir> -j 2`, recording the actual
commands and directory overrides. Never compare mixed build profiles.

## Fixture Identity And Coverage

Every comparison's required primary and control fixtures are enumerated in the
JSON. Run every listed cell. The eight public documents have exact repository
paths, sizes and SHA-256 identities. AssemblyExample is an additional correctness
fixture without a speedup claim. The private Gridfinity document is selected via
`FREECAD_BENCH_FILE`; keep its path, contents and screenshots out of published
artifacts. Its fixed hash/size and three embedded PNG payload hashes/dimensions
are recorded. Select payloads by SHA-256 with a 32 MiB maximum entry read, never
extract arbitrary ZIP paths. Verify the enclosing archive before reading payloads.

P05 includes both declared sizes and **each** indexed grayscale, grayscale-alpha,
premultiplied, 16-bit-gray and SVG-generated case. Recipes, dimensions and recipe
hashes are fixed; T004.2 materializes deterministic asymmetric fixtures, records
generator/output hashes, actual QImage format/stride/color space, and freezes
each case as a distinct context before sampling. Until then these are explicitly
unmaterialized and ineligible for results. Missing private data or generated
fixtures means blocked coverage; do not silently substitute or drop a cell.

For P03, count restored map records and prove the primary fixtures exercise the
parser. If either has no meaningful map payload, record the coverage failure and
approve a revised fixture identity before measurement. All document checks include
object/shape/subelement reference identity; matching counts alone is insufficient.
Image qualification checks dimensions, component/channel/alpha values, stride,
orientation and calibrated XSize/YSize/pixel spacing before and after visibility
changes and a save/reopen. Tree qualification checks final status and scheduling
on success, abort, nested restore and receiver destruction. Archive qualification
includes byte hashes, missing/corrupt entries, surfaced failures and subsequent
entry behavior. These checks run outside primitive timers, on both arms.

## Endpoints And Resources

Use monotonic nanosecond clocks, converted to milliseconds without integer
truncation. Process startup, fixture discovery, setup and correctness validation
are outside the primitive timer. Both arms use identical probes and timer scope.
Instrument current source/test harnesses as needed; removed trace switches and
historical transient scripts are not dependencies.

| Metric | Start → finish |
| --- | --- |
| restore_payload_ms | Included-file hook begins → last byte restored, stream error outcome checked; sum all hook intervals for the document |
| save_payload_ms | Included-file save hook begins → final payload write checked; sum hooks; report total save separately |
| map_restore_ms | Element-map restore parser entry → parsed map complete; sum all maps, record token/record count |
| convert_ms | Already decoded QImage enters conversion → complete Coin image storage available; decoding excluded and separately reported |
| document_open_ms | Command-mode open call → document restore/recompute complete and errors checked |
| document_save_ms | Save call → archive close/finalization and errors checked; save to an isolated temporary copy |
| gui_open_frame_ms | GUI open request → completed correct frame in the active native view after restore/tree completion and required visible textures materialize |
| first_show_frame_ms | First visibility-on request for a restored hidden image → completed correct frame with full-resolution texture |
| repeat_show_frame_ms | Subsequent hide/show request with materialized texture → completed correct frame; one fixed repeat per fresh worker |
| open_plus_first_show_ms | gui_open_frame_ms + first_show_frame_ms within the same worker; fixed hidden-image scenario |
| failed_pre_restore_loads | Attempts to access unavailable image payloads during restore; distinguish attempted/failed/successful loads |
| restore_status_schedules | Actual queued tree status updates during restore plus the final required completion flush |

T020 supplies a qualified native render-completion probe, including reference
frame/pixel checks, visible/pending/materialized counts and restore/tree-finished
signals. A return from open, one `processEvents`, an offscreen screenshot or a
queued paint does not prove a completed correct native frame. Report the probe's
clock/event boundary and limitations; no monitor scanout claim. Run fixed
all-visible and initially-hidden image contexts. P07 primary runs the hidden
scenario with all declared metrics; its additional all-visible scenario requires
open non-regression and resource gates, using the fixed `primary_context_metrics` inventory in the JSON. It cannot replace the required hidden scenario.
For controls use the explicit `control_metrics` inventory in the JSON; document
fixtures without images retain open/resource gates and no first-show claim.
All first-show budgets remain mandatory on the private primary image fixture.

Record process high-water RSS from the OS, maximum simultaneously live decoded
QImage bytes, and maximum simultaneously live Coin image bytes over open/first
show/repeat/save as applicable. Count ownership/shared storage once and record
component count. Record samples at each lifecycle boundary and allocator peaks
so transient decode/convert overlap is visible. GPU residency is unmeasured
unless separately instrumented with a driver-qualified probe. Peak RSS is not a
GPU or decoded-storage estimate. Extend the worker metrics with decoded bytes;
all image contexts must pass the same 1 MiB decoded-storage increase budget as
Coin storage. P05 must expose accidental 1/2-component → RGBA storage growth.

## Sampling And Pass Rules

A cell is one comparison, fixture, format/visibility context and frozen arm pair.
Use 24 fresh process pairs: 1 diagnostic first-request pair, 3 discarded warm-up
pairs and 20 measured pairs. Alternate AB/BA by pair index, A baseline/B change.
Filesystem caches warm naturally; do not drop host caches or claim controlled
cold-cache performance. Record the first-request diagnostics separately. Never
include warm-ups in acceptance. Fix fixtures/config before starting; no retries,
post-hoc outlier removal, best-of selection or extra samples to rescue a result.

For each timing, report arm median, p95 (linear interpolation at `(n-1)*0.95`),
IQR and paired change/baseline ratios. A zero baseline makes a ratio improvement
claim inapplicable, requiring a coverage/protocol decision rather than division
by zero. Primary speedups require median paired ratio ≤0.95, median paired time
saved ≥ the metric's JSON floor (0.5/1/2 ms), and upper 95% percentile bootstrap
bound <1. Resample the 20 pairs jointly, 20,000 times, seed 3001, statistic median
of paired ratios; use the same interpolation rule at 0.025/0.975. Each required
primary fixture/format/context must pass independently; no pooled average hides
a failed cell. Controls require non-regression, not a minimum speedup.

Timing non-regression permits median increase ≤max(5% baseline median, 2 ms),
and p95 increase ≤max(10% baseline p95, 5 ms). P06 primary requires baseline
failed-pre-restore-load count positive and change count zero on every sample.
P08 primary requires positive baseline schedules and ≥50% median paired count
reduction, while retaining the required final flush. P07 first-show median/p95
budgets are 500/1000 ms and repeat-show 50/100 ms, with open-plus-first-show
non-regression; faster open alone cannot justify moving a larger wait to show.

For resources compare the maximum across measured observations. Peak RSS increase
must be ≤max(10% baseline peak, 16 MiB); Coin and decoded image storage increase
must each be ≤1 MiB. All normative correctness checks must pass on every run.
Per-arm timing IQR/median >0.10 makes that cell inconclusive even if a ratio gate
passes. Instrument precision, hardware noise or a failed budget never grants an
automatic waiver. Record failure/inconclusive results; fix causes or approve a
new dated protocol and collect new data. Preserve the original failed evidence.
An optimization with no demonstrated benefit must be reconsidered/routed before
T004/T010 acceptance. T001 selects these local investigation budgets; it does
not promise universal desktop latency or establish upstream policy.

## Runnable Collection Contract

From the repository root, inspect the plan and controller without opening FreeCAD:

```sh
python3 docs/specs/003-freecad-performance-optimization/benchmark-runner.py --check --self-test
python3 docs/specs/003-freecad-performance-optimization/benchmark-runner.py --schedule
```

After Phase 2 and probe qualification, run each frozen execution cell:

```sh
python3 docs/specs/003-freecad-performance-optimization/benchmark-runner.py --run /tmp/spec003-execution.json --out /tmp/spec003-observations.jsonl
```

The runner refuses incomplete source gates, mismatched protocol/fixture/binary
hashes and missing receipts. It checks exact common-base ancestry and the
complete arm diff against the source allowlist/hash. The document and collector
hashes are frozen alongside the JSON hash. Each arm command is an argv array, run without a
shell in a new process group, bounded to 60 seconds and 256 KiB combined output;
a cell has a 1200-second bound. A timeout, crash, invalid/missing record or wrong
correctness identity aborts the cell. Partial JSONL is retained with no completion
marker and is ineligible for acceptance. Commands and receipts are trusted local
inputs reviewed by the task owner, not a sandbox or independent attestation.

Execution JSON fields are listed in `execution_freeze_required`. `context` is
a descriptive label (P07 primary uses one of its two exact context IDs); add `context_path`, `qualification_receipt_path` and their
SHA-256 fields, `fixture_path`, and, per arm, `commit`, `command`, `binary_path`,
`binary_sha256`, `build_receipt_path`, `build_receipt_sha256`. The qualification
receipt must attest the common base/delta isolation, all applicable Phase 2
checks, fixture generation, exact worker/probe source hashes and matching
normative state digest. Review receipt contents; existence/hashes alone do not
prove semantics. Generated output hashes must match the materialization receipt.
Private paths stay in local execution files; sanitize shared reports.

Each worker writes exactly one JSON object to `SPEC003_RESULT_FILE` with
`comparison_id`, `fixture_id`, `fixture_sha256`, `context_sha256`,
`normative_state_sha256`, actual `commit`, `correctness_passed: true` and a
`metrics` object of finite nonnegative numbers using the declared metric names.
The runner provides `SPEC003_ARM`, `SPEC003_PHASE`, `SPEC003_PAIR`. It records
log hashes and all observations; it does not certify frame semantics or compute
acceptance. T020/T004 implement and validate the worker and statistical assessor
against these definitions, then write `performance-results.md`/`.json` including
raw sample/receipt hashes and explicit per-cell pass/fail/inconclusive findings.
No worker, qualified arm, generated fixture or application performance result
is claimed by T001. T010 reconciles final PR composition and evidence invalidation.
