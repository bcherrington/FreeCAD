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


#include "CompactToolBarContainer.h"

#include <QApplication>
#include <QBoxLayout>
#include <QByteArray>
#include <QContextMenuEvent>
#include <QCursor>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>
#include <QTimer>
#include <QToolBar>

#include <algorithm>
#include <utility>

#include <App/Application.h>

#include "MainWindow.h"
#include "ToolBarManager.h"


using namespace Gui;

namespace
{
constexpr int MinimumContainerHeight = 36;
constexpr int ToolbarPadding = 12;
constexpr int MinimumDropZoneWidth = 36;
constexpr int HandleWidth = 12;
constexpr int ZoneGap = 0;
constexpr int DragDistancePadding = 2;
constexpr const char* ParameterGroup = "User parameter:BaseApp/MainWindow/CompactToolBarContainer";

QPoint globalMousePosition(const QMouseEvent* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->globalPos();
#else
    return event->globalPosition().toPoint();
#endif
}

CompactToolBarContainer::Zone zoneFromInt(int value)
{
    switch (value) {
        case 1:
            return CompactToolBarContainer::Zone::Center;
        case 2:
            return CompactToolBarContainer::Zone::Right;
        case 0:
        default:
            return CompactToolBarContainer::Zone::Left;
    }
}

int zoneToInt(CompactToolBarContainer::Zone zone)
{
    switch (zone) {
        case CompactToolBarContainer::Zone::Center:
            return 1;
        case CompactToolBarContainer::Zone::Right:
            return 2;
        case CompactToolBarContainer::Zone::Left:
        default:
            return 0;
    }
}

void paintVerticalGrip(QPainter& painter, const QRect& rect, const QColor& color)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    const int firstX = rect.left() + 3;
    const int firstY = rect.top() + 6;
    for (int x = firstX; x <= firstX + 4; x += 4) {
        for (int y = firstY; y <= rect.bottom() - 5; y += 4) {
            painter.drawEllipse(QPoint(x, y), 1, 1);
        }
    }
}

class CompactToolBarHandle: public QWidget
{
public:
    explicit CompactToolBarHandle(QToolBar* toolBar, QWidget* parent)
        : QWidget(parent)
        , toolBar(toolBar)
    {
        setObjectName(QStringLiteral("_fc_compact_toolbar_handle"));
        setCursor(Qt::OpenHandCursor);
        setFixedWidth(HandleWidth);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        auto hostedToolBar = toolBar.data();
        if (!hostedToolBar) {
            return;
        }

        QPainter painter(this);
        const auto midColor = hostedToolBar->palette().color(QPalette::Mid);
        paintVerticalGrip(painter, rect(), midColor);
    }

private:
    QPointer<QToolBar> toolBar;
};

class CompactToolBarTestWidget: public QWidget
{
public:
    explicit CompactToolBarTestWidget(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("_fc_compact_toolbar_test_widget"));
        setProperty("_fc_compact_toolbar_handle_visible", true);
        setMinimumSize(120, MinimumContainerHeight - 4);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const auto buttonColor = palette().color(QPalette::Button);
        const auto midColor = palette().color(QPalette::Mid);
        painter.setPen(QPen(midColor, 1));
        painter.setBrush(buttonColor);
        painter.drawRoundedRect(rect().adjusted(1, 3, -1, -3), 2, 2);

        const bool showHandle = property("_fc_compact_toolbar_handle_visible").toBool();
        if (showHandle) {
            paintVerticalGrip(painter, QRect(4, 4, HandleWidth, height() - 8), midColor);
        }

        painter.setPen(palette().color(QPalette::ButtonText));
        const int textLeft = showHandle ? HandleWidth + 10 : 8;
        painter.drawText(
            rect().adjusted(textLeft, 0, -6, 0),
            Qt::AlignVCenter,
            QStringLiteral("Test menu")
        );
    }
};
}  // namespace

CompactToolBarContainer::CompactToolBarContainer(MainWindow* mainWindow, QWidget* parent)
    : QWidget(parent)
    , mainWindow(mainWindow)
{
    setObjectName(QStringLiteral("_fc_compact_toolbar_container"));
    const int iconHeight = mainWindow ? mainWindow->iconSize().height() : 24;
    setMinimumHeight(std::max(MinimumContainerHeight, iconHeight + ToolbarPadding));
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setupUi();
}

CompactToolBarContainer::~CompactToolBarContainer()
{
    removeToolBarFilters();
    detachToolBarsForShutdown();
}

void CompactToolBarContainer::setActive(bool enabled)
{
    if (active == enabled) {
        return;
    }

    active = enabled;
    setVisible(active);
    if (active) {
        installToolBarFilters();
        restoreStoredToolBars();
    }
    else {
        removeToolBarFilters();
        restoreToolBarsToMainWindow();
        clearActiveZone();
    }
}

bool CompactToolBarContainer::isActive() const
{
    return active;
}

void CompactToolBarContainer::installToolBarFilters()
{
    if (!mainWindow) {
        return;
    }

    const auto toolBars = mainWindow->findChildren<QToolBar*>();
    for (auto toolBar : toolBars) {
        if (!toolBar || containsToolBar(toolBar)) {
            continue;
        }

        if (filteredToolBars.contains(toolBar)) {
            continue;
        }

        toolBar->installEventFilter(this);
        filteredToolBars.insert(toolBar);
        trackToolBarDestroyed(toolBar);
    }
}

void CompactToolBarContainer::removeToolBarFilters()
{
    if (!mainWindow) {
        return;
    }

    const auto toolBars = filteredToolBars;
    for (auto toolBar : toolBars) {
        if (toolBar) {
            toolBar->removeEventFilter(this);
        }
    }
    filteredToolBars.clear();
}

void CompactToolBarContainer::addToolBar(QToolBar* toolBar, Zone zone)
{
    if (!toolBar) {
        return;
    }

    removeToolBarFromLayouts(toolBar);
    if (filteredToolBars.remove(toolBar) > 0) {
        toolBar->removeEventFilter(this);
    }
    if (mainWindow) {
        mainWindow->removeToolBar(toolBar);
    }

    toolBar->setOrientation(Qt::Horizontal);
    toolBar->setFloatable(true);
    toolBar->setMovable(!toolBarsLocked());
    toolBar->setProperty("_fc_compact_toolbar_hosted", true);
    toolBar->setVisible(true);

    containedToolBars.insert(toolBar, zone);
    createHostedToolBarWrapper(toolBar);
    addToolBarToZoneLayout(toolBar, zone);
    installContainedToolBarFilter(toolBar);
    persistToolBarZone(toolBar, zone);
    updateLabels();
}

void CompactToolBarContainer::restoreStoredToolBars()
{
    if (!mainWindow) {
        return;
    }

    auto group = App::GetApplication().GetParameterGroupByPath(ParameterGroup);
    const auto entries = group->GetIntMap();
    for (const auto& [name, value] : entries) {
        auto toolBar = mainWindow->findChild<QToolBar*>(QString::fromUtf8(name.c_str()));
        if (!toolBar || containsToolBar(toolBar) || toolBar->isFloating()) {
            continue;
        }

        addToolBar(toolBar, zoneFromInt(value));
    }
}

void CompactToolBarContainer::restoreToolBarsToMainWindow()
{
    if (!mainWindow) {
        return;
    }

    const auto toolBars = containedToolBars.keys();
    for (auto toolBar : toolBars) {
        if (!toolBar) {
            continue;
        }

        restoreToolBarToMainWindow(toolBar);
    }

    updateLabels();
}

bool CompactToolBarContainer::eventFilter(QObject* watched, QEvent* event)
{
    if (!active) {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::ContextMenu) {
        auto contextEvent = static_cast<QContextMenuEvent*>(event);
        if (showContextMenu(contextEvent->globalPos())) {
            contextEvent->accept();
            return true;
        }
    }

    auto toolBar = watchedContainedToolBar(watched);
    if (!toolBar) {
        toolBar = qobject_cast<QToolBar*>(watched);
    }
    if (!toolBar) {
        return QWidget::eventFilter(watched, event);
    }

    if (containsToolBar(toolBar)) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                dragStartPositions.insert(toolBar, globalMousePosition(mouseEvent));
            }
        }
        if (event->type() == QEvent::MouseMove) {
            return handleContainedToolBarMouseMove(toolBar, static_cast<QMouseEvent*>(event));
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            const bool hasDragStart = dragStartPositions.contains(toolBar);
            dragStartPositions.remove(toolBar);
            return hasDragStart && handleContainedToolBarMouseRelease(toolBar);
        }
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseMove && toolBar->isFloating()) {
        const auto zone = zoneAtGlobalPos(QCursor::pos());
        const QRect globalRect(mapToGlobal(QPoint(0, 0)), size());
        if (globalRect.contains(QCursor::pos())) {
            setActiveZone(zone);
        }
        else {
            clearActiveZone();
        }
    }

    if (event->type() == QEvent::MouseButtonRelease && toolBar->isFloating()) {
        if (handleToolBarRelease(toolBar)) {
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void CompactToolBarContainer::contextMenuEvent(QContextMenuEvent* event)
{
    if (showContextMenu(event->globalPos())) {
        event->accept();
        return;
    }

    QWidget::contextMenuEvent(event);
}

void CompactToolBarContainer::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto windowColor = palette().color(QPalette::Window);
    const auto midColor = palette().color(QPalette::Mid);
    painter.fillRect(rect(), windowColor);
    painter.setPen(QPen(midColor, 1));
    painter.drawLine(rect().topLeft(), rect().topRight());
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());

    if (containedToolBars.isEmpty()) {
        auto hintColor = midColor;
        hintColor.setAlpha(90);
        QPen hintPen(hintColor, 1);
        hintPen.setStyle(Qt::DashLine);
        painter.setPen(hintPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(4, 4, -4, -4), 3, 3);
    }

    if (!hasActiveZone) {
        return;
    }

    const QColor highlight = palette().color(QPalette::Highlight);
    for (auto zone : {Zone::Left, Zone::Center, Zone::Right}) {
        const QRect rect = zoneRect(zone).adjusted(2, 2, -2, -2);
        const bool selected = hasActiveZone && activeZone == zone;
        painter.setPen(QPen(highlight, selected ? 2 : 1));
        painter.setBrush(
            QColor(highlight.red(), highlight.green(), highlight.blue(), selected ? 80 : 28)
        );
        painter.drawRoundedRect(rect, 3, 3);
    }
}

void CompactToolBarContainer::setupUi()
{
    rootLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(ZoneGap);

    auto createZone =
        [this](const QString& text, QBoxLayout** zoneLayout, QWidget** zoneWidgetStorage, QLabel** label) {
            auto zoneWidget = new QWidget(this);
            zoneWidget->setMinimumWidth(MinimumDropZoneWidth);
            auto layout = new QBoxLayout(QBoxLayout::LeftToRight, zoneWidget);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            auto zoneLabel = new QLabel(text, zoneWidget);
            zoneLabel->setAlignment(Qt::AlignCenter);
            zoneLabel->hide();
            layout->addWidget(zoneLabel);
            *zoneLayout = layout;
            *zoneWidgetStorage = zoneWidget;
            *label = zoneLabel;
        };

    createZone(tr("Left"), &leftLayout, &leftZoneWidget, &leftLabel);
    createZone(tr("Center"), &centerLayout, &centerZoneWidget, &centerLabel);
    createZone(tr("Right"), &rightLayout, &rightZoneWidget, &rightLabel);

    emptyTestWidget = new CompactToolBarTestWidget(this);
    emptyTestWidget->installEventFilter(this);
    leftLayout->addWidget(emptyTestWidget);

    rootLayout->addWidget(leftZoneWidget);
    rootLayout->addStretch(1);
    rootLayout->addWidget(centerZoneWidget);
    rootLayout->addStretch(1);
    rootLayout->addWidget(rightZoneWidget);
}

void CompactToolBarContainer::updateLabels()
{
    setFixedHeight(std::max(minimumHeight(), sizeHint().height()));
    if (emptyTestWidget) {
        emptyTestWidget->setVisible(containedToolBars.isEmpty());
    }
    if (leftLabel) {
        leftLabel->hide();
    }
    if (centerLabel) {
        centerLabel->hide();
    }
    if (rightLabel) {
        rightLabel->hide();
    }
    updateHandleVisibility();
    update();
}

void CompactToolBarContainer::setActiveZone(Zone zone)
{
    if (hasActiveZone && activeZone == zone) {
        return;
    }

    activeZone = zone;
    hasActiveZone = true;
    update();
}

void CompactToolBarContainer::clearActiveZone()
{
    if (!hasActiveZone) {
        clearDragPreview();
        return;
    }

    hasActiveZone = false;
    clearDragPreview();
    update();
}

void CompactToolBarContainer::updateDragPreview(QToolBar* toolBar, const QPoint& globalPos)
{
    if (!toolBar) {
        clearDragPreview();
        return;
    }

    auto label = qobject_cast<QLabel*>(dragPreview.data());
    if (!label) {
        label = new QLabel(this);
        label->setObjectName(QStringLiteral("_fc_compact_toolbar_drag_preview"));
        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        label->setAlignment(Qt::AlignVCenter);
        label->setAutoFillBackground(true);
        label->setFrameShape(QFrame::StyledPanel);
        label->setFrameShadow(QFrame::Raised);
        label->setMargin(8);
        dragPreview = label;
    }

    const QString title = toolBar->windowTitle().isEmpty() ? toolBar->objectName()
                                                           : toolBar->windowTitle();
    label->setText(title.isEmpty() ? tr("Toolbar") : title);
    label->adjustSize();

    const auto sourceWidget = hostedToolBarWidget(toolBar);
    const QSize previewSize(
        std::max(label->sizeHint().width(), sourceWidget ? sourceWidget->width() : toolBar->width()),
        std::max(minimumHeight() - 6, sourceWidget ? sourceWidget->height() : toolBar->height())
    );
    const QPoint localPos = mapFromGlobal(globalPos);
    const int x = std::clamp(
        localPos.x() - (previewSize.width() / 2),
        0,
        std::max(0, width() - previewSize.width())
    );
    const int y = std::max(3, (height() - previewSize.height()) / 2);
    label->setGeometry(QRect(QPoint(x, y), previewSize));
    label->show();
    label->raise();
}

void CompactToolBarContainer::clearDragPreview()
{
    if (dragPreview) {
        dragPreview->hide();
    }
}

bool CompactToolBarContainer::showContextMenu(const QPoint& globalPos)
{
    if (!mainWindow) {
        return false;
    }

    auto menu = mainWindow->createPopupMenu();
    if (!menu) {
        return false;
    }

    menu->setAttribute(Qt::WA_DeleteOnClose);
    connect(menu, &QMenu::aboutToHide, this, &CompactToolBarContainer::updateHandleVisibility);
    menu->popup(globalPos);
    return true;
}

bool CompactToolBarContainer::toolBarsLocked() const
{
    auto manager = ToolBarManager::getInstance();
    return manager && manager->areToolBarsLocked();
}

void CompactToolBarContainer::updateHandleVisibility()
{
    const bool locked = toolBarsLocked();
    const bool visible = !locked;
    if (emptyTestWidget) {
        emptyTestWidget->setProperty("_fc_compact_toolbar_handle_visible", visible);
        emptyTestWidget->update();
    }

    for (auto it = toolBarWrappers.cbegin(); it != toolBarWrappers.cend(); ++it) {
        auto toolBar = it.key();
        if (toolBar) {
            toolBar->setMovable(!locked);
        }

        auto wrapper = it.value();
        if (!wrapper) {
            continue;
        }

        const auto handles = wrapper->findChildren<QWidget*>(
            QStringLiteral("_fc_compact_toolbar_handle")
        );
        for (auto handle : handles) {
            handle->setVisible(visible);
        }
    }
}

CompactToolBarContainer::Zone CompactToolBarContainer::zoneAtGlobalPos(const QPoint& globalPos) const
{
    const QPoint pos = mapFromGlobal(globalPos);
    const QRect center = zoneRect(Zone::Center);

    if (pos.x() < center.left()) {
        return Zone::Left;
    }
    if (pos.x() > center.right()) {
        return Zone::Right;
    }
    return Zone::Center;
}

QRect CompactToolBarContainer::zoneRect(Zone zone) const
{
    const auto leftWidget = widgetForZone(Zone::Left);
    const auto centerWidget = widgetForZone(Zone::Center);
    const auto rightWidget = widgetForZone(Zone::Right);
    if (!leftWidget || !centerWidget || !rightWidget) {
        return {};
    }

    const QRect leftGroup = leftWidget->geometry();
    const QRect centerGroup = centerWidget->geometry();
    const QRect rightGroup = rightWidget->geometry();
    const int leftCenterBoundary = (leftGroup.center().x() + centerGroup.center().x()) / 2;
    const int centerRightBoundary = (centerGroup.center().x() + rightGroup.center().x()) / 2;

    switch (zone) {
        case Zone::Left:
            return QRect(0, 0, std::max(0, leftCenterBoundary), height());
        case Zone::Center:
            return QRect(
                leftCenterBoundary,
                0,
                std::max(0, centerRightBoundary - leftCenterBoundary),
                height()
            );
        case Zone::Right:
        default:
            return QRect(centerRightBoundary, 0, std::max(0, width() - centerRightBoundary), height());
    }
}

QBoxLayout* CompactToolBarContainer::layoutForZone(Zone zone) const
{
    switch (zone) {
        case Zone::Left:
            return leftLayout;
        case Zone::Center:
            return centerLayout;
        case Zone::Right:
        default:
            return rightLayout;
    }
}

QWidget* CompactToolBarContainer::widgetForZone(Zone zone) const
{
    switch (zone) {
        case Zone::Left:
            return leftZoneWidget;
        case Zone::Center:
            return centerZoneWidget;
        case Zone::Right:
        default:
            return rightZoneWidget;
    }
}

void CompactToolBarContainer::addToolBarToZoneLayout(QToolBar* toolBar, Zone zone)
{
    insertToolBarToZoneLayout(toolBar, zone, -1);
}

void CompactToolBarContainer::insertToolBarToZoneLayout(QToolBar* toolBar, Zone zone, int index)
{
    auto layout = layoutForZone(zone);
    if (!layout || !toolBar) {
        return;
    }

    auto widget = hostedToolBarWidget(toolBar);
    if (index >= 0) {
        layout->insertWidget(std::clamp(index, 0, layout->count()), widget);
        return;
    }

    switch (zone) {
        case Zone::Center:
            layout->insertWidget(std::max(0, layout->count() - 1), widget);
            break;
        case Zone::Right:
        case Zone::Left:
        default:
            layout->addWidget(widget);
            break;
    }
}

int CompactToolBarContainer::insertionIndexForZone(
    Zone zone,
    const QPoint& globalPos,
    QToolBar* movingToolBar
) const
{
    auto layout = layoutForZone(zone);
    if (!layout) {
        return -1;
    }

    const int x = globalPos.x();
    int fallback = layout->count();
    for (int index = 0; index < layout->count(); ++index) {
        auto item = layout->itemAt(index);
        auto widget = item ? item->widget() : nullptr;
        if (!widget || widget == hostedToolBarWidget(movingToolBar) || widget->isHidden()) {
            continue;
        }

        const int centerX = widget->mapToGlobal(widget->rect().center()).x();
        if (x < centerX) {
            return index;
        }
        fallback = index + 1;
    }

    return fallback;
}

int CompactToolBarContainer::toolBarCount(Zone zone) const
{
    int count = 0;
    for (auto it = containedToolBars.cbegin(); it != containedToolBars.cend(); ++it) {
        if (it.value() == zone) {
            ++count;
        }
    }

    return count;
}

bool CompactToolBarContainer::handleToolBarRelease(QToolBar* toolBar)
{
    const QRect globalRect(mapToGlobal(QPoint(0, 0)), size());
    if (!globalRect.contains(QCursor::pos())) {
        clearActiveZone();
        return false;
    }

    addToolBar(toolBar, zoneAtGlobalPos(QCursor::pos()));
    clearActiveZone();
    return true;
}

bool CompactToolBarContainer::handleContainedToolBarMouseMove(QToolBar* toolBar, QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton) || !dragStartPositions.contains(toolBar)) {
        return false;
    }

    const QPoint start = dragStartPositions.value(toolBar);
    const int distance = (globalMousePosition(event) - start).manhattanLength();
    if (distance < QApplication::startDragDistance() + DragDistancePadding) {
        return false;
    }

    const QRect globalRect(mapToGlobal(QPoint(0, 0)), size());
    if (globalRect.contains(globalMousePosition(event))) {
        setActiveZone(zoneAtGlobalPos(globalMousePosition(event)));
        updateDragPreview(toolBar, globalMousePosition(event));
    }
    else {
        clearActiveZone();
        clearDragPreview();
    }

    return true;
}

bool CompactToolBarContainer::handleContainedToolBarMouseRelease(QToolBar* toolBar)
{
    const QRect globalRect(mapToGlobal(QPoint(0, 0)), size());
    if (!globalRect.contains(QCursor::pos())) {
        clearActiveZone();
        clearDragPreview();
        QPointer<QToolBar> guardedToolBar(toolBar);
        QTimer::singleShot(0, this, [this, guardedToolBar]() {
            if (guardedToolBar) {
                restoreToolBarToMainWindow(guardedToolBar);
            }
        });
        return true;
    }

    const auto zone = zoneAtGlobalPos(QCursor::pos());
    const int index = insertionIndexForZone(zone, QCursor::pos(), toolBar);
    removeToolBarFromLayouts(toolBar);
    containedToolBars.insert(toolBar, zone);
    insertToolBarToZoneLayout(toolBar, zone, index);
    persistToolBarZone(toolBar, zone);
    updateLabels();
    clearActiveZone();
    clearDragPreview();
    return true;
}

void CompactToolBarContainer::restoreToolBarToMainWindow(QToolBar* toolBar)
{
    if (!mainWindow || !toolBar || !containsToolBar(toolBar)) {
        return;
    }

    removeToolBarFromLayouts(toolBar);
    removeContainedToolBarFilter(toolBar);
    containedToolBars.remove(toolBar);
    toolBar->setProperty("_fc_compact_toolbar_hosted", false);
    removeStoredToolBar(toolBar);
    auto wrapper = toolBarWrappers.take(toolBar);
    if (wrapper) {
        toolBar->setParent(nullptr);
        wrapper->deleteLater();
    }
    toolBar->setParent(mainWindow);
    mainWindow->addToolBar(Qt::TopToolBarArea, toolBar);
    toolBar->setFloatable(true);
    toolBar->setMovable(!toolBarsLocked());
    toolBar->setVisible(true);
    updateLabels();
    QTimer::singleShot(0, this, &CompactToolBarContainer::installToolBarFilters);
}

void CompactToolBarContainer::detachToolBarsForShutdown()
{
    const auto toolBars = containedToolBars.keys();
    for (auto toolBar : toolBars) {
        if (!toolBar) {
            continue;
        }

        removeToolBarFromLayouts(toolBar);
        removeContainedToolBarFilter(toolBar);
        toolBar->setProperty("_fc_compact_toolbar_hosted", false);
        if (auto wrapper = toolBarWrappers.take(toolBar)) {
            toolBar->setParent(nullptr);
            wrapper->deleteLater();
        }
        if (mainWindow) {
            toolBar->setParent(mainWindow);
        }
    }

    containedToolBars.clear();
    toolBarWrappers.clear();
    dragStartPositions.clear();
    filteredToolBars.clear();
    destroyTrackedToolBars.clear();
}

void CompactToolBarContainer::persistToolBarZone(QToolBar* toolBar, Zone zone)
{
    const auto key = parameterKey(toolBar);
    if (key.isEmpty()) {
        return;
    }

    auto group = App::GetApplication().GetParameterGroupByPath(ParameterGroup);
    group->SetInt(key.constData(), zoneToInt(zone));
}

void CompactToolBarContainer::removeStoredToolBar(QToolBar* toolBar)
{
    const auto key = parameterKey(toolBar);
    if (key.isEmpty()) {
        return;
    }

    auto group = App::GetApplication().GetParameterGroupByPath(ParameterGroup);
    group->RemoveInt(key.constData());
}

QByteArray CompactToolBarContainer::parameterKey(const QToolBar* toolBar) const
{
    if (!toolBar || toolBar->objectName().isEmpty()
        || toolBar->objectName().startsWith(QStringLiteral("*"))) {
        return {};
    }

    return toolBar->objectName().toUtf8();
}

void CompactToolBarContainer::installContainedToolBarFilter(QToolBar* toolBar)
{
    if (!toolBar) {
        return;
    }

    toolBar->installEventFilter(this);
    if (auto wrapper = toolBarWrappers.value(toolBar, nullptr)) {
        wrapper->installEventFilter(this);
        const auto handles = wrapper->findChildren<QWidget*>(
            QStringLiteral("_fc_compact_toolbar_handle")
        );
        for (auto handle : handles) {
            handle->installEventFilter(this);
        }
    }
    trackToolBarDestroyed(toolBar);
}

void CompactToolBarContainer::trackToolBarDestroyed(QToolBar* toolBar)
{
    if (!toolBar || destroyTrackedToolBars.contains(toolBar)) {
        return;
    }

    destroyTrackedToolBars.insert(toolBar);
    connect(toolBar, &QObject::destroyed, this, [this, toolBar]() {
        filteredToolBars.remove(toolBar);
        containedToolBars.remove(toolBar);
        toolBarWrappers.remove(toolBar);
        dragStartPositions.remove(toolBar);
        destroyTrackedToolBars.remove(toolBar);
    });
}

void CompactToolBarContainer::removeContainedToolBarFilter(QToolBar* toolBar)
{
    if (toolBar) {
        toolBar->removeEventFilter(this);
        if (auto wrapper = toolBarWrappers.value(toolBar, nullptr)) {
            wrapper->removeEventFilter(this);
            const auto handles = wrapper->findChildren<QWidget*>(
                QStringLiteral("_fc_compact_toolbar_handle")
            );
            for (auto handle : handles) {
                handle->removeEventFilter(this);
            }
        }
    }
    dragStartPositions.remove(toolBar);
}

QWidget* CompactToolBarContainer::createHostedToolBarWrapper(QToolBar* toolBar)
{
    if (!toolBar) {
        return nullptr;
    }

    if (auto wrapper = toolBarWrappers.value(toolBar, nullptr)) {
        return wrapper;
    }

    auto wrapper = new QWidget(this);
    wrapper->setObjectName(QStringLiteral("_fc_compact_toolbar_wrapper"));
    wrapper->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto layout = new QBoxLayout(QBoxLayout::LeftToRight, wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto handle = new CompactToolBarHandle(toolBar, wrapper);
    layout->addWidget(handle);
    layout->addWidget(toolBar);
    toolBarWrappers.insert(toolBar, wrapper);
    return wrapper;
}

QWidget* CompactToolBarContainer::hostedToolBarWidget(QToolBar* toolBar) const
{
    return toolBarWrappers.value(toolBar, toolBar);
}

QToolBar* CompactToolBarContainer::watchedContainedToolBar(QObject* watched) const
{
    auto widget = qobject_cast<QWidget*>(watched);
    if (!widget) {
        return nullptr;
    }

    auto toolBar = qobject_cast<QToolBar*>(widget);
    if (toolBar && containsToolBar(toolBar)) {
        return toolBar;
    }

    for (auto it = toolBarWrappers.cbegin(); it != toolBarWrappers.cend(); ++it) {
        auto wrapper = it.value();
        if (wrapper && (widget == wrapper || wrapper->isAncestorOf(widget))) {
            return it.key();
        }
    }

    toolBar = qobject_cast<QToolBar*>(widget->parentWidget());
    return toolBar && containsToolBar(toolBar) ? toolBar : nullptr;
}

void CompactToolBarContainer::removeToolBarFromLayouts(QToolBar* toolBar)
{
    auto widget = hostedToolBarWidget(toolBar);
    for (auto layout : {leftLayout, centerLayout, rightLayout}) {
        if (layout && layout->indexOf(widget) >= 0) {
            layout->removeWidget(widget);
        }
    }
}

bool CompactToolBarContainer::containsToolBar(const QToolBar* toolBar) const
{
    return containedToolBars.contains(const_cast<QToolBar*>(toolBar));
}
