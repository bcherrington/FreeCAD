---
title: Python Console standalone window
doc_type: design
status: code-derived
owner: local-developer
last_reviewed: 2026-11-04
---

# Python Console Standalone Window

## Current Code Reconciliation — 2026-11-04

Source reviewed at `13d601b9dd6b66026e2d0dbb5c16156f18bdb592` on
`personal/integration-testing`. Python Console and Report View standalone
hosts are implemented in `src/Gui/MainWindow.cpp`. This document records
current source behavior; platform acceptance and upstream review remain open.
Reachable commits include `7b7360a7179`, `dfc3a203f28`, `0ee77cd0057`,
`880d547fa10` (console implementation/persistence/review fixes), and
`16b710fccce` (Report View).

A fresh offscreen GUI smoke attempt produced no result file;
it provides no passing host-transition evidence. Source inspection establishes
the implementation below, while the manual test matrix remains pending.
No live user document or MCP session was used for this review.

## Purpose

This document describes the local implementation associated with FreeCAD issue
[#28879](https://github.com/FreeCAD/FreeCAD/issues/28879): allow the Python
Console to open as a real top-level operating-system window, not only as a
docked or floating `QDockWidget`. The same host pattern is also implemented for the Report View/log panel.
Any upstream submission still needs a focused scope and validation evidence.

## Scope

This design covers the Python Console standalone-window workflow, its
relationship to the existing docked `View > Panels > Python Console` behavior,
and persistence of standalone state. Report View uses an analogous implementation.

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

The original Python Console (`Std_PythonView`) could be docked or floated as a
Qt dock widget. On macOS, floating dock widgets do not behave like independent
application windows: they have weaker multi-display behavior, poor integration
with Mission Control and Spaces, and awkward maximize/fullscreen semantics.

The requested behavior is an option to open the Python Console as a true
standalone window while keeping the existing docked panel workflow unchanged.
The issue discussion was not rechecked during this local-code reconciliation.

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
- `Std_SendToPythonConsole` and macro echo use the central console accessor.
  Host-aware activation must retain its manual focus-routing checks.

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

1. Get the existing shared console through `MainWindow::pythonConsole()`.
2. Save the dock visibility and detach its widget, keeping the original
   `QDockWidget` alive and hidden.
3. Set the console as the standalone `QMainWindow` central widget.
4. Restore screen-clamped geometry, then show/activate when requested.
5. Store standalone mode and geometry separately from dock layout state.

### Docked Transition

1. Take the console from the standalone central widget and hide that host.
2. Reattach it to the original dock, preserving the existing dock object.
3. Restore the dock's prior visibility and refresh overlay integration.

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
| Standalone preferences | `BaseApp/MainWindow/PythonConsoleWindow/Standalone` | Python Console controller | Restores docked or standalone mode. | Boolean; default to docked mode when absent. |
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

## Implemented User Experience And Remaining Expectations

- Keep `View > Panels > Python Console` as the existing docked panel toggle.
- Add a separate command, for example `Open Python Console in Window`.
- The new command opens the same console in a standalone top-level window.
- Add a convenience action to the Python Console panel title/header area:
  `Open in Window` when docked, and `Dock Console` when standalone. In overlay
  mode, the title bar appears on hover, so this action remains reachable without
  adding permanent chrome.
- A spontaneous window-manager close docks the shared console back.
  Application shutdown and non-spontaneous closes are accepted without docking;
  the host has `WA_DeleteOnClose` and `WA_QuitOnClose` disabled.
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

Current methods:

- `PythonConsole* MainWindow::pythonConsole() const`
- `void MainWindow::showPythonConsoleWindow(bool show = true)`
- `void MainWindow::dockPythonConsole()`
- `bool MainWindow::isPythonConsoleStandalone() const`

Current host:

- A small `PythonConsoleWindow` wrapper local to `MainWindow.cpp`.
- Use `Qt::Window`.
- Use no QWidget parent for OS-level independence.
- Use the shared `PythonConsole` as the `QMainWindow` central widget.
- Distinguish spontaneous user close from application/non-spontaneous close as described above.

## Persistence

Keep existing dock persistence untouched:

- main-window dock layout remains in main-window state
- dock visibility remains under `BaseApp/MainWindow/DockWindows/Std_PythonView`
- overlay state remains owned by `OverlayManager`

Store standalone state separately, for example under:

`BaseApp/MainWindow/PythonConsoleWindow`

Implemented keys:

- `Standalone`: boolean mode selection
- `Visible`: whether the standalone window was visible on shutdown
- `Geometry`: base64-encoded `saveGeometry()` data
- `DockVisibleBeforeWindow`: dock visibility to restore when docking back

Restore geometry with screen clamping so stale monitor layouts do not reopen
the window off-screen.

## Validation And Error Handling

- A spontaneous window-manager close docks the shared console back.
  Application shutdown and non-spontaneous closes are accepted without docking;
  the host has `WA_DeleteOnClose` and `WA_QuitOnClose` disabled.
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
- Workbench switching can re-add registered dock widgets. The original dock
  stays alive and hidden while standalone; workbench and overlay transitions
  still need manual validation.
- Overlay setup/unsetup belongs to the dock path. The standalone host should not
  participate in overlay title bars or overlay tabs.
- `Std_SendToPythonConsole` and any code using `findChild<PythonConsole*>()`
  route through the central accessor because a top-level unparented
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

## Report View Implementation

`ReportViewWindow`, `showReportViewWindow()` and `dockReportView()` in
`MainWindow.cpp` provide the analogous standalone mode for the existing
`ReportOutput` widget. It keeps its original dock alive, exposes window/dock
actions, and persists `Standalone`, `Visible`, `Geometry` and
`DockVisibleBeforeWindow` under `BaseApp/MainWindow/ReportViewWindow`.
Close handling follows the same spontaneous/application distinction as Python
Console. Report View also needs the host-transition, restart, overlay and
platform checks below. Implementation does not establish completed acceptance.

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
