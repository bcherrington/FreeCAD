---
title: JetBrains-style compact titlebar
doc_type: design
status: draft
owner: local-developer
last_reviewed: 2026-05-23
---

# JetBrains-Style Compact Titlebar

## Purpose

This branch explores a JetBrains-style compact top chrome for FreeCAD while
leaving panel access and dock layout behavior to separate work.

## Scope

This design covers the compact main-window titlebar/header, compact menu
switching, frameless-window support, and the small `MainWindow` lifecycle bridge.
It does not define compact panel access, dock placement, or a replacement for
FreeCAD's existing dock and overlay systems.

## Components And Responsibilities

| Component | Responsibility | Owned Inputs | Owned Outputs |
| --- | --- | --- | --- |
| `Gui::MainWindow` | Own the compact titlebar preference, instantiate compact chrome, and forward relevant resize/theme/preference events. | User preference, normal main-window lifecycle | Compact chrome activation, minimal bridge into existing main-window behavior |
| `Gui::CompactMainWindowChrome` | Own compact header, menu/tool switch, document/macro/workbench controls, optional frameless window behavior, and compact layout reservation. | Existing menu bar, MDI area, workbench/menu actions | Compact titlebar, restored normal menu visibility, window controls |
| `Gui::CompactTitleBarStyle` | Centralize compact titlebar button sizing, dropdown metrics, icon-menu metrics, and group spacing. | Toolbar icon-size preference and Qt toolbar metrics | Consistent titlebar button sizing and dropdown arrow placement |
| Workbenches | Continue defining menus, toolbars, command bars, and dock windows. | Workbench activation and command definitions | Menus, toolbars, and registered panels |

## Data And Control Flow

1. The `BaseApp/Preferences/MainWindow/CompactJetBrainsLayout` preference gates
   compact mode.
2. `MainWindow` creates `CompactMainWindowChrome` once and toggles it from the
   preference.
3. When compact mode is enabled, compact chrome creates or shows the compact
   top bar, hides the normal menu bar, hides the MDI tab-bar container, and
   reserves top workspace margin for the header.
4. Disabling compact mode hides the extra chrome, restores the previous menu
   bar visibility, and returns `MainWindow` margins to normal.

## Configuration Model

| Config Source | Key Or Parameter | Applied By | Effect | Failure Mode |
| --- | --- | --- | --- | --- |
| User preferences | `BaseApp/Preferences/MainWindow/CompactJetBrainsLayout` | `Gui::MainWindow` | Enables or disables the compact titlebar prototype. | Defaults to normal FreeCAD UI when absent or false. |

## Current Implementation Status

The compact titlebar prototype is disabled by default behind:

`BaseApp/Preferences/MainWindow/CompactJetBrainsLayout`

Enable it from the Python console:

```python
FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow").SetBool(
    "CompactJetBrainsLayout",
    True,
)
```

Disable it with the same command and `False`.

Current behavior:

- Adds compact top chrome implemented primarily in
  `src/Gui/CompactMainWindowChrome.cpp`, with shared titlebar metrics in
  `src/Gui/CompactTitleBarStyle.cpp`.
- Keeps `src/Gui/MainWindow.cpp` changes limited to construction, preference
  updates, frameless startup flags, resize/theme forwarding, and destruction.
- Adds a titlebar-like top row with app icon menu, hamburger menu, document
  menu, workbench-provided top-level menus, macro controls, edit-mode selector,
  workbench selector, recompute, settings, help, and window controls.
- Clicking the hamburger toggles FreeCAD's normal horizontal menu bar inside
  the same switch area used by the titlebar toolbar. Runnable menu actions and
  click-away collapse it after they trigger.
- The toolbar and menu bar share the same switch panel height; the menu bar is
  vertically centered rather than stretched.
- Hides the normal menu bar while compact mode is active, then restores its
  previous visibility when compact mode is disabled.
- Hides the active MDI document tab-bar container while compact mode is active.
- The document menu lists New, Open, Import, Export, Save, Save All, Close,
  Close All, open document views with close affordances, and recent files.
  Document modified markers update on document change/save signals.
- The macro menu lists recent macros, all macros, and Edit Macros. Selecting a
  macro changes the menu label; the adjacent play button runs it.
- The edit-mode selector is compact-owned and writes the same
  `UserEditMode` preference used by `Std_UserEditMode`, avoiding fragile
  mutation of a command-owned toolbar widget.
- Settings and Help are icon-only menu buttons with right-aligned popups.
- Optional frameless mode hides the native titlebar after restart and supplies
  compact chrome drag, resize, minimize, maximize/restore, and close behavior.

Current limitations:

- Frameless mode is implemented but remains parameter-gated and
  restart-required. Platform-specific behavior still needs wider manual
  validation.
- Toolbar alignment/drop-zone experiments were moved out of this compact UI
  branch and should remain separate until the desired Qt architecture is clear.
- Compact panel access belongs in the separate compact panel UI work, not this
  titlebar branch.

## Current Test Coverage

`tests/src/Gui/CompactMainWindowChrome.cpp` covers compact titlebar behavior
that can be validated without screenshot comparison:

- Titlebar buttons expose accessible names/status text and match compact toolbar
  icon sizing.
- Compact mode restores normal menu-bar visibility and contents margins when
  disabled.
- The hamburger menu bar is vertically centered in the toolbar/menu switch
  area.

Manual validation is still required for final visual alignment, theme
appearance, frameless window drag/resize behavior, and workbench-specific menu
content.

## Change Impact Strategy

Keep upstream patching safe by limiting mainline file changes:

- Prefer adding compact-specific files over expanding `MainWindow.cpp`.
- Keep compact UI behavior parameter-gated and disabled by default.
- Use existing FreeCAD command/menu actions where possible instead of
  duplicating command semantics.
- Do not alter normal toolbar, dock, menu, or status bar behavior when compact
  mode is disabled.
- Keep unrelated experiments, including compact panel access, in separate
  branches/specs.

## Evidence

- Code: `src/Gui/MainWindow.cpp`, `src/Gui/CompactMainWindowChrome.cpp`,
  `src/Gui/CompactTitleBarStyle.cpp`
- Config: `BaseApp/Preferences/MainWindow/CompactJetBrainsLayout`
- Tests: `tests/src/Gui/CompactMainWindowChrome.cpp`
