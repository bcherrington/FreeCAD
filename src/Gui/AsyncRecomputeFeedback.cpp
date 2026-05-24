// SPDX-License-Identifier: LGPL-2.1-or-later

#include "AsyncRecomputeFeedback.h"

#include <QProgressDialog>
#include <QPushButton>

#include <App/Application.h>

using namespace Gui;

AsyncRecomputeCancelResult Gui::cancelQueuedAsyncRecomputeFeedback(
    const std::string& documentName,
    QProgressDialog& progress,
    QPushButton& cancelButton,
    const AsyncRecomputeStatusMessage& showStatusMessage
)
{
    const std::size_t canceledRequests
        = App::GetApplication().cancelQueuedRecomputeRequestsForDocument(documentName);

    cancelButton.setEnabled(false);
    if (canceledRequests > 0) {
        const auto message = QObject::tr("Canceling recompute…");
        progress.setLabelText(message);
        if (showStatusMessage) {
            showStatusMessage(message, 0);
        }
        return {
            AsyncRecomputeCancelResult::Status::CanceledQueuedRequest,
            canceledRequests,
        };
    }

    const auto message = QObject::tr("Recompute is already running…");
    progress.setLabelText(message);
    if (showStatusMessage) {
        showStatusMessage(message, 3000);
    }
    return {
        AsyncRecomputeCancelResult::Status::AlreadyRunning,
        canceledRequests,
    };
}
