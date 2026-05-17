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

#include "CompactMainWindowChrome.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFrame>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>

#include <algorithm>

#include <App/Application.h>
#include <Base/Parameter.h>

#include "BitmapFactory.h"
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "OverlayManager.h"

using namespace Gui;

namespace
{
constexpr auto CompactTopBarObjectName = "_fc_compact_top_bar";
constexpr auto CompactToolBarObjectName = "_fc_compact_tool_bar";
constexpr auto CompactMenuBarObjectName = "_fc_compact_menu_bar";
constexpr auto CompactLeftStripObjectName = "_fc_compact_left_panel_rail";
constexpr auto CompactRightStripObjectName = "_fc_compact_right_panel_rail";
constexpr auto CompactLegacyLeftStripObjectName = "_fc_compact_left_panel_strip";
constexpr auto CompactLegacyRightStripObjectName = "_fc_compact_right_panel_strip";
constexpr auto CompactLegacyBottomStripObjectName = "_fc_compact_bottom_panel_strip";
constexpr int CompactPanelStripWidth = 34;
constexpr int CompactResizeBorderWidth = 6;

QString trText(const char* text)
{
    return QCoreApplication::translate("Gui::CompactMainWindowChrome", text);
}

QPoint globalPosition(const QMouseEvent* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->globalPos();
#else
    return event->globalPosition().toPoint();
#endif
}

QIcon hamburgerIcon(const QWidget* widget)
{
    const QPalette palette = widget ? widget->palette() : qApp->palette();
    const QColor background = palette.color(QPalette::Button);
    QColor foreground = palette.color(QPalette::ButtonText);

    if (std::abs(foreground.lightness() - background.lightness()) < 80) {
        foreground = background.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black);
    }

    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(foreground, 1.8, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(3, 5), QPointF(13, 5));
    painter.drawLine(QPointF(3, 8), QPointF(13, 8));
    painter.drawLine(QPointF(3, 11), QPointF(13, 11));
    return QIcon(pixmap);
}

bool isCompactUiDock(const QDockWidget* dock)
{
    if (!dock) {
        return false;
    }

    const QString name = dock->objectName();
    return name == QLatin1String(CompactLegacyLeftStripObjectName)
        || name == QLatin1String(CompactLegacyRightStripObjectName)
        || name == QLatin1String(CompactLegacyBottomStripObjectName);
}

bool isCompactUiDockCandidate(const QDockWidget* dock)
{
    if (!dock || isCompactUiDock(dock)) {
        return false;
    }

    QAction* action = dock->toggleViewAction();
    return action && action->isVisible() && !dock->objectName().isEmpty();
}

QFrame* createStripSeparator(QWidget* parent)
{
    auto separator = new QFrame(parent);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setFixedHeight(8);
    return separator;
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

void addStripSpacer(QWidget* content, Qt::Orientation orientation)
{
    if (!content || !content->layout()) {
        return;
    }

    if (orientation == Qt::Vertical) {
        content->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }
    else {
        content->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    }
}

QWidget* createStrip(MainWindow* window, QWidget** content, const QString& objectName)
{
    auto container = new QFrame(window);
    container->setObjectName(objectName + QStringLiteral("Content"));
    container->setFixedWidth(CompactPanelStripWidth);
    container->setFrameShape(QFrame::NoFrame);
    container->setAutoFillBackground(true);

    auto layout = new QVBoxLayout(container);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(3);

    container->hide();
    *content = container;
    return container;
}

QCursor cursorForEdges(Qt::Edges edges)
{
    if (edges == (Qt::LeftEdge | Qt::TopEdge) || edges == (Qt::RightEdge | Qt::BottomEdge)) {
        return Qt::SizeFDiagCursor;
    }
    if (edges == (Qt::RightEdge | Qt::TopEdge) || edges == (Qt::LeftEdge | Qt::BottomEdge)) {
        return Qt::SizeBDiagCursor;
    }
    if (edges & (Qt::LeftEdge | Qt::RightEdge)) {
        return Qt::SizeHorCursor;
    }
    return Qt::SizeVerCursor;
}

QRect resizedGeometry(
    const QRect& startGeometry,
    const QPoint& delta,
    Qt::Edges edges,
    const QSize& minimumSize,
    const QSize& maximumSize
)
{
    QRect geometry = startGeometry;
    if (edges & Qt::LeftEdge) {
        geometry.setLeft(geometry.left() + delta.x());
    }
    if (edges & Qt::RightEdge) {
        geometry.setRight(geometry.right() + delta.x());
    }
    if (edges & Qt::TopEdge) {
        geometry.setTop(geometry.top() + delta.y());
    }
    if (edges & Qt::BottomEdge) {
        geometry.setBottom(geometry.bottom() + delta.y());
    }

    const int minWidth = std::max(1, minimumSize.width());
    const int minHeight = std::max(1, minimumSize.height());
    const int maxWidth = maximumSize.width() > 0 ? maximumSize.width() : QWIDGETSIZE_MAX;
    const int maxHeight = maximumSize.height() > 0 ? maximumSize.height() : QWIDGETSIZE_MAX;

    if (geometry.width() < minWidth) {
        if (edges & Qt::LeftEdge) {
            geometry.setLeft(geometry.right() - minWidth + 1);
        }
        else {
            geometry.setRight(geometry.left() + minWidth - 1);
        }
    }
    if (geometry.height() < minHeight) {
        if (edges & Qt::TopEdge) {
            geometry.setTop(geometry.bottom() - minHeight + 1);
        }
        else {
            geometry.setBottom(geometry.top() + minHeight - 1);
        }
    }
    if (geometry.width() > maxWidth) {
        if (edges & Qt::LeftEdge) {
            geometry.setLeft(geometry.right() - maxWidth + 1);
        }
        else {
            geometry.setRight(geometry.left() + maxWidth - 1);
        }
    }
    if (geometry.height() > maxHeight) {
        if (edges & Qt::TopEdge) {
            geometry.setTop(geometry.bottom() - maxHeight + 1);
        }
        else {
            geometry.setBottom(geometry.top() + maxHeight - 1);
        }
    }

    return geometry;
}
}  // namespace

CompactMainWindowChrome::CompactMainWindowChrome(MainWindow* mainWindow)
    : QObject(mainWindow)
    , mainWindow(mainWindow)
{
    framelessWindow = mainWindow && (mainWindow->windowFlags() & Qt::FramelessWindowHint);
    setup();
}

CompactMainWindowChrome::~CompactMainWindowChrome()
{
    setGlobalEventFilterActive(false);
}

bool CompactMainWindowChrome::shouldUseFramelessWindow()
{
    auto group = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow"
    );
    return group->GetBool("CompactJetBrainsLayout", false)
        && group->GetBool("CompactJetBrainsFramelessWindow", false);
}

void CompactMainWindowChrome::setup()
{
    if (!mainWindow || topBar) {
        return;
    }

    topBar = new QFrame(mainWindow);
    topBar->setObjectName(QLatin1String(CompactTopBarObjectName));
    topBar->setAutoFillBackground(true);
    topBar->installEventFilter(this);
    topBar->hide();

    auto topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(4, 2, 4, 2);
    topBarLayout->setSpacing(4);

    appIconButton = createTitleButton(trText("Window menu"));
    QIcon appIcon = mainWindow->windowIcon().isNull() ? QApplication::windowIcon()
                                                      : mainWindow->windowIcon();
    if (appIcon.isNull()) {
        appIcon = mainWindow->style()->standardIcon(QStyle::SP_TitleBarMenuButton);
    }
    appIconButton->setIcon(appIcon);
    auto windowMenu = new QMenu(appIconButton);
    auto restoreAction = windowMenu->addAction(trText("Restore"), mainWindow, [this]() {
        mainWindow->showNormal();
    });
    windowMenu->addAction(trText("Minimize"), mainWindow, &MainWindow::showMinimized);
    auto maximizeAction
        = windowMenu->addAction(trText("Maximize"), mainWindow, &MainWindow::showMaximized);
    windowMenu->addSeparator();
    windowMenu->addAction(trText("Close"), mainWindow, &MainWindow::close);
    connect(windowMenu, &QMenu::aboutToShow, this, [this, restoreAction, maximizeAction]() {
        restoreAction->setEnabled(mainWindow->isMinimized() || mainWindow->isMaximized());
        maximizeAction->setEnabled(!mainWindow->isMaximized());
    });
    appIconButton->setMenu(windowMenu);
    appIconButton->setPopupMode(QToolButton::InstantPopup);

    switchArea = new QWidget(topBar);
    switchArea->installEventFilter(this);
    auto switchLayout = new QVBoxLayout(switchArea);
    switchLayout->setContentsMargins(0, 0, 0, 0);
    switchLayout->setSpacing(0);

    toolBar = new QWidget(switchArea);
    toolBar->setObjectName(QLatin1String(CompactToolBarObjectName));
    toolBar->installEventFilter(this);
    auto toolBarLayout = new QHBoxLayout(toolBar);
    toolBarLayout->setContentsMargins(0, 0, 0, 0);
    toolBarLayout->setSpacing(4);

    menuButton = new QToolButton(toolBar);
    menuButton->setIcon(hamburgerIcon(menuButton));
    menuButton->setToolTip(trText("Show the main menu"));
    setupFlatButton(menuButton);
    menuButton->setFixedSize(28, 28);
    toolBarLayout->addWidget(menuButton);
    toolBarLayout->addStretch();

    menuBar = new QMenuBar(switchArea);
    menuBar->setObjectName(QLatin1String(CompactMenuBarObjectName));
    menuBar->installEventFilter(this);
    menuBar->hide();

    switchLayout->addWidget(toolBar);
    switchLayout->addWidget(menuBar);

    minimizeButton = createTitleButton(trText("Minimize"));
    maximizeButton = createTitleButton(trText("Maximize"));
    closeButton = createTitleButton(trText("Close"));

    topBarLayout->addWidget(appIconButton);
    topBarLayout->addWidget(switchArea, 1);
    topBarLayout->addWidget(minimizeButton);
    topBarLayout->addWidget(maximizeButton);
    topBarLayout->addWidget(closeButton);
    topBarLayout->setAlignment(appIconButton, Qt::AlignVCenter);
    topBarLayout->setAlignment(switchArea, Qt::AlignVCenter);
    topBarLayout->setAlignment(minimizeButton, Qt::AlignVCenter);
    topBarLayout->setAlignment(maximizeButton, Qt::AlignVCenter);
    topBarLayout->setAlignment(closeButton, Qt::AlignVCenter);

    connect(menuButton, &QToolButton::clicked, this, &CompactMainWindowChrome::showMainMenu);
    connect(menuBar, &QMenuBar::triggered, this, [this]() {
        QTimer::singleShot(0, this, &CompactMainWindowChrome::hideMainMenu);
    });
    connect(minimizeButton, &QToolButton::clicked, mainWindow, &MainWindow::showMinimized);
    connect(maximizeButton, &QToolButton::clicked, this, [this]() {
        mainWindow->isMaximized() ? mainWindow->showNormal() : mainWindow->showMaximized();
    });
    connect(closeButton, &QToolButton::clicked, mainWindow, &MainWindow::close);
    connect(mainWindow, &MainWindow::workbenchActivated, this, [this]() {
        QTimer::singleShot(0, this, &CompactMainWindowChrome::refreshPanelStrips);
    });

    removeLegacyDockStrips();

    leftStrip = createStrip(mainWindow, &leftStripContent, QLatin1String(CompactLeftStripObjectName));
    rightStrip
        = createStrip(mainWindow, &rightStripContent, QLatin1String(CompactRightStripObjectName));
    createResizeGrips();
}

void CompactMainWindowChrome::setActive(bool enabled)
{
    if (!topBar) {
        return;
    }

    topBar->setVisible(enabled);
    leftStrip->setVisible(enabled);
    rightStrip->setVisible(enabled);

    if (enabled && !active) {
        setGlobalEventFilterActive(true);
        menuBarVisibleBefore = mainWindow->menuBar()->isVisible();
        contentsMarginsBefore = mainWindow->contentsMargins();
        contentsMarginsSaved = true;
        mainWindow->menuBar()->hide();
        syncMenuBar();
        const QList<QDockWidget*> docks = managedDockContainers();
        for (auto dock : docks) {
            const auto slot = panelSlotForDock(dock);
            const bool wasVisible = dock->isVisible();
            mainWindow->addDockWidget(dockAreaForSlot(slot), dock);
            dock->setVisible(wasVisible);
        }
    }
    else if (!enabled && active) {
        hideMainMenu();
        setGlobalEventFilterActive(false);
        mainWindow->menuBar()->setVisible(menuBarVisibleBefore);
        if (contentsMarginsSaved) {
            mainWindow->setContentsMargins(contentsMarginsBefore);
            contentsMarginsSaved = false;
        }
        finishManualResize();
        titleDragActive = false;
    }

    active = enabled;
    if (enabled && toolBar && menuBar && !menuBar->isVisible()) {
        toolBar->show();
    }

    updateWindowControls();
    layoutChrome();
    refreshPanelStrips();
}

bool CompactMainWindowChrome::isActive() const
{
    return active;
}

void CompactMainWindowChrome::setGlobalEventFilterActive(bool enabled)
{
    if (enabled == eventFilterInstalled) {
        return;
    }

    if (enabled) {
        qApp->installEventFilter(this);
    }
    else {
        qApp->removeEventFilter(this);
    }
    eventFilterInstalled = enabled;
}

void CompactMainWindowChrome::syncMenuBar()
{
    if (!menuBar) {
        return;
    }

    for (auto action : menuBar->actions()) {
        menuBar->removeAction(action);
    }
    menuBar->addActions(mainWindow->menuBar()->actions());
}

void CompactMainWindowChrome::updateHamburgerIcon()
{
    if (menuButton) {
        menuButton->setIcon(hamburgerIcon(menuButton));
    }
}

void CompactMainWindowChrome::updateWindowControls()
{
    if (!mainWindow) {
        return;
    }

    if (appIconButton) {
        QIcon appIcon = mainWindow->windowIcon().isNull() ? QApplication::windowIcon()
                                                          : mainWindow->windowIcon();
        if (appIcon.isNull()) {
            appIcon = mainWindow->style()->standardIcon(QStyle::SP_TitleBarMenuButton);
        }
        appIconButton->setIcon(appIcon);
    }

    if (minimizeButton) {
        minimizeButton->setIcon(mainWindow->style()->standardIcon(QStyle::SP_TitleBarMinButton));
    }

    if (maximizeButton) {
        const bool maximized = mainWindow->isMaximized();
        maximizeButton->setIcon(mainWindow->style()->standardIcon(
            maximized ? QStyle::SP_TitleBarNormalButton : QStyle::SP_TitleBarMaxButton
        ));
        maximizeButton->setToolTip(maximized ? trText("Restore") : trText("Maximize"));
    }

    if (closeButton) {
        closeButton->setIcon(mainWindow->style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    }
}

void CompactMainWindowChrome::showMainMenu()
{
    if (!menuBar) {
        return;
    }

    if (menuBar->isVisible()) {
        hideMainMenu();
        return;
    }

    syncMenuBar();
    if (toolBar) {
        toolBar->hide();
    }
    menuBar->show();
    layoutChrome();
    QTimer::singleShot(0, this, &CompactMainWindowChrome::openFirstMenu);
}

void CompactMainWindowChrome::hideMainMenu()
{
    if (!active || !menuBar || !menuBar->isVisible()) {
        return;
    }

    menuBar->hide();
    if (toolBar) {
        toolBar->show();
    }
    layoutChrome();
}

void CompactMainWindowChrome::openFirstMenu()
{
    if (!menuBar || !menuBar->isVisible()) {
        return;
    }

    const auto actions = menuBar->actions();
    if (actions.isEmpty()) {
        return;
    }

    QAction* firstAction = actions.constFirst();
    menuBar->setActiveAction(firstAction);

    if (QMenu* firstMenu = firstAction->menu()) {
        const QRect actionRect = menuBar->actionGeometry(firstAction);
        firstMenu->popup(menuBar->mapToGlobal(actionRect.bottomLeft()));
    }
}

void CompactMainWindowChrome::layoutChrome()
{
    if (!active || !topBar) {
        setResizeGripsVisible(false);
        return;
    }

    updateWindowControls();
    layoutTopBar();
    applyContentsMargins();
    layoutPanelStrips();
    layoutResizeGrips();
}

void CompactMainWindowChrome::applyContentsMargins()
{
    if (!active || !contentsMarginsSaved) {
        return;
    }

    const int topBarHeight = topBar && topBar->isVisible() ? topBar->height() : 0;
    mainWindow->setContentsMargins(
        contentsMarginsBefore.left() + CompactPanelStripWidth,
        contentsMarginsBefore.top() + topBarHeight,
        contentsMarginsBefore.right() + CompactPanelStripWidth,
        contentsMarginsBefore.bottom()
    );
}

void CompactMainWindowChrome::layoutTopBar()
{
    if (!active || !topBar) {
        return;
    }

    if (toolBar && menuBar) {
        const int childHeight = std::max(toolBar->sizeHint().height(), menuBar->sizeHint().height());
        toolBar->setFixedHeight(childHeight);
        menuBar->setFixedHeight(childHeight);
        if (switchArea) {
            switchArea->setFixedHeight(childHeight);
        }
    }

    const int topBarHeight = topBar->sizeHint().height();
    topBar->setGeometry(0, 0, mainWindow->width(), topBarHeight);
    topBar->raise();
}

void CompactMainWindowChrome::layoutPanelStrips()
{
    if (!active || !leftStrip || !rightStrip) {
        return;
    }

    int top = 0;
    if (topBar && topBar->isVisible()) {
        top = topBar->geometry().bottom() + 1;
    }
    else if (mainWindow->menuBar() && mainWindow->menuBar()->isVisible()) {
        top = mainWindow->menuBar()->geometry().bottom() + 1;
    }

    int bottom = mainWindow->height();
    if (mainWindow->statusBar() && mainWindow->statusBar()->isVisible()) {
        bottom = mainWindow->statusBar()->geometry().top();
    }

    const int stripHeight = std::max(0, bottom - top);
    leftStrip->setGeometry(0, top, CompactPanelStripWidth, stripHeight);
    rightStrip->setGeometry(
        mainWindow->width() - CompactPanelStripWidth,
        top,
        CompactPanelStripWidth,
        stripHeight
    );
    leftStrip->raise();
    rightStrip->raise();
    if (topBar) {
        topBar->raise();
    }
}

void CompactMainWindowChrome::refreshPanelStrips()
{
    if (!leftStripContent || !rightStripContent) {
        return;
    }

    clearStrip(leftStripContent);
    clearStrip(rightStripContent);

    auto addButton = [this](QDockWidget* dock, QWidget* content, PanelSlot slot) {
        auto button = new QToolButton(content);
        const QString title = dockTitle(dock);
        button->setIcon(dockIcon(dock, slot));
        button->setToolTip(title);
        button->setCheckable(true);
        button->setChecked(dock->isVisible());
        setupFlatButton(button);
        button->setFixedSize(28, 28);

        connect(button, &QToolButton::clicked, this, [this, dock, slot]() {
            if (dock->isVisible()) {
                dock->toggleViewAction()->activate(QAction::Trigger);
                refreshPanelStrips();
                return;
            }

            const auto group = panelGroup(slot);
            const QList<QDockWidget*> docks = managedDockContainers();
            for (auto other : docks) {
                if (other == dock) {
                    continue;
                }

                if (panelGroup(panelSlotForDock(other)) == group && other->isVisible()) {
                    other->toggleViewAction()->activate(QAction::Trigger);
                }
            }

            if (!dock->isVisible()) {
                dock->toggleViewAction()->activate(QAction::Trigger);
            }
            dock->raise();
            refreshPanelStrips();
        });

        content->layout()->addWidget(button);
    };

    QList<PanelEntry> entries;
    const QList<QDockWidget*> docks = managedDockContainers();
    for (auto dock : docks) {
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
                addButton(entry.dock, leftStripContent, entry.slot);
                addedLeftTop = true;
                break;
            case PanelSlot::LeftLower:
                if (!addedLeftSeparator) {
                    if (addedLeftTop) {
                        leftStripContent->layout()->addWidget(createStripSeparator(leftStripContent));
                    }
                    addedLeftSeparator = true;
                }
                addButton(entry.dock, leftStripContent, entry.slot);
                break;
            case PanelSlot::RightTop:
                addButton(entry.dock, rightStripContent, entry.slot);
                addedRightTop = true;
                break;
            case PanelSlot::RightLower:
                if (!addedRightSeparator) {
                    if (addedRightTop) {
                        rightStripContent->layout()->addWidget(createStripSeparator(rightStripContent));
                    }
                    addedRightSeparator = true;
                }
                addButton(entry.dock, rightStripContent, entry.slot);
                break;
            case PanelSlot::BottomLeft:
                if (!addedLeftBottomSpacer) {
                    addStripSpacer(leftStripContent, Qt::Vertical);
                    addedLeftBottomSpacer = true;
                }
                addButton(entry.dock, leftStripContent, entry.slot);
                break;
            case PanelSlot::BottomRight:
                if (!addedRightBottomSpacer) {
                    addStripSpacer(rightStripContent, Qt::Vertical);
                    addedRightBottomSpacer = true;
                }
                addButton(entry.dock, rightStripContent, entry.slot);
                break;
        }
    }

    if (!addedLeftBottomSpacer) {
        addStripSpacer(leftStripContent, Qt::Vertical);
    }
    if (!addedRightBottomSpacer) {
        addStripSpacer(rightStripContent, Qt::Vertical);
    }

    layoutPanelStrips();
}

void CompactMainWindowChrome::createResizeGrips()
{
    const QList<ResizeGrip> grips = {
        {new QWidget(mainWindow), Qt::LeftEdge},
        {new QWidget(mainWindow), Qt::RightEdge},
        {new QWidget(mainWindow), Qt::TopEdge},
        {new QWidget(mainWindow), Qt::BottomEdge},
        {new QWidget(mainWindow), Qt::LeftEdge | Qt::TopEdge},
        {new QWidget(mainWindow), Qt::RightEdge | Qt::TopEdge},
        {new QWidget(mainWindow), Qt::LeftEdge | Qt::BottomEdge},
        {new QWidget(mainWindow), Qt::RightEdge | Qt::BottomEdge},
    };

    for (const auto& grip : grips) {
        grip.widget->setObjectName(QStringLiteral("_fc_compact_resize_grip"));
        grip.widget->setCursor(cursorForEdges(grip.edges));
        grip.widget->installEventFilter(this);
        grip.widget->hide();
        resizeGripEdges.insert(grip.widget, grip.edges);
        resizeGrips.push_back(grip);
    }
}

void CompactMainWindowChrome::layoutResizeGrips()
{
    const bool visible = active && framelessWindow && !mainWindow->isMaximized()
        && !mainWindow->isFullScreen();
    setResizeGripsVisible(visible);
    if (!visible) {
        return;
    }

    const int width = mainWindow->width();
    const int height = mainWindow->height();
    const int border = CompactResizeBorderWidth;

    for (const auto& grip : resizeGrips) {
        QRect geometry;
        if (grip.edges == Qt::LeftEdge) {
            geometry = QRect(0, border, border, height - 2 * border);
        }
        else if (grip.edges == Qt::RightEdge) {
            geometry = QRect(width - border, border, border, height - 2 * border);
        }
        else if (grip.edges == Qt::TopEdge) {
            geometry = QRect(border, 0, width - 2 * border, border);
        }
        else if (grip.edges == Qt::BottomEdge) {
            geometry = QRect(border, height - border, width - 2 * border, border);
        }
        else if (grip.edges == (Qt::LeftEdge | Qt::TopEdge)) {
            geometry = QRect(0, 0, border, border);
        }
        else if (grip.edges == (Qt::RightEdge | Qt::TopEdge)) {
            geometry = QRect(width - border, 0, border, border);
        }
        else if (grip.edges == (Qt::LeftEdge | Qt::BottomEdge)) {
            geometry = QRect(0, height - border, border, border);
        }
        else if (grip.edges == (Qt::RightEdge | Qt::BottomEdge)) {
            geometry = QRect(width - border, height - border, border, border);
        }

        grip.widget->setGeometry(geometry);
        grip.widget->raise();
    }
}

void CompactMainWindowChrome::setResizeGripsVisible(bool visible)
{
    for (const auto& grip : resizeGrips) {
        grip.widget->setVisible(visible);
    }
}

bool CompactMainWindowChrome::eventFilter(QObject* watched, QEvent* event)
{
    if (!mainWindow || !active) {
        return QObject::eventFilter(watched, event);
    }

    if (resizeGripEdges.contains(watched)) {
        auto grip = qobject_cast<QWidget*>(watched);
        const Qt::Edges edges = resizeGripEdges.value(watched);
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (QWindow* window = mainWindow->windowHandle();
                    window && window->startSystemResize(edges)) {
                    return true;
                }
                startManualResize(edges, mouseEvent, grip);
                return true;
            }
        }
        if (manualResizeActive && event->type() == QEvent::MouseMove) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons() & Qt::LeftButton) {
                updateManualResize(mouseEvent);
                return true;
            }
            finishManualResize();
        }
        if (manualResizeActive
            && (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::Leave)) {
            finishManualResize();
            return true;
        }
    }

    auto compactTitleWidget = watched->isWidgetType() ? qobject_cast<QWidget*>(watched) : nullptr;
    const bool compactTitleBarEvent = topBar && compactTitleWidget
        && (compactTitleWidget == topBar || topBar->isAncestorOf(compactTitleWidget));
    const bool compactTitleDragSource = compactTitleBarEvent
        && !qobject_cast<QToolButton*>(compactTitleWidget)
        && !qobject_cast<QMenuBar*>(compactTitleWidget);

    if (compactTitleDragSource && event->type() == QEvent::MouseButtonDblClick) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            mainWindow->isMaximized() ? mainWindow->showNormal() : mainWindow->showMaximized();
            return true;
        }
    }

    if (compactTitleDragSource && event->type() == QEvent::MouseButtonPress) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (QWindow* window = mainWindow->windowHandle(); window && window->startSystemMove()) {
                titleDragActive = false;
                return true;
            }

            titleDragActive = true;
            titleDragGlobalPosition = globalPosition(mouseEvent);
            titleDragWindowPosition = mainWindow->frameGeometry().topLeft();
            return true;
        }
    }
    else if (titleDragActive && event->type() == QEvent::MouseMove) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->buttons() & Qt::LeftButton) {
            const QPoint position = globalPosition(mouseEvent);
            if (mainWindow->isMaximized()) {
                mainWindow->showNormal();
                titleDragWindowPosition
                    = QPoint(position.x() - mainWindow->width() / 2, position.y() - 14);
                titleDragGlobalPosition = position;
            }
            mainWindow->move(titleDragWindowPosition + position - titleDragGlobalPosition);
            return true;
        }
        titleDragActive = false;
    }
    else if (
        titleDragActive
        && (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::Leave)
    ) {
        titleDragActive = false;
    }

    if (menuBar && menuBar->isVisible() && event->type() == QEvent::MouseButtonPress) {
        const bool insideTopBar = compactTitleBarEvent;
        const bool insideMenu = watched->inherits("QMenu");
        if (!insideTopBar && !insideMenu) {
            hideMainMenu();
        }
    }

    return QObject::eventFilter(watched, event);
}

void CompactMainWindowChrome::startManualResize(Qt::Edges edges, QMouseEvent* event, QWidget* grip)
{
    manualResizeActive = true;
    manualResizeEdges = edges;
    manualResizeGlobalPosition = globalPosition(event);
    manualResizeGeometry = mainWindow->geometry();
    if (grip) {
        grip->grabMouse();
    }
}

void CompactMainWindowChrome::updateManualResize(QMouseEvent* event)
{
    if (!manualResizeActive || mainWindow->isMaximized() || mainWindow->isFullScreen()) {
        return;
    }

    const QPoint delta = globalPosition(event) - manualResizeGlobalPosition;
    mainWindow->setGeometry(resizedGeometry(
        manualResizeGeometry,
        delta,
        manualResizeEdges,
        mainWindow->minimumSize(),
        mainWindow->maximumSize()
    ));
}

void CompactMainWindowChrome::finishManualResize()
{
    if (!manualResizeActive) {
        return;
    }

    manualResizeActive = false;
    if (QWidget* mouseGrabber = QWidget::mouseGrabber()) {
        if (resizeGripEdges.contains(mouseGrabber)) {
            mouseGrabber->releaseMouse();
        }
    }
}

QList<QDockWidget*> CompactMainWindowChrome::managedDockContainers() const
{
    QList<QDockWidget*> docks;
    const QList<QWidget*> widgets = DockWindowManager::instance()->getDockWindows();
    for (auto widget : widgets) {
        auto parent = widget ? widget->parentWidget() : nullptr;
        while (parent) {
            if (auto dock = qobject_cast<QDockWidget*>(parent)) {
                if (isCompactUiDockCandidate(dock) && !docks.contains(dock)) {
                    docks.push_back(dock);
                }
                break;
            }
            parent = parent->parentWidget();
        }
    }

    return docks;
}

QString CompactMainWindowChrome::dockActionId(const QDockWidget* dock) const
{
    if (!dock || !dock->toggleViewAction()) {
        return {};
    }

    return dock->toggleViewAction()->data().toString();
}

const CompactMainWindowChrome::KnownPanel* CompactMainWindowChrome::knownPanelForActionId(
    const QString& actionId
) const
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

QString CompactMainWindowChrome::dockTitle(const QDockWidget* dock) const
{
    QString title = dock->windowTitle();
    if (title.isEmpty()) {
        title = dock->objectName();
    }

    title.remove(QLatin1Char('&'));
    return title;
}

CompactMainWindowChrome::PanelGroup CompactMainWindowChrome::panelGroup(PanelSlot slot) const
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

CompactMainWindowChrome::PanelSlot CompactMainWindowChrome::fallbackSlotForDock(QDockWidget* dock) const
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

CompactMainWindowChrome::PanelSlot CompactMainWindowChrome::panelSlotForDock(QDockWidget* dock) const
{
    if (const auto* knownPanel = knownPanelForActionId(dockActionId(dock))) {
        return knownPanel->slot;
    }

    return fallbackSlotForDock(dock);
}

Qt::DockWidgetArea CompactMainWindowChrome::dockAreaForSlot(PanelSlot slot) const
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

int CompactMainWindowChrome::panelOrderForDock(const QDockWidget* dock, PanelSlot slot) const
{
    if (const auto* knownPanel = knownPanelForActionId(dockActionId(dock))) {
        return knownPanel->order;
    }

    const int fallbackBase = 1000;
    return fallbackBase + static_cast<int>(slot);
}

QIcon CompactMainWindowChrome::dockIcon(const QDockWidget* dock, PanelSlot slot) const
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

QToolButton* CompactMainWindowChrome::createTitleButton(const QString& tooltip)
{
    auto button = new QToolButton(topBar);
    button->setToolTip(tooltip);
    setupFlatButton(button);
    button->setFixedSize(28, 28);
    return button;
}

void CompactMainWindowChrome::setupFlatButton(QToolButton* button)
{
    button->setAutoRaise(true);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setFocusPolicy(Qt::StrongFocus);
}

void CompactMainWindowChrome::removeLegacyDockStrips()
{
    const QList<QDockWidget*> docks = mainWindow->findChildren<QDockWidget*>();
    for (auto dock : docks) {
        if (isCompactUiDock(dock)) {
            dock->hide();
            dock->deleteLater();
        }
    }
}

#include "moc_CompactMainWindowChrome.cpp"
