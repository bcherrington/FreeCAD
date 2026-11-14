// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <atomic>
#include <algorithm>
#include <chrono>
#include <future>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/scope_exit.hpp>
#include <gtest/gtest.h>

#include "App/Application.h"
#include "App/Document.h"
#include "App/FeatureTest.h"
#include "Base/Console.h"
#include "Base/Interpreter.h"
#include <src/App/InitApplication.h>

using namespace std::chrono_literals;

namespace
{
std::thread::id spec012FixtureThread;
}

namespace App
{
// Native ordinary-return failure, with no Python proxy or production fixture.
class Spec012OutcomeFeature: public FeatureTest
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::Spec012OutcomeFeature);

public:
    Spec012OutcomeFeature() = default;
    ~Spec012OutcomeFeature() override = default;
    int mode {0};
    static unsigned long nextSerial()
    {
        static std::atomic<unsigned long> serial {0};
        return ++serial;
    }
    const unsigned long lifetimeSerial {nextSerial()};
    std::atomic<bool> workerAllowed {true};
    bool canRecomputeOnWorker() const override
    {
        return workerAllowed.load();
    }
    std::thread::id executionThread;
    int workerEligibilityAtExecute {-1};
    int observedOptions {0};
    App::RecomputeProgressHandle* observedProgress {nullptr};

    DocumentObjectExecReturn* execute() override
    {
        executionThread = std::this_thread::get_id();
        workerEligibilityAtExecute = canRecomputeOnWorker();
        observedOptions = App::currentRecomputeOptions();
        observedProgress = App::currentRecomputeProgress();
        if (mode == 1) {
            ExecCount.setValue(ExecCount.getValue() + 1);
            return new DocumentObjectExecReturn("Spec012 ordinary native feature error");
        }
        if (mode == 2 || mode == 3) {
            ExecCount.setValue(ExecCount.getValue() + 1);
            if (mode == 2) {
                throw Base::UserAbortException();
            }
            return new DocumentObjectExecReturn(
                "Spec012 returned cancellation",
                nullptr,
                RecomputeIssueKind::Canceled
            );
        }
        return FeatureTest::execute();
    }
};
}  // namespace App

PROPERTY_SOURCE(App::Spec012OutcomeFeature, App::FeatureTest)

namespace App
{
class Spec012UnsafeOutcomeFeature: public Spec012OutcomeFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::Spec012UnsafeOutcomeFeature);

public:
    Spec012UnsafeOutcomeFeature()
    {
        workerAllowed = false;
    }
    ~Spec012UnsafeOutcomeFeature() override = default;
};
}  // namespace App

PROPERTY_SOURCE(App::Spec012UnsafeOutcomeFeature, App::Spec012OutcomeFeature)


namespace
{

struct CapturedLog
{
    std::string notifier;
    std::string msg;
    Base::LogStyle level;
};

class CapturingLogger final: public Base::ILogger
{
public:
    explicit CapturingLogger(std::vector<CapturedLog>& out, std::mutex& outMutex)
        : output(out)
        , mutex(outMutex)
    {}

    void sendLog(
        const std::string& notifiername,
        const std::string& msg,
        Base::LogStyle level,
        Base::IntendedRecipient,
        Base::ContentType
    ) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        output.push_back({notifiername, msg, level});
    }

private:
    std::vector<CapturedLog>& output;
    std::mutex& mutex;
};

class ScopedObserver
{
public:
    explicit ScopedObserver(Base::ILogger& logger)
        : logger(logger)
    {
        Base::Console().attachObserver(&logger);
    }

    ~ScopedObserver()
    {
        Base::Console().detachObserver(&logger);
    }

    ScopedObserver(const ScopedObserver&) = delete;
    ScopedObserver(ScopedObserver&&) = delete;
    ScopedObserver& operator=(const ScopedObserver&) = delete;
    ScopedObserver& operator=(ScopedObserver&&) = delete;

private:
    Base::ILogger& logger;
};

}  // namespace

class AsyncRecomputeTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        App::Spec012OutcomeFeature::init();
        App::Spec012UnsafeOutcomeFeature::init();
    }

    void SetUp() override
    {
        spec012FixtureThread = std::this_thread::get_id();
        _docName = App::GetApplication().getUniqueDocumentName("async_recompute");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        if (!_docName.empty() && App::GetApplication().getDocument(_docName.c_str())) {
            App::GetApplication().closeDocument(_docName.c_str());
        }
    }

    std::string _docName;
    App::Document* _doc {};
};

TEST_F(AsyncRecomputeTest, CloseDocumentWaitsForInFlightAsyncRecompute)
{
    auto* object = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(object, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    object->touch();

    App::GetApplication().queueRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*object));

    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    auto closeFuture = std::async(std::launch::async, [this] {
        return App::GetApplication().closeDocument(_docName.c_str());
    });

    EXPECT_EQ(closeFuture.wait_for(50ms), std::future_status::timeout);

    App::FeatureTestAsyncBlocker::releaseBlocker();

    ASSERT_EQ(closeFuture.wait_for(2s), std::future_status::ready);
    EXPECT_TRUE(closeFuture.get());

    _doc = nullptr;
}

TEST_F(AsyncRecomputeTest, CloseDocumentCancelsQueuedAsyncRecomputeCallback)
{
    auto* blockingObject = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blockingObject, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    auto queuedDocName = App::GetApplication().getUniqueDocumentName("async_recompute_queued");
    auto* queuedDoc = App::GetApplication().newDocument(queuedDocName.c_str(), "testUser");
    ASSERT_NE(queuedDoc, nullptr);

    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (App::GetApplication().getDocument(queuedDocName.c_str())) {
            App::GetApplication().closeDocument(queuedDocName.c_str());
        }
    };

    auto* queuedObject = dynamic_cast<App::FeatureTest*>(
        queuedDoc->addObject("App::FeatureTest", "QueuedFeature")
    );
    ASSERT_NE(queuedObject, nullptr);

    blockingObject->touch();
    App::GetApplication().queueRecomputeRequest(
        App::RecomputeRequest::fromDocumentObject(*blockingObject)
    );

    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    std::promise<App::RecomputeFailure> callbackFailure;
    std::atomic<int> callbackCount {0};

    queuedObject->touch();
    auto request = App::RecomputeRequest::fromDocumentObject(*queuedObject);
    auto callbackFuture = callbackFailure.get_future();
    request.callback =
        [&callbackFailure,
         &callbackCount](const App::RecomputeRequest&, const App::RecomputeResult& result) {
            if (callbackCount.fetch_add(1) == 0) {
                callbackFailure.set_value(result.failure);
            }
        };

    App::GetApplication().queueRecomputeRequest(std::move(request));

    auto closeFuture = std::async(std::launch::async, [&queuedDocName] {
        return App::GetApplication().closeDocument(queuedDocName.c_str());
    });

    ASSERT_EQ(callbackFuture.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(callbackFuture.get(), App::RecomputeFailure::Canceled);
    EXPECT_EQ(callbackCount.load(), 1);

    ASSERT_EQ(closeFuture.wait_for(2s), std::future_status::ready);
    EXPECT_TRUE(closeFuture.get());

    App::FeatureTestAsyncBlocker::releaseBlocker();
}

TEST_F(AsyncRecomputeTest, CancelQueuedAsyncRecomputeDoesNotWaitForOtherDocumentInFlight)
{
    auto* blockingObject = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blockingObject, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    auto queuedDocName = App::GetApplication().getUniqueDocumentName("async_recompute_queued");
    auto* queuedDoc = App::GetApplication().newDocument(queuedDocName.c_str(), "testUser");
    ASSERT_NE(queuedDoc, nullptr);

    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (App::GetApplication().getDocument(queuedDocName.c_str())) {
            App::GetApplication().closeDocument(queuedDocName.c_str());
        }
    };

    auto* queuedObject = dynamic_cast<App::FeatureTest*>(
        queuedDoc->addObject("App::FeatureTest", "QueuedFeature")
    );
    ASSERT_NE(queuedObject, nullptr);

    blockingObject->touch();
    App::GetApplication().queueRecomputeRequest(
        App::RecomputeRequest::fromDocumentObject(*blockingObject)
    );

    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    std::promise<App::RecomputeFailure> callbackFailure;
    std::atomic<int> callbackCount {0};

    queuedObject->touch();
    auto request = App::RecomputeRequest::fromDocumentObject(*queuedObject);
    auto callbackFuture = callbackFailure.get_future();
    request.callback =
        [&callbackFailure,
         &callbackCount](const App::RecomputeRequest&, const App::RecomputeResult& result) {
            if (callbackCount.fetch_add(1) == 0) {
                callbackFailure.set_value(result.failure);
            }
        };

    App::GetApplication().queueRecomputeRequest(std::move(request));

    EXPECT_EQ(App::GetApplication().cancelQueuedRecomputeRequestsForDocument(queuedDocName), 1U);

    ASSERT_EQ(callbackFuture.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(callbackFuture.get(), App::RecomputeFailure::Canceled);
    EXPECT_EQ(callbackCount.load(), 1);

    App::FeatureTestAsyncBlocker::releaseBlocker();
}

TEST_F(AsyncRecomputeTest, WorkerSafetyIsCheckedFromRequest)
{
    auto* safeObject = dynamic_cast<App::FeatureTest*>(
        _doc->addObject("App::FeatureTest", "SafeFeature")
    );
    auto* unsafeObject = dynamic_cast<App::FeatureTestAttribute*>(
        _doc->addObject("App::FeatureTestAttribute", "UnsafeFeature")
    );

    ASSERT_NE(safeObject, nullptr);
    ASSERT_NE(unsafeObject, nullptr);

    EXPECT_TRUE(
        App::GetApplication().canRecomputeRequestOnWorker(
            App::RecomputeRequest::fromDocumentObject(*safeObject)
        )
    );
    EXPECT_FALSE(
        App::GetApplication().canRecomputeRequestOnWorker(
            App::RecomputeRequest::fromDocumentObject(*unsafeObject)
        )
    );
    EXPECT_FALSE(
        App::GetApplication().canRecomputeRequestOnWorker(App::RecomputeRequest::fromDocument(*_doc))
    );
}

TEST_F(AsyncRecomputeTest, QueuedDuplicateRequestsAreCoalesced)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    auto* safeObject = dynamic_cast<App::FeatureTest*>(
        _doc->addObject("App::FeatureTest", "SafeFeature")
    );
    ASSERT_NE(blocker, nullptr);
    ASSERT_NE(safeObject, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    blocker->touch();
    App::GetApplication().queueRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*blocker));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    std::mutex callbackMutex;
    std::condition_variable callbackChanged;
    int callbackCount = 0;
    const auto onQueuedCallback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        ++callbackCount;
        callbackChanged.notify_all();
    };

    safeObject->touch();
    App::RecomputeRequest firstRequest = App::RecomputeRequest::fromDocumentObject(*safeObject);
    firstRequest.callback = onQueuedCallback;
    App::GetApplication().queueRecomputeRequest(std::move(firstRequest));

    safeObject->touch();
    App::RecomputeRequest secondRequest = App::RecomputeRequest::fromDocumentObject(*safeObject);
    secondRequest.callback = onQueuedCallback;
    App::GetApplication().queueRecomputeRequest(std::move(secondRequest));

    App::FeatureTestAsyncBlocker::releaseBlocker();

    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    ASSERT_TRUE(callbackChanged.wait_for(callbackLock, 2s, [&] { return callbackCount == 2; }));
    EXPECT_EQ(safeObject->ExecCount.getValue(), 1);
}

TEST_F(AsyncRecomputeTest, InFlightDuplicateRequestsScheduleSingleRerun)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    std::mutex callbackMutex;
    std::condition_variable callbackChanged;
    int totalCallbacks = 0;
    int firstRunCallbacks = 0;
    int rerunCallbacks = 0;

    const auto onFirstRunCallback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        ++totalCallbacks;
        ++firstRunCallbacks;
        callbackChanged.notify_all();
    };
    const auto onRerunCallback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        ++totalCallbacks;
        ++rerunCallbacks;
        callbackChanged.notify_all();
    };

    blocker->touch();
    App::RecomputeRequest firstRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    firstRequest.callback = onFirstRunCallback;
    App::GetApplication().queueRecomputeRequest(std::move(firstRequest));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    blocker->touch();
    App::RecomputeRequest secondRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    secondRequest.callback = onRerunCallback;
    App::GetApplication().queueRecomputeRequest(std::move(secondRequest));

    blocker->touch();
    App::RecomputeRequest thirdRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    thirdRequest.callback = onRerunCallback;
    App::GetApplication().queueRecomputeRequest(std::move(thirdRequest));

    App::FeatureTestAsyncBlocker::releaseBlocker();

    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    ASSERT_TRUE(callbackChanged.wait_for(callbackLock, 2s, [&] { return totalCallbacks == 3; }));
    EXPECT_EQ(firstRunCallbacks, 1);
    EXPECT_EQ(rerunCallbacks, 2);
    EXPECT_TRUE(App::FeatureTestAsyncBlocker::waitUntilExecutionCount(2, 2s));
    EXPECT_EQ(App::FeatureTestAsyncBlocker::getExecutionCount(), 2);
}

TEST_F(AsyncRecomputeTest, CancelRecomputeRequestClearsMatchingRerun)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    std::mutex callbackMutex;
    std::condition_variable callbackChanged;
    int totalCallbacks = 0;
    int firstRunCallbacks = 0;
    int rerunCallbacks = 0;

    const auto onFirstRunCallback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        ++totalCallbacks;
        ++firstRunCallbacks;
        callbackChanged.notify_all();
    };
    const auto onRerunCallback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        ++totalCallbacks;
        ++rerunCallbacks;
        callbackChanged.notify_all();
    };

    blocker->touch();
    App::RecomputeRequest firstRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    firstRequest.callback = onFirstRunCallback;
    App::GetApplication().queueRecomputeRequest(std::move(firstRequest));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    blocker->touch();
    App::RecomputeRequest secondRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    secondRequest.callback = onRerunCallback;
    App::GetApplication().queueRecomputeRequest(std::move(secondRequest));

    App::RecomputeCancellationResult cancelResult = App::GetApplication().cancelRecomputeRequest(
        App::RecomputeRequest::fromDocumentObject(*blocker)
    );
    EXPECT_TRUE(cancelResult.canceledInProgress);

    App::FeatureTestAsyncBlocker::releaseBlocker();

    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    ASSERT_TRUE(callbackChanged.wait_for(callbackLock, 2s, [&] { return totalCallbacks == 1; }));
    EXPECT_EQ(firstRunCallbacks, 1);
    EXPECT_EQ(rerunCallbacks, 0);
    EXPECT_TRUE(App::FeatureTestAsyncBlocker::waitUntilExecutionCount(1, 2s));
    EXPECT_EQ(App::FeatureTestAsyncBlocker::getExecutionCount(), 1);
}

TEST_F(AsyncRecomputeTest, CancelRecomputeRequestReportsCanceledResult)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    std::mutex callbackMutex;
    std::condition_variable callbackChanged;
    bool callbackDone = false;
    bool callbackSuccess = true;
    App::RecomputeFailure callbackFailure = App::RecomputeFailure::None;
    std::string callbackMessage;
    std::mutex logMutex;
    std::vector<CapturedLog> logs;
    CapturingLogger logger(logs, logMutex);
    ScopedObserver scoped(logger);
    Base::Console().setConnectionMode(Base::ConsoleSingleton::Direct);

    blocker->touch();
    App::RecomputeRequest request = App::RecomputeRequest::fromDocumentObject(*blocker);
    request.callback = [&](App::RecomputeRequest&, App::RecomputeResult& result) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbackDone = true;
        callbackSuccess = result.success;
        callbackFailure = result.failure;
        callbackMessage = result.exception ? result.exception->what() : "";
        callbackChanged.notify_all();
    };
    App::GetApplication().queueRecomputeRequest(std::move(request));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    App::RecomputeCancellationResult cancelResult = App::GetApplication().cancelRecomputeRequest(
        App::RecomputeRequest::fromDocumentObject(*blocker)
    );
    EXPECT_TRUE(cancelResult.canceledInProgress);

    App::FeatureTestAsyncBlocker::releaseBlocker();

    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    ASSERT_TRUE(callbackChanged.wait_for(callbackLock, 2s, [&] { return callbackDone; }));
    EXPECT_FALSE(callbackSuccess);
    EXPECT_EQ(callbackFailure, App::RecomputeFailure::Canceled);
    EXPECT_EQ(callbackMessage, "User aborted");
    EXPECT_FALSE(blocker->isError());
    ASSERT_NE(_doc->getErrorDescription(blocker), nullptr);
    EXPECT_STREQ(_doc->getErrorDescription(blocker), "User aborted");

    {
        std::lock_guard<std::mutex> lock(logMutex);
        const bool reportedAsError = std::any_of(logs.begin(), logs.end(), [](const CapturedLog& log) {
            return log.level == Base::LogStyle::Error
                && log.msg.find("User aborted") != std::string::npos;
        });
        EXPECT_FALSE(reportedAsError);
    }
}

TEST_F(AsyncRecomputeTest, InFlightWorkerRecomputeDoesNotMonopolizeGil)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    blocker->touch();
    App::GetApplication().queueRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*blocker));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    auto gilProbe = std::async(std::launch::async, [] {
        Base::PyGILStateLocker lock;
        return true;
    });

    EXPECT_EQ(gilProbe.wait_for(100ms), std::future_status::ready);
    EXPECT_TRUE(gilProbe.get());

    App::FeatureTestAsyncBlocker::releaseBlocker();
}

namespace
{
bool simulatedWorkerThreadIsNeverMainThread()
{
    return false;
}

void invokeInlineOnCallingThread(std::function<void()>&& fn, bool)
{
    fn();
}
}  // namespace

TEST_F(AsyncRecomputeTest, MainThreadSignalWorkerEmissionDoesNotRequireGil)
{
    App::MainThreadSignal<void(int)> signal;
    int received = 0;
    signal.connect([&received](int value) { received = value; });

    App::MainThreadSignalConfig::setHooks(
        &simulatedWorkerThreadIsNeverMainThread,
        &invokeInlineOnCallingThread
    );
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::MainThreadSignalConfig::setHooks(nullptr, nullptr);
    };

    std::thread worker([&signal] { signal.emit(42); });
    worker.join();

    EXPECT_EQ(received, 42);
}

namespace
{
std::string spec012Thread(std::thread::id id)
{
    std::ostringstream text;
    text << id;
    return text.str();
}

void spec012Record(
    const char* family,
    const std::string& port,
    App::Document& document,
    App::DocumentObject* feature,
    int returned,
    bool hasError,
    const App::RecomputeResult* result = nullptr,
    int terminalCount = 0,
    std::thread::id callbackThread = {}
)
{
    // One isolated process owns this file. Emit on the observing thread after
    // completion; the future/sentinel provides synchronization with execution.
    auto* counted = dynamic_cast<App::FeatureTest*>(feature);
    const int executed = counted ? counted->ExecCount.getValue()
        : dynamic_cast<App::FeatureTestAsyncBlocker*>(feature)
        ? App::FeatureTestAsyncBlocker::getExecutionCount()
        : -1;
    std::ostringstream row;
    row << "{\"case\":" << std::quoted(family) << ",\"port\":" << std::quoted(port)
        << ",\"document\":" << std::quoted(document.getName())
        << ",\"target\":" << std::quoted(feature ? feature->getNameInDocument() : "missing")
        << ",\"native_id\":" << (feature ? feature->getID() : -1)
        << ",\"native_type\":" << std::quoted(feature ? feature->getTypeId().getName() : "missing")
        << ",\"return\":" << returned << ",\"has_error_out\":" << hasError
        << ",\"has_error_out_available\":"
        << (port == "direct_document_scoped" || port == "affected_dependency"
            || port == "direct_early_return" || port == "forced_control"
            || port == "clean_document_noop" || port == "nested_early_return"
            || port == "outer_success" || port.rfind("real_transaction_", 0) == 0)
        << ",\"execute_count\":" << executed << ",\"valid\":" << (feature && feature->isValid())
        << ",\"error\":" << (feature && feature->isError())
        << ",\"touched\":" << (feature && feature->isTouched())
        << ",\"document_touched\":" << document.isTouched()
        << ",\"transaction\":" << document.hasPendingTransaction()
        << ",\"transaction_replay\":" << document.isPerformingTransaction()
        << ",\"undo_count\":" << document.getAvailableUndos()
        << ",\"shape\":\"unavailable: App nongeometry feature\""
        << ",\"fixture_thread\":" << std::quoted(spec012Thread(spec012FixtureThread))
        << ",\"observer_thread\":" << std::quoted(spec012Thread(std::this_thread::get_id()))
        << ",\"callback_thread\":" << std::quoted(spec012Thread(callbackThread))
        << ",\"terminal_count\":" << terminalCount;
    if (auto* probe = dynamic_cast<App::Spec012OutcomeFeature*>(feature)) {
        row << ",\"execute_thread\":" << std::quoted(spec012Thread(probe->executionThread))
            << ",\"native_serial\":" << probe->lifetimeSerial
            << ",\"can_worker_now\":" << probe->canRecomputeOnWorker()
            << ",\"can_worker_at_execute\":" << probe->workerEligibilityAtExecute
            << ",\"execute_on_fixture_thread\":" << (probe->executionThread == spec012FixtureThread);
    }
    const char* error = feature ? document.getErrorDescription(feature) : nullptr;
    row << ",\"error_description\":" << std::quoted(error ? error : "");
    if (result) {
        row << ",\"success\":" << result->success
            << ",\"failure\":" << static_cast<int>(result->failure)
            << ",\"exception\":" << static_cast<bool>(result->exception) << ",\"exception_message\":"
            << std::quoted(result->exception ? result->exception->what() : "");
    }
    row << "}";
    std::cout << "SPEC012 " << row.str() << std::endl;
    if (const char* output = std::getenv("SPEC012_OUT")) {
        std::ofstream file(std::string(output) + "/app-outcomes.jsonl", std::ios::app);
        file << row.str() << '\n';
    }
}

void spec012Queue(
    const char* family,
    const std::string& port,
    App::RecomputeRequest request,
    App::Document& document,
    App::DocumentObject* feature
)
{
    std::promise<void> completion;
    auto done = completion.get_future();
    std::atomic<int> terminals {0};
    request.callback = [&](App::RecomputeRequest&, App::RecomputeResult& result) {
        const int count = ++terminals;
        const std::string category(family);
        const bool repaired = port.find("repaired") != std::string::npos;
        if ((category == "O02" || category == "O03") && !repaired) {
            EXPECT_FALSE(result.success);
            EXPECT_EQ(result.failure, App::RecomputeFailure::ModelError);
            EXPECT_FALSE(result.exception);
        }
        if (category == "O05") {
            const bool affectedFailure = port == "worker_recursive_affected_dependency"
                || port == "worker_document_with_existing_error";
            EXPECT_EQ(result.success, !affectedFailure);
            EXPECT_EQ(
                result.failure,
                affectedFailure ? App::RecomputeFailure::ModelError : App::RecomputeFailure::None
            );
        }
        spec012Record(
            family,
            port,
            document,
            feature,
            -999,
            false,
            &result,
            count,
            std::this_thread::get_id()
        );
        if (count == 1) {
            completion.set_value();
        }
    };
    App::GetApplication().queueRecomputeRequest(std::move(request));
    ASSERT_EQ(done.wait_for(10s), std::future_status::ready);
    EXPECT_EQ(terminals.load(), 1);
    // An ordinary later request observes queue progress beyond this callback;
    // callback delivery itself precedes worker bookkeeping cleanup.
    auto* sentinel = document.addObject<App::FeatureTest>("Spec012Sentinel");
    std::promise<void> drained;
    auto drain = drained.get_future();
    auto next = App::RecomputeRequest::fromDocumentObject(*sentinel);
    next.callback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        drained.set_value();
    };
    App::GetApplication().queueRecomputeRequest(std::move(next));
    ASSERT_EQ(drain.wait_for(10s), std::future_status::ready);
    EXPECT_EQ(terminals.load(), 1);
    document.removeObject(sentinel->getNameInDocument());
}
}  // namespace

TEST_F(AsyncRecomputeTest, Spec012AppOutcomePorts)
{
    for (int mode : {0, 1, 2, 3}) {
        auto* feature = _doc->addObject<App::Spec012OutcomeFeature>("Outcome");
        feature->mode = mode == 1 ? 1 : 0;
        feature->ExceptionType.setValue(mode >= 2 ? mode - 1 : 0);
        const char* family = mode == 0 ? "O01" : mode == 1 ? "O02" : "O03";
        for (bool recursive : {false, true}) {
            feature->touch();
            const bool returned = feature->recomputeFeature(recursive);
            spec012Record(
                family,
                recursive ? "direct_recursive" : "direct_nonrecursive",
                *_doc,
                feature,
                returned,
                false
            );
            EXPECT_EQ(returned, mode == 0);
        }
        feature->touch();
        bool hasError = false;
        const int attempts = _doc->recompute({feature}, true, &hasError);
        spec012Record(family, "direct_document_scoped", *_doc, feature, attempts, hasError);
        EXPECT_EQ(hasError, mode != 0);
        EXPECT_GT(attempts, 0);
        for (bool wholeDocument : {false, true}) {
            feature->touch();
            auto request = wholeDocument ? App::RecomputeRequest::fromDocument(*_doc, true)
                                         : App::RecomputeRequest::fromDocumentObject(*feature, true);
            spec012Queue(
                family,
                wholeDocument ? "worker_document" : "worker_object",
                std::move(request),
                *_doc,
                feature
            );
            EXPECT_EQ(feature->isValid(), mode == 0);
        }
        if (mode != 0) {
            feature->mode = 0;
            feature->ExceptionType.setValue(0);
            feature->touch();
            EXPECT_TRUE(feature->recomputeFeature(true));
            spec012Record(family, "repaired_retry", *_doc, feature, 1, false);
        }
        _doc->removeObject(feature->getNameInDocument());
    }
    auto* valid = _doc->addObject<App::Spec012OutcomeFeature>("NoOp");
    bool error = false;
    _doc->recompute({}, true, &error);
    const int initial = valid->ExecCount.getValue();
    const int count = _doc->recompute({}, false, &error);
    spec012Record("O01", "clean_document_noop", *_doc, valid, count, error);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(valid->ExecCount.getValue(), initial);
}

TEST_F(AsyncRecomputeTest, Spec012ScopedErrorsAndEarlyReturns)
{
    auto* bad = _doc->addObject<App::Spec012OutcomeFeature>("Bad");
    auto* dependent = _doc->addObject<App::Spec012OutcomeFeature>("Dependent");
    auto* good = _doc->addObject<App::Spec012OutcomeFeature>("Unrelated");
    dependent->Source1.setValue(bad);
    bad->mode = 1;
    bad->touch();
    bool error = false;
    const int count = _doc->recompute({dependent}, true, &error);
    spec012Record("O05", "affected_dependency", *_doc, dependent, count, error);
    EXPECT_TRUE(error);
    EXPECT_TRUE(bad->isError());
    EXPECT_EQ(dependent->ExecCount.getValue(), 0);
    good->touch();
    const bool scoped = good->recomputeFeature(true);
    spec012Record("O05", "unrelated_preexisting_error", *_doc, good, scoped, false);
    EXPECT_TRUE(scoped);
    EXPECT_TRUE(bad->isError());
    spec012Queue(
        "O05",
        "worker_scoped_good_with_unrelated_error",
        App::RecomputeRequest::fromDocumentObject(*good, true),
        *_doc,
        good
    );
    bad->touch();
    dependent->touch();
    spec012Queue(
        "O05",
        "worker_recursive_affected_dependency",
        App::RecomputeRequest::fromDocumentObject(*dependent, true),
        *_doc,
        dependent
    );
    spec012Record("O05", "affected_source_after_recursive_request", *_doc, bad, -999, false);
    spec012Queue(
        "O05",
        "worker_document_with_existing_error",
        App::RecomputeRequest::fromDocument(*_doc, true),
        *_doc,
        good
    );

    bad->mode = 0;
    bad->touch();
    EXPECT_TRUE(bad->recomputeFeature(true));
    _doc->recompute({}, true);
    int skipped = 0;
    fastsignals::scoped_connection skipping(_doc->signalSkipRecompute.connect(
        [&](const App::Document&, const std::vector<App::DocumentObject*>&) { ++skipped; }
    ));
    for (auto flag : {App::Document::SkipRecompute, App::Document::PartialDoc}) {
        good->touch();
        const int before = good->ExecCount.getValue();
        _doc->setStatus(flag, true);
        error = false;
        const int returned = _doc->recompute({}, false, &error);
        spec012Record(
            flag == App::Document::PartialDoc ? "O07" : "O06",
            "direct_early_return",
            *_doc,
            good,
            returned,
            error
        );
        EXPECT_EQ(returned, 0);
        EXPECT_EQ(good->ExecCount.getValue(), before);
        if (flag == App::Document::SkipRecompute) {
            EXPECT_EQ(skipped, 1);
            error = false;
            const int forced = _doc->recompute({}, true, &error);
            EXPECT_GT(forced, 0);
            spec012Record("O06", "forced_control", *_doc, good, forced, error);
        }
        spec012Queue(
            flag == App::Document::PartialDoc ? "O07" : "O06",
            "worker_early_return",
            App::RecomputeRequest::fromDocument(*_doc),
            *_doc,
            good
        );
        _doc->setStatus(flag, false);
    }
    skipping.disconnect();
    EXPECT_EQ(skipped, 2);  // Direct and queued skip; PartialDoc emits neither.
    std::cout << "SPEC012 {\"case\":\"O06\",\"skip_signal_count\":" << skipped << "}" << std::endl;
}

TEST_F(AsyncRecomputeTest, Spec012NotificationExceptionAndReentry)
{
    auto* feature = _doc->addObject<App::Spec012OutcomeFeature>("ObserverOutcome");
    bool throwOnce = true;
    fastsignals::scoped_connection throwing(
        _doc->signalBeforeRecompute.connect([&](const App::Document&) {
            if (throwOnce) {
                throwOnce = false;
                throw Base::RuntimeError("Spec012 request-level notification exception");
            }
        })
    );
    spec012Queue(
        "O04",
        "worker_before_recompute_exception",
        App::RecomputeRequest::fromDocument(*_doc, true),
        *_doc,
        feature
    );
    EXPECT_FALSE(throwOnce);
    throwing.disconnect();
    EXPECT_TRUE(feature->recomputeFeature(true));
    spec012Record("O04", "retry_after_observer_exception", *_doc, feature, 1, false);
    int nested = -1;
    bool nestedError = false;
    int observerCount = 0;
    fastsignals::scoped_connection reentrant(
        _doc->signalBeforeRecompute.connect([&](const App::Document&) {
            ++observerCount;
            nested = _doc->recompute({}, true, &nestedError);
        })
    );
    feature->touch();
    bool error = false;
    const int outer = _doc->recompute({}, true, &error);
    reentrant.disconnect();
    EXPECT_EQ(observerCount, 1);
    EXPECT_EQ(nested, 0);
    EXPECT_GT(outer, 0);
    spec012Record("O09", "nested_early_return", *_doc, feature, nested, nestedError);
    spec012Record("O09", "outer_success", *_doc, feature, outer, error);
}

TEST_F(AsyncRecomputeTest, Spec012RealUndoAndRollbackRecompute)
{
    auto* feature = _doc->addObject<App::Spec012OutcomeFeature>("TransactionOutcome");
    _doc->recompute({}, true);
    _doc->setMaxUndoStackSize(10);
    _doc->openTransaction("Spec012 committed edit");
    feature->Integer.setValue(17);
    _doc->commitTransaction();
    int undoEvents = 0;
    int redoEvents = 0;
    int rollbackEvents = 0;
    std::string replayPhase = "undo";
    fastsignals::scoped_connection replay(_doc->signalChangedObject.connect(
        [&](const App::DocumentObject& object, const App::Property&) {
            if (&object != feature || !_doc->isPerformingTransaction()) {
                return;
            }
            if (replayPhase == "undo") {
                ++undoEvents;
            }
            else if (replayPhase == "redo") {
                ++redoEvents;
            }
            else if (replayPhase == "rollback") {
                ++rollbackEvents;
            }
            bool error = false;
            const int result = _doc->recompute({}, true, &error);
            spec012Record("O08", "real_transaction_" + replayPhase, *_doc, feature, result, error);
            EXPECT_EQ(result, 0);
        }
    ));
    EXPECT_TRUE(_doc->undo());
    replayPhase = "redo";
    EXPECT_TRUE(_doc->redo());
    _doc->openTransaction("Spec012 aborted edit");
    feature->Integer.setValue(23);
    replayPhase = "rollback";
    _doc->abortTransaction();
    replay.disconnect();
    EXPECT_GT(undoEvents, 0);
    EXPECT_GT(redoEvents, 0);
    EXPECT_GT(rollbackEvents, 0);
    EXPECT_EQ(feature->Integer.getValue(), 17);
    EXPECT_FALSE(_doc->hasPendingTransaction());
    EXPECT_TRUE(feature->recomputeFeature(true));
    spec012Record("O08", "post_replay_retry", *_doc, feature, 1, false);
}

TEST_F(AsyncRecomputeTest, Spec012QueuedMissingAndCanceledOutcomes)
{
    for (int variant : {0, 1, 2}) {
        const bool removeTarget = variant == 1;
        const bool cancelSingleRequest = variant == 2;
        const auto otherName = App::GetApplication().getUniqueDocumentName("spec012_queued");
        auto* other = App::GetApplication().newDocument(otherName.c_str());
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::GetApplication().closeDocument(otherName.c_str());
        };
        auto* target = other->addObject<App::Spec012OutcomeFeature>("QueuedTarget");
        auto* blocker = _doc->addObject<App::FeatureTestAsyncBlocker>("OutcomeBlocker");
        App::FeatureTestAsyncBlocker::resetBlocker();
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::FeatureTestAsyncBlocker::releaseBlocker();
        };
        std::promise<void> firstComplete;
        auto first = firstComplete.get_future();
        auto blocking = App::RecomputeRequest::fromDocumentObject(*blocker);
        blocking.callback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
            firstComplete.set_value();
        };
        App::GetApplication().queueRecomputeRequest(std::move(blocking));
        ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
        std::atomic<int> terminals {0};
        std::promise<void> completion;
        auto done = completion.get_future();
        auto request = App::RecomputeRequest::fromDocumentObject(*target);
        request.callback = [&](App::RecomputeRequest&, App::RecomputeResult& result) {
            const int count = ++terminals;
            spec012Record(
                removeTarget ? "O12" : "O10",
                removeTarget              ? "queued_missing"
                    : cancelSingleRequest ? "queued_request_cancel"
                                          : "queued_document_cancel",
                *other,
                removeTarget ? nullptr : target,
                -999,
                false,
                &result,
                count,
                std::this_thread::get_id()
            );
            EXPECT_FALSE(result.success);
            if (removeTarget) {
                EXPECT_EQ(result.failure, App::RecomputeFailure::TargetUnavailable);
                EXPECT_FALSE(result.exception);
            }
            else {
                EXPECT_EQ(result.failure, App::RecomputeFailure::Canceled);
            }
            if (count == 1) {
                completion.set_value();
            }
        };
        App::GetApplication().queueRecomputeRequest(std::move(request));
        EXPECT_EQ(target->ExecCount.getValue(), 0);
        if (removeTarget) {
            other->removeObject(target->getNameInDocument());
            target = nullptr;
        }
        else if (cancelSingleRequest) {
            const auto canceled = App::GetApplication().cancelRecomputeRequest(
                App::RecomputeRequest::fromDocumentObject(*target)
            );
            EXPECT_TRUE(canceled.removedQueued);
            EXPECT_EQ(target->ExecCount.getValue(), 0);
        }
        else {
            EXPECT_EQ(App::GetApplication().cancelQueuedRecomputeRequestsForDocument(otherName), 1);
            EXPECT_EQ(target->ExecCount.getValue(), 0);
        }
        App::FeatureTestAsyncBlocker::releaseBlocker();
        ASSERT_EQ(first.wait_for(10s), std::future_status::ready);
        if (!cancelSingleRequest) {
            ASSERT_EQ(done.wait_for(10s), std::future_status::ready);
            EXPECT_EQ(terminals.load(), 1);
        }
        auto* later = other->addObject<App::Spec012OutcomeFeature>("LaterSentinel");
        spec012Queue(
            removeTarget ? "O12" : "O10",
            "later_sentinel",
            App::RecomputeRequest::fromDocumentObject(*later),
            *other,
            later
        );
        EXPECT_EQ(later->ExecCount.getValue(), 1);
        if (cancelSingleRequest) {
            // Observe this API's subscriber disposition after a real successor
            // request, without prescribing T005's callback support policy.
            spec012Record(
                "O10",
                "request_cancel_after_sentinel",
                *other,
                target,
                -999,
                false,
                nullptr,
                terminals.load()
            );
            EXPECT_EQ(target->ExecCount.getValue(), 0);
        }
        _doc->removeObject(blocker->getNameInDocument());
    }
}

TEST_F(AsyncRecomputeTest, Spec012CooperativeCancellationAndRetry)
{
    auto* blocker = _doc->addObject<App::FeatureTestAsyncBlocker>("CooperativeOutcome");
    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };
    std::atomic<int> terminals {0};
    std::promise<void> completion;
    auto done = completion.get_future();
    auto request = App::RecomputeRequest::fromDocumentObject(*blocker);
    request.callback = [&](App::RecomputeRequest&, App::RecomputeResult& result) {
        const int count = ++terminals;
        spec012Record(
            "O11",
            "cooperative_execute_cancel",
            *_doc,
            blocker,
            -999,
            false,
            &result,
            count,
            std::this_thread::get_id()
        );
        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.failure, App::RecomputeFailure::Canceled);
        if (count == 1) {
            completion.set_value();
        }
    };
    App::GetApplication().queueRecomputeRequest(std::move(request));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
    EXPECT_TRUE(
        App::GetApplication()
            .cancelRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*blocker))
            .canceledInProgress
    );
    App::FeatureTestAsyncBlocker::releaseBlocker();
    ASSERT_EQ(done.wait_for(10s), std::future_status::ready);
    EXPECT_EQ(terminals.load(), 1);
    EXPECT_FALSE(blocker->isError());
    EXPECT_STREQ(_doc->getErrorDescription(blocker), "User aborted");
    blocker->touch();
    spec012Queue(
        "O11",
        "retry_after_canceled_execution",
        App::RecomputeRequest::fromDocumentObject(*blocker),
        *_doc,
        blocker
    );
    EXPECT_FALSE(blocker->isError());
}

namespace
{
struct Spec012Subscriber
{
    std::atomic<int> count {0};
    std::promise<void> completed;
    std::future<void> done {completed.get_future()};
};

App::RecomputeRequest::Callback spec012Subscriber(
    const char* family,
    std::string token,
    std::string targetName,
    const std::shared_ptr<Spec012Subscriber>& state
)
{
    return [family,
            token = std::move(token),
            targetName = std::move(targetName),
            state](App::RecomputeRequest& request, App::RecomputeResult& result) {
        const int count = ++state->count;
        if (auto* document = request.resolveDocument()) {
            spec012Record(
                family,
                token,
                *document,
                document->getObject(targetName.c_str()),
                -999,
                false,
                &result,
                count,
                std::this_thread::get_id()
            );
        }
        else {
            std::cout << "SPEC012 {\"case\":" << std::quoted(family)
                      << ",\"port\":" << std::quoted(token)
                      << ",\"document_missing\":true,\"terminal_count\":" << count << "}"
                      << std::endl;
        }
        if (count == 1) {
            state->completed.set_value();
        }
    };
}

class Spec012QueueBlocker
{
public:
    explicit Spec012QueueBlocker(App::Document& document)
        : document(document)
        , blocker(document.addObject<App::FeatureTestAsyncBlocker>("QueueGate"))
        , terminal(std::make_shared<Spec012Subscriber>())
    {
        App::FeatureTestAsyncBlocker::resetBlocker();
        blocker->touch();
        auto request = App::RecomputeRequest::fromDocumentObject(*blocker);
        request.callback = [state = terminal](App::RecomputeRequest&, App::RecomputeResult&) {
            if (++state->count == 1) {
                state->completed.set_value();
            }
        };
        App::GetApplication().queueRecomputeRequest(std::move(request));
        if (!App::FeatureTestAsyncBlocker::waitUntilStarted(10s)) {
            App::FeatureTestAsyncBlocker::releaseBlocker();
            throw std::runtime_error("Spec012 D1 native gate never started");
        }
    }
    ~Spec012QueueBlocker()
    {
        release();
        EXPECT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
        // D1 callback publication precedes worker cleanup. Retire its native
        // object only after a successor has actually traversed that queue.
        auto* successor = document.addObject<App::FeatureTest>("GateRetirementSentinel");
        successor->touch();
        auto retired = std::make_shared<Spec012Subscriber>();
        auto next = App::RecomputeRequest::fromDocumentObject(*successor);
        next.callback = [retired](App::RecomputeRequest&, App::RecomputeResult&) {
            if (++retired->count == 1) {
                retired->completed.set_value();
            }
        };
        App::GetApplication().queueRecomputeRequest(std::move(next));
        EXPECT_EQ(retired->done.wait_for(10s), std::future_status::ready);
        document.removeObject(successor->getNameInDocument());
        document.removeObject(blocker->getNameInDocument());
    }
    void release()
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    }

private:
    App::Document& document;
    App::FeatureTestAsyncBlocker* blocker;
    std::shared_ptr<Spec012Subscriber> terminal;
};
}  // namespace

TEST_F(AsyncRecomputeTest, Spec012QueuedGenerationAndFreshAdmission)
{
    for (const std::string variant :
         {"stable",
          "manual",
          "missing",
          "same_type",
          "unsafe_type",
          "document_closure",
          "recursive_closure"}) {
        const auto name = App::GetApplication().getUniqueDocumentName("spec012_identity");
        auto* document = App::GetApplication().newDocument(name.c_str());
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::GetApplication().closeDocument(name.c_str());
        };
        auto* original = document->addObject<App::Spec012OutcomeFeature>("Target");
        original->touch();
        const long oldId = original->getID();
        const auto oldSerial = original->lifetimeSerial;
        const char* family = variant == "missing"          ? "Q02"
            : variant == "same_type"                       ? "Q03"
            : variant == "unsafe_type"                     ? "Q04"
            : variant.find("closure") != std::string::npos ? "Q06"
                                                           : "Q01";
        Spec012QueueBlocker gate(*_doc);
        auto state = std::make_shared<Spec012Subscriber>();
        App::RecomputeRequest request;
        if (variant == "manual") {
            request.documentName = name;
            request.documentObjectName = "Target";
        }
        else if (variant == "document_closure") {
            request = App::RecomputeRequest::fromDocument(*document, true);
        }
        else {
            request = App::RecomputeRequest::fromDocumentObject(
                *original,
                variant == "recursive_closure"
            );
        }
        EXPECT_TRUE(App::GetApplication().canRecomputeRequestOnWorker(request));
        spec012Record(family, variant + "_enqueue", *document, original, -999, false);
        request.callback = spec012Subscriber(family, variant + "_subscriber_A", "Target", state);
        App::GetApplication().queueRecomputeRequest(std::move(request));
        EXPECT_EQ(original->ExecCount.getValue(), 0);
        if (variant == "missing" || variant == "same_type" || variant == "unsafe_type") {
            document->removeObject("Target");
            original = nullptr;
            if (variant != "missing") {
                auto* replacement = variant == "unsafe_type"
                    ? static_cast<App::Spec012OutcomeFeature*>(
                          document->addObject<App::Spec012UnsafeOutcomeFeature>("Target")
                      )
                    : document->addObject<App::Spec012OutcomeFeature>("Target");
                replacement->touch();
                EXPECT_NE(replacement->getID(), oldId);
                EXPECT_NE(replacement->lifetimeSerial, oldSerial);
                spec012Record(
                    family,
                    variant + "_replacement_before_release",
                    *document,
                    replacement,
                    -999,
                    false
                );
            }
        }
        else if (variant.find("closure") != std::string::npos) {
            auto* unsafe = document->addObject<App::Spec012UnsafeOutcomeFeature>("UnsafeDependency");
            unsafe->touch();
            if (variant == "recursive_closure") {
                original->Source1.setValue(unsafe);
            }
            EXPECT_FALSE(unsafe->canRecomputeOnWorker());
            spec012Record(family, variant + "_unsafe_before_release", *document, unsafe, -999, false);
        }
        gate.release();
        ASSERT_EQ(state->done.wait_for(10s), std::future_status::ready);
        EXPECT_EQ(state->count.load(), 1);
        if (auto* target = dynamic_cast<App::Spec012OutcomeFeature*>(document->getObject("Target"))) {
            spec012Record(family, variant + "_after_completion", *document, target, -999, false);
        }
        if (auto* unsafe = dynamic_cast<App::Spec012UnsafeOutcomeFeature*>(
                document->getObject("UnsafeDependency")
            )) {
            spec012Record(family, variant + "_unsafe_after_completion", *document, unsafe, -999, false);
        }
        auto* sentinel = document->addObject<App::Spec012OutcomeFeature>("Successor");
        spec012Queue(
            family,
            variant + "_later_sentinel",
            App::RecomputeRequest::fromDocumentObject(*sentinel),
            *document,
            sentinel
        );
        spec012Record(
            family,
            variant + "_subscriber_A_after_drain",
            *document,
            document->getObject("Target"),
            -999,
            false,
            nullptr,
            state->count.load()
        );
    }
}

TEST_F(AsyncRecomputeTest, Spec012ClosedDocumentGeneration)
{
    const auto name = App::GetApplication().getUniqueDocumentName("spec012_document_generation");
    auto* document = App::GetApplication().newDocument(name.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (App::GetApplication().getDocument(name.c_str())) {
            App::GetApplication().closeDocument(name.c_str());
        }
    };
    auto* original = document->addObject<App::Spec012OutcomeFeature>("Target");
    const auto oldSerial = original->lifetimeSerial;
    Spec012QueueBlocker gate(*_doc);
    auto old = std::make_shared<Spec012Subscriber>();
    auto request = App::RecomputeRequest::fromDocumentObject(*original);
    request.callback = spec012Subscriber("Q05", "old_document_subscriber", "Target", old);
    spec012Record("Q05", "old_document_enqueue", *document, original, -999, false);
    App::GetApplication().queueRecomputeRequest(std::move(request));
    EXPECT_TRUE(App::GetApplication().closeDocument(name.c_str()));
    EXPECT_EQ(old->count.load(), 1);
    document = App::GetApplication().newDocument(name.c_str());
    EXPECT_EQ(std::string(document->getName()), name);
    auto* replacement = document->addObject<App::Spec012OutcomeFeature>("Target");
    EXPECT_NE(replacement->lifetimeSerial, oldSerial);
    spec012Record("Q05", "new_document_before_release", *document, replacement, -999, false);
    gate.release();
    spec012Queue(
        "Q05",
        "legitimate_new_document_request",
        App::RecomputeRequest::fromDocumentObject(*replacement),
        *document,
        replacement
    );
    EXPECT_EQ(old->count.load(), 1);
}

TEST_F(AsyncRecomputeTest, Spec012QueuedCoalescingGenerationAndMutation)
{
    for (const std::string variant :
         {"stable", "replacement", "mutation", "cancel_document", "cancel_request", "close_document"}) {
        const auto name = App::GetApplication().getUniqueDocumentName("spec012_subscribers");
        auto* document = App::GetApplication().newDocument(name.c_str());
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::GetApplication().closeDocument(name.c_str());
        };
        auto* target = document->addObject<App::Spec012OutcomeFeature>("Target");
        target->touch();
        Spec012QueueBlocker gate(*_doc);
        auto first = std::make_shared<Spec012Subscriber>();
        auto second = std::make_shared<Spec012Subscriber>();
        const char* family = variant == "mutation"                         ? "Q09"
            : (variant.find("cancel") == 0 || variant == "close_document") ? "Q08"
                                                                           : "Q07";
        spec012Record(family, variant + "_old_registration", *document, target, -999, false);
        auto request = App::RecomputeRequest::fromDocumentObject(*target);
        auto firstCallback = spec012Subscriber(family, variant + "_subscriber_A", "Target", first);
        request.callback = [firstCallback,
                            variant](App::RecomputeRequest& req, App::RecomputeResult& result) {
            firstCallback(req, result);
            if (variant == "mutation") {
                req.documentObjectName = "CallbackMutatedName";
                result.success = false;
                result.failure = App::RecomputeFailure::Canceled;
            }
        };
        App::GetApplication().queueRecomputeRequest(std::move(request));
        if (variant == "replacement") {
            document->removeObject("Target");
            target = document->addObject<App::Spec012OutcomeFeature>("Target");
            target->touch();
            spec012Record(family, "replacement_new_registration", *document, target, -999, false);
        }
        auto duplicate = App::RecomputeRequest::fromDocumentObject(*target);
        duplicate.callback =
            [callback = spec012Subscriber(family, variant + "_subscriber_B", "Target", second),
             variant](App::RecomputeRequest& req, App::RecomputeResult& result) {
                if (variant == "mutation") {
                    std::cout << "SPEC012 {\"case\":\"Q09\",\"seen_object_name\":"
                              << std::quoted(req.documentObjectName)
                              << ",\"seen_success\":" << result.success
                              << ",\"seen_failure\":" << static_cast<int>(result.failure) << "}"
                              << std::endl;
                }
                callback(req, result);
            };
        App::GetApplication().queueRecomputeRequest(std::move(duplicate));
        if (variant == "cancel_document") {
            EXPECT_EQ(App::GetApplication().cancelQueuedRecomputeRequestsForDocument(name), 1);
        }
        else if (variant == "cancel_request") {
            EXPECT_TRUE(
                App::GetApplication()
                    .cancelRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*target))
                    .removedQueued
            );
        }
        if (variant == "close_document") {
            EXPECT_TRUE(App::GetApplication().closeDocument(name.c_str()));
            document = App::GetApplication().newDocument(name.c_str());
            target = document->addObject<App::Spec012OutcomeFeature>("Target");
        }
        gate.release();
        if (variant != "cancel_request") {
            ASSERT_EQ(first->done.wait_for(10s), std::future_status::ready);
            ASSERT_EQ(second->done.wait_for(10s), std::future_status::ready);
        }
        auto* sentinel = document->addObject<App::Spec012OutcomeFeature>("Successor");
        spec012Queue(
            family,
            variant + "_sentinel",
            App::RecomputeRequest::fromDocumentObject(*sentinel),
            *document,
            sentinel
        );
        spec012Record(
            family,
            variant + "_after_drain_A",
            *document,
            target,
            -999,
            false,
            nullptr,
            first->count.load()
        );
        spec012Record(
            family,
            variant + "_after_drain_B",
            *document,
            target,
            -999,
            false,
            nullptr,
            second->count.load()
        );
    }
}

TEST_F(AsyncRecomputeTest, Spec012InFlightSubscriberReruns)
{
    for (bool cancel : {false, true}) {
        auto* target = _doc->addObject<App::FeatureTestAsyncBlocker>("RerunTarget");
        target->touch();
        App::FeatureTestAsyncBlocker::resetBlocker();
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::FeatureTestAsyncBlocker::releaseBlocker();
        };
        auto first = std::make_shared<Spec012Subscriber>();
        auto second = std::make_shared<Spec012Subscriber>();
        auto third = std::make_shared<Spec012Subscriber>();
        const char* family = cancel ? "Q08" : "Q07";
        auto request = App::RecomputeRequest::fromDocumentObject(*target);
        request.callback = spec012Subscriber(family, "inflight_A", target->getNameInDocument(), first);
        App::GetApplication().queueRecomputeRequest(std::move(request));
        ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
        for (const auto& item : {std::pair {"rerun_B", second}, std::pair {"rerun_C", third}}) {
            auto rerun = App::RecomputeRequest::fromDocumentObject(*target);
            rerun.callback
                = spec012Subscriber(family, item.first, target->getNameInDocument(), item.second);
            App::GetApplication().queueRecomputeRequest(std::move(rerun));
        }
        if (cancel) {
            EXPECT_TRUE(
                App::GetApplication()
                    .cancelRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*target))
                    .canceledInProgress
            );
        }
        App::FeatureTestAsyncBlocker::releaseBlocker();
        ASSERT_EQ(first->done.wait_for(10s), std::future_status::ready);
        if (!cancel) {
            ASSERT_EQ(second->done.wait_for(10s), std::future_status::ready);
            ASSERT_EQ(third->done.wait_for(10s), std::future_status::ready);
        }
        auto* sentinel = _doc->addObject<App::Spec012OutcomeFeature>("RerunSuccessor");
        spec012Queue(
            family,
            "rerun_later_sentinel",
            App::RecomputeRequest::fromDocumentObject(*sentinel),
            *_doc,
            sentinel
        );
        spec012Record(
            family,
            "rerun_B_after_drain",
            *_doc,
            target,
            -999,
            false,
            nullptr,
            second->count.load()
        );
        spec012Record(
            family,
            "rerun_C_after_drain",
            *_doc,
            target,
            -999,
            false,
            nullptr,
            third->count.load()
        );
        _doc->removeObject(sentinel->getNameInDocument());
        _doc->removeObject(target->getNameInDocument());
    }
}

TEST_F(AsyncRecomputeTest, Spec012IsolatedCallbackLifecycle)
{
    const char* configured = std::getenv("SPEC012_CALLBACK_CASE");
    const std::string variant = configured ? configured : "reentry";
    if (variant == "shutdown") {
        auto* target = _doc->addObject<App::Spec012OutcomeFeature>("ShutdownTarget");
        target->touch();
        auto entered = std::make_shared<std::promise<void>>();
        auto started = entered->get_future();
        auto release = std::make_shared<std::promise<void>>();
        auto proceed = release->get_future().share();
        auto terminal = std::make_shared<Spec012Subscriber>();
        auto request = App::RecomputeRequest::fromDocumentObject(*target);
        auto progress = std::make_shared<App::RecomputeProgressHandle>();
        request.progress = progress;
        request.callback =
            [entered, proceed, terminal](App::RecomputeRequest& req, App::RecomputeResult& result) {
                entered->set_value();
                proceed.wait();
                spec012Subscriber("Q11", "close_all_active_callback", "ShutdownTarget", terminal)(
                    req,
                    result
                );
            };
        App::GetApplication().queueRecomputeRequest(std::move(request));
        ASSERT_EQ(started.wait_for(10s), std::future_status::ready);
        auto closing = std::async(std::launch::async, [] {
            App::GetApplication().closeAllDocuments();
        });
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (!progress->wasCanceled() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        const bool cancellationEntered = progress->wasCanceled();
        release->set_value();
        EXPECT_TRUE(cancellationEntered);
        ASSERT_EQ(closing.wait_for(10s), std::future_status::ready);
        closing.get();
        EXPECT_EQ(terminal->count.load(), 1);
        _doc = nullptr;
        _docName.clear();
        const auto name = App::GetApplication().getUniqueDocumentName("after_close_all");
        auto* later = App::GetApplication().newDocument(name.c_str());
        auto* sentinel = later->addObject<App::Spec012OutcomeFeature>("Successor");
        spec012Queue(
            "Q11",
            "close_all_later_progress",
            App::RecomputeRequest::fromDocumentObject(*sentinel),
            *later,
            sentinel
        );
        App::GetApplication().closeDocument(name.c_str());
        return;
    }
    const auto name = App::GetApplication().getUniqueDocumentName("spec012_callback");
    auto* document = App::GetApplication().newDocument(name.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (App::GetApplication().getDocument(name.c_str())) {
            App::GetApplication().closeDocument(name.c_str());
        }
    };
    auto* target = document->addObject<App::Spec012OutcomeFeature>("Target");
    target->touch();
    auto* successor = document->addObject<App::Spec012OutcomeFeature>("Successor");
    Spec012QueueBlocker gate(*_doc);
    auto first = std::make_shared<Spec012Subscriber>();
    auto second = std::make_shared<Spec012Subscriber>();
    auto nested = std::make_shared<Spec012Subscriber>();
    auto after = std::make_shared<Spec012Subscriber>();
    const char* family = variant == "throw" ? "Q10" : "Q11";
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    auto firstCallback = spec012Subscriber(family, variant + "_subscriber_A", "Target", first);
    request.callback = [variant,
                        firstCallback,
                        nested](App::RecomputeRequest& req, App::RecomputeResult& result) {
        firstCallback(req, result);
        std::cout << "SPEC012 {\"case\":\"Q11\",\"gil_held\":" << PyGILState_Check() << "}"
                  << std::endl;
        if (variant == "throw") {
            throw std::runtime_error("Spec012 isolated terminal callback exception");
        }
        if (variant == "destruct") {
            const char* output = std::getenv("SPEC012_OUT");
            if (!output) {
                throw std::runtime_error("Spec012 destruct requires private output profile");
            }
            std::vector<std::pair<std::string, std::string>> snapshots;
            std::vector<bool> present;
            for (const std::string key : {"UserParameter", "SystemParameter"}) {
                const std::string path = App::Application::Config().at(key);
                if (path.rfind(std::string(output) + "/", 0) != 0) {
                    throw std::runtime_error(
                        "Spec012 refuses destruct outside private parameter profile"
                    );
                }
                std::ifstream input(path, std::ios::binary);
                const bool exists = input.good();
                const std::string bytes(
                    (std::istreambuf_iterator<char>(input)),
                    std::istreambuf_iterator<char>()
                );
                snapshots.emplace_back(path, bytes);
                present.push_back(exists);
                std::ofstream before(
                    std::string(output) + "/parameter-before-" + key + ".bin",
                    std::ios::binary
                );
                before << bytes;
                std::cout
                    << "SPEC012 {\"case\":\"Q11\",\"port\":\"destruct_parameter_before\",\"key\":"
                    << std::quoted(key) << ",\"path\":" << std::quoted(path)
                    << ",\"present\":" << exists << ",\"bytes\":" << bytes.size() << "}"
                    << std::endl;
            }
            std::cout << "SPEC012 {\"case\":\"Q11\",\"port\":\"actual_Application_destruct_enter\"}"
                      << std::endl;
            App::Application::destruct();
            // A supported refusal returns before touching singleton/parameters.
            bool unchanged = true;
            for (std::size_t i = 0; i < snapshots.size(); ++i) {
                std::ifstream input(snapshots[i].first, std::ios::binary);
                const bool exists = input.good();
                const std::string bytes(
                    (std::istreambuf_iterator<char>(input)),
                    std::istreambuf_iterator<char>()
                );
                unchanged = unchanged && exists == present[i] && bytes == snapshots[i].second;
            }
            std::cout
                << "SPEC012 "
                   "{\"case\":\"Q11\",\"port\":\"destruct_returned\",\"parameter_state_preserved\":"
                << unchanged << "}" << std::endl;
            EXPECT_TRUE(unchanged);
        }
        if (variant == "selfclose") {
            const bool closed = App::GetApplication().closeDocument(req.documentName.c_str());
            std::cout << "SPEC012 {\"case\":\"Q11\",\"self_close_return\":" << closed << "}"
                      << std::endl;
        }
        else if (variant == "selfcancel") {
            const auto canceled = App::GetApplication().cancelRecomputeRequest(req);
            std::cout << "SPEC012 {\"case\":\"Q11\",\"self_cancel_inprogress\":"
                      << canceled.canceledInProgress
                      << ",\"self_cancel_removed\":" << canceled.removedQueued << "}" << std::endl;
        }
        else if (variant == "reentry") {
            auto again = App::RecomputeRequest::fromDocumentObject(*req.resolveDocumentObject());
            again.callback = spec012Subscriber("Q11", "reentrant_nested_subscriber", "Target", nested);
            App::GetApplication().queueRecomputeRequest(std::move(again));
        }
    };
    App::GetApplication().queueRecomputeRequest(std::move(request));
    auto duplicate = App::RecomputeRequest::fromDocumentObject(*target);
    duplicate.callback = spec012Subscriber(family, variant + "_subscriber_B", "Target", second);
    App::GetApplication().queueRecomputeRequest(std::move(duplicate));
    auto later = App::RecomputeRequest::fromDocumentObject(*successor);
    later.callback = spec012Subscriber(family, variant + "_later_subscriber", "Successor", after);
    App::GetApplication().queueRecomputeRequest(std::move(later));
    gate.release();
    ASSERT_EQ(first->done.wait_for(10s), std::future_status::ready);
    ASSERT_EQ(second->done.wait_for(10s), std::future_status::ready);
    ASSERT_EQ(after->done.wait_for(10s), std::future_status::ready);
    if (variant == "reentry") {
        ASSERT_EQ(nested->done.wait_for(10s), std::future_status::ready);
    }
    EXPECT_EQ(first->count.load(), 1);
    EXPECT_EQ(second->count.load(), 1);
    EXPECT_EQ(after->count.load(), 1);
}

TEST_F(AsyncRecomputeTest, Spec012CloseInFlightRerunSubscribers)
{
    const auto name = App::GetApplication().getUniqueDocumentName("spec012_close_rerun");
    auto* document = App::GetApplication().newDocument(name.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (App::GetApplication().getDocument(name.c_str())) {
            App::GetApplication().closeDocument(name.c_str());
        }
    };
    auto* target = document->addObject<App::FeatureTestAsyncBlocker>("Target");
    target->touch();
    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };
    auto first = std::make_shared<Spec012Subscriber>();
    auto second = std::make_shared<Spec012Subscriber>();
    auto third = std::make_shared<Spec012Subscriber>();
    auto progress = std::make_shared<App::RecomputeProgressHandle>();
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    request.progress = progress;
    request.callback = spec012Subscriber("Q08", "close_inflight_A", "Target", first);
    spec012Record("Q08", "close_inflight_enqueue", *document, target, -999, false);
    App::GetApplication().queueRecomputeRequest(std::move(request));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
    for (const auto& item :
         {std::pair {"close_rerun_B", second}, std::pair {"close_rerun_C", third}}) {
        auto rerun = App::RecomputeRequest::fromDocumentObject(*target);
        rerun.callback = spec012Subscriber("Q08", item.first, "Target", item.second);
        App::GetApplication().queueRecomputeRequest(std::move(rerun));
    }
    auto closing = std::async(std::launch::async, [&] {
        return App::GetApplication().closeDocument(name.c_str());
    });
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (!progress->wasCanceled() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    const bool cancellationEntered = progress->wasCanceled();
    App::FeatureTestAsyncBlocker::releaseBlocker();
    EXPECT_TRUE(cancellationEntered);
    ASSERT_EQ(closing.wait_for(10s), std::future_status::ready);
    EXPECT_TRUE(closing.get());
    EXPECT_EQ(first->count.load(), 1);
    auto* sentinel = _doc->addObject<App::Spec012OutcomeFeature>("CloseSuccessor");
    spec012Queue(
        "Q08",
        "close_inflight_later_progress",
        App::RecomputeRequest::fromDocumentObject(*sentinel),
        *_doc,
        sentinel
    );
    spec012Record(
        "Q08",
        "closed_target_rerun_B_after_drain",
        *_doc,
        nullptr,
        -999,
        false,
        nullptr,
        second->count.load()
    );
    spec012Record(
        "Q08",
        "closed_target_rerun_C_after_drain",
        *_doc,
        nullptr,
        -999,
        false,
        nullptr,
        third->count.load()
    );
}

TEST_F(AsyncRecomputeTest, Spec012BeforeRecomputeTargetAndMembership)
{
    const char* configured = std::getenv("SPEC012_PREPARATION_CASE");
    const std::string variant = configured ? configured : "control";
    const bool recursive = variant.find("recursive") == 0;
    const bool unsafe = variant.find("unsafe") != std::string::npos;
    const bool removes = variant == "recursive_remove" || variant == "recursive_replace";
    const char* family = removes ? "Q03" : "Q06";
    auto* target = _doc->addObject<App::Spec012OutcomeFeature>("Target");
    target->touch();
    const auto oldSerial = target->lifetimeSerial;
    const auto oldId = target->getID();
    std::atomic<int> observerCalls {0};
    std::atomic<int> deleted {0};
    fastsignals::scoped_connection deleting(
        _doc->signalDeletedObject.connect([&](const App::DocumentObject& object) {
            if (object.getID() == oldId) {
                ++deleted;
            }
        })
    );
    fastsignals::scoped_connection before(_doc->signalBeforeRecompute.connect([&](const App::Document&) {
        if (++observerCalls != 1) {
            return;
        }
        spec012Record(
            family,
            variant + "_before_recompute_entered",
            *_doc,
            _doc->getObject("Target"),
            -999,
            false
        );
        if (removes) {
            _doc->removeObject("Target");
            // No access to the old native pointer after the public removal.
            if (variant == "recursive_replace") {
                auto* replacement = _doc->addObject<App::Spec012OutcomeFeature>("Target");
                replacement->touch();
                spec012Record(family, variant + "_replacement_after_remove", *_doc, replacement, -999, false);
            }
            std::cout << "SPEC012 {\"case\":\"Q03\",\"port\":\"before_recompute_remove_event\","
                      << "\"old_id\":" << oldId << ",\"old_serial\":" << oldSerial
                      << ",\"deleted_signals\":" << deleted.load() << "}" << std::endl;
        }
        else if (variant != "control") {
            auto* member = unsafe
                ? static_cast<App::Spec012OutcomeFeature*>(
                      _doc->addObject<App::Spec012UnsafeOutcomeFeature>("LateMember")
                  )
                : _doc->addObject<App::Spec012OutcomeFeature>("LateMember");
            member->touch();
            if (recursive) {
                auto* current = dynamic_cast<App::Spec012OutcomeFeature*>(_doc->getObject("Target"));
                ASSERT_NE(current, nullptr);
                current->Source1.setValue(member);
            }
            spec012Record(family, variant + "_member_after_admission", *_doc, member, -999, false);
        }
    }));
    auto result = std::make_shared<Spec012Subscriber>();
    auto request = recursive ? App::RecomputeRequest::fromDocumentObject(*target, true)
                             : App::RecomputeRequest::fromDocument(*_doc, true);
    EXPECT_TRUE(App::GetApplication().canRecomputeRequestOnWorker(request));
    spec012Record(family, variant + "_initial_admission", *_doc, target, -999, false);
    request.callback = spec012Subscriber(family, variant + "_terminal", "Target", result);
    App::GetApplication().queueRecomputeRequest(std::move(request));
    ASSERT_EQ(result->done.wait_for(10s), std::future_status::ready);
    before.disconnect();
    deleting.disconnect();
    EXPECT_EQ(observerCalls.load(), 1);
    if (auto* member = _doc->getObject("LateMember")) {
        spec012Record(family, variant + "_member_after_completion", *_doc, member, -999, false);
    }
    auto* sentinel = _doc->addObject<App::Spec012OutcomeFeature>("AfterPreparation");
    sentinel->touch();
    spec012Queue(
        family,
        variant + "_later_sentinel",
        App::RecomputeRequest::fromDocumentObject(*sentinel),
        *_doc,
        sentinel
    );
    spec012Record(
        family,
        variant + "_after_drain",
        *_doc,
        _doc->getObject("Target"),
        -999,
        false,
        nullptr,
        result->count.load()
    );
}

TEST_F(AsyncRecomputeTest, Spec012NativeCancellationWithoutProgressFlag)
{
    for (int mode : {2, 3}) {
        for (int port : {0, 1, 2}) {
            auto* feature = _doc->addObject<App::Spec012OutcomeFeature>("NativeCancellation");
            feature->mode = mode;
            feature->touch();
            auto progress = std::make_shared<App::RecomputeProgressHandle>();
            EXPECT_FALSE(progress->wasCanceled());
            auto completed = std::make_shared<Spec012Subscriber>();
            auto request = port == 2
                ? App::RecomputeRequest::fromDocument(*_doc, true)
                : App::RecomputeRequest::fromDocumentObject(*feature, port == 1);
            request.progress = progress;
            const std::string name = mode == 2 ? "native_user_abort" : "native_canceled_return";
            request.callback =
                [&, completed, name, port](App::RecomputeRequest&, App::RecomputeResult& result) {
                    spec012Record(
                        "O11",
                        name + "_port_" + std::to_string(port),
                        *_doc,
                        feature,
                        -999,
                        false,
                        &result,
                        ++completed->count,
                        std::this_thread::get_id()
                    );
                    EXPECT_FALSE(result.success);
                    EXPECT_EQ(result.failure, App::RecomputeFailure::Canceled);
                    EXPECT_TRUE(progress->wasCanceled());
                    completed->completed.set_value();
                };
            App::GetApplication().queueRecomputeRequest(std::move(request));
            ASSERT_EQ(completed->done.wait_for(10s), std::future_status::ready);
            EXPECT_EQ(feature->ExecCount.getValue(), 1);
            EXPECT_FALSE(feature->isError());
            // Repair on a new progress handle; no stale cancellation or no-op control.
            feature->mode = 0;
            feature->touch();
            spec012Queue(
                "O11",
                name + "_repaired_actual_retry",
                App::RecomputeRequest::fromDocumentObject(*feature),
                *_doc,
                feature
            );
            EXPECT_EQ(feature->ExecCount.getValue(), 2);
            _doc->removeObject(feature->getNameInDocument());
        }
    }
}

TEST_F(AsyncRecomputeTest, Spec012SynchronousStructuredOutcomes)
{
    static_assert(static_cast<int>(App::RecomputeFailure::None) == 0);
    static_assert(static_cast<int>(App::RecomputeFailure::Canceled) == 1);
    static_assert(static_cast<int>(App::RecomputeFailure::DependencyCycle) == 2);
    static_assert(static_cast<int>(App::RecomputeFailure::Exception) == 3);
    static_assert(static_cast<int>(App::RecomputeFailure::ModelError) == 4);
    static_assert(static_cast<int>(App::RecomputeFailure::TargetUnavailable) == 5);
    int callbacks = 0;
    for (int mode : {0, 1, 2, 3}) {
        for (bool documentRequest : {false, true}) {
            auto* target = _doc->addObject<App::Spec012OutcomeFeature>("SynchronousTarget");
            target->mode = mode;
            target->touch();
            auto request = documentRequest
                ? App::RecomputeRequest::fromDocument(*_doc, true)
                : App::RecomputeRequest::fromDocumentObject(*target, true);
            request.progress = std::make_shared<App::RecomputeProgressHandle>();
            request.callback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
                ++callbacks;
            };
            auto result = App::GetApplication().recomputeRequestSynchronously(request);
            const auto expected = mode == 0 ? App::RecomputeFailure::None
                : mode == 1                 ? App::RecomputeFailure::ModelError
                                            : App::RecomputeFailure::Canceled;
            EXPECT_EQ(result.success, mode == 0);
            EXPECT_EQ(result.failure, expected);
            EXPECT_EQ(target->ExecCount.getValue(), 1);
            if (mode <= 1) {
                EXPECT_FALSE(result.exception);
            }
            spec012Record(
                "O01",
                "sync_api_mode_" + std::to_string(mode) + (documentRequest ? "_document" : "_object"),
                *_doc,
                target,
                -999,
                false,
                &result
            );
            _doc->removeObject(target->getNameInDocument());
        }
    }
    for (int exceptionType : {1, 2}) {
        auto* target = _doc->addObject<App::Spec012OutcomeFeature>("CaughtExceptionTarget");
        target->ExceptionType.setValue(exceptionType);
        target->touch();
        auto result = App::GetApplication().recomputeRequestSynchronously(
            App::RecomputeRequest::fromDocumentObject(*target, true)
        );
        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.failure, App::RecomputeFailure::ModelError);
        EXPECT_FALSE(result.exception);  // Exception was handled inside the native feature path.
        _doc->removeObject(target->getNameInDocument());
    }
    auto empty = App::GetApplication().recomputeRequestSynchronously({});
    EXPECT_FALSE(empty.success);
    EXPECT_EQ(empty.failure, App::RecomputeFailure::TargetUnavailable);
    EXPECT_FALSE(empty.exception);
    App::RecomputeRequest missingObject;
    missingObject.documentName = _doc->getName();
    missingObject.documentObjectName = "AbsentTarget";
    missingObject.callback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        ++callbacks;
    };
    auto absent = App::GetApplication().recomputeRequestSynchronously(missingObject);
    EXPECT_FALSE(absent.success);
    EXPECT_EQ(absent.failure, App::RecomputeFailure::TargetUnavailable);
    EXPECT_FALSE(absent.exception);
    EXPECT_EQ(callbacks, 0);
}

TEST_F(AsyncRecomputeTest, Spec012SynchronousScopeAndContextPreservation)
{
    auto* bad = _doc->addObject<App::Spec012OutcomeFeature>("UnrelatedError");
    bad->mode = 1;
    bad->touch();
    EXPECT_FALSE(bad->recomputeFeature(true));
    auto* good = _doc->addObject<App::Spec012OutcomeFeature>("ScopedGood");
    good->touch();
    auto request = App::RecomputeRequest::fromDocumentObject(*good, true);
    request.progress = std::make_shared<App::RecomputeProgressHandle>();
    {
        App::ScopedRecomputeOptions outer(App::RecomputeOption::InteractivePreview);
        const int outerOptions = App::currentRecomputeOptions();
        request.options = 0;
        auto result = App::GetApplication().recomputeRequestSynchronously(request);
        EXPECT_TRUE(result.success);
        EXPECT_EQ(good->observedOptions, 0);
        EXPECT_EQ(good->observedProgress, request.progress.get());
        EXPECT_EQ(good->executionThread, std::this_thread::get_id());
        EXPECT_EQ(good->ExecCount.getValue(), 1);
        EXPECT_EQ(App::currentRecomputeOptions(), outerOptions);
        EXPECT_EQ(App::currentRecomputeProgress(), nullptr);
        EXPECT_TRUE(bad->isError());
    }
    bad->mode = 0;
    bad->touch();
    EXPECT_TRUE(bad->recomputeFeature(true));
    _doc->recompute({}, true);
    const int goodCount = good->ExecCount.getValue();
    auto noOp = App::GetApplication().recomputeRequestSynchronously(
        App::RecomputeRequest::fromDocument(*_doc)
    );
    EXPECT_TRUE(noOp.success);
    EXPECT_EQ(noOp.failure, App::RecomputeFailure::None);
    EXPECT_EQ(good->ExecCount.getValue(), goodCount);
}
