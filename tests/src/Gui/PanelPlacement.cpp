// SPDX-License-Identifier: LGPL-2.1-or-later

#include <array>
#include <tuple>

#include <QTest>
#include <QStringList>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/PanelPlacement.h>
#include <src/App/InitApplication.h>

using namespace Gui;

namespace
{

bool findInt(ParameterGrp::handle group, const char* key, long* value = nullptr)
{
    for (const auto& [name, entry] : group->GetIntMap()) {
        if (name == key) {
            if (value) {
                *value = entry;
            }
            return true;
        }
    }

    return false;
}

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

bool findAscii(ParameterGrp::handle group, const char* key, QString* value = nullptr)
{
    for (const auto& [name, entry] : group->GetASCIIMap()) {
        if (name == key) {
            if (value) {
                *value = QString::fromUtf8(entry.c_str());
            }
            return true;
        }
    }

    return false;
}

struct ExactKeyBackup
{
    bool hasSchemaVersion = false;
    long schemaVersion = 0;
    bool hasMigrationMarker = false;
    bool migrationMarker = false;
};

}  // namespace

class testPanelPlacement final: public QObject
{
    Q_OBJECT

public:
    testPanelPlacement()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void init()  // NOLINT
    {
        placementsRoot = App::GetApplication().GetParameterGroupByPath(
            PanelPlacementStore::placementsPath()
        );
        compactSlots = App::GetApplication().GetParameterGroupByPath(
            PanelPlacementStore::compactLegacySlotsPath()
        );
        panelRailsSlots = App::GetApplication().GetParameterGroupByPath(
            PanelPlacementStore::panelRailsLegacySlotsPath()
        );

        backups.hasSchemaVersion
            = findInt(placementsRoot, PanelPlacementStore::schemaVersionKey(), &backups.schemaVersion);
        backups.hasMigrationMarker = findBool(
            placementsRoot,
            PanelPlacementStore::migrationMarkerKey(),
            &backups.migrationMarker
        );

        clearTestState();
    }

    void cleanup()  // NOLINT
    {
        clearTestState();

        if (backups.hasSchemaVersion) {
            placementsRoot->SetInt(PanelPlacementStore::schemaVersionKey(), backups.schemaVersion);
        }
        else {
            placementsRoot->RemoveInt(PanelPlacementStore::schemaVersionKey());
        }

        if (backups.hasMigrationMarker) {
            placementsRoot->SetBool(PanelPlacementStore::migrationMarkerKey(), backups.migrationMarker);
        }
        else {
            placementsRoot->RemoveBool(PanelPlacementStore::migrationMarkerKey());
        }
    }

    void enumRoundtrip()  // NOLINT
    {
        const std::array<std::pair<PanelPlacement::Mode, QString>, 4> modes {{
            {PanelPlacement::Mode::Docked, QStringLiteral("docked")},
            {PanelPlacement::Mode::Overlay, QStringLiteral("overlay")},
            {PanelPlacement::Mode::AutoHide, QStringLiteral("auto-hide")},
            {PanelPlacement::Mode::Floating, QStringLiteral("floating")},
        }};
        for (const auto& [mode, text] : modes) {
            PanelPlacement::Mode parsed = PanelPlacement::Mode::Docked;
            QCOMPARE(panelPlacementModeToString(mode), text);
            QVERIFY(panelPlacementModeFromString(text, &parsed));
            QCOMPARE(parsed, mode);
        }

        const std::array<std::pair<PanelPlacement::Edge, QString>, 5> edges {{
            {PanelPlacement::Edge::Left, QStringLiteral("left")},
            {PanelPlacement::Edge::Right, QStringLiteral("right")},
            {PanelPlacement::Edge::Top, QStringLiteral("top")},
            {PanelPlacement::Edge::Bottom, QStringLiteral("bottom")},
            {PanelPlacement::Edge::None, QStringLiteral("none")},
        }};
        for (const auto& [edge, text] : edges) {
            PanelPlacement::Edge parsed = PanelPlacement::Edge::Left;
            QCOMPARE(panelPlacementEdgeToString(edge), text);
            QVERIFY(panelPlacementEdgeFromString(text, &parsed));
            QCOMPARE(parsed, edge);
        }

        const std::array<std::pair<PanelPlacement::Region, QString>, 3> regions {{
            {PanelPlacement::Region::Start, QStringLiteral("start")},
            {PanelPlacement::Region::Center, QStringLiteral("center")},
            {PanelPlacement::Region::End, QStringLiteral("end")},
        }};
        for (const auto& [region, text] : regions) {
            PanelPlacement::Region parsed = PanelPlacement::Region::Start;
            QCOMPARE(panelPlacementRegionToString(region), text);
            QVERIFY(panelPlacementRegionFromString(text, &parsed));
            QCOMPARE(parsed, region);
        }

        const std::array<std::pair<PanelPlacement::VisibilityPolicy, QString>, 2> policies {{
            {PanelPlacement::VisibilityPolicy::Exclusive, QStringLiteral("exclusive")},
            {PanelPlacement::VisibilityPolicy::Multiple, QStringLiteral("multiple")},
        }};
        for (const auto& [policy, text] : policies) {
            PanelPlacement::VisibilityPolicy parsed = PanelPlacement::VisibilityPolicy::Exclusive;
            QCOMPARE(panelPlacementVisibilityPolicyToString(policy), text);
            QVERIFY(panelPlacementVisibilityPolicyFromString(text, &parsed));
            QCOMPARE(parsed, policy);
        }

        const std::array<std::pair<PanelPlacement::Launcher::Rail, QString>, 2> rails {{
            {PanelPlacement::Launcher::Rail::Left, QStringLiteral("left")},
            {PanelPlacement::Launcher::Rail::Right, QStringLiteral("right")},
        }};
        for (const auto& [rail, text] : rails) {
            PanelPlacement::Launcher::Rail parsed = PanelPlacement::Launcher::Rail::Left;
            QCOMPARE(panelPlacementRailToString(rail), text);
            QVERIFY(panelPlacementRailFromString(text, &parsed));
            QCOMPARE(parsed, rail);
        }

        const std::array<std::pair<PanelPlacement::Launcher::Cluster, QString>, 3> clusters {{
            {PanelPlacement::Launcher::Cluster::Upper, QStringLiteral("upper")},
            {PanelPlacement::Launcher::Cluster::Lower, QStringLiteral("lower")},
            {PanelPlacement::Launcher::Cluster::Bottom, QStringLiteral("bottom")},
        }};
        for (const auto& [cluster, text] : clusters) {
            PanelPlacement::Launcher::Cluster parsed = PanelPlacement::Launcher::Cluster::Upper;
            QCOMPARE(panelPlacementClusterToString(cluster), text);
            QVERIFY(panelPlacementClusterFromString(text, &parsed));
            QCOMPARE(parsed, cluster);
        }

        PanelPlacement::Mode mode = PanelPlacement::Mode::Docked;
        QVERIFY(!panelPlacementModeFromString(QStringLiteral("bad"), &mode));
        QVERIFY(!panelPlacementEdgeFromString(QStringLiteral("bad"), nullptr));
    }

    void normalizePlacement()  // NOLINT
    {
        PanelPlacement floating;
        floating.panelId = QStringLiteral("CodexTestFloating");
        floating.mode = PanelPlacement::Mode::Floating;
        floating.edge = PanelPlacement::Edge::Right;
        floating.launcher.order = -5;
        floating.normalize();
        QCOMPARE(floating.mode, PanelPlacement::Mode::Floating);
        QCOMPARE(floating.edge, PanelPlacement::Edge::None);
        QCOMPARE(floating.launcher.order, 0);

        PanelPlacement corrupt;
        corrupt.panelId = QStringLiteral("Translated title");
        corrupt.mode = PanelPlacement::Mode::Overlay;
        corrupt.edge = PanelPlacement::Edge::None;
        corrupt.groupOrder = -4;
        corrupt.tabOrder = -2;
        corrupt.extent = -10;
        corrupt.launcher.order = -1;
        corrupt.normalize();
        QCOMPARE(corrupt.mode, PanelPlacement::Mode::Docked);
        QCOMPARE(corrupt.edge, PanelPlacement::Edge::Left);
        QCOMPARE(corrupt.groupOrder, 0);
        QCOMPARE(corrupt.tabOrder, 0);
        QCOMPARE(corrupt.extent, 0);
        QCOMPARE(corrupt.launcher.order, 0);
        QVERIFY(!corrupt.hasStablePanelId());
    }

    void saveLoadRoundtrip()  // NOLINT
    {
        const std::array<PanelPlacement, 2> cases = []() {
            std::array<PanelPlacement, 2> placements;

            placements[0].panelId = QStringLiteral("CodexTestPanelPlacementRoundtrip");
            placements[0].mode = PanelPlacement::Mode::Overlay;
            placements[0].edge = PanelPlacement::Edge::Right;
            placements[0].region = PanelPlacement::Region::End;
            placements[0].order = 7;
            placements[0].visibilityPolicy = PanelPlacement::VisibilityPolicy::Multiple;
            placements[0].groupId = QStringLiteral("main-right");
            placements[0].groupOrder = 5;
            placements[0].tabOrder = 2;
            placements[0].splitRelation = QStringLiteral("after:Std_TaskView");
            placements[0].extent = 240;
            placements[0].floatingGeometry = QRect(100, 120, 640, 480);

            placements[1].panelId = QStringLiteral("CodexTestPanelPlacementRoundtripSecond");
            placements[1].mode = PanelPlacement::Mode::Docked;
            placements[1].edge = PanelPlacement::Edge::Bottom;
            placements[1].region = PanelPlacement::Region::Start;
            placements[1].order = 3;
            placements[1].visibilityPolicy = PanelPlacement::VisibilityPolicy::Exclusive;
            placements[1].groupId = QStringLiteral("main-bottom");

            return placements;
        }();

        for (const auto& input : cases) {
            QVERIFY(PanelPlacementStore::savePlacement(input));
            QCOMPARE(PanelPlacementStore::schemaVersion(), PanelPlacementStore::CurrentSchemaVersion);
            QVERIFY(PanelPlacementStore::migrationMarker());

            PanelPlacement loaded;
            QVERIFY(PanelPlacementStore::loadUnifiedPlacement(input.panelId, &loaded));

            PanelPlacement expected = input;
            expected.normalize();
            QCOMPARE(loaded, expected);

            auto group = placementsRoot->GetGroup(input.panelId.toUtf8().constData());
            long storedOrder = -1;
            QString storedPolicy;
            QVERIFY(findInt(group, "Order", &storedOrder));
            QVERIFY(findAscii(group, "VisibilityPolicy", &storedPolicy));
            QCOMPARE(storedOrder, static_cast<long>(expected.order));
            QCOMPARE(storedPolicy, panelPlacementVisibilityPolicyToString(expected.visibilityPolicy));
            QVERIFY(!findAscii(group, "LauncherRail"));
            QVERIFY(!findAscii(group, "LauncherCluster"));
            QVERIFY(!findInt(group, "LauncherOrder"));
        }
    }

    void mismatchedFallbackIdUsesRequestedId()  // NOLINT
    {
        PanelPlacement fallback;
        fallback.panelId = QStringLiteral("WrongFallbackId");
        fallback.edge = PanelPlacement::Edge::Right;
        fallback.region = PanelPlacement::Region::Center;
        fallback.order = 6;
        fallback.visibilityPolicy = PanelPlacement::VisibilityPolicy::Multiple;

        const QString requestedId = QStringLiteral("CodexTestPanelPlacementRequestedId");
        PanelPlacement loaded = PanelPlacementStore::loadPlacement(requestedId, fallback);
        QCOMPARE(loaded.panelId, requestedId);
        QCOMPARE(loaded.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loaded.region, PanelPlacement::Region::Center);
        QCOMPARE(loaded.order, 6);
        QCOMPARE(loaded.visibilityPolicy, PanelPlacement::VisibilityPolicy::Multiple);
    }

    void corruptFallback()  // NOLINT
    {
        const QString panelId = QStringLiteral("CodexTestPanelPlacementCorrupt");
        auto group = placementsRoot->GetGroup(panelId.toUtf8().constData());
        placementsRoot->SetInt(PanelPlacementStore::schemaVersionKey(), -3);
        group->SetASCII("Mode", "bad-mode");
        group->SetASCII("Edge", "none");
        group->SetASCII("Region", "bad-region");
        group->SetASCII("LauncherRail", "bad-rail");
        group->SetASCII("LauncherCluster", "bad-cluster");
        group->SetASCII("FloatingGeometry", "1,2,three,4");
        group->SetInt("GroupOrder", -8);
        group->SetInt("TabOrder", -2);
        group->SetInt("Extent", -11);
        group->SetInt("LauncherOrder", -6);

        PanelPlacement loaded;
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(panelId, &loaded));
        QCOMPARE(loaded.panelId, panelId);
        QCOMPARE(loaded.schemaVersion, PanelPlacementStore::CurrentSchemaVersion);
        QCOMPARE(loaded.mode, PanelPlacement::Mode::Docked);
        QCOMPARE(loaded.edge, PanelPlacement::Edge::Left);
        QCOMPARE(loaded.region, PanelPlacement::Region::Start);
        QCOMPARE(loaded.order, 0);
        QCOMPARE(loaded.visibilityPolicy, PanelPlacement::VisibilityPolicy::Exclusive);
        QCOMPARE(loaded.groupOrder, 0);
        QCOMPARE(loaded.tabOrder, 0);
        QCOMPARE(loaded.extent, 0);
        QCOMPARE(loaded.floatingGeometry, QRect());
        QCOMPARE(loaded.launcher.rail, PanelPlacement::Launcher::Rail::Left);
        QCOMPARE(loaded.launcher.cluster, PanelPlacement::Launcher::Cluster::Upper);
        QCOMPARE(loaded.launcher.order, 0);
    }

    void migrationPrecedence()  // NOLINT
    {
        const QString unifiedId = QStringLiteral("CodexTestPanelPlacementUnified");
        const QString compactId = QStringLiteral("CodexTestPanelPlacementCompact");
        const QString railsId = QStringLiteral("CodexTestPanelPlacementRails");

        PanelPlacement unified;
        unified.panelId = unifiedId;
        unified.mode = PanelPlacement::Mode::Overlay;
        unified.edge = PanelPlacement::Edge::Right;
        unified.region = PanelPlacement::Region::Center;
        unified.order = 3;
        QVERIFY(PanelPlacementStore::savePlacement(unified));
        compactSlots->SetASCII(unifiedId.toUtf8().constData(), "right-upper");

        compactSlots->SetASCII(compactId.toUtf8().constData(), "right-lower");
        panelRailsSlots->SetASCII(compactId.toUtf8().constData(), "left-upper");

        panelRailsSlots->SetASCII(railsId.toUtf8().constData(), "bottom-left");

        const PanelPlacement loadedUnified = PanelPlacementStore::loadPlacement(unifiedId);
        QCOMPARE(loadedUnified.mode, PanelPlacement::Mode::Overlay);
        QCOMPARE(loadedUnified.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loadedUnified.region, PanelPlacement::Region::Center);
        QCOMPARE(loadedUnified.order, 3);
        QCOMPARE(loadedUnified.visibilityPolicy, PanelPlacement::VisibilityPolicy::Exclusive);

        const PanelPlacement loadedCompact = PanelPlacementStore::loadPlacement(compactId);
        QCOMPARE(loadedCompact.mode, PanelPlacement::Mode::Docked);
        QCOMPARE(loadedCompact.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loadedCompact.region, PanelPlacement::Region::Center);
        QCOMPARE(loadedCompact.order, 0);
        QCOMPARE(loadedCompact.visibilityPolicy, PanelPlacement::VisibilityPolicy::Exclusive);

        const PanelPlacement loadedRails = PanelPlacementStore::loadPlacement(railsId);
        QCOMPARE(loadedRails.mode, PanelPlacement::Mode::Docked);
        QCOMPARE(loadedRails.edge, PanelPlacement::Edge::Bottom);
        QCOMPARE(loadedRails.region, PanelPlacement::Region::Start);
        QCOMPARE(loadedRails.order, 0);
        QCOMPARE(loadedRails.visibilityPolicy, PanelPlacement::VisibilityPolicy::Exclusive);
    }

    void partialUnifiedLauncherBackfillsFromLegacy()  // NOLINT
    {
        const QString panelId = QStringLiteral("CodexTestPanelPlacementPartialUnified");
        auto group = placementsRoot->GetGroup(panelId.toUtf8().constData());
        group->SetASCII("LauncherRail", "right");
        group->SetASCII("LauncherCluster", "bad-cluster");
        group->SetInt("LauncherOrder", 9);
        compactSlots->SetASCII(panelId.toUtf8().constData(), "left-lower");
        panelRailsSlots->SetASCII(panelId.toUtf8().constData(), "bottom-right");

        PanelPlacement loaded = PanelPlacementStore::loadPlacement(panelId);
        QCOMPARE(loaded.panelId, panelId);
        QCOMPARE(loaded.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loaded.region, PanelPlacement::Region::Center);
        QCOMPARE(loaded.order, 9);
        QCOMPARE(loaded.launcher.rail, PanelPlacement::Launcher::Rail::Right);
        QCOMPARE(loaded.launcher.cluster, PanelPlacement::Launcher::Cluster::Lower);
    }

    void legacyRetention()  // NOLINT
    {
        const QString panelId = QStringLiteral("CodexTestPanelPlacementRetention");
        compactSlots->SetASCII(panelId.toUtf8().constData(), "right-upper");
        panelRailsSlots->SetASCII(panelId.toUtf8().constData(), "bottom-left");

        PanelPlacement placement = PanelPlacementStore::loadPlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(placement));

        QCOMPARE(
            QString::fromUtf8(compactSlots->GetASCII(panelId.toUtf8().constData(), "").c_str()),
            QStringLiteral("right-upper")
        );
        QCOMPARE(
            QString::fromUtf8(panelRailsSlots->GetASCII(panelId.toUtf8().constData(), "").c_str()),
            QStringLiteral("bottom-left")
        );
    }

    void loadSaveLoadIsIdempotent()  // NOLINT
    {
        const QString panelId = QStringLiteral("CodexTestPanelPlacementIdempotent");
        compactSlots->SetASCII(panelId.toUtf8().constData(), "bottom-right");

        PanelPlacement first = PanelPlacementStore::loadPlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(first));
        PanelPlacement second = PanelPlacementStore::loadPlacement(panelId);
        QVERIFY(PanelPlacementStore::savePlacement(second));
        PanelPlacement third = PanelPlacementStore::loadPlacement(panelId);

        QCOMPARE(second, first);
        QCOMPARE(third, second);
    }

    void canonicalLocationDerivesCompatibilityLauncher()  // NOLINT
    {
        const std::array<
            std::tuple<PanelPlacement::Edge, PanelPlacement::Region, PanelPlacement::Launcher::Rail, PanelPlacement::Launcher::Cluster>,
            4>
            cases {{
                {PanelPlacement::Edge::Left,
                 PanelPlacement::Region::Start,
                 PanelPlacement::Launcher::Rail::Left,
                 PanelPlacement::Launcher::Cluster::Upper},
                {PanelPlacement::Edge::Right,
                 PanelPlacement::Region::Center,
                 PanelPlacement::Launcher::Rail::Right,
                 PanelPlacement::Launcher::Cluster::Lower},
                {PanelPlacement::Edge::Bottom,
                 PanelPlacement::Region::Start,
                 PanelPlacement::Launcher::Rail::Left,
                 PanelPlacement::Launcher::Cluster::Bottom},
                {PanelPlacement::Edge::Bottom,
                 PanelPlacement::Region::End,
                 PanelPlacement::Launcher::Rail::Right,
                 PanelPlacement::Launcher::Cluster::Bottom},
            }};

        int index = 0;
        for (const auto& [edge, region, expectedRail, expectedCluster] : cases) {
            PanelPlacement placement;
            placement.panelId = QStringLiteral("CodexTestPanelPlacementIndependent%1").arg(index++);
            placement.mode = PanelPlacement::Mode::Docked;
            placement.edge = edge;
            placement.region = region;
            placement.order = 4;

            QVERIFY(PanelPlacementStore::savePlacement(placement));

            PanelPlacement loaded;
            QVERIFY(PanelPlacementStore::loadUnifiedPlacement(placement.panelId, &loaded));
            QCOMPARE(loaded.mode, PanelPlacement::Mode::Docked);
            QCOMPARE(loaded.edge, edge);
            QCOMPARE(loaded.region, region);
            QCOMPARE(loaded.order, 4);
            QCOMPARE(loaded.launcher.rail, expectedRail);
            QCOMPARE(loaded.launcher.cluster, expectedCluster);
            QCOMPARE(loaded.launcher.order, 4);
        }
    }

    void v1LocationWinsButLauncherOrderMigrates()  // NOLINT
    {
        const QString panelId = QStringLiteral("CodexTestPanelPlacementV1Conflict");
        auto group = placementsRoot->GetGroup(panelId.toUtf8().constData());
        placementsRoot->SetInt(PanelPlacementStore::schemaVersionKey(), 1);
        group->SetASCII("Mode", "overlay");
        group->SetASCII("Edge", "right");
        group->SetASCII("Region", "end");
        group->SetASCII("LauncherRail", "left");
        group->SetASCII("LauncherCluster", "lower");
        group->SetInt("LauncherOrder", 11);

        const PanelPlacement loaded = PanelPlacementStore::loadPlacement(panelId);
        QCOMPARE(loaded.mode, PanelPlacement::Mode::Overlay);
        QCOMPARE(loaded.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loaded.region, PanelPlacement::Region::End);
        QCOMPARE(loaded.order, 11);
        QCOMPARE(loaded.visibilityPolicy, PanelPlacement::VisibilityPolicy::Exclusive);
        QCOMPARE(loaded.launcher.rail, PanelPlacement::Launcher::Rail::Right);
        QCOMPARE(loaded.launcher.cluster, PanelPlacement::Launcher::Cluster::Bottom);
        QCOMPARE(loaded.launcher.order, 11);
    }

    void loadPlacementsSkipsInvalidDormantGroups()  // NOLINT
    {
        PanelPlacement validA;
        validA.panelId = QStringLiteral("CodexTestPanelPlacementListedB");
        validA.launcher.order = 2;
        QVERIFY(PanelPlacementStore::savePlacement(validA));

        PanelPlacement validB;
        validB.panelId = QStringLiteral("CodexTestPanelPlacementListedA");
        validB.launcher.order = 1;
        QVERIFY(PanelPlacementStore::savePlacement(validB));

        placementsRoot->GetGroup("Translated title");
        placementsRoot->GetGroup("bad id");

        const QString migratedId = QStringLiteral("CodexTestPanelPlacementListedLegacy");
        placementsRoot->GetGroup(migratedId.toUtf8().constData());
        compactSlots->SetASCII(migratedId.toUtf8().constData(), "bottom-right");

        QList<QString> testIds;
        PanelPlacement migrated;
        const QList<PanelPlacement> placements = PanelPlacementStore::loadPlacements();
        for (const auto& placement : placements) {
            if (placement.panelId.startsWith(QStringLiteral("CodexTestPanelPlacementListed"))) {
                testIds.push_back(placement.panelId);
            }
            if (placement.panelId == migratedId) {
                migrated = placement;
            }
            QVERIFY(placement.panelId != QStringLiteral("Translated title"));
            QVERIFY(placement.panelId != QStringLiteral("bad id"));
        }

        QCOMPARE(testIds.size(), 3);
        QCOMPARE(testIds.at(0), QStringLiteral("CodexTestPanelPlacementListedA"));
        QCOMPARE(testIds.at(1), QStringLiteral("CodexTestPanelPlacementListedB"));
        QCOMPARE(testIds.at(2), migratedId);
        QCOMPARE(migrated.launcher.rail, PanelPlacement::Launcher::Rail::Right);
        QCOMPARE(migrated.launcher.cluster, PanelPlacement::Launcher::Cluster::Bottom);
    }

    void forwardSchemaVersionUsesSafeReadPolicy()  // NOLINT
    {
        const QString panelId = QStringLiteral("CodexTestPanelPlacementForwardSchema");
        placementsRoot->SetInt(PanelPlacementStore::schemaVersionKey(), 99);
        auto group = placementsRoot->GetGroup(panelId.toUtf8().constData());
        group->SetASCII("Mode", "overlay");
        group->SetASCII("Edge", "right");
        group->SetASCII("LauncherCluster", "lower");
        compactSlots->SetASCII(panelId.toUtf8().constData(), "left-upper");

        PanelPlacement loaded = PanelPlacementStore::loadPlacement(panelId);
        QCOMPARE(loaded.schemaVersion, 99);
        QCOMPARE(loaded.mode, PanelPlacement::Mode::Overlay);
        QCOMPARE(loaded.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loaded.region, PanelPlacement::Region::Start);
        QCOMPARE(loaded.order, 0);
        QCOMPARE(loaded.visibilityPolicy, PanelPlacement::VisibilityPolicy::Exclusive);
        QCOMPARE(loaded.launcher.rail, PanelPlacement::Launcher::Rail::Right);
        QCOMPARE(loaded.launcher.cluster, PanelPlacement::Launcher::Cluster::Upper);

        QVERIFY(PanelPlacementStore::savePlacement(loaded));
        QCOMPARE(PanelPlacementStore::schemaVersion(), PanelPlacementStore::CurrentSchemaVersion);
    }

private:
    void clearTestState()
    {
        const QStringList panelIds {
            QStringLiteral("CodexTestPanelPlacementRequestedId"),
            QStringLiteral("CodexTestPanelPlacementRoundtrip"),
            QStringLiteral("CodexTestPanelPlacementRoundtripSecond"),
            QStringLiteral("CodexTestPanelPlacementCorrupt"),
            QStringLiteral("CodexTestPanelPlacementUnified"),
            QStringLiteral("CodexTestPanelPlacementCompact"),
            QStringLiteral("CodexTestPanelPlacementRails"),
            QStringLiteral("CodexTestPanelPlacementPartialUnified"),
            QStringLiteral("CodexTestPanelPlacementRetention"),
            QStringLiteral("CodexTestPanelPlacementIdempotent"),
            QStringLiteral("CodexTestPanelPlacementIndependent0"),
            QStringLiteral("CodexTestPanelPlacementIndependent1"),
            QStringLiteral("CodexTestPanelPlacementIndependent2"),
            QStringLiteral("CodexTestPanelPlacementIndependent3"),
            QStringLiteral("CodexTestPanelPlacementV1Conflict"),
            QStringLiteral("CodexTestPanelPlacementListedA"),
            QStringLiteral("CodexTestPanelPlacementListedB"),
            QStringLiteral("CodexTestPanelPlacementListedLegacy"),
            QStringLiteral("CodexTestPanelPlacementForwardSchema"),
        };

        for (const auto& panelId : panelIds) {
            placementsRoot->RemoveGrp(panelId.toUtf8().constData());
            compactSlots->RemoveASCII(panelId.toUtf8().constData());
            panelRailsSlots->RemoveASCII(panelId.toUtf8().constData());
        }

        placementsRoot->RemoveGrp("Translated title");
        placementsRoot->RemoveGrp("bad id");
    }

    ParameterGrp::handle placementsRoot;
    ParameterGrp::handle compactSlots;
    ParameterGrp::handle panelRailsSlots;
    ExactKeyBackup backups;
};

QTEST_MAIN(testPanelPlacement)

#include "PanelPlacement.moc"
