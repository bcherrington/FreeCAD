---
title: CAD and mesh file thumbnailing technical design
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Technical Design

## Overview

Extend Linux file-manager previews to imported geometry while retaining the existing FCStd
extraction path. Rendering remains proposed.

## Requirement Coverage

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
| --- | --- | --- | --- |
| Requirement 1 | AC1 | FCStd compatibility; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 2 | AC1 | STL preview MVP; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 3 | AC1 | STEP and IGES follow-up; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 4 | AC1 | Non-interactive execution; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 5 | AC1 | Bounded failures; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 6 | AC1 | Restricted execution; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 7 | AC1 | Output image contract; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 8 | AC1 | Explicit MIME registration; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |

## Correctness Property Coverage

| Property | Design Behavior | Validation Direction | Notes |
| --- | --- | --- | --- |
| CP-001 | FCStd extraction preserves the embedded/fallback image bytes and does not invoke geometry import. | T002; existing conventional tests and bounded manual checks | No new test framework |
| CP-002 | A geometry request cannot escape its approved execution/time/resource boundary or leave a successful partial image. | T001, T003, T004; existing conventional tests and bounded manual checks | No new test framework |

## High-Level Design

### System Architecture

Keep FCStd extraction as the existing fast path. A future geometry path should
use a FreeCAD-owned helper isolated from the interactive session. The renderer,
process and configuration boundary remain D001, not an implemented contract.

### Components and Changes

`src/Tools/freecad-thumbnailer.in` owns helper dispatch/output;
`src/XDGData/FreeCAD.thumbnailer.in` owns installed MIME registration. Add an
STL render path only after feasibility and isolation review. STEP/IGES follow
an explicit later gate; OBJ/3MF and Windows/macOS integrations stay follow-ups.

### Data Models And Flow

Input path + requested size + output path → format dispatch → FCStd extraction
or approved geometry importer → fit view → PNG output → temporary-state cleanup.
The CLI shape remains `freecad-thumbnailer -s %s %i %o`. Existing extraction
parses the size option but does not rescale the embedded PNG; preserve baseline.
No saved document format change is proposed.

## Low-Level Design

### Algorithms And Interfaces

Before rendering, validate format, output path and approved resource policy.
Choose offscreen GUI, tessellation or another FreeCAD-owned renderer in D001.
Specify camera, background, empty geometry and output dimensions at that gate.
Do not advertise MIME support until its import/render path passes fixtures.
Candidate MIME names in legacy-spec.md require distribution/package validation.

### Error Handling And Security

Renderer failures, timeout, oversize input, unavailable offscreen support,
corrupt input and unwritable output must produce non-zero failure without a
prompt. Avoid user macros, addons and startup code. Bound memory, time, input
size and concurrent requests; remove incomplete output and temporary documents.
The extraction-only baseline is not evidence that rendering is safely isolated.

### Migration And Compatibility

FCStd continues extracting embedded previews/fallback icons without import.
Keep desktop registration package-controlled and Linux-first. Test installed
paths, PNG compatibility and cache refresh after input modification. Five seconds
per request and sub-second simple STL are candidate budgets pending D002;
there is no accepted numeric memory/file-size/concurrency policy yet.

## Validation Strategy

T002 preserves extraction fixtures; T003/T004 verify the STL success/failure
matrix and installed GNOME Files/KDE Dolphin behavior; T005 gates STEP/IGES.
Include large, empty, malformed and concurrent inputs and offscreen failure.
Record small/medium/large STL runtime against D002's approved limits.

## Downstream Task Guidance

Resolve the decisions blocking the selected task, read its requirement criteria
and obtain task context before coding. Preserve existing source changes. Run
focused checks for code changes and broaden according to root AGENTS.md.
Reconcile design/tasks/traceability after changing a decision. No new testing
dependency is proposed; use existing fixtures, conventional tests and bounded
manual checks.

## Operational Considerations

Promote accepted current behavior to `docs/reference/cad-file-thumbnailing.md` and operational
validation
to the runbook target in requirements.md before closure. These are proposed
destinations, not claims that those documents already exist. Route residual work
to docs/backlog/README.md with local-developer ownership. Keep the active package
until acceptance, reviews, promotion and closure metadata are complete.

## Open Questions

| Decision | Resolution needed | Blocks | Owner |
| --- | --- | --- | --- |
| D001 | Choose the STL importer/render process and isolation boundary, including user configuration/addon suppression. | T001 | local-developer |
| D002 | Approve timeout, size, memory and concurrency limits; five seconds and sub-second simple STL rendering remain tentative targets. | T001 | local-developer |
| D003 | Confirm MIME aliases and STEP/IGES feasibility after the STL MVP; OBJ/3MF remain candidates, not committed support. | T005 | local-developer |

## Related Artifacts

- [Requirements](requirements.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Migration input](legacy-spec.md)
