---
title: T010 final Linux verification and review evidence
doc_type: spec
artifact_type: review
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# T010 Final Linux Verification And Review Evidence

T010 and all four children are complete for the declared Linux scope. The final
full test gate, independent reviews and hash-bound evidence checks passed.
This review selects the integrated base plus the exact five-file patch in the
[final source manifest](t010-final-source-manifest.json). It is Linux-only,
including the declared compose/X11 software-GL and generic/Fusion viewer/memory
profiles. Windows/macOS/Wayland/other drivers and GNOME/IBus memory qualification
remain unqualified. It does not certify an isolated upstream PR or a release.

## Corrective Changes

- Defer image file/size/show work while the App object is restoring, including
  the interval before the GUI restore flag. Two new independent ready/late rows
  failed on old code and pass on the correction; finish-time calibration remains.
- Make preview hiding idempotent, removing the traced close-time Coin warnings.
- Restore finite single-edge loft admission and initialize the real Measure
  module in its test fixture. These repair two reproduced pre-existing full-gate
  failures; no test assertions or full-suite exclusions were weakened.

The [build receipt](t010-build-receipt.json) binds successful locked Pixi build,
30 source/test/config hashes, 18 binaries/build IDs and normal trace-OFF proof.
[The patch](t010-evidence/final-corrections.patch) is not committed yet.
Any later source/dependency/fixture/runtime/profile change needs affected
requalification or separately reviewed equivalence.

## Executed Checks

| Check | Outcome |
| --- | --- |
| Focused App/archive/map; Reader; bitmap | 310;26;82 cases pass |
| GUI lifecycle/render/tree/lifetime/selection/queued updates | Eight target suites pass; new App-only restore rows green |
| Lifecycle Memcheck | 26 Qt passes, zero access errors, one existing libunwind suppression; leaks/undefined values not checked |
| Dependent CAD | Two save/reopen cycles, three forced recomputes, four target edges preserved |
| Additional Sketcher Python API | 16 cases pass |
| Comparison fixture identity | Seven exact original campaign identities; exact reopened identities for three P02 fixtures; separate save/reopen semantic checks |
| Supplemental Assembly/VRML | Pass; Assembly uses the separate sphere-only supplement below |
| Final full suite | 2426 pass,0 failures,2 skips,8 disabled of 2436 registered |
| Native public/private viewer | Original-resolution pixels/orientation/calibration; hidden0 → first show 1 / leaves 2; repeat inventory preserved; private viewer post-close document/viewer gone |
| Native load/cleanup trace | Six empty initializations and one nonempty first-show load; zero App-restoring loads, missing-image warnings or preview-removal warnings |

Two BackupPolicy tests unconditionally skip in existing source for filesystem
caching. Eight existing disabled tests concern backup formatting, observer helpers,
a parser wrapper and deliberate crash testing. They are not passing coverage.
The relevant enabled archive/map/image/tree/geometry gates pass independently;
no new exclusions were introduced. Exact names and original/final run outcomes
are in [the machine report](t010-results.json) and command receipts.

## Applicability And Historical Limits

The [applicability matrix](t010-applicability.json) preserves every P01–P08
source delta and required qualification. The core App/Base/Part/Sketcher binaries
are byte-identical across the last GUI-only corrections; this bridges the seven
executed core fixture checks. GUI behavior is freshly checked on final libraries.
P03-only campaigns never had reopened identity baselines; do not call their
separate final roundtrips exact campaign-reopen matches.

P01/P07 historical embedded-Python external DSO closure is unknown: selected
build-relative modules and locked 510 package metadata did not exclude user-site
NumPy. Their old magnitudes, tails, whole-open/open+show and RSS figures remain
historical. P02/P03 used exact isolated `-I -S -B` workers and full runtime maps;
that concern is excluded for their accepted records. All original data,
statistical labels, policies and receipts remain unchanged.

D007 retains narrowly scoped local directions and the hidden/subset first-use
and logical-memory tradeoff. This is not exact-final timing equivalence, a new
numeric response budget or a whole-open/RSS guarantee. Current numeric budgets,
tails or absolute P01/P07 claims would require a newly frozen masked-runtime
campaign. No new timing campaign was necessary for this scoped disposition.

The old premature-load classifier omitted App-only restore. Its old zero count
is incomplete for that state and is not used as the final hard-gate proof.
Corrected source, RED/GREEN rows and final stage load/warning provenance establish
the new invariant. Empty initialization calls are not payload reads.

## Runtime Isolation And Memory Scope

Embedded FreeCAD explicitly enables user-site Python and does not honor the
usual no-user-site environment flag. Earlier viewer runs mapped a different
user NumPy and are ineligible as locked-runtime evidence. Accepted GUI runs hide
that directory in a task-owned mount namespace, keep user files unchanged, and
hash actual mapped modules. Selected loaded App/Gui and other module hashes
match the final build receipt and final patch, not merely the branch name.

The original GNOME-theme/Qt-D-Bus Memcheck startup failure is preserved. The
accepted memory-access profile is compose + generic/Fusion with only the existing
libunwind address-probe suppression. No IBus/GNOME memory-safety claim is made.

Nested Pixi environment probing cannot run inside this namespace; two pre-test
failures are retained. The accepted final full run activates the locked Pixi
environment first and runs the exact repository test-task body,
`ctest --test-dir build/debug`, under the mask. Reporting flags add output/JUnit
only. This lower-level diagnostic route runs the same full test graph and
introduces no filter, skip, lock solve or fabricated platform override.

## Assembly-Only Spherical Basis Supplement

The unchanged campaign oracle rejects one Assembly spherical-surface basis
normalization record, although topology and document-roundtrip invariants match
exactly. A pure kernel import/export reproduces this sphere record. OCCT 8.0.1
[defines its format](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V8_0_1/src/ModelingData/TKGeomBase/GeomTools/GeomTools_SurfaceSet.cxx)
as center, three basis vectors and radius. The separate supplement accepts only
Surfaces/type4/exactly 14 tokens, protects center/radius/structure exactly, and uses
the existing 2e-15 component bound plus unit/orthogonal/handedness and independent
invariant checks. Eight negative checks reject changed centers/radii, excessive
or nonfinite drift, invalid frames, malformed records and the wrong section.

This does not change the seven campaign fixture oracles or old results. The
pure kernel roundtrip also normalizes eight curve records; those remain outside
this supplement. Its full-shape invariants pass the existing tolerance, not exact
numeric identity. See the separately bound reproduction and fixture report.

## Evidence And Next Boundary

[T010 validation](t010-validation.json) binds current authority, earlier receipt
archives, checks and independent reviews. [Prior authority](t010-prior-authority/manifest.json)
retains11 exact T004.4 document snapshots. Evidence helpers are historical
execution snapshots, with locator normalization declared; the permanent restore
regressions remain in the normal native test suite.

The generated [owned application window](t010-evidence/native-viewer-window.png)
was visually inspected; no desktop, operator window or private model image is
published. First-correct-frame observations are functional checks, not latency
samples. The initial single-instance launch and harness-preparation failures
remain ineligible and are disclosed; qualified commands omit that flag.

T011 still owns final promotion audit, residual ownership and spec closure.
