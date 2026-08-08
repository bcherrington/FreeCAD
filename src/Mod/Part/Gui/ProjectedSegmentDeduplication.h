// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD Project contributors

#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace PartGui::Detail
{

struct ProjectedSegment
{
    double firstX {0.0};
    double firstY {0.0};
    double firstZ {0.0};
    double secondX {0.0};
    double secondY {0.0};
    double secondZ {0.0};
    bool valid {false};
};

struct ProjectedViewport
{
    double left {0.0};
    double bottom {0.0};
    double right {0.0};
    double top {0.0};
};

struct ProjectedSegmentDeduplicationStats
{
    std::size_t inputSegments {0};
    std::size_t rejectedSegments {0};
    std::size_t duplicateSegments {0};
    std::size_t outputSegments {0};
};

struct ProjectedSegmentDeduplicationResult
{
    std::vector<ProjectedSegment> segments;
    ProjectedSegmentDeduplicationStats stats;
};

struct ProjectedSegmentKeyHash
{
    std::size_t operator()(const std::array<std::int64_t, 4>& key) const noexcept
    {
        std::size_t result = 0;
        for (const auto value : key) {
            const auto valueHash = std::hash<std::int64_t> {}(value);
            result ^= valueHash + 0x9e3779b9U + (result << 6U) + (result >> 2U);
        }
        return result;
    }
};

inline bool isProjectedSegmentRenderable(const ProjectedSegment& segment, const ProjectedViewport& viewport)
{
    const bool finite = std::isfinite(segment.firstX) && std::isfinite(segment.firstY)
        && std::isfinite(segment.firstZ) && std::isfinite(segment.secondX)
        && std::isfinite(segment.secondY) && std::isfinite(segment.secondZ);
    const bool fullyOffscreen = (segment.firstX < viewport.left && segment.secondX < viewport.left)
        || (segment.firstX > viewport.right && segment.secondX > viewport.right)
        || (segment.firstY < viewport.bottom && segment.secondY < viewport.bottom)
        || (segment.firstY > viewport.top && segment.secondY > viewport.top);
    const double dx = segment.secondX - segment.firstX;
    const double dy = segment.secondY - segment.firstY;
    return segment.valid && finite && !fullyOffscreen && std::hypot(dx, dy) > 1.0e-6;
}

inline ProjectedSegmentDeduplicationResult deduplicateProjectedSegments(
    const std::vector<ProjectedSegment>& input,
    const ProjectedViewport& viewport,
    double tolerance
)
{
    ProjectedSegmentDeduplicationResult result;
    result.stats.inputSegments = input.size();
    if (!std::isfinite(tolerance) || tolerance <= 0.0) {
        result.stats.rejectedSegments = input.size();
        return result;
    }

    using SegmentKey = std::array<std::int64_t, 4>;
    std::unordered_map<SegmentKey, std::size_t, ProjectedSegmentKeyHash> outputIndexByKey;
    outputIndexByKey.reserve(input.size());

    const auto quantize = [tolerance](double value) {
        const double scaled = value / tolerance;
        const double lower = static_cast<double>(std::numeric_limits<std::int64_t>::min());
        const double upper = static_cast<double>(std::numeric_limits<std::int64_t>::max());
        if (scaled <= lower) {
            return std::numeric_limits<std::int64_t>::min();
        }
        if (scaled >= upper) {
            return std::numeric_limits<std::int64_t>::max();
        }
        return static_cast<std::int64_t>(std::llround(scaled));
    };

    for (auto segment : input) {
        if (!isProjectedSegmentRenderable(segment, viewport)) {
            ++result.stats.rejectedSegments;
            continue;
        }

        std::array<std::int64_t, 2> first {
            quantize(segment.firstX),
            quantize(segment.firstY),
        };
        std::array<std::int64_t, 2> second {
            quantize(segment.secondX),
            quantize(segment.secondY),
        };
        if (second < first) {
            std::swap(first, second);
            std::swap(segment.firstX, segment.secondX);
            std::swap(segment.firstY, segment.secondY);
            std::swap(segment.firstZ, segment.secondZ);
        }

        const SegmentKey key {first[0], first[1], second[0], second[1]};
        const auto [existing, inserted] = outputIndexByKey.emplace(key, result.segments.size());
        if (inserted) {
            result.segments.push_back(segment);
            continue;
        }

        ++result.stats.duplicateSegments;
        auto& representative = result.segments[existing->second];
        const double existingDepth = representative.firstZ + representative.secondZ;
        const double candidateDepth = segment.firstZ + segment.secondZ;
        if (candidateDepth < existingDepth) {
            representative = segment;
        }
    }

    result.stats.outputSegments = result.segments.size();
    return result;
}

}  // namespace PartGui::Detail
