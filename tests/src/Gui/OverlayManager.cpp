// SPDX-License-Identifier: LGPL-2.1-or-later

#include <memory>

#include <QAction>
#include <QCoreApplication>
#include <QDockWidget>
#include <QTabBar>
#include <QTest>
#include <QToolButton>
#include <QWidget>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/OverlayManager.h>
#include <Gui/OverlayParams.h>
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
        overlayHideTabBarBefore = Gui::OverlayParams::getDockOverlayHideTabBar();
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
        mainWindow->show();
        Gui::OverlayParams::setDockOverlayHideTabBar(false);
        QCoreApplication::processEvents();
    }

    void cleanup()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", false);
        if (mainWindow) {
            Gui::OverlayManager::instance()->setCompactRailTabOwnershipEnabled(false);
        }
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
                    if (!Gui::DockWindowManager::instance()->removeDockWindow(
                            dock->objectName().toUtf8().constData()
                        )) {
                        delete dock;
                    }
                }
            }
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    }

    void cleanupTestCase()  // NOLINT
    {
        Gui::OverlayParams::setDockOverlayHideTabBar(overlayHideTabBarBefore);
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

    void compactRailOwnershipHidesAndRestoresLeftOverlayTabs()  // NOLINT
    {
        QDockWidget* first = addDock(QStringLiteral("OverlayCompactTabsFirst"));
        addDock(QStringLiteral("OverlayCompactTabsSecond"));

        auto* manager = Gui::OverlayManager::instance();
        manager->moveDockWidgetToOverlay(first, Qt::LeftDockWidgetArea);

        auto* leftOverlay = overlay(QStringLiteral("OverlayLeft"));
        QVERIFY(leftOverlay);
        QCOMPARE(leftOverlay->count(), 2);

        leftOverlay->show();
        leftOverlay->setOverlayMode(false);
        QVERIFY(!leftOverlay->isHidden());
        QVERIFY(!leftOverlay->tabBar()->isHidden());

        manager->setCompactRailTabOwnershipEnabled(true);
        QVERIFY(leftOverlay->tabBar()->isHidden());
        QVERIFY(leftOverlay->isCompactRailTabOwnershipEnabled());

        manager->setCompactRailTabOwnershipEnabled(false);
        leftOverlay->show();
        leftOverlay->setOverlayMode(false);
        QVERIFY(!leftOverlay->tabBar()->isHidden());
        QVERIFY(!leftOverlay->isCompactRailTabOwnershipEnabled());
    }

    void compactRailOwnershipAppliesToRecreatedOverlayHosts()  // NOLINT
    {
        auto* manager = Gui::OverlayManager::instance();
        manager->setCompactRailTabOwnershipEnabled(true);
        Gui::OverlayManager::destruct();
        QCoreApplication::processEvents();
        const auto staleOverlays = mainWindow->findChildren<Gui::OverlayTabWidget*>();
        for (Gui::OverlayTabWidget* staleOverlay : staleOverlays) {
            delete staleOverlay;
        }

        QDockWidget* first = addDock(QStringLiteral("OverlayCompactRecreatedFirst"));
        addDock(QStringLiteral("OverlayCompactRecreatedSecond"));

        auto* recreated = Gui::OverlayManager::instance();
        recreated->moveDockWidgetToOverlay(first, Qt::LeftDockWidgetArea);

        auto* leftOverlay = overlay(QStringLiteral("OverlayLeft"));
        QVERIFY(leftOverlay);
        QCOMPARE(leftOverlay->count(), 2);
        leftOverlay->show();
        leftOverlay->setOverlayMode(false);
        QVERIFY(!leftOverlay->isHidden());
        QVERIFY(leftOverlay->tabBar()->isHidden());
        QVERIFY(leftOverlay->isCompactRailTabOwnershipEnabled());

        recreated->setCompactRailTabOwnershipEnabled(false);
        leftOverlay->show();
        leftOverlay->setOverlayMode(false);
        QVERIFY(!leftOverlay->tabBar()->isHidden());
        QVERIFY(!leftOverlay->isCompactRailTabOwnershipEnabled());
    }

    void canonicalHeaderOwnershipKeepsSingleVisibleHeader()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        auto* dock = addDock(QStringLiteral("OverlayCanonicalHeaderDock"));
        auto* manager = Gui::OverlayManager::instance();
        manager->setCompactRailTabOwnershipEnabled(true);
        manager->setupTitleBar(dock);
        QCoreApplication::processEvents();
        QVERIFY(dock);
        QVERIFY(!manager->isDockHeaderOwnedByOverlay(dock));
        QVERIFY(dock->titleBarWidget());
        QVERIFY(!dock->titleBarWidget()->isHidden());
        QCOMPARE(
            dock->titleBarWidget()->property("_fc_overlay_title_role").toByteArray(),
            QByteArray("dock")
        );

        QVERIFY(manager->moveDockWidgetToOverlayOnly(dock, Qt::LeftDockWidgetArea));

        auto* leftOverlay = overlay(QStringLiteral("OverlayLeft"));
        QVERIFY(leftOverlay);
        leftOverlay->show();
        QCoreApplication::processEvents();

        QVERIFY(manager->isDockHeaderOwnedByOverlay(dock));
        QVERIFY(dock->titleBarWidget());
        QCOMPARE(
            dock->titleBarWidget()->property("_fc_overlay_title_role").toByteArray(),
            QByteArray("placeholder")
        );
        QCOMPARE(activeHeaderCount(dock, leftOverlay), 1);

        manager->floatDockWidget(dock);
        QCoreApplication::processEvents();

        QCOMPARE(manager->dockWidgetOverlayArea(dock), Qt::NoDockWidgetArea);
        QVERIFY(!manager->isDockHeaderOwnedByOverlay(dock));
        QVERIFY(dock->titleBarWidget());
        QVERIFY(!dock->titleBarWidget()->isHidden());
        QCOMPARE(
            dock->titleBarWidget()->property("_fc_overlay_title_role").toByteArray(),
            QByteArray("dock")
        );
        QCOMPARE(activeHeaderCount(dock, nullptr), 1);

        delete dock;
    }

    void overlayHeaderTracksActiveDockActions()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        auto* first = addDock(QStringLiteral("OverlayHeaderActionsFirst"));
        auto* second = addDock(QStringLiteral("OverlayHeaderActionsSecond"));
        QVERIFY(first);
        QVERIFY(second);

        auto* firstAction = new QAction(QStringLiteral("Pin First"), first);
        firstAction->setProperty("DockTitleBarAction", true);
        first->addAction(firstAction);

        auto* manager = Gui::OverlayManager::instance();
        manager->setCompactRailTabOwnershipEnabled(true);
        manager->moveDockWidgetToOverlay(first, Qt::LeftDockWidgetArea);

        auto* leftOverlay = overlay(QStringLiteral("OverlayLeft"));
        QVERIFY(leftOverlay);
        leftOverlay->show();
        leftOverlay->setOverlayMode(false);
        QCoreApplication::processEvents();
        const int firstIndex = leftOverlay->dockWidgetIndex(first);
        const int secondIndex = leftOverlay->dockWidgetIndex(second);
        QVERIFY(firstIndex >= 0);
        QVERIFY(secondIndex >= 0);
        const QRect secondTab = leftOverlay->tabBar()->tabRect(secondIndex);
        const QRect firstTab = leftOverlay->tabBar()->tabRect(firstIndex);
        QVERIFY(secondTab.isValid());
        QVERIFY(firstTab.isValid());
        QTest::mouseClick(leftOverlay->tabBar(), Qt::LeftButton, Qt::NoModifier, secondTab.center());
        QCoreApplication::processEvents();
        QCOMPARE(leftOverlay->currentDockWidget(), second);
        QTest::mouseClick(leftOverlay->tabBar(), Qt::LeftButton, Qt::NoModifier, firstTab.center());
        QCoreApplication::processEvents();
        QCOMPARE(leftOverlay->currentDockWidget(), first);
        manager->refreshOverlayTitleBar(leftOverlay);

        QWidget* overlayTitleBar = leftOverlay->getTitleBar();
        QVERIFY(overlayTitleBar);
        QCOMPARE(titleBarButtonCount(overlayTitleBar), 4);

        QTest::mouseClick(leftOverlay->tabBar(), Qt::LeftButton, Qt::NoModifier, secondTab.center());
        QCoreApplication::processEvents();

        QCOMPARE(leftOverlay->currentDockWidget(), second);
        manager->refreshOverlayTitleBar(leftOverlay);
        overlayTitleBar = leftOverlay->getTitleBar();
        QVERIFY(overlayTitleBar);
        QCOMPARE(titleBarButtonCount(overlayTitleBar), 3);
    }

    void overlayAutoHideApiIsRuntimeOnlyForSingleDock()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        auto* dock = addManagedDock(QStringLiteral("OverlayAutoHideDock"));
        QVERIFY(dock);

        auto* manager = Gui::OverlayManager::instance();
        manager->setCompactRailTabOwnershipEnabled(true);
        QVERIFY(manager->moveDockWidgetToOverlayOnly(dock, Qt::LeftDockWidgetArea));
        QVERIFY(!manager->dockWidgetOverlayAutoHide(dock));
        QVERIFY(manager->setDockWidgetOverlayAutoHide(dock, true));
        QVERIFY(manager->dockWidgetOverlayAutoHide(dock));
        QVERIFY(manager->setDockWidgetOverlayAutoHide(dock, false));
        QVERIFY(!manager->dockWidgetOverlayAutoHide(dock));
    }

    void compactOverlayOrderMovesTabAndSurfaceTogether()  // NOLINT
    {
        auto* first = addDock(QStringLiteral("OverlayOrderFirst"));
        auto* second = addDock(QStringLiteral("OverlayOrderSecond"));
        auto* manager = Gui::OverlayManager::instance();
        manager->setCompactRailTabOwnershipEnabled(true);
        QVERIFY(manager->moveDockWidgetToOverlayOnly(first, Qt::LeftDockWidgetArea));
        QVERIFY(manager->moveDockWidgetToOverlayOnly(second, Qt::LeftDockWidgetArea));

        auto* leftOverlay = overlay(QStringLiteral("OverlayLeft"));
        QVERIFY(leftOverlay);
        QCOMPARE(leftOverlay->dockWidget(0), first);
        QCOMPARE(leftOverlay->getSplitter()->widget(0), first);

        QVERIFY(manager->moveDockWidgetInOverlay(second, 0));
        QCOMPARE(leftOverlay->dockWidget(0), second);
        QCOMPARE(leftOverlay->getSplitter()->widget(0), second);
        QCOMPARE(leftOverlay->dockWidget(1), first);
        QCOMPARE(leftOverlay->getSplitter()->widget(1), first);
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
        Gui::OverlayManager::instance()->setCompactRailTabOwnershipEnabled(true);
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
    int activeHeaderCount(QDockWidget* dock, Gui::OverlayTabWidget* overlayHost) const
    {
        int active = 0;
        if (dock && dock->titleBarWidget() && !dock->titleBarWidget()->isHidden()
            && dock->titleBarWidget()->property("_fc_overlay_title_role").toByteArray()
                != QByteArray("placeholder")) {
            ++active;
        }
        if (overlayHost && overlayHost->dockWidgetIndex(dock) >= 0 && !overlayHost->isHidden()
            && overlayHost->getTitleBar() && !overlayHost->getTitleBar()->isHidden()) {
            ++active;
        }
        return active;
    }

    int titleBarButtonCount(QWidget* titleBar) const
    {
        return titleBar
            ? titleBar->findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly).size()
            : 0;
    }

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

    QDockWidget* addManagedDock(const QString& name)
    {
        auto* panel = new QWidget();
        panel->setObjectName(name + QStringLiteral("Panel"));
        panel->setWindowTitle(name);
        auto* dock = Gui::DockWindowManager::instance()
                         ->addDockWindow(name.toUtf8().constData(), panel, Qt::LeftDockWidgetArea);
        if (!dock) {
            delete panel;
            return nullptr;
        }
        dock->toggleViewAction()->setData(name.toUtf8());
        dock->toggleViewAction()->setVisible(true);
        dock->show();
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
    bool overlayHideTabBarBefore = true;
};

QTEST_MAIN(testOverlayManager)

#include "OverlayManager.moc"
