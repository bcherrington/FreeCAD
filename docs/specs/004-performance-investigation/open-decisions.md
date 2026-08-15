---
title: FreeCAD Performance Investigation Open Decisions
doc_type: spec
artifact_type: open-decisions
status: draft
owner: local-developer
last_reviewed: 2026-08-14
---

# Open Decisions

## Purpose

Track decisions that select, accept, reject, route, promote, or close Spec 004
work. A decision row is not implementation evidence.

## Decision Log

| ID | Decision Needed | Options | Owner | Needed By | Status |
| --- | --- | --- | --- | --- | --- |
| D001 | Which investigation slice runs next? | overlay matched validation; document-open profile; presentation/render attribution; Preferences ownership | local-developer | 2026-08-14 review | resolved: overlay matched validation |
| D002 | Does the DockOverlay zero-radius-blur experiment merit a current source slice? | keep/recreate; reject; gather stronger evidence | local-developer with GUI/rendering reviewer | T026 completion | open |
| D003 | Where should document-open optimization continue after source narrowing? | focused Spec 004 slice; new focused spec; issue/backlog; reject | local-developer with GUI/Part GUI owner | T018 completion | open |
| D004 | Who owns the Preferences/addFC timeout behavior? | addon; environment; FreeCAD core startup; mixed ownership | local-developer with addon/core owner | T021 completion | open |
| D005 | Where should exercised profiling guidance and accepted conclusions be promoted? | performance findings; viewport reference; new developer runbook; discard as historical-only | local-developer/docs owner | before closure | open |

## Blocking Impact

| Decision ID | Blocks | Impact If Unresolved |
| --- | --- | --- |
| D002 | T022, T023 and source-implementation readiness | The overlay experiment cannot become a bounded implementation target. |
| D003 | T018, T024 and closure | Document-open work has no durable owner or rejection rationale. |
| D004 | T021, T024 and closure | Preferences evidence cannot be routed safely. |
| D005 | promotion and closure | Lasting procedure/findings would remain only in the active package. |

## Resolved Decisions

### D001 - Validate the overlay experiment first

- **Resolution date:** 2026-08-14
- **Decision:** Run a same-duration, externally profiled before/after comparison
  of the historical DockOverlay experiment before advancing another track.
- **Included workflow:** controlled 3D-view rotation with the historical
  zero-radius-blur behavior as the only source variable.
- **Excluded workflows:** document open/restore, Preferences, unrelated dialogs,
  and the queued presentation/render-attribution track.
- **Evidence checkpoint:** T026 must produce matched baseline and experiment
  captures with the manifest controls in `quickstart.md` and record whether the
  evidence identifies a bounded source cause.
- **Next decision:** D002 keeps/recreates the experiment, rejects it, or requires
  stronger evidence; only a keep decision may advance to T022 and T023.
- **Rationale:** The multi-expert review found this to be the only current
  source-backed experiment with a direct historical hotspot match. Current
  source no longer contains the behavior, so validation requires a clean
  recreation or reapplication on a permitted source branch.
- **Does not authorize:** a production patch, integration-branch edits, or an
  upstream PR.

## Related Artifacts

- Requirements: `requirements.md`
- Canonical Context: `canonical-context.md`
- Design: `design.md`
- Tasks: `tasks.md`
- Traceability: `traceability.md`
- Verification: `verification.md`
