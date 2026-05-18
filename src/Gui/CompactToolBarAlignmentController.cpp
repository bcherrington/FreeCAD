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

#include <QBoxLayout>
#include <QEvent>
#include <QTimer>
#include <QWidget>

#include <algorithm>

#include "CompactToolBarContainer.h"
#include "MainWindow.h"


using namespace Gui;

CompactToolBarAlignmentController::CompactToolBarAlignmentController(MainWindow* mainWindow)
    : QObject(mainWindow)
    , mainWindow(mainWindow)
{}

CompactToolBarAlignmentController::~CompactToolBarAlignmentController()
{
    if (mainWindow) {
        mainWindow->removeEventFilter(this);
    }
    removeContainer();
}

void CompactToolBarAlignmentController::setActive(bool enabled)
{
    if (active == enabled) {
        return;
    }

    active = enabled;
    if (active) {
        createContainer();
        installCentralWrapper();
        if (mainWindow) {
            mainWindow->installEventFilter(this);
        }
        container->setActive(true);
        QTimer::singleShot(0, container, &CompactToolBarContainer::installToolBarFilters);
    }
    else {
        if (mainWindow) {
            mainWindow->removeEventFilter(this);
        }
        removeContainer();
    }
}

bool CompactToolBarAlignmentController::isActive() const
{
    return active;
}

void CompactToolBarAlignmentController::layoutContainer()
{
    if (!active || !container) {
        return;
    }

    container->setFixedHeight(reservedHeight());
}

int CompactToolBarAlignmentController::reservedHeight() const
{
    if (!container || !container->isVisible()) {
        return 0;
    }

    return std::max(container->sizeHint().height(), container->minimumHeight());
}

bool CompactToolBarAlignmentController::eventFilter(QObject* watched, QEvent* event)
{
    if (!active || watched != mainWindow) {
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
        case QEvent::ChildAdded:
            if (container) {
                QTimer::singleShot(0, container, &CompactToolBarContainer::installToolBarFilters);
            }
            break;
        case QEvent::LayoutRequest:
        case QEvent::Resize:
        case QEvent::Show:
            layoutContainer();
            break;
        default:
            break;
    }

    return QObject::eventFilter(watched, event);
}

void CompactToolBarAlignmentController::createContainer()
{
    if (!mainWindow || container) {
        return;
    }

    container = new CompactToolBarContainer(mainWindow);
    container->setFixedHeight(container->minimumHeight());
}

void CompactToolBarAlignmentController::removeContainer()
{
    if (!container) {
        removeCentralWrapper();
        return;
    }

    container->setActive(false);
    removeCentralWrapper();
    container->deleteLater();
    container = nullptr;
}

void CompactToolBarAlignmentController::installCentralWrapper()
{
    if (!mainWindow || !container || centralWrapper) {
        return;
    }

    originalCentralWidget = mainWindow->takeCentralWidget();
    if (!originalCentralWidget) {
        return;
    }

    centralWrapper = new QWidget(mainWindow);
    centralWrapper->setObjectName(QStringLiteral("_fc_compact_toolbar_central_wrapper"));
    auto layout = new QBoxLayout(QBoxLayout::TopToBottom, centralWrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(container, 0);
    layout->addWidget(originalCentralWidget, 1);
    mainWindow->setCentralWidget(centralWrapper);
    container->show();
    container->raise();
}

void CompactToolBarAlignmentController::removeCentralWrapper()
{
    if (!mainWindow || !centralWrapper) {
        return;
    }

    auto wrapper = mainWindow->takeCentralWidget();
    if (container) {
        container->setParent(nullptr);
    }
    if (originalCentralWidget) {
        originalCentralWidget->setParent(nullptr);
        mainWindow->setCentralWidget(originalCentralWidget);
    }
    if (wrapper && wrapper != originalCentralWidget) {
        wrapper->deleteLater();
    }
    centralWrapper = nullptr;
    originalCentralWidget = nullptr;
}
