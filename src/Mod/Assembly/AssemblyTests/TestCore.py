# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# ***************************************************************************/

import FreeCAD as App
import Part
import AssemblyApp
import gc
import hashlib
import math
import os
import tempfile
import unittest
from contextlib import ExitStack

import UtilsAssembly
import JointObject


def _msg(text, end="\n"):
    """Write messages to the console including the line ending."""
    App.Console.PrintMessage(text + end)


class AssemblyTestBase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """setUpClass()...
        This method is called upon instantiation of this test class.  Add code and objects here
        that are needed for the duration of the test() methods in this class.  In other words,
        set up the 'global' test environment here; use the `setUp()` method to set up a 'local'
        test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """
        pass

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        pass

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and objects here
        that are needed for multiple `test()` methods.
        """
        doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != doc_name:
                App.newDocument(doc_name)
        else:
            App.newDocument(doc_name)
        App.setActiveDocument(doc_name)
        self.doc = App.ActiveDocument

        self.assembly = App.ActiveDocument.addObject("Assembly::AssemblyObject", "Assembly")
        if self.assembly:
            self.jointgroup = self.assembly.newObject("Assembly::JointGroup", "Joints")

        _msg("  Temporary document '{}'".format(self.doc.Name))

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        App.closeDocument(self.doc.Name)


class TestCore(AssemblyTestBase):
    def _suspended_joint_fixture(self, joint_type="Fixed", contained=True):
        first = self.assembly.newObject("Part::Box", "First")
        second = self.assembly.newObject("Part::Box", "Second")
        second.Placement = App.Placement(App.Vector(50, 20, 10), App.Rotation())
        self.doc.recompute()
        owner = self.jointgroup if contained else self.doc
        create = owner.newObject if contained else owner.addObject
        joint = create("App::FeaturePython", "Joint")
        JointObject.Joint(joint, JointObject.JointTypes.index(joint_type))
        if App.GuiUp:
            JointObject.ViewProviderJoint(joint.ViewObject)
        joint.Proxy.setJointConnectors(joint, [(first, ["Face1"]), (second, ["Face1"])])
        return first, second, joint

    def test_motion_suspension_nesting_exception_and_gc(self):
        self.assertFalse(AssemblyApp.isAutomaticMotionSuspended(self.doc))
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            self.assertTrue(AssemblyApp.isAutomaticMotionSuspended(self.doc))
            with self.assertRaisesRegex(RuntimeError, "injected"):
                with AssemblyApp.suspendAutomaticMotion(self.doc):
                    raise RuntimeError("injected")
            self.assertTrue(AssemblyApp.isAutomaticMotionSuspended(self.doc))
            self.assertEqual(self.assembly.solve(), -7)
        self.assertFalse(AssemblyApp.isAutomaticMotionSuspended(self.doc))

        scope = AssemblyApp.suspendAutomaticMotion(self.doc)
        scope.__enter__()
        self.assertTrue(AssemblyApp.isAutomaticMotionSuspended(self.doc))
        del scope
        gc.collect()
        self.assertFalse(AssemblyApp.isAutomaticMotionSuspended(self.doc))

    def test_motion_suspension_context_is_single_use(self):
        context = AssemblyApp.suspendAutomaticMotion(self.doc)
        with context:
            with self.assertRaises(RuntimeError):
                context.__enter__()
            self.assertTrue(AssemblyApp.isAutomaticMotionSuspended(self.doc))
        with self.assertRaises(RuntimeError):
            context.__enter__()
        self.assertFalse(AssemblyApp.isAutomaticMotionSuspended(self.doc))

    def test_motion_suspension_generated_array_remapping(self):
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            self._check_generated_array_joint("Face1")

    def test_motion_suspension_generated_whole_element_remapping(self):
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            self._check_generated_array_joint("")

    def test_motion_suspension_preserves_existing_rigid_instance_pose(self):
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            source = self.assembly.newObject("Part::Box", "SourceBox")
            parent = self.doc.addObject("Assembly::AssemblyObject", "ParentAssembly")
            instance = parent.newObject("Assembly::AssemblyLink", "Instance")
            instance.LinkedObject = self.assembly
            self.doc.recompute()
            local = next(obj for obj in instance.Group if obj.TypeId == "App::Link")
            local.Placement = App.Placement(App.Vector(51, 23, 7), App.Rotation())
            pose = local.Placement
            source.Placement = App.Placement(App.Vector(12, 34, 56), App.Rotation())
            instance.touch()
            self.doc.recompute()
            self.assertTrue(local.Placement.isSame(pose, 1e-9))
        instance.touch()
        self.doc.recompute()
        self.assertTrue(local.Placement.isSame(source.Placement, 1e-9))

    def test_motion_suspension_before_creation_and_document_close(self):
        other = App.newDocument("MotionSuspensionLifetime")
        name = other.Name
        try:
            with AssemblyApp.suspendAutomaticMotion(other):
                assembly = other.addObject("Assembly::AssemblyObject", "Assembly")
                assembly.newObject("Assembly::JointGroup", "Joints")
                self.assertEqual(assembly.solve(), -7)
                self.assertFalse(AssemblyApp.isAutomaticMotionSuspended(self.doc))
                App.closeDocument(name)
                other = App.newDocument(name)
                self.assertFalse(AssemblyApp.isAutomaticMotionSuspended(other))
            self.assertFalse(AssemblyApp.isAutomaticMotionSuspended(other))
        finally:
            App.closeDocument(name)

    def test_motion_suspension_callbacks_before_containment(self):
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            first, second, joint = self._suspended_joint_fixture(contained=False)
            poses = [part.Placement for part in (first, second)]
            suppressed = joint.Suppressed
            joint.Offset1 = App.Placement(App.Vector(4, 3, 2), App.Rotation())
            self.assertFalse(joint.Proxy.matchJCS(joint))
            self.assertEqual(joint.Suppressed, suppressed)
            joint.JointType = "Angle"
            joint.Angle = 25
            joint.Proxy.preventParallel(joint)
            joint.Proxy.setJointConnectors(joint, [])
            self.doc.recompute()
            for part, pose in zip((first, second), poses):
                self.assertTrue(part.Placement.isSame(pose, 1e-9))
            self.assertIsNone(joint.Reference1)
            self.assertIsNone(joint.Reference2)
        for part, pose in zip((first, second), poses):
            self.assertTrue(part.Placement.isSame(pose, 1e-9))

    def test_motion_suspension_preserves_presolve_undo(self):
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            first, second, joint = self._suspended_joint_fixture(contained=False)
            original = second.Placement
        self.assertTrue(joint.Proxy.matchJCS(joint))
        self.assertFalse(second.Placement.isSame(original, 1e-9))
        backups = dict(joint.Proxy.partsMovedByPresolved)
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            current = second.Placement
            joint.Proxy.undoPreSolve(joint)
            self.assertTrue(second.Placement.isSame(current, 1e-9))
            self.assertEqual(joint.Proxy.partsMovedByPresolved, backups)
        joint.Proxy.undoPreSolve(joint)
        self.assertTrue(second.Placement.isSame(original, 1e-9))

    def test_motion_suspension_rigid_group_restore(self):
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            first = self.assembly.newObject("Part::Box", "First")
            second = self.assembly.newObject("Part::Box", "Second")
            second.Placement = App.Placement(App.Vector(20, 0, 0), App.Rotation())
            rigid = self.doc.addObject("App::FeaturePython", "Rigid")
            JointObject.RigidGroupJoint(rigid, [first, second])
            first.Placement = App.Placement(App.Vector(100, 0, 0), App.Rotation())
            pose = second.Placement
            rigid.Suppressed = True
            rigid.Suppressed = False
            rigid.Proxy.restoreFromPlacements(rigid)
            self.assertTrue(second.Placement.isSame(pose, 1e-9))
        rigid.Proxy.restoreFromPlacements(rigid)
        self.assertTrue(second.Placement.Base.isEqual(App.Vector(120, 0, 0), 1e-9))

    def test_motion_suspension_grounded_status_and_validated_solve(self):
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            first, second, joint = self._suspended_joint_fixture()
            ground = self.jointgroup.newObject("App::FeaturePython", "Grounded")
            status = first.getPropertyStatus("Placement")
            JointObject.GroundedJoint(ground, first)
            self.assertEqual(first.getPropertyStatus("Placement"), status)
            self.assertEqual(self.assembly.solve(), -7)
            self.assertEqual(self.assembly.solveValidated(), 0)
            self.assertIn("ReadOnly", first.getPropertyStatus("Placement"))
            self.assertTrue(AssemblyApp.isAutomaticMotionSuspended(self.doc))
        self.assertEqual(self.assembly.solve(), 0)

    def test_motion_suspension_reconciles_ground_retargeting(self):
        first = self.assembly.newObject("Part::Box", "First")
        second = self.assembly.newObject("Part::Box", "Second")
        ground = self.jointgroup.newObject("App::FeaturePython", "Grounded")
        JointObject.GroundedJoint(ground, first)
        self.assertIn("ReadOnly", first.getPropertyStatus("Placement"))
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            ground.ObjectToGround = second
            self.assertIn("ReadOnly", first.getPropertyStatus("Placement"))
            self.assertNotIn("ReadOnly", second.getPropertyStatus("Placement"))
            self.assertEqual(self.assembly.solveValidated(), 0)
            self.assertNotIn("ReadOnly", first.getPropertyStatus("Placement"))
            self.assertIn("ReadOnly", second.getPropertyStatus("Placement"))
            targets = [
                j.ObjectToGround for j in self.jointgroup.Group if hasattr(j, "ObjectToGround")
            ]
            self.assertEqual(targets, [second])

    def test_motion_suspension_reconciles_ground_deletion(self):
        first = self.assembly.newObject("Part::Box", "First")
        second = self.assembly.newObject("Part::Box", "Second")
        manually_locked = self.assembly.newObject("Part::Box", "ManualLock")
        ground = self.jointgroup.newObject("App::FeaturePython", "FirstGround")
        JointObject.GroundedJoint(ground, first)
        retained_ground = self.jointgroup.newObject("App::FeaturePython", "SecondGround")
        JointObject.GroundedJoint(retained_ground, second)
        manually_locked.setPropertyStatus("Placement", "ReadOnly")
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            self.doc.removeObject(ground.Name)
            self.assertIn("ReadOnly", first.getPropertyStatus("Placement"))
            self.assertEqual(self.assembly.solveValidated(), 0)
            self.assertNotIn("ReadOnly", first.getPropertyStatus("Placement"))
            self.assertIn("ReadOnly", second.getPropertyStatus("Placement"))
            self.assertIn("ReadOnly", manually_locked.getPropertyStatus("Placement"))
            targets = [
                j.ObjectToGround for j in self.jointgroup.Group if hasattr(j, "ObjectToGround")
            ]
            self.assertNotIn(first, targets)

    def test_motion_suspension_rejected_graph_preserves_pending_ground_release(self):
        first = self.assembly.newObject("Part::Box", "First")
        second = self.assembly.newObject("Part::Box", "Second")
        ground = self.jointgroup.newObject("App::FeaturePython", "Grounded")
        JointObject.GroundedJoint(ground, first)
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            ground.ObjectToGround = second
            incomplete = self.jointgroup.newObject("App::FeaturePython", "Incomplete")
            JointObject.Joint(incomplete, JointObject.JointTypes.index("Fixed"))
            self.assertEqual(self.assembly.solveValidated(), -8)
            self.assertIn("ReadOnly", first.getPropertyStatus("Placement"))
            self.assertNotIn("ReadOnly", second.getPropertyStatus("Placement"))
            self.doc.removeObject(incomplete.Name)
            self.assertEqual(self.assembly.solveValidated(), 0)
            self.assertNotIn("ReadOnly", first.getPropertyStatus("Placement"))
            self.assertIn("ReadOnly", second.getPropertyStatus("Placement"))

    def test_motion_suspension_rejects_escaped_ground_without_source_effects(self):
        outside = self.doc.addObject("Part::Box", "OutsideAssembly")
        outside.Placement = App.Placement(App.Vector(80, 10, 0), App.Rotation())
        self.doc.recompute()
        pose = outside.Placement
        status = outside.getPropertyStatus("Placement")
        with AssemblyApp.suspendAutomaticMotion(self.doc):
            local = self.assembly.newObject("Part::Box", "Local")
            target_pose = local.Placement
            ground = self.jointgroup.newObject("App::FeaturePython", "Grounded")
            JointObject.GroundedJoint(ground, outside)
            self.assertEqual(outside.getPropertyStatus("Placement"), status)
            self.assertEqual(self.assembly.solveValidated(), -8)
            self.assertTrue(outside.Placement.isSame(pose, 1e-9))
            self.assertTrue(local.Placement.isSame(target_pose, 1e-9))
            self.assertEqual(outside.getPropertyStatus("Placement"), status)
            self.doc.removeObject(ground.Name)

    def test_motion_suspension_validated_external_link(self):
        source = App.newDocument("MotionSuspensionLinkSource")
        directory = tempfile.TemporaryDirectory()
        try:
            external = source.addObject("Part::Box", "External")
            external.Placement = App.Placement(App.Vector(80, 10, 0), App.Rotation())
            source.recompute()
            source.saveAs(os.path.join(directory.name, "Source.FCStd"))
            self.doc.saveAs(os.path.join(directory.name, "Assembly.FCStd"))
            pose = external.Placement
            with AssemblyApp.suspendAutomaticMotion(self.doc):
                first = self.assembly.newObject("Part::Box", "Local")
                link = self.assembly.newObject("App::Link", "ExternalInstance")
                link.LinkedObject = external
                link.Placement = App.Placement(App.Vector(100, 20, 0), App.Rotation())
                self.doc.recompute()
                ground = self.jointgroup.newObject("App::FeaturePython", "Grounded")
                JointObject.GroundedJoint(ground, first)
                joint = self.jointgroup.newObject("App::FeaturePython", "Joint")
                JointObject.Joint(joint, JointObject.JointTypes.index("Fixed"))
                if App.GuiUp:
                    JointObject.ViewProviderJoint(joint.ViewObject)
                joint.Proxy.setJointConnectors(joint, [(first, ["Face1"]), (link, ["Face1"])])
                self.assertEqual(self.assembly.solveValidated(), 0)
                self.assertTrue(external.Placement.isSame(pose, 1e-9))
                self.assertTrue(AssemblyApp.isAutomaticMotionSuspended(self.doc))
        finally:
            App.closeDocument(source.Name)
            directory.cleanup()

    @unittest.skipUnless(App.GuiUp, "Native Motion Python proxy requires the GUI module")
    def test_motion_suspension_native_motion_admission(self):
        from CommandCreateSimulation import Motion, Simulation

        source = App.newDocument("MotionSuspensionDriveSource")
        directory = tempfile.TemporaryDirectory()
        try:
            escaped_joint = source.addObject("App::FeaturePython", "EscapedJoint")
            JointObject.Joint(escaped_joint, JointObject.JointTypes.index("Revolute"))
            JointObject.ViewProviderJoint(escaped_joint.ViewObject)
            source.saveAs(os.path.join(directory.name, "Source.FCStd"))
            self.doc.saveAs(os.path.join(directory.name, "Assembly.FCStd"))
            with AssemblyApp.suspendAutomaticMotion(self.doc):
                first, second, joint = self._suspended_joint_fixture("Revolute")
                ground = self.jointgroup.newObject("App::FeaturePython", "Grounded")
                JointObject.GroundedJoint(ground, first)
                group = self.assembly.newObject("Assembly::SimulationGroup", "Simulations")
                simulation = group.newObject("App::FeaturePython", "Simulation")
                Simulation(simulation)
                motion = self.assembly.newObject("App::FeaturePython", "Motion")
                Motion(motion, "Angular", joint, "30*time")
                simulation.Group = [motion]
                self.assertEqual(self.assembly.solveValidated(), 0)
                self.assertEqual(self.assembly.generateSimulation(simulation), -7)

                motion.Joint = escaped_joint
                poses = [part.Placement for part in (first, second)]
                self.assertEqual(self.assembly.solveValidated(), -8)
                for part, pose in zip((first, second), poses):
                    self.assertTrue(part.Placement.isSame(pose, 1e-9))
                motion.Joint = joint
        finally:
            App.closeDocument(source.Name)
            directory.cleanup()

    @unittest.skipUnless(App.GuiUp, "Native Motion Python proxy requires the GUI module")
    def test_mixed_rigid_occurrences_admit_only_active_motion_targets(self):
        from CommandCreateSimulation import Motion, Simulation

        def closure(owner):
            result = [owner]
            for obj in result:
                result.extend(child for child in getattr(obj, "Group", ()) if child not in result)
            return result

        def poses_and_status(objects):
            return [
                (
                    obj.Name,
                    tuple(obj.Placement.toMatrix().A) if hasattr(obj, "Placement") else None,
                    (
                        tuple(obj.LinkPlacement.toMatrix().A)
                        if hasattr(obj, "LinkPlacement")
                        else None
                    ),
                    (
                        tuple(obj.getPropertyStatus("Placement"))
                        if hasattr(obj, "Placement")
                        else None
                    ),
                    (
                        tuple(obj.getPropertyStatus("LinkPlacement"))
                        if hasattr(obj, "LinkPlacement")
                        else None
                    ),
                )
                for obj in objects
            ]

        def snapshot(doc):
            return (
                poses_and_status(doc.Objects),
                [(obj.Name, tuple(obj.State)) for obj in doc.Objects],
                doc.isTouched(),
                doc.UndoCount,
            )

        def pivot(owner):
            return next(
                obj for obj in closure(owner) if getattr(obj, "JointType", None) == "Revolute"
            )

        documents = []
        with tempfile.TemporaryDirectory() as directory, ExitStack() as scopes:

            def document():
                doc = App.newDocument()
                documents.append(doc)
                scopes.enter_context(AssemblyApp.suspendAutomaticMotion(doc))
                doc.saveAs(os.path.join(directory, doc.Name + ".FCStd"))
                return doc

            def assembly(doc):
                obj = doc.addObject("Assembly::AssemblyObject", "Assembly")
                obj.Type = "Assembly"
                obj.newObject("Assembly::JointGroup", "Joints")
                return obj

            def occurrence(owner, source, rigid):
                obj = owner.newObject("Assembly::AssemblyLink", "Occurrence")
                obj.Rigid = rigid
                obj.LinkedObject = source
                for _ in range(3):
                    owner.Document.recompute()
                self.assertTrue(obj.Group)
                return obj

            scopes.enter_context(AssemblyApp.suspendAutomaticMotion(self.doc))
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
                    link = unit.newObject("App::Link", name)
                    link.LinkedObject = definitions.getObject(name)
                    link.LinkTransform = False
                    parts.append(link)
                group = next(obj for obj in unit.Group if obj.TypeId == "Assembly::JointGroup")
                ground = group.newObject("App::FeaturePython", "Ground")
                JointObject.GroundedJoint(ground, parts[0])
                joint = group.newObject("App::FeaturePython", "Pivot")
                JointObject.Joint(joint, JointObject.JointTypes.index("Revolute"))
                JointObject.ViewProviderJoint(joint.ViewObject)
                joint.Detach1 = joint.Detach2 = True
                joint.Reference1 = (parts[0], [""])
                joint.Reference2 = (parts[1], [""])
                joint.Placement1 = joint.Placement2 = App.Placement()
                mechanism.recompute()
                self.assertEqual(unit.solveValidated(), 0)
                mechanism.save()
                middle_doc = document()
                middle = assembly(middle_doc)
                occurrence(middle, unit, False)
                middle_doc.save()
                self.doc.saveAs(os.path.join(directory, "Target.FCStd"))
                frozen = occurrence(self.assembly, middle, True)
                flexible = occurrence(self.assembly, middle, False)
                self.assertTrue(
                    any(
                        obj.TypeId == "Assembly::AssemblyLink" and not obj.Rigid
                        for obj in closure(frozen)
                        if obj != frozen
                    )
                )
                simulations = self.assembly.newObject("Assembly::SimulationGroup", "Simulations")
                simulation = simulations.newObject("App::FeaturePython", "Simulation")
                Simulation(simulation)
                motion = self.assembly.newObject("App::FeaturePython", "Motion")
                Motion(motion, "Angular", pivot(flexible), "30*time")
                simulation.Group = [motion]
                self.doc.recompute()
                sources = [(doc, snapshot(doc)) for doc in documents]
                frozen_arm = next(
                    obj
                    for obj in closure(frozen)
                    if obj.TypeId == "App::Link" and obj.getLinkedObject(True).Name == "Arm"
                )

                for angle in (25, -40):
                    with self.subTest(angle=angle):
                        frozen_arm.LinkPlacement = App.Placement(
                            App.Vector(), App.Rotation(App.Vector(0, 0, 1), angle)
                        )
                        frozen_before = poses_and_status(closure(frozen))
                        self.assertEqual(self.assembly.solveValidated(), 0)
                        self.assertEqual(poses_and_status(closure(frozen)), frozen_before)
                        for target in (pivot(frozen), pivot(flexible)):
                            motion.Joint = target
                            target.Suppressed = target != pivot(frozen)
                            before = snapshot(self.doc)
                            self.assertEqual(self.assembly.solveValidated(True), -8)
                            self.assertEqual(snapshot(self.doc), before)
                            target.Suppressed = False
                        motion.Joint = pivot(flexible)
                        self.assertEqual(self.assembly.solve(), -7)
                        self.assertTrue(AssemblyApp.isAutomaticMotionSuspended(self.doc))
                        self.assertEqual(poses_and_status(closure(frozen)), frozen_before)
                        for doc, before in sources:
                            self.assertEqual(snapshot(doc), before)
            finally:
                for doc in reversed(documents):
                    App.closeDocument(doc.Name)

    @unittest.skipUnless(App.GuiUp, "Native Motion Python proxy requires the GUI module")
    def test_posed_flexible_wrappers_keep_motion_holds_and_fixed_tool(self):
        self._check_posed_flexible_motion_holds(body_sources=False)

    @unittest.skipUnless(App.GuiUp, "Native Motion Python proxy requires the GUI module")
    def test_linked_bodies_keep_posed_motion_holds_and_fixed_tool(self):
        self._check_posed_flexible_motion_holds(body_sources=True)

    def _check_posed_flexible_motion_holds(self, body_sources):
        from CommandCreateSimulation import Motion, Simulation

        def closure(owner):
            result = [owner]
            for obj in result:
                result.extend(child for child in getattr(obj, "Group", ()) if child not in result)
            return result

        def snapshot(doc):
            return (
                [
                    (
                        obj.Name,
                        tuple(obj.State),
                        tuple(obj.Placement.toMatrix().A) if hasattr(obj, "Placement") else None,
                        (
                            tuple(obj.getPropertyStatus("Placement"))
                            if hasattr(obj, "Placement")
                            else None
                        ),
                    )
                    for obj in doc.Objects
                ],
                doc.isTouched(),
                doc.UndoCount,
            )

        documents = []
        with tempfile.TemporaryDirectory() as directory, ExitStack() as source_scopes:

            def document():
                doc = App.newDocument()
                documents.append(doc)
                source_scopes.enter_context(AssemblyApp.suspendAutomaticMotion(doc))
                doc.saveAs(os.path.join(directory, doc.Name + ".FCStd"))
                return doc

            def assembly(doc):
                obj = doc.addObject("Assembly::AssemblyObject", "Assembly")
                obj.Type = "Assembly"
                obj.newObject("Assembly::JointGroup", "Joints")
                return obj

            def occurrence(owner, source):
                obj = owner.newObject("Assembly::AssemblyLink", "Occurrence")
                obj.Rigid = False
                obj.LinkedObject = source
                for _ in range(3):
                    owner.Document.recompute()
                self.assertTrue(obj.Group)
                return obj

            def joint(group, name, kind, first, second, offset):
                obj = group.newObject("App::FeaturePython", name)
                JointObject.Joint(obj, JointObject.JointTypes.index(kind))
                JointObject.ViewProviderJoint(obj.ViewObject)
                obj.Detach1 = obj.Detach2 = True
                obj.Reference1 = (first, [""])
                obj.Reference2 = (second, [""])
                obj.Placement1 = App.Placement(offset, App.Rotation())
                obj.Placement2 = App.Placement()
                if kind == "Revolute":
                    obj.AngleMin, obj.AngleMax = -60, 60
                    obj.EnableAngleMin = obj.EnableAngleMax = True
                return obj

            try:
                with AssemblyApp.suspendAutomaticMotion(self.doc):
                    source = document()
                    unit = assembly(source)
                    parts = {}
                    for name, offset in (("Base", 0), ("Head", 0), ("Cam", 15), ("Tool", 5)):
                        if body_sources:
                            part = unit.newObject("PartDesign::Body", name)
                            box = part.newObject("PartDesign::AdditiveBox", name + "Box")
                            box.Length, box.Width, box.Height = 2, 3, 4
                        else:
                            part = unit.newObject("Part::Feature", name)
                            part.Shape = Part.makeCylinder(2, 4)
                        part.Placement.Base = App.Vector(offset, 0, 0)
                        parts[name] = part
                    group = next(obj for obj in unit.Group if obj.TypeId == "Assembly::JointGroup")
                    joint(
                        group, "HeadPivot", "Revolute", parts["Base"], parts["Head"], App.Vector()
                    )
                    joint(
                        group,
                        "CamPivot",
                        "Revolute",
                        parts["Base"],
                        parts["Cam"],
                        App.Vector(15, 0, 0),
                    )
                    joint(
                        group,
                        "ToolMount",
                        "Fixed",
                        parts["Head"],
                        parts["Tool"],
                        App.Vector(5, 0, 0),
                    )
                    source.recompute()
                    source.save()
                    middle_doc = document()
                    middle = assembly(middle_doc)
                    occurrence(middle, unit)
                    middle_doc.save()
                    self.doc.saveAs(os.path.join(directory, "Target.FCStd"))
                    outer = occurrence(self.assembly, middle)
                    inner = next(
                        obj for obj in outer.Group if obj.TypeId == "Assembly::AssemblyLink"
                    )
                    self.assembly.Placement = App.Placement(
                        App.Vector(7, -3, 11), App.Rotation(17, 23, 9)
                    )
                    outer.Placement = App.Placement(
                        App.Vector(40, 8, -6), App.Rotation(31, -19, 14)
                    )
                    inner.Placement = App.Placement(
                        App.Vector(-4, 12, 3), App.Rotation(-21, 16, 37)
                    )
                    local_parts = {
                        obj.getLinkedObject(True).Name: obj
                        for obj in inner.Group
                        if obj.TypeId == "App::Link"
                    }
                    joints = {obj.Label: obj for obj in closure(inner) if hasattr(obj, "JointType")}
                    head, cam, fixed = (
                        joints[name] for name in ("HeadPivot", "CamPivot", "ToolMount")
                    )
                    for pivot in (head, cam):
                        self.assertEqual(
                            pivot.getTypeIdOfProperty("AngleMin"), "App::PropertyAngle"
                        )
                        pivot.AngleMin, pivot.AngleMax = -60, 60
                        pivot.EnableAngleMin = pivot.EnableAngleMax = True
                    ground = self.jointgroup.newObject("App::FeaturePython", "RootGround")
                    JointObject.GroundedJoint(ground, local_parts["Base"])
                    simulations = self.assembly.newObject(
                        "Assembly::SimulationGroup", "Simulations"
                    )
                    simulation = simulations.newObject("App::FeaturePython", "Simulation")
                    Simulation(simulation)
                    simulation.bTimeEnd = 0.02
                    motions = []
                    for pivot, angle in ((head, 25), (cam, -40)):
                        motion = self.assembly.newObject("App::FeaturePython", "Motion")
                        Motion(motion, "Angular", pivot, f"{angle}*pi/180")
                        motions.append(motion)
                    simulation.Group = motions
                    self.doc.recompute()
                    wrappers = [
                        App.Placement(obj.Placement) for obj in (self.assembly, outer, inner)
                    ]
                    anchor = (
                        self.assembly.Placement
                        * outer.Placement
                        * inner.Placement
                        * local_parts["Base"].Placement
                    )
                    sources = []
                    for doc in documents:
                        with open(doc.FileName, "rb") as saved:
                            sources.append(
                                (doc, snapshot(doc), hashlib.sha256(saved.read()).hexdigest())
                            )
                    self.assertEqual(self.assembly.solveValidated(), 0)
                    self.assertTrue(AssemblyApp.isAutomaticMotionSuspended(self.doc))

                # This oracle composes the fixture's known containment, independently
                # of native placement/current-joint helpers and source link geometry.
                def world(part):
                    return (
                        self.assembly.Placement * outer.Placement * inner.Placement * part.Placement
                    )

                def check_constraints(expected_angles):
                    for pivot in (head, cam, fixed):
                        first = world(pivot.Reference1[0]) * pivot.Placement1
                        second = world(pivot.Reference2[0]) * pivot.Placement2
                        self.assertLess((first.Base - second.Base).Length, 1e-6, pivot.Label)
                        relative = first.Rotation.inverted() * second.Rotation
                        if pivot == fixed:
                            self.assertLess(math.degrees(relative.Angle), 1e-6)
                        else:
                            self.assertTrue(
                                relative.multVec(App.Vector(0, 0, 1)).isEqual(
                                    App.Vector(0, 0, 1), 1e-8
                                )
                            )
                            axis = relative.multVec(App.Vector(1, 0, 0))
                            angle = math.degrees(math.atan2(axis.y, axis.x))
                            self.assertAlmostEqual(angle, expected_angles[pivot.Label], delta=1e-6)
                            self.assertGreaterEqual(angle, pivot.AngleMin.Value - 1e-6)
                            self.assertLessEqual(angle, pivot.AngleMax.Value + 1e-6)

                def apply_frames(expected_angles, extra_checks=None):
                    with AssemblyApp.suspendAutomaticMotion(self.doc):
                        before = snapshot(self.doc)
                        self.assertEqual(self.assembly.generateSimulation(simulation), -7)
                        self.assembly.updateForFrame(0)
                        self.assertEqual(snapshot(self.doc), before)
                        self.assertTrue(AssemblyApp.isAutomaticMotionSuspended(self.doc))
                    self.assertFalse(AssemblyApp.isAutomaticMotionSuspended(self.doc))
                    input_poses = {
                        name: App.Placement(part.Placement) for name, part in local_parts.items()
                    }
                    input_angles = {}
                    for pivot in (head, cam):
                        first = world(pivot.Reference1[0]) * pivot.Placement1
                        second = world(pivot.Reference2[0]) * pivot.Placement2
                        axis = (first.Rotation.inverted() * second.Rotation).multVec(
                            App.Vector(1, 0, 0)
                        )
                        input_angles[pivot.Label] = math.degrees(math.atan2(axis.y, axis.x))
                    self.assertEqual(self.assembly.generateSimulation(simulation), 0)
                    self.assertGreater(self.assembly.numberOfFrames(), 1)
                    # Slot zero is the native series' input pose; the GUI player
                    # starts at frame one, the first solved motion frame.
                    self.assembly.updateForFrame(0)
                    for name, part in local_parts.items():
                        self.assertTrue(part.Placement.isSame(input_poses[name], 1e-8))
                    check_constraints(input_angles)
                    if extra_checks:
                        extra_checks()
                    self.assertTrue(world(local_parts["Base"]).isSame(anchor, 1e-8))
                    for index in range(1, self.assembly.numberOfFrames()):
                        self.assembly.updateForFrame(index)
                        check_constraints(expected_angles)
                        if extra_checks:
                            extra_checks()
                        self.assertTrue(world(local_parts["Base"]).isSame(anchor, 1e-8))
                    for obj, pose in zip((self.assembly, outer, inner), wrappers):
                        self.assertTrue(obj.Placement.isSame(pose, 1e-9))
                    for doc, state, digest in sources:
                        self.assertEqual(snapshot(doc), state)
                        with open(doc.FileName, "rb") as saved:
                            self.assertEqual(hashlib.sha256(saved.read()).hexdigest(), digest)

                apply_frames({"HeadPivot": 25, "CamPivot": -40})
                with AssemblyApp.suspendAutomaticMotion(self.doc):
                    motions[0].Formula = "35*pi/180"
                apply_frames({"HeadPivot": 35, "CamPivot": -40})
                with AssemblyApp.suspendAutomaticMotion(self.doc):
                    simulation.Group = [motions[1]]
                    self.doc.removeObject(motions[0].Name)
                    head_part = local_parts["Head"]
                    head_part.Placement = App.Placement(
                        head_part.Placement.Base, App.Rotation(App.Vector(0, 0, 1), 10)
                    )
                    local_parts["Tool"].Placement = head_part.Placement * fixed.Placement1
                apply_frames({"HeadPivot": 10, "CamPivot": -40})

                # Repeat the complete mechanism, retaining every internal joint,
                # and couple its Base 40mm from the first through a root Fixed.
                with AssemblyApp.suspendAutomaticMotion(self.doc):
                    other_outer = occurrence(self.assembly, middle)
                    other_inner = next(
                        obj for obj in other_outer.Group if obj.TypeId == "Assembly::AssemblyLink"
                    )
                    other_outer.Placement = (
                        outer.Placement
                        * inner.Placement
                        * App.Placement(App.Vector(40, 0, 0), App.Rotation())
                    )
                    other_inner.Placement = App.Placement()
                    other_parts = {
                        obj.getLinkedObject(True).Name: obj
                        for obj in other_inner.Group
                        if obj.TypeId == "App::Link"
                    }
                    other_joints = [
                        obj for obj in closure(other_inner) if hasattr(obj, "JointType")
                    ]
                    self.assertEqual(len(other_joints), 3)
                    self.assertTrue(all(not obj.Suppressed for obj in other_joints))
                    coupling = joint(
                        self.jointgroup,
                        "JoinBases",
                        "Fixed",
                        local_parts["Base"],
                        other_parts["Base"],
                        App.Vector(40, 0, 0),
                    )
                    held_head = self.assembly.newObject("App::FeaturePython", "HeadHold")
                    Motion(held_head, "Angular", head, "25*pi/180")
                    simulation.Group = [held_head, motions[1]]
                    self.assertEqual(self.assembly.solveValidated(), 0)
                other_wrappers = [
                    App.Placement(obj.Placement) for obj in (other_outer, other_inner)
                ]

                def other_world(part):
                    return (
                        self.assembly.Placement
                        * other_outer.Placement
                        * other_inner.Placement
                        * part.Placement
                    )

                def check_second_mechanism():
                    for obj in other_joints + [coupling]:
                        first_world = world if obj == coupling else other_world
                        first = first_world(obj.Reference1[0]) * obj.Placement1
                        second = other_world(obj.Reference2[0]) * obj.Placement2
                        self.assertLess((first.Base - second.Base).Length, 1e-6, obj.Label)
                        relative = first.Rotation.inverted() * second.Rotation
                        if obj.JointType == "Fixed":
                            self.assertLess(math.degrees(relative.Angle), 1e-6)
                        else:
                            self.assertTrue(
                                relative.multVec(App.Vector(0, 0, 1)).isEqual(
                                    App.Vector(0, 0, 1), 1e-8
                                )
                            )
                    for obj, pose in zip((other_outer, other_inner), other_wrappers):
                        self.assertTrue(obj.Placement.isSame(pose, 1e-9))

                apply_frames({"HeadPivot": 25, "CamPivot": -40}, check_second_mechanism)
                with AssemblyApp.suspendAutomaticMotion(self.doc):
                    simulation.Group = [motions[1]]
                    self.doc.removeObject(held_head.Name)
                    head_part.Placement = App.Placement(
                        head_part.Placement.Base, App.Rotation(App.Vector(0, 0, 1), 10)
                    )
                    local_parts["Tool"].Placement = head_part.Placement * fixed.Placement1
                apply_frames({"HeadPivot": 10, "CamPivot": -40}, check_second_mechanism)

                # Unlike the same-wrapper holds, this initialValue must include
                # two distinct containing frames (whose yaw differs by 45deg).
                with AssemblyApp.suspendAutomaticMotion(self.doc):
                    coupling.Suppressed = True
                    other_outer.Placement = (
                        outer.Placement
                        * inner.Placement
                        * App.Placement(App.Vector(), App.Rotation(App.Vector(0, 0, 1), 45))
                    )
                    for obj in closure(other_inner):
                        if hasattr(obj, "JointType"):
                            obj.Suppressed = True
                    other_parts["Head"].Placement = App.Placement()
                    for name in ("Base", "Cam", "Tool"):
                        extra_ground = self.jointgroup.newObject(
                            "App::FeaturePython", "OtherGround"
                        )
                        JointObject.GroundedJoint(extra_ground, other_parts[name])
                    cross = joint(
                        self.jointgroup,
                        "CrossWrapperPivot",
                        "Revolute",
                        local_parts["Base"],
                        other_parts["Head"],
                        App.Vector(),
                    )
                    initial_hold = self.assembly.newObject("App::FeaturePython", "InitialHold")
                    Motion(initial_hold, "Angular", cross, "initialValue")
                    simulation.Group = [motions[1], initial_hold]
                    self.assertEqual(self.assembly.solveValidated(), 0)
                other_wrappers = [
                    App.Placement(obj.Placement) for obj in (other_outer, other_inner)
                ]
                apply_frames({"HeadPivot": 10, "CamPivot": -40})
                for index in range(1, self.assembly.numberOfFrames()):
                    self.assembly.updateForFrame(index)
                    first = world(local_parts["Base"]) * cross.Placement1
                    second = (
                        self.assembly.Placement
                        * other_outer.Placement
                        * other_inner.Placement
                        * other_parts["Head"].Placement
                        * cross.Placement2
                    )
                    self.assertLess((first.Base - second.Base).Length, 1e-6)
                    relative = first.Rotation.inverted() * second.Rotation
                    self.assertTrue(
                        relative.multVec(App.Vector(0, 0, 1)).isEqual(App.Vector(0, 0, 1), 1e-8)
                    )
                    axis = relative.multVec(App.Vector(1, 0, 0))
                    self.assertAlmostEqual(math.degrees(math.atan2(axis.y, axis.x)), 45, delta=1e-6)
                for obj, pose in zip((other_outer, other_inner), other_wrappers):
                    self.assertTrue(obj.Placement.isSame(pose, 1e-9))
            finally:
                for doc in reversed(documents):
                    App.closeDocument(doc.Name)

    def test_component_count_for_link_array(self):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("App::Link", "Array")
        array.LinkedObject = source
        array.ElementCount = 3
        array.ShowElement = False
        self.doc.recompute()
        self.assertEqual(len(array.ElementList), 0)
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 3)

        array.ShowElement = True
        self.doc.recompute()
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 3)
        array.ElementList[1].Suppressed = True
        self.doc.recompute()
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 2)
        array.ElementList[1].Suppressed = False
        self.doc.recompute()
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 3)

    def test_generated_array_is_one_component(self):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("Part::LinkArrayLinear", "Array")
        array.LinkedObject = source
        array.Occurrences = 3
        self.doc.recompute()
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 1)
        self.assertEqual(UtilsAssembly.getSubMovingParts(array, False), [array])
        self.assertEqual(UtilsAssembly.getObject((array, ["1.Face1"])), array)

    def test_suppressed_link_elements_are_not_movable(self):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("App::Link", "Array")
        array.LinkedObject = source
        array.ElementCount = 3
        self.doc.recompute()
        element = array.ElementList[1]
        element.Suppressed = True
        self.doc.recompute()
        self.assertNotIn(element, UtilsAssembly.getMovablePartsWithin(array))
        element.Suppressed = False
        self.doc.recompute()
        self.assertIn(element, UtilsAssembly.getMovablePartsWithin(array))

    def test_assembly_link_synchronizes_element_suppression(self):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("App::Link", "Array")
        array.LinkedObject = source
        array.ElementCount = 3
        array.ElementList[1].Suppressed = True
        parent = self.doc.addObject("Assembly::AssemblyObject", "ParentAssembly")
        instance = parent.newObject("Assembly::AssemblyLink", "Instance")
        instance.LinkedObject = self.assembly
        self.doc.recompute()
        local_array = next(obj for obj in instance.Group if obj.TypeId == "App::Link")
        self.assertTrue(local_array.ElementList[1].Suppressed)
        array.ElementList[1].Suppressed = False
        instance.touch()
        self.doc.recompute()
        self.assertFalse(local_array.ElementList[1].Suppressed)

    def test_assembly_link_maps_generated_array_joint(self):
        self._check_generated_array_joint("Face1")

    def test_assembly_link_maps_generated_array_whole_element_joint(self):
        self._check_generated_array_joint("")

    def _check_generated_array_joint(self, sub):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("Part::LinkArrayLinear", "Array")
        array.LinkedObject = source
        array.Occurrences = 3
        array.ShowElement = True
        self.doc.recompute()
        joint = self.jointgroup.newObject("App::FeaturePython", "Joint")
        JointObject.Joint(joint, 0)
        joint.Reference1 = (array.ElementList[1], [sub])
        joint.Reference2 = (array.ElementList[2], ["Face1"])
        source_poses = {obj.Name: obj.Placement for obj in [array] + list(array.ElementList)}
        parent = self.doc.addObject("Assembly::AssemblyObject", "ParentAssembly")
        instance = parent.newObject("Assembly::AssemblyLink", "Instance")
        instance.LinkedObject = self.assembly
        instance.Rigid = False
        self.doc.recompute()
        local_array = next(obj for obj in instance.Group if obj.TypeId == "App::Link")
        local_group = next(obj for obj in instance.Group if obj.TypeId == "Assembly::JointGroup")
        local_joint = local_group.Group[0]
        self.assertEqual(local_joint.Reference1, (local_array, ["1." + sub]))
        self.assertIsNotNone(local_array.getSubObject("1." + sub))
        if AssemblyApp.isAutomaticMotionSuspended(self.doc):
            for name, pose in source_poses.items():
                self.assertTrue(self.doc.getObject(name).Placement.isSame(pose, 1e-9))

    def test_create_assembly(self):
        """Create an assembly."""
        operation = "Create Assembly Object"
        _msg("  Test '{}'".format(operation))
        self.assertTrue(self.assembly, "'{}' failed".format(operation))

    def test_create_jointGroup(self):
        """Create a joint group in an assembly."""
        operation = "Create JointGroup Object"
        _msg("  Test '{}'".format(operation))
        self.assertTrue(self.jointgroup, "'{}' failed".format(operation))

    def test_create_joint(self):
        """Create a joint in an assembly."""
        operation = "Create Joint Object"
        _msg("  Test '{}'".format(operation))

        joint = self.jointgroup.newObject("App::FeaturePython", "testJoint")
        self.assertTrue(joint, "'{}' failed (FeaturePython creation failed)".format(operation))
        JointObject.Joint(joint, 0)

        self.assertTrue(hasattr(joint, "JointType"), "'{}' failed".format(operation))

    def test_create_grounded_joint(self):
        """Create a grounded joint in an assembly."""
        operation = "Create Grounded Joint Object"
        _msg("  Test '{}'".format(operation))

        groundedjoint = self.jointgroup.newObject("App::FeaturePython", "testJoint")
        self.assertTrue(
            groundedjoint, "'{}' failed (FeaturePython creation failed)".format(operation)
        )

        box = self.assembly.newObject("Part::Box", "Box")

        JointObject.GroundedJoint(groundedjoint, box)

        self.assertTrue(
            hasattr(groundedjoint, "ObjectToGround"),
            "'{}' failed: No attribute 'ObjectToGround'".format(operation),
        )
        self.assertTrue(
            groundedjoint.ObjectToGround == box,
            "'{}' failed: ObjectToGround not set correctly.".format(operation),
        )

    def test_toggle_grounded_joint(self):
        """test grounding and ungrounding a part, added because of github.com/freecad/freecad/issues/28440"""
        operation = "Toggle Grounded Joint"
        _msg("  Test '{}'".format(operation))

        box = self.assembly.newObject("Part::Box", "Box")

        # ground the part
        groundedjoint = self.jointgroup.newObject("App::FeaturePython", "GroundedJoint")
        JointObject.GroundedJoint(groundedjoint, box)
        self.doc.recompute()

        # verify grounded
        self.assertTrue(
            hasattr(groundedjoint, "ObjectToGround"),
            "'{}' failed: No attribute 'ObjectToGround'".format(operation),
        )
        self.assertEqual(
            groundedjoint.ObjectToGround,
            box,
            "'{}' failed: ObjectToGround not set correctly".format(operation),
        )

        # unground the part
        self.doc.removeObject(groundedjoint.Name)
        self.doc.recompute()

        # verify no grounded joints remain in this part
        for joint in self.jointgroup.Group:
            if hasattr(joint, "ObjectToGround"):
                self.assertNotEqual(
                    joint.ObjectToGround,
                    box,
                    "'{}' failed: part still grounded after toggle".format(operation),
                )

    def test_find_placement(self):
        """Test find placement of joint."""
        operation = "Find placement"
        _msg("  Test '{}'".format(operation))

        joint = self.jointgroup.newObject("App::FeaturePython", "testJoint")
        JointObject.Joint(joint, 0)

        L = 2
        W = 3
        H = 7
        box = self.assembly.newObject("Part::Box", "Box")
        box.Length = L
        box.Width = W
        box.Height = H
        box.Placement = App.Placement(App.Vector(10, 20, 30), App.Rotation(15, 25, 35))

        # Step 0 : box with placement. No element selected
        ref = [self.assembly, [box.Name + ".", box.Name + "."]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(), App.Rotation())
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 0".format(operation))

        # Step 1 : box with placement. Face + Vertex
        ref = [self.assembly, [box.Name + ".Face6", box.Name + ".Vertex7"]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(L, W, H), App.Rotation())
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 1".format(operation))

        # Step 2 : box with placement. Edge + Vertex
        ref = [self.assembly, [box.Name + ".Edge8", box.Name + ".Vertex8"]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(L, W, 0), App.Rotation(0, -90, 270))
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 2".format(operation))

        # Step 3 : box with placement. Vertex
        ref = [self.assembly, [box.Name + ".Vertex3", box.Name + ".Vertex3"]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(0, W, H), App.Rotation())
        _msg("  plc '{}'".format(plc))
        _msg("  targetPlc '{}'".format(targetPlc))
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 3".format(operation))

        # Step 4 : box with placement. Face
        ref = [self.assembly, [box.Name + ".Face2", box.Name + ".Face2"]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(L, W / 2, H / 2), App.Rotation(0, -90, 180))
        _msg("  plc '{}'".format(plc))
        _msg("  targetPlc '{}'".format(targetPlc))
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 4".format(operation))

    def test_solve_assembly(self):
        """Test solving an assembly."""
        operation = "Solve assembly"
        _msg("  Test '{}'".format(operation))

        box = self.assembly.newObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        box.Placement = App.Placement(App.Vector(10, 20, 30), App.Rotation(15, 25, 35))

        box2 = self.assembly.newObject("Part::Box", "Box")
        box2.Length = 10
        box2.Width = 10
        box2.Height = 10
        box2.Placement = App.Placement(App.Vector(40, 50, 60), App.Rotation(45, 55, 65))

        ground = self.jointgroup.newObject("App::FeaturePython", "GroundedJoint")
        JointObject.GroundedJoint(ground, box2)

        joint = self.jointgroup.newObject("App::FeaturePython", "testJoint")
        JointObject.Joint(joint, 0)

        refs = [
            [box2, ["Face6", "Vertex7"]],
            [box, ["Face6", "Vertex7"]],
        ]

        joint.Proxy.setJointConnectors(joint, refs)

        self.assertTrue(box.Placement.isSame(box2.Placement, 1e-6), "'{}'".format(operation))

    def test_rack_pinion_with_slider_offset(self):
        """Rack and pinion joint whose rack slider has a yaw offset, see
        github.com/freecad/freecad/issues/17563"""
        operation = "Rack and pinion with slider offset"
        _msg("  Test '{}'".format(operation))

        ground = self.assembly.newObject("Part::Box", "Ground")
        ground.Length = 200
        ground.Width = 200
        ground.Height = 10

        rack = self.assembly.newObject("Part::Box", "Rack")
        rack.Length = 10
        rack.Width = 100
        rack.Height = 10
        rack.Placement.Base = App.Vector(50, 0, 10)

        pinion = self.assembly.newObject("Part::Cylinder", "Pinion")
        pinion.Radius = 10
        pinion.Height = 10
        pinion.Placement.Base = App.Vector(30, 50, 10)
        self.doc.recompute()

        # Attach to real faces, but specify the JCS explicitly so the test does
        # not depend on OpenCASCADE's orientation of those faces.
        along_rack = App.Rotation(App.Vector(1, 0, 0), -90)
        yaw_offset = App.Rotation(App.Vector(0, 0, 1), -90)

        slider = self.jointgroup.newObject("App::FeaturePython", "Slider")
        JointObject.Joint(slider, JointObject.JointTypes.index("Slider"))
        slider.Detach1 = True
        slider.Detach2 = True
        slider.Reference1 = (ground, ["Face1"])
        slider.Reference2 = (rack, ["Face1"])
        slider.Placement1 = App.Placement(App.Vector(), along_rack)
        slider.Placement2 = App.Placement(App.Vector(), along_rack * yaw_offset)

        rackPinion = self.jointgroup.newObject("App::FeaturePython", "RackPinion")
        JointObject.Joint(rackPinion, JointObject.JointTypes.index("RackPinion"))
        rackPinion.Detach1 = True
        rackPinion.Reference1 = (rack, ["Face1"])
        rackPinion.Reference2 = (pinion, ["Face1"])
        rackPinion.Placement1 = App.Placement(App.Vector(), along_rack)
        rackPinion.Distance = 10

        self.doc.recompute()

        slider_axis = UtilsAssembly.getJcsGlobalPlc(
            slider.Placement2, slider.Reference2
        ).Rotation.multVec(App.Vector(0, 0, 1))
        rack_axis = UtilsAssembly.getJcsGlobalPlc(
            rackPinion.Placement1, rackPinion.Reference1
        ).Rotation.multVec(App.Vector(0, 0, 1))
        self.assertLess(slider_axis.cross(rack_axis).Length, 1e-7)

        # The rack and pinion joint must reach the solver: it is silently dropped when
        # the rack cannot be identified from its slider. Look for its pitchRadius field
        # rather than its type name, which the solver derives from the compiler-specific
        # typeid name and is only readable with MSVC.
        with tempfile.TemporaryDirectory() as temp_dir:
            fileName = os.path.join(temp_dir, "rackPinion.asmt")
            self.assembly.exportAsASMT(fileName)
            with open(fileName) as asmt:
                content = asmt.read()
        self.assertIn(
            "pitchRadius",
            content,
            "'{}' failed - joint not exported; slider state: {}, rack-pinion state: {}; "
            "exported assembly: {}".format(
                operation, slider.State, rackPinion.State, content[-2500:]
            ),
        )

    def _point(self, sub):
        shape = Part.getShape(self.assembly, sub, App.Matrix(), True)
        self.assertGreater(len(shape.Vertexes), 0, sub)
        return shape.Vertexes[0].Point

    def _component(self, full):
        comp, rel = UtilsAssembly.getComponentReference(self.assembly, self.assembly, full)
        self.assertIsNotNone(comp, full)
        return comp, rel

    def _assert_marker(self, full, vertex_rel=None):
        comp, rel = self._component(full)
        if vertex_rel is None:
            vertex_rel = rel
        ref = (comp, [rel, vertex_rel])
        marker = UtilsAssembly.getJcsGlobalPlc(UtilsAssembly.findPlacement(ref), ref)
        gap = (marker.Base - self._point(full)).Length
        self.assertLess(gap, 1e-6, "marker gap {:.4f} mm".format(gap))
        return comp, rel, ref

    def _fixed_to_target(self, target, comp, rel, vertex_rel=None):
        ground = self.jointgroup.newObject("App::FeaturePython", "GroundedJoint")
        JointObject.GroundedJoint(ground, target)
        joint = self.jointgroup.newObject("App::FeaturePython", "FixedJoint")
        JointObject.Joint(joint, 0)
        target_comp, target_rel = self._component(target.Name + ".Vertex1")
        if vertex_rel is None:
            vertex_rel = rel
        joint.Proxy.setJointConnectors(
            joint,
            [(target_comp, [target_rel, target_rel]), (comp, [rel, vertex_rel])],
        )
        self.doc.recompute()
        self.assertEqual(self.assembly.solve(), 0)

    def _box(self, parent, name, length, width, height, placement):
        maker = parent.newObject if hasattr(parent, "newObject") else parent.addObject
        box = maker("Part::Box", name)
        box.Length = length
        box.Width = width
        box.Height = height
        box.Placement = placement
        return box

    def _linear_array(self, linked, occurrences, offset, spacing=False):
        array = self.assembly.newObject("Part::LinkArrayLinear", "Array")
        array.LinkedObject = linked
        array.Occurrences = occurrences
        array.Offset = offset
        array.ShowElement = True
        if spacing:
            array.Mode = "Spacing"
        return array

    def _move(self, obj, placement):
        if hasattr(self.doc, "commitCompatibilityMutation"):

            def mutate():
                obj.Placement = placement
                return True

            committed = self.doc.commitCompatibilityMutation(
                mutate, structural=True, recompute=True
            )
            self.assertTrue(committed["committed"], committed)
        else:
            obj.Placement = placement
        self.doc.recompute()
        self.assertEqual(self.assembly.solve(), 0)

    def _assert_stored(self, true, target, ref, limit, solved_limit=None):
        stored = UtilsAssembly.getJcsGlobalPlc(self.doc.getObject("FixedJoint").Placement2, ref)
        gap = (stored.Base - true).Length
        self.assertLess(gap, limit, "stored gap {:.4f} mm".format(gap))
        solved_limit = limit if solved_limit is None else solved_limit
        solved = (true - self._point(target.Name + ".Vertex1")).Length
        self.assertLess(solved, solved_limit, "solved gap {:.4f} mm".format(solved))

    def _circle(self, face_sub):
        edge = Part.getShape(self.assembly, face_sub, App.Matrix(), True).getElement("Edge1")
        self.assertEqual(edge.Curve.TypeId, "Part::GeomCircle", face_sub)
        return edge.Curve.Location

    def _axes(self, face_full):
        dot = face_full.rfind(".")
        shape = Part.getShape(self.assembly, face_full[: dot + 1], App.Matrix(), True)
        face = shape.getElement(face_full[dot + 1 :])
        edge = face.getElement("Edge2")
        self.assertEqual(edge.Curve.TypeId, "Part::GeomBSplineCurve", face_full)
        axes = [
            other.Surface
            for other in shape.Faces
            if other.Surface.TypeId == "Part::GeomCylinder"
            and any(
                item.Curve.TypeId == "Part::GeomBSplineCurve"
                and item.CenterOfGravity == edge.CenterOfGravity
                and item.Length == edge.Length
                for item in other.Edges
            )
        ]
        line1 = Part.Line(axes[0].Center, axes[0].Center + axes[0].Axis)
        line2 = Part.Line(axes[1].Center, axes[1].Center + axes[1].Axis)
        point = line1.intersect(line2, Part.Precision.confusion())[0]
        return App.Vector(point.X, point.Y, point.Z)

    def test_link_array_fixed_joint_uses_inner_source_placement_once(self):
        _msg("  Test 'Link array joint matches inner source placement'")
        sub = self.doc.addObject("Assembly::AssemblyObject", "Sub")
        sub.newObject("Assembly::JointGroup", "SubJoints")
        source = self._box(
            sub,
            "Source",
            10,
            20,
            30,
            App.Placement(App.Vector(0, 40, 15), App.Rotation(App.Vector(0, 0, 1), 90)),
        )
        array = self._linear_array(sub, 2, 100)
        target = self._box(
            self.assembly,
            "Target",
            10,
            20,
            30,
            App.Placement(App.Vector(0, 200, 0), App.Rotation()),
        )
        plain = self.assembly.newObject("App::Link", "Plain")
        plain.LinkedObject = sub
        plain.Placement = App.Placement(App.Vector(0, -80, 0), App.Rotation())
        self.doc.recompute()

        element = array.ElementList[1]
        self.assertGreater(element.Placement.Base.x, 50)
        full = "{}.{}.Sub.Source.Vertex1".format(array.Name, element.Name)
        comp, rel, _ref = self._assert_marker(full)
        edge_comp, edge_rel = self._component(
            "{}.{}.Sub.Source.Edge1".format(array.Name, element.Name)
        )
        edge_ref = (edge_comp, [edge_rel, rel])
        edge_marker = UtilsAssembly.getJcsGlobalPlc(UtilsAssembly.findPlacement(edge_ref), edge_ref)
        edge_gap = (edge_marker.Base - self._point(full)).Length
        self.assertLess(edge_gap, 1e-6, "marker gap {:.4f} mm".format(edge_gap))
        plain_full = "Plain.Sub.Source.Vertex1"
        _plain_comp, _plain_rel, plain_ref = self._assert_marker(plain_full)
        self.assertEqual(UtilsAssembly.getObject(plain_ref).Name, source.Name)

        self._fixed_to_target(target, comp, rel)
        solved = (self._point(full) - self._point(target.Name + ".Vertex1")).Length
        self.assertLess(solved, 1e-5, "solved gap {:.4f} mm".format(solved))

    def test_link_array_fixed_joint_ignores_source_object_placement(self):
        _msg("  Test 'Link array joint ignores source object placement'")
        source = self._box(
            self.doc, "Source", 10, 20, 30, App.Placement(App.Vector(0, 80, 15), App.Rotation())
        )
        array = self._linear_array(source, 2, 100)
        target = self._box(
            self.assembly,
            "Target",
            10,
            20,
            30,
            App.Placement(App.Vector(0, 200, 0), App.Rotation()),
        )
        plain = self.assembly.newObject("App::Link", "Plain")
        plain.LinkedObject = source
        self.doc.recompute()

        element = array.ElementList[1]
        full = "{}.{}.{}.Vertex1".format(array.Name, element.Name, source.Name)
        comp, rel, _ref = self._assert_marker(full)
        self._assert_marker(plain.Name + ".Vertex1")

        self._fixed_to_target(target, comp, rel)
        solved = (self._point(full) - self._point(target.Name + ".Vertex1")).Length
        self.assertLess(solved, 1e-5, "solved gap {:.4f} mm".format(solved))

    def test_link_array_joint_follows_moved_inner_body(self):
        _msg("  Test 'Link array joint follows a moved inner body'")
        sub = self.doc.addObject("Assembly::AssemblyObject", "Sub")
        sub.newObject("Assembly::JointGroup", "SubJoints")
        inner = self._box(
            sub, "InnerBox", 10, 10, 10, App.Placement(App.Vector(0, 30, 0), App.Rotation())
        )
        array = self._linear_array(sub, 3, 120, spacing=True)
        target = self._box(self.assembly, "Target", 20, 20, 20, App.Placement())
        self.doc.recompute()

        full = "{}.{}.Sub.InnerBox.Vertex1".format(array.Name, array.ElementList[1].Name)
        comp, rel, ref = self._assert_marker(full)
        self._fixed_to_target(target, comp, rel)
        self._assert_stored(self._point(full), target, ref, 1e-6)

        self._move(inner, App.Placement(App.Vector(0, 70, 0), App.Rotation()))
        self._assert_stored(self._point(full), target, ref, 1e-6)

    def test_link_array_joint_follows_moved_inner_cylinder_bottom(self):
        _msg("  Test 'Link array joint follows a moved cylinder bottom circle'")
        sub = self.doc.addObject("Assembly::AssemblyObject", "Sub")
        sub.newObject("Assembly::JointGroup", "SubJoints")
        inner = sub.newObject("Part::Cylinder", "InnerCyl")
        inner.Radius = 5
        inner.Height = 10
        inner.Placement = App.Placement(App.Vector(0, 30, 0), App.Rotation())
        array = self._linear_array(sub, 3, 120, spacing=True)
        target = self._box(self.assembly, "Target", 20, 20, 20, App.Placement())
        self.doc.recompute()

        bottom_index = None
        bottom_face = None
        for index, face in enumerate(inner.Shape.Faces, 1):
            if face.Surface.TypeId != "Part::GeomPlane":
                continue
            if bottom_face is None or face.CenterOfGravity.z < bottom_face.CenterOfGravity.z:
                bottom_face = face
                bottom_index = index
        self.assertEqual(len(bottom_face.Edges), 1)
        self.assertEqual(bottom_face.getElement("Edge1").Curve.TypeId, "Part::GeomCircle")
        self.assertNotEqual(inner.Shape.getElement("Edge1").Curve.TypeId, "Part::GeomCircle")

        element = array.ElementList[1]
        face_full = "{}.{}.Sub.InnerCyl.Face{}".format(array.Name, element.Name, bottom_index)
        comp, face_rel = self._component(face_full)
        _edge_comp, edge_rel = self._component(
            "{}.{}.Sub.InnerCyl.Edge1".format(array.Name, element.Name)
        )
        self.assertTrue(face_rel.endswith("Face{}".format(bottom_index)), face_rel)
        self.assertTrue(edge_rel.endswith("Edge1"), edge_rel)
        ref = (comp, [face_rel, edge_rel])
        marker = UtilsAssembly.getJcsGlobalPlc(UtilsAssembly.findPlacement(ref), ref)
        gap = (marker.Base - self._circle(face_full)).Length
        self.assertLess(gap, 1e-6, "marker gap {:.4f} mm".format(gap))

        self._fixed_to_target(target, comp, face_rel, edge_rel)
        self._assert_stored(self._circle(face_full), target, ref, 1e-6)
        self._move(inner, App.Placement(App.Vector(0, 70, 0), App.Rotation()))
        self._assert_stored(self._circle(face_full), target, ref, 1e-6)

    def test_link_array_joint_follows_moved_cylinder_intersection(self):
        _msg("  Test 'Link array joint follows a moved cylinder intersection'")
        sub = self.doc.addObject("Assembly::AssemblyObject", "Sub")
        sub.newObject("Assembly::JointGroup", "SubJoints")
        inner = sub.newObject("Part::Feature", "Fusion")
        inner.Shape = Part.makeCylinder(5, 40).fuse(
            Part.makeCylinder(8, 40, App.Vector(-20, 0, 20), App.Vector(1, 0, 0))
        )
        inner.Placement = App.Placement(App.Vector(0, 30, 0), App.Rotation())
        array = self._linear_array(sub, 2, 100)
        target = self._box(self.assembly, "Target", 20, 20, 20, App.Placement())
        self.doc.recompute()

        full = "{}.{}.Sub.Fusion.Face1".format(array.Name, array.ElementList[1].Name)
        comp, rel = self._component(full)
        _edge_comp, edge_rel = self._component(full.replace("Face1", "Edge2"))
        ref = (comp, [rel, edge_rel])
        marker = UtilsAssembly.getJcsGlobalPlc(UtilsAssembly.findPlacement(ref), ref)
        gap = (marker.Base - self._axes(full)).Length
        self.assertLess(gap, 1e-6, "marker gap {:.4f} mm".format(gap))
        self._fixed_to_target(target, comp, rel, edge_rel)
        self._assert_stored(self._axes(full), target, ref, 1e-6)
        self._move(inner, App.Placement(App.Vector(0, 70, 0), App.Rotation()))
        self._assert_stored(self._axes(full), target, ref, 1e-6)

    def _check_pattern_array_joint(self, kind):
        _msg("  Test 'Link array joint on a {} pattern'".format(kind.lower()))
        sub = self.doc.addObject("Assembly::AssemblyObject", "Sub")
        sub.newObject("Assembly::JointGroup", "SubJoints")
        rotation = App.Rotation(App.Vector(0, 0, 1), 90)
        inner = self._box(
            sub, "InnerBox", 10, 10, 10, App.Placement(App.Vector(0, 30, 15), rotation)
        )
        array = self.assembly.newObject("Part::LinkArray" + kind, "Array")
        array.LinkedObject = sub
        array.ShowElement = True
        if kind == "Circular":
            array.Axis = (None, ["Z_Axis"])
            array.BinormalAxis = (None, ["Y_Axis"])
        elif kind in ("Path", "Point"):
            pattern = self.doc.addObject("Part::Feature", "Pattern")
            pattern.Shape = Part.makePolygon([App.Vector(), App.Vector(120, 0, 0)])
            if kind == "Path":
                array.Path = (pattern, ["Edge1"])
                array.Count = 2
            else:
                array.PointObject = pattern
        target = self._box(self.assembly, "Target", 20, 20, 20, App.Placement())
        self.doc.recompute()

        self.assertGreaterEqual(len(array.ElementList), 2)
        element = array.ElementList[1]
        self.assertFalse(element.Placement.isIdentity())
        full = "{}.{}.Sub.InnerBox.Vertex1".format(array.Name, element.Name)
        comp, rel, ref = self._assert_marker(full)
        self._fixed_to_target(target, comp, rel)
        solved = (self._point(full) - self._point(target.Name + ".Vertex1")).Length
        self.assertLess(solved, 1e-5, "solved gap {:.4f} mm".format(solved))
        self._move(inner, App.Placement(App.Vector(0, 70, 15), rotation))
        self._assert_stored(self._point(full), target, ref, 1e-6, 1e-5)

    def test_link_array_joint_on_polar_pattern(self):
        self._check_pattern_array_joint("Polar")

    def test_link_array_joint_on_circular_pattern(self):
        self._check_pattern_array_joint("Circular")

    def test_link_array_joint_on_path_pattern(self):
        self._check_pattern_array_joint("Path")

    def test_link_array_joint_on_point_pattern(self):
        self._check_pattern_array_joint("Point")
