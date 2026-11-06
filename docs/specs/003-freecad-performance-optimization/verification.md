---
title: Performance optimization reconciliation
doc_type: spec
artifact_type: verification
status: code-derived
owner: local-developer
last_reviewed: 2026-11-06
---

# Performance Optimization Reconciliation

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
F003 remains a source blocker. T003 now qualifies local Requirement 2 fidelity;
T006 now qualifies local Requirement 5 lifecycle. Requirement 6 and exact-final/platform acceptance remain pending.

## MoE Review Findings

| ID | Severity | Observed evidence / consequence | Owner / task destination | Disposition |
| --- | --- | --- | --- | --- |
| F001 | blocker | BitmapFactory.cpp:64–98 allocates using narrowed SbVec2s dimensions but writes original dimensions; inspected Linux 65537x1 becomes a 1x1 allocation. Null/extent/arithmetic policy was missing. | local-developer; T012 then T003 | T012 correction plus T003 current format/render/access qualification pass; final T010 applicability remains |
| F002 | blocker | Gui/Document.cpp:2170 clears GUI restoring before finishRestoring; ViewProviderImagePlane.cpp:215–251 can write DPI-derived XSize/YSize during deferred load. Non-default calibration can be lost. | local-developer; T013 then T006 | T013 calibration correction plus T003 fidelity and T006 expanded lifecycle pass locally; T010 final applicability remains |
| F003 | blocker | Tree.cpp:696–700 captures raw this in application callbacks; destructor at 762–775 omits those connection disconnections; fastsignals::connection destructor is unscoped. | local-developer; T014 then T007 | Open receiver-lifetime fix and destroy/recreate/open regression |
| F004 | request | ElementMap.cpp:89–105 changes numeric conversion as well as token splitting, with unchecked signed arithmetic; current parser fixtures do not cover all writer/alias/radix/child identity forms. | local-developer; T015, T016 then T005 | Open bounded numeric policy/implementation and CAD identity fixtures |
| F005 | request | Tree suppression uses broad isRestoring while completion follows application finish-open; idle direct load/restore/import/paste can lack a matched completion. | local-developer; T017 then T007 | Open scheduling contract and idle completion/error tests |
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
| Receiver lifetime | Actual tree destroy/recreate then start/finish-open; callbacks and queued work cannot access dead receiver | T014, T007 | Pending source fix and execution |
| Numeric and subelement identity | Signed/unsigned-form tolerance, decimal/hex bounds, malformed/overflow tokens; semicolon/colon/dollar writer forms; multiple aliases/SIDs/shared children; bidirectional lookups after buffers die; dependent CAD target reopen/recompute | T015, T016, T005 | Pending policy, source correction and fixtures |
| Restore completion | Initially idle timers; app open, direct load/restore, import/paste and failure cleanup; eventual tree population/status/observer work without unrelated edit/open | T017, T007 | Pending scheduling correction and execution |
| Archive failures | Empty/nonempty failing first entry plus valid second; read/write/close injection, diagnostics/failure list, partial bytes/permissions and notifications | T018, T002 | Pending approved outcome table and execution |
| End-to-end performance/resources | Pinned command restore, GUI first-correct-frame, hidden-first-show and repeat-show endpoints; visibility/texture counts; process/decoded/Coin peaks for direct and grayscale/alpha fallback fixtures | T020, T004 | Protocol/initial policy selected; measurements pending |
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

## Next Actions

1. **Next: T014 — Bind tree application callbacks to widget lifetime.**
   Phase 2A image correction/qualification is complete; retain D001/T019 applicability rules.
2. Follow [the explicit Phase 2 subgroups](tasks.md#execution-order): images,
   tree lifecycle, CAD maps, then archives; finish each fix's focused regression
   and subgroup qualification before advancing.
3. Run T020/T004 comparative latency/memory/per-change evidence under the approved
   protocol, then T010 final applicability/platform gates and T011 promotion.
4. B009/B010/B011 are owned deferred follow-ups; no optional investigation must
   execute to close the core slice, and none may waive a current safety check.

## Quality Gates

| Gate | Required? | Status | Evidence |
| --- | --- | --- | --- |
| Requirements and decisions accepted | yes | partial | D001 protocol recorded; D002 routes to B009; D003 initial policy recorded; final T010.1 confirmation pending |
| MoE source-safety blockers resolved | yes | blocked | F001/T012 and F002/T013 locally qualified; F003 correction and broader regression matrix remain |
| Candidate and final evidence applicability | yes | partial | T019 initial manifest/policy recorded; actual new-build/runtime receipts and T010 final reconciliation pending |
| Predeclared per-optimization performance/resource acceptance | yes | pending | T001 protocol complete; T020/T004 and final T010 evidence pending |
| Complete task/criterion evidence | yes | pending | tasks.md; traceability.md |
| Focused current correctness and failure tests | yes | pending | T003/T006 current image fidelity/lifecycle pass; T002/T005/T007 qualification remains pending |
| Current measurements and platform/manual acceptance | yes | pending | Historical observations are insufficient |
| Durable promotion and residual ownership | yes | pending | requirements.md durable impact |
| Review and closure metadata | yes | pending | No closure claimed |

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

- F003 remains an implementation acceptance blocker. F001/T012 and F002/T013 are
  locally corrected/qualified; T003 image qualification passes on its recorded
  software-GL profile; T006 local lifecycle qualification passes. Final runtime/platform
  applicability, including Qt/IBus Memcheck startup diagnosis, remains T010.
- Malformed numeric tokens, archive-level error outcomes and broader restore paths require
  explicit compatibility decisions and regression evidence.
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

## Readiness Decision

- Ready to implement: T014 — Bind tree application callbacks to widget lifetime; Phase 2A and all image children complete.
- Ready to validate: local image slice qualified by T003/T006; T014 implementation comes next. Whole-package acceptance remains blocked by F003 and other checkpoints; final/platform applicability remains T010.
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
