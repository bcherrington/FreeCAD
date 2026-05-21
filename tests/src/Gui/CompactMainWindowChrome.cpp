// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cstdlib>
#include <memory>

#include <QAction>
#include <QCoreApplication>
#include <QMenuBar>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
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
    }

    void cleanup()  // NOLINT
    {
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

        const auto buttons = panelStripButtons();
        for (auto button : buttons) {
            if (button->toolTip().isEmpty()) {
                continue;
            }
            QVERIFY(qobject_cast<QToolBar*>(button->parentWidget()));
            auto action = button->defaultAction();
            QVERIFY(action);
            QVERIFY(action->isCheckable());
            auto toolbar = qobject_cast<QToolBar*>(button->parentWidget());
            QCOMPARE(button->iconSize(), toolbar->iconSize());
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
        auto topBar = compactTopBar();
        QVERIFY(topBar);
        QVERIFY(!topBar->isHidden());
        QVERIFY(menuBar->isHidden());

        preferences->SetBool("CompactJetBrainsLayout", false);
        QCoreApplication::processEvents();
        QVERIFY(topBar->isHidden());
        QVERIFY(!menuBar->isHidden());
        QCOMPARE(mainWindow->contentsMargins(), margins);
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

private:
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

    std::unique_ptr<Gui::Application> guiApplication;
    std::unique_ptr<Gui::MainWindow> mainWindow;
    ParameterGrp::handle preferences;
    bool compactLayoutBefore = false;
    bool framelessBefore = false;
};

QTEST_MAIN(testCompactMainWindowChrome)

#include "CompactMainWindowChrome.moc"
