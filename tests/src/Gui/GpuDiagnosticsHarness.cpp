// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <tuple>
#include <string_view>
#include <vector>

#ifdef FREECAD_GPU_DIAGNOSTICS_HAS_PART
# include <Base/Tools.h>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRep_Tool.hxx>
# include <BRepTools.hxx>
# include <IMeshTools_Parameters.hxx>
# include <Precision.hxx>
# include <Mod/Part/App/PartFeature.h>
# include <Mod/Part/App/Tools.h>
# include <Poly_Triangulation.hxx>
# include <TopAbs.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopLoc_Location.hxx>
# include <TopoDS.hxx>
# include <App/Material.h>
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QDialog>
#include <QDir>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>

#include <App/Application.h>
#include <App/Document.h>

#include <Base/Interpreter.h>

#include <Gui/Application.h>
#include <Gui/Camera.h>
#include <Gui/Document.h>
#include <Gui/GpuDiagnostics.h>
#include <Gui/Multisample.h>
#include <Gui/NaviCube.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>

#ifdef FREECAD_GPU_DIAGNOSTICS_HAS_PART
# include <Inventor/SbName.h>
# include <Inventor/SbVec3f.h>
# include <Inventor/SoType.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/nodes/SoLightModel.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoPolygonOffset.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoDirectionalLight.h>

# include <Mod/Part/Gui/SoBrepEdgeSet.h>
# include <Gui/ViewProviderGeometryObject.h>
#endif

namespace
{

constexpr int defaultAutoExitMs = 2000;
constexpr int maximumAutoExitMs = 60000;
constexpr int shutdownGraceMs = 250;
constexpr int fixtureSettleMs = 250;
constexpr long long nanosecondsPerMicrosecond = 1000LL;
constexpr long long approximateBytesPerTriangle = 48LL;
constexpr long long approximateBytesPerNode = 32LL;
constexpr auto documentName = "GpuDiagnosticsBrepDocument";
constexpr auto fixtureRevision = "009-document-v1";
constexpr std::array<std::string_view, 4> fixtureComposition = {
    "RoundedHousing",
    "RoundedBoss",
    "RoundedTransparentCanopy",
    "HiddenRoundedSolid",
};
constexpr std::string_view fixtureSelected = "RoundedBoss";
constexpr std::string_view fixturePreselected = "RoundedHousing";
constexpr std::string_view fixtureHidden = "HiddenRoundedSolid";
constexpr std::string_view fixtureTransparent = "RoundedTransparentCanopy";

void closeDiagnosticsDialogs()
{
    for (auto* widget : QApplication::topLevelWidgets()) {
        if (auto* dialog = qobject_cast<QDialog*>(widget)) {
            dialog->reject();
        }
    }
}

#ifdef FREECAD_GPU_DIAGNOSTICS_HAS_PART
SoGroup* findObjectGroup(Gui::View3DInventorViewer& view)
{
    auto* viewerRoot = static_cast<SoSeparator*>(view.getSceneGraph());
    auto* modelRoot = static_cast<SoSeparator*>(viewerRoot->getChild(1));
    for (int index = 0; index < modelRoot->getNumChildren(); ++index) {
        auto* child = modelRoot->getChild(index);
        if (child->getName() == SbName("ObjectGroup")
            && child->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
            return static_cast<SoGroup*>(child);
        }
    }
    return modelRoot;
}

void installBrepFixture(
    Gui::View3DInventorViewer& view,
    bool includeOverlays,
    bool includeDuplicateEdges,
    bool denseFixture
)
{
    if (PartGui::SoBrepEdgeSet::getClassTypeId() == SoType::badType()) {
        PartGui::SoBrepEdgeSet::initClass();
    }

    const std::array<SbVec3f, 8> cubePoints {
        SbVec3f(-5.0F, -5.0F, -5.0F),
        SbVec3f(5.0F, -5.0F, -5.0F),
        SbVec3f(5.0F, 5.0F, -5.0F),
        SbVec3f(-5.0F, 5.0F, -5.0F),
        SbVec3f(-5.0F, -5.0F, 5.0F),
        SbVec3f(5.0F, -5.0F, 5.0F),
        SbVec3f(5.0F, 5.0F, 5.0F),
        SbVec3f(-5.0F, 5.0F, 5.0F),
    };
    constexpr std::array<int32_t, 36> cubeEdgeIndices {
        0, 1, -1, 1, 2, -1, 2, 3, -1, 3, 0, -1, 4, 5, -1, 5, 6, -1,
        6, 7, -1, 7, 4, -1, 0, 4, -1, 1, 5, -1, 2, 6, -1, 3, 7, -1,
    };
    constexpr int denseFixtureSide = 10;
    constexpr float denseFixtureSpacing = 12.0F;
    const int fixtureSide = denseFixture ? denseFixtureSide : 1;
    const float fixtureOffset = 0.5F * static_cast<float>(fixtureSide - 1) * denseFixtureSpacing;
    std::vector<SbVec3f> cubeCenters;
    std::vector<SbVec3f> points;
    std::vector<int32_t> baseEdgeIndices;
    const auto cubeCount = static_cast<std::size_t>(fixtureSide)
        * static_cast<std::size_t>(fixtureSide);
    cubeCenters.reserve(cubeCount);
    points.reserve(cubeCenters.capacity() * cubePoints.size());
    baseEdgeIndices.reserve(cubeCenters.capacity() * cubeEdgeIndices.size());
    for (int row = 0; row < fixtureSide; ++row) {
        for (int column = 0; column < fixtureSide; ++column) {
            const SbVec3f center(
                static_cast<float>(column) * denseFixtureSpacing - fixtureOffset,
                static_cast<float>(row) * denseFixtureSpacing - fixtureOffset,
                0.0F
            );
            const auto pointOffset = static_cast<int32_t>(points.size());
            cubeCenters.push_back(center);
            for (const auto& point : cubePoints) {
                points.push_back(point + center);
            }
            for (const auto index : cubeEdgeIndices) {
                baseEdgeIndices.push_back(index < 0 ? index : pointOffset + index);
            }
        }
    }

    std::vector<int32_t> edgeIndices(baseEdgeIndices.begin(), baseEdgeIndices.end());
    if (includeDuplicateEdges) {
        for (std::size_t index = 0; index < baseEdgeIndices.size(); index += 3) {
            edgeIndices.push_back(baseEdgeIndices[index + 1]);
            edgeIndices.push_back(baseEdgeIndices[index]);
            edgeIndices.push_back(-1);
        }
    }

    auto* root = new SoSeparator;
    root->ref();

    auto* coordinates = new SoCoordinate3;
    coordinates->point.setValues(0, static_cast<int>(points.size()), points.data());
    root->addChild(coordinates);

    auto* lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    root->addChild(lightModel);

    auto* faceRoot = new SoSeparator;
    auto* faceMaterial = new SoMaterial;
    faceMaterial->diffuseColor = SbColor(0.72F, 0.78F, 0.86F);
    faceRoot->addChild(faceMaterial);

    auto* polygonOffset = new SoPolygonOffset;
    polygonOffset->units = 1.0F;
    polygonOffset->styles = SoPolygonOffset::FILLED;
    faceRoot->addChild(polygonOffset);

    for (const auto& center : cubeCenters) {
        auto* cubeRoot = new SoSeparator;
        auto* translation = new SoTranslation;
        translation->translation = center;
        cubeRoot->addChild(translation);
        auto* faces = new SoCube;
        faces->width = 10.0F;
        faces->height = 10.0F;
        faces->depth = 10.0F;
        cubeRoot->addChild(faces);
        faceRoot->addChild(cubeRoot);
    }
    root->addChild(faceRoot);

    auto* edgeRoot = new SoSeparator;
    edgeRoot->renderCaching = SoSeparator::OFF;
    auto* edgeMaterial = new SoMaterial;
    edgeMaterial->diffuseColor = SbColor(0.05F, 0.05F, 0.05F);
    edgeRoot->addChild(edgeMaterial);

    auto* edgeStyle = new SoDrawStyle;
    edgeStyle->lineWidth = 1.0F;
    edgeRoot->addChild(edgeStyle);

    auto* edges = new PartGui::SoBrepEdgeSet;
    edges->coordIndex.setValues(0, static_cast<int>(edgeIndices.size()), edgeIndices.data());
    if (includeOverlays) {
        constexpr std::array<int32_t, 3> highlightIndices {0, 1, -1};
        constexpr std::array<int32_t, 3> selectionIndices {1, 2, -1};
        edges->highlightCoordIndex
            .setValues(0, static_cast<int>(highlightIndices.size()), highlightIndices.data());
        edges->selectionCoordIndex
            .setValues(0, static_cast<int>(selectionIndices.size()), selectionIndices.data());
    }
    edgeRoot->addChild(edges);
    root->addChild(edgeRoot);

    findObjectGroup(view)->addChild(root);
    root->unref();
}

bool installDocumentBrepFixture(Gui::View3DInventorViewer& view)
{
    App::Document* appDocument = nullptr;
    try {
        Base::Interpreter().runString("import PartGui");
        App::DocumentInitFlags createFlags;
        createFlags.createView = false;
        appDocument = App::GetApplication().newDocument(documentName, "gpuDiagnostics", createFlags);
        Base::Interpreter().runString(R"PY(
import FreeCAD as App
import Part

doc = App.getDocument("GpuDiagnosticsBrepDocument")

housing = Part.makeBox(60, 30, 16, App.Vector(-30, -15, -8))
housing = housing.makeFillet(4, housing.Edges)
for x in (-17, 0, 17):
    hole = Part.makeCylinder(5, 40, App.Vector(x, -20, 0), App.Vector(0, 1, 0))
    housing = housing.cut(hole)

housing_feature = doc.addObject("Part::Feature", "RoundedHousing")
housing_feature.Label = "Rounded housing with through holes"
housing_feature.Shape = housing
housing_feature.ViewObject.ShapeColor = (0.55, 0.68, 0.80)
housing_feature.ViewObject.LineColor = (0.12, 0.16, 0.20)

boss = Part.makeBox(16, 12, 5, App.Vector(-8, -6, 8))
boss = boss.makeFillet(1.5, boss.Edges)
boss_feature = doc.addObject("Part::Feature", "RoundedBoss")
boss_feature.Label = "Rounded top boss"
boss_feature.Shape = boss
boss_feature.ViewObject.ShapeColor = (0.62, 0.80, 0.65)

canopy = Part.makeBox(20, 16, 10, App.Vector(18, -8, 4))
canopy = canopy.makeFillet(2.0, canopy.Edges)
canopy_feature = doc.addObject("Part::Feature", "RoundedTransparentCanopy")
canopy_feature.Label = "Rounded transparent canopy"
canopy_feature.Shape = canopy
canopy_feature.ViewObject.ShapeColor = (0.82, 1.00, 0.18)
canopy_feature.ViewObject.Transparency = 50

hidden_shape = Part.makeBox(24, 18, 10, App.Vector(-12, -9, -5))
hidden_shape = hidden_shape.makeFillet(3, hidden_shape.Edges)
hidden_feature = doc.addObject("Part::Feature", "HiddenRoundedSolid")
hidden_feature.Label = "Hidden rounded solid"
hidden_feature.Shape = hidden_shape
hidden_feature.ViewObject.ShapeColor = (0.85, 0.25, 0.35)
hidden_feature.ViewObject.Visibility = False

doc.recompute()
)PY");
    }
    catch (const Base::Exception& error) {
        std::fprintf(stderr, "Could not create the document BRep fixture: %s\n", error.what());
        return false;
    }

    Gui::Document* guiDocument = Gui::Application::Instance
        ? Gui::Application::Instance->getDocument(appDocument)
        : nullptr;
    if (!appDocument || !guiDocument) {
        std::fprintf(stderr, "Could not attach the document BRep fixture to the GUI.\n");
        return false;
    }

    view.setDocument(guiDocument);
    const auto objectNames = fixtureComposition;
    for (const auto objectName : objectNames) {
        App::DocumentObject* object = appDocument->getObject(objectName.data());
        Gui::ViewProvider* provider = object ? Gui::Application::Instance->getViewProvider(object)
                                             : nullptr;
        if (!provider || !provider->getRoot()) {
            std::fprintf(stderr, "Document BRep fixture is missing %s.\n", objectName.data());
            return false;
        }
        if (objectName == fixtureHidden && provider->isShow()) {
            std::fprintf(stderr, "Document BRep fixture hidden object became visible.\n");
            return false;
        }
        view.addViewProvider(provider);
    }

    {
        Gui::SelectionLogDisabler disableSelectionLog(true);
        Gui::Selection().clearCompleteSelection();
        Gui::Selection().addSelection(documentName, "RoundedBoss", "", 0, 0, 0, nullptr, false);
        Gui::Selection().setPreselect(documentName, "RoundedHousing", "");
    }
    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_FIXTURE revision=%s composition=%s,%s,%s,%s selected=%s "
        "preselected=%s hidden=%s transparent=%s\n",
        fixtureRevision,
        fixtureComposition[0].data(),
        fixtureComposition[1].data(),
        fixtureComposition[2].data(),
        fixtureComposition[3].data(),
        fixtureSelected.data(),
        fixturePreselected.data(),
        fixtureHidden.data(),
        fixtureTransparent.data()
    );
    return true;
}

struct FixtureMaterialState
{
    std::string name;
    bool visible = false;
    bool expectedVisible = true;
    std::vector<App::Material> shapeAppearance;
};

using DocumentFixtureMaterialSet = std::array<FixtureMaterialState, fixtureComposition.size()>;

struct DocumentViewerLightState
{
    bool backlightEnabled = false;
    bool fillLightEnabled = false;
    SbColor backlightColor;
    SbColor fillLightColor;
    SbVec3f backlightDirection;
    SbVec3f fillLightDirection;
    float backlightIntensity = 0.0F;
    float fillLightIntensity = 0.0F;
};

struct DocumentViewerPoseState
{
    SbRotation orientation;
    SbVec3f viewDirection;
    SbVec3f upDirection;
};

struct DocumentLightPreferenceState
{
    std::vector<std::pair<std::string, bool>> boolValues;
    std::vector<std::pair<std::string, long>> intValues;
    std::vector<std::pair<std::string, unsigned long>> unsignedValues;
    std::vector<std::pair<std::string, double>> floatValues;
    std::vector<std::pair<std::string, std::string>> stringValues;
};

struct DocumentBrepFrameStats
{
    int visibleCenterPixels = 0;
    int hiddenRedPixels = 0;
    int navigationCubePixels = 0;
    int selectionPixels = 0;
    int preselectionPixels = 0;
    int canopyOverlapPixels = 0;
    int differingPixels = 0;
    long long renderUs = 0LL;
};

bool fixtureMaterialStateFromDocument(
    const std::array<std::string_view, 4>& names,
    DocumentFixtureMaterialSet& snapshot
)
{
    auto* appDocument = App::GetApplication().getDocument(documentName);
    if (!appDocument) {
        std::fprintf(stderr, "Could not locate the document BRep fixture.\n");
        return false;
    }
    for (std::size_t index = 0; index < names.size(); ++index) {
        const auto& objectName = names[index];
        snapshot[index].name = objectName;
        snapshot[index].expectedVisible = std::string_view(objectName) != fixtureHidden;
        const bool expectedVisible = snapshot[index].expectedVisible;

        App::DocumentObject* object = appDocument->getObject(objectName.data());
        auto* provider = object ? dynamic_cast<Gui::ViewProviderGeometryObject*>(
                                      Gui::Application::Instance->getViewProvider(object)
                                  )
                                : nullptr;
        if (!provider) {
            std::fprintf(stderr, "Could not access fixture object %s.\n", objectName.data());
            return false;
        }
        if (provider->isShow() != expectedVisible) {
            std::fprintf(
                stderr,
                "Fixture object visibility changed for %s expected=%d actual=%d.\n",
                objectName.data(),
                static_cast<int>(expectedVisible),
                static_cast<int>(provider->isShow())
            );
            return false;
        }
        snapshot[index].visible = provider->isShow();
        snapshot[index].shapeAppearance = provider->ShapeAppearance.getValues();
    }
    return true;
}

DocumentViewerLightState captureDocumentViewerLightState(Gui::View3DInventorViewer& view)
{
    const SoDirectionalLight* backlight = view.getBacklight();
    const SoDirectionalLight* fillLight = view.getFillLight();
    DocumentViewerLightState state;
    state.backlightEnabled = view.isBacklightEnabled();
    state.fillLightEnabled = view.isFillLightEnabled();
    if (backlight) {
        state.backlightColor = backlight->color.getValue();
        state.backlightDirection = backlight->direction.getValue();
        state.backlightIntensity = backlight->intensity.getValue();
    }
    if (fillLight) {
        state.fillLightColor = fillLight->color.getValue();
        state.fillLightDirection = fillLight->direction.getValue();
        state.fillLightIntensity = fillLight->intensity.getValue();
    }
    return state;
}

DocumentLightPreferenceState captureDocumentLightPreferenceState()
{
    const auto lightPreferences = App::GetApplication()
                                      .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                                      ->GetGroup("LightSources");
    return {
        lightPreferences->GetBoolMap(),
        lightPreferences->GetIntMap(),
        lightPreferences->GetUnsignedMap(),
        lightPreferences->GetFloatMap(),
        lightPreferences->GetASCIIMap(),
    };
}

bool lightPreferencesEqual(
    const DocumentLightPreferenceState& lhs,
    const DocumentLightPreferenceState& rhs
)
{
    return lhs.boolValues == rhs.boolValues && lhs.intValues == rhs.intValues
        && lhs.unsignedValues == rhs.unsignedValues && lhs.floatValues == rhs.floatValues
        && lhs.stringValues == rhs.stringValues;
}

bool viewerLightStatesEqual(const DocumentViewerLightState& lhs, const DocumentViewerLightState& rhs)
{
    return lhs.backlightEnabled == rhs.backlightEnabled
        && lhs.fillLightEnabled == rhs.fillLightEnabled && lhs.backlightColor == rhs.backlightColor
        && lhs.fillLightColor == rhs.fillLightColor
        && lhs.backlightDirection == rhs.backlightDirection
        && lhs.fillLightDirection == rhs.fillLightDirection
        && lhs.backlightIntensity == rhs.backlightIntensity
        && lhs.fillLightIntensity == rhs.fillLightIntensity;
}

void restoreDocumentViewerLightState(
    Gui::View3DInventorViewer& view,
    const DocumentViewerLightState& state
)
{
    SoDirectionalLight* backlight = view.getBacklight();
    SoDirectionalLight* fillLight = view.getFillLight();
    if (backlight) {
        backlight->on.setValue(state.backlightEnabled);
        backlight->color.setValue(state.backlightColor);
        backlight->direction.setValue(state.backlightDirection);
        backlight->intensity.setValue(state.backlightIntensity);
    }
    if (fillLight) {
        fillLight->on.setValue(state.fillLightEnabled);
        fillLight->color.setValue(state.fillLightColor);
        fillLight->direction.setValue(state.fillLightDirection);
        fillLight->intensity.setValue(state.fillLightIntensity);
    }
}

void applyCandidateDocumentLighting(Gui::View3DInventorViewer& view)
{
    if (auto* backlight = view.getBacklight()) {
        backlight->on.setValue(true);
        backlight->color.setValue(SbColor(0.82F, 0.88F, 1.0F));
        backlight->direction.setValue(SbVec3f(-0.24F, -0.42F, -0.88F));
        backlight->intensity.setValue(0.32F);
    }
    if (auto* fillLight = view.getFillLight()) {
        fillLight->on.setValue(true);
        fillLight->color.setValue(SbColor(1.0F, 0.96F, 0.90F));
        fillLight->direction.setValue(SbVec3f(0.36F, -0.56F, 0.74F));
        fillLight->intensity.setValue(0.24F);
    }
}

void applyCandidateDocumentMaterials(const DocumentFixtureMaterialSet& baselineMaterialState)
{
    auto* appDocument = App::GetApplication().getDocument(documentName);
    if (!appDocument) {
        return;
    }
    for (const auto& entry : baselineMaterialState) {
        if (entry.name != fixtureHidden) {
            auto materials = entry.shapeAppearance;
            if (materials.empty()) {
                materials.resize(1);
            }
            for (auto& material : materials) {
                material.specularColor.set(0.45F, 0.45F, 0.45F);
                material.shininess = std::max(material.shininess, 0.65F);
            }
            App::DocumentObject* object = appDocument->getObject(entry.name.c_str());
            if (!object) {
                continue;
            }
            auto* provider = dynamic_cast<Gui::ViewProviderGeometryObject*>(
                Gui::Application::Instance->getViewProvider(object)
            );
            if (!provider) {
                continue;
            }
            provider->ShapeAppearance.setValues(materials);
        }
    }
}

void restoreDocumentMaterials(const DocumentFixtureMaterialSet& snapshot)
{
    auto* appDocument = App::GetApplication().getDocument(documentName);
    if (!appDocument) {
        return;
    }
    for (const auto& entry : snapshot) {
        App::DocumentObject* object = appDocument->getObject(entry.name.c_str());
        auto* provider = object ? dynamic_cast<Gui::ViewProviderGeometryObject*>(
                                      Gui::Application::Instance->getViewProvider(object)
                                  )
                                : nullptr;
        if (provider) {
            provider->ShapeAppearance.setValues(entry.shapeAppearance);
        }
    }
}

QByteArray computeDocumentImageHash(const QImage& frame)
{
    const QImage image = frame.convertToFormat(QImage::Format_RGB32);
    return QCryptographicHash::hash(
               QByteArray::fromRawData(
                   reinterpret_cast<const char*>(image.constBits()),
                   image.sizeInBytes()
               ),
               QCryptographicHash::Sha256
    )
        .toHex();
}

int diffDocumentImagePixels(const QImage& lhs, const QImage& rhs)
{
    if (lhs.size() != rhs.size()) {
        return -1;
    }
    const QImage lhsImage = lhs.convertToFormat(QImage::Format_RGB32);
    const QImage rhsImage = rhs.convertToFormat(QImage::Format_RGB32);
    if (lhsImage.isNull() || rhsImage.isNull()) {
        return -1;
    }
    int differing = 0;
    for (int y = 0; y < lhsImage.height(); ++y) {
        const auto* lhsPixels = reinterpret_cast<const QRgb*>(lhsImage.constScanLine(y));
        const auto* rhsPixels = reinterpret_cast<const QRgb*>(rhsImage.constScanLine(y));
        for (int x = 0; x < lhsImage.width(); ++x) {
            if (lhsPixels[x] != rhsPixels[x]) {
                ++differing;
            }
        }
    }
    return differing;
}

DocumentBrepFrameStats gatherDocumentBrepFrameStats(const QImage& frame)
{
    const QImage image = frame.convertToFormat(QImage::Format_RGB32);
    const int centerLeft = image.width() / 2 - 80;
    const int centerRight = image.width() / 2 + 80;
    const int centerTop = image.height() / 2 - 80;
    const int centerBottom = image.height() / 2 + 80;
    DocumentBrepFrameStats stats;
    for (int y = 0; y < image.height(); ++y) {
        const auto* pixels = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const int red = qRed(pixels[x]);
            const int green = qGreen(pixels[x]);
            const int blue = qBlue(pixels[x]);
            if (x >= centerLeft && x < centerRight && y >= centerTop && y < centerBottom
                && red < 210 && green < 210 && blue < 220) {
                ++stats.visibleCenterPixels;
            }
            if (red > 140 && red > green + 40 && red > blue + 30) {
                ++stats.hiddenRedPixels;
            }
            if (red > 200 && green < 80 && blue > 200) {
                ++stats.selectionPixels;
            }
            if (red < 80 && green > 150 && blue > 220) {
                ++stats.preselectionPixels;
            }
            if (green > 170 && green > red + 30 && green > blue + 30) {
                ++stats.canopyOverlapPixels;
            }
            if (x >= image.width() - 180 && y < 160
                && std::max({red, green, blue}) - std::min({red, green, blue}) < 20
                && std::max({red, green, blue}) > 80 && std::max({red, green, blue}) < 240) {
                ++stats.navigationCubePixels;
            }
        }
    }
    return stats;
}

void printDocumentBrepFrameStats(const DocumentBrepFrameStats& stats)
{
    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_FRAME visible_center_pixels=%d hidden_red_pixels=%d "
        "navigation_cube_pixels=%d selection_pixels=%d preselection_pixels=%d "
        "canopy_overlap_pixels=%d\n",
        stats.visibleCenterPixels,
        stats.hiddenRedPixels,
        stats.navigationCubePixels,
        stats.selectionPixels,
        stats.preselectionPixels,
        stats.canopyOverlapPixels
    );
}

bool validateDocumentBrepFrame(const QImage& frame)
{
    const auto stats = gatherDocumentBrepFrameStats(frame);
    printDocumentBrepFrameStats(stats);
    return stats.visibleCenterPixels >= 5000
        && stats.hiddenRedPixels <= frame.width() * frame.height() / 200
        && stats.navigationCubePixels >= 25 && stats.selectionPixels >= 1000
        && stats.preselectionPixels >= 1000 && stats.canopyOverlapPixels >= 500;
}

DocumentViewerPoseState captureDocumentViewerPoseState(const Gui::View3DInventorViewer& view)
{
    return {
        view.getCameraOrientation(),
        view.getViewDirection(),
        view.getUpDirection(),
    };
}

bool vectorsClose(const SbVec3f& lhs, const SbVec3f& rhs, float epsilon = 1e-6F)
{
    return std::fabs(lhs[0] - rhs[0]) < epsilon && std::fabs(lhs[1] - rhs[1]) < epsilon
        && std::fabs(lhs[2] - rhs[2]) < epsilon;
}

bool posesClose(
    const DocumentViewerPoseState& lhs,
    const DocumentViewerPoseState& rhs,
    float epsilon = 1e-6F
)
{
    float lhsAngle = 0.0F;
    float rhsAngle = 0.0F;
    SbVec3f lhsAxis;
    SbVec3f rhsAxis;
    lhs.orientation.getValue(lhsAxis, lhsAngle);
    rhs.orientation.getValue(rhsAxis, rhsAngle);
    return vectorsClose(lhs.viewDirection, rhs.viewDirection, epsilon)
        && vectorsClose(lhs.upDirection, rhs.upDirection, epsilon)
        && vectorsClose(lhsAxis, rhsAxis, epsilon) && std::fabs(lhsAngle - rhsAngle) < epsilon;
}

bool fixtureVisibilityInvariant(
    const DocumentFixtureMaterialSet& expected,
    const DocumentFixtureMaterialSet& actual
)
{
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (expected[index].name != actual[index].name
            || expected[index].visible != actual[index].visible) {
            std::fprintf(
                stderr,
                "Fixture visibility invariant changed at index=%zu expected=%s:%d actual=%s:%d.\n",
                index,
                expected[index].name.c_str(),
                static_cast<int>(expected[index].visible),
                actual[index].name.c_str(),
                static_cast<int>(actual[index].visible)
            );
            return false;
        }
    }
    return true;
}

bool fixtureMaterialsInvariant(
    const DocumentFixtureMaterialSet& expected,
    const DocumentFixtureMaterialSet& actual
)
{
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (expected[index].name != actual[index].name
            || expected[index].shapeAppearance != actual[index].shapeAppearance) {
            std::fprintf(
                stderr,
                "Fixture material invariant changed at index=%zu object=%s.\n",
                index,
                expected[index].name.c_str()
            );
            return false;
        }
    }
    return true;
}

QImage renderDocumentFrameForLightingMatrix(Gui::View3DInventorViewer& view, long long& renderUs)
{
    renderUs = 0LL;
    QElapsedTimer timer;
    timer.start();
    if (auto* graph = view.getSceneGraph()) {
        graph->touch();
    }
    view.redraw();
    QCoreApplication::processEvents();
    renderUs = timer.nsecsElapsed() / nanosecondsPerMicrosecond;
    return view.grabFramebuffer();
}

struct DocumentLightingMaterialProfileResult
{
    const char* profile = "";
    bool lighting = false;
    bool material = false;
    long long renderUs = 0LL;
    int diffPixels = 0;
    QByteArray hash;
    DocumentBrepFrameStats stats;
};

void printDocumentLightingMaterialProfile(
    const DocumentLightingMaterialProfileResult& result,
    int diffVsBaseline
)
{
    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_LIGHTING_MATERIAL profile=%s lighting=%d material=%d "
        "render_us=%lld visible_center_pixels=%d hidden_red_pixels=%d navigation_cube_pixels=%d "
        "selection_pixels=%d preselection_pixels=%d canopy_overlap_pixels=%d "
        "diff_vs_baseline_pixels=%d "
        "hash=%s\n",
        result.profile,
        static_cast<int>(result.lighting),
        static_cast<int>(result.material),
        result.renderUs,
        result.stats.visibleCenterPixels,
        result.stats.hiddenRedPixels,
        result.stats.navigationCubePixels,
        result.stats.selectionPixels,
        result.stats.preselectionPixels,
        result.stats.canopyOverlapPixels,
        diffVsBaseline,
        result.hash.constData()
    );
}

bool runDocumentLightingMaterialProbe(Gui::View3DInventorViewer& view)
{
    constexpr int minimumVisualChangePixels = 500;
    DocumentFixtureMaterialSet baselineMaterialState {};
    DocumentFixtureMaterialSet currentMaterialState {};
    if (!fixtureMaterialStateFromDocument(fixtureComposition, baselineMaterialState)
        || !fixtureMaterialStateFromDocument(fixtureComposition, currentMaterialState)) {
        return false;
    }
    if (!fixtureVisibilityInvariant(baselineMaterialState, currentMaterialState)) {
        return false;
    }

    const auto baselinePose = captureDocumentViewerPoseState(view);
    const auto baselineLightState = captureDocumentViewerLightState(view);
    const auto baselineLightPreferences = captureDocumentLightPreferenceState();

    const std::array<std::tuple<const char*, bool, bool>, 4> profiles {
        std::tuple<const char*, bool, bool> {"baseline", false, false},
        {"lighting_only", true, false},
        {"material_only", false, true},
        {"combined", true, true},
    };
    std::array<DocumentLightingMaterialProfileResult, 4> results {};
    std::array<QImage, 4> renderedFrames {};
    bool relationOk = true;

    for (std::size_t index = 0; index < profiles.size(); ++index) {
        const auto& profile = profiles[index];
        const bool lighting = std::get<1>(profile);
        const bool material = std::get<2>(profile);

        restoreDocumentMaterials(baselineMaterialState);
        restoreDocumentViewerLightState(view, baselineLightState);
        if (lighting) {
            applyCandidateDocumentLighting(view);
        }
        if (material) {
            applyCandidateDocumentMaterials(baselineMaterialState);
        }

        if (!fixtureMaterialStateFromDocument(fixtureComposition, currentMaterialState)) {
            return false;
        }
        if (!fixtureVisibilityInvariant(baselineMaterialState, currentMaterialState)) {
            relationOk = false;
        }
        const auto profilePose = captureDocumentViewerPoseState(view);
        if (!posesClose(profilePose, baselinePose)) {
            relationOk = false;
        }

        long long renderUs = 0LL;
        const QImage frame = renderDocumentFrameForLightingMatrix(view, renderUs);
        if (frame.isNull()) {
            relationOk = false;
            break;
        }
        renderedFrames[index] = frame;

        auto& result = results[index];
        result.profile = std::get<0>(profile);
        result.lighting = lighting;
        result.material = material;
        result.renderUs = renderUs;
        result.stats = gatherDocumentBrepFrameStats(frame);
        result.hash = computeDocumentImageHash(frame);
        if (index > 0) {
            result.diffPixels = diffDocumentImagePixels(frame, renderedFrames[0]);
        }
        printDocumentLightingMaterialProfile(result, result.diffPixels);
        printDocumentBrepFrameStats(result.stats);
    }

    if (renderedFrames[0].isNull()) {
        return false;
    }

    restoreDocumentMaterials(baselineMaterialState);
    restoreDocumentViewerLightState(view, baselineLightState);
    long long restoredRenderUs = 0LL;
    const QImage restoredFrame = renderDocumentFrameForLightingMatrix(view, restoredRenderUs);
    if (restoredFrame.isNull()) {
        return false;
    }
    const int restoreDiffPixels = diffDocumentImagePixels(restoredFrame, renderedFrames[0]);
    const QByteArray restoredHash = computeDocumentImageHash(restoredFrame);
    const bool restoreMatch = restoreDiffPixels == 0 && !restoredHash.isEmpty()
        && !results[0].hash.isEmpty() && restoredHash == results[0].hash;
    const auto restoredPose = captureDocumentViewerPoseState(view);
    if (!posesClose(restoredPose, baselinePose)) {
        relationOk = false;
    }
    if (!fixtureMaterialStateFromDocument(fixtureComposition, currentMaterialState)) {
        return false;
    }
    const bool visibilityStable
        = fixtureVisibilityInvariant(baselineMaterialState, currentMaterialState);
    const bool materialsRestored
        = fixtureMaterialsInvariant(baselineMaterialState, currentMaterialState);
    const bool lightStateRestored
        = viewerLightStatesEqual(baselineLightState, captureDocumentViewerLightState(view));
    const bool preferencesStable
        = lightPreferencesEqual(baselineLightPreferences, captureDocumentLightPreferenceState());
    if (!visibilityStable || !materialsRestored || !lightStateRestored || !preferencesStable) {
        relationOk = false;
    }

    const bool baselineValid = validateDocumentBrepFrame(renderedFrames[0]);
    const bool lightingChanged = results[1].diffPixels > minimumVisualChangePixels;
    const bool materialChanged = results[2].diffPixels > minimumVisualChangePixels;
    const bool combinedChanged = results[3].diffPixels > minimumVisualChangePixels;
    const int baselineDiffLighting = results[1].diffPixels;
    const int baselineDiffMaterial = results[2].diffPixels;
    const int baselineDiffCombined = results[3].diffPixels;
    const int lightingVsMaterialDiff = diffDocumentImagePixels(renderedFrames[1], renderedFrames[2]);
    const int lightingVsCombinedDiff = diffDocumentImagePixels(renderedFrames[1], renderedFrames[3]);
    const int materialVsCombinedDiff = diffDocumentImagePixels(renderedFrames[2], renderedFrames[3]);
    relationOk = relationOk && baselineValid && lightingChanged && materialChanged
        && combinedChanged && restoreMatch && visibilityStable && materialsRestored
        && lightStateRestored && preferencesStable && lightingVsMaterialDiff >= 0
        && lightingVsCombinedDiff >= 0 && materialVsCombinedDiff >= 0;

    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_LIGHTING_MATERIAL_DIFF profile=lighting_vs_material "
        "diff_pixels=%d\n",
        lightingVsMaterialDiff
    );
    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_LIGHTING_MATERIAL_DIFF profile=lighting_vs_combined "
        "diff_pixels=%d\n",
        lightingVsCombinedDiff
    );
    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_LIGHTING_MATERIAL_DIFF profile=material_vs_combined "
        "diff_pixels=%d\n",
        materialVsCombinedDiff
    );
    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_LIGHTING_MATERIAL_RELATION outcome=%s baseline=%d lighting=%d "
        "material=%d combined=%d restore_match=%d camera_stable=%d visibility_stable=%d "
        "material_restore=%d light_restore=%d light_preferences_stable=%d "
        "baseline_render_us=%lld lighting_render_us=%lld material_render_us=%lld "
        "combined_render_us=%lld restore_render_us=%lld baseline_diff_lighting=%d "
        "baseline_diff_material=%d baseline_diff_combined=%d\n",
        relationOk ? "pass" : "fail",
        static_cast<int>(baselineValid),
        static_cast<int>(lightingChanged),
        static_cast<int>(materialChanged),
        static_cast<int>(combinedChanged),
        static_cast<int>(restoreMatch),
        static_cast<int>(posesClose(restoredPose, baselinePose)),
        static_cast<int>(visibilityStable),
        static_cast<int>(materialsRestored),
        static_cast<int>(lightStateRestored),
        static_cast<int>(preferencesStable),
        results[0].renderUs,
        results[1].renderUs,
        results[2].renderUs,
        results[3].renderUs,
        restoredRenderUs,
        baselineDiffLighting,
        baselineDiffMaterial,
        baselineDiffCombined
    );
    if (!relationOk) {
        std::fprintf(stderr, "Document lighting-material matrix failed.\n");
    }
    return relationOk;
}

struct TessellationProfileResult
{
    const char* profile = "";
    double deviation = 0.0;
    double angularDeg = 0.0;
    double viewScale = 0.0;
    double effectiveDeviation = 0.0;
    double computedDeflection = 0.0;
    int triangles = 0;
    int nodes = 0;
    long long approxMemoryBytes = 0LL;
    long long meshTimeUs = 0LL;
    const char* fallback = "none";
};

struct TessellationProfileInput
{
    const char* name;
    double deviation;
    double angularDeg;
    double viewScale;
    int triangleCap;
};

void runTessellationProfile(
    const TopoDS_Shape& sourceShape,
    const TessellationProfileInput& input,
    long long memoryCapBytes,
    long long timeCapUs,
    TessellationProfileResult& result
)
{
    const double safeViewScale = std::max(0.25, input.viewScale);
    TopoDS_Shape copiedShape = BRepBuilderAPI_Copy(sourceShape).Shape();
    IMeshTools_Parameters params;

    result.profile = input.name;
    result.deviation = input.deviation;
    result.angularDeg = input.angularDeg;
    result.viewScale = safeViewScale;
    result.effectiveDeviation = input.deviation / safeViewScale;
    result.computedDeflection = Part::Tools::getDeflection(copiedShape, result.effectiveDeviation);
    if (result.computedDeflection < Precision::Confusion()) {
        result.computedDeflection = Precision::Confusion();
    }
    params.Deflection = result.computedDeflection;
    params.Relative = Standard_False;
    params.Angle = Base::toRadians(input.angularDeg);
    params.InParallel = Standard_True;
    params.AllowQualityDecrease = Standard_True;

# if OCC_VERSION_HEX < 0x070600
    BRepTools::Clean(copiedShape);
# else
    BRepTools::Clean(copiedShape, Standard_True);
# endif

    QElapsedTimer timer;
    timer.start();
    [[maybe_unused]] BRepMesh_IncrementalMesh mesher(copiedShape, params);
    result.meshTimeUs = timer.nsecsElapsed() / nanosecondsPerMicrosecond;

    TopTools_IndexedMapOfShape faceMap;
    TopExp::MapShapes(copiedShape, TopAbs_FACE, faceMap);
    TopLoc_Location location;
    int triangles = 0;
    int nodes = 0;
    for (int index = 1; index <= faceMap.Extent(); ++index) {
        Handle(Poly_Triangulation)
            mesh = BRep_Tool::Triangulation(TopoDS::Face(faceMap(index)), location);
        if (mesh.IsNull()) {
            continue;
        }
        triangles += mesh->NbTriangles();
        nodes += mesh->NbNodes();
    }
    result.triangles = triangles;
    result.nodes = nodes;
    result.approxMemoryBytes = static_cast<long long>(triangles) * approximateBytesPerTriangle
        + static_cast<long long>(nodes) * approximateBytesPerNode;

    if (result.triangles == 0 || result.nodes == 0) {
        result.fallback = "mesh_failed";
    }
    else if (result.triangles > input.triangleCap) {
        result.fallback = "triangle_cap";
    }
    else if (result.approxMemoryBytes > memoryCapBytes) {
        result.fallback = "memory_cap";
    }
    else if (result.meshTimeUs > timeCapUs) {
        result.fallback = "time_cap";
    }

    // Estimated memory bytes from face mesh counts.
    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_TESSELLATION profile=%s deviation=%0.6f angular_deg=%0.3f "
        "effective_deviation=%0.6f view_scale=%0.3f computed_deflection=%0.6f triangles=%d "
        "nodes=%d approximate_memory_bytes=%lld mesh_time_us=%lld triangle_cap=%d "
        "memory_cap_bytes=%lld time_cap_us=%lld fallback=%s\n",
        result.profile,
        result.deviation,
        result.angularDeg,
        result.effectiveDeviation,
        result.viewScale,
        result.computedDeflection,
        result.triangles,
        result.nodes,
        result.approxMemoryBytes,
        result.meshTimeUs,
        input.triangleCap,
        memoryCapBytes,
        timeCapUs,
        result.fallback
    );
}

bool runDocumentTessellationProbe()
{
    constexpr int triangleCap = 3000000;
    constexpr long long memoryCapBytes = 64LL * 1024LL * 1024LL;
    constexpr long long timeCapUs = 1200000LL;
    constexpr auto fixtureMeshObject = "RoundedHousing";

    auto* appDocument = App::GetApplication().getDocument(documentName);
    if (!appDocument) {
        std::fprintf(stderr, "Could not locate the document BRep fixture.\n");
        return false;
    }

    auto* sourceObject = dynamic_cast<Part::Feature*>(appDocument->getObject(fixtureMeshObject));
    if (!sourceObject) {
        std::fprintf(stderr, "Could not access fixture mesh object %s.\n", fixtureMeshObject);
        return false;
    }

    const TopoDS_Shape sourceShape = sourceObject->Shape.getValue();
    if (sourceShape.IsNull()) {
        std::fprintf(stderr, "Fixture mesh object %s is empty.\n", fixtureMeshObject);
        return false;
    }

    const std::array<TessellationProfileInput, 5> profiles {
        TessellationProfileInput {"baseline", 0.2, 28.65, 1.0, 3000000},
        TessellationProfileInput {"finer", 0.05, 28.65, 1.0, 3000000},
        TessellationProfileInput {"angle_reduced", 0.2, 4.0, 1.0, 3000000},
        TessellationProfileInput {"view_scale_simulated", 0.2, 28.65, 2.25, 3000000},
        TessellationProfileInput {"triangle_cap_probe", 0.2, 28.65, 1.0, 1},
    };

    std::array<TessellationProfileResult, 5> results;
    for (std::size_t index = 0; index < profiles.size(); ++index) {
        runTessellationProfile(sourceShape, profiles[index], memoryCapBytes, timeCapUs, results[index]);
    }

    const auto fallbackCount = std::count_if(results.begin(), results.end(), [](const auto& result) {
        return std::string_view(result.fallback) != std::string_view("none");
    });
    const bool baselineOk = results[0].triangles >= 1000;
    const bool finerOk = results[1].triangles >= results[0].triangles;
    const bool angleOk = results[2].triangles >= results[0].triangles;
    const bool scaleOk = results[3].triangles >= results[0].triangles;
    const bool capProbeOk = results[4].fallback == std::string_view("triangle_cap");
    const bool relationOk = baselineOk && finerOk && angleOk && scaleOk && capProbeOk
        && (fallbackCount == 1);

    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_TESSELLATION_RELATION outcome=%s baseline=%s finer=%s angle=%s "
        "scale=%s "
        "triangle_cap=%d memory_cap_bytes=%lld time_cap_us=%lld "
        "baseline_triangles=%d finer_triangles=%d angle_triangles=%d scale_triangles=%d "
        "cap_probe_fallback=%s cap_probe_triangles=%d fallback_count=%td\n",
        relationOk ? "pass" : "fail",
        results[0].fallback,
        results[1].fallback,
        results[2].fallback,
        results[3].fallback,
        triangleCap,
        memoryCapBytes,
        timeCapUs,
        results[0].triangles,
        results[1].triangles,
        results[2].triangles,
        results[3].triangles,
        results[4].fallback,
        results[4].triangles,
        fallbackCount
    );
    if (!relationOk) {
        std::fprintf(stderr, "Mesh profile relation check failed.\n");
    }
    return relationOk;
}

constexpr int postprocessDepthProbeWidth = 960;
constexpr int postprocessDepthProbeHeight = 640;

struct DocumentPostprocessDepthProbeResult
{
    const char* profile = "document_postprocess_depth";
    int requestedWidth = 0;
    int requestedHeight = 0;
    int available = 0;
    const char* fallback = "none";
    const char* reason = "none";
    const char* gammaPolicy = "unmanaged-linear";
    int colorInternalFormat = 0;
    int depthInternalFormat = 0;
    int colorWidth = 0;
    int colorHeight = 0;
    int depthBits = 0;
    const char* colorEncoding = "unknown";
    const char* depthAttachmentType = "unknown";
    const char* depthAttachmentTarget = "unknown";
    int sampleableDepth = 0;
    long long allocationUs = 0LL;
    long long clearUs = 0LL;
    long long estimatedBytes = 0LL;
    int framebufferRestored = 0;
    int viewportRestored = 0;
    int textureRestored = 0;
    int framebufferSrgbRestored = 0;
    int framebufferSrgbBefore = 0;
    int framebufferSrgbAfter = 0;
    int objectsDeleted = 0;
};

const char* colorEncodingFromFramebufferAttachment(int value)
{
    if (value == GL_SRGB) {
        return "GL_SRGB";
    }
    if (value == GL_LINEAR) {
        return "GL_LINEAR";
    }
    return "unknown";
}

const char* framebufferAttachmentType(int type)
{
    if (type == GL_TEXTURE) {
        return "GL_TEXTURE";
    }
    if (type == GL_RENDERBUFFER) {
        return "GL_RENDERBUFFER";
    }
    return "unknown";
}

const char* framebufferAttachmentTarget(int target)
{
    if (target == GL_TEXTURE_2D) {
        return "GL_TEXTURE_2D";
    }
    if (target == GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
        return "GL_TEXTURE_CUBE_MAP_POSITIVE_X";
    }
    return "none";
}

void printDocumentPostprocessDepthProfile(const DocumentPostprocessDepthProbeResult& result)
{
    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_POSTPROCESS_DEPTH profile=%s requested=%dx%d available=%d "
        "fallback=%s reason=%s "
        "gamma_policy=%s "
        "color_internal=%s depth_internal=%s depth_bits=%d color_encoding=%s "
        "depth_attachment_type=%s depth_attachment_target=%s sampleable_depth=%d "
        "no_pixel_readback=1 size=%dx%d estimated_bytes=%lld allocation_us=%lld clear_us=%lld\n",
        result.profile,
        result.requestedWidth,
        result.requestedHeight,
        result.available,
        result.fallback,
        result.reason,
        result.gammaPolicy,
        result.colorInternalFormat == GL_RGBA8 ? "GL_RGBA8" : "other",
        result.depthInternalFormat == GL_DEPTH_COMPONENT24 ? "GL_DEPTH_COMPONENT24" : "other",
        result.depthBits,
        result.colorEncoding,
        result.depthAttachmentType,
        result.depthAttachmentTarget,
        result.sampleableDepth,
        result.colorWidth,
        result.colorHeight,
        result.estimatedBytes,
        result.allocationUs,
        result.clearUs
    );
}

void printDocumentPostprocessDepthRelation(
    const DocumentPostprocessDepthProbeResult& result,
    bool relationOk
)
{
    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_POSTPROCESS_DEPTH_RELATION outcome=%s requested=%dx%d available=%d "
        "fallback=%s reason=%s gamma_policy=%s color_internal=%s depth_internal=%s depth_bits=%d "
        "color_encoding=%s depth_attachment_type=%s sampleable_depth=%d no_pixel_readback=1 "
        "framebuffer_restored=%d viewport_restored=%d "
        "texture_restored=%d framebuffer_srgb_before=%d framebuffer_srgb_after=%d "
        "framebuffer_srgb_restored=%d "
        "objects_deleted=%d\n",
        relationOk ? "pass" : "fail",
        result.requestedWidth,
        result.requestedHeight,
        result.available,
        result.fallback,
        result.reason,
        result.gammaPolicy,
        result.colorInternalFormat == GL_RGBA8 ? "GL_RGBA8" : "other",
        result.depthInternalFormat == GL_DEPTH_COMPONENT24 ? "GL_DEPTH_COMPONENT24" : "other",
        result.depthBits,
        result.colorEncoding,
        result.depthAttachmentType,
        result.sampleableDepth,
        result.framebufferRestored,
        result.viewportRestored,
        result.textureRestored,
        result.framebufferSrgbBefore,
        result.framebufferSrgbAfter,
        result.framebufferSrgbRestored,
        result.objectsDeleted
    );
}

bool runDocumentPostprocessDepthProbe(Gui::View3DInventorViewer& view, bool forceNativeFallback)
{
    DocumentPostprocessDepthProbeResult result;
    result.requestedWidth = postprocessDepthProbeWidth;
    result.requestedHeight = postprocessDepthProbeHeight;

    auto* glWidget = view.findChild<QOpenGLWidget*>();
    if (!glWidget) {
        std::fprintf(stderr, "No QOpenGLWidget found for document postprocess depth probe.\n");
        return false;
    }
    auto* context = glWidget->context();
    if (!context || !context->isValid()) {
        std::fprintf(stderr, "Invalid GL context for document postprocess depth probe.\n");
        return false;
    }
    auto* functions = context->extraFunctions();
    if (!functions) {
        std::fprintf(stderr, "Could not access OpenGL extra functions for depth probe.\n");
        return false;
    }
    glWidget->makeCurrent();
    functions->initializeOpenGLFunctions();

    GLint prevDrawFbo = 0;
    GLint prevReadFbo = 0;
    GLint prevViewport[4] = {0, 0, 0, 0};
    GLfloat prevClearColor[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    GLfloat prevClearDepth = 1.0F;
    const bool prevSrgbEnabled = functions->glIsEnabled(GL_FRAMEBUFFER_SRGB);
    GLint framebufferColorEncoding = 0;
    GLint prevActiveTexture = 0;
    GLint prevTextureBinding = 0;
    functions->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFbo);
    functions->glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFbo);
    functions->glGetIntegerv(GL_VIEWPORT, prevViewport);
    functions->glGetFloatv(GL_COLOR_CLEAR_VALUE, prevClearColor);
    functions->glGetFloatv(GL_DEPTH_CLEAR_VALUE, &prevClearDepth);
    functions->glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTexture);
    if (prevActiveTexture == 0) {
        prevActiveTexture = GL_TEXTURE0;
    }
    functions->glActiveTexture(prevActiveTexture);
    functions->glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTextureBinding);
    bool relationOk = true;
    bool framebufferRestored = false;
    bool viewportRestored = false;
    bool textureRestored = false;
    bool srgbRestored = false;
    bool objectsDeleted = false;
    result.framebufferSrgbBefore = static_cast<int>(prevSrgbEnabled);

    if (forceNativeFallback) {
        result.fallback = "native";
        result.reason = "forced";
        result.available = 0;
        result.colorInternalFormat = GL_RGBA8;
        result.depthInternalFormat = GL_DEPTH_COMPONENT24;
        result.colorWidth = postprocessDepthProbeWidth;
        result.colorHeight = postprocessDepthProbeHeight;
        result.depthBits = 24;
        result.colorEncoding = "unknown";
        result.depthAttachmentType = "none";
        result.depthAttachmentTarget = "none";
        result.sampleableDepth = 0;
        result.estimatedBytes = static_cast<long long>(postprocessDepthProbeWidth)
            * postprocessDepthProbeHeight * 7LL;
        result.allocationUs = 0LL;
        result.clearUs = 0LL;
        objectsDeleted = true;
    }
    else {
        GLuint colorTexture = 0;
        GLuint depthTexture = 0;
        GLuint fbo = 0;
        bool sampledDepth = false;
        bool colorAliveBeforeDelete = false;
        bool depthAliveBeforeDelete = false;
        bool fboAliveBeforeDelete = false;
        GLint depthAttachmentType = 0;
        GLint depthAttachmentTarget = 0;
        GLint depthAttachmentName = 0;
        GLint colorWidth = 0;
        GLint colorHeight = 0;
        GLint depthWidth = 0;
        GLint depthHeight = 0;
        GLint depthBits = 0;
        GLint colorInternal = 0;
        GLint depthInternal = 0;
        QElapsedTimer allocationTimer;
        allocationTimer.start();
        functions->glViewport(0, 0, postprocessDepthProbeWidth, postprocessDepthProbeHeight);
        functions->glGenTextures(1, &colorTexture);
        functions->glBindTexture(GL_TEXTURE_2D, colorTexture);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        functions->glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            postprocessDepthProbeWidth,
            postprocessDepthProbeHeight,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );

        functions->glGenTextures(1, &depthTexture);
        functions->glBindTexture(GL_TEXTURE_2D, depthTexture);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        functions->glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_DEPTH_COMPONENT24,
            postprocessDepthProbeWidth,
            postprocessDepthProbeHeight,
            0,
            GL_DEPTH_COMPONENT,
            GL_UNSIGNED_INT,
            nullptr
        );

        functions->glGenFramebuffers(1, &fbo);
        functions->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        functions->glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            colorTexture,
            0
        );
        functions->glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            depthTexture,
            0
        );

        const GLenum framebufferStatus = functions->glCheckFramebufferStatus(GL_FRAMEBUFFER);
        result.available = framebufferStatus == GL_FRAMEBUFFER_COMPLETE ? 1 : 0;
        result.fallback = result.available ? "none" : "native";
        result.reason = result.available ? "none" : "framebuffer_incomplete";

        functions->glBindTexture(GL_TEXTURE_2D, colorTexture);
        functions->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &colorWidth);
        functions->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &colorHeight);
        functions->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &colorInternal);
        functions->glBindTexture(GL_TEXTURE_2D, depthTexture);
        functions->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &depthWidth);
        functions->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &depthHeight);
        functions->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_DEPTH_SIZE, &depthBits);
        functions->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &depthInternal);

        functions->glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
            &depthAttachmentType
        );
        functions->glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
            &depthAttachmentName
        );
        if (depthAttachmentType == GL_TEXTURE) {
# ifdef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_TARGET
            functions->glGetFramebufferAttachmentParameteriv(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_TARGET,
                &depthAttachmentTarget
            );
# endif
            sampledDepth = depthAttachmentName == static_cast<GLint>(depthTexture);
        }

        result.colorInternalFormat = colorInternal;
        result.depthInternalFormat = depthInternal;
# ifdef GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING
        functions->glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,
            &framebufferColorEncoding
        );
        result.colorEncoding = colorEncodingFromFramebufferAttachment(framebufferColorEncoding);
# else
        result.colorEncoding = "unknown";
# endif
        result.colorWidth = colorWidth;
        result.colorHeight = colorHeight;
        result.depthBits = depthBits;
        result.depthAttachmentType = framebufferAttachmentType(depthAttachmentType);
        result.depthAttachmentTarget = framebufferAttachmentTarget(depthAttachmentTarget);
        result.sampleableDepth = (depthAttachmentType == GL_TEXTURE && sampledDepth) ? 1 : 0;
        result.estimatedBytes = static_cast<long long>(result.colorWidth)
                * static_cast<long long>(result.colorHeight) * 4LL
            + static_cast<long long>(result.colorWidth) * static_cast<long long>(result.colorHeight)
                * 3LL;
        result.allocationUs = allocationTimer.nsecsElapsed() / nanosecondsPerMicrosecond;

        if (result.available) {
            QElapsedTimer clearTimer;
            clearTimer.start();
            functions->glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
            functions->glClearDepthf(1.0F);
            functions->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            functions->glFinish();
            result.clearUs = clearTimer.nsecsElapsed() / nanosecondsPerMicrosecond;
        }
        objectsDeleted = false;

        if (result.available
            && (result.colorWidth != depthWidth || result.colorHeight != depthHeight)) {
            result.fallback = "inconsistent_attachment_sizes";
            result.reason = "inconsistent_attachment_sizes";
            relationOk = false;
        }
        if (result.available
            && (result.colorWidth != postprocessDepthProbeWidth
                || result.colorHeight != postprocessDepthProbeHeight)) {
            result.fallback = "unexpected_texture_size";
            result.reason = "unexpected_texture_size";
            relationOk = false;
        }

        colorAliveBeforeDelete = functions->glIsTexture(colorTexture);
        depthAliveBeforeDelete = functions->glIsTexture(depthTexture);
        fboAliveBeforeDelete = functions->glIsFramebuffer(fbo);
        if (colorTexture == 0 || depthTexture == 0 || fbo == 0 || !colorAliveBeforeDelete
            || !depthAliveBeforeDelete || !fboAliveBeforeDelete) {
            result.fallback = "allocation_failed";
            result.reason = "allocation_failed";
            result.available = 0;
            relationOk = false;
        }

        const GLuint allocatedColorTexture = colorTexture;
        const GLuint allocatedDepthTexture = depthTexture;
        const GLuint allocatedFbo = fbo;
        functions->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFbo);
        functions->glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFbo);
        functions->glActiveTexture(prevActiveTexture);
        functions->glBindTexture(GL_TEXTURE_2D, prevTextureBinding);
        if (fbo != 0) {
            functions->glDeleteFramebuffers(1, &fbo);
        }
        if (colorTexture != 0) {
            functions->glDeleteTextures(1, &colorTexture);
        }
        if (depthTexture != 0) {
            functions->glDeleteTextures(1, &depthTexture);
        }
        objectsDeleted = allocatedColorTexture != 0 && allocatedDepthTexture != 0
            && allocatedFbo != 0 && colorAliveBeforeDelete && depthAliveBeforeDelete
            && fboAliveBeforeDelete && !functions->glIsTexture(allocatedColorTexture)
            && !functions->glIsTexture(allocatedDepthTexture)
            && !functions->glIsFramebuffer(allocatedFbo);
    }

    functions->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFbo);
    functions->glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFbo);
    functions->glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    functions->glClearColor(prevClearColor[0], prevClearColor[1], prevClearColor[2], prevClearColor[3]);
    functions->glClearDepthf(prevClearDepth);
    functions->glActiveTexture(prevActiveTexture);
    functions->glBindTexture(GL_TEXTURE_2D, prevTextureBinding);
    if (prevSrgbEnabled) {
        functions->glEnable(GL_FRAMEBUFFER_SRGB);
    }
    else {
        functions->glDisable(GL_FRAMEBUFFER_SRGB);
    }

    GLint restoredActiveTexture = 0;
    GLint restoredTextureBinding = 0;
    GLint restoredDrawFbo = 0;
    GLint restoredReadFbo = 0;
    bool restoredSrgb = false;
    GLint restoredViewport[4] = {0, 0, 0, 0};
    functions->glGetIntegerv(GL_ACTIVE_TEXTURE, &restoredActiveTexture);
    functions->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &restoredDrawFbo);
    functions->glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoredReadFbo);
    functions->glGetIntegerv(GL_VIEWPORT, restoredViewport);
    restoredSrgb = functions->glIsEnabled(GL_FRAMEBUFFER_SRGB);
    functions->glActiveTexture(restoredActiveTexture);
    functions->glGetIntegerv(GL_TEXTURE_BINDING_2D, &restoredTextureBinding);

    framebufferRestored = restoredDrawFbo == prevDrawFbo && restoredReadFbo == prevReadFbo;
    viewportRestored = restoredViewport[0] == prevViewport[0]
        && restoredViewport[1] == prevViewport[1] && restoredViewport[2] == prevViewport[2]
        && restoredViewport[3] == prevViewport[3];
    textureRestored = restoredTextureBinding == prevTextureBinding
        && prevActiveTexture == restoredActiveTexture;
    srgbRestored = static_cast<int>(restoredSrgb) == static_cast<int>(prevSrgbEnabled);
    result.framebufferSrgbAfter = static_cast<int>(restoredSrgb);

    result.framebufferRestored = static_cast<int>(framebufferRestored);
    result.viewportRestored = static_cast<int>(viewportRestored);
    result.textureRestored = static_cast<int>(textureRestored);
    result.framebufferSrgbRestored = static_cast<int>(srgbRestored);
    result.objectsDeleted = static_cast<int>(objectsDeleted);
    printDocumentPostprocessDepthProfile(result);

    const bool supportedOutcome = result.available && result.sampleableDepth && objectsDeleted;
    const bool fallbackOutcome = !result.available && result.fallback == std::string_view("native")
        && objectsDeleted;
    relationOk = relationOk && framebufferRestored && viewportRestored && textureRestored
        && srgbRestored && (forceNativeFallback || supportedOutcome || fallbackOutcome);
    printDocumentPostprocessDepthRelation(result, relationOk);

    if (!relationOk) {
        std::fprintf(stderr, "Document postprocess depth probe relation check failed.\n");
    }
    return relationOk;
}

#endif

}  // namespace

int main(int argc, char* argv[])
{
    QTemporaryDir isolatedUserHome(
        QDir::tempPath() + QStringLiteral("/freecad-gpu-diagnostics-XXXXXX")
    );
    if (!isolatedUserHome.isValid()) {
        QTextStream(stderr) << "Could not create an isolated FreeCAD user directory.\n";
        return 2;
    }
    qputenv("FREECAD_USER_HOME", isolatedUserHome.path().toUtf8());

    std::array<char*, 1> freecadArgv {argv[0]};
    App::Application::Config()["ExeName"] = "GpuDiagnosticsHarness";
    App::Application::init(static_cast<int>(freecadArgv.size()), freecadArgv.data());
    Gui::Application::initApplication();
    Gui::Application::setupDefaultSurfaceFormat();

    const bool forceLiveSceneAa = std::any_of(argv + 1, argv + argc, [](const char* argument) {
        return QByteArrayView(argument) == QByteArrayView("--scene-aa-live-force");
    });
    int forcedWidgetSamples = 0;
    if (forceLiveSceneAa) {
        for (int index = 1; index + 1 < argc; ++index) {
            if (QByteArrayView(argv[index]) != QByteArrayView("--samples")) {
                continue;
            }
            bool samplesOk = false;
            const int samples = QByteArray(argv[index + 1]).toInt(&samplesOk);
            forcedWidgetSamples = samplesOk ? samples : 0;
            break;
        }
    }

    QApplication qtApplication(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GpuDiagnosticsHarness"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Focused FreeCAD production-viewport GPU diagnostics harness")
    );
    parser.addHelpOption();
    const QCommandLineOption jsonOption(
        QStringLiteral("json"),
        QStringLiteral("Write the production JSON diagnostics report to stdout.")
    );
    const QCommandLineOption textOption(
        QStringLiteral("text"),
        QStringLiteral("Write the production text diagnostics report to stdout.")
    );
    const QCommandLineOption dialogOption(
        QStringLiteral("dialog"),
        QStringLiteral("Open the production GPU Diagnostics dialog.")
    );
    const QCommandLineOption samplesOption(
        QStringLiteral("samples"),
        QStringLiteral("Request 0, 1, 2, 4, 6, or 8 samples in the isolated preferences."),
        QStringLiteral("count")
    );
    const QCommandLineOption autoExitOption(
        QStringLiteral("auto-exit-ms"),
        QStringLiteral("Bound the run and close dialogs after this many milliseconds."),
        QStringLiteral("milliseconds"),
        QString::number(defaultAutoExitMs)
    );
    const QCommandLineOption unrelatedWidgetOption(
        QStringLiteral("unrelated-widget"),
        QStringLiteral("Create an unrelated QOpenGLWidget to verify viewport filtering.")
    );
    const QCommandLineOption brepFixtureOption(
        QStringLiteral("brep-fixture"),
        QStringLiteral("Render a deterministic production SoBrepEdgeSet fixture.")
    );
    const QCommandLineOption brepDocumentFixtureOption(
        QStringLiteral("brep-document-fixture"),
        QStringLiteral(
            "Use rounded, selected, preselected, and hidden Part document objects for the fixture."
        )
    );
    const QCommandLineOption brepDocumentLightingMaterialOption(
        QStringLiteral("brep-document-lighting-material"),
        QStringLiteral("Run deterministic lighting/material matrix checks on the document fixture.")
    );
    const QCommandLineOption brepDocumentTessellationOption(
        QStringLiteral("brep-document-tessellation"),
        QStringLiteral("Run adaptive tessellation probes on the document BRep fixture.")
    );
    const QCommandLineOption brepDocumentPostprocessDepthOption(
        QStringLiteral("brep-document-postprocess-depth"),
        QStringLiteral("Run deterministic postprocess depth attachment probe on the document fixture.")
    );
    const QCommandLineOption brepDocumentPostprocessDepthForceNativeOption(
        QStringLiteral("brep-document-postprocess-depth-force-native"),
        QStringLiteral("Force document postprocess depth probe to use native fallback.")
    );
    const QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Save the rendered viewport to this path."),
        QStringLiteral("path")
    );
    const QCommandLineOption sceneAaSamplesOption(
        QStringLiteral("scene-aa-samples"),
        QStringLiteral("Render a scene-only capture through an owned FBO with this sample count."),
        QStringLiteral("count")
    );
    const QCommandLineOption sceneAaStatsOption(
        QStringLiteral("scene-aa-stats"),
        QStringLiteral("Report owned-FBO sample and timing statistics to stderr.")
    );
    const QCommandLineOption sceneAaLiveOption(
        QStringLiteral("scene-aa-live"),
        QStringLiteral("Enable the developer-gated live owned-MSAA-FBO presentation path.")
    );
    const QCommandLineOption sceneAaLiveForceOption(
        QStringLiteral("scene-aa-live-force"),
        QStringLiteral("Force the live owned-MSAA-FBO path with a single-sample Qt widget.")
    );
    const QCommandLineOption sceneAaLiveStatsOption(
        QStringLiteral("scene-aa-live-stats"),
        QStringLiteral("Report live owned-FBO activation and GPU-complete timing statistics.")
    );
    const QCommandLineOption sceneAaLiveResizeOption(
        QStringLiteral("scene-aa-live-resize"),
        QStringLiteral("Resize the settled live-AA viewport to exercise FBO reallocation.")
    );
    const QCommandLineOption sceneAaLiveFallbackTransitionOption(
        QStringLiteral("scene-aa-live-fallback-transition"),
        QStringLiteral("Switch from an allocated live-AA frame to the native zero-sample fallback.")
    );
    const QCommandLineOption frameStatsOption(
        QStringLiteral("frame-stats"),
        QStringLiteral("Report five GPU-complete settled frame timings without readback.")
    );
    const QCommandLineOption brepOverlaysOption(
        QStringLiteral("brep-overlays"),
        QStringLiteral("Add deterministic BRep highlight and selection overlay lines.")
    );
    const QCommandLineOption brepDuplicateEdgesOption(
        QStringLiteral("brep-duplicate-edges"),
        QStringLiteral("Add reversed duplicate BRep edge segments to the fixture.")
    );
    const QCommandLineOption brepDenseFixtureOption(
        QStringLiteral("brep-dense-fixture"),
        QStringLiteral("Expand the BRep fixture to a deterministic 10x10 cube grid.")
    );
    const QCommandLineOption edgeAaModeOption(
        QStringLiteral("edge-aa-mode"),
        QStringLiteral("Select an opt-in production BRep edge diagnostic mode."),
        QStringLiteral("mode"),
        QStringLiteral("disabled")
    );
    const QCommandLineOption edgeAaDedupToleranceOption(
        QStringLiteral("edge-aa-dedup-tolerance"),
        QStringLiteral("Set the projected segment de-duplication tolerance in pixels."),
        QStringLiteral("pixels"),
        QStringLiteral("0.5")
    );
    const QCommandLineOption edgeAaStatsOption(
        QStringLiteral("edge-aa-stats"),
        QStringLiteral("Report projected segment de-duplication statistics to stderr.")
    );
    const QCommandLineOption edgeAaShaderWidthOption(
        QStringLiteral("edge-aa-shader-width"),
        QStringLiteral("Set the shader edge core width in logical pixels."),
        QStringLiteral("pixels"),
        QStringLiteral("1.5")
    );
    parser.addOptions({
        jsonOption,
        textOption,
        dialogOption,
        samplesOption,
        autoExitOption,
        unrelatedWidgetOption,
        brepFixtureOption,
        brepDocumentFixtureOption,
        brepDocumentLightingMaterialOption,
        screenshotOption,
        brepOverlaysOption,
        brepDuplicateEdgesOption,
        brepDenseFixtureOption,
        brepDocumentTessellationOption,
        brepDocumentPostprocessDepthOption,
        brepDocumentPostprocessDepthForceNativeOption,
        edgeAaModeOption,
        edgeAaDedupToleranceOption,
        edgeAaStatsOption,
        edgeAaShaderWidthOption,
        sceneAaSamplesOption,
        sceneAaStatsOption,
        sceneAaLiveOption,
        sceneAaLiveForceOption,
        sceneAaLiveStatsOption,
        sceneAaLiveResizeOption,
        sceneAaLiveFallbackTransitionOption,
        frameStatsOption,
    });
    parser.process(qtApplication);

    bool millisecondsOk = false;
    const int requestedAutoExitMs = parser.value(autoExitOption).toInt(&millisecondsOk);
    if (!millisecondsOk || requestedAutoExitMs <= 0) {
        QTextStream(stderr) << "--auto-exit-ms must be a positive integer.\n";
        App::Application::destruct();
        return 2;
    }
    const int autoExitMs = std::min(requestedAutoExitMs, maximumAutoExitMs);

    const auto edgeAaMode = parser.value(edgeAaModeOption);
    const std::array<QString, 12> supportedEdgeAaModes {
        QStringLiteral("disabled"),
        QStringLiteral("hide"),
        QStringLiteral("line-smooth"),
        QStringLiteral("line-smooth-off"),
        QStringLiteral("dedup-screen-space"),
        QStringLiteral("shader-screen-space"),
        QStringLiteral("shader-screen-space-dedup"),
        QStringLiteral("shader-screen-space-overlay"),
        QStringLiteral("screen-space-debug"),
        QStringLiteral("screen-space-only"),
        QStringLiteral("screen-space-overlay"),
        QStringLiteral("suppress-overlays"),
    };
    if (std::find(supportedEdgeAaModes.begin(), supportedEdgeAaModes.end(), edgeAaMode)
        == supportedEdgeAaModes.end()) {
        QTextStream(stderr) << "--edge-aa-mode must be disabled, hide, line-smooth, "
                               "line-smooth-off, dedup-screen-space, screen-space-debug, "
                               "screen-space-only, screen-space-overlay, shader-screen-space, "
                               "shader-screen-space-dedup, shader-screen-space-overlay, or "
                               "suppress-overlays.\n";
        App::Application::destruct();
        return 2;
    }
    if (edgeAaMode == QStringLiteral("disabled")) {
        qunsetenv("FREECAD_EDGE_AA_DIAGNOSTIC");
    }
    else {
        qputenv("FREECAD_EDGE_AA_DIAGNOSTIC", edgeAaMode.toUtf8());
    }

    const bool shaderMode = edgeAaMode.startsWith(QStringLiteral("shader-screen-space"));
    const bool dedupMode = edgeAaMode == QStringLiteral("dedup-screen-space")
        || edgeAaMode == QStringLiteral("shader-screen-space-dedup")
        || edgeAaMode == QStringLiteral("shader-screen-space-overlay");
    bool dedupToleranceOk = false;
    const double dedupTolerance = parser.value(edgeAaDedupToleranceOption).toDouble(&dedupToleranceOk);
    if (!dedupToleranceOk || !std::isfinite(dedupTolerance) || dedupTolerance <= 0.0) {
        QTextStream(stderr) << "--edge-aa-dedup-tolerance must be a positive number.\n";
        App::Application::destruct();
        return 2;
    }
    if (dedupMode) {
        qputenv("FREECAD_EDGE_AA_DEDUP_TOLERANCE_PX", QByteArray::number(dedupTolerance, 'g', 15));
    }
    else {
        qunsetenv("FREECAD_EDGE_AA_DEDUP_TOLERANCE_PX");
    }
    bool shaderWidthOk = false;
    const double shaderWidth = parser.value(edgeAaShaderWidthOption).toDouble(&shaderWidthOk);
    if (!shaderWidthOk || !std::isfinite(shaderWidth) || shaderWidth <= 0.0) {
        QTextStream(stderr) << "--edge-aa-shader-width must be a positive number.\n";
        App::Application::destruct();
        return 2;
    }
    if (shaderMode) {
        qputenv("FREECAD_EDGE_AA_SHADER_WIDTH_PX", QByteArray::number(shaderWidth, 'g', 15));
    }
    else {
        qunsetenv("FREECAD_EDGE_AA_SHADER_WIDTH_PX");
    }
    // Enable statistics only for the settled evidence redraw so early camera
    // setup frames do not produce misleading counts.
    qunsetenv("FREECAD_EDGE_AA_DIAGNOSTIC_STATS");

    if (parser.isSet(brepOverlaysOption) && !parser.isSet(brepFixtureOption)) {
        QTextStream(stderr) << "--brep-overlays requires --brep-fixture.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(brepDuplicateEdgesOption) && !parser.isSet(brepFixtureOption)) {
        QTextStream(stderr) << "--brep-duplicate-edges requires --brep-fixture.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(brepDenseFixtureOption) && !parser.isSet(brepFixtureOption)) {
        QTextStream(stderr) << "--brep-dense-fixture requires --brep-fixture.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(brepDocumentFixtureOption)
        && (parser.isSet(brepDenseFixtureOption) || parser.isSet(brepDuplicateEdgesOption))) {
        QTextStream(
            stderr
        ) << "--brep-document-fixture cannot be combined with dense or duplicate-edge fixtures.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(edgeAaStatsOption) && edgeAaMode != QStringLiteral("dedup-screen-space")
        && !shaderMode) {
        QTextStream(
            stderr
        ) << "--edge-aa-stats requires a de-duplication or shader screen-space mode.\n";
        App::Application::destruct();
        return 2;
    }

    int sceneAaSamples = 0;
    if (parser.isSet(sceneAaSamplesOption)) {
        bool sceneAaSamplesOk = false;
        sceneAaSamples = parser.value(sceneAaSamplesOption).toInt(&sceneAaSamplesOk);
        constexpr std::array<int, 4> supportedSceneAaSamples {0, 2, 4, 8};
        if (!sceneAaSamplesOk
            || std::find(supportedSceneAaSamples.begin(), supportedSceneAaSamples.end(), sceneAaSamples)
                == supportedSceneAaSamples.end()) {
            QTextStream(stderr) << "--scene-aa-samples must be one of 0, 2, 4, or 8.\n";
            App::Application::destruct();
            return 2;
        }
        if (!parser.isSet(brepFixtureOption)) {
            QTextStream(stderr) << "--scene-aa-samples requires --brep-fixture.\n";
            App::Application::destruct();
            return 2;
        }
    }
    if (parser.isSet(sceneAaStatsOption) && !parser.isSet(sceneAaSamplesOption)) {
        QTextStream(stderr) << "--scene-aa-stats requires --scene-aa-samples.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(sceneAaLiveOption) && !parser.isSet(brepFixtureOption)) {
        QTextStream(stderr) << "--scene-aa-live requires --brep-fixture.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(sceneAaLiveForceOption)
        && (!parser.isSet(sceneAaLiveOption) || !parser.isSet(brepFixtureOption))) {
        QTextStream(
            stderr
        ) << "--scene-aa-live-force requires --scene-aa-live and --brep-fixture.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(sceneAaLiveStatsOption) && !parser.isSet(sceneAaLiveOption)) {
        QTextStream(stderr) << "--scene-aa-live-stats requires --scene-aa-live.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(sceneAaLiveResizeOption) && !parser.isSet(sceneAaLiveOption)) {
        QTextStream(stderr) << "--scene-aa-live-resize requires --scene-aa-live.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(sceneAaLiveFallbackTransitionOption)
        && (!parser.isSet(sceneAaLiveOption) || !parser.isSet(sceneAaLiveStatsOption))) {
        QTextStream(stderr) << "--scene-aa-live-fallback-transition requires --scene-aa-live and "
                               "--scene-aa-live-stats.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(frameStatsOption) && !parser.isSet(brepFixtureOption)) {
        QTextStream(stderr) << "--frame-stats requires --brep-fixture.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(brepDocumentTessellationOption) && !parser.isSet(brepDocumentFixtureOption)) {
        QTextStream(stderr) << "--brep-document-tessellation requires --brep-document-fixture.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(brepDocumentLightingMaterialOption) && !parser.isSet(brepDocumentFixtureOption)) {
        QTextStream(
            stderr
        ) << "--brep-document-lighting-material requires --brep-document-fixture.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(brepDocumentPostprocessDepthOption) && !parser.isSet(brepDocumentFixtureOption)) {
        QTextStream(
            stderr
        ) << "--brep-document-postprocess-depth requires --brep-document-fixture.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(brepDocumentPostprocessDepthForceNativeOption)
        && !parser.isSet(brepDocumentPostprocessDepthOption)) {
        QTextStream(stderr) << "--brep-document-postprocess-depth-force-native requires "
                            << "--brep-document-postprocess-depth.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(sceneAaLiveOption)) {
        qputenv(
            "FREECAD_SCENE_AA_LIVE",
            parser.isSet(sceneAaLiveForceOption) ? QByteArrayLiteral("force") : QByteArrayLiteral("1")
        );
    }
    else {
        qunsetenv("FREECAD_SCENE_AA_LIVE");
    }
    qunsetenv("FREECAD_SCENE_AA_LIVE_STATS");

    if (parser.isSet(samplesOption)) {
        bool samplesOk = false;
        const int samples = parser.value(samplesOption).toInt(&samplesOk);
        constexpr std::array<int, 6> supportedSamples {0, 1, 2, 4, 6, 8};
        if (!samplesOk
            || std::find(supportedSamples.begin(), supportedSamples.end(), samples)
                == supportedSamples.end()) {
            QTextStream(stderr) << "--samples must be one of 0, 1, 2, 4, 6, or 8.\n";
            App::Application::destruct();
            return 2;
        }
        Gui::Multisample::writeMSAAToSettings(Gui::Multisample::toAntiAliasing(samples));
    }

    Gui::Application::initOpenInventor();

#ifndef FREECAD_GPU_DIAGNOSTICS_HAS_PART
    if (parser.isSet(brepFixtureOption)) {
        QTextStream(stderr) << "--brep-fixture requires a build with the Part workbench.\n";
        App::Application::destruct();
        return 2;
    }
    if (parser.isSet(brepDocumentFixtureOption) || parser.isSet(brepDocumentLightingMaterialOption)
        || parser.isSet(brepDocumentTessellationOption)
        || parser.isSet(brepDocumentPostprocessDepthOption)
        || parser.isSet(brepDocumentPostprocessDepthForceNativeOption)) {
        QTextStream(
            stderr
        ) << "--brep-document-* options require a build with the Part workbench.\n";
        App::Application::destruct();
        return 2;
    }
#endif

    if (parser.isSet(brepDocumentFixtureOption)) {
        auto viewParameters = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View"
        );
        viewParameters->SetUnsigned("SelectionColor", 0xFF00FFFF);
        viewParameters->SetUnsigned("HighlightColor", 0x0AC8FFFF);
    }

    int result = 0;
    {
        Gui::Application guiApplication(parser.isSet(brepDocumentFixtureOption));
        bool fixtureInstallFailed = false;
        // The production viewer always creates a navigation cube. Avoid adding its
        // product commands because their active-state checks require MainWindow.
        NaviCube::setNaviCubeCommands({"GpuDiagnosticsHarness_NoCommand"});

        std::unique_ptr<Gui::View3DInventorViewer> viewOwner;
        if (forceLiveSceneAa && forcedWidgetSamples > 0) {
            QSurfaceFormat format;
            format.setSamples(forcedWidgetSamples);
            viewOwner = std::make_unique<Gui::View3DInventorViewer>(format, nullptr);
        }
        else {
            viewOwner = std::make_unique<Gui::View3DInventorViewer>(nullptr);
        }
        auto& view = *viewOwner;
        view.setWindowTitle(QStringLiteral("FreeCAD GPU Diagnostics Harness"));
        view.resize(960, 640);
#ifdef FREECAD_GPU_DIAGNOSTICS_HAS_PART
        if (parser.isSet(brepFixtureOption)) {
            if (parser.isSet(brepDocumentFixtureOption)) {
                if (!installDocumentBrepFixture(view)) {
                    fixtureInstallFailed = true;
                }
            }
            else {
                installBrepFixture(
                    view,
                    parser.isSet(brepOverlaysOption),
                    parser.isSet(brepDuplicateEdgesOption),
                    parser.isSet(brepDenseFixtureOption)
                );
            }
        }
#endif
        view.show();
#ifdef FREECAD_GPU_DIAGNOSTICS_HAS_PART
        if (parser.isSet(brepFixtureOption)) {
            qtApplication.processEvents();
            view.setAnimationEnabled(false);
            view.setCameraOrientation(Gui::Camera::isometric());
            view.viewAll();
            view.redraw();
        }
#endif

        std::unique_ptr<QOpenGLWidget> unrelatedWidget;
        if (parser.isSet(unrelatedWidgetOption)) {
            unrelatedWidget = std::make_unique<QOpenGLWidget>();
            unrelatedWidget->setObjectName(QStringLiteral("GpuDiagnosticsUnrelatedWidget"));
            unrelatedWidget->resize(64, 64);
            unrelatedWidget->show();
        }

        QElapsedTimer contextDeadline;
        contextDeadline.start();
        const int contextWaitMs = std::max(0, autoExitMs - shutdownGraceMs);
        bool reportCollected = false;
        QTimer contextPoll(&view);
        contextPoll.setInterval(25);
        auto collectReport = [&](bool allowDialog) {
            if (reportCollected) {
                return;
            }
            reportCollected = true;
            contextPoll.stop();
            if (parser.isSet(sceneAaLiveResizeOption)) {
                view.resize(800, 600);
                qtApplication.processEvents();
            }
            if (parser.isSet(sceneAaLiveStatsOption)) {
                qputenv("FREECAD_SCENE_AA_LIVE_STATS", "1");
            }
            if (parser.isSet(sceneAaLiveFallbackTransitionOption)) {
                view.getSceneGraph()->touch();
                view.redraw();
                qtApplication.processEvents();
                Gui::Multisample::writeMSAAToSettings(Gui::AntiAliasing::None);
                view.getSceneGraph()->touch();
                view.redraw();
                qtApplication.processEvents();
            }
            if (parser.isSet(frameStatsOption)) {
                auto* glWidget = view.findChild<QOpenGLWidget*>();
                QOpenGLFunctions* functions = glWidget && glWidget->context()
                    ? glWidget->context()->functions()
                    : nullptr;
                if (!glWidget || !functions) {
                    QTextStream(stderr) << "Could not time frames without a current GL context.\n";
                    qtApplication.exit(3);
                    return;
                }
                for (int frame = 1; frame <= 5; ++frame) {
                    glWidget->makeCurrent();
                    functions->glFinish();
                    QElapsedTimer frameTimer;
                    frameTimer.start();
                    view.getSceneGraph()->touch();
                    view.redraw();
                    qtApplication.processEvents();
                    glWidget->makeCurrent();
                    functions->glFinish();
                    std::fprintf(
                        stderr,
                        "FREECAD_SCENE_AA_FRAME_STATS mode=%s frame=%d frame_us=%lld\n",
                        parser.isSet(sceneAaLiveOption) ? "live-owned-fbo" : "native",
                        frame,
                        static_cast<long long>(frameTimer.nsecsElapsed() / 1000)
                    );
                }
            }
#ifdef FREECAD_GPU_DIAGNOSTICS_HAS_PART
            if (parser.isSet(brepDocumentLightingMaterialOption)
                && !runDocumentLightingMaterialProbe(view)) {
                QTextStream(stderr) << "Document BRep lighting/material matrix failed.\n";
                qtApplication.exit(3);
                return;
            }
#endif
            if (parser.isSet(screenshotOption) || parser.isSet(sceneAaSamplesOption)) {
                view.redraw();
                qtApplication.processEvents();
                QImage screenshot;
                if (parser.isSet(sceneAaSamplesOption)) {
                    Gui::View3DInventorViewer::RenderImageOptions options;
                    options.samples = sceneAaSamples;
                    options.intent = Gui::View3DInventorViewer::RenderIntent::RasterCapture;
                    if (parser.isSet(sceneAaStatsOption)) {
                        qputenv("FREECAD_SCENE_AA_DIAGNOSTIC_STATS", "1");
                    }
                    screenshot = view.renderToImage(options);
                    qunsetenv("FREECAD_SCENE_AA_DIAGNOSTIC_STATS");
                    if (screenshot.isNull()) {
                        std::fprintf(
                            stderr,
                            "FREECAD_SCENE_AA_FALLBACK requested_samples=%d "
                            "method=live-framebuffer\n",
                            sceneAaSamples
                        );
                        screenshot = view.grabFramebuffer();
                    }
                }
                else {
                    screenshot = view.grabFramebuffer();
                }
#ifdef FREECAD_GPU_DIAGNOSTICS_HAS_PART
                if (parser.isSet(brepDocumentFixtureOption)
                    && !validateDocumentBrepFrame(screenshot)) {
                    QTextStream(stderr) << "Document BRep fixture frame validation failed.\n";
                    qtApplication.exit(3);
                    return;
                }
#endif
                const auto screenshotPath = parser.value(screenshotOption);
                if (screenshot.isNull()
                    || (parser.isSet(screenshotOption) && !screenshot.save(screenshotPath))) {
                    QTextStream(stderr)
                        << "Could not save viewport screenshot to " << screenshotPath << ".\n";
                    qtApplication.exit(3);
                    return;
                }
            }
            if (parser.isSet(sceneAaLiveOption)) {
                view.getSceneGraph()->touch();
                view.redraw();
                qtApplication.processEvents();
            }
#ifdef FREECAD_GPU_DIAGNOSTICS_HAS_PART
            if (parser.isSet(brepDocumentTessellationOption) && !runDocumentTessellationProbe()) {
                QTextStream(stderr) << "Document BRep tessellation probe failed.\n";
                qtApplication.exit(3);
                return;
            }
            if (parser.isSet(brepDocumentPostprocessDepthOption)
                && !runDocumentPostprocessDepthProbe(
                    view,
                    parser.isSet(brepDocumentPostprocessDepthForceNativeOption)
                )) {
                QTextStream(stderr) << "Document postprocess depth probe failed.\n";
                qtApplication.exit(3);
                return;
            }
#endif
            qunsetenv("FREECAD_SCENE_AA_LIVE_STATS");
            const auto report = Gui::GpuDiagnostics::collect(&view);
            if (parser.isSet(edgeAaStatsOption)) {
                qputenv("FREECAD_EDGE_AA_DIAGNOSTIC_STATS", "1");
                view.getSceneGraph()->touch();
                view.redraw();
                qtApplication.processEvents();
            }
            qunsetenv("FREECAD_EDGE_AA_DIAGNOSTIC_STATS");
            QTextStream output(stdout);
            if (parser.isSet(jsonOption)) {
                output << Gui::GpuDiagnostics::toJson(report);
            }
            if (parser.isSet(textOption)) {
                output << Gui::GpuDiagnostics::toText(report);
            }
            output.flush();

            const bool showDialog = allowDialog
                && (parser.isSet(dialogOption)
                    || (!parser.isSet(jsonOption) && !parser.isSet(textOption)));
            if (showDialog) {
                Gui::GpuDiagnosticsDialog::showDialog(&view);
            }
            qtApplication.quit();
        };
        QObject::connect(&contextPoll, &QTimer::timeout, [&]() {
            auto* glWidget = view.findChild<QOpenGLWidget*>();
            const bool contextReady = glWidget && glWidget->context() && glWidget->isValid();
            const bool fixtureReady = !parser.isSet(brepFixtureOption)
                || contextDeadline.elapsed() >= fixtureSettleMs;
            if ((contextReady && fixtureReady) || contextDeadline.elapsed() >= contextWaitMs) {
                collectReport(true);
            }
        });
        if (!fixtureInstallFailed) {
            contextPoll.start();
        }
        QTimer shutdownTimer(&view);
        shutdownTimer.setInterval(autoExitMs);
        QObject::connect(&shutdownTimer, &QTimer::timeout, [&]() {
            // Collection performed while opening the dialog can span the deadline. Keep
            // polling after it so the modal dialog is rejected as soon as it enters exec().
            shutdownTimer.setInterval(25);
            collectReport(false);
            closeDiagnosticsDialogs();
            qtApplication.quit();
        });
        if (fixtureInstallFailed) {
            QTimer::singleShot(0, &qtApplication, [&qtApplication]() { qtApplication.exit(3); });
        }
        else {
            shutdownTimer.start();
        }

        result = qtApplication.exec();
        view.close();
    }

    App::Application::destruct();
    return result;
}
