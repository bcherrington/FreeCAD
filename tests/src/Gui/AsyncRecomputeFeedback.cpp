// SPDX-License-Identifier: LGPL-2.1-or-later

#include <atomic>
#include <future>
#include <memory>

#include <boost/scope_exit.hpp>

#include <QProgressDialog>
#include <QPushButton>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/FeatureTest.h>
#include <Gui/AsyncRecomputeFeedback.h>
#include <src/App/InitApplication.h>

using namespace std::chrono_literals;

class testAsyncRecomputeFeedback final: public QObject
{
    Q_OBJECT

public:
    testAsyncRecomputeFeedback()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void init()  // NOLINT
    {
        docName = App::GetApplication().getUniqueDocumentName("async_feedback");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
    }

    void cleanup()  // NOLINT
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }
        doc = nullptr;
        docName.clear();
    }

    void cancelQueuedRequestUpdatesFeedback()  // NOLINT
    {
        auto* blockingObject = dynamic_cast<App::FeatureTestAsyncBlocker*>(
            doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
        );
        QVERIFY(blockingObject);

        App::FeatureTestAsyncBlocker::resetBlocker();
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::FeatureTestAsyncBlocker::releaseBlocker();
        };

        auto queuedDocName = App::GetApplication().getUniqueDocumentName("async_feedback_queued");
        auto* queuedDoc = App::GetApplication().newDocument(queuedDocName.c_str(), "testUser");
        QVERIFY(queuedDoc);
        BOOST_SCOPE_EXIT_ALL(&)
        {
            if (App::GetApplication().getDocument(queuedDocName.c_str())) {
                App::GetApplication().closeDocument(queuedDocName.c_str());
            }
        };

        auto* queuedObject = dynamic_cast<App::FeatureTest*>(
            queuedDoc->addObject("App::FeatureTest", "QueuedFeature")
        );
        QVERIFY(queuedObject);

        blockingObject->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blockingObject)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

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

        QProgressDialog progress;
        QPushButton cancelButton;
        QString statusMessage;
        int statusTimeout = -1;

        const auto result = Gui::cancelQueuedAsyncRecomputeFeedback(
            queuedDocName,
            progress,
            cancelButton,
            [&statusMessage, &statusTimeout](const QString& message, int timeout) {
                statusMessage = message;
                statusTimeout = timeout;
            }
        );

        QCOMPARE(result.status, Gui::AsyncRecomputeCancelResult::Status::CanceledQueuedRequest);
        QCOMPARE(result.canceledRequests, 1U);
        QVERIFY(!cancelButton.isEnabled());
        QCOMPARE(progress.labelText(), QStringLiteral("Canceling recompute…"));
        QCOMPARE(statusMessage, QStringLiteral("Canceling recompute…"));
        QCOMPARE(statusTimeout, 0);
        QVERIFY(callbackFuture.wait_for(2s) == std::future_status::ready);
        QCOMPARE(callbackFuture.get(), App::RecomputeFailure::Canceled);
        QCOMPARE(callbackCount.load(), 1);

        App::FeatureTestAsyncBlocker::releaseBlocker();
    }

    void alreadyRunningRequestDisablesCancelButton()  // NOLINT
    {
        auto* blockingObject = dynamic_cast<App::FeatureTestAsyncBlocker*>(
            doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
        );
        QVERIFY(blockingObject);

        App::FeatureTestAsyncBlocker::resetBlocker();
        BOOST_SCOPE_EXIT_ALL(&)
        {
            App::FeatureTestAsyncBlocker::releaseBlocker();
        };

        blockingObject->touch();
        App::GetApplication().queueRecomputeRequest(
            App::RecomputeRequest::fromDocumentObject(*blockingObject)
        );
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

        QProgressDialog progress;
        QPushButton cancelButton;
        QString statusMessage;
        int statusTimeout = -1;

        const auto result = Gui::cancelQueuedAsyncRecomputeFeedback(
            docName,
            progress,
            cancelButton,
            [&statusMessage, &statusTimeout](const QString& message, int timeout) {
                statusMessage = message;
                statusTimeout = timeout;
            }
        );

        QCOMPARE(result.status, Gui::AsyncRecomputeCancelResult::Status::AlreadyRunning);
        QCOMPARE(result.canceledRequests, 0U);
        QVERIFY(!cancelButton.isEnabled());
        QCOMPARE(progress.labelText(), QStringLiteral("Recompute is already running…"));
        QCOMPARE(statusMessage, QStringLiteral("Recompute is already running…"));
        QCOMPARE(statusTimeout, 3000);

        App::FeatureTestAsyncBlocker::releaseBlocker();
    }

private:
    std::string docName;
    App::Document* doc {};
};

QTEST_MAIN(testAsyncRecomputeFeedback)

#include "AsyncRecomputeFeedback.moc"
