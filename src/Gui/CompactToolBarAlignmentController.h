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

#pragma once

#include <QPointer>
#include <QObject>

#include <FCGlobal.h>

class QEvent;
class QWidget;

namespace Gui
{

class CompactToolBarContainer;
class MainWindow;

class GuiExport CompactToolBarAlignmentController: public QObject
{
    Q_OBJECT

public:
    explicit CompactToolBarAlignmentController(MainWindow* mainWindow);
    ~CompactToolBarAlignmentController() override;

    void setActive(bool active);
    bool isActive() const;
    void layoutContainer();
    int reservedHeight() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void createContainer();
    void removeContainer();
    void installCentralWrapper();
    void removeCentralWrapper();

private:
    QPointer<MainWindow> mainWindow;
    QPointer<CompactToolBarContainer> container;
    QPointer<QWidget> centralWrapper;
    QPointer<QWidget> originalCentralWidget;
    bool active = false;
};

}  // namespace Gui
