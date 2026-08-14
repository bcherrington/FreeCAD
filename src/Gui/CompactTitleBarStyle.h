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

#ifndef GUI_COMPACTTITLEBARSTYLE_H
#define GUI_COMPACTTITLEBARSTYLE_H

#include <QColor>
#include <QPalette>
#include <QSize>
#include <QString>

class QToolBar;
class QToolButton;

namespace Gui::CompactTitleBarStyle
{

enum class PanelIndicatorEdge
{
    Left,
    Right,
    Top,
    Bottom
};

int iconSize();
QSize buttonSize(const QToolBar* toolbar);
int groupGap();
int tightGap();

int panelRailWidth();
int panelRailIconSize();
QSize panelButtonSize();
int panelOuterPadding();
int panelItemGap();
int panelGroupGap();
int panelActiveIndicatorThickness();
int panelHeaderHeight();
int panelHeaderControlSize();
int panelSplitterThickness();
int panelSplitterHitThickness();
int panelOverlayBoundaryThickness();
int panelOverlayElevation();
QColor panelHighlightTint(const QPalette& palette);
QColor panelOpenIndicatorColor(const QPalette& palette);
QColor panelFocusCueColor(const QPalette& palette);
QString panelButtonStyleSheet(
    const QPalette& palette,
    PanelIndicatorEdge indicatorEdge = PanelIndicatorEdge::Right
);

void applyIconButtonMetrics(QToolButton* button, const QToolBar* toolbar);
void applyMenuButtonMetrics(QToolButton* button, const QToolBar* toolbar);
void applyIconMenuButtonMetrics(QToolButton* button, const QToolBar* toolbar);
void applyPanelButtonMetrics(QToolButton* button);
void resizeMenuButton(QToolButton* button, int maximumWidth = 260);

}  // namespace Gui::CompactTitleBarStyle

#endif  // GUI_COMPACTTITLEBARSTYLE_H
