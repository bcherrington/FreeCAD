// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <array>
#include <memory>

#include <QApplication>
#include <QCommandLineParser>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QImage>
#include <QOpenGLWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>

#include <App/Application.h>

#include <Gui/Application.h>
#include <Gui/Camera.h>
#include <Gui/GpuDiagnostics.h>
#include <Gui/Multisample.h>
#include <Gui/NaviCube.h>
#include <Gui/View3DInventorViewer.h>

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
void installBrepFixture(Gui::View3DInventorViewer& view, bool includeOverlays)
{
    if (PartGui::SoBrepEdgeSet::getClassTypeId() == SoType::badType()) {
        PartGui::SoBrepEdgeSet::initClass();
    }

    const std::array<SbVec3f, 8> points {
        SbVec3f(-5.0F, -5.0F, -5.0F),
        SbVec3f(5.0F, -5.0F, -5.0F),
        SbVec3f(5.0F, 5.0F, -5.0F),
        SbVec3f(-5.0F, 5.0F, -5.0F),
        SbVec3f(-5.0F, -5.0F, 5.0F),
        SbVec3f(5.0F, -5.0F, 5.0F),
        SbVec3f(5.0F, 5.0F, 5.0F),
        SbVec3f(-5.0F, 5.0F, 5.0F),
    };
    constexpr std::array<int32_t, 36> edgeIndices {
        0, 1, -1, 1, 2, -1, 2, 3, -1, 3, 0, -1, 4, 5, -1, 5, 6, -1,
        6, 7, -1, 7, 4, -1, 0, 4, -1, 1, 5, -1, 2, 6, -1, 3, 7, -1,
    };

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

    auto* faces = new SoCube;
    faces->width = 10.0F;
    faces->height = 10.0F;
    faces->depth = 10.0F;
    faceRoot->addChild(faces);
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

    auto* viewerRoot = static_cast<SoSeparator*>(view.getSceneGraph());
    auto* modelRoot = static_cast<SoSeparator*>(viewerRoot->getChild(1));
    SoGroup* objectRoot = modelRoot;
    for (int index = 0; index < modelRoot->getNumChildren(); ++index) {
        auto* child = modelRoot->getChild(index);
        if (child->getName() == SbName("ObjectGroup")
            && child->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
            objectRoot = static_cast<SoGroup*>(child);
            break;
        }
    }
    objectRoot->addChild(root);
    root->unref();
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
    const QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Save the rendered viewport to this path."),
        QStringLiteral("path")
    );
    const QCommandLineOption brepOverlaysOption(
        QStringLiteral("brep-overlays"),
        QStringLiteral("Add deterministic BRep highlight and selection overlay lines.")
    );
    const QCommandLineOption edgeAaModeOption(
        QStringLiteral("edge-aa-mode"),
        QStringLiteral("Select an opt-in production BRep edge diagnostic mode."),
        QStringLiteral("mode"),
        QStringLiteral("disabled")
    );
    parser.addOptions({
        jsonOption,
        textOption,
        dialogOption,
        samplesOption,
        autoExitOption,
        unrelatedWidgetOption,
        brepFixtureOption,
        screenshotOption,
        brepOverlaysOption,
        edgeAaModeOption,
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
    const std::array<QString, 8> supportedEdgeAaModes {
        QStringLiteral("disabled"),
        QStringLiteral("hide"),
        QStringLiteral("line-smooth"),
        QStringLiteral("line-smooth-off"),
        QStringLiteral("screen-space-debug"),
        QStringLiteral("screen-space-only"),
        QStringLiteral("screen-space-overlay"),
        QStringLiteral("suppress-overlays"),
    };
    if (std::find(supportedEdgeAaModes.begin(), supportedEdgeAaModes.end(), edgeAaMode)
        == supportedEdgeAaModes.end()) {
        QTextStream(stderr) << "--edge-aa-mode must be disabled, hide, line-smooth, "
                               "line-smooth-off, screen-space-debug, screen-space-only, "
                               "screen-space-overlay, or suppress-overlays.\n";
        App::Application::destruct();
        return 2;
    }
    if (edgeAaMode == QStringLiteral("disabled")) {
        qunsetenv("FREECAD_EDGE_AA_DIAGNOSTIC");
    }
    else {
        qputenv("FREECAD_EDGE_AA_DIAGNOSTIC", edgeAaMode.toUtf8());
    }

    if (parser.isSet(brepOverlaysOption) && !parser.isSet(brepFixtureOption)) {
        QTextStream(stderr) << "--brep-overlays requires --brep-fixture.\n";
        App::Application::destruct();
        return 2;
    }

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

    int result = 0;
    {
        Gui::Application guiApplication(false);
        // The production viewer always creates a navigation cube. Avoid adding its
        // product commands because their active-state checks require MainWindow.
        NaviCube::setNaviCubeCommands({"GpuDiagnosticsHarness_NoCommand"});

        Gui::View3DInventorViewer view(nullptr);
        view.setWindowTitle(QStringLiteral("FreeCAD GPU Diagnostics Harness"));
        view.resize(960, 640);
#ifdef FREECAD_GPU_DIAGNOSTICS_HAS_PART
        if (parser.isSet(brepFixtureOption)) {
            installBrepFixture(view, parser.isSet(brepOverlaysOption));
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
            if (parser.isSet(screenshotOption)) {
                view.redraw();
                const auto screenshot = view.grabFramebuffer();
                const auto screenshotPath = parser.value(screenshotOption);
                if (screenshot.isNull() || !screenshot.save(screenshotPath)) {
                    QTextStream(stderr)
                        << "Could not save viewport screenshot to " << screenshotPath << ".\n";
                    qtApplication.exit(3);
                    return;
                }
            }
            const auto report = Gui::GpuDiagnostics::collect(&view);
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
        contextPoll.start();
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
        shutdownTimer.start();

        result = qtApplication.exec();
        view.close();
    }

    App::Application::destruct();
    return result;
}
