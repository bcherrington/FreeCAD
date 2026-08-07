---
title: JetBrains-style compact titlebar
doc_type: design
status: active
owner: bcherrington
last_reviewed: 2026-08-07
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
| User preferences | `BaseApp/Preferences/MainWindow/CompactJetBrainsFramelessWindow` | `Gui::MainWindow` at startup | Replaces native window decorations with compact drag, resize, and window controls. | Defaults to native window decorations when absent or false; restart is required after changing it. |

## Layout And Refresh Contracts

Compact chrome must converge once startup or an explicit window transition has
settled. `Gui::MainWindow` therefore invokes the full compact layout only for
show, resize, and window-state changes. A bare `QEvent::LayoutRequest` is not a
full-layout trigger: routing it back through `layoutChrome()` can turn
`updateGeometry()` into a self-sustaining Qt event loop.

MDI tab suppression is idempotent. Compact mode saves the prior tab visibility
and height constraints, changes them only when they differ from the compact
target, and requests geometry updates only after a real state change. Disabling
compact mode restores the saved visibility, constraints, menu-bar state, and
contents margins.

Document-button metadata is independent of geometry layout. Document and view
signals schedule one receiver-bound queued refresh per event-loop turn. A burst
therefore renders the latest state once; deactivation or object destruction
cancels the pending work safely. Layout-only activity does not refresh document
metadata.

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
- Leaves a settled compact window quiescent: repeated `LayoutRequest` delivery
  does not re-enter the full compact relayout path.
- Coalesces document/view signal bursts into one latest-state document-button
  refresh per event-loop turn.
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
that can be validated without screenshot comparison, including:

- Titlebar buttons expose accessible names/status text and match compact toolbar
  icon sizing.
- Compact mode restores normal menu-bar visibility and contents margins when
  disabled.
- The hamburger menu bar is vertically centered in the toolbar/menu switch
  area.
- A bare `LayoutRequest` does not invoke compact relayout, and layout events
  converge within bounded event-loop turns.
- Stable MDI tab suppression produces no redundant geometry invalidation.
- Document-button refresh bursts coalesce, and queued work is safe across
  deactivation and `MainWindow` destruction.
- Compact activation/deactivation restores saved menu, margin, visibility, and
  MDI tab constraints; compact-disabled layout requests remain quiescent.
- Deferred workbench activation updates settle without sustained layout work.

Manual validation is still required for final visual alignment, theme
appearance, pointer-driven frameless drag/resize behavior, and
workbench-specific menu content on each supported window system. This residual
belongs to the FreeCAD GUI maintainer or release tester validating the target
platform; it does not change the X11 idle-loop acceptance described below.

## Idle CPU Regression Diagnosis

Use one committed Debug binary and one controlled user home for all cases.
Restart after changing the two compact flags, close documents and the Start
page, wait 30 seconds, and sample the Qt main thread three times for:

1. layout off, frameless off;
2. layout on, frameless off; and
3. layout on, frameless on.

For example:

```sh
pidstat -t -p <freecad-pid> 5 3
```

When `perf` is unavailable, launch the same binary under `gdb`, interrupt it
after the warm-up, and capture repeated main-thread backtraces. A healthy idle
stack waits in `ppoll`/GLib/`QEventDispatcherGlib`/`QEventLoop`. Repeated
`CompactMainWindowChrome::layoutChrome`, `updateMdiTabBarVisibility`,
`QWidget::updateGeometry`, or `QLayout` frames indicate a layout feedback
cycle. CPU percentages are comparative evidence, not a portable absolute
threshold.

The 2026-08-07 Linux/X11 validation of committed Debug build
`1.0rc1-9333-g62ff9e3b1c5` recorded main-thread averages of 0.13% with both
flags off, 0.13% with compact framed, and 0.07% with compact frameless. All
nine debugger snapshots were in the sleeping Qt dispatcher path.

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
