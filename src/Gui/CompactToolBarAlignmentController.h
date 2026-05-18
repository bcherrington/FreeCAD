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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QPointer>

#include <Base/Parameter.h>
#include <FCGlobal.h>

class QEvent;
class QToolBar;

namespace Gui
{

class MainWindow;

class GuiExport CompactToolBarAlignmentController: public QObject
{
    Q_OBJECT

public:
    explicit CompactToolBarAlignmentController(MainWindow* mainWindow);
    ~CompactToolBarAlignmentController() override;

    void setActive(bool active);
    bool isActive() const;
    void syncToolBars();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    enum class Alignment
    {
        Left,
        Center,
        Right,
    };

    void installToolBarFilters();
    void removeToolBarFilters();
    void recordToolBarAlignment(QToolBar* toolBar);
    Alignment alignmentForToolBar(const QToolBar* toolBar) const;
    QByteArray parameterKey(const QToolBar* toolBar) const;
    const char* parameterValue(Alignment alignment) const;
    Alignment alignmentFromParameter(const char* value) const;

private:
    QPointer<MainWindow> mainWindow;
    ParameterGrp::handle hAlignment;
    QHash<QToolBar*, Alignment> knownAlignments;
    bool active = false;
};

}  // namespace Gui
