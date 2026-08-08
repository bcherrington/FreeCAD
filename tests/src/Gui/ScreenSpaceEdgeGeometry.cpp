// SPDX-License-Identifier: LGPL-2.1-or-later

#include <vector>

#include <gtest/gtest.h>

#include <Mod/Part/Gui/ScreenSpaceEdgeGeometry.h>

namespace
{

using PartGui::Detail::buildScreenSpaceEdgeBatch;
using PartGui::Detail::ProjectedSegment;

TEST(ScreenSpaceEdgeGeometry, BatchesEachSegmentIntoTwoTriangles)
{
    const std::vector<ProjectedSegment> segments {
        {10.0, 20.0, 0.25, 30.0, 20.0, 0.5, true},
        {30.0, 20.0, 0.5, 30.0, 40.0, 0.5, true},
    };

    const auto batch = buildScreenSpaceEdgeBatch(segments, 1.5, 1.0);

    EXPECT_EQ(batch.rejectedSegments, 0);
    ASSERT_EQ(batch.vertices.size(), 12);
    EXPECT_DOUBLE_EQ(batch.halfWidthPhysical, 0.75);
    EXPECT_DOUBLE_EQ(batch.featherPhysical, 0.75);
    EXPECT_FLOAT_EQ(batch.vertices.front().segmentLength, 20.0F);
    EXPECT_LT(batch.vertices.front().z, 0.25F);
    EXPECT_GT(batch.vertices[1].z, 0.5F);
}

TEST(ScreenSpaceEdgeGeometry, ScalesWidthAndFeatherByDevicePixelRatio)
{
    const std::vector<ProjectedSegment> segments {
        {10.0, 20.0, 0.0, 30.0, 20.0, 0.0, true},
    };

    const auto dprOne = buildScreenSpaceEdgeBatch(segments, 1.5, 1.0);
    const auto dprTwo = buildScreenSpaceEdgeBatch(segments, 1.5, 2.0);

    EXPECT_DOUBLE_EQ(dprTwo.halfWidthPhysical, dprOne.halfWidthPhysical * 2.0);
    EXPECT_DOUBLE_EQ(dprTwo.featherPhysical, dprOne.featherPhysical * 2.0);
    ASSERT_EQ(dprOne.vertices.size(), dprTwo.vertices.size());
    EXPECT_LT(dprTwo.vertices.front().x, dprOne.vertices.front().x);
    EXPECT_LT(dprTwo.vertices.front().y, dprOne.vertices.front().y);
}

TEST(ScreenSpaceEdgeGeometry, RejectsInvalidAndDegenerateSegments)
{
    const std::vector<ProjectedSegment> segments {
        {10.0, 20.0, 0.0, 30.0, 20.0, 0.0, false},
        {10.0, 20.0, 0.0, 10.0, 20.0, 0.0, true},
    };

    const auto batch = buildScreenSpaceEdgeBatch(segments, 1.5, 1.0);

    EXPECT_EQ(batch.rejectedSegments, 2);
    EXPECT_TRUE(batch.vertices.empty());
}

}  // namespace
