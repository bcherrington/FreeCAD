---
title: Compact view rails rework requirements
doc_type: spec
artifact_type: requirements
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Requirements

## Introduction

Propose explicit rail slots over existing dock actions. Current titlebar code has no rails; the
idle-layout fix is a baseline constraint, not delivery of this feature.

## Goals

- Replace inferred rail drop zones with explicit visible slot containers.
- Keep `QAction` as the source of truth for each panel launcher.
- Keep real `QToolButton` launchers so action text, tooltips, checked state,
  enabled state, accessibility, icon sizing, palette behavior, and stylesheet
  integration continue to work.
- Provide real insertion placeholders during drag so users can see where a
  panel launcher will land.
- Preserve persisted logical slot assignments for compact panel launchers.
- Eliminate the central-widget geometry fight that causes viewport resize and
  OpenGL framebuffer churn.
- Keep normal FreeCAD dock, overlay, menu, toolbar, status bar, and workbench
  behavior unchanged when compact mode is disabled.

## Non-Goals

- Do not rewrite FreeCAD's dock system.
- Do not replace dock panel content with a new panel framework in this rework.
- Do not use `QTabBar` for rail launchers; rail buttons launch panels, they are
  not tabs in a single page stack.
- Do not implement unrelated toolbar alignment or drop-zone experiments in this
  spec.
- Do not broaden the work into native titlebar or frameless-window changes.

## Glossary


- **Compact rail**: A left or right launcher surface owned by compact chrome.
- **Rail slot**: A visible placement container such as `left-upper` or
  `bottom-right`.
- **Rail launcher**: A `QAction`-backed button that shows, hides, or activates
  an existing dock panel.
- **Insertion placeholder**: A transient widget or layout item that reserves the
  launcher landing position during drag.
- **Geometry contract**: The rule that determines whether rails reserve central
  area space through Qt-owned layout state or overlay the viewport.

## Durable Source Baseline

| Source | Current behavior relied on | Confidence | Notes |
| --- | --- | --- | --- |
| `src/Gui/CompactMainWindowChrome.cpp` | docs/design/jetbrains-style-compact-ui.md describes the implemented titlebar and explicitly leaves panel access separate. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |
| `src/Gui/MainWindow.cpp` | docs/design/jetbrains-style-compact-ui.md describes the implemented titlebar and explicitly leaves panel access separate. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |
| `tests/src/Gui/CompactMainWindowChrome.cpp` | docs/design/jetbrains-style-compact-ui.md describes the implemented titlebar and explicitly leaves panel access separate. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |

## Durable Impact

| Durable area | Action | Target | Notes |
| --- | --- | --- | --- |
| reference/design | modify | `docs/design/jetbrains-style-compact-ui.md` | Proposed promotion destination; publish only accepted current behavior. |
| operations | add | `docs/runbooks/compact-view-rails-rework.md` | Validation, recovery and limitations before closure. |
| follow-up | modify | `docs/backlog/README.md` | Route unfinished decisions/work with owner before closure. |

## Staged Readiness

- **Current stage:** reconcile
- **Next stage:** resolve decisions, then resume one scoped task.
- **Design-first exception:** no. Legacy exploration is retained as history.
- **Downstream review needed:** design, tasks, traceability and verification after decisions.
- **Ready to implement:** no blanket readiness; use task context and resolve its blocking decisions.

## Requirements

### Requirement 1: Six explicit slots

**User Story:** As a FreeCAD user who needs to reorganize compact panels, I want to see all six
placement targets, so that I can place launchers without guessing pointer regions.

**Priority:** must-have

**Legacy reference:** FR-001. The rail MUST expose explicit slot containers for `left-upper`,
`left-lower`, `bottom-left`, `right-upper`, `right-lower`, and `bottom-right`.

#### Acceptance Criteria

1. WHILE dragging a launcher, THE rail SHALL expose left-upper, left-lower, bottom-left,
   right-upper, right-lower and bottom-right as visible targets.

### Requirement 2: Slot alignment

**User Story:** As a FreeCAD user who needs to arrange rail launchers, I want to keep top groups and
bottom-aligned groups distinct, so that I can find panel controls consistently after resize.

**Priority:** must-have

**Legacy reference:** FR-002. Upper and lower slot containers MUST appear near the top of their
rail, while bottom slots MUST remain bottom-aligned.

#### Acceptance Criteria

1. WHEN launchers occupy upper/lower slots, THEN those slots SHALL sit near the rail top; bottom
   slots SHALL stay bottom-aligned after resize and restart.

### Requirement 3: Insertion placeholders

**User Story:** As a FreeCAD user who needs to drag panel launchers, I want to see the eventual
insertion position, so that I can predict placement and cancel safely.

**Priority:** must-have

**Legacy reference:** FR-003. Dragging a rail button MUST show a real insertion placeholder in the
target slot at the eventual insertion index.

#### Acceptance Criteria

1. WHEN dragging into an empty, populated or same slot, THEN a real placeholder SHALL show the
   eventual insertion index and clear on cancellation.

### Requirement 4: Theme feedback

**User Story:** As a FreeCAD user who needs to use different themes and displays, I want to see
consistent hover and insertion feedback, so that I can recognize drag targets across palette and DPI
changes.

**Priority:** must-have

**Legacy reference:** FR-004. Slot hover and placeholder feedback MUST use `QPalette`, `QStyle`, or
compact-specific object names that themes can target.

#### Acceptance Criteria

1. WHEN theme, palette or DPI changes, THE hover and placeholder feedback SHALL stay visible through
   QPalette, QStyle or compact-specific object names.

### Requirement 5: Action-backed launchers

**User Story:** As a FreeCAD user who needs to use dock panel controls, I want to retain action-
backed button behavior, so that I can receive correct labels, state, tooltips and accessible
controls.

**Priority:** must-have

**Legacy reference:** FR-005. Rail launchers MUST continue to be backed by `QAction` and shown
through `QToolButton` or an equivalent widget that preserves action behavior.

#### Acceptance Criteria

1. WHEN an action changes text, tooltip, checked/enabled state or icon size, THE QToolButton
   launcher SHALL reflect it and retain accessibility.

### Requirement 6: Slot persistence

**User Story:** As a FreeCAD user who needs to restart compact mode, I want to restore each panel to
its saved logical slot, so that I can retain my panel arrangement.

**Priority:** must-have

**Legacy reference:** FR-006. The implementation MUST preserve
`BaseApp/Preferences/MainWindow/CompactJetBrainsPanelSlots/<panel-id>` slot assignment behavior.

#### Acceptance Criteria

1. GIVEN saved CompactJetBrainsPanelSlots/<panel-id> values, WHEN compact mode restarts, THEN each
   recognized panel SHALL restore its logical slot under the approved missing-ID policy.

### Requirement 7: Conditional ordering

**User Story:** As a FreeCAD user who needs to reorder launchers inside a slot, I want to persist
ordering when that feature is selected, so that I can retain the arrangement I explicitly chose.

**Priority:** should-have

**Legacy reference:** FR-007. The implementation SHOULD add intra-slot ordering persistence only
when the rework implements user-controlled ordering inside a slot.

#### Acceptance Criteria

1. IF user-controlled intra-slot ordering is implemented, THEN its persistence SHALL be specified
   and tested; otherwise this conditional extension SHALL be explicitly deferred.

### Requirement 8: Existing dock actions

**User Story:** As a FreeCAD user who needs to toggle dock panels, I want to use the existing dock
action from the rail, so that I can preserve View-menu and overlay behavior.

**Priority:** must-have

**Legacy reference:** FR-008. Clicking a launcher MUST continue to drive the existing dock toggle
action path so normal dock and overlay behavior remain consistent with the View menu.

#### Acceptance Criteria

1. WHEN a rail launcher is clicked with overlay on or off, THEN the same dock toggle action used by
   the View menu SHALL run; disabling compact mode SHALL preserve ordinary docks and toolbars.

### Requirement 9: Geometry ownership

**User Story:** As a FreeCAD user who needs to interact with a document, I want to let Qt own
central-widget layout, so that I can avoid repeated viewport resizing.

**Priority:** must-have

**Legacy reference:** FR-009. The rail layout MUST not reserve rail space by repeatedly calling
`central->setGeometry()` from compact chrome.

#### Acceptance Criteria

1. WHEN Qt relays layout events, THE compact rail SHALL avoid repeated central->setGeometry calls to
   reserve rail width.

### Requirement 10: Explicit geometry contract

**User Story:** As a FreeCAD user who needs to enable compact rails, I want to have a defined
reservation or overlay rule, so that I can understand how panels affect workspace geometry.

**Priority:** must-have

**Legacy reference:** FR-010. The rail geometry contract MUST be explicit: rails either reserve
space through Qt-owned layout state or overlay the central area without shrinking it.

#### Acceptance Criteria

1. BEFORE rail implementation, THE design SHALL select and document Qt-owned reservation or overlay
   placement and its resizing rules.

### Requirement 11: Viewport convergence

**User Story:** As a FreeCAD user who needs to work in the 3D view, I want to keep viewport geometry
settled, so that I can avoid repeated backing-buffer allocation.

**Priority:** must-have

**Legacy reference:** PR-001. Compact mode MUST not cause the 3D viewport to repeatedly alternate
between widths that differ by the compact rail width.

#### Acceptance Criteria

1. WHEN the controlled document interaction is replayed, THE viewport SHALL converge without
   repeated rail-width alternation.

### Requirement 12: Layout refresh discipline

**User Story:** As a FreeCAD user who needs to leave a compact window idle, I want to avoid relayout
for unchanged inputs, so that I can keep the UI quiescent.

**Priority:** must-have

**Legacy reference:** PR-002. The compact rail rework MUST avoid full compact chrome relayout on
every `MainWindow.LayoutRequest` unless a compact-owned input actually changed.

#### Acceptance Criteria

1. WHEN a bare MainWindow.LayoutRequest arrives without a changed compact-owned input, THE chrome
   SHALL avoid full relayout.

### Requirement 13: Comparable GL measurements

**User Story:** As a FreeCAD user who needs to review a rail change, I want to compare equivalent
resize and GL traces, so that I can verify that the measured layout regression is resolved.

**Priority:** must-have

**Legacy reference:** PR-003. Validation MUST compare pre-change and post-change traces for viewport
resize counts and OpenGL framebuffer allocation counts.

#### Acceptance Criteria

1. WHEN validating the rework, THE evidence SHALL compare viewport resize and full-size GL
   framebuffer allocations for the same workflow before and after the change.

### Requirement 14: Stable dimensions

**User Story:** As a FreeCAD user who needs to hover and drag launchers, I want to keep rail and
work-area dimensions fixed, so that I can avoid workspace movement during feedback.

**Priority:** must-have

**Legacy reference:** PR-004. The rework MUST preserve stable rail dimensions so hover, dragging,
checked state, and placeholder feedback do not resize the rail or central work area.

#### Acceptance Criteria

1. WHEN hover, drag, checked state or placeholder changes, THE rail dimensions and central work area
   SHALL remain stable.

## Correctness Properties

- **CP-001**: A placeholder, hover or action-state transition does not change rail or viewport
  dimensions. Covers Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 14.
- **CP-002**: A launcher placement restores to the persisted logical slot and uses the existing dock
  action. Covers Requirement 1, Requirement 2, Requirement 3, Requirement 5, Requirement 6,
  Requirement 7, Requirement 8.

## Edge Cases

- Compact mode disabled.
- Overlay mode enabled and disabled.
- Rail launcher with missing icon or renamed dock action.
- Multiple launchers in one slot.
- Dropping into an empty slot.
- Dropping back into the same slot.
- Theme changes and high-DPI icon sizing.
- Workbench switching while a panel is active.
- Restart after persisted slot changes.
- Frameless compact mode enabled.

## Technical Context

- Language/dependencies: repository C++ toolchain and Qt Widgets; QAction/QToolButton and existing dock/overlay actions.
- Storage: MainWindow compact panel-slot preferences; ordering/missing-ID policy pending D002.
- Target: supported FreeCAD desktop window systems, with explicit DPI/theme/frameless validation.
- Constraints: preserve titlebar convergence and normal compact-disabled behavior.
- Performance: compare resize/FBO counts under the quantitative D003 protocol; no portable absolute budget is accepted yet.

## Success Criteria

- **SC-001**: All six rail slots are visible placement targets during drag.
- **SC-002**: Launcher placement persists through restart for all supported
  slots.
- **SC-003**: Layout tracing no longer shows the measured compact-rail
  width bounce.
- **SC-004**: OpenGL tracing shows full-size framebuffer allocation churn is
  sharply reduced for the same compact interaction workflow.
- **SC-005**: Existing compact chrome tests are updated or extended for slot
  layout, button fitting, persistence, and menu restoration.

Success criteria remain acceptance gates, not claims of completion. Tentative budgets and
qualitative improvement targets require a recorded decision before validation.

## Related Artifacts

- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Historical input](legacy-spec.md)
