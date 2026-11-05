// SPDX-License-Identifier: LGPL-2.1-or-later

#include <numbers>
#include <sstream>

#include "SketcherTestHelpers.h"

namespace
{
using namespace Sketcher;

Constraint* constraint(ConstraintType type, int geo = 0)
{
    auto* result = new Constraint;
    result->Type = type;
    result->setGeoId(0, geo);
    return result;
}

void addLine(SketchObject* sketch)
{
    Part::GeomLineSegment line;
    line.setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(4, 0, 0));
    sketch->addGeometry(&line);
}

struct Observation
{
    std::string events;
    std::vector<bool> touchedAtChange;
    bool touchedAfter {};
};

Observation observeRecompute(SketchObject* sketch)
{
    // Establish the diagnostic state before observing a forced unchanged execute.
    sketch->getDocument()->recompute();
    sketch->Constraints.purgeTouched();
    sketch->touch();
    Observation result;
    fastsignals::scoped_connection before = sketch->signalBeforeChange.connect(
        [&](const App::DocumentObject&, const App::Property& property) {
            if (&property == &sketch->Constraints) {
                result.events += 'B';
            }
        }
    );
    fastsignals::scoped_connection changed = sketch->signalChanged.connect(
        [&](const App::DocumentObject&, const App::Property& property) {
            if (&property == &sketch->Constraints) {
                result.events += 'C';
                result.touchedAtChange.push_back(property.isTouched());
            }
        }
    );
    fastsignals::scoped_connection propertyChanged = sketch->Constraints.signalChanged.connect(
        [&](const App::Property&) { result.events += 'P'; }
    );
    fastsignals::scoped_connection solver = sketch->signalSolverUpdate.connect([&]() {
        result.events += 'S';
    });
    sketch->getDocument()->recompute();
    result.touchedAfter = sketch->Constraints.isTouched();
    return result;
}

void record(const Observation& observation)
{
    ::testing::Test::RecordProperty("constraints_observers", observation.events);
    std::ostringstream touched;
    for (bool value : observation.touchedAtChange) {
        touched << (value ? '1' : '0');
    }
    ::testing::Test::RecordProperty("constraints_touched_at_change", touched.str());
    ::testing::Test::RecordProperty("constraints_touched_after", observation.touchedAfter);
}
}  // namespace

TEST_F(SketchObjectTest, diagnosisObserversValidBaseline)
{
    auto* sketch = getObject();
    addLine(sketch);
    sketch->Constraints.setValues({constraint(Sketcher::Horizontal)});
    auto observed = observeRecompute(sketch);
    record(observed);
    EXPECT_EQ(observed.events, "BCPBCPBCPS");
    EXPECT_EQ(observed.touchedAtChange, (std::vector<bool> {false, true, true}));
    EXPECT_FALSE(observed.touchedAfter);
    EXPECT_EQ(sketch->getLastDoF(), 3);
}

TEST_F(SketchObjectTest, diagnosisObserversRedundantBaseline)
{
    auto* sketch = getObject();
    addLine(sketch);
    sketch->Constraints.setValues({constraint(Sketcher::Horizontal), constraint(Sketcher::Horizontal)});
    auto observed = observeRecompute(sketch);
    record(observed);
    EXPECT_EQ(observed.events, "BCPBCCPS");
    EXPECT_EQ(observed.touchedAtChange, (std::vector<bool> {false, true, true}));
    EXPECT_TRUE(observed.touchedAfter);
    EXPECT_EQ(sketch->getLastRedundant(), (std::vector<int> {2}));
}

TEST_F(SketchObjectTest, diagnosisObserversConflictingBaseline)
{
    auto* sketch = getObject();
    addLine(sketch);
    auto* angle = constraint(Sketcher::Angle);
    angle->setValue(std::numbers::pi / 4);
    sketch->Constraints.setValues({constraint(Sketcher::Horizontal), angle});
    auto observed = observeRecompute(sketch);
    record(observed);
    EXPECT_EQ(observed.events, "BCPBCCPS");
    EXPECT_EQ(observed.touchedAtChange, (std::vector<bool> {false, true, true}));
    EXPECT_TRUE(observed.touchedAfter);
    EXPECT_EQ(sketch->getLastConflicting(), (std::vector<int> {1}));
}

TEST_F(SketchObjectTest, diagnosisObserversMalformedBaseline)
{
    auto* sketch = getObject();
    addLine(sketch);
    auto* radius = constraint(Sketcher::Radius);
    radius->setValue(5);
    sketch->Constraints.setValues({radius});
    auto observed = observeRecompute(sketch);
    record(observed);
    EXPECT_EQ(observed.events, "BCPBCCPS");
    EXPECT_EQ(observed.touchedAtChange, (std::vector<bool> {false, true, true}));
    EXPECT_TRUE(observed.touchedAfter);
    EXPECT_EQ(sketch->getLastMalformedConstraints(), (std::vector<int> {1}));
}

TEST_F(SketchObjectTest, diagnosisObserversPartiallyRedundantBaseline)
{
    auto* sketch = getObject();
    Part::GeomArcOfCircle arc;
    arc.setCenter(Base::Vector3d(0, 0, 0));
    arc.setRadius(5);
    arc.setRange(0, 2 * std::numbers::pi, true);
    sketch->addGeometry(&arc);
    auto* coincident = constraint(Sketcher::Coincident);
    coincident->setPosId(0, PointPos::start);
    coincident->setGeoId(1, 0);
    coincident->setPosId(1, PointPos::end);
    sketch->Constraints.setValues({coincident});
    auto observed = observeRecompute(sketch);
    record(observed);
    EXPECT_EQ(observed.events, "BCPBCCPBCPS");
    EXPECT_EQ(observed.touchedAtChange, (std::vector<bool> {false, true, true, true}));
    EXPECT_FALSE(observed.touchedAfter);
    EXPECT_EQ(sketch->getLastPartiallyRedundant(), (std::vector<int> {1}));
}

TEST_F(SketchObjectTest, diagnosisIndicesInvalidThenRepaired)
{
    auto* sketch = getObject();
    addLine(sketch);
    // Geometry index 1 is absent, but the raw constraint must survive rejection.
    sketch->Constraints.setValues({constraint(Sketcher::Horizontal, 1)});
    EXPECT_EQ(sketch->Constraints.getValuesForce().size(), 1);
    EXPECT_TRUE(sketch->Constraints.getValues().empty());
    EXPECT_TRUE(sketch->Constraints.checkConstraintIndices(0, -2));
    // Accepting geometry alone does not clear the separate invalid-index flag.
    sketch->acceptGeometry();
    EXPECT_TRUE(sketch->Constraints.getValues().empty());
    addLine(sketch);
    EXPECT_FALSE(sketch->Constraints.checkConstraintIndices(1, -2));
    sketch->acceptGeometry();
    ASSERT_EQ(sketch->Constraints.getValues().size(), 1);
    EXPECT_EQ(sketch->Constraints.getValues().front()->getGeoId(0), 1);
    EXPECT_EQ(sketch->solve(), SketchSolveStatus::Success);
    EXPECT_EQ(sketch->getLastDoF(), 7);
}
