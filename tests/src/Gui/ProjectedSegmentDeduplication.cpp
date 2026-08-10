// SPDX-License-Identifier: LGPL-2.1-or-later

#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include <Mod/Part/Gui/ProjectedSegmentDeduplication.h>

namespace
{

using PartGui::Detail::deduplicateProjectedSegments;
using PartGui::Detail::deduplicateProjectedSegmentsWithTopologyId;
using PartGui::Detail::ProjectedSegment;
using PartGui::Detail::ProjectedViewport;

constexpr ProjectedViewport viewport {0.0, 0.0, 100.0, 100.0};

TEST(ProjectedSegmentDeduplication, NormalizesDirectionAndKeepsNearestDuplicate)
{
    const std::vector<ProjectedSegment> segments {
        {10.0, 20.0, 0.5, 30.0, 40.0, 0.5, true},
        {30.0, 40.0, -0.5, 10.0, 20.0, -0.5, true},
    };

    const auto result = deduplicateProjectedSegments(segments, viewport, 0.5);

    EXPECT_EQ(result.stats.inputSegments, 2);
    EXPECT_EQ(result.stats.rejectedSegments, 0);
    EXPECT_EQ(result.stats.duplicateSegments, 1);
    ASSERT_EQ(result.stats.outputSegments, 1);
    EXPECT_DOUBLE_EQ(result.segments.front().firstZ, -0.5);
    EXPECT_DOUBLE_EQ(result.segments.front().secondZ, -0.5);
}

TEST(ProjectedSegmentDeduplication, RejectsInvalidDegenerateAndFullyOffscreenSegments)
{
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const std::vector<ProjectedSegment> segments {
        {10.0, 10.0, 0.0, 20.0, 20.0, 0.0, false},
        {nan, 10.0, 0.0, 20.0, 20.0, 0.0, true},
        {10.0, 10.0, 0.0, 10.0, 10.0, 0.0, true},
        {-20.0, 10.0, 0.0, -10.0, 20.0, 0.0, true},
        {-10.0, 50.0, 0.0, 50.0, 50.0, 0.0, true},
    };

    const auto result = deduplicateProjectedSegments(segments, viewport, 0.5);

    EXPECT_EQ(result.stats.inputSegments, 5);
    EXPECT_EQ(result.stats.rejectedSegments, 4);
    EXPECT_EQ(result.stats.duplicateSegments, 0);
    EXPECT_EQ(result.stats.outputSegments, 1);
}

TEST(ProjectedSegmentDeduplication, ToleranceControlsCloseParallelEdgeMerging)
{
    const std::vector<ProjectedSegment> segments {
        {10.0, 10.0, 0.0, 40.0, 40.0, 0.0, true},
        {10.2, 10.2, 0.0, 40.2, 40.2, 0.0, true},
    };

    const auto coarse = deduplicateProjectedSegments(segments, viewport, 0.5);
    const auto fine = deduplicateProjectedSegments(segments, viewport, 0.1);

    EXPECT_EQ(coarse.stats.duplicateSegments, 1);
    EXPECT_EQ(coarse.stats.outputSegments, 1);
    EXPECT_EQ(fine.stats.duplicateSegments, 0);
    EXPECT_EQ(fine.stats.outputSegments, 2);
}

TEST(ProjectedSegmentDeduplication, TopologyIdPreservesDistinctCoincidentSegments)
{
    constexpr std::uint64_t highBitTopologyId = 0x8000000000000000ULL;
    const std::vector<ProjectedSegment> segments {
        {10.0, 20.0, 0.2, 30.0, 40.0, 0.2, true, 101ULL},
        {10.0, 20.0, 0.3, 30.0, 40.0, 0.3, true, highBitTopologyId},
    };

    const auto result = deduplicateProjectedSegmentsWithTopologyId(segments, viewport, 0.5);

    EXPECT_EQ(result.stats.inputSegments, 2);
    EXPECT_EQ(result.stats.rejectedSegments, 0);
    EXPECT_EQ(result.stats.duplicateSegments, 0);
    ASSERT_EQ(result.stats.outputSegments, 2);
    EXPECT_EQ(result.segments.back().topologyId, highBitTopologyId);
}

TEST(ProjectedSegmentDeduplication, TopologyIdCollapsesOnlyMatchingIdentity)
{
    const std::vector<ProjectedSegment> segments {
        {10.0, 20.0, 0.8, 30.0, 40.0, 1.0, true, 77ULL},
        {30.0, 40.0, 0.2, 10.0, 20.0, 0.4, true, 77ULL},
        {10.0, 20.0, 1.6, 30.0, 40.0, 1.2, true, 78ULL},
    };

    const auto result = deduplicateProjectedSegmentsWithTopologyId(segments, viewport, 0.5);

    EXPECT_EQ(result.stats.inputSegments, 3);
    EXPECT_EQ(result.stats.rejectedSegments, 0);
    EXPECT_EQ(result.stats.duplicateSegments, 1);
    ASSERT_EQ(result.stats.outputSegments, 2);
    EXPECT_EQ(result.segments.front().topologyId, 77ULL);
    EXPECT_DOUBLE_EQ(result.segments.front().firstZ, 0.4);
    EXPECT_DOUBLE_EQ(result.segments.front().secondZ, 0.2);
    EXPECT_EQ(result.segments.back().topologyId, 78ULL);
}

}  // namespace
