// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <chrono>
#include <algorithm>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <QCoreApplication>
#include <QPointer>
#include <QPushButton>
#include <QTest>
#include <Gui/AsyncPreviewSession.h>
#include <Mod/PartDesign/Gui/TaskFeatureParameters.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/MainThreadSignal.h>
#include <App/PropertyContainer.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/App/FeatureThickness.h>
#include <Mod/PartDesign/Gui/TaskThicknessParameters.h>
#include <Mod/PartDesign/Gui/ViewProviderThickness.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingThicknessTest: public PartDesign::Thickness
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingThicknessTest);

public:
    BlockingThicknessTest() = default;
    ~BlockingThicknessTest() override = default;

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static int getTotalExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

}  // namespace PartDesign

namespace
{
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingThicknessState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

Gui::QuantitySpinBox* findValueSpinBox(QWidget* taskBox)
{
    return taskBox ? taskBox->findChild<Gui::QuantitySpinBox*>(QStringLiteral("Value")) : nullptr;
}

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingThicknessTest,
    PartDesign::Thickness,
    blockingThicknessState
)

namespace
{
// Exercise the exported common acceptance helper with two genuine sessions,
// sharing the real feature/provider used by the native thickness fixture.
class SettlementPanel final: public PartDesignGui::TaskFeatureParameters
{
public:
    explicit SettlementPanel(PartDesignGui::ViewProvider* view)
        : TaskFeatureParameters(view, nullptr, "", QStringLiteral("Settlement control"))
    {
        auto* feature = view->getObject();
        Gui::AsyncPreviewController::Callbacks callbacks;
        callbacks.makeRequest = [feature] {
            return App::RecomputeRequest::fromDocumentObject(*feature);
        };
        callbacks.runSyncWithResult = [feature] {
            return Gui::recomputeDocumentObjectWithResult(feature);
        };
        callbacks.onCompleted =
            [this](bool success, bool, App::RecomputeFailure failure, const std::string&) {
                lastSuccess = success;
                lastFailure = failure;
                if (completed) {
                    completed();
                }
            };
        session = std::make_unique<Gui::AsyncPreviewSession>(std::move(callbacks), this);
        session->setSchedulerInterval(60000);
    }
    Gui::AsyncPreviewSession* getAcceptedRecomputeProgressSession() override
    {
        return session.get();
    }
    bool hasOutstandingRecompute() const override
    {
        return session->hasOutstandingRecompute();
    }
    void stopPendingRecompute() override
    {
        session->stopPendingRecompute();
    }
    void saveHistory() override
    {
        ++saved;
    }
    void apply() override
    {
        ++applied;
    }
    std::unique_ptr<Gui::AsyncPreviewSession> session;
    std::function<void()> completed;
    bool lastSuccess {false};
    App::RecomputeFailure lastFailure {App::RecomputeFailure::None};
    unsigned saved {0};
    unsigned applied {0};
};

class SettlementDialog final: public PartDesignGui::TaskDlgFeatureParameters
{
public:
    SettlementDialog(PartDesignGui::ViewProvider* view, SettlementPanel* first, SettlementPanel* second)
        : TaskDlgFeatureParameters(view)
    {
        Content = {preview, first, second};
    }
    bool settleAcceptedPanels()
    {
        bool blocked = false;
        return applyAcceptedFeatureParameters(AcceptPendingRecomputeAction::Stop, blocked);
    }
    bool replaceRetainedPanel(SettlementPanel* original, SettlementPanel* replacement)
    {
        const auto position
            = std::find(Content.begin(), Content.end(), static_cast<QWidget*>(original));
        if (position == Content.end()) {
            return false;
        }
        *position = replacement;
        original->setParent(nullptr);
        return true;
    }
};
class FinalCompletionDialog final: public PartDesignGui::TaskDlgFeatureParameters
{
public:
    FinalCompletionDialog(PartDesignGui::ViewProvider* view, SettlementPanel* panel)
        : TaskDlgFeatureParameters(view)
    {
        Content = {preview, panel};
    }

protected:
    AcceptPendingRecomputeAction acceptPendingRecomputeAction() const override
    {
        // Final acceptance must recompute the touched feature, rather than
        // purging it as an already successful preview.
        return AcceptPendingRecomputeAction::Stop;
    }
};
}  // namespace

class testTaskThicknessParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingThicknessTest::init}));

        asyncParams = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Document"
        );
        oldAsyncEnabled = asyncParams->GetBool("EnableAsyncRecompute", true);
        asyncParams->SetBool("EnableAsyncRecompute", true);

        gizmoParams = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Gui/Gizmos"
        );
        oldGizmosEnabled = gizmoParams->GetBool("EnableGizmos", true);
        gizmoParams->SetBool("EnableGizmos", false);
    }

    void cleanupTestCase()  // NOLINT
    {
        if (gizmoParams) {
            gizmoParams->SetBool("EnableGizmos", oldGizmosEnabled);
            gizmoParams = nullptr;
        }
        if (asyncParams) {
            asyncParams->SetBool("EnableAsyncRecompute", oldAsyncEnabled);
            asyncParams = nullptr;
        }
    }

    void init()  // NOLINT
    {
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingThicknessTest::init}));

        PartDesign::BlockingThicknessTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_thickness_dialog");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        guiDoc = Gui::Application::Instance->getDocument(doc);
        QVERIFY(guiDoc != nullptr);

        body = doc->addObject<PartDesign::Body>("Body");
        QVERIFY(body != nullptr);

        baseBox = doc->addObject<PartDesign::AdditiveBox>("BaseBox");
        QVERIFY(baseBox != nullptr);
        body->addObject(baseBox);
        baseBox->Length.setValue(10.0);
        baseBox->Width.setValue(10.0);
        baseBox->Height.setValue(10.0);

        doc->recompute();

        thickness = doc->addObject<PartDesign::BlockingThicknessTest>("BlockingThickness");
        QVERIFY(thickness != nullptr);
        body->addObject(thickness);
        thickness->Base.setValue(baseBox, {"Face1"});
        thickness->Value.setValue(1.0);
        thickness->Reversed.setValue(true);
        thickness->Mode.setValue(0L);
        thickness->Join.setValue(0L);

        doc->recompute();

        thicknessView = dynamic_cast<PartDesignGui::ViewProviderThickness*>(
            guiDoc->getViewProvider(thickness)
        );
        QVERIFY(thicknessView != nullptr);

        guiDoc->openCommand("Edit blocking thickness");
        PartDesign::BlockingThicknessTest::resetBlocker();
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingThicknessTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        thicknessView = nullptr;
        thickness = nullptr;
        baseBox = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void finalCompletionRetiresOriginalDialogBeforeFinalization_data()  // NOLINT
    {
        QTest::addColumn<bool>("deleteOriginal");
        QTest::newRow("retain-original-positive-control") << false;
        QTest::newRow("delete-original-after-native-retirement") << true;
    }

    void finalCompletionRetiresOriginalDialogBeforeFinalization()  // NOLINT
    {
        QFETCH(bool, deleteOriginal);
        auto* panel = new SettlementPanel(thicknessView);
        QPointer<FinalCompletionDialog> dialog(new FinalCompletionDialog(thicknessView, panel));
        const auto transaction = doc->getBookedTransactionID();
        QVERIFY(transaction != App::NullTransaction);
        unsigned commits = 0;
        fastsignals::scoped_connection committed(
            doc->signalCommitTransaction.connect([&](const App::Document&) { ++commits; })
        );
        bool connectedAtNativeGate = false;
        bool completedAfterRetirement = false;
        bool completionSucceeded = false;
        unsigned historiesBeforeCompletion = 0;
        unsigned applicationsBeforeCompletion = 0;
        QObject completionContext;
        auto bindAtNativeGate = std::make_shared<std::function<void()>>();
        std::weak_ptr<std::function<void()>> weakBinder = bindAtNativeGate;
        *bindAtNativeGate = [&, weakBinder] {
            if (!dialog) {
                return;
            }
            if (PartDesign::BlockingThicknessTest::getExecutionCount() != 1) {
                // Actual MetaCall pumping is available even when a controller
                // wait intentionally omits ordinary timer delivery.
                QMetaObject::invokeMethod(
                    qApp,
                    [weakBinder] {
                        if (auto binder = weakBinder.lock()) {
                            (*binder)();
                        }
                    },
                    Qt::QueuedConnection
                );
                return;
            }
            const auto controllers = Gui::getMainWindow()->findChildren<Gui::AsyncPreviewController*>(
                QString(),
                Qt::FindDirectChildrenOnly
            );
            if (controllers.size() != 1) {
                // Unblock native work so setup failure is reported as an
                // assertion below, rather than turning into a stranded worker.
                PartDesign::BlockingThicknessTest::releaseBlocker();
                return;
            }
            auto* controller = controllers.front();
            connectedAtNativeGate = true;
            QObject::connect(
                controller,
                &Gui::AsyncPreviewController::recomputeSettled,
                &completionContext,
                [&, controller] {
                    completedAfterRetirement = !controller->hasOutstandingRecompute();
                    completionSucceeded = controller->didLastRecomputeSucceed();
                    historiesBeforeCompletion = panel->saved;
                    applicationsBeforeCompletion = panel->applied;
                    if (deleteOriginal) {
                        // This is the actual final document controller's GUI
                        // completion, after its accepted native ticket retired.
                        delete dialog.data();
                    }
                }
            );
            PartDesign::BlockingThicknessTest::releaseBlocker();
        };
        PartDesign::BlockingThicknessTest::armBlocker();
        thickness->touch();
        QMetaObject::invokeMethod(
            qApp,
            [weakBinder] {
                if (auto binder = weakBinder.lock()) {
                    (*binder)();
                }
            },
            Qt::QueuedConnection
        );
        bool accepted = false;
        std::string acceptError;
        try {
            accepted = dialog->accept();
        }
        catch (const Base::Exception& error) {
            acceptError = error.what();
        }
        PartDesign::BlockingThicknessTest::releaseBlocker();
        bindAtNativeGate.reset();
        QVERIFY2(acceptError.empty(), acceptError.c_str());
        QVERIFY(connectedAtNativeGate);
        QVERIFY(completedAfterRetirement);
        QVERIFY(completionSucceeded);
        QCOMPARE(historiesBeforeCompletion, 1U);
        QCOMPARE(applicationsBeforeCompletion, 1U);
        QCOMPARE(accepted, !deleteOriginal);
        QCOMPARE(dialog.isNull(), deleteOriginal);
        QCOMPARE(commits, deleteOriginal ? 0U : 1U);
        QCOMPARE(guiDoc->hasPendingCommand(), deleteOriginal);
        if (deleteOriginal) {
            // Native completion did not authorize the deleted dialog's tail to
            // commit, abort or finalize the still-owned edit transaction.
            QCOMPARE(doc->getBookedTransactionID(), transaction);
            QVERIFY(thickness->isAttachedToDocument());
        }
        delete dialog.data();
    }

    void commonAcceptRefusesRetainedPanelContentReplacement()  // NOLINT
    {
        auto* first = new SettlementPanel(thicknessView);
        auto* second = new SettlementPanel(thicknessView);
        auto dialog = std::make_unique<SettlementDialog>(thicknessView, first, second);
        std::unique_ptr<SettlementPanel> retainedOriginal;
        SettlementPanel* replacement = nullptr;
        bool replacedDuringNativeSettlement = false;
        const auto transaction = doc->getBookedTransactionID();
        second->completed = [&] {
            replacement = new SettlementPanel(thicknessView);
            replacedDuringNativeSettlement = dialog->replaceRetainedPanel(first, replacement);
            if (replacedDuringNativeSettlement) {
                retainedOriginal.reset(first);
            }
            else {
                delete replacement;
                replacement = nullptr;
            }
        };
        PartDesign::BlockingThicknessTest::armBlocker();
        thickness->touch();
        second->session->requestRecompute(false);
        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingThicknessTest::getExecutionCount(), 1, 3000);
        QVERIFY(second->hasOutstandingRecompute());
        App::MainThreadSignalConfig::invoke(
            [] { PartDesign::BlockingThicknessTest::releaseBlocker(); },
            false
        );
        const bool accepted = dialog->settleAcceptedPanels();
        QVERIFY(replacedDuringNativeSettlement);
        QVERIFY(retainedOriginal);
        QVERIFY(replacement);
        // The original panel/controller/model remain live and unchanged. Only
        // actual Content membership changed during the real retirement pump.
        QVERIFY(!accepted);
        QCOMPARE(first->saved, 0U);
        QCOMPARE(first->applied, 0U);
        QCOMPARE(second->saved, 0U);
        QCOMPARE(second->applied, 0U);
        QCOMPARE(replacement->saved, 0U);
        QCOMPARE(replacement->applied, 0U);
        QVERIFY(!second->hasOutstandingRecompute());
        QCOMPARE(PartDesign::BlockingThicknessTest::getTotalExecutionCount(), 1);
        QCOMPARE(doc->getBookedTransactionID(), transaction);
        QVERIFY(guiDoc->hasPendingCommand());
        second->completed = {};
    }

    void commonAcceptPreservesOtherPanelReentry_data()  // NOLINT
    {
        QTest::addColumn<bool>("completeNewRequest");
        QTest::newRow("scheduled-only") << false;
        QTest::newRow("new-request-already-completed") << true;
    }

    void commonAcceptPreservesOtherPanelReentry()  // NOLINT
    {
        QFETCH(bool, completeNewRequest);
        auto* first = new SettlementPanel(thicknessView);
        auto* second = new SettlementPanel(thicknessView);
        auto dialog = std::make_unique<SettlementDialog>(thicknessView, first, second);
        bool reentered = false;
        bool newRequestCompleted = false;
        first->completed = [&] {
            newRequestCompleted = true;
        };
        second->completed = [&] {
            reentered = true;
            if (completeNewRequest) {
                // Complete a real new native request before B's settling pump
                // returns: an empty ledger alone must not hide this activity.
                thickness->touch();
                first->session->requestRecompute(true);
            }
            else {
                first->session->scheduleRecompute();
            }
        };
        PartDesign::BlockingThicknessTest::armBlocker();
        thickness->touch();
        second->session->requestRecompute(false);
        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingThicknessTest::getExecutionCount(), 1, 3000);
        QVERIFY(second->hasOutstandingRecompute());
        const auto firstBefore = first->session->recomputeActivityToken();
        // The blocking controller wait pumps FreeCAD's owned dispatcher, not
        // arbitrary qApp events. Release through that exact supported channel.
        App::MainThreadSignalConfig::invoke(
            [] { PartDesign::BlockingThicknessTest::releaseBlocker(); },
            /*blocking=*/false
        );
        QVERIFY(!dialog->settleAcceptedPanels());
        QVERIFY(reentered);
        QVERIFY(first->session->recomputeActivityToken() != firstBefore);
        QCOMPARE(first->saved, 0U);
        QCOMPARE(second->saved, 0U);
        QCOMPARE(first->applied, 0U);
        QCOMPARE(second->applied, 0U);
        QVERIFY(!first->hasOutstandingRecompute());
        QVERIFY(!second->hasOutstandingRecompute());
        QCOMPARE(first->session->controller()->hasScheduledRecompute(), !completeNewRequest);
        QCOMPARE(newRequestCompleted, completeNewRequest);
        QCOMPARE(second->lastFailure, App::RecomputeFailure::Canceled);
        if (completeNewRequest) {
            QVERIFY(first->lastSuccess);
            QCOMPARE(first->lastFailure, App::RecomputeFailure::None);
        }
        QCOMPARE(
            PartDesign::BlockingThicknessTest::getTotalExecutionCount(),
            completeNewRequest ? 2 : 1
        );
        QVERIFY(guiDoc->hasPendingCommand());
        // Cleanup only after proving the newly scheduled work was preserved.
        first->completed = {};
        second->completed = {};
        QVERIFY(first->stopPendingRecomputeAndWait());
    }

    void thicknessRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgThicknessParameters(thicknessView);
        QPointer<PartDesignGui::TaskDlgThicknessParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskThicknessParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* value = findValueSpinBox(taskBox);
        QVERIFY(value != nullptr);

        PartDesign::BlockingThicknessTest::armBlocker();
        value->setValue(value->rawValue() + 0.5);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingThicknessTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingThicknessTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void thicknessCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgThicknessParameters(thicknessView);
        QPointer<PartDesignGui::TaskDlgThicknessParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskThicknessParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* value = findValueSpinBox(taskBox);
        QVERIFY(value != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingThicknessTest::armBlocker();
        value->setValue(value->rawValue() + 0.5);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingThicknessTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingThicknessTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingThicknessTest::getExecutionCount(), 1);
    }

    void thicknessAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgThicknessParameters(thicknessView);
        QPointer<PartDesignGui::TaskDlgThicknessParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskThicknessParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* value = findValueSpinBox(taskBox);
        QVERIFY(value != nullptr);

        PartDesign::BlockingThicknessTest::armBlocker();
        value->setValue(value->rawValue() + 0.5);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingThicknessTest::getExecutionCount(), 1, 3000);
        QVERIFY2(
            PartDesign::BlockingThicknessTest::getTotalExecutionCount() <= 2,
            "accept should settle without replaying the queued preview and then rerunning again"
        );
        QVERIFY(taskBox->hasOutstandingRecompute());

        value->setValue(value->rawValue() + 0.5);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingThicknessTest::releaseBlocker();
        });

        std::string acceptError;
        try {
            Gui::Control().accept(doc);
        }
        catch (const Base::Exception& e) {
            acceptError = e.what();
        }
        catch (const std::exception& e) {
            acceptError = e.what();
        }
        catch (...) {
            acceptError = "unknown exception";
        }
        releaser.join();
        QVERIFY2(acceptError.empty(), acceptError.c_str());

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QVERIFY2(
            PartDesign::BlockingThicknessTest::getTotalExecutionCount() <= 2,
            "accept should finish with at most one settled preview and one final recompute"
        );
    }

private:
    Base::Reference<ParameterGrp> asyncParams;
    bool oldAsyncEnabled = true;
    Base::Reference<ParameterGrp> gizmoParams;
    bool oldGizmosEnabled = true;
    std::string docName;
    App::Document* doc = nullptr;
    Gui::Document* guiDoc = nullptr;
    PartDesign::Body* body = nullptr;
    PartDesign::AdditiveBox* baseBox = nullptr;
    PartDesign::BlockingThicknessTest* thickness = nullptr;
    PartDesignGui::ViewProviderThickness* thicknessView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskThicknessParametersTest(int argc, char** argv)
{
    testTaskThicknessParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskThicknessParametersTest.moc"
