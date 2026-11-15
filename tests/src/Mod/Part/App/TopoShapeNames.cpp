// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <barrier>
#include <string>
#include <thread>
#include <vector>

#include <Mod/Part/App/TopoShape.h>

// This test needs no application initialization or shape construction. A filtered
// invocation exercises concurrent first use of the shape-name table directly.
TEST(TopoShapeNamesTest, ConcurrentFirstUsePreservesNamesAndReferences)
{
    const std::array<std::pair<TopAbs_ShapeEnum, const char*>, 8> cases {{
        {TopAbs_VERTEX, "Vertex"},
        {TopAbs_EDGE, "Edge"},
        {TopAbs_FACE, "Face"},
        {TopAbs_WIRE, "Wire"},
        {TopAbs_SHELL, "Shell"},
        {TopAbs_SOLID, "Solid"},
        {TopAbs_COMPOUND, "Compound"},
        {TopAbs_COMPSOLID, "CompSolid"},
    }};
    constexpr int threadCount = 8;
    constexpr int iterations = 128;
    std::barrier start(threadCount);
    std::atomic<bool> valid {true};
    std::array<std::array<const std::string*, cases.size()>, threadCount> references {};
    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        threads.emplace_back([&, threadIndex] {
            start.arrive_and_wait();
            for (int iteration = 0; iteration < iterations; ++iteration) {
                for (std::size_t i = 0; i < cases.size(); ++i) {
                    const auto& [type, expected] = cases[i];
                    // Use the non-throwing form so incomplete initialization is
                    // reported as a failed invariant rather than a worker exception.
                    const auto& name = Part::TopoShape::shapeName(type, true);
                    references[threadIndex][i] = &name;
                    const auto indexed = Data::IndexedName::fromConst(expected, 17);
                    if (name != expected || Part::TopoShape::shapeType(expected, true) != type
                        || Part::TopoShape::shapeTypeAndIndex(indexed) != std::make_pair(type, 17)
                        || Part::TopoShape::shapeTypeAndIndex((name + "17").c_str())
                            != std::make_pair(type, 17)) {
                        valid = false;
                    }
                }
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_TRUE(valid.load());
    for (std::size_t i = 0; i < cases.size(); ++i) {
        for (const auto& observed : references) {
            ASSERT_NE(observed[i], nullptr);
            EXPECT_EQ(observed[i], &Part::TopoShape::shapeName(cases[i].first));
            EXPECT_EQ(*observed[i], cases[i].second);
        }
    }
}
