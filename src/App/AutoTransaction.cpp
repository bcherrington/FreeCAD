// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2019 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/


#include <Base/Interpreter.h>
#include <Base/Tools.h>

#include "AutoTransaction.h"
#include "Application.h"
#include "Document.h"
#include "Transactions.h"
#include "private/RecomputeAdmission.h"


FC_LOG_LEVEL_INIT("App", true, true)

using namespace App;

AutoTransaction::AutoTransaction(int tid)
    : tid(tid)
{
    if (GetApplication().transactionIsActive(tid)) {
        generation = RecomputeDetail::Coordinator::instance().captureClosure(tid).serial;
    }
    else {
        this->tid = NullTransaction;
    }
}
AutoTransaction::AutoTransaction(Document* doc, const std::string& name)
    : AutoTransaction(doc->openTransaction(name))
{}

AutoTransaction::~AutoTransaction() noexcept
{
    try {
        if (tryClose(requestedMode)) {
            return;
        }
    }
    catch (...) {
        // The exact closure is still owned below, including on an exceptional attempt.
    }
    if (tid != NullTransaction) {
        RecomputeDetail::Coordinator::instance().adoptClosure(
            {generation, tid},
            requestedMode == TransactionCloseMode::Abort
        );
    }
}

bool AutoTransaction::tryClose(TransactionCloseMode mode)
{
    requestedMode = mode;
    if (tid == NullTransaction) {
        return true;
    }
    auto& coordinator = RecomputeDetail::Coordinator::instance();
    if (!coordinator.closureCurrent({generation, tid})) {
        tid = NullTransaction;
        return true;
    }
    if (!GetApplication().closeActiveTransaction(mode, tid)) {
        return false;
    }
    tid = NullTransaction;
    return true;
}

void AutoTransaction::close(TransactionCloseMode mode)
{
    if (!tryClose(mode)) {
        throw RecomputeDetail::AdmissionRefusal("Transaction close is busy; ownership retained.");
    }
}

void App::drainDeferredTransactions()
{
    auto& coordinator = RecomputeDetail::Coordinator::instance();
    coordinator.drainClosures([&coordinator](const RecomputeDetail::ClosureToken& token, bool abort) {
        return !coordinator.closureCurrent(token)
            || GetApplication().closeActiveTransaction(
                abort ? TransactionCloseMode::Abort : TransactionCloseMode::Commit,
                token.transactionId
            );
    });
}

int Application::setActiveTransaction(TransactionName name)
{
    if (name.name.empty()) {
        name.name = "Command";
    }

    if (_pActiveDoc != nullptr) {
        return _pActiveDoc->setActiveTransaction(name);
    }
    return openGlobalTransaction(name);
}

int Application::openGlobalTransaction(TransactionName name)
{
    if (name.name.empty()) {
        name.name = "Command";
    }

    FC_WARN("Setting a global transaction with name='" << name.name);
    if (_globalTransactionID != 0 && transactionTmpName(_globalTransactionID)) {
        setTransactionName(_globalTransactionID, name);
    }
    else {
        FC_LOG("set global transaction '" << name.name << "'");

        if (_globalTransactionID != 0 && !commitTransaction(_globalTransactionID)) {
            FC_WARN("could not close current global transaction");
            return _globalTransactionID;
        }

        _globalTransactionID = Transaction::getNewID();
        setTransactionDescription(
            _globalTransactionID,
            TransactionDescription {.initiator = nullptr, .name = name}
        );
    }

    return _globalTransactionID;
}
int Application::getGlobalTransaction() const
{
    return _globalTransactionID;
}
bool Application::transactionIsActive(int tid) const
{
    return transactionDescription(tid) != std::nullopt;
}
std::string Application::getTransactionName(int tid) const
{
    auto desc = transactionDescription(tid);
    return desc ? desc->name.name : "";
}
bool Application::transactionTmpName(int tid) const
{
    auto desc = transactionDescription(tid);
    return desc ? desc->name.temporary : false;
}
Document* Application::transactionInitiator(int tid) const
{
    auto desc = transactionDescription(tid);
    return desc ? desc->initiator : nullptr;
}
std::optional<TransactionDescription> Application::transactionDescription(int tid) const
{
    if (tid == NullTransaction) {
        return std::nullopt;
    }
    auto found = _activeTransactionDescriptions.find(tid);
    if (found != _activeTransactionDescriptions.end()) {
        return std::optional<TransactionDescription>(found->second);
    }
    return std::nullopt;
}
void Application::setTransactionDescription(int tid, const TransactionDescription& desc)
{
    if (tid != NullTransaction && tid == currentlyClosingID) {
        throw RecomputeDetail::AdmissionRefusal("Cannot reuse a transaction ID while it is closing.");
    }
    if (tid == NullTransaction) {
        return;
    }
    auto found = _activeTransactionDescriptions.find(tid);
    bool wasPresent = (found != _activeTransactionDescriptions.end());

    if (wasPresent && found->second.name.temporary) {
        for (auto& v : DocMap) {
            v.second->renameTransaction(desc.name.name, tid);
        }
    }

    if (!wasPresent || found->second.name.temporary) {
        if (!wasPresent) {
            RecomputeDetail::Coordinator::instance().captureClosure(tid);
        }
        _activeTransactionDescriptions[tid] = desc;
        FC_LOG("transaction rename to '" << desc.name.name << "'");
    }
}
void Application::setTransactionName(int tid, const TransactionName& name)
{
    if (tid == NullTransaction || !transactionIsActive(tid)) {
        return;
    }
    auto found = _activeTransactionDescriptions.find(tid);
    if (found == _activeTransactionDescriptions.end() || found->second.name.temporary) {
        _activeTransactionDescriptions[tid].name = name;
        FC_LOG("transaction rename to '" << name.name << "'");
        for (auto& v : DocMap) {
            v.second->renameTransaction(name.name, tid);
        }
    }
}

bool Application::closeActiveTransaction(TransactionCloseMode mode, int id)
{
    bool abort = (mode == TransactionCloseMode::Abort);

    if (id == NullTransaction) {
        if (_pActiveDoc != nullptr && _pActiveDoc->getBookedTransactionID() != NullTransaction) {
            id = _pActiveDoc->getBookedTransactionID();
        }
        else {
            id = _globalTransactionID;
        }
    }
    if (id == NullTransaction || currentlyClosingID != NullTransaction || !transactionIsActive(id)) {
        return false;
    }
    std::vector<Document*> docsToPoke;
    for (auto& docNameAndDoc : DocMap) {
        if (docNameAndDoc.second->getBookedTransactionID() != id) {
            continue;
        }
        if (docNameAndDoc.second->isTransactionLocked() || docNameAndDoc.second->transacting()) {
            FC_LOG("pending " << (abort ? "abort" : "close") << " transaction");
            return false;
        }
        docsToPoke.push_back(docNameAndDoc.second);
    }

    auto& coordinator = RecomputeDetail::Coordinator::instance();
    std::optional<RecomputeDetail::MutationScope> admission;
    if (abort) {
        admission = coordinator.tryAtomic();
    }
    else {
        try {
            admission.emplace(coordinator.mutation({}, docsToPoke));
            std::vector<DocumentObject*> endpoints;
            for (const auto* document : docsToPoke) {
                document->collectCommitMutationEndpoints(endpoints);
            }
            admission->include(endpoints, docsToPoke);
        }
        catch (const RecomputeDetail::AdmissionRefusal&) {
            return false;
        }
    }
    if (!admission) {
        return false;
    }
    if (abort) {
        try {
            admission->include({}, docsToPoke);
        }
        catch (const RecomputeDetail::AdmissionRefusal&) {
            return false;
        }
    }
    Base::ScopeGuard resetClosing([this]() { currentlyClosingID = NullTransaction; });
    currentlyClosingID = id;
    Base::ScopeGuard resetClosingToken([&]() { coordinator.markClosureClosing(id, false); });
    coordinator.markClosureClosing(id, true);

    FC_LOG("close transaction '" << _activeTransactionDescriptions[id].name.name << "' " << abort);
    decltype(_activeTransactionDescriptions)::node_type description;
    const bool wasGlobal = id == _globalTransactionID;
    bool closed = false;
    Base::ScopeGuard restoreDescription([&]() {
        if (!closed) {
            if (description) {
                _activeTransactionDescriptions.insert(std::move(description));
            }
            if (wasGlobal && _globalTransactionID == NullTransaction) {
                _globalTransactionID = id;
            }
        }
    });
    description = _activeTransactionDescriptions.extract(id);
    if (wasGlobal) {
        _globalTransactionID = NullTransaction;
    }
    TransactionSignaller signaller(abort, true);
    struct CloseCompletion
    {
        TransactionSignaller& signaller;
        const bool& closed;
        ~CloseCompletion()
        {
            if (!closed) {
                signaller.suppressCompletion();
            }
        }
    } completion {signaller, closed};

    if (!abort) {
        // Before-close observers may extend this same transaction. Keep the
        // source reservation, then refresh the COMPLETE saved/current footprint
        // before any native history change. Refusal restores close metadata;
        // observer actions are independent effects and are not rolled back here.
        try {
            std::vector<DocumentObject*> endpoints;
            for (const auto* document : docsToPoke) {
                document->collectCommitMutationEndpoints(endpoints);
            }
            admission->include(endpoints, docsToPoke);
        }
        catch (const RecomputeDetail::AdmissionRefusal&) {
            return false;
        }
    }

    for (auto& doc : docsToPoke) {
        if (abort) {
            doc->_abortTransaction();
        }
        else {
            if (!doc->_commitTransaction()) {
                throw Base::RuntimeError("Transaction state changed during admitted close.");
            }
        }
    }
    closed = true;
    coordinator.retireClosure(id);

    return true;
}
bool Application::commitTransaction(int tid)
{
    return closeActiveTransaction(TransactionCloseMode::Commit, tid);
}
bool Application::abortTransaction(int tid)
{
    return closeActiveTransaction(TransactionCloseMode::Abort, tid);
}

////////////////////////////////////////////////////////////////////////

TransactionLocker::TransactionLocker(Document* doc, bool lock)
    : active(lock)
    , doc(doc)
{
    if (lock) {
        doc->lockTransaction();
    }
}

TransactionLocker::~TransactionLocker()
{
    if (active) {
        try {
            activate(false);
            return;
        }
        catch (Base::Exception& e) {
            e.reportException();
        }
        catch (Py::Exception&) {
            Base::PyException e;
            e.reportException();
        }
        catch (std::exception& e) {
            FC_ERR(e.what());
        }
        catch (...) {
        }
        FC_ERR("Exception when unlocking transaction");
    }
}

void TransactionLocker::activate(bool enable)
{
    if (active == enable) {
        return;
    }

    active = enable;
    if (active) {
        doc->lockTransaction();
        return;
    }

    doc->unlockTransaction();
    RecomputeDetail::Coordinator::instance().requestCleanup();
}
