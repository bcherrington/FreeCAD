/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Library General Public License for more details.                      *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QHash>
#include <QList>
#include <QMargins>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QVector>
#include <QWidget>
#include <fastsignals/signal.h>

#include <FCGlobal.h>

class QMenuBar;
class QMouseEvent;
class QAction;
class QToolButton;

namespace Gui
{

class MainWindow;

class GuiExport CompactMainWindowChrome: public QObject
{
    Q_OBJECT

public:
    explicit CompactMainWindowChrome(MainWindow* mainWindow);
    ~CompactMainWindowChrome() override;

    void setActive(bool active);
    bool isActive() const;

    void layoutChrome();
    void updateWindowControls();
    void updateHamburgerIcon();

    static bool shouldUseFramelessWindow();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct ResizeGrip
    {
        QWidget* widget = nullptr;
        Qt::Edges edges;
    };

    void setup();
    void setGlobalEventFilterActive(bool active);
    void syncMenuBar();
    void showMainMenu();
    void hideMainMenu();
    void openFirstMenu();
    void updateDocumentButton();
    void rebuildDocumentMenu();
    void rebuildMacroMenu();
    void selectMacro(const QString& filePath);
    void runSelectedMacro();
    void updateEditModeButton();
    void updateWorkbenchButton();
    void rebuildWorkbenchMenuButtons();
    void clearWorkbenchMenuButtons();
    void updateMdiTabBarVisibility();
    void applyContentsMargins();
    void layoutTopBar();
    void layoutResizeGrips();
    void createResizeGrips();
    void setResizeGripsVisible(bool visible);
    void startManualResize(Qt::Edges edges, QMouseEvent* event, QWidget* grip);
    void updateManualResize(QMouseEvent* event);
    void finishManualResize();

    QToolButton* createTitleButton(const QString& tooltip, QWidget* parent);
    void setButtonTextMetadata(QToolButton* button, const QString& text);
    void setupFlatButton(QToolButton* button);

private:
    MainWindow* mainWindow = nullptr;
    QWidget* topBar = nullptr;
    QWidget* switchArea = nullptr;
    QPointer<QWidget> toolBar = nullptr;
    QMenuBar* menuBar = nullptr;
    QToolButton* appIconButton = nullptr;
    QToolButton* menuButton = nullptr;
    QToolButton* documentButton = nullptr;
    QToolButton* macroButton = nullptr;
    QToolButton* editModeButton = nullptr;
    QToolButton* workbenchButton = nullptr;
    QAction* workbenchMenuInsertionPoint = nullptr;
    QList<QAction*> workbenchMenuTitleActions;
    QToolButton* minimizeButton = nullptr;
    QToolButton* maximizeButton = nullptr;
    QToolButton* closeButton = nullptr;
    QVector<ResizeGrip> resizeGrips;
    QHash<QObject*, Qt::Edges> resizeGripEdges;
    bool active = false;
    bool eventFilterInstalled = false;
    bool framelessWindow = false;
    bool titleDragActive = false;
    bool menuBarVisibleBefore = true;
    bool contentsMarginsSaved = false;
    bool manualResizeActive = false;
    bool mdiTabBarVisibilitySaved = false;
    bool mdiTabBarVisibleBefore = true;
    bool shuttingDown = false;
    int mdiTabBarMinimumHeightBefore = 0;
    int mdiTabBarMaximumHeightBefore = QWIDGETSIZE_MAX;
    Qt::Edges manualResizeEdges;
    QPoint titleDragGlobalPosition;
    QPoint titleDragWindowPosition;
    QPoint manualResizeGlobalPosition;
    QRect manualResizeGeometry;
    QMargins contentsMarginsBefore;
    QString selectedMacroFile;
    fastsignals::scoped_connection newDocumentConnection;
    fastsignals::scoped_connection deleteDocumentConnection;
    fastsignals::scoped_connection activeDocumentConnection;
    fastsignals::scoped_connection relabelDocumentConnection;
    fastsignals::scoped_connection renameDocumentConnection;
    fastsignals::scoped_connection changedDocumentConnection;
    fastsignals::scoped_connection finishSaveDocumentConnection;
    fastsignals::scoped_connection activateViewConnection;
    fastsignals::scoped_connection closeViewConnection;
    fastsignals::scoped_connection userEditModeConnection;
};

}  // namespace Gui
