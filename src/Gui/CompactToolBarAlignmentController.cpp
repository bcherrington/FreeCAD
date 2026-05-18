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


#include "CompactToolBarAlignmentController.h"

#include <QEvent>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QTimer>
#include <QToolBar>

#include <App/Application.h>

#include "MainWindow.h"


using namespace Gui;

CompactToolBarAlignmentController::CompactToolBarAlignmentController(MainWindow* mainWindow)
    : QObject(mainWindow)
    , mainWindow(mainWindow)
{
    hAlignment = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/MainWindow/CompactToolBarAlignment"
    );
}

CompactToolBarAlignmentController::~CompactToolBarAlignmentController()
{
    removeToolBarFilters();
}

void CompactToolBarAlignmentController::setActive(bool enabled)
{
    if (active == enabled) {
        return;
    }

    active = enabled;
    if (active) {
        installToolBarFilters();
        QTimer::singleShot(0, this, &CompactToolBarAlignmentController::syncToolBars);
    }
    else {
        removeToolBarFilters();
        knownAlignments.clear();
    }
}

bool CompactToolBarAlignmentController::isActive() const
{
    return active;
}

void CompactToolBarAlignmentController::syncToolBars()
{
    if (!active || !mainWindow) {
        return;
    }

    installToolBarFilters();
    const auto toolBars = mainWindow->findChildren<QToolBar*>();
    for (auto toolBar : toolBars) {
        if (!toolBar || toolBar->isFloating()
            || mainWindow->toolBarArea(toolBar) != Qt::TopToolBarArea) {
            continue;
        }

        const auto key = parameterKey(toolBar);
        if (key.isEmpty()) {
            continue;
        }

        const auto value = hAlignment->GetASCII(key.constData(), "left");
        const auto alignment = alignmentFromParameter(value.c_str());
        knownAlignments.insert(toolBar, alignment);
    }
}

bool CompactToolBarAlignmentController::eventFilter(QObject* watched, QEvent* event)
{
    if (!active || !mainWindow) {
        return QObject::eventFilter(watched, event);
    }

    if (watched == mainWindow && event->type() == QEvent::ChildAdded) {
        QTimer::singleShot(0, this, &CompactToolBarAlignmentController::installToolBarFilters);
        return QObject::eventFilter(watched, event);
    }

    auto toolBar = qobject_cast<QToolBar*>(watched);
    if (!toolBar) {
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
        case QEvent::MouseButtonRelease:
        case QEvent::Move:
        case QEvent::Show:
            QPointer<QToolBar> guardedToolBar(toolBar);
            QTimer::singleShot(0, this, [this, guardedToolBar]() {
                auto toolBar = guardedToolBar.data();
                if (toolBar) {
                    recordToolBarAlignment(toolBar);
                }
            });
            break;
        default:
            break;
    }

    return QObject::eventFilter(watched, event);
}

void CompactToolBarAlignmentController::installToolBarFilters()
{
    if (!mainWindow) {
        return;
    }

    mainWindow->installEventFilter(this);

    const auto toolBars = mainWindow->findChildren<QToolBar*>();
    for (auto toolBar : toolBars) {
        if (!toolBar) {
            continue;
        }

        toolBar->installEventFilter(this);
    }
}

void CompactToolBarAlignmentController::removeToolBarFilters()
{
    if (mainWindow) {
        mainWindow->removeEventFilter(this);
    }

    const auto toolBars = mainWindow ? mainWindow->findChildren<QToolBar*>() : QList<QToolBar*>();
    for (auto toolBar : toolBars) {
        if (toolBar) {
            toolBar->removeEventFilter(this);
        }
    }
}

void CompactToolBarAlignmentController::recordToolBarAlignment(QToolBar* toolBar)
{
    if (!active || !mainWindow || !toolBar || toolBar->isFloating()
        || mainWindow->toolBarArea(toolBar) != Qt::TopToolBarArea) {
        return;
    }

    const auto key = parameterKey(toolBar);
    if (key.isEmpty()) {
        return;
    }

    const auto alignment = alignmentForToolBar(toolBar);
    knownAlignments.insert(toolBar, alignment);
    hAlignment->SetASCII(key.constData(), parameterValue(alignment));
}

CompactToolBarAlignmentController::Alignment CompactToolBarAlignmentController::alignmentForToolBar(
    const QToolBar* toolBar
) const
{
    if (!mainWindow || !toolBar) {
        return Alignment::Left;
    }

    const QRect mainRect(mainWindow->mapToGlobal(QPoint(0, 0)), mainWindow->size());
    const QPoint toolbarCenter = toolBar->mapToGlobal(toolBar->rect().center());
    const int third = mainRect.width() / 3;
    const int relativeX = toolbarCenter.x() - mainRect.left();

    if (relativeX < third) {
        return Alignment::Left;
    }
    if (relativeX >= 2 * third) {
        return Alignment::Right;
    }
    return Alignment::Center;
}

QByteArray CompactToolBarAlignmentController::parameterKey(const QToolBar* toolBar) const
{
    if (!toolBar || toolBar->objectName().isEmpty()) {
        return {};
    }

    return toolBar->objectName().toUtf8();
}

const char* CompactToolBarAlignmentController::parameterValue(Alignment alignment) const
{
    switch (alignment) {
        case Alignment::Center:
            return "center";
        case Alignment::Right:
            return "right";
        case Alignment::Left:
        default:
            return "left";
    }
}

CompactToolBarAlignmentController::Alignment CompactToolBarAlignmentController::alignmentFromParameter(
    const char* value
) const
{
    const QByteArray alignment(value ? value : "");
    if (alignment == "center") {
        return Alignment::Center;
    }
    if (alignment == "right") {
        return Alignment::Right;
    }
    return Alignment::Left;
}
