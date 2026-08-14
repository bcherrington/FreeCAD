---
title: Python Console standalone window
doc_type: design
status: draft
owner: local-developer
last_reviewed: 2026-05-18
---

# Python Console Standalone Window

## Purpose

This document prepares the implementation for FreeCAD issue
[#28879](https://github.com/FreeCAD/FreeCAD/issues/28879): allow the Python
Console to open as a real top-level operating-system window, not only as a
docked or floating `QDockWidget`. The same pattern is also relevant to the
Report View/log panel, but the first implementation should stay focused on the
Python Console and avoid turning this into a broad dock-system rewrite.

## Scope

This design covers the Python Console standalone-window workflow, its
relationship to the existing docked `View > Panels > Python Console` behavior,
and persistence of standalone state. Report View support is documented only as
a follow-up.

## Design Summary

Keep exactly one `PythonConsole` instance and move that widget between two
hosts:

1. The existing `QDockWidget` host managed by `DockWindowManager`.
2. A new top-level host widget with `Qt::Window` and no QWidget parent.

Do not create a second console. `PythonConsole` owns Python stdin/stdout/stderr
redirectors, history, timers, prompt state, and interpreter integration; two
instances would risk conflicting Python I/O ownership and confusing state.

Do not use `QDockWidget::setFloating(true)` for this feature. A floating dock is
still part of the main-window dock system and does not solve the OS-window
behavior requested in the issue.

## Issue Summary

The current Python Console (`Std_PythonView`) can be docked or floated as a
Qt dock widget. On macOS, floating dock widgets do not behave like independent
application windows: they have weaker multi-display behavior, poor integration
with Mission Control and Spaces, and awkward maximize/fullscreen semantics.

The requested behavior is an option to open the Python Console as a true
standalone window while keeping the existing docked panel workflow unchanged.
The GitHub issue currently has no additional comments beyond the original
problem report.

## Current Architecture

- `MainWindow::setupPythonConsole()` creates one `Gui::PythonConsole` and
  registers it as `Std_PythonView`.
- `StdWorkbench::setupDockWindows()` adds `Std_PythonView` as a bottom dock
  panel.
- `DockWindowManager::addDockWindow()` wraps registered dock widgets in a
  `QDockWidget`; the Python Console dock object name is `Python console`, while
  the toggle action data is `Std_PythonView`.
- `View > Panels` is populated dynamically from existing `QDockWidget`
  `toggleViewAction()` objects.
- `Std_SendToPythonConsole` currently reveals the console through
  `DockWindowManager::activate()`.

## Components And Responsibilities

| Component | Responsibility | Owned Inputs | Owned Outputs |
| --- | --- | --- | --- |
| `Gui::MainWindow` | Own the Python Console accessor, mode switch, menu commands, and host transitions. | Existing `PythonConsole`, dock manager state, standalone preference state | Docked or standalone console visibility, focus routing, saved geometry |
| `Gui::PythonConsole` | Provide the single console widget and preserve interpreter interaction state. | Python I/O redirects, history, prompt state, user commands | Console output, command execution, persistent console state across host moves |
| `DockWindowManager` | Keep the normal docked panel host and View menu behavior. | `Std_PythonView` registration and `QDockWidget` host | Dock visibility, dock layout persistence, overlay setup path |
| `PythonConsoleWindow` | Host the shared console as a top-level OS window. | Shared `PythonConsole`, saved window geometry | Independent window, dock-back command, close handling |
| Python Console actions | Expose explicit transitions between docked and standalone modes. | Menu command, panel/header/context-menu action, standalone toolbar action | Calls to the central `MainWindow` controller path |

## Data And Control Flow

### Standalone Transition

1. Get the existing console.
2. If it is docked, detach it with
   `DockWindowManager::removeDockWindow("Python console")`.
3. Put the returned widget in the standalone host.
4. Show, raise, activate, and focus the console.
5. Store standalone mode and geometry separately from dock layout state.

### Docked Transition

1. Hide the standalone host and save its geometry.
2. Remove the console from the standalone layout.
3. Register/add it back as `Std_PythonView` through the existing dock path.
4. Reveal and focus it through the dock toggle/action path.

### Focus Routing

`Std_SendToPythonConsole` and macro command echo should use a central
`MainWindow::pythonConsole()` accessor and focus whichever host is active.

## Contracts And Schemas

| Contract | Location | Producer | Consumer | Compatibility Notes |
| --- | --- | --- | --- | --- |
| Python Console dock registration | `src/Gui/MainWindow.cpp`, `src/Gui/Workbench.cpp`, `src/Gui/DockWindowManager.cpp` | Main-window setup and Standard workbench | View menu, dock manager, overlay system | Preserve `Std_PythonView` and existing docked panel behavior. |
| Console access | `MainWindow::pythonConsole()` | `Gui::MainWindow` | `Std_SendToPythonConsole`, macro command echo, future console callers | Avoid relying on `findChild<PythonConsole*>()` when the console is standalone. |
| Standalone host state | `BaseApp/MainWindow/PythonConsoleWindow` | `Gui::MainWindow` or standalone host | Startup restore and mode transitions | Keep separate from Qt dock layout state. |
| User-facing actions | `src/Gui` command/action wiring | Main window and console host | Menus, title/header/context menus, standalone toolbar | Strings must be translatable and should avoid the word "floating". |

## Configuration Model

| Config Source | Key Or Parameter | Applied By | Effect | Failure Mode |
| --- | --- | --- | --- | --- |
| Dock preferences | `BaseApp/MainWindow/DockWindows/Std_PythonView` | Existing dock restore path | Preserves docked Python Console visibility. | Existing dock fallback remains available. |
| Standalone preferences | `BaseApp/MainWindow/PythonConsoleWindow/Mode` | Python Console controller | Restores docked or standalone mode. | Default to docked mode when absent or invalid. |
| Standalone preferences | `BaseApp/MainWindow/PythonConsoleWindow/Visible` | Python Console controller | Restores standalone visibility on startup. | Hide the standalone host when absent. |
| Standalone preferences | `BaseApp/MainWindow/PythonConsoleWindow/Geometry` | `PythonConsoleWindow` | Restores standalone window placement and size. | Clamp stale geometry to current screens. |

## Contribution Compliance Notes

This change should follow FreeCAD's contribution process and Developer
Handbook guidance:

- Keep the PR scoped to issue #28879: the first patch should implement the
  Python Console standalone window only. Treat Report View/log support as a
  follow-up unless maintainers explicitly ask to include it.
- Avoid new dependencies. The design can be implemented with existing Qt and
  FreeCAD GUI infrastructure.
- Preserve existing public Python APIs and the current `View > Panels >
  Python Console` docked workflow.
- Use the prevailing C++ style in `src/Gui` and let `pre-commit`/clang-format
  handle touched C++ files. Do not reformat unrelated code.
- Any new source/header files should include the repository's usual license
  header/SPDX treatment.
- User-visible strings such as `Open Python Console in Window` and
  `Dock Console` must be translatable with Qt translation functions.
- If a new helper class is added, keep UI code and mode-management logic
  separated enough that the behavior is reviewable and later reusable.
- PR text must describe the UI change and include manual test results; because
  this is UI/window-management work, screenshots or a short screen recording
  would be useful.

## Proposed User Experience

- Keep `View > Panels > Python Console` as the existing docked panel toggle.
- Add a separate command, for example `Open Python Console in Window`.
- The new command opens the same console in a standalone top-level window.
- Add a convenience action to the Python Console panel title/header area:
  `Open in Window` when docked, and `Dock Console` when standalone. In overlay
  mode, the title bar appears on hover, so this action remains reachable without
  adding permanent chrome.
- Closing the standalone window hides it; it must not destroy the console.
- Showing the docked panel while standalone is active should re-dock the same
  console and hide the standalone host.
- `Send to Python Console` should reveal and focus whichever host is active.
- The title/header action should call the same controller path as the menu
  command. If wiring the action into both normal and overlay title bars is too
  invasive for the first patch, add it to the Python Console context menu as a
  fallback and keep the title/header button as a follow-up.

Avoid using "floating" in UI text because Qt floating docks already use that
term and are the behavior this issue is trying to bypass.

## Implementation Plan

Add a Python-console-specific controller path, likely owned by `MainWindow`.
Keep the first implementation narrow, but avoid names and structure that would
block a later Report View/log-panel implementation.

Current implementation status:

- `MainWindow` owns the mode switch and keeps one shared `PythonConsole`.
- `View > Panels > Python Console` appears at the Python Console's normal
  position in the panel list and contains `Show Python Console`, `Panel`, and
  `Window` actions.
- Closing the standalone window now docks the console back into the panel area
  instead of only hiding the standalone host.
- The original `QDockWidget` is kept alive while the console is standalone.
  This lets Qt preserve the previous dock area, splitter size, and tab group
  when the console returns to the panel.
- The Python Console context menu has `Open Python Console in Window` /
  `Dock Python Console` as the first convenience action.
- The dock title bar gets an `Open Python Console in Window` action. Overlay
  title bars include dock-specific actions before their existing overlay,
  float, and close buttons.
- The standalone window has a small toolbar action to dock the console again.
- The standalone/dock actions use dedicated Python Console icons registered in
  `src/Gui/Icons/resource.qrc`. They should remain distinct from model view
  direction icons such as `view-bottom`.
- `Std_SendToPythonConsole` and macro command echo use the central
  `MainWindow::pythonConsole()` accessor.

Suggested methods:

- `PythonConsole* MainWindow::pythonConsole() const`
- `void MainWindow::showPythonConsoleDocked()`
- `void MainWindow::showPythonConsoleWindow()`
- `void MainWindow::dockPythonConsole()`
- `bool MainWindow::isPythonConsoleStandalone() const`

Suggested host:

- A small `PythonConsoleWindow` wrapper, either local to `MainWindow.cpp` or
  near `PythonConsole`.
- Use `Qt::Window`.
- Use no QWidget parent for OS-level independence.
- Use a zero-margin layout containing the shared `PythonConsole`.
- Override close handling to hide and save geometry instead of deleting.

## Persistence

Keep existing dock persistence untouched:

- main-window dock layout remains in main-window state
- dock visibility remains under `BaseApp/MainWindow/DockWindows/Std_PythonView`
- overlay state remains owned by `OverlayManager`

Store standalone state separately, for example under:

`BaseApp/MainWindow/PythonConsoleWindow`

Recommended keys:

- `Mode`: `Docked` or `Standalone`
- `Visible`: whether the standalone window was visible on shutdown
- `Geometry`: `saveGeometry()` data

Restore geometry with screen clamping so stale monitor layouts do not reopen
the window off-screen.

## Validation And Error Handling

- Closing the standalone window hides it; it must not destroy the console.
- Showing the docked panel while standalone is active should re-dock the same
  console and hide the standalone host.
- `Send to Python Console` should reveal and focus whichever host is active.
- Workbench switching should not duplicate or silently re-dock the console.
- Overlay setup/unsetup belongs to the dock path. The standalone host should
  not participate in overlay title bars or overlay tabs.
- `WA_DeleteOnClose` must not delete the standalone host while it owns the
  console unless the console is removed first.

## Security And Access

This design uses existing in-process Qt widgets, FreeCAD preferences, and the
existing Python Console integration. It adds no new dependencies, network
access, external process execution, credential handling, or workspace
permissions.

## Observability And Operations

Manual validation should report the platforms tested, display configuration,
restart behavior, host transitions, overlay mode behavior, and Python Console
I/O continuity. UI/window-management behavior is difficult to cover fully with
automated tests, so PR notes should include manual results and screenshots or a
short recording where useful.

## Icon Notes

The Python Console window/dock icons should follow the draft FreeCAD icon art
guide: <https://github.com/obelisk79/IconArtGuide>.

Relevant requirements for this change:

- Use a 24 px icon canvas and respect the 2 px protected perimeter.
- Keep shapes simple and readable in grayscale.
- Use a neutral gray base plus one action color rather than multiple competing
  colors.
- Avoid using the FreeCAD logo inside command icons.
- Use palette colors already common in FreeCAD icons, such as neutral grays and
  FreeCAD blue.
- Keep outlines and defining lines light enough to match existing FreeCAD
  toolbar icons.

For this feature, `python-console-window.svg` represents moving the console to
a standalone top-level window, and `python-console-dock.svg` represents moving
the same console back into the dock panel. Do not reuse model-view orientation
icons for these actions, because those icons already mean camera/view direction.

## Tradeoffs And Constraints

- A fully generic standalone-dock system would touch many more surfaces:
  overlay title bars, workbench dock registration, View > Panels population,
  saved layouts, preference packs, bottom-panel toggling, and tabified docks.
  Start with Python Console, then extract a reusable helper once the behavior is
  proven.
- Workbench switching can re-add registered dock widgets. Standalone mode should
  avoid leaving `Std_PythonView` registered as a normal dock item while the
  console is hosted by the standalone window.
- Overlay setup/unsetup belongs to the dock path. The standalone host should not
  participate in overlay title bars or overlay tabs.
- `Std_SendToPythonConsole` and any code using `findChild<PythonConsole*>()`
  may need to route through a central accessor because a top-level unparented
  console will not necessarily be found as a `MainWindow` child.
- `WA_DeleteOnClose` must not delete the standalone host while it owns the
  console unless the console is removed first.
- Preference packs and saved layouts should not encode standalone geometry into
  Qt dock layout state.

## Evidence

- Code: `src/Gui/MainWindow.cpp`, `src/Gui/DockWindowManager.cpp`,
  `src/Gui/Workbench.cpp`, `src/Gui/CommandWindow.cpp`,
  `src/Gui/Icons/resource.qrc`
- Config: `BaseApp/MainWindow/DockWindows/Std_PythonView`,
  `BaseApp/MainWindow/PythonConsoleWindow`
- Tests: manual UI validation required
- Runbooks: none
- Requirements: FreeCAD issue
  [#28879](https://github.com/FreeCAD/FreeCAD/issues/28879)

## Related Docs

- `docs/checklists/contribution-overview.md`

## Report View Follow-Up

The Report View/log panel has a similar multi-monitor use case. It is also a
bottom dock registered by `MainWindow::setupReportView()` as `Std_ReportView`.
Once the Python Console path is proven, the same standalone host/controller
shape can be extended to Report View:

- keep `View > Panels > Report View` unchanged
- add `Open Report View in Window`
- move the existing `ReportOutput` widget between its dock host and a true
  top-level host
- add an `Open in Window` / `Dock Report View` title/header action
- persist report-window geometry separately from dock layout

Report View is less risky than Python Console because it does not own Python
stdin/stdout interpreter input, but it still participates in the same
`DockWindowManager`, overlay, bottom-panel toggle, and saved layout systems.
Avoid implementing it first if issue #28879 remains specifically scoped to the
Python Console.

## Test Matrix

Required manual tests:

- macOS: move standalone console to a second display.
- macOS: switch Spaces and Mission Control with the console open.
- macOS: maximize or fullscreen the standalone console.
- macOS: test with "Displays have separate Spaces" enabled and disabled.
- Linux and Windows: open standalone, dock back, close standalone, restart.
- Confirm `View > Panels > Python Console` remains unchanged in docked mode.
- Confirm `Send to Python Console` focuses the standalone console when active.
- Confirm Python history, stdout/stderr, pasted commands, and prompts survive
  mode switches.
- Confirm overlay mode on/off does not affect standalone mode.
- Confirm workbench switching does not duplicate or silently re-dock the console.
