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
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <set>
#include <mutex>
#include <thread>
#include <vector>
#include <type_traits>

#include <boost/scope_exit.hpp>
#include <gtest/gtest.h>

#include "App/Application.h"
#include "App/Document.h"
#include "App/AutoTransaction.h"
#include "App/Expression.h"
#include "App/ObjectIdentifier.h"
#include "App/PropertyExpressionEngine.h"
#include "App/PropertyLinks.h"
#include "App/private/RecomputeAdmission.h"
#include "App/FeatureTest.h"
#include "App/GeoFeature.h"
#include "Base/Console.h"
#include "Base/Interpreter.h"
#include "Base/Sequencer.h"
#include <src/App/InitApplication.h>

using namespace std::chrono_literals;

namespace
{
// Test-only adapters preserve protected production visibility.
class Spec012ApplicationAccess: public App::Application
{
public:
    // Type access only: never construct or destroy an Application adapter.
    Spec012ApplicationAccess() = delete;
    ~Spec012ApplicationAccess() override;
    using App::Application::TransactionSignaller;
};

class Spec012CancellationSequencer final: public Base::SequencerBase
{
public:
    using Base::SequencerBase::rejectCancel;
    using Base::SequencerBase::tryToCancel;

protected:
    void setText(const char*) override
    {}
    void setProgress(std::size_t) override
    {}
};

std::thread::id spec012FixtureThread;
std::atomic<int> spec012WakeBoundary {0};
struct Spec012GilCompletionState
{
    std::promise<void> entered;
    std::shared_future<void> proceed;
    std::atomic<bool> acquired {false};
};
}  // namespace

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
    mutable std::atomic<int> workerSafetyQueries {0};
    bool canRecomputeOnWorker() const override
    {
        ++workerSafetyQueries;
        return workerAllowed.load();
    }
    std::thread::id executionThread;
    int workerEligibilityAtExecute {-1};
    int observedOptions {0};
    std::shared_ptr<Spec012GilCompletionState> gilCompletion;
    std::function<void()> duringExecute;
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
        if (mode == 6) {
            duringExecute();
            ExecCount.setValue(ExecCount.getValue() + 1);
            return StdReturn;
        }
        if (mode == 5) {
            auto state = gilCompletion;
            state->entered.set_value();
            state->proceed.wait();
            Base::PyGILStateLocker gil;
            state->acquired = true;
            ExecCount.setValue(ExecCount.getValue() + 1);
            return StdReturn;
        }
        if (mode == 4) {
            ExecCount.setValue(ExecCount.getValue() + 1);
            duringExecute();
            return StdReturn;
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


namespace App
{
class Spec012ProposalProperty: public PropertyLinkSub
{
public:
    DocumentObject* nextTarget {};
    mutable unsigned proposals {};
    Property* CopyOnLabelChange(DocumentObject*, const std::string&, const char*) const override
    {
        ++proposals;
        auto copy = std::make_unique<PropertyLinkSub>();
        copy->setAllowExternal(true);
        copy->setValue(nextTarget);
        return copy.release();
    }
};
class Spec012LabelProposalFeature: public FeatureTest
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::Spec012LabelProposalFeature);

public:
    Spec012LabelProposalFeature()
    {
        ADD_PROPERTY(Proposed, (nullptr));
    }
    Spec012ProposalProperty Proposed;
};
}  // namespace App
PROPERTY_SOURCE(App::Spec012LabelProposalFeature, App::FeatureTest)
// Register Spec012LabelProposalFeature::init() once in test suite setup.


namespace App
{
class Spec012LifetimeLinkSub: public PropertyLinkSub
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    inline static std::atomic<int> destructions {0};
    Spec012LifetimeLinkSub() = default;
    ~Spec012LifetimeLinkSub() override
    {
        ++destructions;
    }
};
}  // namespace App
TYPESYSTEM_SOURCE(App::Spec012LifetimeLinkSub, App::PropertyLinkSub)

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
        App::Spec012LabelProposalFeature::init();
        App::Spec012LifetimeLinkSub::init();
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
    ASSERT_TRUE(callbackChanged.wait_for(callbackLock, 2s, [&] { return totalCallbacks == 2; }));
    EXPECT_EQ(firstRunCallbacks, 1);
    EXPECT_EQ(rerunCallbacks, 1);
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

bool spec012WaitRetired(const std::shared_ptr<App::RecomputeDetail::Retirement>& ticket)
{
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (!ticket->retired() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    return ticket->retired();
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
    auto retirement = std::make_shared<App::RecomputeDetail::Retirement>();
    App::GetApplication().queueRecomputeRequestWithRetirement(std::move(request), retirement);
    ASSERT_EQ(done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(retirement));
    EXPECT_EQ(terminals.load(), 1);
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
    std::atomic<bool> success {false};
    std::atomic<App::RecomputeFailure> failure {App::RecomputeFailure::None};
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
        state->success = result.success;
        state->failure = result.failure;
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
                      << ",\"binding_unavailable\":true,\"terminal_count\":" << count << "}"
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
        , retirement(std::make_shared<App::RecomputeDetail::Retirement>())
    {
        App::FeatureTestAsyncBlocker::resetBlocker();
        blocker->touch();
        auto request = App::RecomputeRequest::fromDocumentObject(*blocker);
        request.callback = [state = terminal](App::RecomputeRequest&, App::RecomputeResult&) {
            if (++state->count == 1) {
                state->completed.set_value();
            }
        };
        App::GetApplication().queueRecomputeRequestWithRetirement(std::move(request), retirement);
        if (!App::FeatureTestAsyncBlocker::waitUntilStarted(10s)) {
            App::FeatureTestAsyncBlocker::releaseBlocker();
            throw std::runtime_error("Spec012 D1 native gate never started");
        }
    }
    ~Spec012QueueBlocker()
    {
        release();
        EXPECT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
        EXPECT_TRUE(spec012WaitRetired(retirement));
        // Callback publication precedes worker cleanup. Await the exact native
        // retirement of both fixture requests before direct object removal.
        auto* successor = document.addObject<App::FeatureTest>("GateRetirementSentinel");
        successor->touch();
        auto retired = std::make_shared<Spec012Subscriber>();
        auto next = App::RecomputeRequest::fromDocumentObject(*successor);
        next.callback = [retired](App::RecomputeRequest&, App::RecomputeResult&) {
            if (++retired->count == 1) {
                retired->completed.set_value();
            }
        };
        auto successorRetirement = std::make_shared<App::RecomputeDetail::Retirement>();
        App::GetApplication().queueRecomputeRequestWithRetirement(std::move(next), successorRetirement);
        EXPECT_EQ(retired->done.wait_for(10s), std::future_status::ready);
        EXPECT_TRUE(spec012WaitRetired(successorRetirement));
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
    std::shared_ptr<App::RecomputeDetail::Retirement> retirement;
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
        if (variant == "missing" || variant == "same_type" || variant == "unsafe_type") {
            EXPECT_FALSE(state->success.load());
            EXPECT_EQ(state->failure.load(), App::RecomputeFailure::TargetUnavailable);
        }
        else if (variant.find("closure") != std::string::npos) {
            EXPECT_FALSE(state->success.load());
            EXPECT_EQ(state->failure.load(), App::RecomputeFailure::Exception);
        }
        else {
            EXPECT_TRUE(state->success.load());
        }
        if (auto* target = dynamic_cast<App::Spec012OutcomeFeature*>(document->getObject("Target"))) {
            spec012Record(family, variant + "_after_completion", *document, target, -999, false);
            EXPECT_EQ(target->ExecCount.getValue(), variant == "stable" || variant == "manual" ? 1 : 0);
        }
        if (auto* unsafe = dynamic_cast<App::Spec012UnsafeOutcomeFeature*>(
                document->getObject("UnsafeDependency")
            )) {
            spec012Record(family, variant + "_unsafe_after_completion", *document, unsafe, -999, false);
            EXPECT_EQ(unsafe->ExecCount.getValue(), 0);
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
        auto incoherent = std::make_shared<Spec012Subscriber>();
        const char* family = variant == "mutation"                         ? "Q09"
            : (variant.find("cancel") == 0 || variant == "close_document") ? "Q08"
                                                                           : "Q07";
        spec012Record(family, variant + "_old_registration", *document, target, -999, false);
        auto request = App::RecomputeRequest::fromDocumentObject(*target);
        auto firstCallback = spec012Subscriber(family, variant + "_subscriber_A", "Target", first);
        request.callback = [firstCallback,
                            variant,
                            incoherent](App::RecomputeRequest& req, App::RecomputeResult& result) {
            firstCallback(req, result);
            if (variant == "mutation") {
                req.documentObjectName = "CallbackMutatedName";
                result.success = false;
                result.failure = App::RecomputeFailure::Canceled;
                auto edited = req;
                edited.callback
                    = spec012Subscriber("Q09", "edited_bound_view_rejected", "Target", incoherent);
                App::GetApplication().queueRecomputeRequest(std::move(edited));
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
        ASSERT_EQ(first->done.wait_for(10s), std::future_status::ready);
        ASSERT_EQ(second->done.wait_for(10s), std::future_status::ready);
        EXPECT_EQ(first->count.load(), 1);
        EXPECT_EQ(second->count.load(), 1);
        if (variant == "mutation") {
            ASSERT_EQ(incoherent->done.wait_for(2s), std::future_status::ready);
            EXPECT_EQ(incoherent->count.load(), 1);
            EXPECT_EQ(incoherent->failure.load(), App::RecomputeFailure::Exception);
        }
        if (variant == "replacement") {
            EXPECT_EQ(first->failure.load(), App::RecomputeFailure::TargetUnavailable);
            EXPECT_TRUE(second->success.load());
            EXPECT_EQ(target->ExecCount.getValue(), 1);
        }
        else if (variant.find("cancel") == 0 || variant == "close_document") {
            EXPECT_EQ(first->failure.load(), App::RecomputeFailure::Canceled);
            EXPECT_EQ(second->failure.load(), App::RecomputeFailure::Canceled);
        }
        else {
            EXPECT_TRUE(first->success.load());
            EXPECT_EQ(
                second->failure.load(),
                variant == "mutation" ? App::RecomputeFailure::Canceled : App::RecomputeFailure::None
            );
            EXPECT_EQ(target->ExecCount.getValue(), 1);
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
        ASSERT_EQ(second->done.wait_for(10s), std::future_status::ready);
        ASSERT_EQ(third->done.wait_for(10s), std::future_status::ready);
        EXPECT_EQ(second->count.load(), 1);
        EXPECT_EQ(third->count.load(), 1);
        EXPECT_EQ(
            second->failure.load(),
            cancel ? App::RecomputeFailure::Canceled : App::RecomputeFailure::None
        );
        EXPECT_EQ(
            third->failure.load(),
            cancel ? App::RecomputeFailure::Canceled : App::RecomputeFailure::None
        );
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

namespace
{
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void spec012ShutdownReturnedMarker()
{
    std::atomic_signal_fence(std::memory_order_seq_cst);
}
}  // namespace

TEST_F(AsyncRecomputeTest, Spec012IsolatedCallbackLifecycle)
{
    const char* configured = std::getenv("SPEC012_CALLBACK_CASE");
    const std::string variant = configured ? configured : "reentry";
    if (variant == "normal_destruct") {
        const char* configuredOut = std::getenv("SPEC012_OUT");
        ASSERT_NE(configuredOut, nullptr);
        const std::string output(configuredOut);
        struct ParameterSnapshot
        {
            std::string key;
            std::string path;
            bool existed;
            std::string bytes;
        };
        std::vector<ParameterSnapshot> parameters;
        for (const std::string key : {"UserParameter", "SystemParameter"}) {
            const auto path = App::Application::Config().at(key);
            ASSERT_EQ(path.rfind(output + "/", 0), 0U);
            std::ifstream input(path, std::ios::binary);
            const bool exists = input.good();
            std::string bytes {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
            parameters.push_back({key, path, exists, bytes});
            std::ofstream snapshot(output + "/normal-before-" + key + ".bin", std::ios::binary);
            snapshot << bytes;
        }
        auto& application = App::GetApplication();
        auto* active = _doc->addObject<App::FeatureTestAsyncBlocker>("NormalShutdownActive");
        const auto queuedName = application.getUniqueDocumentName("normal_shutdown_queued");
        auto* queuedDocument = application.newDocument(queuedName.c_str());
        auto* queued = queuedDocument->addObject<App::Spec012OutcomeFeature>("Queued");
        auto* reentrant = queuedDocument->addObject<App::Spec012OutcomeFeature>("Reentrant");
        active->touch();
        queued->touch();
        reentrant->touch();
        std::vector<std::shared_ptr<Spec012Subscriber>> states;
        std::vector<std::shared_ptr<App::RecomputeProgressHandle>> handles;
        std::vector<std::shared_ptr<App::RecomputeDetail::Retirement>> retirements;
        for (int index = 0; index != 7; ++index) {
            states.push_back(std::make_shared<Spec012Subscriber>());
            handles.push_back(std::make_shared<App::RecomputeProgressHandle>());
            retirements.push_back(std::make_shared<App::RecomputeDetail::Retirement>());
        }
        std::atomic<int> waitRefusals {0};
        std::atomic<int> activeExecutions {0};
        std::atomic<bool> sawCancellation {false};
        std::function<void(App::DocumentObject*, int, bool)> submit;
        submit = [&](App::DocumentObject* object, int index, bool hasCallback) {
            auto request = App::RecomputeRequest::fromDocumentObject(*object);
            request.progress = handles[index];
            if (hasCallback) {
                request.callback = [&, index](App::RecomputeRequest&, App::RecomputeResult& result) {
                    ++states[index]->count;
                    states[index]->success = result.success;
                    states[index]->failure = result.failure;
                    EXPECT_EQ(result.failure, App::RecomputeFailure::Canceled);
                    bool refused = false;
                    try {
                        App::RecomputeDetail::Coordinator::instance().ensureWaitAllowed();
                    }
                    catch (const App::RecomputeDetail::AdmissionRefusal&) {
                        refused = true;
                        ++waitRefusals;
                    }
                    EXPECT_TRUE(refused);
                    if (index == 0) {
                        activeExecutions = App::FeatureTestAsyncBlocker::getExecutionCount();
                    }
                    if (index == 3) {
                        EXPECT_EQ(queued->ExecCount.getValue(), 0);
                        App::GetApplication()
                            .GetParameterGroupByPath("User parameter:Spec012")
                            ->SetASCII("ShutdownCallbackMarker", "completed-before-save");
                        throw std::runtime_error("normal shutdown subscriber exception");
                    }
                    if (index == 4) {
                        submit(reentrant, 5, true);
                    }
                    if (index == 5) {
                        EXPECT_EQ(reentrant->ExecCount.getValue(), 0);
                    }
                };
            }
            application.queueRecomputeRequestWithRetirement(std::move(request), retirements[index]);
        };
        App::FeatureTestAsyncBlocker::resetBlocker();
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::FeatureTestAsyncBlocker::releaseBlocker();
        };
        submit(active, 0, true);
        ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
        submit(active, 1, true);
        submit(active, 2, true);
        submit(queued, 3, true);
        submit(queued, 4, true);
        submit(queued, 6, false);
        std::thread releaseWorker([&] {
            const auto deadline = std::chrono::steady_clock::now() + 10s;
            while (!handles[0]->wasCanceled() && std::chrono::steady_clock::now() < deadline) {
                std::this_thread::yield();
            }
            sawCancellation = handles[0]->wasCanceled();
            App::FeatureTestAsyncBlocker::releaseBlocker();
        });
        // No fixture or static App teardown may run after the actual singleton
        // and Python/type registries have been destroyed in this private child.
        _doc = nullptr;
        _docName.clear();
        std::cout
            << "SPEC012 {\"case\":\"Q11\",\"port\":\"actual_normal_Application_destruct_enter\"}"
            << std::endl;
        App::Application::destruct();
        spec012ShutdownReturnedMarker();
        releaseWorker.join();
        bool allRetired = true;
        bool allCanceled = true;
        bool exactTerminals = true;
        for (int index = 0; index != 7; ++index) {
            allRetired = allRetired && retirements[index]->retired();
            allCanceled = allCanceled && handles[index]->wasCanceled();
            exactTerminals = exactTerminals && states[index]->count == (index == 6 ? 0 : 1);
        }
        EXPECT_TRUE(sawCancellation.load());
        EXPECT_TRUE(allRetired);
        EXPECT_TRUE(allCanceled);
        EXPECT_TRUE(exactTerminals);
        EXPECT_EQ(activeExecutions.load(), 1);
        EXPECT_EQ(waitRefusals.load(), 6);
        bool parameterMarkerPersisted = false;
        bool serialized = true;
        std::ofstream receipt(output + "/normal-destruct-receipt.json");
        receipt << std::boolalpha;
        receipt << "{\"custom_native_port\":true,\"actual_destruct_returned\":true,"
                << "\"gtest_xml_pass_claimed\":false,\"saw_native_cancel\":" << sawCancellation.load()
                << ",\"all_retired\":" << allRetired << ",\"all_handles_canceled\":" << allCanceled
                << ",\"exact_terminals\":" << exactTerminals
                << ",\"active_executions\":" << activeExecutions.load()
                << ",\"wait_refusals\":" << waitRefusals.load() << ",\"terminals\":[";
        for (int index = 0; index != 7; ++index) {
            if (index) {
                receipt << ',';
            }
            receipt << states[index]->count.load();
        }
        receipt << "],\"parameters\":[";
        for (std::size_t index = 0; index != parameters.size(); ++index) {
            const auto& before = parameters[index];
            std::ifstream input(before.path, std::ios::binary);
            const bool exists = input.good();
            const std::string after {
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>()
            };
            serialized = serialized && exists && !after.empty();
            if (before.key == "UserParameter") {
                parameterMarkerPersisted = after.find("completed-before-save") != std::string::npos;
            }
            if (index) {
                receipt << ',';
            }
            receipt << "{\"key\":" << std::quoted(before.key)
                    << ",\"path\":" << std::quoted(before.path)
                    << ",\"before_exists\":" << before.existed << ",\"after_exists\":" << exists
                    << ",\"before_bytes\":" << before.bytes.size()
                    << ",\"after_bytes\":" << after.size() << '}';
        }
        receipt << "],\"parameter_callback_persisted\":" << parameterMarkerPersisted
                << ",\"parameters_serialized\":" << serialized << "}\n";
        receipt.close();
        EXPECT_TRUE(parameterMarkerPersisted);
        EXPECT_TRUE(serialized);
        std::cout << "SPEC012 "
                     "{\"case\":\"Q11\",\"port\":\"actual_normal_destruct_custom_receipt_written\"}"
                  << std::endl;
        std::_Exit(::testing::Test::HasFailure() ? 1 : 0);
    }
    if (variant == "owner_destruct") {
        const char* output = std::getenv("SPEC012_OUT");
        ASSERT_NE(output, nullptr);
        auto* target = _doc->addObject<App::FeatureTestAsyncBlocker>("OwnerShutdownTarget");
        App::FeatureTestAsyncBlocker::resetBlocker();
        target->touch();
        auto request = App::RecomputeRequest::fromDocumentObject(*target);
        auto running = std::async(std::launch::async, [request = std::move(request)]() mutable {
            return App::GetApplication().recomputeRequestSynchronously(std::move(request));
        });
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::FeatureTestAsyncBlocker::releaseBlocker();
        };
        ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
        std::vector<std::pair<std::string, std::string>> parameters;
        std::vector<bool> existed;
        for (const std::string key : {"UserParameter", "SystemParameter"}) {
            const auto path = App::Application::Config().at(key);
            ASSERT_EQ(path.rfind(std::string(output) + "/", 0), 0U);
            std::ifstream input(path, std::ios::binary);
            existed.push_back(input.good());
            parameters.emplace_back(
                path,
                std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>())
            );
        }
        std::cout << "SPEC012 "
                     "{\"case\":\"Q11\",\"port\":\"actual_owner_busy_Application_destruct_enter\"}"
                  << std::endl;
        App::Application::destruct();
        for (std::size_t index = 0; index != parameters.size(); ++index) {
            std::ifstream input(parameters[index].first, std::ios::binary);
            EXPECT_EQ(input.good(), existed[index]);
            EXPECT_EQ(
                std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()),
                parameters[index].second
            );
        }
        EXPECT_EQ(App::GetApplication().getDocument(_docName.c_str()), _doc);
        App::FeatureTestAsyncBlocker::releaseBlocker();
        ASSERT_EQ(running.wait_for(10s), std::future_status::ready);
        EXPECT_TRUE(running.get().success);
        auto* control = _doc->addObject<App::Spec012OutcomeFeature>("OwnerShutdownControl");
        control->touch();
        spec012Queue(
            "Q11",
            "owner_busy_destruct_later_queue_progress",
            App::RecomputeRequest::fromDocumentObject(*control),
            *_doc,
            control
        );
        return;
    }
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
            EXPECT_FALSE(closed);
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
    EXPECT_EQ(second->count.load(), 1);
    EXPECT_EQ(third->count.load(), 1);
    EXPECT_EQ(second->failure.load(), App::RecomputeFailure::Canceled);
    EXPECT_EQ(third->failure.load(), App::RecomputeFailure::Canceled);
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
    auto progress = std::make_shared<App::RecomputeProgressHandle>();
    request.progress = progress;
    request.callback = [callback = spec012Subscriber(family, variant + "_terminal", "Target", result),
                        removes,
                        target](App::RecomputeRequest& req, App::RecomputeResult& outcome) {
        if (removes) {
            EXPECT_EQ(target->ExecCount.getValue(), 0);
            EXPECT_FALSE(outcome.exception);
        }
        callback(req, outcome);
    };
    auto retirement = std::make_shared<App::RecomputeDetail::Retirement>();
    App::GetApplication().queueRecomputeRequestWithRetirement(std::move(request), retirement);
    ASSERT_EQ(result->done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(retirement));
    EXPECT_EQ(result->count.load(), 1);
    if (unsafe) {
        EXPECT_FALSE(result->success.load());
        EXPECT_EQ(result->failure.load(), App::RecomputeFailure::Exception);
        EXPECT_EQ(target->ExecCount.getValue(), 0);
    }
    else if (!removes) {
        EXPECT_TRUE(result->success.load());
    }
    if (removes) {
        EXPECT_FALSE(result->success.load());
        EXPECT_EQ(result->failure.load(), App::RecomputeFailure::TargetUnavailable);
        EXPECT_FALSE(progress->wasCanceled());
        EXPECT_EQ(deleted.load(), 1);
    }
    before.disconnect();
    deleting.disconnect();
    EXPECT_EQ(observerCalls.load(), 1);
    if (auto* member = _doc->getObject("LateMember")) {
        spec012Record(family, variant + "_member_after_completion", *_doc, member, -999, false);
        auto* observed = dynamic_cast<App::Spec012OutcomeFeature*>(member);
        ASSERT_NE(observed, nullptr);
        EXPECT_EQ(observed->ExecCount.getValue(), unsafe ? 0 : 1);
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


static_assert(std::is_aggregate_v<App::RecomputeRequest>);
static_assert(std::is_copy_constructible_v<App::RecomputeRequest>);
static_assert(std::is_move_constructible_v<App::RecomputeRequest>);
static_assert(std::is_copy_assignable_v<App::RecomputeRequest>);
static_assert(std::is_move_assignable_v<App::RecomputeRequest>);
static_assert(!std::is_assignable_v<App::RecomputeBindingHandle&, App::RecomputeBindingHandle>);

TEST_F(AsyncRecomputeTest, Spec012FactoryBindingSurvivesPublicHandleMoveAndNameReuse)
{
    auto& app = App::GetApplication();
    auto* original = _doc->addObject<App::Spec012OutcomeFeature>("Original");
    original->touch();
    auto request = App::RecomputeRequest::fromDocumentObject(*original);
    auto externalHandle = std::move(request.targetBinding);
    auto copied = request;
    _doc->removeObject("Original");
    auto* replacement = _doc->addObject<App::Spec012OutcomeFeature>("Original");
    replacement->touch();
    for (auto* stale : {&request, &copied}) {
        EXPECT_EQ(stale->resolveDocumentObject(), nullptr);
        auto outcome = app.recomputeRequestSynchronously(*stale);
        EXPECT_FALSE(outcome.success);
        EXPECT_EQ(outcome.failure, App::RecomputeFailure::TargetUnavailable);
    }
    EXPECT_EQ(replacement->ExecCount.getValue(), 0);
    // The old seven-field aggregate syntax remains source compatible. A manually
    // assembled request binds at first admission, rather than at declaration.
    App::RecomputeRequest manual {_docName, "Original", false, 0, false, {}, {}};
    auto result = app.recomputeRequestSynchronously(manual);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(replacement->ExecCount.getValue(), 1);
    auto incoherent = App::RecomputeRequest::fromDocumentObject(*replacement);
    incoherent.documentObjectName = "DifferentTarget";
    EXPECT_EQ(app.recomputeRequestSynchronously(incoherent).failure, App::RecomputeFailure::Exception);
}

TEST_F(AsyncRecomputeTest, Spec012CoalescedCancellationRetainsNoCallbackHandles)
{
    auto& app = App::GetApplication();
    const auto name = app.getUniqueDocumentName("coalesced_cancel_target");
    auto* document = app.newDocument(name.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(name.c_str());
    };
    auto* target = document->addObject<App::Spec012OutcomeFeature>("Target");
    target->touch();
    Spec012QueueBlocker gate(*_doc);
    std::vector<std::shared_ptr<App::RecomputeProgressHandle>> handles;
    std::vector<std::shared_ptr<App::RecomputeDetail::Retirement>> tickets;
    auto first = std::make_shared<Spec012Subscriber>();
    auto third = std::make_shared<Spec012Subscriber>();
    for (int index = 0; index != 3; ++index) {
        auto request = App::RecomputeRequest::fromDocumentObject(*target);
        handles.push_back(std::make_shared<App::RecomputeProgressHandle>());
        tickets.push_back(std::make_shared<App::RecomputeDetail::Retirement>());
        request.progress = handles.back();
        if (index != 1) {
            request.callback
                = spec012Subscriber("Q08", "coalesced_handle", "Target", index == 0 ? first : third);
        }
        app.queueRecomputeRequestWithRetirement(std::move(request), tickets.back());
    }
    const auto canceled = app.cancelRecomputeRequest(
        App::RecomputeRequest::fromDocumentObject(*target)
    );
    EXPECT_TRUE(canceled.removedQueued);
    EXPECT_FALSE(canceled.canceledInProgress);
    ASSERT_EQ(first->done.wait_for(2s), std::future_status::ready);
    ASSERT_EQ(third->done.wait_for(2s), std::future_status::ready);
    for (const auto& handle : handles) {
        EXPECT_TRUE(handle->wasCanceled());
    }
    for (const auto& ticket : tickets) {
        EXPECT_TRUE(ticket->retired());
    }
    EXPECT_EQ(target->ExecCount.getValue(), 0);
    gate.release();
    spec012Queue(
        "Q08",
        "post_cancel_control",
        App::RecomputeRequest::fromDocumentObject(*target),
        *document,
        target
    );
    EXPECT_EQ(first->count.load(), 1);
    EXPECT_EQ(third->count.load(), 1);
    EXPECT_EQ(target->ExecCount.getValue(), 1);
}

TEST_F(AsyncRecomputeTest, Spec012AcceptedQueuedCancellationSurvivesActivation)
{
    auto& app = App::GetApplication();
    const auto name = app.getUniqueDocumentName("activation_cancel_target");
    auto* document = app.newDocument(name.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(name.c_str());
    };
    auto* target = document->addObject<App::Spec012OutcomeFeature>("Target");
    target->touch();
    Spec012QueueBlocker gate(*_doc);
    auto handle = std::make_shared<App::RecomputeProgressHandle>();
    auto terminal = std::make_shared<Spec012Subscriber>();
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    request.progress = handle;
    request.callback = spec012Subscriber("Q08", "accepted_then_canceled", "Target", terminal);
    auto retirement = std::make_shared<App::RecomputeDetail::Retirement>();
    app.queueRecomputeRequestWithRetirement(std::move(request), retirement);
    // The queue has accepted this registration; the native gate proves that its
    // activation cannot start yet. Cancel the original handle without removing it.
    handle->cancel();
    gate.release();
    ASSERT_EQ(terminal->done.wait_for(2s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(retirement));
    EXPECT_EQ(terminal->failure.load(), App::RecomputeFailure::Canceled);
    EXPECT_TRUE(handle->wasCanceled());
    EXPECT_EQ(target->ExecCount.getValue(), 0);
    auto retry = App::RecomputeRequest::fromDocumentObject(*target);
    retry.progress = handle;
    EXPECT_TRUE(app.recomputeRequestSynchronously(retry).success);
    EXPECT_FALSE(handle->wasCanceled());
    EXPECT_EQ(target->ExecCount.getValue(), 1);
}

TEST_F(AsyncRecomputeTest, Spec012FreshRerunAfterCancelUsesFreshHandle)
{
    auto& app = App::GetApplication();
    auto* target = _doc->addObject<App::FeatureTestAsyncBlocker>("FreshRerun");
    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };
    target->touch();
    auto oldHandle = std::make_shared<App::RecomputeProgressHandle>();
    auto freshHandle = std::make_shared<App::RecomputeProgressHandle>();
    auto first = std::make_shared<Spec012Subscriber>();
    auto later = std::make_shared<Spec012Subscriber>();
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    request.progress = oldHandle;
    request.callback = spec012Subscriber("Q08", "old_active", "FreshRerun", first);
    app.queueRecomputeRequest(std::move(request));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
    EXPECT_TRUE(
        app.cancelRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*target)).canceledInProgress
    );
    auto rerun = App::RecomputeRequest::fromDocumentObject(*target);
    rerun.progress = freshHandle;
    rerun.callback = spec012Subscriber("Q08", "fresh_rerun", "FreshRerun", later);
    target->touch();
    app.queueRecomputeRequest(std::move(rerun));
    App::FeatureTestAsyncBlocker::releaseBlocker();
    ASSERT_EQ(first->done.wait_for(2s), std::future_status::ready);
    ASSERT_EQ(later->done.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(first->failure.load(), App::RecomputeFailure::Canceled);
    EXPECT_TRUE(later->success.load());
    EXPECT_TRUE(oldHandle->wasCanceled());
    EXPECT_FALSE(freshHandle->wasCanceled());
    EXPECT_EQ(first->count.load(), 1);
    EXPECT_EQ(later->count.load(), 1);
}

TEST_F(AsyncRecomputeTest, Spec012RetirementIncludesCapturedDestructorUnderNativePins)
{
    auto& app = App::GetApplication();
    auto* target = _doc->addObject<App::Spec012OutcomeFeature>("RetirementTarget");
    target->touch();
    auto ticket = std::make_shared<App::RecomputeDetail::Retirement>();
    auto terminal = std::make_shared<Spec012Subscriber>();
    std::promise<void> destroyed;
    auto destruction = destroyed.get_future();
    struct Capture
    {
        std::function<void()> onDestroy;
        ~Capture()
        {
            onDestroy();
        }
    };
    auto captured = std::make_shared<Capture>();
    captured->onDestroy = [&] {
        EXPECT_FALSE(ticket->retired());
        EXPECT_FALSE(app.closeDocument(_docName.c_str()));
        EXPECT_FALSE(App::RecomputeDetail::Coordinator::instance().tryAtomic().has_value());
        EXPECT_THROW(
            App::RecomputeDetail::Coordinator::instance().ensureWaitAllowed(),
            App::RecomputeDetail::AdmissionRefusal
        );
        destroyed.set_value();
    };
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    request.callback = [captured, terminal](App::RecomputeRequest& req, App::RecomputeResult& result) {
        spec012Subscriber("Q11", "retirement_callback", "RetirementTarget", terminal)(req, result);
    };
    captured.reset();
    app.queueRecomputeRequestWithRetirement(std::move(request), ticket);
    ASSERT_EQ(terminal->done.wait_for(2s), std::future_status::ready);
    ASSERT_EQ(destruction.wait_for(2s), std::future_status::ready);
    // A later native callback proves the earlier complete terminal phase drained.
    auto* successor = _doc->addObject<App::Spec012OutcomeFeature>("RetirementSuccessor");
    spec012Queue(
        "Q11",
        "retirement_sentinel",
        App::RecomputeRequest::fromDocumentObject(*successor),
        *_doc,
        successor
    );
    EXPECT_TRUE(ticket->retired());
}

TEST_F(AsyncRecomputeTest, Spec012PropertyWritersRefuseBeforeNativeEffects)
{
    auto& application = App::GetApplication();
    const auto name = application.getUniqueDocumentName("spec012_writer_source");
    auto* document = application.newDocument(name.c_str());
    const auto directory = std::filesystem::temp_directory_path()
        / ("spec012-writer-"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    BOOST_SCOPE_EXIT_ALL(&)
    {
        application.closeDocument(name.c_str());
        std::filesystem::remove_all(directory);
    };
    auto* source = document->addObject<App::FeatureTest>("Source");
    auto* destination = document->addObject<App::FeatureTest>("Destination");
    auto* safe = document->addObject<App::FeatureTest>("Safe");
    auto* pinned = _doc->addObject<App::FeatureTestAsyncBlocker>("PinnedDependency");
    auto* link = dynamic_cast<App::PropertyLink*>(
        source->addDynamicProperty("App::PropertyLink", "GuardedDynamic")
    );
    ASSERT_TRUE(link);
    link->setAllowExternal(true);
    link->setValue(pinned);
    auto* links = dynamic_cast<App::PropertyLinkList*>(
        source->addDynamicProperty("App::PropertyLinkList", "GuardedList")
    );
    ASSERT_TRUE(links);
    links->setAllowExternal(true);
    links->setValues({safe, safe});
    auto* hidden = dynamic_cast<App::PropertyLink*>(
        source->addDynamicProperty("App::PropertyLinkHidden", "GuardedHidden")
    );
    ASSERT_TRUE(hidden);
    hidden->setAllowExternal(true);
    auto* bulk = dynamic_cast<App::PropertyLinkSubList*>(
        source->addDynamicProperty("App::PropertyLinkSubList", "GuardedBulk")
    );
    ASSERT_TRUE(bulk);
    bulk->setAllowExternal(true);
    bulk->setValues(std::vector<App::DocumentObject*> {safe, safe}, std::vector<std::string> {"", ""});
    auto* control = dynamic_cast<App::PropertyLink*>(
        destination->addDynamicProperty("App::PropertyLink", "DisjointControl")
    );
    ASSERT_TRUE(control);
    control->setValue(safe);
    // Native cross-document expressions require both real document filenames.
    // Save the private fixtures before entering any active worker lease.
    ASSERT_TRUE(document->saveAs((directory / "source.FCStd").string().c_str()));
    ASSERT_TRUE(_doc->saveAs((directory / "target.FCStd").string().c_str()));
    const App::ObjectIdentifier path(source->String);
    const auto expressionText = std::string(_doc->getName()) + "#PinnedDependency.Label";
    const auto originalLabel = std::string(pinned->Label.getValue());
    int beforeChanges = 0;
    fastsignals::scoped_connection before(document->signalBeforeChangeObject.connect(
        [&](const App::DocumentObject& object, const App::Property& property) {
            if (&object == source
                && (&property == &source->ExpressionEngine || &property == links
                    || &property == link || &property == hidden || &property == bulk)) {
                ++beforeChanges;
            }
        }
    ));
    {
        App::FeatureTestAsyncBlocker::resetBlocker();
        pinned->touch();
        auto terminal = std::make_shared<Spec012Subscriber>();
        auto request = App::RecomputeRequest::fromDocumentObject(*pinned);
        request.callback = [terminal](App::RecomputeRequest&, App::RecomputeResult&) {
            if (++terminal->count == 1) {
                terminal->completed.set_value();
            }
        };
        application.queueRecomputeRequest(std::move(request));
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::FeatureTestAsyncBlocker::releaseBlocker();
        };
        ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
        const auto oldBacklinks = pinned->getInList();
        EXPECT_THROW(link->setValue(nullptr), App::RecomputeDetail::AdmissionRefusal);
        EXPECT_THROW(link->setValue(safe), App::RecomputeDetail::AdmissionRefusal);
        EXPECT_EQ(link->getValue(), pinned);
        EXPECT_EQ(pinned->getInList(), oldBacklinks);
        EXPECT_THROW(hidden->setValue(pinned), App::RecomputeDetail::AdmissionRefusal);
        EXPECT_EQ(hidden->getValue(), nullptr);
        EXPECT_EQ(pinned->getInList(), oldBacklinks);
        EXPECT_THROW(
            bulk->setValues(
                std::vector<App::DocumentObject*> {destination, pinned},
                std::vector<std::string> {"", ""}
            ),
            App::RecomputeDetail::AdmissionRefusal
        );
        EXPECT_EQ(bulk->getValues(), std::vector<App::DocumentObject*>({safe, safe}));
        EXPECT_EQ(bulk->getSubValues(), std::vector<std::string>({"", ""}));
        EXPECT_EQ(beforeChanges, 0);
        EXPECT_NO_THROW(control->setValue(source));
        EXPECT_EQ(control->getValue(), source);
        EXPECT_THROW(
            source->ExpressionEngine.setValue(
                path,
                std::shared_ptr<App::Expression>(App::Expression::parse(source, expressionText))
            ),
            App::RecomputeDetail::AdmissionRefusal
        );
        EXPECT_TRUE(source->ExpressionEngine.getExpressions().empty());
        EXPECT_EQ(beforeChanges, 0);
        EXPECT_THROW(
            pinned->Label.setValue("Spec012 busy label"),
            App::RecomputeDetail::AdmissionRefusal
        );
        EXPECT_EQ(std::string(pinned->Label.getValue()), originalLabel);
        EXPECT_FALSE(source->removeDynamicProperty("GuardedDynamic"));
        EXPECT_EQ(source->getPropertyByName("GuardedDynamic"), link);
        EXPECT_EQ(link->getValue(), pinned);
        EXPECT_THROW(
            source->moveDynamicProperty(link, destination),
            App::RecomputeDetail::AdmissionRefusal
        );
        EXPECT_EQ(source->getPropertyByName("GuardedDynamic"), link);
        EXPECT_FALSE(destination->getPropertyByName("GuardedDynamic"));
        // Decode both dictionary values before touching index0. Old/new union
        // includes pinned even though index0's new target is disjoint.
        {
            Base::PyGILStateLocker gil;
            Py::Dict changes;
            changes[Py::Long(0)] = Py::asObject(destination->getPyObject());
            changes[Py::Long(1)] = Py::asObject(pinned->getPyObject());
            EXPECT_THROW(links->setPyObject(changes.ptr()), App::RecomputeDetail::AdmissionRefusal);
            EXPECT_EQ(links->getValues(), std::vector<App::DocumentObject*>({safe, safe}));
            EXPECT_EQ(beforeChanges, 0);
        }
        App::FeatureTestAsyncBlocker::releaseBlocker();
        EXPECT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
        // The callback still owns native pins. Queue a disjoint successor and
        // await it before testing post-release writes or fixture destruction.
        auto after = std::make_shared<Spec012Subscriber>();
        auto successor = App::RecomputeRequest::fromDocumentObject(*safe);
        successor.callback = [after](App::RecomputeRequest&, App::RecomputeResult&) {
            if (++after->count == 1) {
                after->completed.set_value();
            }
        };
        auto successorRetirement = std::make_shared<App::RecomputeDetail::Retirement>();
        application.queueRecomputeRequestWithRetirement(std::move(successor), successorRetirement);
        EXPECT_EQ(after->done.wait_for(10s), std::future_status::ready);
        EXPECT_TRUE(spec012WaitRetired(successorRetirement));
    }
    // Same native entrypoints succeed after release; Busy is not sticky.
    EXPECT_NO_THROW(source->ExpressionEngine.setValue(
        path,
        std::shared_ptr<App::Expression>(App::Expression::parse(source, expressionText))
    ));
    EXPECT_EQ(source->ExpressionEngine.getExpressions().size(), 1U);
    EXPECT_NO_THROW(pinned->Label.setValue("Spec012 released label"));
    EXPECT_EQ(std::string(pinned->Label.getValue()), "Spec012 released label");
    EXPECT_NO_THROW(link->setValue(nullptr));
    EXPECT_NO_THROW(link->setValue(safe));
    EXPECT_NO_THROW(hidden->setValue(pinned));
    EXPECT_NO_THROW(bulk->setValues(
        std::vector<App::DocumentObject*> {destination, pinned},
        std::vector<std::string> {"", ""}
    ));
    EXPECT_TRUE(source->removeDynamicProperty("GuardedDynamic"));
    EXPECT_FALSE(source->getPropertyByName("GuardedDynamic"));
}

TEST_F(AsyncRecomputeTest, Spec012OpaqueUndoBusyHasNoTransactionEffects)
{
    auto& app = App::GetApplication();
    auto* feature = _doc->addObject<App::Spec012OutcomeFeature>("UndoCandidate");
    ASSERT_NE(feature, nullptr);
    _doc->recompute({}, true);
    _doc->setMaxUndoStackSize(10);
    const int before = feature->Integer.getValue();
    const int id = _doc->openTransaction("Undo admission");
    feature->Integer.setValue(before + 1);
    _doc->commitTransaction();
    const int available = _doc->getAvailableUndos();

    const auto blockerName = app.getUniqueDocumentName("transaction_blocker");
    auto* blockerDoc = app.newDocument(blockerName.c_str());
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        blockerDoc->addObject("App::FeatureTestAsyncBlocker", "Blocker")
    );
    ASSERT_NE(blocker, nullptr);
    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
        app.closeDocument(blockerName.c_str());
    };
    blocker->touch();
    app.queueRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*blocker));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    EXPECT_FALSE(_doc->undo(id));
    EXPECT_EQ(feature->Integer.getValue(), before + 1);
    EXPECT_EQ(_doc->getAvailableUndos(), available);
    EXPECT_EQ(_doc->getAvailableRedos(), 0);
    EXPECT_EQ(_doc->getTransactionID(true), id);
    EXPECT_EQ(_doc->getBookedTransactionID(), App::NullTransaction);
}

TEST_F(AsyncRecomputeTest, Spec012AutoTransactionBusyAbortTransfersExactMode)
{
    auto& app = App::GetApplication();
    auto* feature = _doc->addObject<App::Spec012OutcomeFeature>("AbortCandidate");
    ASSERT_NE(feature, nullptr);
    _doc->recompute({}, true);
    _doc->setMaxUndoStackSize(10);
    const int before = feature->Integer.getValue();
    const int id = _doc->openTransaction("Deferred abort");
    feature->Integer.setValue(before + 1);

    const auto blockerName = app.getUniqueDocumentName("abort_blocker");
    auto* blockerDoc = app.newDocument(blockerName.c_str());
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        blockerDoc->addObject("App::FeatureTestAsyncBlocker", "Blocker")
    );
    ASSERT_NE(blocker, nullptr);
    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
        app.closeDocument(blockerName.c_str());
    };
    blocker->touch();
    app.queueRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*blocker));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
    std::promise<void> aborted;
    auto abortedFuture = aborted.get_future();
    fastsignals::scoped_connection abortObserver(
        _doc->signalAbortTransaction.connect([&](const App::Document&) { aborted.set_value(); })
    );
    {
        App::AutoTransaction owner(id);
        EXPECT_FALSE(owner.tryClose(App::TransactionCloseMode::Abort));
        EXPECT_TRUE(app.transactionIsActive(id));
        EXPECT_EQ(_doc->getBookedTransactionID(), id);
        EXPECT_EQ(feature->Integer.getValue(), before + 1);
        // Destructor must preserve requested Abort, not revert to implicit Commit.
    }
    EXPECT_TRUE(app.transactionIsActive(id));
    EXPECT_EQ(abortedFuture.wait_for(0ms), std::future_status::timeout);
    App::FeatureTestAsyncBlocker::releaseBlocker();
    // No explicit drain or rescue request: completion must autonomously settle it.
    ASSERT_EQ(abortedFuture.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(feature->Integer.getValue(), before);
    EXPECT_FALSE(_doc->hasPendingTransaction());
}

TEST_F(AsyncRecomputeTest, Spec012AutoTransactionNeverClosesReusedTransactionId)
{
    auto& app = App::GetApplication();
    auto* feature = _doc->addObject<App::Spec012OutcomeFeature>("GenerationCandidate");
    ASSERT_NE(feature, nullptr);
    _doc->recompute({}, true);
    _doc->setMaxUndoStackSize(10);
    const int id = _doc->openTransaction("Original generation");
    feature->Integer.setValue(17);
    App::AutoTransaction oldOwner(id);
    ASSERT_TRUE(app.abortTransaction(id));
    app.setTransactionDescription(
        id,
        App::TransactionDescription {
            .initiator = _doc,
            .name = App::TransactionName {.name = "Replacement generation"}
        }
    );
    ASSERT_EQ(_doc->openTransaction("Replacement generation", id), id);
    feature->Integer.setValue(23);
    EXPECT_TRUE(oldOwner.tryClose(App::TransactionCloseMode::Abort));
    EXPECT_TRUE(app.transactionIsActive(id));
    EXPECT_TRUE(_doc->hasPendingTransaction());
    EXPECT_EQ(feature->Integer.getValue(), 23);
    EXPECT_TRUE(app.abortTransaction(id));
}

TEST_F(AsyncRecomputeTest, Spec012UndoObserverQueuedRequestExecutesAfterAtomicScope)
{
    auto& app = App::GetApplication();
    auto* feature = _doc->addObject<App::Spec012OutcomeFeature>("UndoSource");
    ASSERT_NE(feature, nullptr);
    _doc->recompute({}, true);
    _doc->setMaxUndoStackSize(10);
    _doc->openTransaction("Observer scheduling");
    feature->Integer.setValue(17);
    _doc->commitTransaction();
    const auto targetName = app.getUniqueDocumentName("undo_observer_target");
    auto* targetDoc = app.newDocument(targetName.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(targetName.c_str());
    };
    auto* target = targetDoc->addObject<App::Spec012OutcomeFeature>("QueuedTarget");
    ASSERT_NE(target, nullptr);
    target->touch();
    std::promise<bool> completed;
    auto completion = completed.get_future();
    bool submitted = false;
    fastsignals::scoped_connection observer(_doc->signalChangedObject.connect(
        [&](const App::DocumentObject& changed, const App::Property&) {
            if (&changed != feature || !_doc->isPerformingTransaction() || submitted) {
                return;
            }
            submitted = true;
            auto request = App::RecomputeRequest::fromDocumentObject(*target);
            request.callback = [&](App::RecomputeRequest&, App::RecomputeResult& result) {
                completed.set_value(result.success);
            };
            app.queueRecomputeRequest(std::move(request));
            EXPECT_EQ(completion.wait_for(0ms), std::future_status::timeout);
            EXPECT_THROW(
                App::RecomputeDetail::Coordinator::instance().ensureWaitAllowed(),
                App::RecomputeDetail::AdmissionRefusal
            );
        }
    ));
    ASSERT_TRUE(_doc->undo());
    ASSERT_TRUE(submitted);
    ASSERT_EQ(completion.wait_for(2s), std::future_status::ready);
    EXPECT_TRUE(completion.get());
    EXPECT_EQ(target->ExecCount.getValue(), 1);
}


TEST_F(AsyncRecomputeTest, Spec012ThrowingCallbacksDoNotDropLaterSubscribers)
{
    auto& app = App::GetApplication();
    const auto name = app.getUniqueDocumentName("throwing_subscribers");
    auto* document = app.newDocument(name.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(name.c_str());
    };
    auto* target = document->addObject<App::Spec012OutcomeFeature>("Target");
    target->touch();
    Spec012QueueBlocker gate(*_doc);
    std::vector<std::shared_ptr<Spec012Subscriber>> terminals;
    std::vector<std::shared_ptr<App::RecomputeDetail::Retirement>> retirements;
    for (int index = 0; index != 4; ++index) {
        terminals.push_back(std::make_shared<Spec012Subscriber>());
        retirements.push_back(std::make_shared<App::RecomputeDetail::Retirement>());
        auto request = App::RecomputeRequest::fromDocumentObject(*target);
        request.callback =
            [index,
             state = terminals.back()](App::RecomputeRequest& req, App::RecomputeResult& result) {
                spec012Subscriber("Q10", "exception_isolation", "Target", state)(req, result);
                if (index == 0) {
                    throw std::runtime_error("standard subscriber failure");
                }
                if (index == 1) {
                    throw Base::RuntimeError("native subscriber failure");
                }
                if (index == 2) {
                    throw 17;
                }
            };
        app.queueRecomputeRequestWithRetirement(std::move(request), retirements.back());
    }
    gate.release();
    for (const auto& state : terminals) {
        ASSERT_EQ(state->done.wait_for(2s), std::future_status::ready);
        EXPECT_EQ(state->count.load(), 1);
        EXPECT_TRUE(state->success.load());
    }
    for (const auto& retirement : retirements) {
        ASSERT_TRUE(spec012WaitRetired(retirement));
    }
    EXPECT_EQ(target->ExecCount.getValue(), 1);
    target->touch();
    spec012Queue(
        "Q10",
        "after_throw_control",
        App::RecomputeRequest::fromDocumentObject(*target),
        *document,
        target
    );
    EXPECT_EQ(target->ExecCount.getValue(), 2);
}


TEST_F(AsyncRecomputeTest, Spec012TransactionSignallerRestoresStateAfterObserverRefusal)
{
    auto& app = App::GetApplication();
    int beforeCount = 0;
    int afterCount = 0;
    bool refuseBefore = true;
    bool refuseAfter = false;
    fastsignals::scoped_connection before(app.signalBeforeCloseTransaction.connect([&](bool) {
        ++beforeCount;
        if (refuseBefore) {
            throw App::RecomputeDetail::AdmissionRefusal("before-close observer refusal");
        }
    }));
    fastsignals::scoped_connection after(app.signalCloseTransaction.connect([&](bool) {
        ++afterCount;
        if (refuseAfter) {
            throw App::RecomputeDetail::AdmissionRefusal("after-close observer refusal");
        }
    }));
    EXPECT_THROW(
        (Spec012ApplicationAccess::TransactionSignaller(false, true)),
        App::RecomputeDetail::AdmissionRefusal
    );
    EXPECT_EQ(beforeCount, 1);
    EXPECT_EQ(afterCount, 0);
    refuseBefore = false;
    refuseAfter = true;
    {
        Spec012ApplicationAccess::TransactionSignaller next(false, true);
    }
    EXPECT_EQ(beforeCount, 2);
    EXPECT_EQ(afterCount, 1);
    refuseAfter = false;
    {
        Spec012ApplicationAccess::TransactionSignaller next(false, true);
    }
    EXPECT_EQ(beforeCount, 3);
    EXPECT_EQ(afterCount, 2);
    {
        Spec012ApplicationAccess::TransactionSignaller refused(false, true);
        refused.suppressCompletion();
    }
    EXPECT_EQ(beforeCount, 4);
    EXPECT_EQ(afterCount, 2);
    {
        Spec012ApplicationAccess::TransactionSignaller retried(false, true);
    }
    EXPECT_EQ(beforeCount, 5);
    EXPECT_EQ(afterCount, 3);
}


TEST_F(AsyncRecomputeTest, Spec012ClosingBorrowedDocumentCancelsRootOperation)
{
    auto& app = App::GetApplication();
    for (bool beforeCapture : {false, true}) {
        const auto dependencyName = app.getUniqueDocumentName("borrowed_close_dependency");
        auto* dependencyDoc = app.newDocument(dependencyName.c_str());
        const auto directory = std::filesystem::temp_directory_path()
            / ("spec012-close-"
               + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(directory);
        BOOST_SCOPE_EXIT_ALL(&)
        {
            app.closeDocument(dependencyName.c_str());
            std::filesystem::remove_all(directory);
        };
        auto* dependency = dependencyDoc->addObject<App::FeatureTestAsyncBlocker>("Dependency");
        auto* root = _doc->addObject<App::Spec012OutcomeFeature>("BorrowingRoot");
        auto* external = dynamic_cast<App::PropertyXLink*>(
            root->addDynamicProperty("App::PropertyXLink", "ExternalDependency")
        );
        ASSERT_NE(external, nullptr);
        // Real cross-document links require saved owner and dependency documents.
        ASSERT_TRUE(_doc->saveAs((directory / "source.FCStd").string().c_str()));
        ASSERT_TRUE(dependencyDoc->saveAs((directory / "dependency.FCStd").string().c_str()));
        external->setValue(dependency);
        dependency->touch();
        root->touch();
        App::FeatureTestAsyncBlocker::resetBlocker();
        std::promise<void> beforeEntered;
        auto beforeStarted = beforeEntered.get_future();
        std::promise<void> releaseBefore;
        auto beforeProceed = releaseBefore.get_future().share();
        std::atomic<bool> entered {false};
        fastsignals::scoped_connection before(
            _doc->signalBeforeRecompute.connect([&](const App::Document&) {
                if (beforeCapture && !entered.exchange(true)) {
                    // Root A has not captured B yet, but closing B would mutate
                    // the incoming source that this very operation already owns.
                    EXPECT_FALSE(app.closeDocument(dependencyName.c_str()));
                    EXPECT_EQ(app.getDocument(dependencyName.c_str()), dependencyDoc);
                    EXPECT_EQ(external->getValue(), dependency);
                    beforeEntered.set_value();
                    beforeProceed.wait();
                }
            })
        );
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::FeatureTestAsyncBlocker::releaseBlocker();
            if (beforeCapture) {
                try {
                    releaseBefore.set_value();
                }
                catch (const std::future_error&) {
                }
            }
        };
        auto handle = std::make_shared<App::RecomputeProgressHandle>();
        auto terminal = std::make_shared<Spec012Subscriber>();
        auto retirement = std::make_shared<App::RecomputeDetail::Retirement>();
        auto request = App::RecomputeRequest::fromDocumentObject(*root, true);
        request.progress = handle;
        request.callback = spec012Subscriber("Q11", "borrowed_close_root", "BorrowingRoot", terminal);
        app.queueRecomputeRequestWithRetirement(std::move(request), retirement);
        if (beforeCapture) {
            ASSERT_EQ(beforeStarted.wait_for(10s), std::future_status::ready);
        }
        else {
            ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
        }
        auto closing = std::async(std::launch::async, [&] {
            return app.closeDocument(dependencyName.c_str());
        });
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (!handle->wasCanceled() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        const bool canceled = handle->wasCanceled();
        App::FeatureTestAsyncBlocker::releaseBlocker();
        if (beforeCapture) {
            releaseBefore.set_value();
        }
        EXPECT_TRUE(canceled);
        ASSERT_EQ(closing.wait_for(10s), std::future_status::ready);
        EXPECT_TRUE(closing.get());
        ASSERT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
        ASSERT_TRUE(spec012WaitRetired(retirement));
        EXPECT_EQ(terminal->count.load(), 1);
        EXPECT_EQ(terminal->failure.load(), App::RecomputeFailure::Canceled);
        EXPECT_EQ(app.getDocument(dependencyName.c_str()), nullptr);
        EXPECT_EQ(root->ExecCount.getValue(), 0);
        before.disconnect();
        _doc->removeObject(root->getNameInDocument());
        auto* successor = _doc->addObject<App::Spec012OutcomeFeature>("AfterBorrowedClose");
        successor->touch();
        spec012Queue(
            "Q11",
            "borrowed_close_later_progress",
            App::RecomputeRequest::fromDocumentObject(*successor),
            *_doc,
            successor
        );
        _doc->removeObject(successor->getNameInDocument());
    }
}


TEST_F(AsyncRecomputeTest, Spec012ClosingBorrowedDocumentCancelsForeignAppSync)
{
    auto& app = App::GetApplication();
    const auto name = app.getUniqueDocumentName("sync_borrowed_dependency");
    auto* dependencyDoc = app.newDocument(name.c_str());
    const auto directory = std::filesystem::temp_directory_path()
        / ("spec012-close-"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(name.c_str());
        std::filesystem::remove_all(directory);
    };
    auto* dependency = dependencyDoc->addObject<App::FeatureTestAsyncBlocker>("Dependency");
    auto* root = _doc->addObject<App::Spec012OutcomeFeature>("SyncBorrower");
    auto* external = dynamic_cast<App::PropertyXLink*>(
        root->addDynamicProperty("App::PropertyXLink", "ExternalDependency")
    );
    ASSERT_NE(external, nullptr);
    // Real cross-document links require saved owner and dependency documents.
    ASSERT_TRUE(_doc->saveAs((directory / "source.FCStd").string().c_str()));
    ASSERT_TRUE(dependencyDoc->saveAs((directory / "dependency.FCStd").string().c_str()));
    external->setValue(dependency);
    dependency->touch();
    root->touch();
    App::FeatureTestAsyncBlocker::resetBlocker();
    spec012WakeBoundary = 0;
    auto handle = std::make_shared<App::RecomputeProgressHandle>();
    auto request = App::RecomputeRequest::fromDocumentObject(*root, true);
    request.progress = handle;
    auto running = std::async(std::launch::async, [request = std::move(request)]() mutable {
        return App::GetApplication().recomputeRequestSynchronously(std::move(request));
    });
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
    auto closing = std::async(std::launch::async, [&] { return app.closeDocument(name.c_str()); });
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (!handle->wasCanceled() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    const bool canceled = handle->wasCanceled();
    if (const char* seam = std::getenv("SPEC012_WAKE_SEAM"); seam && std::string(seam) == "close") {
        const auto boundaryDeadline = std::chrono::steady_clock::now() + 10s;
        while (spec012WakeBoundary == 0 && std::chrono::steady_clock::now() < boundaryDeadline) {
            std::this_thread::yield();
        }
        EXPECT_EQ(spec012WakeBoundary.load(), 1);
    }
    App::FeatureTestAsyncBlocker::releaseBlocker();
    EXPECT_TRUE(canceled);
    ASSERT_EQ(running.wait_for(10s), std::future_status::ready);
    EXPECT_EQ(running.get().failure, App::RecomputeFailure::Canceled);
    ASSERT_EQ(closing.wait_for(10s), std::future_status::ready);
    EXPECT_TRUE(closing.get());
    EXPECT_EQ(app.getDocument(name.c_str()), nullptr);
    EXPECT_EQ(root->ExecCount.getValue(), 0);
    auto* control = _doc->addObject<App::Spec012OutcomeFeature>("AfterSyncBorrowedClose");
    control->touch();
    spec012Queue(
        "Q11",
        "foreign_sync_close_later_queue_control",
        App::RecomputeRequest::fromDocumentObject(*control),
        *_doc,
        control
    );
}


TEST_F(AsyncRecomputeTest, Spec012StandaloneProgressScopeMirrorsNativeSequencerCancel)
{
    Spec012CancellationSequencer sequencer;
    App::RecomputeProgressHandle handle;
    EXPECT_EQ(App::currentRecomputeProgress(), nullptr);
    {
        auto local = handle.makeScope("Local progress without App TLS");
        sequencer.tryToCancel();
        EXPECT_FALSE(handle.wasCanceled());
        EXPECT_TRUE(local.wasCanceled());
        EXPECT_TRUE(handle.wasCanceled());
        auto foreign = std::async(std::launch::async, [&] { return handle.wasCanceled(); });
        EXPECT_TRUE(foreign.get());
    }
    sequencer.rejectCancel();
    handle.activate();
    EXPECT_FALSE(handle.wasCanceled());
}

TEST_F(AsyncRecomputeTest, Spec012ForeignProgressPollDuringAcceptedActivation)
{
    App::RecomputeProgressHandle handle;
    auto legacyActivate = &App::RecomputeProgressHandle::activate;
    (handle.*legacyActivate)();
    handle.setText("Prior display");
    std::promise<void> entered;
    auto inside = entered.get_future();
    std::promise<void> release;
    auto proceed = release.get_future().share();
    handle.setDisplayObserver([&](const App::RecomputeProgressDisplayState& state) {
        if (state.text.empty()) {
            entered.set_value();
            proceed.wait();
        }
    });
    auto activation = std::async(std::launch::async, [&] { handle.activateAccepted(); });
    BOOST_SCOPE_EXIT_ALL(&)
    {
        try {
            release.set_value();
        }
        catch (const std::future_error&) {
        }
    };
    ASSERT_EQ(inside.wait_for(10s), std::future_status::ready);
    EXPECT_FALSE(handle.wasCanceled());
    release.set_value();
    // Poll while the owner resets/recreates its sequencer. Foreign polling touches
    // only the atomic publication, never the concurrently changing unique_ptr.
    while (activation.wait_for(0ms) != std::future_status::ready) {
        EXPECT_FALSE(handle.wasCanceled());
        std::this_thread::yield();
    }
    activation.get();
    handle.setDisplayObserver({});
    handle.cancel();
    EXPECT_TRUE(handle.wasCanceled());
    handle.activate();
    EXPECT_FALSE(handle.wasCanceled());
}

TEST_F(AsyncRecomputeTest, Spec012PostNativeCommitObserverFailureFinishesWholeCohort)
{
    auto& app = App::GetApplication();
    const auto otherName = app.getUniqueDocumentName("commit_cohort_other");
    auto* other = app.newDocument(otherName.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(otherName.c_str());
    };
    auto* first = _doc->addObject<App::Spec012OutcomeFeature>("First");
    auto* second = other->addObject<App::Spec012OutcomeFeature>("Second");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    _doc->recompute({}, true);
    other->recompute({}, true);
    _doc->setMaxUndoStackSize(10);
    other->setMaxUndoStackSize(10);
    const int beforeFirst = first->Integer.getValue();
    const int beforeSecond = second->Integer.getValue();
    const int id = _doc->openTransaction("Two-document commit");
    ASSERT_EQ(other->openTransaction("Two-document commit", id), id);
    first->Integer.setValue(beforeFirst + 1);
    second->Integer.setValue(beforeSecond + 1);
    int firstPost = 0;
    int secondPost = 0;
    fastsignals::scoped_connection firstObserver(
        _doc->signalCommitTransaction.connect([&](const App::Document&) {
            ++firstPost;
            // This exact document's history has completed before notification.
            EXPECT_FALSE(_doc->hasPendingTransaction());
            throw App::RecomputeDetail::AdmissionRefusal("independent postcommit observer refusal");
        })
    );
    fastsignals::scoped_connection secondObserver(
        other->signalCommitTransaction.connect([&](const App::Document&) { ++secondPost; })
    );
    bool committed = false;
    EXPECT_NO_THROW(committed = app.commitTransaction(id));
    EXPECT_TRUE(committed);
    EXPECT_EQ(firstPost, 1);
    EXPECT_EQ(secondPost, 1);
    EXPECT_FALSE(_doc->hasPendingTransaction());
    EXPECT_FALSE(other->hasPendingTransaction());
    EXPECT_FALSE(app.transactionIsActive(id));
    EXPECT_EQ(_doc->getBookedTransactionID(), App::NullTransaction);
    EXPECT_EQ(other->getBookedTransactionID(), App::NullTransaction);
    EXPECT_EQ(_doc->getTransactionID(true), id);
    EXPECT_EQ(other->getTransactionID(true), id);
    EXPECT_EQ(first->Integer.getValue(), beforeFirst + 1);
    EXPECT_EQ(second->Integer.getValue(), beforeSecond + 1);
    // The observer failure must not strand or duplicate either inverse record.
    EXPECT_TRUE(_doc->undo(id));
    EXPECT_TRUE(other->undo(id));
    EXPECT_EQ(first->Integer.getValue(), beforeFirst);
    EXPECT_EQ(second->Integer.getValue(), beforeSecond);
}

TEST_F(AsyncRecomputeTest, Spec012ForeignNativeExecutionConflictsAreDocumentScoped)
{
    auto& app = App::GetApplication();
    auto* active = _doc->addObject<App::FeatureTestAsyncBlocker>("ActiveMember");
    auto* sameDoc = _doc->addObject<App::Spec012OutcomeFeature>("DifferentMember");
    sameDoc->touch();
    const auto name = app.getUniqueDocumentName("disjoint_native_owner");
    auto* other = app.newDocument(name.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(name.c_str());
    };
    auto* disjoint = other->addObject<App::Spec012OutcomeFeature>("DisjointMember");
    disjoint->touch();
    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };
    active->touch();
    auto terminal = std::make_shared<Spec012Subscriber>();
    auto retirement = std::make_shared<App::RecomputeDetail::Retirement>();
    auto request = App::RecomputeRequest::fromDocumentObject(*active);
    request.callback = spec012Subscriber("Q06", "document_execution_owner", "ActiveMember", terminal);
    app.queueRecomputeRequestWithRetirement(std::move(request), retirement);
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
    for (auto* object :
         {static_cast<App::DocumentObject*>(active), static_cast<App::DocumentObject*>(sameDoc)}) {
        auto result = app.recomputeRequestSynchronously(
            App::RecomputeRequest::fromDocumentObject(*object)
        );
        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.failure, App::RecomputeFailure::Exception);
    }
    EXPECT_EQ(App::FeatureTestAsyncBlocker::getExecutionCount(), 1);
    EXPECT_EQ(sameDoc->ExecCount.getValue(), 0);
    auto control = app.recomputeRequestSynchronously(
        App::RecomputeRequest::fromDocumentObject(*disjoint)
    );
    EXPECT_TRUE(control.success);
    EXPECT_EQ(disjoint->ExecCount.getValue(), 1);
    App::FeatureTestAsyncBlocker::releaseBlocker();
    ASSERT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(retirement));
    EXPECT_TRUE(
        app.recomputeRequestSynchronously(App::RecomputeRequest::fromDocumentObject(*sameDoc)).success
    );
    EXPECT_EQ(sameDoc->ExecCount.getValue(), 1);
}


TEST_F(AsyncRecomputeTest, Spec012OwnerRoutePreflightDoesNotCheckWorkerPredicates)
{
    auto& app = App::GetApplication();
    auto* target = _doc->addObject<App::Spec012OutcomeFeature>("OwnerPreflight");
    auto request = App::RecomputeRequest::fromDocumentObject(*target, true);
    EXPECT_EQ(target->workerSafetyQueries.load(), 0);
    EXPECT_EQ(
        app.classifyRecomputeRequest(request, false),
        App::RecomputeDetail::RouteDisposition::OwnerReady
    );
    EXPECT_EQ(target->workerSafetyQueries.load(), 0);
    EXPECT_EQ(target->ExecCount.getValue(), 0);
    EXPECT_EQ(
        app.classifyRecomputeRequest(request),
        App::RecomputeDetail::RouteDisposition::WorkerEligible
    );
    EXPECT_GT(target->workerSafetyQueries.load(), 0);
    const int checked = target->workerSafetyQueries.load();
    target->workerAllowed = false;
    EXPECT_EQ(
        app.classifyRecomputeRequest(request, false),
        App::RecomputeDetail::RouteDisposition::OwnerReady
    );
    EXPECT_EQ(target->workerSafetyQueries.load(), checked);
    target->touch();
    EXPECT_TRUE(app.recomputeRequestSynchronously(request).success);
    EXPECT_EQ(target->ExecCount.getValue(), 1);
    EXPECT_EQ(target->workerEligibilityAtExecute, 0);
}


TEST_F(AsyncRecomputeTest, Spec012BorrowedCallbackCannotCloseDifferentRootDocument)
{
    auto& app = App::GetApplication();
    const auto name = app.getUniqueDocumentName("own_borrowed_close");
    auto* borrowed = app.newDocument(name.c_str());
    const auto directory = std::filesystem::temp_directory_path()
        / ("spec012-close-"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(name.c_str());
        std::filesystem::remove_all(directory);
    };
    auto* member = borrowed->addObject<App::Spec012OutcomeFeature>("BorrowedMember");
    auto* root = _doc->addObject<App::Spec012OutcomeFeature>("OwningRoot");
    auto* external = dynamic_cast<App::PropertyXLink*>(
        root->addDynamicProperty("App::PropertyXLink", "BorrowedDependency")
    );
    ASSERT_NE(external, nullptr);
    // Real cross-document links require saved owner and dependency documents.
    ASSERT_TRUE(_doc->saveAs((directory / "source.FCStd").string().c_str()));
    ASSERT_TRUE(borrowed->saveAs((directory / "dependency.FCStd").string().c_str()));
    external->setValue(member);
    member->touch();
    root->touch();
    auto ticket = std::make_shared<App::RecomputeDetail::Retirement>();
    std::promise<void> callbackDone;
    auto done = callbackDone.get_future();
    auto request = App::RecomputeRequest::fromDocumentObject(*root, true);
    request.callback = [&](App::RecomputeRequest&, App::RecomputeResult& result) {
        EXPECT_TRUE(result.success);
        EXPECT_FALSE(app.closeDocument(name.c_str()));
        EXPECT_EQ(app.getDocument(name.c_str()), borrowed);
        EXPECT_EQ(external->getValue(), member);
        EXPECT_EQ(member->ExecCount.getValue(), 1);
        callbackDone.set_value();
    };
    app.queueRecomputeRequestWithRetirement(std::move(request), ticket);
    ASSERT_EQ(done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(ticket));
    EXPECT_TRUE(app.closeDocument(name.c_str()));
    EXPECT_EQ(app.getDocument(name.c_str()), nullptr);
    EXPECT_EQ(external->getValue(), nullptr);
    root->touch();
    spec012Queue(
        "Q11",
        "after_own_borrowed_close_refusal",
        App::RecomputeRequest::fromDocumentObject(*root),
        *_doc,
        root
    );
}


TEST_F(AsyncRecomputeTest, Spec012RetainedCanceledHandleDoesNotCancelFreshLegacySequencer)
{
    Spec012CancellationSequencer sequencer;
    auto& app = App::GetApplication();
    auto* first = _doc->addObject<App::Spec012OutcomeFeature>("LegacyCanceled");
    first->mode = 4;
    first->duringExecute = [&sequencer] {
        sequencer.tryToCancel();
    };
    first->touch();
    auto retainedHandle = std::make_shared<App::RecomputeProgressHandle>();
    auto canceled = std::make_shared<Spec012Subscriber>();
    auto ticket = std::make_shared<App::RecomputeDetail::Retirement>();
    auto request = App::RecomputeRequest::fromDocumentObject(*first);
    request.progress = retainedHandle;
    request.callback = spec012Subscriber("O11", "legacy_seq_canceled", "LegacyCanceled", canceled);
    app.queueRecomputeRequestWithRetirement(std::move(request), ticket);
    ASSERT_EQ(canceled->done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(ticket));
    EXPECT_EQ(canceled->failure.load(), App::RecomputeFailure::Canceled);
    EXPECT_TRUE(retainedHandle->wasCanceled());
    EXPECT_EQ(first->ExecCount.getValue(), 1);
    auto* fresh = _doc->addObject<App::Spec012OutcomeFeature>("FreshLegacySequence");
    fresh->touch();
    auto completed = std::make_shared<Spec012Subscriber>();
    auto freshTicket = std::make_shared<App::RecomputeDetail::Retirement>();
    auto retry = App::RecomputeRequest::fromDocumentObject(*fresh);
    retry.callback
        = spec012Subscriber("O11", "fresh_after_legacy_cancel", "FreshLegacySequence", completed);
    app.queueRecomputeRequestWithRetirement(std::move(retry), freshTicket);
    ASSERT_EQ(completed->done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(freshTicket));
    EXPECT_TRUE(completed->success.load());
    EXPECT_EQ(fresh->ExecCount.getValue(), 1);
    EXPECT_TRUE(retainedHandle->wasCanceled());
}

TEST_F(AsyncRecomputeTest, Spec012NativeProgressCleanupCannotChangeCommittedResult)
{
    class CleanupSequencer final: public Base::SequencerBase
    {
    public:
        std::function<void()> cleanup;
        int stops {0};
        bool throwOnce {true};

    protected:
        void setText(const char*) override
        {}
        void setProgress(std::size_t) override
        {}
        void stopStep() override
        {
            ++stops;
            if (cleanup) {
                cleanup();
            }
            if (throwOnce) {
                throwOnce = false;
                throw Base::RuntimeError("Native progress cleanup observer failure");
            }
        }
    } sequencer;
    auto& app = App::GetApplication();
    auto* target = _doc->addObject<App::Spec012OutcomeFeature>("CommittedBeforeCleanup");
    target->touch();
    auto handle = std::make_shared<App::RecomputeProgressHandle>();
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    request.progress = handle;
    auto cancelKey = request;
    sequencer.cleanup = [&] {
        EXPECT_EQ(App::currentRecomputeProgress(), handle.get());
        EXPECT_FALSE(app.closeDocument(_docName.c_str()));
        EXPECT_TRUE(app.cancelRecomputeRequest(cancelKey).canceledInProgress);
        _doc->removeObject("CommittedBeforeCleanup");
    };
    std::promise<void> completed;
    auto done = completed.get_future();
    auto ticket = std::make_shared<App::RecomputeDetail::Retirement>();
    request.callback = [&](App::RecomputeRequest&, App::RecomputeResult& result) {
        EXPECT_TRUE(result.success);
        EXPECT_EQ(result.failure, App::RecomputeFailure::None);
        EXPECT_EQ(target->ExecCount.getValue(), 1);
        completed.set_value();
    };
    app.queueRecomputeRequestWithRetirement(std::move(request), ticket);
    ASSERT_EQ(done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(ticket));
    EXPECT_EQ(sequencer.stops, 1);
    EXPECT_TRUE(handle->wasCanceled());
    EXPECT_EQ(_doc->getObject("CommittedBeforeCleanup"), nullptr);
    sequencer.cleanup = {};
    auto* retry = _doc->addObject<App::Spec012OutcomeFeature>("AfterCleanupFailure");
    retry->touch();
    spec012Queue(
        "Q10",
        "after_progress_cleanup_failure",
        App::RecomputeRequest::fromDocumentObject(*retry),
        *_doc,
        retry
    );
    EXPECT_EQ(retry->ExecCount.getValue(), 1);
    EXPECT_EQ(sequencer.stops, 2);
    EXPECT_EQ(App::currentRecomputeProgress(), nullptr);
}


TEST_F(AsyncRecomputeTest, Spec012ProgressActivationObserverExceptionDoesNotStopWorker)
{
    auto& app = App::GetApplication();
    auto* target = _doc->addObject<App::Spec012OutcomeFeature>("ActivationObserverFailure");
    target->touch();
    auto handle = std::make_shared<App::RecomputeProgressHandle>();
    handle->setText("Display before activation");
    handle->setDisplayObserver([](const App::RecomputeProgressDisplayState& state) {
        if (state.text.empty()) {
            throw std::runtime_error("Progress display observer failure");
        }
    });
    auto terminal = std::make_shared<Spec012Subscriber>();
    auto retirement = std::make_shared<App::RecomputeDetail::Retirement>();
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    request.progress = handle;
    request.callback
        = spec012Subscriber("Q10", "activation_observer_failure", "ActivationObserverFailure", terminal);
    app.queueRecomputeRequestWithRetirement(std::move(request), retirement);
    ASSERT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(retirement));
    EXPECT_EQ(terminal->count.load(), 1);
    EXPECT_EQ(terminal->failure.load(), App::RecomputeFailure::Exception);
    EXPECT_EQ(target->ExecCount.getValue(), 0);
    handle->setDisplayObserver({});
    spec012Queue(
        "Q10",
        "after_activation_observer_failure",
        App::RecomputeRequest::fromDocumentObject(*target),
        *_doc,
        target
    );
    EXPECT_EQ(target->ExecCount.getValue(), 1);
}

TEST_F(AsyncRecomputeTest, Spec012LabelProposalPreflightsNewCrossDocumentEndpoint)
{
    auto& application = App::GetApplication();
    const auto name = application.getUniqueDocumentName("spec012_label_plan");
    auto* sourceDocument = application.newDocument(name.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        application.closeDocument(name.c_str());
    };
    auto* renamed = sourceDocument->addObject<App::FeatureTest>("Renamed");
    auto* safe = sourceDocument->addObject<App::FeatureTest>("SafeOriginal");
    auto* holder = sourceDocument->addObject<App::Spec012LabelProposalFeature>("Holder");
    auto* pinned = _doc->addObject<App::FeatureTestAsyncBlocker>("CrossDocumentNewTarget");
    ASSERT_TRUE(renamed && safe && holder && pinned);
    holder->Proposed.setAllowExternal(true);
    holder->Proposed.setValue(safe);
    holder->Proposed.nextTarget = pinned;
    const std::string oldLabel = renamed->Label.getValue();
    // Native extension property declares label-interest through the supported
    // reference index API; its native CopyOnLabelChange returns a genuinely NEW
    // cross-document target absent from its current link and initial snapshots.
    holder->Proposed.registerLabelReferences({oldLabel});
    ASSERT_TRUE(sourceDocument->containsLabel(oldLabel));
    int labelBefore = 0;
    fastsignals::scoped_connection before(sourceDocument->signalBeforeChangeObject.connect(
        [&](const App::DocumentObject& object, const App::Property& property) {
            if (&object == renamed && &property == &renamed->Label) {
                ++labelBefore;
            }
        }
    ));
    App::FeatureTestAsyncBlocker::resetBlocker();
    pinned->touch();
    auto terminal = std::make_shared<Spec012Subscriber>();
    auto request = App::RecomputeRequest::fromDocumentObject(*pinned);
    request.callback = [terminal](App::RecomputeRequest&, App::RecomputeResult&) {
        if (++terminal->count == 1) {
            terminal->completed.set_value();
        }
    };
    application.queueRecomputeRequest(std::move(request));
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
    const auto transaction = sourceDocument->getBookedTransactionID();
    const auto count = holder->Proposed.proposals;
    EXPECT_THROW(
        renamed->Label.setValue("Spec012 proposal refused"),
        App::RecomputeDetail::AdmissionRefusal
    );
    EXPECT_EQ(holder->Proposed.proposals, count + 1);
    EXPECT_EQ(std::string(renamed->Label.getValue()), oldLabel);
    EXPECT_TRUE(sourceDocument->containsLabel(oldLabel));
    EXPECT_FALSE(sourceDocument->containsLabel("Spec012 proposal refused"));
    EXPECT_EQ(holder->Proposed.getValue(), safe);
    EXPECT_EQ(sourceDocument->getBookedTransactionID(), transaction);
    EXPECT_EQ(labelBefore, 0);
    App::FeatureTestAsyncBlocker::releaseBlocker();
    EXPECT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
    auto after = std::make_shared<Spec012Subscriber>();
    auto successor = App::RecomputeRequest::fromDocumentObject(*safe);
    successor.callback = [after](App::RecomputeRequest&, App::RecomputeResult&) {
        if (++after->count == 1) {
            after->completed.set_value();
        }
    };
    auto successorRetirement = std::make_shared<App::RecomputeDetail::Retirement>();
    application.queueRecomputeRequestWithRetirement(std::move(successor), successorRetirement);
    ASSERT_EQ(after->done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(successorRetirement));
    // Same actual native proposal succeeds once the NEW endpoint is released.
    EXPECT_NO_THROW(renamed->Label.setValue("Spec012 proposal accepted"));
    EXPECT_EQ(std::string(renamed->Label.getValue()), "Spec012 proposal accepted");
    EXPECT_EQ(holder->Proposed.getValue(), pinned);
    EXPECT_EQ(labelBefore, 1);
}


TEST_F(AsyncRecomputeTest, Spec012ElementReferenceSnapshotOwnsNativePropertyStorage)
{
    auto* target = _doc->addObject<App::GeoFeature>("SymbolicElementTarget");
    auto* owner = _doc->addObject<App::FeatureTest>("ElementReferenceOwner");
    App::Spec012LifetimeLinkSub::destructions = 0;
    auto* property = dynamic_cast<App::Spec012LifetimeLinkSub*>(
        owner->addDynamicProperty("App::Spec012LifetimeLinkSub", "ObservedElementLink")
    );
    ASSERT_NE(property, nullptr);
    property->setValue(target, std::vector<std::string> {"Edge1"});
    bool logicallyRemoved;
    {
        auto snapshot = App::PropertyLinkBase::getElementReferences(target);
        ASSERT_NE(std::ranges::find(snapshot.properties(), property), snapshot.properties().end());
        // Native symbolic GeoFeature index/storage coverage; the default geometry
        // is empty, so this does not claim BRep/cache validity qualification.
        logicallyRemoved = owner->removeDynamicProperty("ObservedElementLink");
        EXPECT_EQ(App::Spec012LifetimeLinkSub::destructions.load(), 0);
    }
    if (!logicallyRemoved) {
        EXPECT_TRUE(owner->removeDynamicProperty("ObservedElementLink"));
    }
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (App::Spec012LifetimeLinkSub::destructions == 0
           && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    EXPECT_EQ(App::Spec012LifetimeLinkSub::destructions.load(), 1);
    EXPECT_EQ(owner->getPropertyByName("ObservedElementLink"), nullptr);
}

TEST_F(AsyncRecomputeTest, Spec012SavedDocInfoMembershipSurvivesRejectedUnlink)
{
    auto& app = App::GetApplication();
    const auto targetName = _docName;
    const auto firstName = app.getUniqueDocumentName("saved_docinfo_first");
    auto* firstDocument = app.newDocument(firstName.c_str());
    const auto secondName = app.getUniqueDocumentName("saved_docinfo_second");
    auto* secondDocument = app.newDocument(secondName.c_str());
    const auto directory = std::filesystem::temp_directory_path()
        / ("spec012-docinfo-"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(firstName.c_str());
        app.closeDocument(secondName.c_str());
        app.closeDocument(targetName.c_str());
        _doc = nullptr;
        _docName.clear();
        std::filesystem::remove_all(directory);
    };
    auto* target = _doc->addObject<App::FeatureTestAsyncBlocker>("SavedTarget");
    auto* first = firstDocument->addObject<App::FeatureTest>("FirstSource");
    auto* second = secondDocument->addObject<App::FeatureTest>("SecondSource");
    auto* firstLink = dynamic_cast<App::PropertyXLink*>(
        first->addDynamicProperty("App::PropertyXLink", "External")
    );
    auto* secondLink = dynamic_cast<App::PropertyXLink*>(
        second->addDynamicProperty("App::PropertyXLink", "External")
    );
    ASSERT_TRUE(firstLink && secondLink);
    ASSERT_TRUE(_doc->saveAs((directory / "target.FCStd").string().c_str()));
    ASSERT_TRUE(firstDocument->saveAs((directory / "first.FCStd").string().c_str()));
    ASSERT_TRUE(secondDocument->saveAs((directory / "second.FCStd").string().c_str()));
    firstLink->setValue(target);
    secondLink->setValue(target);
    second->ExpressionEngine.setValue(
        App::ObjectIdentifier(second->String),
        App::Expression::parse(second, targetName + "#SavedTarget.Label")
    );
    const auto beforeOut = App::PropertyXLink::getDocumentOutList();
    const auto beforeIn = App::PropertyXLink::getDocumentInList();
    ASSERT_EQ(beforeOut.at(firstDocument), std::set<App::Document*>({_doc}));
    ASSERT_EQ(beforeOut.at(secondDocument), std::set<App::Document*>({_doc}));
    ASSERT_EQ(beforeIn.at(_doc), std::set<App::Document*>({firstDocument, secondDocument}));
    int changes = 0;
    fastsignals::scoped_connection before(firstDocument->signalBeforeChangeObject.connect(
        [&](const App::DocumentObject& object, const App::Property& value) {
            if (&object == first && &value == firstLink) {
                ++changes;
            }
        }
    ));
    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };
    target->touch();
    auto terminal = std::make_shared<Spec012Subscriber>();
    auto ticket = std::make_shared<App::RecomputeDetail::Retirement>();
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    request.callback = spec012Subscriber("Q06", "saved_docinfo_lease", "SavedTarget", terminal);
    app.queueRecomputeRequestWithRetirement(std::move(request), ticket);
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(10s));
    const auto backlinks = target->getInList();
    EXPECT_THROW(firstLink->setValue(nullptr), App::RecomputeDetail::AdmissionRefusal);
    EXPECT_EQ(firstLink->getValue(), target);
    EXPECT_EQ(secondLink->getValue(), target);
    EXPECT_EQ(target->getInList(), backlinks);
    EXPECT_EQ(changes, 0);
    App::FeatureTestAsyncBlocker::releaseBlocker();
    ASSERT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(ticket));
    EXPECT_EQ(App::PropertyXLink::getDocumentOutList(), beforeOut);
    EXPECT_EQ(App::PropertyXLink::getDocumentInList(), beforeIn);
    EXPECT_NO_THROW(firstLink->setValue(nullptr));
    EXPECT_EQ(firstLink->getValue(), nullptr);
    EXPECT_EQ(secondLink->getValue(), target);
    const auto afterIn = App::PropertyXLink::getDocumentInList();
    ASSERT_EQ(afterIn.at(_doc), std::set<App::Document*>({secondDocument}));
    EXPECT_EQ(changes, 1);
    // The real incoming scalar XLink plus ExpressionEngine's owned XLink child
    // must not hit a late storage Busy after the target-removal snapshot starts.
    EXPECT_NO_THROW(_doc->removeObject("SavedTarget"));
    EXPECT_EQ(_doc->getObject("SavedTarget"), nullptr);
    EXPECT_EQ(secondLink->getValue(), nullptr);
}


TEST_F(AsyncRecomputeTest, Spec012PythonCloseReleasesGilOnlyWhileWaiting)
{
    auto& app = App::GetApplication();
    const auto name = _docName;
    auto* target = _doc->addObject<App::Spec012OutcomeFeature>("NeedsGilToFinish");
    target->mode = 5;
    auto state = std::make_shared<Spec012GilCompletionState>();
    auto entered = state->entered.get_future();
    auto release = std::make_shared<std::promise<void>>();
    state->proceed = release->get_future().share();
    target->gilCompletion = state;
    target->touch();
    auto progress = std::make_shared<App::RecomputeProgressHandle>();
    std::atomic<int> activeCallbacks {0};
    std::atomic<int> queuedCallbacks {0};
    std::atomic<bool> completionGil {false};
    std::atomic<bool> callerGil {false};
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    request.progress = progress;
    request.callback = [&](App::RecomputeRequest&, App::RecomputeResult& result) {
        Base::PyGILStateLocker gil;
        completionGil = PyGILState_Check();
        ++activeCallbacks;
        EXPECT_EQ(result.failure, App::RecomputeFailure::Canceled);
    };
    app.queueRecomputeRequest(std::move(request));
    BOOST_SCOPE_EXIT_ALL(&)
    {
        try {
            release->set_value();
        }
        catch (const std::future_error&) {
        }
    };
    ASSERT_EQ(entered.wait_for(10s), std::future_status::ready);
    auto rerun = App::RecomputeRequest::fromDocumentObject(*target);
    rerun.callback = [&](App::RecomputeRequest&, App::RecomputeResult& result) {
        callerGil = PyGILState_Check();
        ++queuedCallbacks;
        EXPECT_EQ(result.failure, App::RecomputeFailure::Canceled);
    };
    app.queueRecomputeRequest(std::move(rerun));
    std::atomic<bool> observedCancel {false};
    std::thread releaseGate([&] {
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (!progress->wasCanceled() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        observedCancel = progress->wasCanceled();
        release->set_value();
    });
    {
        // Exercise the actual Python binding, whose native close holds this GIL
        // on entry. Completion deliberately needs the same GIL after cancellation.
        Base::PyGILStateLocker gil;
        const std::string code = "import FreeCAD\nFreeCAD.closeDocument('" + name + "')";
        EXPECT_NO_THROW(Base::Interpreter().runString(code.c_str()));
    }
    releaseGate.join();
    EXPECT_TRUE(observedCancel.load());
    EXPECT_TRUE(state->acquired.load());
    EXPECT_TRUE(completionGil.load());
    EXPECT_TRUE(callerGil.load());
    EXPECT_EQ(activeCallbacks.load(), 1);
    EXPECT_EQ(queuedCallbacks.load(), 1);
    EXPECT_EQ(app.getDocument(name.c_str()), nullptr);
    _doc = nullptr;
    _docName.clear();
    const auto controlName = app.getUniqueDocumentName("after_python_close");
    auto* controlDoc = app.newDocument(controlName.c_str());
    BOOST_SCOPE_EXIT_ALL(&)
    {
        app.closeDocument(controlName.c_str());
    };
    auto* control = controlDoc->addObject<App::Spec012OutcomeFeature>("Control");
    control->touch();
    spec012Queue(
        "Q11",
        "after_python_gil_close_control",
        App::RecomputeRequest::fromDocumentObject(*control),
        *controlDoc,
        control
    );
}


TEST_F(AsyncRecomputeTest, Spec012DetachedPropertyStorageWaitsForOriginalNativeOwner)
{
    auto& app = App::GetApplication();
    auto* target = _doc->addObject<App::Spec012OutcomeFeature>("AuxiliaryPropertyOwner");
    target->mode = 6;
    App::Spec012LifetimeLinkSub::destructions = 0;
    target->duringExecute = [target] {
        // Auxiliary heap properties with a native container are also used by
        // composite link storage. This one was never inserted into the owner's
        // dynamic-property map, so public destroy cannot leave a dangling slot.
        auto* auxiliary = new App::Spec012LifetimeLinkSub;
        auxiliary->setContainer(target);
        auxiliary->setContainer(nullptr);
        App::Property::destroy(auxiliary);
        target->String.setValue("Trigger the real PropertyCleaner after detach");
        EXPECT_EQ(App::Spec012LifetimeLinkSub::destructions.load(), 0);
    };
    target->touch();
    auto terminal = std::make_shared<Spec012Subscriber>();
    auto retirement = std::make_shared<App::RecomputeDetail::Retirement>();
    auto request = App::RecomputeRequest::fromDocumentObject(*target);
    request.callback
        = spec012Subscriber("Q06", "original_owner_storage", "AuxiliaryPropertyOwner", terminal);
    app.queueRecomputeRequestWithRetirement(std::move(request), retirement);
    ASSERT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(retirement));
    EXPECT_TRUE(terminal->success.load());
    EXPECT_EQ(target->ExecCount.getValue(), 1);
    EXPECT_EQ(App::Spec012LifetimeLinkSub::destructions.load(), 1);
}

TEST_F(AsyncRecomputeTest, Spec012PlanningPinsRetireBeforeEarlierUndoRangeStep)
{
    auto* source = _doc->addObject<App::Spec012OutcomeFeature>("LinkSource");
    ASSERT_NE(source, nullptr);
    _doc->setMaxUndoStackSize(10);
    const int createId = _doc->openTransaction("Create target and refer to it");
    auto* target = _doc->addObject<App::Spec012OutcomeFeature>("RangeTarget");
    ASSERT_NE(target, nullptr);
    source->LinkSub.setValue(target, std::vector<std::string> {});
    _doc->commitTransaction();
    const int clearId = _doc->openTransaction("Clear target reference");
    source->LinkSub.setValue(nullptr, std::vector<std::string> {});
    _doc->commitTransaction();
    ASSERT_EQ(_doc->getAvailableUndos(), 2);
    ASSERT_EQ(source->LinkSub.getValue(), nullptr);

    // First range step restores LinkSub(target), entering real planning capture;
    // next step deletes target. A completed setter must not pin it until the
    // enclosing opaque undo(id) range ends.
    bool undone = false;
    EXPECT_NO_THROW(undone = _doc->undo(createId));
    ASSERT_TRUE(undone);
    EXPECT_EQ(_doc->getObject("RangeTarget"), nullptr);
    EXPECT_EQ(source->LinkSub.getValue(), nullptr);
    EXPECT_EQ(_doc->getAvailableUndos(), 0);
    EXPECT_EQ(_doc->getAvailableRedos(), 2);
    bool redone = false;
    EXPECT_NO_THROW(redone = _doc->redo(clearId));
    ASSERT_TRUE(redone);
    EXPECT_NE(_doc->getObject("RangeTarget"), nullptr);
    EXPECT_EQ(source->LinkSub.getValue(), nullptr);
    EXPECT_EQ(_doc->getAvailableUndos(), 2);
    EXPECT_EQ(_doc->getAvailableRedos(), 0);
}


TEST_F(AsyncRecomputeTest, Spec012WriterReleaseWakesAcceptedRequestWithoutRescue)
{
    auto& app = App::GetApplication();
    auto* target = _doc->addObject<App::Spec012OutcomeFeature>("WriterParkedTarget");
    target->touch();
    auto terminal = std::make_shared<Spec012Subscriber>();
    auto ticket = std::make_shared<App::RecomputeDetail::Retirement>();
    spec012WakeBoundary = 0;
    {
        auto writer = App::RecomputeDetail::Coordinator::instance().mutation({target});
        auto request = App::RecomputeRequest::fromDocumentObject(*target);
        request.callback
            = spec012Subscriber("Q06", "writer_release_only", "WriterParkedTarget", terminal);
        app.queueRecomputeRequestWithRetirement(std::move(request), ticket);
        EXPECT_EQ(terminal->done.wait_for(0ms), std::future_status::timeout);
        EXPECT_EQ(target->ExecCount.getValue(), 0);
        if (const char* seam = std::getenv("SPEC012_WAKE_SEAM"); seam && std::string(seam) == "park") {
            const auto deadline = std::chrono::steady_clock::now() + 10s;
            while (spec012WakeBoundary == 0 && std::chrono::steady_clock::now() < deadline) {
                std::this_thread::yield();
            }
            EXPECT_EQ(spec012WakeBoundary.load(), 1);
        }
    }  // Sole producer: release the real writer; no enqueue/pump/cleanup rescue.
    ASSERT_EQ(terminal->done.wait_for(10s), std::future_status::ready);
    ASSERT_TRUE(spec012WaitRetired(ticket));
    EXPECT_TRUE(terminal->success.load());
    EXPECT_EQ(terminal->count.load(), 1);
    EXPECT_EQ(target->ExecCount.getValue(), 1);
}
