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

## Requirements And Acceptance

| Requirement | Current disposition | Evidence / remaining gate |
| --- | --- | --- |
| FR-001; SC-001 | Implemented; focused validation passes | `PropertyFileIncluded::RestoreDocFile()` in `src/App/PropertyFile.cpp` uses bounded reads and checks read/write/close errors, preserves notifications and read-only destination handling. `XMLReader::readFiles()` in `src/Base/Reader.cpp` preserves entry boundaries. Full archive compatibility matrix remains a broader gate. |
| FR-002 | Implemented; visual acceptance partial | `convertDirect32BitImage()` and `BitmapFactoryInst::convert()` in `src/Gui/BitmapFactory.cpp` preserve vertical flip and RGBA order. Four bitmap tests pass. Premultiplied/uncommon formats use Qt conversion to RGBA8888; they do not use the direct scanline path. Full image-plane color/alpha/scale/ownership matrix remains open. |
| FR-003; SC-002 | Historical measurements recorded; current measurements unverified | May 22 before/after observations remain in tasks/research. No current revision performance comparison was run. |
| SC-003 | Unverified | Local evidence exists; exact proposed upstream PR descriptions and their validation packages were not reviewed. |
| Acceptance 1 | Correctness partially verified; speed requires comparable rerun | Binary roundtrips passed; no current large-document benchmark. |
| Acceptance 2 | Partial | Unit conversion tests pass; rendered image-plane visual comparison remains open. |

## Plan Task Reconciliation

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

## Next Actions

1. Remeasure the representative large document on comparable baseline/current builds.
2. Complete rendered-image and uncommon-format validation before claiming full FR-002 acceptance.
3. Profile larger-model tree cost before proposing more batching or bulk deletion.
4. Decide whether reduced-resolution initial textures are wanted; current lazy loading uses full resolution.
5. Run the build/full test and platform gates for the exact production PR revisions, then assemble their evidence.

## Quality Gates

| Gate | Required? | Status | Evidence |
| --- | --- | --- | --- |
| Requirements and decisions accepted | yes | pending | design.md open questions |
| Complete task/criterion evidence | yes | pending | tasks.md; traceability.md |
| Focused correctness and failure tests | yes | partial or pending | Dated checks above; selected revision still needs acceptance |
| Current measurements and platform/manual acceptance | yes | pending | Historical observations are insufficient |
| Durable promotion and residual ownership | yes | pending | requirements.md durable impact |
| Review and closure metadata | yes | pending | No closure claimed |

## Evidence Log

| Date | Evidence | Result | Limits |
| --- | --- | --- | --- |
| 2026-11-04 | Imported report and commands above | Retained historical results | Not rerun during format migration; transient logs may be unavailable |
| 2026-11-06 | Direct source reads at d0644236e41f35a849b12a1e77941aa88354496f | Baseline implementation disposition confirmed | No build, runtime, visual or current benchmark certification |

## Correctness Property Coverage

See traceability.md#correctness-property-coverage for each property, owning
tasks and validation direction. No property is fully verified by this format
migration; bounded conventional/unit/manual checks remain declared task gates.

## Scope Reconciliation Before Closure

The requirement-to-delivery matrix in traceability.md records every current
requirement, its task destination and partial-blocking/not-covered disposition.
Legacy SC identifiers in requirements.md are still gates. An existing baseline
implementation does not satisfy the proposed extension or its broader criteria.

## Residual Risks

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

- Ready to implement: no package-wide claim; resolve selected task blockers.
- Ready to validate: no package-wide claim; scoped baseline checks may run.
- Ready to close: no.
- Ready to archive: no.
- Cleanup action: keep active, draft.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
