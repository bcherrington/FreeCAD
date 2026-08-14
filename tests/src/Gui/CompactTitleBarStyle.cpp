// SPDX-License-Identifier: LGPL-2.1-or-later

#include <memory>

#include <QSizePolicy>
#include <QSize>
#include <QToolBar>
#include <QToolButton>
#include <QTest>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/CompactTitleBarStyle.h>
#include <src/App/InitApplication.h>

class testCompactTitleBarStyle final: public QObject
{
    Q_OBJECT

public:
    testCompactTitleBarStyle()
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
        general = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/General"
        );
        originalToolbarIconSize = general->GetInt("ToolbarIconSize", 24);
    }

    void cleanup()  // NOLINT
    {
        if (general) {
            general->SetInt("ToolbarIconSize", originalToolbarIconSize);
        }
        general = nullptr;
    }

    void panelTokensStayStableAcrossToolbarSizes()  // NOLINT
    {
        general->SetInt("ToolbarIconSize", 12);
        QCOMPARE(Gui::CompactTitleBarStyle::iconSize(), 12);
        QCOMPARE(Gui::CompactTitleBarStyle::panelRailWidth(), 32);
        QCOMPARE(Gui::CompactTitleBarStyle::panelRailIconSize(), 18);
        QCOMPARE(Gui::CompactTitleBarStyle::panelButtonSize(), QSize(28, 28));
        QCOMPARE(Gui::CompactTitleBarStyle::panelOuterPadding(), 2);
        QCOMPARE(Gui::CompactTitleBarStyle::panelItemGap(), 2);
        QCOMPARE(Gui::CompactTitleBarStyle::panelGroupGap(), 8);
        QCOMPARE(Gui::CompactTitleBarStyle::panelActiveIndicatorThickness(), 2);
        QCOMPARE(Gui::CompactTitleBarStyle::panelHeaderHeight(), 32);
        QCOMPARE(Gui::CompactTitleBarStyle::panelHeaderControlSize(), 24);
        QCOMPARE(Gui::CompactTitleBarStyle::panelSplitterThickness(), 1);
        QCOMPARE(Gui::CompactTitleBarStyle::panelSplitterHitThickness(), 6);
        QCOMPARE(Gui::CompactTitleBarStyle::panelOverlayBoundaryThickness(), 1);
        QCOMPARE(Gui::CompactTitleBarStyle::panelOverlayElevation(), 10);

        general->SetInt("ToolbarIconSize", 40);
        QCOMPARE(Gui::CompactTitleBarStyle::iconSize(), 40);
        QCOMPARE(Gui::CompactTitleBarStyle::panelRailWidth(), 32);
        QCOMPARE(Gui::CompactTitleBarStyle::panelRailIconSize(), 18);
        QCOMPARE(Gui::CompactTitleBarStyle::panelButtonSize(), QSize(28, 28));
    }

    void panelTokensRespectMinimums()  // NOLINT
    {
        QVERIFY(Gui::CompactTitleBarStyle::panelRailIconSize() >= 16);
        QVERIFY(Gui::CompactTitleBarStyle::panelRailIconSize() <= 20);
        QVERIFY(Gui::CompactTitleBarStyle::panelButtonSize().width() >= 28);
        QVERIFY(Gui::CompactTitleBarStyle::panelButtonSize().height() >= 28);
        QVERIFY(Gui::CompactTitleBarStyle::panelOuterPadding() >= 2);
        QVERIFY(Gui::CompactTitleBarStyle::panelOuterPadding() <= 4);
        QVERIFY(Gui::CompactTitleBarStyle::panelOverlayElevation() >= 8);
        QVERIFY(Gui::CompactTitleBarStyle::panelOverlayElevation() <= 12);
        QVERIFY(
            Gui::CompactTitleBarStyle::panelSplitterHitThickness()
            >= Gui::CompactTitleBarStyle::panelSplitterThickness()
        );
    }

    void applyPanelButtonMetricsIgnoresToolbarIconSize()  // NOLINT
    {
        general->SetInt("ToolbarIconSize", 48);

        QToolBar toolbar;
        toolbar.setIconSize(QSize(48, 48));

        QToolButton button;
        button.setParent(&toolbar);
        button.setMinimumSize(QSize(1, 1));
        button.setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

        Gui::CompactTitleBarStyle::applyPanelButtonMetrics(&button);

        QCOMPARE(button.iconSize(), QSize(18, 18));
        QCOMPARE(button.minimumSize(), QSize(28, 28));
        QCOMPARE(button.maximumSize(), QSize(28, 28));
        QCOMPARE(button.sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
        QCOMPARE(button.sizePolicy().verticalPolicy(), QSizePolicy::Fixed);
    }

private:
    std::unique_ptr<Gui::Application> guiApplication;
    ParameterGrp::handle general;
    long originalToolbarIconSize = 24;
};

QTEST_MAIN(testCompactTitleBarStyle)

#include "CompactTitleBarStyle.moc"
