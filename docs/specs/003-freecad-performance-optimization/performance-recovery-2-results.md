---
title: T020 corrected native-frame measurement results
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# T020 Corrected Native-Frame Measurement Results

**T020 measurement/disposition is complete. P07 is NOT ACCEPTED under D001.**
All seven valid cells completed once. First-show and non-regression failures
remain failures; high-variability findings remain inconclusive. Memory passes
do not offset them. Phase 3 and production acceptance remain unsatisfied.

D005 and mandatory T022 block T004/T010 pending the material owner decision and
a corresponding source/measurement/revalidation plan. No behavior change, lower
resolution, revised budget, accepted tradeoff or further sampling is authorized
by publishing these findings.

[Machine-readable results](performance-recovery-2-results.json) bind all raw
observations, assessments, source/build equivalence, corrected worker, exact
contexts, qualification, controller and pre/post-collection audits. Private
locators, model contents and screenshots are excluded.

## Complete Fixed Campaign

Campaign `recovery-2-corrected-native-probe` contains 24 alternating fresh-process
pairs per cell: one diagnostic, three warmup and twenty measured. All 336 worker
observations passed the frozen correctness checks; every cell has its completion
marker. There were no retries, failed workers, exclusions or pooled campaigns.
Both audits passed 42,290 hashes.

| GUI endpoint | Eager median / p95 (ms) | Lazy median / p95 (ms) | Outcome |
| --- | --- | --- | --- |
| Hidden open → correct frame | 3321.13 / 6482.52 | 1736.84 / 3562.18 | inconclusive |
| First show → correct frame | 71.66 / 103.36 | 857.91 / 1843.09 | fail |
| Repeat show → correct frame | 7.78 / 13.61 | 7.66 / 12.43 | inconclusive |
| Open + first show | 3390.18 / 6570.10 | 2565.95 / 5488.32 | inconclusive |
| All-visible open → correct frame | 4731.77 / 11922.84 | 3430.84 / 14979.88 | fail |
| PartDesign control open → frame | 284.79 / 543.87 | 318.75 / 425.29 | fail |
| VRML control open → textured frame | 2786.67 / 2899.67 | 2824.17 / 2931.50 | inconclusive |

The lazy first-show median **857.91 ms** exceeds 500 ms and p95 **1843.09 ms**
exceeds 1000 ms. All-visible p95 exceeds its non-regression allowance; PartDesign
median exceeds its allowance. These remain failures under the predeclared rule
even though variability is high. VRML and the other hidden timing endpoints pass
numeric rules but exceed the 0.10 IQR/median limit, so remain inconclusive.

The nominal hidden-open median is lower, but no accepted speedup follows from
that value: variability and the failed first-show gate prevent P07 acceptance.
Failed control limits likewise do not establish a causal P07 regression; the
all-visible code paths are equivalent and the host retained background load.

## Command Restore — Descriptive Auxiliary

| Fixture | Eager median / p95 (ms) | Lazy median / p95 (ms) |
| --- | --- | --- |
| Pinned Gridfinity | 829.12 / 1250.93 | 842.97 / 1201.21 |
| PartDesignExample | 248.65 / 288.37 | 257.39 / 278.19 |
| TestVRMLTextures | 1.83 / 2.16 | 1.90 / 2.49 |

These cells measure command-only restore completion, not native first-frame
latency. They cannot replace a GUI timing gate or independently establish P07
improvement. Model/reference checks run outside the primitive timer.

## Storage And Work Disposition

All four GUI resource comparisons pass their original limits. Values below are
maximum measured process/decoded/Coin high-water observations, not summed peaks.

| Case | Eager max RSS (MiB) | Lazy max RSS (MiB) | Eager / lazy Coin peak (MiB) | Eager / lazy decoded peak (MiB) |
| --- | --- | --- | --- | --- |
| Hidden first/repeat | 921.39 | 832.16 | 280.04 / 186.69 | 93.35 / 93.35 |
| All-visible | 918.21 | 920.04 | 280.04 / 280.04 | 93.35 / 93.35 |
| PartDesign | 398.34 | 387.25 | 0.00 / 0.00 | 0.00 / 0.00 |
| VRML | 374.09 | 373.27 | 0.00 / 0.00 | 0.00 / 0.00 |

Eager open materializes three hidden textures. Lazy open keeps three pending;
first show materializes one and leaves two pending; repeat show reuses it without
another load. Premature restore loads remain zero. This is deferred first-show
work plus two unshown loads avoided within the measured window, not lifetime
elimination of all future work. Original full resolution and calibration remain
correct in both arms.

The logical counters cover unique image-plane-owned decoded buffers/Coin fields,
including conversion/copy overlap. Separate peaks must not be summed. Process RSS
includes observed driver/harness effects from the fixed 3,686,400-byte CPU readback
buffer and reused RGBA8 capture renderbuffer. These values do not measure GPU
residency. VRML has no image-plane workload; its zero logical counters do not
measure its separate VRML textures.

Eighteen fresh-process direct/fallback format rows per original arm remain in
[the retained first-campaign evidence](t020-evidence/freeze-1-performance-results.md#deferred-work-and-storage).
Their C++ source and native binaries are identical in recovery-2 and are reused
with explicit equivalence. One/two-component fallback expansion is recorded there.
They do not close the separate P05/T004.2 comparative gate.

## Provenance And Limits

- Lazy/common commit: `613def1ab6bf615f82fe163e3b41489d0228acbf`.
- Eager baseline commit: `c574d418c015972cbe4109983c289ce963d24b47`.
- Qualification SHA-256: `a1ad40bbe6bc83c8761ef7776f08c93cdb44c406eb1d04e665e5c8579b2e49e9`.
- Executable freeze SHA-256: `1737adc03bf23359236dab8f889b214b163b0175c02a79c4697c1cdf7a74f563`.

Both arms pass 551 focused tests, seven runtime qualifications each, fourteen
repeat checks against exact frozen contexts and six ownership controls. Their
`src` trees and independently copied native outputs match qualified origins;
only the common Python probe changed. No new C++ recompilation is claimed. The
corrected probe retains the exact first qualifying swap and restores GL/context
state. Keyboard focus does not determine document ownership.

The locked 510-package RelWithDebInfo environment, native X11/Intel Mesa,
1280×720 DPR 1, eight MSAA samples, fixed camera/configuration and original D001
numeric/sampling rules are unchanged. There is no controlled idle-host claim.
No accepted performance gain is inferred from high-variability timings.

The first campaign remains independently preserved: its partial hidden cell stays
ineligible and its inconclusive findings remain unchanged. No samples are pooled
or selected between campaigns. See [the two-campaign disposition](performance-results.md).

The original freeze's `decision_ledger` binding is now archived as
[t020-recovery-2-evidence/frozen-decision-ledger.md](t020-recovery-2-evidence/frozen-decision-ledger.md).
The published validation manifest records its original repository path, exact
old SHA-256, archival path/SHA and the
[decision receipt](t020-recovery-2-evidence/decision-receipt.json) path/SHA.
D005 was added to the current ledger only after all samples and the successful
post-collection audit. This is an explicit archival substitution for future
verification, not a change to any frozen Q/index/audit or observation.
