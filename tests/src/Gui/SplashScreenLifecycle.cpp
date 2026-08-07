// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QPixmap>
#include <QTest>

#include <src/App/InitApplication.h>

#include "Gui/SplashScreen.h"

class SplashScreenLifecycleTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
    }

    void yieldsToDialogsAndCanBeRaisedAgain()
    {
        QPixmap image(64, 48);
        image.fill(Qt::blue);
        Gui::SplashScreen splash(image);

        QVERIFY(splash.windowFlags().testFlag(Qt::WindowStaysOnTopHint));

        splash.raiseSplash();
        QVERIFY(splash.isVisible());
        QCOMPARE(splash.size(), image.size());

        splash.allowDialogsToCover();
        QVERIFY(splash.isVisible());
        QVERIFY(!splash.windowFlags().testFlag(Qt::WindowStaysOnTopHint));

        splash.raiseSplash();
        QVERIFY(splash.isVisible());

        splash.close();
        QVERIFY(!splash.isVisible());
    }
};

QTEST_MAIN(SplashScreenLifecycleTest)

#include "SplashScreenLifecycle.moc"
