// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Gui/EarlySplash.h"

TEST(EarlySplash, ShowsForDefaultGuiStartup)
{
    Gui::EarlySplashOptions options;

    EXPECT_TRUE(Gui::shouldShowEarlySplash(options));
}

TEST(EarlySplash, RespectsDisabledSplashPreference)
{
    Gui::EarlySplashOptions options;
    options.showSplasher = false;

    EXPECT_FALSE(Gui::shouldShowEarlySplash(options));
}

TEST(EarlySplash, RespectsStartHidden)
{
    Gui::EarlySplashOptions options;
    options.startHidden = true;

    EXPECT_FALSE(Gui::shouldShowEarlySplash(options));
}

TEST(EarlySplash, RespectsStrictVerbose)
{
    Gui::EarlySplashOptions options;
    options.strictVerbose = true;

    EXPECT_FALSE(Gui::shouldShowEarlySplash(options));
}

TEST(EarlySplash, RespectsNonGuiRunMode)
{
    Gui::EarlySplashOptions options;
    options.guiRunMode = false;

    EXPECT_FALSE(Gui::shouldShowEarlySplash(options));
}
