// SPDX-License-Identifier: LGPL-2.1-or-later
// Qualification probes of the existing API, not the proposed implementation.
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <FCConfig.h>
#include <gtest/gtest.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Interpreter.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/ExternalGeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <src/App/InitApplication.h>

namespace
{
void py(const char* script)
{
    try {
        Base::Interpreter().runString(script);
    }
    catch (const Base::PyException& error) {
        throw std::runtime_error(std::string(error.what()) + "\n" + error.getStackTrace());
    }
    catch (const Base::Exception& error) {
        throw std::runtime_error(error.what());
    }
}
class T001NativeProbe: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        std::string executable = std::getenv("SPEC004_APP_EXECUTABLE");
        std::array<char*, 1> argv {executable.data()};
        App::Application::Config()["ExeName"] = "FreeCAD";
        App::Application::init(1, argv.data());
        py("import os, sys; sys.path[:0] = os.environ['SPEC004_MODULE_PATH'].split(os.pathsep)");
    }
    void SetUp() override
    {
        py(R"PY(
import FreeCAD as App, Part, Sketcher
d = App.newDocument('Spec004T001Probe')
d.UndoMode = 1
)PY");
        doc = App::GetApplication().getDocument("Spec004T001Probe");
        ASSERT_NE(doc, nullptr);
    }
    void TearDown() override
    {
        App::GetApplication().closeDocument("Spec004T001Probe");
    }
    void circles()
    {
        py(R"PY(
source = d.addObject('Part::Feature', 'Source')
source.Shape = Part.makeCompound([Part.makeCircle(3, App.Vector(x,0,0)) for x in (0,20,40)])
sketch = d.addObject('Sketcher::SketchObject','Sketch')
sketch.addExternal('Source','Edge1')
sketch.addExternal('Source','Edge2')
i = sketch.addGeometry(Part.Circle(App.Vector(0,0,0), App.Vector(0,0,1),1),False)
sketch.addConstraint(Sketcher.Constraint('Coincident',i,3,-3,3))
sketch.addConstraint(Sketcher.Constraint('Radius',i,1.0))
d.recompute()
d.clearUndos()
d.purgeTouched()
)PY");
        sk = static_cast<Sketcher::SketchObject*>(doc->getObject("Sketch"));
        source = doc->getObject("Source");
    }
    long id(int index) const
    {
        return Sketcher::ExternalGeometryFacade::getFacade(sk->getGeometry(index))->getId();
    }
    double x(int index) const
    {
        return sk->getGeometry<Part::GeomCircle>(index)->getCenter().x;
    }
    App::Document* doc = nullptr;
    App::DocumentObject* source = nullptr;
    Sketcher::SketchObject* sk = nullptr;
};

TEST_F(T001NativeProbe, legacyCircleRetargetKeepsIdsButAppendsLink)
{
    circles();
    const auto firstId = id(-3);
    const auto secondId = id(-4);
    const auto types = sk->ExternalTypes.getValues();
    ASSERT_EQ(sk->attachExternal({-3}, source, "Edge3"), 1);
    EXPECT_EQ(id(-3), firstId);
    EXPECT_EQ(id(-4), secondId);
    ASSERT_EQ(sk->ExternalGeometry.getSubValues().size(), 2U);
    EXPECT_EQ(sk->ExternalGeometry.getSubValues()[0], "Edge2");
    EXPECT_EQ(sk->ExternalGeometry.getSubValues()[1], "Edge3");
    EXPECT_EQ(sk->ExternalTypes.getValues(), types);
    EXPECT_NEAR(x(-3), 40.0, 1e-9);
    EXPECT_NEAR(x(-4), 20.0, 1e-9);
    doc->recompute();
    sk->touch();
    doc->recompute();
    EXPECT_EQ(id(-3), firstId);
    EXPECT_NEAR(x(-3), 40.0, 1e-9);
    EXPECT_EQ(sk->Constraints.getSize(), 2);
    RecordProperty("observed_link_order", "Edge2,Edge3");
}

TEST_F(T001NativeProbe, legacyElementObserverSeesMixedState)
{
    circles();
    int mixed = 0;
    int notifications = 0;
    auto conn = sk->signalElementsChanged.connect([&]() {
        ++notifications;
        const auto& subs = sk->ExternalGeometry.getSubValues();
        const bool replaced = std::find(subs.begin(), subs.end(), "Edge3") != subs.end();
        if (replaced && std::abs(x(-3)) < 1e-9) {
            ++mixed;
        }
    });
    EXPECT_EQ(sk->attachExternal({-3}, source, "Edge3"), 1);
    conn.disconnect();
    EXPECT_GT(mixed, 0);
    RecordProperty("mixed_notifications", mixed);
    RecordProperty("element_notifications", notifications);
}

TEST_F(T001NativeProbe, disableNotifyStillPublishesPropertySignal)
{
    circles();
    const auto objs = sk->ExternalGeometry.getValues();
    auto subs = sk->ExternalGeometry.getSubValues();
    subs[0] = "Edge3";
    int count = 0;
    auto conn = sk->ExternalGeometry.signalChanged.connect([&](const App::Property&) { ++count; });
    const bool oldNotify = sk->ExternalGeometry.enableNotify(false);
    sk->ExternalGeometry.setValues(objs, subs);
    sk->ExternalGeometry.enableNotify(oldNotify);
    conn.disconnect();
    EXPECT_GT(count, 0);
    RecordProperty("property_notifications_with_parent_notify_disabled", count);
}

TEST_F(T001NativeProbe, setterRestorationLeavesTouchedState)
{
    circles();
    const auto objs = sk->ExternalGeometry.getValues();
    const auto original = sk->ExternalGeometry.getSubValues();
    EXPECT_FALSE(sk->isTouched());
    auto changed = original;
    changed[0] = "Edge3";
    sk->ExternalGeometry.setValues(objs, changed);
    sk->ExternalGeometry.setValues(objs, original);
    EXPECT_EQ(sk->ExternalGeometry.getSubValues(), original);
    EXPECT_TRUE(sk->isTouched());
    EXPECT_TRUE(sk->ExternalGeometry.isTouched());
    RecordProperty("restored_values_but_touched", "true");
}

TEST_F(T001NativeProbe, setterRestorationCannotRecoverClearedRedo)
{
    circles();
    doc->openTransaction(std::string("earlier"));
    sk->Label.setValue("Earlier label");
    doc->commitTransaction();
    doc->undo();
    ASSERT_EQ(doc->getAvailableRedos(), 1);
    const std::string original = sk->Label.getValue();
    doc->openTransaction(std::string("booked"));
    const int redosBefore = doc->getAvailableRedos();
    sk->Label.setValue("Temporary label");
    sk->Label.setValue(original);
    EXPECT_EQ(std::string(sk->Label.getValue()), original);
    EXPECT_EQ(redosBefore, 1);
    EXPECT_EQ(doc->getAvailableRedos(), 0);
    RecordProperty("redos_before", redosBefore);
    RecordProperty("redos_after_value_restore", doc->getAvailableRedos());
    doc->abortTransaction();
}

TEST_F(T001NativeProbe, legacyDuplicateAdmissionIsNonmutating)
{
    circles();
    const auto subs = sk->ExternalGeometry.getSubValues();
    const auto first = id(-3);
    int events = 0;
    auto conn = sk->signalElementsChanged.connect([&]() { ++events; });
    EXPECT_EQ(sk->attachExternal({-3}, source, "Edge2"), -1);
    conn.disconnect();
    EXPECT_EQ(sk->ExternalGeometry.getSubValues(), subs);
    EXPECT_EQ(id(-3), first);
    EXPECT_EQ(events, 0);
}

TEST_F(T001NativeProbe, legacyReversedLineReencodesOrientation)
{
    py(R"PY(
source = d.addObject('Part::Feature','Source')
source.Shape = Part.makeCompound([
    Part.makeLine(App.Vector(0,0,0),App.Vector(10,0,0)),
    Part.makeLine(App.Vector(10,0,0),App.Vector(0,0,0))])
sketch = d.addObject('Sketcher::SketchObject','Sketch')
sketch.addExternal('Source','Edge1')
i = sketch.addGeometry(Part.Circle(App.Vector(5,2,0),App.Vector(0,0,1),1),False)
sketch.addConstraint(Sketcher.Constraint('Radius',i,1.0))
sketch.addConstraint(Sketcher.Constraint('Distance',i,3,-3,2.0))
d.recompute()
)PY");
    sk = static_cast<Sketcher::SketchObject*>(doc->getObject("Sketch"));
    source = doc->getObject("Source");
    const auto orientation = sk->Constraints.getValues()[1]->Orientation.toUnderlyingType();
    EXPECT_EQ(sk->attachExternal({-3}, source, "Edge2"), 0);
    const auto after = sk->Constraints.getValues()[1]->Orientation.toUnderlyingType();
    EXPECT_NE(orientation, after);
    doc->recompute();
    sk->touch();
    doc->recompute();
    EXPECT_EQ(sk->Constraints.getValues()[1]->Orientation.toUnderlyingType(), after);
    EXPECT_NEAR(sk->getGeometry<Part::GeomCircle>(0)->getCenter().y, 2.0, 1e-7);
    RecordProperty("orientation_changed", "true");
}

TEST_F(T001NativeProbe, callerAbortRecoversUnexpectedApplyFailure)
{
    circles();
    const auto originalLinks = sk->ExternalGeometry.getSubValues();
    const auto originalId = id(-3);
    const std::string originalLabel = sk->Label.getValue();
    doc->openTransaction(std::string("caller-owned reattachment"));
    sk->Label.setValue("Earlier edit in caller transaction");
    bool inject = true;
    auto conn = sk->signalElementsChanged.connect([&]() {
        const auto& subs = sk->ExternalGeometry.getSubValues();
        if (inject && std::find(subs.begin(), subs.end(), "Edge3") != subs.end()) {
            inject = false;
            throw std::runtime_error("T001 injected unexpected application failure");
        }
    });
    EXPECT_THROW(sk->attachExternal({-3}, source, "Edge3"), std::runtime_error);
    conn.disconnect();
    EXPECT_FALSE(inject);
    EXPECT_TRUE(doc->hasPendingTransaction());
    EXPECT_EQ(std::string(sk->Label.getValue()), "Earlier edit in caller transaction");
    EXPECT_NE(sk->ExternalGeometry.getSubValues(), originalLinks);
    // Recovery is explicitly caller-owned and reverts earlier edits too.
    doc->abortTransaction();
    doc->recompute();
    EXPECT_EQ(sk->ExternalGeometry.getSubValues(), originalLinks);
    EXPECT_EQ(id(-3), originalId);
    EXPECT_NEAR(x(-3), 0.0, 1e-9);
    EXPECT_EQ(std::string(sk->Label.getValue()), originalLabel);
    EXPECT_EQ(sk->solve(), Sketcher::SketchSolveStatus::Success);
    RecordProperty("caller_abort_restored_model_and_earlier_edit", "true");
}
}  // namespace
