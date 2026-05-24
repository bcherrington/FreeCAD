/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <FCGlobal.h>

#include <memory>

#include <QColor>
#include <QPixmap>
#include <QWidget>

namespace Gui
{

class SplashObserver;

struct GuiExport SplashScreenOptions
{
    bool strictVerbose = false;
    bool guiRunMode = true;
    bool startHidden = false;
    bool showSplasher = true;
};

/** This widget provides a splash screen that can be shown during application startup.
 *
 * \author Werner Mayer
 */
class GuiExport SplashScreen: public QWidget
{
    Q_OBJECT

public:
    explicit SplashScreen(const QPixmap& pixmap = QPixmap(), Qt::WindowFlags f = Qt::WindowFlags());
    ~SplashScreen() override;

    void setShowMessages(bool on);
    void showMessage(
        const QString& message,
        int alignment = Qt::AlignLeft,
        const QColor& color = Qt::black
    );
    void allowDialogsToCover();
    void raiseSplash();
    void waitForPaint();

    static QPixmap splashImage();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void resizeToPixmap();

private:
    QPixmap pixmap;
    QString message;
    int messageAlignment = Qt::AlignBottom | Qt::AlignLeft;
    QColor messageColor = Qt::black;
    SplashObserver* messages;
    bool painted = false;
};

GuiExport bool shouldShowSplashScreen(const SplashScreenOptions& options);
GuiExport std::unique_ptr<SplashScreen> showSplashScreen();

}  // namespace Gui
