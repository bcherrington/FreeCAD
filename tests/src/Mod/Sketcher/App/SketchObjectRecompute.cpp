// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Base/Console.h>
#include <Mod/Sketcher/App/Sketch.h>

#include "SketcherTestHelpers.h"

namespace
{

// Baseline-only observation of the existing diagnostic message. This does not
// add counters or timing overhead to production code. Replace this observation
// with the optional diagnostic test seam when that interface is implemented.
class SetupObserver: public Base::ILogger
{
public:
    SetupObserver()
    {
        Base::Console().attachObserver(this);
    }

    ~SetupObserver() override
    {
        Base::Console().detachObserver(this);
    }

    void sendLog(
        const std::string&,
        const std::string& message,
        Base::LogStyle,
        Base::IntendedRecipient,
        Base::ContentType
    ) override
    {
        if (message.starts_with("Sketcher::setUpSketch()-T:")) {
            ++count;
        }
    }

    unsigned int count {0};
};

}  // namespace

TEST_F(SketchObjectTest, recomputeCleanSketchDoesNoWork)
{
    auto* sketch = getObject();
    Part::GeomCircle circle;
    SketcherTestHelpers::setupCircle(circle);
    sketch->addGeometry(&circle);
    ASSERT_EQ(sketch->getDocument()->recompute(), 1);

    SetupObserver observer;
    EXPECT_EQ(sketch->getDocument()->recompute(), 0);
    EXPECT_EQ(observer.count, 0);
}

TEST_F(SketchObjectTest, recomputeReportsBaselineSetupPasses)
{
    auto* sketch = getObject();
    Part::GeomCircle circle;
    SketcherTestHelpers::setupCircle(circle);
    sketch->addGeometry(&circle);
    ASSERT_EQ(sketch->getDocument()->recompute(), 1);
    const int dof = sketch->getLastDoF();

    // Force execution without changing geometry. A clean recompute would skip
    // execute() entirely and could not establish the duplicate-diagnosis path.
    sketch->touch();
    SetupObserver observer;
    EXPECT_EQ(sketch->getDocument()->recompute(), 1);
    // T004 must reduce this baseline to one while preserving the assertions below.
    EXPECT_EQ(observer.count, 2);
    EXPECT_EQ(sketch->getLastDoF(), dof);
    ASSERT_EQ(sketch->Geometry.getSize(), 1);
    const auto* result = dynamic_cast<const Part::GeomCircle*>(sketch->Geometry[0]);
    ASSERT_NE(result, nullptr);
    EXPECT_DOUBLE_EQ(result->getRadius(), circle.getRadius());
    EXPECT_DOUBLE_EQ(result->getCenter().x, circle.getCenter().x);
    EXPECT_DOUBLE_EQ(result->getCenter().y, circle.getCenter().y);
}
