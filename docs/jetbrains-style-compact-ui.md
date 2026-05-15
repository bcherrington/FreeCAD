# JetBrains-Style Compact UI Notes

This branch explores a JetBrains-style compact UI shell for FreeCAD. The goal is
to make panels easier to reach while keeping the central CAD viewport dominant.

## Design Direction

Approach this as a UI shell prototype, not a workbench rewrite.

FreeCAD already has an overlay dock system in addition to normal `QDockWidget`
panels. That is important because the JetBrains-style "side buttons open
panels" model may fit better as an evolution of overlay tabs than as a rewrite
of every dock widget.

The safest direction is:

1. Keep FreeCAD's existing `QDockWidget` panels.
2. Add a JetBrains-like compact shell mode.
3. Reuse or extend the existing overlay dock system for side icon bars.
4. Add a logical panel-slot model over Qt docks.

## JetBrains UI Model

JetBrains IDEs use a simplified window header, collapsed main menu on
Windows/Linux, compact toolbar/header density, and side tool-window bars that
expose panels by icon.

Useful behaviors to mirror:

- Main menu can be merged into the toolbar, hidden under a hamburger button, or
  shown above the toolbar.
- Tool-window icons live on left and right bars.
- Upper side-bar icons open vertical side panels.
- Lower side-bar icons open bottom panels.
- Tool windows can be moved, resized, detached, split vertically or
  horizontally, and remembered as part of the layout.
- Compact mode reduces toolbar heights, tool-window headers, padding, and icon
  sizes.

References:

- JetBrains New UI: https://www.jetbrains.com/help/idea/new-ui.html
- JetBrains Tool Windows: https://www.jetbrains.com/help/idea/tool-windows.html
- Arrange Tool Windows: https://www.jetbrains.com/help/webstorm/manipulating-the-tool-windows.html
- IntelliJ Platform UI Overview: https://plugins.jetbrains.com/docs/intellij/ui-overview.html

## FreeCAD/Qt Layout Reality

FreeCAD's main UI is already a `QMainWindow`. It owns the central `QMdiArea`,
status bar, menu bar, toolbars, and dock windows.

Relevant code:

- `src/Gui/MainWindow.h`: `Gui::MainWindow` derives from `QMainWindow`.
- `src/Gui/MainWindow.cpp`: constructs the `QMdiArea`, status bar, dock setup,
  and saved main-window state.
- `src/Gui/DockWindowManager.cpp`: creates, registers, shows, hides, and
  persists dock widgets.
- `src/Gui/Workbench.cpp`: workbenches define menus, toolbars, command bars, and
  dock windows.
- `src/Gui/MenuManager.cpp`: rebuilds the active workbench menu bar.
- `src/Gui/ToolBarManager.cpp`: manages normal toolbars plus toolbar areas in
  menu-bar corners and the status bar.
- `src/Gui/OverlayWidgets.h` and `src/Gui/OverlayManager.cpp`: existing overlay
  tab/dock system.
- `src/Gui/ComboView.cpp`: existing combined Tree and Property view using a
  vertical splitter.
- `src/Gui/TaskView/TaskView.cpp`: task-panel implementation used by many
  modeling operations.

Qt supports the needed primitives: `QMainWindow` dock areas, `addDockWidget`,
`tabifyDockWidget`, split/nested docks, vertical dock tabs, custom dock title
bars through `QDockWidget::setTitleBarWidget`, and dock options such as
`AllowNestedDocks`, `AllowTabbedDocks`, `ForceTabbedDocks`, `VerticalTabs`, and
`GroupedDragging`.

## Logical Panel Slots

Qt has physical dock areas, but the desired layout is more specific than native
`QMainWindow` areas. Model these as FreeCAD-level logical slots and map them to
Qt docks, split docks, tabified docks, overlay tabs, and saved state.

Candidate slots:

- `left-upper`
- `left-lower`
- `right-upper`
- `right-lower`
- `bottom-left`
- `bottom-right`
- `top-upper`
- `top-lower`

The top slots should remain optional until there is a concrete need. Native top
docking can compete with toolbar/header space.

Panels in the same group should replace the currently open panel for that group
instead of opening more permanent chrome. This matches the JetBrains "select a
tool window from the stripe" interaction.

## Risk Areas

Native titlebar integration is the hardest part. Putting toolbars into the
actual OS titlebar usually means custom or frameless window chrome, window
dragging logic, system buttons, platform quirks, Wayland/X11/Windows/macOS
differences, and accessibility concerns.

A better first milestone is a JetBrains-like header below the native titlebar,
using FreeCAD's existing menu-bar corner toolbar support where possible. Native
titlebar integration can be explored later.

## First Prototype Scope

Build a preference-gated mode, for example:

`BaseApp/Preferences/MainWindow/CompactJetBrainsLayout`

First milestone:

- Add a hamburger menu button that opens the current `QMenuBar` menus.
- Hide or reduce the visible menu bar when compact mode is enabled.
- Move key toolbar areas into a compact top header or menu-bar corner area.
- Add left and right vertical side strips with buttons for registered dock
  windows.
- Clicking a side button shows one panel in that side group and hides or
  replaces the previous panel in that group.
- Leave the existing View menu, toolbar visibility controls, and current docking
  behavior available as a fallback.

This should provide the interaction model without breaking FreeCAD's existing
workbench, menu, toolbar, and dock-window architecture.

## Current POC

The first proof of concept is implemented in `src/Gui/MainWindow.cpp` and is
disabled by default behind:

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

- Adds a compact top toolbar with a hamburger button. Clicking it toggles
  FreeCAD's normal horizontal menu bar; runnable menu actions collapse it after
  they trigger.
- Hides the normal menu bar while compact mode is active, then restores its
  previous visibility when compact mode is disabled.
- Adds narrow left and right rail widgets as `MainWindow` chrome, not
  `QDockWidget`s.
- Lists current dock windows as icon buttons in deterministic slot order.
- Known FreeCAD panels use explicit bundled FreeCAD icons instead of inheriting
  the application icon.
- Clicking a rail button drives the existing dock toggle action so overlay mode
  sees the same path as the View menu; clicking it again hides the panel. The
  compact rail code no longer calls `splitDockWidget()` on each click.
- Default slots currently map tree/model to upper left, selection/properties
  below the left separator, tasks to the right, Python console to bottom left,
  and report view to bottom right. Bottom-left and bottom-right buttons are
  bottom-aligned on the left and right rails.
- Compact mode reserves left/right contents margins while active so the rails
  are independent from the normal dock splitter layout.

Current limitations:

- Strip buttons are assigned by hard-coded defaults or current Qt dock area;
  drag-and-drop movement of the buttons is not implemented yet.
- Panel content still uses existing Qt dock widgets. A later pass should route
  compact rail activation through FreeCAD's overlay dock system.
- This does not yet integrate with the native OS titlebar.
- The implementation is intentionally local to `MainWindow` until the desired
  interaction model is validated.

## Research Commands Used

Local exploration was intentionally targeted because this repository is large.

```sh
rg -n "class OverlayTabWidget|class OverlayTitleBar|class OverlayProxyWidget|setTitleBarWidget|addWidget\\(|removeWidget\\(|toggle|Side|Tab" src/Gui/Overlay*.h src/Gui/Overlay*.cpp
nl -ba src/Gui/OverlayWidgets.h | sed -n '1,280p'
nl -ba src/Gui/ComboView.cpp | sed -n '1,220p'
nl -ba src/Gui/TaskView/TaskView.cpp | sed -n '1,220p'
```
