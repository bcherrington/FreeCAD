// SPDX-License-Identifier: LGPL-2.1-or-later

#include <array>

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
        PanelPlacement placement;
        placement.panelId = QStringLiteral("CodexTestPanelPlacementRoundtrip");
        placement.mode = PanelPlacement::Mode::Overlay;
        placement.edge = PanelPlacement::Edge::Right;
        placement.region = PanelPlacement::Region::End;
        placement.groupId = QStringLiteral("main-right");
        placement.groupOrder = 5;
        placement.tabOrder = 2;
        placement.splitRelation = QStringLiteral("after:Std_TaskView");
        placement.extent = 240;
        placement.floatingGeometry = QRect(100, 120, 640, 480);
        placement.launcher.rail = PanelPlacement::Launcher::Rail::Left;
        placement.launcher.cluster = PanelPlacement::Launcher::Cluster::Bottom;
        placement.launcher.order = 7;

        QVERIFY(PanelPlacementStore::savePlacement(placement));
        QCOMPARE(PanelPlacementStore::schemaVersion(), PanelPlacementStore::CurrentSchemaVersion);
        QVERIFY(PanelPlacementStore::migrationMarker());

        PanelPlacement loaded;
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(placement.panelId, &loaded));

        PanelPlacement expected = placement;
        expected.normalize();
        QCOMPARE(loaded, expected);
    }

    void mismatchedFallbackIdUsesRequestedId()  // NOLINT
    {
        PanelPlacement fallback;
        fallback.panelId = QStringLiteral("WrongFallbackId");
        fallback.edge = PanelPlacement::Edge::Right;
        fallback.launcher.cluster = PanelPlacement::Launcher::Cluster::Lower;

        const QString requestedId = QStringLiteral("CodexTestPanelPlacementRequestedId");
        PanelPlacement loaded = PanelPlacementStore::loadPlacement(requestedId, fallback);
        QCOMPARE(loaded.panelId, requestedId);
        QCOMPARE(loaded.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loaded.launcher.cluster, PanelPlacement::Launcher::Cluster::Lower);
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
        unified.launcher.rail = PanelPlacement::Launcher::Rail::Left;
        unified.launcher.cluster = PanelPlacement::Launcher::Cluster::Lower;
        unified.launcher.order = 3;
        QVERIFY(PanelPlacementStore::savePlacement(unified));
        compactSlots->SetASCII(unifiedId.toUtf8().constData(), "right-upper");

        compactSlots->SetASCII(compactId.toUtf8().constData(), "right-lower");
        panelRailsSlots->SetASCII(compactId.toUtf8().constData(), "left-upper");

        panelRailsSlots->SetASCII(railsId.toUtf8().constData(), "bottom-left");

        QCOMPARE(
            PanelPlacementStore::loadPlacement(unifiedId).launcher.cluster,
            PanelPlacement::Launcher::Cluster::Lower
        );
        QCOMPARE(
            PanelPlacementStore::loadPlacement(unifiedId).launcher.rail,
            PanelPlacement::Launcher::Rail::Left
        );

        QCOMPARE(
            PanelPlacementStore::loadPlacement(compactId).launcher.rail,
            PanelPlacement::Launcher::Rail::Right
        );
        QCOMPARE(
            PanelPlacementStore::loadPlacement(compactId).launcher.cluster,
            PanelPlacement::Launcher::Cluster::Lower
        );

        QCOMPARE(
            PanelPlacementStore::loadPlacement(railsId).launcher.rail,
            PanelPlacement::Launcher::Rail::Left
        );
        QCOMPARE(
            PanelPlacementStore::loadPlacement(railsId).launcher.cluster,
            PanelPlacement::Launcher::Cluster::Bottom
        );
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
        QCOMPARE(loaded.launcher.rail, PanelPlacement::Launcher::Rail::Right);
        QCOMPARE(loaded.launcher.cluster, PanelPlacement::Launcher::Cluster::Lower);
        QCOMPARE(loaded.launcher.order, 9);
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

    void launcherPlacementIsIndependent()  // NOLINT
    {
        PanelPlacement placement;
        placement.panelId = QStringLiteral("CodexTestPanelPlacementIndependent");
        placement.mode = PanelPlacement::Mode::Docked;
        placement.edge = PanelPlacement::Edge::Right;
        placement.region = PanelPlacement::Region::Center;
        placement.launcher.rail = PanelPlacement::Launcher::Rail::Left;
        placement.launcher.cluster = PanelPlacement::Launcher::Cluster::Bottom;
        placement.launcher.order = 4;

        QVERIFY(PanelPlacementStore::savePlacement(placement));

        PanelPlacement loaded;
        QVERIFY(PanelPlacementStore::loadUnifiedPlacement(placement.panelId, &loaded));
        QCOMPARE(loaded.mode, PanelPlacement::Mode::Docked);
        QCOMPARE(loaded.edge, PanelPlacement::Edge::Right);
        QCOMPARE(loaded.region, PanelPlacement::Region::Center);
        QCOMPARE(loaded.launcher.rail, PanelPlacement::Launcher::Rail::Left);
        QCOMPARE(loaded.launcher.cluster, PanelPlacement::Launcher::Cluster::Bottom);
        QCOMPARE(loaded.launcher.order, 4);
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
        QCOMPARE(loaded.launcher.rail, PanelPlacement::Launcher::Rail::Left);
        QCOMPARE(loaded.launcher.cluster, PanelPlacement::Launcher::Cluster::Lower);

        QVERIFY(PanelPlacementStore::savePlacement(loaded));
        QCOMPARE(PanelPlacementStore::schemaVersion(), PanelPlacementStore::CurrentSchemaVersion);
    }

private:
    void clearTestState()
    {
        const QStringList panelIds {
            QStringLiteral("CodexTestPanelPlacementRequestedId"),
            QStringLiteral("CodexTestPanelPlacementRoundtrip"),
            QStringLiteral("CodexTestPanelPlacementCorrupt"),
            QStringLiteral("CodexTestPanelPlacementUnified"),
            QStringLiteral("CodexTestPanelPlacementCompact"),
            QStringLiteral("CodexTestPanelPlacementRails"),
            QStringLiteral("CodexTestPanelPlacementPartialUnified"),
            QStringLiteral("CodexTestPanelPlacementRetention"),
            QStringLiteral("CodexTestPanelPlacementIdempotent"),
            QStringLiteral("CodexTestPanelPlacementIndependent"),
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
