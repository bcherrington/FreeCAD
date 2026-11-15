// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <future>

#include <boost/scope_exit.hpp>

#include <QCoreApplication>
#include <QEvent>
#include <QSignalSpy>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTest>
#include <QWidget>

#include <App/Application.h>
#include <App/Document.h>
#include <App/FeatureTest.h>
#include <App/MainThreadSignal.h>
#include <App/private/RecomputeAdmission.h>
#include <Base/Parameter.h>

#include <Gui/AsyncRecomputeProgressDialog.h>
#include <Gui/AsyncPreviewController.h>
#include <Gui/AsyncPreviewSession.h>
#include <src/App/InitApplication.h>

namespace PartDesign
{
class PreviewOutcomeFeature: public App::FeatureTest
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::PreviewOutcomeFeature);

public:
    App::DocumentObjectExecReturn* execute() override
    {
        ++executionCount;
        observedOptions.store(App::currentRecomputeOptions());
        executedOnOwnerThread.store(std::this_thread::get_id() == ownerThread);
        if (!Bool.getValue()) {
            return new App::DocumentObjectExecReturn("Ordinary preview model error");
        }
        return App::FeatureTest::execute();
    }

    bool canRecomputeOnWorker() const override
    {
        ++eligibilityCalls;
        return App::FeatureTest::canRecomputeOnWorker();
    }

    mutable std::atomic<int> eligibilityCalls {0};
    const std::thread::id ownerThread = std::this_thread::get_id();
    std::atomic<int> executionCount {0};
    std::atomic<int> observedOptions {0};
    std::atomic<bool> executedOnOwnerThread {false};
};
}  // namespace PartDesign

PROPERTY_SOURCE(PartDesign::PreviewOutcomeFeature, App::FeatureTest)

using namespace std::chrono_literals;

namespace
{
constexpr auto AsyncPrefsPath = "User parameter:BaseApp/Preferences/Document";

struct CallbackState
{
    int completedCount = 0;
    bool success = true;
    bool canceled = false;
    App::RecomputeFailure failure = App::RecomputeFailure::None;
    std::string message;
    int appliedCount = 0;
    bool appliedSuccess = true;
    bool appliedCanceled = false;
    int stateChangedCount = 0;
};

struct FakeMainThreadInvokeItem
{
    std::function<void()> fn;
    std::mutex mutex;
    bool done = false;
    std::condition_variable changed;
};

struct FakeMainThreadHookState
{
    std::mutex mutex;
    std::deque<std::shared_ptr<FakeMainThreadInvokeItem>> queued;
    std::thread::id mainThreadId;
    int pumpCount = 0;
};

FakeMainThreadHookState& getFakeMainThreadHookState()
{
    static FakeMainThreadHookState state;
    return state;
}

void resetFakeMainThreadHooks()
{
    auto& state = getFakeMainThreadHookState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.queued.clear();
    state.mainThreadId = std::this_thread::get_id();
    state.pumpCount = 0;
}

bool fakeIsMainThread()
{
    return std::this_thread::get_id() == getFakeMainThreadHookState().mainThreadId;
}

void fakeInvokeOnMainThread(std::function<void()>&& fn, bool blocking)
{
    if (fakeIsMainThread()) {
        fn();
        return;
    }

    auto item = std::make_shared<FakeMainThreadInvokeItem>();
    item->fn = std::move(fn);

    {
        std::lock_guard<std::mutex> lock(getFakeMainThreadHookState().mutex);
        getFakeMainThreadHookState().queued.push_back(item);
    }

    if (!blocking) {
        return;
    }

    std::unique_lock<std::mutex> lock(item->mutex);
    item->changed.wait(lock, [&item]() { return item->done; });
}

void fakePumpMainThreadEvents()
{
    std::deque<std::shared_ptr<FakeMainThreadInvokeItem>> queued;
    {
        auto& state = getFakeMainThreadHookState();
        std::lock_guard<std::mutex> lock(state.mutex);
        ++state.pumpCount;
        queued.swap(state.queued);
    }

    for (const auto& item : queued) {
        item->fn();
        {
            std::lock_guard<std::mutex> lock(item->mutex);
            item->done = true;
        }
        item->changed.notify_all();
    }
}
}  // namespace

class testAsyncPreviewController: public QObject
{
    Q_OBJECT

public:
    testAsyncPreviewController()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void init()  // NOLINT
    {
        if (PartDesign::PreviewOutcomeFeature::getClassTypeId().isBad()) {
            PartDesign::PreviewOutcomeFeature::init();
        }
        params = App::GetApplication().GetParameterGroupByPath(AsyncPrefsPath);
        const auto boolValues = params->GetBoolMap();
        hadAsyncEnabled = std::any_of(boolValues.begin(), boolValues.end(), [](const auto& value) {
            return value.first == "EnableAsyncRecompute";
        });
        oldAsyncEnabled = params->GetBool("EnableAsyncRecompute", true);
        params->SetBool("EnableAsyncRecompute", true);

        callbackState = {};
        App::FeatureTestAsyncBlocker::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("async_preview_controller");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        QVERIFY(doc != nullptr);

        blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
            doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
        );
        QVERIFY(blocker != nullptr);

        Gui::AsyncPreviewController::Callbacks callbacks;
        callbacks.makeRequest = [this]() {
            auto* object = static_cast<App::DocumentObject*>(blocker);
            return object ? App::RecomputeRequest::fromDocumentObject(*object)
                          : App::RecomputeRequest {};
        };
        callbacks.runSync = [this]() {
            if (blocker) {
                blocker->recomputeFeature();
            }
        };
        callbacks.onCompleted = [this](
                                    bool success,
                                    bool canceled,
                                    App::RecomputeFailure failure,
                                    const std::string& message
                                ) {
            ++callbackState.completedCount;
            callbackState.success = success;
            callbackState.canceled = canceled;
            callbackState.failure = failure;
            callbackState.message = message;
        };
        callbacks.onAppliedResult = [this](bool success, bool canceled) {
            ++callbackState.appliedCount;
            callbackState.appliedSuccess = success;
            callbackState.appliedCanceled = canceled;
        };
        callbacks.onStateChanged = [this]() {
            ++callbackState.stateChangedCount;
        };
        controller = std::make_unique<Gui::AsyncPreviewController>(std::move(callbacks));
    }

    void cleanup()  // NOLINT
    {
        if (controller) {
            controller->stopPendingRecompute();
            App::FeatureTestAsyncBlocker::releaseBlocker();
            auto recomputeSettled = [&] {
                if (App::MainThreadSignalConfig::hasHooks()
                    && App::MainThreadSignalConfig::isMainThread()
                    && App::MainThreadSignalConfig::canPumpEvents()) {
                    App::MainThreadSignalConfig::pumpEvents();
                }
                return !controller || !controller->hasOutstandingRecompute();
            };
            QTRY_VERIFY_WITH_TIMEOUT(recomputeSettled(), 2000);
            controller.reset();
        }
        else {
            App::FeatureTestAsyncBlocker::releaseBlocker();
        }

        blocker = nullptr;

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        // A worker may still be returning through the fake synchronous dispatcher.
        // Keep its owner pump installed until document close has drained it.
        App::MainThreadSignalConfig::setHooks(nullptr, nullptr, nullptr);

        doc = nullptr;
        docName.clear();

        if (params) {
            if (hadAsyncEnabled) {
                params->SetBool("EnableAsyncRecompute", oldAsyncEnabled);
            }
            else {
                params->RemoveBool("EnableAsyncRecompute");
            }
            params = nullptr;
        }
    }

    void ordinaryModelErrorCompletionObservation_data()  // NOLINT
    {
        QTest::addColumn<bool>("asyncEnabled");
        QTest::newRow("sync-fallback") << false;
        QTest::newRow("worker") << true;
    }

    void ordinaryModelErrorCompletionObservation()  // NOLINT
    {
        QFETCH(bool, asyncEnabled);
        params->SetBool("EnableAsyncRecompute", asyncEnabled);
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("OutcomeFeature");
        QVERIFY(feature != nullptr);
        QVERIFY(feature->getPropertyByName("Shape") == nullptr);
        auto* sentinel = doc->addObject<App::FeatureTest>("OutcomeDrainSentinel");
        QVERIFY(sentinel != nullptr);

        int completed = 0;
        int applied = 0;
        int syncCalls = 0;
        int legacyCalls = 0;
        bool completionSuccess = false;
        bool appliedSuccess = false;
        bool canceled = true;
        bool syncModelResult = false;
        App::RecomputeFailure failure = App::RecomputeFailure::None;
        const auto ownerThread = std::this_thread::get_id();
        bool callbacksOnOwnerThread = true;
        Gui::AsyncPreviewController::Callbacks callbacks;
        callbacks.makeRequest = [feature]() {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        callbacks.runSync = [&]() {
            ++legacyCalls;
        };
        callbacks.runSyncWithResult = [&]() {
            ++syncCalls;
            auto result = Gui::recomputeDocumentObjectWithResult(feature);
            syncModelResult = result.success;
            return result;
        };
        callbacks.onCompleted =
            [&](bool success, bool wasCanceled, App::RecomputeFailure why, const std::string&) {
                ++completed;
                completionSuccess = success;
                canceled = wasCanceled;
                failure = why;
                callbacksOnOwnerThread &= std::this_thread::get_id() == ownerThread;
            };
        callbacks.onAppliedResult = [&](bool success, bool) {
            ++applied;
            appliedSuccess = success;
            callbacksOnOwnerThread &= std::this_thread::get_id() == ownerThread;
        };
        Gui::AsyncPreviewController observed(std::move(callbacks));

        // The original false-success observations are retained in the baseline
        // receipt. Both paths now preserve the same scoped result and recovery.
        for (int phase = 0; phase < 3; ++phase) {
            const bool valid = phase != 1;
            feature->Bool.setValue(valid);
            feature->touch();
            const int before = feature->executionCount.load();
            observed.requestRecompute(/*waitForCompletion=*/true);
            QTRY_VERIFY_WITH_TIMEOUT(!observed.hasOutstandingRecompute(), 10000);
            QCOMPARE(feature->executionCount.load(), before + 1);
            QCOMPARE(feature->executedOnOwnerThread.load(), !asyncEnabled);
            QCOMPARE(feature->isValid(), valid);
            QCOMPARE(feature->isError(), !valid);
            if (!valid) {
                QVERIFY(
                    QString::fromUtf8(feature->getStatusString())
                        .contains(QStringLiteral("Ordinary preview model error"))
                );
            }
            QCOMPARE(completed, phase + 1);
            QCOMPARE(applied, phase + 1);
            QCOMPARE(completionSuccess, valid);
            QCOMPARE(appliedSuccess, valid);
            QCOMPARE(observed.didLastRecomputeSucceed(), valid);
            QVERIFY(!canceled);
            QCOMPARE(failure, valid ? App::RecomputeFailure::None : App::RecomputeFailure::ModelError);
            QCOMPARE(legacyCalls, 0);
            QVERIFY(callbacksOnOwnerThread);
            QVERIFY(!observed.isInProgress());
            QVERIFY(!observed.isQueued());
            if (!asyncEnabled) {
                QCOMPARE(syncCalls, phase + 1);
                QCOMPARE(syncModelResult, valid);
            }
            else {
                QCOMPARE(syncCalls, 0);
            }
            // A later distinct request proves the model request passed worker cleanup;
            // a GUI completion can otherwise precede App queue cleanup.
            auto drained = std::make_shared<std::atomic<bool>>(false);
            sentinel->touch();
            auto drainRequest = App::RecomputeRequest::fromDocumentObject(*sentinel);
            drainRequest.callback = [drained](App::RecomputeRequest&, App::RecomputeResult&) {
                drained->store(true);
            };
            App::GetApplication().queueRecomputeRequest(std::move(drainRequest));
            QTRY_VERIFY_WITH_TIMEOUT(drained->load(), 10000);
            qInfo(
                "preview outcome phase=%d async=%d model_valid=%d completed_success=%d "
                "applied_success=%d last_success=%d callbacks=%d execute_on_owner=%d Shape=N/A",
                phase,
                asyncEnabled,
                feature->isValid(),
                completionSuccess,
                appliedSuccess,
                observed.didLastRecomputeSucceed(),
                completed,
                feature->executedOnOwnerThread.load()
            );
        }
    }

    void legacyVoidFallbackReportsCompletionOnly()  // NOLINT
    {
        params->SetBool("EnableAsyncRecompute", false);
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("LegacyOutcome");
        QVERIFY(feature != nullptr);
        feature->Bool.setValue(false);
        feature->touch();
        int calls = 0;
        int completed = 0;
        bool completionSuccess = false;
        Gui::AsyncPreviewController::Callbacks callbacks;
        callbacks.makeRequest = [feature]() {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        callbacks.runSync = [&]() {
            ++calls;
            feature->recomputeFeature();
        };
        callbacks.onCompleted = [&](bool success, bool, App::RecomputeFailure, const std::string&) {
            ++completed;
            completionSuccess = success;
        };
        Gui::AsyncPreviewController legacy(std::move(callbacks));
        legacy.requestRecompute(true);
        QCOMPARE(calls, 1);
        QCOMPARE(completed, 1);
        QVERIFY(completionSuccess);
        QVERIFY(!feature->isValid());
        QVERIFY(!legacy.hasOutstandingRecompute());
    }

    void synchronousExceptionDoesNotRegisterOutstandingWork_data()  // NOLINT
    {
        QTest::addColumn<bool>("typed");
        QTest::newRow("legacy") << false;
        QTest::newRow("typed") << true;
    }

    void synchronousExceptionDoesNotRegisterOutstandingWork()  // NOLINT
    {
        QFETCH(bool, typed);
        params->SetBool("EnableAsyncRecompute", false);
        auto* feature = doc->addObject<App::FeatureTest>("RetryAfterSyncException");
        bool throwNext = true;
        int calls = 0;
        Gui::AsyncPreviewController::Callbacks callbacks;
        callbacks.makeRequest = [feature]() {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        if (typed) {
            callbacks.runSyncWithResult = [&]() {
                ++calls;
                if (throwNext) {
                    throw Base::RuntimeError("Synchronous task callback exception");
                }
                return Gui::recomputeDocumentObjectWithResult(feature);
            };
        }
        else {
            callbacks.runSync = [&]() {
                ++calls;
                if (throwNext) {
                    throw Base::RuntimeError("Synchronous task callback exception");
                }
                feature->recomputeFeature();
            };
        }
        Gui::AsyncPreviewController observed(std::move(callbacks));
        QSignalSpy settled(&observed, &Gui::AsyncPreviewController::recomputeSettled);
        QVERIFY_EXCEPTION_THROWN(observed.requestRecompute(true), Base::RuntimeError);
        QVERIFY(!observed.hasOutstandingRecompute());
        QVERIFY(!observed.isInProgress());
        QCOMPARE(observed.currentGeneration(), std::uint64_t(0));
        QCOMPARE(settled.count(), 0);
        throwNext = false;
        observed.requestRecompute(true);
        QCOMPARE(calls, 2);
        QCOMPARE(settled.count(), 1);
        QVERIFY(!observed.hasOutstandingRecompute());
    }

    void completionCallbacksMayReleaseController_data()  // NOLINT
    {
        QTest::addColumn<QString>("hook");
        QTest::newRow("completed") << QStringLiteral("completed");
        QTest::newRow("applied") << QStringLiteral("applied");
        QTest::newRow("state") << QStringLiteral("state");
    }

    void completionCallbacksMayReleaseController()  // NOLINT
    {
        QFETCH(QString, hook);
        params->SetBool("EnableAsyncRecompute", false);
        auto* feature = doc->addObject<App::FeatureTest>("ReleaseController");
        std::unique_ptr<Gui::AsyncPreviewController> observed;
        int completed = 0;
        int applied = 0;
        int states = 0;
        Gui::AsyncPreviewController::Callbacks callbacks;
        callbacks.makeRequest = [feature]() {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        callbacks.runSyncWithResult = [feature]() {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        callbacks.onCompleted = [&](bool, bool, App::RecomputeFailure, const std::string&) {
            ++completed;
            if (hook == QStringLiteral("completed")) {
                observed.reset();
            }
        };
        callbacks.onAppliedResult = [&](bool, bool) {
            ++applied;
            if (hook == QStringLiteral("applied")) {
                observed.reset();
            }
        };
        callbacks.onStateChanged = [&]() {
            ++states;
            observed.reset();
        };
        observed = std::make_unique<Gui::AsyncPreviewController>(std::move(callbacks));
        QPointer<Gui::AsyncPreviewController> guard(observed.get());
        QSignalSpy stateSignals(observed.get(), &Gui::AsyncPreviewController::stateChanged);
        QSignalSpy settled(observed.get(), &Gui::AsyncPreviewController::recomputeSettled);
        observed->requestRecompute(true);
        QVERIFY(!guard);
        QCOMPARE(completed, 1);
        QCOMPARE(applied, hook == QStringLiteral("completed") ? 0 : 1);
        QCOMPARE(states, hook == QStringLiteral("state") ? 1 : 0);
        QCOMPARE(stateSignals.count(), 0);
        QCOMPARE(settled.count(), 0);
    }

    void typedProgressTemplatesKeepScopedOutcomes_data()  // NOLINT
    {
        QTest::addColumn<bool>("documentRequest");
        QTest::addColumn<bool>("typed");
        QTest::newRow("object-typed") << false << true;
        QTest::newRow("document-typed") << true << true;
        QTest::newRow("object-legacy") << false << false;
        QTest::newRow("document-legacy") << true << false;
    }

    void typedProgressTemplatesKeepScopedOutcomes()  // NOLINT
    {
        QFETCH(bool, documentRequest);
        QFETCH(bool, typed);
        params->SetBool("EnableAsyncRecompute", false);
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("ProgressOutcome");
        feature->Bool.setValue(false);
        feature->touch();
        Gui::AsyncRecomputeDialogOptions options;
        options.showDialog = false;
        int calls = 0;
        const int inherited = static_cast<int>(App::RecomputeOption::InteractivePreview);
        App::ScopedRecomputeOptions scope(inherited);
        const auto native = [&]() {
            ++calls;
            return documentRequest ? Gui::recomputeDocumentWithResult(doc)
                                   : Gui::recomputeDocumentObjectWithResult(feature);
        };
        Gui::AsyncRecomputeProgressOutcome outcome;
        if (typed) {
            outcome = documentRequest ? Gui::runAsyncDocumentRecomputeProgressDialog(
                                            nullptr,
                                            QStringLiteral("Test"),
                                            {},
                                            doc,
                                            false,
                                            options,
                                            native
                                        )
                                      : Gui::runAsyncDocumentObjectRecomputeProgressDialog(
                                            nullptr,
                                            QStringLiteral("Test"),
                                            {},
                                            feature,
                                            false,
                                            options,
                                            native
                                        );
        }
        else {
            const auto legacy = [&]() {
                native();
            };
            outcome = documentRequest ? Gui::runAsyncDocumentRecomputeProgressDialog(
                                            nullptr,
                                            QStringLiteral("Test"),
                                            {},
                                            doc,
                                            false,
                                            options,
                                            legacy
                                        )
                                      : Gui::runAsyncDocumentObjectRecomputeProgressDialog(
                                            nullptr,
                                            QStringLiteral("Test"),
                                            {},
                                            feature,
                                            false,
                                            options,
                                            legacy
                                        );
        }
        QCOMPARE(calls, 1);
        QCOMPARE(feature->executionCount.load(), 1);
        QCOMPARE(feature->observedOptions.load(), inherited);
        QCOMPARE(outcome.success, !typed);
        QCOMPARE(
            outcome.failure,
            typed ? App::RecomputeFailure::ModelError : App::RecomputeFailure::None
        );
        QVERIFY(!outcome.canceled);
        QCOMPARE(App::currentRecomputeOptions(), inherited);
    }

    void subscriberRetirementPreventsGuiApplication()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("RetireBeforeGui");
        feature->Bool.setValue(true);
        const std::string featureName = feature->getNameInDocument();
        std::atomic<bool> nativeBefore {false};
        std::atomic<bool> nativeAfter {false};
        std::atomic<bool> canceledAfter {true};
        int completed = 0;
        int applied = 0;
        App::RecomputeFailure guiFailure = App::RecomputeFailure::None;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onCompleted = [&](bool success, bool, App::RecomputeFailure failure, const std::string&) {
            ++completed;
            QVERIFY(!success);
            guiFailure = failure;
        };
        hooks.onAppliedResult = [&](bool, bool) {
            ++applied;
        };
        Gui::AsyncPreviewController observed(std::move(hooks));
        blocker->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blocker)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        observed.requestRecompute(false);
        auto removal = App::RecomputeRequest::fromDocumentObject(*feature);
        removal.callback =
            [&, owner = doc, featureName](App::RecomputeRequest&, App::RecomputeResult& result) {
                nativeBefore.store(result.success);
                owner->removeObject(featureName.c_str());
            };
        App::GetApplication().queueRecomputeRequest(std::move(removal));
        auto later = App::RecomputeRequest::fromDocumentObject(*feature);
        later.callback = [&](App::RecomputeRequest& request, App::RecomputeResult& result) {
            nativeAfter.store(result.success);
            canceledAfter.store(request.progress && request.progress->wasCanceled());
        };
        App::GetApplication().queueRecomputeRequest(std::move(later));
        App::FeatureTestAsyncBlocker::releaseBlocker();
        QTRY_VERIFY_WITH_TIMEOUT(!observed.hasOutstandingRecompute(), 2000);
        QCOMPARE(completed, 1);
        QCOMPARE(applied, 0);
        QVERIFY(nativeBefore.load());
        QVERIFY(nativeAfter.load());
        QVERIFY(!canceledAfter.load());
        QVERIFY(guiFailure == App::RecomputeFailure::TargetUnavailable);
        QVERIFY(!observed.didLastRecomputeSucceed());
        QTRY_VERIFY_WITH_TIMEOUT(doc->getObject(featureName.c_str()) == nullptr, 2000);
    }

    void completionRetirementPreventsFollowingModelApplication()  // NOLINT
    {
        params->SetBool("EnableAsyncRecompute", false);
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("RetireInCompletion");
        feature->Bool.setValue(true);
        const std::string featureName = feature->getNameInDocument();
        int completed = 0;
        int applied = 0;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onCompleted = [&](bool success, bool, App::RecomputeFailure failure, const std::string&) {
            ++completed;
            QVERIFY(success);
            QVERIFY(failure == App::RecomputeFailure::None);
            doc->removeObject(featureName.c_str());
        };
        hooks.onAppliedResult = [&](bool, bool) {
            ++applied;
        };
        Gui::AsyncPreviewController observed(std::move(hooks));
        observed.requestRecompute(true);
        QCOMPARE(completed, 1);
        QCOMPARE(applied, 0);
        QVERIFY(!observed.didLastRecomputeSucceed());
        QVERIFY(!observed.hasOutstandingRecompute());
    }

    void disabledAsyncDoesNotInvokeWorkerEligibility()  // NOLINT
    {
        params->SetBool("EnableAsyncRecompute", false);
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("OwnerWithoutWorkerProbe");
        feature->Bool.setValue(true);
        int fallback = 0;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [&] {
            ++fallback;
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        Gui::AsyncPreviewController observed(std::move(hooks));
        observed.requestRecompute(false);
        QCOMPARE(fallback, 1);
        QCOMPARE(feature->executionCount.load(), 1);
        QCOMPARE(feature->eligibilityCalls.load(), 0);
        QVERIFY(feature->executedOnOwnerThread.load());
        QVERIFY(observed.didLastRecomputeSucceed());
        QVERIFY(!observed.hasOutstandingRecompute());
    }

    void completionReentryKeepsNewRetirementWakeup()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("ReentrantRetirement");
        feature->Bool.setValue(true);
        std::unique_ptr<Gui::AsyncPreviewController> observed;
        auto entered = std::make_shared<std::promise<void>>();
        auto enteredFuture = entered->get_future();
        auto release = std::make_shared<std::promise<void>>();
        auto proceed = release->get_future().share();
        int completed = 0;
        bool nestedDrain = false;
        bool released = false;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onCompleted = [&](bool, bool, App::RecomputeFailure, const std::string&) {
            if (++completed != 1) {
                return;
            }
            feature->touch();
            blocker->touch();
            App::GetApplication().queueRecomputeRequest(
                App::RecomputeRequest::fromDocumentObject(*blocker)
            );
            QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
            observed->requestRecompute(false);
            auto second = App::RecomputeRequest::fromDocumentObject(*feature);
            second.callback = [entered, proceed](App::RecomputeRequest&, App::RecomputeResult&) {
                entered->set_value();
                proceed.wait();
            };
            App::GetApplication().queueRecomputeRequest(std::move(second));
            App::FeatureTestAsyncBlocker::releaseBlocker();
            QVERIFY(enteredFuture.wait_for(2s) == std::future_status::ready);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
            nestedDrain = true;
        };
        observed = std::make_unique<Gui::AsyncPreviewController>(std::move(hooks));
        BOOST_SCOPE_EXIT_ALL(&)
        {
            if (!released) {
                release->set_value();
            }
        };
        observed->requestRecompute(false);
        QTRY_VERIFY_WITH_TIMEOUT(nestedDrain, 2000);
        QCOMPARE(completed, 1);
        QVERIFY(observed->hasOutstandingRecompute());
        released = true;
        release->set_value();
        QTRY_VERIFY_WITH_TIMEOUT(!observed->hasOutstandingRecompute(), 2000);
        QCOMPARE(completed, 2);
        QCOMPARE(feature->executionCount.load(), 2);
    }

    void resultDoesNotApplyUntilSubscriberCleanupRetires()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("TerminalRetirement");
        feature->Bool.setValue(true);
        int completed = 0;
        bool nativeQuiescent = false;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onCompleted = [&](bool, bool, App::RecomputeFailure, const std::string&) {
            ++completed;
            try {
                auto operation = App::RecomputeDetail::Coordinator::instance().atomic();
                nativeQuiescent = true;
            }
            catch (const App::RecomputeDetail::AdmissionRefusal&) {
                nativeQuiescent = false;
            }
        };
        Gui::AsyncPreviewController observed(std::move(hooks));
        auto entered = std::make_shared<std::promise<void>>();
        auto enteredFuture = entered->get_future();
        auto release = std::make_shared<std::promise<void>>();
        auto proceed = release->get_future().share();
        bool released = false;
        BOOST_SCOPE_EXIT_ALL(&)
        {
            if (!released) {
                release->set_value();
            }
        };
        blocker->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blocker)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        observed.requestRecompute(false);
        auto second = App::RecomputeRequest::fromDocumentObject(*feature);
        second.callback = [entered, proceed](App::RecomputeRequest&, App::RecomputeResult&) {
            entered->set_value();
            proceed.wait();
        };
        App::GetApplication().queueRecomputeRequest(std::move(second));
        App::FeatureTestAsyncBlocker::releaseBlocker();
        QVERIFY(enteredFuture.wait_for(2s) == std::future_status::ready);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(completed, 0);
        QVERIFY(observed.hasOutstandingRecompute());
        released = true;
        release->set_value();
        QTRY_VERIFY_WITH_TIMEOUT(!observed.hasOutstandingRecompute(), 2000);
        QCOMPARE(completed, 1);
        QVERIFY(nativeQuiescent);
        QCOMPARE(feature->executionCount.load(), 1);
    }

    void borrowedGuiSignalCannotCloseItsNativeOperation()  // NOLINT
    {
        resetFakeMainThreadHooks();
        App::MainThreadSignalConfig::setHooks(
            &fakeIsMainThread,
            &fakeInvokeOnMainThread,
            &fakePumpMainThreadEvents
        );
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("BorrowedSignal");
        feature->Bool.setValue(true);
        bool attempted = false;
        bool closed = true;
        bool onGui = false;
        fastsignals::scoped_connection before(
            doc->signalBeforeRecompute.connect([&](const App::Document&) {
                App::MainThreadSignalConfig::callOnMainThreadSync([&] {
                    onGui = fakeIsMainThread();
                    attempted = true;
                    closed = App::GetApplication().closeDocument(docName.c_str());
                });
            })
        );
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature, true);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature, true);
        };
        Gui::AsyncPreviewController observed(std::move(hooks));
        observed.requestRecompute(true);
        QVERIFY(attempted);
        QVERIFY(onGui);
        QVERIFY(!closed);
        QCOMPARE(App::GetApplication().getDocument(docName.c_str()), doc);
        QCOMPARE(feature->executionCount.load(), 1);
        QVERIFY(observed.didLastRecomputeSucceed());
        QVERIFY(!observed.hasOutstandingRecompute());
    }

    void borrowedGuiSignalMayReleaseWaitingController()  // NOLINT
    {
        resetFakeMainThreadHooks();
        App::MainThreadSignalConfig::setHooks(
            &fakeIsMainThread,
            &fakeInvokeOnMainThread,
            &fakePumpMainThreadEvents
        );
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("WaitingRelease");
        feature->Bool.setValue(true);
        std::unique_ptr<Gui::AsyncPreviewController> observed;
        bool destroyed = false;
        fastsignals::scoped_connection before(
            doc->signalBeforeRecompute.connect([&](const App::Document&) {
                App::MainThreadSignalConfig::callOnMainThreadSync([&] {
                    observed.reset();
                    destroyed = true;
                });
            })
        );
        auto request = App::RecomputeRequest::fromDocumentObject(*feature, true);
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [request] {
            return request;
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature, true);
        };
        observed = std::make_unique<Gui::AsyncPreviewController>(std::move(hooks));
        observed->requestRecompute(true);
        QVERIFY(destroyed);
        QVERIFY(!observed);
        auto& app = App::GetApplication();
        app.cancelRecomputeRequest(request);
        auto nativeRetired = [&] {
            fakePumpMainThreadEvents();
            return app.classifyRecomputeRequest(request, false)
                == App::RecomputeDetail::RouteDisposition::OwnerReady;
        };
        QTRY_VERIFY_WITH_TIMEOUT(nativeRetired(), 2000);
        QCOMPARE(App::GetApplication().getDocument(docName.c_str()), doc);
    }

    void stateCallbackCanCancelBeforeSubmission()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("BeforeSubmission");
        feature->Bool.setValue(true);
        std::unique_ptr<Gui::AsyncPreviewController> observed;
        bool stopOnce = true;
        int completed = 0;
        bool canceled = false;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onStateChanged = [&] {
            if (stopOnce) {
                stopOnce = false;
                observed->stopPendingRecompute();
            }
        };
        hooks.onCompleted =
            [&](bool success, bool wasCanceled, App::RecomputeFailure failure, const std::string&) {
                ++completed;
                QVERIFY(!success);
                QVERIFY(failure == App::RecomputeFailure::Canceled);
                canceled = wasCanceled;
            };
        observed = std::make_unique<Gui::AsyncPreviewController>(std::move(hooks));
        QSignalSpy settled(observed.get(), &Gui::AsyncPreviewController::recomputeSettled);
        observed->requestRecompute(false);
        QVERIFY(!observed->hasOutstandingRecompute());
        QVERIFY(canceled);
        QCOMPARE(completed, 1);
        QCOMPARE(settled.count(), 1);
        QCOMPARE(feature->executionCount.load(), 0);
        QVERIFY(observed->currentGeneration() > 0);
    }

    void completionReentryDoesNotApplyOlderResultOrSettleTwice()  // NOLINT
    {
        params->SetBool("EnableAsyncRecompute", false);
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("CompletionReentry");
        feature->Bool.setValue(true);
        std::unique_ptr<Gui::AsyncPreviewController> observed;
        int completed = 0;
        int applied = 0;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onCompleted = [&](bool, bool, App::RecomputeFailure, const std::string&) {
            if (++completed == 1) {
                feature->touch();
                observed->requestRecompute(true);
            }
        };
        hooks.onAppliedResult = [&](bool, bool) {
            ++applied;
        };
        observed = std::make_unique<Gui::AsyncPreviewController>(std::move(hooks));
        QSignalSpy settled(observed.get(), &Gui::AsyncPreviewController::recomputeSettled);
        observed->requestRecompute(true);
        QCOMPARE(completed, 2);
        QCOMPARE(applied, 1);
        QCOMPARE(settled.count(), 1);
        QCOMPARE(feature->executionCount.load(), 2);
        QCOMPARE(observed->currentGeneration(), std::uint64_t(2));
        QVERIFY(!observed->hasOutstandingRecompute());
    }

    void progressStateCallbackCanReleaseControllerBeforeSubmission()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("ProgressRelease");
        feature->Bool.setValue(true);
        std::unique_ptr<Gui::AsyncPreviewController> observed;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onStateChanged = [&] {
            observed.reset();
        };
        observed = std::make_unique<Gui::AsyncPreviewController>(std::move(hooks));
        observed->requestRecompute(false);
        QVERIFY(!observed);
        QCOMPARE(feature->executionCount.load(), 0);
    }

    void queuedCancellationRetiresAndFreshRetryExecutes()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("QueuedRetry");
        feature->Bool.setValue(true);
        int completed = 0;
        bool canceled = false;
        bool success = false;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onCompleted = [&](bool ok, bool wasCanceled, App::RecomputeFailure, const std::string&) {
            ++completed;
            success = ok;
            canceled = wasCanceled;
        };
        Gui::AsyncPreviewController observed(std::move(hooks));
        blocker->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blocker)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        observed.requestRecompute(false);
        const auto firstGeneration = observed.currentGeneration();
        observed.stopPendingRecompute();
        QTRY_VERIFY_WITH_TIMEOUT(!observed.hasOutstandingRecompute(), 2000);
        QCOMPARE(completed, 1);
        QVERIFY(canceled);
        QVERIFY(!success);
        QCOMPARE(observed.currentGeneration(), firstGeneration);
        QCOMPARE(feature->executionCount.load(), 0);
        observed.requestRecompute(false);
        QVERIFY(observed.currentGeneration() > firstGeneration);
        QVERIFY(observed.hasOutstandingRecompute());
        App::FeatureTestAsyncBlocker::releaseBlocker();
        QTRY_VERIFY_WITH_TIMEOUT(!observed.hasOutstandingRecompute(), 2000);
        QCOMPARE(completed, 2);
        QVERIFY(success);
        QVERIFY(!canceled);
        QCOMPARE(feature->executionCount.load(), 1);
    }

    void opaqueObserverAsyncSubmissionExecutesAfterReleaseAndRefusesFlushWait_data()  // NOLINT
    {
        QTest::addColumn<bool>("asyncEnabled");
        QTest::newRow("enabled") << true;
        QTest::newRow("disabled") << false;
    }

    void opaqueObserverAsyncSubmissionExecutesAfterReleaseAndRefusesFlushWait()  // NOLINT
    {
        QFETCH(bool, asyncEnabled);
        params->SetBool("EnableAsyncRecompute", asyncEnabled);
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("OpaqueQueued");
        feature->Bool.setValue(true);
        int fallbackCalls = 0;
        App::RecomputeFailure failure = App::RecomputeFailure::None;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [&] {
            ++fallbackCalls;
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onCompleted = [&](bool, bool, App::RecomputeFailure value, const std::string&) {
            failure = value;
        };
        Gui::AsyncPreviewController observed(std::move(hooks));
        {
            auto operation = App::RecomputeDetail::Coordinator::instance().atomic();
            observed.requestRecompute(false);
            QCOMPARE(feature->executionCount.load(), 0);
            QCOMPARE(fallbackCalls, 0);
            if (asyncEnabled) {
                QVERIFY(observed.hasOutstandingRecompute());
                bool refused = false;
                try {
                    observed.flushPendingRecompute();
                }
                catch (const App::RecomputeDetail::AdmissionRefusal&) {
                    refused = true;
                }
                QVERIFY(refused);
                QVERIFY(observed.hasOutstandingRecompute());
            }
            else {
                QVERIFY(!observed.hasOutstandingRecompute());
                QVERIFY(!observed.didLastRecomputeSucceed());
                QVERIFY(failure == App::RecomputeFailure::Exception);
            }
        }
        if (!asyncEnabled) {
            QCOMPARE(feature->executionCount.load(), 0);
            observed.requestRecompute(false);
            QCOMPARE(fallbackCalls, 1);
            QVERIFY(feature->executedOnOwnerThread.load());
        }
        QTRY_VERIFY_WITH_TIMEOUT(!observed.hasOutstandingRecompute(), 2000);
        QVERIFY(observed.didLastRecomputeSucceed());
        QCOMPARE(feature->executionCount.load(), 1);
    }

    void stopPendingRecomputeCancelsScheduledBurstBeforeQueueing()  // NOLINT
    {
        QSignalSpy settledSpy(controller.get(), &Gui::AsyncPreviewController::recomputeSettled);

        blocker->touch();
        controller->setSchedulerInterval(200);
        controller->scheduleRecompute();
        controller->scheduleRecompute();
        QVERIFY(controller->hasScheduledRecompute());

        controller->stopPendingRecompute();

        QVERIFY(!controller->hasScheduledRecompute());
        QVERIFY(!controller->hasOutstandingRecompute());
        QCOMPARE(callbackState.completedCount, 0);
        QCOMPARE(callbackState.appliedCount, 0);
        QCOMPARE(settledSpy.count(), 0);

        QTest::qWait(300);
        QCOMPARE(App::FeatureTestAsyncBlocker::getExecutionCount(), 0);
    }

    void scheduledRecomputeBurstCoalescesIntoSingleRun()  // NOLINT
    {
        QSignalSpy settledSpy(controller.get(), &Gui::AsyncPreviewController::recomputeSettled);

        blocker->touch();
        controller->setSchedulerInterval(20);
        controller->scheduleRecompute();
        controller->scheduleRecompute();
        controller->scheduleRecompute();

        QVERIFY(controller->hasScheduledRecompute());
        QTRY_COMPARE_WITH_TIMEOUT(App::FeatureTestAsyncBlocker::getExecutionCount(), 1, 2000);
        QTRY_VERIFY_WITH_TIMEOUT(controller->isInProgress(), 2000);
        QVERIFY(!controller->isQueued());

        App::FeatureTestAsyncBlocker::releaseBlocker();

        QTRY_COMPARE_WITH_TIMEOUT(settledSpy.count(), 1, 2000);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(callbackState.stateChangedCount > 0);
    }

    void duplicateInFlightRequestSettlesOnlyAfterFinalRun()  // NOLINT
    {
        QSignalSpy settledSpy(controller.get(), &Gui::AsyncPreviewController::recomputeSettled);

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);

        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        QVERIFY(controller->isInProgress());
        QVERIFY(!controller->isQueued());

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);

        QVERIFY(controller->isInProgress());
        QVERIFY(controller->isQueued());

        App::FeatureTestAsyncBlocker::releaseBlocker();

        QTRY_COMPARE_WITH_TIMEOUT(App::FeatureTestAsyncBlocker::getExecutionCount(), 2, 2000);
        QTRY_COMPARE_WITH_TIMEOUT(settledSpy.count(), 1, 2000);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(callbackState.stateChangedCount > 0);
        QVERIFY(!controller->hasOutstandingRecompute());
    }

    void cancelAndSettleRetiresRunningAndCoalescedPreviewWithoutRerun()  // NOLINT
    {
        blocker->touch();
        controller->requestRecompute(false);
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        controller->requestRecompute(false);
        QVERIFY(controller->isQueued());
        QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [] { App::FeatureTestAsyncBlocker::releaseBlocker(); },
            Qt::QueuedConnection
        );
        QVERIFY(controller->stopPendingRecomputeAndWait());
        QVERIFY(!controller->hasOutstandingRecompute());
        QVERIFY(!controller->hasScheduledRecompute());
        QCOMPARE(App::FeatureTestAsyncBlocker::getExecutionCount(), 1);
        // This actual destructive admission would refuse a still-live receiver
        // pin even after a callback promise had been published.
        {
            auto writer
                = App::RecomputeDetail::Coordinator::instance().mutation({blocker}, {doc}, true);
            QVERIFY(static_cast<bool>(writer));
        }
        QCOMPARE(callbackState.completedCount, 1);
        QVERIFY(callbackState.canceled);
    }

    void cancelAndSettleQueuedPreviewDoesNotLaunchNativeWork()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("SettleQueued");
        feature->Bool.setValue(true);
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        Gui::AsyncPreviewController observed(std::move(hooks));
        blocker->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blocker)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        observed.requestRecompute(false);
        QVERIFY(observed.stopPendingRecomputeAndWait());
        QVERIFY(!observed.hasOutstandingRecompute());
        QCOMPARE(feature->executionCount.load(), 0);
        App::FeatureTestAsyncBlocker::releaseBlocker();
    }

    void cancelAndSettleBorrowedWaitRefusesBeforeCancellation()  // NOLINT
    {
        resetFakeMainThreadHooks();
        App::MainThreadSignalConfig::setHooks(
            &fakeIsMainThread,
            &fakeInvokeOnMainThread,
            &fakePumpMainThreadEvents
        );
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("SettleBorrowed");
        feature->Bool.setValue(true);
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature, true);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature, true);
        };
        Gui::AsyncPreviewController observed(std::move(hooks));
        observed.setSchedulerInterval(60000);
        bool attempted = false;
        bool settled = true;
        bool stillScheduled = false;
        bool canceled = true;
        fastsignals::scoped_connection before(
            doc->signalBeforeRecompute.connect([&](const App::Document&) {
                App::MainThreadSignalConfig::callOnMainThreadSync([&] {
                    attempted = true;
                    observed.scheduleRecompute();
                    settled = observed.stopPendingRecomputeAndWait();
                    stillScheduled = observed.hasScheduledRecompute();
                    canceled = observed.isCancelRequested();
                    observed.stopScheduledRecompute();
                });
            })
        );
        observed.requestRecompute(true);
        QVERIFY(attempted);
        QVERIFY(!settled);
        QVERIFY(stillScheduled);
        QVERIFY(!canceled);
        QCOMPARE(feature->executionCount.load(), 1);
        QVERIFY(observed.didLastRecomputeSucceed());
    }

    void cancelAndSettleSurvivesControllerDeletionInCompletion()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("SettleDeleted");
        feature->Bool.setValue(true);
        std::unique_ptr<Gui::AsyncPreviewController> observed;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onCompleted = [&](bool, bool, App::RecomputeFailure, const std::string&) {
            observed.reset();
        };
        observed = std::make_unique<Gui::AsyncPreviewController>(std::move(hooks));
        blocker->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blocker)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        observed->requestRecompute(false);
        QVERIFY(!observed->stopPendingRecomputeAndWait());
        QVERIFY(!observed);
        QCOMPARE(feature->executionCount.load(), 0);
        App::FeatureTestAsyncBlocker::releaseBlocker();
    }

    void cancelAndSettleSessionCallbackCanDeleteItsOwnerWithoutUiTail()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("SettleSessionDeleted");
        feature->Bool.setValue(true);
        QWidget owner;
        QWidget status(&owner);
        QProgressBar progress(&owner);
        QLabel label(&owner);
        QPushButton cancel(&owner);
        std::unique_ptr<Gui::AsyncPreviewSession> session;
        bool armed = false;
        bool deleted = false;
        unsigned translationsAfterDelete = 0;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onStateChanged = [&] {
            if (armed) {
                armed = false;
                session.reset();
                deleted = true;
                label.setText(QStringLiteral("retired session"));
            }
        };
        session = std::make_unique<Gui::AsyncPreviewSession>(std::move(hooks), &owner);
        session->bindWidgets({&status, &progress, &label, &cancel}, [&](const char* text) {
            if (deleted) {
                ++translationsAfterDelete;
            }
            return QString::fromUtf8(text);
        });
        session->setForcedBusy(true);
        blocker->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blocker)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        session->controller()->requestRecompute(false);
        armed = true;
        QVERIFY(!session->stopPendingRecomputeAndWait());
        QVERIFY(deleted);
        QVERIFY(!session);
        QCOMPARE(translationsAfterDelete, 0U);
        QCOMPARE(label.text(), QStringLiteral("retired session"));
        QCOMPARE(feature->executionCount.load(), 0);
        App::FeatureTestAsyncBlocker::releaseBlocker();
    }

    void activityFenceDetectsLaterPanelReentryAfterQuiescence_data()  // NOLINT
    {
        QTest::addColumn<bool>("scheduleOnly");
        QTest::newRow("new-schedule-stopped") << true;
        QTest::newRow("new-registration-canceled-and-retired") << false;
    }

    void activityFenceDetectsLaterPanelReentryAfterQuiescence()  // NOLINT
    {
        QFETCH(bool, scheduleOnly);
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("SettlePanelFence");
        feature->Bool.setValue(true);
        Gui::AsyncPreviewController::Callbacks firstHooks;
        firstHooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        firstHooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        Gui::AsyncPreviewSession first(std::move(firstHooks));
        first.setSchedulerInterval(60000);
        Gui::AsyncPreviewController::Callbacks secondHooks;
        secondHooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        secondHooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        secondHooks.onCompleted = [&](bool, bool, App::RecomputeFailure, const std::string&) {
            if (scheduleOnly) {
                first.scheduleRecompute();
                first.stopScheduledRecompute();
            }
            else {
                first.controller()->requestRecompute(false);
                first.stopPendingRecompute();
            }
        };
        Gui::AsyncPreviewSession second(std::move(secondHooks));
        blocker->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blocker)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        QVERIFY(first.stopPendingRecomputeAndWait());
        const auto firstFence = first.recomputeActivityToken();
        second.controller()->requestRecompute(false);
        QVERIFY(second.stopPendingRecomputeAndWait());
        QVERIFY(!first.hasOutstandingRecompute());
        QVERIFY(!first.controller()->hasScheduledRecompute());
        QVERIFY(first.recomputeActivityToken() != firstFence);
        QCOMPARE(feature->executionCount.load(), 0);
        App::FeatureTestAsyncBlocker::releaseBlocker();
    }

    void cancelAndSettlePreservesReentrantScheduledWork()  // NOLINT
    {
        auto* feature = doc->addObject<PartDesign::PreviewOutcomeFeature>("SettleReentry");
        feature->Bool.setValue(true);
        std::unique_ptr<Gui::AsyncPreviewController> observed;
        Gui::AsyncPreviewController::Callbacks hooks;
        hooks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        hooks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        hooks.onCompleted = [&](bool, bool, App::RecomputeFailure, const std::string&) {
            observed->scheduleRecompute();
        };
        observed = std::make_unique<Gui::AsyncPreviewController>(std::move(hooks));
        observed->setSchedulerInterval(60000);
        blocker->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blocker)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        observed->requestRecompute(false);
        QVERIFY(!observed->stopPendingRecomputeAndWait());
        QVERIFY(observed->hasScheduledRecompute());
        QCOMPARE(feature->executionCount.load(), 0);
        observed->stopScheduledRecompute();
        App::FeatureTestAsyncBlocker::releaseBlocker();
    }

    void stopPendingRecomputeCancelsInFlightAndClearsQueuedRerun()  // NOLINT
    {
        QSignalSpy settledSpy(controller.get(), &Gui::AsyncPreviewController::recomputeSettled);

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(controller->isQueued());

        controller->stopPendingRecompute();

        QVERIFY(controller->isCancelRequested());
        QVERIFY(controller->isInProgress());
        QVERIFY(!controller->isQueued());

        App::FeatureTestAsyncBlocker::releaseBlocker();

        QTRY_COMPARE_WITH_TIMEOUT(App::FeatureTestAsyncBlocker::getExecutionCount(), 1, 2000);
        QTRY_COMPARE_WITH_TIMEOUT(settledSpy.count(), 1, 2000);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(!callbackState.success);
        QVERIFY(callbackState.canceled);
        QVERIFY(callbackState.failure == App::RecomputeFailure::Canceled);
        QCOMPARE(QString::fromStdString(callbackState.message), QString("User aborted"));
        QVERIFY(!callbackState.appliedSuccess);
        QVERIFY(callbackState.appliedCanceled);
        QVERIFY(!controller->hasOutstandingRecompute());
        QVERIFY(!controller->isInProgress());
        QVERIFY(!controller->isQueued());
        QVERIFY(!controller->isCancelRequested());
    }

    void flushPendingRecomputeRunsScheduledRecomputeImmediately()  // NOLINT
    {
        blocker->touch();
        controller->setSchedulerInterval(200);
        controller->scheduleRecompute();
        QVERIFY(controller->hasScheduledRecompute());
        QVERIFY(!controller->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(50ms);
            App::FeatureTestAsyncBlocker::releaseBlocker();
        });

        controller->flushPendingRecompute();
        releaser.join();

        QCOMPARE(App::FeatureTestAsyncBlocker::getExecutionCount(), 1);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(!controller->hasScheduledRecompute());
        QVERIFY(!controller->hasOutstandingRecompute());
        QVERIFY(!controller->isInProgress());
    }

    void flushPendingRecomputeWaitsForQueuedRunWithoutExtraRerun()  // NOLINT
    {
        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(controller->isQueued());

        std::thread releaser([]() {
            std::this_thread::sleep_for(50ms);
            App::FeatureTestAsyncBlocker::releaseBlocker();
        });

        controller->flushPendingRecompute();
        releaser.join();

        QCOMPARE(App::FeatureTestAsyncBlocker::getExecutionCount(), 2);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(!controller->hasOutstandingRecompute());
        QVERIFY(!controller->isInProgress());
        QVERIFY(!controller->isQueued());
    }

    void delayedPreviewUiDoesNotReuseTimerAcrossGenerations()  // NOLINT
    {
        QWidget statusWidget;
        QHBoxLayout layout(&statusWidget);
        QProgressBar progressBar(&statusWidget);
        QLabel statusLabel(&statusWidget);
        QPushButton cancelButton(&statusWidget);
        layout.addWidget(&progressBar);
        layout.addWidget(&statusLabel);
        layout.addWidget(&cancelButton);

        Gui::AsyncPreviewController::Callbacks callbacks;
        callbacks.makeRequest = [this]() {
            auto* object = static_cast<App::DocumentObject*>(blocker);
            return object ? App::RecomputeRequest::fromDocumentObject(*object)
                          : App::RecomputeRequest {};
        };
        callbacks.runSync = [this]() {
            if (blocker) {
                blocker->recomputeFeature();
            }
        };
        Gui::AsyncPreviewSession session(std::move(callbacks));
        session.bindWidgets(
            {&statusWidget, &progressBar, &statusLabel, &cancelButton},
            [](const char* text) { return QString::fromUtf8(text); }
        );
        session.setProgressUiDelay(300);

        blocker->touch();
        session.requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        QVERIFY(session.controller()->isInProgress());
        QVERIFY(statusLabel.isHidden());

        QTest::qWait(120);

        blocker->touch();
        session.requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(session.controller()->isQueued());

        QTest::qWait(220);

        QVERIFY(session.controller()->isInProgress());
        QVERIFY2(
            statusLabel.isHidden(),
            "an older delay timer must not reveal progress UI for a newer preview generation"
        );

        App::FeatureTestAsyncBlocker::releaseBlocker();
        QTRY_VERIFY_WITH_TIMEOUT(!session.hasOutstandingRecompute(), 2000);
        QVERIFY(statusLabel.isHidden());
    }

    void waitForCompletionPumpsMainThreadHooksWhileBlocked()  // NOLINT
    {
        resetFakeMainThreadHooks();
        App::MainThreadSignalConfig::setHooks(
            &fakeIsMainThread,
            &fakeInvokeOnMainThread,
            &fakePumpMainThreadEvents
        );

        blocker->touch();
        bool helperObservedStart = false;
        bool helperInvokeReturned = false;
        bool queuedMainThreadCallRan = false;

        std::thread helper([&]() {
            helperObservedStart = App::FeatureTestAsyncBlocker::waitUntilStarted(2s);
            if (!helperObservedStart) {
                return;
            }

            App::MainThreadSignalConfig::invoke(
                [&queuedMainThreadCallRan]() { queuedMainThreadCallRan = true; },
                /*blocking=*/true
            );
            helperInvokeReturned = true;
            App::FeatureTestAsyncBlocker::releaseBlocker();
        });

        controller->requestRecompute(/*waitForCompletion=*/true);
        helper.join();

        QVERIFY(helperObservedStart);
        QVERIFY(queuedMainThreadCallRan);
        QVERIFY(helperInvokeReturned);
        QVERIFY(getFakeMainThreadHookState().pumpCount > 0);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(!controller->hasOutstandingRecompute());
    }

    void documentRecomputeProgressDialogUsesStableDocumentRequestWithoutObjectAnchor()  // NOLINT
    {
        auto* removed = doc->addObject("App::FeatureTest", "RemovedFeature");
        QVERIFY(removed != nullptr);
        const std::string removedName = removed->getNameInDocument();
        doc->removeObject(removedName.c_str());
        QVERIFY(doc->getObject(removedName.c_str()) == nullptr);

        blocker->touch();
        bool releaserObservedStart = false;
        std::thread releaser([&]() {
            releaserObservedStart = App::FeatureTestAsyncBlocker::waitUntilStarted(2s);
            App::FeatureTestAsyncBlocker::releaseBlocker();
        });

        const auto outcome = Gui::runAsyncDocumentRecomputeProgressDialog(
            nullptr,
            QStringLiteral("Document recompute"),
            QStringLiteral("Computing document..."),
            doc,
            /*force=*/false,
            [this]() {
                if (doc) {
                    doc->recompute();
                }
            }
        );
        releaser.join();

        QVERIFY(releaserObservedStart);
        QVERIFY(outcome.success);
        QVERIFY(!outcome.canceled);
        QCOMPARE(outcome.failure, App::RecomputeFailure::None);
        QCOMPARE(App::FeatureTestAsyncBlocker::getExecutionCount(), 1);
    }

    void documentRecomputeProgressDialogDoesNotNeedObjectAnchor()  // NOLINT
    {
        const std::string emptyDocName = App::GetApplication().getUniqueDocumentName(
            "async_progress_empty"
        );
        App::Document* emptyDoc = App::GetApplication().newDocument(emptyDocName.c_str(), "testUser");
        QVERIFY(emptyDoc != nullptr);

        int recomputeCount = 0;
        const auto outcome = Gui::runAsyncDocumentRecomputeProgressDialog(
            nullptr,
            QStringLiteral("Document recompute"),
            QStringLiteral("Computing document..."),
            emptyDoc,
            /*force=*/false,
            [&recomputeCount]() { ++recomputeCount; }
        );

        App::GetApplication().closeDocument(emptyDocName.c_str());

        QVERIFY(outcome.success);
        QVERIFY(!outcome.canceled);
        QCOMPARE(outcome.failure, App::RecomputeFailure::None);
        QCOMPARE(recomputeCount, 0);
    }

private:
    Base::Reference<ParameterGrp> params;
    bool oldAsyncEnabled = true;
    bool hadAsyncEnabled = false;
    CallbackState callbackState;
    std::string docName;
    App::Document* doc = nullptr;
    App::FeatureTestAsyncBlocker* blocker = nullptr;
    std::unique_ptr<Gui::AsyncPreviewController> controller;
};

QTEST_MAIN(testAsyncPreviewController)

#include "AsyncPreviewControllerTest.moc"
