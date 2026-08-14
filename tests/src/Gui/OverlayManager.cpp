// SPDX-License-Identifier: LGPL-2.1-or-later

#include <memory>

#include <QCoreApplication>
#include <QDockWidget>
#include <QTest>
#include <QWidget>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/OverlayManager.h>
#include <Gui/OverlayWidgets.h>
#include <Gui/PanelPlacementManager.h>
#include <src/App/InitApplication.h>

class testOverlayManager final: public QObject
{
    Q_OBJECT

public:
    testOverlayManager()
    {
        tests::initApplication();
        if (!Gui::Application::Instance) {
            guiApplication = std::make_unique<Gui::Application>(false);
        }
        Gui::Application::initOpenInventor();
    }

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        preferences = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/MainWindow"
        );
        compactLayoutBefore = preferences->GetBool("CompactJetBrainsLayout", false);
        panelPlacementBefore = preferences->GetBool("CompactJetBrainsPanelPlacementEnabled", false);
        framelessBefore = preferences->GetBool("CompactJetBrainsFramelessWindow", false);
        preferences->SetBool("CompactJetBrainsLayout", false);
        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", false);
        preferences->SetBool("CompactJetBrainsFramelessWindow", false);
    }

    void init()  // NOLINT
    {
        Gui::OverlayManager::destruct();
        if (!mainWindow) {
            mainWindow = std::make_unique<Gui::MainWindow>();
            mainWindow->resize(900, 600);
        }
        QCoreApplication::processEvents();
    }

    void cleanup()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", false);
        auto placements = App::GetApplication().GetParameterGroupByPath(
            Gui::PanelPlacementStore::placementsPath()
        );
        placements->RemoveGrp("OverlayManagerPlacementFirst");
        placements->RemoveGrp("OverlayManagerPlacementSecond");
        Gui::OverlayManager::destruct();
        if (mainWindow) {
            const auto overlays = mainWindow->findChildren<Gui::OverlayTabWidget*>();
            for (Gui::OverlayTabWidget* overlayWidget : overlays) {
                delete overlayWidget;
            }
            const auto docks = mainWindow->findChildren<QDockWidget*>();
            for (QDockWidget* dock : docks) {
                if (dock->objectName().startsWith(QStringLiteral("Overlay"))) {
                    delete dock;
                }
            }
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    }

    void cleanupTestCase()  // NOLINT
    {
        Gui::OverlayManager::destruct();
        mainWindow.reset();
        preferences->SetBool("CompactJetBrainsLayout", compactLayoutBefore);
        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", panelPlacementBefore);
        preferences->SetBool("CompactJetBrainsFramelessWindow", framelessBefore);
    }

    void moveSinglePanelLeavesPeerDocked()  // NOLINT
    {
        QDockWidget* first = addDock(QStringLiteral("OverlayOnlyFirst"));
        QDockWidget* second = addDock(QStringLiteral("OverlayOnlySecond"));

        auto* manager = Gui::OverlayManager::instance();
        QVERIFY(manager->moveDockWidgetToOverlayOnly(first, Qt::LeftDockWidgetArea));

        auto* leftOverlay = overlay(QStringLiteral("OverlayLeft"));
        QVERIFY(leftOverlay);
        QCOMPARE(manager->dockWidgetOverlayArea(first), Qt::LeftDockWidgetArea);
        QCOMPARE(manager->dockWidgetOverlayArea(second), Qt::NoDockWidgetArea);
        QCOMPARE(leftOverlay->count(), 1);
        QCOMPARE(leftOverlay->dockWidget(0), first);
        QCOMPARE(leftOverlay->dockWidgetIndex(second), -1);
        QCOMPARE(mainWindow->dockWidgetArea(second), Qt::LeftDockWidgetArea);
    }

    void legacyMoveStillAbsorbsPeer()  // NOLINT
    {
        QDockWidget* first = addDock(QStringLiteral("OverlayLegacyFirst"));
        QDockWidget* second = addDock(QStringLiteral("OverlayLegacySecond"));

        auto* manager = Gui::OverlayManager::instance();
        manager->moveDockWidgetToOverlay(first, Qt::LeftDockWidgetArea);

        auto* leftOverlay = overlay(QStringLiteral("OverlayLeft"));
        QVERIFY(leftOverlay);
        QCOMPARE(manager->dockWidgetOverlayArea(first), Qt::LeftDockWidgetArea);
        QCOMPARE(manager->dockWidgetOverlayArea(second), Qt::LeftDockWidgetArea);
        QCOMPARE(leftOverlay->count(), 2);
    }

    void invalidAreaIsRejectedWithoutMutation()  // NOLINT
    {
        QDockWidget* first = addDock(QStringLiteral("OverlayInvalidFirst"));
        QDockWidget* second = addDock(QStringLiteral("OverlayInvalidSecond"));
        const auto invalidArea = static_cast<Qt::DockWidgetArea>(
            static_cast<int>(Qt::LeftDockWidgetArea) | static_cast<int>(Qt::RightDockWidgetArea)
        );

        auto* manager = Gui::OverlayManager::instance();
        QVERIFY(!manager->moveDockWidgetToOverlayOnly(first, invalidArea));
        QCOMPARE(manager->dockWidgetOverlayArea(first), Qt::NoDockWidgetArea);
        QCOMPARE(manager->dockWidgetOverlayArea(second), Qt::NoDockWidgetArea);
        QCOMPARE(overlay(QStringLiteral("OverlayLeft"))->count(), 0);
    }

    void placementManagerOverlaysOnlyRequestedPanel()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        QDockWidget* first = addDock(QStringLiteral("OverlayManagerPlacementFirst"));
        QDockWidget* second = addDock(QStringLiteral("OverlayManagerPlacementSecond"));

        Gui::PanelPlacement fallback;
        fallback.mode = Gui::PanelPlacement::Mode::Docked;
        fallback.edge = Gui::PanelPlacement::Edge::Left;

        Gui::PanelPlacementManager placementManager(mainWindow.get());
        placementManager.setActive(true);
        QVERIFY(placementManager.registerPanel(first->objectName(), first, fallback));
        QVERIFY(placementManager.registerPanel(second->objectName(), second, fallback));

        Gui::PanelPlacement target = placementManager.persistedPlacement(first->objectName());
        target.mode = Gui::PanelPlacement::Mode::Overlay;
        target.edge = Gui::PanelPlacement::Edge::Left;
        target.normalize();
        const auto result = placementManager.requestPlacement(first->objectName(), target);

        QVERIFY(result.success);
        QCOMPARE(Gui::OverlayManager::instance()->dockWidgetOverlayArea(first), Qt::LeftDockWidgetArea);
        QCOMPARE(Gui::OverlayManager::instance()->dockWidgetOverlayArea(second), Qt::NoDockWidgetArea);
        QCOMPARE(
            placementManager.runtimePlacement(first->objectName()).mode,
            Gui::PanelPlacement::Mode::Overlay
        );
        QCOMPARE(mainWindow->dockWidgetArea(second), Qt::LeftDockWidgetArea);
    }

    void missingHostIsRejectedWithoutMutation()  // NOLINT
    {
        mainWindow.reset();
        QCoreApplication::processEvents();

        QDockWidget dock(QStringLiteral("OverlayMissingHost"));
        dock.setObjectName(QStringLiteral("OverlayMissingHost"));
        dock.setWidget(new QWidget(&dock));

        auto* manager = Gui::OverlayManager::instance();
        QVERIFY(!manager->moveDockWidgetToOverlayOnly(&dock, Qt::LeftDockWidgetArea));
        QCOMPARE(manager->dockWidgetOverlayArea(&dock), Qt::NoDockWidgetArea);
    }

private:
    QDockWidget* addDock(const QString& name)
    {
        auto* dock = new QDockWidget(name, mainWindow.get());
        dock->setObjectName(name);
        dock->setWidget(new QWidget(dock));
        mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dock);
        dock->show();
        dock->toggleViewAction()->setChecked(true);
        QCoreApplication::processEvents();
        return dock;
    }

    Gui::OverlayTabWidget* overlay(const QString& name) const
    {
        return mainWindow->findChild<Gui::OverlayTabWidget*>(name);
    }

    std::unique_ptr<Gui::Application> guiApplication;
    std::unique_ptr<Gui::MainWindow> mainWindow;
    ParameterGrp::handle preferences;
    bool compactLayoutBefore = false;
    bool panelPlacementBefore = false;
    bool framelessBefore = false;
};

QTEST_MAIN(testOverlayManager)

#include "OverlayManager.moc"
