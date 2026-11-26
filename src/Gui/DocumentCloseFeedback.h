// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QString>
#include <FCGlobal.h>

class QWidget;
namespace App
{
class Document;
}

namespace Gui
{
QString GuiExport documentCloseMessage(const App::Document& document);
void GuiExport showDocumentCloseWarning(QWidget* parent, const App::Document& document);
bool GuiExport closeAllDocumentsWithFeedback(QWidget* parent);
}  // namespace Gui
