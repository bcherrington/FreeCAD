// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include <QCoreApplication>
#include <QEvent>
#include <QMetaObject>
#include <QPointer>
#include <QThread>
#include <QTimer>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <sstream>

#include <App/AsyncRecomputeDebug.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/MainThreadSignal.h>
#include <App/private/RecomputeAdmission.h>
#include <Gui/ActionFunction.h>
#include <Gui/Inventor/Draggers/Gizmo.h>

#include "AsyncPreviewController.h"

namespace Gui
{
namespace
{
std::atomic<std::uint64_t> nextControllerActivityIdentity {1};

bool recomputeResultWasCanceled(const App::RecomputeResult& result)
{
    return result.failure == App::RecomputeFailure::Canceled;
}

bool hasStableDocumentIdentity(const App::RecomputeRequest& request)
{
    return !request.documentName.empty();
}

void appendPreviewControllerDebugLog(
    const AsyncPreviewController* controller,
    const char* event,
    const std::string& details = {}
)
{
    std::ostringstream stream;
    stream << "[Gui::AsyncPreviewController] " << event;
    stream << " controller=" << controller;
    if (!details.empty()) {
        stream << ' ' << details;
    }
    App::appendAsyncRecomputeDebugLog(stream.str());
}

void queueOnMainThread(std::function<void()>&& fn)
{
    if (App::MainThreadSignalConfig::hasHooks()) {
        App::MainThreadSignalConfig::invoke(std::move(fn), /*blocking=*/false);
        return;
    }

    if (auto* app = QCoreApplication::instance()) {
        QMetaObject::invokeMethod(app, [f = std::move(fn)]() mutable { f(); }, Qt::QueuedConnection);
        return;
    }

    fn();
}

bool canPumpQueuedMainThreadDispatches()
{
    if (App::MainThreadSignalConfig::hasHooks() && App::MainThreadSignalConfig::isMainThread()
        && App::MainThreadSignalConfig::canPumpEvents()) {
        return true;
    }

    auto* app = QCoreApplication::instance();
    return app && QThread::currentThread() == app->thread();
}

void pumpQueuedMainThreadDispatches()
{
    if (App::MainThreadSignalConfig::hasHooks() && App::MainThreadSignalConfig::isMainThread()
        && App::MainThreadSignalConfig::canPumpEvents()) {
        App::MainThreadSignalConfig::pumpEvents();
        return;
    }

    if (auto* app = QCoreApplication::instance(); app && QThread::currentThread() == app->thread()) {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    }
}

}  // namespace

struct AsyncPreviewController::Registration
{
    std::uint64_t generation {};
    App::RecomputeRequest request;
    App::RecomputeDetail::RetirementPtr retirement
        = std::make_shared<App::RecomputeDetail::Retirement>();
    std::mutex mutex;
    std::condition_variable changed;
    bool submitted {false};        // GUI owner only
    bool cancelRequested {false};  // GUI owner only
    bool resultReady {false};
    bool success {true};
    bool canceled {false};
    App::RecomputeFailure failure {App::RecomputeFailure::None};
    std::string message;
};

AsyncPreviewController::AsyncPreviewController(Callbacks callbacks, QObject* parent)
    : QObject(parent)
    , callbacks(std::move(callbacks))
    , activityIdentity(nextControllerActivityIdentity.fetch_add(1, std::memory_order_relaxed))
{}

AsyncPreviewController::~AsyncPreviewController()
{
    controllerAlive->store(false);
    if (retirementTimer) {
        retirementTimer->stop();
    }
    for (const auto& [generation, registration] : registrations) {
        registration->changed.notify_all();
    }
    if (recomputeScheduler) {
        recomputeScheduler->stop();
    }
    detachRecomputeProgressObserver();
}

void AsyncPreviewController::setSchedulerInterval(int intervalMs)
{
    schedulerIntervalMs = std::max(0, intervalMs);

    if (!recomputeScheduler) {
        recomputeScheduler = std::make_unique<Gui::DebouncedFunction>(this);
        recomputeScheduler->setFunction([this]() { requestRecompute(/*waitForCompletion=*/false); });
    }

    recomputeScheduler->setInterval(effectiveSchedulerIntervalMs());
}

bool AsyncPreviewController::hasScheduledRecompute() const
{
    return recomputeScheduler && recomputeScheduler->isActive();
}

bool AsyncPreviewController::triggerScheduledRecomputeNow()
{
    if (!hasScheduledRecompute()) {
        return false;
    }

    recomputeScheduler->triggerNow();
    return true;
}

void AsyncPreviewController::scheduleRecompute()
{
    ++activitySerial;
    if (recomputeScheduler) {
        recomputeScheduler->setInterval(effectiveSchedulerIntervalMs());
        recomputeScheduler->start();
        return;
    }

    requestRecompute(/*waitForCompletion=*/false);
}

int AsyncPreviewController::effectiveSchedulerIntervalMs() const
{
    if (schedulerIntervalMs <= 0) {
        return schedulerIntervalMs;
    }

    if (Gui::Gizmo::isAnyDragActive()) {
        return std::min(schedulerIntervalMs, Gui::Gizmo::activeDragPreviewDebounceMs());
    }

    return schedulerIntervalMs;
}

void AsyncPreviewController::stopScheduledRecompute()
{
    if (recomputeScheduler) {
        recomputeScheduler->stop();
    }
}

void AsyncPreviewController::flushPendingRecompute()
{
    if (!hasScheduledRecompute() && !hasOutstandingRecompute() && lastRecomputeSucceeded) {
        return;
    }

    if (hasScheduledRecompute()) {
        stopScheduledRecompute();
        requestRecompute(/*waitForCompletion=*/true);
        return;
    }

    if (hasOutstandingRecompute()) {
        waitForOutstandingRecomputeToSettle();
        return;
    }

    requestRecompute(/*waitForCompletion=*/true);
}

void AsyncPreviewController::stopPendingRecompute()
{
    stopScheduledRecompute();
    const auto alive = controllerAlive;
    QPointer<AsyncPreviewController> guard(this);
    std::vector<std::shared_ptr<Registration>> canceled;
    for (const auto& [generation, registration] : registrations) {
        if (!registration->cancelRequested) {
            registration->cancelRequested = true;
            canceled.push_back(registration);
        }
    }
    if (canceled.empty()) {
        return;
    }
    const auto latest = recomputeGeneration;
    std::uint64_t activeCancellation {};
    for (const auto& registration : canceled) {
        if (!registration->submitted) {
            {
                std::lock_guard lock(registration->mutex);
                registration->resultReady = true;
                registration->success = false;
                registration->canceled = true;
                registration->failure = App::RecomputeFailure::Canceled;
            }
            registration->retirement->markRetired();
            registration->changed.notify_all();
            continue;
        }
        const auto result = App::GetApplication().cancelRecomputeRequest(registration->request);
        if (!alive->load() || !guard) {
            return;
        }
        if (result.canceledInProgress && !activeCancellation) {
            activeCancellation = registration->generation;
        }
    }
    if (recomputeGeneration == latest && activeCancellation) {
        // The active native cancellation retains its native exception metadata;
        // removed reruns retire too, without rewinding the accepted serial.
        applyEligibleGeneration = activeCancellation;
    }
    updatePendingState();
    drainRetiredRecomputes();
    if (alive->load() && guard) {
        notifyStateChanged();
    }
}

AsyncPreviewController::ActivityToken AsyncPreviewController::recomputeActivityToken() const noexcept
{
    return {activityIdentity, activitySerial};
}

bool AsyncPreviewController::stopPendingRecomputeAndWait()
{
    const auto alive = controllerAlive;
    QPointer<AsyncPreviewController> guard(this);
    const auto entryActivity = recomputeActivityToken();
    std::vector<std::shared_ptr<Registration>> accepted;
    bool needsNativeWait = false;
    for (const auto& [generation, registration] : registrations) {
        accepted.push_back(registration);
        needsNativeWait = needsNativeWait
            || (registration->submitted && !registration->retirement->retired());
    }
    if (needsNativeWait) {
        try {
            App::RecomputeDetail::throwIfBlockingRecomputeWaitIsCyclic();
        }
        catch (const App::RecomputeDetail::AdmissionRefusal& exception) {
            appendPreviewControllerDebugLog(this, "settle-wait-refused", exception.what());
            return false;
        }
    }
    // Cancellation and debounce stop happen only after wait admission succeeds.
    // The existing stop snapshots its registrations before invoking callbacks.
    stopPendingRecompute();
    if (!alive->load() || !guard) {
        return false;
    }
    for (const auto& registration : accepted) {
        if (!registration->retirement->retired()) {
            waitForRegistration(registration);
            if (!alive->load() || !guard) {
                return false;
            }
        }
    }
    // Pumped completion callbacks may accept a fresh preview or schedule one.
    // Preserve that work and decline acceptance rather than cancel/rerun it.
    return recomputeActivityToken() == entryActivity && !hasScheduledRecompute()
        && registrations.empty();
}

void AsyncPreviewController::requestRecompute(bool waitForCompletion)
{
    ++activitySerial;
    if (!callbacks.makeRequest || (!callbacks.runSync && !callbacks.runSyncWithResult)) {
        return;
    }
    const auto alive = controllerAlive;
    QPointer<AsyncPreviewController> guard(this);
    const auto generation = ++nextRecomputeGeneration;
    const auto makeRequest = callbacks.makeRequest;
    App::RecomputeRequest request = makeRequest();
    if (!alive->load() || !guard || !hasStableDocumentIdentity(request)) {
        return;
    }
    // This controller owns its completion channel. Retiring any ignored factory
    // callback may itself reenter; no member is used until lifetime is checked.
    request.callback = {};
    if (!alive->load() || !guard) {
        return;
    }
    const bool asyncEnabled = App::GetApplication().isAsyncRecomputeEnabled();
    const auto route = App::GetApplication().classifyRecomputeRequest(request, asyncEnabled);
    if (!alive->load() || !guard) {
        return;
    }
    const bool workerRoute = asyncEnabled
        && route != App::RecomputeDetail::RouteDisposition::KnownUnsafe;
    if (waitForCompletion
        && (workerRoute || route == App::RecomputeDetail::RouteDisposition::Parked)) {
        App::RecomputeDetail::throwIfBlockingRecomputeWaitIsCyclic();
    }
    auto registration = std::make_shared<Registration>();
    registration->generation = generation;
    if (!workerRoute) {
        App::ScopedRecomputeOptions optionsScope(request.options);
        const auto typed = callbacks.runSyncWithResult;
        const auto legacy = callbacks.runSync;
        App::RecomputeResult result;
        if (route == App::RecomputeDetail::RouteDisposition::Parked) {
            result.success = false;
            result.failure = App::RecomputeFailure::Exception;
            result.exception = std::make_unique<App::RecomputeDetail::AdmissionRefusal>(
                "Recompute is busy; asynchronous execution is disabled"
            );
        }
        else if (route == App::RecomputeDetail::RouteDisposition::Unavailable) {
            result = App::GetApplication().recomputeRequestSynchronously(request);
        }
        else if (typed) {
            result = typed();
        }
        else {
            legacy();
        }
        if (!alive->load() || !guard) {
            return;
        }
        registration->request = request;
        registration->resultReady = true;
        registration->success = result.success;
        registration->canceled = recomputeResultWasCanceled(result);
        registration->failure = result.failure;
        registration->message = result.exception ? result.exception->what() : std::string();
        registration->retirement->markRetired();
        registrations.emplace(generation, registration);
        if (generation > recomputeGeneration) {
            recomputeGeneration = generation;
            applyEligibleGeneration = generation;
        }
        drainRetiredRecomputes();
        return;
    }

    const bool hadOutstanding = hasOutstandingRecompute();
    if (!recomputeProgress || !hadOutstanding || recomputeProgress->wasCanceled()
        || recomputeCancelRequested) {
        detachRecomputeProgressObserver();
        recomputeProgress = std::make_shared<App::RecomputeProgressHandle>();
        resetRecomputeProgressDisplayState();
    }
    request.progress = recomputeProgress;
    registration->request = request;
    registrations.emplace(generation, registration);
    if (generation > recomputeGeneration) {
        recomputeGeneration = generation;
        applyEligibleGeneration = generation;
    }
    updatePendingState();
    try {
        if (generation == applyEligibleGeneration) {
            attachRecomputeProgressObserver(generation);
        }
        if (!alive->load() || !guard) {
            return;
        }
        if (registration->cancelRequested || !registrations.contains(generation)) {
            drainRetiredRecomputes();
            return;
        }
        notifyStateChanged();
        if (!alive->load() || !guard) {
            return;
        }
        if (registration->cancelRequested || !registrations.contains(generation)) {
            drainRetiredRecomputes();
            return;
        }
        request.callback = [guard,
                            alive,
                            registration](App::RecomputeRequest&, App::RecomputeResult& result) {
            {
                std::lock_guard lock(registration->mutex);
                registration->resultReady = true;
                registration->success = result.success;
                registration->canceled = recomputeResultWasCanceled(result);
                registration->failure = result.failure;
                registration->message = result.exception ? result.exception->what() : std::string();
            }
            registration->changed.notify_all();
            queueOnMainThread([guard, alive] {
                if (alive->load() && guard) {
                    guard->drainRetiredRecomputes();
                }
            });
        };
        registration->submitted = true;
        App::GetApplication().queueRecomputeRequestWithRetirement(
            std::move(request),
            registration->retirement
        );
    }
    catch (...) {
        if (alive->load() && guard) {
            registrations.erase(generation);
            updatePendingState();
            lastRecomputeSucceeded = false;
        }
        throw;
    }
    if (!alive->load() || !guard) {
        return;
    }
    if (waitForCompletion) {
        waitForRegistration(registration);
        if (alive->load() && guard) {
            waitForOutstandingRecomputeToSettle();
        }
    }
    else {
        drainRetiredRecomputes();
    }
}

void AsyncPreviewController::updatePendingState()
{
    recomputeInProgress = !registrations.empty();
    const auto eligible = std::ranges::count_if(registrations, [](const auto& value) {
        return !value.second->cancelRequested;
    });
    recomputeQueued = eligible > 1;
    recomputeCancelRequested = std::ranges::any_of(registrations, [](const auto& value) {
        return value.second->cancelRequested;
    });
}

void AsyncPreviewController::drainRetiredRecomputes()
{
    const auto alive = controllerAlive;
    QPointer<AsyncPreviewController> guard(this);
    std::vector<std::shared_ptr<Registration>> snapshot;
    for (const auto& [generation, registration] : registrations) {
        snapshot.push_back(registration);
    }
    for (const auto& registration : snapshot) {
        if (!registration->retirement->retired()) {
            continue;
        }
        bool ready;
        bool success;
        bool canceled;
        App::RecomputeFailure failure;
        std::string message;
        {
            std::lock_guard lock(registration->mutex);
            ready = registration->resultReady;
            success = registration->success;
            canceled = registration->canceled;
            failure = registration->failure;
            message = registration->message;
        }
        if (ready) {
            finishRecompute(registration->generation, success, canceled, failure, std::move(message));
            if (!alive->load() || !guard) {
                return;
            }
        }
    }
    // Completion callbacks may have inserted and partially drained new entries.
    // Inspect the live ledger: a stale snapshot must not cancel their only wakeup.
    const bool readyPending = std::ranges::any_of(registrations, [](const auto& value) {
        std::lock_guard lock(value.second->mutex);
        return value.second->resultReady;
    });
    if (readyPending) {
        if (!retirementTimer) {
            retirementTimer = std::make_unique<QTimer>(this);
            retirementTimer->setSingleShot(true);
            retirementTimer->setInterval(10);
            QObject::connect(retirementTimer.get(), &QTimer::timeout, this, [this] {
                drainRetiredRecomputes();
            });
        }
        if (!retirementTimer->isActive()) {
            retirementTimer->start();
        }
    }
    else if (retirementTimer) {
        retirementTimer->stop();
    }
}

void AsyncPreviewController::waitForRegistration(const std::shared_ptr<Registration>& registration)
{
    App::RecomputeDetail::throwIfBlockingRecomputeWaitIsCyclic();
    const auto alive = controllerAlive;
    QPointer<AsyncPreviewController> guard(this);
    while (alive->load() && !registration->retirement->retired()) {
        if (canPumpQueuedMainThreadDispatches()) {
            pumpQueuedMainThreadDispatches();
        }
        if (!alive->load()) {
            return;
        }
        std::unique_lock lock(registration->mutex);
        registration->changed.wait_for(lock, std::chrono::milliseconds(1));
    }
    if (alive->load() && guard) {
        drainRetiredRecomputes();
    }
}

bool AsyncPreviewController::hasOutstandingRecompute() const
{
    return !registrations.empty();
}

bool AsyncPreviewController::isInProgress() const
{
    return recomputeInProgress;
}

bool AsyncPreviewController::isQueued() const
{
    return recomputeQueued;
}

bool AsyncPreviewController::isCancelRequested() const
{
    return recomputeCancelRequested;
}

bool AsyncPreviewController::didLastRecomputeSucceed() const
{
    return lastRecomputeSucceeded;
}

std::uint64_t AsyncPreviewController::currentGeneration() const
{
    return recomputeGeneration;
}

bool AsyncPreviewController::isProgressDeterminate() const
{
    return recomputeProgressDeterminate;
}

int AsyncPreviewController::progressPercent() const
{
    return recomputeProgressPercent;
}

const std::string& AsyncPreviewController::progressText() const
{
    return recomputeProgressText;
}

void AsyncPreviewController::finishRecompute(
    std::uint64_t generation,
    bool success,
    bool canceled,
    App::RecomputeFailure failure,
    std::string message
)
{
    const auto found = registrations.find(generation);
    if (found == registrations.end()) {
        return;
    }
    const auto registration = found->second;
    registrations.erase(found);
    const auto targetAvailable = [&registration] {
        const auto& request = registration->request;
        return request.documentObjectName.empty() ? request.resolveDocument() != nullptr
                                                  : request.resolveDocumentObject() != nullptr;
    };
    const auto alive = controllerAlive;
    QPointer<AsyncPreviewController> guard(this);
    updatePendingState();
    const bool apply = generation == applyEligibleGeneration;
    if (apply) {
        // Native completion and the current GUI receiver are separate facts.
        // Keep the App subscribers' shared outcome unchanged.
        if (success && !targetAvailable()) {
            success = false;
            failure = App::RecomputeFailure::TargetUnavailable;
            message = "Recompute target was retired before GUI application";
        }
        lastRecomputeSucceeded = success;
        detachRecomputeProgressObserver();
        recomputeProgress.reset();
        resetRecomputeProgressDisplayState();
        const auto onCompleted = callbacks.onCompleted;
        if (onCompleted) {
            onCompleted(success, canceled, failure, message);
        }
        if (!alive->load() || !guard) {
            return;
        }
        if (generation == applyEligibleGeneration) {
            if (!targetAvailable()) {
                lastRecomputeSucceeded = false;
            }
            else {
                const auto onAppliedResult = callbacks.onAppliedResult;
                if (onAppliedResult) {
                    onAppliedResult(success, canceled);
                }
            }
        }
        if (!alive->load() || !guard) {
            return;
        }
    }
    notifyStateChanged();
    if (alive->load() && guard && registrations.empty()
        && settledRecomputeGeneration != recomputeGeneration) {
        settledRecomputeGeneration = recomputeGeneration;
        Q_EMIT recomputeSettled();
    }
}

void AsyncPreviewController::waitForOutstandingRecomputeToSettle()
{
    App::RecomputeDetail::throwIfBlockingRecomputeWaitIsCyclic();
    const auto alive = controllerAlive;
    QPointer<AsyncPreviewController> guard(this);
    while (alive->load() && guard) {
        drainRetiredRecomputes();
        if (!alive->load() || !guard || registrations.empty()) {
            return;
        }
        const auto registration = registrations.begin()->second;
        waitForRegistration(registration);
    }
}

void AsyncPreviewController::attachRecomputeProgressObserver(std::uint64_t generation)
{
    if (!recomputeProgress) {
        resetRecomputeProgressDisplayState();
        return;
    }
    const auto alive = controllerAlive;
    const auto progress = recomputeProgress;
    auto* progressHandle = progress.get();
    QPointer<AsyncPreviewController> guard(this);
    progress->setDisplayObserver(
        [guard, alive, progressHandle, generation](const App::RecomputeProgressDisplayState& state) {
            queueOnMainThread([guard, alive, progressHandle, generation, state]() {
                if (!alive->load() || !guard || guard->recomputeProgress.get() != progressHandle
                    || guard->recomputeGeneration != generation) {
                    return;
                }
                guard->setRecomputeProgressDisplayState(generation, state);
            });
        }
    );
    if (alive->load() && guard && recomputeProgress == progress && recomputeGeneration == generation) {
        setRecomputeProgressDisplayState(generation, progress->displayState());
    }
}

void AsyncPreviewController::detachRecomputeProgressObserver()
{
    if (recomputeProgress) {
        recomputeProgress->setDisplayObserver({});
    }
}

void AsyncPreviewController::resetRecomputeProgressDisplayState()
{
    recomputeProgressGeneration = 0;
    recomputeProgressText.clear();
    recomputeProgressPercent = 0;
    recomputeProgressDeterminate = false;
    appendPreviewControllerDebugLog(this, "progress_reset");
}

void AsyncPreviewController::setRecomputeProgressDisplayState(
    std::uint64_t generation,
    const App::RecomputeProgressDisplayState& state
)
{
    const bool textChanged = recomputeProgressText != state.text;
    const bool percentChanged = recomputeProgressPercent != state.percent;
    const bool determinateChanged = recomputeProgressDeterminate != state.determinate;
    const bool generationChanged = recomputeProgressGeneration != generation;
    if (generationChanged) {
        recomputeProgressGeneration = generation;
        recomputeProgressPercent = 0;
        recomputeProgressDeterminate = false;
    }

    const bool clampedRegression = state.determinate && !generationChanged
        && state.percent < recomputeProgressPercent;
    recomputeProgressDeterminate = state.determinate;
    if (state.determinate) {
        recomputeProgressPercent = std::max(recomputeProgressPercent, state.percent);
    }
    else {
        recomputeProgressPercent = state.percent;
    }
    if (!clampedRegression) {
        recomputeProgressText = state.text;
    }

    if (generationChanged || clampedRegression || textChanged || percentChanged
        || determinateChanged) {
        std::ostringstream details;
        details << "generation=" << generation;
        details << " state_text=" << (state.text.empty() ? "<empty>" : state.text);
        details << " state_percent=" << state.percent;
        details << " state_determinate=" << state.determinate;
        details << " display_text="
                << (recomputeProgressText.empty() ? "<empty>" : recomputeProgressText);
        details << " display_percent=" << recomputeProgressPercent;
        details << " display_determinate=" << recomputeProgressDeterminate;
        details << " generation_changed=" << generationChanged;
        details << " clamped_regression=" << clampedRegression;
        appendPreviewControllerDebugLog(this, "progress_update", details.str());
    }

    notifyStateChanged();
}

void AsyncPreviewController::notifyStateChanged()
{
    QPointer<AsyncPreviewController> guard(this);
    const auto onStateChanged = callbacks.onStateChanged;
    if (onStateChanged) {
        onStateChanged();
    }
    if (guard) {
        Q_EMIT stateChanged();
    }
}

}  // namespace Gui

#include "moc_AsyncPreviewController.cpp"
