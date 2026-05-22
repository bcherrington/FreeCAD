---
title: Compact view rails rework
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-05-22
---

# Feature Specification

## Summary

Rework the compact view side rails from transitional toolbar-based drop zones
into explicit rail slot containers with stable geometry, visible drag targets,
and layout behavior that does not trigger repeated 3D viewport resize churn.

## Problem

The current compact rail prototype works as a first pass, but its rail
drag-and-drop behavior is still transitional. Each side uses one vertical
toolbar and derives logical drop zones from pointer position, which makes lower
and bottom targets implicit instead of visible.

Profiling also found a more serious layout issue: compact chrome currently
shrinks the `QMainWindow` central widget after Qt has laid it out. That causes
the 3D viewport to oscillate between widths that differ by the rail reservation
width. The oscillation reaches Qt's `QOpenGLWidget` backing framebuffer path and
causes repeated full-size multisampled framebuffer allocation.

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

## Requirements

### Functional Requirements

- **FR-001**: The rail MUST expose explicit slot containers for `left-upper`,
  `left-lower`, `bottom-left`, `right-upper`, `right-lower`, and
  `bottom-right`.
- **FR-002**: Upper and lower slot containers MUST appear near the top of their
  rail, while bottom slots MUST remain bottom-aligned.
- **FR-003**: Dragging a rail button MUST show a real insertion placeholder in
  the target slot at the eventual insertion index.
- **FR-004**: Slot hover and placeholder feedback MUST use `QPalette`,
  `QStyle`, or compact-specific object names that themes can target.
- **FR-005**: Rail launchers MUST continue to be backed by `QAction` and shown
  through `QToolButton` or an equivalent widget that preserves action behavior.
- **FR-006**: The implementation MUST preserve
  `BaseApp/Preferences/MainWindow/CompactJetBrainsPanelSlots/<panel-id>` slot
  assignment behavior.
- **FR-007**: The implementation SHOULD add intra-slot ordering persistence
  only when the rework implements user-controlled ordering inside a slot.
- **FR-008**: Clicking a launcher MUST continue to drive the existing dock
  toggle action path so normal dock and overlay behavior remain consistent with
  the View menu.
- **FR-009**: The rail layout MUST not reserve rail space by repeatedly calling
  `central->setGeometry()` from compact chrome.
- **FR-010**: The rail geometry contract MUST be explicit: rails either reserve
  space through Qt-owned layout state or overlay the central area without
  shrinking it.

### Performance Requirements

- **PR-001**: Compact mode MUST not cause the 3D viewport to repeatedly
  alternate between widths that differ by the compact rail width.
- **PR-002**: The compact rail rework MUST avoid full compact chrome relayout on
  every `MainWindow.LayoutRequest` unless a compact-owned input actually
  changed.
- **PR-003**: Validation MUST compare pre-change and post-change traces for
  viewport resize counts and OpenGL framebuffer allocation counts.
- **PR-004**: The rework MUST preserve stable rail dimensions so hover,
  dragging, checked state, and placeholder feedback do not resize the rail or
  central work area.

### Key Entities

- **Compact rail**: A left or right launcher surface owned by compact chrome.
- **Rail slot**: A visible placement container such as `left-upper` or
  `bottom-right`.
- **Rail launcher**: A `QAction`-backed button that shows, hides, or activates
  an existing dock panel.
- **Insertion placeholder**: A transient widget or layout item that reserves the
  launcher landing position during drag.
- **Geometry contract**: The rule that determines whether rails reserve central
  area space through Qt-owned layout state or overlay the viewport.

## Acceptance Criteria

1. **Given** compact mode is enabled, **When** a user drags a rail launcher,
   **Then** the destination slot is visible and the eventual insertion position
   is represented by a real placeholder.
2. **Given** a launcher is dropped into `bottom-left` or `bottom-right`,
   **When** compact mode is restored, **Then** the launcher appears in the
   persisted bottom-aligned slot.
3. **Given** a 3D view is open in compact mode, **When** the layout trace is
   repeated, **Then** the viewport no longer alternates between widths such as
   `1720` and `1664` because of compact rail reservation.
4. **Given** overlay mode is enabled, **When** a compact rail launcher is
   clicked, **Then** panel activation still flows through the same dock toggle
   path used by the View menu.

## User Scenarios And Testing

### User Story 1 - Visible Rail Reordering (Priority: P1)

As a compact UI user, I want clear rail drop targets so I can move panel
launchers without guessing where a pointer region maps.

**Why this priority**: The current inferred drop-zone model is the main
interaction limitation of the rail prototype.

**Independent Test**: Drag launchers between all six slots and confirm visible
slot hover, placeholder insertion, persisted slot assignment, and restored
placement after restart.

**Acceptance Scenarios**:

1. **Given** a rail launcher, **When** it is dragged over `left-lower`, **Then**
   `left-lower` is visibly highlighted and shows the insertion position.
2. **Given** a launcher is dropped into a bottom slot, **When** compact mode is
   reopened, **Then** the launcher remains bottom-aligned.

### User Story 2 - Stable Viewport Geometry (Priority: P1)

As a compact UI user working in the 3D viewport, I want rail layout to remain
stable so viewport interaction does not repeatedly recreate OpenGL backing
buffers.

**Why this priority**: Profiling found hundreds of repeated framebuffer and
renderbuffer allocations caused by compact layout width oscillation.

**Independent Test**: Run `FREECAD_QUARTER_GL_TRACE=1` and the GL interposer on
the same interaction workflow before and after the rework.

**Acceptance Scenarios**:

1. **Given** compact mode and a document view, **When** the traced interaction
   runs, **Then** the viewport size does not bounce between rail-reserved and
   full-width geometry.

### User Story 3 - Existing Panel Behavior Preserved (Priority: P1)

As a FreeCAD user, I want compact rails to expose panels without breaking normal
dock, overlay, and View menu behavior.

**Why this priority**: Compact rails are an access layer over existing dock
panels, not a replacement panel system.

**Independent Test**: Toggle panels through rail buttons, View menu entries,
overlay mode, and workbench switches.

**Acceptance Scenarios**:

1. **Given** a panel is available in the View menu, **When** its rail launcher
   is clicked, **Then** the same dock visibility state is updated.
2. **Given** compact mode is disabled, **When** FreeCAD is used normally,
   **Then** normal dock and toolbar behavior is unchanged.

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

## Related Artifacts

- Design: [Compact UI design](../../design/jetbrains-style-compact-ui.md)
- Performance spec:
  [FreeCAD performance optimization](../freecad-performance-optimization/spec.md)
- Performance research:
  [FreeCAD profiling findings](../freecad-performance-optimization/research.md)
- Code: `src/Gui/CompactMainWindowChrome.cpp`,
  `src/Gui/CompactMainWindowChrome.h`, `src/Gui/MainWindow.cpp`,
  `src/Gui/DockWindowManager.cpp`, `src/Gui/OverlayWidgets.h`,
  `src/Gui/OverlayManager.cpp`
- Tests: `tests/src/Gui/CompactMainWindowChrome.cpp`
