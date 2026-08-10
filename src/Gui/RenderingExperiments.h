/***************************************************************************
 *   Copyright (c) 2026                                                    *
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

#include <QWidget>
#include <FCGlobal.h>

namespace App
{
class Document;
}

namespace Gui
{
class View3DInventor;

namespace Dialog
{

class GuiExport RenderingExperiments: public QWidget
{
public:
    static RenderingExperiments* makeDockWidget(Gui::View3DInventor* view, App::Document* showOn);

    RenderingExperiments(Gui::View3DInventor* view, App::Document* showOn, QWidget* parent = nullptr);
    ~RenderingExperiments() override;

    void bindTo(Gui::View3DInventor* view, App::Document* showOn);

protected:
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    class Private;
    Private* d;
};

}  // namespace Dialog
}  // namespace Gui
