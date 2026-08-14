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

#include <memory>

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QStringList>

#include <FCGlobal.h>

#include "PanelPlacement.h"

class QDockWidget;
class QWidget;

namespace Gui
{

class MainWindow;

class GuiExport PanelPlacementHost
{
public:
    enum class Error
    {
        None,
        Unsupported,
        InvalidTarget,
        ApplyFailed,
    };

    struct Result
    {
        bool success = false;
        bool mutated = false;
        Error error = Error::None;
        QString message;
    };

    virtual ~PanelPlacementHost() = default;

    virtual Result applyPlacement(
        MainWindow* mainWindow,
        QDockWidget* dockWidget,
        const PanelPlacement& placement
    ) = 0;

    /// Query semantic host state that cannot be inferred from QDockWidget alone.
    /// Return false to let the manager fall back to normal dock/floating inspection.
    virtual bool queryPlacement(
        MainWindow* mainWindow,
        QDockWidget* dockWidget,
        PanelPlacement* placement
    ) const;
};

class GuiExport PanelPlacementManager: public QObject
{
    Q_OBJECT

public:
    enum class RequestError
    {
        None,
        FeatureDisabled,
        Inactive,
        UnknownPanel,
        InvalidPanelId,
        InvalidDock,
        TransitionInProgress,
        UnsupportedPlacement,
        HostFailure,
        RollbackFailed,
        PersistenceFailed,
    };

    struct RequestResult
    {
        bool success = false;
        RequestError error = RequestError::None;
        QString message;
        PanelPlacement persistedPlacement;
        PanelPlacement runtimePlacement;
    };

    explicit PanelPlacementManager(MainWindow* mainWindow, QObject* parent = nullptr);
    PanelPlacementManager(
        MainWindow* mainWindow,
        std::unique_ptr<PanelPlacementHost> host,
        QObject* parent = nullptr
    );
    ~PanelPlacementManager() override;

    static std::unique_ptr<PanelPlacementHost> createDefaultHost();

    bool isFeatureEnabled() const;
    bool isActive() const;
    bool isEnabled() const;
    void setActive(bool active);

    bool registerPanel(
        const QString& panelId,
        QDockWidget* dockWidget,
        const PanelPlacement& fallbackPlacement = PanelPlacement(),
        QString* errorMessage = nullptr
    );
    bool unregisterPanel(const QString& panelId);

    bool isRegistered(const QString& panelId) const;
    QStringList registeredPanelIds() const;
    QDockWidget* dockWidget(const QString& panelId) const;
    PanelPlacement persistedPlacement(const QString& panelId) const;
    PanelPlacement runtimePlacement(const QString& panelId) const;

    RequestResult requestPlacement(const QString& panelId, const PanelPlacement& placement);
    RequestResult updateLauncher(const QString& panelId, const PanelPlacement::Launcher& launcher);

    void setHost(std::unique_ptr<PanelPlacementHost> host);
    PanelPlacementHost* host() const;

Q_SIGNALS:
    void activeChanged(bool active);
    void panelRegistered(const QString& panelId);
    void panelUnregistered(const QString& panelId);
    void placementChanged(const QString& panelId);
    void launcherChanged(const QString& panelId);

private:
    struct Registration
    {
        QString panelId;
        QPointer<QDockWidget> dockWidget;
        PanelPlacement persistedPlacement;
        PanelPlacement runtimePlacement;
        bool transitionInProgress = false;
        QMetaObject::Connection destroyedConnection;
    };

    Registration* findRegistration(const QString& panelId);
    const Registration* findRegistration(const QString& panelId) const;

    static PanelPlacement normalizePlacement(const QString& panelId, const PanelPlacement& placement);
    PanelPlacement snapshotRuntimePlacement(
        MainWindow* mainWindow,
        QDockWidget* dockWidget,
        const PanelPlacement& basis
    ) const;
    static void restoreVisibilityAndFocus(QDockWidget* dockWidget, bool visible, QWidget* focusWidget);

    RequestResult makeUnavailableResult(
        const QString& panelId,
        RequestError error,
        const QString& message
    ) const;
    RequestResult makeEntryResult(
        const Registration& registration,
        bool success,
        RequestError error,
        const QString& message
    ) const;

    MainWindow* mainWindow;
    std::unique_ptr<PanelPlacementHost> placementHost;
    bool active = false;
    QMap<QString, Registration> registrations;
};

}  // namespace Gui
