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

#pragma once

#include <QList>
#include <QRect>
#include <QString>

#include <FCGlobal.h>

namespace Gui
{

struct GuiExport PanelPlacement
{
    static constexpr int CurrentSchemaVersion = 1;
    // Safe policy:
    // - Reads accept any positive root schema version and hydrate known fields.
    // - Non-positive or missing schema versions normalize to CurrentSchemaVersion.
    // - Saves always rewrite the store at CurrentSchemaVersion.

    enum class Mode
    {
        Docked,
        Overlay,
        AutoHide,
        Floating,
    };

    enum class Edge
    {
        Left,
        Right,
        Top,
        Bottom,
        None,
    };

    enum class Region
    {
        Start,
        Center,
        End,
    };

    struct Launcher
    {
        enum class Rail
        {
            Left,
            Right,
        };

        enum class Cluster
        {
            Upper,
            Lower,
            Bottom,
        };

        Rail rail = Rail::Left;
        Cluster cluster = Cluster::Upper;
        int order = 0;

        void normalize();
        bool operator==(const Launcher& other) const;
        bool operator!=(const Launcher& other) const
        {
            return !(*this == other);
        }
    };

    int schemaVersion = CurrentSchemaVersion;
    QString panelId;
    Mode mode = Mode::Docked;
    Edge edge = Edge::Left;
    Region region = Region::Start;
    QString groupId;
    int groupOrder = 0;
    int tabOrder = 0;
    QString splitRelation;
    int extent = 0;
    QRect floatingGeometry;
    Launcher launcher;

    void normalize();
    bool hasStablePanelId() const;

    bool operator==(const PanelPlacement& other) const;
    bool operator!=(const PanelPlacement& other) const
    {
        return !(*this == other);
    }
};

GuiExport bool isStablePanelId(const QString& panelId);

GuiExport QString panelPlacementModeToString(PanelPlacement::Mode mode);
GuiExport bool panelPlacementModeFromString(const QString& value, PanelPlacement::Mode* mode);

GuiExport QString panelPlacementEdgeToString(PanelPlacement::Edge edge);
GuiExport bool panelPlacementEdgeFromString(const QString& value, PanelPlacement::Edge* edge);

GuiExport QString panelPlacementRegionToString(PanelPlacement::Region region);
GuiExport bool panelPlacementRegionFromString(const QString& value, PanelPlacement::Region* region);

GuiExport QString panelPlacementRailToString(PanelPlacement::Launcher::Rail rail);
GuiExport bool panelPlacementRailFromString(const QString& value, PanelPlacement::Launcher::Rail* rail);

GuiExport QString panelPlacementClusterToString(PanelPlacement::Launcher::Cluster cluster);
GuiExport bool panelPlacementClusterFromString(
    const QString& value,
    PanelPlacement::Launcher::Cluster* cluster
);

class GuiExport PanelPlacementStore
{
public:
    static constexpr int CurrentSchemaVersion = PanelPlacement::CurrentSchemaVersion;

    static const char* mainWindowPreferencesPath();
    static const char* placementsPath();
    static const char* compactLegacySlotsPath();
    static const char* panelRailsLegacySlotsPath();
    static const char* featureFlagKey();
    static const char* schemaVersionKey();
    static const char* migrationMarkerKey();

    static bool isFeatureEnabled();
    static int schemaVersion();
    static bool migrationMarker();
    static void setMigrationMarker(bool enabled = true);

    static bool hasPlacement(const QString& panelId);
    static bool loadUnifiedPlacement(const QString& panelId, PanelPlacement* placement);
    static PanelPlacement loadPlacement(
        const QString& panelId,
        const PanelPlacement& fallback = PanelPlacement()
    );
    static QList<PanelPlacement> loadPlacements();

    static bool savePlacement(const PanelPlacement& placement);
    static bool removePlacement(const QString& panelId);
    static bool importLegacyLauncher(const QString& panelId, PanelPlacement::Launcher* launcher);
};

}  // namespace Gui
