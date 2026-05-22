// SPDX-License-Identifier: LGPL-2.1-or-later

#include "EarlySplash.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QGuiApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QScreen>
#include <QWidget>

#include <App/Application.h>

#include "SplashScreen.h"

namespace
{
Gui::EarlySplashOptions earlySplashOptionsFromConfig()
{
    Gui::EarlySplashOptions options;
    const auto& cfg = App::Application::Config();
    const auto verbose = cfg.find("Verbose");
    const auto runMode = cfg.find("RunMode");
    options.strictVerbose = verbose != cfg.end() && verbose->second == "Strict";
    options.guiRunMode = runMode == cfg.end() || runMode->second == "Gui";
    options.startHidden = cfg.find("StartHidden") != cfg.end();

    auto hGrp = App::GetApplication()
                    .GetUserParameter()
                    .GetGroup("BaseApp")
                    ->GetGroup("Preferences")
                    ->GetGroup("General");
    options.showSplasher = hGrp->GetBool("ShowSplasher", true);
    return options;
}

class EarlySplashWidget: public QWidget
{
public:
    explicit EarlySplashWidget(QPixmap pixmap)
        : pixmap(std::move(pixmap))
    {
        setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_DeleteOnClose, false);
        setAttribute(Qt::WA_NativeWindow, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAutoFillBackground(false);
        resizeToPixmap();
        setWindowTitle(QStringLiteral("FreeCAD"));
    }

    bool hasPainted() const
    {
        return painted;
    }

    void setPixmap(QPixmap newPixmap)
    {
        pixmap = std::move(newPixmap);
        painted = false;
        resizeToPixmap();
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.drawPixmap(0, 0, pixmap);
        painted = true;
    }

private:
    void resizeToPixmap()
    {
        setFixedSize(pixmap.size() / pixmap.devicePixelRatio());
        if (auto* screen = QGuiApplication::primaryScreen()) {
            move(screen->availableGeometry().center() - rect().center());
        }
    }

private:
    QPixmap pixmap;
    bool painted = false;
};

void waitForSplashPaint(EarlySplashWidget* splash)
{
    QElapsedTimer paintTimer;
    paintTimer.start();
    while (!splash->hasPainted() && paintTimer.elapsed() < 250) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 25);
    }
}
}  // namespace

namespace Gui
{

bool shouldShowEarlySplash(const EarlySplashOptions& options)
{
    return !options.strictVerbose && options.guiRunMode && !options.startHidden
        && options.showSplasher;
}

std::unique_ptr<QWidget> showEarlySplash()
{
    if (!shouldShowEarlySplash(earlySplashOptionsFromConfig())) {
        return nullptr;
    }

    QPixmap pixmap(QStringLiteral(":/icons/freecadsplash.png"));
    if (pixmap.isNull()) {
        return nullptr;
    }

    auto splash = std::make_unique<EarlySplashWidget>(pixmap);
    splash->winId();
    splash->show();
    splash->raise();
    splash->repaint();
    waitForSplashPaint(splash.get());
    return splash;
}

void updateEarlySplash(QWidget* splash)
{
    auto* earlySplash = dynamic_cast<EarlySplashWidget*>(splash);
    if (!earlySplash) {
        return;
    }

    QPixmap overlaySplash = SplashScreen::splashImage();
    if (overlaySplash.isNull()) {
        return;
    }

    earlySplash->setPixmap(overlaySplash);
    earlySplash->repaint();
    waitForSplashPaint(earlySplash);
}

}  // namespace Gui
