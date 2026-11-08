---
title: T020 deferred image measurement results
doc_type: spec
artifact_type: verification
status: in-progress
owner: local-developer
last_reviewed: 2026-11-07
---

# T020 Deferred Image Measurement Results

This is the historical freeze-1 report. Current corrected-probe selection is
recorded in [the recovery plan](t020-recovery-2-plan.md); the outcomes below remain
unchanged and do not describe completion of the new campaign.

**T020 requires review/decision under D004. P07 is not accepted.** The required hidden-image
primary aborted; all-visible open and the PartDesign control are inconclusive.
No performance improvement or GPU residency is claimed. P06 remains independently
non-identifiable under the qualified generalized lifecycle and is excluded as
subsumed, as recorded in [the measurement review](t020-measurement-review.md).

[Machine-readable results](performance-results.json) bind the source, qualification,
execution, raw observations and assessments. Private fixture locators, document
contents and screenshots are excluded. The checksum-pinned original is unchanged.

## Fixed Collection And Disposition

Each complete cell contains 24 alternating fresh-process pairs: one diagnostic,
three warmup and twenty measured pairs. No failed worker was retried and no
outlier was removed. Historical D001 plan/protocol/collector bytes are unchanged.
Both pre/post-collection audits passed all 6,055 frozen hashes.

| GUI case | Baseline median / p95 (ms) | Lazy median / p95 (ms) | IQR/median baseline / lazy | Disposition |
| --- | --- | --- | --- | --- |
| Hidden first/repeat show | Not assessed | Not assessed | Not assessed | Aborted; ineligible |
| All-visible open | 3758.41 / 7588.37 | 3892.84 / 7793.63 | 0.664 / 0.758 | inconclusive |
| PartDesignExample open | 294.44 / 524.48 | 296.17 / 537.83 | 0.636 / 0.613 | inconclusive |
| TestVRMLTextures open | 602.70 / 648.30 | 606.33 / 686.27 | 0.030 / 0.067 | pass |

The three complete GUI cells pass their numeric timing non-regression limits
and all process/decoded/Coin resource limits. Passing numeric timing limits does
not override the 0.10 dispersion limit. Completed observations all pass the frozen
model/reference, frame, renderer, configuration, namespace and runtime-module checks.

The hidden cell contains only 24 completed observations and no completion marker.
Its next worker failed at pair 12, baseline arm: the ninth measured pair. Those
partial observations are preserved locally, hashed and never assessed or combined
with another run. Failure classification is **unclassified worker/process failure**.
The original collector discarded failed-worker output/status and its temporary
result; available kernel/coredump records show no relevant event in the failure
window. This does not prove an external cause or a FreeCAD defect.

## Command Restore Observations

These complete cells are descriptive auxiliary measurements. They cannot replace
the missing GUI primary or independently establish a P07 speedup.

| Fixture | Baseline median / p95 (ms) | Lazy median / p95 (ms) |
| --- | --- | --- |
| Pinned Gridfinity | 872.18 / 1308.03 | 863.96 / 1269.58 |
| PartDesignExample | 188.66 / 275.70 | 188.68 / 304.34 |
| TestVRMLTextures | 1.67 / 1.79 | 1.63 / 1.72 |

The endpoint is command-only `App.openDocument` return with restore/recompute
complete. Model/reference/payload checks run outside its timer. Gridfinity imports
FreeCAD's console-safe GUI Python bootstrap; `App.GuiUp` remains false. Its module
is verified against the same arm's hashed build manifest.

## Deferred Work And Storage

Qualification counters establish the workload boundary, without using the invalid
hidden timing sample: eager open materializes three hidden textures; lazy open
keeps all three pending. First show materializes only the selected texture; repeat
show does not reload it. Both arms record zero premature restore loads. This
shows deferred first-show work and avoids two unshown loads during that scenario;
it does not establish accepted latency or memory improvement.

The direct/fallback resource qualification runs each of eighteen generated format
rows in a fresh process in each arm. The table shows the lazy arm's 4096-square
absolute observations, with two independent Coin copies live at the copy-overlap
boundary. It is not the P05/T004.2 before/after comparison.

| Generated format | Decoded peak (MiB) | Coin peak (MiB) | Process peak RSS (MiB) |
| --- | --- | --- | --- |
| direct-rgb32 | 64.00 | 128.00 | 281.27 |
| direct-argb32 | 64.00 | 128.00 | 281.27 |
| direct-rgbx | 64.00 | 128.00 | 281.14 |
| svg-derived | 128.00 | 128.00 | 282.95 |
| direct-rgba | 64.00 | 128.00 | 280.90 |
| one-component-gray | 80.00 | 128.00 | 233.61 |
| indexed-gray-alpha | 80.00 | 128.00 | 233.16 |
| premultiplied | 128.00 | 128.00 | 281.61 |
| gray16 | 96.00 | 128.00 | 249.55 |

Decoded/Coin counters count unique image-plane-owned logical pixel storage,
including conversion and temporary/persistent copy overlap. One/two-component
fallback rows expose their source-plus-RGBA expansion. RSS is the independent
Linux process high-water mark, including allocator/driver/startup effects. Separate
counter peaks must not be summed. Non-image-plane VRML texture storage lies outside
the two logical counters; its process RSS is still measured. GPU residency is unmeasured.

## Qualification And Provenance

Both optimized arms use RelWithDebInfo, the pinned 510-package environment,
native X11/Intel Mesa, physical 1280x720, DPR 1 and eight effective MSAA samples.
The baseline has independently copied common build outputs plus a genuine
ccache-disabled provider compile/Gui relink. It is not a separate clean build.
The sole performance source delta is the visibility guard at the payload-ready
restore barrier; both arms retain the Phase 2 safety/calibration fixes.

- Eager baseline: `c4e009ed3eb5e517da5ea01d6143ff84e3bf4738`.
- Lazy change/common: `c7b9d163df2bf1b0e3e05195a4d93ca4235cb2d1`.
- Qualification receipt SHA-256: `3221c047f1cc350be584bbc9916ae44680a0e1f70e1181583e13f1534a52068a`.

Each arm passes nine focused C++ suites, legacy map save/reopen/forced-recompute
reference validation, eighteen absolute format-resource rows and seven runtime
contexts. All fourteen second-round checks against exact frozen contexts pass.
The native endpoint observes Coin rendering, a later swap and GL completion.
VRML completion follows its natural load-completion notification and later render;
it does not force an early redraw when decoded bytes appear. Independent source
texture/reference checks remain outside the primitive timers.

The recorded host retained background applications, including an unrelated
FreeCAD process predating the run by hours. Only one T020 worker ran at a time.
There is no controlled idle-host claim; the fixed dispersion rule applies.

## Recovery Within T020

1. Preserve the aborted cell and complete-cell outcomes unchanged; keep T004/T010 gated.
2. Diagnose the unclassified failure using explicitly ineligible executions and retained logs.
3. Resolve the timing uncertainty through a dated D001 owner decision and a fresh execution
   profile/freeze where justified; do not relax thresholds or select replacement outliers.
4. Before any future collection, freeze the separately tested
   [failure-capture extension](benchmark-image-collect.py), its diagnostic destination
   and the original collector. Collect a whole required cell from pair zero; never resume
   or merge the partial run. Any worker/context/arm change requires affected requalification.

The failure extension retains bounded raw output/result files only in an owner-only
local directory after failed-child cleanup, plus sanitized status/hash metadata.
Its six synthetic tests cover success parity, exit/result preservation, signal,
timeout, output-cap capture and exclusive destination/identity reuse rejection. This artifact authorizes no acceptance retry.
Further technical selection is tracked under D004 and T020.

Diagnostic investigation completed 24 baseline qualification executions and 24
executions of the exact frozen baseline worker command. All passed and all are
explicitly ineligible for acceptance. They do not recover the lost original
failure diagnostics or resolve the timing variability. No acceptance was retried.
