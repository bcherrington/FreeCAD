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

#include "PanelPlacementManager.h"

#include <algorithm>
#include <utility>
#include <vector>

#include <QApplication>
#include <QDockWidget>
#include <QLayout>
#include <QScopedValueRollback>
#include <QTimer>
#include <QWidget>

#include "Application.h"
#include "MainWindow.h"
#include "OverlayManager.h"

using namespace Gui;

namespace
{

constexpr int CompactDefaultOverlayExtent = 300;

Qt::DockWidgetArea dockAreaForEdge(PanelPlacement::Edge edge)
{
    switch (edge) {
        case PanelPlacement::Edge::Left:
            return Qt::LeftDockWidgetArea;
        case PanelPlacement::Edge::Right:
            return Qt::RightDockWidgetArea;
        case PanelPlacement::Edge::Top:
            return Qt::TopDockWidgetArea;
        case PanelPlacement::Edge::Bottom:
            return Qt::BottomDockWidgetArea;
        case PanelPlacement::Edge::None:
            return Qt::NoDockWidgetArea;
    }

    return Qt::NoDockWidgetArea;
}

PanelPlacement::Edge edgeForDockArea(Qt::DockWidgetArea area, PanelPlacement::Edge fallback)
{
    switch (area) {
        case Qt::LeftDockWidgetArea:
            return PanelPlacement::Edge::Left;
        case Qt::RightDockWidgetArea:
            return PanelPlacement::Edge::Right;
        case Qt::TopDockWidgetArea:
            return PanelPlacement::Edge::Top;
        case Qt::BottomDockWidgetArea:
            return PanelPlacement::Edge::Bottom;
        default:
            return fallback;
    }
}

int normalizedInsertionOrder(int requestedOrder, int count)
{
    return std::clamp(requestedOrder, 0, std::max(count, 0));
}

PanelPlacement mergedRuntimePlacement(const PanelPlacement& runtime, const PanelPlacement& placement)
{
    PanelPlacement merged = runtime;
    merged.panelId = placement.panelId;
    merged.mode = placement.mode;
    merged.edge = placement.edge;
    merged.region = placement.region;
    merged.order = placement.order;
    merged.visibilityPolicy = placement.visibilityPolicy;
    merged.groupId = placement.groupId;
    merged.groupOrder = placement.groupOrder;
    merged.tabOrder = placement.tabOrder;
    merged.splitRelation = placement.splitRelation;
    merged.extent = placement.extent;
    merged.launcher = placement.launcher;
    if (placement.mode != PanelPlacement::Mode::Floating) {
        merged.floatingGeometry = QRect();
    }
    merged.normalize();
    return merged;
}

PanelPlacementManager::RequestError requestErrorForHostError(PanelPlacementHost::Error error)
{
    switch (error) {
        case PanelPlacementHost::Error::Unsupported:
            return PanelPlacementManager::RequestError::UnsupportedPlacement;
        case PanelPlacementHost::Error::InvalidTarget:
        case PanelPlacementHost::Error::ApplyFailed:
            return PanelPlacementManager::RequestError::HostFailure;
        case PanelPlacementHost::Error::None:
            return PanelPlacementManager::RequestError::HostFailure;
    }

    return PanelPlacementManager::RequestError::HostFailure;
}

QStringList insertionOrderedIds(QStringList ids, const QString& panelId, int requestedOrder)
{
    ids.removeAll(panelId);
    ids.insert(normalizedInsertionOrder(requestedOrder, ids.size()), panelId);
    return ids;
}

bool panelIsShown(const QDockWidget* dockWidget)
{
    // isVisible() also depends on ancestor visibility. During startup the main
    // window is still hidden, so use the dock's explicit show/hide state.
    return dockWidget && !dockWidget->isHidden();
}

bool savePlacementMap(const QMap<QString, PanelPlacement>& placements, QString* failedPanelId = nullptr)
{
    for (auto it = placements.cbegin(); it != placements.cend(); ++it) {
        if (!PanelPlacementStore::savePlacement(it.value())) {
            if (failedPanelId) {
                *failedPanelId = it.key();
            }
            return false;
        }
    }

    return true;
}

bool applyVisibilityTransaction(
    PanelPlacementHost* host,
    MainWindow* mainWindow,
    const QMap<QString, QPointer<QDockWidget>>& docks,
    const QMap<QString, PanelPlacement>& placements,
    const QStringList& executionOrder,
    const QMap<QString, bool>& previousVisibility,
    const QMap<QString, bool>& desiredVisibility,
    PanelPlacementManager::RequestError* error,
    QString* message
)
{
    if (!host) {
        if (error) {
            *error = PanelPlacementManager::RequestError::HostFailure;
        }
        if (message) {
            *message = QStringLiteral("Panel placement host is unavailable.");
        }
        return false;
    }

    QStringList mutatedIds;
    for (const auto& panelId : executionOrder) {
        const bool before = previousVisibility.value(panelId, false);
        const bool after = desiredVisibility.value(panelId, before);
        if (before == after) {
            continue;
        }

        QDockWidget* dockWidget = docks.value(panelId).data();
        if (!dockWidget) {
            for (auto it = mutatedIds.crbegin(); it != mutatedIds.crend(); ++it) {
                host->applyVisibility(
                    mainWindow,
                    docks.value(*it).data(),
                    placements.value(*it),
                    previousVisibility.value(*it, false)
                );
            }
            if (error) {
                *error = PanelPlacementManager::RequestError::InvalidDock;
            }
            if (message) {
                *message = QStringLiteral("Dock widget is unavailable.");
            }
            return false;
        }

        const PanelPlacementHost::Result result
            = host->applyVisibility(mainWindow, dockWidget, placements.value(panelId), after);
        if (!result.success) {
            bool rollbackFailed = false;
            QString rollbackMessage;
            for (auto it = mutatedIds.crbegin(); it != mutatedIds.crend(); ++it) {
                const PanelPlacementHost::Result rollbackResult = host->applyVisibility(
                    mainWindow,
                    docks.value(*it).data(),
                    placements.value(*it),
                    previousVisibility.value(*it, false)
                );
                if (!rollbackResult.success && rollbackMessage.isEmpty()) {
                    rollbackFailed = true;
                    rollbackMessage = rollbackResult.message;
                }
            }

            if (rollbackFailed) {
                if (error) {
                    *error = PanelPlacementManager::RequestError::RollbackFailed;
                }
                if (message) {
                    *message = rollbackMessage.isEmpty()
                        ? QStringLiteral("Panel visibility rollback failed.")
                        : rollbackMessage;
                }
            }
            else {
                if (error) {
                    *error = requestErrorForHostError(result.error);
                }
                if (message) {
                    *message = result.message.isEmpty()
                        ? QStringLiteral("Panel visibility request failed.")
                        : result.message;
                }
            }
            return false;
        }

        if (result.mutated) {
            mutatedIds.push_back(panelId);
        }
    }

    return true;
}

class DefaultPanelPlacementHost final: public PanelPlacementHost
{
public:
    bool queryVisibility(
        MainWindow*,
        QDockWidget* dockWidget,
        const PanelPlacement& placement,
        bool* visible
    ) const override
    {
        if (!dockWidget || !visible) {
            return false;
        }
        if (placement.mode != PanelPlacement::Mode::Overlay
            && placement.mode != PanelPlacement::Mode::AutoHide) {
            return false;
        }

        *visible = OverlayManager::instance()->dockWidgetOverlayVisible(dockWidget);
        return true;
    }

    bool queryPlacement(MainWindow* mainWindow, QDockWidget* dockWidget, PanelPlacement* placement) const override
    {
        if (!mainWindow || !Gui::Application::Instance || !dockWidget || !placement) {
            return false;
        }

        OverlayManager* overlayManager = OverlayManager::instance();
        const Qt::DockWidgetArea overlayArea = overlayManager->dockWidgetOverlayArea(dockWidget);
        if (overlayArea == Qt::NoDockWidgetArea) {
            return false;
        }

        placement->mode = overlayManager->dockWidgetOverlayAutoHide(dockWidget)
            ? PanelPlacement::Mode::AutoHide
            : PanelPlacement::Mode::Overlay;
        placement->edge = edgeForDockArea(overlayArea, PanelPlacement::Edge::Left);
        placement->extent = overlayManager->dockWidgetOverlayExtent(dockWidget);
        placement->floatingGeometry = QRect();
        placement->normalize();
        return true;
    }

    Result applyPlacement(
        MainWindow* mainWindow,
        QDockWidget* dockWidget,
        const PanelPlacement& placement
    ) override
    {
        if (!dockWidget) {
            Result result;
            result.error = Error::InvalidTarget;
            result.message = QStringLiteral("Dock widget is unavailable.");
            return result;
        }

        if (!mainWindow) {
            Result result;
            result.error = Error::InvalidTarget;
            result.message = QStringLiteral("Main window is unavailable.");
            return result;
        }

        OverlayManager* overlayManager = OverlayManager::instance();
        if (placement.mode == PanelPlacement::Mode::Overlay
            || placement.mode == PanelPlacement::Mode::AutoHide) {
            const Qt::DockWidgetArea area = dockAreaForEdge(placement.edge);
            if (area == Qt::NoDockWidgetArea) {
                Result result;
                result.error = Error::InvalidTarget;
                result.message = QStringLiteral("Overlay placement requires a valid edge.");
                return result;
            }

            const Qt::DockWidgetArea previousArea = overlayManager->dockWidgetOverlayArea(dockWidget);
            const bool previousAutoHide = overlayManager->dockWidgetOverlayAutoHide(dockWidget);
            const int previousExtent = overlayManager->dockWidgetOverlayExtent(dockWidget);
            Result result;
            result.success = overlayManager->moveDockWidgetToOverlayOnly(dockWidget, area);
            if (!result.success) {
                result.error = Error::ApplyFailed;
                result.message = QStringLiteral("The overlay host rejected the panel.");
                return result;
            }
            if (!overlayManager->moveDockWidgetInOverlay(dockWidget, placement.order)) {
                result.success = false;
                result.mutated = true;
                result.error = Error::ApplyFailed;
                result.message = QStringLiteral("The overlay host rejected the panel order.");
                return result;
            }

            const bool autoHideEnabled = placement.mode == PanelPlacement::Mode::AutoHide;
            if (!overlayManager->setDockWidgetOverlayAutoHide(dockWidget, autoHideEnabled)) {
                result.success = false;
                result.mutated = true;
                result.error = Error::Unsupported;
                result.message = QStringLiteral("The overlay auto-hide host rejected the panel.");
                return result;
            }
            if (!overlayManager->setDockWidgetOverlayExtent(
                    dockWidget,
                    placement.extent > 0 ? placement.extent : CompactDefaultOverlayExtent
                )) {
                result.success = false;
                result.mutated = true;
                result.error = Error::ApplyFailed;
                result.message = QStringLiteral("The overlay host rejected the panel extent.");
                return result;
            }

            const int requestedExtent = placement.extent > 0 ? placement.extent
                                                             : CompactDefaultOverlayExtent;
            result.mutated = previousArea != area || previousAutoHide != autoHideEnabled
                || previousExtent != requestedExtent;
            return result;
        }

        overlayManager->setDockWidgetOverlayAutoHide(dockWidget, false);
        overlayManager->unsetupDockWidget(dockWidget);

        if (placement.mode == PanelPlacement::Mode::Docked) {
            const Qt::DockWidgetArea area = dockAreaForEdge(placement.edge);
            if (area == Qt::NoDockWidgetArea) {
                Result result;
                result.error = Error::InvalidTarget;
                result.message = QStringLiteral("Docked placement requires a valid edge.");
                return result;
            }

            if (dockWidget->isFloating()) {
                dockWidget->setFloating(false);
            }
            mainWindow->addDockWidget(area, dockWidget);
            Result result;
            result.success = true;
            result.mutated = true;
            return result;
        }

        const Qt::DockWidgetArea fallbackArea = dockAreaForEdge(
            placement.edge == PanelPlacement::Edge::None ? PanelPlacement::Edge::Left : placement.edge
        );
        if (fallbackArea != Qt::NoDockWidgetArea
            && mainWindow->dockWidgetArea(dockWidget) == Qt::NoDockWidgetArea) {
            mainWindow->addDockWidget(fallbackArea, dockWidget);
        }

        dockWidget->setFloating(true);
        if (placement.floatingGeometry.isValid() && !placement.floatingGeometry.isEmpty()) {
            dockWidget->setGeometry(placement.floatingGeometry);
        }
        Result result;
        result.success = true;
        result.mutated = true;
        return result;
    }

    Result applyVisibility(
        MainWindow* mainWindow,
        QDockWidget* dockWidget,
        const PanelPlacement& placement,
        bool visible
    ) override
    {
        Result result;
        if (!dockWidget) {
            result.error = Error::InvalidTarget;
            result.message = QStringLiteral("Dock widget is unavailable.");
            return result;
        }

        bool previousVisibility = false;
        if (!queryVisibility(mainWindow, dockWidget, placement, &previousVisibility)) {
            previousVisibility = panelIsShown(dockWidget);
        }
        if (previousVisibility == visible) {
            result.success = true;
            return result;
        }

        QAction* toggleAction = dockWidget->toggleViewAction();
        if (!toggleAction || !toggleAction->isEnabled()) {
            result.error = Error::ApplyFailed;
            result.message = QStringLiteral("The dock toggle action is unavailable.");
            return result;
        }

        // Use the same activation path as the View menu and compact rail. OverlayManager is
        // already connected to this QAction and remains responsible for the surface itself.
        toggleAction->trigger();

        bool resultingVisibility = false;
        if (!queryVisibility(mainWindow, dockWidget, placement, &resultingVisibility)) {
            resultingVisibility = panelIsShown(dockWidget);
        }
        result.success = resultingVisibility == visible;
        result.mutated = previousVisibility != resultingVisibility;
        if (!result.success) {
            result.error = Error::ApplyFailed;
            result.message = QStringLiteral(
                "The dock toggle action did not reach the requested state."
            );
        }
        return result;
    }

    Result applyAreaOrder(
        MainWindow* mainWindow,
        const QList<QDockWidget*>& dockWidgets,
        const PanelPlacement& area
    ) override
    {
        Result result;
        result.success = true;
        if (dockWidgets.size() < 2) {
            return result;
        }

        if (area.mode == PanelPlacement::Mode::Overlay
            || area.mode == PanelPlacement::Mode::AutoHide) {
            OverlayManager* overlayManager = OverlayManager::instance();
            for (int index = 0; index < dockWidgets.size(); ++index) {
                if (overlayManager->dockWidgetOverlayArea(dockWidgets.at(index))
                        != dockAreaForEdge(area.edge)
                    || !overlayManager->moveDockWidgetInOverlay(dockWidgets.at(index), index)) {
                    result.success = false;
                    result.error = Error::ApplyFailed;
                    result.message = QStringLiteral("The overlay host rejected the area order.");
                    return result;
                }
            }
            result.mutated = true;
            return result;
        }

        if (area.mode == PanelPlacement::Mode::Docked && mainWindow) {
            const Qt::DockWidgetArea dockArea = dockAreaForEdge(area.edge);
            const Qt::Orientation orientation = area.edge == PanelPlacement::Edge::Left
                    || area.edge == PanelPlacement::Edge::Right
                ? Qt::Vertical
                : Qt::Horizontal;
            for (QDockWidget* dock : dockWidgets) {
                if (!dock || dockArea == Qt::NoDockWidgetArea) {
                    result.success = false;
                    result.error = Error::InvalidTarget;
                    result.message = QStringLiteral("The docked area order is invalid.");
                    return result;
                }
            }

            QLayout* windowLayout = mainWindow->layout();
            const bool layoutWasEnabled = windowLayout && windowLayout->isEnabled();
            if (layoutWasEnabled) {
                windowLayout->setEnabled(false);
            }
            for (QDockWidget* dock : dockWidgets) {
                if (dock->isFloating()) {
                    dock->setFloating(false);
                }
                mainWindow->addDockWidget(dockArea, dock);
            }
            for (int index = 1; index < dockWidgets.size(); ++index) {
                mainWindow->splitDockWidget(dockWidgets.at(index - 1), dockWidgets.at(index), orientation);
            }
            if (layoutWasEnabled) {
                windowLayout->setEnabled(true);
                windowLayout->invalidate();
                windowLayout->activate();
            }
            result.mutated = true;
        }
        return result;
    }
};

}  // namespace

PanelPlacementHost::Result PanelPlacementHost::applyVisibility(
    MainWindow*,
    QDockWidget* dockWidget,
    const PanelPlacement& placement,
    bool visible
)
{
    if (!dockWidget) {
        Result result;
        result.error = Error::InvalidTarget;
        result.message = QStringLiteral("Dock widget is unavailable.");
        return result;
    }

    if (placement.mode == PanelPlacement::Mode::AutoHide) {
        Result result;
        result.error = Error::Unsupported;
        result.message = QStringLiteral("The auto-hide visibility host is not installed.");
        return result;
    }

    const bool previousVisibility = panelIsShown(dockWidget);
    if (visible) {
        dockWidget->show();
        dockWidget->raise();
    }
    else {
        dockWidget->hide();
    }

    Result result;
    result.success = true;
    result.mutated = previousVisibility != visible;
    return result;
}

PanelPlacementHost::Result PanelPlacementHost::applyAreaOrder(
    MainWindow*,
    const QList<QDockWidget*>&,
    const PanelPlacement&
)
{
    Result result;
    result.success = true;
    return result;
}

bool PanelPlacementHost::queryPlacement(MainWindow*, QDockWidget*, PanelPlacement*) const
{
    return false;
}

bool PanelPlacementHost::queryVisibility(
    MainWindow*,
    QDockWidget* dockWidget,
    const PanelPlacement&,
    bool* visible
) const
{
    if (!dockWidget || !visible) {
        return false;
    }
    *visible = panelIsShown(dockWidget);
    return true;
}

PanelPlacementManager::PanelPlacementManager(MainWindow* mainWindow, QObject* parent)
    : PanelPlacementManager(mainWindow, createDefaultHost(), parent)
{}

PanelPlacementManager::PanelPlacementManager(
    MainWindow* mainWindow,
    std::unique_ptr<PanelPlacementHost> host,
    QObject* parent
)
    : QObject(parent)
    , mainWindow(mainWindow)
    , placementHost(std::move(host))
{
    if (!placementHost) {
        placementHost = createDefaultHost();
    }
}

PanelPlacementManager::~PanelPlacementManager() = default;

std::unique_ptr<PanelPlacementHost> PanelPlacementManager::createDefaultHost()
{
    return std::make_unique<DefaultPanelPlacementHost>();
}

bool PanelPlacementManager::isFeatureEnabled() const
{
    return PanelPlacementStore::isFeatureEnabled();
}

bool PanelPlacementManager::isActive() const
{
    return active;
}

bool PanelPlacementManager::isEnabled() const
{
    return active && isFeatureEnabled();
}

void PanelPlacementManager::setActive(bool enabled)
{
    if (active == enabled) {
        return;
    }

    active = enabled;
    Q_EMIT activeChanged(active);
}

bool PanelPlacementManager::registerPanel(
    const QString& panelId,
    QDockWidget* dockWidget,
    const PanelPlacement& fallbackPlacement,
    QString* errorMessage
)
{
    if (!isStablePanelId(panelId)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Panel ID is not stable.");
        }
        return false;
    }

    if (!dockWidget) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Dock widget is unavailable.");
        }
        return false;
    }

    if (registrations.contains(panelId)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Panel is already registered.");
        }
        return false;
    }

    for (const auto& registration : std::as_const(registrations)) {
        if (registration.dockWidget == dockWidget) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Dock widget is already registered.");
            }
            return false;
        }
    }

    Registration registration;
    registration.panelId = panelId;
    registration.dockWidget = dockWidget;
    const PanelPlacement normalizedFallback = normalizePlacement(panelId, fallbackPlacement);
    const bool hasPersistedPlacement = PanelPlacementStore::hasPlacement(panelId);
    registration.persistedPlacement = PanelPlacementStore::loadPlacement(panelId, normalizedFallback);
    registration.runtimePlacement
        = snapshotRuntimePlacement(mainWindow, dockWidget, registration.persistedPlacement);
    if (!hasPersistedPlacement) {
        registration.runtimePlacement.launcher = registration.persistedPlacement.launcher;
        registration.runtimePlacement.normalize();
        registration.persistedPlacement = registration.runtimePlacement;
    }
    registration.destroyedConnection = connect(dockWidget, &QObject::destroyed, this, [this, panelId]() {
        unregisterPanel(panelId);
    });
    if (QAction* toggleAction = dockWidget->toggleViewAction()) {
        registration.toggleConnection
            = connect(toggleAction, &QAction::triggered, this, [this, panelId]() {
                  // QAction is the shared View-menu/rail source of truth. Let QDockWidget and the
                  // overlay host consume it first, then enforce the area's visibility policy.
                  QTimer::singleShot(0, this, [this, panelId]() {
                      Registration* current = findRegistration(panelId);
                      if (!current || current->transitionInProgress || !isEnabled()) {
                          return;
                      }
                      requestVisibility(panelId, isPanelVisible(panelId));
                  });
              });
    }

    registrations.insert(panelId, registration);
    if (active && isFeatureEnabled() && isPanelVisible(panelId)
        && supportsArea(registration.persistedPlacement)
        && registration.persistedPlacement.visibilityPolicy
            == PanelPlacement::VisibilityPolicy::Exclusive) {
        QString survivorId;
        for (const QString& orderedId : orderedPanelIdsInternal(registration.persistedPlacement)) {
            const Registration* orderedRegistration = findRegistration(orderedId);
            if (orderedRegistration && isPanelVisible(orderedId)) {
                survivorId = orderedId;
                break;
            }
        }
        if (!survivorId.isEmpty()) {
            requestVisibility(survivorId, true);
        }
    }
    Q_EMIT panelRegistered(panelId);
    return true;
}

bool PanelPlacementManager::unregisterPanel(const QString& panelId)
{
    auto it = registrations.find(panelId);
    if (it == registrations.end()) {
        return false;
    }

    if (it->destroyedConnection) {
        disconnect(it->destroyedConnection);
    }
    if (it->toggleConnection) {
        disconnect(it->toggleConnection);
    }

    registrations.erase(it);
    Q_EMIT panelUnregistered(panelId);
    return true;
}

bool PanelPlacementManager::isRegistered(const QString& panelId) const
{
    return registrations.contains(panelId);
}

QStringList PanelPlacementManager::registeredPanelIds() const
{
    return registrations.keys();
}

QDockWidget* PanelPlacementManager::dockWidget(const QString& panelId) const
{
    const Registration* registration = findRegistration(panelId);
    return registration ? registration->dockWidget.data() : nullptr;
}

PanelPlacement PanelPlacementManager::persistedPlacement(const QString& panelId) const
{
    const Registration* registration = findRegistration(panelId);
    if (registration) {
        return registration->persistedPlacement;
    }

    PanelPlacement placement;
    placement.panelId = panelId;
    placement.normalize();
    return placement;
}

PanelPlacement PanelPlacementManager::runtimePlacement(const QString& panelId) const
{
    const Registration* registration = findRegistration(panelId);
    if (registration) {
        return registration->runtimePlacement;
    }

    PanelPlacement placement;
    placement.panelId = panelId;
    placement.normalize();
    return placement;
}

bool PanelPlacementManager::isPanelVisible(const QString& panelId) const
{
    const Registration* registration = findRegistration(panelId);
    if (!registration || !registration->dockWidget) {
        return false;
    }

    bool visible = false;
    if (placementHost
        && placementHost->queryVisibility(
            mainWindow,
            registration->dockWidget,
            registration->runtimePlacement,
            &visible
        )) {
        return visible;
    }
    return panelIsShown(registration->dockWidget);
}

PanelPlacementManager::RequestResult PanelPlacementManager::requestPlacement(
    const QString& panelId,
    const PanelPlacement& placement
)
{
    if (!isFeatureEnabled()) {
        return makeUnavailableResult(
            panelId,
            RequestError::FeatureDisabled,
            QStringLiteral("Panel placement feature is disabled.")
        );
    }

    if (!active) {
        return makeUnavailableResult(
            panelId,
            RequestError::Inactive,
            QStringLiteral("Panel placement manager is inactive.")
        );
    }

    Registration* registration = findRegistration(panelId);
    if (!registration) {
        return makeUnavailableResult(
            panelId,
            RequestError::UnknownPanel,
            QStringLiteral("Panel is not registered.")
        );
    }

    if (!registration->dockWidget) {
        return makeUnavailableResult(
            panelId,
            RequestError::InvalidDock,
            QStringLiteral("Dock widget is unavailable.")
        );
    }

    if (registration->transitionInProgress) {
        return makeEntryResult(
            *registration,
            false,
            RequestError::TransitionInProgress,
            QStringLiteral("Panel transition is already in progress.")
        );
    }

    if (!placementHost) {
        return makeEntryResult(
            *registration,
            false,
            RequestError::HostFailure,
            QStringLiteral("Panel placement host is unavailable.")
        );
    }

    const QScopedValueRollback<bool> transitionGuard(registration->transitionInProgress);
    registration->transitionInProgress = true;

    QDockWidget* targetDock = registration->dockWidget.data();
    QPointer<QWidget> previousFocus = QApplication::focusWidget();
    const PanelPlacement previousTargetRuntime
        = snapshotRuntimePlacement(mainWindow, targetDock, registration->runtimePlacement);
    const PanelPlacement targetPlacement = normalizePlacement(panelId, placement);

    QStringList sourceOrder;
    QStringList targetOrder;
    const bool sourceHasArea = supportsArea(registration->persistedPlacement);
    const bool targetHasArea = supportsArea(targetPlacement);
    const bool sameArea = sourceHasArea && targetHasArea
        && isSameArea(registration->persistedPlacement, targetPlacement);

    if (sourceHasArea) {
        sourceOrder = orderedPanelIdsInternal(registration->persistedPlacement);
    }
    if (targetHasArea) {
        targetOrder = sameArea ? sourceOrder : orderedPanelIdsInternal(targetPlacement, panelId);
    }
    const QStringList previousTargetOrder = targetOrder;

    QStringList affectedPanelIds;
    if (sourceHasArea) {
        affectedPanelIds.append(sourceOrder);
    }
    if (targetHasArea) {
        affectedPanelIds.append(targetOrder);
    }
    affectedPanelIds.append(panelId);
    affectedPanelIds.removeDuplicates();

    QMap<QString, PanelPlacement> previousPersisted;
    QMap<QString, PanelPlacement> previousRuntime;
    QMap<QString, bool> previousVisibility;
    QMap<QString, QPointer<QDockWidget>> docks;
    for (const auto& affectedId : affectedPanelIds) {
        const Registration* affectedRegistration = findRegistration(affectedId);
        if (!affectedRegistration || !affectedRegistration->dockWidget) {
            return makeEntryResult(
                *registration,
                false,
                RequestError::InvalidDock,
                QStringLiteral("Dock widget is unavailable.")
            );
        }

        previousPersisted.insert(affectedId, affectedRegistration->persistedPlacement);
        previousRuntime.insert(
            affectedId,
            snapshotRuntimePlacement(
                mainWindow,
                affectedRegistration->dockWidget.data(),
                affectedRegistration->runtimePlacement
            )
        );
        previousVisibility.insert(affectedId, isPanelVisible(affectedId));
        docks.insert(affectedId, affectedRegistration->dockWidget);
    }

    QMap<QString, PanelPlacement> desiredPersisted = previousPersisted;
    auto assignAreaPlacements = [&](const QStringList& orderedIds,
                                    const PanelPlacement& areaPlacement,
                                    PanelPlacement::VisibilityPolicy policy) {
        for (int index = 0; index < orderedIds.size(); ++index) {
            const QString& orderedId = orderedIds.at(index);
            PanelPlacement updated = orderedId == panelId ? targetPlacement
                                                          : previousPersisted.value(orderedId);
            updated.mode = areaPlacement.mode;
            updated.edge = areaPlacement.edge;
            updated.region = areaPlacement.region;
            updated.order = index;
            updated.visibilityPolicy = policy;
            updated.normalize();
            desiredPersisted.insert(orderedId, updated);
        }
    };

    if (sameArea) {
        targetOrder = insertionOrderedIds(sourceOrder, panelId, targetPlacement.order);
        assignAreaPlacements(targetOrder, targetPlacement, targetPlacement.visibilityPolicy);
    }
    else {
        if (sourceHasArea) {
            sourceOrder.removeAll(panelId);
            if (!sourceOrder.isEmpty()) {
                assignAreaPlacements(
                    sourceOrder,
                    registration->persistedPlacement,
                    resolvedAreaPolicy(sourceOrder, registration->persistedPlacement.visibilityPolicy)
                );
            }
        }

        desiredPersisted.insert(panelId, targetPlacement);
        if (targetHasArea) {
            targetOrder = insertionOrderedIds(targetOrder, panelId, targetPlacement.order);
            assignAreaPlacements(targetOrder, targetPlacement, targetPlacement.visibilityPolicy);
        }
    }

    const PanelPlacementHost::Result applyResult
        = placementHost->applyPlacement(mainWindow, targetDock, targetPlacement);
    if (!applyResult.success) {
        if (applyResult.mutated) {
            const PanelPlacementHost::Result rollbackResult
                = placementHost->applyPlacement(mainWindow, targetDock, previousTargetRuntime);
            restoreFocus(previousFocus.data());
            if (!rollbackResult.success) {
                registration->runtimePlacement = mergedRuntimePlacement(
                    snapshotRuntimePlacement(mainWindow, targetDock, targetPlacement),
                    targetPlacement
                );
                return makeEntryResult(
                    *registration,
                    false,
                    RequestError::RollbackFailed,
                    rollbackResult.message.isEmpty()
                        ? QStringLiteral("Panel placement rollback failed.")
                        : rollbackResult.message
                );
            }
            registration->runtimePlacement = previousTargetRuntime;
        }
        else {
            restoreFocus(previousFocus.data());
        }
        return makeEntryResult(
            *registration,
            false,
            requestErrorForHostError(applyResult.error),
            applyResult.message.isEmpty() ? QStringLiteral("Panel placement request failed.")
                                          : applyResult.message
        );
    }

    if (targetHasArea) {
        QList<QDockWidget*> orderedDocks;
        orderedDocks.reserve(targetOrder.size());
        for (const QString& orderedId : std::as_const(targetOrder)) {
            if (QDockWidget* orderedDock = docks.value(orderedId)) {
                orderedDocks.push_back(orderedDock);
            }
        }
        const PanelPlacementHost::Result orderResult = placementHost->applyAreaOrder(
            mainWindow,
            orderedDocks,
            desiredPersisted.value(panelId, targetPlacement)
        );
        if (!orderResult.success) {
            const PanelPlacementHost::Result rollbackResult
                = placementHost->applyPlacement(mainWindow, targetDock, previousTargetRuntime);
            restoreFocus(previousFocus.data());
            return makeEntryResult(
                *registration,
                false,
                rollbackResult.success ? requestErrorForHostError(orderResult.error)
                                       : RequestError::RollbackFailed,
                rollbackResult.success
                    ? (orderResult.message.isEmpty()
                           ? QStringLiteral("Panel surface order request failed.")
                           : orderResult.message)
                    : (rollbackResult.message.isEmpty()
                           ? QStringLiteral("Panel placement rollback failed after ordering error.")
                           : rollbackResult.message)
            );
        }
    }

    QString failedPanelId;
    if (!savePlacementMap(desiredPersisted, &failedPanelId)) {
        const PanelPlacementHost::Result rollbackResult
            = placementHost->applyPlacement(mainWindow, targetDock, previousTargetRuntime);
        if (sameArea && !previousTargetOrder.isEmpty()) {
            QList<QDockWidget*> previousOrderedDocks;
            previousOrderedDocks.reserve(previousTargetOrder.size());
            for (const QString& orderedId : previousTargetOrder) {
                if (QDockWidget* orderedDock = docks.value(orderedId)) {
                    previousOrderedDocks.push_back(orderedDock);
                }
            }
            placementHost
                ->applyAreaOrder(mainWindow, previousOrderedDocks, registration->persistedPlacement);
        }
        else if (sourceHasArea && !sourceOrder.isEmpty()) {
            QList<QDockWidget*> previousSourceDocks;
            previousSourceDocks.reserve(sourceOrder.size());
            for (const QString& orderedId : sourceOrder) {
                if (QDockWidget* orderedDock = docks.value(orderedId)) {
                    previousSourceDocks.push_back(orderedDock);
                }
            }
            placementHost
                ->applyAreaOrder(mainWindow, previousSourceDocks, registration->persistedPlacement);
        }
        savePlacementMap(previousPersisted);
        if (!rollbackResult.success) {
            registration->runtimePlacement = mergedRuntimePlacement(
                snapshotRuntimePlacement(mainWindow, targetDock, targetPlacement),
                desiredPersisted.value(panelId, targetPlacement)
            );
            return makeEntryResult(
                *registration,
                false,
                RequestError::RollbackFailed,
                rollbackResult.message.isEmpty()
                    ? QStringLiteral("Panel placement rollback failed after persistence error.")
                    : rollbackResult.message
            );
        }

        restoreFocus(previousFocus.data());
        return makeEntryResult(
            *registration,
            false,
            RequestError::PersistenceFailed,
            QStringLiteral("Panel placement could not be persisted.")
        );
    }

    QMap<QString, PanelPlacement> desiredRuntime = previousRuntime;
    for (auto it = desiredPersisted.cbegin(); it != desiredPersisted.cend(); ++it) {
        if (it.key() == panelId) {
            desiredRuntime.insert(
                it.key(),
                mergedRuntimePlacement(
                    snapshotRuntimePlacement(mainWindow, targetDock, it.value()),
                    it.value()
                )
            );
        }
        else if (previousRuntime.contains(it.key())) {
            desiredRuntime.insert(
                it.key(),
                mergedRuntimePlacement(previousRuntime.value(it.key()), it.value())
            );
        }
    }

    QMap<QString, bool> desiredVisibility = previousVisibility;
    if (targetHasArea
        && desiredPersisted.value(panelId).visibilityPolicy
            == PanelPlacement::VisibilityPolicy::Exclusive) {
        QString survivorId;
        if (previousVisibility.value(panelId, isPanelVisible(panelId))) {
            survivorId = panelId;
        }
        else {
            for (const auto& orderedId : targetOrder) {
                if (previousVisibility.value(orderedId, false)) {
                    survivorId = orderedId;
                    break;
                }
            }
        }

        for (const auto& orderedId : targetOrder) {
            desiredVisibility.insert(orderedId, !survivorId.isEmpty() && orderedId == survivorId);
        }
    }
    else {
        desiredVisibility.insert(panelId, previousVisibility.value(panelId, isPanelVisible(panelId)));
    }

    QStringList visibilityOrder = targetOrder;
    if (visibilityOrder.isEmpty()) {
        visibilityOrder.push_back(panelId);
    }
    visibilityOrder.removeAll(panelId);
    visibilityOrder.push_front(panelId);
    for (const auto& sourceId : sourceOrder) {
        if (!visibilityOrder.contains(sourceId)) {
            visibilityOrder.push_back(sourceId);
        }
    }

    RequestError visibilityError = RequestError::None;
    QString visibilityMessage;
    if (!applyVisibilityTransaction(
            placementHost.get(),
            mainWindow,
            docks,
            desiredRuntime,
            visibilityOrder,
            previousVisibility,
            desiredVisibility,
            &visibilityError,
            &visibilityMessage
        )) {
        const PanelPlacementHost::Result rollbackResult
            = placementHost->applyPlacement(mainWindow, targetDock, previousTargetRuntime);
        savePlacementMap(previousPersisted);

        for (auto it = previousPersisted.cbegin(); it != previousPersisted.cend(); ++it) {
            if (Registration* previousRegistration = findRegistration(it.key())) {
                previousRegistration->persistedPlacement = it.value();
                previousRegistration->runtimePlacement = previousRuntime.value(it.key(), it.value());
            }
        }

        restoreFocus(previousFocus.data());
        if (!rollbackResult.success) {
            return makeEntryResult(
                *registration,
                false,
                RequestError::RollbackFailed,
                rollbackResult.message.isEmpty()
                    ? QStringLiteral("Panel placement rollback failed after visibility error.")
                    : rollbackResult.message
            );
        }
        return makeEntryResult(*registration, false, visibilityError, visibilityMessage);
    }

    QStringList changedPanelIds;
    for (auto it = desiredPersisted.cbegin(); it != desiredPersisted.cend(); ++it) {
        Registration* changedRegistration = findRegistration(it.key());
        if (!changedRegistration) {
            continue;
        }
        changedRegistration->persistedPlacement = it.value();
        changedRegistration->runtimePlacement = desiredRuntime.value(it.key(), it.value());
        changedPanelIds.push_back(it.key());
    }

    restoreFocus(previousFocus.data());
    changedPanelIds.removeDuplicates();
    for (const auto& changedPanelId : changedPanelIds) {
        Q_EMIT placementChanged(changedPanelId);
    }

    return makeEntryResult(*registration, true, RequestError::None, QString());
}

PanelPlacementManager::RequestResult PanelPlacementManager::requestVisibility(
    const QString& panelId,
    bool visible
)
{
    if (!isFeatureEnabled()) {
        return makeUnavailableResult(
            panelId,
            RequestError::FeatureDisabled,
            QStringLiteral("Panel placement feature is disabled.")
        );
    }

    if (!active) {
        return makeUnavailableResult(
            panelId,
            RequestError::Inactive,
            QStringLiteral("Panel placement manager is inactive.")
        );
    }

    Registration* registration = findRegistration(panelId);
    if (!registration) {
        return makeUnavailableResult(
            panelId,
            RequestError::UnknownPanel,
            QStringLiteral("Panel is not registered.")
        );
    }

    if (!registration->dockWidget) {
        return makeUnavailableResult(
            panelId,
            RequestError::InvalidDock,
            QStringLiteral("Dock widget is unavailable.")
        );
    }

    if (registration->transitionInProgress) {
        return makeEntryResult(
            *registration,
            false,
            RequestError::TransitionInProgress,
            QStringLiteral("Panel transition is already in progress.")
        );
    }

    if (!placementHost) {
        return makeEntryResult(
            *registration,
            false,
            RequestError::HostFailure,
            QStringLiteral("Panel placement host is unavailable.")
        );
    }

    const QScopedValueRollback<bool> transitionGuard(registration->transitionInProgress);
    registration->transitionInProgress = true;

    const PanelPlacement anchorPlacement = registration->persistedPlacement;
    QStringList executionOrder = supportsArea(anchorPlacement)
        ? orderedPanelIdsInternal(anchorPlacement)
        : QStringList {panelId};
    if (executionOrder.isEmpty()) {
        executionOrder.push_back(panelId);
    }

    QMap<QString, QPointer<QDockWidget>> docks;
    QMap<QString, PanelPlacement> placements;
    QMap<QString, bool> previousVisibility;
    for (const auto& orderedId : executionOrder) {
        const Registration* affectedRegistration = findRegistration(orderedId);
        if (!affectedRegistration || !affectedRegistration->dockWidget) {
            return makeEntryResult(
                *registration,
                false,
                RequestError::InvalidDock,
                QStringLiteral("Dock widget is unavailable.")
            );
        }
        docks.insert(orderedId, affectedRegistration->dockWidget);
        placements.insert(orderedId, affectedRegistration->runtimePlacement);
        previousVisibility.insert(orderedId, isPanelVisible(orderedId));
    }

    QMap<QString, bool> desiredVisibility = previousVisibility;
    if (visible && supportsArea(anchorPlacement)
        && anchorPlacement.visibilityPolicy == PanelPlacement::VisibilityPolicy::Exclusive) {
        for (const auto& orderedId : executionOrder) {
            desiredVisibility.insert(orderedId, orderedId == panelId);
        }
    }
    else {
        desiredVisibility.insert(panelId, visible);
    }

    executionOrder.removeAll(panelId);
    executionOrder.push_front(panelId);

    QPointer<QWidget> previousFocus = QApplication::focusWidget();
    RequestError error = RequestError::None;
    QString message;
    if (!applyVisibilityTransaction(
            placementHost.get(),
            mainWindow,
            docks,
            placements,
            executionOrder,
            previousVisibility,
            desiredVisibility,
            &error,
            &message
        )) {
        restoreFocus(previousFocus.data());
        return makeEntryResult(*registration, false, error, message);
    }

    restoreFocus(previousFocus.data());
    return makeEntryResult(*registration, true, RequestError::None, QString());
}

PanelPlacementManager::RequestResult PanelPlacementManager::requestAreaVisibilityPolicy(
    const QString& panelId,
    PanelPlacement::VisibilityPolicy policy
)
{
    if (!isFeatureEnabled()) {
        return makeUnavailableResult(
            panelId,
            RequestError::FeatureDisabled,
            QStringLiteral("Panel placement feature is disabled.")
        );
    }

    if (!active) {
        return makeUnavailableResult(
            panelId,
            RequestError::Inactive,
            QStringLiteral("Panel placement manager is inactive.")
        );
    }

    Registration* registration = findRegistration(panelId);
    if (!registration) {
        return makeUnavailableResult(
            panelId,
            RequestError::UnknownPanel,
            QStringLiteral("Panel is not registered.")
        );
    }

    if (!registration->dockWidget) {
        return makeUnavailableResult(
            panelId,
            RequestError::InvalidDock,
            QStringLiteral("Dock widget is unavailable.")
        );
    }

    if (registration->transitionInProgress) {
        return makeEntryResult(
            *registration,
            false,
            RequestError::TransitionInProgress,
            QStringLiteral("Panel transition is already in progress.")
        );
    }

    if (!placementHost) {
        return makeEntryResult(
            *registration,
            false,
            RequestError::HostFailure,
            QStringLiteral("Panel placement host is unavailable.")
        );
    }

    if (!supportsArea(registration->persistedPlacement)) {
        return makeEntryResult(
            *registration,
            false,
            RequestError::UnsupportedPlacement,
            QStringLiteral("Area policy requires a docked, overlay, or auto-hide area.")
        );
    }

    const QScopedValueRollback<bool> transitionGuard(registration->transitionInProgress);
    registration->transitionInProgress = true;

    QStringList areaIds = orderedPanelIdsInternal(registration->persistedPlacement);
    if (areaIds.isEmpty()) {
        return makeEntryResult(*registration, true, RequestError::None, QString());
    }

    QMap<QString, PanelPlacement> previousPersisted;
    QMap<QString, PanelPlacement> previousRuntime;
    QMap<QString, bool> previousVisibility;
    QMap<QString, QPointer<QDockWidget>> docks;
    for (const auto& areaId : areaIds) {
        const Registration* areaRegistration = findRegistration(areaId);
        if (!areaRegistration || !areaRegistration->dockWidget) {
            return makeEntryResult(
                *registration,
                false,
                RequestError::InvalidDock,
                QStringLiteral("Dock widget is unavailable.")
            );
        }

        previousPersisted.insert(areaId, areaRegistration->persistedPlacement);
        previousRuntime.insert(areaId, areaRegistration->runtimePlacement);
        previousVisibility.insert(areaId, isPanelVisible(areaId));
        docks.insert(areaId, areaRegistration->dockWidget);
    }

    QMap<QString, PanelPlacement> desiredPersisted = previousPersisted;
    for (int index = 0; index < areaIds.size(); ++index) {
        PanelPlacement updated = previousPersisted.value(areaIds.at(index));
        updated.order = index;
        updated.visibilityPolicy = policy;
        updated.normalize();
        desiredPersisted.insert(areaIds.at(index), updated);
    }

    QString failedPanelId;
    if (!savePlacementMap(desiredPersisted, &failedPanelId)) {
        savePlacementMap(previousPersisted);
        return makeEntryResult(
            *registration,
            false,
            RequestError::PersistenceFailed,
            QStringLiteral("Area visibility policy could not be persisted.")
        );
    }

    QMap<QString, PanelPlacement> desiredRuntime = previousRuntime;
    for (auto it = desiredPersisted.cbegin(); it != desiredPersisted.cend(); ++it) {
        desiredRuntime.insert(
            it.key(),
            mergedRuntimePlacement(previousRuntime.value(it.key()), it.value())
        );
    }

    QMap<QString, bool> desiredVisibility = previousVisibility;
    if (policy == PanelPlacement::VisibilityPolicy::Exclusive) {
        QString survivorId;
        for (const auto& areaId : areaIds) {
            if (previousVisibility.value(areaId, false)) {
                survivorId = areaId;
                break;
            }
        }

        for (const auto& areaId : areaIds) {
            desiredVisibility.insert(areaId, !survivorId.isEmpty() && areaId == survivorId);
        }
    }

    QPointer<QWidget> previousFocus = QApplication::focusWidget();
    RequestError visibilityError = RequestError::None;
    QString visibilityMessage;
    if (!applyVisibilityTransaction(
            placementHost.get(),
            mainWindow,
            docks,
            desiredRuntime,
            areaIds,
            previousVisibility,
            desiredVisibility,
            &visibilityError,
            &visibilityMessage
        )) {
        savePlacementMap(previousPersisted);
        restoreFocus(previousFocus.data());
        for (auto it = previousPersisted.cbegin(); it != previousPersisted.cend(); ++it) {
            if (Registration* previousRegistration = findRegistration(it.key())) {
                previousRegistration->persistedPlacement = it.value();
                previousRegistration->runtimePlacement = previousRuntime.value(it.key(), it.value());
            }
        }
        return makeEntryResult(*registration, false, visibilityError, visibilityMessage);
    }

    for (auto it = desiredPersisted.cbegin(); it != desiredPersisted.cend(); ++it) {
        if (Registration* changedRegistration = findRegistration(it.key())) {
            changedRegistration->persistedPlacement = it.value();
            changedRegistration->runtimePlacement = desiredRuntime.value(it.key(), it.value());
        }
    }

    restoreFocus(previousFocus.data());
    for (const auto& areaId : areaIds) {
        Q_EMIT placementChanged(areaId);
    }
    return makeEntryResult(*registration, true, RequestError::None, QString());
}

QStringList PanelPlacementManager::orderedPanelIds(const PanelPlacement& area) const
{
    return orderedPanelIdsInternal(area);
}

void PanelPlacementManager::setHost(std::unique_ptr<PanelPlacementHost> host)
{
    placementHost = std::move(host);
    if (!placementHost) {
        placementHost = createDefaultHost();
    }
}

PanelPlacementHost* PanelPlacementManager::host() const
{
    return placementHost.get();
}

PanelPlacementManager::Registration* PanelPlacementManager::findRegistration(const QString& panelId)
{
    auto it = registrations.find(panelId);
    return it == registrations.end() ? nullptr : &it.value();
}

const PanelPlacementManager::Registration* PanelPlacementManager::findRegistration(
    const QString& panelId
) const
{
    auto it = registrations.constFind(panelId);
    return it == registrations.cend() ? nullptr : &it.value();
}

PanelPlacement PanelPlacementManager::normalizePlacement(
    const QString& panelId,
    const PanelPlacement& placement
)
{
    PanelPlacement normalized = placement;
    normalized.panelId = panelId;
    normalized.normalize();
    return normalized;
}

bool PanelPlacementManager::supportsArea(const PanelPlacement& placement)
{
    return placement.mode != PanelPlacement::Mode::Floating
        && placement.edge != PanelPlacement::Edge::None;
}

bool PanelPlacementManager::isSameArea(const PanelPlacement& left, const PanelPlacement& right)
{
    return supportsArea(left) && supportsArea(right) && left.mode == right.mode
        && left.edge == right.edge && left.region == right.region;
}

QStringList PanelPlacementManager::orderedPanelIdsInternal(
    const PanelPlacement& area,
    const QString& excludedPanelId
) const
{
    if (!supportsArea(area)) {
        return {};
    }

    std::vector<const Registration*> orderedRegistrations;
    orderedRegistrations.reserve(static_cast<std::size_t>(registrations.size()));
    for (auto it = registrations.cbegin(); it != registrations.cend(); ++it) {
        if (it.key() == excludedPanelId) {
            continue;
        }
        if (!isSameArea(it.value().persistedPlacement, area)) {
            continue;
        }
        orderedRegistrations.push_back(&it.value());
    }

    std::sort(
        orderedRegistrations.begin(),
        orderedRegistrations.end(),
        [](const Registration* left, const Registration* right) {
            if (left->persistedPlacement.order != right->persistedPlacement.order) {
                return left->persistedPlacement.order < right->persistedPlacement.order;
            }
            return left->panelId < right->panelId;
        }
    );

    QStringList panelIds;
    panelIds.reserve(static_cast<qsizetype>(orderedRegistrations.size()));
    for (const Registration* orderedRegistration : orderedRegistrations) {
        panelIds.push_back(orderedRegistration->panelId);
    }
    return panelIds;
}

PanelPlacement::VisibilityPolicy PanelPlacementManager::resolvedAreaPolicy(
    const QStringList& panelIds,
    PanelPlacement::VisibilityPolicy fallback
) const
{
    for (const auto& panelId : panelIds) {
        if (const Registration* registration = findRegistration(panelId)) {
            return registration->persistedPlacement.visibilityPolicy;
        }
    }

    return fallback;
}

PanelPlacement PanelPlacementManager::snapshotRuntimePlacement(
    MainWindow* mainWindow,
    QDockWidget* dockWidget,
    const PanelPlacement& basis
) const
{
    PanelPlacement runtime = basis;
    runtime.normalize();
    if (!dockWidget) {
        return runtime;
    }

    if (placementHost && placementHost->queryPlacement(mainWindow, dockWidget, &runtime)) {
        runtime.panelId = basis.panelId;
        runtime.order = basis.order;
        runtime.visibilityPolicy = basis.visibilityPolicy;
        runtime.launcher = basis.launcher;
        runtime.region = basis.region;
        runtime.groupId = basis.groupId;
        runtime.groupOrder = basis.groupOrder;
        runtime.tabOrder = basis.tabOrder;
        runtime.splitRelation = basis.splitRelation;
        runtime.extent = basis.extent;
        runtime.normalize();
        return runtime;
    }

    if (dockWidget->isFloating()) {
        runtime.mode = PanelPlacement::Mode::Floating;
        runtime.edge = PanelPlacement::Edge::None;
        runtime.floatingGeometry = dockWidget->geometry();
        runtime.normalize();
        return runtime;
    }

    runtime.mode = PanelPlacement::Mode::Docked;
    if (mainWindow) {
        runtime.edge = edgeForDockArea(mainWindow->dockWidgetArea(dockWidget), runtime.edge);
    }

    runtime.floatingGeometry = QRect();
    runtime.normalize();
    return runtime;
}

void PanelPlacementManager::restoreFocus(QWidget* focusWidget)
{
    if (!focusWidget || !focusWidget->isVisible()) {
        return;
    }

    if (QWidget* focusWindow = focusWidget->window()) {
        focusWindow->activateWindow();
    }
    focusWidget->setFocus(Qt::OtherFocusReason);
}

PanelPlacementManager::RequestResult PanelPlacementManager::makeUnavailableResult(
    const QString& panelId,
    RequestError error,
    const QString& message
) const
{
    RequestResult result;
    result.success = false;
    result.error = error;
    result.message = message;
    result.persistedPlacement.panelId = panelId;
    result.persistedPlacement.normalize();
    result.runtimePlacement = result.persistedPlacement;
    return result;
}

PanelPlacementManager::RequestResult PanelPlacementManager::makeEntryResult(
    const Registration& registration,
    bool success,
    RequestError error,
    const QString& message
) const
{
    RequestResult result;
    result.success = success;
    result.error = error;
    result.message = message;
    result.persistedPlacement = registration.persistedPlacement;
    result.runtimePlacement = registration.runtimePlacement;
    return result;
}
