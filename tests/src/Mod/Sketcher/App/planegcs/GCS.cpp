// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Mod/Sketcher/App/planegcs/GCS.h"

class SystemTest: public GCS::System
{
public:
    size_t getNumberOfConstraints(int tagID = -1)
    {
        return _getNumberOfConstraints(tagID);
    }
};

class GCSTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        _system = std::make_unique<SystemTest>();
    }

    void TearDown() override
    {
        _system.reset();
    }

    SystemTest* System()
    {
        return _system.get();
    }

private:
    std::unique_ptr<SystemTest> _system;
};

TEST_F(GCSTest, clearConstraints)  // NOLINT
{
    // Arrange
    const size_t numConstraints {100};
    for (size_t i = 0; i < numConstraints; ++i) {
        System()->addConstraint(new GCS::Constraint());
    }
    ASSERT_EQ(numConstraints, System()->getNumberOfConstraints());

    // Act
    System()->clear();

    // Assert
    EXPECT_EQ(0, System()->getNumberOfConstraints());
}

TEST_F(GCSTest, failedSubsystemIsNotHiddenByLaterSuccess)
{
    double x = 0, y = 0, targetX = 40, targetY = 0;
    auto* system = System();
    system->addConstraintEqual(&x, &targetX, 1);
    system->addConstraintEqual(&y, &targetY, 2);
    GCS::VEC_pD unknowns {&x, &y};
    system->declareUnknowns(unknowns);
    system->initSolution();
    // The moved X subsystem cannot converge in one LM iteration; the independent
    // Y subsystem is already satisfied. Its success must not erase the X failure.
    system->maxIter = 1;
    system->sketchSizeMultiplier = false;
    EXPECT_NE(system->solve(GCS::LevenbergMarquardt), GCS::SolveStatus::Success);
    system->maxIter = 100;
    ASSERT_EQ(system->solve(GCS::DogLeg), GCS::SolveStatus::Success);
    system->applySolution();
    EXPECT_NEAR(x, 40, 1e-9);
    EXPECT_NEAR(y, 0, 1e-9);
}
