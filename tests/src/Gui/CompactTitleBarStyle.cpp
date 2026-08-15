// SPDX-License-Identifier: LGPL-2.1-or-later

#include <memory>

#include <QColor>
#include <QPalette>
#include <QSizePolicy>
#include <QSize>
#include <QString>
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
        QCOMPARE(
            Gui::CompactTitleBarStyle::panelRailWidth()
                - Gui::CompactTitleBarStyle::panelButtonSize().width(),
            2 * Gui::CompactTitleBarStyle::panelOuterPadding()
        );
        QCOMPARE(Gui::CompactTitleBarStyle::panelOuterPadding(), 2);
        QVERIFY(Gui::CompactTitleBarStyle::panelOverlayElevation() >= 8);
        QVERIFY(Gui::CompactTitleBarStyle::panelOverlayElevation() <= 12);
        QVERIFY(
            Gui::CompactTitleBarStyle::panelSplitterHitThickness()
            >= Gui::CompactTitleBarStyle::panelSplitterThickness()
        );
    }

    void paletteDerivedPanelColorsRespectThemeSemantics()  // NOLINT
    {
        const QPalette lightPalette = makePalette(QColor(255, 255, 255), QColor(64, 128, 255));
        const QColor lightTint = Gui::CompactTitleBarStyle::panelHighlightTint(lightPalette);
        const QColor lightIndicator = Gui::CompactTitleBarStyle::panelOpenIndicatorColor(lightPalette);
        const QColor lightFocus = Gui::CompactTitleBarStyle::panelFocusCueColor(lightPalette);
        QCOMPARE(lightTint.alpha(), 40);
        QCOMPARE(lightIndicator.alpha(), 230);
        QCOMPARE(lightFocus.alpha(), 190);
        QVERIFY(lightIndicator.lightness() < lightPalette.color(QPalette::Highlight).lightness());
        QVERIFY(lightFocus.lightness() < lightPalette.color(QPalette::Highlight).lightness());

        const QPalette darkPalette = makePalette(QColor(24, 24, 24), QColor(64, 128, 255));
        const QColor darkTint = Gui::CompactTitleBarStyle::panelHighlightTint(darkPalette);
        const QColor darkIndicator = Gui::CompactTitleBarStyle::panelOpenIndicatorColor(darkPalette);
        const QColor darkFocus = Gui::CompactTitleBarStyle::panelFocusCueColor(darkPalette);
        QCOMPARE(darkTint.alpha(), 72);
        QCOMPARE(darkIndicator.alpha(), 230);
        QCOMPARE(darkFocus.alpha(), 190);
        QVERIFY(darkIndicator.lightness() > darkPalette.color(QPalette::Highlight).lightness());
        QVERIFY(darkFocus.lightness() > darkPalette.color(QPalette::Highlight).lightness());
    }

    void panelButtonStyleSheetExposesIndicatorAndFocusCue()  // NOLINT
    {
        const QPalette palette = makePalette(QColor(255, 255, 255), QColor(64, 128, 255));
        const QString leftStyle = Gui::CompactTitleBarStyle::panelButtonStyleSheet(
            palette,
            Gui::CompactTitleBarStyle::PanelIndicatorEdge::Left
        );
        QVERIFY(leftStyle.contains(QStringLiteral("border-left: 2px solid")));
        QVERIFY(leftStyle.contains(QStringLiteral("QToolButton:focus")));
        QVERIFY(leftStyle.contains(QStringLiteral("border: 2px solid transparent")));
        QVERIFY(leftStyle.contains(QStringLiteral("border: 2px solid rgba(")));
        QVERIFY(leftStyle.contains(QStringLiteral("background-color: rgba(64, 128, 255, 40)")));

        const QString rightStyle = Gui::CompactTitleBarStyle::panelButtonStyleSheet(
            palette,
            Gui::CompactTitleBarStyle::PanelIndicatorEdge::Right
        );
        QVERIFY(rightStyle.contains(QStringLiteral("border-right: 2px solid")));
        QVERIFY(!rightStyle.contains(QStringLiteral("background-color: #555555")));
    }

    void applyPanelButtonMetricsIgnoresToolbarIconSizeAndClearsInlineStyle()  // NOLINT
    {
        general->SetInt("ToolbarIconSize", 48);

        QToolBar toolbar;
        toolbar.setIconSize(QSize(48, 48));

        QToolButton button;
        button.setParent(&toolbar);
        button.setMinimumSize(QSize(1, 1));
        button.setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        button.setStyleSheet(QStringLiteral("QToolButton { background: magenta; }"));
        button.setProperty("compactPanelIndicatorEdge", QStringLiteral("left"));

        Gui::CompactTitleBarStyle::applyPanelButtonMetrics(&button);

        QCOMPARE(button.iconSize(), QSize(18, 18));
        QCOMPARE(button.minimumSize(), QSize(28, 28));
        QCOMPARE(button.maximumSize(), QSize(28, 28));
        QCOMPARE(button.sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
        QCOMPARE(button.sizePolicy().verticalPolicy(), QSizePolicy::Fixed);
        QVERIFY(button.autoRaise());
        QVERIFY(button.styleSheet().isEmpty());
    }

private:
    static QPalette makePalette(const QColor& window, const QColor& highlight)
    {
        QPalette palette;
        palette.setColor(QPalette::Window, window);
        palette.setColor(QPalette::Base, window);
        palette.setColor(QPalette::Button, window);
        palette.setColor(QPalette::Highlight, highlight);
        return palette;
    }

    std::unique_ptr<Gui::Application> guiApplication;
    ParameterGrp::handle general;
    long originalToolbarIconSize = 24;
};

QTEST_MAIN(testCompactTitleBarStyle)

#include "CompactTitleBarStyle.moc"
