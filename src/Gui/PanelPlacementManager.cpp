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

#include <utility>

#include <QApplication>
#include <QDockWidget>
#include <QScopedValueRollback>
#include <QWidget>

#include "Application.h"
#include "MainWindow.h"
#include "OverlayManager.h"

using namespace Gui;

namespace
{

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

class DefaultPanelPlacementHost final: public PanelPlacementHost
{
public:
    bool queryPlacement(MainWindow* mainWindow, QDockWidget* dockWidget, PanelPlacement* placement) const override
    {
        if (!mainWindow || !Gui::Application::Instance || !dockWidget || !placement) {
            return false;
        }

        const Qt::DockWidgetArea overlayArea = OverlayManager::instance()->dockWidgetOverlayArea(
            dockWidget
        );
        if (overlayArea == Qt::NoDockWidgetArea) {
            return false;
        }

        placement->mode = PanelPlacement::Mode::Overlay;
        placement->edge = edgeForDockArea(overlayArea, PanelPlacement::Edge::Left);
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

        if (placement.mode == PanelPlacement::Mode::AutoHide) {
            Result result;
            result.error = Error::Unsupported;
            result.message = QStringLiteral("The auto-hide host is not installed.");
            return result;
        }

        if (!mainWindow) {
            Result result;
            result.error = Error::InvalidTarget;
            result.message = QStringLiteral("Main window is unavailable.");
            return result;
        }

        if (placement.mode == PanelPlacement::Mode::Overlay) {
            const Qt::DockWidgetArea area = dockAreaForEdge(placement.edge);
            if (area == Qt::NoDockWidgetArea) {
                Result result;
                result.error = Error::InvalidTarget;
                result.message = QStringLiteral("Overlay placement requires a valid edge.");
                return result;
            }

            OverlayManager* overlayManager = OverlayManager::instance();
            const Qt::DockWidgetArea previousArea = overlayManager->dockWidgetOverlayArea(dockWidget);
            Result result;
            result.success = overlayManager->moveDockWidgetToOverlayOnly(dockWidget, area);
            result.mutated = result.success && previousArea != area;
            if (!result.success) {
                result.error = Error::ApplyFailed;
                result.message = QStringLiteral("The overlay host rejected the panel.");
            }
            return result;
        }

        OverlayManager::instance()->unsetupDockWidget(dockWidget);

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
};

}  // namespace

bool PanelPlacementHost::queryPlacement(MainWindow*, QDockWidget*, PanelPlacement*) const
{
    return false;
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
        // Import the live semantic host (including a legacy overlay) while retaining
        // the independently migrated launcher anchor. Persistence stays lazy until
        // the user changes placement or launcher state.
        registration.runtimePlacement.launcher = registration.persistedPlacement.launcher;
        registration.runtimePlacement.normalize();
        registration.persistedPlacement = registration.runtimePlacement;
    }
    registration.destroyedConnection = connect(dockWidget, &QObject::destroyed, this, [this, panelId]() {
        unregisterPanel(panelId);
    });

    registrations.insert(panelId, registration);
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

    QDockWidget* registeredDock = registration->dockWidget.data();
    const bool previousVisibility = registeredDock->isVisible();
    QPointer<QWidget> previousFocus = QApplication::focusWidget();
    const PanelPlacement previousPersisted = registration->persistedPlacement;
    const PanelPlacement previousRuntime
        = snapshotRuntimePlacement(mainWindow, registeredDock, registration->runtimePlacement);

    PanelPlacement targetPlacement = normalizePlacement(panelId, placement);
    PanelPlacementHost::Result applyResult
        = placementHost->applyPlacement(mainWindow, registeredDock, targetPlacement);

    if (!applyResult.success) {
        RequestError error = applyResult.error == PanelPlacementHost::Error::Unsupported
            ? RequestError::UnsupportedPlacement
            : RequestError::HostFailure;

        if (applyResult.mutated) {
            const PanelPlacementHost::Result rollbackResult
                = placementHost->applyPlacement(mainWindow, registeredDock, previousRuntime);
            restoreVisibilityAndFocus(registeredDock, previousVisibility, previousFocus.data());

            if (!rollbackResult.success) {
                registration->runtimePlacement
                    = snapshotRuntimePlacement(mainWindow, registeredDock, targetPlacement);
                return makeEntryResult(
                    *registration,
                    false,
                    RequestError::RollbackFailed,
                    rollbackResult.message.isEmpty()
                        ? QStringLiteral("Panel placement rollback failed.")
                        : rollbackResult.message
                );
            }

            registration->runtimePlacement = previousRuntime;
        }
        registration->persistedPlacement = previousPersisted;
        return makeEntryResult(
            *registration,
            false,
            error,
            applyResult.message.isEmpty() ? QStringLiteral("Panel placement request failed.")
                                          : applyResult.message
        );
    }

    restoreVisibilityAndFocus(registeredDock, previousVisibility, previousFocus.data());

    if (!PanelPlacementStore::savePlacement(targetPlacement)) {
        const PanelPlacementHost::Result rollbackResult
            = placementHost->applyPlacement(mainWindow, registeredDock, previousRuntime);
        restoreVisibilityAndFocus(registeredDock, previousVisibility, previousFocus.data());

        if (!rollbackResult.success) {
            registration->runtimePlacement
                = snapshotRuntimePlacement(mainWindow, registeredDock, targetPlacement);
            return makeEntryResult(
                *registration,
                false,
                RequestError::RollbackFailed,
                rollbackResult.message.isEmpty()
                    ? QStringLiteral("Panel placement rollback failed after persistence error.")
                    : rollbackResult.message
            );
        }

        registration->runtimePlacement = previousRuntime;
        registration->persistedPlacement = previousPersisted;
        return makeEntryResult(
            *registration,
            false,
            RequestError::PersistenceFailed,
            QStringLiteral("Panel placement could not be persisted.")
        );
    }

    registration->persistedPlacement = targetPlacement;
    // A successful host application is authoritative for semantic modes such as
    // Overlay and AutoHide, which cannot be inferred from QDockWidget alone.
    registration->runtimePlacement = targetPlacement;
    Q_EMIT placementChanged(panelId);
    return makeEntryResult(*registration, true, RequestError::None, QString());
}

PanelPlacementManager::RequestResult PanelPlacementManager::updateLauncher(
    const QString& panelId,
    const PanelPlacement::Launcher& launcher
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

    if (registration->transitionInProgress) {
        return makeEntryResult(
            *registration,
            false,
            RequestError::TransitionInProgress,
            QStringLiteral("Panel transition is already in progress.")
        );
    }

    PanelPlacement updatedPlacement = registration->persistedPlacement;
    updatedPlacement.launcher = launcher;
    updatedPlacement.normalize();
    if (!PanelPlacementStore::savePlacement(updatedPlacement)) {
        return makeEntryResult(
            *registration,
            false,
            RequestError::PersistenceFailed,
            QStringLiteral("Launcher placement could not be persisted.")
        );
    }

    registration->persistedPlacement = updatedPlacement;
    registration->runtimePlacement.launcher = updatedPlacement.launcher;
    registration->runtimePlacement.normalize();
    Q_EMIT launcherChanged(panelId);
    return makeEntryResult(*registration, true, RequestError::None, QString());
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

void PanelPlacementManager::restoreVisibilityAndFocus(
    QDockWidget* dockWidget,
    bool visible,
    QWidget* focusWidget
)
{
    if (!dockWidget) {
        return;
    }

    if (visible) {
        dockWidget->show();
        dockWidget->raise();
    }
    else {
        dockWidget->hide();
    }

    if (focusWidget) {
        if (QWidget* focusWindow = focusWidget->window()) {
            focusWindow->activateWindow();
        }
        focusWidget->setFocus(Qt::OtherFocusReason);
    }
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
