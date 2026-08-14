// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cstdlib>
#include <memory>
#include <string>

#include <QAction>
#include <QCoreApplication>
#include <QDockWidget>
#include <QKeySequence>
#include <QLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QMenu>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/CompactMainWindowChrome.h>
#include <Gui/CompactTitleBarStyle.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/OverlayManager.h>
#include <Gui/PanelPlacementManager.h>
#include <src/App/InitApplication.h>

class testCompactMainWindowChrome final: public QObject
{
    Q_OBJECT

public:
    testCompactMainWindowChrome()
    {
        tests::initApplication();
        if (!Gui::Application::Instance) {
            guiApplication = std::make_unique<Gui::Application>(false);
        }
        Gui::Application::initOpenInventor();
    }

private Q_SLOTS:
    void init()  // NOLINT
    {
        preferences = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/MainWindow"
        );
        compactLayoutBefore = preferences->GetBool("CompactJetBrainsLayout", false);
        panelPlacementBefore = preferences->GetBool("CompactJetBrainsPanelPlacementEnabled", false);
        framelessBefore = preferences->GetBool("CompactJetBrainsFramelessWindow", false);
        preferences->SetBool("CompactJetBrainsLayout", true);
        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", false);
        preferences->SetBool("CompactJetBrainsFramelessWindow", false);
    }

    void cleanup()  // NOLINT
    {
        if (preferences) {
            preferences->SetBool("CompactJetBrainsLayout", compactLayoutBefore);
            preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", panelPlacementBefore);
            preferences->SetBool("CompactJetBrainsFramelessWindow", framelessBefore);
        }
        preferences = nullptr;
    }

    void titleButtonsExposeAccessibleText()  // NOLINT
    {
        createMainWindow();

        const QStringList labels {
            QStringLiteral("Window menu"),
            QStringLiteral("Show the main menu"),
            QStringLiteral("Minimize"),
            QStringLiteral("Maximize"),
            QStringLiteral("Close"),
        };

        for (const auto& label : labels) {
            auto button = buttonWithToolTip(label);
            QVERIFY2(button, qPrintable(QStringLiteral("Missing compact chrome button: %1").arg(label)));
            QCOMPARE(button->accessibleName(), label);
            QCOMPARE(button->statusTip(), label);
            auto toolbar = qobject_cast<QToolBar*>(button->parentWidget());
            QVERIFY(toolbar);
            QCOMPARE(button->iconSize(), toolbar->iconSize());
        }

        const auto buttons = panelStripButtons();
        for (auto button : buttons) {
            if (button->toolTip().isEmpty()) {
                continue;
            }
            QVERIFY(qobject_cast<QToolBar*>(button->parentWidget()));
            auto action = button->defaultAction();
            QVERIFY(action);
            QVERIFY(action->isCheckable());
            QCOMPARE(
                button->iconSize(),
                QSize(
                    Gui::CompactTitleBarStyle::panelRailIconSize(),
                    Gui::CompactTitleBarStyle::panelRailIconSize()
                )
            );
            QCOMPARE(button->minimumSize(), Gui::CompactTitleBarStyle::panelButtonSize());
        }
    }

    void compactModeRestoresMenuBarAndContentsMargins()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto menuBar = mainWindow->menuBar();
        QVERIFY(menuBar);
        menuBar->show();
        const QMargins margins(7, 8, 9, 10);
        mainWindow->setContentsMargins(margins);

        preferences->SetBool("CompactJetBrainsLayout", true);
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        auto topBar = compactTopBar();
        QVERIFY(topBar);
        QVERIFY(!compactTopBarHost()->isHidden());
        QVERIFY(menuBar->isHidden());

        preferences->SetBool("CompactJetBrainsLayout", false);
        QCoreApplication::processEvents();
        QVERIFY(compactTopBarHost()->isHidden());
        QVERIFY(!menuBar->isHidden());
        QCOMPARE(mainWindow->contentsMargins(), margins);
    }

    void experimentalPlacementOwnsLegacyOverlayTabsOnlyWhileEnabled()  // NOLINT
    {
        createMainWindow();

        QVERIFY(!Gui::OverlayManager::compactRailTabOwnershipEnabled());

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        processPendingEvents();
        QVERIFY(Gui::OverlayManager::compactRailTabOwnershipEnabled());

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", false);
        processPendingEvents();
        QVERIFY(!Gui::OverlayManager::compactRailTabOwnershipEnabled());
    }

    void compactShellMarginsReserveRailsForCentralAndDocks()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        struct DockCleanup
        {
            ~DockCleanup()
            {
                for (const auto& name : dockNames) {
                    Gui::DockWindowManager::instance()->removeDockWindow(name.toUtf8().constData());
                }
            }

            QStringList dockNames;
        } cleanup {
            {QStringLiteral("CompactGeometryLeftDock"), QStringLiteral("CompactGeometryBottomDock")}
        };

        auto central = mainWindow->centralWidget();
        QVERIFY(central);

        auto leftPanel = new QWidget();
        leftPanel->setObjectName(QStringLiteral("CompactGeometryLeftPanel"));
        leftPanel->setWindowTitle(QStringLiteral("Compact geometry left"));
        auto leftDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactGeometryLeftDock",
            leftPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(leftDock);
        leftDock->toggleViewAction()->setData(QByteArray("CompactGeometryLeftDock"));
        leftDock->toggleViewAction()->setVisible(true);

        auto bottomPanel = new QWidget();
        bottomPanel->setObjectName(QStringLiteral("CompactGeometryBottomPanel"));
        bottomPanel->setWindowTitle(QStringLiteral("Compact geometry bottom"));
        auto bottomDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactGeometryBottomDock",
            bottomPanel,
            Qt::BottomDockWidgetArea
        );
        QVERIFY(bottomDock);
        bottomDock->toggleViewAction()->setData(QByteArray("CompactGeometryBottomDock"));
        bottomDock->toggleViewAction()->setVisible(true);

        const QMargins originalMargins(6, 7, 8, 9);
        mainWindow->setContentsMargins(originalMargins);
        const QMargins originalLayoutMargins = mainWindow->layout()->contentsMargins();
        mainWindow->show();
        leftDock->show();
        bottomDock->show();
        processPendingEvents();

        QTRY_VERIFY(central->isVisible());
        QTRY_VERIFY(leftDock->isVisible());
        QTRY_VERIFY(bottomDock->isVisible());

        const QRect centralBefore = central->geometry();
        const QRect leftDockBefore = leftDock->geometry();
        const QRect bottomDockBefore = bottomDock->geometry();

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();

        auto topBar = compactTopBar();
        auto leftRail = mainWindow->findChild<QToolBar*>(
            QStringLiteral("_fc_compact_left_panel_rail_host")
        );
        auto rightRail = mainWindow->findChild<QToolBar*>(
            QStringLiteral("_fc_compact_right_panel_rail_host")
        );
        QVERIFY(topBar);
        QVERIFY(leftRail);
        QVERIFY(rightRail);
        QTRY_VERIFY(topBar->isVisible());
        QTRY_VERIFY(leftRail->isVisible());
        QTRY_VERIFY(rightRail->isVisible());

        QCOMPARE(mainWindow->contentsMargins(), originalMargins);
        const QMargins compactLayoutMargins = mainWindow->layout()->contentsMargins();
        QCOMPARE(compactLayoutMargins, originalLayoutMargins);

        const QRect leftRailRect = widgetRectInMainWindow(leftRail);
        const QRect rightRailRect = widgetRectInMainWindow(rightRail);
        const QRect topBarRect = widgetRectInMainWindow(topBar);
        const QRect centralRect = widgetRectInMainWindow(central);
        const QRect leftDockRect = widgetRectInMainWindow(leftDock);
        const QRect bottomDockRect = widgetRectInMainWindow(bottomDock);

        QVERIFY2(
            !leftRailRect.intersects(centralRect),
            qPrintable(QStringLiteral("Left rail intersects central widget: rail=%1 central=%2")
                           .arg(rectString(leftRailRect), rectString(centralRect)))
        );
        QVERIFY2(
            !rightRailRect.intersects(centralRect),
            qPrintable(QStringLiteral("Right rail intersects central widget: rail=%1 central=%2")
                           .arg(rectString(rightRailRect), rectString(centralRect)))
        );
        QVERIFY2(
            !leftRailRect.intersects(leftDockRect),
            qPrintable(QStringLiteral("Left rail intersects left dock: rail=%1 dock=%2")
                           .arg(rectString(leftRailRect), rectString(leftDockRect)))
        );
        QVERIFY2(
            !leftRailRect.intersects(bottomDockRect),
            qPrintable(QStringLiteral("Left rail intersects bottom dock: rail=%1 dock=%2")
                           .arg(rectString(leftRailRect), rectString(bottomDockRect)))
        );
        QVERIFY2(
            !rightRailRect.intersects(bottomDockRect),
            qPrintable(QStringLiteral("Right rail intersects bottom dock: rail=%1 dock=%2")
                           .arg(rectString(rightRailRect), rectString(bottomDockRect)))
        );
        QVERIFY(!topBarRect.intersects(leftRailRect));
        QVERIFY(!topBarRect.intersects(rightRailRect));
        QVERIFY(!topBarRect.intersects(centralRect));

        preferences->SetBool("CompactJetBrainsLayout", false);
        processPendingEvents();

        QCOMPARE(mainWindow->contentsMargins(), originalMargins);
        QCOMPARE(mainWindow->layout()->contentsMargins(), originalLayoutMargins);
        QCOMPARE(central->geometry(), centralBefore);
        QCOMPARE(leftDock->geometry(), leftDockBefore);
        QCOMPARE(bottomDock->geometry(), bottomDockBefore);
    }

    void panelPlacementManagerRequiresDedicatedFlag()  // NOLINT
    {
        createMainWindow();
        processPendingEvents();

        auto manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(manager);
        QVERIFY(compactTopBar());
        QVERIFY(!compactTopBarHost()->isHidden());
        QCOMPARE(manager->isActive(), false);

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        processPendingEvents();
        QCOMPARE(manager->isActive(), true);

        preferences->SetBool("CompactJetBrainsLayout", false);
        processPendingEvents();
        QCOMPARE(manager->isActive(), false);
        QVERIFY(compactTopBarHost()->isHidden());

        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();
        QVERIFY(!compactTopBarHost()->isHidden());
        QCOMPARE(manager->isActive(), true);

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", false);
        processPendingEvents();
        QCOMPARE(manager->isActive(), false);
        QVERIFY(!compactTopBarHost()->isHidden());
    }

    void compactMenuBarIsVerticallyCenteredInSwitchArea()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();
        preferences->SetBool("CompactJetBrainsLayout", true);
        QCoreApplication::processEvents();

        auto menuButton = buttonWithToolTip(QStringLiteral("Show the main menu"));
        QVERIFY(menuButton);
        QTest::mouseClick(menuButton, Qt::LeftButton);
        QCoreApplication::processEvents();

        auto compactMenuBar = mainWindow->findChild<QMenuBar*>(QStringLiteral("_fc_compact_menu_bar"));
        QVERIFY(compactMenuBar);
        QVERIFY(!compactMenuBar->isHidden());

        auto switchArea = compactMenuBar->parentWidget();
        QVERIFY(switchArea);
        const int menuCenter = compactMenuBar->geometry().center().y();
        const int switchCenter = switchArea->rect().center().y();
        QVERIFY2(
            std::abs(menuCenter - switchCenter) <= 1,
            qPrintable(QStringLiteral("Menu bar center %1 is not aligned with switch area center %2")
                           .arg(menuCenter)
                           .arg(switchCenter))
        );
    }

    void panelRailButtonsFitWithinRail()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();
        preferences->SetBool("CompactJetBrainsLayout", true);
        QCoreApplication::processEvents();

        const QStringList stripNames {
            QStringLiteral("_fc_compact_left_panel_railContent"),
            QStringLiteral("_fc_compact_right_panel_railContent"),
        };

        for (const auto& stripName : stripNames) {
            auto strip = mainWindow->findChild<QWidget*>(stripName);
            QVERIFY2(strip, qPrintable(QStringLiteral("Missing strip: %1").arg(stripName)));
            const auto buttons = strip->findChildren<QToolButton*>();
            for (auto button : buttons) {
                if (button->isHidden()) {
                    continue;
                }

                const QRect buttonRect(button->mapTo(strip, QPoint(0, 0)), button->size());
                QVERIFY2(
                    strip->rect().contains(buttonRect),
                    qPrintable(QStringLiteral("Button %1 is clipped in %2: button=%3,%4 %5x%6 strip=%7x%8")
                                   .arg(button->toolTip(), stripName)
                                   .arg(buttonRect.x())
                                   .arg(buttonRect.y())
                                   .arg(buttonRect.width())
                                   .arg(buttonRect.height())
                                   .arg(strip->width())
                                   .arg(strip->height()))
                );
            }
        }
    }

    void compactPanelSlotPreferenceOverridesDefaultRail()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();
        auto panel = new QWidget();
        panel->setObjectName(QStringLiteral("CompactSlotTestPanel"));
        panel->setWindowTitle(QStringLiteral("Compact slot test"));
        auto dock = Gui::DockWindowManager::instance()
                        ->addDockWindow("CompactSlotTestDock", panel, Qt::RightDockWidgetArea);
        QVERIFY(dock);
        dock->toggleViewAction()->setData(QByteArray("CompactSlotTestDock"));
        dock->toggleViewAction()->setVisible(true);

        auto slots = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/MainWindow/CompactJetBrainsPanelSlots"
        );
        const std::string previousSlot = slots->GetASCII("CompactSlotTestDock", "");
        slots->RemoveASCII("CompactSlotTestDock");

        preferences->SetBool("CompactJetBrainsLayout", true);
        QCoreApplication::processEvents();

        auto leftStrip = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_compact_left_panel_railContent")
        );
        auto rightStrip = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_compact_right_panel_railContent")
        );
        QVERIFY(leftStrip);
        QVERIFY(rightStrip);

        auto button = panelButtonForAssignment(QStringLiteral("CompactSlotTestDock"));
        QVERIFY(button);
        const QString assignmentId = button->property("_fc_compact_panel_assignment").toString();
        QVERIFY(!assignmentId.isEmpty());
        const QString overrideSlot = QStringLiteral("right-upper");

        slots->SetASCII(assignmentId.toUtf8().constData(), overrideSlot.toUtf8().constData());

        preferences->SetBool("CompactJetBrainsLayout", false);
        QCoreApplication::processEvents();
        preferences->SetBool("CompactJetBrainsLayout", true);
        QCoreApplication::processEvents();

        button = panelButtonForAssignment(assignmentId);
        QVERIFY(button);
        QCOMPARE(leftStrip->isAncestorOf(button), false);
        QCOMPARE(rightStrip->isAncestorOf(button), true);

        if (previousSlot.empty()) {
            slots->RemoveASCII(assignmentId.toUtf8().constData());
        }
        else {
            slots->SetASCII(assignmentId.toUtf8().constData(), previousSlot.c_str());
        }
        Gui::DockWindowManager::instance()->removeDockWindow("CompactSlotTestDock");
    }

    void panelLauncherUsesLiveDockActionAndTriggersExactlyOnce()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto panel = new QWidget();
        panel->setObjectName(QStringLiteral("CompactLiveActionPanel"));
        panel->setWindowTitle(QStringLiteral("Compact live action"));
        auto dock = Gui::DockWindowManager::instance()
                        ->addDockWindow("CompactLiveActionDock", panel, Qt::LeftDockWidgetArea);
        QVERIFY(dock);

        QAction* action = dock->toggleViewAction();
        QVERIFY(action);
        action->setData(QByteArray("CompactLiveActionDock"));
        action->setVisible(true);
        action->setText(QStringLiteral("Live panel"));
        action->setToolTip(QStringLiteral("Open the live panel"));
        action->setStatusTip(QStringLiteral("Live panel status"));
        action->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+9")));

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();

        QTRY_VERIFY(panelButtonForAssignment(QStringLiteral("CompactLiveActionDock")));
        QToolButton* button = panelButtonForAssignment(QStringLiteral("CompactLiveActionDock"));
        QCOMPARE(button->defaultAction(), action);
        QCOMPARE(button->accessibleName(), QStringLiteral("Live panel"));
        QCOMPARE(button->accessibleDescription(), QStringLiteral("Open the live panel"));
        QCOMPARE(button->statusTip(), QStringLiteral("Live panel status"));
        QCOMPARE(button->isChecked(), action->isChecked());
        QCOMPARE(button->defaultAction()->shortcut(), QKeySequence(QStringLiteral("Ctrl+Alt+9")));

        action->setText(QStringLiteral("Renamed live panel"));
        action->setToolTip(QStringLiteral("Open the renamed live panel"));
        action->setEnabled(false);
        QCOMPARE(button->accessibleName(), QStringLiteral("Renamed live panel"));
        QCOMPARE(button->accessibleDescription(), QStringLiteral("Open the renamed live panel"));
        QVERIFY(!button->isEnabled());

        action->setEnabled(true);
        int triggerCount = 0;
        connect(action, &QAction::triggered, this, [&triggerCount]() { ++triggerCount; });
        QTest::mouseClick(button, Qt::LeftButton);
        QCOMPARE(triggerCount, 1);

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactLiveActionDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactLiveActionDock");
    }

    void panelPlacementManagerUsesUnifiedLauncherWithoutMovingDock()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto chrome = compactChrome();
        auto manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(chrome);
        QVERIFY(manager);
        chrome->setPanelPlacementManager(manager);

        auto panel = new QWidget();
        panel->setObjectName(QStringLiteral("CompactPlacementManagerPanel"));
        panel->setWindowTitle(QStringLiteral("Compact placement manager"));
        auto dock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactPlacementManagerDock",
            panel,
            Qt::RightDockWidgetArea
        );
        QVERIFY(dock);
        dock->toggleViewAction()->setData(QByteArray("CompactPlacementManagerDock"));
        dock->toggleViewAction()->setVisible(true);

        Gui::PanelPlacement placement;
        placement.panelId = QStringLiteral("CompactPlacementManagerDock");
        placement.mode = Gui::PanelPlacement::Mode::Docked;
        placement.edge = Gui::PanelPlacement::Edge::Right;
        placement.launcher.rail = Gui::PanelPlacement::Launcher::Rail::Left;
        placement.launcher.cluster = Gui::PanelPlacement::Launcher::Cluster::Upper;
        placement.launcher.order = 7;
        QVERIFY(Gui::PanelPlacementStore::savePlacement(placement));

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        manager->setActive(true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();

        auto leftStrip = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_compact_left_panel_railContent")
        );
        auto rightStrip = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_compact_right_panel_railContent")
        );
        QVERIFY(leftStrip);
        QVERIFY(rightStrip);

        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactPlacementManagerDock")));
        QTRY_VERIFY(panelButtonForAssignment(QStringLiteral("CompactPlacementManagerDock")));
        auto button = panelButtonForAssignment(QStringLiteral("CompactPlacementManagerDock"));
        QVERIFY(button);
        QCOMPARE(leftStrip->isAncestorOf(button), true);
        QCOMPARE(rightStrip->isAncestorOf(button), false);
        QCOMPARE(mainWindow->dockWidgetArea(dock), Qt::RightDockWidgetArea);

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactPlacementManagerDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactPlacementManagerDock");
    }

    void panelPlacementManagerLauncherUpdateMovesButtonNotDock()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto chrome = compactChrome();
        auto manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(chrome);
        QVERIFY(manager);
        chrome->setPanelPlacementManager(manager);

        auto panel = new QWidget();
        panel->setObjectName(QStringLiteral("CompactLauncherUpdatePanel"));
        panel->setWindowTitle(QStringLiteral("Compact launcher update"));
        auto dock = Gui::DockWindowManager::instance()
                        ->addDockWindow("CompactLauncherUpdateDock", panel, Qt::RightDockWidgetArea);
        QVERIFY(dock);
        dock->toggleViewAction()->setData(QByteArray("CompactLauncherUpdateDock"));
        dock->toggleViewAction()->setVisible(true);

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        manager->setActive(true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();

        auto leftStrip = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_compact_left_panel_railContent")
        );
        auto rightStrip = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_compact_right_panel_railContent")
        );
        QVERIFY(leftStrip);
        QVERIFY(rightStrip);

        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactLauncherUpdateDock")));
        QTRY_VERIFY(panelButtonForAssignment(QStringLiteral("CompactLauncherUpdateDock")));
        auto button = panelButtonForAssignment(QStringLiteral("CompactLauncherUpdateDock"));
        QVERIFY(button);
        QCOMPARE(rightStrip->isAncestorOf(button), true);
        QCOMPARE(mainWindow->dockWidgetArea(dock), Qt::RightDockWidgetArea);

        Gui::PanelPlacement::Launcher launcher;
        launcher.rail = Gui::PanelPlacement::Launcher::Rail::Left;
        launcher.cluster = Gui::PanelPlacement::Launcher::Cluster::Lower;
        launcher.order = 3;
        const auto result
            = manager->updateLauncher(QStringLiteral("CompactLauncherUpdateDock"), launcher);
        QVERIFY2(result.success, qPrintable(result.message));
        processPendingEvents();

        button = panelButtonForAssignment(QStringLiteral("CompactLauncherUpdateDock"));
        QVERIFY(button);
        QCOMPARE(leftStrip->isAncestorOf(button), true);
        QCOMPARE(rightStrip->isAncestorOf(button), false);
        QCOMPARE(mainWindow->dockWidgetArea(dock), Qt::RightDockWidgetArea);

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactLauncherUpdateDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactLauncherUpdateDock");
    }

    void compactShortcutDispatchesExactlyOnceWithOriginalActionsHiddenBars()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();
        compactMenuBar()->hide();

        auto focusHost = new QWidget(mainWindow->centralWidget());
        focusHost->resize(320, 180);
        focusHost->setFocusPolicy(Qt::StrongFocus);
        focusHost->show();
        mainWindow->show();
        focusHost->setFocus();

        auto syntheticMenu = mainWindow->menuBar()->addMenu(QStringLiteral("Compact synthetic"));
        auto nestedMenu = syntheticMenu->addMenu(QStringLiteral("Nested"));
        auto undoAction = new QAction(QStringLiteral("Synthetic undo"), syntheticMenu);
        auto vtAction = new QAction(QStringLiteral("Synthetic VT"), nestedMenu);
        undoAction->setShortcut(QKeySequence::fromString(QStringLiteral("Ctrl+Z")));
        vtAction->setShortcut(QKeySequence::fromString(QStringLiteral("V,T")));

        int undoTriggers = 0;
        int vtTriggers = 0;
        QObject::connect(undoAction, &QAction::triggered, this, [&undoTriggers]() { ++undoTriggers; });
        QObject::connect(vtAction, &QAction::triggered, this, [&vtTriggers]() { ++vtTriggers; });

        syntheticMenu->addAction(undoAction);
        nestedMenu->addAction(vtAction);

        processPendingEvents();
        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();

        QTRY_VERIFY(compactTopBar() && compactTopBar()->isVisible());
        QCOMPARE(mainWindow->menuBar()->isHidden(), true);
        QVERIFY(compactTopBar());
        auto compactMenu = compactMenuBar();
        QVERIFY(compactMenu);
        QCOMPARE(compactMenu->isHidden(), true);

        QTRY_VERIFY(focusHost->hasFocus());

        QTest::keyClick(focusHost, Qt::Key_Z, Qt::ControlModifier);
        processPendingEvents();

        QTest::keyClick(focusHost, Qt::Key_V);
        processPendingEvents();
        QTest::keyClick(focusHost, Qt::Key_T);
        processPendingEvents();

        QTRY_COMPARE(undoTriggers, 1);
        QTRY_COMPARE(vtTriggers, 1);

        delete focusHost;
        delete syntheticMenu;
    }

    void qLineEditCtrlZHasLocalPriority()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();
        compactMenuBar()->hide();

        auto lineEdit = new QLineEdit(mainWindow->centralWidget());
        lineEdit->setText(QStringLiteral("base"));
        lineEdit->insert(QStringLiteral("-changed"));
        lineEdit->show();
        mainWindow->show();
        lineEdit->setFocus();

        auto syntheticMenu = mainWindow->menuBar()->addMenu(QStringLiteral("Edit menu"));
        auto undoAction = new QAction(QStringLiteral("Undo"), syntheticMenu);
        undoAction->setShortcut(QKeySequence::fromString(QStringLiteral("Ctrl+Z")));
        int undoTriggers = 0;
        QObject::connect(undoAction, &QAction::triggered, this, [&undoTriggers]() { ++undoTriggers; });
        syntheticMenu->addAction(undoAction);

        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();

        QTRY_VERIFY(compactTopBar() && compactTopBar()->isVisible());
        QCOMPARE(mainWindow->menuBar()->isHidden(), true);
        QVERIFY(compactTopBar());
        auto compactMenu = compactMenuBar();
        QVERIFY(compactMenu);
        QCOMPARE(compactMenu->isHidden(), true);
        QTRY_VERIFY(lineEdit->hasFocus());

        QTest::keyClick(lineEdit, Qt::Key_Z, Qt::ControlModifier);
        processPendingEvents();

        QTRY_COMPARE(undoTriggers, 0);
        QCOMPARE(lineEdit->text(), QStringLiteral("base"));

        delete lineEdit;
        delete syntheticMenu;
    }

    void workbenchRefreshRemovesStaleHostedActionsAndAddsCurrent()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();
        compactMenuBar()->hide();
        mainWindow->show();
        mainWindow->setFocus();

        auto oldMenu = mainWindow->menuBar()->addMenu(QStringLiteral("Old workbench menu"));
        auto oldAction = new QAction(QStringLiteral("Old action"), oldMenu);
        oldAction->setShortcut(QKeySequence::fromString(QStringLiteral("Ctrl+Y")));
        int oldTriggers = 0;
        QObject::connect(oldAction, &QAction::triggered, this, [&oldTriggers]() { ++oldTriggers; });
        oldMenu->addAction(oldAction);

        auto oldActionInTree = oldMenu->menuAction();

        QCOMPARE(mainWindowActionAssociationCount(oldAction, mainWindow.get()), 0);
        QCOMPARE(mainWindowActionListCount(oldAction), 0);

        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();
        QTRY_VERIFY(compactTopBar() && compactTopBar()->isVisible());
        QTRY_COMPARE(mainWindowActionAssociationCount(oldAction, mainWindow.get()), 1);
        QTRY_COMPARE(mainWindowActionListCount(oldAction), 1);

        mainWindow->activateWorkbench(QStringLiteral("PartWorkbench"));
        processPendingEvents();

        oldMenu->removeAction(oldAction);
        mainWindow->menuBar()->removeAction(oldActionInTree);

        auto freshMenu = mainWindow->menuBar()->addMenu(QStringLiteral("Fresh workbench menu"));
        auto freshAction = new QAction(QStringLiteral("Fresh action"), freshMenu);
        freshAction->setShortcut(QKeySequence::fromString(QStringLiteral("Ctrl+U")));
        int freshTriggers = 0;
        QObject::connect(freshAction, &QAction::triggered, this, [&freshTriggers]() {
            ++freshTriggers;
        });
        freshMenu->addAction(freshAction);

        mainWindow->activateWorkbench(QStringLiteral("PartWorkbench"));
        processPendingEvents();

        QCOMPARE(mainWindowActionAssociationCount(oldAction, mainWindow.get()), 0);
        QCOMPARE(mainWindowActionListCount(oldAction), 0);
        QCOMPARE(mainWindowActionAssociationCount(freshAction, mainWindow.get()), 1);
        QCOMPARE(mainWindowActionListCount(freshAction), 1);

        mainWindow->activateWorkbench(QStringLiteral("PartWorkbench"));
        processPendingEvents();

        QCOMPARE(mainWindowActionAssociationCount(freshAction, mainWindow.get()), 1);
        QCOMPARE(mainWindowActionListCount(freshAction), 1);

        QTest::keyClick(mainWindow.get(), Qt::Key_Y, Qt::ControlModifier);
        processPendingEvents();
        QTest::keyClick(mainWindow.get(), Qt::Key_U, Qt::ControlModifier);
        processPendingEvents();

        QCOMPARE(oldTriggers, 0);
        QCOMPARE(freshTriggers, 1);

        delete oldMenu;
        delete freshMenu;
    }

    void compactCleanupPreservesPreExistingMainWindowAssociation()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();
        compactMenuBar()->hide();
        mainWindow->show();
        mainWindow->setFocus();

        auto directMenu = mainWindow->menuBar()->addMenu(QStringLiteral("Direct action menu"));
        auto directAction = new QAction(QStringLiteral("Direct action"), directMenu);
        directAction->setShortcut(QKeySequence::fromString(QStringLiteral("Ctrl+X")));
        int directTriggers = 0;
        QObject::connect(directAction, &QAction::triggered, this, [&directTriggers]() {
            ++directTriggers;
        });
        directMenu->addAction(directAction);

        auto hostedMenu = mainWindow->menuBar()->addMenu(QStringLiteral("Hosted action menu"));
        auto hostedAction = new QAction(QStringLiteral("Hosted action"), hostedMenu);
        hostedAction->setShortcut(QKeySequence::fromString(QStringLiteral("Ctrl+Y")));
        int hostedTriggers = 0;
        QObject::connect(hostedAction, &QAction::triggered, this, [&hostedTriggers]() {
            ++hostedTriggers;
        });
        hostedMenu->addAction(hostedAction);

        mainWindow->addAction(directAction);
        QCOMPARE(mainWindowActionAssociationCount(directAction, mainWindow.get()), 1);
        QCOMPARE(mainWindowActionListCount(directAction), 1);
        QCOMPARE(mainWindowActionAssociationCount(hostedAction, mainWindow.get()), 0);
        QCOMPARE(mainWindowActionListCount(hostedAction), 0);

        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();
        QTRY_VERIFY(compactTopBar() && compactTopBar()->isVisible());
        QCOMPARE(mainWindowActionAssociationCount(directAction, mainWindow.get()), 1);
        QCOMPARE(mainWindowActionListCount(directAction), 1);
        QCOMPARE(mainWindowActionAssociationCount(hostedAction, mainWindow.get()), 1);
        QCOMPARE(mainWindowActionListCount(hostedAction), 1);

        preferences->SetBool("CompactJetBrainsLayout", false);
        processPendingEvents();
        QTRY_VERIFY(!compactTopBarHost() || compactTopBarHost()->isHidden());
        QCOMPARE(mainWindowActionAssociationCount(directAction, mainWindow.get()), 1);
        QCOMPARE(mainWindowActionListCount(directAction), 1);
        QCOMPARE(mainWindowActionAssociationCount(hostedAction, mainWindow.get()), 0);
        QCOMPARE(mainWindowActionListCount(hostedAction), 0);

        QTest::keyClick(mainWindow.get(), Qt::Key_X, Qt::ControlModifier);
        processPendingEvents();
        QTest::keyClick(mainWindow.get(), Qt::Key_Y, Qt::ControlModifier);
        processPendingEvents();

        QCOMPARE(directTriggers, 1);
        QCOMPARE(hostedTriggers, 1);

        delete directMenu;
        delete hostedMenu;
    }

private:
    void processPendingEvents() const
    {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    }

    void createMainWindow()
    {
        if (mainWindow) {
            return;
        }

        mainWindow = std::make_unique<Gui::MainWindow>();
        mainWindow->resize(900, 600);
        QCoreApplication::processEvents();
    }

    QWidget* compactTopBar() const
    {
        return mainWindow->findChild<QWidget*>(QStringLiteral("_fc_compact_top_bar"));
    }

    QToolBar* compactTopBarHost() const
    {
        return mainWindow->findChild<QToolBar*>(QStringLiteral("_fc_compact_top_bar_host"));
    }

    Gui::CompactMainWindowChrome* compactChrome() const
    {
        return mainWindow->findChild<Gui::CompactMainWindowChrome*>();
    }

    QMenuBar* compactMenuBar() const
    {
        return mainWindow->findChild<QMenuBar*>(QStringLiteral("_fc_compact_menu_bar"));
    }

    QWidget* panelRail(const QString& objectName) const
    {
        return mainWindow->findChild<QWidget*>(objectName);
    }

    int mainWindowActionAssociationCount(const QAction* action, const QWidget* widget) const
    {
        if (!action || !widget) {
            return 0;
        }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        int count = 0;
        for (auto* obj : action->associatedObjects()) {
            if (obj == widget) {
                ++count;
            }
        }
#else
        int count = 0;
        for (auto* widgetCandidate : action->associatedWidgets()) {
            if (widgetCandidate == widget) {
                ++count;
            }
        }
#endif

        return count;
    }

    int mainWindowActionListCount(const QAction* action) const
    {
        if (!mainWindow || !action) {
            return 0;
        }

        int count = 0;
        for (auto* candidate : mainWindow->actions()) {
            if (candidate == action) {
                ++count;
            }
        }

        return count;
    }

    QToolButton* buttonWithToolTip(const QString& tooltip) const
    {
        auto topBar = compactTopBar();
        if (!topBar) {
            return nullptr;
        }

        const auto buttons = topBar->findChildren<QToolButton*>();
        for (auto button : buttons) {
            if (button->toolTip() == tooltip) {
                return button;
            }
        }

        return nullptr;
    }

    QList<QToolButton*> panelStripButtons() const
    {
        QList<QToolButton*> buttons;
        const QStringList stripNames {
            QStringLiteral("_fc_compact_left_panel_railContent"),
            QStringLiteral("_fc_compact_right_panel_railContent"),
        };

        for (const auto& stripName : stripNames) {
            auto strip = mainWindow->findChild<QWidget*>(stripName);
            if (strip) {
                buttons.append(strip->findChildren<QToolButton*>());
            }
        }

        return buttons;
    }

    QToolButton* panelButtonForAssignment(const QString& assignmentId) const
    {
        QToolButton* match = nullptr;
        for (auto button : panelStripButtons()) {
            if (button->property("_fc_compact_panel_assignment").toString() == assignmentId) {
                match = button;
            }
        }

        return match;
    }

    static QString rectString(const QRect& rect)
    {
        return QStringLiteral("[%1,%2 %3x%4]")
            .arg(rect.x())
            .arg(rect.y())
            .arg(rect.width())
            .arg(rect.height());
    }

    QRect widgetRectInMainWindow(const QWidget* widget) const
    {
        if (!widget || !mainWindow) {
            return {};
        }
        return QRect(widget->mapTo(mainWindow.get(), QPoint()), widget->size());
    }

    std::unique_ptr<Gui::Application> guiApplication;
    std::unique_ptr<Gui::MainWindow> mainWindow;
    ParameterGrp::handle preferences;
    bool compactLayoutBefore = false;
    bool panelPlacementBefore = false;
    bool framelessBefore = false;
};

QTEST_MAIN(testCompactMainWindowChrome)

#include "CompactMainWindowChrome.moc"
