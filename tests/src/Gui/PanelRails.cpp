// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cstdlib>
#include <memory>
#include <string>

#include <QAction>
#include <QCoreApplication>
#include <QDockWidget>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <src/App/InitApplication.h>

class testPanelRails final: public QObject
{
    Q_OBJECT

public:
    testPanelRails()
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
        panelRailsBefore = preferences->GetBool("PanelRailsEnabled", false);
        preferences->SetBool("PanelRailsEnabled", true);
    }

    void cleanup()  // NOLINT
    {
        if (preferences) {
            preferences->SetBool("PanelRailsEnabled", panelRailsBefore);
        }
        preferences = nullptr;
    }

    void panelRailsRestoreContentsMargins()  // NOLINT
    {
        preferences->SetBool("PanelRailsEnabled", false);
        createMainWindow();

        const QMargins margins(7, 8, 9, 10);
        mainWindow->setContentsMargins(margins);

        preferences->SetBool("PanelRailsEnabled", true);
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();

        preferences->SetBool("PanelRailsEnabled", false);
        QCoreApplication::processEvents();
        QCOMPARE(mainWindow->contentsMargins(), margins);
    }

    void panelRailButtonsFitWithinRail()  // NOLINT
    {
        preferences->SetBool("PanelRailsEnabled", false);
        createMainWindow();
        preferences->SetBool("PanelRailsEnabled", true);
        QCoreApplication::processEvents();

        const QStringList stripNames {
            QStringLiteral("_fc_panel_rail_leftContent"),
            QStringLiteral("_fc_panel_rail_rightContent"),
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

    void panelSlotPreferenceOverridesDefaultRail()  // NOLINT
    {
        preferences->SetBool("PanelRailsEnabled", false);
        createMainWindow();
        auto panel = new QWidget();
        panel->setObjectName(QStringLiteral("PanelRailSlotTestPanel"));
        panel->setWindowTitle(QStringLiteral("Panel rail slot test"));
        auto dock = Gui::DockWindowManager::instance()
                        ->addDockWindow("PanelRailSlotTestDock", panel, Qt::RightDockWidgetArea);
        QVERIFY(dock);
        dock->toggleViewAction()->setData(QByteArray("PanelRailSlotTestDock"));
        dock->toggleViewAction()->setVisible(true);

        auto slots = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/MainWindow/PanelRailsSlots"
        );
        const std::string previousSlot = slots->GetASCII("PanelRailSlotTestDock", "");
        slots->RemoveASCII("PanelRailSlotTestDock");

        preferences->SetBool("PanelRailsEnabled", true);
        QCoreApplication::processEvents();

        auto leftStrip = mainWindow->findChild<QWidget*>(QStringLiteral("_fc_panel_rail_leftContent"));
        auto rightStrip = mainWindow->findChild<QWidget*>(
            QStringLiteral("_fc_panel_rail_rightContent")
        );
        QVERIFY(leftStrip);
        QVERIFY(rightStrip);

        auto button = panelButtonForAssignment(QStringLiteral("PanelRailSlotTestDock"));
        QVERIFY(button);
        const QString assignmentId = button->property("_fc_panel_rail_assignment").toString();
        QVERIFY(!assignmentId.isEmpty());
        const QString overrideSlot = QStringLiteral("right-upper");

        slots->SetASCII(assignmentId.toUtf8().constData(), overrideSlot.toUtf8().constData());

        preferences->SetBool("PanelRailsEnabled", false);
        QCoreApplication::processEvents();
        preferences->SetBool("PanelRailsEnabled", true);
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
        Gui::DockWindowManager::instance()->removeDockWindow("PanelRailSlotTestDock");
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

    QList<QToolButton*> panelStripButtons() const
    {
        QList<QToolButton*> buttons;
        const QStringList stripNames {
            QStringLiteral("_fc_panel_rail_leftContent"),
            QStringLiteral("_fc_panel_rail_rightContent"),
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
            if (button->property("_fc_panel_rail_assignment").toString() == assignmentId) {
                match = button;
            }
        }

        return match;
    }

    std::unique_ptr<Gui::Application> guiApplication;
    std::unique_ptr<Gui::MainWindow> mainWindow;
    ParameterGrp::handle preferences;
    bool panelRailsBefore = false;
};

QTEST_MAIN(testPanelRails)

#include "PanelRails.moc"
