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

struct LauncherValidity
{
    bool rail = false;
    bool cluster = false;
};

LauncherValidity loadPlacementFields(ParameterGrp::handle group, PanelPlacement* placement)
{
    LauncherValidity validity;
    if (!group || !placement) {
        return validity;
    }

    PanelPlacement loaded;
    loaded.panelId = placement->panelId;
    loaded.schemaVersion = PanelPlacementStore::schemaVersion();

    const QString mode = fromAscii(group->GetASCII(ModeKey, ""));
    (void)panelPlacementModeFromString(mode, &loaded.mode);

    const QString edge = fromAscii(group->GetASCII(EdgeKey, ""));
    (void)panelPlacementEdgeFromString(edge, &loaded.edge);

    const QString region = fromAscii(group->GetASCII(RegionKey, ""));
    (void)panelPlacementRegionFromString(region, &loaded.region);

    loaded.groupId = fromAscii(group->GetASCII(GroupIdKey, ""));
    loaded.groupOrder = static_cast<int>(group->GetInt(GroupOrderKey, 0));
    loaded.tabOrder = static_cast<int>(group->GetInt(TabOrderKey, 0));
    loaded.splitRelation = fromAscii(group->GetASCII(SplitRelationKey, ""));
    loaded.extent = static_cast<int>(group->GetInt(ExtentKey, 0));

    QRect floatingGeometry;
    if (decodeRect(fromAscii(group->GetASCII(FloatingGeometryKey, "")), &floatingGeometry)) {
        loaded.floatingGeometry = floatingGeometry;
    }

    const QString rail = fromAscii(group->GetASCII(LauncherRailKey, ""));
    validity.rail = panelPlacementRailFromString(rail, &loaded.launcher.rail);

    const QString cluster = fromAscii(group->GetASCII(LauncherClusterKey, ""));
    validity.cluster = panelPlacementClusterFromString(cluster, &loaded.launcher.cluster);

    loaded.launcher.order = static_cast<int>(group->GetInt(LauncherOrderKey, 0));
    loaded.normalize();
    *placement = loaded;
    return validity;
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
}

bool PanelPlacement::hasStablePanelId() const
{
    return isStablePanelId(panelId);
}

bool PanelPlacement::operator==(const PanelPlacement& other) const
{
    return schemaVersion == other.schemaVersion && panelId == other.panelId && mode == other.mode
        && edge == other.edge && region == other.region && groupId == other.groupId
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

    placement->panelId = panelId;
    loadPlacementFields(root->GetGroup(groupName.constData()), placement);
    return true;
}

PanelPlacement PanelPlacementStore::loadPlacement(const QString& panelId, const PanelPlacement& fallback)
{
    PanelPlacement placement = fallback;
    placement.panelId = panelId;

    auto root = preferencesGroup(placementsPath());
    const QByteArray groupName = stableIdBytes(panelId);
    if (root->HasGroup(groupName.constData())) {
        const Launcher originalLauncher = placement.launcher;
        const LauncherValidity validity
            = loadPlacementFields(root->GetGroup(groupName.constData()), &placement);
        if (!validity.rail || !validity.cluster) {
            Launcher legacyLauncher = originalLauncher;
            if (importLegacyLauncher(panelId, &legacyLauncher)) {
                if (!validity.rail) {
                    placement.launcher.rail = legacyLauncher.rail;
                }
                if (!validity.cluster) {
                    placement.launcher.cluster = legacyLauncher.cluster;
                }
            }
        }
        placement.panelId = panelId;
        placement.normalize();
        return placement;
    }

    PanelPlacement::Launcher legacyLauncher = placement.launcher;
    if (importLegacyLauncher(panelId, &legacyLauncher)) {
        placement.launcher = legacyLauncher;
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

    group->SetASCII(
        LauncherRailKey,
        panelPlacementRailToString(normalized.launcher.rail).toUtf8().constData()
    );
    group->SetASCII(
        LauncherClusterKey,
        panelPlacementClusterToString(normalized.launcher.cluster).toUtf8().constData()
    );
    group->SetInt(LauncherOrderKey, normalized.launcher.order);
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
