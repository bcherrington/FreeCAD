// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
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
