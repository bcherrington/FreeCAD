---
title: UI rails for panels
doc_type: design
status: draft
owner: local-developer
last_reviewed: 2026-05-23
---

# UI Rails For Panels

## Purpose

This branch explores side rails for FreeCAD panels. The goal is to make docked
panels easier to reach while keeping the central CAD viewport dominant.

## Scope

This design covers left and right panel rails, rail button ordering, persisted
panel slot overrides, drag/drop feedback, and activation through existing dock
toggle actions. It does not define replacement window chrome, custom window
controls, collapsed main menus, native-window behavior, or replacement dock
content.

## Components And Responsibilities

| Component | Responsibility | Owned Inputs | Owned Outputs |
| --- | --- | --- | --- |
| `Gui::MainWindow` | Own the panel rail preference, instantiate the rail helper, and forward resize/preference events. | User preference, normal main-window lifecycle | Rail activation and layout updates |
| `Gui::PanelRails` | Own left/right rail widgets, dock discovery, panel slot persistence, drag/drop feedback, and panel activation. | Registered dock widgets, status bar, central widget, dock toggle actions | Rail buttons, reserved side margins, dock activation requests |
| `DockWindowManager` | Continue creating, registering, showing, hiding, and persisting dock widgets. | Registered panel widgets and dock toggle actions | Existing `QDockWidget` hosts and View menu behavior |
| Overlay dock system | Keep existing overlay tabs and title bars available for docked panels. | Dock widgets and overlay preferences | Overlay panel activation and dock controls |

## Configuration Model

| Config Source | Key Or Parameter | Applied By | Effect | Failure Mode |
| --- | --- | --- | --- | --- |
| User preferences | `BaseApp/Preferences/MainWindow/PanelRailsEnabled` | `Gui::MainWindow` | Enables or disables panel rails. | Defaults to normal FreeCAD UI when absent or false. |
| User preferences | `BaseApp/Preferences/MainWindow/PanelRailsSlots/<panel-id>` | Rail helper | Overrides the logical rail slot for a dock panel. Values are `left-upper`, `left-lower`, `right-upper`, `right-lower`, `bottom-left`, and `bottom-right`. | Falls back to known-panel defaults or current Qt dock area when absent or invalid. |

## Current Behavior

- Adds narrow left and right rail widgets as `MainWindow` chrome, not
  `QDockWidget`s.
- Lists current dock windows as icon buttons in deterministic slot order.
- Known FreeCAD panels use explicit bundled FreeCAD icons instead of inheriting
  the application icon.
- Clicking a rail button drives the existing dock toggle action so overlay mode
  sees the same path as the View menu.
- Rail buttons can be dragged between rail regions. Dropping a button persists
  the panel's logical slot under
  `BaseApp/Preferences/MainWindow/PanelRailsSlots`.
- Moving a rail button also moves the associated panel's real `QDockWidget` to
  the Qt dock area mapped from the target rail slot.
- All slots behave identically: a slot can contain multiple rail buttons, but
  only one panel in that slot is expanded at a time.
- Rails reserve left/right contents margins while active so they are independent
  from the normal dock splitter layout.
- The status bar remains below the rails and spans the full window width.

## Current Limitations

- Panel content still uses existing Qt dock widgets. Rail activation is isolated
  behind a helper, but it does not yet present panels through overlay proxy
  widgets.
- Drag/drop currently assigns a panel to a logical slot; drag ordering inside a
  slot is not implemented.

## Current Test Coverage

`tests/src/Gui/PanelRails.cpp` covers behavior that can be
validated without screenshot comparison:

- Rails restore `MainWindow` contents margins when disabled.
- Left/right rail buttons fit inside their rail content and are not clipped.
- Persisted panel slot overrides move rail buttons and their associated dock
  panels away from their default rail/dock area.

Manual validation is still required for final visual alignment, theme
appearance, overlay mode behavior, and workbench-specific panel content.

## Change Impact Strategy

Keep upstream patching safe by limiting mainline file changes:

- Prefer rail-specific files over expanding `MainWindow.cpp`.
- Keep panel rail behavior parameter-gated and disabled by default.
- Use existing FreeCAD dock actions where possible instead of duplicating dock
  semantics.
- Do not alter normal toolbar, menu, window chrome, dock, or status bar behavior
  when panel rails are disabled.
- Keep top-chrome work in separate branches.
