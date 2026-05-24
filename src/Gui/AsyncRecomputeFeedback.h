// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstddef>
#include <functional>
#include <string>

#include <FCGlobal.h>

class QProgressDialog;
class QPushButton;
class QString;

namespace Gui
{

struct GuiExport AsyncRecomputeCancelResult
{
    enum class Status
    {
        CanceledQueuedRequest,
        AlreadyRunning
    };

    Status status;
    std::size_t canceledRequests;
};

using AsyncRecomputeStatusMessage = std::function<void(const QString& message, int timeout)>;

GuiExport AsyncRecomputeCancelResult cancelQueuedAsyncRecomputeFeedback(
    const std::string& documentName,
    QProgressDialog& progress,
    QPushButton& cancelButton,
    const AsyncRecomputeStatusMessage& showStatusMessage = {}
);

}  // namespace Gui
