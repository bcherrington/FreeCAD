// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Gui/SplashScreen.h"

TEST(SplashScreen, ShowsForDefaultGuiStartup)
{
    Gui::SplashScreenOptions options;

    EXPECT_TRUE(Gui::shouldShowSplashScreen(options));
}

TEST(SplashScreen, RespectsDisabledSplashPreference)
{
    Gui::SplashScreenOptions options;
    options.showSplasher = false;

    EXPECT_FALSE(Gui::shouldShowSplashScreen(options));
}

TEST(SplashScreen, RespectsStartHidden)
{
    Gui::SplashScreenOptions options;
    options.startHidden = true;

    EXPECT_FALSE(Gui::shouldShowSplashScreen(options));
}

TEST(SplashScreen, RespectsStrictVerbose)
{
    Gui::SplashScreenOptions options;
    options.strictVerbose = true;

    EXPECT_FALSE(Gui::shouldShowSplashScreen(options));
}

TEST(SplashScreen, RespectsNonGuiRunMode)
{
    Gui::SplashScreenOptions options;
    options.guiRunMode = false;

    EXPECT_FALSE(Gui::shouldShowSplashScreen(options));
}
