// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <array>
#include <memory>

#include <QApplication>
#include <QCommandLineParser>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>

#include <App/Application.h>

#include <Gui/Application.h>
#include <Gui/GpuDiagnostics.h>
#include <Gui/Multisample.h>
#include <Gui/NaviCube.h>
#include <Gui/View3DInventorViewer.h>

namespace
{

constexpr int defaultAutoExitMs = 2000;
constexpr int maximumAutoExitMs = 60000;
constexpr int shutdownGraceMs = 250;

void closeDiagnosticsDialogs()
{
    for (auto* widget : QApplication::topLevelWidgets()) {
        if (auto* dialog = qobject_cast<QDialog*>(widget)) {
            dialog->reject();
        }
    }
}

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
    parser.addOptions(
        {jsonOption, textOption, dialogOption, samplesOption, autoExitOption, unrelatedWidgetOption}
    );
    parser.process(qtApplication);

    bool millisecondsOk = false;
    const int requestedAutoExitMs = parser.value(autoExitOption).toInt(&millisecondsOk);
    if (!millisecondsOk || requestedAutoExitMs <= 0) {
        QTextStream(stderr) << "--auto-exit-ms must be a positive integer.\n";
        App::Application::destruct();
        return 2;
    }
    const int autoExitMs = std::min(requestedAutoExitMs, maximumAutoExitMs);

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

    int result = 0;
    {
        Gui::Application guiApplication(false);
        // The production viewer always creates a navigation cube. Avoid adding its
        // product commands because their active-state checks require MainWindow.
        NaviCube::setNaviCubeCommands({"GpuDiagnosticsHarness_NoCommand"});

        Gui::View3DInventorViewer view(nullptr);
        view.setWindowTitle(QStringLiteral("FreeCAD GPU Diagnostics Harness"));
        view.resize(960, 640);
        view.show();

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
            if (contextReady || contextDeadline.elapsed() >= contextWaitMs) {
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
