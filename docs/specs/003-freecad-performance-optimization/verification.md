---
title: Performance optimization reconciliation
doc_type: spec
artifact_type: verification
status: code-derived
owner: local-developer
last_reviewed: 2026-11-09
---

# Performance Optimization Reconciliation

Current performance policy: **D007 cost-benefit review** in
[acceptance-policy-v2.md](acceptance-policy-v2.md). Original D001 v1 outcomes and
dated checkpoints below remain evidence; their numeric vetoes do not govern current
product decisions. Correctness/provenance/coverage gates remain mandatory.


Reviewed source on `personal/integration-testing` at `13d601b9dd6b66026e2d0dbb5c16156f18bdb592`.
This is a source-backed reconciliation, not lifecycle closure or upstream PR
approval. This dated report predates the 2026-11-06 format migration. Current artifact
authority is requirements.md, design.md, tasks.md and traceability.md; spec.md
is now a brief. The migration does not make the historical test run current. Historical measurements
remain dated evidence, not fresh benchmarks of this revision.

## Historical Requirements And Acceptance

| Requirement | Current disposition | Evidence / remaining gate |
| --- | --- | --- |
| FR-001; SC-001 | Implemented; focused validation passes | `PropertyFileIncluded::RestoreDocFile()` in `src/App/PropertyFile.cpp` uses bounded reads and checks read/write/close errors, preserves notifications and read-only destination handling. `XMLReader::readFiles()` in `src/Base/Reader.cpp` preserves entry boundaries. Full archive compatibility matrix remains a broader gate. |
| FR-002 | Implemented; visual acceptance partial | `convertDirect32BitImage()` and `BitmapFactoryInst::convert()` in `src/Gui/BitmapFactory.cpp` preserve vertical flip and RGBA order. Four bitmap tests pass. Premultiplied/uncommon formats use Qt conversion to RGBA8888; they do not use the direct scanline path. Full image-plane color/alpha/scale/ownership matrix remains open. |
| FR-003; SC-002 | Historical measurements recorded; current measurements unverified | May 22 before/after observations remain in tasks/research. No current revision performance comparison was run. |
| SC-003 | Unverified | Local evidence exists; exact proposed upstream PR descriptions and their validation packages were not reviewed. |
| Acceptance 1 | Correctness partially verified; speed requires comparable rerun | Binary roundtrips passed; no current large-document benchmark. |
| Acceptance 2 | Partial | Unit conversion tests pass; rendered image-plane visual comparison remains open. |

## Historical Plan Task Reconciliation

| Plan IDs | Disposition at this revision |
| --- | --- |
| A1, B3, C1, D1, E3 | Historical investigation/measurements recorded; no fresh benchmark claim. |
| A2, B1, B2 | Buffered restore implemented and focused correctness checks pass. |
| C2 | Conditional: add GUI restore timings only if renewed profiling identifies a remaining hotspot. |
| D2, E2 | Fixture/unit coverage exists; uncommon/premultiplied alpha and rendered-image visual matrix remains incomplete. |
| E1 | Direct scanline conversion implemented for RGB32, ARGB32, RGBX8888 and RGBA8888. Other formats use Qt conversion. |
| F1 | Implemented: image/size updates defer during restore in `ViewProviderImagePlane.cpp`. |
| F2 | Lazy full-resolution texture loading implemented through `pendingFullResTexture`, `finishRestoring()`, and `show()`. Reduced-resolution initial textures are a separate unimplemented behavior decision. |
| G1 | Implemented: `Tree.cpp` records one pending restore status update and flushes after document open. Larger-model profiling still needed. |
| G2 | Side effects documented; bulk-delete bypass deliberately unimplemented. `_slotDeleteObject()` still handles selection, focus, children/reparenting, maps and refreshes per object. |
| G3 | Historical startup investigation recorded; optimization remains lower priority. |
| H1 | Implemented: named temporary trace switches absent from App/Base/Gui and tests. Reintroduce instrumentation deliberately before using old trace commands. |
| H2 | Partial: focused suites pass; no fresh full build or complete CTest run. |
| H3 | Open: reviewer-ready PR evidence and platform/visual results need checking. |
| H4 | Implemented as focused reachable commits; upstream branch selection/review remains separate. |

Reachable implementation commits include `57a9b795e82` / `a93b54fb63d`
(buffered restore), `eff963284de` (buffered save), `e093a1f66b3`
(element-map parsing), `f7227a66218` and `7252af06fb8` (image conversion),
`cdf2a84f4ba` and `c1722e4b2c5` (image-plane restore/lazy loading), and
`61031d7f8a5` / `00dd158de83` (tree restore batching). These demonstrate
separate changes, not that every PR gate is satisfied.

## Validation Run On 2026-11-04

Existing debug binaries were exercised; this documentation reconciliation did
not rebuild source or run `pixi run --locked test`.

```sh
pixi run build/debug/tests/App_tests_run --gtest_filter='PropertyFileIncludedTest.*:ElementMapTest.*'
pixi run build/debug/tests/Base_tests_run --gtest_filter='ReaderTest.readData*'
QT_QPA_PLATFORM=offscreen pixi run build/debug/tests/Gui_tests_run --gtest_filter='BitmapFactoryTest.*'
```

Results: 24 App tests, four Reader tests and four bitmap tests passed.
A scratch `FreeCADCmd` script additionally saved and reopened documents with
two included-file properties each: empty, 768-byte binary and 262403-byte
multi-buffer payloads. Both entries retained exact bytes and read-only restored
permissions in all three cases. This is a local integration check, not a
committed regression test or exhaustive corruption/failure coverage.

Transient logs: `/tmp/freecad-spec-recon-{app,base,bitmap}-tests.log`.
Scratch scripts/results: `/tmp/freecad-spec-reconciliation-afbb1wf9/`.
These temporary files are not durable project artifacts; repeat the commands
and recreate fixtures for any future review.

## Current MoE Disposition

The 2026-11-06 review inspected root c69560af8a and nested docs 8d1abe9.
Five AI specialist perspectives covered CAD persistence, rendering, FreeCAD
lifecycle, performance engineering and requirements/QA. Findings are static
source/code-path evidence and acceptance gaps, not executed reproductions.
Historical implementation-presence/test rows below do not discharge these new
criteria. F001 has since been reproduced and locally corrected/qualified by
T012 (see below); T013 locally qualifies the F002 calibration correction.
T014 now locally corrects/qualifies F003 receiver lifetime. T003 qualifies local Requirement 2 fidelity;
T006 qualifies local Requirement 5 lifecycle; T007 now qualifies local Requirement 6. Exact-final/platform acceptance remains pending.

## MoE Review Findings

| ID | Severity | Observed evidence / consequence | Owner / task destination | Disposition |
| --- | --- | --- | --- | --- |
| F001 | blocker | BitmapFactory.cpp:64–98 allocates using narrowed SbVec2s dimensions but writes original dimensions; inspected Linux 65537x1 becomes a 1x1 allocation. Null/extent/arithmetic policy was missing. | local-developer; T012 then T003 | T012 correction plus T003 current format/render/access qualification pass; final T010 applicability remains |
| F002 | blocker | Gui/Document.cpp:2170 clears GUI restoring before finishRestoring; ViewProviderImagePlane.cpp:215–251 can write DPI-derived XSize/YSize during deferred load. Non-default calibration can be lost. | local-developer; T013 then T006 | T013 calibration correction plus T003 fidelity and T006 expanded lifecycle pass locally; T010 final applicability remains |
| F003 | blocker | Tree.cpp:696–700 captures raw this in application callbacks; destructor at 762–775 omits those connection disconnections; fastsignals::connection destructor is unscoped. | local-developer; T014 then T007 | T014 two disconnections plus real destroy/recreate/open and queued-status Memcheck checks pass locally; T010 final platform applicability remains |
| F004 | request | ElementMap.cpp:89–105 changes numeric conversion as well as token splitting, with unchecked signed arithmetic; current parser fixtures do not cover all writer/alias/radix/child identity forms. | local-developer; T015, T016 then T005 | T015 numeric/range/prefix-ownership correction qualified; T016 identity fixtures and T005 checkpoint remain |
| F005 | request | Tree suppression uses broad isRestoring while completion follows application finish-open; idle direct load/restore/import/paste can lack a matched completion. | local-developer; T017 then T007 | T017 correction and T007 current broader tree checkpoint pass locally; T010 final applicability remains |
| F006 | request | PropertyFile copy errors are caught by XMLReader::readFiles; hook exceptions alone do not establish diagnostic/failure-list/partial-file/continuation behavior. | local-developer; T018 then T002 | Open archive-level outcome table and failure fixtures |
| F007 | request | Deferred image decode/conversion moves into show; fallback may expand 1/2-component sources to RGBA. Open-only timings can hide first-frame latency or image-memory effects. | local-developer; T020 then T004 | Open comparative frame/show/repeat latency, texture counts and peak memory evidence |
| F008 | request | Candidate/final revision selection, per-change coverage and benchmark pass rules were insufficiently connected to acceptance; final task assembled timings without explicit evidence invalidation. | local-developer; T019, T001, T004.3, T010 | Open pinned manifest/protocol, per-optimization evidence and exact-final applicability |
| F009 | suggestion | spec_open_decisions returned no records for embedded D001–D003, while task_context retained references; advisory ready packets describe context availability. | local-developer; T021 | Discovery/guardrails verified; D001 protocol resolved, D002 routed to B009, D003 initial policy resolved |

All findings stay in this active package because they change its existing
contracts and acceptance. No source finding is merely routed to an unowned
backlog. Expert subsidiary recommendations are included: hashed/alias/shared-map
identity, per-optimization attribution, explicit noise/non-regression rules,
peak CPU/Coin image storage and applicability of manual evidence. Driver-specific
GPU residency claims require separate actual measurements.

## MoE Regression Matrix

| Gate | Required fixtures / outcome | Owner tasks | Evidence status |
| --- | --- | --- | --- |
| Image extent/arithmetic | Null, valid edge and slender 32767/32768/65537 inputs; safe documented rejection, checked offsets; sanitizer-equivalent checks where available | T012, T003 | T003 current 82-case expanded byte/access matrix and real rendered-plane qualification pass |
| Calibrated CAD planes | Independently non-default XSize/YSize and placement; visible/hidden reopen, hidden import/paste/show, repeated show, save/reopen; preserve new-image/explicit-resize behavior | T013, T006 | T006 expanded calibrated archive/copy/import/show/edit/error checks pass locally |
| Receiver lifetime | Actual tree destroy/recreate then start/finish-open; callbacks and queued work cannot access dead receiver | T014, T007 | T014 and current T007 receiver-lifetime reruns pass; T010 final applicability remains |
| Numeric and subelement identity | Signed/unsigned-form tolerance, decimal/hex bounds, malformed/overflow tokens; semicolon/colon/dollar writer forms; multiple aliases/SIDs/shared children; bidirectional lookups after buffers die; dependent CAD target reopen/recompute | T015, T016, T005 | T015 numeric correction and local tests pass; broader writer/child/CAD identity remains T016/T005 |
| Restore completion | Initially idle timers; app open, direct load/restore, import/paste and failure cleanup; eventual tree population/status/observer work without unrelated edit/open | T017, T007 | T017 correction and T007 small/larger idle restore/load/import/error and observer completion pass locally |
| Archive failures | Empty/nonempty failing first entry plus valid second; read/write/close injection, diagnostics/failure list, partial bytes/permissions and notifications | T018, T002 | Pending approved outcome table and execution |
| End-to-end performance/resources | Pinned command restore, GUI first-correct-frame, hidden-first-show and repeat-show endpoints; visibility/texture counts; process/decoded/Coin peaks for direct and grayscale/alpha fallback fixtures | T020, T004 | First collection incomplete: hidden primary aborted, two GUI timing cells inconclusive; performance-results.md and D004 |
| Exact final evidence | Per-optimization inventory for restore/save/parser/conversion/lazy/tree; predeclared pass rules; final-revision automated/manual/archive/lifecycle/benchmark reruns or reviewed equivalence | T019, T001, T010 | Manifest/protocol recorded; exact-final execution pending |
| Decision discovery | MCP exposes D001/D002/D003, owners and blockers; task contexts agree; context readiness does not overrule status/dependencies | T021 | Passed documentation/tool checks; no product decision resolved |

## T001 Protocol Definition — 2026-11-06

D001 is resolved as delegated technical selection under the user's instruction to
complete T001; local-developer owns the decision. The authoritative
[benchmark protocol](benchmark-protocol.md), [numeric plan](benchmark-plan.json)
and [bounded collector](benchmark-runner.py) fix eight exact optimization source
origin pairs, primary/control fixture identities, matched optimized profile,
endpoints, 20 measured pairs, statistics, variability and pass rules. The source
origins are historical lineage; future corrected arms must preserve Phase 2 fixes
and freeze exact commits/build/probe/generated-fixture receipts before sampling.
The T019 Debug intake snapshot remains immutable. No unsafe historical tree is
approved as a runtime comparison arm.

Executed checks:

- `python3 docs/specs/003-freecad-performance-optimization/benchmark-runner.py --check --self-test`
  passed: eight comparison origins reachable from intake, eight public fixture
  hashes, manifest/source/recipe identities, 24-pair schedule with 20 measured,
  and synthetic success, timeout, output-limit and failed-worker handling.
- `python3 docs/specs/003-freecad-performance-optimization/benchmark-runner.py --schedule`
  emitted the prescribed diagnostic/warm-up/measured AB/BA schedule. A synthetic
  execution request was rejected at the incomplete T012 qualification gate,
  without creating an output/sample file.
- `black --line-length 100` formatted the collector. Python AST parsing and the
  bounded direct artifact checker passed: all 40 task identities, completed
  T019/T021/T009 evidence, immutable manifests/historical artifacts and baseline
  inventory preserved; 79 local links resolved; private document plus three
  bounded PNG payload hashes/sizes/dimensions match; production source/config
  unchanged. The private path/payloads were not copied into this package.
- MCP `spec_open_decisions` returned no unresolved decisions;
  `task_context(T001)` returned zero gaps and no open decisions;
  `lint_spec_package` returned zero diagnostics. Final `active_spec_preflight`
  selects T012 with satisfied dependencies, zero context gaps and no open decisions.
  Root/nested-docs whitespace
  checks passed. Workbench could not traverse the nested docs authority boundary;
  direct reads/Git/hash checks and lifecycle tools supplied the actual evidence.

T001 is a planning completion only. No FreeCAD build, GUI execution, corrective
regression or real performance sample was run. Actual probes, generated fallback
outputs, qualified same-base arms, statistical assessor and application results
remain T020/T004 responsibilities after Phase 2. F001–F008 runtime/final gates,
platform confirmation and durable promotion remain open. Next: T012 — Guard
bitmap dimensions and conversion arithmetic.

## T012 Extent And Arithmetic Correction — 2026-11-06

T012 guards null, nonpositive and signed-short-unrepresentable QImage extents
before either conversion path can allocate Coin storage or narrow a dimension.
RGBA byte counts are checked by division against `qsizetype::max()`; direct row
offsets use `std::size_t`. Valid input keeps existing RGBA/channel/vertical-flip
semantics. Unsupported input or failed fallback conversion clears an existing
output to zero dimensions/components and no pixels. Coin allocation exceptions
remain exceptions. The public method signature is unchanged; its header documents
the failure contract, preserving the original CRLF endings in both GUI files.

The [execution receipt](t012-validation.json) binds T019's immutable manifest to
actual root revision plus the exact three-file working diff/source hashes,
seven clean matching dependencies, 510 matching package identities, unchanged
config/tool identities, effective Debug compile commands, binaries, resolved
libraries, fixtures and result/log hashes. This is a local CPU conversion check,
not native viewport or whole-image lifecycle acceptance. Final PR reconciliation
remains T010; no old timing or unrelated checkpoint is transferred as a pass.

- **Bounded red reproduction:** the new 65537×1 grayscale fallback regression
  failed on the old converter: a 1×1 output was returned instead of rejection.
  Memcheck on the old direct ARGB32 path then detected an invalid one-byte write
  immediately after Coin's four-byte allocation, exiting with code 99 at the
  first bitmap error. This turns F001's static finding into a runtime reproduction.
- **Fresh build:** `pixi run --locked build -j 2` completed successfully. Pivy
  emitted stub-generation warnings; unchanged workbench/test code emitted
  OpenCASCADE deprecation warnings. The receipt names the actual Debug profile
  and effective flags; these are not optimized performance results.
- **Focused checks:** the receipt's bounded offscreen gtest command passed all
  30 cases: four retained channel/alpha/orientation cases, null input, replacement
  of an existing output by an oversized fallback, and 24 parameterized boundary
  cases (32767/32768/65537, both orientations, ARGB32/RGBA8888/Grayscale8/premultiplied).
  Supported boundary fixtures compare every pixel; unsupported fixtures assert
  zero extents/components and empty storage. Inputs stay slender and bounded.
- **Memory access:** the final executable passed all 30 cases under Valgrind
  Memcheck with zero errors. Leak and undefined-value checks were disabled.
  The sole suppression matches libunwind's startup `syscall write(buf)` address
  probe, not converter/Coin errors; its exact text/hash is in the receipt. An
  initial unsuppressed startup stop and a 90-second startup timeout are retained
  as unsuccessful attempts, not bitmap passes. Qualified runs used a 300-second
  bound. This is sanitizer-equivalent access checking for these fixtures, not an
  ASan/UBSan, allocation-failure or 32-bit execution claim.
- **Style/scope:** focused pre-commit hooks pass after the formatter's local
  update. CRLF-aware root whitespace and nested-docs whitespace checks pass.
  Immutable T019 artifacts and completed T001/T019/T021/T009 evidence are retained.
  The direct final checker passed receipt source/binary/log/diff identities, all
  40 stable tasks, 81 local links/anchors, frozen history/manifest/protocol and
  preservation of the unrelated console working diff. Package lint returned
  zero diagnostics; T012 context has zero gaps. Workbench retained its nested
  docs authority-boundary limitation; direct Git/hash checks and lifecycle tools
  provide the execution evidence. Final lifecycle preflight selects T013 with
  satisfied dependencies, zero context gaps and no open decisions; task counts
  are five complete, 33 pending and two follow-up, with no partial/in-progress task.

T012 closes its local F001 correction scope. T003's broader format/visual matrix,
T013 calibration, T006 lifecycle, T020 performance/resources and T010 final
platform/composition gates remain open. Next: T013 — Preserve calibrated
image-plane dimensions during texture loading.

## T013 Calibration Preservation

The [T013 execution receipt](t013-validation.json) binds the correction to the
actual root revision plus exact source/header/CMake/new-test patch and file
hashes, build flags/cache/tool versions, seven clean matching dependencies,
510 matching package identities, binaries/resolved libraries, fixture recipes
and log hashes. The immutable T019 intake and T012 receipt are unchanged.

Pending texture materialization now chooses an explicit private `Preserve`
size policy. New image assignment and explicit file replacement choose
`Initialize`; normal DPI-based initial sizing remains intact. Both GUI restoring
and App importing defer loads. Loading still refreshes original texture and DPI
metadata, without rewriting calibrated XSize/YSize, Placement or quad geometry.
Gui::Document's existing finish/restore/import ordering was inspected and used
unchanged; correctness no longer depends on those flags remaining set.

- **Red reproduction:** the old provider passed five QtTest cases and failed
  four. Hidden reopen and controlled visible completion after flags clear reset
  calibrated XSize from 123.25 to 20 mm and changed the quad coordinates. Hidden
  copy and merge/import materialized textures prematurely. The visible archive
  reopen control already passed; no universal visible-restore failure is claimed.
  A missing complete-type include was corrected in the new test harness before
  this bounded runtime reproduction; the initial compile failure is retained
  separately in the receipt.
- **Build:** `pixi run --locked build -j 2` passed on the corrected composition,
  with Pivy stub-generation warnings. Focused target builds were used to iterate
  on the red diagnostic before changing production behavior.
- **Lifecycle checks:** the bounded offscreen `ImagePlane_Tests_run` executable
  passed seven functional rows plus initialization/cleanup (9 passes, no failures
  or skips). The CTest registration passed with its 120-second timeout. Tests use
  real App/Gui document controllers, embedded FCStd image payloads, visibility
  changes, actual copy and merge/import operations, and save/reopen. A separate
  native 3D viewer is not created.
- **Fixture assertions:** a 40×20 raster has independent X/Y DPI and 20×5 mm
  initial size, then calibration 123.25×67.5 with nonidentity translation/rotation.
  Visible/hidden reopen, first show, repeated show and another save/reopen retain
  dimensions, Placement, Coin translation/quad coordinates, full texture extents
  and components; loading refreshes DPI metadata. The restored file path differs
  from the source fixture. Hidden copied/imported objects are shown after closing
  the source document and saved/reopened again. Fresh image and explicit raster
  replacement retain DPI initialization; explicit raster/SVG resize retains
  calibration. Generated files are temporary construction fixtures, not pinned
  reusable performance workloads.
- **Regression/style:** all 30 existing bitmap cases pass on the current GUI
  library. The T012 source/test hashes are unchanged; its older Memcheck receipt
  remains dated rather than becoming a new memory-access claim. Scoped pre-commit
  and CRLF-aware whitespace checks pass; both edited GUI files retain CRLF.

The direct final checker passed source/new-test/diff/binary/log identities,
40 stable task IDs, five prior completed records, 83 local links/anchors and
preservation of frozen history/protocol/manifests/T012 receipt and the unrelated
console working diff. Package lint returned zero diagnostics; T013 context has
zero gaps and no open decisions. Workbench still cannot traverse the nested docs
authority boundary; direct reads/Git/hash checks and lifecycle tools provide the
actual validation evidence. Final lifecycle preflight selects T003 with satisfied
dependencies, zero context gaps and no open decisions: six tasks complete,
32 pending and two follow-up; none partial or in progress.

T013 completes the local F002 correction and its declared calibration fixtures.
No native frame, screenshot, broad format/visual/error matrix, cross-platform,
sanitizer or performance/resource acceptance is claimed. T003/T006, T020 and
T010 remain required. Next: T003 — Complete bitmap and rendered image
correctness coverage.

## T003 Bitmap And Rendered Fidelity Qualification

The [execution receipt](t003-validation.json) records the exact root revision,
tracked/new-test patch and file hashes, unchanged production sources, seven clean
matching dependencies, 510 matching packages, effective Debug build flags,
binaries/resolved libraries, actual renderer profile, fixture/reference contract,
commands and log/output hashes. T019/T012/T013 snapshots remain unchanged.
T003 adds tests and registration; no production converter/provider change was
needed after T012/T013.

| Slice | Current evidence | Result |
| --- | --- | --- |
| T003.1 direct conversion and extents | Four exact direct-format tests; all four direct formats included in the scalar/ownership matrix and 32767/32768/65537 width/height boundary matrix; null and replacement-output assertions | Current expanded 82-case suite and Memcheck pass |
| T003.2 fallback fidelity | Every valid QImage format in Qt 6.11.3 (36 total, including 32 fallback formats); indexed color/gray alpha palettes, odd/padded strides, non-byte-aligned RGBA64 and SVG-derived pixels | Scalar/source semantics, alpha, orientation and caller-storage lifetime assertions pass |
| T003.3 rendered planes | Real ViewProviderImagePlane Coin graphs, production SoQtOffscreenRenderer, independent scalar luminance/LA/RGB/RGBA reference, source-over color samples and calibrated scale/Placement checks | 39 format rows plus scale/placement row pass (42 QtTest passes including setup/cleanup), no skips |

The baseline is an independent historical-style QColor/pixelColor scalar
reference with normalized component semantics, not execution of unsafe old
binaries or reproduction of obsolete sizeInBytes/stride inference. Extra
alpha-only, float and CMYK cases are checked against normalized QImage semantics.
The predeclared [reference contract](design.md#image-qualification-reference)
keeps direct byte fixtures exact, permits one RGBA8 level for normalized fallback
quantization, two RGB levels at every rendered reference pixel and three at
analytic texel-center source-over samples. Geometry bounding rectangles are exact
with zero MSAA. No tolerances were relaxed after seeing render results.

Executed validation:

- `pixi run --locked build -j 2` passed; Pivy stub-generation warnings were emitted.
  Focused target builds were used during test development. Scoped pre-commit and
  whitespace checks pass; no existing production file or line ending changed.
- All **82 bitmap cases** passed normally and under bounded Valgrind Memcheck,
  with **zero errors**. The sole suppression is the libunwind startup address
  probe, whose exact text/hash is retained in the receipt. Leak and undefined-value
  checks were disabled; this is fixture-bounded access checking, not ASan/UBSan.
- All **40 functional render rows** passed using Mesa llvmpipe, OpenGL 4.5
  compatibility Mesa 25.2.8, Qt 6.11.3, DPR 1, a 320×240 framebuffer and no MSAA.
  Both native X11/xcb and Qt offscreen profiles ran inside task-owned Xvfb
  displays with software rendering, with 42 QtTest passes/no skips in each.
  The native X11 profile satisfies T019's rendered-check scope; the supplementary
  offscreen profile is recorded separately. The test compares every pixel to the scalar render, independently checks orientation
  and alpha blending at all 16 texel centers, and checks 20×10 world-unit scale
  (80×40 pixels) followed by translation and Z rotation (40×80 pixels).
- Ownership checks render/read only after QImage mutation/destruction, external
  backing-buffer destruction, clearing a temporary SoSFImage and allocator-pressure
  allocations. A copied Coin image remains valid after clearing its original.
- CTest passed **2/2 targets**: ImagePlaneRender_Tests_run and the existing
  ImagePlane_Tests_run calibration/archive suite. Both executed with no skipped
  cases; a missing OpenGL context is an explicit skip in the test and would not
  satisfy this acceptance record.
- All **41 output PNG hashes** match the earlier render run and the native
  X11/xcb run. Codex inspected the
  six-panel [render contact sheet](t003-render-contact.png): opaque/alpha formats,
  premultiplied alpha, indexed gray alpha, SVG and rotated placement agree with
  the numerical assertions. These are synthetic framebuffer images, not captures
  of the user's desktop/windows. The contact is retained; other outputs/logs are
  temporary, identified by hash and reproducible using the receipt commands.

The direct final checker passed exact source/new-test/diff/binary/log identities,
41 repeated/native output hashes and the retained contact, unchanged production
source, 40 stable task IDs, six prior completed records, frozen history/manifests/
receipts/protocol, 87 local links/anchors and the unrelated console working diff.
Package lint returned zero diagnostics; all three child contexts have zero gaps.
Workbench still reports its nested-docs authority limitation; direct Git/hash
checks and lifecycle tools supply the actual execution evidence. Final lifecycle
preflight selects T006 with satisfied dependencies, zero context gaps and no open
decisions: 10 complete, 28 pending, two follow-up, none partial/in progress.

Requirement 2 AC1–AC3 is implemented/verified for this named local profile.
T006 still owns broader deferred-load lifecycle/error/observer qualification;
T020 owns native-view first-correct-frame, latency and memory budgets; T010 owns
exact-final-revision and platform applicability. The recorded native X11 profile uses framebuffer rendering; it is not an
interactive native viewer, hardware-GPU, scanout, HDR/ICC policy or performance
acceptance.
Next: T006 — Validate deferred full-resolution image-plane lifecycle.

## T006 Deferred Image-Plane Lifecycle

T006 completes the local Requirement 5 AC1–AC4 checkpoint on root
`f46d1d28e729560950c20e20bcf0376b48b6cbf2`, docs
`946e26240d8364468a334d6d3f23d77bf72a229c`, plus the three tracked
source/test changes identified by [t006-validation.json](t006-validation.json).
T003 was committed before this slice. Previous manifests, receipts, rendered
contact and benchmark protocol remain immutable.

The final corrected fault-injection fixture reproduced eight failing rows on
the old provider (16 Qt passes, eight failures). Four early-show flag/payload
combinations consumed deferred work too early; removed/corrupt/empty payloads
left stale textures; an unsupported extent reset calibration before conversion
failed. Initial harness assumptions about blank-string property assignment and
read-only included-file permissions were corrected before that final red run.
The provider now defers ordinary show during restore/import, loads visible
pending data at the verified payload-ready completion barrier, clears rejected
textures, and initializes physical dimensions only after successful conversion.
Gui/App Document and MergeDocuments ordering are unchanged.

| Acceptance | Current evidence | Outcome |
| --- | --- | --- |
| Requirement 5 AC1 | GUI restore/App import crossed with ready/late extracted payload; show before completion retains pending request | All four rows pass; completion may run before ObjImporting clears |
| Requirement 5 AC2 | Visible finish and hidden first show materialize original raster/SVG; hidden paste/import survives source document closure | Full source resolution and DPI metadata retained |
| Requirement 5 AC3 | Independently calibrated XSize/YSize, nonidentity Placement and Coin geometry across visible/hidden reopen, first show, save/reopen | Raster and SVG rows pass; hidden resize/save before first show preserves new 80x30 dimensions |
| Requirement 5 AC4 | Explicit image replacement/resize, missing/corrupt/unsupported payload, empty property Paste and invalid external assignment | Successful insertion retains DPI sizing; failures clear stale texture while preserving calibration; valid reassignment recovers |
| Requirement 2 regression | Expanded bitmap matrix plus production-renderer scenes rerun against changed provider | 82 byte/access cases and 42 native X11 render Qt passes; 41 PNG hashes match immutable T003 outputs |

`pixi run --locked build -j 2` passes. Lifecycle QtTest has **24 passes**
(22 functional rows plus setup/cleanup), zero failures/skips. Both lifecycle and
renderer CTest targets pass under task-owned Xvfb; native xcb rendering uses
Mesa llvmpipe, not an interactive viewer. Scoped pre-commit passes and GUI source
CRLF is preserved. The receipt binds the exact tracked diff, source/build/runtime
hashes, seven clean dependency heads, 510 matching package identities, log hashes,
fixture construction and unchanged lifecycle inputs.

The error policy consumes a failed pending request with a warning for nonempty
paths; no automatic retry loop is added. Already materialized textures remain
cached across repeated show even if their backing payload disappears. Deleting
an external original after inclusion does not invalidate the embedded payload.
Empty-property Paste/undo clears texture and survives save/reopen; existing
blank-string setValue behavior remains unchanged. SVG keeps its existing
explicit-resize rerasterization behavior.

Lifecycle Memcheck is **not qualified**: default and compose-input-method retry
both exited 99 in Qt aeshash128/QDBus/QIBus startup before the first test; the
retry still loaded IBus. No broad suppression was introduced. Logs are hashed
in the receipt; T010 owns environment diagnosis and final revalidation. Prior
T003 bitmap Memcheck evidence stays dated and is not transferred to lifecycle.
T018 owns missing archive-entry save outcomes and the source-observed
_OriginalName mutation before rejected external assignment; metadata
transactionality was not tested here. No performance/resource, interactive
first-frame, hardware-GPU, cross-platform or final PR acceptance is claimed.
T020/T004 and T010 remain required. Guarded lifecycle updates complete both
children and parent: 13 complete, 25 pending, two routed follow-ups, zero partial
or in-progress records, all 40 IDs retained. Package lint reports zero diagnostics;
preflight selects T014 with satisfied dependencies, zero context gaps and no open
decisions. The direct checker verifies 88 local links/anchors, ten prior completed
task records, immutable artifacts, exact receipt hashes, CRLF and preservation of
the unrelated console edit. Agent Workbench remains navigation-only because its
nested docs authority boundary blocks validation planning; direct Git/source
checks and executed commands establish the evidence above.
Next: T014 — Bind tree application callbacks to widget lifetime, in Phase 2B.

## T014 Tree Widget Receiver Lifetime

T014 completes local Requirement 6 AC3 on committed T006 root
`99b341fa31` / docs `413cefe`, plus the exact source/new-test diff identified
by [t014-validation.json](t014-validation.json). Previous receipts and frozen
manifest/history remain unchanged. Only two production lines are added:
start-open and finish-open explicitly disconnect in TreeWidget's destructor
alongside its seven existing Gui subscriptions. Tree.h, public API, connection
member types, restore suppression and scheduler behavior are unchanged.

The new QtTest reproduces retained receivers after actual direct and parent
widget deletion: both rows fail on the old code, each leaving one start-open
and one finish-open subscription, while seven Gui counts return to baseline.
A separate old-code Memcheck run opens a real saved FCStd after heap receiver
deletion and detects an invalid one-byte write in slotStartOpenDocument, inside
the freed TreeWidget allocation. It exits 99 at the first error; it is failure
reproduction, not a passing check.

| Regression | Current outcome |
| --- | --- |
| Direct and parent destruction | QPointer confirms deletion; all nine App/Gui subscriber counts return to baseline |
| Actual App open after destruction/recreation | Three cycles, six successful App::Application::openDocument calls, balanced start/finish signals; replacement tree populates/clears a real Gui document item |
| Queued status work | Real status timer active, production slot queued before deletion, owned timer pointers clear, event loop drains; replacement receiver can queue/drain again |
| Memory access | All four functional rows pass under Memcheck with zero errors; one pre-existing exact libunwind probe suppression only |

`pixi run --locked build -j 2` passes. The lifetime executable has **six Qt
passes** (four functional rows plus setup/cleanup), zero failures/skips, both
normally and under Memcheck. Five CTest targets pass: TreeWidgetLifetime,
ObjectSelection, SelectionView, ImagePlane and ImagePlaneRender. Scoped
pre-commit and root/nested-doc whitespace checks pass; Tree.cpp CRLF is retained.
The new test is bounded to 120 seconds. The receipt binds source/new-test/diff,
red and corrected binaries, runtime/build/log identities, seven clean dependency
heads and 510 unchanged package identities. Guarded lifecycle completion leaves
14 complete, 24 pending and two routed follow-ups, with zero partial/in-progress
records and all 40 IDs retained. Lint reports zero diagnostics; preflight selects
T017 with satisfied dependencies, zero context gaps and no open decisions. The
direct checker verifies exact receipt hashes, 89 local links/anchors, all 13 prior
completed task records, frozen artifacts, CRLF and the unrelated console diff.
Agent Workbench remains navigation-only at its nested docs authority boundary;
direct checks and executed commands establish the validation above.

Memcheck explicitly sets both QT_IM_MODULES=compose and QT_IM_MODULE=compose,
which reaches the tests. The earlier T006 single-variable compose retry still
loaded IBus; its unsuccessful result remains dated and unchanged. No tree or Qt
lifetime errors are suppressed. Leak/undefined-value checks are disabled;
this is bounded memory-access validation, not ASan/UBSan. The test is not
registered on Windows because internal TreeWidget has no DLL export; adding a
public export solely for testing was unnecessary. T010 owns Windows/native and
final runtime composition qualification, including default IBus startup.

These tests isolate receiver lifetime with an empty saved archive and explicit
Gui document registration after real App open. They do not qualify all object
side effects, deferred focus interactions, restore completion paths, scheduler
semantics or performance. Existing receiver-context Qt callbacks and owned
timers retain their lifetime handling; no new queued callback is introduced.
T017/T007 remain required. Next: T017 — Match tree restore suppression to all
completion paths.

## T017 Tree Restore Completion

T017 completes local Requirement 6 AC1/AC4 correction and focused qualification
on committed T014 root `3587399d1b` / docs `c79bd6e`, plus the exact tracked
source/registration and new-test diff in [t017-validation.json](t017-validation.json).
Prior receipts, benchmark protocol, manifest and completed task records remain
unchanged. Tree.h, App/Gui document ordering/flags, public API and layout are
inspected unchanged.

The final old-code run has five Qt passes and seven failures: direct restore,
Python doc.load(), delayed restore, zero-timeout restore, merge import all leave
objects unpopulated; empty app-open and failed-direct restore leave observers
unattached. Populated app-open, copy/paste and failed app-open are passing
controls. The initial Python C-API harness omitted the GIL and crashed in the
Python allocator; GDB identified that harness fault. Final red/green runs acquire
the GIL and release owned references correctly. Import sources stay alive during
completion checks so closing a source cannot trigger the target scheduler.

T017 keeps broad restore guards against processing, while retaining one pending
flag and one existing owned status QTimer. Requests during restore schedule the
timer instead of stranding it; a configured zero retry timeout becomes 1ms while
restoring. App finish-open preserves the pending flag until an immediate flush
runs. Direct restore/import/failure exits resume through the timer after their
global guards unwind. Successful processing clears pending and stops the timer.
New restoring document items request a pass even with no queued objects so the
completion pass attaches deferred change/touch observers. No additional signal
subscription, timer or document lifecycle change is introduced.

| Entry point / outcome | Current result |
| --- | --- |
| Populated and empty App open | Synchronous population and observer attachment at completion |
| Direct restore, real Python load, delayed restore/afterRestore | Event loop alone completes population and attaches observers from idle timers |
| Zero configured timeout | Restored tree completes; no processing while restoring and no continuing ticks after completion |
| Copy/paste and MergeDocuments archive import | Target populates without closing source or another edit/open |
| Failed direct restore after controller recreation | Invalid-file exception unwinds; empty tree attaches change/touch observers without another operation |
| Failed App open | Restoring guard exits; scheduler becomes idle without another operation |

All **ten functional rows** pass (12 Qt passes including setup/cleanup), zero
failures/skips. Direct/Python/delayed/zero-timeout hooks pump Qt events while the
restore guard is active and confirm no premature population. Final checks
observe child counts, Gui change/App touch subscriber counts, timer stopping and
no later ticks; a label edit only after completion confirms observer delivery.
Memcheck repeats all 12 Qt passes with zero errors and one pre-existing exact
libunwind startup-probe suppression; no tree/Qt/Python lifetime error is
suppressed. Leak/undefined-value checks are disabled; compose input-method
profile is explicit, and default IBus/final-platform applicability remains T010.

Full `pixi run --locked build -j 2`, scoped pre-commit and six CTest targets pass:
TreeRestore, TreeWidgetLifetime, ObjectSelection, SelectionView, ImagePlane and
ImagePlaneRender. The receipt binds actual patch/source/build/runtime/log hashes,
seven clean dependency heads and 510 matching package identities. Tree.cpp CRLF
is retained. The new test has a 120-second bound and shares T014's non-Windows
registration because internal TreeWidget has no DLL export.

Small empty/one-object/invalid-file fixtures qualify this correction; they do not
complete T007's larger-model, selection/focus or per-object-effect acceptance.
Broader malformed-archive outcome policy remains T018/T002. No performance,
resource-budget, interactive/native viewer, cross-platform or final PR acceptance
is claimed. Guarded completion leaves 15 complete, 23 pending and two routed
follow-ups, with zero partial/in-progress records and all 40 IDs retained. Lint
reports zero diagnostics; preflight selects T007 with satisfied dependencies,
zero context gaps and no open decisions. The direct checker verifies exact
receipt hashes, 90 local links/anchors, all 14 prior completed task records,
frozen artifacts, CRLF and the unrelated console edit. Agent Workbench remains
navigation-only at the nested docs authority boundary; direct checks and executed
commands establish the evidence above.
Next: T007 — Validate tree status batching and larger-model behavior.

## T007 Tree Functional Qualification

T007/T007.1 complete the local Requirement 6 AC1–AC4 and CP-003 checkpoint on
committed T017 root `da90da6f93` / docs `68370d7`, plus the exact new-test,
registration and retained fixture diff in [t007-validation.json](t007-validation.json).
Production source, public API and prior T014/T017 tests remain unchanged.

The retained FCStd inputs contain 10 and 514 objects respectively: eight or 512
image planes, two nested groups, half the planes nested and half at document
root. Tests verify the exact archive SHA-256 before opening them; an independent
ZIP/XML check verifies object counts. The fixture contract and generator live
under tests/src/Gui/data and TreeBehavior.cpp. These are functional hierarchy
inputs with no image payload; they do not replace T001's BIM/Arch benchmark
inputs or certify workbench geometry.

| Criterion | Current evidence |
| --- | --- |
| Requirement 6 AC1/AC4 | Nine operation rows at each size: app open, empty-open control, direct/Python/delayed/zero-timeout restore, copy/paste, archive import and direct failure cleanup; extra failed-App-open control. Idle timers, no premature population under restoring guards, observer attachment, label/ShowInTree status delivery and no continuing ticks after completion |
| Requirement 6 AC2 | Mouse selection, deletion of a selected root object, another selection retained, Qt focus retained; group deletion keeps every child, reparents to document root, preserves view-provider lookup/map behavior and label observers at both sizes |
| Requirement 6 AC3 | Tree destruction/recreation before close/open at both sizes; current T014 lifetime/subscriber-count CTest rerun against this build |
| T007.1 interactions | Expansion/collapse state and label survive save/direct reload/close/open; controlled production DraggingState defers pending work then resumes; production QDropEvent move changes group membership and eventual tree parent/count at both sizes |
| T007.2 disposition | B011 remains deferred with local-developer ownership and a reactivation trigger; no further-optimization profile or bulk-delete bypass executed |

The full native xcb suite passes **25 functional cases** (27 Qt passes including
setup/cleanup), with zero failures/skips. Seven CTest targets pass: TreeBehavior,
TreeRestore, TreeWidgetLifetime, ObjectSelection, SelectionView, ImagePlane and
ImagePlaneRender. Full `pixi run --locked build -j 2`, scoped pre-commit and
whitespace checks pass. The focused Memcheck subset covers both sizes of
per-object deletion/selection/focus/reparenting, expansion/reload and drag/drop,
plus larger archive import: seven functional cases, nine Qt passes, zero access
errors; one existing exact libunwind startup-probe suppression. Leak and
undefined-value checks are disabled; no ASan/UBSan or leak-clean claim.

The interaction harness uses normal Gui application services, a macro manager,
MainWindow and active NoneWorkbench. The tree is a child in a panel, matching
production dock ownership. Initial harness attempts lacked those services or
used a top-level tree that lost activation when batching hid it; GDB/Qt state
identified those setup errors. The drop assertion now waits for scheduled tree
reparenting after document mutation. No production fix was needed. A preliminary
Memcheck attempt overlapped a harness rebuild and did not complete; its zero-error
summary is discarded. Final validation freezes and rechecks source/binary hashes
across all runs.

Native validation uses task-owned Xvfb, Metacity and a private D-Bus session;
CTest and Memcheck use Qt offscreen under Xvfb. The compose input-method profile
is explicit for native/Memory runs. The receipt binds actual diff/source/build,
seven clean dependency heads, 510 matching package identities, input/runtime/log
hashes and unchanged validation binaries. Internal-widget registration remains
non-Windows. T010 owns final production/platform/default-IBus and mixed-workbench
integration applicability.

Controlled drag state and the production drop handler qualify internal tree move
semantics; no cross-application OS drag gesture is claimed. No performance,
resource-budget, hardware-GPU or first-frame result is claimed. T004.3 retains
the predeclared tree comparisons, and B011 remains an owned deferred exploration.
Guarded updates complete T007.1 and its parent, retain T007.2/B011 routing, and
leave 17 complete, 21 pending and two follow-ups, with zero partial/in-progress
records and all 40 IDs retained. Package lint reports zero diagnostics; preflight
selects T015 with satisfied dependencies, zero context gaps and no open decisions.
The direct checker verifies exact input/source/binary/log hashes and unchanged
validation identities, 91 local links/anchors, all 15 prior completed task records,
frozen history/receipts/protocol and the unrelated console edit. Agent Workbench
remains navigation-only at its nested docs authority boundary; direct checks and
executed commands establish this evidence.
Next: T015 — Bound element-map numeric parsing and define compatibility.

## T015 Element-Map Numeric Restoration

T015 qualifies local Requirement 4 AC1/AC2 numeric behavior on root `09c9edc703`
/docs `a38de02`, plus the exact tracked/private-header diff and final App
build/runtime identities in [t015-validation.json](t015-validation.json).
Broader Requirement 4 and CP-005 identity acceptance remains T016/T005.

The [numeric policy](design.md#element-map-numeric-restoration-policy) preserves
legacy strtol values from the five old call sites at `8dc855b1948^`: explicit
decimal child IDs, hexadecimal mapped fields, signs, leading whitespace, optional
hex prefix, numeric-prefix/suffix tolerance, no-digits zero and range saturation.
Checked unsigned accumulation handles LONG_MIN without positive signed overflow.
The converter is in a private implementation header so the exact implementation
can be tested without loading unsanitized App/Qt libraries; no public API or
writer-format change is introduced.

The initial old-algorithm run failed seven of eight new cases; UBSan reproduces
signed addition overflow at LONG_MIN. Caller tests then reproduced an assertion
abort for indices outside IndexedName's nonnegative-int domain, two failed
range/count tests (with valid controls), and Memcheck found a freed-prefix-vector
read for a colon name at mapped index zero. Zero's expected formatting was
corrected in the fixture to Face, not Face0, before the final caller-count run.
The fixes reject unrepresentable indices/counts/ranges before narrowing or
allocation and compact colon names before the temporary prefix vector dies.
Saturated bad postfix/SIDs cannot wrap into valid aliases; existing warning and
ignore policies remain for those fields.

| Check | Final outcome |
| --- | --- |
| Decimal/hex boundary and tolerance oracle | Explicit long/int limits, signs, whitespace, prefix/suffix, embedded NUL/high byte, 4096-byte inputs; 2048 deterministic comparisons per base against std::strtol |
| Bounded storage | Non-terminated stack/heap views and empty one-past-end view; no conversion buffer allocation |
| Actual restore callers | Valid 0/INT_MAX, rejected mapped indices, saturated IDs/postfix, invalid child interval/count metadata; zero-index name read after prefix-table destruction |
| Focused current ElementMap tests | 30 cases pass normally and under Memcheck; zero access errors, one exact existing libunwind startup probe suppression |
| Broader regression | 277 App naming/hash/mapping cases and 290 matching CTests, including TopoShape element-map cases, pass |
| Exact-converter sanitizer probe | ASan/UBSan exit zero with no diagnostic, including 65536-byte tokens; old code produced UBSan overflow |

Full `pixi run --locked build -j 2`, scoped pre-commit and whitespace checks pass.
Final source and App binary hashes were frozen before validation and remain
unchanged. The App/test compilation uses Debug -g/-O0/FC_DEBUG with assertions
active; the library itself is not sanitizer-instrumented. The standalone probe
uses the same private header under ASan/UBSan; Memcheck tests actual restore with
leak/undefined-value checks disabled. No parser/name access is suppressed.
The receipt binds seven clean dependency heads, 510 matching package identities,
legacy/current source, compile/cache/runtime/library/log/probe hashes and its
reproducible probe text. Previous receipts/manifests/protocol remain unchanged.

The shared external COORDINATION.txt supplied by the user now records this work's
scope and status, preserving earlier investigation evidence. Its D600 native
geometry-read profiling request remains separate and unexecuted; no live
operator document, MCP mutation/restart, installed snapshot or external MCP
source/workflow was touched. No geometry-read/solver performance, native crash
resolution, Windows/LLP64 or final PR acceptance is claimed. T016/T005 retain
writer/alias/child/shared-map and dependent CAD reference qualification; T010
retains exact-final/platform applicability. Next: T016 — Qualify element-map and
dependent CAD reference identity.

Guarded completion leaves 18 complete, 20 pending and two follow-ups, with zero
partial/in-progress records and all 40 IDs retained. Lint reports zero diagnostics;
preflight selects T016 with satisfied dependencies, zero context gaps and no open
decisions. Direct checks verify exact source/diff/binary/log/probe identities,
93 links/anchors, 17 prior completed task records, frozen artifacts and original
line endings, plus preservation of the unrelated console edit. Agent Workbench
remains navigation-only at the nested docs authority boundary; executed checks
and direct evidence above establish qualification. The shared coordination file
has a dated T015 completion note; earlier content was preserved.

## T016 Element-Map And Dependent CAD Identity

2026-11-07: [t016-validation.json](t016-validation.json) binds root `b086a0bb2f` /
docs `7ff7e15`, the exact two-file C++ diff plus new Python harness bytes, final
source/executable/core/Part/PartDesign hashes, actual App compile command,
CMake cache/lockfile, seven clean dependency commits and 510 installed packages
matching the selected manifest. This is actual local working-tree evidence;
the immutable T019 intake and prior receipts remain unchanged.

Four new C++ cases in `tests/src/App/ElementMap.cpp` qualify:

- Real writer semicolon/colon/dollar-hashed output, IDs 11/13 above nine,
  multiple marked SIDs, postfixes and two aliases. Numeric field domains remain
  decimal child SIDs and hexadecimal mapped SIDs. Stream/token storage dies
  before lookup, followed by allocation churn and semantic save/restore/save/restore.
- A literal three-map fixture with a nested leaf and the same middle map shared
  by two root ranges. Recursive metadata, SID values, both lookup directions,
  aliases, four resulting elements and shared-pointer identity survive roundtrips.
  Child iteration is compared by postfix, not QHash order; no byte-order promise.
- Missing hasher and missing/nondigit/empty/negative child-SID handling. The
  missing-hasher case reproduced SIGSEGV (-11) before correction. A null guard
  now emits the existing hasher warning and omits unavailable SIDs while retaining
  child metadata, matching mapped-SID behavior. Valid writer output is unchanged.

The standalone harness
`tests/src/Mod/PartDesign/App/ElementMapDocumentIdentity.py` uses the existing
`TestModels/ModelFromV021.FCStd` (SHA-256
`b173cdc3cf25b50375c4779a459b90484586e42caa69a69c792b313766d627a5`).
It verifies Chamfer.Base still targets Revolution’s four top square edges,
independently checking their expected endpoints (x/y ±20, z 10) and length 40.
It also compares valid final shape, face count and volume, across two map-bearing
save/reopen cycles and three explicitly dirtied recomputes. Each saved archive
contains nonempty Revolution/Chamfer map entries and a StringHasher table;
output hashes and six snapshots are recorded in the receipt. The original
fixture hash remains unchanged. This is one legacy model’s target identity,
not general feature/workbench or byte-identical serialization qualification.

Final execution uses separate disposable FREECAD_USER_HOME/DATA/TEMP and XDG
paths per run. The CAD report records its selected user/system config files.
Exact argv, working directory, start/end times and log hashes are in the receipt
and `build/debug/artifacts/spec003-t016/commands.json`:

| Check | Result |
| --- | --- |
| `pixi run --locked build -j 2` after formatting | Passed |
| App naming/hash/element-map filter from T015, plus four new cases | 281/281 passed |
| `ctest --test-dir build/debug -R 'ElementMap\|IndexedName\|MappedName\|MappedElement\|ElementNaming\|StringHasher\|StringID' --output-on-failure` through Pixi | 294/294 passed |
| Standalone CAD harness through Pixi Python with built module on PYTHONPATH | Passed both save/reopen cycles and three forced recomputes |
| `valgrind ... --gtest_filter=*ElementMapRestoreNumberTest*:ElementMapTest.*` through Pixi | 34/34 passed; zero errors |
| Scoped pre-commit, source/diff/binary/fixture/log identity checks | Passed |

Memcheck uses the existing exact libunwind startup-probe suppression; one context
suppressed, leak/undefined-value checks disabled. No whole-application sanitizer,
Windows/LLP64, native geometry-read profiling or performance result is claimed.
An initial diagnostic module import used default settings and is excluded from
qualification. Initial CAD harness runs exposed import-namespace interference and
that `force=True` permits rather than dirties recompute; final imports run after
module initialization and explicitly enforce feature recompute. The latter raw
failure is retained. Red executable hashes were not captured; red source/argv/exit
and final source/binary identities are recorded without inferring image equivalence.

T016 completes this bounded local identity/correction slice. T005 is next to
reconcile all Requirement 4 criteria and CP-005 against T015/T016 evidence;
Requirement 4 stays partial-blocking until that checkpoint. T010 still confirms
final production composition/platform applicability.

Guarded completion leaves 19 complete, 19 pending and two follow-ups across the
40 stable IDs, with no partial/in-progress records. Package lint reports zero
diagnostics and preflight selects T005 with satisfied dependencies, zero gaps
and no open decisions. Direct checks preserve all 18 earlier completed task
blocks, immutable receipts/protocol/manifest, original line endings and the
unrelated console documentation diff. Agent Workbench remains stale/navigation
evidence only; executed commands and direct identities establish these results.

## T005 Aggregate Element-Map Parser Compatibility

2026-11-07: T016 committed as root `95d1df147f` / docs `e3bf027` after the
user’s commit instruction. [t005-validation.json](t005-validation.json) records
75 direct identity, integrity and applicability checks at that committed
composition and reconciles all Requirement 4 criteria and CP-005. T005 changes
spec/evidence only; it adds no runtime code and reports no fresh build/test run.

| Criterion | Accepted local evidence |
| --- | --- |
| Requirement 4 AC1: existing dot-entry/map semantics and format | T015 actual restore callers and numeric policy; T016 real writer semicolon/colon/dollar-hash forms, decimal child/hexadecimal mapped SIDs and nested/shared maps |
| Requirement 4 AC2: bounded, compatible numeric conversion | T015 unchanged exact production header with libc strtol differential cases, boundary/storage/caller tests and exact-header ASan/UBSan; all numeric/caller regressions rerun by T016’s final App/Memcheck checks |
| Requirement 4 AC3: aliases/SIDs/children and storage lifetime | T016 writerFormsAliasesAndMultipleSidsRoundTrip and nestedSharedChildrenAndDecimalSidsRoundTrip; semantic save/restore/save/restore, bidirectional lookups after stream destruction, SID metadata and shared-child identity |
| Requirement 4 AC4: dependent CAD targets across reopen/recompute | T016 checksum-pinned legacy ModelFromV021.FCStd: four independently specified Chamfer.Base edge targets, two map-bearing save/reopen cycles, three explicitly dirtied recomputes and six snapshots |

The exact T016 source/test diff is now committed unchanged. The audit verifies
all recorded final source and App/Base/Part/PartDesign binary hashes, 13 T016 raw
artifact hashes, retained T015 raw logs/probe, unchanged related naming/hash/build
sources, seven clean dependency revisions, 510 matching installed package
identities, lockfile/CMake cache and original/generated CAD fixture hashes.
The T015 numeric tests remain intact before the appended T016 tests. The sole
production delta after T015 is the four-line child-SID null-hasher guard; it does
not change the numeric converter, valid writer output, table bounds or prefix
ownership. Its missing-hasher and malformed-ID behavior is covered by T016.

This establishes applicability of T016’s executed full build, 281 App regressions,
294 CTests, 34 Memcheck cases and CAD round trips to the committed source. T015’s
exact-header ASan/UBSan applies to the unchanged converter; no whole-application
sanitizer or loaded-image equivalence is inferred. Fresh runtime reruns would
add no changed-source coverage here and are intentionally omitted. The input
receipts, commands and original counts remain immutable and retain their dates.

Local Requirement 4, CP-005 and F004 are complete. One Linux LP64 debug profile,
one legacy CAD model and the existing Memcheck suppression/checking limits do
not certify other platforms, final production composition or performance. T010
retains final applicability/platform checks, T004 owns comparisons and T011 owns
durable promotion. Phase 2C is complete; T018 archive failure recovery is next.

Guarded completion leaves 20 complete, 18 pending and two follow-ups across the
40 stable IDs, with no partial/in-progress records. Package lint has zero
diagnostics; preflight selects T018 with satisfied dependencies, zero context
gaps and no open decisions. Evidence-quality inspection classifies T005 as
concrete. Its three existing advisory warnings concern retained historical
verification rows/the table separator, which are not T005 acceptance evidence.
Agent Workbench remains bounded by the nested docs authority; explicit lifecycle
checks, source reads and the direct 75-check audit establish this completion.

## T018 Document-Level Archive Failure Recovery

2026-11-07: [t018-validation.json](t018-validation.json) records root `95d1df147f`
/docs `e3bf027` plus the exact test/CMake/shim diff and actual new configuration,
compile commands, binaries, libraries, seven clean dependency commits and 510
matching packages. T005’s uncommitted documentation/receipt is preserved.
Production PropertyFile.cpp/Reader.cpp bytes, public API and archive format remain
unchanged. The delegated compatibility decision preserves the existing source
policy in [the outcome table](design.md#archive-failure-compatibility-policy).
This is local compatibility qualification, not independent upstream approval.

`IncludedFileArchiveTest` executes full `Document::saveAs` / fresh target
`Document::restore` with two registered included files. It injects the first
failure and observes the live XMLReader FailedFiles list while the second entry
restores; it also checks document status, diagnostics, exact destination bytes,
permissions and payload-phase before/after notifications. Twelve rows cover
empty/nonempty success, read/write/close/create errors and preexisting read-only
files. Binary payloads use byte i = (i * 37) mod 256, sizes 0/131089 followed by
8193 bytes; generator/payload identities and each actual archive hash are retained
in the receipt and gtest XML. Temporary native archives are cleaned by the test.

Read errors are injected at the real payload streambuf boundary, after zero or
65536 bytes. Linux-only write/close interception is thread-local and restricted
to the exact temporary destination; the shim is linked solely to App_tests_run.
Write failures retain zero bytes in this injection; close failures retain all
known flushed bytes but return failure after releasing the descriptor. Neither
is a general hardware byte-retention guarantee. Fault-hit assertions establish
that read/write/close error rows were exercised. Empty payloads perform no write:
that row has zero write hits and is a successful empty-file control, not an
invented failure. The second entry restores correctly and read-only in every row.

Source-observed empty/nonempty asymmetry is retained: empty hook failures log
`Skipped empty embedded file`, remain absent from FailedFiles and emit no completed
payload notification; nonempty failures emit an error and enter FailedFiles.
Payload faults leave the partial destination writable and do not set the document’s
RestoreError/PartialRestore status; restore finishes and continues. Create errors
have no payload notifications and retain the directory obstacle. Existing read-only
payloads retain their sentinel with no copy notifications. XML path assignment
notifications occur earlier and are excluded from payload-phase counts.

Additional native checks establish that rejected external assignment retains value,
bytes/read-only state and emits neither notification, but updates OriginalName
before existence validation. Basename retention is also source-inspected. Two
missing-payload save cases (empty/nonempty) throw FileSystemError, preserve the
previous archive byte-for-byte and property path, and leave one UUID temporary
archive. T018 documents these existing behaviors without adding rollback/cleanup.

| Final check | Result |
| --- | --- |
| `pixi run --locked build -j 2` | Passed; expected Pivy stub-generation warnings |
| App included-file/matrix/missing-save plus existing naming/hash filters | 303/303 passed, including 22 included-file cases and 281 naming/hash regressions |
| `Base_tests_run --gtest_filter=ReaderTest.*` through Pixi | 26/26 passed |
| Focused included-file/Reader/naming/TopoShape CTests through Pixi | 342/342 passed |
| All 22 included-file cases under Memcheck | Passed, zero errors; one exact libunwind startup-probe suppression |
| Scoped hooks, source/binary/log/fixture identities, package/traceability checks | Passed |

Exact argv, working directories, isolation locations, times and gtest properties
are in the receipt and `build/debug/artifacts/spec003-t018/commands.json`. Each
run uses its own disposable FREECAD_USER_HOME/DATA/TEMP and XDG directories.
Memcheck disables leak/undefined-value checks; no whole-FreeCAD sanitizer claim.
The Linux shim uses dlsym/proc file-descriptor resolution and is not registered on
other platforms. Existing Base Reader tests are rerun; the new App fixture directly
covers the real document/reader boundary, so no Base Reader test-source change is
needed. No operator model/configuration or installed runtime is changed.

A reporting extension initially missed its QCryptographicHash include. A validation
pass was incorrectly started after that failed build and used the preceding
binary (302 App/341 CTest/21 Memcheck); it is explicitly discarded. Its raw logs,
commands, failure and not-qualified marker are retained under
`build/debug/artifacts/spec003-t018/discarded-reporting-build-failure/`.
The final rebuilt source/binaries are frozen unchanged across all accepted runs;
the receipt accepts only the final 303/26/342/22 results. The diagnostic collector
also uses the repository’s mutex-protected observer pattern.

T018 completes the local policy/failure matrix and metadata/save audit. Requirement
1/CP-001 remain partial-blocking until T002’s aggregate byte/ZIP/permission/
notification compatibility checkpoint. Final production/platform applicability,
performance and durable promotion remain T010/T004/T011.

Guarded completion leaves 21 complete, 17 pending and two follow-ups, retaining
all 40 IDs with no partial/in-progress records. Package lint has zero diagnostics;
preflight selects T002 with satisfied dependencies, zero context gaps and no open
decisions. Earlier completed task records, receipts/manifest/protocol, T005’s
uncommitted result and the unrelated console edit are preserved.

## T002 Included-File Compatibility Checkpoint

2026-11-07: T005/T018 committed as root `ad1bf7eac8` / docs `ee77d7f` under
the user’s commit instruction. [t002-validation.json](t002-validation.json) binds
that base, the exact one-file native-test diff and final source/build/runtime/
fixture identities. Production PropertyFile.cpp/Reader.cpp, existing Linux fault
shim and test CMake configuration remain unchanged; the T018 helper/test bodies
are retained verbatim and rerun on the final binary. The selected manifest,
seven clean dependencies and 510 package identities remain pinned.

T002.1 adds `archiveBoundariesAndNormalApplicationOpen`: a native VarSet document
contains four included files of 0, 65536, 65537 and 131089 bytes (byte i =
(i * 37) mod 256). It compares each ZIP entry’s declared size and complete bytes,
then closes the source document and uses normal `Application::openDocument`.
Every reopened payload must exist as a regular file, have its exact size/bytes
and be read-only, including the empty file. This covers the copy-buffer boundary,
one byte past it and multiple buffers without relying on equal-sized ZIP entries.
The output archive’s SHA-256 and generator/payload identities are in the receipt.
No byte-deterministic ZIP ordering or timestamp claim is made.

T002.2 adds six direct `IncludedFileHookTest` cases for empty/nonempty write,
close and bad-reader conditions. They assert Base::FileSystemError before payload
completion, payload before/after notifications, regular destination existence,
permissions and explicit output fault hits. Empty writes correctly attempt no
write and complete normally. Direct read-error cases begin with badbit; T018’s
full document rows separately exercise the actual payload stream read boundary.
All twelve T018 document-level rows are rerun, including live failure-list state,
diagnostics, partial bytes, permissions, document flags and correct later-entry
bytes. Existing metadata/missing-save checks and seven traversal-security cases
also remain in the included-file filter.

| Criterion | Current proof |
| --- | --- |
| Requirement 1 AC1 | Four raw ZIP/reopened-file byte/size controls, binary and multi-buffer payloads; current notifications/read-only controls and direct hook observability |
| Requirement 1 AC2 | Preserved T018 outcome table, twelve current full-document fault/control rows and six direct hook rows; partial bytes/permissions/notifications/diagnostics/failure lists verified |
| Requirement 1 AC3 | The real document reader continues after the first failure and restores the second payload byte-for-byte/read-only in all twelve rows; no hook-only inference |

| Final check | Result |
| --- | --- |
| `pixi run --locked build -j 2` | Passed; existing Pivy stub-generation warnings |
| Included-file and existing naming/hash App filter | 310/310 passed: 29 included-file cases plus 281 naming/hash cases |
| `Base_tests_run --gtest_filter=ReaderTest.*` through Pixi | 26/26 passed |
| Focused included-file/hook/Reader/naming/TopoShape CTests | 349/349 passed |
| All 29 included-file cases under Memcheck | Passed, zero errors; one existing libunwind startup-probe suppression |
| Scoped hooks, 45 integrity/applicability checks, package/links/preservation | Passed |

Exact argv, working directories, disposable FREECAD_USER_HOME/DATA/TEMP and XDG
paths, times, raw gtest properties and log hashes are in the receipt and
`build/debug/artifacts/spec003-t002/commands.json`. Final source/executable/core/
shim hashes were frozen unchanged across accepted execution. Earlier receipts
and completed task records remain immutable and dated; no new runtime result is
inferred from the commit alone. Memcheck disables leak/undefined-value checks;
no whole-application ASan/UBSan or loaded-image equivalence is claimed.

Requirement 1, CP-001, F006, T002.1 and T002.2 are complete locally. This is the
recorded Linux debug composition with the existing Linux-only fault shim, not
Windows/other-libc or hardware-failure qualification. T010 still owns exact-final
production/platform applicability and T011 durable promotion. All Phase 2 local
correctness subgroups are complete; next is T020 under the predeclared D001
performance contract. Correctness passes do not establish a latency/memory gain.

Guarded completion leaves 24 complete, 14 pending and two follow-ups across all
40 stable IDs, with no partial/in-progress records. Package lint reports zero
diagnostics and preflight selects T020 with satisfied dependencies, zero context
gaps and no open decisions. Earlier completed records, immutable artifacts and
the unrelated console edit remain preserved. Agent Workbench is navigation-only
at the nested docs boundary; executed checks and exact identities establish proof.

## Next Actions

1. **T011 — Promote durable documentation, audit residual ownership and close the spec.**
   T010 and all four children are complete for the declared Linux scope. Reconcile
   the closure tool's treatment of routed T008/T007.2 with owned B010/B011 destinations.
   Do not mark routed implementation work complete to satisfy a closure tool.
2. Complete the promotion/closure audit; B009/B010/B011 remain optional and inactive.
   No new source change or numeric performance promise is selected.

## Quality Gates

| Gate | Required? | Status | Evidence |
| --- | --- | --- | --- |
| Requirements and decisions accepted | yes | passed for Linux local scope | D003 owner selection; D007 dispositions; t010-results.md; t010-validation.json |
| MoE source-safety blockers resolved | yes | passed for qualified composition | Three independent reviews; final five-file patch; focused/native/full checks |
| Candidate and final evidence applicability | yes | passed for declared Linux runtime | Final v3 source manifest, build receipt, applicability and module hashes |
| Per-optimization performance/resource acceptance | yes | scoped D007 acceptance | Original v1 outcomes unchanged; reviewed directions/first-use/logical RAM; no current numeric timing claim |
| Complete task/criterion evidence | yes | T010 complete; T011 pending | tasks.md; traceability.md; t010-validation.json |
| Focused current correctness and failure tests | yes | passed locally | Final focused targets, CAD/native regressions and scoped Memcheck |
| Current measurements and platform/manual acceptance | yes | Linux local scope passed | Final runtime-bound manual checks and reviewed equivalence; other platforms/profiles unqualified |
| Durable promotion and residual ownership | yes | final audit pending T011 | Active reference/runbook; requirements.md durable impact |
| Review and closure metadata | yes | T010 reviewed; closure pending T011 | Whole spec remains draft |

## Evidence Log

| Date | Evidence | Result | Limits |
| --- | --- | --- | --- |
| 2026-11-04 | Imported report and commands above | Retained historical results | Not rerun during format migration; transient logs may be unavailable |
| 2026-11-06 | Direct source reads at d0644236e41f35a849b12a1e77941aa88354496f | Baseline implementation presence confirmed | Superseded for safety/acceptance by MoE findings; no runtime certification |
| 2026-11-06 | Five specialist reviews at root c69560af8a / docs 8d1abe9 | F001–F009 dispositioned into this package | Static evidence; no source correction or runtime reproduction performed |
| 2026-11-06 | T021: spec_open_decisions; task_context(T021/T001/T019/T012); lint_spec_package; direct documentation/link/dependency checks and root/nested-docs git diff --check | Passed: three open decisions with owners/blockers; zero context gaps/lint diagnostics; 8 artifacts, 45 links, 6 requirements/24 ACs, 40 tasks, acyclic parent/child dependency graph; F001–F009 mapped and historical evidence unchanged | Documentation-only completion. Temporary checker output is a bounded artifact, not a maintained runtime test. D001–D003 and source corrections remain open |
| 2026-11-06 | T019: direct Git/recursive gitlink cleanliness, file/tool/fixture SHA-256, CMake/ELF metadata, installed package normalization; spec_open_decisions; task_context(T019); lint_spec_package; direct document/dependency checks | Initial candidate manifest and D003 policy verified; seven dependencies, 510 packages, 43 file/tool hashes, nine fixtures, eight quarantined binaries, four planned recipes, 38 explicit task bindings | No new application build, runtime test or final production revision approval; actual execution receipts and D001/D002 remain required |
| 2026-11-06 | User-authorized execution replan; guarded task-state updates; lint_spec_package, active_spec_preflight, task_context(T001/T007), spec_open_decisions, backlog_items and direct phase/link/dependency/preservation checks | Four phases, T001 selected next, zero lint/context gaps, all 40 IDs retained, 98 links checked; baseline and optional routing separated from current execution | Documentation/routing only; source-safety fixes and runtime/performance/platform acceptance remain pending |

## T019 Validation Manifest

Initial local candidate: [validation-manifest.md](validation-manifest.md),
ID `spec003-linux-x86_64-debug-candidate-20261106-v1`, and [JSON snapshot](validation-manifest.json) SHA-256
`2ceaf3dbe3d474f4bb8d70ae6706218ddffdd3616473a1c13a55133611a19d01`. Captured root `f7fe2520c2edf25a17f3ad1fde084116cb1627d4` and docs `da79497b9b4ee447fbd8e5b0cde168af55b47118` with seven matching
clean recursive Git dependencies, 510 installed package identities, effective
Debug cache/compiler/tool metadata, nine hashed available document fixtures,
four unit-fixture sources and planned generated fixture recipes. Eight observed
executables/core libraries have hashes/ELF identities but remain historical,
source-unverified and ineligible for new acceptance until tied to a new build.

D003 is resolved for this delegated initial technical selection and invalidation
policy. This does not resolve D001 or D002, choose benchmark pass thresholds,
confirm an upstream PR base/final revisions, execute source fixes, rebuild or
certify any runtime/platform behavior. Rendered first-frame evidence still needs
the actual backend/renderer receipt. Planned generated fixtures are explicitly
unmaterialized with null output hashes, not completed regression evidence.

Every T019-dependent task has an explicit Candidate manifest field and must append an actual execution
receipt identifying new source/dependency/build/binary/fixture state. T010.1
checks final production applicability after any cherry-pick/rebase change;
affected automated/manual/archive/calibration/lifecycle/benchmark results must be
rerun or have explicit reviewed equivalence. Historical results stay dated.

Manifest validation passed: seven clean matching recursive Git dependencies,
510 installed package identities, 43 recorded file/tool hashes, nine available
fixture identities, eight explicitly ineligible historical binaries/libraries,
four unmaterialized recipe identities, and 38 explicit transitive task bindings.
MCP decision discovery lists only D001/D002 as open; task_context(T019) has no
traceability gaps or open-decision blockers. D003 initial selection is recorded
in the resolved ledger. Package lint has zero diagnostics. Direct link, dependency
and historical-preservation checks and root/nested-docs whitespace checks pass.

No application build/test, GUI or package installation was executed to complete
this initial inventory/policy task. Tool-version/help, Git/ELF metadata and
hashing commands are the executed evidence, not runtime behavior acceptance.

## Correctness Property Coverage

See traceability.md#correctness-property-coverage for each property, owning
tasks and validation direction. No property is fully verified by this format
migration; bounded conventional/unit/manual checks remain declared task gates.

## Scope Reconciliation Before Closure

The requirement-to-delivery matrix in traceability.md records every current
requirement, its task destination and partial-blocking/not-covered disposition.
Legacy SC identifiers in requirements.md are still gates. An existing baseline
implementation does not satisfy the proposed extension or its broader criteria.

## Execution Replan — 2026-11-06

User instruction: replan around planning, coherent correction/qualification,
comparative performance and final delivery. Stable task IDs and completed
T019/T021 evidence remain. The immutable validation-manifest JSON/Markdown and
all historical inputs are unchanged. No source correction, runtime test or
benchmark was performed by this replan.

The five old partial parent checkpoints had no completed current children and
mixed code-presence observations with outstanding expanded acceptance. Their
observations now live in [baseline-status.md](baseline-status.md); the remaining
qualification work is pending, not silently completed or discarded. Requirement
coverage remains partial-blocking until the actual required evidence exists.
Future partial markers need evidence-bearing completed portions of that task.

Optional work is explicitly owned: T009 records D002/B009 routing while retaining
full resolution; T008's original conditional GUI/startup investigation routes to
B010; T007.2's further-tree-optimization profile routes to B011. The latter two
are routed outcomes, not completed investigations. T011 checks ownership and
scope, not their future execution. T007.1 retains required current small/larger
model functional checks and T004.3 retains current tree performance evidence.

The graph now encodes the agreed serial delivery sequence so a first-ready-task
heuristic cannot bypass Phase 1 or leave an unfinished subgroup without an
explicit replan. T001 is pending planning work that resolves its own D001;
D001 continues to block Phase 2 and sampling until that task completes.
Replan checks passed: all 40 stable task IDs retained; four phases with four
coherent correction subgroups; 98 file/anchor links across 13 current documents;
acyclic required-child/dependency graph; every active source/acceptance task is
behind T001; no routed optional investigation is a core prerequisite. Original
T019/T021 completion evidence, all five baseline observations, immutable manifest
JSON/Markdown, historical inputs, existing B001–B008 rows and unrelated console
edits were preserved. Package lint has zero diagnostics; preflight selects T001,
with 35 pending tasks, three completed documentation/preparation tasks and two
routed records (counts include parents and children, not implementation percent).
D001 is the sole open active decision; backlog discovery recognizes B009–B011
as deferred with local-developer ownership and valid planning-schema/source paths.
No application build, source regression, benchmark or GUI validation was run.

## Documentation Validation Limits

Agent Workbench planned no validation commands across the nested docs repository
boundary; its receipt is navigation/context evidence. Explicit lifecycle queries
and direct checks above establish this documentation change's validation. No
FreeCAD source, binary, benchmark, sanitizer, GUI or platform acceptance was
executed. Runtime regression evidence for F001–F008 remains pending.

## Residual Risks

- F001/T012, F002/T013 and F003/T014 are locally corrected/qualified. T003 image
  fidelity, T006 lifecycle and T014 receiver lifetime pass on their recorded
  profiles. T007 now qualifies common tree operations on pinned 10/514-object
  documents. Mixed-workbench integration and final
  runtime/platform applicability, including default Qt/IBus startup diagnosis,
  remains T010; the T014 compose profile does not certify default IBus or T006.
- T015 numeric safety and T016 writer/alias/child/dependent CAD identity pass locally,
  including the reproduced/corrected missing-hasher child-SID path. Aggregate parser
  compatibility is complete through T005. T018 qualifies the preserved archive-level
  failure outcomes; T002 completes aggregate included-file compatibility locally.
  Final production/platform applicability remains T010.
- Dated tests do not certify the selected final production revision.
- Unresolved design/budget decisions block dependent implementation/acceptance.
- Current measurements, desktop/visual/platform coverage and durable promotion
  remain owned by local-developer through the tasks in traceability.md.

## Durable Promotion And Cleanup


Promotion destinations are proposed in requirements.md; do not publish intended
behavior as current. Preserve this package while criteria remain unresolved.
Before closure, resolve each coverage row, route residuals with owner, promote
lasting knowledge and update docs/history/spec-closure-log.md and the archive
index with actual final/cleanup commit references. No cleanup commit exists for
this migration.

## Historical Readiness Decision — Before T022/D005 Resolution

This checkpoint predates T004 implementation. For current readiness, see the
T004.2 conversion measurement disposition below and the active queue in tasks.md.

- Ready to implement: T022 decision/plan work only; T004 is blocked by T022/D005. T020 measurement/disposition is complete.
- Ready to validate: T020 evidence is complete and P07 is not accepted. New implementation/sampling needs T022/D005 scope, source composition and affected revalidation; whole-package acceptance still requires T004/T010.
- Ready to close: no.
- Ready to archive: no.
- Cleanup action: keep active, draft.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Open decisions](open-decisions.md)
- [Candidate validation manifest](validation-manifest.md)
- [Baseline implementation inventory](baseline-status.md)
- [Optional backlog routes](../../backlog/spec003-follow-ups.md)

## T020 Optimized Measurement Attempt And Recovery

This is the historical first-campaign checkpoint, before D004 selected the corrected
probe. At that point T020 required review/decision, with P07 acceptance incomplete. See
[performance-results.md](performance-results.md) and its hashed
[machine-readable evidence](performance-results.json). The fixed first campaign
attempted every predeclared image/command cell once. Six complete cells remain
bound to their original freeze; the hidden primary aborted with 24 completed
observations and is ineligible/unassessed. All-visible and PartDesign GUI timing
are inconclusive under D001's dispersion limit despite passing numeric limits;
VRML GUI passes. Three command cells are descriptive auxiliary measurements.
All three complete GUI resource comparisons pass. No P07 speedup is claimed.

Both matched optimized arms pass nine C++ suites, independent legacy map
save/reopen/forced-recompute, eighteen generated absolute resource rows and seven
runtime contexts each. All fourteen second-round qualifications against exact
frozen contexts pass. Pre/post-collection audits verify 6,055 identities each.
The natural VRML texture-completion/render endpoint and concrete execution freeze
were independently reviewed before sampling. Both arms retain full-resolution
calibration and Phase 2 safety. P06 is reviewed as subsumed/non-identifiable.

Reproducible commands and bounds:

- Build: locked RelWithDebInfo preset, optional image probe enabled; real provider
  compilation/Gui relink with ccache disabled and `-j 2`, separate eager/lazy arms.
- Complete GUI cells: `benchmark-runner.py --run <frozen execution> --out <new JSONL>`;
  assess with `benchmark-assess.py <execution> <observations> --out <assessment>`.
- Auxiliary cells: `benchmark-command-collect.py <frozen execution> --out <new JSONL>`.
- Synthetic assessment: `python test_benchmark_assess.py` (14 tests pass).
- Historical bounded runner: `python benchmark-runner.py --self-test` (passes).
- Failure observability: `python test_benchmark_image_collect.py` (6 tests pass).

The original runner discarded the failed worker's status/output/result. Available
kernel/coredump records show no relevant event; failure cause remains unclassified.
The tested new `benchmark-image-collect.py` extension preserves bounded diagnostics
only after failed-child cleanup and requires its own frozen hash/destination;
it has not been used for acceptance. Explicit diagnostic-only runs cannot replace
an acceptance cell. D004/T020 retain investigation, uncertainty disposition,
any justified profile/probe correction and a dated new freeze as remaining work.
T004/T010 stay gated; this checkpoint supersedes the earlier “T020 ready” handoff.

## T020 Corrected-Probe Qualification And Campaign Selection

This is the pre-collection checkpoint. The completed outcome and current next
action are recorded below; this section does not set current readiness.

D004 now selects the [corrected native-probe campaign](t020-recovery-2-plan.md).
Both recovery arms pass 551 focused tests, seven runtime qualifications each,
all fourteen repeat checks against exact frozen contexts, and six ownership
controls. Native output/source equivalence is recorded rather than claiming a
new C++ compilation. New acceptance remains a separate runtime gate.

The probe verifies its single owned document window independently of keyboard
focus. It completes/copies the first qualifying native swap in a direct GUI-thread
callback, preserving context and all touched GL/readback state. The native
framebuffer must use PartialUpdate. Logical image counters exclude the fixed
capture buffer; final process RSS includes observed driver/harness effects.

The protected qualification receipt is SHA-256
`a1ad40bbe6bc83c8761ef7776f08c93cdb44c406eb1d04e665e5c8579b2e49e9`.
All seven whole cells must start at pair zero under the same D001 numeric rules,
with one attempt each and no pooling with the original incomplete campaign.
Final concrete-freeze review and before-collection audit precede sampling.

## T020 Complete Measurement And Negative Optimization Disposition

All seven corrected-campaign cells completed once with 336 valid fresh-process
observations, 20 measured pairs per cell and no worker failure, retry, exclusion
or cross-campaign pooling. Both 42,290-hash audits pass. Four GUI cells have
literal D001 assessments; three command cells remain descriptive auxiliary data.
See [the aggregate](performance-results.md), [corrected results](performance-recovery-2-results.md),
[raw/statistical identities](performance-recovery-2-results.json), and
[completion receipt](t020-validation.json). Exact first-campaign result/report/
receipt bytes remain versioned; its original partial/inconclusive findings are
unchanged. No private locators, model contents or screenshots are published.

The corrected first-show median/p95 are 857.91/1843.09 ms, failing the 500/1000 ms
budgets. All-visible p95 and PartDesign median fail their non-regression limits;
other hidden timing and VRML remain inconclusive under the 0.10 variability rule.
All resource limits pass, but they cannot offset the timing failures. The control
outcomes do not establish a causal P07 regression amid high variability.

The measured lifecycle distinction is explicit: eager open materializes three
hidden textures; lazy open leaves three pending, first show materializes one,
and repeat show reuses it. Two unshown loads are avoided within this scenario;
first-show work is deferred, not universally eliminated. Full resolution,
calibration, pixel/reference identity and zero premature loads remain qualified.
Logical peaks are not summed and do not establish GPU residency. Fixed capture
renderbuffer driver effects remain part of observed final process RSS.

T020's literal measurement/disposition criteria (Requirement 3 AC3–AC4) are now
fulfilled, with **P07 NOT ACCEPTED**. This does not satisfy Phase 3 exit, SC-005,
T004 or T010. Material owner gate D005 and mandatory T022 are placed before T004,
because rework/behavior/requirements decisions may invalidate subsequent evidence.
No code behavior change, accepted tradeoff or threshold revision is chosen here.
Parser/tree T004.3 is not the primary P07 remediation owner. T022 must specify
any implementation and affected correctness/performance/measurement plan before
further comparisons; no backlog pointer or completed evidence checkbox waives D001.

## P07 Owner Retention Direction — 2026-11-08

The user explicitly agreed not to discard P07, valuing the apparent opening-time
improvement with hidden images and the observed RAM saving. Retention is the
selected direction. T022/D005 now defines reliable benefit validation and
appropriate first-use/tail targets, including repeat use and responsiveness.
No exact revised median/p95 budget was selected, no historical result is
reclassified, and no new performance run or production behavior change occurs
in this documentation update. Full resolution/calibration/safety remain required.

## T022 Plan And T004.1 Source Construction — 2026-11-08

Historical pre-build/pre-sample checkpoint. Current measurement/disposition is
in T004.1 Comparable Restore Measurement below; this section preserves its
original source-construction evidence and limits.

Committed T020 as root `95e99e22f2`, docs `2470275`. Scoped pre-commit hooks,
CRLF-aware diff checks and retained evidence/protocol hash checks pass. Concurrent
Sketcher and console work is excluded and preserved.

D005 resolves the technical plan in [t004-execution-plan.md](t004-execution-plan.md):
retain full-resolution lazy P07, keep D001 numeric criteria unchanged and bound
non-acceptance diagnostics/correction/revalidation. Independent `t004_plan_review`
reviewed the actual source/plan/artifacts; missing header identity and diagnostic
A/B definition were corrected, with no remaining blocker. T022 is complete for
planning only; P07's failed/inconclusive results and production limits remain.
Earlier dated T022/D005 blocker statements and machine completion receipts above
record their checkpoint; this section and the active ledger give current state.

The [readiness audit](t004-comparison-readiness.json) verifies eight public
fixtures, eight full historical delta hashes and eight affected source files
against corrected common `613def1ab6bf615f82fe163e3b41489d0228acbf` and committed
T020. Full-tree equivalence to current HEAD is not claimed; unrelated Sketcher
changes are excluded. Both P03 primary archives contain map-like entries/XML
nodes, but runtime parser-record coverage is still required before sampling.

T004/T004.1 now track source construction/qualification. The
[draft P01 receipt](t004-source-review/p01-arm-construction.json) binds scalar arm
`de308e8c6ea46e07580bc132f5be776e9827339b` and its exact patch/source hashes. It
changes one restore copy loop, retaining read/write/close checks and surrounding
policy. `git apply --check` passes; source review found no blocker. No source
worktree was changed, no new native worker/build/acceptance freeze or observation
was produced. Both-arm T002/T018 fault/byte/document outcomes, new shared hook
instrumentation, matched optimized builds and all five fixed P01 cells remain
required. The draft is not runtime-qualified or performance evidence.

Final checks for this slice: package lint reports zero errors/warnings; T022
state audit passes with zero findings; unresolved-decision discovery is empty;
T004.1 context has no gaps or open decisions. Direct hash checks confirm all
eight source identities and origin deltas, draft patch/source identities, retained
T020 raw/context/result hashes and immutable protocol bytes. Patch application
check and CRLF-aware scoped diff check pass. These are static/lifecycle checks,
not new builds, runtime passes or performance observations.

## T004.1 Comparable Restore Measurement

2026-11-08: measurement/disposition complete. [P01 results](t004-p01-results.md)
and [machine results](t004-p01-results.json) bind a non-self-referential full
published-evidence manifest, exact raw/context/assessment files and post-campaign
archive mappings. All five cells completed once with 240 successful fresh-process
observations and twenty measured pairs each. Both audits pass 39,805 hashes and
510 live installed-package checks; no retry, pooling, deletion or exclusion.

Both isolated locked RelWithDebInfo builds pass; each arm passes 31 included-file/
probe and 26 Reader cases, with all ten final runtime contexts qualified. Shared
fault-stream scalar support preserves exact archive failure policy. Required
completion recompute counts are fixed at 0/3/2/0/0. The normalized shared SQL
parser removes an unreviewed generated-output difference; its loaded path/hash
is enforced. Only the reviewed three native output files differ. Exact source,
CPP/Python equivalence, package/profile and generated-input identities are bound.

P01 is NOT ACCEPTED overall under unchanged D001. The primary passes (payload
median 513.92→26.79 ms; complete console open 1183.27→704.44 ms); EngineBlock
passes. BIMExample open p95 2784.98→3347.69 ms fails. ArchDetail and PartDesignExample
are inconclusive; all resources pass. The control tail's cause is not established.
D006 marks only two structural-zero payload metrics not_applicable, preserving
all five cells and other requirements. T004/local-developer owns the P01 residual.

Independent review recomputed all five assessments exactly and checked frozen
inputs, provenance, raw/context hashes, privacy and prospective task semantics.
At that T004.1 checkpoint its literal comparable-measurement criteria were fulfilled;
T004 remained in progress, T004.2 was next, and P07 remained retained/unaccepted.
The later T004.2 completion below makes T004.3 next. Phase 3/SC-005/T010
acceptance remains unsatisfied. Current aggregate schema v3 explicitly preserves
the original T020 aggregate/receipt and both campaigns; historical next-task fields
are not current lifecycle authority. No new production behavior or numeric waiver.

Final independent publication/lifecycle review found no remaining blockers: the
manifest covers all 31 evidence files, all hashes and archived bindings agree,
classification recomputation is exact, privacy passes and active task/aggregate
state is consistent. t004-p01-validation.json records final checks. Scoped hooks
and CRLF-aware diff checks pass; the original private fixture hash is unchanged.
The external chronological coordination file was appended with this scoped
result, explicitly preserving separate D600/live-model/crash investigation scope.


### Historical T004.2 Host Telemetry Preparation (2026-11-08)

Shared one-second host/owned-process telemetry is implemented. The preparation
report is [t004-p07-diagnostic-preparation.md](t004-p07-diagnostic-preparation.md);
[t004-telemetry-qualification.json](t004-telemetry-qualification.json) binds its
smoke and source. Fifteen focused Python tests pass, including cross-session
cleanup, identity races, unknown executable identity, explicit raw counter units
and terminal failure evidence. These are harness checks using Python surrogates,
not new FreeCAD observations. At that preparation checkpoint native timing/build
qualification, the fixed 16-worker P07 diagnostic and P04/P05 comparisons were
pending. They subsequently completed and are documented below. Verified storage
retirement reclaimed 16.54 GiB and resolved the space blocker; see
storage-retirement-20261108.json and the storage runbook. T004.2 was then in
progress; it has now completed measurement/disposition. P01/P07 acceptance and
original evidence remain unchanged.


### Benchmark Storage Retirement (2026-11-08)

User-requested cleanup retired exactly two redundant compiler caches and four
completed T020 output trees after independent retention review. All 9,732 retiring
output-manifest entries matched before removal. All 1,155 retained evidence-file
hashes, 3,788 P01 output hashes/inodes and six source checkouts/refs were verified
afterwards. The six targets are absent; 17,755,156,480 allocated bytes were
reclaimed. Normal debug development and the user compiler cache were untouched.
T020 live native replay/audits now require reconstruction; original evidence and
acceptance classifications did not change. The dated retirement record is separate
from immutable campaign receipts.


### T004.2 P07 Corrected Diagnostic

2026-11-08: the corrected, independently frozen exploratory run completed16fresh
GUI workers across hidden/visible/PartDesign/VRML contexts, AB thenBA percontext.
All normative model/payload/calibration/frame/profile/lifecycle/runtime checks
passed. Both full39,985-binding audits passed; zero worker failures, retries,
replacements, exclusions or pooling. Finalnativequalification:408cases, zero
failures/skips. Source branches differ only in the reviewed eager/lazy condition;
all other App/Gui objects matched after proven shared RCC timestamp input
normalization and independent no-cache recompilation.

[Results](t004-p07-diagnostic-results.md), [machine data](t004-p07-diagnostic-results.json),
[validation](t004-p07-diagnostic-validation.json) and the sanitized evidence manifest
record source/freeze identities, all16descriptive rows and198hostsamples. Raw
absolute clocks, owned PIDs, local traces and fixture locators remain private.

Prior attempts remain separate: zero-GUI unsupported-Pixi-observer preflight; one
closed eager worker with stale mapped-resource expectations and15unexecuted;
two bounded forensic qualification processes with no timing values accepted or
reported. Forensics found only Arch_rc.rcc/Draft_rc.rcc additions, both qualified
hashes, no missing or changed original native mappings. The correction retained
strict equality and all old fidelity witnesses; no library was ignored.

P07firstshow is primarily read/decode (~80% in two lazy observations); hidden open
and memory benefits remain descriptive. Control variation and median28.6ms
collectorCPU/sample prevent causal claims about old BIMp95 or acceptance. P01/P07
remain unaccepted. At that diagnostic checkpoint T004.2 remained in progress for
P04/P05 comparisons; the completed conversion disposition below supersedes that
queue. No original protocol, assessor, result or acceptance receipt changed.

## T004.2 Conversion Measurement Disposition

2026-11-08: T004.2 completes comparable measurement/disposition under the
prospectively reviewed conversion plan, preserving D001 and all prior evidence.
All 13 required P04/P05 cells completed: 624 fresh workers, 520 measured
observations (20 pairs/cell), 26 first-request diagnostics and 78 discarded
warm-ups. No worker failure, retry, replacement, exclusion or pooling. P04 pre/post
31,256-binding and P05 pre/post 31,350-binding audits pass. Per-observation source,
output, full pixel/alpha/orientation, ownership and runtime checks pass.

Matched locked RelWithDebInfo builds independently recompile the affected objects;
App/common objects and probe executable match, sole Gui object delta is
BitmapFactory.cpp.o and sole scoped runtime delta libFreeCADGui.so. P04 passes
148 native cases/arm. P05 current candidate passes 148; its historical comparator
passes 5 shared direct/null, 24 lifecycle and 12 render/calibration cases, plus
full-image qualification for every one of its ten fallback cells. The comparator
is not a production implementation passing the complete current suite. Total
485 scoped native cases; all 56 focused Python tests pass under selected host
Python -I -S -B -OO (optimization2), zero failures/errors/skips.

P04 has descriptive 8–10x median conversion gains with resource passes, but
original dispersion rules make timing inconclusive. P05 records all per-cell
improvement/non-regression/resource decisions, including compact grayscale
expansion and temporary RGBA storage. See t004-conversion-results.md/json and the
hash-bound sanitized evidence/validation receipt. No failed gate is waived.

P04 retains its exact frozen controller; explicit operator launch and independently
rechecked live executable/hash/argv/start identity provide reviewed Req3 AC5
provenance equivalence. P05 self-checks exact interpreter/pidfd/subreaper, captured
activation/closure/tools, exact worker argv and complete writable state before
artifacts. Original unlaunched pre-isolation drafts/qualification remain archived.
Aggregate host telemetry is captured between workers; its measured cost and load
ranges are published, without claiming idle-host or old BIM-p95 causation.

The immutable T020 GUI opening/hidden-first-show/repeat/visibility/materialization
and resource results, plus the corrected exploratory P07 diagnostic, remain
separate. No primitive conversion timing is a GUI timing. P01/P04/P05/P07 remain
unaccepted overall. Parent T004 owns every numeric residual; next T004.3.
T010 retains final integrated-source, full-suite/desktop/platform applicability.

## D007 Cost-Benefit Policy Revision

2026-11-08: the owner explicitly selected relaxed criteria emphasizing cost-benefit
after seeing P04/P05. acceptance-policy-v2.md/json now governs product decisions;
cost-benefit-review.md/json is a separately labeled retrospective direction review.
All prior v1 protocols/assessors/raw/result/validation labels remain unchanged.
Historical statements above requiring every D001 numeric gate describe those
checkpoints, not current D007 product approval rules.

Preserve hard correctness/fidelity/calibration/model/error/provenance/coverage gates.
Dispersion/latency/resource breaches instead require a record of absolute cost,
benefit, confidence/tails, frequency, retained/transient memory, concurrency/scope,
rationale/mitigation and residual owner. The new direction retains P04/P05/P07;
no blanket memory/latency approval or predeclared statistical pass is created.

Exact pre-D007 mutable authority bytes hashed by t004-conversion-validation.json
are preserved in d007-prior-authority/manifest.json (.txt snapshots). New policy
verification uses d007-validation.json, not edits to the old completion receipt.
No new native build/test/benchmark is needed for this documentation-only policy
change. T004.3 freezes D007's future review plan; T004.4/T010 own actual workflow
and final applicability questions. Original P07 n=2 observations remain descriptive.


## T004.3 Parser Save Measurement and Tree Exclusion

T004.3 completes the measurement/disposition slice described in
[t0043-results.md](t0043-results.md), [machine results](t0043-results.json),
[the public evidence manifest](t0043-evidence/manifest.json) and
[t0043-validation.json](t0043-validation.json). Accepted fixed campaigns contain
240 P03 and 144 P02 fresh workers (320 measured, 48 warm-up, 16 diagnostic), with
zero accepted worker failures/retries/replacements/pooling. Pre/post live audits
pass 31,647 P03 and 32,199 P02 bindings. The same optimized pair is reused; P03
native replay is explicitly retired before P02 replacement, with source refs,
raw records, source/policy capsules and complete evidence seals retained.

Both variants qualify exact original/reopened geometry-manifest, persistent-object,
payload/declaration/calibration and runtime identities. Every PartShape property
and checked geometric reference participates, including additional/null shapes.
The earlier seven-document semantic oracle remains unchanged under the additive
manifest repair; independent CAD review confirms its bounded serialization allowance.
The original P03 verifier attempt stops at 135 completed rows and is retained as
ineligible, never pooled, with passing unchanged-input/runtime audit at abort.
Prior namespace/raw-BRep/null/compound/timeout construction failures remain
qualification diagnostics, not benchmark outcomes. Detailed scope and recovery are
in the frozen execution plan and report.

The accepted source builds use matched Linux RelWithDebInfo and locked dependencies,
identical canonical prefix and opt-in shared tracing. Only ElementMap.cpp.o (P03)
or PropertyFile.cpp.o (P02) differs in App; GUI objects/loadable sections match.
The scalar save's exact versioned get(char&) import/provider is qualified; public
defined ABI and all source error/ownership behavior are preserved. Full native
regression filters pass 91 cases per arm for each comparison. Focused Python
fidelity/collector tests pass 20 cases under both normal and optimized interpreters.
Normal debug output and external operator runtimes are untouched.

Original statistical diagnostics stay intact: the two primary parser hooks pass,
but whole-open confidence intervals cross zero, median RSS rises about 1.7–2.0 MiB,
and Draft remains an overall legacy failure (tiny-hook dispersion and open-p95
regression). Buffered save's primary hook/full-save outcomes and control variability
are recorded separately. No numeric label is rewritten under D007. P08 has an
independently reviewed current-effect attribution exclusion preserving T014/T017/
T007 behavior, with no independent speed claim and T004.4 as its sole residual
owner; B011 remains inactive. T004.4 is next for actual workflow cost-benefit and
operating-scope dispositions; T010 retains final composition/platform applicability.

## T004.4 Workflow Cost-Benefit And Operating Scope

[t0044-cost-benefit-review.md/json](t0044-cost-benefit-review.md) records every
D007 required field for P01–P08, with local-developer ownership. P05 many-image
logical accounting and selected bounded inventory are explicit; P07 uses full
20-pair tails and keeps exploratory n=2 decomposition separate. P06/P08 adopt
reviewed attribution exclusions without standalone benefit claims. Unknown
population frequency is handled by selected workflow scope; BIM host cause is
not inferred. Hard-gate evidence remains scoped to its qualified receipts.

[t0044-validation.json](t0044-validation.json) binds the completed review,
independent CAD/performance/operations-document reviews, preserved source
artifacts, eight exact prior-authority snapshots and document/accounting checks.
No source changes, native tests or new benchmark samples were needed for this
review. Frozen D007/prior review/results/receipts are unchanged. Lasting scoped
decisions are promoted to the reference and runbook; final promotion/closure
remains T011. T004/T004.4 local acceptance is complete. Next T010.1; SC-005 and
whole-spec final production acceptance remain open.

## T010 Final Linux Composition And Review

T010 and all four children are complete for the declared Linux scope; next T011 promotion/closure.

[t010-results.md/json](t010-results.md), [the final source manifest](t010-final-source-manifest.json),
[build receipt](t010-build-receipt.json), [applicability matrix](t010-applicability.json)
and [validation](t010-validation.json) identify base `b5f1581921` plus the exact
five-file corrective patch. Source selection changed only after actual blockers
were observed; earlier failed and green attempts remain distinct evidence.

The final patch restores finite loft edge admission, initializes the real Measure
test module, defers image loading during App-only restore and makes preview hide
idempotent. The two new App-only ready/late rows fail old code and pass the fix.
Current native stage traces show six empty initialization calls, one nonempty
first-show load, zero App-restoring loads, zero image/preview warnings and a
successful document-owned viewer close. A retained Start tab is not a leaked
model window. Full-resolution pixels, calibration and hidden0→show1/leaves2
are verified on the final module closure. Scoped Memcheck passes26 Qt cases
with zero access errors and only the existing libunwind suppression.

Earlier viewer runs loaded user-site NumPy despite environment flags and are
ineligible. Accepted GUI runs mask that directory in an owned namespace and bind
actual mapped modules to the final build/patch. Generic/Fusion+compose is the
native viewer/memory profile. Windows/macOS/Wayland/other GPUs/drivers and
GNOME/IBus memory qualification remain unqualified under the user's Linux-only
selection. No source or user package was altered to create this isolation.

P01/P07 historical external Python DSO closure is unknown; their absolute,
tail, whole-open and RSS figures are historical, not final-composition proof.
Old premature-load counting omitted App-only restore. Keep the old results and
labels unchanged; the corrected source and new proof establish the current
hard gate. P02/P03 isolated workers and full maps exclude that runtime concern.
D007 retains local directions and the hidden/subset first-use/logical-memory
tradeoff without a numeric SLA or exact-final timing claim. Numeric production
claims would require a newly frozen masked-runtime campaign.

Seven original campaign fixture identities match; exact reopened campaign
identities exist for three P02 fixtures. Other roundtrips are separately checked
semantically. Core binary hashes remain exact across the last GUI-only changes.
Assembly uses an explicit sphere-only supplemental oracle with eight negative
checks; the original seven campaign oracles remain unchanged. See the reports
for bounds, prior-run disposition and skips/disabled coverage limits.

The full task uses the repository's exact CTest graph after locked Pixi
activation. Nested Pixi probing fails in the namespace before tests; the final
lower-level route runs the same `ctest --test-dir build/debug` body with reporting
flags only. No testcase filter/exclusion or lock change is introduced.

The public viewer capture contains a generated scene only. Independent CAD,
performance/statistical and operations/document reviews are recorded in the
validation receipt. T011 still owns final promotion audit and whole-spec closure.
