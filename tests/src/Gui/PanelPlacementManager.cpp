// SPDX-License-Identifier: LGPL-2.1-or-later

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <QDockWidget>
#include <QLineEdit>
#include <QMainWindow>
#include <QSignalSpy>
#include <QTest>
#include <QVBoxLayout>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/PanelPlacementManager.h>
#include <src/App/InitApplication.h>

using namespace Gui;

namespace
{

bool findBool(ParameterGrp::handle group, const char* key, bool* value = nullptr)
{
    for (const auto& [name, entry] : group->GetBoolMap()) {
        if (name == key) {
            if (value) {
                *value = entry;
            }
            return true;
        }
    }

    return false;
}

struct ExactBoolBackup
{
    bool present = false;
    bool value = false;
};

class FakeHost final: public PanelPlacementHost
{
public:
    struct Step
    {
        Result result;
        std::function<void(QDockWidget*, const PanelPlacement&)> mutate;
    };

    Result applyPlacement(MainWindow*, QDockWidget* dockWidget, const PanelPlacement& placement) override
    {
        ++applyCount;
        appliedPlacements.push_back(placement);

        Step step;
        if (!steps.empty()) {
            step = std::move(steps.front());
            steps.erase(steps.begin());
        }
        else {
            step.result.success = true;
        }

        if (step.mutate) {
            step.result.mutated = true;
            step.mutate(dockWidget, placement);
        }

        return step.result;
    }

    bool queryPlacement(MainWindow*, QDockWidget*, PanelPlacement* placement) const override
    {
        if (!placement || !queriedPlacement) {
            return false;
        }
        *placement = *queriedPlacement;
        return true;
    }

    void enqueueSuccess(std::function<void(QDockWidget*, const PanelPlacement&)> mutate = {})
    {
        Step step;
        step.result.success = true;
        step.mutate = std::move(mutate);
        steps.push_back(std::move(step));
    }

    void enqueueFailure(
        PanelPlacementHost::Error error,
        const QString& message,
        std::function<void(QDockWidget*, const PanelPlacement&)> mutate = {}
    )
    {
        Step step;
        step.result.success = false;
        step.result.error = error;
        step.result.message = message;
        step.mutate = std::move(mutate);
        steps.push_back(std::move(step));
    }

    int applyCount = 0;
    std::vector<PanelPlacement> appliedPlacements;
    std::vector<Step> steps;
    std::optional<PanelPlacement> queriedPlacement;
};

PanelPlacement makePlacement(
    const QString& panelId,
    PanelPlacement::Mode mode = PanelPlacement::Mode::Docked,
    PanelPlacement::Edge edge = PanelPlacement::Edge::Left
)
{
    PanelPlacement placement;
    placement.panelId = panelId;
    placement.mode = mode;
    placement.edge = edge;
    placement.region = PanelPlacement::Region::Start;
    placement.launcher.rail = PanelPlacement::Launcher::Rail::Left;
    placement.launcher.cluster = PanelPlacement::Launcher::Cluster::Upper;
    placement.launcher.order = 0;
    placement.normalize();
    return placement;
}

QDockWidget* makeDock(const QString& title)
{
    auto* dock = new QDockWidget(title);
    dock->setObjectName(title);
    dock->setWidget(new QWidget(dock));
    return dock;
}

}  // namespace

class testPanelPlacementManager final: public QObject
{
    Q_OBJECT

public:
    testPanelPlacementManager()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void init()  // NOLINT
    {
        placementsRoot = App::GetApplication().GetParameterGroupByPath(
            PanelPlacementStore::placementsPath()
        );
        mainWindowPrefs = App::GetApplication().GetParameterGroupByPath(
            PanelPlacementStore::mainWindowPreferencesPath()
        );

        featureFlagBackup.present = findBool(
            mainWindowPrefs,
            PanelPlacementStore::featureFlagKey(),
            &featureFlagBackup.value
        );

        clearTestState();
        mainWindowPrefs->RemoveBool(PanelPlacementStore::featureFlagKey());
    }

    void cleanup()  // NOLINT
    {
        clearTestState();
        if (featureFlagBackup.present) {
            mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), featureFlagBackup.value);
        }
        else {
            mainWindowPrefs->RemoveBool(PanelPlacementStore::featureFlagKey());
        }
    }

    void featureFlagDelegatesToStore()  // NOLINT
    {
        PanelPlacementManager manager(nullptr);
        QVERIFY(!PanelPlacementStore::isFeatureEnabled());
        QVERIFY(!manager.isFeatureEnabled());

        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);
        QVERIFY(PanelPlacementStore::isFeatureEnabled());
        QVERIFY(manager.isFeatureEnabled());
    }

    void activationDoesNotMovePanels()  // NOLINT
    {
        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        PanelPlacementManager manager(nullptr, std::move(host));
        std::unique_ptr<QDockWidget> dock(
            makeDock(QStringLiteral("CodexTestPanelPlacementManagerActivation"))
        );

        QVERIFY(manager.registerPanel(
            QStringLiteral("CodexTestPanelPlacementManagerActivation"),
            dock.get(),
            makePlacement(QStringLiteral("CodexTestPanelPlacementManagerActivation"))
        ));
        const PanelPlacement before = manager.runtimePlacement(
            QStringLiteral("CodexTestPanelPlacementManagerActivation")
        );

        QSignalSpy activeSpy(&manager, &PanelPlacementManager::activeChanged);
        manager.setActive(true);
        manager.setActive(false);

        QCOMPARE(activeSpy.count(), 2);
        QCOMPARE(hostPtr->applyCount, 0);
        QCOMPARE(
            manager.runtimePlacement(QStringLiteral("CodexTestPanelPlacementManagerActivation")),
            before
        );
    }

    void registrationRejectsInvalidAndDuplicateInput()  // NOLINT
    {
        PanelPlacementManager manager(nullptr, std::make_unique<FakeHost>());
        std::unique_ptr<QDockWidget> dockA(makeDock(QStringLiteral("CodexTestPanelPlacementManagerA")));
        std::unique_ptr<QDockWidget> dockB(makeDock(QStringLiteral("CodexTestPanelPlacementManagerB")));
        QString error;

        QVERIFY(
            !manager.registerPanel(QStringLiteral(" Invalid"), dockA.get(), PanelPlacement(), &error)
        );
        QVERIFY(error.contains(QStringLiteral("stable")));

        QVERIFY(manager.registerPanel(
            QStringLiteral("CodexTestPanelPlacementManagerA"),
            dockA.get(),
            makePlacement(QStringLiteral("CodexTestPanelPlacementManagerA"))
        ));
        QVERIFY(!manager.registerPanel(
            QStringLiteral("CodexTestPanelPlacementManagerA"),
            dockB.get(),
            makePlacement(QStringLiteral("CodexTestPanelPlacementManagerA"))
        ));
        QVERIFY(!manager.registerPanel(
            QStringLiteral("CodexTestPanelPlacementManagerB"),
            dockA.get(),
            makePlacement(QStringLiteral("CodexTestPanelPlacementManagerB"))
        ));

        QCOMPARE(
            manager.registeredPanelIds(),
            QStringList {QStringLiteral("CodexTestPanelPlacementManagerA")}
        );
    }

    void successfulTransactionPersistsPlacement()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        hostPtr->enqueueSuccess([](QDockWidget* dockWidget, const PanelPlacement& placement) {
            if (placement.mode == PanelPlacement::Mode::Floating) {
                dockWidget->setFloating(true);
                dockWidget->setGeometry(placement.floatingGeometry);
            }
        });

        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerSuccess");
        PanelPlacement original = makePlacement(panelId);
        original.launcher.order = 3;
        QVERIFY(PanelPlacementStore::savePlacement(original));

        std::unique_ptr<QDockWidget> dock(makeDock(panelId));
        QVERIFY(manager.registerPanel(panelId, dock.get(), original));

        PanelPlacement target = manager.persistedPlacement(panelId);
        target.mode = PanelPlacement::Mode::Floating;
        target.floatingGeometry = QRect(40, 60, 320, 240);
        target.normalize();

        const auto result = manager.requestPlacement(panelId, target);
        QVERIFY(result.success);
        QCOMPARE(
            static_cast<int>(result.error),
            static_cast<int>(PanelPlacementManager::RequestError::None)
        );
        QCOMPARE(hostPtr->applyCount, 1);

        PanelPlacement loaded;
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(panelId, &loaded));
        QCOMPARE(loaded, target);
        QCOMPARE(manager.persistedPlacement(panelId), target);
        QCOMPARE(manager.runtimePlacement(panelId).mode, PanelPlacement::Mode::Floating);
        QCOMPARE(manager.runtimePlacement(panelId).floatingGeometry, QRect(40, 60, 320, 240));
    }

    void failedTransactionRollsBackWithoutPersisting()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        hostPtr->enqueueFailure(
            PanelPlacementHost::Error::ApplyFailed,
            QStringLiteral("attach failed"),
            [](QDockWidget* dockWidget, const PanelPlacement&) {
                dockWidget->hide();
                dockWidget->setFloating(true);
                dockWidget->setGeometry(QRect(15, 20, 410, 300));
            }
        );
        hostPtr->enqueueSuccess([](QDockWidget* dockWidget, const PanelPlacement& placement) {
            if (placement.mode == PanelPlacement::Mode::Docked) {
                dockWidget->setFloating(false);
            }
        });

        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerRollback");
        PanelPlacement original = makePlacement(panelId);
        original.launcher.order = 2;
        QVERIFY(PanelPlacementStore::savePlacement(original));

        QDockWidget* dock = makeDock(panelId);
        QMainWindow dockHost;
        dockHost.addDockWidget(Qt::LeftDockWidgetArea, dock);
        dockHost.show();
        QApplication::processEvents();
        QVERIFY(manager.registerPanel(panelId, dock, original));

        PanelPlacement target = manager.persistedPlacement(panelId);
        target.mode = PanelPlacement::Mode::Floating;
        target.floatingGeometry = QRect(25, 35, 500, 280);
        target.normalize();

        const auto result = manager.requestPlacement(panelId, target);
        QVERIFY(!result.success);
        QCOMPARE(
            static_cast<int>(result.error),
            static_cast<int>(PanelPlacementManager::RequestError::HostFailure)
        );
        QCOMPARE(hostPtr->applyCount, 2);
        QVERIFY(dock->isVisible());
        QVERIFY(!dock->isFloating());

        PanelPlacement loaded;
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(panelId, &loaded));
        QCOMPARE(loaded, original);
        QCOMPARE(manager.persistedPlacement(panelId), original);
        QCOMPARE(manager.runtimePlacement(panelId).mode, PanelPlacement::Mode::Docked);
    }

    void rollbackFailureReportsSafeState()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        hostPtr->enqueueFailure(
            PanelPlacementHost::Error::ApplyFailed,
            QStringLiteral("attach failed"),
            [](QDockWidget* dockWidget, const PanelPlacement&) {
                dockWidget->setFloating(true);
                dockWidget->setGeometry(QRect(90, 90, 280, 180));
            }
        );
        hostPtr->enqueueFailure(
            PanelPlacementHost::Error::ApplyFailed,
            QStringLiteral("rollback failed")
        );

        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerRollbackFailure");
        PanelPlacement original = makePlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(original));

        std::unique_ptr<QDockWidget> dock(makeDock(panelId));
        QVERIFY(manager.registerPanel(panelId, dock.get(), original));

        PanelPlacement target = manager.persistedPlacement(panelId);
        target.mode = PanelPlacement::Mode::Floating;
        target.floatingGeometry = QRect(90, 90, 280, 180);
        target.normalize();

        const auto result = manager.requestPlacement(panelId, target);
        QVERIFY(!result.success);
        QCOMPARE(
            static_cast<int>(result.error),
            static_cast<int>(PanelPlacementManager::RequestError::RollbackFailed)
        );
        QCOMPARE(hostPtr->applyCount, 2);
        QCOMPARE(manager.persistedPlacement(panelId), original);
        QCOMPARE(manager.runtimePlacement(panelId).mode, PanelPlacement::Mode::Floating);

        PanelPlacement loaded;
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(panelId, &loaded));
        QCOMPARE(loaded, original);
    }

    void overlayOriginRollbackPreservesSemanticPlacement()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerOverlayRollback");
        PanelPlacement original
            = makePlacement(panelId, PanelPlacement::Mode::Overlay, PanelPlacement::Edge::Left);
        QVERIFY(PanelPlacementStore::savePlacement(original));

        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        hostPtr->queriedPlacement = original;
        hostPtr->enqueueFailure(
            PanelPlacementHost::Error::ApplyFailed,
            QStringLiteral("attach failed"),
            [](QDockWidget* dockWidget, const PanelPlacement&) { dockWidget->setFloating(true); }
        );
        hostPtr->enqueueSuccess();

        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);
        std::unique_ptr<QDockWidget> dock(makeDock(panelId));
        QVERIFY(manager.registerPanel(panelId, dock.get(), original));
        QCOMPARE(manager.runtimePlacement(panelId).mode, PanelPlacement::Mode::Overlay);

        PanelPlacement target = original;
        target.mode = PanelPlacement::Mode::Floating;
        target.edge = PanelPlacement::Edge::None;
        target.floatingGeometry = QRect(80, 100, 320, 240);
        target.normalize();

        const auto result = manager.requestPlacement(panelId, target);
        QVERIFY(!result.success);
        QCOMPARE(hostPtr->applyCount, 2);
        QCOMPARE(hostPtr->appliedPlacements.at(1).mode, PanelPlacement::Mode::Overlay);
        QCOMPARE(hostPtr->appliedPlacements.at(1).edge, PanelPlacement::Edge::Left);
        QCOMPARE(manager.runtimePlacement(panelId), original);
        QCOMPARE(manager.persistedPlacement(panelId), original);
    }

    void registrationImportsLiveOverlayWithoutEagerPersistence()  // NOLINT
    {
        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerOverlayImport");
        PanelPlacement liveOverlay
            = makePlacement(panelId, PanelPlacement::Mode::Overlay, PanelPlacement::Edge::Right);
        liveOverlay.launcher.rail = PanelPlacement::Launcher::Rail::Left;
        liveOverlay.launcher.cluster = PanelPlacement::Launcher::Cluster::Bottom;

        auto host = std::make_unique<FakeHost>();
        host->queriedPlacement = liveOverlay;
        PanelPlacementManager manager(nullptr, std::move(host));
        std::unique_ptr<QDockWidget> dock(makeDock(panelId));

        QVERIFY(!PanelPlacementStore::hasPlacement(panelId));
        QVERIFY(manager.registerPanel(panelId, dock.get(), liveOverlay));
        QCOMPARE(manager.runtimePlacement(panelId), liveOverlay);
        QCOMPARE(manager.persistedPlacement(panelId), liveOverlay);
        QVERIFY(!PanelPlacementStore::hasPlacement(panelId));
    }

    void launcherUpdateDoesNotAttach()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerLauncher");
        PanelPlacement original = makePlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(original));

        std::unique_ptr<QDockWidget> dock(makeDock(panelId));
        QVERIFY(manager.registerPanel(panelId, dock.get(), original));

        PanelPlacement::Launcher launcher = original.launcher;
        launcher.rail = PanelPlacement::Launcher::Rail::Right;
        launcher.cluster = PanelPlacement::Launcher::Cluster::Bottom;
        launcher.order = 5;

        const auto result = manager.updateLauncher(panelId, launcher);
        QVERIFY(result.success);
        QCOMPARE(hostPtr->applyCount, 0);

        PanelPlacement loaded;
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(panelId, &loaded));
        QCOMPARE(loaded.launcher, launcher);
        QCOMPARE(manager.runtimePlacement(panelId).launcher, launcher);
    }

    void unsupportedAutoHideRequestIsInert()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        PanelPlacementManager manager(nullptr);
        manager.setActive(true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerOverlay");
        PanelPlacement original = makePlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(original));

        QDockWidget* dock = makeDock(panelId);
        QMainWindow dockHost;
        dockHost.addDockWidget(Qt::LeftDockWidgetArea, dock);
        dockHost.show();
        QApplication::processEvents();
        QVERIFY(manager.registerPanel(panelId, dock, original));

        PanelPlacement target = manager.persistedPlacement(panelId);
        target.mode = PanelPlacement::Mode::AutoHide;
        target.edge = PanelPlacement::Edge::Left;
        target.normalize();

        const auto result = manager.requestPlacement(panelId, target);
        QVERIFY(!result.success);
        QCOMPARE(
            static_cast<int>(result.error),
            static_cast<int>(PanelPlacementManager::RequestError::UnsupportedPlacement)
        );
        QVERIFY(!dock->isFloating());
        QVERIFY(dock->isVisible());
        QCOMPARE(manager.persistedPlacement(panelId), original);
    }

    void focusAndVisibilityAreRestored()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        PanelPlacementManager manager(nullptr, std::make_unique<FakeHost>());
        manager.setActive(true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerFocus");
        PanelPlacement original = makePlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(original));

        QDockWidget* dock = makeDock(panelId);
        auto* content = new QWidget(dock);
        auto* layout = new QVBoxLayout(content);
        auto* editor = new QLineEdit(content);
        layout->addWidget(editor);
        content->setLayout(layout);
        dock->setWidget(content);

        QMainWindow focusHost;
        auto* focusSink = new QWidget(&focusHost);
        auto* focusSinkEditor = new QLineEdit(focusSink);
        auto* sinkLayout = new QVBoxLayout(focusSink);
        sinkLayout->addWidget(focusSinkEditor);
        focusHost.setCentralWidget(focusSink);
        focusHost.addDockWidget(Qt::LeftDockWidgetArea, dock);
        focusHost.show();
        focusHost.activateWindow();
        QApplication::processEvents();
        editor->setFocus(Qt::OtherFocusReason);
        QApplication::processEvents();

        auto* fakeHost = static_cast<FakeHost*>(manager.host());
        fakeHost->enqueueSuccess(
            [focusSinkEditor](QDockWidget* dockWidget, const PanelPlacement& placement) {
                dockWidget->hide();
                dockWidget->setFloating(placement.mode == PanelPlacement::Mode::Floating);
                focusSinkEditor->setFocus(Qt::OtherFocusReason);
            }
        );

        QVERIFY(manager.registerPanel(panelId, dock, original));
        focusHost.activateWindow();
        editor->setFocus(Qt::OtherFocusReason);
        QApplication::processEvents();
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(editor));

        PanelPlacement target = manager.persistedPlacement(panelId);
        target.mode = PanelPlacement::Mode::Floating;
        target.floatingGeometry = QRect(100, 120, 240, 180);
        target.normalize();

        const auto result = manager.requestPlacement(panelId, target);
        QVERIFY(result.success);
        QVERIFY(dock->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget*>(editor));
    }

    void reentrantTransitionIsRejected()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerReentrant");
        PanelPlacement original = makePlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(original));

        std::unique_ptr<QDockWidget> dock(makeDock(panelId));
        QVERIFY(manager.registerPanel(panelId, dock.get(), original));

        auto* fakeHost = static_cast<FakeHost*>(manager.host());
        PanelPlacementManager::RequestResult nestedResult;
        fakeHost->enqueueSuccess(
            [&manager, &panelId, &nestedResult](QDockWidget*, const PanelPlacement& placement) {
                nestedResult = manager.requestPlacement(panelId, placement);
            }
        );

        PanelPlacement target = manager.persistedPlacement(panelId);
        target.mode = PanelPlacement::Mode::Floating;
        target.floatingGeometry = QRect(60, 80, 280, 180);
        target.normalize();

        const auto result = manager.requestPlacement(panelId, target);
        QVERIFY(result.success);
        QVERIFY(!nestedResult.success);
        QCOMPARE(
            static_cast<int>(nestedResult.error),
            static_cast<int>(PanelPlacementManager::RequestError::TransitionInProgress)
        );
    }

private:
    void clearTestState()
    {
        const std::array<QString, 12> panelIds {{
            QStringLiteral("CodexTestPanelPlacementManagerA"),
            QStringLiteral("CodexTestPanelPlacementManagerActivation"),
            QStringLiteral("CodexTestPanelPlacementManagerB"),
            QStringLiteral("CodexTestPanelPlacementManagerSuccess"),
            QStringLiteral("CodexTestPanelPlacementManagerRollback"),
            QStringLiteral("CodexTestPanelPlacementManagerRollbackFailure"),
            QStringLiteral("CodexTestPanelPlacementManagerOverlayRollback"),
            QStringLiteral("CodexTestPanelPlacementManagerOverlayImport"),
            QStringLiteral("CodexTestPanelPlacementManagerLauncher"),
            QStringLiteral("CodexTestPanelPlacementManagerOverlay"),
            QStringLiteral("CodexTestPanelPlacementManagerFocus"),
            QStringLiteral("CodexTestPanelPlacementManagerReentrant"),
        }};

        for (const auto& panelId : panelIds) {
            placementsRoot->RemoveGrp(panelId.toUtf8().constData());
        }
    }

    ParameterGrp::handle placementsRoot;
    ParameterGrp::handle mainWindowPrefs;
    ExactBoolBackup featureFlagBackup;
};

QTEST_MAIN(testPanelPlacementManager)

#include "PanelPlacementManager.moc"
