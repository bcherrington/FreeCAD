// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/Link.h>
#include <App/ObjectIdentifier.h>
#include <App/Part.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyUtils.h>
#include <Mod/Assembly/App/Groups.h>
#include <Mod/Assembly/App/MotionSuspension.h>
#include <src/App/InitApplication.h>

class AssemblyObjectTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        auto _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _assemblyObj = _doc->addObject<Assembly::AssemblyObject>();
        _jointGroupObj = _assemblyObj->addObject<Assembly::JointGroup>("jointGroupTest");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    Assembly::AssemblyObject* getObject()
    {
        return _assemblyObj;
    }

private:
    // TODO: use shared_ptr or something else here?
    Assembly::AssemblyObject* _assemblyObj;
    Assembly::JointGroup* _jointGroupObj;
    std::string _docName;
};

TEST_F(AssemblyObjectTest, createAssemblyObject)  // NOLINT
{
    // Arrange

    // Act

    // Assert
}

TEST_F(AssemblyObjectTest, automaticMotionScopeNestsAndIsolatesDocuments)
{
    auto& document = *getObject()->getDocument();
    EXPECT_FALSE(Assembly::isAutomaticMotionSuspended(&document));
    const auto otherName = App::GetApplication().getUniqueDocumentName("motionOther");
    auto* other = App::GetApplication().newDocument(otherName.c_str());
    {
        Assembly::AutomaticMotionSuspension outer(document);
        EXPECT_TRUE(Assembly::isAutomaticMotionSuspended(&document));
        EXPECT_FALSE(Assembly::isAutomaticMotionSuspended(other));
        {
            Assembly::AutomaticMotionSuspension inner(document);
            Assembly::AutomaticMotionSuspension otherScope(*other);
            EXPECT_TRUE(Assembly::isAutomaticMotionSuspended(&document));
            EXPECT_TRUE(Assembly::isAutomaticMotionSuspended(other));
            inner.release();
            inner.release();
            EXPECT_TRUE(Assembly::isAutomaticMotionSuspended(&document));
        }
        EXPECT_TRUE(Assembly::isAutomaticMotionSuspended(&document));
        EXPECT_FALSE(Assembly::isAutomaticMotionSuspended(other));
        EXPECT_THROW(
            [&document]() {
                Assembly::AutomaticMotionSuspension exceptional(document);
                throw std::runtime_error("scope unwind");
            }(),
            std::runtime_error
        );
        EXPECT_TRUE(Assembly::isAutomaticMotionSuspended(&document));
    }
    EXPECT_FALSE(Assembly::isAutomaticMotionSuspended(&document));
    App::GetApplication().closeDocument(otherName.c_str());
}

TEST_F(AssemblyObjectTest, automaticMotionScopeSurvivesCloseAndNameReuse)
{
    const auto name = App::GetApplication().getUniqueDocumentName("motionClose");
    auto* document = App::GetApplication().newDocument(name.c_str());
    std::unique_ptr<Assembly::AutomaticMotionSuspension> newScope;
    {
        Assembly::AutomaticMotionSuspension scope(*document);
        // The scope exists before either the assembly or its joint group.
        auto* assembly = document->addObject<Assembly::AssemblyObject>();
        assembly->addObject<Assembly::JointGroup>("Joints");
        EXPECT_TRUE(Assembly::isAutomaticMotionSuspended(document));
        App::GetApplication().closeDocument(name.c_str());
        auto* reopened = App::GetApplication().newDocument(name.c_str());
        EXPECT_FALSE(Assembly::isAutomaticMotionSuspended(reopened));
        newScope = std::make_unique<Assembly::AutomaticMotionSuspension>(*reopened);
        EXPECT_TRUE(Assembly::isAutomaticMotionSuspended(reopened));
    }
    auto* reopened = App::GetApplication().getDocument(name.c_str());
    // Releasing the dead document's handle must not release a live scope even
    // if the allocator reused the old document address.
    EXPECT_TRUE(Assembly::isAutomaticMotionSuspended(reopened));
    newScope.reset();
    EXPECT_FALSE(Assembly::isAutomaticMotionSuspended(reopened));
    App::GetApplication().closeDocument(name.c_str());
}

TEST_F(AssemblyObjectTest, automaticMotionStopsBeforeNativePlacementPreparation)
{
    auto* assembly = getObject();
    auto* document = assembly->getDocument();
    auto* source = document->addObject<App::Part>("ArraySource");
    App::Link* array = nullptr;
    const Base::Placement placement(Base::Vector3d(23, 47, 59), Base::Rotation());
    {
        Assembly::AutomaticMotionSuspension scope(*document);
        array = assembly->addObject<App::Link>("Array");
        array->LinkedObject.setValue(source);
        array->ElementCount.setValue(2);
        array->Placement.setValue(placement);
        ASSERT_TRUE(array->isLinkGroup());
        const auto degreesOfFreedom = assembly->getLastDoF();
        const auto solverStatus = assembly->getLastSolverStatus();

        // A nonidentity array is an oracle for native preparation: the first
        // step of an unguarded solve normalizes it, even without grounded parts.
        assembly->addObject<Assembly::JointGroup>("AdditionalJoints");
        assembly->updateSolveStatus();
        document->recompute();
        EXPECT_EQ(assembly->solve(), -7);
        EXPECT_EQ(assembly->generateSimulation(nullptr), -7);
        EXPECT_EQ(assembly->updateForFrame(0), -7);
        assembly->ensureIdentityPlacements();
        assembly->undoSolve();
        EXPECT_TRUE(array->Placement.getValue().isSame(placement, 1e-9));
        EXPECT_EQ(assembly->getLastDoF(), degreesOfFreedom);
        EXPECT_EQ(assembly->getLastSolverStatus(), solverStatus);
    }
    // No replay on scope exit; the normal native helper still works afterwards.
    EXPECT_TRUE(array->Placement.getValue().isSame(placement, 1e-9));
    assembly->ensureIdentityPlacements();
    EXPECT_TRUE(array->Placement.getValue().isIdentity());
}

namespace
{
class HasherMismatchObserver: public Base::ILogger
{
public:
    HasherMismatchObserver()
    {
        Base::Console().attachObserver(this);
    }

    ~HasherMismatchObserver() override
    {
        Base::Console().detachObserver(this);
    }

    const char* name() override
    {
        return "AssemblyHasherMismatchObserver";
    }

    void sendLog(
        const std::string&,
        const std::string& message,
        Base::LogStyle,
        Base::IntendedRecipient,
        Base::ContentType
    ) override
    {
        if (message.find("hasher mismatch") != std::string::npos) {
            ++mismatches;
        }
    }

    int mismatches = 0;
};
}  // namespace

TEST_F(AssemblyObjectTest, jointSnappingToExternalBody)
{
    HasherMismatchObserver observer;
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import os
import tempfile
import FreeCAD as App
import Part
import Sketcher
import UtilsAssembly

with tempfile.TemporaryDirectory() as directory:
    source = App.newDocument()
    target = App.newDocument()
    try:
        body = source.addObject("PartDesign::Body", "Body")
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        points = [App.Vector(0, 0, 0), App.Vector(2, 0, 0),
                  App.Vector(2, 3, 0), App.Vector(0, 3, 0)]
        sketch.addGeometry([Part.LineSegment(points[i], points[(i + 1) % 4])
                            for i in range(4)], False)
        pad = body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Length = 7
        source.recompute()
        source.saveAs(os.path.join(directory, "Source.FCStd"))
        source_map = body.Shape.ElementMap
        assert source_map

        assembly = target.addObject("Assembly::AssemblyObject", "Assembly")
        target.saveAs(os.path.join(directory, "Assembly.FCStd"))
        link = assembly.newObject("App::Link", "Link")
        link.LinkedObject = body
        link.Placement = App.Placement(App.Vector(10, 20, 30), App.Rotation(15, 25, 35))
        target.recompute()
        link_map = link.Shape.ElementMap
        shape = Part.getShape(body, noElementMap=True)
        assert shape.isPartner(body.Shape)
        owner_shape = UtilsAssembly.getReferenceOwnerShape([assembly, [link.Name + ".Face1"]])
        assert owner_shape is not None
        assert owner_shape.isPartner(link.Shape)

        for token in ("Face1", "Edge1", "Vertex1"):
            element = shape.getElement(token)
            point = element.Point if token == "Vertex1" else element.Vertexes[0].Point
            ref = [link, [token]]
            mouse = UtilsAssembly.getGlobalPlacement(ref).multVec(point)
            vertex = UtilsAssembly.findElementClosestVertex(ref, mouse)
            assert vertex == "Vertex1", (token, vertex)
            ref = UtilsAssembly.addVertexToReference(ref, vertex)
            placement = UtilsAssembly.findPlacement(ref)
            assert placement.Base.isEqual(point, 1e-7), (token, placement.Base, point)
            vertex_shape = Part.getShape(link, noElementMap=True).getElement(vertex)
            assert UtilsAssembly.findVertexNameInObject(vertex_shape, link) == vertex

        assert body.Shape.ElementMap == source_map
        assert link.Shape.ElementMap == link_map
    finally:
        App.closeDocument(target.Name)
        App.closeDocument(source.Name)
)PY"));
    EXPECT_EQ(observer.mismatches, 0);
}

TEST_F(AssemblyObjectTest, validatedSolveRejectsEscapedTargetsBeforePreparation)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import FreeCAD as App
import AssemblyApp
import Part
import JointObject
import os
import tempfile

def check_rejected_motion_graph(kind):
    doc = App.newDocument()
    external = None
    temporary = tempfile.TemporaryDirectory()
    try:
        with AssemblyApp.suspendAutomaticMotion(doc):
            assembly = doc.addObject("Assembly::AssemblyObject", "Assembly")
            joints = assembly.newObject("Assembly::JointGroup", "Joints")
            local = assembly.newObject("Part::Box", "Local")
            escaped = doc.addObject("Part::Box", "OutsideAssembly")
            escaped.Placement.Base = App.Vector(71, 83, 97)
            # If native preparation runs, it normalizes this nonidentity array.
            array = assembly.newObject("App::Link", "Array")
            array.LinkedObject = local
            array.ElementCount = 2
            array.Placement.Base = App.Vector(13, 17, 19)
            if kind in ("endpoint", "external_endpoint"):
                if kind == "external_endpoint":
                    external = App.newDocument()
                    escaped = external.addObject("Part::Box", "SourceComponent")
                    escaped.Placement.Base = App.Vector(71, 83, 97)
                    external.recompute()
                    external.saveAs(os.path.join(temporary.name, "EscapedSource.FCStd"))
                    doc.saveAs(os.path.join(temporary.name, "EscapedTarget.FCStd"))
                bad = joints.newObject("App::FeaturePython", "EscapedEndpoint")
                JointObject.Joint(bad, JointObject.JointTypes.index("Fixed"))
                bad.Reference1 = (local, ["Face1"])
                bad.Reference2 = (escaped, ["Face1"])
            elif kind == "ground":
                bad = joints.newObject("App::FeaturePython", "EscapedGround")
                bad.addProperty("App::PropertyLinkGlobal", "ObjectToGround")
                bad.ObjectToGround = escaped
            elif kind == "rigid":
                bad = joints.newObject("App::FeaturePython", "EscapedRigid")
                bad.addProperty("App::PropertyBool", "Suppressed")
                bad.addProperty("App::PropertyLinkList", "ObjectsToRigidGroup")
                bad.ObjectsToRigidGroup = [local, escaped]
            elif kind == "motion":
                simulations = assembly.newObject("Assembly::SimulationGroup", "Simulations")
                simulation = simulations.newObject("App::DocumentObjectGroup", "Simulation")
                bad = simulation.newObject("App::FeaturePython", "EscapedMotion")
                # This is the exact property class used by native Motion objects.
                bad.addProperty("App::PropertyXLinkSubHidden", "Joint")
                bad.Joint = (escaped, [""])
                bad.addProperty("App::PropertyEnumeration", "MotionType")
                bad.MotionType = ["Angular", "Linear"]
                bad.addProperty("App::PropertyString", "Formula")
                bad.Formula = "time"
            else:
                raise AssertionError(kind)
            doc.recompute()
            doc.purgeTouched()
            rows = [(obj.Name, tuple(obj.State),
                     obj.Placement.toMatrix().A if hasattr(obj, "Placement") else None)
                    for obj in doc.Objects]
            touched = doc.isTouched()
            undos = doc.UndoCount
            if external is not None:
                source_pose = escaped.Placement.toMatrix().A
                source_state = tuple(escaped.State)
                source_touched = external.isTouched()
            # No wrapper intercepts solveValidated: it executes the native preflight.
            assert assembly.solveValidated(True) == -8, kind
            after = [(obj.Name, tuple(obj.State),
                      obj.Placement.toMatrix().A if hasattr(obj, "Placement") else None)
                     for obj in doc.Objects]
            assert rows == after, (kind, rows, after)
            assert doc.isTouched() == touched, kind
            assert doc.UndoCount == undos, kind
            if external is not None:
                assert escaped.Placement.toMatrix().A == source_pose
                assert tuple(escaped.State) == source_state
                assert external.isTouched() == source_touched
    finally:
        App.closeDocument(doc.Name)
        if external is not None:
            App.closeDocument(external.Name)
        temporary.cleanup()

for kind in ("endpoint", "external_endpoint", "ground", "rigid", "motion"):
    check_rejected_motion_graph(kind)
)PY"));
}

TEST_F(AssemblyObjectTest, validatedSolveRejectsIncompleteCyclicAndUnsupportedGraphs)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import FreeCAD as App
import AssemblyApp
import Part
import JointObject

def check_malformed_motion_graph(kind):
    doc = App.newDocument()
    cycle = None
    try:
        with AssemblyApp.suspendAutomaticMotion(doc):
            assembly = doc.addObject("Assembly::AssemblyObject", "Assembly")
            joints = assembly.newObject("Assembly::JointGroup", "Joints")
            first = assembly.newObject("Part::Box", "First")
            second = assembly.newObject("Part::Box", "Second")
            second.Placement.Base = App.Vector(11, 13, 17)
            doc.recompute()
            joint = joints.newObject("App::FeaturePython", "Joint")
            JointObject.Joint(joint, JointObject.JointTypes.index("Fixed"))
            joint.Proxy.setJointConnectors(
                joint, [(first, ["Face1"]), (second, ["Face1"])])
            if kind == "incomplete_joint":
                joint.Reference2 = None
            elif kind == "containment_cycle":
                cycle = assembly.newObject("App::FeaturePython", "RawGroup")
                cycle.addProperty("App::PropertyLinkList", "Group")
                cycle.Group = [assembly]
            else:
                simulations = assembly.newObject("Assembly::SimulationGroup", "Simulations")
                if kind == "malformed_simulation":
                    simulations.newObject("App::FeaturePython", "MissingGroup")
                else:
                    simulation = simulations.newObject("App::DocumentObjectGroup", "Simulation")
                    motion = simulation.newObject("App::FeaturePython", "Motion")
                    if kind != "incomplete_motion":
                        motion.addProperty("App::PropertyXLinkSubHidden", "Joint")
                        motion.Joint = (joint, [""])
                    motion.addProperty("App::PropertyEnumeration", "MotionType")
                    motion.MotionType = ["Angular", "Unsupported"]
                    motion.MotionType = "Unsupported" if kind == "unsupported_motion" else "Angular"
                    motion.addProperty("App::PropertyString", "Formula")
                    motion.Formula = "time"
            # Deliberately do not recompute the malformed graph: admission must
            # reject its raw shape before filtering, repair or dependency traversal.
            poses = [(obj.Name, obj.Placement.toMatrix().A)
                     for obj in doc.Objects if hasattr(obj, "Placement")]
            rows = [(obj.Name, tuple(obj.State)) for obj in doc.Objects]
            touched = doc.isTouched()
            undos = doc.UndoCount
            assert assembly.solveValidated(True) == -8, kind
            assert poses == [(obj.Name, obj.Placement.toMatrix().A)
                             for obj in doc.Objects if hasattr(obj, "Placement")], kind
            assert rows == [(obj.Name, tuple(obj.State)) for obj in doc.Objects], kind
            assert doc.isTouched() == touched, kind
            assert doc.UndoCount == undos, kind
            if cycle is not None:
                cycle.Group = []
    finally:
        if cycle is not None:
            cycle.Group = []
        App.closeDocument(doc.Name)

for kind in ("incomplete_joint", "containment_cycle", "incomplete_motion",
             "unsupported_motion", "incompatible_motion", "malformed_simulation"):
    check_malformed_motion_graph(kind)
)PY"));
}

TEST_F(AssemblyObjectTest, validatedSolveMovesLocalLinksAndPreservesExternalSource)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import os
import tempfile
import FreeCAD as App
import AssemblyApp
import Part
import JointObject

with tempfile.TemporaryDirectory() as directory:
    source = App.newDocument()
    target = App.newDocument()
    try:
        geometry = source.addObject("Part::Box", "Geometry")
        geometry.Placement.Base = App.Vector(101, 103, 107)
        source.recompute()
        source.saveAs(os.path.join(directory, "MotionSource.FCStd"))
        source.purgeTouched()
        source_pose = geometry.Placement.toMatrix().A
        source_state = tuple(geometry.State)
        source_touched = source.isTouched()
        with AssemblyApp.suspendAutomaticMotion(target):
            assembly = target.addObject("Assembly::AssemblyObject", "Assembly")
            target.saveAs(os.path.join(directory, "MotionTarget.FCStd"))
            joints = assembly.newObject("Assembly::JointGroup", "Joints")
            fixed = assembly.newObject("App::Link", "Fixed")
            fixed.LinkedObject = geometry
            moving = assembly.newObject("App::Link", "Moving")
            moving.LinkedObject = geometry
            moving.Placement.Base = App.Vector(23, 29, 31)
            target.recompute()
            ground = joints.newObject("App::FeaturePython", "Ground")
            JointObject.GroundedJoint(ground, fixed)
            joint = joints.newObject("App::FeaturePython", "FixedJoint")
            JointObject.Joint(joint, JointObject.JointTypes.index("Fixed"))
            joint.Proxy.setJointConnectors(
                joint, [(fixed, ["Face1"]), (moving, ["Face1"])])
            before = moving.Placement
            assert assembly.solve() == -7
            assert assembly.solveValidated() == 0
            assert AssemblyApp.isAutomaticMotionSuspended(target)
            assert not moving.Placement.isSame(before, 1e-7)
            assert moving.Placement.isSame(fixed.Placement, 1e-6)
            assert geometry.Placement.toMatrix().A == source_pose
            assert tuple(geometry.State) == source_state
            assert source.isTouched() == source_touched
    finally:
        App.closeDocument(target.Name)
        App.closeDocument(source.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, suspendedPopulatedLinkModeChangeRejectsBeforeMutation)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import FreeCAD as App
import AssemblyApp
import Part
import JointObject

doc = App.newDocument()
try:
    with AssemblyApp.suspendAutomaticMotion(doc):
        source = doc.addObject("Assembly::AssemblyObject", "SourceAssembly")
        source.newObject("Assembly::JointGroup", "SourceJoints")
        source.newObject("Part::Box", "SourceBox")
        target = doc.addObject("Assembly::AssemblyObject", "TargetAssembly")
        joints = target.newObject("Assembly::JointGroup", "TargetJoints")
        link = target.newObject("Assembly::AssemblyLink", "Instance")
        # Empty initial mode selection must remain usable.
        link.Rigid = False
        link.LinkedObject = source
        doc.recompute()
        assert link.Group
        link.Placement.Base = App.Vector(37, 41, 43)
        ground = joints.newObject("App::FeaturePython", "PreservedGround")
        ground.addProperty("App::PropertyLinkGlobal", "ObjectToGround")
        ground.ObjectToGround = link
        mode = link.Rigid
        group = tuple(obj.Name for obj in link.Group)
        objects = tuple(obj.Name for obj in doc.Objects)
        poses = [(obj.Name, obj.Placement.toMatrix().A)
                 for obj in doc.Objects if hasattr(obj, "Placement")]
        try:
            link.Rigid = not mode
        except (RuntimeError, ValueError):
            pass
        else:
            raise AssertionError("populated mode conversion was accepted while suspended")
        assert link.Rigid == mode
        assert tuple(obj.Name for obj in link.Group) == group
        assert tuple(obj.Name for obj in doc.Objects) == objects
        assert ground.ObjectToGround == link
        assert poses == [(obj.Name, obj.Placement.toMatrix().A)
                         for obj in doc.Objects if hasattr(obj, "Placement")]
finally:
    App.closeDocument(doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, suspendedCopyAndRestorePreserveSerializedPlacements)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import os
import tempfile
import FreeCAD as App
import AssemblyApp
import Part

with tempfile.TemporaryDirectory() as directory:
    source = App.newDocument()
    target = App.newDocument()
    try:
        with AssemblyApp.suspendAutomaticMotion(source):
            assembly = source.addObject("Assembly::AssemblyObject", "Assembly")
            assembly.newObject("Assembly::JointGroup", "Joints")
            box = assembly.newObject("Part::Box", "Box")
            array = assembly.newObject("App::Link", "Array")
            array.LinkedObject = box
            array.ElementCount = 2
            array.Placement.Base = App.Vector(53, 61, 67)
            source.recompute()
            source.saveAs(os.path.join(directory, "CopySource.FCStd"))
            source_pose = array.Placement.toMatrix().A
            source_poses = {obj.Name: obj.Placement.toMatrix().A for obj in source.Objects
                            if hasattr(obj, "Placement")}
            with AssemblyApp.suspendAutomaticMotion(target):
                copied = target.copyObject(assembly, True)
                target.recompute()
                copied_array = next(obj for obj in copied.Group
                                    if obj.isDerivedFrom("App::Link"))
                assert copied_array.Placement.toMatrix().A == source_pose
                assert array.Placement.toMatrix().A == source_pose
                target.saveAs(os.path.join(directory, "CopyTarget.FCStd"))
                copied_name = copied.Name

                def component_poses(root):
                    poses = {}
                    for component in root.Group:
                        if component.isDerivedFrom("App::Link") and component.ElementCount:
                            for element in component.ElementList:
                                path = component.Name + "." + element.Name + "."
                                poses[path] = root.getPlacementOf(path).toMatrix().A
                        elif hasattr(component, "Placement"):
                            path = component.Name + "."
                            poses[path] = root.getPlacementOf(path).toMatrix().A
                    return poses

                expected = component_poses(copied)
                # restore() uses the already scoped live document, rather than
                # opening a document before suspension can be established.
                # App::Link may canonicalize serialized array coordinates even
                # under an ordinary App::Part; physical component poses stay fixed.
                target.restore()
                assert AssemblyApp.isAutomaticMotionSuspended(target)
                restored = target.getObject(copied_name)
                assert expected == component_poses(restored)
                restored.updateSolveStatus()
                assert expected == component_poses(restored)
            assert array.Placement.toMatrix().A == source_pose
            assert source_poses == {obj.Name: obj.Placement.toMatrix().A for obj in source.Objects
                                    if hasattr(obj, "Placement")}
    finally:
        App.closeDocument(target.Name)
        App.closeDocument(source.Name)
)PY"));
}

namespace
{
// Native-owned fixtures: saved XLinks, real joint proxies, and the persisted
// Motion schema. CommandCreateSimulation itself requires a GUI process.
constexpr const char* mixedAssemblyFixture = R"PY(
import os
import tempfile
from contextlib import contextmanager, ExitStack
import FreeCAD as App
import AssemblyApp
import JointObject
import Part

def mixed_closure(owner):
    result = []
    def visit(obj):
        if obj in result:
            return
        result.append(obj)
        for child in getattr(obj, "Group", []):
            visit(child)
    visit(owner)
    return result

def mixed_joint(owner):
    return next(obj for obj in mixed_closure(owner)
                if getattr(obj, "JointType", None) == "Revolute")

def mixed_parts(owner):
    result = {}
    for obj in mixed_closure(owner):
        if obj.TypeId == "App::Link":
            source = obj.getLinkedObject(True)
            if source and source.Name in ("Base", "Arm"):
                result[source.Name] = obj
    assert set(result) == {"Base", "Arm"}, result
    return result

def mixed_snapshot(doc):
    return ([(obj.Name, tuple(obj.State),
              tuple(obj.Placement.toMatrix().A) if hasattr(obj, "Placement") else None,
              tuple(obj.LinkPlacement.toMatrix().A) if hasattr(obj, "LinkPlacement") else None,
              tuple(obj.getPropertyStatus("Placement")) if hasattr(obj, "Placement") else None,
              tuple(obj.getPropertyStatus("LinkPlacement")) if hasattr(obj, "LinkPlacement") else None)
             for obj in doc.Objects], doc.isTouched(), doc.UndoCount)

def mixed_motion(owner, joint, kind="Angular"):
    simulations = next((obj for obj in owner.Group
                        if obj.TypeId == "Assembly::SimulationGroup"), None)
    if simulations is None:
        simulations = owner.newObject("Assembly::SimulationGroup", "Simulations")
    simulation = simulations.newObject("App::DocumentObjectGroup", "Simulation")
    motion = simulation.newObject("App::FeaturePython", "Motion")
    motion.addProperty("App::PropertyXLinkSubHidden", "Joint")
    motion.Joint = (joint, [""])
    motion.addProperty("App::PropertyEnumeration", "MotionType")
    motion.MotionType = ["Angular", "Linear"]
    motion.MotionType = kind
    motion.addProperty("App::PropertyString", "Formula")
    motion.Formula = "time"
    return motion

@contextmanager
def mixed_fixture(modes=(True,), inner_rigid=False, angle=25, release_motion=False):
    documents = []
    with tempfile.TemporaryDirectory() as directory, ExitStack() as guards:
        def document():
            doc = App.newDocument()
            documents.append(doc)
            guards.enter_context(AssemblyApp.suspendAutomaticMotion(doc))
            doc.saveAs(os.path.join(directory, doc.Name + ".FCStd"))
            return doc
        def assembly(doc):
            obj = doc.addObject("Assembly::AssemblyObject", "Assembly")
            obj.Type = "Assembly"
            obj.newObject("Assembly::JointGroup", "Joints")
            return obj
        def occurrence(owner, source, rigid):
            node = owner.newObject("Assembly::AssemblyLink", "Occurrence")
            node.Rigid = rigid  # Supported only before population.
            node.LinkedObject = source
            for _ in range(3):
                owner.Document.recompute()
            assert node.Group
            return node
        try:
            definitions = document()
            for name, radius in (("Base", 3), ("Arm", 2)):
                part = definitions.addObject("Part::Feature", name)
                part.Shape = Part.makeCylinder(radius, 4)
            definitions.recompute()
            definitions.save()
            mechanism = document()
            unit = assembly(mechanism)
            parts = []
            for name in ("Base", "Arm"):
                part = unit.newObject("App::Link", name)
                part.LinkedObject = definitions.getObject(name)
                part.LinkTransform = False
                parts.append(part)
            group = next(obj for obj in unit.Group if obj.TypeId == "Assembly::JointGroup")
            ground = group.newObject("App::FeaturePython", "Ground")
            JointObject.GroundedJoint(ground, parts[0])
            pivot = group.newObject("App::FeaturePython", "Pivot")
            JointObject.Joint(pivot, JointObject.JointTypes.index("Revolute"))
            pivot.Detach1 = True
            pivot.Detach2 = True
            pivot.Reference1 = (parts[0], [""])
            pivot.Reference2 = (parts[1], [""])
            pivot.Placement1 = App.Placement()
            pivot.Placement2 = App.Placement()
            mechanism.recompute()
            assert unit.solveValidated(False) == 0
            mechanism.save()
            middle_doc = document()
            middle = assembly(middle_doc)
            occurrence(middle, unit, inner_rigid)
            middle_doc.save()
            target = document()
            root = assembly(target)
            nodes = [occurrence(root, middle, mode) for mode in modes]
            for index, node in enumerate(nodes):
                moving = mixed_parts(node)["Arm"]
                moving.LinkPlacement = App.Placement(
                    App.Vector(), App.Rotation(App.Vector(0, 0, 1), angle if index == 0 else -angle))
            target.recompute()
            target.save()
            sources = [(doc, mixed_snapshot(doc)) for doc in documents[:-1]]
            if release_motion:
                guards.close()
            yield root, nodes, sources
        finally:
            for doc in reversed(documents):
                App.closeDocument(doc.Name)
)PY";
}  // namespace

TEST_F(AssemblyObjectTest, jointGroupBelongsToImmediateAssemblyOwner)
{
    auto* document = getObject()->getDocument();
    auto* other = document->addObject<Assembly::AssemblyObject>("OtherAssembly");
    auto* nested = other->addObject<App::Part>("NestedOwner");
    auto* nestedJoints = nested->addObject<Assembly::JointGroup>("NestedJoints");
    EXPECT_NE(nestedJoints, nullptr);
    // A descendant's group was created first. Recursive membership must not
    // let the parent borrow it, even when the parent has no group of its own.
    EXPECT_EQ(other->getJointGroup(), nullptr);
    auto* owned = other->addObject<Assembly::JointGroup>("OwnedJoints");
    EXPECT_EQ(other->getJointGroup(), owned);
    EXPECT_NE(other->getJointGroup(), getObject()->getJointGroup());
}

TEST_F(AssemblyObjectTest, validatedMixedRigidAdmissionPreservesFrozenArticulation)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for angle in (25, -40):
    for modes, inner_rigid in (((True,), False), ((False,), True),
                               ((True, False), False), ((False, True), False)):
        with mixed_fixture(modes, inner_rigid, angle) as (root, nodes, sources):
            frozen = [(obj, App.Placement(obj.Placement))
                      for node in nodes if node.Rigid or inner_rigid
                      for obj in mixed_closure(node) if hasattr(obj, "Placement")]
            expected = [mixed_joint(node) for node in nodes if not node.Rigid and not inner_rigid]
            actual = list(root.Joints)
            assert len(actual) == len(set(actual)), actual
            assert set(actual) == set(expected), (modes, inner_rigid, actual, expected)
            assert root.solveValidated(False) == 0, (modes, inner_rigid, angle)
            assert AssemblyApp.isAutomaticMotionSuspended(root.Document)
            for obj, pose in frozen:
                assert obj.Placement.isSame(pose, 1e-9), (obj.Name, modes, inner_rigid, angle)
            for doc, before in sources:
                assert mixed_snapshot(doc) == before, doc.Name
)PY"));
}

TEST_F(AssemblyObjectTest, validatedDormantRecordsRejectWrongOccurrenceBeforeEffects)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for kind in ("joint", "suppressed_joint", "ground", "rigid", "source", "outside"):
    with mixed_fixture((True, True)) as (root, nodes, sources):
        joint = mixed_joint(nodes[0])
        owner_group = next(obj for obj in mixed_closure(nodes[0])
                           if obj.TypeId == "Assembly::JointGroup" and joint in obj.Group)
        first = mixed_parts(nodes[0])["Base"]
        escaped = mixed_parts(nodes[1])["Arm"]
        if kind == "source":
            escaped = sources[1][0].getObject("Arm")
        elif kind == "outside":
            escaped = root.Document.addObject("Part::Box", "Outside")
        if kind in ("joint", "suppressed_joint", "source", "outside"):
            joint.Reference2 = (escaped, [""])
            if kind == "suppressed_joint":
                joint.Suppressed = True
        elif kind == "ground":
            ground = owner_group.newObject("App::FeaturePython", "LocalGround")
            JointObject.GroundedJoint(ground, escaped)
        elif kind == "rigid":
            rigid = owner_group.newObject("App::FeaturePython", "BadRigidGroup")
            rigid.addProperty("App::PropertyBool", "Suppressed")
            rigid.addProperty("App::PropertyLinkList", "ObjectsToRigidGroup")
            rigid.ObjectsToRigidGroup = [first, escaped]
        before = mixed_snapshot(root.Document)
        # Do not recompute/repair the malformed graph before native admission.
        assert root.solveValidated(True) == -8, kind
        assert mixed_snapshot(root.Document) == before, kind
        for doc, snapshot in sources:
            assert mixed_snapshot(doc) == snapshot, (kind, doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, validatedRootJointMayConnectRigidSiblingOccurrences)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
with mixed_fixture((True, True)) as (root, nodes, sources):
    group = next(obj for obj in root.Group if obj.TypeId == "Assembly::JointGroup")
    joint = group.newObject("App::FeaturePython", "RootJoint")
    JointObject.Joint(joint, JointObject.JointTypes.index("Revolute"))
    joint.Detach1 = True
    joint.Detach2 = True
    joint.Reference1 = (nodes[0], [""])
    joint.Reference2 = (nodes[1], [""])
    joint.Placement1 = App.Placement()
    joint.Placement2 = App.Placement()
    assert list(root.Joints) == [joint]
    assert root.solveValidated(False) == 0
    assert list(root.Joints) == [joint]
    for doc, before in sources:
        assert mixed_snapshot(doc) == before
)PY"));
}

TEST_F(AssemblyObjectTest, validatedMotionUsesInheritedJointActivity)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for kind in ("rigid", "suppressed", "flexible", "dormant", "bad_dormant"):
    with mixed_fixture((True, False)) as (root, nodes, sources):
        if kind in ("dormant", "bad_dormant"):
            owner = next(obj for obj in mixed_closure(nodes[0])
                         if obj.TypeId == "Assembly::AssemblyLink" and obj != nodes[0])
            target = mixed_joint(nodes[0] if kind == "dormant" else nodes[1])
        else:
            owner = root
            target = mixed_joint(nodes[0] if kind == "rigid" else nodes[1])
            if kind == "suppressed":
                target.Suppressed = True
        mixed_motion(owner, target)
        before = mixed_snapshot(root.Document)
        expected = 0 if kind in ("flexible", "dormant") else -8
        assert root.solveValidated(False) == expected, kind
        if expected == -8:
            assert mixed_snapshot(root.Document) == before, kind
        for doc, snapshot in sources:
            assert mixed_snapshot(doc) == snapshot, (kind, doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, validatedMixedSolveRetainsDormantGroundReleaseIntent)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
with mixed_fixture((True,)) as (root, nodes, sources):
    doc = root.Document
    parts = mixed_parts(nodes[0])
    first, second = parts["Base"], parts["Arm"]
    joint = mixed_joint(nodes[0])
    group = next(obj for obj in mixed_closure(nodes[0])
                 if obj.TypeId == "Assembly::JointGroup" and joint in obj.Group)
    ground = group.newObject("App::FeaturePython", "LocalGround")
    JointObject.GroundedJoint(ground, first)
    for prop in ("Placement", "LinkPlacement"):
        first.setPropertyStatus(prop, "ReadOnly")
    ground.ObjectToGround = second  # Native callback queues first's deferred release.
    assert root.solveValidated(False) == 0
    for prop in ("Placement", "LinkPlacement"):
        assert "ReadOnly" in first.getPropertyStatus(prop)
        assert "ReadOnly" not in second.getPropertyStatus(prop)
    # Give the exact same old target an eligible owner. A later validated solve
    # must consume the retained intent; a dropped queue would leave it locked.
    eligible = doc.addObject("Assembly::AssemblyObject", "Eligible")
    eligible.newObject("Assembly::JointGroup", "EligibleJoints")
    old_owner = next(obj for obj in mixed_closure(nodes[0])
                     if hasattr(obj, "Group") and first in obj.Group)
    old_owner.removeObject(first)
    eligible.addObject(first)
    assert first not in old_owner.Group
    assert eligible.solveValidated(False) == 0
    for prop in ("Placement", "LinkPlacement"):
        assert "ReadOnly" not in first.getPropertyStatus(prop)
)PY"));
}

TEST_F(AssemblyObjectTest, suspendedMixedModesAndRestorePreserveArticulation)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for angle in (25, -40):
    with mixed_fixture((True, False), angle=angle) as (root, nodes, sources):
        doc = root.Document
        links = [obj for obj in mixed_closure(root) if obj.TypeId == "Assembly::AssemblyLink"]
        for link in links:
            for requested in (link.Rigid, not link.Rigid):
                before = mixed_snapshot(doc)
                mode = link.Rigid
                try:
                    link.Rigid = requested
                except (RuntimeError, ValueError):
                    pass
                else:
                    raise AssertionError(("populated mode accepted", link.Name, requested))
                assert link.Rigid == mode
                assert mixed_snapshot(doc) == before
        # These files remain together, so every restored external XLink is valid.
        expected = {obj.Name: App.Placement(obj.Placement) for obj in mixed_closure(root)
                    if hasattr(obj, "Placement")}
        name = root.Name
        doc.save()
        doc.restore()
        root = doc.getObject(name)
        assert AssemblyApp.isAutomaticMotionSuspended(doc)
        for obj in mixed_closure(root):
            if obj.TypeId == "Assembly::AssemblyLink":
                assert obj.LinkedObject is not None
            if obj.Name in expected:
                assert obj.Placement.isSame(expected[obj.Name], 1e-9), obj.Name
        assert root.solveValidated(False) == 0
        for doc, before in sources:
            assert mixed_snapshot(doc) == before
)PY"));
}

TEST_F(AssemblyObjectTest, validatedDormantMalformedGraphsStillRejectBeforeEffects)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for rigid in (False, True):
    for kind in ("missing_link", "incomplete_joint", "containment_cycle", "incomplete_motion"):
        with mixed_fixture((rigid,)) as (root, nodes, sources):
            cycle = None
            try:
                joint = mixed_joint(nodes[0])
                owner = next(obj for obj in mixed_closure(nodes[0])
                             if obj.TypeId == "Assembly::AssemblyLink" and obj != nodes[0])
                if kind == "missing_link":
                    nodes[0].LinkedObject = None
                elif kind == "incomplete_joint":
                    joint.Reference2 = None
                elif kind == "containment_cycle":
                    cycle = owner.newObject("App::FeaturePython", "RawCycle")
                    cycle.addProperty("App::PropertyLinkList", "Group")
                    cycle.Group = [owner]
                else:
                    motion = mixed_motion(owner, joint)
                    motion.Joint = None
                before = mixed_snapshot(root.Document)
                assert root.solveValidated(True) == -8, (rigid, kind)
                assert mixed_snapshot(root.Document) == before, (rigid, kind)
                for doc, snapshot in sources:
                    assert mixed_snapshot(doc) == snapshot, (rigid, kind, doc.Name)
            finally:
                # Never recompute or close a deliberately cyclic dependency graph.
                if cycle is not None:
                    cycle.Group = []
)PY"));
}

TEST_F(AssemblyObjectTest, mixedNativeJointEnumerationHonorsAncestorsAndDeduplicates)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
mixed_native_context = mixed_fixture((True, False))
mixed_native_root, mixed_native_nodes, _ = mixed_native_context.__enter__()
mixed_joint(mixed_native_nodes[1]).Label = "ExpectedActiveJoint"
)PY"));
    auto* document = App::GetApplication().getActiveDocument();
    auto* root = dynamic_cast<Assembly::AssemblyObject*>(document->getObject("Assembly"));
    EXPECT_NE(root, nullptr);
    if (root) {
        EXPECT_TRUE(root->getJoints(false, false).empty());
        const auto joints = root->getJoints(false, true);
        EXPECT_EQ(joints.size(), 1);
        if (joints.size() == 1) {
            EXPECT_EQ(std::string(joints.front()->Label.getValue()), "ExpectedActiveJoint");
        }
        // An assembly without an own group still has eligible descendant joints.
        auto* owned = root->getJointGroup();
        if (owned) {
            const std::string name = owned->getNameInDocument();
            document->removeObject(name.c_str());
        }
        EXPECT_EQ(root->getJointGroup(), nullptr);
        const auto withoutRootGroup = root->getJoints(false, true);
        EXPECT_EQ(withoutRootGroup.size(), 1);
        if (withoutRootGroup.size() == 1) {
            EXPECT_EQ(std::string(withoutRootGroup.front()->Label.getValue()), "ExpectedActiveJoint");
        }
    }
    ASSERT_NO_THROW(Base::Interpreter().runString("mixed_native_context.__exit__(None, None, None)"));
}

TEST_F(AssemblyObjectTest, suppressedLinkElementMakesJointInactiveWithoutDroppingMetadata)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for rigid in (False, True):
    for driven in (False, True):
        with mixed_fixture((rigid,)) as (root, nodes, sources):
            joint = mixed_joint(nodes[0])
            group = next(obj for obj in mixed_closure(nodes[0])
                         if obj.TypeId == "Assembly::JointGroup" and joint in obj.Group)
            owner = next(obj for obj in mixed_closure(nodes[0])
                         if obj.TypeId == "Assembly::AssemblyLink" and group in obj.Group)
            array = owner.newObject("App::Link", "LocalArray")
            array.LinkedObject = sources[0][0].getObject("Arm")
            array.ElementCount = 2
            # ElementCount creates real LinkElement objects; suppression belongs
            # to the element's native extension, never to the Motion object.
            assert len(array.ElementList) == 2
            element = array.ElementList[1]
            joint.Reference2 = (element, [""])
            element.Suppressed = True
            assert joint not in root.Joints
            if driven:
                mixed_motion(root, joint)
            before = mixed_snapshot(root.Document)
            assert root.solveValidated(False) == (-8 if driven else 0), (rigid, driven)
            assert joint.Reference2[0] == element
            assert element.Suppressed
            if driven:
                assert mixed_snapshot(root.Document) == before
            for doc, snapshot in sources:
                assert mixed_snapshot(doc) == snapshot
)PY"));
}

TEST_F(AssemblyObjectTest, sourceElementSuppressionRetainsSynchronizedLocalJointMetadata)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
doc = App.newDocument()
try:
    with AssemblyApp.suspendAutomaticMotion(doc):
        source = doc.addObject("Assembly::AssemblyObject", "Source")
        source_group = source.newObject("Assembly::JointGroup", "SourceJoints")
        geometry = source.newObject("Part::Box", "Geometry")
        array = source.newObject("App::Link", "SourceArray")
        array.LinkedObject = geometry
        array.ElementCount = 2
        source_joint = source_group.newObject("App::FeaturePython", "SourcePivot")
        JointObject.Joint(source_joint, JointObject.JointTypes.index("Revolute"))
        source_joint.Detach1 = True
        source_joint.Detach2 = True
        source_joint.Reference1 = (array.ElementList[0], [""])
        source_joint.Reference2 = (array.ElementList[1], [""])
        source_joint.Placement1 = App.Placement()
        source_joint.Placement2 = App.Placement()
        root = doc.addObject("Assembly::AssemblyObject", "Root")
        root.newObject("Assembly::JointGroup", "RootJoints")
        instance = root.newObject("Assembly::AssemblyLink", "Instance")
        instance.Rigid = False
        instance.LinkedObject = source
        for _ in range(3):
            doc.recompute()
        local_group = next(obj for obj in instance.Group if obj.TypeId == "Assembly::JointGroup")
        local_array = next(obj for obj in instance.Group
                           if obj.TypeId == "App::Link" and obj.ElementCount == 2)
        assert len(local_group.Group) == 1
        local_joint = local_group.Group[0]
        name = local_joint.Name
        references = (local_joint.Reference1, local_joint.Reference2)
        assert references == ((local_array.ElementList[0], [""]),
                              (local_array.ElementList[1], [""]))
        assert list(root.Joints) == [local_joint]
        for suppressed in (True, False, True):
            array.ElementList[1].Suppressed = suppressed
            instance.touch()
            for _ in range(3):
                doc.recompute()
            # Source-joint synchronization needs structural records even when
            # the ordinary solver excludes their temporarily suppressed endpoint.
            assert doc.getObject(name) == local_joint
            assert list(local_group.Group) == [local_joint]
            assert (local_joint.Reference1, local_joint.Reference2) == references
            assert local_array.ElementList[1].Suppressed == suppressed
            assert list(root.Joints) == ([] if suppressed else [local_joint])
            assert source_joint.Reference2 == (array.ElementList[1], [""])
            assert root.solveValidated(False) == 0
            if suppressed:
                motion = mixed_motion(root, local_joint)
                before = mixed_snapshot(doc)
                assert root.solveValidated(False) == -8
                assert mixed_snapshot(doc) == before
                doc.removeObject(motion.Name)
finally:
    App.closeDocument(doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, ordinarySolveKeepsRootCoordinateSystemJointActive)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import FreeCAD as App
import AssemblyApp
import JointObject
import Part

doc = App.newDocument()
try:
    with AssemblyApp.suspendAutomaticMotion(doc):
        root = doc.addObject("Assembly::AssemblyObject", "Assembly")
        group = root.newObject("Assembly::JointGroup", "Joints")
        datum = root.newObject("App::LocalCoordinateSystem", "ReferenceFrame")
        datum.addExtension("Part::AttachExtensionPython")
        assert datum.MapMode == "Deactivated"
        moving = root.newObject("Part::Box", "Moving")
        moving.Placement.Base = App.Vector(13, 17, 19)
        joint = group.newObject("App::FeaturePython", "DatumJoint")
        JointObject.Joint(joint, JointObject.JointTypes.index("Fixed"))
        joint.Detach1 = True
        joint.Detach2 = True
        joint.Reference1 = (datum, [""])
        joint.Reference2 = (moving, [""])
        joint.Placement1 = App.Placement()
        joint.Placement2 = App.Placement()
        doc.recompute()
    assert not AssemblyApp.isAutomaticMotionSuspended(doc)
    assert list(root.Joints) == [joint]
    fixed_pose = App.Placement(datum.Placement)
    initial = App.Placement(moving.Placement)
    assert root.solve(False) == 0
    assert datum.Placement.isSame(fixed_pose, 1e-9)
    assert not moving.Placement.isSame(initial, 1e-9)
    assert moving.Placement.isSame(fixed_pose, 1e-9)
finally:
    App.closeDocument(doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, ordinaryPopulatedModeConversionRemapsBothParentJointEndpoints)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import FreeCAD as App
import AssemblyApp
import JointObject
import Part

doc = App.newDocument()
try:
    with AssemblyApp.suspendAutomaticMotion(doc):
        source = doc.addObject("Assembly::AssemblyObject", "Source")
        source.newObject("Assembly::JointGroup", "SourceJoints")
        source.newObject("Part::Box", "First")
        source.newObject("Part::Box", "Second")
        root = doc.addObject("Assembly::AssemblyObject", "Root")
        group = root.newObject("Assembly::JointGroup", "RootJoints")
        anchor = root.newObject("Part::Box", "Anchor")
        instance = root.newObject("Assembly::AssemblyLink", "Instance")
        instance.Rigid = False
        instance.LinkedObject = source
        for _ in range(3):
            doc.recompute()
        children = [obj for obj in instance.Group if obj.TypeId == "App::Link"]
        assert len(children) == 2
        joints = []
        for refs in (((children[0], [""]), (anchor, [""])),
                     ((anchor, [""]), (children[1], [""]))):
            joint = group.newObject("App::FeaturePython", "ParentJoint")
            JointObject.Joint(joint, JointObject.JointTypes.index("Fixed"))
            joint.Detach1 = True
            joint.Detach2 = True
            joint.Reference1, joint.Reference2 = refs
            joint.Placement1 = App.Placement()
            joint.Placement2 = App.Placement()
            joints.append(joint)
        doc.recompute()
    assert not AssemblyApp.isAutomaticMotionSuspended(doc)
    for _ in range(2):
        instance.Rigid = True
        assert joints[0].Reference1 == (instance, [children[0].Name + "."])
        assert joints[0].Reference2 == (anchor, [""])
        assert joints[1].Reference1 == (anchor, [""])
        assert joints[1].Reference2 == (instance, [children[1].Name + "."])
        instance.Rigid = False
        assert joints[0].Reference1 == (children[0], [""])
        assert joints[0].Reference2 == (anchor, [""])
        assert joints[1].Reference1 == (anchor, [""])
        assert joints[1].Reference2 == (children[1], [""])
        assert set(root.Joints) == set(joints)
finally:
    App.closeDocument(doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, validatedRootSolveRetainsFlexibleDescendantGroundIntents)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for deleted in (False, True):
    with mixed_fixture((False,)) as (root, nodes, sources):
        doc = root.Document
        first, second = (mixed_parts(nodes[0])[name] for name in ("Base", "Arm"))
        joint = mixed_joint(nodes[0])
        local_group = next(obj for obj in mixed_closure(nodes[0])
                           if obj.TypeId == "Assembly::JointGroup" and joint in obj.Group)
        ground = local_group.newObject("App::FeaturePython", "LocalGround")
        JointObject.GroundedJoint(ground, first)
        for prop in ("Placement", "LinkPlacement"):
            first.setPropertyStatus(prop, "ReadOnly")
        if deleted:
            doc.removeObject(ground.Name)
        else:
            ground.ObjectToGround = second
        assert root.solveValidated(False) == 0
        for prop in ("Placement", "LinkPlacement"):
            assert "ReadOnly" in first.getPropertyStatus(prop), deleted
            assert "ReadOnly" not in second.getPropertyStatus(prop), deleted
        # The descendant's pending operation belongs to that owner's solve.
        # Transfer its exact target identities and ground to an eligible root,
        # rather than recreating an intent after the first solve.
        eligible = doc.addObject("Assembly::AssemblyObject", "EligibleGroundOwner")
        eligible_group = eligible.newObject("Assembly::JointGroup", "EligibleGrounds")
        old_owner = next(obj for obj in mixed_closure(nodes[0])
                         if hasattr(obj, "Group") and first in obj.Group)
        old_owner.removeObject(first)
        eligible.addObject(first)
        assert first not in old_owner.Group
        if not deleted:
            old_owner.removeObject(second)
            eligible.addObject(second)
            assert second not in old_owner.Group
            local_group.removeObject(ground)
            eligible_group.addObject(ground)
        assert eligible.solveValidated(False) == 0
        for prop in ("Placement", "LinkPlacement"):
            assert "ReadOnly" not in first.getPropertyStatus(prop), deleted
            if not deleted:
                assert "ReadOnly" in second.getPropertyStatus(prop)
        for source_doc, before in sources:
            assert mixed_snapshot(source_doc) == before
)PY"));
}

TEST_F(AssemblyObjectTest, validatedMotionRejectsErroredNativeJointBeforeEffects)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
mixed_error_doc = App.newDocument()
mixed_error_scope = AssemblyApp.suspendAutomaticMotion(mixed_error_doc)
mixed_error_scope.__enter__()
mixed_error_root = mixed_error_doc.addObject("Assembly::AssemblyObject", "Assembly")
mixed_error_group = mixed_error_root.newObject("Assembly::JointGroup", "Joints")
mixed_error_first = mixed_error_root.newObject("Part::Box", "First")
mixed_error_second = mixed_error_root.newObject("Part::Box", "Second")
mixed_error_joint = mixed_error_group.newObject("App::FeaturePython", "ErroredJoint")
JointObject.Joint(mixed_error_joint, JointObject.JointTypes.index("Revolute"))
mixed_error_joint.Detach1 = True
mixed_error_joint.Detach2 = True
mixed_error_joint.Reference1 = (mixed_error_first, [""])
mixed_error_joint.Reference2 = (mixed_error_second, [""])
mixed_error_joint.Placement1 = App.Placement()
mixed_error_joint.Placement2 = App.Placement()
mixed_error_doc.recompute()
mixed_motion(mixed_error_root, mixed_error_joint)
assert mixed_error_root.solveValidated(False) == 0
)PY"));
    auto* document = App::GetApplication().getActiveDocument();
    auto* joint = document->getObject("ErroredJoint");
    EXPECT_NE(joint, nullptr);
    if (joint) {
        // Error is a native object status, with no public Python setter. Keep
        // the complete real Joint proxy/schema to isolate eligibility alone.
        joint->setStatus(App::ObjectStatus::Error, true);
        EXPECT_TRUE(joint->isError());
        EXPECT_NO_THROW(Base::Interpreter().runString(R"PY(
assert "Invalid" in mixed_error_joint.State
assert mixed_error_joint not in mixed_error_root.Joints
mixed_error_before = mixed_snapshot(mixed_error_doc)
assert mixed_error_root.solveValidated(False) == -8
assert mixed_snapshot(mixed_error_doc) == mixed_error_before
)PY"));
    }
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
mixed_error_scope.__exit__(None, None, None)
App.closeDocument(mixed_error_doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, overlappingGenericGroupsKeepImmediateOwnerOrderIndependent)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import FreeCAD as App
import AssemblyApp
import JointObject
import Part

for generic_first in (False, True):
    doc = App.newDocument()
    try:
        with AssemblyApp.suspendAutomaticMotion(doc):
            root = doc.addObject("Assembly::AssemblyObject", "Assembly")
            group = root.newObject("Assembly::JointGroup", "Joints")
            generic = root.newObject("App::DocumentObjectGroup", "Metadata")
            first = root.newObject("Part::Box", "First")
            second = root.newObject("Part::Box", "Second")
            joint = group.newObject("App::FeaturePython", "Joint")
            JointObject.Joint(joint, JointObject.JointTypes.index("Revolute"))
            joint.Detach1 = True
            joint.Detach2 = True
            joint.Reference1 = (first, [""])
            joint.Reference2 = (second, [""])
            joint.Placement1 = App.Placement()
            joint.Placement2 = App.Placement()
            # Both paths reach the same immediate-owner group. Explicit Group
            # membership, rather than the first traversal path, defines authority.
            generic.Group = [group]
            root.Group = ([generic, group] if generic_first else [group, generic]) + [first, second]
            doc.recompute()
            assert list(root.Joints) == [joint]
            assert root.solveValidated(False) == 0, generic_first
            assert list(group.Group) == [joint]
    finally:
        App.closeDocument(doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, deferredGroundReleaseDistinguishesUnknownAndExpiredOwners)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import FreeCAD as App
import AssemblyApp
import JointObject
import Part

for known_owner in (False, True):
    doc = App.newDocument()
    try:
        with AssemblyApp.suspendAutomaticMotion(doc):
            if known_owner:
                old_owner = doc.addObject("Assembly::AssemblyObject", "OldOwner")
                old_group = old_owner.newObject("Assembly::JointGroup", "OldJoints")
                target = old_owner.newObject("Part::Box", "Target")
                ground = old_group.newObject("App::FeaturePython", "OldGround")
                JointObject.GroundedJoint(ground, target)
            else:
                target = doc.addObject("Part::Box", "Target")
            target.setPropertyStatus("Placement", "ReadOnly")
            if known_owner:
                # The real callback captures provenance before record deletion.
                doc.removeObject(ground.Name)
                old_owner.removeObject(target)
                doc.removeObject(old_owner.Name)
            else:
                # Legacy callers can queue an unowned target. Later containment
                # cannot retroactively grant that unknown intent authority.
                AssemblyApp._deferGroundedRelease(doc, target)
            eligible = doc.addObject("Assembly::AssemblyObject", "NewOwner")
            eligible.newObject("Assembly::JointGroup", "NewJoints")
            eligible.addObject(target)
            for _ in range(2):
                assert eligible.solveValidated(False) == 0
                assert ("ReadOnly" in target.getPropertyStatus("Placement")) == (not known_owner)
    finally:
        App.closeDocument(doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, rootGroundRetargetRetainsAuthorityOverFlexibleDescendants)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
with mixed_fixture((False,)) as (root, nodes, sources):
    first, second = (mixed_parts(nodes[0])[name] for name in ("Base", "Arm"))
    group = next(obj for obj in root.Group if obj.TypeId == "Assembly::JointGroup")
    ground = group.newObject("App::FeaturePython", "RootOwnedGround")
    JointObject.GroundedJoint(ground, first)
    for prop in ("Placement", "LinkPlacement"):
        first.setPropertyStatus(prop, "ReadOnly")
    ground.ObjectToGround = second
    assert root.solveValidated(False) == 0
    for prop in ("Placement", "LinkPlacement"):
        assert "ReadOnly" not in first.getPropertyStatus(prop)
        assert "ReadOnly" in second.getPropertyStatus(prop)
    assert ground.ObjectToGround == second
    for source_doc, before in sources:
        assert mixed_snapshot(source_doc) == before
)PY"));
}

namespace
{
constexpr const char* posedWrapperFixture = R"PY(
import hashlib
from pathlib import Path

def coordinate_inner(node):
    return next(obj for obj in node.Group if obj.TypeId == "Assembly::AssemblyLink")

def coordinate_world(root, node, part):
    # Independent oracle: explicit known spatial ancestry, with no reference or
    # solver placement helpers. Source definitions are not spatial parents.
    return root.Placement * node.Placement * coordinate_inner(node).Placement * part.Placement

def coordinate_sources(sources):
    return [(doc, mixed_snapshot(doc), hashlib.sha256(Path(doc.FileName).read_bytes()).hexdigest())
            for doc, _ in sources]

def coordinate_check_sources(proof):
    for doc, state, digest in proof:
        assert mixed_snapshot(doc) == state, doc.Name
        assert hashlib.sha256(Path(doc.FileName).read_bytes()).hexdigest() == digest, doc.Name

def coordinate_set_frames(root, nodes, variant):
    nodes[1].Placement = App.Placement(App.Vector(40, 0, 0), App.Rotation())
    if variant >= 1:
        nodes[1].Placement = App.Placement(App.Vector(40, -13, 9), App.Rotation(31, -17, 23))
    if variant >= 2:
        coordinate_inner(nodes[0]).Placement = App.Placement(
            App.Vector(3, 11, -7), App.Rotation(-19, 13, 7))
        coordinate_inner(nodes[1]).Placement = App.Placement(
            App.Vector(-5, 2, 17), App.Rotation(11, 29, -13))
    if variant >= 3:
        root.Placement = App.Placement(App.Vector(101, -53, 27), App.Rotation(17, 31, -23))

def coordinate_joint(root, nodes):
    first, second = [mixed_parts(node)["Base"] for node in nodes]
    group = next(obj for obj in root.Group if obj.TypeId == "Assembly::JointGroup")
    ground = group.newObject("App::FeaturePython", "CoordinateGround")
    JointObject.GroundedJoint(ground, first)
    joint = group.newObject("App::FeaturePython", "CoordinateFixed")
    JointObject.Joint(joint, JointObject.JointTypes.index("Fixed"))
    joint.Detach1 = True
    joint.Detach2 = True
    joint.Reference1 = (first, [""])
    joint.Reference2 = (second, [""])
    joint.Placement1 = coordinate_world(root, nodes[0], first).inverse() * coordinate_world(root, nodes[1], second)
    joint.Placement2 = App.Placement()
    return joint, first, second

def coordinate_assert_fixed(root, nodes, joint, first, second):
    left = coordinate_world(root, nodes[0], first) * joint.Placement1
    right = coordinate_world(root, nodes[1], second) * joint.Placement2
    delta = left.inverse() * right
    assert delta.Base.Length < 1e-6, (left, right, delta)
    assert abs(delta.Rotation.Angle) < 1.7453292519943295e-8, delta

def coordinate_frames(root):
    return [(obj, App.Placement(obj.Placement)) for obj in mixed_closure(root)
            if obj == root or obj.TypeId == "Assembly::AssemblyLink"]

def coordinate_check_frames(frames):
    for obj, pose in frames:
        assert obj.Placement.isSame(pose, 1e-12), obj.Name
)PY";
}  // namespace

TEST_F(AssemblyObjectTest, posedFlexibleWrappersSolveInOneCoordinateFrame)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(posedWrapperFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for variant in range(4):
    for perturbed in (False, True):
        with mixed_fixture((False, False), angle=0) as (root, nodes, sources):
            coordinate_set_frames(root, nodes, variant)
            joint, first, second = coordinate_joint(root, nodes)
            coordinate_assert_fixed(root, nodes, joint, first, second)
            frames = coordinate_frames(root)
            proof = coordinate_sources(sources)
            anchored = App.Placement(coordinate_world(root, nodes[0], first))
            if perturbed:
                second.Placement = second.Placement * App.Placement(
                    App.Vector(7, -3, 5), App.Rotation(9, -7, 11))
            assert root.solveValidated(False) == 0, (variant, perturbed)
            coordinate_assert_fixed(root, nodes, joint, first, second)
            assert coordinate_world(root, nodes[0], first).isSame(anchored, 1e-7)
            coordinate_check_frames(frames)
            coordinate_check_sources(proof)
            assert AssemblyApp.isAutomaticMotionSuspended(root.Document)
)PY"));
}

TEST_F(AssemblyObjectTest, posedWrapperRefusalPreservesLocalAndDocumentPoses)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(posedWrapperFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
with mixed_fixture((False, False), angle=0) as (root, nodes, sources):
    coordinate_set_frames(root, nodes, 3)
    joint, first, second = coordinate_joint(root, nodes)
    joint.Reference2 = (sources[1][0].getObject("Base"), [""])
    before = mixed_snapshot(root.Document)
    worlds = [coordinate_world(root, node, mixed_parts(node)["Base"]) for node in nodes]
    proof = coordinate_sources(sources)
    assert root.solveValidated(True) == -8
    assert mixed_snapshot(root.Document) == before
    for node, pose in zip(nodes, worlds):
        assert coordinate_world(root, node, mixed_parts(node)["Base"]).isSame(pose, 1e-12)
    coordinate_check_sources(proof)
)PY"));
}

TEST_F(AssemblyObjectTest, posedWrapperCurrentJointValueIncludesBothContainingFrames)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(posedWrapperFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
coordinate_context = mixed_fixture((False, False), angle=0)
root, nodes, sources = coordinate_context.__enter__()
root.Placement = App.Placement(App.Vector(101, -53, 27), App.Rotation(17, 31, -23))
nodes[0].Placement = App.Placement(App.Vector(3, 4, 5), App.Rotation(App.Vector(0, 0, 1), 10))
nodes[1].Placement = App.Placement(App.Vector(3, 4, 5), App.Rotation(App.Vector(0, 0, 1), 55))
joint, first, second = coordinate_joint(root, nodes)
joint.Placement1 = App.Placement()
joint.Placement2 = App.Placement()
)PY"));
    auto* document = App::GetApplication().getActiveDocument();
    auto* joint = document->getObject("CoordinateFixed");
    ASSERT_NE(joint, nullptr);
    EXPECT_NEAR(Assembly::getJointCurrentValue(joint, true), std::acos(-1.0) / 4.0, 1e-9);
    ASSERT_NO_THROW(Base::Interpreter().runString("coordinate_context.__exit__(None, None, None)"));
}

TEST_F(AssemblyObjectTest, posedWrapperFixedBundlesDragAndRefreshContainingFrames)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(posedWrapperFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
coordinate_context = mixed_fixture((False, False), angle=0, release_motion=True)
root, nodes, sources = coordinate_context.__enter__()
with AssemblyApp.suspendAutomaticMotion(root.Document):
    coordinate_set_frames(root, nodes, 3)
    joint, first, second = coordinate_joint(root, nodes)
    arm = mixed_parts(nodes[0])["Arm"]
    pivot = mixed_joint(nodes[0])
    pivot.AngleMin = -60
    pivot.AngleMax = 60
    pivot.EnableAngleMin = True
    pivot.EnableAngleMax = True
    arm.Label = "CoordinateDragArm"
    tool = root.newObject("Part::Box", "CoordinateTool")
    tool.Placement = root.Placement.inverse() * coordinate_world(root, nodes[0], arm) * App.Placement(App.Vector(5, 0, 0), App.Rotation())
    group = next(obj for obj in root.Group if obj.TypeId == "Assembly::JointGroup")
    attachment = group.newObject("App::FeaturePython", "ToolAttachment")
    JointObject.Joint(attachment, JointObject.JointTypes.index("Fixed"))
    attachment.Detach1 = True
    attachment.Detach2 = True
    attachment.Reference1 = (arm, [""])
    attachment.Reference2 = (tool, [""])
    attachment.Placement1 = App.Placement(App.Vector(5, 0, 0), App.Rotation())
    attachment.Placement2 = App.Placement()
    assert root.solveValidated(False) == 0
frames = coordinate_frames(root)
proof = coordinate_sources(sources)
tool_before = root.Placement * tool.Placement
)PY"));
    auto* document = App::GetApplication().getActiveDocument();
    auto* root = dynamic_cast<Assembly::AssemblyObject*>(document->getObject("Assembly"));
    auto* tool = document->getObject("CoordinateTool");
    App::DocumentObject* arm = nullptr;
    for (auto* object : document->getObjects()) {
        if (std::string(object->Label.getValue()) == "CoordinateDragArm") {
            arm = object;
        }
    }
    ASSERT_NE(root, nullptr);
    ASSERT_NE(arm, nullptr);
    ASSERT_NE(tool, nullptr);
    root->preDrag({arm});
    EXPECT_EQ(root->getMbDData(arm).part, root->getMbDData(tool).part);
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
arm.Placement = arm.Placement * App.Placement(App.Vector(), App.Rotation(App.Vector(0, 0, 1), 25))
)PY"));
    root->doDragStep();
    for (const int requested : {75, -75}) {
        const std::string command = "arm.Placement = mixed_parts(nodes[0])['Base'].Placement * "
                                    "App.Placement(App.Vector(), App.Rotation(App.Vector(0, 0, 1), "
            + std::to_string(requested) + "))";
        ASSERT_NO_THROW(Base::Interpreter().runString(command.c_str()));
        root->doDragStep();
        ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import math
relative_arm = coordinate_world(root, nodes[0], first).inverse() * coordinate_world(root, nodes[0], arm)
x_axis = relative_arm.Rotation.multVec(App.Vector(1, 0, 0))
angle = math.degrees(math.atan2(x_axis.y, x_axis.x))
print("SPEC008_DRAG_LIMIT", angle, flush=True)
assert -60.000001 <= angle <= 60.000001, angle
)PY"));
    }
    root->postDrag();
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
coordinate_assert_fixed(root, nodes, joint, first, second)
left = coordinate_world(root, nodes[0], arm) * attachment.Placement1
right = root.Placement * tool.Placement
assert left.isSame(right, 1e-7), (left, right)
assert not right.isSame(tool_before, 1e-7), "drag did not move the fixed attachment"
coordinate_check_frames(frames)
coordinate_check_sources(proof)
# The solver graph is prepared. Moving a wrapper changes the conversion frame;
# writeback must use its current frame rather than a stale cached transform.
prepared = [(part, coordinate_world(root, node, part)) for node in nodes
            for part in mixed_parts(node).values()]
nodes[0].Placement = nodes[0].Placement * App.Placement(App.Vector(2, -3, 7), App.Rotation(11, 5, -9))
changed_wrapper = App.Placement(nodes[0].Placement)
)PY"));
    root->setNewPlacements();
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for part, pose in prepared:
    node = next(node for node in nodes if part in mixed_closure(node))
    assert coordinate_world(root, node, part).isSame(pose, 1e-7), part.Name
assert nodes[0].Placement.isSame(changed_wrapper, 1e-12)
coordinate_check_sources(proof)
coordinate_context.__exit__(None, None, None)
)PY"));
}

TEST_F(AssemblyObjectTest, posedRigidClusterCacheUsesCommonFrameAcrossParents)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(posedWrapperFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
with mixed_fixture((False, False), angle=0) as (root, nodes, sources):
    coordinate_set_frames(root, nodes, 3)
    first, second = [mixed_parts(node)["Base"] for node in nodes]
    group = next(obj for obj in root.Group if obj.TypeId == "Assembly::JointGroup")
    ground = group.newObject("App::FeaturePython", "Ground")
    JointObject.GroundedJoint(ground, first)
    rigid = group.newObject("App::FeaturePython", "Rigid")
    rigid.addProperty("App::PropertyBool", "Suppressed")
    rigid.addProperty("App::PropertyLinkList", "ObjectsToRigidGroup")
    rigid.ObjectsToRigidGroup = [first, second]
    proof = coordinate_sources(sources)
    frames = coordinate_frames(root)
    relative = coordinate_world(root, nodes[0], first).inverse() * coordinate_world(root, nodes[1], second)
    assert root.solveValidated(False) == 0
    first.Placement = first.Placement * App.Placement(App.Vector(7, -3, 5), App.Rotation(9, -7, 11))
    assert root.solveValidated(False) == 0
    actual = coordinate_world(root, nodes[0], first).inverse() * coordinate_world(root, nodes[1], second)
    assert actual.isSame(relative, 1e-7), (relative, actual)
    coordinate_check_frames(frames)
    coordinate_check_sources(proof)
)PY"));
}

TEST_F(AssemblyObjectTest, posedWrapperRestoreOrderKeepsDocumentConnectorResiduals)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(posedWrapperFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for target_first in (False, True):
    with mixed_fixture((False, False), angle=0) as (root, nodes, sources):
        coordinate_set_frames(root, nodes, 3)
        joint, first, second = coordinate_joint(root, nodes)
        target = root.Document
        names = (root.Name, [node.Name for node in nodes], joint.Name, first.Name, second.Name)
        target.save()
        documents = [doc for doc, _ in sources]
        if target_first:
            target.restore()
        for doc in documents:
            doc.restore()
        if not target_first:
            target.restore()
        root = target.getObject(names[0])
        nodes = [target.getObject(name) for name in names[1]]
        joint, first, second = [target.getObject(name) for name in names[2:]]
        assert all(obj.LinkedObject is not None for obj in mixed_closure(root)
                   if obj.TypeId == "Assembly::AssemblyLink")
        coordinate_assert_fixed(root, nodes, joint, first, second)
        frames = coordinate_frames(root)
        proof = coordinate_sources(sources)
        assert root.solveValidated(False) == 0, target_first
        coordinate_assert_fixed(root, nodes, joint, first, second)
        coordinate_check_frames(frames)
        coordinate_check_sources(proof)
        assert AssemblyApp.isAutomaticMotionSuspended(target)
)PY"));
}

TEST_F(AssemblyObjectTest, posedRootDatumsKeepPlainAndArrayComponentCoordinates)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import FreeCAD as App
import AssemblyApp
import JointObject
import Part

for use_array in (False, True):
    doc = App.newDocument()
    try:
        with AssemblyApp.suspendAutomaticMotion(doc):
            root = doc.addObject("Assembly::AssemblyObject", "Assembly")
            root.Placement = App.Placement(App.Vector(101, -53, 27), App.Rotation(17, 31, -23))
            root_pose = App.Placement(root.Placement)
            group = root.newObject("Assembly::JointGroup", "Joints")
            datum = root.newObject("App::LocalCoordinateSystem", "Frame")
            datum.addExtension("Part::AttachExtensionPython")
            datum.Placement = App.Placement(App.Vector(2, 5, -7), App.Rotation(11, 7, -19))
            if use_array:
                geometry = doc.addObject("Part::Box", "ArraySource")
                array = root.newObject("App::Link", "Array")
                array.LinkedObject = geometry
                array.ElementCount = 2
                array.Placement = App.Placement(App.Vector(23, 29, 31), App.Rotation(7, 13, -17))
                moving = array.ElementList[1]
                expected_local = array.Placement * moving.Placement
            else:
                moving = root.newObject("Part::Box", "Moving")
                moving.Placement = App.Placement(App.Vector(23, 29, 31), App.Rotation(7, 13, -17))
                expected_local = App.Placement(moving.Placement)
            joint = group.newObject("App::FeaturePython", "Fixed")
            JointObject.Joint(joint, JointObject.JointTypes.index("Fixed"))
            joint.Detach1 = True
            joint.Detach2 = True
            joint.Reference1 = (datum, [""])
            joint.Reference2 = (moving, [""])
            joint.Placement1 = datum.Placement.inverse() * expected_local
            joint.Placement2 = App.Placement()
        # Root datum endpoints retain their established ordinary-solver policy;
        # explicit validated admission intentionally does not admit this schema.
        assert not AssemblyApp.isAutomaticMotionSuspended(doc)
        assert root.solve(False) == 0
        actual_local = array.Placement * moving.Placement if use_array else moving.Placement
        assert (root.Placement * actual_local).isSame(root_pose * expected_local, 1e-7)
        assert root.Placement.isSame(root_pose, 1e-12)
    finally:
        App.closeDocument(doc.Name)
)PY"));
}

TEST_F(AssemblyObjectTest, posedRigidSiblingAndFlexibleSiblingKeepSeparateFrames)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(posedWrapperFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
with mixed_fixture((True, False), angle=0) as (root, nodes, sources):
    coordinate_set_frames(root, nodes, 3)
    group = next(obj for obj in root.Group if obj.TypeId == "Assembly::JointGroup")
    second = mixed_parts(nodes[1])["Base"]
    ground = group.newObject("App::FeaturePython", "RigidGround")
    JointObject.GroundedJoint(ground, nodes[0])
    joint = group.newObject("App::FeaturePython", "SiblingJoint")
    JointObject.Joint(joint, JointObject.JointTypes.index("Fixed"))
    joint.Detach1 = True
    joint.Detach2 = True
    joint.Reference1 = (nodes[0], [""])
    joint.Reference2 = (second, [""])
    joint.Placement1 = (root.Placement * nodes[0].Placement).inverse() * coordinate_world(root, nodes[1], second)
    joint.Placement2 = App.Placement()
    frozen = [(obj, App.Placement(obj.Placement)) for obj in mixed_closure(nodes[0]) if hasattr(obj, "Placement")]
    proof = coordinate_sources(sources)
    second.Placement = second.Placement * App.Placement(App.Vector(7, -3, 5), App.Rotation(9, -7, 11))
    assert root.solveValidated(False) == 0
    assert (root.Placement * nodes[0].Placement * joint.Placement1).isSame(
        coordinate_world(root, nodes[1], second), 1e-7)
    for obj, pose in frozen:
        assert obj.Placement.isSame(pose, 1e-12)
    coordinate_check_sources(proof)
)PY"));
}

TEST_F(AssemblyObjectTest, posedWrapperMetadataAliasOrderDoesNotCreateSpatialFrames)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(posedWrapperFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
for alias_first in (False, True):
    with mixed_fixture((False, False), angle=0) as (root, nodes, sources):
        coordinate_set_frames(root, nodes, 3)
        joint, first, second = coordinate_joint(root, nodes)
        alias = root.newObject("App::DocumentObjectGroup", "MetadataAlias")
        alias.addProperty("App::PropertyPlacement", "Placement")
        alias.Placement.Base = App.Vector(1000, 2000, 3000)
        alias.Group = [nodes[1]]
        members = [obj for obj in root.Group if obj != alias]
        root.Group = ([alias] + members) if alias_first else (members + [alias])
        proof = coordinate_sources(sources)
        frames = coordinate_frames(root)
        assert root.solveValidated(False) == 0, alias_first
        coordinate_assert_fixed(root, nodes, joint, first, second)
        coordinate_check_frames(frames)
        coordinate_check_sources(proof)
)PY"));
}

TEST_F(AssemblyObjectTest, ambiguousLinkElementOwnerOutsideRootRejectsBeforeEffects)
{
    ASSERT_NO_THROW(Base::Interpreter().runString(mixedAssemblyFixture));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
coordinate_ambiguous_doc = App.newDocument()
coordinate_ambiguous_scope = AssemblyApp.suspendAutomaticMotion(coordinate_ambiguous_doc)
coordinate_ambiguous_scope.__enter__()
root = coordinate_ambiguous_doc.addObject("Assembly::AssemblyObject", "Assembly")
root.newObject("Assembly::JointGroup", "Joints")
geometry = coordinate_ambiguous_doc.addObject("Part::Box", "Geometry")
array = root.newObject("App::Link", "Array")
array.LinkedObject = geometry
array.ElementCount = 2
outside = coordinate_ambiguous_doc.addObject("App::Link", "OutsideArray")
outside.LinkedObject = geometry
outside.ElementCount = 2
assert root.solveValidated(False) == 0
)PY"));
    auto* document = App::GetApplication().getActiveDocument();
    auto* array = document->getObject("Array");
    auto* outside = document->getObject("OutsideArray");
    ASSERT_NE(array, nullptr);
    ASSERT_NE(outside, nullptr);
    auto* elements = array->getPropertyByName<App::PropertyLinkList>("ElementList");
    auto* outsideElements = outside->getPropertyByName<App::PropertyLinkList>("ElementList");
    ASSERT_NE(elements, nullptr);
    ASSERT_NE(outsideElements, nullptr);
    // Construct the raw ambiguous metadata natively: Python protects ElementList
    // against writes. Both real link groups now explicitly claim the same elements.
    outsideElements->setValues(elements->getValues());
    EXPECT_NO_THROW(Base::Interpreter().runString(R"PY(
assert array.ElementList[1] in outside.ElementList
before = mixed_snapshot(coordinate_ambiguous_doc)
assert root.solveValidated(False) == -8
assert mixed_snapshot(coordinate_ambiguous_doc) == before
)PY"));
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
coordinate_ambiguous_scope.__exit__(None, None, None)
App.closeDocument(coordinate_ambiguous_doc.Name)
)PY"));
}
