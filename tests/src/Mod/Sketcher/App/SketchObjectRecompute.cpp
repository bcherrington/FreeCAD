// SPDX-License-Identifier: LGPL-2.1-or-later

#include <stdexcept>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Sketcher/App/SketcherTrace.h>

#include "SketcherTestHelpers.h"

namespace
{
using namespace Sketcher;

class SetupCounter
{
public:
    unsigned int count {};
    Trace::Hooks hooks {
        true,
        nullptr,
        [](const Trace::Context& context, void* user) {
            auto& counter = *static_cast<SetupCounter*>(user);
            EXPECT_EQ(context.dropped.load(), 0);
            for (std::uint32_t i = 0; i < context.used.load(); ++i) {
                if (context.records[i].phase == Trace::Phase::Setup) {
                    ++counter.count;
                }
            }
        },
        this
    };
    Trace::TestHooks capture {hooks};
};
void addLine(SketchObject* sketch)
{
    Part::GeomLineSegment line;
    line.setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(4, 0, 0));
    sketch->addGeometry(&line);
}
std::unique_ptr<Constraint> horizontal(int geo = 0)
{
    auto c = std::make_unique<Constraint>();
    c->Type = Horizontal;
    c->setGeoId(0, geo);
    return c;
}
void validLine(SketchObject* sketch)
{
    addLine(sketch);
    sketch->addConstraint(horizontal());
    sketch->getDocument()->recompute();
}
}  // namespace

TEST_F(SketchObjectTest, recomputeCleanSketchDoesNoWork)
{
    auto* sketch = getObject();
    validLine(sketch);
    SetupCounter observer;
    EXPECT_EQ(sketch->getDocument()->recompute(), 0);
    EXPECT_EQ(observer.count, 0);
}

TEST_F(SketchObjectTest, recomputeDiagnosesOnce)
{
    auto* sketch = getObject();
    Part::GeomCircle circle;
    SketcherTestHelpers::setupCircle(circle);
    sketch->addGeometry(&circle);
    ASSERT_EQ(sketch->getDocument()->recompute(), 1);
    const int dof = sketch->getLastDoF();
    sketch->touch();
    SetupCounter observer;
    EXPECT_EQ(sketch->getDocument()->recompute(), 1);
    EXPECT_EQ(observer.count, 1);
    EXPECT_EQ(sketch->getLastDoF(), dof);
    ASSERT_EQ(sketch->Geometry.getSize(), 1);
    const auto* result = dynamic_cast<const Part::GeomCircle*>(sketch->Geometry[0]);
    ASSERT_NE(result, nullptr);
    EXPECT_DOUBLE_EQ(result->getRadius(), circle.getRadius());
    EXPECT_DOUBLE_EQ(result->getCenter().x, circle.getCenter().x);
    EXPECT_DOUBLE_EQ(result->getCenter().y, circle.getCenter().y);
}

TEST_F(SketchObjectTest, recomputeNoRecomputesRetainsLegacySetups)
{
    auto* sketch = getObject();
    validLine(sketch);
    Base::StateLocker editMode(sketch->noRecomputes, true);
    sketch->touch();
    SetupCounter observer;
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 2);
    EXPECT_EQ(sketch->getLastDoF(), 3);
}

TEST_F(SketchObjectTest, recomputeDirectSolveAndPropertyChangeRemainImmediate)
{
    auto* sketch = getObject();
    validLine(sketch);
    SetupCounter observer;
    EXPECT_EQ(sketch->solve(), SketchSolveStatus::Success);
    EXPECT_EQ(observer.count, 1);
    observer.count = 0;
    auto c = horizontal();
    sketch->Constraints.set1Value(0, c.get());
    EXPECT_EQ(observer.count, 1);
    EXPECT_EQ(sketch->getLastDoF(), 3);
    observer.count = 0;
    sketch->Constraints.touch();
    EXPECT_EQ(observer.count, 1);
}

TEST_F(SketchObjectTest, recomputeNestedCallbackCancelsOnlyItsOwnDeferral)
{
    auto* sketch = getObject();
    validLine(sketch);
    unsigned values = 0;
    fastsignals::scoped_connection nested = sketch->Constraints.signalChanged.connect(
        [&](const App::Property&) {
            if (++values == 2) {
                sketch->Constraints.touch();
            }
        }
    );
    SetupCounter observer;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 3);
    EXPECT_EQ(sketch->getLastDoF(), 3);
    observer.count = 0;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 1);
}

TEST_F(SketchObjectTest, recomputeAcceptanceBaseExceptionRestoresDeferredSetup)
{
    auto* sketch = getObject();
    validLine(sketch);
    unsigned values = 0;
    fastsignals::scoped_connection throwing = sketch->Constraints.signalChanged.connect(
        [&](const App::Property&) {
            if (++values == 2) {
                throw Base::RuntimeError("acceptance observer");
            }
        }
    );
    SetupCounter observer;
    sketch->touch();
    EXPECT_EQ(sketch->getDocument()->recompute(), 1);
    EXPECT_EQ(observer.count, 2);
    EXPECT_EQ(sketch->getLastDoF(), 3);
    EXPECT_EQ(sketch->getLastSolverStatus(), GCS::SolveStatus::Success);
    observer.count = 0;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 1);
}

TEST_F(SketchObjectTest, recomputeAcceptanceStdExceptionDoesNotLeakContext)
{
    auto* sketch = getObject();
    validLine(sketch);
    unsigned values = 0;
    fastsignals::scoped_connection throwing = sketch->Constraints.signalChanged.connect(
        [&](const App::Property&) {
            if (++values == 2) {
                throw std::runtime_error("std acceptance observer");
            }
        }
    );
    SetupCounter observer;
    sketch->touch();
    try {
        sketch->getDocument()->recompute();
    }
    catch (const std::runtime_error&) {
    }
    EXPECT_EQ(observer.count, 1);  // Omitted callback setup restored before the original error escapes.
    observer.count = 0;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 1);
    EXPECT_EQ(sketch->getLastDoF(), 3);
}

TEST_F(SketchObjectTest, recomputeInvalidSolverKeepsLegacyDiagnosticAndErrorOrder)
{
    auto* sketch = getObject();
    validLine(sketch);
    sketch->addConstraint(horizontal());
    auto preferences = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/SolverAdvanced"
    );
    const bool stored = !preferences->GetIntMap("DefaultSolver").empty();
    const auto old = preferences->GetInt("DefaultSolver", 2);
    Base::ScopeGuard restore([&] {
        if (stored) {
            preferences->SetInt("DefaultSolver", old);
        }
        else {
            preferences->RemoveInt("DefaultSolver");
        }
    });
    preferences->SetInt("DefaultSolver", 99);
    SetupCounter observer;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 1);
    EXPECT_EQ(sketch->getLastRedundant(), (std::vector<int> {2}));
    EXPECT_TRUE(sketch->Constraints.isTouched());
}

TEST_F(SketchObjectTest, recomputeLateInvalidSolverRestoresOmittedDiagnostics)
{
    auto* sketch = getObject();
    validLine(sketch);
    sketch->addConstraint(horizontal());
    auto preferences = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/SolverAdvanced"
    );
    const bool stored = !preferences->GetIntMap("DefaultSolver").empty();
    const auto old = preferences->GetInt("DefaultSolver", 2);
    Base::ScopeGuard restore([&] {
        if (stored) {
            preferences->SetInt("DefaultSolver", old);
        }
        else {
            preferences->RemoveInt("DefaultSolver");
        }
    });
    unsigned values = 0;
    fastsignals::scoped_connection changing = sketch->Constraints.signalChanged.connect(
        [&](const App::Property&) {
            if (++values == 2) {
                preferences->SetInt("DefaultSolver", 99);
            }
        }
    );
    SetupCounter observer;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 1);
    EXPECT_EQ(sketch->getLastRedundant(), (std::vector<int> {2}));
    preferences->SetInt("DefaultSolver", 2);
    observer.count = 0;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 1);
}

TEST_F(SketchObjectTest, recomputeDiagnosticObserverEditsAreSolved)
{
    auto* sketch = getObject();
    validLine(sketch);
    sketch->addConstraint(horizontal());
    bool edited = false;
    fastsignals::scoped_connection edit = sketch->signalChanged.connect(
        [&](const App::DocumentObject&, const App::Property& property) {
            if (&property == &sketch->Constraints && !edited && !sketch->getLastRedundant().empty()) {
                edited = true;
                const auto c = horizontal();
                const std::vector<Constraint*> list {c.get()};
                sketch->Constraints.setValues(list);
            }
        }
    );
    SetupCounter observer;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_TRUE(edited);
    EXPECT_EQ(observer.count, 2);
    EXPECT_EQ(sketch->Constraints.getSize(), 1);
    EXPECT_TRUE(sketch->getLastRedundant().empty());
    EXPECT_EQ(sketch->getLastDoF(), 3);
    EXPECT_EQ(sketch->getLastSolverStatus(), GCS::SolveStatus::Success);
}

TEST_F(SketchObjectTest, recomputeDiagnosticObserverGeometryTypeRepairIsSolved)
{
    auto* sketch = getObject();
    addLine(sketch);
    sketch->getDocument()->recompute();
    auto radius = std::make_unique<Constraint>();
    radius->Type = Radius;
    radius->setGeoId(0, 0);
    radius->setValue(5);
    sketch->addConstraint(std::move(radius));
    bool edited = false;
    fastsignals::scoped_connection edit = sketch->signalChanged.connect(
        [&](const App::DocumentObject&, const App::Property& property) {
            if (&property == &sketch->Constraints && !edited
                && !sketch->getLastMalformedConstraints().empty()) {
                edited = true;
                Part::GeomCircle circle;
                SketcherTestHelpers::setupCircle(circle);
                const std::vector<Part::Geometry*> geometry {&circle};
                sketch->Geometry.setValues(geometry);
            }
        }
    );
    SetupCounter observer;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_TRUE(edited);
    EXPECT_EQ(observer.count, 2);
    ASSERT_EQ(sketch->Constraints.getValues().size(), 1);
    EXPECT_EQ(sketch->getLastDoF(), 2);
    EXPECT_TRUE(sketch->getLastMalformedConstraints().empty());
    EXPECT_EQ(sketch->getLastSolverStatus(), GCS::SolveStatus::Success);
    auto* circle = dynamic_cast<const Part::GeomCircle*>(sketch->Geometry[0]);
    ASSERT_NE(circle, nullptr);
    EXPECT_NEAR(circle->getRadius(), 5, 1e-7);
}

TEST_F(SketchObjectTest, recomputeDiagnosticNotificationBaseExceptionKeepsRecovery)
{
    auto* sketch = getObject();
    validLine(sketch);
    sketch->addConstraint(horizontal());
    bool threw = false;
    fastsignals::scoped_connection throwing = sketch->signalChanged.connect(
        [&](const App::DocumentObject&, const App::Property& property) {
            if (&property == &sketch->Constraints && !threw && !sketch->getLastRedundant().empty()) {
                threw = true;
                throw Base::RuntimeError("diagnostic observer");
            }
        }
    );
    SetupCounter observer;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_TRUE(threw);
    EXPECT_EQ(observer.count, 2);
    EXPECT_EQ(sketch->getLastRedundant(), (std::vector<int> {2}));
    observer.count = 0;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 1);
}

TEST_F(SketchObjectTest, recomputeInternalIndicesValidateInvalidAndRepairedInputs)
{
    auto* sketch = getObject();
    addLine(sketch);
    addLine(sketch);
    sketch->addConstraint(horizontal(1));
    sketch->getDocument()->recompute();
    ASSERT_EQ(sketch->getLastDoF(), 7);
    Part::GeomLineSegment line;
    line.setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(4, 0, 0));
    const std::vector<Part::Geometry*> shortened {&line};
    sketch->Geometry.setValues(shortened);
    ASSERT_EQ(sketch->Constraints.getValuesForce().size(), 1);
    EXPECT_TRUE(sketch->Constraints.getValues().empty());
    sketch->getDocument()->recompute();
    EXPECT_TRUE(sketch->Constraints.getValues().empty());
    addLine(sketch);
    EXPECT_TRUE(sketch->Constraints.getValues().empty());
    SetupCounter observer;
    sketch->touch();
    sketch->getDocument()->recompute();
    EXPECT_EQ(observer.count, 1);
    ASSERT_EQ(sketch->Constraints.getValues().size(), 1);
    EXPECT_EQ(sketch->Constraints.getValues()[0]->getGeoId(0), 1);
    EXPECT_EQ(sketch->getLastDoF(), 7);
}

TEST_F(SketchObjectTest, recomputeMissingExternalIndexRepairsBeforeSolverConsumption)
{
    auto* sketch = getObject();
    addLine(sketch);
    auto* doc = sketch->getDocument();
    auto* box = doc->addObject("Part::Box");
    doc->recompute();
    std::string edge;
    for (int i = 1; i <= 12; ++i) {
        auto name = "Edge" + std::to_string(i);
        ASSERT_GE(sketch->addExternal(box, name.c_str()), 0);
        if (dynamic_cast<const Part::GeomLineSegment*>(sketch->getGeometry(-3))) {
            edge = name;
            break;
        }
        sketch->delExternal(0);
    }
    ASSERT_FALSE(edge.empty());
    auto c = std::make_unique<Constraint>();
    c->Type = PointOnObject;
    c->setGeoId(0, 0);
    c->setPosId(0, PointPos::start);
    c->setGeoId(1, -3);
    sketch->addConstraint(std::move(c));
    doc->recompute();
    ASSERT_EQ(sketch->getLastDoF(), 3);
    auto saved = std::unique_ptr<Constraint>(sketch->Constraints.getValues()[0]->clone());
    sketch->delExternal(0);
    // Reinstate the orphan as restoration can: preserve its identity while its
    // referenced projected geometry is absent.
    sketch->addConstraint(std::move(saved));
    doc->recompute();
    ASSERT_TRUE(sketch->Constraints.getValues().empty());
    ASSERT_EQ(sketch->Constraints.getValuesForce().size(), 1);
    ASSERT_GE(sketch->addExternal(box, edge.c_str()), 0);
    SetupCounter observer;
    sketch->touch();
    doc->recompute();
    EXPECT_EQ(observer.count, 1);
    ASSERT_EQ(sketch->Constraints.getValues().size(), 1);
    EXPECT_EQ(sketch->Constraints.getValues()[0]->getGeoId(1), -3);
    EXPECT_EQ(sketch->getLastDoF(), 3);
}
