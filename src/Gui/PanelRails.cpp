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

#include "PreCompiled.h"

#include "PanelRails.h"

#include <QAction>
#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDockWidget>
#include <QDropEvent>
#include <QFrame>
#include <QLayoutItem>
#include <QMainWindow>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

#include <App/Application.h>
#include <Base/Parameter.h>

#include "BitmapFactory.h"
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "OverlayManager.h"

using namespace Gui;

namespace
{
constexpr auto PanelLeftRailObjectName = "_fc_panel_rail_left";
constexpr auto PanelRightRailObjectName = "_fc_panel_rail_right";
constexpr auto PanelRailDropIndicatorObjectName = "_fc_panel_rail_drop_indicator";
constexpr auto PanelRailDropInsertionIndicatorObjectName = "_fc_panel_rail_drop_insert_indicator";
constexpr auto PanelRailDragMimeType = "application/x-freecad-panel-rail";
constexpr auto PanelRailAssignmentProperty = "_fc_panel_rail_assignment";
constexpr auto PanelRailSlotProperty = "_fc_panel_rail_slot";
constexpr int PanelRailStripMargin = 3;
constexpr int PanelRailStripClearance = 2;
constexpr int PanelRailIconSize = 24;

class PanelRailDropFrame: public QFrame
{
public:
    explicit PanelRailDropFrame(bool insertion, QWidget* parent = nullptr)
        : QFrame(parent)
        , insertion(insertion)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFrameShape(QFrame::NoFrame);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        const QColor highlight = palette().color(QPalette::Highlight);
        QColor fill = highlight;
        fill.setAlpha(insertion ? 90 : 38);

        QColor border = highlight;
        border.setAlpha(insertion ? 230 : 180);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(border, insertion ? 2 : 1));
        painter.setBrush(fill);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);
    }

private:
    bool insertion = false;
};

bool isPanelRailDockCandidate(const QDockWidget* dock)
{
    if (!dock) {
        return false;
    }

    QAction* action = dock->toggleViewAction();
    return action && action->isVisible() && !dock->objectName().isEmpty();
}

int panelRailWidth()
{
    return PanelRailIconSize + 4 + (2 * PanelRailStripMargin) + PanelRailStripClearance;
}

QSize panelRailButtonSize()
{
    return QSize(PanelRailIconSize + 4, PanelRailIconSize + 4);
}

QToolBar* createButtonToolBar(QWidget* parent, Qt::Orientation orientation)
{
    auto toolbar = new QToolBar(parent);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setOrientation(orientation);
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(PanelRailIconSize, PanelRailIconSize));
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->layout()->setContentsMargins(0, 0, 0, 0);
    return toolbar;
}

void addToolBarStretch(QToolBar* toolbar)
{
    if (!toolbar) {
        return;
    }

    auto spacer = new QWidget(toolbar);
    if (toolbar->orientation() == Qt::Vertical) {
        spacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    }
    else {
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    }
    toolbar->addWidget(spacer);
}

void clearStrip(QWidget* content)
{
    if (!content || !content->layout()) {
        return;
    }

    QLayoutItem* item = nullptr;
    while ((item = content->layout()->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void addStripSpacer(QToolBar* toolbar)
{
    addToolBarStretch(toolbar);
}

QWidget* createStrip(MainWindow* window, QWidget** content, const QString& objectName)
{
    auto container = new QFrame(window);
    container->setObjectName(objectName + QStringLiteral("Content"));
    container->setFixedWidth(panelRailWidth());
    container->setFrameShape(QFrame::NoFrame);
    container->setAutoFillBackground(true);
    container->setAcceptDrops(true);

    auto layout = new QVBoxLayout(container);
    layout->setContentsMargins(
        PanelRailStripMargin,
        PanelRailStripMargin,
        PanelRailStripMargin,
        PanelRailStripMargin
    );
    layout->setSpacing(PanelRailStripMargin);

    container->hide();
    *content = container;
    return container;
}

}  // namespace

PanelRails::PanelRails(MainWindow* mainWindow)
    : QObject(mainWindow)
    , mainWindow(mainWindow)
{
    setup();
}

PanelRails::~PanelRails()
{
    hidePanelDropIndicator();
}

void PanelRails::setup()
{
    if (!mainWindow || leftStrip) {
        return;
    }

    connect(mainWindow, &MainWindow::workbenchActivated, this, [this]() {
        QTimer::singleShot(0, this, &PanelRails::refreshPanelStrips);
    });

    leftStrip = createStrip(mainWindow, &leftStripContent, QLatin1String(PanelLeftRailObjectName));
    rightStrip = createStrip(mainWindow, &rightStripContent, QLatin1String(PanelRightRailObjectName));
    leftStripContent->installEventFilter(this);
    rightStripContent->installEventFilter(this);

    panelDropIndicator = new PanelRailDropFrame(false, mainWindow);
    panelDropIndicator->setObjectName(QLatin1String(PanelRailDropIndicatorObjectName));
    panelDropIndicator->hide();

    panelDropInsertionIndicator = new PanelRailDropFrame(true, mainWindow);
    panelDropInsertionIndicator->setObjectName(
        QLatin1String(PanelRailDropInsertionIndicatorObjectName)
    );
    panelDropInsertionIndicator->hide();
}

void PanelRails::setActive(bool enabled)
{
    if (!leftStrip || !rightStrip) {
        return;
    }

    leftStrip->setVisible(enabled);
    rightStrip->setVisible(enabled);

    if (enabled && !active) {
        contentsMarginsBefore = mainWindow->contentsMargins();
        contentsMarginsSaved = true;
        const QList<QDockWidget*> docks = managedDockContainers();
        for (auto dock : docks) {
            const auto slot = panelSlotForDock(dock);
            movePanelDockToSlot(dock, slot);
        }
    }
    else if (!enabled && active) {
        hidePanelDropIndicator();
        if (contentsMarginsSaved) {
            mainWindow->setContentsMargins(contentsMarginsBefore);
            contentsMarginsSaved = false;
        }
    }

    active = enabled;

    layoutChrome();
    refreshPanelStrips();
}

bool PanelRails::isActive() const
{
    return active;
}

void PanelRails::layoutChrome()
{
    if (!active) {
        return;
    }

    applyContentsMargins();
    layoutWorkArea();
    layoutPanelStrips();
}

void PanelRails::applyContentsMargins()
{
    if (!active || !contentsMarginsSaved) {
        return;
    }

    mainWindow->setContentsMargins(
        contentsMarginsBefore.left() + panelRailWidth(),
        contentsMarginsBefore.top(),
        contentsMarginsBefore.right() + panelRailWidth(),
        contentsMarginsBefore.bottom()
    );
}

void PanelRails::layoutWorkArea()
{
    auto central = mainWindow->centralWidget();
    if (!active || !central) {
        return;
    }

    const int panelStripWidth = panelRailWidth();
    QRect geometry = central->geometry();
    geometry.setLeft(panelStripWidth);
    geometry.setRight(mainWindow->width() - panelStripWidth - 1);
    if (geometry.isValid()) {
        central->setGeometry(geometry);
    }
}

void PanelRails::layoutPanelStrips()
{
    if (!active || !leftStrip || !rightStrip) {
        return;
    }

    int top = 0;

    if (auto central = mainWindow->centralWidget()) {
        top = central->geometry().top();
    }
    else if (mainWindow->menuBar() && mainWindow->menuBar()->isVisible()) {
        top = mainWindow->menuBar()->geometry().bottom() + 1;
    }

    int bottom = mainWindow->height();
    if (mainWindow->statusBar() && mainWindow->statusBar()->isVisible()) {
        bottom = mainWindow->statusBar()->geometry().top();
    }

    const int panelStripWidth = panelRailWidth();
    const int stripHeight = std::max(0, bottom - top);
    leftStrip->setFixedWidth(panelStripWidth);
    rightStrip->setFixedWidth(panelStripWidth);
    leftStrip->setGeometry(0, top, panelStripWidth, stripHeight);
    rightStrip->setGeometry(mainWindow->width() - panelStripWidth, top, panelStripWidth, stripHeight);
    leftStrip->raise();
    rightStrip->raise();
}

void PanelRails::refreshPanelStrips()
{
    panelStripRefreshQueued = false;
    if (!leftStripContent || !rightStripContent) {
        return;
    }

    clearStrip(leftStripContent);
    clearStrip(rightStripContent);

    auto leftStripToolBar = createButtonToolBar(leftStripContent, Qt::Vertical);
    auto rightStripToolBar = createButtonToolBar(rightStripContent, Qt::Vertical);
    leftStripToolBar->setAcceptDrops(true);
    rightStripToolBar->setAcceptDrops(true);
    leftStripToolBar->installEventFilter(this);
    rightStripToolBar->installEventFilter(this);
    leftStripContent->layout()->addWidget(leftStripToolBar);
    rightStripContent->layout()->addWidget(rightStripToolBar);

    auto addButton = [this](QDockWidget* dock, QToolBar* toolbar, PanelSlot slot) {
        const QString title = dockTitle(dock);
        auto action = new QAction(dockIcon(dock, slot), title, toolbar);
        action->setToolTip(title);
        action->setStatusTip(title);
        action->setCheckable(true);
        action->setChecked(dock->isVisible());

        connect(action, &QAction::triggered, this, [this, dock, slot]() {
            activatePanelDock(dock, slot);
        });

        toolbar->addAction(action);
        if (auto button = qobject_cast<QToolButton*>(toolbar->widgetForAction(action))) {
            button->setAccessibleName(title);
            button->setProperty(PanelRailAssignmentProperty, panelAssignmentId(dock));
            button->setProperty(PanelRailSlotProperty, panelSlotName(slot));
            button->installEventFilter(this);
            button->setIconSize(toolbar->iconSize());
            button->setFixedSize(panelRailButtonSize());
        }
    };

    QList<PanelEntry> entries;
    const QList<QDockWidget*> docks = managedDockContainers();
    for (auto dock : docks) {
        connect(
            dock,
            &QDockWidget::visibilityChanged,
            this,
            &PanelRails::schedulePanelStripRefresh,
            Qt::UniqueConnection
        );
        connect(
            dock,
            &QDockWidget::dockLocationChanged,
            this,
            &PanelRails::schedulePanelStripRefresh,
            Qt::UniqueConnection
        );
        const auto slot = panelSlotForDock(dock);
        entries.push_back({dock, slot, panelOrderForDock(dock, slot)});
    }

    std::sort(entries.begin(), entries.end(), [this](const PanelEntry& left, const PanelEntry& right) {
        if (left.slot != right.slot) {
            return static_cast<int>(left.slot) < static_cast<int>(right.slot);
        }
        if (left.order != right.order) {
            return left.order < right.order;
        }

        return dockTitle(left.dock).localeAwareCompare(dockTitle(right.dock)) < 0;
    });

    bool addedLeftTop = false;
    bool addedLeftSeparator = false;
    bool addedRightTop = false;
    bool addedRightSeparator = false;
    bool addedLeftBottomSpacer = false;
    bool addedRightBottomSpacer = false;
    for (const auto& entry : entries) {
        switch (entry.slot) {
            case PanelSlot::LeftTop:
                addButton(entry.dock, leftStripToolBar, entry.slot);
                addedLeftTop = true;
                break;
            case PanelSlot::LeftLower:
                if (!addedLeftSeparator) {
                    if (addedLeftTop) {
                        leftStripToolBar->addSeparator();
                    }
                    addedLeftSeparator = true;
                }
                addButton(entry.dock, leftStripToolBar, entry.slot);
                break;
            case PanelSlot::RightTop:
                addButton(entry.dock, rightStripToolBar, entry.slot);
                addedRightTop = true;
                break;
            case PanelSlot::RightLower:
                if (!addedRightSeparator) {
                    if (addedRightTop) {
                        rightStripToolBar->addSeparator();
                    }
                    addedRightSeparator = true;
                }
                addButton(entry.dock, rightStripToolBar, entry.slot);
                break;
            case PanelSlot::BottomLeft:
                if (!addedLeftBottomSpacer) {
                    addStripSpacer(leftStripToolBar);
                    addedLeftBottomSpacer = true;
                }
                addButton(entry.dock, leftStripToolBar, entry.slot);
                break;
            case PanelSlot::BottomRight:
                if (!addedRightBottomSpacer) {
                    addStripSpacer(rightStripToolBar);
                    addedRightBottomSpacer = true;
                }
                addButton(entry.dock, rightStripToolBar, entry.slot);
                break;
        }
    }

    if (!addedLeftBottomSpacer) {
        addStripSpacer(leftStripToolBar);
    }
    if (!addedRightBottomSpacer) {
        addStripSpacer(rightStripToolBar);
    }

    layoutPanelStrips();
}

void PanelRails::schedulePanelStripRefresh()
{
    if (panelStripRefreshQueued) {
        return;
    }

    panelStripRefreshQueued = true;
    QTimer::singleShot(0, this, &PanelRails::refreshPanelStrips);
}

bool PanelRails::eventFilter(QObject* watched, QEvent* event)
{
    if (!mainWindow || !active) {
        return QObject::eventFilter(watched, event);
    }

    auto watchedWidget = watched->isWidgetType() ? qobject_cast<QWidget*>(watched) : nullptr;
    const bool panelDropTarget = watchedWidget
        && ((leftStripContent
             && (watchedWidget == leftStripContent || leftStripContent->isAncestorOf(watchedWidget)))
            || (rightStripContent
                && (watchedWidget == rightStripContent
                    || rightStripContent->isAncestorOf(watchedWidget))));
    if (panelDropTarget && (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove)) {
        auto dragMoveEvent = static_cast<QDragMoveEvent*>(event);
        if (dragMoveEvent->mimeData()->hasFormat(PanelRailDragMimeType)) {
            updatePanelDropIndicator(watchedWidget, dragMoveEvent->position().toPoint());
            dragMoveEvent->acceptProposedAction();
            return true;
        }
    }
    if (panelDropTarget && event->type() == QEvent::Drop) {
        auto dropEvent = static_cast<QDropEvent*>(event);
        const QString assignmentId = QString::fromUtf8(
            dropEvent->mimeData()->data(PanelRailDragMimeType)
        );
        if (handlePanelDrop(watchedWidget, dropEvent->position().toPoint(), assignmentId)) {
            hidePanelDropIndicator();
            dropEvent->acceptProposedAction();
            return true;
        }
    }
    if (panelDropTarget && event->type() == QEvent::DragLeave) {
        hidePanelDropIndicator();
        return true;
    }

    if (auto button = qobject_cast<QToolButton*>(watched);
        button && button->property(PanelRailAssignmentProperty).isValid()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                panelDragStartPositions[button] = mouseEvent->position().toPoint();
            }
        }
        else if (event->type() == QEvent::MouseMove) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons() & Qt::LeftButton) {
                const auto start
                    = panelDragStartPositions.value(button, mouseEvent->position().toPoint());
                if ((mouseEvent->position().toPoint() - start).manhattanLength()
                    >= QApplication::startDragDistance()) {
                    panelDragStartPositions.remove(button);
                    startPanelButtonDrag(button);
                    return true;
                }
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::Leave) {
            panelDragStartPositions.remove(button);
        }
    }

    return QObject::eventFilter(watched, event);
}

QList<QDockWidget*> PanelRails::managedDockContainers() const
{
    QList<QDockWidget*> docks;
    const QList<QWidget*> widgets = DockWindowManager::instance()->getDockWindows();
    for (auto widget : widgets) {
        auto parent = widget ? widget->parentWidget() : nullptr;
        while (parent) {
            if (auto dock = qobject_cast<QDockWidget*>(parent)) {
                if (isPanelRailDockCandidate(dock) && !docks.contains(dock)) {
                    docks.push_back(dock);
                }
                break;
            }
            parent = parent->parentWidget();
        }
    }

    return docks;
}

QString PanelRails::dockActionId(const QDockWidget* dock) const
{
    if (!dock || !dock->toggleViewAction()) {
        return {};
    }

    return dock->toggleViewAction()->data().toString();
}

QString PanelRails::panelAssignmentId(const QDockWidget* dock) const
{
    QString id = dockActionId(dock);
    if (id.isEmpty() && dock) {
        id = dock->objectName();
    }

    return id;
}

const PanelRails::KnownPanel* PanelRails::knownPanelForActionId(const QString& actionId) const
{
    static constexpr KnownPanel panels[] = {
        {"Std_TreeView", PanelSlot::LeftTop, 10, "tree-doc-single", QStyle::SP_DirIcon},
        {"Std_ComboView", PanelSlot::LeftTop, 20, "tree-doc-multi", QStyle::SP_FileDialogDetailedView},
        {"Std_SelectionView", PanelSlot::LeftLower, 10, "view-select", QStyle::SP_FileDialogContentsView},
        {"Std_PropertyView", PanelSlot::LeftLower, 20, "document-properties", QStyle::SP_FileDialogListView},
        {"Std_TaskView",
         PanelSlot::RightTop,
         10,
         "qss:overlay/icons/taskshow.svg",
         QStyle::SP_FileDialogDetailedView},
        {"Std_TaskWatcher",
         PanelSlot::RightLower,
         10,
         "qss:overlay/icons/taskshow.svg",
         QStyle::SP_MessageBoxWarning},
        {"Std_DAGView", PanelSlot::RightLower, 20, "dagViewVisible", QStyle::SP_ComputerIcon},
        {"Std_PythonView", PanelSlot::BottomLeft, 10, "applications-python", QStyle::SP_ComputerIcon},
        {"Std_ReportView", PanelSlot::BottomRight, 10, "MacroEditor", QStyle::SP_MessageBoxInformation},
    };

    for (const auto& panel : panels) {
        if (actionId == QLatin1String(panel.actionId)) {
            return &panel;
        }
    }

    return nullptr;
}

QString PanelRails::dockTitle(const QDockWidget* dock) const
{
    QString title = dock->windowTitle();
    if (title.isEmpty()) {
        title = dock->objectName();
    }

    title.remove(QLatin1Char('&'));
    return title;
}

PanelRails::PanelGroup PanelRails::panelGroup(PanelSlot slot) const
{
    switch (slot) {
        case PanelSlot::LeftTop:
            return PanelGroup::LeftTop;
        case PanelSlot::LeftLower:
            return PanelGroup::LeftLower;
        case PanelSlot::RightTop:
            return PanelGroup::RightTop;
        case PanelSlot::RightLower:
            return PanelGroup::RightLower;
        case PanelSlot::BottomLeft:
            return PanelGroup::BottomLeft;
        case PanelSlot::BottomRight:
            return PanelGroup::BottomRight;
    }

    return PanelGroup::LeftLower;
}

QString PanelRails::panelSlotName(PanelSlot slot) const
{
    switch (slot) {
        case PanelSlot::LeftTop:
            return QStringLiteral("left-upper");
        case PanelSlot::LeftLower:
            return QStringLiteral("left-lower");
        case PanelSlot::RightTop:
            return QStringLiteral("right-upper");
        case PanelSlot::RightLower:
            return QStringLiteral("right-lower");
        case PanelSlot::BottomLeft:
            return QStringLiteral("bottom-left");
        case PanelSlot::BottomRight:
            return QStringLiteral("bottom-right");
    }

    return QStringLiteral("left-lower");
}

bool PanelRails::panelSlotFromName(const QString& name, PanelSlot* slot) const
{
    if (!slot) {
        return false;
    }

    static const std::pair<QLatin1String, PanelSlot> slots[] = {
        {QLatin1String("left-upper"), PanelSlot::LeftTop},
        {QLatin1String("left-lower"), PanelSlot::LeftLower},
        {QLatin1String("right-upper"), PanelSlot::RightTop},
        {QLatin1String("right-lower"), PanelSlot::RightLower},
        {QLatin1String("bottom-left"), PanelSlot::BottomLeft},
        {QLatin1String("bottom-right"), PanelSlot::BottomRight},
    };

    for (const auto& [slotName, value] : slots) {
        if (name == slotName) {
            *slot = value;
            return true;
        }
    }

    return false;
}

QWidget* PanelRails::panelDropStripForTarget(QWidget* target) const
{
    QWidget* strip = target;
    while (strip && strip != leftStripContent && strip != rightStripContent) {
        strip = strip->parentWidget();
    }

    return strip;
}

QVector<QRect> PanelRails::panelButtonGeometries(QWidget* strip, PanelSlot slot) const
{
    QVector<QRect> buttonRects;
    if (!strip) {
        return buttonRects;
    }

    const QString slotName = panelSlotName(slot);
    const auto buttons = strip->findChildren<QToolButton*>();
    for (auto button : buttons) {
        if (button->property(PanelRailSlotProperty).toString() != slotName) {
            continue;
        }
        buttonRects.push_back(QRect(button->mapTo(strip, QPoint(0, 0)), button->size()));
    }

    std::sort(buttonRects.begin(), buttonRects.end(), [](const QRect& left, const QRect& right) {
        return left.top() < right.top();
    });
    return buttonRects;
}

PanelRails::PanelSlot PanelRails::fallbackSlotForDock(QDockWidget* dock) const
{
    switch (mainWindow->dockWidgetArea(dock)) {
        case Qt::RightDockWidgetArea:
            return PanelSlot::RightTop;
        case Qt::BottomDockWidgetArea:
            return PanelSlot::BottomLeft;
        case Qt::TopDockWidgetArea:
        case Qt::LeftDockWidgetArea:
        default:
            return PanelSlot::LeftLower;
    }
}

PanelRails::PanelSlot PanelRails::panelSlotForDock(QDockWidget* dock) const
{
    const QString assignmentId = panelAssignmentId(dock);
    if (!assignmentId.isEmpty()) {
        auto group = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/MainWindow/PanelRailsSlots"
        );
        PanelSlot configuredSlot = PanelSlot::LeftLower;
        const QString configuredSlotName = QString::fromUtf8(
            group->GetASCII(assignmentId.toUtf8().constData(), "").c_str()
        );
        if (panelSlotFromName(configuredSlotName, &configuredSlot)) {
            return configuredSlot;
        }
    }

    if (const auto* knownPanel = knownPanelForActionId(dockActionId(dock))) {
        return knownPanel->slot;
    }

    return fallbackSlotForDock(dock);
}

void PanelRails::setPanelSlotForDock(QDockWidget* dock, PanelSlot slot)
{
    const QString assignmentId = panelAssignmentId(dock);
    if (assignmentId.isEmpty()) {
        return;
    }

    auto group = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow/PanelRailsSlots"
    );
    group->SetASCII(assignmentId.toUtf8().constData(), panelSlotName(slot).toUtf8().constData());
}

void PanelRails::movePanelDockToSlot(QDockWidget* dock, PanelSlot slot)
{
    if (!dock) {
        return;
    }

    const bool wasVisible = dock->isVisible();
    const auto dockArea = dockAreaForSlot(slot);
    if (DockWindowManager::instance()->isOverlayActivated()) {
        OverlayManager::instance()->moveDockWidgetToOverlay(dock, dockArea);
    }
    else {
        mainWindow->addDockWidget(dockArea, dock);
    }
    dock->setVisible(wasVisible);
    layoutChrome();
}

PanelRails::PanelSlot PanelRails::dropPanelSlotForPosition(QWidget* target, const QPoint& position) const
{
    QWidget* strip = panelDropStripForTarget(target);
    const bool leftSide = strip == leftStripContent;
    const QPoint globalPosition = target ? target->mapToGlobal(position) : position;
    const QPoint stripPosition = strip ? strip->mapFromGlobal(globalPosition) : position;
    const int height = std::max(1, strip ? strip->height() : target->height());
    const int buttonHeight = panelRailButtonSize().height();
    const int margin = PanelRailStripMargin;

    const PanelSlot topSlot = leftSide ? PanelSlot::LeftTop : PanelSlot::RightTop;
    const PanelSlot lowerSlot = leftSide ? PanelSlot::LeftLower : PanelSlot::RightLower;
    const PanelSlot bottomSlot = leftSide ? PanelSlot::BottomLeft : PanelSlot::BottomRight;

    const auto topButtons = panelButtonGeometries(strip, topSlot);
    const auto lowerButtons = panelButtonGeometries(strip, lowerSlot);
    const auto bottomButtons = panelButtonGeometries(strip, bottomSlot);

    int bottomBoundary = height - std::max(buttonHeight * 2, height / 4);
    if (!bottomButtons.isEmpty()) {
        if (!lowerButtons.isEmpty()) {
            bottomBoundary = (lowerButtons.constLast().bottom() + bottomButtons.constFirst().top())
                / 2;
        }
        else {
            bottomBoundary = bottomButtons.constFirst().top() - margin - (buttonHeight / 2);
        }
    }
    bottomBoundary = std::clamp(bottomBoundary, margin, height - margin);
    if (stripPosition.y() >= bottomBoundary) {
        return bottomSlot;
    }

    int lowerBoundary = margin + buttonHeight;
    if (!topButtons.isEmpty() && !lowerButtons.isEmpty()) {
        lowerBoundary = (topButtons.constLast().bottom() + lowerButtons.constFirst().top()) / 2;
    }
    else if (!topButtons.isEmpty()) {
        lowerBoundary = topButtons.constLast().bottom() + margin + (buttonHeight / 2);
    }
    else if (!lowerButtons.isEmpty()) {
        lowerBoundary = lowerButtons.constFirst().top() - margin - (buttonHeight / 2);
    }
    lowerBoundary = std::clamp(lowerBoundary, margin, std::max(margin, bottomBoundary - 1));

    return stripPosition.y() <= lowerBoundary ? topSlot : lowerSlot;
}

QRect PanelRails::panelDropZoneGeometry(QWidget* strip, PanelSlot slot) const
{
    if (!strip) {
        return {};
    }

    const QRect bounds = strip->rect().adjusted(2, 2, -2, -2);
    const int zoneHeight = std::max(1, bounds.height() / 3);
    int top = bounds.top();
    switch (slot) {
        case PanelSlot::LeftLower:
        case PanelSlot::RightLower:
            top += zoneHeight;
            break;
        case PanelSlot::BottomLeft:
        case PanelSlot::BottomRight:
            top += zoneHeight * 2;
            break;
        case PanelSlot::LeftTop:
        case PanelSlot::RightTop:
            break;
    }

    const int bottom = slot == PanelSlot::BottomLeft || slot == PanelSlot::BottomRight
        ? bounds.bottom()
        : top + zoneHeight - 1;
    return QRect(bounds.left(), top, bounds.width(), std::max(1, bottom - top + 1));
}

QRect PanelRails::panelDropInsertionGeometry(QWidget* strip, PanelSlot slot) const
{
    if (!strip) {
        return {};
    }

    const QSize size = panelRailButtonSize();
    const QVector<QRect> slotButtonRects = panelButtonGeometries(strip, slot);

    const QRect bounds = strip->rect().adjusted(
        PanelRailStripMargin,
        PanelRailStripMargin,
        -PanelRailStripMargin,
        -PanelRailStripMargin
    );
    const int x = bounds.left() + std::max(0, (bounds.width() - size.width()) / 2);
    const bool bottomSlot = slot == PanelSlot::BottomLeft || slot == PanelSlot::BottomRight;
    int y = panelDropZoneGeometry(strip, slot).top() + PanelRailStripMargin;

    if (!slotButtonRects.isEmpty()) {
        if (bottomSlot) {
            y = slotButtonRects.constFirst().top() - PanelRailStripMargin - size.height();
        }
        else {
            y = slotButtonRects.constLast().bottom() + 1 + PanelRailStripMargin;
        }
    }
    else if (bottomSlot) {
        y = panelDropZoneGeometry(strip, slot).bottom() - PanelRailStripMargin - size.height() + 1;
    }

    y = std::clamp(y, bounds.top(), std::max(bounds.top(), bounds.bottom() - size.height() + 1));
    return QRect(QPoint(x, y), size);
}

QRect PanelRails::panelDropGroupGeometry(QWidget* strip, PanelSlot slot, const QRect& insertion) const
{
    if (!strip) {
        return {};
    }

    QRect group = insertion;
    const QVector<QRect> slotButtonRects = panelButtonGeometries(strip, slot);
    for (const QRect& buttonRect : slotButtonRects) {
        group = group.united(buttonRect);
    }

    return group.adjusted(-2, -2, 2, 2).intersected(strip->rect().adjusted(1, 1, -1, -1));
}

void PanelRails::updatePanelDropIndicator(QWidget* target, const QPoint& position)
{
    if (!panelDropIndicator || !panelDropInsertionIndicator) {
        return;
    }

    QWidget* strip = panelDropStripForTarget(target);
    if (!strip) {
        hidePanelDropIndicator();
        return;
    }

    const PanelSlot slot = dropPanelSlotForPosition(target, position);
    const QRect insertion = panelDropInsertionGeometry(strip, slot);
    const QRect group = panelDropGroupGeometry(strip, slot, insertion);

    panelDropIndicator->setGeometry(QRect(strip->mapTo(mainWindow, group.topLeft()), group.size()));
    panelDropIndicator->raise();
    panelDropIndicator->show();
    panelDropInsertionIndicator->setGeometry(
        QRect(strip->mapTo(mainWindow, insertion.topLeft()), insertion.size())
    );
    panelDropInsertionIndicator->raise();
    panelDropInsertionIndicator->show();
}

void PanelRails::hidePanelDropIndicator()
{
    if (panelDropIndicator) {
        panelDropIndicator->hide();
    }
    if (panelDropInsertionIndicator) {
        panelDropInsertionIndicator->hide();
    }
}

void PanelRails::startPanelButtonDrag(QToolButton* button)
{
    if (!button) {
        return;
    }

    const QString assignmentId = button->property(PanelRailAssignmentProperty).toString();
    if (assignmentId.isEmpty()) {
        return;
    }

    auto mimeData = new QMimeData();
    mimeData->setData(PanelRailDragMimeType, assignmentId.toUtf8());
    mimeData->setText(button->toolTip());

    auto drag = new QDrag(button);
    drag->setMimeData(mimeData);
    QPixmap pixmap = button->icon().pixmap(button->iconSize());
    if (pixmap.isNull()) {
        pixmap = button->grab();
    }
    if (!pixmap.isNull()) {
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
    }
    drag->exec(Qt::MoveAction);
    hidePanelDropIndicator();
}

bool PanelRails::handlePanelDrop(QWidget* target, const QPoint& position, const QString& assignmentId)
{
    if (assignmentId.isEmpty()) {
        return false;
    }

    for (auto dock : managedDockContainers()) {
        if (panelAssignmentId(dock) != assignmentId) {
            continue;
        }

        const PanelSlot slot = dropPanelSlotForPosition(target, position);
        setPanelSlotForDock(dock, slot);
        const bool wasVisible = dock->isVisible();
        movePanelDockToSlot(dock, slot);
        if (wasVisible) {
            hideOtherPanelsInSlot(dock, slot);
            dock->raise();
        }
        refreshPanelStrips();
        return true;
    }

    return false;
}

void PanelRails::activatePanelDock(QDockWidget* dock, PanelSlot slot)
{
    if (!dock || !dock->toggleViewAction()) {
        return;
    }

    if (dock->isVisible()) {
        dock->toggleViewAction()->activate(QAction::Trigger);
        refreshPanelStrips();
        return;
    }

    hideOtherPanelsInSlot(dock, slot);
    if (!dock->isVisible()) {
        dock->toggleViewAction()->activate(QAction::Trigger);
    }
    dock->raise();
    refreshPanelStrips();
}

void PanelRails::hideOtherPanelsInSlot(QDockWidget* dock, PanelSlot slot)
{
    const auto group = panelGroup(slot);
    const QList<QDockWidget*> docks = managedDockContainers();
    for (auto other : docks) {
        if (other == dock || !other->isVisible()) {
            continue;
        }

        if (panelGroup(panelSlotForDock(other)) == group && other->toggleViewAction()) {
            other->toggleViewAction()->activate(QAction::Trigger);
        }
    }
}

Qt::DockWidgetArea PanelRails::dockAreaForSlot(PanelSlot slot) const
{
    switch (slot) {
        case PanelSlot::RightTop:
        case PanelSlot::RightLower:
            return Qt::RightDockWidgetArea;
        case PanelSlot::BottomLeft:
        case PanelSlot::BottomRight:
            return Qt::BottomDockWidgetArea;
        case PanelSlot::LeftTop:
        case PanelSlot::LeftLower:
        default:
            return Qt::LeftDockWidgetArea;
    }
}

int PanelRails::panelOrderForDock(const QDockWidget* dock, PanelSlot slot) const
{
    if (const auto* knownPanel = knownPanelForActionId(dockActionId(dock))) {
        return knownPanel->order;
    }

    const int fallbackBase = 1000;
    return fallbackBase + static_cast<int>(slot);
}

QIcon PanelRails::dockIcon(const QDockWidget* dock, PanelSlot slot) const
{
    QIcon icon;

    if (const auto* knownPanel = knownPanelForActionId(dockActionId(dock))) {
        icon = BitmapFactory().iconFromTheme(knownPanel->iconName);
        if (icon.isNull()) {
            icon = BitmapFactory().pixmap(knownPanel->iconName);
        }
        if (icon.isNull()) {
            icon = QApplication::style()->standardIcon(
                static_cast<QStyle::StandardPixmap>(knownPanel->fallbackIcon)
            );
        }
    }

    if (icon.isNull() && dock->widget()) {
        icon = dock->widget()->windowIcon();
    }

    if (icon.isNull()) {
        icon = dock->windowIcon();
    }

    if (icon.isNull()) {
        const auto group = panelGroup(slot);
        const auto fallback = group == PanelGroup::RightTop || group == PanelGroup::RightLower
            ? QStyle::SP_FileDialogDetailedView
            : QStyle::SP_FileDialogListView;
        icon = QApplication::style()->standardIcon(fallback);
    }

    return icon;
}

#include "moc_PanelRails.cpp"
