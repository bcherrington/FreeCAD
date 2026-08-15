// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>

#include <QAction>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeySequence>
#include <QLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QMenu>
#include <QMimeData>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPointer>
#include <QTabBar>
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
            QCOMPARE(
                button->iconSize(),
                QSize(Gui::CompactTitleBarStyle::iconSize(), Gui::CompactTitleBarStyle::iconSize())
            );
        }

        const auto buttons = panelStripButtons();
        for (auto button : buttons) {
            if (button->toolTip().isEmpty()) {
                continue;
            }
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

    void compactModeCollapsesAndRestoresMdiDocumentTabRow()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto* mdiArea = mainWindow->getMdiArea();
        QVERIFY(mdiArea);
        auto* document = new QWidget();
        document->setWindowTitle(QStringLiteral("Compact tab row geometry"));
        mdiArea->addSubWindow(document);
        document->show();
        mainWindow->show();
        processPendingEvents();

        auto* tabBar = mdiArea->findChild<QTabBar*>(QStringLiteral("mdiAreaTabBar"));
        QVERIFY(tabBar);
        const QString originalStyleSheet = tabBar->styleSheet();
        QVERIFY(tabBar->sizeHint().height() > 0);
        QVERIFY(mdiArea->viewport()->height() < mdiArea->height());

        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();

        QCOMPARE(tabBar->sizeHint().height(), 0);
        QCOMPARE(mdiArea->viewport()->geometry(), mdiArea->rect());

        preferences->SetBool("CompactJetBrainsLayout", false);
        processPendingEvents();

        QCOMPARE(tabBar->styleSheet(), originalStyleSheet);
        QVERIFY(tabBar->sizeHint().height() > 0);
        QVERIFY(mdiArea->viewport()->height() < mdiArea->height());
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

        const QList<QPair<QString, QString>> railNames {
            {QStringLiteral("_fc_compact_left_panel_rail_host"),
             QStringLiteral("_fc_compact_left_panel_railContent")},
            {QStringLiteral("_fc_compact_right_panel_rail_host"),
             QStringLiteral("_fc_compact_right_panel_railContent")},
        };

        for (const auto& [hostName, stripName] : railNames) {
            auto host = mainWindow->findChild<QToolBar*>(hostName);
            auto strip = mainWindow->findChild<QWidget*>(stripName);
            QVERIFY2(host, qPrintable(QStringLiteral("Missing rail host: %1").arg(hostName)));
            QVERIFY2(strip, qPrintable(QStringLiteral("Missing strip: %1").arg(stripName)));
            const QRect stripRectInHost(strip->mapTo(host, QPoint(0, 0)), strip->size());
            QCOMPARE(stripRectInHost.left(), host->contentsRect().left());
            QCOMPARE(stripRectInHost.center().x(), host->contentsRect().center().x());
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
                QCOMPARE(buttonRect.center().x(), strip->rect().center().x());
            }
        }
    }

    void panelRailArrowKeysTraverseVisibleLaunchers()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        const QStringList dockNames {
            QStringLiteral("CompactKeyboardFirstDock"),
            QStringLiteral("CompactKeyboardSecondDock"),
        };
        for (const QString& name : dockNames) {
            auto panel = new QWidget();
            panel->setObjectName(name + QStringLiteral("Panel"));
            panel->setWindowTitle(name);
            auto dock = Gui::DockWindowManager::instance()->addDockWindow(
                name.toUtf8().constData(),
                panel,
                Qt::LeftDockWidgetArea
            );
            QVERIFY(dock);
            dock->toggleViewAction()->setData(name.toUtf8());
            dock->toggleViewAction()->setVisible(true);
        }

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        mainWindow->show();
        processPendingEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        processPendingEvents();

        auto strip = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_compact_left_panel_railContent")
        );
        QVERIFY(strip);

        QList<QToolButton*> buttons;
        for (QToolButton* button : strip->findChildren<QToolButton*>()) {
            if (button->isVisibleTo(strip) && button->isEnabled()
                && button->property("_fc_compact_panel_assignment").isValid()) {
                buttons.append(button);
            }
        }
        std::sort(buttons.begin(), buttons.end(), [strip](QToolButton* left, QToolButton* right) {
            return left->mapTo(strip, QPoint()).y() < right->mapTo(strip, QPoint()).y();
        });
        QVERIFY(buttons.size() >= 2);

        auto* lane = buttons.first()->parentWidget();
        QVERIFY(lane);

        QList<QToolButton*> laneButtons;
        for (QToolButton* button : buttons) {
            if (button->parentWidget() == lane) {
                laneButtons.append(button);
            }
        }
        std::sort(laneButtons.begin(), laneButtons.end(), [lane](QToolButton* left, QToolButton* right) {
            const int leftY = left->mapTo(lane, QPoint()).y();
            const int rightY = right->mapTo(lane, QPoint()).y();
            if (leftY != rightY) {
                return leftY < rightY;
            }
            const int leftOrder = left->property("_fc_compact_panel_order").toInt();
            const int rightOrder = right->property("_fc_compact_panel_order").toInt();
            if (leftOrder != rightOrder) {
                return leftOrder < rightOrder;
            }
            return left->property("_fc_compact_panel_assignment").toString()
                < right->property("_fc_compact_panel_assignment").toString();
        });
        QVERIFY(laneButtons.size() >= 2);

        const QString firstPanelId
            = laneButtons.first()->property("_fc_compact_panel_assignment").toString();
        const QString secondPanelId
            = laneButtons.at(1)->property("_fc_compact_panel_assignment").toString();
        const auto focusedPanelId = []() {
            auto* focused = qobject_cast<QToolButton*>(QApplication::focusWidget());
            return focused ? focused->property("_fc_compact_panel_assignment").toString() : QString();
        };

        laneButtons.first()->setFocus(Qt::TabFocusReason);
        QTRY_COMPARE(focusedPanelId(), firstPanelId);
        QTest::keyClick(laneButtons.first(), Qt::Key_Down);
        QTRY_COMPARE(focusedPanelId(), secondPanelId);
        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Up);
        QTRY_COMPARE(focusedPanelId(), firstPanelId);

        for (const QString& name : dockNames) {
            Gui::PanelPlacementStore::removePlacement(name);
            Gui::DockWindowManager::instance()->removeDockWindow(name.toUtf8().constData());
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

    void managedPanelLauncherUsesLiveMetadataWithoutBypassingManager()  // NOLINT
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
        QVERIFY(button->styleSheet().isEmpty());

        action->setText(QStringLiteral("Renamed live panel"));
        action->setToolTip(QStringLiteral("Open the renamed live panel"));
        action->setEnabled(false);
        QCOMPARE(button->accessibleName(), QStringLiteral("Renamed live panel"));
        QCOMPARE(button->accessibleDescription(), QStringLiteral("Open the renamed live panel"));
        QVERIFY(!button->isEnabled());

        action->setEnabled(true);
        int triggerCount = 0;
        connect(action, &QAction::triggered, this, [&triggerCount]() { ++triggerCount; });

        auto* mdiArea = mainWindow->getMdiArea();
        QVERIFY(mdiArea);
        auto* firstDocument = mdiArea->addSubWindow(new QWidget());
        auto* activeDocument = mdiArea->addSubWindow(new QWidget());
        firstDocument->show();
        activeDocument->show();
        mdiArea->setActiveSubWindow(activeDocument);
        QCOMPARE(mdiArea->activeSubWindow(), activeDocument);

        QTest::mouseClick(button, Qt::LeftButton);
        QCOMPARE(triggerCount, 1);
        QCOMPARE(button->isChecked(), action->isChecked());
        QCOMPARE(mdiArea->activeSubWindow(), activeDocument);

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactLiveActionDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactLiveActionDock");
    }

    void managedPanelButtonsEnforceExclusiveArea()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto addPanel = [](const char* name) {
            auto* panel = new QWidget();
            panel->setWindowTitle(QString::fromLatin1(name));
            auto* dock = Gui::DockWindowManager::instance()
                             ->addDockWindow(name, panel, Qt::LeftDockWidgetArea);
            if (dock) {
                dock->toggleViewAction()->setData(QByteArray(name));
                dock->toggleViewAction()->setVisible(true);
                dock->show();
            }
            return dock;
        };

        auto* firstDock = addPanel("CompactExclusiveFirstDock");
        auto* secondDock = addPanel("CompactExclusiveSecondDock");
        QVERIFY(firstDock);
        QVERIFY(secondDock);

        Gui::PanelPlacement firstPlacement;
        firstPlacement.panelId = QStringLiteral("CompactExclusiveFirstDock");
        firstPlacement.mode = Gui::PanelPlacement::Mode::Docked;
        firstPlacement.edge = Gui::PanelPlacement::Edge::Left;
        firstPlacement.region = Gui::PanelPlacement::Region::Start;
        firstPlacement.order = 0;
        firstPlacement.visibilityPolicy = Gui::PanelPlacement::VisibilityPolicy::Exclusive;
        Gui::PanelPlacement secondPlacement = firstPlacement;
        secondPlacement.panelId = QStringLiteral("CompactExclusiveSecondDock");
        secondPlacement.order = 1;
        QVERIFY(Gui::PanelPlacementStore::savePlacement(firstPlacement));
        QVERIFY(Gui::PanelPlacementStore::savePlacement(secondPlacement));

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        mainWindow->show();
        processPendingEvents();

        auto* manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(manager);
        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactExclusiveFirstDock")));
        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactExclusiveSecondDock")));
        QTRY_VERIFY(!firstDock->isHidden());
        QTRY_VERIFY(secondDock->isHidden());

        auto* secondButton = panelButtonForAssignment(QStringLiteral("CompactExclusiveSecondDock"));
        QVERIFY(secondButton);
        QTest::mouseClick(secondButton, Qt::LeftButton);
        QTRY_VERIFY(firstDock->isHidden());
        QTRY_VERIFY(!secondDock->isHidden());

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactExclusiveFirstDock"));
        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactExclusiveSecondDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactExclusiveSecondDock");
        Gui::DockWindowManager::instance()->removeDockWindow("CompactExclusiveFirstDock");
    }

    void panelContextMenuMovesDockAndToggleTogether()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto panel = new QWidget();
        panel->setObjectName(QStringLiteral("CompactContextMenuPanel"));
        panel->setWindowTitle(QStringLiteral("Compact context menu"));
        auto dock = Gui::DockWindowManager::instance()
                        ->addDockWindow("CompactContextMenuDock", panel, Qt::LeftDockWidgetArea);
        QVERIFY(dock);
        dock->toggleViewAction()->setData(QByteArray("CompactContextMenuDock"));
        dock->toggleViewAction()->setVisible(true);

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        mainWindow->show();
        processPendingEvents();

        QTRY_VERIFY(panelButtonForAssignment(QStringLiteral("CompactContextMenuDock")));
        QToolButton* button = panelButtonForAssignment(QStringLiteral("CompactContextMenuDock"));
        const QPoint localPosition = button->rect().center();
        QContextMenuEvent contextEvent(
            QContextMenuEvent::Keyboard,
            localPosition,
            button->mapToGlobal(localPosition)
        );
        QApplication::sendEvent(button, &contextEvent);

        QPointer<QMenu> menu = mainWindow->findChild<QMenu*>(
            QStringLiteral("_fc_compact_panel_context_menu")
        );
        QVERIFY(menu);
        QMenu* moveMenu = nullptr;
        for (QAction* action : menu->actions()) {
            if (action->text() == QStringLiteral("Move To")) {
                moveMenu = action->menu();
                break;
            }
        }
        QVERIFY(moveMenu);
        QVERIFY(!menu->findChild<QMenu*>(QStringLiteral("Move Launcher To")));
        QAction* dockRight = nullptr;
        for (QAction* action : moveMenu->actions()) {
            if (action->text() == QStringLiteral("Dock Right")) {
                dockRight = action;
                break;
            }
        }
        QVERIFY(dockRight);
        dockRight->trigger();
        processPendingEvents();

        QTRY_VERIFY(panelButtonForAssignment(QStringLiteral("CompactContextMenuDock")));
        button = panelButtonForAssignment(QStringLiteral("CompactContextMenuDock"));
        auto* rightOuterLane = panelLaneWidget(QStringLiteral("_fc_compact_right_panel_dock_lane"));
        QVERIFY(rightOuterLane);
        QVERIFY(rightOuterLane->isAncestorOf(button));
        QCOMPARE(mainWindow->dockWidgetArea(dock), Qt::RightDockWidgetArea);

        const Gui::PanelPlacement placement = Gui::PanelPlacementStore::loadPlacement(
            QStringLiteral("CompactContextMenuDock")
        );
        QCOMPARE(placement.mode, Gui::PanelPlacement::Mode::Docked);
        QCOMPARE(placement.edge, Gui::PanelPlacement::Edge::Right);
        QVERIFY(placement.groupOrder >= 0);

        if (menu) {
            menu->close();
        }
        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactContextMenuDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactContextMenuDock");
    }

    void panelPlacementOverlaysOnlyTheRequestedPanel()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto peerPanel = new QWidget();
        peerPanel->setWindowTitle(QStringLiteral("Compact overlay peer"));
        auto peerDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactOverlayPeerDock",
            peerPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(peerDock);
        peerDock->toggleViewAction()->setData(QByteArray("CompactOverlayPeerDock"));
        peerDock->toggleViewAction()->setVisible(true);

        auto targetPanel = new QWidget();
        targetPanel->setWindowTitle(QStringLiteral("Compact overlay target"));
        auto targetDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactOverlayTargetDock",
            targetPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(targetDock);
        targetDock->toggleViewAction()->setData(QByteArray("CompactOverlayTargetDock"));
        targetDock->toggleViewAction()->setVisible(true);
        targetDock->setFloating(true);

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        mainWindow->show();
        peerDock->show();
        targetDock->show();
        processPendingEvents();

        auto* manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(manager);
        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactOverlayTargetDock")));
        const QRect centralBefore = mainWindow->centralWidget()->geometry();
        const QRect peerBefore = peerDock->geometry();
        Gui::PanelPlacement overlayPlacement = manager->persistedPlacement(
            QStringLiteral("CompactOverlayTargetDock")
        );
        overlayPlacement.mode = Gui::PanelPlacement::Mode::Overlay;
        overlayPlacement.edge = Gui::PanelPlacement::Edge::Left;
        overlayPlacement.region = Gui::PanelPlacement::Region::Center;
        const auto moveResult
            = manager->requestPlacement(QStringLiteral("CompactOverlayTargetDock"), overlayPlacement);
        QVERIFY2(moveResult.success, qPrintable(moveResult.message));
        processPendingEvents();

        QTRY_VERIFY(panelButtonForAssignment(QStringLiteral("CompactOverlayTargetDock")));
        QToolButton* button = panelButtonForAssignment(QStringLiteral("CompactOverlayTargetDock"));
        auto* leftInnerLane = panelLaneWidget(QStringLiteral("_fc_compact_left_panel_overlay_lane"));
        QVERIFY(leftInnerLane);
        QVERIFY(leftInnerLane->isAncestorOf(button));
        QCOMPARE(
            Gui::OverlayManager::instance()->dockWidgetOverlayArea(targetDock),
            Qt::LeftDockWidgetArea
        );
        QCOMPARE(Gui::OverlayManager::instance()->dockWidgetOverlayArea(peerDock), Qt::NoDockWidgetArea);
        QCOMPARE(mainWindow->dockWidgetArea(peerDock), Qt::LeftDockWidgetArea);
        QCOMPARE(peerDock->geometry(), peerBefore);
        QCOMPARE(mainWindow->centralWidget()->geometry(), centralBefore);

        targetDock->hide();
        processPendingEvents();
        QCOMPARE(mainWindow->centralWidget()->geometry(), centralBefore);
        targetDock->show();
        processPendingEvents();
        QCOMPARE(mainWindow->centralWidget()->geometry(), centralBefore);

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactOverlayPeerDock"));
        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactOverlayTargetDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactOverlayTargetDock");
        Gui::DockWindowManager::instance()->removeDockWindow("CompactOverlayPeerDock");
    }

    void panelPlacementManagerRoutesButtonsByPlacementModeAndEdge()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto chrome = compactChrome();
        auto manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(chrome);
        QVERIFY(manager);
        chrome->setPanelPlacementManager(manager);

        auto dockedPanel = new QWidget();
        dockedPanel->setObjectName(QStringLiteral("CompactPlacementManagerDockedPanel"));
        dockedPanel->setWindowTitle(QStringLiteral("Compact placement manager docked"));
        auto dockedDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactPlacementManagerDockedDock",
            dockedPanel,
            Qt::RightDockWidgetArea
        );
        QVERIFY(dockedDock);
        dockedDock->toggleViewAction()->setData(QByteArray("CompactPlacementManagerDockedDock"));
        dockedDock->toggleViewAction()->setVisible(true);

        auto overlayPanel = new QWidget();
        overlayPanel->setObjectName(QStringLiteral("CompactPlacementManagerOverlayPanel"));
        overlayPanel->setWindowTitle(QStringLiteral("Compact placement manager overlay"));
        auto overlayDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactPlacementManagerOverlayDock",
            overlayPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(overlayDock);
        overlayDock->toggleViewAction()->setData(QByteArray("CompactPlacementManagerOverlayDock"));
        overlayDock->toggleViewAction()->setVisible(true);

        Gui::PanelPlacement dockedPlacement;
        dockedPlacement.panelId = QStringLiteral("CompactPlacementManagerDockedDock");
        dockedPlacement.mode = Gui::PanelPlacement::Mode::Docked;
        dockedPlacement.edge = Gui::PanelPlacement::Edge::Right;
        dockedPlacement.groupOrder = 1;
        dockedPlacement.launcher.rail = Gui::PanelPlacement::Launcher::Rail::Left;
        dockedPlacement.launcher.cluster = Gui::PanelPlacement::Launcher::Cluster::Upper;
        dockedPlacement.launcher.order = 1;
        QVERIFY(Gui::PanelPlacementStore::savePlacement(dockedPlacement));

        Gui::PanelPlacement overlayPlacement;
        overlayPlacement.panelId = QStringLiteral("CompactPlacementManagerOverlayDock");
        overlayPlacement.mode = Gui::PanelPlacement::Mode::Overlay;
        overlayPlacement.edge = Gui::PanelPlacement::Edge::Left;
        overlayPlacement.groupOrder = 2;
        overlayPlacement.launcher.rail = Gui::PanelPlacement::Launcher::Rail::Right;
        overlayPlacement.launcher.cluster = Gui::PanelPlacement::Launcher::Cluster::Lower;
        overlayPlacement.launcher.order = 2;
        QVERIFY(Gui::PanelPlacementStore::savePlacement(overlayPlacement));

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        manager->setActive(true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();

        auto* leftInnerLane = panelLaneWidget(QStringLiteral("_fc_compact_left_panel_overlay_lane"));
        auto* rightOuterLane = panelLaneWidget(QStringLiteral("_fc_compact_right_panel_dock_lane"));
        QVERIFY(leftInnerLane);
        QVERIFY(rightOuterLane);

        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactPlacementManagerDockedDock")));
        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactPlacementManagerOverlayDock")));
        auto* dockedButton = panelButtonForAssignment(
            QStringLiteral("CompactPlacementManagerDockedDock")
        );
        auto* overlayButton = panelButtonForAssignment(
            QStringLiteral("CompactPlacementManagerOverlayDock")
        );
        QVERIFY(dockedButton);
        QVERIFY(overlayButton);
        QVERIFY(rightOuterLane->isAncestorOf(dockedButton));
        QVERIFY(leftInnerLane->isAncestorOf(overlayButton));
        QCOMPARE(mainWindow->dockWidgetArea(dockedDock), Qt::RightDockWidgetArea);
        QCOMPARE(
            Gui::OverlayManager::instance()->dockWidgetOverlayArea(overlayDock),
            Qt::LeftDockWidgetArea
        );

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactPlacementManagerDockedDock"));
        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactPlacementManagerOverlayDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactPlacementManagerDockedDock");
        Gui::DockWindowManager::instance()->removeDockWindow("CompactPlacementManagerOverlayDock");
    }

    void floatingPanelsOnlyAppearInOverflowMenu()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto chrome = compactChrome();
        auto manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(chrome);
        QVERIFY(manager);
        chrome->setPanelPlacementManager(manager);

        auto panel = new QWidget();
        panel->setObjectName(QStringLiteral("CompactFloatingPanel"));
        panel->setWindowTitle(QStringLiteral("Compact floating panel"));
        auto dock = Gui::DockWindowManager::instance()
                        ->addDockWindow("CompactFloatingDock", panel, Qt::LeftDockWidgetArea);
        QVERIFY(dock);
        dock->toggleViewAction()->setData(QByteArray("CompactFloatingDock"));
        dock->toggleViewAction()->setVisible(true);

        Gui::PanelPlacement floatingPlacement;
        floatingPlacement.panelId = QStringLiteral("CompactFloatingDock");
        floatingPlacement.mode = Gui::PanelPlacement::Mode::Floating;
        floatingPlacement.edge = Gui::PanelPlacement::Edge::None;
        floatingPlacement.launcher.rail = Gui::PanelPlacement::Launcher::Rail::Left;
        floatingPlacement.launcher.cluster = Gui::PanelPlacement::Launcher::Cluster::Upper;
        floatingPlacement.launcher.order = 0;
        floatingPlacement.floatingGeometry = dock->geometry();
        QVERIFY(Gui::PanelPlacementStore::savePlacement(floatingPlacement));

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        manager->setActive(true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        processPendingEvents();

        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactFloatingDock")));
        QVERIFY(!panelButtonForAssignment(QStringLiteral("CompactFloatingDock")));
        auto* overflow = overflowButton(QStringLiteral("_fc_compact_left_panel_dock_overflow"));
        QVERIFY(overflow);
        QVERIFY(overflow->menu());
        QStringList menuTexts;
        for (QAction* action : overflow->menu()->actions()) {
            menuTexts.push_back(action->text());
        }
        QVERIFY(menuTexts.contains(QStringLiteral("Compact floating panel")));

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactFloatingDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactFloatingDock");
    }

    void panelContextMoveAppendsToTargetLaneOrder()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto existingPanel = new QWidget();
        existingPanel->setWindowTitle(QStringLiteral("Existing right panel"));
        auto existingDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactExistingRightDock",
            existingPanel,
            Qt::RightDockWidgetArea
        );
        QVERIFY(existingDock);
        existingDock->toggleViewAction()->setData(QByteArray("CompactExistingRightDock"));
        existingDock->toggleViewAction()->setVisible(true);

        auto movingPanel = new QWidget();
        movingPanel->setWindowTitle(QStringLiteral("Moving left panel"));
        auto movingDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactMovingDock",
            movingPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(movingDock);
        movingDock->toggleViewAction()->setData(QByteArray("CompactMovingDock"));
        movingDock->toggleViewAction()->setVisible(true);

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        mainWindow->show();
        processPendingEvents();

        QTRY_VERIFY(panelButtonForAssignment(QStringLiteral("CompactMovingDock")));
        QToolButton* button = panelButtonForAssignment(QStringLiteral("CompactMovingDock"));
        const QPoint localPosition = button->rect().center();
        QContextMenuEvent contextEvent(
            QContextMenuEvent::Keyboard,
            localPosition,
            button->mapToGlobal(localPosition)
        );
        QApplication::sendEvent(button, &contextEvent);

        QPointer<QMenu> menu = mainWindow->findChild<QMenu*>(
            QStringLiteral("_fc_compact_panel_context_menu")
        );
        QVERIFY(menu);
        QMenu* moveMenu = nullptr;
        for (QAction* action : menu->actions()) {
            if (action->text() == QStringLiteral("Move To")) {
                moveMenu = action->menu();
                break;
            }
        }
        QVERIFY(moveMenu);
        QAction* dockRight = nullptr;
        for (QAction* action : moveMenu->actions()) {
            if (action->text() == QStringLiteral("Dock Right")) {
                dockRight = action;
                break;
            }
        }
        QVERIFY(dockRight);
        dockRight->trigger();
        processPendingEvents();

        QTRY_COMPARE(
            laneAssignments(QStringLiteral("_fc_compact_right_panel_dock_lane")),
            QStringList(
                {QStringLiteral("CompactExistingRightDock"), QStringLiteral("CompactMovingDock")}
            )
        );
        const Gui::PanelPlacement placement = Gui::PanelPlacementStore::loadPlacement(
            QStringLiteral("CompactMovingDock")
        );
        QCOMPARE(placement.edge, Gui::PanelPlacement::Edge::Right);
        // The move preserves the panel's lower region. The existing right panel
        // is in the upper region, so this is the first item in its target area.
        QCOMPARE(placement.order, 0);

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactExistingRightDock"));
        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactMovingDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactExistingRightDock");
        Gui::DockWindowManager::instance()->removeDockWindow("CompactMovingDock");
        if (menu) {
            menu->close();
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        processPendingEvents();
    }

    void panelPlacementUsesSingleRailWithCenteredOverlayGroup()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto dockedPanel = new QWidget();
        dockedPanel->setWindowTitle(QStringLiteral("Compact stacked docked"));
        auto dockedDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactStackedDockedDock",
            dockedPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(dockedDock);
        dockedDock->toggleViewAction()->setData(QByteArray("CompactStackedDockedDock"));
        dockedDock->toggleViewAction()->setVisible(true);

        auto overlayPanel = new QWidget();
        overlayPanel->setWindowTitle(QStringLiteral("Compact stacked overlay"));
        auto overlayDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactStackedOverlayDock",
            overlayPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(overlayDock);
        overlayDock->toggleViewAction()->setData(QByteArray("CompactStackedOverlayDock"));
        overlayDock->toggleViewAction()->setVisible(true);
        for (const QString& label :
             {QStringLiteral("Panel action one"), QStringLiteral("Panel action two")}) {
            auto* panelAction = new QAction(label, overlayDock);
            panelAction->setProperty("DockTitleBarAction", true);
            overlayDock->addAction(panelAction);
        }

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        mainWindow->show();
        processPendingEvents();

        auto* manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(manager);
        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactStackedOverlayDock")));

        Gui::PanelPlacement overlayPlacement = manager->persistedPlacement(
            QStringLiteral("CompactStackedOverlayDock")
        );
        overlayPlacement.mode = Gui::PanelPlacement::Mode::Overlay;
        overlayPlacement.edge = Gui::PanelPlacement::Edge::Right;
        overlayPlacement.region = Gui::PanelPlacement::Region::Center;
        QVERIFY(
            manager->requestPlacement(QStringLiteral("CompactStackedOverlayDock"), overlayPlacement).success
        );
        processPendingEvents();
        QCOMPARE(Gui::OverlayManager::instance()->dockWidgetOverlayExtent(overlayDock), 300);
        QTRY_VERIFY(mainWindow->findChild<QToolButton*>(QStringLiteral("OverlayTitleOverflow")));

        auto* leftRailHost = mainWindow->findChild<QToolBar*>(
            QStringLiteral("_fc_compact_left_panel_rail_host")
        );
        auto* leftStripContent = panelRail(QStringLiteral("_fc_compact_left_panel_railContent"));
        auto* leftOuterLane = panelLaneWidget(QStringLiteral("_fc_compact_left_panel_dock_lane"));
        auto* rightInnerLane = panelLaneWidget(QStringLiteral("_fc_compact_right_panel_overlay_lane"));
        QVERIFY(leftRailHost);
        QVERIFY(leftStripContent);
        QVERIFY(leftOuterLane);
        QVERIFY(rightInnerLane);

        const int expectedRailWidth = Gui::CompactTitleBarStyle::panelRailWidth();
        const int expectedLaneWidth = Gui::CompactTitleBarStyle::panelButtonSize().width();
        QCOMPARE(leftRailHost->width(), expectedRailWidth);
        QCOMPARE(leftStripContent->width(), expectedRailWidth);
        QCOMPARE(leftOuterLane->width(), expectedLaneWidth);
        QCOMPARE(rightInnerLane->width(), expectedLaneWidth);

        const QRect stripRect = widgetRectInMainWindow(leftStripContent);
        const QRect outerRect = widgetRectInMainWindow(leftOuterLane);
        const QRect innerRect = widgetRectInMainWindow(rightInnerLane);
        QVERIFY(stripRect.contains(outerRect));
        QCOMPARE(innerRect.width(), expectedLaneWidth);
        QVERIFY(rightInnerLane->mask().isEmpty());

        auto* rightOuterLane = panelLaneWidget(QStringLiteral("_fc_compact_right_panel_dock_lane"));
        QVERIFY(rightOuterLane);
        QVERIFY(rightOuterLane->isAncestorOf(rightInnerLane));

        QTRY_VERIFY(panelButtonForAssignment(QStringLiteral("CompactStackedOverlayDock")));
        auto* overlayButton = panelButtonForAssignment(QStringLiteral("CompactStackedOverlayDock"));
        QVERIFY(rightInnerLane->isAncestorOf(overlayButton));
        const QRect overlayButtonRect = widgetRectInMainWindow(overlayButton);
        QVERIFY(innerRect.contains(overlayButtonRect));
        QVERIFY(std::abs(overlayButtonRect.center().y() - innerRect.center().y()) <= 1);
        QWidget* overlayHit = mainWindow->childAt(overlayButtonRect.center());
        QVERIFY(overlayHit == overlayButton || overlayButton->isAncestorOf(overlayHit));

        auto* dockedButton = panelButtonForAssignment(QStringLiteral("CompactStackedDockedDock"));
        QVERIFY(dockedButton);
        const QRect dockedButtonRect = widgetRectInMainWindow(dockedButton);
        QWidget* dockedHit = mainWindow->childAt(dockedButtonRect.center());
        QVERIFY(dockedHit == dockedButton || dockedButton->isAncestorOf(dockedHit));

        QVERIFY(manager->requestVisibility(QStringLiteral("CompactStackedOverlayDock"), true).success);
        processPendingEvents();
        overlayButton = panelButtonForAssignment(QStringLiteral("CompactStackedOverlayDock"));
        QVERIFY(overlayButton);
        QTest::mouseClick(overlayButton, Qt::LeftButton);
        QTRY_VERIFY(!manager->isPanelVisible(QStringLiteral("CompactStackedOverlayDock")));
        QTRY_VERIFY(!overlayDock->isVisibleTo(mainWindow.get()));
        overlayButton = panelButtonForAssignment(QStringLiteral("CompactStackedOverlayDock"));
        QVERIFY(overlayButton);
        const QRect collapsedOverlayButtonRect = widgetRectInMainWindow(overlayButton);
        QWidget* collapsedOverlayHit = mainWindow->childAt(collapsedOverlayButtonRect.center());
        QVERIFY(
            collapsedOverlayHit == overlayButton || overlayButton->isAncestorOf(collapsedOverlayHit)
        );
        QTest::mouseClick(overlayButton, Qt::LeftButton);
        QTRY_VERIFY(manager->isPanelVisible(QStringLiteral("CompactStackedOverlayDock")));
        QTRY_VERIFY(overlayDock->isVisibleTo(mainWindow.get()));

        auto* leftInnerLane = panelLaneWidget(QStringLiteral("_fc_compact_left_panel_overlay_lane"));
        QVERIFY(leftInnerLane);
        QMimeData mimeData;
        mimeData.setData("application/x-freecad-compact-panel", QByteArray("CompactStackedOverlayDock"));
        const QPoint dropPosition = leftInnerLane->rect().center();
        QDragEnterEvent dragEnter(dropPosition, Qt::MoveAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(leftInnerLane, &dragEnter);
        QVERIFY(dragEnter.isAccepted());
        QDragMoveEvent dragMove(dropPosition, Qt::MoveAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(leftInnerLane, &dragMove);
        QVERIFY(dragMove.isAccepted());
        QDropEvent dropEvent(
            QPointF(dropPosition),
            Qt::MoveAction,
            &mimeData,
            Qt::LeftButton,
            Qt::NoModifier
        );
        QApplication::sendEvent(leftInnerLane, &dropEvent);
        QVERIFY(dropEvent.isAccepted());
        processPendingEvents();

        QTRY_COMPARE(
            manager->persistedPlacement(QStringLiteral("CompactStackedOverlayDock")).edge,
            Gui::PanelPlacement::Edge::Left
        );
        overlayButton = panelButtonForAssignment(QStringLiteral("CompactStackedOverlayDock"));
        QVERIFY(overlayButton);
        leftInnerLane = panelLaneWidget(QStringLiteral("_fc_compact_left_panel_overlay_lane"));
        QVERIFY(leftInnerLane);
        QVERIFY(leftInnerLane->isAncestorOf(overlayButton));

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactStackedDockedDock"));
        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactStackedOverlayDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactStackedOverlayDock");
        Gui::DockWindowManager::instance()->removeDockWindow("CompactStackedDockedDock");
    }

    void emptyOverlayTopLaneStillShowsDropTarget()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto sourcePanel = new QWidget();
        sourcePanel->setWindowTitle(QStringLiteral("Compact drop source"));
        auto sourceDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactDropSourceDock",
            sourcePanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(sourceDock);
        sourceDock->toggleViewAction()->setData(QByteArray("CompactDropSourceDock"));
        sourceDock->toggleViewAction()->setVisible(true);

        auto overlayPanel = new QWidget();
        overlayPanel->setWindowTitle(QStringLiteral("Compact drop target lane"));
        auto overlayDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactDropOverlayDock",
            overlayPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(overlayDock);
        overlayDock->toggleViewAction()->setData(QByteArray("CompactDropOverlayDock"));
        overlayDock->toggleViewAction()->setVisible(true);

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        mainWindow->show();
        processPendingEvents();

        auto* manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(manager);
        QTRY_VERIFY(manager->isRegistered(QStringLiteral("CompactDropOverlayDock")));

        Gui::PanelPlacement overlayPlacement = manager->persistedPlacement(
            QStringLiteral("CompactDropOverlayDock")
        );
        overlayPlacement.mode = Gui::PanelPlacement::Mode::Overlay;
        overlayPlacement.edge = Gui::PanelPlacement::Edge::Left;
        overlayPlacement.region = Gui::PanelPlacement::Region::Center;
        QVERIFY(
            manager->requestPlacement(QStringLiteral("CompactDropOverlayDock"), overlayPlacement).success
        );
        processPendingEvents();

        auto* leftInnerLane = panelLaneWidget(QStringLiteral("_fc_compact_left_panel_overlay_lane"));
        auto* overlayButton = panelButtonForAssignment(QStringLiteral("CompactDropOverlayDock"));
        QVERIFY(leftInnerLane);
        QVERIFY(overlayButton);
        QCOMPARE(
            overlayButton->property("_fc_compact_panel_slot").toString(),
            QStringLiteral("left-lower")
        );

        QMimeData mimeData;
        mimeData.setData("application/x-freecad-compact-panel", QByteArray("CompactDropSourceDock"));
        const QPoint targetPos(leftInnerLane->width() / 2, 4);
        QDragEnterEvent dragEnter(targetPos, Qt::MoveAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(leftInnerLane, &dragEnter);
        QDragMoveEvent dragMove(targetPos, Qt::MoveAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(leftInnerLane, &dragMove);
        processPendingEvents();

        auto* dropGroup = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_compact_panel_drop_indicator")
        );
        auto* dropInsertion = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_compact_panel_drop_insert_indicator")
        );
        QVERIFY(dropGroup);
        QVERIFY(dropInsertion);
        QVERIFY(dropGroup->isVisible());
        QVERIFY(dropInsertion->isVisible());

        const QRect laneRect = widgetRectInMainWindow(leftInnerLane);
        const QRect groupRect = widgetRectInMainWindow(dropGroup);
        const QRect insertionRect = widgetRectInMainWindow(dropInsertion);
        QVERIFY(groupRect.width() > 0);
        QVERIFY(groupRect.height() > 0);
        QVERIFY(insertionRect.width() > 0);
        QVERIFY(insertionRect.height() > 0);
        QVERIFY2(
            laneRect.contains(groupRect),
            qPrintable(QStringLiteral("Drop group escapes lane: lane=%1 group=%2")
                           .arg(rectString(laneRect), rectString(groupRect)))
        );
        QVERIFY2(
            laneRect.contains(insertionRect),
            qPrintable(QStringLiteral("Drop insertion escapes lane: lane=%1 insertion=%2")
                           .arg(rectString(laneRect), rectString(insertionRect)))
        );
        QVERIFY2(
            insertionRect.center().y() < laneRect.center().y(),
            qPrintable(QStringLiteral("Top drop target not visible near lane top: lane=%1 insertion=%2")
                           .arg(rectString(laneRect), rectString(insertionRect)))
        );

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactDropSourceDock"));
        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactDropOverlayDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactDropOverlayDock");
        Gui::DockWindowManager::instance()->removeDockWindow("CompactDropSourceDock");
    }

    void panelContextMenuAreaModeReflectsAndUpdatesPolicy()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto firstPanel = new QWidget();
        firstPanel->setWindowTitle(QStringLiteral("Compact mode first"));
        auto firstDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactAreaModeFirstDock",
            firstPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(firstDock);
        firstDock->toggleViewAction()->setData(QByteArray("CompactAreaModeFirstDock"));
        firstDock->toggleViewAction()->setVisible(true);

        auto secondPanel = new QWidget();
        secondPanel->setWindowTitle(QStringLiteral("Compact mode second"));
        auto secondDock = Gui::DockWindowManager::instance()->addDockWindow(
            "CompactAreaModeSecondDock",
            secondPanel,
            Qt::LeftDockWidgetArea
        );
        QVERIFY(secondDock);
        secondDock->toggleViewAction()->setData(QByteArray("CompactAreaModeSecondDock"));
        secondDock->toggleViewAction()->setVisible(true);

        preferences->SetBool("CompactJetBrainsPanelPlacementEnabled", true);
        preferences->SetBool("CompactJetBrainsLayout", true);
        mainWindow->show();
        processPendingEvents();

        auto* manager = mainWindow->findChild<Gui::PanelPlacementManager*>();
        QVERIFY(manager);
        QTRY_VERIFY(panelButtonForAssignment(QStringLiteral("CompactAreaModeFirstDock")));

        auto openContextMenu = [this](QToolButton* button) {
            const QPoint localPosition = button->rect().center();
            QContextMenuEvent contextEvent(
                QContextMenuEvent::Keyboard,
                localPosition,
                button->mapToGlobal(localPosition)
            );
            QApplication::sendEvent(button, &contextEvent);
            return mainWindow->findChild<QMenu*>(QStringLiteral("_fc_compact_panel_context_menu"));
        };
        auto areaModeMenu = [](QMenu* menu) {
            if (!menu) {
                return static_cast<QMenu*>(nullptr);
            }
            for (QAction* action : menu->actions()) {
                if (action->text() == QStringLiteral("Area Mode")) {
                    return action->menu();
                }
            }
            return static_cast<QMenu*>(nullptr);
        };
        auto findAction = [](QMenu* menu, const QString& text) {
            if (!menu) {
                return static_cast<QAction*>(nullptr);
            }
            for (QAction* action : menu->actions()) {
                if (action->text() == text) {
                    return action;
                }
            }
            return static_cast<QAction*>(nullptr);
        };

        QToolButton* button = panelButtonForAssignment(QStringLiteral("CompactAreaModeFirstDock"));
        QPointer<QMenu> menu = openContextMenu(button);
        QVERIFY(menu);
        QMenu* modeMenu = areaModeMenu(menu);
        QVERIFY(modeMenu);
        QAction* exclusiveAction = findAction(modeMenu, QStringLiteral("One Panel at a Time"));
        QAction* multipleAction = findAction(modeMenu, QStringLiteral("Multiple Panels"));
        QVERIFY(exclusiveAction);
        QVERIFY(multipleAction);
        QVERIFY(exclusiveAction->isChecked());
        QVERIFY(!multipleAction->isChecked());

        multipleAction->trigger();
        processPendingEvents();
        QCOMPARE(
            manager->persistedPlacement(QStringLiteral("CompactAreaModeFirstDock")).visibilityPolicy,
            Gui::PanelPlacement::VisibilityPolicy::Multiple
        );
        QCOMPARE(
            manager->persistedPlacement(QStringLiteral("CompactAreaModeSecondDock")).visibilityPolicy,
            Gui::PanelPlacement::VisibilityPolicy::Multiple
        );

        if (menu) {
            menu->close();
        }
        processPendingEvents();

        button = panelButtonForAssignment(QStringLiteral("CompactAreaModeFirstDock"));
        menu = openContextMenu(button);
        QVERIFY(menu);
        modeMenu = areaModeMenu(menu);
        QVERIFY(modeMenu);
        exclusiveAction = findAction(modeMenu, QStringLiteral("One Panel at a Time"));
        multipleAction = findAction(modeMenu, QStringLiteral("Multiple Panels"));
        QVERIFY(exclusiveAction);
        QVERIFY(multipleAction);
        QVERIFY(!exclusiveAction->isChecked());
        QVERIFY(multipleAction->isChecked());

        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactAreaModeFirstDock"));
        Gui::PanelPlacementStore::removePlacement(QStringLiteral("CompactAreaModeSecondDock"));
        Gui::DockWindowManager::instance()->removeDockWindow("CompactAreaModeSecondDock");
        Gui::DockWindowManager::instance()->removeDockWindow("CompactAreaModeFirstDock");
        if (menu) {
            menu->close();
        }
        processPendingEvents();
    }

    void compactShortcutDispatchesExactlyOnceWithOriginalActionsHiddenBars()  // NOLINT
    {
        if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
            QSKIP("Native shortcut activation requires a window-system focus provider.");
        }

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

        // Qt's offscreen test platform does not provide native-window focus,
        // so deliver the synthesized input directly to the intended target.
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
        QTest::keyClick(lineEdit, Qt::Key_Z, Qt::ControlModifier);
        processPendingEvents();

        QTRY_COMPARE(undoTriggers, 0);
        QCOMPARE(lineEdit->text(), QStringLiteral("base"));

        delete lineEdit;
        delete syntheticMenu;
    }

    void workbenchRefreshRemovesStaleHostedActionsAndAddsCurrent()  // NOLINT
    {
        if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
            QSKIP("Native shortcut activation requires a window-system focus provider.");
        }

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
        if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
            QSKIP("Native shortcut activation requires a window-system focus provider.");
        }

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

    QWidget* panelLaneWidget(const QString& objectName) const
    {
        return mainWindow->findChild<QWidget*>(objectName);
    }

    QToolButton* overflowButton(const QString& objectName) const
    {
        return mainWindow->findChild<QToolButton*>(objectName);
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

    QStringList laneAssignments(const QString& objectName) const
    {
        QStringList assignments;
        auto* lane = panelLaneWidget(objectName);
        if (!lane) {
            return assignments;
        }

        QList<QToolButton*> buttons;
        for (auto* button : lane->findChildren<QToolButton*>()) {
            if (button->property("_fc_compact_panel_assignment").isValid()
                && !button->property("_fc_compact_panel_overflow").toBool()) {
                buttons.push_back(button);
            }
        }
        std::sort(buttons.begin(), buttons.end(), [lane](QToolButton* left, QToolButton* right) {
            return left->mapTo(lane, QPoint()).y() < right->mapTo(lane, QPoint()).y();
        });
        for (auto* button : buttons) {
            assignments.push_back(button->property("_fc_compact_panel_assignment").toString());
        }
        return assignments;
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
