// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <cstdio>
#include <limits>
#include <stdexcept>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Pln.hxx>

#include <Base/Exception.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Sketcher/App/ExternalGeometryFacade.h>
#include "SketcherTestHelpers.h"

namespace
{
using namespace Sketcher;

TopoDS_Edge line(double y, bool reversed = false)
{
    const gp_Pnt a(0, y, 0), b(10, y, 0);
    return BRepBuilderAPI_MakeEdge(reversed ? b : a, reversed ? a : b).Edge();
}
TopoDS_Edge circle(double x)
{
    return BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(gp_Pnt(x, 0, 0), gp_Dir(0, 0, 1)), 3)).Edge();
}
TopoDS_Edge arc(double x, bool reversed = false)
{
    return BRepBuilderAPI_MakeEdge(
               gp_Circ(gp_Ax2(gp_Pnt(x, 0, 0), gp_Dir(0, 0, reversed ? -1 : 1)), 3),
               0.2,
               2.0
    )
        .Edge();
}

class SketchObjectReattach: public SketchObjectTest
{
protected:
    App::Document* doc()
    {
        return getObject()->getDocument();
    }
    Part::Feature* source(const std::vector<TopoDS_Shape>& shapes, const char* name = "Source")
    {
        auto* object = static_cast<Part::Feature*>(doc()->addObject("Part::Feature", name));
        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);
        for (const auto& shape : shapes) {
            builder.Add(compound, shape);
        }
        object->Shape.setValue(compound);
        return object;
    }
    Part::Feature* setup(const std::vector<TopoDS_Shape>& shapes)
    {
        doc()->setTransactionMode(1);
        auto* object = source(shapes);
        EXPECT_EQ(getObject()->addExternal(object, "Edge1"), 0);
        EXPECT_EQ(getObject()->addExternal(object, "Edge2"), 1);
        clean();
        return object;
    }
    void clean()
    {
        doc()->recompute();
        doc()->clearUndos();
        doc()->purgeTouched();
    }
    long id(int index)
    {
        return ExternalGeometryFacade::getFacade(getObject()->getGeometry(index))->getId();
    }
    double x(int index = -3)
    {
        return getObject()->getGeometry<Part::GeomCircle>(index)->getCenter().x;
    }
    void dependentCircle(bool signedDistance = false)
    {
        auto* sketch = getObject();
        Part::GeomCircle geometry;
        geometry.setCenter(Base::Vector3d(signedDistance ? 5 : 0, signedDistance ? 2 : 0, 0));
        geometry.setRadius(1);
        sketch->addGeometry(&geometry);
        auto radius = std::make_unique<Constraint>();
        radius->Type = Radius;
        radius->setGeoId(0, 0);
        radius->setValue(1);
        radius->Name = "AuthoredRadius";
        sketch->addConstraint(std::move(radius));
        auto relation = std::make_unique<Constraint>();
        relation->Type = signedDistance ? Distance : Coincident;
        relation->setGeoId(0, 0);
        relation->setPosId(0, PointPos::mid);
        relation->setGeoId(1, -3);
        if (!signedDistance) {
            relation->setPosId(1, PointPos::mid);
        }
        relation->setValue(signedDistance ? 2 : 0);
        sketch->addConstraint(std::move(relation));
        sketch->setExpression(
            sketch->Constraints.createPath(0),
            std::shared_ptr<App::Expression>(App::Expression::parse(sketch, "1 mm"))
        );
    }
    class ConstraintSnapshot
    {
    public:
        explicit ConstraintSnapshot(SketchObject* sketch)
        {
            for (const auto* constraint : sketch->Constraints.getValues()) {
                values.emplace_back(constraint->clone());
            }
        }
        void unchanged(SketchObject* sketch) const
        {
            ASSERT_EQ(sketch->Constraints.getSize(), values.size());
            for (std::size_t i = 0; i < values.size(); ++i) {
                const auto* actual = sketch->Constraints[i];
                const auto* expected = values[i].get();
                EXPECT_EQ(actual->Type, expected->Type);
                EXPECT_EQ(actual->AlignmentType, expected->AlignmentType);
                EXPECT_EQ(
                    actual->Orientation.toUnderlyingType(),
                    expected->Orientation.toUnderlyingType()
                );
                EXPECT_EQ(actual->Name, expected->Name);
                EXPECT_EQ(actual->MetaData, expected->MetaData);
                EXPECT_EQ(actual->getValue(), expected->getValue());
                EXPECT_EQ(actual->isDriving, expected->isDriving);
                EXPECT_EQ(actual->isActive, expected->isActive);
                EXPECT_EQ(actual->isInVirtualSpace, expected->isInVirtualSpace);
                EXPECT_EQ(actual->isVisible, expected->isVisible);
                EXPECT_EQ(actual->InternalAlignmentIndex, expected->InternalAlignmentIndex);
                EXPECT_EQ(actual->LabelDistance, expected->LabelDistance);
                EXPECT_EQ(actual->LabelPosition, expected->LabelPosition);
                EXPECT_EQ(actual->getText(), expected->getText());
                EXPECT_EQ(actual->getFont(), expected->getFont());
                EXPECT_EQ(actual->getIsTextHeight(), expected->getIsTextHeight());
                ASSERT_EQ(actual->getElementsSize(), expected->getElementsSize());
                for (std::size_t j = 0; j < actual->getElementsSize(); ++j) {
                    EXPECT_EQ(actual->getGeoId(j), expected->getGeoId(j));
                    EXPECT_EQ(actual->getPosId(j), expected->getPosId(j));
                }
            }
        }

    private:
        std::vector<std::unique_ptr<Constraint>> values;
    };
    struct Snapshot
    {
        explicit Snapshot(SketchObject* sketch)
            : objects(sketch->ExternalGeometry.getValues())
            , subs(sketch->ExternalGeometry.getSubValues())
            , types(sketch->ExternalTypes.getValues())
            , constraints(sketch->Constraints.getValues())
            , geometry(sketch->Geometry.getValues())
            , external(sketch->ExternalGeo.getValues())
            , touched(sketch->isTouched())
            , propertyTouched(sketch->ExternalGeometry.isTouched())
            , docTouched(sketch->getDocument()->isTouched())
            , externalTouched(sketch->ExternalGeo.isTouched())
            , constraintsTouched(sketch->Constraints.isTouched())
            , undos(sketch->getDocument()->getAvailableUndos())
            , redos(sketch->getDocument()->getAvailableRedos())
            , pending(sketch->getDocument()->hasPendingTransaction())
            , booked(sketch->getDocument()->getBookedTransactionID())
            , label(sketch->Label.getValue())
            , constraintValues(sketch)
        {}
        void unchanged(SketchObject* sketch) const
        {
            EXPECT_EQ(sketch->ExternalGeometry.getValues(), objects);
            EXPECT_EQ(sketch->ExternalGeometry.getSubValues(), subs);
            EXPECT_EQ(sketch->ExternalTypes.getValues(), types);
            EXPECT_EQ(sketch->Constraints.getValues(), constraints);
            EXPECT_EQ(sketch->Geometry.getValues(), geometry);
            EXPECT_EQ(sketch->ExternalGeo.getValues(), external);
            EXPECT_EQ(sketch->isTouched(), touched);
            EXPECT_EQ(sketch->ExternalGeometry.isTouched(), propertyTouched);
            EXPECT_EQ(sketch->getDocument()->isTouched(), docTouched);
            EXPECT_EQ(sketch->ExternalGeo.isTouched(), externalTouched);
            EXPECT_EQ(sketch->Constraints.isTouched(), constraintsTouched);
            EXPECT_EQ(sketch->getDocument()->getAvailableUndos(), undos);
            EXPECT_EQ(sketch->getDocument()->getAvailableRedos(), redos);
            EXPECT_EQ(sketch->getDocument()->hasPendingTransaction(), pending);
            EXPECT_EQ(sketch->getDocument()->getBookedTransactionID(), booked);
            EXPECT_EQ(std::string(sketch->Label.getValue()), label);
            constraintValues.unchanged(sketch);
        }
        std::vector<App::DocumentObject*> objects;
        std::vector<std::string> subs;
        std::vector<long> types;
        std::vector<Constraint*> constraints;
        std::vector<Part::Geometry*> geometry, external;
        bool touched, propertyTouched, docTouched, externalTouched, constraintsTouched;
        int undos, redos;
        bool pending;
        int booked;
        std::string label;
        ConstraintSnapshot constraintValues;
    };
};

TEST_F(SketchObjectReattach, CirclesPreserveIdentityOrderModesAndConstraints)
{
    auto* src = setup({circle(0), circle(20), circle(40)});
    auto* sketch = getObject();
    dependentCircle();
    clean();
    const long first = id(-3), second = id(-4);
    const auto types = sketch->ExternalTypes.getValues();
    const auto constraints = sketch->Constraints.getValues();
    ConstraintSnapshot constraintValues(sketch);
    const auto internal = sketch->Geometry.getValues();
    const auto orientation = constraints[1]->Orientation.toUnderlyingType();
    const auto expression = sketch->getExpression(sketch->Constraints.createPath(0)).expression;
    int events = 0;
    auto connection = sketch->signalElementsChanged.connect([&]() { ++events; });
    sketch->reattachExternal(-3, src, "Edge3");
    connection.disconnect();
    EXPECT_GT(events, 0);
    EXPECT_EQ(sketch->ExternalGeometry.getSubValues(), (std::vector<std::string> {"Edge3", "Edge2"}));
    EXPECT_EQ(sketch->ExternalTypes.getValues(), types);
    EXPECT_EQ(sketch->Constraints.getValues(), constraints);
    constraintValues.unchanged(sketch);
    EXPECT_EQ(sketch->Geometry.getValues(), internal);
    EXPECT_EQ(constraints[1]->Orientation.toUnderlyingType(), orientation);
    EXPECT_EQ(sketch->getExpression(sketch->Constraints.createPath(0)).expression, expression);
    EXPECT_EQ(id(-3), first);
    EXPECT_EQ(id(-4), second);
    EXPECT_NEAR(x(), 40, 1e-9);
    EXPECT_NEAR(x(-4), 20, 1e-9);
    EXPECT_EQ(sketch->getRelatedGeometry(-3), std::vector<int> {-3});
    EXPECT_EQ(sketch->solve(), SketchSolveStatus::Success);
    // Fully constrained centre-coincidence movement also fails with legacy attachExternal
    // on this branch. Its solver qualification is tracked in T004; here assert the
    // unchanged native constraint bindings and successful explicit solve.
    for (int i = 0; i < 2; ++i) {
        sketch->touch();
        doc()->recompute();
        EXPECT_EQ(id(-3), first);
        EXPECT_EQ(id(-4), second);
        EXPECT_EQ(sketch->Constraints.getValues()[1]->Orientation.toUnderlyingType(), orientation);
        EXPECT_EQ(sketch->ExternalGeometry.getSubValues()[0], "Edge3");
    }
}

TEST_F(SketchObjectReattach, CircleCentreDistanceFollowsNewProjection)
{
    auto* src = setup({circle(0), circle(20), circle(40)});
    auto* sketch = getObject();
    Part::GeomCircle geometry;
    geometry.setCenter(Base::Vector3d(2, 1, 0));
    geometry.setRadius(1);
    sketch->addGeometry(&geometry);
    auto distance = std::make_unique<Constraint>();
    distance->Type = DistanceX;
    distance->setGeoId(0, -3);
    distance->setPosId(0, PointPos::mid);
    distance->setGeoId(1, 0);
    distance->setPosId(1, PointPos::mid);
    distance->setValue(2);
    sketch->addConstraint(std::move(distance));
    clean();
    const auto orientation = sketch->Constraints[0]->Orientation.toUnderlyingType();
    sketch->reattachExternal(-3, src, "Edge3");
    EXPECT_EQ(sketch->Constraints[0]->Orientation.toUnderlyingType(), orientation);
    EXPECT_EQ(sketch->solve(), SketchSolveStatus::Success);
    EXPECT_NEAR(sketch->getGeometry<Part::GeomCircle>(0)->getCenter().x, 42, 1e-7);
    sketch->touch();
    doc()->recompute();
    EXPECT_NEAR(sketch->getGeometry<Part::GeomCircle>(0)->getCenter().x, 42, 1e-7);
}

TEST_F(SketchObjectReattach, PreservesAuthoredExtensionsAtReturn)
{
    auto* src = setup({circle(0), circle(20), circle(40)});
    auto* sketch = getObject();
    std::unique_ptr<Part::Geometry> authored(sketch->getGeometry(-3)->clone());
    ExternalGeometryFacade::getFacade(authored.get())->setFlag(ExternalGeometryExtension::Defining);
    GeometryFacade::setConstruction(authored.get(), false);
    sketch->setGeometry(-3, authored.get());
    const auto flags = ExternalGeometryFacade::getFacade(sketch->getGeometry(-3))->getFlags();
    const auto first = id(-3);
    sketch->reattachExternal(-3, src, "Edge3");
    EXPECT_EQ(ExternalGeometryFacade::getFacade(sketch->getGeometry(-3))->getFlags(), flags);
    EXPECT_FALSE(GeometryFacade::getConstruction(sketch->getGeometry(-3)));
    EXPECT_EQ(id(-3), first);
    EXPECT_NEAR(x(), 40, 1e-9);
}

TEST_F(SketchObjectReattach, SignedLineKeepsOrientationThroughRecompute)
{
    auto* src = setup({line(0), line(20), line(5), line(5, true)});
    auto* sketch = getObject();
    dependentCircle(true);
    clean();
    const auto orientation = sketch->Constraints[1]->Orientation.toUnderlyingType();
    const long first = id(-3);
    sketch->reattachExternal(-3, src, "Edge3");
    EXPECT_EQ(sketch->Constraints[1]->Orientation.toUnderlyingType(), orientation);
    EXPECT_EQ(sketch->solve(), SketchSolveStatus::Success);
    EXPECT_NEAR(sketch->getGeometry<Part::GeomCircle>(0)->getCenter().y, 7, 1e-7);
    for (int i = 0; i < 2; ++i) {
        sketch->touch();
        doc()->recompute();
        EXPECT_EQ(sketch->Constraints[1]->Orientation.toUnderlyingType(), orientation);
        EXPECT_EQ(id(-3), first);
        EXPECT_NEAR(sketch->getGeometry<Part::GeomCircle>(0)->getCenter().y, 7, 1e-7);
    }
    Snapshot before(sketch);
    EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge4"), Base::ValueError);
    before.unchanged(sketch);
}

TEST_F(SketchObjectReattach, ArcKeepsParameterizedEndpointRoles)
{
    auto* src = setup({arc(0), arc(20), arc(40), arc(40, true)});
    auto* sketch = getObject();
    const long first = id(-3), second = id(-4);
    const auto* old = sketch->getGeometry<Part::GeomArcOfCircle>(-3);
    const auto start = old->getStartPoint(), end = old->getEndPoint();
    sketch->reattachExternal(-3, src, "Edge3");
    for (int i = 0; i < 3; ++i) {
        const auto* current = sketch->getGeometry<Part::GeomArcOfCircle>(-3);
        EXPECT_NEAR(current->getStartPoint().x, start.x + 40, 1e-9);
        EXPECT_NEAR(current->getEndPoint().x, end.x + 40, 1e-9);
        EXPECT_NEAR(current->getStartPoint().y, start.y, 1e-9);
        EXPECT_NEAR(current->getEndPoint().y, end.y, 1e-9);
        EXPECT_EQ(id(-3), first);
        EXPECT_EQ(id(-4), second);
        sketch->touch();
        doc()->recompute();
    }
    Snapshot before(sketch);
    EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge4"), Base::ValueError);
    before.unchanged(sketch);
}

TEST_F(SketchObjectReattach, RejectionAndNoopPreserveRedoAndCallerEdits)
{
    auto* src = setup({circle(0), circle(20), circle(40)});
    auto* sketch = getObject();
    doc()->openTransaction(std::string("redo marker"));
    sketch->Label.setValue("Undo this label");
    doc()->commitTransaction();
    doc()->undo();
    ASSERT_EQ(doc()->getAvailableRedos(), 1);
    int events = 0;
    auto connection = doc()->signalChangedObject.connect([&](const App::DocumentObject&,
                                                             const App::Property&) { ++events; });
    auto check = [&]() {
        Snapshot before(sketch);
        EXPECT_NO_THROW(sketch->reattachExternal(-3, src, "Edge1"));
        for (int index :
             {0, 1, -1, -2, -5, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
            EXPECT_THROW(sketch->reattachExternal(index, src, "Edge3"), Base::ValueError);
        }
        EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge2"), Base::ValueError);
        EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge99"), Base::ValueError);
        EXPECT_THROW(sketch->reattachExternal(-3, nullptr, "Edge3"), Base::ValueError);
        EXPECT_THROW(sketch->reattachExternal(-3, src, nullptr), Base::ValueError);
        EXPECT_THROW(sketch->reattachExternal(-3, sketch, "Edge1"), Base::ValueError);
        before.unchanged(sketch);
    };
    check();
    EXPECT_EQ(events, 0);
    doc()->openTransaction(std::string("booked caller transaction"));
    check();
    EXPECT_EQ(events, 0);
    sketch->Label.setValue("Earlier caller edit");
    // Earlier changes to the same property remain the caller's responsibility too.
    sketch->reattachExternal(-3, src, "Edge3");
    events = 0;
    Snapshot before(sketch);
    EXPECT_NO_THROW(sketch->reattachExternal(-3, src, "Edge3"));
    EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge2"), Base::ValueError);
    before.unchanged(sketch);
    EXPECT_EQ(events, 0);
    connection.disconnect();
    doc()->abortTransaction();
    EXPECT_NEAR(x(), 0, 1e-9);
    EXPECT_EQ(sketch->solve(), SketchSolveStatus::Success);
}

TEST_F(SketchObjectReattach, UnsupportedStatesRejectBeforeNoop)
{
    auto* src = setup({circle(0), circle(20), circle(40)});
    auto* sketch = getObject();
    for (int flag :
         {ExternalGeometryExtension::Frozen,
          ExternalGeometryExtension::Missing,
          ExternalGeometryExtension::Detached,
          ExternalGeometryExtension::Sync}) {
        // User3 prevents normal Detached handling while preparing this invalid-state fixture.
        std::unique_ptr<Part::Geometry> copy(sketch->getGeometry(-3)->clone());
        ExternalGeometryFacade::getFacade(copy.get())->setFlag(flag);
        sketch->ExternalGeo.setStatus(App::Property::User3, true);
        sketch->setGeometry(-3, copy.get());
        sketch->ExternalGeo.setStatus(App::Property::User3, false);
        Snapshot before(sketch);
        EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge1"), Base::ValueError);
        before.unchanged(sketch);
        ExternalGeometryFacade::getFacade(copy.get())->setFlag(flag, false);
        sketch->setGeometry(-3, copy.get());
    }
    for (long mode : {1L, 2L}) {
        sketch->ExternalTypes.setValues({mode, 0});
        Snapshot before(sketch);
        EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge1"), Base::ValueError);
        before.unchanged(sketch);
    }
    sketch->ExternalTypes.setValues({0, 0});
    sketch->noRecomputes = true;
    Snapshot before(sketch);
    EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge1"), Base::ValueError);
    before.unchanged(sketch);
    sketch->noRecomputes = false;
}

TEST_F(SketchObjectReattach, UnsupportedAndAmbiguousProjectionsAreNonmutating)
{
    auto* src = setup({line(0), line(20), line(5, true)});
    auto* sketch = getObject();
    auto* other = source(
        {circle(0),
         BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(0, 10, 0)).Edge(),
         BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(0, 0, 10)).Edge(),
         BRepBuilderAPI_MakeEdge(gp_Elips(gp_Ax2(gp_Pnt(), gp_Dir(0, 0, 1)), 3, 2)).Edge()},
        "Unsupported"
    );
    clean();
    Snapshot before(sketch);
    EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge3"), Base::ValueError);
    for (const char* sub : {"Edge1", "Edge2", "Edge3", "Edge4", "Vertex1", ""}) {
        EXPECT_THROW(sketch->reattachExternal(-3, other, sub), Base::ValueError);
    }
    before.unchanged(sketch);
}

TEST_F(SketchObjectReattach, ReplacingObjectUpdatesDependenciesAtReturn)
{
    auto* original = setup({circle(0), circle(20), circle(40)});
    auto* sketch = getObject();
    auto* unrelated = source({circle(20)}, "Unrelated");
    auto* destination = source({circle(40)}, "Destination");
    sketch->reattachExternal(-4, unrelated, "Edge1");
    // Deliberately keep a mode vector longer than the link list, as in legacy data.
    sketch->ExternalTypes.setValues({0, 0, 2});
    const auto types = sketch->ExternalTypes.getValues();
    const auto first = id(-3), second = id(-4);
    sketch->reattachExternal(-3, destination, "Edge1");
    const auto& dependencies = sketch->getOutList();
    EXPECT_EQ(std::count(dependencies.begin(), dependencies.end(), destination), 1);
    EXPECT_EQ(std::count(dependencies.begin(), dependencies.end(), original), 0);
    EXPECT_EQ(std::count(dependencies.begin(), dependencies.end(), unrelated), 1);
    EXPECT_EQ(sketch->ExternalTypes.getValues(), types);
    EXPECT_EQ(id(-3), first);
    EXPECT_EQ(id(-4), second);
    doc()->removeObject(original->getNameInDocument());
    doc()->recompute();
    EXPECT_NEAR(x(), 40, 1e-9);
    EXPECT_NEAR(x(-4), 20, 1e-9);
    EXPECT_EQ(sketch->solve(), SketchSolveStatus::Success);
}

TEST_F(SketchObjectReattach, GroupedFaceReferenceIsRejected)
{
    auto* sketch = getObject();
    auto* src = source(
        {BRepBuilderAPI_MakeFace(gp_Pln(gp_Pnt(), gp_Dir(0, 0, 1)), 0, 10, 0, 10).Face(), line(20)}
    );
    ASSERT_EQ(sketch->addExternal(src, "Face1"), 0);
    clean();
    ASSERT_GT(sketch->getRelatedGeometry(-3).size(), 1U);
    Snapshot before(sketch);
    EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge5"), Base::ValueError);
    before.unchanged(sketch);
}

TEST_F(SketchObjectReattach, CrossDocumentAndCyclicSourcesAreRejected)
{
    auto* src = setup({circle(0), circle(20), circle(40)});
    auto* sketch = getObject();
    auto* otherDoc = App::GetApplication().newDocument("OtherReattachDocument");
    auto* foreign = static_cast<Part::Feature*>(otherDoc->addObject("Part::Feature", "Foreign"));
    foreign->Shape.setValue(circle(40));
    auto* cyclic = static_cast<Part::Feature*>(doc()->addObject("Part::Feature", "Cyclic"));
    cyclic->Shape.setValue(circle(40));
    auto* link = static_cast<App::PropertyLink*>(
        cyclic->addDynamicProperty("App::PropertyLink", "Back")
    );
    link->setValue(sketch);
    clean();
    Snapshot before(sketch);
    EXPECT_THROW(sketch->reattachExternal(-3, foreign, "Edge1"), Base::ValueError);
    EXPECT_THROW(sketch->reattachExternal(-3, cyclic, "Edge1"), Base::ValueError);
    // Valid calls still work immediately after rejecting an unrelated graph cycle.
    before.unchanged(sketch);
    EXPECT_NO_THROW(sketch->reattachExternal(-3, src, "Edge3"));
    App::GetApplication().closeDocument(otherDoc->getName());
}

TEST_F(SketchObjectReattach, CallerAbortRecoversApplicationExceptions)
{
    auto* src = setup({circle(0), circle(20), circle(40)});
    auto* sketch = getObject();
    dependentCircle();
    clean();
    const long first = id(-3);
    const std::string label = sketch->Label.getValue();
    for (bool afterProjection : {false, true}) {
        doc()->openTransaction(std::string("caller owns recovery"));
        sketch->Label.setValue("Earlier edit in shared transaction");
        int failures = 0;
        auto connection = sketch->signalElementsChanged.connect([&]() {
            if (sketch->ExternalGeometry.getSubValues()[0] == "Edge3"
                && (!afterProjection || x() > 39)) {
                ++failures;
                throw std::runtime_error("injected apply failure");
            }
        });
        EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge3"), std::runtime_error);
        connection.disconnect();
        EXPECT_EQ(failures, 1);  // no retry
        EXPECT_TRUE(doc()->hasPendingTransaction());
        EXPECT_EQ(std::string(sketch->Label.getValue()), "Earlier edit in shared transaction");
        doc()->abortTransaction();
        doc()->recompute();
        EXPECT_EQ(std::string(sketch->Label.getValue()), label);
        EXPECT_EQ(sketch->ExternalGeometry.getSubValues()[0], "Edge1");
        EXPECT_EQ(id(-3), first);
        EXPECT_NEAR(x(), 0, 1e-9);
        EXPECT_EQ(sketch->solve(), SketchSolveStatus::Success);
        EXPECT_NEAR(sketch->getGeometry<Part::GeomCircle>(0)->getCenter().x, 0, 1e-9);
    }
    Part::GeomLineSegment edit;
    edit.setPoints(Base::Vector3d(0, 10, 0), Base::Vector3d(10, 10, 0));
    EXPECT_EQ(sketch->addGeometry(&edit), 1);
    EXPECT_EQ(sketch->solve(), SketchSolveStatus::Success);
}

TEST_F(SketchObjectReattach, CallerReloadsCheckpointAfterUntransactedApplicationException)
{
    auto* src = setup({circle(0), circle(20), circle(40)});
    auto* sketch = getObject();
    dependentCircle();
    clean();
    const std::string sketchName = sketch->getNameInDocument();
    const auto checkpoint = App::Application::getTempFileName();
    ASSERT_TRUE(doc()->saveAs(checkpoint.c_str()));
    const long first = id(-3);
    auto connection = sketch->signalElementsChanged.connect([&]() {
        if (sketch->ExternalGeometry.getSubValues()[0] == "Edge3") {
            throw std::runtime_error("injected untransacted apply failure");
        }
    });
    EXPECT_THROW(sketch->reattachExternal(-3, src, "Edge3"), std::runtime_error);
    connection.disconnect();
    EXPECT_FALSE(doc()->hasPendingTransaction());
    auto* document = doc();
    const std::string savedPath = document->getFileName();
    document->restore(savedPath.c_str());
    document->recompute();
    auto* restored = static_cast<SketchObject*>(document->getObject(sketchName.c_str()));
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->ExternalGeometry.getSubValues()[0], "Edge1");
    EXPECT_EQ(ExternalGeometryFacade::getFacade(restored->getGeometry(-3))->getId(), first);
    EXPECT_NEAR(restored->getGeometry<Part::GeomCircle>(-3)->getCenter().x, 0, 1e-9);
    EXPECT_EQ(restored->solve(), SketchSolveStatus::Success);
    EXPECT_NEAR(restored->getGeometry<Part::GeomCircle>(0)->getCenter().x, 0, 1e-9);
    Part::GeomLineSegment edit;
    edit.setPoints(Base::Vector3d(0, 10, 0), Base::Vector3d(10, 10, 0));
    EXPECT_EQ(restored->addGeometry(&edit), 1);
    EXPECT_EQ(restored->solve(), SketchSolveStatus::Success);
    std::remove(savedPath.c_str());
}
}  // namespace
