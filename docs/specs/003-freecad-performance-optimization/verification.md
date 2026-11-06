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
not rebuild source or run `pixi run test`.

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
criteria. Requirement 2/5/6 acceptance remains blocked by F001/F002/F003 until
source correction and meaningful regression checks exist.

## MoE Review Findings

| ID | Severity | Observed evidence / consequence | Owner / task destination | Disposition |
| --- | --- | --- | --- | --- |
| F001 | blocker | BitmapFactory.cpp:64–98 allocates using narrowed SbVec2s dimensions but writes original dimensions; inspected Linux 65537x1 becomes a 1x1 allocation. Null/extent/arithmetic policy was missing. | local-developer; T012 then T003 | Open source fix and boundary/safety regression |
| F002 | blocker | Gui/Document.cpp:2170 clears GUI restoring before finishRestoring; ViewProviderImagePlane.cpp:215–251 can write DPI-derived XSize/YSize during deferred load. Non-default calibration can be lost. | local-developer; T013 then T006 | Open source correction and visible/hidden/import calibration regressions |
| F003 | blocker | Tree.cpp:696–700 captures raw this in application callbacks; destructor at 762–775 omits those connection disconnections; fastsignals::connection destructor is unscoped. | local-developer; T014 then T007 | Open receiver-lifetime fix and destroy/recreate/open regression |
| F004 | request | ElementMap.cpp:89–105 changes numeric conversion as well as token splitting, with unchecked signed arithmetic; current parser fixtures do not cover all writer/alias/radix/child identity forms. | local-developer; T015, T016 then T005 | Open bounded numeric policy/implementation and CAD identity fixtures |
| F005 | request | Tree suppression uses broad isRestoring while completion follows application finish-open; idle direct load/restore/import/paste can lack a matched completion. | local-developer; T017 then T007 | Open scheduling contract and idle completion/error tests |
| F006 | request | PropertyFile copy errors are caught by XMLReader::readFiles; hook exceptions alone do not establish diagnostic/failure-list/partial-file/continuation behavior. | local-developer; T018 then T002 | Open archive-level outcome table and failure fixtures |
| F007 | request | Deferred image decode/conversion moves into show; fallback may expand 1/2-component sources to RGBA. Open-only timings can hide first-frame latency or image-memory effects. | local-developer; T020 then T004 | Open comparative frame/show/repeat latency, texture counts and peak memory evidence |
| F008 | request | Candidate/final revision selection, per-change coverage and benchmark pass rules were insufficiently connected to acceptance; final task assembled timings without explicit evidence invalidation. | local-developer; T019, T001, T004.3, T010 | Open pinned manifest/protocol, per-optimization evidence and exact-final applicability |
| F009 | suggestion | spec_open_decisions returned no records for embedded D001–D003, while task_context retained references; advisory ready packets describe context availability. | local-developer; T021 | Documentation ledger/guardrails verified by T021; D001–D003 remain open |

All findings stay in this active package because they change its existing
contracts and acceptance. No source finding is merely routed to an unowned
backlog. Expert subsidiary recommendations are included: hashed/alias/shared-map
identity, per-optimization attribution, explicit noise/non-regression rules,
peak CPU/Coin image storage and applicability of manual evidence. Driver-specific
GPU residency claims require separate actual measurements.

## MoE Regression Matrix

| Gate | Required fixtures / outcome | Owner tasks | Evidence status |
| --- | --- | --- | --- |
| Image extent/arithmetic | Null, valid edge and slender 32767/32768/65537 inputs; safe documented rejection, checked offsets; sanitizer-equivalent checks where available | T012, T003 | Pending source fix and execution |
| Calibrated CAD planes | Independently non-default XSize/YSize and placement; visible/hidden reopen, hidden import/paste/show, repeated show, save/reopen; preserve new-image/explicit-resize behavior | T013, T006 | Pending source fix and execution |
| Receiver lifetime | Actual tree destroy/recreate then start/finish-open; callbacks and queued work cannot access dead receiver | T014, T007 | Pending source fix and execution |
| Numeric and subelement identity | Signed/unsigned-form tolerance, decimal/hex bounds, malformed/overflow tokens; semicolon/colon/dollar writer forms; multiple aliases/SIDs/shared children; bidirectional lookups after buffers die; dependent CAD target reopen/recompute | T015, T016, T005 | Pending policy, source correction and fixtures |
| Restore completion | Initially idle timers; app open, direct load/restore, import/paste and failure cleanup; eventual tree population/status/observer work without unrelated edit/open | T017, T007 | Pending scheduling correction and execution |
| Archive failures | Empty/nonempty failing first entry plus valid second; read/write/close injection, diagnostics/failure list, partial bytes/permissions and notifications | T018, T002 | Pending approved outcome table and execution |
| End-to-end performance/resources | Pinned command restore, GUI first-correct-frame, hidden-first-show and repeat-show endpoints; visibility/texture counts; process/decoded/Coin peaks for direct and grayscale/alpha fallback fixtures | T020, T004 | Pending D001/D003 and measurements |
| Exact final evidence | Per-optimization inventory for restore/save/parser/conversion/lazy/tree; predeclared pass rules; final-revision automated/manual/archive/lifecycle/benchmark reruns or reviewed equivalence | T019, T001, T010 | Pending manifest, owner decisions and execution |
| Decision discovery | MCP exposes D001/D002/D003, owners and blockers; task contexts agree; context readiness does not overrule status/dependencies | T021 | Passed documentation/tool checks; no product decision resolved |

## Next Actions

1. T021 — Expose lifecycle decisions and verify execution guardrails is complete; use the verified ledger.
2. Resolve D003/T019 candidate validation provenance, then D001/T001 benchmark rules.
3. Reproduce and correct F001/F002/F003 in T012/T013/T014 on named source branches;
   complete T015–T018 numeric/identity/archive/restore-path qualification.
4. Execute dependent correctness matrices and T020/T004 latency, memory and per-change comparisons.
5. Reconcile all affected evidence against exact final revisions at T010 before promotion/closure.

## Quality Gates

| Gate | Required? | Status | Evidence |
| --- | --- | --- | --- |
| Requirements and decisions accepted | yes | pending | open-decisions.md; D001/D002/D003 remain open |
| MoE source-safety blockers resolved | yes | blocked | F001/F002/F003; T012/T013/T014 and regression matrix |
| Candidate and final evidence applicability | yes | pending | T019 manifest, D003 and T010 invalidation/rerun decisions |
| Predeclared per-optimization performance/resource acceptance | yes | pending | D001/T001, T020/T004 and final T010 evidence |
| Complete task/criterion evidence | yes | pending | tasks.md; traceability.md |
| Focused correctness and failure tests | yes | partial or pending | Dated checks above; selected revision still needs acceptance |
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

## Correctness Property Coverage

See traceability.md#correctness-property-coverage for each property, owning
tasks and validation direction. No property is fully verified by this format
migration; bounded conventional/unit/manual checks remain declared task gates.

## Scope Reconciliation Before Closure

The requirement-to-delivery matrix in traceability.md records every current
requirement, its task destination and partial-blocking/not-covered disposition.
Legacy SC identifiers in requirements.md are still gates. An existing baseline
implementation does not satisfy the proposed extension or its broader criteria.

## Documentation Validation Limits

Agent Workbench planned no validation commands across the nested docs repository
boundary; its receipt is navigation/context evidence. Explicit lifecycle queries
and direct checks above establish this documentation change's validation. No
FreeCAD source, binary, benchmark, sanitizer, GUI or platform acceptance was
executed. Runtime regression evidence for F001–F008 remains pending.

## Residual Risks

- F001/F002/F003 remain implementation acceptance blockers; source changes and runtime
  regressions are not completed by adding tasks or strengthening prose.
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

- Ready to implement: only a selected dependency-safe correction slice after owner decisions; no package-wide approval.
- Ready to validate: whole-package acceptance blocked by F001/F002/F003; bounded reproductions may run on pinned candidates.
- Ready to close: no.
- Ready to archive: no.
- Cleanup action: keep active, draft.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Open decisions](open-decisions.md)
