---
title: Compact view rails rework technical design
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Technical Design

## Overview

Propose explicit rail slots over existing dock actions. Current titlebar code has no rails; the
idle-layout fix is a baseline constraint, not delivery of this feature.

## Requirement Coverage

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
| --- | --- | --- | --- |
| Requirement 1 | AC1 | Six explicit slots; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 2 | AC1 | Slot alignment; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 3 | AC1 | Insertion placeholders; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 4 | AC1 | Theme feedback; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 5 | AC1 | Action-backed launchers; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 6 | AC1 | Slot persistence; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 7 | AC1 | Conditional ordering; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 8 | AC1 | Existing dock actions; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 9 | AC1 | Geometry ownership; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 10 | AC1 | Explicit geometry contract; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 11 | AC1 | Viewport convergence; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 12 | AC1 | Layout refresh discipline; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 13 | AC1 | Comparable GL measurements; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 14 | AC1 | Stable dimensions; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |

## Correctness Property Coverage

| Property | Design Behavior | Validation Direction | Notes |
| --- | --- | --- | --- |
| CP-001 | A placeholder, hover or action-state transition does not change rail or viewport dimensions. | T002, T003, T004; existing conventional tests and bounded manual checks | No new test framework |
| CP-002 | A launcher placement restores to the persisted logical slot and uses the existing dock action. | T002, T003, T004; existing conventional tests and bounded manual checks | No new test framework |

## High-Level Design

### System Architecture

Rails are a proposed access layer over existing dock actions. Current compact
chrome owns the titlebar only. D001 must confirm reintroduction and choose either
Qt-owned reservation or an overlay; neither is selected by this migration.
Preserve titlebar layout convergence and compact-disabled behavior.

### Components and Changes

Proposed owners are `src/Gui/CompactMainWindowChrome.cpp` and its header for
slot widgets and launcher placement, `src/Gui/MainWindow.cpp` for the lifecycle
bridge, and existing DockWindowManager/OverlayManager actions for panel state.
Keep launcher semantics in QAction with real QToolButton presentation.

### Data Models And Flow

Six slot IDs: left-upper, left-lower, bottom-left, right-upper, right-lower,
bottom-right. A stable panel ID maps to a slot using
`BaseApp/Preferences/MainWindow/CompactJetBrainsPanelSlots/<panel-id>`.
Pointer drag → explicit slot hover → insertion placeholder/index → accepted drop
→ placement/persistence → restored layout. Cancel restores prior placement.
D002 defines renamed/missing IDs and any conditional intra-slot ordering.

## Low-Level Design

### Algorithms And Interfaces

Use explicit slot containers with upper/lower groups near the top and bottom
containers anchored below. Placeholders reserve the eventual insertion index.
QAction owns text, tooltip, accessibility-related presentation, checked and
enabled state. Use palette/style or compact-specific object names for feedback.
No new public API or widget class signature is approved before D001/D002.

### Geometry And Compatibility

Never repeatedly shrink the QMainWindow central widget with setGeometry.
Hover, drag, checked state and placeholders must not change rail dimensions.
Keep bare LayoutRequest outside full compact relayout unless a compact-owned
input changes. Disabling compact mode restores normal menu, dock, toolbar and
status behavior. Exercise overlay, workbench switches, themes, DPI and frameless
mode without broadening into a titlebar rewrite.

### Error Handling

Reject unsupported drop data without changing persisted placement. Handle
missing icons/actions and renamed panel IDs under D002; do not silently invent
a migration policy. Remove transient placeholders on cancellation/destruction.

## Validation Strategy

Extend `tests/src/Gui/CompactMainWindowChrome.cpp` for all slots, empty/same-slot
and populated drops, action state, restart persistence and compact-disabled
restoration. Existing offscreen tests prove titlebar convergence only.
T004 needs real drag/drop and before/after viewport/GL allocation captures under
the D003 protocol, with equivalent document, dimensions, DPI and interaction.
The historical trace switches are absent and need deliberate instrumentation.

## Downstream Task Guidance

Resolve the decisions blocking the selected task, read its requirement criteria
and obtain task context before coding. Preserve existing source changes. Run
focused checks for code changes and broaden according to root AGENTS.md.
Reconcile design/tasks/traceability after changing a decision. No new testing
dependency is proposed; use existing fixtures, conventional tests and bounded
manual checks.

## Operational Considerations

Promote accepted current behavior to `docs/design/jetbrains-style-compact-ui.md` and operational
validation
to the runbook target in requirements.md before closure. These are proposed
destinations, not claims that those documents already exist. Route residual work
to docs/backlog/README.md with local-developer ownership. Keep the active package
until acceptance, reviews, promotion and closure metadata are complete.

## Open Questions

| Decision | Resolution needed | Blocks | Owner |
| --- | --- | --- | --- |
| D001 | Confirm rail reintroduction and choose Qt-owned reservation or overlay geometry before implementation. | T001 | local-developer |
| D002 | Define missing/renamed panel IDs, legacy slot preference restoration and conditional intra-slot ordering. | T001 | local-developer |
| D003 | Specify the comparable interaction/GL capture method and quantitative allocation-regression gate; removed trace switches cannot be used as-is. | T001 | local-developer |

## Related Artifacts

- [Requirements](requirements.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Migration input](legacy-spec.md)
