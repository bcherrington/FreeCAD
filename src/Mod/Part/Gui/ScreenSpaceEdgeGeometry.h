// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD Project contributors

#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

#include "ProjectedSegmentDeduplication.h"

namespace PartGui::Detail
{

struct ScreenSpaceEdgeVertex
{
    float x {0.0F};
    float y {0.0F};
    float z {0.0F};
    float along {0.0F};
    float across {0.0F};
    float segmentLength {0.0F};
};

struct ScreenSpaceEdgeBatch
{
    std::vector<ScreenSpaceEdgeVertex> vertices;
    std::size_t rejectedSegments {0};
    double halfWidthPhysical {0.0};
    double featherPhysical {0.0};
};

inline ScreenSpaceEdgeBatch buildScreenSpaceEdgeBatch(
    const std::vector<ProjectedSegment>& segments,
    double logicalWidth,
    double devicePixelRatio,
    double logicalFeather = 0.75
)
{
    ScreenSpaceEdgeBatch batch;
    if (!std::isfinite(logicalWidth) || logicalWidth <= 0.0 || !std::isfinite(devicePixelRatio)
        || devicePixelRatio <= 0.0 || !std::isfinite(logicalFeather) || logicalFeather <= 0.0) {
        batch.rejectedSegments = segments.size();
        return batch;
    }

    batch.halfWidthPhysical = logicalWidth * devicePixelRatio * 0.5;
    batch.featherPhysical = logicalFeather * devicePixelRatio;
    const double radius = batch.halfWidthPhysical + batch.featherPhysical;
    batch.vertices.reserve(segments.size() * 6);

    for (const auto& segment : segments) {
        const double dx = segment.secondX - segment.firstX;
        const double dy = segment.secondY - segment.firstY;
        const double length = std::hypot(dx, dy);
        if (!segment.valid || !std::isfinite(length) || length <= 1.0e-6) {
            ++batch.rejectedSegments;
            continue;
        }

        const double directionX = dx / length;
        const double directionY = dy / length;
        const double depthPerPixel = (segment.secondZ - segment.firstZ) / length;
        const double normalX = -directionY;
        const double normalY = directionX;
        const double firstX = segment.firstX - directionX * radius;
        const double firstY = segment.firstY - directionY * radius;
        const double secondX = segment.secondX + directionX * radius;
        const double secondY = segment.secondY + directionY * radius;
        const float firstAlong = static_cast<float>(-radius);
        const float secondAlong = static_cast<float>(length + radius);
        const float acrossNegative = static_cast<float>(-radius);
        const float acrossPositive = static_cast<float>(radius);
        const float segmentLength = static_cast<float>(length);
        const float firstDepth = static_cast<float>(segment.firstZ - depthPerPixel * radius);
        const float secondDepth = static_cast<float>(segment.secondZ + depthPerPixel * radius);

        const ScreenSpaceEdgeVertex firstNegative {
            static_cast<float>(firstX - normalX * radius),
            static_cast<float>(firstY - normalY * radius),
            firstDepth,
            firstAlong,
            acrossNegative,
            segmentLength,
        };
        const ScreenSpaceEdgeVertex firstPositive {
            static_cast<float>(firstX + normalX * radius),
            static_cast<float>(firstY + normalY * radius),
            firstDepth,
            firstAlong,
            acrossPositive,
            segmentLength,
        };
        const ScreenSpaceEdgeVertex secondNegative {
            static_cast<float>(secondX - normalX * radius),
            static_cast<float>(secondY - normalY * radius),
            secondDepth,
            secondAlong,
            acrossNegative,
            segmentLength,
        };
        const ScreenSpaceEdgeVertex secondPositive {
            static_cast<float>(secondX + normalX * radius),
            static_cast<float>(secondY + normalY * radius),
            secondDepth,
            secondAlong,
            acrossPositive,
            segmentLength,
        };

        batch.vertices.insert(
            batch.vertices.end(),
            {
                firstNegative,
                secondNegative,
                secondPositive,
                firstNegative,
                secondPositive,
                firstPositive,
            }
        );
    }

    return batch;
}

}  // namespace PartGui::Detail
