// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>

class QWidget;

namespace Gui
{

std::unique_ptr<QWidget> showEarlySplash();
void updateEarlySplash(QWidget* splash);

}  // namespace Gui
