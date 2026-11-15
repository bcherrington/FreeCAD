// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include <Base/Interpreter.h>
#include <src/App/InitApplication.h>

namespace
{
class SimulationArgumentsTest: public ::testing::TestWithParam<std::string>
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

TEST_P(SimulationArgumentsTest, RejectsInvalidInputBeforeNativeMutation)
{
    const std::string script = "simulation_argument_kind = '" + GetParam() + "'\n" + R"PY(
import FreeCAD as App
import AssemblyApp
import Part
import contextlib

def check_simulation_argument(kind):
    document = App.newDocument()
    other = None
    try:
        with AssemblyApp.suspendAutomaticMotion(document):
            assembly = document.addObject("Assembly::AssemblyObject", "Assembly")
            assembly.newObject("Assembly::JointGroup", "Joints")
            simulation = document.addObject("App::FeaturePython", "Simulation")
            simulation.addExtension("App::GroupExtensionPython")
            component = assembly.newObject("Part::Box", "Component")
            array = assembly.newObject("App::Link", "Array")
            array.LinkedObject = component
            array.ElementCount = 2
            document.recompute()
        # Initialize native frame storage through an ordinary, live argument.
        assert assembly.generateSimulation(simulation) == 0
        initial_frames = assembly.numberOfFrames()
        assert initial_frames > 0
        if kind == "Deleted":
            argument = document.addObject("App::FeaturePython", "Deleted")
            document.removeObject(argument.Name)
        elif kind == "ClosedDocument":
            other = App.newDocument()
            argument = other.addObject("App::FeaturePython", "Deleted")
            App.closeDocument(other.Name)
            other = None
        else:
            argument = {"None": None, "Integer": 1, "PlainObject": object(),
                        "String": "simulation", "List": []}.get(kind)
        error_type = ReferenceError if kind in ("Deleted", "ClosedDocument") else TypeError

        for suspended in (False, True):
            scope = AssemblyApp.suspendAutomaticMotion(document) if suspended else contextlib.nullcontext()
            with scope:
                array.Placement.Base = App.Vector(13, 17, 19)
                document.purgeTouched()
                def snapshot():
                    return (tuple(array.Placement.toMatrix().A),
                            tuple(component.Placement.toMatrix().A), component.Shape.Volume,
                            tuple((obj.Name, tuple(obj.State)) for obj in document.Objects),
                            tuple(obj.Name for obj in assembly.Group),
                            document.isTouched(), document.UndoCount, document.RedoCount,
                            assembly.numberOfFrames())
                before = snapshot()
                for attempt in range(2):
                    try:
                        if kind == "NoArgument":
                            assembly.generateSimulation()
                        elif kind == "ExtraArgument":
                            assembly.generateSimulation(simulation, simulation)
                        else:
                            assembly.generateSimulation(argument)
                    except error_type:
                        pass
                    else:
                        raise AssertionError((kind, suspended, "invalid argument accepted"))
                    assert snapshot() == before, (kind, suspended, "native state changed")

                # Rejection leaves the same assembly able to accept a live argument.
                expected = -7 if suspended else 0
                assert assembly.generateSimulation(simulation) == expected
                assert assembly.numberOfFrames() == initial_frames
        # No deferred simulation runs when motion suspension is released.
        assert assembly.numberOfFrames() == initial_frames
    finally:
        if other is not None:
            App.closeDocument(other.Name)
        App.closeDocument(document.Name)

check_simulation_argument(simulation_argument_kind)
)PY";
    try {
        Base::Interpreter().runString(script.c_str());
    }
    catch (const Base::Exception& error) {
        FAIL() << error.what();
    }
}

INSTANTIATE_TEST_SUITE_P(
    AssemblyBoundary,
    SimulationArgumentsTest,
    ::testing::ValuesIn(
        std::vector<std::string> {
            "None",
            "Integer",
            "PlainObject",
            "String",
            "List",
            "Deleted",
            "ClosedDocument",
            "NoArgument",
            "ExtraArgument"
        }
    ),
    [](const ::testing::TestParamInfo<std::string>& info) { return info.param; }
);

class SimulationLiveArgumentsTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

TEST_F(SimulationLiveArgumentsTest, AcceptsDocumentObjectSubtypesAndPreservesNativeStatuses)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import FreeCAD as App
import AssemblyApp
import Part

def check_live_simulation_arguments():
    document = App.newDocument()
    try:
        with AssemblyApp.suspendAutomaticMotion(document):
            assembly = document.addObject("Assembly::AssemblyObject", "Assembly")
            assembly.newObject("Assembly::JointGroup", "Joints")
            generic = document.addObject("App::FeaturePython", "GenericSimulation")
            generic.addExtension("App::GroupExtensionPython")
            derived = document.addObject("Part::FeaturePython", "DerivedSimulation")
            derived.addExtension("App::GroupExtensionPython")
            assert isinstance(generic, App.DocumentObject)
            assert isinstance(derived, App.DocumentObject)
            assert type(derived) is not type(generic)
            for argument in (generic, derived):
                assert assembly.generateSimulation(argument) == -7
        for argument in (generic, derived):
            assert assembly.generateSimulation(argument) == 0
            assert assembly.numberOfFrames() > 0
        with AssemblyApp.suspendAutomaticMotion(document):
            assert assembly.generateSimulation(derived) == -7
        assert assembly.generateSimulation(generic) == 0
    finally:
        App.closeDocument(document.Name)

check_live_simulation_arguments()
)PY"));
}
}  // namespace
