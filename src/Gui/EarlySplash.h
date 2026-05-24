// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <FCGlobal.h>

#include <memory>

class QWidget;

namespace Gui
{

struct GuiExport EarlySplashOptions
{
    bool strictVerbose = false;
    bool guiRunMode = true;
    bool startHidden = false;
    bool showSplasher = true;
};

GuiExport bool shouldShowEarlySplash(const EarlySplashOptions& options);
GuiExport std::unique_ptr<QWidget> showEarlySplash();
GuiExport void allowEarlySplashToYieldToDialogs(QWidget* splash);

}  // namespace Gui
