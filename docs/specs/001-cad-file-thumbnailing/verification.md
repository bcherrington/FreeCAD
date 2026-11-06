---
title: CAD thumbnailing reconciliation
doc_type: spec
artifact_type: verification
status: code-derived
owner: local-developer
last_reviewed: 2026-11-06
---

# CAD Thumbnailing Reconciliation

Reviewed source on `personal/integration-testing` at `13d601b9dd6b66026e2d0dbb5c16156f18bdb592`.
This is a source-backed reconciliation, not lifecycle closure or upstream PR
approval. This dated report predates the 2026-11-06 format migration. Current artifact
authority is requirements.md, design.md, tasks.md and traceability.md; spec.md
is now a brief. The migration does not make the historical test run current. Historical measurements
remain dated evidence, not fresh benchmarks of this revision.

## Requirement Disposition

`src/Tools/freecad-thumbnailer.in` extracts an embedded PNG from an FCStd ZIP
containing `Document.xml`, or copies the installed fallback icon. It imports no
FreeCAD renderer. `src/XDGData/FreeCAD.thumbnailer.in` registers only
`application/x-extension-fcstd`.

| Requirements | Disposition |
| --- | --- |
| FR-001 | Existing FCStd extraction/fallback implemented and locally checked. |
| FR-002, FR-003 | Not implemented: no STL, STEP or IGES import/render path. |
| FR-004, FR-006 | Existing extraction is non-interactive and imports no user macro/workbench. The proposed renderer still needs its own isolation design and validation. |
| FR-005 | Not implemented: no explicit execution timeout or render resource budget. |
| FR-007 | Existing helper writes embedded/fallback PNG to requested output; rendered output not implemented. |
| FR-008 | FCStd registration exists; exchange/mesh registrations remain future work. |

User stories 1 and 2 remain unimplemented. Story 3's existing extraction path
passes fixture checks; no new speed comparison or desktop integration test was
performed. Existing extraction is not completion of the extension spec.

## Local Validation

Executed the exact helper template with only its install-data placeholder
pointing at a scratch fallback icon. Five cases passed: embedded PNG copied
byte-for-byte, fallback PNG copied byte-for-byte, non-FCStd ZIP rejected,
corrupt input rejected, and STL rejected (expected current limitation).
No GNOME/Dolphin registration, installed-package behavior, imported rendering,
large/corrupt archive resource bounds or desktop batching was validated.
Scratch fixture/script/results: `/tmp/freecad-spec-reconciliation-afbb1wf9/`.
These are transient checks, not committed regression coverage.

## Next Actions

Choose the importer/render process and isolation strategy, specify timeout and
resource limits, then create tasks for an STL vertical slice with packaging and
file-manager acceptance checks. Add STEP/IGES only after import cost and safety
have been measured. Keep the package draft.

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
