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

#include "PreCompiled.h"

#include "CompactTitleBarStyle.h"

#include <QColor>
#include <QString>
#include <QSizePolicy>
#include <QToolBar>
#include <QToolButton>

#include <algorithm>

#include <App/Application.h>
#include <Base/Parameter.h>

namespace Gui::CompactTitleBarStyle
{
namespace
{
constexpr int CompactGroupGap = 18;
constexpr int CompactTightGap = 8;
constexpr int CompactDropdownRightPadding = 12;
constexpr int CompactDropdownIndicatorRight = 2;
constexpr int CompactDropdownIndicatorWidth = 8;
constexpr int CompactPanelRailWidth = 32;
constexpr int CompactPanelRailIconSize = 18;
constexpr int CompactPanelButtonSize = 28;
constexpr int CompactPanelOuterPadding = 2;
constexpr int CompactPanelItemGap = 2;
constexpr int CompactPanelGroupGap = 8;
constexpr int CompactPanelActiveIndicatorThickness = 2;
constexpr int CompactPanelHeaderHeight = 32;
constexpr int CompactPanelHeaderControlSize = 24;
constexpr int CompactPanelSplitterThickness = 1;
constexpr int CompactPanelSplitterHitThickness = 6;
constexpr int CompactPanelOverlayBoundaryThickness = 1;
constexpr int CompactPanelOverlayElevation = 10;

bool isDarkPalette(const QPalette& palette)
{
    return palette.color(QPalette::Window).lightness() < 128;
}

QString toCssRgba(const QColor& color)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

QString indicatorBorderProperty(PanelIndicatorEdge indicatorEdge)
{
    switch (indicatorEdge) {
        case PanelIndicatorEdge::Left:
            return QStringLiteral("border-left");
        case PanelIndicatorEdge::Right:
            return QStringLiteral("border-right");
        case PanelIndicatorEdge::Top:
            return QStringLiteral("border-top");
        case PanelIndicatorEdge::Bottom:
            return QStringLiteral("border-bottom");
    }

    return QStringLiteral("border-right");
}

}  // namespace

int iconSize()
{
    auto hGeneral = App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/General");
    return std::max(static_cast<int>(hGeneral->GetInt("ToolbarIconSize", 24)), 5);
}

QSize buttonSize(const QToolBar* toolbar)
{
    const int size = toolbar ? toolbar->iconSize().width() : iconSize();
    return QSize(size + 4, size + 4);
}

int groupGap()
{
    return CompactGroupGap;
}

int tightGap()
{
    return CompactTightGap;
}

int panelRailWidth()
{
    return CompactPanelRailWidth;
}

int panelRailIconSize()
{
    return CompactPanelRailIconSize;
}

QSize panelButtonSize()
{
    return QSize(CompactPanelButtonSize, CompactPanelButtonSize);
}

int panelOuterPadding()
{
    return CompactPanelOuterPadding;
}

int panelItemGap()
{
    return CompactPanelItemGap;
}

int panelGroupGap()
{
    return CompactPanelGroupGap;
}

int panelActiveIndicatorThickness()
{
    return CompactPanelActiveIndicatorThickness;
}

int panelHeaderHeight()
{
    return CompactPanelHeaderHeight;
}

int panelHeaderControlSize()
{
    return CompactPanelHeaderControlSize;
}

int panelSplitterThickness()
{
    return CompactPanelSplitterThickness;
}

int panelSplitterHitThickness()
{
    return CompactPanelSplitterHitThickness;
}

int panelOverlayBoundaryThickness()
{
    return CompactPanelOverlayBoundaryThickness;
}

int panelOverlayElevation()
{
    return CompactPanelOverlayElevation;
}

QColor panelHighlightTint(const QPalette& palette)
{
    QColor tint = palette.color(QPalette::Highlight);
    tint.setAlpha(isDarkPalette(palette) ? 72 : 40);
    return tint;
}

QColor panelOpenIndicatorColor(const QPalette& palette)
{
    QColor indicator = palette.color(QPalette::Highlight);
    indicator = isDarkPalette(palette) ? indicator.lighter(125) : indicator.darker(105);
    indicator.setAlpha(230);
    return indicator;
}

QColor panelFocusCueColor(const QPalette& palette)
{
    QColor focus = palette.color(QPalette::Highlight);
    focus = isDarkPalette(palette) ? focus.lighter(150) : focus.darker(115);
    focus.setAlpha(190);
    return focus;
}

QString panelButtonStyleSheet(const QPalette& palette, PanelIndicatorEdge indicatorEdge)
{
    const QString hoverTint = toCssRgba(panelHighlightTint(palette));
    const QString checkedTint = toCssRgba(panelHighlightTint(palette));
    const QString indicator = toCssRgba(panelOpenIndicatorColor(palette));
    const QString focusCue = toCssRgba(panelFocusCueColor(palette));
    const QString indicatorBorder = indicatorBorderProperty(indicatorEdge);

    return QStringLiteral(
               "QToolButton {"
               " padding: 0px;"
               " margin: 0px;"
               " background-color: transparent;"
               " border: 2px solid transparent;"
               " border-radius: 6px;"
               "}"
               "QToolButton:hover {"
               " background-color: %1;"
               "}"
               "QToolButton:checked {"
               " background-color: %2;"
               " %3: %4px solid %5;"
               "}"
               "QToolButton:focus {"
               " border: 2px solid %6;"
               "}"
               "QToolButton:checked:focus {"
               " background-color: %2;"
               " border: 2px solid %6;"
               " %3: %4px solid %5;"
               "}"
    )
        .arg(hoverTint)
        .arg(checkedTint)
        .arg(indicatorBorder)
        .arg(panelActiveIndicatorThickness())
        .arg(indicator)
        .arg(focusCue);
}

void applyIconButtonMetrics(QToolButton* button, const QToolBar* toolbar)
{
    if (!button) {
        return;
    }

    const QSize size = toolbar ? toolbar->iconSize() : QSize(iconSize(), iconSize());
    button->setIconSize(size);
    button->setFixedSize(buttonSize(toolbar));
}

void resizeMenuButton(QToolButton* button, int maximumWidth)
{
    if (!button) {
        return;
    }

    const int minimumWidth = std::min(button->minimumSizeHint().width(), maximumWidth);
    const int width = std::clamp(button->sizeHint().width(), minimumWidth, maximumWidth);
    button->setFixedWidth(width);
}

void applyMenuButtonMetrics(QToolButton* button, const QToolBar* toolbar)
{
    if (!button) {
        return;
    }

    const QSize size = toolbar ? toolbar->iconSize() : QSize(iconSize(), iconSize());
    button->setIconSize(size);
    button->setMinimumSize(QSize(0, 0));
    button->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    button->setMinimumHeight(buttonSize(toolbar).height());
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setStyleSheet(QStringLiteral(
                              "QToolButton { padding-left: 4px; padding-right: %1px; }"
                              "QToolButton::menu-indicator {"
                              " subcontrol-position: right center;"
                              " right: %2px;"
                              " width: %3px;"
                              "}"
    )
                              .arg(CompactDropdownRightPadding)
                              .arg(CompactDropdownIndicatorRight)
                              .arg(CompactDropdownIndicatorWidth));
    resizeMenuButton(button);
}

void applyIconMenuButtonMetrics(QToolButton* button, const QToolBar* toolbar)
{
    if (!button) {
        return;
    }

    const QSize size = toolbar ? toolbar->iconSize() : QSize(iconSize(), iconSize());
    button->setIconSize(size);
    button->setFixedSize(buttonSize(toolbar));
    button->setStyleSheet(QStringLiteral("QToolButton::menu-indicator { image: none; width: 0px; }"));
}

void applyPanelButtonMetrics(QToolButton* button)
{
    if (!button) {
        return;
    }

    const QSize size = panelButtonSize();
    const QSize icon(panelRailIconSize(), panelRailIconSize());
    button->setIconSize(icon);
    button->setMinimumSize(size);
    button->setMaximumSize(size);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setAutoRaise(true);
    button->setStyleSheet(QString());
}

}  // namespace Gui::CompactTitleBarStyle
