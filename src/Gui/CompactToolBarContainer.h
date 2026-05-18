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
#include <QByteArray>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QWidget>

#include <FCGlobal.h>

class QBoxLayout;
class QContextMenuEvent;
class QEvent;
class QLabel;
class QMouseEvent;
class QPaintEvent;
class QToolBar;
class QWidget;

namespace Gui
{

class MainWindow;

class GuiExport CompactToolBarContainer: public QWidget
{
    Q_OBJECT

public:
    enum class Zone
    {
        Left = 0,
        Center = 1,
        Right = 2,
    };

    explicit CompactToolBarContainer(MainWindow* mainWindow, QWidget* parent = nullptr);
    ~CompactToolBarContainer() override;

    void setActive(bool active);
    bool isActive() const;
    void installToolBarFilters();
    void removeToolBarFilters();
    void addToolBar(QToolBar* toolBar, Zone zone);
    void restoreToolBarsToMainWindow();
    void restoreStoredToolBars();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUi();
    void updateLabels();
    void setActiveZone(Zone zone);
    void clearActiveZone();
    Zone zoneAtGlobalPos(const QPoint& globalPos) const;
    QRect zoneRect(Zone zone) const;
    QBoxLayout* layoutForZone(Zone zone) const;
    QWidget* widgetForZone(Zone zone) const;
    void addToolBarToZoneLayout(QToolBar* toolBar, Zone zone);
    void insertToolBarToZoneLayout(QToolBar* toolBar, Zone zone, int index);
    int insertionIndexForZone(Zone zone, const QPoint& globalPos, QToolBar* movingToolBar) const;
    int toolBarCount(Zone zone) const;
    bool handleToolBarRelease(QToolBar* toolBar);
    bool handleContainedToolBarMouseMove(QToolBar* toolBar, QMouseEvent* event);
    bool handleContainedToolBarMouseRelease(QToolBar* toolBar);
    void updateDragPreview(QToolBar* toolBar, const QPoint& globalPos);
    void clearDragPreview();
    bool showContextMenu(const QPoint& globalPos);
    bool toolBarsLocked() const;
    void updateHandleVisibility();
    void restoreToolBarToMainWindow(QToolBar* toolBar);
    void detachToolBarsForShutdown();
    void persistToolBarZone(QToolBar* toolBar, Zone zone);
    void removeStoredToolBar(QToolBar* toolBar);
    QByteArray parameterKey(const QToolBar* toolBar) const;
    void installContainedToolBarFilter(QToolBar* toolBar);
    void removeContainedToolBarFilter(QToolBar* toolBar);
    void trackToolBarDestroyed(QToolBar* toolBar);
    QWidget* createHostedToolBarWrapper(QToolBar* toolBar);
    QWidget* hostedToolBarWidget(QToolBar* toolBar) const;
    QToolBar* watchedContainedToolBar(QObject* watched) const;
    void removeToolBarFromLayouts(QToolBar* toolBar);
    bool containsToolBar(const QToolBar* toolBar) const;

private:
    QPointer<MainWindow> mainWindow;
    QBoxLayout* rootLayout = nullptr;
    QBoxLayout* leftLayout = nullptr;
    QBoxLayout* centerLayout = nullptr;
    QBoxLayout* rightLayout = nullptr;
    QWidget* leftZoneWidget = nullptr;
    QWidget* centerZoneWidget = nullptr;
    QWidget* rightZoneWidget = nullptr;
    QWidget* emptyTestWidget = nullptr;
    QLabel* leftLabel = nullptr;
    QLabel* centerLabel = nullptr;
    QLabel* rightLabel = nullptr;
    QPointer<QWidget> dragPreview;
    QHash<QToolBar*, Zone> containedToolBars;
    QHash<QToolBar*, QWidget*> toolBarWrappers;
    QHash<QToolBar*, QPoint> dragStartPositions;
    QSet<QToolBar*> filteredToolBars;
    QSet<QToolBar*> destroyTrackedToolBars;
    Zone activeZone = Zone::Left;
    bool hasActiveZone = false;
    bool active = false;
};

}  // namespace Gui
