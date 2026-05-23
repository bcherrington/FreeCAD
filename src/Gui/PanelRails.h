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
#include <QIcon>
#include <QList>
#include <QMargins>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QVector>
#include <QWidget>

#include <FCGlobal.h>

class QDockWidget;
class QToolButton;

namespace Gui
{

class MainWindow;

class GuiExport PanelRails: public QObject
{
    Q_OBJECT

public:
    explicit PanelRails(MainWindow* mainWindow);
    ~PanelRails() override;

    void setActive(bool active);
    bool isActive() const;

    void layoutChrome();
    void refreshPanelStrips();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    enum class PanelSlot
    {
        LeftTop,
        LeftLower,
        RightTop,
        RightLower,
        BottomLeft,
        BottomRight,
    };

    enum class PanelGroup
    {
        LeftTop,
        LeftLower,
        RightTop,
        RightLower,
        BottomLeft,
        BottomRight,
    };

    struct PanelEntry
    {
        QDockWidget* dock = nullptr;
        PanelSlot slot = PanelSlot::LeftLower;
        int order = 0;
    };

    struct KnownPanel
    {
        const char* actionId = nullptr;
        PanelSlot slot = PanelSlot::LeftLower;
        int order = 0;
        const char* iconName = nullptr;
        int fallbackIcon = 0;
    };

    void setup();
    void applyContentsMargins();
    void layoutWorkArea();
    void layoutPanelStrips();

    QList<QDockWidget*> managedDockContainers() const;
    PanelSlot panelSlotForDock(QDockWidget* dock) const;
    PanelSlot fallbackSlotForDock(QDockWidget* dock) const;
    Qt::DockWidgetArea dockAreaForSlot(PanelSlot slot) const;
    int panelOrderForDock(const QDockWidget* dock, PanelSlot slot) const;
    QIcon dockIcon(const QDockWidget* dock, PanelSlot slot) const;
    QString dockTitle(const QDockWidget* dock) const;
    QString dockActionId(const QDockWidget* dock) const;
    QString panelAssignmentId(const QDockWidget* dock) const;
    const KnownPanel* knownPanelForActionId(const QString& actionId) const;
    PanelGroup panelGroup(PanelSlot slot) const;
    QString panelSlotName(PanelSlot slot) const;
    bool panelSlotFromName(const QString& name, PanelSlot* slot) const;
    QWidget* panelDropStripForTarget(QWidget* target) const;
    QVector<QRect> panelButtonGeometries(QWidget* strip, PanelSlot slot) const;
    PanelSlot dropPanelSlotForPosition(QWidget* target, const QPoint& position) const;
    QRect panelDropZoneGeometry(QWidget* strip, PanelSlot slot) const;
    QRect panelDropInsertionGeometry(QWidget* strip, PanelSlot slot) const;
    QRect panelDropGroupGeometry(QWidget* strip, PanelSlot slot, const QRect& insertion) const;
    void updatePanelDropIndicator(QWidget* target, const QPoint& position);
    void hidePanelDropIndicator();
    void setPanelSlotForDock(QDockWidget* dock, PanelSlot slot);
    void movePanelDockToSlot(QDockWidget* dock, PanelSlot slot);
    void startPanelButtonDrag(QToolButton* button);
    bool handlePanelDrop(QWidget* target, const QPoint& position, const QString& assignmentId);
    void activatePanelDock(QDockWidget* dock, PanelSlot slot);
    void hideOtherPanelsInSlot(QDockWidget* dock, PanelSlot slot);
    void schedulePanelStripRefresh();

private:
    MainWindow* mainWindow = nullptr;
    QWidget* leftStrip = nullptr;
    QWidget* rightStrip = nullptr;
    QWidget* leftStripContent = nullptr;
    QWidget* rightStripContent = nullptr;
    QWidget* panelDropIndicator = nullptr;
    QWidget* panelDropInsertionIndicator = nullptr;
    QHash<QObject*, QPoint> panelDragStartPositions;
    bool panelStripRefreshQueued = false;
    bool active = false;
    bool contentsMarginsSaved = false;
    QMargins contentsMarginsBefore;
};

}  // namespace Gui
