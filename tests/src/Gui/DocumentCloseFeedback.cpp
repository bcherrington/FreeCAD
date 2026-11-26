// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QMessageBox>
#include <string>
#include <QTimer>

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/DocumentCloseFeedback.h>
#include <src/App/InitApplication.h>

class DocumentCloseFeedbackTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char name[] = "DocumentCloseFeedbackTest";
            static char* argv[] = {name, nullptr};
            static QApplication application(argc, argv);
        }
        ASSERT_NE(qobject_cast<QApplication*>(QCoreApplication::instance()), nullptr);
        tests::initApplication();
    }

    void SetUp() override
    {
        auto& application = App::GetApplication();
        name = application.getUniqueDocumentName("close_feedback");
        App::DocumentInitFlags flags;
        flags.createView = false;
        document = application.newDocument(name.c_str(), "testUser", flags);
        document->Label.setValue("Feedback <part>");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(name.c_str());
    }

    std::string name;
    App::Document* document {};
};

TEST_F(DocumentCloseFeedbackTest, ExplainsProtectionAfterRecomputingStatusClears)
{
    bool called = false;
    auto connection = document->signalBecameStable.connect([&](const App::Document& changed) {
        called = true;
        EXPECT_FALSE(changed.testStatus(App::Document::Recomputing));
        const auto text = Gui::documentCloseMessage(changed);
        EXPECT_TRUE(text.contains("Feedback <part>"));
        EXPECT_TRUE(text.contains("being recomputed"));
        EXPECT_TRUE(text.contains("try again"));
    });
    document->recompute();
    connection.disconnect();
    EXPECT_TRUE(called);
}

TEST_F(DocumentCloseFeedbackTest, ExplicitRestrictionDoesNotClaimRecompute)
{
    document->setClosable(false);
    const auto text = Gui::documentCloseMessage(*document);
    EXPECT_TRUE(text.contains("not closable"));
    EXPECT_FALSE(text.contains("recomputed"));
    document->setClosable(true);
}

TEST_F(DocumentCloseFeedbackTest, RefusedCloseAllShowsReasonAndReturnsFalse)
{
    bool callbackCalled = false;
    bool warningShown = false;
    auto connection = document->signalBeforeRecompute.connect([&](const App::Document&) {
        callbackCalled = true;
        QTimer dismissWarning;
        dismissWarning.setSingleShot(true);
        QObject::connect(&dismissWarning, &QTimer::timeout, [&] {
            for (auto* widget : QApplication::topLevelWidgets()) {
                if (auto* message = qobject_cast<QMessageBox*>(widget)) {
                    warningShown = true;
                    EXPECT_EQ(message->textFormat(), Qt::PlainText);
                    EXPECT_TRUE(message->text().contains("Feedback <part>"));
                    EXPECT_TRUE(message->text().contains("being recomputed"));
                    message->accept();
                }
            }
        });
        dismissWarning.start(0);
        EXPECT_FALSE(Gui::closeAllDocumentsWithFeedback(nullptr));
        EXPECT_EQ(App::GetApplication().getDocument(name.c_str()), document);
    });
    document->recompute();
    connection.disconnect();
    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(warningShown);
    EXPECT_TRUE(Gui::closeAllDocumentsWithFeedback(nullptr));
    EXPECT_EQ(App::GetApplication().getDocument(name.c_str()), nullptr);
}

TEST_F(DocumentCloseFeedbackTest, SuccessfulCloseAllReturnsTrue)
{
    EXPECT_TRUE(Gui::closeAllDocumentsWithFeedback(nullptr));
    EXPECT_EQ(App::GetApplication().getDocument(name.c_str()), nullptr);
}
