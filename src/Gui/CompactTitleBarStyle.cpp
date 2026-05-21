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

}  // namespace Gui::CompactTitleBarStyle
