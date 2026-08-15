// SPDX-License-Identifier: LGPL-2.1-or-later

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
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
    struct PlacementStep
    {
        Result result;
        std::function<void(QDockWidget*, const PanelPlacement&)> mutate;
    };

    struct VisibilityStep
    {
        Result result;
        std::function<void(QDockWidget*, const PanelPlacement&, bool)> mutate;
    };

    Result applyPlacement(MainWindow*, QDockWidget* dockWidget, const PanelPlacement& placement) override
    {
        ++applyCount;
        appliedPlacements.push_back(placement);

        PlacementStep step;
        if (!placementSteps.empty()) {
            step = std::move(placementSteps.front());
            placementSteps.erase(placementSteps.begin());
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

    Result applyVisibility(
        MainWindow* mainWindow,
        QDockWidget* dockWidget,
        const PanelPlacement& placement,
        bool visible
    ) override
    {
        ++visibilityApplyCount;
        appliedVisibility.emplace_back(placement.panelId, visible);

        VisibilityStep step;
        if (!visibilitySteps.empty()) {
            step = std::move(visibilitySteps.front());
            visibilitySteps.erase(visibilitySteps.begin());
        }
        else {
            step.result
                = PanelPlacementHost::applyVisibility(mainWindow, dockWidget, placement, visible);
        }

        if (step.mutate) {
            step.result.mutated = true;
            step.mutate(dockWidget, placement, visible);
        }

        return step.result;
    }

    Result applyAreaOrder(MainWindow*, const QList<QDockWidget*>& dockWidgets, const PanelPlacement&) override
    {
        ++orderApplyCount;
        QStringList ids;
        for (QDockWidget* dock : dockWidgets) {
            ids.push_back(dock ? dock->objectName() : QString());
        }
        appliedOrders.push_back(ids);
        Result result;
        result.success = true;
        result.mutated = dockWidgets.size() > 1;
        return result;
    }

    bool queryPlacement(MainWindow*, QDockWidget*, PanelPlacement* placement) const override
    {
        if (!placement || !queriedPlacement) {
            return false;
        }
        *placement = *queriedPlacement;
        return true;
    }

    void enqueuePlacementSuccess(std::function<void(QDockWidget*, const PanelPlacement&)> mutate = {})
    {
        PlacementStep step;
        step.result.success = true;
        step.mutate = std::move(mutate);
        placementSteps.push_back(std::move(step));
    }

    void enqueuePlacementFailure(
        PanelPlacementHost::Error error,
        const QString& message,
        std::function<void(QDockWidget*, const PanelPlacement&)> mutate = {}
    )
    {
        PlacementStep step;
        step.result.success = false;
        step.result.error = error;
        step.result.message = message;
        step.mutate = std::move(mutate);
        placementSteps.push_back(std::move(step));
    }

    void enqueueVisibilitySuccess(
        std::function<void(QDockWidget*, const PanelPlacement&, bool)> mutate = {}
    )
    {
        VisibilityStep step;
        step.result.success = true;
        step.mutate = std::move(mutate);
        visibilitySteps.push_back(std::move(step));
    }

    void enqueueVisibilityFailure(
        PanelPlacementHost::Error error,
        const QString& message,
        std::function<void(QDockWidget*, const PanelPlacement&, bool)> mutate = {}
    )
    {
        VisibilityStep step;
        step.result.success = false;
        step.result.error = error;
        step.result.message = message;
        step.mutate = std::move(mutate);
        visibilitySteps.push_back(std::move(step));
    }

    int applyCount = 0;
    int orderApplyCount = 0;
    int visibilityApplyCount = 0;
    std::vector<PanelPlacement> appliedPlacements;
    std::vector<QStringList> appliedOrders;
    std::vector<std::pair<QString, bool>> appliedVisibility;
    std::vector<PlacementStep> placementSteps;
    std::vector<VisibilityStep> visibilitySteps;
    std::optional<PanelPlacement> queriedPlacement;
};

PanelPlacement makePlacement(
    const QString& panelId,
    PanelPlacement::Mode mode = PanelPlacement::Mode::Docked,
    PanelPlacement::Edge edge = PanelPlacement::Edge::Left,
    PanelPlacement::Region region = PanelPlacement::Region::Start,
    int order = 0,
    PanelPlacement::VisibilityPolicy policy = PanelPlacement::VisibilityPolicy::Exclusive
)
{
    PanelPlacement placement;
    placement.panelId = panelId;
    placement.mode = mode;
    placement.edge = edge;
    placement.region = region;
    placement.order = order;
    placement.visibilityPolicy = policy;
    placement.launcher.rail = edge == PanelPlacement::Edge::Right
        ? PanelPlacement::Launcher::Rail::Right
        : PanelPlacement::Launcher::Rail::Left;
    placement.launcher.cluster = PanelPlacement::Launcher::Cluster::Upper;
    placement.launcher.order = order;
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

        QSignalSpy activeSpy(&manager, &PanelPlacementManager::activeChanged);
        manager.setActive(true);
        manager.setActive(false);

        QCOMPARE(activeSpy.count(), 2);
        QCOMPARE(hostPtr->applyCount, 0);
        QCOMPARE(hostPtr->visibilityApplyCount, 0);
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

    void requestPlacementReordersSourceAndTargetAreasAndPersistsPeers()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        hostPtr->enqueuePlacementSuccess();

        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString leftA = QStringLiteral("CodexTestPanelPlacementManagerMoveLeftA");
        const QString leftB = QStringLiteral("CodexTestPanelPlacementManagerMoveLeftB");
        const QString rightA = QStringLiteral("CodexTestPanelPlacementManagerMoveRightA");
        const PanelPlacement persistedLeftA = makePlacement(
            leftA,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            0
        );
        const PanelPlacement persistedLeftB = makePlacement(
            leftB,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            1
        );
        const PanelPlacement persistedRightA = makePlacement(
            rightA,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Right,
            PanelPlacement::Region::Start,
            0
        );
        QVERIFY(PanelPlacementStore::savePlacement(persistedLeftA));
        QVERIFY(PanelPlacementStore::savePlacement(persistedLeftB));
        QVERIFY(PanelPlacementStore::savePlacement(persistedRightA));

        QMainWindow hostWindow;
        std::unique_ptr<QDockWidget> dockLeftA(makeDock(leftA));
        std::unique_ptr<QDockWidget> dockLeftB(makeDock(leftB));
        std::unique_ptr<QDockWidget> dockRightA(makeDock(rightA));
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockLeftA.get());
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockLeftB.get());
        hostWindow.addDockWidget(Qt::RightDockWidgetArea, dockRightA.get());
        hostWindow.show();
        QApplication::processEvents();
        QVERIFY(manager.registerPanel(leftA, dockLeftA.get(), persistedLeftA));
        QVERIFY(manager.registerPanel(leftB, dockLeftB.get(), persistedLeftB));
        QVERIFY(manager.registerPanel(rightA, dockRightA.get(), persistedRightA));

        PanelPlacement target = persistedLeftA;
        target.edge = PanelPlacement::Edge::Right;
        target.order = 0;
        target.visibilityPolicy = PanelPlacement::VisibilityPolicy::Multiple;
        target.normalize();

        const auto result = manager.requestPlacement(leftA, target);
        QVERIFY(result.success);
        QCOMPARE(hostPtr->applyCount, 1);
        QCOMPARE(hostPtr->orderApplyCount, 1);
        QCOMPARE(hostPtr->appliedOrders.back(), (QStringList {leftA, rightA}));

        QCOMPARE(
            manager.orderedPanelIds(
                makePlacement(QString(), PanelPlacement::Mode::Docked, PanelPlacement::Edge::Left)
            ),
            QStringList {leftB}
        );
        QCOMPARE(
            manager.orderedPanelIds(
                makePlacement(QString(), PanelPlacement::Mode::Docked, PanelPlacement::Edge::Right)
            ),
            (QStringList {leftA, rightA})
        );

        PanelPlacement loadedLeftA;
        PanelPlacement loadedLeftB;
        PanelPlacement loadedRightA;
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(leftA, &loadedLeftA));
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(leftB, &loadedLeftB));
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(rightA, &loadedRightA));
        QCOMPARE(loadedLeftA.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loadedLeftA.order, 0);
        QCOMPARE(loadedLeftA.visibilityPolicy, PanelPlacement::VisibilityPolicy::Multiple);
        QCOMPARE(loadedLeftB.edge, PanelPlacement::Edge::Left);
        QCOMPARE(loadedLeftB.order, 0);
        QCOMPARE(loadedRightA.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loadedRightA.order, 1);
        QCOMPARE(loadedRightA.visibilityPolicy, PanelPlacement::VisibilityPolicy::Multiple);
        QCOMPARE(manager.persistedPlacement(rightA).order, 1);
        QCOMPARE(manager.runtimePlacement(rightA).order, 1);
    }

    void requestVisibilityExclusiveShowsOnePanel()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelA = QStringLiteral("CodexTestPanelPlacementManagerVisibleA");
        const QString panelB = QStringLiteral("CodexTestPanelPlacementManagerVisibleB");
        const PanelPlacement placementA = makePlacement(
            panelA,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            0
        );
        const PanelPlacement placementB = makePlacement(
            panelB,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            1
        );
        QVERIFY(PanelPlacementStore::savePlacement(placementA));
        QVERIFY(PanelPlacementStore::savePlacement(placementB));

        QMainWindow hostWindow;
        QDockWidget* dockA = makeDock(panelA);
        QDockWidget* dockB = makeDock(panelB);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockA);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockB);
        hostWindow.show();
        QApplication::processEvents();
        dockA->show();
        dockB->show();

        QVERIFY(manager.registerPanel(panelA, dockA, placementA));
        QVERIFY(manager.registerPanel(panelB, dockB, placementB));
        hostPtr->visibilityApplyCount = 0;

        const auto result = manager.requestVisibility(panelB, true);
        QVERIFY(result.success);
        QCOMPARE(hostPtr->visibilityApplyCount, 2);
        QCOMPARE(hostPtr->appliedVisibility.back(), std::make_pair(panelA, false));
        QVERIFY(!dockA->isVisible());
        QVERIFY(dockB->isVisible());
    }

    void registerPanelExclusiveNormalizesVisiblePeers()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelA = QStringLiteral("CodexTestPanelPlacementManagerRegisterExclusiveA");
        const QString panelB = QStringLiteral("CodexTestPanelPlacementManagerRegisterExclusiveB");
        const PanelPlacement placementA = makePlacement(panelA);
        const PanelPlacement placementB = makePlacement(
            panelB,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            1
        );
        QVERIFY(PanelPlacementStore::savePlacement(placementA));
        QVERIFY(PanelPlacementStore::savePlacement(placementB));

        QMainWindow hostWindow;
        QDockWidget* dockA = makeDock(panelA);
        QDockWidget* dockB = makeDock(panelB);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockA);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockB);
        // Match application startup: docks have been explicitly shown, but
        // their main window has not appeared yet.
        dockA->show();
        dockB->show();

        // Register in reverse order to prove that normalization follows the
        // persisted rail order rather than registration order.
        QVERIFY(manager.registerPanel(panelB, dockB, placementB));
        QVERIFY(manager.registerPanel(panelA, dockA, placementA));
        QVERIFY(!dockA->isHidden());
        QVERIFY(dockB->isHidden());
        QCOMPARE(
            manager.persistedPlacement(panelA).visibilityPolicy,
            PanelPlacement::VisibilityPolicy::Exclusive
        );
        QCOMPARE(
            manager.persistedPlacement(panelB).visibilityPolicy,
            PanelPlacement::VisibilityPolicy::Exclusive
        );
    }

    void requestAreaVisibilityPolicyMultipleKeepsPeersVisible()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelA = QStringLiteral("CodexTestPanelPlacementManagerMultipleA");
        const QString panelB = QStringLiteral("CodexTestPanelPlacementManagerMultipleB");
        const PanelPlacement placementA = makePlacement(
            panelA,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            0
        );
        const PanelPlacement placementB = makePlacement(
            panelB,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            1
        );
        QVERIFY(PanelPlacementStore::savePlacement(placementA));
        QVERIFY(PanelPlacementStore::savePlacement(placementB));

        QMainWindow hostWindow;
        QDockWidget* dockA = makeDock(panelA);
        QDockWidget* dockB = makeDock(panelB);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockA);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockB);
        hostWindow.show();
        QApplication::processEvents();
        dockA->show();
        dockB->hide();

        QVERIFY(manager.registerPanel(panelA, dockA, placementA));
        QVERIFY(manager.registerPanel(panelB, dockB, placementB));
        QVERIFY(
            manager.requestAreaVisibilityPolicy(panelA, PanelPlacement::VisibilityPolicy::Multiple).success
        );

        const auto visibilityResult = manager.requestVisibility(panelB, true);
        QVERIFY(visibilityResult.success);
        QVERIFY(dockA->isVisible());
        QVERIFY(dockB->isVisible());
        QCOMPARE(
            manager.persistedPlacement(panelA).visibilityPolicy,
            PanelPlacement::VisibilityPolicy::Multiple
        );
        QCOMPARE(
            manager.persistedPlacement(panelB).visibilityPolicy,
            PanelPlacement::VisibilityPolicy::Multiple
        );
    }

    void registerPanelMultiplePreservesVisiblePeers()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelA = QStringLiteral("CodexTestPanelPlacementManagerRegisterMultipleA");
        const QString panelB = QStringLiteral("CodexTestPanelPlacementManagerRegisterMultipleB");
        const PanelPlacement placementA = makePlacement(
            panelA,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            0,
            PanelPlacement::VisibilityPolicy::Multiple
        );
        const PanelPlacement placementB = makePlacement(
            panelB,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            1,
            PanelPlacement::VisibilityPolicy::Multiple
        );
        QVERIFY(PanelPlacementStore::savePlacement(placementA));
        QVERIFY(PanelPlacementStore::savePlacement(placementB));

        QMainWindow hostWindow;
        QDockWidget* dockA = makeDock(panelA);
        QDockWidget* dockB = makeDock(panelB);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockA);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockB);
        hostWindow.show();
        QApplication::processEvents();
        dockA->show();
        dockB->show();

        QVERIFY(manager.registerPanel(panelA, dockA, placementA));
        QVERIFY(manager.registerPanel(panelB, dockB, placementB));
        QVERIFY(dockA->isVisible());
        QVERIFY(dockB->isVisible());
        QCOMPARE(
            manager.persistedPlacement(panelA).visibilityPolicy,
            PanelPlacement::VisibilityPolicy::Multiple
        );
        QCOMPARE(
            manager.persistedPlacement(panelB).visibilityPolicy,
            PanelPlacement::VisibilityPolicy::Multiple
        );
    }

    void requestAreaVisibilityPolicyExclusiveKeepsFirstVisibleByOrder()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelA = QStringLiteral("CodexTestPanelPlacementManagerPolicyA");
        const QString panelB = QStringLiteral("CodexTestPanelPlacementManagerPolicyB");
        const QString panelC = QStringLiteral("CodexTestPanelPlacementManagerPolicyC");
        const PanelPlacement placementA = makePlacement(
            panelA,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            0,
            PanelPlacement::VisibilityPolicy::Multiple
        );
        const PanelPlacement placementB = makePlacement(
            panelB,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            1,
            PanelPlacement::VisibilityPolicy::Multiple
        );
        const PanelPlacement placementC = makePlacement(
            panelC,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            2,
            PanelPlacement::VisibilityPolicy::Multiple
        );
        QVERIFY(PanelPlacementStore::savePlacement(placementA));
        QVERIFY(PanelPlacementStore::savePlacement(placementB));
        QVERIFY(PanelPlacementStore::savePlacement(placementC));

        QMainWindow hostWindow;
        QDockWidget* dockA = makeDock(panelA);
        QDockWidget* dockB = makeDock(panelB);
        QDockWidget* dockC = makeDock(panelC);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockA);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockB);
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockC);
        hostWindow.show();
        QApplication::processEvents();
        dockA->show();
        dockB->show();
        dockC->hide();

        QVERIFY(manager.registerPanel(panelA, dockA, placementA));
        QVERIFY(manager.registerPanel(panelB, dockB, placementB));
        QVERIFY(manager.registerPanel(panelC, dockC, placementC));

        const auto result = manager.requestAreaVisibilityPolicy(
            panelB,
            PanelPlacement::VisibilityPolicy::Exclusive
        );
        QVERIFY(result.success);
        QVERIFY(dockA->isVisible());
        QVERIFY(!dockB->isVisible());
        QVERIFY(!dockC->isVisible());
        QCOMPARE(
            manager.persistedPlacement(panelA).visibilityPolicy,
            PanelPlacement::VisibilityPolicy::Exclusive
        );
        QCOMPARE(
            manager.persistedPlacement(panelB).visibilityPolicy,
            PanelPlacement::VisibilityPolicy::Exclusive
        );
        QCOMPARE(
            manager.persistedPlacement(panelC).visibilityPolicy,
            PanelPlacement::VisibilityPolicy::Exclusive
        );
    }

    void requestPlacementFailureRollsBackMutatedTargetAndPeerPersistence()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        hostPtr->enqueuePlacementFailure(
            PanelPlacementHost::Error::ApplyFailed,
            QStringLiteral("attach failed"),
            [](QDockWidget* dockWidget, const PanelPlacement&) {
                dockWidget->setFloating(true);
                dockWidget->setGeometry(QRect(30, 40, 320, 240));
            }
        );
        hostPtr->enqueuePlacementSuccess([](QDockWidget* dockWidget, const PanelPlacement&) {
            dockWidget->setFloating(false);
        });

        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString leftA = QStringLiteral("CodexTestPanelPlacementManagerRollbackA");
        const QString leftB = QStringLiteral("CodexTestPanelPlacementManagerRollbackB");
        const QString rightA = QStringLiteral("CodexTestPanelPlacementManagerRollbackC");
        const PanelPlacement persistedLeftA = makePlacement(
            leftA,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            0
        );
        const PanelPlacement persistedLeftB = makePlacement(
            leftB,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Left,
            PanelPlacement::Region::Start,
            1
        );
        const PanelPlacement persistedRightA = makePlacement(
            rightA,
            PanelPlacement::Mode::Docked,
            PanelPlacement::Edge::Right,
            PanelPlacement::Region::Start,
            0
        );
        QVERIFY(PanelPlacementStore::savePlacement(persistedLeftA));
        QVERIFY(PanelPlacementStore::savePlacement(persistedLeftB));
        QVERIFY(PanelPlacementStore::savePlacement(persistedRightA));

        QMainWindow hostWindow;
        std::unique_ptr<QDockWidget> dockLeftA(makeDock(leftA));
        std::unique_ptr<QDockWidget> dockLeftB(makeDock(leftB));
        std::unique_ptr<QDockWidget> dockRightA(makeDock(rightA));
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockLeftA.get());
        hostWindow.addDockWidget(Qt::LeftDockWidgetArea, dockLeftB.get());
        hostWindow.addDockWidget(Qt::RightDockWidgetArea, dockRightA.get());
        hostWindow.show();
        QApplication::processEvents();
        QVERIFY(manager.registerPanel(leftA, dockLeftA.get(), persistedLeftA));
        QVERIFY(manager.registerPanel(leftB, dockLeftB.get(), persistedLeftB));
        QVERIFY(manager.registerPanel(rightA, dockRightA.get(), persistedRightA));

        PanelPlacement target = persistedLeftA;
        target.edge = PanelPlacement::Edge::Right;
        target.order = 0;
        target.visibilityPolicy = PanelPlacement::VisibilityPolicy::Multiple;
        target.normalize();

        const auto result = manager.requestPlacement(leftA, target);
        QVERIFY(!result.success);
        QCOMPARE(
            static_cast<int>(result.error),
            static_cast<int>(PanelPlacementManager::RequestError::HostFailure)
        );
        QCOMPARE(hostPtr->applyCount, 2);
        QCOMPARE(manager.persistedPlacement(leftA), persistedLeftA);
        QCOMPARE(manager.runtimePlacement(leftA).mode, PanelPlacement::Mode::Docked);

        PanelPlacement loadedLeftA;
        PanelPlacement loadedLeftB;
        PanelPlacement loadedRightA;
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(leftA, &loadedLeftA));
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(leftB, &loadedLeftB));
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(rightA, &loadedRightA));
        QCOMPARE(loadedLeftA, persistedLeftA);
        QCOMPARE(loadedLeftB, persistedLeftB);
        QCOMPARE(loadedRightA, persistedRightA);
        QCOMPARE(manager.persistedPlacement(leftB), persistedLeftB);
        QCOMPARE(manager.persistedPlacement(rightA), persistedRightA);
    }

    void rollbackFailureReportsUnsafeRuntimeState()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        FakeHost* hostPtr = host.get();
        hostPtr->enqueuePlacementFailure(
            PanelPlacementHost::Error::ApplyFailed,
            QStringLiteral("attach failed"),
            [](QDockWidget* dockWidget, const PanelPlacement&) {
                dockWidget->setFloating(true);
                dockWidget->setGeometry(QRect(90, 90, 280, 180));
            }
        );
        hostPtr->enqueuePlacementFailure(
            PanelPlacementHost::Error::ApplyFailed,
            QStringLiteral("rollback failed")
        );

        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerRollbackFailure");
        const PanelPlacement original = makePlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(original));

        std::unique_ptr<QDockWidget> dock(makeDock(panelId));
        QVERIFY(manager.registerPanel(panelId, dock.get(), original));

        PanelPlacement target = original;
        target.mode = PanelPlacement::Mode::Floating;
        target.edge = PanelPlacement::Edge::None;
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
    }

    void reentrantTransitionIsRejected()  // NOLINT
    {
        mainWindowPrefs->SetBool(PanelPlacementStore::featureFlagKey(), true);

        auto host = std::make_unique<FakeHost>();
        PanelPlacementManager manager(nullptr, std::move(host));
        manager.setActive(true);

        const QString panelId = QStringLiteral("CodexTestPanelPlacementManagerReentrant");
        const PanelPlacement original = makePlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(original));

        std::unique_ptr<QDockWidget> dock(makeDock(panelId));
        QVERIFY(manager.registerPanel(panelId, dock.get(), original));

        auto* fakeHost = static_cast<FakeHost*>(manager.host());
        PanelPlacementManager::RequestResult nestedResult;
        fakeHost->enqueuePlacementSuccess(
            [&manager, &panelId, &nestedResult](QDockWidget*, const PanelPlacement& placement) {
                nestedResult = manager.requestPlacement(panelId, placement);
            }
        );

        PanelPlacement target = original;
        target.mode = PanelPlacement::Mode::Floating;
        target.edge = PanelPlacement::Edge::None;
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
        const std::array<QString, 21> panelIds {{
            QStringLiteral("CodexTestPanelPlacementManagerA"),
            QStringLiteral("CodexTestPanelPlacementManagerActivation"),
            QStringLiteral("CodexTestPanelPlacementManagerB"),
            QStringLiteral("CodexTestPanelPlacementManagerMoveLeftA"),
            QStringLiteral("CodexTestPanelPlacementManagerMoveLeftB"),
            QStringLiteral("CodexTestPanelPlacementManagerMoveRightA"),
            QStringLiteral("CodexTestPanelPlacementManagerVisibleA"),
            QStringLiteral("CodexTestPanelPlacementManagerVisibleB"),
            QStringLiteral("CodexTestPanelPlacementManagerRegisterExclusiveA"),
            QStringLiteral("CodexTestPanelPlacementManagerRegisterExclusiveB"),
            QStringLiteral("CodexTestPanelPlacementManagerMultipleA"),
            QStringLiteral("CodexTestPanelPlacementManagerMultipleB"),
            QStringLiteral("CodexTestPanelPlacementManagerRegisterMultipleA"),
            QStringLiteral("CodexTestPanelPlacementManagerRegisterMultipleB"),
            QStringLiteral("CodexTestPanelPlacementManagerPolicyA"),
            QStringLiteral("CodexTestPanelPlacementManagerPolicyB"),
            QStringLiteral("CodexTestPanelPlacementManagerPolicyC"),
            QStringLiteral("CodexTestPanelPlacementManagerRollbackA"),
            QStringLiteral("CodexTestPanelPlacementManagerRollbackB"),
            QStringLiteral("CodexTestPanelPlacementManagerRollbackC"),
            QStringLiteral("CodexTestPanelPlacementManagerRollbackFailure"),
        }};

        for (const auto& panelId : panelIds) {
            placementsRoot->RemoveGrp(panelId.toUtf8().constData());
        }
        placementsRoot->RemoveGrp("CodexTestPanelPlacementManagerReentrant");
    }

    ParameterGrp::handle placementsRoot;
    ParameterGrp::handle mainWindowPrefs;
    ExactBoolBackup featureFlagBackup;
};

QTEST_MAIN(testPanelPlacementManager)

#include "PanelPlacementManager.moc"
