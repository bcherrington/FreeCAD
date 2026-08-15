// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cstdlib>
#include <memory>

#include <QAction>
#include <QCoreApplication>
#include <QEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QTabBar>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/CompactMainWindowChrome.h>
#include <Gui/MainWindow.h>
#include <src/App/InitApplication.h>

class LayoutEventCounter final: public QObject
{
public:
    void watch(QWidget* window, QWidget* tabBar)
    {
        watchedWindow = window;
        watchedTabBar = tabBar;
        if (watchedWindow) {
            watchedWindow->installEventFilter(this);
        }
        if (watchedTabBar) {
            watchedTabBar->installEventFilter(this);
        }
    }

    void reset()
    {
        mainWindowLayoutRequests = 0;
        mdiTabBarLayoutRequests = 0;
    }

    int total() const
    {
        return mainWindowLayoutRequests + mdiTabBarLayoutRequests;
    }

    int mainWindowCount() const
    {
        return mainWindowLayoutRequests;
    }

    int mdiTabBarCount() const
    {
        return mdiTabBarLayoutRequests;
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::LayoutRequest) {
            if (watched == watchedWindow) {
                ++mainWindowLayoutRequests;
            }
            else if (watched == watchedTabBar) {
                ++mdiTabBarLayoutRequests;
            }
        }

        return QObject::eventFilter(watched, event);
    }

private:
    QWidget* watchedWindow = nullptr;
    QWidget* watchedTabBar = nullptr;
    int mainWindowLayoutRequests = 0;
    int mdiTabBarLayoutRequests = 0;
};

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
        framelessBefore = preferences->GetBool("CompactJetBrainsFramelessWindow", false);
        preferences->SetBool("CompactJetBrainsLayout", true);
        preferences->SetBool("CompactJetBrainsFramelessWindow", false);
        createMainWindow();
        resetMainWindowState(true);
    }

    void cleanup()  // NOLINT
    {
        App::GetApplication().closeAllDocuments();
        processEvents();
        if (mainWindow) {
            resetMainWindowState(true);
        }
        if (preferences) {
            preferences->SetBool("CompactJetBrainsLayout", compactLayoutBefore);
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
    }

    void compactLayoutRequestConverges()  // NOLINT
    {
        createMainWindow();

        const int initialLayoutCount = chromeProperty("_fc_compact_layout_count");
        layoutEventCounter.reset();
        QCoreApplication::postEvent(mainWindow.get(), new QEvent(QEvent::LayoutRequest));
        waitForLayoutConvergence();
        const int convergedCount = layoutEventCounter.mainWindowCount();
        processEvents(5);

        QCOMPARE(chromeProperty("_fc_compact_layout_count"), initialLayoutCount);
        QCOMPARE(layoutEventCounter.mainWindowCount(), convergedCount);
        QVERIFY2(
            convergedCount <= 1,
            qPrintable(
                QStringLiteral("Expected one MainWindow LayoutRequest, got %1").arg(convergedCount)
            )
        );
    }

    void compactModeKeepsMdiTabBarStableAcrossLayoutRequests()  // NOLINT
    {
        createMainWindow();

        auto tabBar = mdiTabBar();
        QVERIFY(tabBar);
        QCOMPARE(tabBar->minimumHeight(), 0);
        QCOMPARE(tabBar->maximumHeight(), 0);
        QVERIFY(tabBar->isHidden());

        layoutEventCounter.reset();
        for (int index = 0; index < 3; ++index) {
            QCoreApplication::postEvent(mainWindow.get(), new QEvent(QEvent::LayoutRequest));
        }
        waitForLayoutConvergence();
        const int convergedCount = layoutEventCounter.mdiTabBarCount();
        processEvents(5);

        QCOMPARE(layoutEventCounter.mdiTabBarCount(), convergedCount);
        QCOMPARE(tabBar->minimumHeight(), 0);
        QCOMPARE(tabBar->maximumHeight(), 0);
        QVERIFY(tabBar->isHidden());
        QVERIFY2(
            convergedCount == 0,
            qPrintable(QStringLiteral("Stable MDI tab bar still received %1 LayoutRequest events")
                           .arg(convergedCount))
        );
    }

    void compactDocumentButtonRefreshCoalescesBursts()  // NOLINT
    {
        createMainWindow();

        auto chrome = compactChrome();
        auto button = currentDocumentButton();
        QVERIFY(chrome);
        QVERIFY(button);

        const int initialRequestCount = chromeProperty("_fc_compact_document_button_request_count");
        const int initialUpdateCount = chromeProperty("_fc_compact_document_button_update_count");
        button->setText(QStringLiteral("Stale document label"));
        button->setToolTip(QStringLiteral("Stale document label"));

        auto* firstDocument = App::GetApplication().newDocument("BurstAlpha");
        QVERIFY(firstDocument);
        firstDocument->Label.setValue("Burst Alpha");
        auto* secondDocument = App::GetApplication().newDocument("BurstBeta");
        QVERIFY(secondDocument);
        secondDocument->Label.setValue("Burst Beta");
        App::GetApplication().setActiveDocument(secondDocument);

        const int requestedBeforeProcessing = chromeProperty(
            "_fc_compact_document_button_request_count"
        );
        const int updatedBeforeProcessing = chromeProperty("_fc_compact_document_button_update_count");
        QVERIFY(requestedBeforeProcessing - initialRequestCount >= 2);
        QCOMPARE(updatedBeforeProcessing, initialUpdateCount);
        QVERIFY(chrome->property("_fc_compact_document_button_update_queued").toBool());

        waitForLayoutConvergence();
        processEvents(2);

        QCOMPARE(chromeProperty("_fc_compact_document_button_update_count") - initialUpdateCount, 1);
        button = currentDocumentButton();
        QVERIFY(button);
        QVERIFY2(
            button->text().contains(QStringLiteral("No document")),
            qPrintable(QStringLiteral("Unexpected document button text: %1").arg(button->text()))
        );
        QVERIFY2(
            button->toolTip().contains(QStringLiteral("No document")),
            qPrintable(QStringLiteral("Unexpected document button tooltip: %1").arg(button->toolTip()))
        );
        QVERIFY(!chrome->property("_fc_compact_document_button_update_queued").toBool());
    }

    void compactDocumentButtonQueuedUpdateCancelsOnDeactivate()  // NOLINT
    {
        createMainWindow();

        auto chrome = compactChrome();
        QVERIFY(chrome);
        const int initialUpdateCount = chromeProperty("_fc_compact_document_button_update_count");

        auto* document = App::GetApplication().newDocument("DeferredDeactivate");
        QVERIFY(document);
        document->Label.setValue("Deferred Deactivate");
        QVERIFY(chrome->property("_fc_compact_document_button_update_queued").toBool());

        chrome->setActive(false);
        processEvents(2);

        QVERIFY(!chrome->property("_fc_compact_document_button_update_queued").toBool());
        QCOMPARE(chromeProperty("_fc_compact_document_button_update_count"), initialUpdateCount);
    }

    void compactDisabledLayoutRequestsStayQuiescent()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        resetMainWindowState(false);

        auto chrome = compactChrome();
        QVERIFY(chrome);
        QVERIFY(!chrome->isActive());
        const int initialRequestCount = chromeProperty("_fc_compact_document_button_request_count");
        const int initialUpdateCount = chromeProperty("_fc_compact_document_button_update_count");
        QVERIFY(!chrome->property("_fc_compact_document_button_update_queued").toBool());

        layoutEventCounter.reset();
        for (int index = 0; index < 3; ++index) {
            QCoreApplication::postEvent(mainWindow.get(), new QEvent(QEvent::LayoutRequest));
        }
        waitForLayoutConvergence();
        processEvents(5);

        QCOMPARE(chromeProperty("_fc_compact_document_button_request_count"), initialRequestCount);
        QCOMPARE(chromeProperty("_fc_compact_document_button_update_count"), initialUpdateCount);
        QVERIFY(!chrome->property("_fc_compact_document_button_update_queued").toBool());
        QVERIFY2(
            layoutEventCounter.mainWindowCount() <= 3,
            qPrintable(QStringLiteral("Compact-disabled MainWindow saw %1 LayoutRequest events")
                           .arg(layoutEventCounter.mainWindowCount()))
        );
    }

    void compactModeRestoresMenuBarAndContentsMargins()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();

        auto menuBar = mainWindow->menuBar();
        auto tabBar = mdiTabBar();
        QVERIFY(menuBar);
        QVERIFY(tabBar);
        menuBar->show();
        tabBar->setVisible(true);
        tabBar->setMinimumHeight(11);
        tabBar->setMaximumHeight(23);
        const QMargins margins(7, 8, 9, 10);
        mainWindow->setContentsMargins(margins);

        preferences->SetBool("CompactJetBrainsLayout", true);
        processEvents();
        auto topBar = compactTopBar();
        QVERIFY(topBar);
        QVERIFY(!topBar->isHidden());
        QVERIFY(menuBar->isHidden());
        QCOMPARE(tabBar->minimumHeight(), 0);
        QCOMPARE(tabBar->maximumHeight(), 0);
        QVERIFY(tabBar->isHidden());

        preferences->SetBool("CompactJetBrainsLayout", false);
        processEvents();
        QVERIFY(topBar->isHidden());
        QVERIFY(!menuBar->isHidden());
        QCOMPARE(mainWindow->contentsMargins(), margins);
        QCOMPARE(tabBar->minimumHeight(), 11);
        QCOMPARE(tabBar->maximumHeight(), 23);
        QVERIFY(!tabBar->isHidden());
    }

    void compactMenuBarIsVerticallyCenteredInSwitchArea()  // NOLINT
    {
        preferences->SetBool("CompactJetBrainsLayout", false);
        createMainWindow();
        preferences->SetBool("CompactJetBrainsLayout", true);
        processEvents();

        auto menuButton = buttonWithToolTip(QStringLiteral("Show the main menu"));
        QVERIFY(menuButton);
        QTest::mouseClick(menuButton, Qt::LeftButton);
        processEvents();

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

    void compactWorkbenchActivationSettlesDeferredUpdates()  // NOLINT
    {
        createMainWindow();

        layoutEventCounter.reset();
        mainWindow->activateWorkbench(QStringLiteral("StartWorkbench"));
        waitForLayoutConvergence();
        const int convergedCount = layoutEventCounter.mainWindowCount();
        processEvents(5);

        QCOMPARE(layoutEventCounter.mainWindowCount(), convergedCount);
        QVERIFY(compactTopBar());
        QVERIFY(!compactTopBar()->isHidden());
        QVERIFY(currentDocumentButton());
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
        QTRY_VERIFY(!compactTopBar() || compactTopBar()->isHidden());
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

    void compactDocumentButtonQueuedUpdateIsSafeOnMainWindowDestroy()  // NOLINT
    {
        createMainWindow();

        QPointer<Gui::CompactMainWindowChrome> chrome = compactChrome();
        QVERIFY(chrome);

        auto* document = App::GetApplication().newDocument("DestroyQueued");
        QVERIFY(document);
        document->Label.setValue("Destroy Queued");
        QVERIFY(chrome->property("_fc_compact_document_button_update_queued").toBool());

        mainWindow.reset();
        processEvents(2);

        QVERIFY(chrome.isNull());
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
            processEvents();
            return;
        }

        mainWindow = std::make_unique<Gui::MainWindow>();
        mainWindow->resize(900, 600);
        processEvents();
        layoutEventCounter.watch(mainWindow.get(), mdiTabBar());
        if (auto chrome = compactChrome(); chrome && chrome->isActive()) {
            waitForLayoutConvergence();
        }
    }

    void resetMainWindowState(bool compactEnabled)
    {
        if (!mainWindow) {
            return;
        }

        mainWindow->hide();
        mainWindow->resize(900, 600);
        mainWindow->unsetCursor();

        if (auto chrome = compactChrome()) {
            chrome->setActive(false);
            processEvents();
            chrome->setActive(compactEnabled);
        }

        processEvents();
        layoutEventCounter.reset();
        if (compactEnabled) {
            waitForLayoutConvergence();
        }
    }

    QWidget* compactTopBar() const
    {
        return mainWindow->findChild<QWidget*>(QStringLiteral("_fc_compact_top_bar"));
    }

    Gui::CompactMainWindowChrome* compactChrome() const
    {
        return mainWindow->findChild<Gui::CompactMainWindowChrome*>();
    }

    QTabBar* mdiTabBar() const
    {
        return mainWindow ? mainWindow->findChild<QTabBar*>(QStringLiteral("mdiAreaTabBar"))
                          : nullptr;
    }

    QMenuBar* compactMenuBar() const
    {
        return mainWindow->findChild<QMenuBar*>(QStringLiteral("_fc_compact_menu_bar"));
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

    QToolButton* currentDocumentButton() const
    {
        auto topBar = compactTopBar();
        if (!topBar) {
            return nullptr;
        }

        const auto buttons = topBar->findChildren<QToolButton*>();
        for (auto button : buttons) {
            if (button->accessibleName() == QStringLiteral("Current tab")) {
                return button;
            }
        }

        return nullptr;
    }

    int chromeProperty(const char* name) const
    {
        auto chrome = compactChrome();
        return chrome ? chrome->property(name).toInt() : -1;
    }

    void processEvents(int rounds = 6) const
    {
        for (int index = 0; index < rounds; ++index) {
            QCoreApplication::sendPostedEvents(nullptr);
            QCoreApplication::processEvents();
        }
    }

    void waitForLayoutConvergence()
    {
        int stableRounds = 0;
        int lastTotal = layoutEventCounter.total();
        for (int iteration = 0; iteration < 40 && stableRounds < 3; ++iteration) {
            processEvents();
            const int total = layoutEventCounter.total();
            if (total == lastTotal) {
                ++stableRounds;
            }
            else {
                stableRounds = 0;
                lastTotal = total;
            }
        }

        QVERIFY2(
            stableRounds >= 3,
            qPrintable(QStringLiteral("Layout events did not converge; last total was %1")
                           .arg(layoutEventCounter.total()))
        );
    }

    std::unique_ptr<Gui::Application> guiApplication;
    std::unique_ptr<Gui::MainWindow> mainWindow;
    LayoutEventCounter layoutEventCounter;
    ParameterGrp::handle preferences;
    bool compactLayoutBefore = false;
    bool framelessBefore = false;
};

QTEST_MAIN(testCompactMainWindowChrome)

#include "CompactMainWindowChrome.moc"
