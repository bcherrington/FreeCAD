// SPDX-License-Identifier: LGPL-2.1-or-later

#include "DocumentCloseFeedback.h"

#include <QMessageBox>
#include <string>
#include <App/Application.h>
#include <App/Document.h>

namespace
{
void showCloseWarning(QWidget* parent, const QString& text)
{
    QMessageBox message(
        QMessageBox::Warning,
        QObject::tr("Document not closable"),
        text,
        QMessageBox::Ok,
        parent
    );
    message.setTextFormat(Qt::PlainText);
    message.exec();
}
}  // namespace

QString Gui::documentCloseMessage(const App::Document& document)
{
    if (document.isRecomputingOnCurrentThread()) {
        return QObject::tr(
                   "Cannot close \"%1\" while it is being recomputed.\n"
                   "Please try again after recompute finishes."
        )
            .arg(QString::fromUtf8(document.Label.getValue()));
    }
    return QObject::tr("The document is not closable for the moment.");
}

void Gui::showDocumentCloseWarning(QWidget* parent, const App::Document& document)
{
    showCloseWarning(parent, documentCloseMessage(document));
}

bool Gui::closeAllDocumentsWithFeedback(QWidget* parent)
{
    auto& application = App::GetApplication();
    std::string blockedDocument;
    if (application.tryCloseAllDocuments(&blockedDocument)) {
        return true;
    }
    if (const auto* document = application.getDocument(blockedDocument.c_str())) {
        showDocumentCloseWarning(parent, *document);
    }
    else {
        showCloseWarning(
            parent,
            QObject::tr(
                "Could not close document \"%1\". "
                "Some documents are still open."
            )
                .arg(QString::fromUtf8(blockedDocument.c_str()))
        );
    }
    return false;
}
