// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <atomic>
#include <barrier>
#include <cmath>
#include <thread>
#include <vector>

#include <BRepBuilderAPI_Transform.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>

#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Mod/Part/Gui/ViewProviderExt.h>

class ViewProviderPartExtMeshTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        SoDB::init();
        if (PartGui::SoBrepFaceSet::getClassTypeId().isBad()) {
            PartGui::SoBrepFaceSet::initClass();
        }
        if (PartGui::SoBrepEdgeSet::getClassTypeId().isBad()) {
            PartGui::SoBrepEdgeSet::initClass();
        }
        if (PartGui::SoBrepPointSet::getClassTypeId().isBad()) {
            PartGui::SoBrepPointSet::initClass();
        }
    }

    void SetUp() override
    {
        coordinates = new SoCoordinate3;
        normals = new SoNormal;
        faces = new PartGui::SoBrepFaceSet;
        edges = new PartGui::SoBrepEdgeSet;
        points = new PartGui::SoBrepPointSet;
        coordinates->ref();
        normals->ref();
        faces->ref();
        edges->ref();
        points->ref();
    }

    void TearDown() override
    {
        points->unref();
        edges->unref();
        faces->unref();
        normals->unref();
        coordinates->unref();
    }

    static std::vector<Handle(Poly_Triangulation)> triangulations(const TopoDS_Shape& shape)
    {
        std::vector<Handle(Poly_Triangulation)> result;
        for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next()) {
            TopLoc_Location location;
            result.push_back(BRep_Tool::Triangulation(TopoDS::Face(it.Current()), location));
        }
        return result;
    }

    void render(const TopoDS_Shape& shape)
    {
        PartGui::ViewProviderPartExt::setupCoinGeometry(
            shape,
            coordinates,
            faces,
            normals,
            edges,
            points,
            0.1,
            20.0
        );
        EXPECT_GT(coordinates->point.getNum(), 0);
        EXPECT_GT(normals->vector.getNum(), 0);
        EXPECT_GT(faces->coordIndex.getNum(), 0);
        EXPECT_GT(edges->coordIndex.getNum(), 0);
    }

    SoCoordinate3* coordinates {};
    SoNormal* normals {};
    PartGui::SoBrepFaceSet* faces {};
    PartGui::SoBrepEdgeSet* edges {};
    PartGui::SoBrepPointSet* points {};
};

TEST_F(ViewProviderPartExtMeshTest, UnmeshedModelRemainsUnmeshedAfterRendering)
{
    const TopoDS_Shape shape = BRepPrimAPI_MakeCylinder(5.0, 10.0).Shape();
    const auto original = triangulations(shape);
    ASSERT_FALSE(original.empty());
    for (const auto& mesh : original) {
        ASSERT_TRUE(mesh.IsNull());
    }

    render(shape);

    for (const auto& mesh : triangulations(shape)) {
        EXPECT_TRUE(mesh.IsNull());
    }
}

TEST_F(ViewProviderPartExtMeshTest, ExistingModelTriangulationsSurviveRendering)
{
    const TopoDS_Shape shape = BRepPrimAPI_MakeCylinder(5.0, 10.0).Shape();
    BRepMesh_IncrementalMesh mesher(shape, 0.2, false, 0.5, false);
    const auto original = triangulations(shape);
    ASSERT_FALSE(original.empty());
    for (const auto& mesh : original) {
        ASSERT_FALSE(mesh.IsNull());
    }

    render(shape);

    const auto after = triangulations(shape);
    ASSERT_EQ(after.size(), original.size());
    for (std::size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(after[i].get(), original[i].get());
    }
}

TEST_F(ViewProviderPartExtMeshTest, LocatedModelKeepsLocalDisplayCoordinates)
{
    gp_Trsf placement;
    placement.SetTranslation(gp_Vec(11.0, 17.0, 23.0));
    const TopoDS_Shape shape = BRepPrimAPI_MakeCylinder(5.0, 10.0).Shape().Moved(placement);
    const auto originalLocation = shape.Location();

    render(shape);

    EXPECT_EQ(shape.Location(), originalLocation);
    bool bottom = false;
    bool top = false;
    for (int i = 0; i < coordinates->point.getNum(); ++i) {
        const auto& point = coordinates->point[i];
        EXPECT_LE(std::abs(point[0]), 5.001);
        EXPECT_LE(std::abs(point[1]), 5.001);
        EXPECT_GE(point[2], -0.001);
        EXPECT_LE(point[2], 10.001);
        bottom = bottom || std::abs(point[2]) < 0.001;
        top = top || std::abs(point[2] - 10.0) < 0.001;
    }
    EXPECT_TRUE(bottom);
    EXPECT_TRUE(top);
}

TEST_F(ViewProviderPartExtMeshTest, RenderingDoesNotDisturbConcurrentScaling)
{
    const TopoDS_Shape shape = BRepPrimAPI_MakeCylinder(5.0, 10.0).Shape();
    BRepMesh_IncrementalMesh mesher(shape, 0.2, false, 0.5, false);
    const auto original = triangulations(shape);
    GProp_GProps sourceProperties;
    BRepGProp::VolumeProperties(shape, sourceProperties);
    const double expectedVolume = sourceProperties.Mass() * 8.0;

    std::barrier start(2);
    std::atomic<bool> valid {true};
    std::atomic<int> scaledCount {0};
    std::jthread worker([&](std::stop_token stop) {
        start.arrive_and_wait();
        try {
            gp_Trsf scale;
            scale.SetScale(gp_Pnt(0.0, 0.0, 0.0), 2.0);
            do {
                BRepBuilderAPI_Transform transformed(shape, scale, true);
                GProp_GProps properties;
                BRepGProp::VolumeProperties(transformed.Shape(), properties);
                if (!std::isfinite(properties.Mass())
                    || std::abs(properties.Mass() - expectedVolume) > expectedVolume * 1e-8) {
                    valid = false;
                }
                ++scaledCount;
            } while (!stop.stop_requested());
        }
        catch (...) {
            valid = false;
        }
    });

    start.arrive_and_wait();
    for (int i = 0; i < 20; ++i) {
        render(shape);
    }
    worker.request_stop();
    worker.join();

    EXPECT_TRUE(valid.load());
    EXPECT_GT(scaledCount.load(), 0);
    const auto after = triangulations(shape);
    ASSERT_EQ(after.size(), original.size());
    for (std::size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(after[i].get(), original[i].get());
    }
}

TEST_F(ViewProviderPartExtMeshTest, DisplayFaceIndicesStillReferToModelFaces)
{
    const TopoDS_Shape shape = BRepPrimAPI_MakeCylinder(5.0, 10.0).Shape();
    TopTools_IndexedMapOfShape modelFaces;
    TopExp::MapShapes(shape, TopAbs_FACE, modelFaces);

    render(shape);

    ASSERT_EQ(faces->partIndex.getNum(), modelFaces.Extent());
    int triangleOffset = 0;
    for (int i = 0; i < modelFaces.Extent(); ++i) {
        const int triangles = faces->partIndex[i];
        ASSERT_GT(triangles, 0);
        TopLoc_Location location;
        const auto surface = BRep_Tool::Surface(TopoDS::Face(modelFaces(i + 1)), location);
        ASSERT_FALSE(surface.IsNull());
        // Sample all corners of the first triangle for each displayed FaceN.
        // The cylinder's side and two caps have different supporting surfaces.
        for (int corner = 0; corner < 3; ++corner) {
            const int vertex = faces->coordIndex[triangleOffset * 4 + corner];
            ASSERT_GE(vertex, 0);
            ASSERT_LT(vertex, coordinates->point.getNum());
            const auto& coordinate = coordinates->point[vertex];
            gp_Pnt point(coordinate[0], coordinate[1], coordinate[2]);
            point.Transform(location.Inverted().Transformation());
            GeomAPI_ProjectPointOnSurf projection(point, surface);
            ASSERT_GT(projection.NbPoints(), 0);
            EXPECT_NEAR(projection.LowerDistance(), 0.0, 1e-5);
        }
        triangleOffset += triangles;
    }
}
