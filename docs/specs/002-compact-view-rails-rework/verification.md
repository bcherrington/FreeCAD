---
title: Compact rails reconciliation
doc_type: spec
artifact_type: verification
status: code-derived
owner: local-developer
last_reviewed: 2026-11-06
---

# Compact Rails Reconciliation

Reviewed source on `personal/integration-testing` at `13d601b9dd6b66026e2d0dbb5c16156f18bdb592`.
This is a source-backed reconciliation, not lifecycle closure or upstream PR
approval. This dated report predates the 2026-11-06 format migration. Current artifact
authority is requirements.md, design.md, tasks.md and traceability.md; spec.md
is now a brief. The migration does not make the historical test run current. Historical measurements
remain dated evidence, not fresh benchmarks of this revision.

## Source And Requirements

`src/Gui/CompactMainWindowChrome.cpp` currently implements compact titlebar
chrome, without rail slot containers, rail launchers, drop placeholders or rail
persistence. `layoutChrome()` handles titlebar controls and top contents margin;
it no longer shrinks the central widget to reserve rail width.
`MainWindow::event()` excludes `LayoutRequest` from compact layout events.

| Requirements / acceptance | Disposition |
| --- | --- |
| FR-001–FR-008; acceptance 1, 2, 4 | Not implemented: six slots, insertion/hover feedback, launcher actions and rail persistence are future work. Dock behavior existing elsewhere is not proof of rail integration. |
| FR-009 | Related old geometry-fight path removed. New rails must still obey this constraint. |
| FR-010 | Future geometry contract undecided; reserve space or overlay must be chosen before reintroduction. |
| PR-001, PR-002; acceptance 3 | Related titlebar idle-layout fix implemented and regression tested; rail-specific and rendered viewport acceptance remains unverified. |
| PR-003 | No fresh before/after GL framebuffer measurements. Old trace switches are absent. |
| PR-004 | No current rail dimensions to validate; test once rails exist. |

Reachable commits `3830ca629d8` (idle relayout fix), `52fe1e76ba6`
and `d5db974da8b` (rail removal) explain why the original spec's current-state
premise no longer applies. Rail removal does not fulfill the rail feature.

## Validation

```sh
QT_QPA_PLATFORM=offscreen pixi run build/debug/tests/CompactMainWindowChrome_Tests_run
```

16 Qt test entries passed (including setup/cleanup), zero failures or skips.
`tests/src/Gui/CompactMainWindowChrome.cpp` covers layout convergence, MDI tab
stability, refresh coalescing, activation/deactivation, workbench settling and
lifetime behavior. These are titlebar tests; there are no current rail slot,
drag/drop or persistence tests. Offscreen Qt results do not prove OpenGL FBO
behavior or desktop drag-and-drop. Log: `/tmp/freecad-spec-recon-compact-tests.log`.

## Next Actions

Confirm whether rails should be reintroduced as a separate feature. If so,
choose the geometry and persistence contracts, create implementation tasks,
then add slot/drop/restore tests and real viewport/GL measurements. Preserve
the current titlebar convergence behavior. Keep this package draft.

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
