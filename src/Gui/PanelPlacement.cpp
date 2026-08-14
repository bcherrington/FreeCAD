/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Library General Public License for more details.                      *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include "PanelPlacement.h"

#include <algorithm>
#include <array>
#include <utility>

#include <QByteArray>
#include <QStringList>

#include <App/Application.h>
#include <Base/Parameter.h>

using namespace Gui;

namespace
{
constexpr auto MainWindowPreferencesPath = "User parameter:BaseApp/Preferences/MainWindow";
constexpr auto PlacementsPath = "User parameter:BaseApp/Preferences/MainWindow/PanelPlacements";
constexpr auto CompactLegacySlotsPath
    = "User parameter:BaseApp/Preferences/MainWindow/CompactJetBrainsPanelSlots";
constexpr auto PanelRailsLegacySlotsPath
    = "User parameter:BaseApp/Preferences/MainWindow/PanelRailsSlots";
constexpr auto FeatureFlagKey = "CompactJetBrainsPanelPlacementEnabled";
constexpr auto SchemaVersionKey = "SchemaVersion";
constexpr auto MigrationMarkerKey = "MigrationMarker";
constexpr auto ModeKey = "Mode";
constexpr auto EdgeKey = "Edge";
constexpr auto RegionKey = "Region";
constexpr auto OrderKey = "Order";
constexpr auto VisibilityPolicyKey = "VisibilityPolicy";
constexpr auto GroupIdKey = "GroupId";
constexpr auto GroupOrderKey = "GroupOrder";
constexpr auto TabOrderKey = "TabOrder";
constexpr auto SplitRelationKey = "SplitRelation";
constexpr auto ExtentKey = "Extent";
constexpr auto FloatingGeometryKey = "FloatingGeometry";
constexpr auto LauncherRailKey = "LauncherRail";
constexpr auto LauncherClusterKey = "LauncherCluster";
constexpr auto LauncherOrderKey = "LauncherOrder";

template<typename Enum>
struct EnumString
{
    Enum value;
    const char* text;
};

using Launcher = PanelPlacement::Launcher;

const std::array<EnumString<PanelPlacement::Mode>, 4> modeStrings {{
    {PanelPlacement::Mode::Docked, "docked"},
    {PanelPlacement::Mode::Overlay, "overlay"},
    {PanelPlacement::Mode::AutoHide, "auto-hide"},
    {PanelPlacement::Mode::Floating, "floating"},
}};

const std::array<EnumString<PanelPlacement::Edge>, 5> edgeStrings {{
    {PanelPlacement::Edge::Left, "left"},
    {PanelPlacement::Edge::Right, "right"},
    {PanelPlacement::Edge::Top, "top"},
    {PanelPlacement::Edge::Bottom, "bottom"},
    {PanelPlacement::Edge::None, "none"},
}};

const std::array<EnumString<PanelPlacement::Region>, 3> regionStrings {{
    {PanelPlacement::Region::Start, "start"},
    {PanelPlacement::Region::Center, "center"},
    {PanelPlacement::Region::End, "end"},
}};

const std::array<EnumString<PanelPlacement::VisibilityPolicy>, 2> visibilityPolicyStrings {{
    {PanelPlacement::VisibilityPolicy::Exclusive, "exclusive"},
    {PanelPlacement::VisibilityPolicy::Multiple, "multiple"},
}};

const std::array<EnumString<Launcher::Rail>, 2> railStrings {{
    {Launcher::Rail::Left, "left"},
    {Launcher::Rail::Right, "right"},
}};

const std::array<EnumString<Launcher::Cluster>, 3> clusterStrings {{
    {Launcher::Cluster::Upper, "upper"},
    {Launcher::Cluster::Lower, "lower"},
    {Launcher::Cluster::Bottom, "bottom"},
}};

const std::array<std::pair<const char*, std::pair<Launcher::Rail, Launcher::Cluster>>, 6>
    legacyLauncherSlots {{
        {"left-upper", {Launcher::Rail::Left, Launcher::Cluster::Upper}},
        {"left-lower", {Launcher::Rail::Left, Launcher::Cluster::Lower}},
        {"right-upper", {Launcher::Rail::Right, Launcher::Cluster::Upper}},
        {"right-lower", {Launcher::Rail::Right, Launcher::Cluster::Lower}},
        {"bottom-left", {Launcher::Rail::Left, Launcher::Cluster::Bottom}},
        {"bottom-right", {Launcher::Rail::Right, Launcher::Cluster::Bottom}},
    }};

template<typename Enum, std::size_t Size>
QString enumToString(const std::array<EnumString<Enum>, Size>& values, Enum value)
{
    for (const auto& entry : values) {
        if (entry.value == value) {
            return QString::fromLatin1(entry.text);
        }
    }

    return {};
}

template<typename Enum, std::size_t Size>
bool enumFromString(const std::array<EnumString<Enum>, Size>& values, const QString& text, Enum* value)
{
    if (!value) {
        return false;
    }

    for (const auto& entry : values) {
        if (text == QLatin1String(entry.text)) {
            *value = entry.value;
            return true;
        }
    }

    return false;
}

ParameterGrp::handle preferencesGroup(const char* path)
{
    return App::GetApplication().GetParameterGroupByPath(path);
}

bool hasIntEntry(ParameterGrp::handle group, const char* key, long* value = nullptr)
{
    if (!group || !key) {
        return false;
    }

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

bool hasAsciiEntry(ParameterGrp::handle group, const char* key, QString* value = nullptr)
{
    if (!group || !key) {
        return false;
    }

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

QString fromAscii(const std::string& value)
{
    return QString::fromUtf8(value.c_str());
}

QByteArray stableIdBytes(const QString& panelId)
{
    return panelId.toUtf8();
}

bool isAsciiStableIdChar(QChar c)
{
    const ushort code = c.unicode();
    const bool asciiLetter = (code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z');
    const bool asciiDigit = code >= '0' && code <= '9';
    switch (code) {
        case '_':
        case '-':
        case '.':
        case ':':
            return true;
        default:
            return asciiLetter || asciiDigit;
    }
}

QString encodeRect(const QRect& rect)
{
    return QStringLiteral("%1,%2,%3,%4").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
}

bool decodeRect(const QString& encoded, QRect* rect)
{
    if (!rect) {
        return false;
    }

    const QStringList parts = encoded.split(QLatin1Char(','));
    if (parts.size() != 4) {
        return false;
    }

    bool okX = false;
    bool okY = false;
    bool okWidth = false;
    bool okHeight = false;
    const int x = parts.at(0).toInt(&okX);
    const int y = parts.at(1).toInt(&okY);
    const int width = parts.at(2).toInt(&okWidth);
    const int height = parts.at(3).toInt(&okHeight);
    if (!okX || !okY || !okWidth || !okHeight || width < 0 || height < 0) {
        return false;
    }

    *rect = QRect(x, y, width, height);
    return true;
}

bool legacyLauncherFromSlotName(const QString& slotName, Launcher* launcher)
{
    if (!launcher) {
        return false;
    }

    for (const auto& [name, placement] : legacyLauncherSlots) {
        if (slotName == QLatin1String(name)) {
            launcher->rail = placement.first;
            launcher->cluster = placement.second;
            launcher->normalize();
            return true;
        }
    }

    return false;
}

void applyCompatibilityLauncherFromLocation(PanelPlacement* placement)
{
    if (!placement) {
        return;
    }

    placement->launcher.order = placement->order;
    switch (placement->edge) {
        case PanelPlacement::Edge::Left:
            placement->launcher.rail = Launcher::Rail::Left;
            placement->launcher.cluster = placement->region == PanelPlacement::Region::Start
                ? Launcher::Cluster::Upper
                : placement->region == PanelPlacement::Region::End ? Launcher::Cluster::Bottom
                                                                   : Launcher::Cluster::Lower;
            break;
        case PanelPlacement::Edge::Right:
            placement->launcher.rail = Launcher::Rail::Right;
            placement->launcher.cluster = placement->region == PanelPlacement::Region::Start
                ? Launcher::Cluster::Upper
                : placement->region == PanelPlacement::Region::End ? Launcher::Cluster::Bottom
                                                                   : Launcher::Cluster::Lower;
            break;
        case PanelPlacement::Edge::Bottom:
            placement->launcher.cluster = Launcher::Cluster::Bottom;
            placement->launcher.rail = placement->region == PanelPlacement::Region::End
                ? Launcher::Rail::Right
                : Launcher::Rail::Left;
            break;
        case PanelPlacement::Edge::Top:
            placement->launcher.cluster = Launcher::Cluster::Upper;
            placement->launcher.rail = placement->region == PanelPlacement::Region::End
                ? Launcher::Rail::Right
                : Launcher::Rail::Left;
            break;
        case PanelPlacement::Edge::None:
            placement->launcher.rail = Launcher::Rail::Left;
            placement->launcher.cluster = Launcher::Cluster::Upper;
            break;
    }

    placement->launcher.normalize();
}

bool applyLocationFromLauncher(
    PanelPlacement* placement,
    Launcher::Rail rail,
    Launcher::Cluster cluster,
    int order,
    bool overwriteLocation
)
{
    if (!placement) {
        return false;
    }

    if (overwriteLocation) {
        placement->mode = PanelPlacement::Mode::Docked;
    }

    switch (cluster) {
        case Launcher::Cluster::Upper:
            if (overwriteLocation) {
                placement->edge = rail == Launcher::Rail::Right ? PanelPlacement::Edge::Right
                                                                : PanelPlacement::Edge::Left;
                placement->region = PanelPlacement::Region::Start;
            }
            break;
        case Launcher::Cluster::Lower:
            if (overwriteLocation) {
                placement->edge = rail == Launcher::Rail::Right ? PanelPlacement::Edge::Right
                                                                : PanelPlacement::Edge::Left;
                placement->region = PanelPlacement::Region::Center;
            }
            break;
        case Launcher::Cluster::Bottom:
            if (overwriteLocation) {
                placement->edge = PanelPlacement::Edge::Bottom;
                placement->region = rail == Launcher::Rail::Right ? PanelPlacement::Region::End
                                                                  : PanelPlacement::Region::Start;
            }
            break;
    }

    placement->order = order;
    placement->launcher.rail = rail;
    placement->launcher.cluster = cluster;
    placement->launcher.order = order;
    placement->launcher.normalize();
    return true;
}

struct PlacementFieldValidity
{
    bool mode = false;
    bool edge = false;
    bool region = false;
    bool order = false;
    bool visibilityPolicy = false;
    bool groupOrder = false;
    bool launcherRail = false;
    bool launcherCluster = false;
    bool launcherOrder = false;
};

PlacementFieldValidity loadPlacementFields(ParameterGrp::handle group, PanelPlacement* placement)
{
    PlacementFieldValidity validity;
    if (!group || !placement) {
        return validity;
    }

    PanelPlacement loaded;
    loaded.panelId = placement->panelId;
    loaded.schemaVersion = PanelPlacementStore::schemaVersion();

    QString mode;
    validity.mode = hasAsciiEntry(group, ModeKey, &mode)
        && panelPlacementModeFromString(mode, &loaded.mode);

    QString edge;
    validity.edge = hasAsciiEntry(group, EdgeKey, &edge)
        && panelPlacementEdgeFromString(edge, &loaded.edge);

    QString region;
    validity.region = hasAsciiEntry(group, RegionKey, &region)
        && panelPlacementRegionFromString(region, &loaded.region);

    long order = 0;
    validity.order = hasIntEntry(group, OrderKey, &order);
    if (validity.order) {
        loaded.order = static_cast<int>(order);
    }

    QString visibilityPolicy;
    validity.visibilityPolicy = hasAsciiEntry(group, VisibilityPolicyKey, &visibilityPolicy)
        && panelPlacementVisibilityPolicyFromString(visibilityPolicy, &loaded.visibilityPolicy);

    loaded.groupId = fromAscii(group->GetASCII(GroupIdKey, ""));
    long groupOrder = 0;
    validity.groupOrder = hasIntEntry(group, GroupOrderKey, &groupOrder);
    loaded.groupOrder = static_cast<int>(groupOrder);
    loaded.tabOrder = static_cast<int>(group->GetInt(TabOrderKey, 0));
    loaded.splitRelation = fromAscii(group->GetASCII(SplitRelationKey, ""));
    loaded.extent = static_cast<int>(group->GetInt(ExtentKey, 0));

    QRect floatingGeometry;
    if (decodeRect(fromAscii(group->GetASCII(FloatingGeometryKey, "")), &floatingGeometry)) {
        loaded.floatingGeometry = floatingGeometry;
    }

    QString rail;
    validity.launcherRail = hasAsciiEntry(group, LauncherRailKey, &rail)
        && panelPlacementRailFromString(rail, &loaded.launcher.rail);

    QString cluster;
    validity.launcherCluster = hasAsciiEntry(group, LauncherClusterKey, &cluster)
        && panelPlacementClusterFromString(cluster, &loaded.launcher.cluster);

    long launcherOrder = 0;
    validity.launcherOrder = hasIntEntry(group, LauncherOrderKey, &launcherOrder);
    if (validity.launcherOrder) {
        loaded.launcher.order = static_cast<int>(launcherOrder);
    }
    *placement = loaded;
    return validity;
}

PanelPlacement migrateLoadedPlacement(
    const QString& panelId,
    const PanelPlacement& fallback,
    const PlacementFieldValidity& validity,
    const PanelPlacement& rawPlacement
)
{
    PanelPlacement placement = rawPlacement;
    const Launcher fallbackLauncher = fallback.launcher;

    if (!validity.order) {
        if (validity.groupOrder) {
            placement.order = rawPlacement.groupOrder;
        }
        else if (validity.launcherOrder) {
            placement.order = rawPlacement.launcher.order;
        }
        else {
            placement.order = fallback.order;
        }
    }

    if (!validity.visibilityPolicy) {
        placement.visibilityPolicy = PanelPlacement::VisibilityPolicy::Exclusive;
    }

    if (!validity.region) {
        placement.region = fallback.region;
    }

    // A legacy launcher may supply a missing location, but it must not replace a
    // valid persisted mode/edge pair merely because schema v1 did not store Region.
    const bool needsLocationFromLauncher = !(validity.mode && validity.edge);
    if (needsLocationFromLauncher) {
        if (validity.launcherRail && validity.launcherCluster) {
            applyLocationFromLauncher(
                &placement,
                rawPlacement.launcher.rail,
                rawPlacement.launcher.cluster,
                validity.launcherOrder ? rawPlacement.launcher.order : placement.order,
                true
            );
        }
        else {
            Launcher legacyLauncher = rawPlacement.launcher;
            if (!validity.launcherRail) {
                legacyLauncher.rail = fallbackLauncher.rail;
            }
            if (!validity.launcherCluster) {
                legacyLauncher.cluster = fallbackLauncher.cluster;
            }
            if (!validity.launcherOrder) {
                legacyLauncher.order = placement.order;
            }
            Launcher importedLegacyLauncher = legacyLauncher;
            if (PanelPlacementStore::importLegacyLauncher(panelId, &importedLegacyLauncher)) {
                if (!validity.launcherRail) {
                    legacyLauncher.rail = importedLegacyLauncher.rail;
                }
                if (!validity.launcherCluster) {
                    legacyLauncher.cluster = importedLegacyLauncher.cluster;
                }
            }
            else {
                legacyLauncher.normalize();
            }
            applyLocationFromLauncher(
                &placement,
                legacyLauncher.rail,
                legacyLauncher.cluster,
                legacyLauncher.order,
                true
            );
        }
    }

    placement.panelId = panelId;
    placement.normalize();
    return placement;
}

}  // namespace

void PanelPlacement::Launcher::normalize()
{
    order = std::max(order, 0);
}

bool PanelPlacement::Launcher::operator==(const Launcher& other) const
{
    return rail == other.rail && cluster == other.cluster && order == other.order;
}

void PanelPlacement::normalize()
{
    if (schemaVersion <= 0) {
        schemaVersion = CurrentSchemaVersion;
    }
    panelId = panelId.trimmed();
    order = std::max(order, 0);
    groupOrder = std::max(groupOrder, 0);
    tabOrder = std::max(tabOrder, 0);
    extent = std::max(extent, 0);
    launcher.normalize();

    if (floatingGeometry.width() < 0 || floatingGeometry.height() < 0) {
        floatingGeometry = QRect();
    }

    if (mode == Mode::Floating) {
        edge = Edge::None;
    }
    else if (edge == Edge::None) {
        mode = Mode::Docked;
        edge = Edge::Left;
    }

    applyCompatibilityLauncherFromLocation(this);
}

bool PanelPlacement::hasStablePanelId() const
{
    return isStablePanelId(panelId);
}

bool PanelPlacement::operator==(const PanelPlacement& other) const
{
    return schemaVersion == other.schemaVersion && panelId == other.panelId && mode == other.mode
        && edge == other.edge && region == other.region && order == other.order
        && visibilityPolicy == other.visibilityPolicy && groupId == other.groupId
        && groupOrder == other.groupOrder && tabOrder == other.tabOrder
        && splitRelation == other.splitRelation && extent == other.extent
        && floatingGeometry == other.floatingGeometry && launcher == other.launcher;
}

bool Gui::isStablePanelId(const QString& panelId)
{
    if (panelId.isEmpty() || panelId.trimmed() != panelId) {
        return false;
    }

    for (const auto& c : panelId) {
        if (!isAsciiStableIdChar(c)) {
            return false;
        }
    }

    return true;
}

QString Gui::panelPlacementModeToString(PanelPlacement::Mode mode)
{
    return enumToString(modeStrings, mode);
}

bool Gui::panelPlacementModeFromString(const QString& value, PanelPlacement::Mode* mode)
{
    return enumFromString(modeStrings, value, mode);
}

QString Gui::panelPlacementEdgeToString(PanelPlacement::Edge edge)
{
    return enumToString(edgeStrings, edge);
}

bool Gui::panelPlacementEdgeFromString(const QString& value, PanelPlacement::Edge* edge)
{
    return enumFromString(edgeStrings, value, edge);
}

QString Gui::panelPlacementRegionToString(PanelPlacement::Region region)
{
    return enumToString(regionStrings, region);
}

bool Gui::panelPlacementRegionFromString(const QString& value, PanelPlacement::Region* region)
{
    return enumFromString(regionStrings, value, region);
}

QString Gui::panelPlacementVisibilityPolicyToString(PanelPlacement::VisibilityPolicy policy)
{
    return enumToString(visibilityPolicyStrings, policy);
}

bool Gui::panelPlacementVisibilityPolicyFromString(
    const QString& value,
    PanelPlacement::VisibilityPolicy* policy
)
{
    return enumFromString(visibilityPolicyStrings, value, policy);
}

QString Gui::panelPlacementRailToString(Launcher::Rail rail)
{
    return enumToString(railStrings, rail);
}

bool Gui::panelPlacementRailFromString(const QString& value, Launcher::Rail* rail)
{
    return enumFromString(railStrings, value, rail);
}

QString Gui::panelPlacementClusterToString(Launcher::Cluster cluster)
{
    return enumToString(clusterStrings, cluster);
}

bool Gui::panelPlacementClusterFromString(const QString& value, Launcher::Cluster* cluster)
{
    return enumFromString(clusterStrings, value, cluster);
}

const char* PanelPlacementStore::mainWindowPreferencesPath()
{
    return MainWindowPreferencesPath;
}

const char* PanelPlacementStore::placementsPath()
{
    return PlacementsPath;
}

const char* PanelPlacementStore::compactLegacySlotsPath()
{
    return CompactLegacySlotsPath;
}

const char* PanelPlacementStore::panelRailsLegacySlotsPath()
{
    return PanelRailsLegacySlotsPath;
}

const char* PanelPlacementStore::featureFlagKey()
{
    return FeatureFlagKey;
}

const char* PanelPlacementStore::schemaVersionKey()
{
    return SchemaVersionKey;
}

const char* PanelPlacementStore::migrationMarkerKey()
{
    return MigrationMarkerKey;
}

bool PanelPlacementStore::isFeatureEnabled()
{
    auto group = preferencesGroup(mainWindowPreferencesPath());
    return group->GetBool(featureFlagKey(), false);
}

int PanelPlacementStore::schemaVersion()
{
    auto root = preferencesGroup(placementsPath());
    const long version = root->GetInt(schemaVersionKey(), CurrentSchemaVersion);
    return version > 0 ? static_cast<int>(version) : CurrentSchemaVersion;
}

bool PanelPlacementStore::migrationMarker()
{
    auto root = preferencesGroup(placementsPath());
    return root->GetBool(migrationMarkerKey(), false);
}

void PanelPlacementStore::setMigrationMarker(bool enabled)
{
    auto root = preferencesGroup(placementsPath());
    root->SetBool(migrationMarkerKey(), enabled);
}

bool PanelPlacementStore::hasPlacement(const QString& panelId)
{
    if (!isStablePanelId(panelId)) {
        return false;
    }

    auto root = preferencesGroup(placementsPath());
    const QByteArray groupName = stableIdBytes(panelId);
    return root->HasGroup(groupName.constData());
}

bool PanelPlacementStore::loadUnifiedPlacement(const QString& panelId, PanelPlacement* placement)
{
    if (!placement || !isStablePanelId(panelId)) {
        return false;
    }

    auto root = preferencesGroup(placementsPath());
    const QByteArray groupName = stableIdBytes(panelId);
    if (!root->HasGroup(groupName.constData())) {
        return false;
    }

    PanelPlacement rawPlacement;
    rawPlacement.panelId = panelId;
    const PlacementFieldValidity validity
        = loadPlacementFields(root->GetGroup(groupName.constData()), &rawPlacement);
    *placement = migrateLoadedPlacement(panelId, PanelPlacement(), validity, rawPlacement);
    return true;
}

PanelPlacement PanelPlacementStore::loadPlacement(const QString& panelId, const PanelPlacement& fallback)
{
    PanelPlacement placement = fallback;
    placement.panelId = panelId;

    auto root = preferencesGroup(placementsPath());
    const QByteArray groupName = stableIdBytes(panelId);
    if (root->HasGroup(groupName.constData())) {
        PanelPlacement rawPlacement = placement;
        rawPlacement.panelId = panelId;
        const PlacementFieldValidity validity
            = loadPlacementFields(root->GetGroup(groupName.constData()), &rawPlacement);
        return migrateLoadedPlacement(panelId, fallback, validity, rawPlacement);
    }

    PanelPlacement::Launcher legacyLauncher = placement.launcher;
    if (importLegacyLauncher(panelId, &legacyLauncher)) {
        applyLocationFromLauncher(
            &placement,
            legacyLauncher.rail,
            legacyLauncher.cluster,
            legacyLauncher.order,
            true
        );
        placement.visibilityPolicy = PanelPlacement::VisibilityPolicy::Exclusive;
    }

    placement.normalize();
    return placement;
}

QList<PanelPlacement> PanelPlacementStore::loadPlacements()
{
    QList<PanelPlacement> placements;
    auto root = preferencesGroup(placementsPath());
    for (const auto& group : root->GetGroups()) {
        const QString panelId = QString::fromLatin1(group->GetGroupName());
        if (!isStablePanelId(panelId)) {
            continue;
        }

        placements.push_back(loadPlacement(panelId));
    }

    std::sort(
        placements.begin(),
        placements.end(),
        [](const PanelPlacement& left, const PanelPlacement& right) {
            return left.panelId < right.panelId;
        }
    );
    return placements;
}

bool PanelPlacementStore::savePlacement(const PanelPlacement& placement)
{
    PanelPlacement normalized = placement;
    normalized.schemaVersion = CurrentSchemaVersion;
    normalized.normalize();
    if (!normalized.hasStablePanelId()) {
        return false;
    }

    auto root = preferencesGroup(placementsPath());
    root->SetInt(schemaVersionKey(), CurrentSchemaVersion);
    root->SetBool(migrationMarkerKey(), true);

    const QByteArray groupName = stableIdBytes(normalized.panelId);
    auto group = root->GetGroup(groupName.constData());
    group->SetASCII(ModeKey, panelPlacementModeToString(normalized.mode).toUtf8().constData());
    group->SetASCII(EdgeKey, panelPlacementEdgeToString(normalized.edge).toUtf8().constData());
    group->SetASCII(RegionKey, panelPlacementRegionToString(normalized.region).toUtf8().constData());
    group->SetInt(OrderKey, normalized.order);
    group->SetASCII(
        VisibilityPolicyKey,
        panelPlacementVisibilityPolicyToString(normalized.visibilityPolicy).toUtf8().constData()
    );

    if (normalized.groupId.isEmpty()) {
        group->RemoveASCII(GroupIdKey);
    }
    else {
        group->SetASCII(GroupIdKey, normalized.groupId.toUtf8().constData());
    }

    group->SetInt(GroupOrderKey, normalized.groupOrder);
    group->SetInt(TabOrderKey, normalized.tabOrder);

    if (normalized.splitRelation.isEmpty()) {
        group->RemoveASCII(SplitRelationKey);
    }
    else {
        group->SetASCII(SplitRelationKey, normalized.splitRelation.toUtf8().constData());
    }

    group->SetInt(ExtentKey, normalized.extent);

    if (normalized.floatingGeometry.isValid() && !normalized.floatingGeometry.isEmpty()) {
        group->SetASCII(
            FloatingGeometryKey,
            encodeRect(normalized.floatingGeometry).toUtf8().constData()
        );
    }
    else {
        group->RemoveASCII(FloatingGeometryKey);
    }

    // Schema-v1 launcher keys are intentionally left untouched when present so the
    // default-off experimental feature can still roll back to the legacy reader. New
    // schema-v2 groups never create them.
    return true;
}

bool PanelPlacementStore::removePlacement(const QString& panelId)
{
    if (!isStablePanelId(panelId)) {
        return false;
    }

    auto root = preferencesGroup(placementsPath());
    const QByteArray groupName = stableIdBytes(panelId);
    if (!root->HasGroup(groupName.constData())) {
        return false;
    }

    root->RemoveGrp(groupName.constData());
    return true;
}

bool PanelPlacementStore::importLegacyLauncher(const QString& panelId, Launcher* launcher)
{
    if (!launcher || !isStablePanelId(panelId)) {
        return false;
    }

    const QByteArray key = stableIdBytes(panelId);
    const auto tryGroup = [&key, launcher](const char* path) {
        auto group = preferencesGroup(path);
        return legacyLauncherFromSlotName(fromAscii(group->GetASCII(key.constData(), "")), launcher);
    };

    if (tryGroup(compactLegacySlotsPath())) {
        return true;
    }

    return tryGroup(panelRailsLegacySlotsPath());
}
