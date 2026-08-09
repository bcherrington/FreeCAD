// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string_view>
#include <vector>

#include <QApplication>
#include <QCommandLineParser>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QImage>
#include <QOpenGLFunctions>
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

# include <Mod/Part/Gui/SoBrepEdgeSet.h>
#endif

namespace
{

constexpr int defaultAutoExitMs = 2000;
constexpr int maximumAutoExitMs = 60000;
constexpr int shutdownGraceMs = 250;
constexpr int fixtureSettleMs = 250;

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
    constexpr auto documentName = "GpuDiagnosticsBrepDocument";
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
    constexpr std::array<const char*, 3> objectNames {
        "RoundedHousing",
        "RoundedBoss",
        "HiddenRoundedSolid",
    };
    for (const char* objectName : objectNames) {
        App::DocumentObject* object = appDocument->getObject(objectName);
        Gui::ViewProvider* provider = object ? Gui::Application::Instance->getViewProvider(object)
                                             : nullptr;
        if (!provider || !provider->getRoot()) {
            std::fprintf(stderr, "Document BRep fixture is missing %s.\n", objectName);
            return false;
        }
        if (std::string_view(objectName) == "HiddenRoundedSolid" && provider->isShow()) {
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
    return true;
}

bool validateDocumentBrepFrame(const QImage& frame)
{
    const QImage image = frame.convertToFormat(QImage::Format_RGB32);
    const int centerLeft = image.width() / 2 - 80;
    const int centerRight = image.width() / 2 + 80;
    const int centerTop = image.height() / 2 - 80;
    const int centerBottom = image.height() / 2 + 80;
    int visibleCenterPixels = 0;
    int hiddenRedPixels = 0;
    int navigationCubePixels = 0;
    int selectionPixels = 0;
    int preselectionPixels = 0;

    for (int y = 0; y < image.height(); ++y) {
        const auto* pixels = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const int red = qRed(pixels[x]);
            const int green = qGreen(pixels[x]);
            const int blue = qBlue(pixels[x]);
            if (x >= centerLeft && x < centerRight && y >= centerTop && y < centerBottom
                && red < 210 && green < 210 && blue < 220) {
                ++visibleCenterPixels;
            }
            if (red > 140 && red > green + 40 && red > blue + 30) {
                ++hiddenRedPixels;
            }
            if (red > 200 && green < 80 && blue > 200) {
                ++selectionPixels;
            }
            if (red < 80 && green > 150 && blue > 220) {
                ++preselectionPixels;
            }
            if (x >= image.width() - 180 && y < 160
                && std::max({red, green, blue}) - std::min({red, green, blue}) < 20
                && std::max({red, green, blue}) > 80 && std::max({red, green, blue}) < 240) {
                ++navigationCubePixels;
            }
        }
    }

    std::fprintf(
        stderr,
        "FREECAD_BREP_DOCUMENT_FRAME visible_center_pixels=%d hidden_red_pixels=%d "
        "navigation_cube_pixels=%d selection_pixels=%d preselection_pixels=%d\n",
        visibleCenterPixels,
        hiddenRedPixels,
        navigationCubePixels,
        selectionPixels,
        preselectionPixels
    );
    return visibleCenterPixels >= 5000 && hiddenRedPixels <= image.width() * image.height() / 200
        && navigationCubePixels >= 25 && selectionPixels >= 1000 && preselectionPixels >= 1000;
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
        screenshotOption,
        brepOverlaysOption,
        brepDuplicateEdgesOption,
        brepDenseFixtureOption,
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
            else if (parser.isSet(sceneAaLiveOption)) {
                view.getSceneGraph()->touch();
                view.redraw();
                qtApplication.processEvents();
            }
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
