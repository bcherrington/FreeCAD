---
title: JetBrains-style compact UI
doc_type: design
status: draft
owner: local-developer
last_reviewed: 2026-05-21
---

# JetBrains-Style Compact UI

## Purpose

This branch explores a JetBrains-style compact UI shell for FreeCAD. The goal is
to make panels easier to reach while keeping the central CAD viewport dominant.

## Scope

This design covers the compact main-window shell, side rail interaction model,
and relationship to FreeCAD's existing dock and overlay systems. It does not
define a full replacement for Qt docks, a new workbench system, or native
operating-system titlebar integration.

## Design Summary

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

## Components And Responsibilities

| Component | Responsibility | Owned Inputs | Owned Outputs |
| --- | --- | --- | --- |
| `Gui::MainWindow` | Own the compact shell preference, instantiate compact chrome, and forward relevant resize/theme/preference events. | User preference, normal main-window lifecycle | Compact chrome activation, minimal bridge into existing main-window behavior |
| `Gui::CompactMainWindowChrome` | Own compact header, frameless window behavior, side rails, menu/tool switch, document/macro/workbench controls, and compact layout reservation. | Existing menu bar, dock widgets, status bar, MDI area, workbench/menu actions | Compact titlebar, rail buttons, restored normal menu visibility, panel activation requests |
| `Gui::CompactTitleBarStyle` | Centralize compact titlebar button sizing, dropdown metrics, icon-menu metrics, and group spacing. | Toolbar icon-size preference and Qt toolbar metrics | Consistent titlebar button sizing and dropdown arrow placement |
| `DockWindowManager` | Continue creating, registering, showing, hiding, and persisting dock widgets. | Registered panel widgets and dock toggle actions | Existing `QDockWidget` hosts and View menu behavior |
| Overlay dock system | Keep existing overlay tabs and title bars available for docked panels. | Dock widgets and overlay preferences | Overlay panel activation and title-bar controls |
| Workbenches | Continue defining menus, toolbars, command bars, and dock windows. | Workbench activation and command definitions | Menus, toolbars, and registered panels |
| Compact rail buttons | Provide deterministic side access to registered dock windows. | Dock registry, slot mapping, panel icons | Show/hide requests through existing dock toggle actions |

## Data And Control Flow

1. The `BaseApp/Preferences/MainWindow/CompactJetBrainsLayout` preference gates
   compact mode.
2. `MainWindow` creates `CompactMainWindowChrome` once and toggles it from the
   preference.
3. When compact mode is enabled, compact chrome creates or shows the compact
   top bar, hides the normal menu bar, hides the MDI tab-bar container, reserves
   left/right workspace margins, and shows narrow left/right rail widgets.
4. The rail code lists current dock windows in deterministic slot order and
   assigns known panels to explicit defaults.
5. Clicking a rail button uses the existing dock toggle action path so normal
   dock handling and overlay mode see the same activation request as the View
   menu.
6. Disabling compact mode removes the extra chrome, restores the previous menu
   bar visibility, and returns `MainWindow` margins to normal.

## Contracts And Schemas

| Contract | Location | Producer | Consumer | Compatibility Notes |
| --- | --- | --- | --- | --- |
| Dock widget registration | `src/Gui/DockWindowManager.cpp` | Workbenches and main-window setup code | View menu, compact rail, overlay system | Compact mode should not bypass existing dock toggle actions. |
| Overlay title/tab behavior | `src/Gui/OverlayWidgets.h`, `src/Gui/OverlayManager.cpp` | Overlay manager | Docked panels and compact-mode users | Compact mode should remain compatible with overlay on/off state. |
| Main-window state | `src/Gui/MainWindow.cpp` | Main window and Qt dock system | Startup restore and preference packs | Compact rail margins and chrome must not corrupt saved dock layout. |
| Compact titlebar metrics | `src/Gui/CompactTitleBarStyle.cpp` | Compact chrome | Compact titlebar buttons and dropdowns | Keep local to compact UI so normal toolbar styling remains unchanged. |

## Configuration Model

| Config Source | Key Or Parameter | Applied By | Effect | Failure Mode |
| --- | --- | --- | --- | --- |
| User preferences | `BaseApp/Preferences/MainWindow/CompactJetBrainsLayout` | `Gui::MainWindow` | Enables or disables the compact shell prototype. | Defaults to normal FreeCAD UI when absent or false. |
| User preferences | `BaseApp/Preferences/MainWindow/CompactJetBrainsPanelSlots/<panel-id>` | `Gui::CompactMainWindowChrome` | Overrides the logical rail slot for a dock panel. Values are `left-upper`, `left-lower`, `right-upper`, `right-lower`, `bottom-left`, and `bottom-right`. | Falls back to known-panel defaults or current Qt dock area when absent or invalid. |

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

## FreeCAD And Qt Layout Constraints

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

## Tradeoffs And Constraints

Native titlebar integration is the hardest part. The current prototype supports
an optional restart-required frameless mode behind:

`BaseApp/Preferences/MainWindow/CompactJetBrainsFramelessWindow`

When enabled at startup, compact chrome supplies drag, minimize, maximize,
restore, close, and resize-edge behavior. This remains optional because
frameless behavior is platform-sensitive and should not affect the normal
FreeCAD window path.

The compact UI should continue to minimize changes to existing mainline files.
Use the splash-screen pattern as the model: `MainWindow` owns only the small
lifecycle bridge and preference hook; compact-specific behavior should live in
compact-specific files.

## Validation And Error Handling

Compact mode should always leave the existing View menu, toolbar visibility
controls, and dock-window behavior available as a fallback. Rail activation
should tolerate missing or renamed panels by relying on the current dock
registry and explicit defaults only where known.

## Security And Access

This design uses existing in-process Qt widgets and FreeCAD preferences. It
does not add network access, external process execution, credentials, or new
workspace permissions.

## Observability And Operations

Manual validation should check dock visibility, overlay mode, workbench
switching, menu restoration, saved layout behavior, and compact-mode preference
changes across restart.

## First Prototype

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

## Current Implementation Status

The compact UI prototype is disabled by default behind:

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

- Adds a compact top chrome implemented primarily in
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
- Adds narrow left and right rail widgets as `MainWindow` chrome, not
  `QDockWidget`s.
- Lists current dock windows as icon buttons in deterministic slot order.
- Known FreeCAD panels use explicit bundled FreeCAD icons instead of inheriting
  the application icon.
- Rail buttons can be dragged between compact rail regions. Dropping a button
  writes a logical slot override under
  `BaseApp/Preferences/MainWindow/CompactJetBrainsPanelSlots`.
- Moving a rail button also moves the associated panel's real `QDockWidget` to
  the Qt dock area mapped from the target compact slot.
- The compact rail slot model now supports persisted `left-upper`,
  `left-lower`, `right-upper`, `right-lower`, `bottom-left`, and
  `bottom-right` assignments over the existing Qt dock widgets.
- All compact slots behave identically: a slot can contain multiple rail
  buttons, but only one panel in that slot is expanded at a time. Clicking the
  active panel hides it; clicking another panel in the same slot hides the
  previous panel and shows the selected one.
- Clicking a rail button drives the existing dock toggle action so overlay mode
  sees the same path as the View menu. The compact rail code no longer calls
  `splitDockWidget()` on each click.
- Compact rail activation is isolated behind a compact chrome helper so the
  current dock-backed path can be replaced with an overlay-backed path later
  without changing button construction or slot persistence.
- Default slots currently map tree/model to upper left, selection/properties
  below the left separator, tasks to the right, Python console to bottom left,
  and report view to bottom right. Bottom-left and bottom-right buttons are
  bottom-aligned on the left and right rails.
- Compact mode reserves left/right contents margins while active so the rails
  are independent from the normal dock splitter layout.
- The status bar remains below compact rails and spans the full window width.
- Compact rails are slightly wider than the strict button size to avoid clipping
  the right edge of rail buttons under native styles.
- Optional frameless mode hides the native titlebar after restart and supplies
  compact chrome drag, resize, minimize, maximize/restore, and close behavior.

Current limitations:

- Panel content still uses existing Qt dock widgets. Compact rail activation is
  isolated behind a helper, but it does not yet present panels through
  FreeCAD's overlay widgets.
- The current rail drag feedback is a transitional implementation. It still
  uses one vertical toolbar per side and derives logical drop zones from pointer
  position. That makes the lower and bottom targets feel implicit rather than
  visible, and it should not be treated as the final interaction model.
- Frameless mode is implemented but remains parameter-gated and
  restart-required. Platform-specific behavior still needs wider manual
  validation.
- Toolbar alignment/drop-zone experiments were moved out of this compact UI
  branch and should remain separate until the desired Qt architecture is clear.

## Planned Rail Drag-and-Drop Replacement

The compact rail should move away from inferred drop zones and toward explicit
slot containers with real insertion placeholders. The target model is:

- Keep `QAction` as the source of truth for each panel launcher.
- Keep real `QToolButton`s for launcher widgets so FreeCAD keeps normal action
  text, tooltips, checked state, enabled state, accessibility names, icon
  sizing, palette behavior, and stylesheet compatibility.
- Replace each side's single vertical toolbar/drop-zone calculation with
  explicit slot containers:
  - `left-upper`
  - `left-lower`
  - `bottom-left`
  - `right-upper`
  - `right-lower`
  - `bottom-right`
- Layout the upper and lower slot containers near the top of the rail and the
  bottom slot container at the bottom, with a stretch between lower and bottom.
  Users should drop into visible containers, not into invisible thirds of a
  tall rail.
- During drag, insert a placeholder widget directly into the target slot layout
  at the eventual insertion index. This should create real space where the icon
  will land, matching toolbar-style insertion behavior more closely than a
  floating outline.
- Paint slot hover and placeholder feedback with `QPalette`/`QStyle`, or with
  compact-specific object names that themes can target. Avoid hard-coded colors
  and avoid visual feedback that only appears as an outline.
- Persist slot assignment independently from widget type. Keep
  `CompactJetBrainsPanelSlots/<panel-id>` for slot placement and add an order
  preference only when intra-slot reordering is implemented.

Qt architecture notes:

- `QToolBar`/`QToolButton` remains a good fit for command launchers, but native
  toolbar dragging is not designed for FreeCAD's logical rail slots. If the
  current toolbar container keeps forcing custom hit testing, migrate the
  container layer only.
- A custom compact rail widget containing slot widgets and `QToolButton`s is
  the preferred replacement path. It keeps FreeCAD action integration while
  giving full control over visible slots, insertion placeholders, and bottom
  alignment.
- `QListView` with a model/delegate is a defensible alternative only if the rail
  becomes a true model/view collection with richer keyboard reordering,
  selection, or multi-item drag-and-drop requirements. It would require wrapping
  FreeCAD action state into model data.
- `QTabBar` should not be used for this rail. The rail entries are launchers for
  dock panels, not tabs in one page stack, and cross-slot dragging would still
  require custom behavior.
- `QDockWidget`/`QMainWindow` docking should continue to own the panel windows
  themselves, but it should not be used to implement the compact icon rail.

## Layout Churn Investigation

Profiling the current compact UI prototype exposed a layout feedback loop that
must be addressed during the rails redesign.

Observed symptom:

- The 3D viewport repeatedly alternates between two widths that differ by
  `56px`, for example `1720x986` and `1664x986`.
- Each alternation reaches Qt's `QOpenGLWidget` backing framebuffer path and
  recreates full-size multisampled render targets.
- In an interactive trace, the GL interposer counted hundreds of repeated
  framebuffer/renderbuffer allocations and deletes:
  - `glGenFramebuffers`: `754`
  - `glDeleteFramebuffers`: `754`
  - `glGenRenderbuffers`: `754`
  - `glDeleteRenderbuffers`: `754`
  - `glRenderbufferStorageMultisample`: `752`
- The repeated renderbuffer sizes matched the viewport bounce, including
  `1720x986` and `1664x986`.

Quarter viewport instrumentation confirmed the geometry churn is real for one
viewport, not a counting artifact:

- `QuarterWidget.uniqueWidgets`: `1`
- `CustomGLWidget.uniqueWidgets`: `1`
- `QuarterWidget.resizeEvent`: `4032`
- `QuarterWidget.viewport.event.Resize`: `4032`
- `QuarterWidget.viewport.event.Move`: `4031`
- Observed viewport sizes:
  - `1664x986`, DPR `1.00`: `4033`
  - `1720x986`, DPR `1.00`: `4030`

Compact layout tracing identified the source of the `56px` bounce:

- `layoutChrome`: `32`
- `layoutChrome.MainWindow.LayoutRequest`: `27`
- `layoutChrome.changed.MainWindow.LayoutRequest`: `27`
- `central.1720x1016.to.1664x1016.deltaW56.source.MainWindow.LayoutRequest`:
  `27`

Interpretation:

- `QMainWindow::event()` handles a `LayoutRequest`.
- Qt's normal `QMainWindow` layout expands the central widget to the full
  available width.
- `MainWindow::event()` then calls `CompactMainWindowChrome::layoutChrome()`.
- `CompactMainWindowChrome::layoutWorkArea()` manually calls
  `central->setGeometry(...)`, shrinking the central widget by
  `compactPanelStripWidth()` on both sides.
- In the profiled run, `compactPanelStripWidth()` was `28px`, so both rails
  account exactly for the `56px` width delta.

Relevant source:

- `src/Gui/MainWindow.cpp`: `MainWindow::event()` calls compact chrome layout
  after `QMainWindow::event()` for `Resize`, `Show`, `LayoutRequest`, and
  `WindowStateChange`.
- `src/Gui/CompactMainWindowChrome.cpp`: `layoutWorkArea()` sets the central
  widget left edge to `compactPanelStripWidth()` and the right edge to
  `mainWindow->width() - compactPanelStripWidth() - 1`.
- `src/Gui/CompactMainWindowChrome.cpp`: `compactPanelStripWidth()` is button
  width plus rail margins and clearance.

Root-cause hypothesis:

Compact chrome is fighting `QMainWindow` for ownership of the central widget's
geometry. `QMainWindow` lays out the central widget, compact chrome manually
overrides it, and later layout requests repeat the cycle. That central-widget
oscillation propagates into the `QuarterWidget` viewport and causes expensive
OpenGL framebuffer churn.

Rails redesign implications:

- Do not reserve rail space by repeatedly calling `central->setGeometry()` from
  compact chrome.
- Prefer a Qt-owned layout approach for persistent rail reservation, such as
  contents margins, a wrapper central widget, or another layout structure where
  Qt has one authoritative geometry source.
- If the rails are meant to overlay the viewport, do not shrink the central
  widget at all. Keep the central widget at normal `QMainWindow` size and raise
  the rail widgets above it.
- Avoid running full compact chrome relayout on every `MainWindow.LayoutRequest`
  unless a compact-owned input actually changed.
- The rails reimplementation should make the geometry contract explicit:
  either rails reserve space through Qt layout ownership, or they are overlays.
  It should not mix overlay widgets with manual central-widget resizing.

Temporary trace switches used during this investigation:

- `FREECAD_QUARTER_GL_TRACE=1`
- `FREECAD_COMPACT_LAYOUT_TRACE=1`

## Current Test Coverage

`tests/src/Gui/CompactMainWindowChrome.cpp` covers compact UI behavior that can
be validated without screenshot comparison:

- Titlebar and rail buttons expose accessible names/status text and match
  compact toolbar icon sizing.
- Compact mode restores normal menu-bar visibility and contents margins when
  disabled.
- The hamburger menu bar is vertically centered in the toolbar/menu switch
  area.
- Left/right rail buttons fit inside their rail content and are not clipped.
- Persisted compact panel slot overrides move rail buttons and their associated
  dock panels away from their default rail/dock area.

Manual validation is still required for final visual alignment, theme
appearance, frameless window drag/resize behavior, and workbench-specific menu
content.

## Change Impact Strategy

Keep upstream patching safe by limiting mainline file changes:

- Prefer adding compact-specific files over expanding `MainWindow.cpp`.
- Keep compact UI behavior parameter-gated and disabled by default.
- Use existing FreeCAD command/menu/dock actions where possible instead of
  duplicating command semantics.
- Do not alter normal toolbar, dock, menu, or status bar behavior when compact
  mode is disabled.
- Keep unrelated experiments, such as custom toolbar drop zones and OS file
  thumbnailing, in separate branches/specs.

## Evidence

- Code: `src/Gui/MainWindow.cpp`, `src/Gui/DockWindowManager.cpp`,
  `src/Gui/Workbench.cpp`, `src/Gui/OverlayWidgets.h`,
  `src/Gui/OverlayManager.cpp`, `src/Gui/ComboView.cpp`,
  `src/Gui/TaskView/TaskView.cpp`
- Config: `BaseApp/Preferences/MainWindow/CompactJetBrainsLayout`
- Tests: manual UI validation required
- Runbooks: none
- Requirements: none

## Related Docs

- `docs/contribution-overview.md`

## Research Commands Used

Local exploration was intentionally targeted because this repository is large.

```sh
rg -n "class OverlayTabWidget|class OverlayTitleBar|class OverlayProxyWidget|setTitleBarWidget|addWidget\\(|removeWidget\\(|toggle|Side|Tab" src/Gui/Overlay*.h src/Gui/Overlay*.cpp
nl -ba src/Gui/OverlayWidgets.h | sed -n '1,280p'
nl -ba src/Gui/ComboView.cpp | sed -n '1,220p'
nl -ba src/Gui/TaskView/TaskView.cpp | sed -n '1,220p'
```
