// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include <boost/core/ignore_unused.hpp>
#include <QAction>
#include <QCoreApplication>
#include <QMessageBox>


#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObserver.h>
#include <App/private/RecomputeAdmission.h>
#include <Base/Console.h>
#include <Gui/AsyncPreviewSession.h>
#include <Gui/Application.h>
#include <Gui/AsyncRecomputeProgressDialog.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Tools.h>
#include <Gui/Widgets.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/ShapeBinder.h>

#include "ui_TaskShapeBinder.h"
#include "TaskShapeBinder.h"
#include "TaskFeatureParameters.h"
#include "DeferredDialogRejectUtils.h"


using namespace PartDesignGui;
using namespace Gui;

struct PartDesignGui::TaskShapeBinder::SupportCandidate
{
    bool clear;
    App::RecomputeRequest target;
    std::vector<std::string> subelements;
};

/* TRANSLATOR PartDesignGui::TaskShapeBinder */

namespace
{

Gui::AsyncRecomputeDialogOptions shapeBinderRollbackDialogOptions()
{
    Gui::AsyncRecomputeDialogOptions options;
    options.showDelayMs = 450;
    options.cancelable = false;
    options.dynamicLabel = false;
    options.forceIndeterminate = true;
    return options;
}

}  // namespace

//**************************************************************************
//**************************************************************************
// TaskShapeBinder
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskShapeBinder::TaskShapeBinder(ViewProviderShapeBinder* view, bool newObj, QWidget* parent)
    : Gui::TaskView::TaskBox(
          Gui::BitmapFactory().pixmap("PartDesign_ShapeBinder"),
          tr("Shape Binder Parameters"),
          true,
          parent
      )
    , SelectionObserver(view)
    , ui(new Ui_TaskShapeBinder)
    , vp(view)
{
    boost::ignore_unused(newObj);

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);
    setupButtonGroup();
    setupContextMenu();
    this->groupLayout()->addWidget(proxy);

    AsyncPreviewController::Callbacks callbacks;
    callbacks.makeRequest = [this]() {
        auto* object = vp.expired() ? nullptr : vp->getObject<PartDesign::ShapeBinder>();
        return object ? App::RecomputeRequest::fromDocumentObject(*object) : App::RecomputeRequest {};
    };
    callbacks.runSyncWithResult = [this]() -> App::RecomputeResult {
        if (!vp.expired()) {
            if (auto* binder = vp->getObject<PartDesign::ShapeBinder>()) {
                return Gui::recomputeDocumentObjectWithResult(binder);
            }
        }
        return Gui::recomputeDocumentObjectWithResult(nullptr);
    };
    asyncPreviewSession = std::make_unique<Gui::AsyncPreviewSession>(std::move(callbacks), this);
    connect(
        asyncPreviewSession->controller(),
        &AsyncPreviewController::recomputeSettled,
        this,
        &TaskShapeBinder::onPreviewRecomputeSettled
    );
    asyncPreviewSession->bindWidgetsWithCancelAction(
        {
            ui->previewStatusWidget,
            ui->progressBarPreview,
            ui->labelPreviewStatus,
            ui->buttonCancelPreview,
        },
        [](const char* text) { return TaskShapeBinder::tr(text); },
        proxy,
        [this] { stopPendingRecompute(); }
    );

    updateUI();
}

TaskShapeBinder::~TaskShapeBinder()
{
    closing = true;
    stopPendingRecompute();
}

void TaskShapeBinder::updateUI()
{
    Gui::Document* doc = vp->getDocument();

    // add initial values
    App::GeoFeature* obj = nullptr;
    std::vector<std::string> subs;

    PartDesign::ShapeBinder::getFilteredReferences(
        &vp->getObject<PartDesign::ShapeBinder>()->Support,
        obj,
        subs
    );

    if (obj) {
        ui->baseEdit->setText(QString::fromStdString(obj->Label.getStrValue()));
    }

    // Allow one to clear the Support
    ui->baseEdit->setClearButtonEnabled(true);
    connect(ui->baseEdit, &QLineEdit::textChanged, this, &TaskShapeBinder::supportChanged);

    for (const auto& sub : subs) {
        ui->listWidgetReferences->addItem(QString::fromStdString(sub));
    }

    if (obj) {
        auto* svp = doc->getViewProvider(obj);
        if (svp) {
            svp->setVisible(true);
        }
    }
}

void TaskShapeBinder::setupButtonGroup()
{
    buttonGroup = new ButtonGroup(this);
    buttonGroup->setExclusive(true);

    buttonGroup->addButton(ui->buttonRefAdd, TaskShapeBinder::refAdd);
    buttonGroup->addButton(ui->buttonRefRemove, TaskShapeBinder::refRemove);
    buttonGroup->addButton(ui->buttonBase, TaskShapeBinder::refObjAdd);
    connect(
        buttonGroup,
        qOverload<QAbstractButton*, bool>(&QButtonGroup::buttonToggled),
        this,
        &TaskShapeBinder::onButtonToggled
    );
}

void TaskShapeBinder::setupContextMenu()
{
    // Create context menu
    QAction* remove = new QAction(tr("Remove"), this);
    remove->setShortcut(Gui::QtTools::deleteKeySequence());
    remove->setShortcutContext(Qt::WidgetShortcut);
    // display shortcut behind the context menu entry
    remove->setShortcutVisibleInContextMenu(true);
    ui->listWidgetReferences->addAction(remove);
    connect(remove, &QAction::triggered, this, &TaskShapeBinder::deleteItem);
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void TaskShapeBinder::supportChanged(const QString& text)
{
    if (!vp.expired() && text.isEmpty()) {
        setSupportCandidate(nullptr, {});
        ui->listWidgetReferences->clear();
        clearButtons();
        exitSelectionMode();
        requestRecompute(/*waitForCompletion=*/false);
    }
}

void TaskShapeBinder::onButtonToggled(QAbstractButton* button, bool checked)
{
    int id = buttonGroup->id(button);

    if (checked) {
        Gui::Selection().clearSelection();
        selectionMode = static_cast<TaskShapeBinder::selectionModes>(id);
    }
    else {
        Gui::Selection().clearSelection();
        if (selectionMode == static_cast<TaskShapeBinder::selectionModes>(id)) {
            selectionMode = TaskShapeBinder::none;
        }
    }

    if (!vp.expired()) {
        const bool highlight = checked
            && (id == TaskShapeBinder::refAdd || id == TaskShapeBinder::refRemove);
        vp->highlightReferences(highlight);
    }
}

void TaskShapeBinder::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
        updateRecomputeUi();
    }
}

void TaskShapeBinder::deleteItem()
{
    if (vp.expired()) {
        return;
    }

    // Delete the selected spine
    int row = ui->listWidgetReferences->currentRow();
    QListWidgetItem* item = ui->listWidgetReferences->takeItem(row);
    if (item) {
        QByteArray data = item->text().toUtf8();
        delete item;

        // search inside the list of sub-elements
        App::GeoFeature* obj = nullptr;
        std::vector<std::string> subs;

        if (!supportReferences(obj, subs)) {
            return;
        }

        const std::string subname = data.constData();

        // if something was found, delete it and update the support
        if (const auto it = std::ranges::find(subs, subname); it != subs.end()) {
            subs.erase(it);
            setSupportCandidate(obj, subs);

            clearButtons();
            exitSelectionMode();
            requestRecompute(/*waitForCompletion=*/false);
        }
    }
}

void TaskShapeBinder::removeFromListWidget(QListWidget* widget, QString itemstr)
{
    QList<QListWidgetItem*> items = widget->findItems(itemstr, Qt::MatchExactly);
    if (!items.empty()) {
        for (auto item : items) {
            QListWidgetItem* it = widget->takeItem(widget->row(item));
            delete it;
        }
    }
}

void TaskShapeBinder::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    auto setObjectLabel = [this](const Gui::SelectionChanges& msg) {
        App::DocumentObject* obj = msg.Object.getObject();
        if (obj) {
            ui->baseEdit->setText(QString::fromStdString(obj->Label.getStrValue()));
        }
    };

    if (selectionMode == none) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString sub = QString::fromUtf8(msg.pSubName);
                if (!sub.isEmpty()) {
                    ui->listWidgetReferences->addItem(sub);
                }

                setObjectLabel(msg);
            }
            else if (selectionMode == refRemove) {
                QString sub = QString::fromUtf8(msg.pSubName);
                if (!sub.isEmpty()) {
                    removeFromListWidget(ui->listWidgetReferences, sub);
                }
            }
            else if (selectionMode == refObjAdd) {
                ui->listWidgetReferences->clear();
                setObjectLabel(msg);
            }

            requestRecompute(/*waitForCompletion=*/false);
        }

        clearButtons();
        exitSelectionMode();
    }
}

bool TaskShapeBinder::referenceSelected(const SelectionChanges& msg)
{
    if (vp.expired()) {
        return false;
    }

    if ((msg.Type == Gui::SelectionChanges::AddSelection)
        && ((selectionMode == refAdd) || (selectionMode == refRemove)
            || (selectionMode == refObjAdd))) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0) {
            return false;
        }

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();
        if (strcmp(msg.pObjectName, fname) == 0) {
            return false;
        }

        // change the references
        std::string subName(msg.pSubName);

        Part::Feature* selectedObj = nullptr;
        App::GeoFeature* obj = nullptr;
        std::vector<std::string> refs;

        // A new object selection replaces the old candidate independently of
        // whether its original target was meanwhile retired or name-reused.
        if (selectionMode != refObjAdd && !supportReferences(obj, refs)) {
            return false;
        }

        // get selected object
        auto docObj = vp->getObject()->getDocument()->getObject(msg.pObjectName);
        if (docObj && docObj->isDerivedFrom<Part::Feature>()) {
            selectedObj = static_cast<Part::Feature*>(docObj);
        }

        // ensure we have a valid object
        if (!selectedObj) {
            return false;
        }
        if (!obj) {
            // Support has not been set before
            obj = selectedObj;
        }

        if (selectionMode != refObjAdd) {
            // ensure the new selected subref belongs to the same object
            if (strcmp(msg.pObjectName, obj->getNameInDocument()) != 0) {
                return false;
            }

            const auto f = std::ranges::find(refs, subName);

            if (selectionMode == refAdd) {
                if (f == refs.end()) {
                    refs.push_back(subName);
                }
                else {
                    return false;  // duplicate selection
                }
            }
            else {
                if (f != refs.end()) {
                    refs.erase(f);
                }
                else {
                    return false;
                }
            }
        }
        else {
            // change object
            refs.clear();
            obj = selectedObj;
        }

        setSupportCandidate(obj, refs);

        return true;
    }

    return false;
}

void TaskShapeBinder::clearButtons()
{

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
    ui->buttonBase->setChecked(false);
}

void TaskShapeBinder::exitSelectionMode()
{
    selectionMode = none;
    Gui::Selection().clearSelection();
    if (!vp.expired()) {
        vp->highlightReferences(false);
    }
}

void TaskShapeBinder::accept()
{
    if (vp.expired()) {
        return;
    }

    std::string label = ui->baseEdit->text().toStdString();
    PartDesign::ShapeBinder* binder = vp->getObject<PartDesign::ShapeBinder>();
    if (!binder->Support.getValue() && !label.empty()) {
        App::DocumentObject* support = binder->getDocument()->getObject(label.c_str());
        if (!support) {
            const auto objects = binder->getDocument()->findObjects(
                App::DocumentObject::getClassTypeId(),
                nullptr,
                label.c_str()
            );
            if (!objects.empty()) {
                support = objects.front();
            }
        }

        if (!support) {
            return;
        }

        auto mode = selectionMode;
        selectionMode = refObjAdd;
        SelectionChanges msg(
            SelectionChanges::AddSelection,
            binder->getDocument()->getName(),
            support->getNameInDocument()
        );
        referenceSelected(msg);
        selectionMode = mode;
        requestRecompute(/*waitForCompletion=*/false);
    }
}

bool TaskShapeBinder::hasOutstandingRecompute() const
{
    return asyncPreviewSession && asyncPreviewSession->hasOutstandingRecompute();
}

bool TaskShapeBinder::canReuseAcceptedPreviewResult() const
{
    return !pendingSupport && asyncPreviewSession && asyncPreviewSession->didLastRecomputeSucceed();
}

void TaskShapeBinder::setDeferredClosePending(bool pending)
{
    if (asyncPreviewSession) {
        asyncPreviewSession->setDeferredClosePending(pending);
    }
}

Gui::AsyncInlineRecomputeProgressTarget TaskShapeBinder::makeAcceptedRecomputeProgressTarget(
    QDialogButtonBox* dialogButtonBox,
    const QString& statusText
)
{
    if (!asyncPreviewSession) {
        return {};
    }

    return asyncPreviewSession->makeInlineRecomputeProgressTarget(this, dialogButtonBox, statusText);
}

void TaskShapeBinder::clearInteractiveSelection()
{
    clearButtons();
    exitSelectionMode();
}

void TaskShapeBinder::updateRecomputeUi()
{
    if (!ui || !asyncPreviewSession) {
        return;
    }
    asyncPreviewSession->updateUi();
}

void TaskShapeBinder::setSupportCandidate(
    App::DocumentObject* target,
    const std::vector<std::string>& subelements
)
{
    if (closing) {
        return;
    }
    ++supportActivitySerial;
    previewReplayStopped = false;
    pendingSupport = std::make_shared<SupportCandidate>(SupportCandidate {
        !target,
        target ? App::RecomputeRequest::fromDocumentObject(*target) : App::RecomputeRequest {},
        subelements
    });
    if (!hasOutstandingRecompute() && !settlingAcceptedSupport) {
        applySupportCandidate();
    }
}

bool TaskShapeBinder::supportReferences(App::GeoFeature*& object, std::vector<std::string>& subs) const
{
    if (pendingSupport) {
        object = pendingSupport->clear
            ? nullptr
            : dynamic_cast<App::GeoFeature*>(pendingSupport->target.resolveDocumentObject());
        if (!pendingSupport->clear && !object) {
            return false;
        }
        subs = pendingSupport->subelements;
        return true;
    }
    if (vp.expired()) {
        return false;
    }
    PartDesign::ShapeBinder::getFilteredReferences(
        &vp->getObject<PartDesign::ShapeBinder>()->Support,
        object,
        subs
    );
    return true;
}

bool TaskShapeBinder::applySupportCandidate()
{
    auto candidate = pendingSupport;
    if (!candidate) {
        return true;
    }
    if (vp.expired()) {
        return false;
    }
    auto* target = candidate->clear ? nullptr : candidate->target.resolveDocumentObject();
    if (!candidate->clear && !target) {
        return false;
    }
    QPointer<TaskShapeBinder> self(this);
    PreviewRecomputeGuard guard(vp->getObject(), this, vp.get());
    try {
        vp->getObject<PartDesign::ShapeBinder>()->Support.setValue(target, candidate->subelements);
    }
    catch (const App::RecomputeDetail::AdmissionRefusal&) {
        // The candidate remains available for a later owner-thread attempt.
        return false;
    }
    if (!self || !guard.isLive()) {
        return false;
    }
    // Preserve independently staged input arriving through native notifications.
    if (self->pendingSupport == candidate) {
        self->pendingSupport.reset();
    }
    return !self->pendingSupport;
}

void TaskShapeBinder::onPreviewRecomputeSettled()
{
    QPointer<TaskShapeBinder> self(this);
    if (pendingSupport && !settlingAcceptedSupport && !previewReplayStopped && !closing) {
        if (applySupportCandidate() && self) {
            requestRecompute(false);
        }
    }
    if (self && !self->closing) {
        Q_EMIT self->recomputeSettled();
    }
}

void TaskShapeBinder::flushPendingRecompute()
{
    if (asyncPreviewSession) {
        asyncPreviewSession->flushPendingRecompute();
    }
}

bool TaskShapeBinder::stopPendingRecomputeAndWait()
{
    PreviewRecomputeGuard guard(
        vp.expired() ? nullptr : vp->getObject(),
        this,
        vp.expired() ? nullptr : vp.operator->()
    );
    if (!guard.isLive()) {
        return false;
    }
    QPointer<TaskShapeBinder> self(this);
    const auto originalActivity = supportActivitySerial;
    const bool previous = settlingAcceptedSupport;
    settlingAcceptedSupport = true;
    bool settled;
    try {
        auto* session = asyncPreviewSession.get();
        settled = !session || session->stopPendingRecomputeAndWait();
    }
    catch (...) {
        if (self) {
            self->settlingAcceptedSupport = previous;
        }
        throw;
    }
    if (!self) {
        return false;
    }
    self->settlingAcceptedSupport = previous;
    return settled && guard.isLive() && self->supportActivitySerial == originalActivity;
}

void TaskShapeBinder::stopPendingRecompute()
{
    // Cancel Preview retains the visible candidate for explicit OK, but a
    // canceled run must not autonomously install it or submit more work.
    previewReplayStopped = true;
    if (asyncPreviewSession) {
        asyncPreviewSession->stopPendingRecompute();
    }
}

void TaskShapeBinder::requestRecompute(bool waitForCompletion)
{
    if (closing || previewReplayStopped) {
        return;
    }
    // An accepted native run is computing the previous immutable support. Its
    // terminal callback will submit the latest staged candidate once retired.
    if (pendingSupport) {
        if (hasOutstandingRecompute() || settlingAcceptedSupport) {
            return;
        }
        if (!applySupportCandidate()) {
            return;
        }
    }
    if (asyncPreviewSession) {
        asyncPreviewSession->requestRecompute(waitForCompletion);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDlgShapeBinder
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgShapeBinder::TaskDlgShapeBinder(ViewProviderShapeBinder* view, bool newObj)
    : Gui::TaskView::TaskDialog()
    , vp(view)
{
    assert(view);
    parameter = new TaskShapeBinder(view, newObj);

    Content.push_back(parameter);
}

TaskDlgShapeBinder::~TaskDlgShapeBinder() = default;

void TaskDlgShapeBinder::ensureDeferredRejectConnection()
{
    ensureDeferredDialogRejectConnection(
        deferredReject,
        parameter,
        &TaskShapeBinder::recomputeSettled,
        this,
        &TaskDlgShapeBinder::onParameterRecomputeSettled
    );
}

void TaskDlgShapeBinder::setDeferredRejectPending(bool pending)
{
    setDeferredDialogRejectPending(deferredReject, pending, buttonBox, [this](bool pending) {
        if (parameter) {
            parameter->setDeferredClosePending(pending);
        }
    });
}

bool TaskDlgShapeBinder::rejectNow()
{
    if (vp.expired()) {
        return false;
    }

    App::Document* doc = vp->getObject()->getDocument();
    vp->getDocument()->abortCommand();
    Gui::cmdGuiDocument(doc, "resetEdit()");
    if (doc->mustExecute()) {
        const auto outcome = Gui::runAsyncDocumentRecomputeProgressDialog(
            parameter,
            tr("Shape binder"),
            tr("Discarding changes..."),
            doc,
            /*force=*/false,
            shapeBinderRollbackDialogOptions(),
            [doc]() { return Gui::recomputeDocumentWithResult(doc); }
        );
        if (!outcome.success && !outcome.canceled) {
            Base::Console().error(
                "%s\n",
                outcome.message.empty() ? "Shape binder rollback recompute failed"
                                        : outcome.message.c_str()
            );
        }
    }
    return true;
}

bool TaskDlgShapeBinder::accept()
{
    if (deferredReject.pending) {
        return false;
    }

    try {
        if (!vp.expired()) {
            PartDesign::ShapeBinder* binder = vp->getObject<PartDesign::ShapeBinder>();
            if (!binder || !binder->isAttachedToDocument()) {
                return false;
            }
            QPointer<TaskDlgShapeBinder> dialogGuard(this);
            QPointer<TaskShapeBinder> parameterGuard(parameter);
            PreviewRecomputeGuard originalGuard(binder, parameter, vp.operator->());
            auto* editOwner = Gui::Control().activeDialog(binder->getDocument());
            App::DocumentObjectWeakPtrT binderGuard(binder);
            const bool previewSettled = !parameter->hasOutstandingRecompute()
                && parameter->canReuseAcceptedPreviewResult();
            if (previewSettled) {
                parameter->flushPendingRecompute();
                if (!dialogGuard || !parameterGuard || parameter != parameterGuard.data()
                    || !originalGuard.isLive() || vp->getObject() != binder
                    || Gui::Control().activeDialog(binder->getDocument()) != editOwner) {
                    return false;
                }
                if (!parameter->applySupportCandidate()) {
                    return false;
                }
                if (!dialogGuard || !parameterGuard || parameter != parameterGuard.data()
                    || !originalGuard.isLive() || vp.expired() || vp->getObject() != binder
                    || Gui::Control().activeDialog(binder->getDocument()) != editOwner) {
                    return false;
                }
                parameter->clearInteractiveSelection();
                parameter->accept();
                Gui::cmdAppDocument(binder, "purgeTouched()");
                if (!binder->isValid()) {
                    throw Base::RuntimeError(binder->getStatusString());
                }
            }
            else {
                if (!parameter->stopPendingRecomputeAndWait() || !dialogGuard || !parameterGuard
                    || parameter != parameterGuard.data() || !originalGuard.isLive()
                    || vp->getObject() != binder
                    || Gui::Control().activeDialog(binder->getDocument()) != editOwner) {
                    return false;
                }
                if (!parameter->applySupportCandidate()) {
                    return false;
                }
                if (!dialogGuard || !parameterGuard || parameter != parameterGuard.data()
                    || !originalGuard.isLive() || vp.expired() || vp->getObject() != binder
                    || Gui::Control().activeDialog(binder->getDocument()) != editOwner) {
                    return false;
                }
                parameter->clearInteractiveSelection();
                parameter->accept();
                auto progressTarget = parameter->makeAcceptedRecomputeProgressTarget(
                    buttonBox,
                    tr("Applying changes...")
                );
                Gui::AsyncRecomputeDialogOptions options;
                options.cancelable = false;
                options.dynamicLabel = false;
                options.forceIndeterminate = true;
                options.inlineProgressTarget = progressTarget;
                if (!runAsyncAcceptDocumentRecompute(binder->getDocument(), options)) {
                    return false;
                }
                if (!dialogGuard || !parameterGuard || parameter != parameterGuard.data()
                    || !originalGuard.isLive() || vp->getObject() != binder
                    || Gui::Control().activeDialog(binder->getDocument()) != editOwner) {
                    return false;
                }
                if (!binder->isValid()) {
                    throw Base::RuntimeError(binder->getStatusString());
                }
            }
            Gui::cmdGuiDocument(binder, "resetEdit()");
            vp->getDocument()->commitCommand();
        }
    }
    catch (const Base::Exception& e) {
        if (vp.expired()) {
            return false;
        }
        vp->getDocument()->abortCommand();
        QMessageBox::warning(
            parameter,
            tr("Input error"),
            QCoreApplication::translate("Exception", e.what())
        );
        return false;
    }

    return true;
}

bool TaskDlgShapeBinder::reject()
{
    if (vp.expired()) {
        return true;
    }

    ensureDeferredRejectConnection();
    parameter->clearInteractiveSelection();
    parameter->stopPendingRecompute();

    if (!parameter->hasOutstandingRecompute()) {
        return rejectNow();
    }

    if (!deferredReject.pending) {
        App::Document* doc = vp->getObject()->getDocument();
        deferredReject.documentName = doc ? std::string(doc->getName()) : std::string();
        setDeferredRejectPending(true);
    }

    return false;
}

void TaskDlgShapeBinder::onParameterRecomputeSettled()
{
    finishDeferredDialogReject(
        this,
        deferredReject,
        parameter && !parameter->hasOutstandingRecompute(),
        [this]() { return rejectNow(); },
        [this](bool pending) { setDeferredRejectPending(pending); }
    );
}

#include "moc_TaskShapeBinder.cpp"
