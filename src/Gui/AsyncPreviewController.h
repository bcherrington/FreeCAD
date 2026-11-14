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

#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <functional>
#include <memory>
#include <string>

#include <QObject>

#include <App/Application.h>

class QTimer;

namespace Gui
{
class DebouncedFunction;
class Gizmo;

/// Execute once on the caller's model thread and retain the native scoped outcome.
inline App::RecomputeResult recomputeDocumentObjectWithResult(
    App::DocumentObject* object,
    bool recursive = false,
    int options = App::currentRecomputeOptions()
)
{
    return App::GetApplication().recomputeRequestSynchronously(
        object ? App::RecomputeRequest::fromDocumentObject(*object, recursive, options)
               : App::RecomputeRequest {}
    );
}

inline App::RecomputeResult recomputeDocumentWithResult(
    App::Document* document,
    bool force = false,
    int options = App::currentRecomputeOptions()
)
{
    return App::GetApplication().recomputeRequestSynchronously(
        document ? App::RecomputeRequest::fromDocument(*document, force, options)
                 : App::RecomputeRequest {}
    );
}

/**
 * @brief GUI-side controller for an async interactive preview recompute.
 *
 * The controller converts task-panel preview updates into `App::RecomputeRequest`
 * instances, applies debounce/coalescing policy, tracks request generations, and
 * exposes progress/cancel state back to widgets. It does not know about a
 * specific task panel; callers provide the request/sync-fallback hooks
 * through `Callbacks`.
 *
 * Cancellation is cooperative. `stopPendingRecompute()` cancels the active
 * request handle and removes queued matching requests, but running modeling
 * code must poll `App::currentRecomputeWasCanceled()` or an active
 * `App::RecomputeProgressScope` for cancellation to take effect.
 */
class GuiExport AsyncPreviewController: public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Task-specific hooks used by AsyncPreviewController.
     *
     * `makeRequest()` creates the stable `App::RecomputeRequest` identity.
     * It should return an empty request when the task no longer has a valid
     * document/object target.
     * `runSyncWithResult()` retains the native outcome when async recompute is
     * disabled or the request is not worker-safe. The legacy `runSync()` hook
     * reports completion only; it cannot infer scoped model validity. Supply
     * the result hook for model/acceptance decisions. Only one hook executes.
     * Completion callbacks are always delivered
     * on the GUI side after native retirement. If the original target has
     * retired before GUI delivery, a native success becomes a GUI
     * TargetUnavailable result; the shared App outcome is unchanged. Model
     * application is skipped when the original receiver is no longer live.
     */
    struct Callbacks
    {
        std::function<App::RecomputeRequest()> makeRequest;
        std::function<void()> runSync;
        std::function<
            void(bool success, bool canceled, App::RecomputeFailure failure, const std::string& message)>
            onCompleted;
        std::function<void(bool success, bool canceled)> onAppliedResult;
        std::function<void()> onStateChanged;
        // Appended to preserve existing source aggregate initialization.
        std::function<App::RecomputeResult()> runSyncWithResult;
    };

    explicit AsyncPreviewController(Callbacks callbacks, QObject* parent = nullptr);
    ~AsyncPreviewController() override;

    /// Configure debounce for high-frequency UI changes before recompute starts.
    void setSchedulerInterval(int intervalMs);
    bool hasScheduledRecompute() const;
    /// Run a currently scheduled debounce callback immediately, if one exists.
    bool triggerScheduledRecomputeNow();
    void scheduleRecompute();
    void stopScheduledRecompute();

    /**
     * @brief Start a recompute request.
     *
     * When `waitForCompletion` is false, completion is reported asynchronously
     * through callbacks/signals. When true, this method waits for the queued
     * request to settle while draining queued main-thread invocations needed by
     * worker-side document notifications.
     */
    void requestRecompute(bool waitForCompletion);
    /// Run a scheduled recompute now or wait for an outstanding recompute to settle.
    void flushPendingRecompute();
    /// Cancel the active request and remove queued matching preview requests.
    void stopPendingRecompute();

    bool hasOutstandingRecompute() const;
    bool isInProgress() const;
    bool isQueued() const;
    bool isCancelRequested() const;
    bool didLastRecomputeSucceed() const;
    std::uint64_t currentGeneration() const;
    bool isProgressDeterminate() const;
    int progressPercent() const;
    const std::string& progressText() const;

Q_SIGNALS:
    /// Emitted whenever progress, queued, in-progress, or cancel state changes.
    void stateChanged();
    /// Emitted when accepted registrations retire and the latest application settles.
    void recomputeSettled();

private:
    struct Registration;
    void drainRetiredRecomputes();
    void updatePendingState();
    void waitForRegistration(const std::shared_ptr<Registration>& registration);
    int effectiveSchedulerIntervalMs() const;
    void waitForOutstandingRecomputeToSettle();
    void finishRecompute(
        std::uint64_t generation,
        bool success,
        bool canceled,
        App::RecomputeFailure failure,
        std::string message
    );
    void attachRecomputeProgressObserver(std::uint64_t generation);
    void detachRecomputeProgressObserver();
    void resetRecomputeProgressDisplayState();
    void setRecomputeProgressDisplayState(
        std::uint64_t generation,
        const App::RecomputeProgressDisplayState& state
    );
    void notifyStateChanged();

private:
    Callbacks callbacks;
    std::unique_ptr<Gui::DebouncedFunction> recomputeScheduler;
    int schedulerIntervalMs = 0;
    std::shared_ptr<App::RecomputeProgressHandle> recomputeProgress;
    std::uint64_t recomputeGeneration = 0;
    std::uint64_t nextRecomputeGeneration = 0;
    std::uint64_t applyEligibleGeneration = 0;
    std::uint64_t settledRecomputeGeneration = 0;
    std::map<std::uint64_t, std::shared_ptr<Registration>> registrations;
    std::unique_ptr<QTimer> retirementTimer;
    std::shared_ptr<std::atomic<bool>> controllerAlive = std::make_shared<std::atomic<bool>>(true);
    bool lastRecomputeSucceeded = true;
    bool recomputeInProgress = false;
    bool recomputeQueued = false;
    bool recomputeCancelRequested = false;
    std::uint64_t recomputeProgressGeneration = 0;
    std::string recomputeProgressText;
    int recomputeProgressPercent = 0;
    bool recomputeProgressDeterminate = false;
};

}  // namespace Gui
