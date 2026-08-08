// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "GpuDiagnostics.h"

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSurfaceFormat>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWindow>

#include <App/Application.h>

#include "Multisample.h"
#include "ViewParams.h"

#if HAVE_CONFIG_H
# include <config.h>
# ifdef HAVE_GL_GL_H
#  include <GL/gl.h>
# endif
#endif

namespace
{

QString rectToString(const QRect& rect)
{
    return QStringLiteral("%1x%2+%3+%4").arg(rect.width()).arg(rect.height()).arg(rect.x()).arg(rect.y());
}

QString sizeToString(const QSize& size)
{
    return QStringLiteral("%1x%2").arg(size.width()).arg(size.height());
}

QString sizeToString(const QSizeF& size)
{
    return QStringLiteral("%1x%2").arg(size.width()).arg(size.height());
}

QString optionalIntToString(const std::optional<int>& value)
{
    return value ? QString::number(*value) : QObject::tr("Unavailable");
}

QString optionalUIntToString(const std::optional<unsigned int>& value)
{
    return value ? QString::number(*value) : QObject::tr("Unavailable");
}

QString optionalBoolToString(const std::optional<bool>& value)
{
    if (!value) {
        return QObject::tr("Unavailable");
    }
    return *value ? QObject::tr("Enabled") : QObject::tr("Disabled");
}

QString optionalDoubleToString(const std::optional<double>& value)
{
    return value ? QString::number(*value, 'f', 2) : QObject::tr("Unavailable");
}

QString lineSmoothHintToString(const std::optional<int>& value)
{
    if (!value) {
        return QObject::tr("Unavailable");
    }

    switch (*value) {
        case GL_FASTEST:
            return QStringLiteral("GL_FASTEST");
        case GL_NICEST:
            return QStringLiteral("GL_NICEST");
        case GL_DONT_CARE:
            return QStringLiteral("GL_DONT_CARE");
    }

    return QString::number(*value);
}

QString blendFactorToString(const std::optional<int>& value)
{
    if (!value) {
        return QObject::tr("Unavailable");
    }

    switch (*value) {
        case GL_ZERO:
            return QStringLiteral("GL_ZERO");
        case GL_ONE:
            return QStringLiteral("GL_ONE");
        case GL_SRC_COLOR:
            return QStringLiteral("GL_SRC_COLOR");
        case GL_ONE_MINUS_SRC_COLOR:
            return QStringLiteral("GL_ONE_MINUS_SRC_COLOR");
        case GL_DST_COLOR:
            return QStringLiteral("GL_DST_COLOR");
        case GL_ONE_MINUS_DST_COLOR:
            return QStringLiteral("GL_ONE_MINUS_DST_COLOR");
        case GL_SRC_ALPHA:
            return QStringLiteral("GL_SRC_ALPHA");
        case GL_ONE_MINUS_SRC_ALPHA:
            return QStringLiteral("GL_ONE_MINUS_SRC_ALPHA");
        case GL_DST_ALPHA:
            return QStringLiteral("GL_DST_ALPHA");
        case GL_ONE_MINUS_DST_ALPHA:
            return QStringLiteral("GL_ONE_MINUS_DST_ALPHA");
        case GL_CONSTANT_COLOR:
            return QStringLiteral("GL_CONSTANT_COLOR");
        case GL_ONE_MINUS_CONSTANT_COLOR:
            return QStringLiteral("GL_ONE_MINUS_CONSTANT_COLOR");
        case GL_CONSTANT_ALPHA:
            return QStringLiteral("GL_CONSTANT_ALPHA");
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            return QStringLiteral("GL_ONE_MINUS_CONSTANT_ALPHA");
        case GL_SRC_ALPHA_SATURATE:
            return QStringLiteral("GL_SRC_ALPHA_SATURATE");
    }

    return QString::number(*value);
}

QString blendEquationToString(const std::optional<int>& value)
{
    if (!value) {
        return QObject::tr("Unavailable");
    }

    switch (*value) {
        case GL_FUNC_ADD:
            return QStringLiteral("GL_FUNC_ADD");
        case GL_FUNC_SUBTRACT:
            return QStringLiteral("GL_FUNC_SUBTRACT");
        case GL_FUNC_REVERSE_SUBTRACT:
            return QStringLiteral("GL_FUNC_REVERSE_SUBTRACT");
        case GL_MIN:
            return QStringLiteral("GL_MIN");
        case GL_MAX:
            return QStringLiteral("GL_MAX");
    }

    return QString::number(*value);
}

QString antiAliasingModeToString(Gui::AntiAliasing mode)
{
    switch (mode) {
        case Gui::AntiAliasing::None:
            return QObject::tr("None");
        case Gui::AntiAliasing::MSAA1x:
            return QObject::tr("Line smoothing");
        case Gui::AntiAliasing::MSAA2x:
            return QObject::tr("MSAA 2x");
        case Gui::AntiAliasing::MSAA4x:
            return QObject::tr("MSAA 4x");
        case Gui::AntiAliasing::MSAA6x:
            return QObject::tr("MSAA 6x");
        case Gui::AntiAliasing::MSAA8x:
            return QObject::tr("MSAA 8x");
    }
    return QString::number(static_cast<int>(mode));
}

QString profileToString(QSurfaceFormat::OpenGLContextProfile profile)
{
    switch (profile) {
        case QSurfaceFormat::NoProfile:
            return QStringLiteral("NoProfile");
        case QSurfaceFormat::CoreProfile:
            return QStringLiteral("CoreProfile");
        case QSurfaceFormat::CompatibilityProfile:
            return QStringLiteral("CompatibilityProfile");
    }
    return QObject::tr("Unavailable");
}

QJsonValue optionalIntToJson(const std::optional<int>& value)
{
    return value ? QJsonValue(*value) : QJsonValue();
}

QJsonValue optionalUIntToJson(const std::optional<unsigned int>& value)
{
    return value ? QJsonValue(static_cast<int>(*value)) : QJsonValue();
}

QJsonValue optionalBoolToJson(const std::optional<bool>& value)
{
    return value ? QJsonValue(*value) : QJsonValue();
}

QJsonValue optionalDoubleToJson(const std::optional<double>& value)
{
    return value ? QJsonValue(*value) : QJsonValue();
}

QJsonValue optionalStringToJson(const QString& value, bool available)
{
    return available ? QJsonValue(value) : QJsonValue();
}

QString glString(GLenum name)
{
    const auto* value = glGetString(name);
    return value ? QString::fromLatin1(reinterpret_cast<const char*>(value)) : QString();
}

void addFormRow(QFormLayout* form, const QString& label, const QString& value)
{
    auto* valueLabel = new QLabel(value);
    valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    valueLabel->setWordWrap(true);
    form->addRow(label, valueLabel);
}

QWidget* createScrollableTab(QWidget* content)
{
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(content);
    return scrollArea;
}

QWidget* createSummaryTab(const Gui::GpuDiagnosticsReport& report)
{
    auto* tab = new QWidget;
    auto* form = new QFormLayout(tab);
    addFormRow(
        form,
        QObject::tr("MSAA status"),
        Gui::GpuDiagnostics::msaaStatusToString(report.msaaStatus)
    );
    addFormRow(form, QObject::tr("Summary"), report.summary);
    addFormRow(form, QObject::tr("Requested samples"), QString::number(report.requestedSamples));
    addFormRow(
        form,
        QObject::tr("MSAA requested"),
        report.msaaRequested ? QObject::tr("Yes") : QObject::tr("No")
    );
    addFormRow(
        form,
        QObject::tr("Line smoothing requested"),
        report.lineSmoothingRequested ? QObject::tr("Yes") : QObject::tr("No")
    );
    addFormRow(form, QObject::tr("Anti-aliasing preference"), report.antiAliasingMode);
    addFormRow(
        form,
        QObject::tr("Default shape line width"),
        QString::number(report.defaultShapeLineWidth)
    );
    addFormRow(
        form,
        QObject::tr("Default shape point size"),
        QString::number(report.defaultShapePointSize)
    );
    addFormRow(
        form,
        QObject::tr("Render cache"),
        report.renderCache ? QObject::tr("Enabled") : QObject::tr("Disabled")
    );
    addFormRow(
        form,
        QObject::tr("VBO"),
        report.useVbo ? QObject::tr("Enabled") : QObject::tr("Disabled")
    );
    addFormRow(
        form,
        QObject::tr("New selection"),
        report.useNewSelection ? QObject::tr("Enabled") : QObject::tr("Disabled")
    );
    addFormRow(form, QObject::tr("FreeCAD"), report.freecadVersion);
    addFormRow(form, QObject::tr("Qt"), report.qtVersion);
    addFormRow(form, QObject::tr("Qt platform"), report.platform);
    return tab;
}

QWidget* createViewportsTab(const Gui::GpuDiagnosticsReport& report)
{
    auto* tab = new QWidget;
    auto* layout = new QVBoxLayout(tab);

    if (report.viewports.empty()) {
        layout->addWidget(new QLabel(QObject::tr("No OpenGL viewports were found.")));
        layout->addStretch();
        return tab;
    }

    for (std::size_t index = 0; index < report.viewports.size(); ++index) {
        const auto& viewport = report.viewports[index];
        auto* form = new QFormLayout;
        addFormRow(form, QObject::tr("Viewport"), QString::number(index + 1));
        addFormRow(form, QObject::tr("Widget class"), viewport.widgetClass);
        addFormRow(form, QObject::tr("Object name"), viewport.objectName);
        addFormRow(form, QObject::tr("Size"), sizeToString(viewport.size));
        addFormRow(form, QObject::tr("Screen"), viewport.screenName);
        addFormRow(
            form,
            QObject::tr("Device pixel ratio"),
            QString::number(viewport.devicePixelRatio, 'f', 2)
        );
        addFormRow(form, QObject::tr("Requested samples"), QString::number(viewport.requestedSamples));
        addFormRow(
            form,
            QObject::tr("Widget format samples"),
            optionalIntToString(viewport.widgetFormatSamples)
        );
        addFormRow(
            form,
            QObject::tr("Context format samples"),
            optionalIntToString(viewport.contextFormatSamples)
        );
        addFormRow(form, QObject::tr("Context profile"), viewport.contextProfile);
        addFormRow(form, QObject::tr("GL sample buffers"), optionalIntToString(viewport.glSampleBuffers));
        addFormRow(form, QObject::tr("GL samples"), optionalIntToString(viewport.glSamples));
        addFormRow(
            form,
            QObject::tr("GL multisample"),
            optionalBoolToString(viewport.glMultisampleEnabled)
        );
        addFormRow(
            form,
            QObject::tr("GL line smoothing"),
            optionalBoolToString(viewport.glLineSmoothEnabled)
        );
        addFormRow(form, QObject::tr("GL blending"), optionalBoolToString(viewport.glBlendEnabled));
        addFormRow(
            form,
            QObject::tr("Line smoothing blend ready"),
            optionalBoolToString(viewport.lineSmoothingBlendReady)
        );
        addFormRow(form, QObject::tr("GL blend source RGB"), blendFactorToString(viewport.glBlendSrcRgb));
        addFormRow(
            form,
            QObject::tr("GL blend destination RGB"),
            blendFactorToString(viewport.glBlendDstRgb)
        );
        addFormRow(
            form,
            QObject::tr("GL blend source alpha"),
            blendFactorToString(viewport.glBlendSrcAlpha)
        );
        addFormRow(
            form,
            QObject::tr("GL blend destination alpha"),
            blendFactorToString(viewport.glBlendDstAlpha)
        );
        addFormRow(
            form,
            QObject::tr("GL blend equation RGB"),
            blendEquationToString(viewport.glBlendEquationRgb)
        );
        addFormRow(
            form,
            QObject::tr("GL blend equation alpha"),
            blendEquationToString(viewport.glBlendEquationAlpha)
        );
        addFormRow(form, QObject::tr("GL depth test"), optionalBoolToString(viewport.glDepthTestEnabled));
        addFormRow(form, QObject::tr("GL line width"), optionalDoubleToString(viewport.glLineWidth));
        addFormRow(
            form,
            QObject::tr("GL aliased line width range"),
            QStringLiteral("%1 - %2")
                .arg(optionalDoubleToString(viewport.glAliasedLineWidthRangeMin))
                .arg(optionalDoubleToString(viewport.glAliasedLineWidthRangeMax))
        );
        addFormRow(
            form,
            QObject::tr("GL line smoothing hint"),
            lineSmoothHintToString(viewport.glLineSmoothHint)
        );
        addFormRow(
            form,
            QObject::tr("Default framebuffer"),
            optionalUIntToString(viewport.defaultFramebufferObject)
        );
        addFormRow(form, QObject::tr("Draw framebuffer"), optionalIntToString(viewport.drawFramebuffer));
        addFormRow(form, QObject::tr("Read framebuffer"), optionalIntToString(viewport.readFramebuffer));
        addFormRow(form, QObject::tr("Vendor"), viewport.vendor);
        addFormRow(form, QObject::tr("Renderer"), viewport.renderer);
        addFormRow(form, QObject::tr("OpenGL version"), viewport.version);
        addFormRow(form, QObject::tr("Status"), viewport.status);
        layout->addLayout(form);
    }

    layout->addStretch();
    return tab;
}

QWidget* createScreensTab(const Gui::GpuDiagnosticsReport& report)
{
    auto* tab = new QWidget;
    auto* layout = new QVBoxLayout(tab);

    if (report.screens.empty()) {
        layout->addWidget(new QLabel(QObject::tr("No screens were reported by Qt.")));
        layout->addStretch();
        return tab;
    }

    for (std::size_t index = 0; index < report.screens.size(); ++index) {
        const auto& screen = report.screens[index];
        auto* form = new QFormLayout;
        addFormRow(form, QObject::tr("Screen"), QString::number(index + 1));
        addFormRow(form, QObject::tr("Name"), screen.name);
        addFormRow(form, QObject::tr("Geometry"), screen.geometry);
        addFormRow(form, QObject::tr("Available geometry"), screen.availableGeometry);
        addFormRow(form, QObject::tr("Physical size (mm)"), sizeToString(screen.physicalSizeMm));
        addFormRow(
            form,
            QObject::tr("Device pixel ratio"),
            QString::number(screen.devicePixelRatio, 'f', 2)
        );
        addFormRow(
            form,
            QObject::tr("Logical DPI"),
            QStringLiteral("%1 x %2").arg(screen.logicalDpiX, 0, 'f', 1).arg(screen.logicalDpiY, 0, 'f', 1)
        );
        addFormRow(form, QObject::tr("Refresh rate"), QString::number(screen.refreshRate, 'f', 2));
        layout->addLayout(form);
    }

    layout->addStretch();
    return tab;
}

}  // namespace

Gui::GpuDiagnosticsReport Gui::GpuDiagnostics::collect(QWidget* parent)
{
    GpuDiagnosticsReport report;
    report.schema = QStringLiteral("org.freecad.gpu-diagnostics.v1");
    report.freecadVersion = QString::fromStdString(App::Application::getNameWithVersion());
    report.qtVersion = QString::fromLatin1(qVersion());
    report.platform = QGuiApplication::platformName();

    const auto antiAliasing = Multisample::readMSAAFromSettings();
    report.requestedSamples = Multisample::toSamples(antiAliasing);
    report.lineSmoothingRequested = antiAliasing == Gui::AntiAliasing::MSAA1x;
    report.msaaRequested = report.requestedSamples > 1;
    report.antiAliasingMode = antiAliasingModeToString(antiAliasing);
    auto* viewParams = ViewParams::instance();
    report.defaultShapeLineWidth = viewParams->getDefaultShapeLineWidth();
    report.defaultShapePointSize = viewParams->getDefaultShapePointSize();
    report.renderCache = viewParams->getRenderCache() != 0;
    report.useVbo = viewParams->getHandle()->GetBool("UseVBO", true);
    report.useNewSelection = viewParams->getUseNewSelection();

    const auto screens = QGuiApplication::screens();
    report.screens.reserve(static_cast<std::size_t>(screens.size()));
    for (const auto* screen : screens) {
        if (!screen) {
            continue;
        }

        GpuDiagnosticsScreen info;
        info.name = screen->name();
        info.geometry = rectToString(screen->geometry());
        info.availableGeometry = rectToString(screen->availableGeometry());
        info.physicalSizeMm = screen->physicalSize();
        info.devicePixelRatio = screen->devicePixelRatio();
        info.logicalDpiX = screen->logicalDotsPerInchX();
        info.logicalDpiY = screen->logicalDotsPerInchY();
        info.refreshRate = screen->refreshRate();
        report.screens.push_back(info);
    }

    const auto widgets = QApplication::allWidgets();
    for (auto* widget : widgets) {
        auto* glWidget = qobject_cast<QOpenGLWidget*>(widget);
        if (!glWidget) {
            continue;
        }

        GpuDiagnosticsViewport viewport;
        viewport.widgetClass = QString::fromLatin1(glWidget->metaObject()->className());
        viewport.objectName = glWidget->objectName();
        viewport.size = glWidget->size();
        viewport.devicePixelRatio = glWidget->devicePixelRatioF();
        viewport.requestedSamples = report.requestedSamples;
        viewport.widgetFormatSamples = glWidget->format().samples();

        const auto* nativeWindow = glWidget->window() ? glWidget->window()->windowHandle() : nullptr;
        const auto* screen = nativeWindow ? nativeWindow->screen() : nullptr;
        viewport.screenName = screen ? screen->name() : QString();

        if (glWidget->context()) {
            viewport.contextFormatSamples = glWidget->context()->format().samples();
            viewport.contextProfile = profileToString(glWidget->context()->format().profile());

            auto* previousContext = QOpenGLContext::currentContext();
            auto* previousSurface = previousContext ? previousContext->surface() : nullptr;
            glWidget->makeCurrent();
            if (QOpenGLContext::currentContext() != glWidget->context()) {
                viewport.status = QObject::tr("OpenGL context could not be made current");
            }
            else {
                GLint sampleBuffers = 0;
                GLint samples = 0;
                GLint drawFramebuffer = 0;
                GLint readFramebuffer = 0;
                GLint lineSmoothHint = 0;
                GLint blendSrcRgb = 0;
                GLint blendDstRgb = 0;
                GLint blendSrcAlpha = 0;
                GLint blendDstAlpha = 0;
                GLint blendEquationRgb = 0;
                GLint blendEquationAlpha = 0;
                GLfloat lineWidth = 0.0F;
                GLfloat aliasedLineWidthRange[2] = {0.0F, 0.0F};
                glGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);
                glGetIntegerv(GL_SAMPLES, &samples);
                glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
                glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
                glGetIntegerv(GL_LINE_SMOOTH_HINT, &lineSmoothHint);
                glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
                glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
                glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
                glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);
                glGetIntegerv(GL_BLEND_EQUATION_RGB, &blendEquationRgb);
                glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blendEquationAlpha);
                glGetFloatv(GL_LINE_WIDTH, &lineWidth);
                glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, aliasedLineWidthRange);
                viewport.glSampleBuffers = sampleBuffers;
                viewport.glSamples = samples;
                viewport.glMultisampleEnabled = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;
                viewport.glLineSmoothEnabled = glIsEnabled(GL_LINE_SMOOTH) == GL_TRUE;
                viewport.glBlendEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
                viewport.lineSmoothingBlendReady = viewport.glLineSmoothEnabled.value_or(false)
                    && viewport.glBlendEnabled.value_or(false);
                viewport.glDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
                viewport.glBlendSrcRgb = blendSrcRgb;
                viewport.glBlendDstRgb = blendDstRgb;
                viewport.glBlendSrcAlpha = blendSrcAlpha;
                viewport.glBlendDstAlpha = blendDstAlpha;
                viewport.glBlendEquationRgb = blendEquationRgb;
                viewport.glBlendEquationAlpha = blendEquationAlpha;
                viewport.glLineWidth = static_cast<double>(lineWidth);
                viewport.glAliasedLineWidthRangeMin = static_cast<double>(aliasedLineWidthRange[0]);
                viewport.glAliasedLineWidthRangeMax = static_cast<double>(aliasedLineWidthRange[1]);
                viewport.glLineSmoothHint = lineSmoothHint;
                viewport.drawFramebuffer = drawFramebuffer;
                viewport.readFramebuffer = readFramebuffer;
                viewport.defaultFramebufferObject = glWidget->defaultFramebufferObject();
                viewport.vendor = glString(GL_VENDOR);
                viewport.renderer = glString(GL_RENDERER);
                viewport.version = glString(GL_VERSION);
                viewport.status = QObject::tr("Available");
            }

            if (previousContext && previousSurface) {
                previousContext->makeCurrent(previousSurface);
            }
            else {
                glWidget->doneCurrent();
            }
        }
        else {
            viewport.status = QObject::tr("OpenGL context is not initialized");
        }

        report.viewports.push_back(viewport);
    }

    if (report.viewports.empty()) {
        report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Unknown;
        report.summary = QObject::tr("No OpenGL viewport was available for MSAA diagnostics.");
    }
    else {
        bool hasActualSamples = false;
        bool hasZeroSamples = false;
        for (auto& viewport : report.viewports) {
            const auto actualSamples = viewport.glSamples.value_or(
                viewport.contextFormatSamples.value_or(-1)
            );
            if (actualSamples > 0) {
                hasActualSamples = true;
            }
            if (actualSamples == 0) {
                hasZeroSamples = true;
            }
        }

        bool hasLineSmoothing = false;
        bool hasLineSmoothingWithBlend = false;
        for (const auto& viewport : report.viewports) {
            hasLineSmoothing = hasLineSmoothing || viewport.glLineSmoothEnabled.value_or(false);
            hasLineSmoothingWithBlend = hasLineSmoothingWithBlend
                || viewport.lineSmoothingBlendReady.value_or(false);
        }

        if (report.lineSmoothingRequested && hasLineSmoothing && hasLineSmoothingWithBlend) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QObject::tr(
                "Line smoothing was requested and at least one OpenGL viewport reports "
                "GL_LINE_SMOOTH and GL_BLEND enabled."
            );
        }
        else if (report.lineSmoothingRequested && hasLineSmoothing) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QObject::tr(
                "Line smoothing was requested and GL_LINE_SMOOTH is enabled, but no OpenGL "
                "viewport reports GL_BLEND enabled. Alpha line smoothing may be ineffective."
            );
        }
        else if (report.lineSmoothingRequested) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Inactive;
            report.summary
                = QObject::tr("Line smoothing was requested, but no OpenGL viewport reports GL_LINE_SMOOTH enabled.");
        }
        else if (report.msaaRequested && hasActualSamples) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QObject::tr(
                "MSAA was requested and at least one OpenGL viewport reports active samples."
            );
        }
        else if (report.msaaRequested && hasZeroSamples) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Inactive;
            report.summary = QObject::tr(
                "MSAA was requested, but an OpenGL viewport reports zero active samples."
            );
        }
        else if (!report.msaaRequested) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Inactive;
            report.summary = QObject::tr("MSAA is not requested in the current FreeCAD preferences.");
        }
        else {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Unknown;
            report.summary = QObject::tr(
                "MSAA status could not be determined from the current viewport state."
            );
        }
    }

    Q_UNUSED(parent);
    return report;
}

Gui::GpuDiagnosticsReport Gui::GpuDiagnostics::makeReportForTesting(
    int requestedSamples,
    const std::vector<std::optional<int>>& viewportSamples
)
{
    GpuDiagnosticsReport report;
    report.schema = QStringLiteral("org.freecad.gpu-diagnostics.v1");
    report.requestedSamples = requestedSamples;
    report.msaaRequested = requestedSamples > 1;
    report.lineSmoothingRequested = requestedSamples == 1;
    report.antiAliasingMode = QString::number(requestedSamples);
    report.qtVersion = QStringLiteral("test");
    report.freecadVersion = QStringLiteral("FreeCAD test");
    report.platform = QStringLiteral("test");
    report.defaultShapeLineWidth = 2;
    report.defaultShapePointSize = 2;
    report.renderCache = false;
    report.useVbo = true;
    report.useNewSelection = true;

    for (const auto& samples : viewportSamples) {
        GpuDiagnosticsViewport viewport;
        viewport.widgetClass = QStringLiteral("QOpenGLWidget");
        viewport.requestedSamples = requestedSamples;
        viewport.glSamples = samples;
        if (report.lineSmoothingRequested) {
            viewport.glLineSmoothEnabled = true;
            viewport.glBlendEnabled = false;
            viewport.lineSmoothingBlendReady = false;
        }
        report.viewports.push_back(viewport);
    }

    if (report.viewports.empty()) {
        report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Unknown;
        report.summary = QStringLiteral("No OpenGL viewport was available for MSAA diagnostics.");
    }
    else if (report.lineSmoothingRequested) {
        bool hasLineSmoothing = false;
        bool hasLineSmoothingWithBlend = false;
        for (const auto& viewport : report.viewports) {
            hasLineSmoothing = hasLineSmoothing || viewport.glLineSmoothEnabled.value_or(false);
            hasLineSmoothingWithBlend = hasLineSmoothingWithBlend
                || viewport.lineSmoothingBlendReady.value_or(false);
        }
        if (hasLineSmoothing && hasLineSmoothingWithBlend) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QStringLiteral(
                "Line smoothing was requested and at least one OpenGL viewport reports "
                "GL_LINE_SMOOTH and GL_BLEND enabled."
            );
        }
        else if (hasLineSmoothing) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QStringLiteral(
                "Line smoothing was requested and GL_LINE_SMOOTH is enabled, but no OpenGL "
                "viewport reports GL_BLEND enabled. Alpha line smoothing may be ineffective."
            );
        }
        else {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Inactive;
            report.summary
                = QStringLiteral("Line smoothing was requested, but no OpenGL viewport reports GL_LINE_SMOOTH enabled.");
        }
    }
    else if (report.msaaRequested) {
        bool hasActualSamples = false;
        bool hasZeroSamples = false;
        for (const auto& viewport : report.viewports) {
            const auto actualSamples = viewport.glSamples.value_or(-1);
            hasActualSamples = hasActualSamples || actualSamples > 0;
            hasZeroSamples = hasZeroSamples || actualSamples == 0;
        }
        if (hasActualSamples) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QStringLiteral(
                "MSAA was requested and at least one OpenGL viewport reports active samples."
            );
        }
        else if (hasZeroSamples) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Inactive;
            report.summary = QStringLiteral(
                "MSAA was requested, but an OpenGL viewport reports zero active samples."
            );
        }
    }
    else {
        report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Inactive;
        report.summary = QStringLiteral("MSAA is not requested in the current FreeCAD preferences.");
    }

    return report;
}

QString Gui::GpuDiagnostics::toJson(const GpuDiagnosticsReport& report)
{
    QJsonObject root;
    root.insert(QStringLiteral("schema"), report.schema);

    QJsonObject freecad;
    freecad.insert(QStringLiteral("version"), report.freecadVersion);
    root.insert(QStringLiteral("freecad"), freecad);

    QJsonObject qt;
    qt.insert(QStringLiteral("version"), report.qtVersion);
    qt.insert(QStringLiteral("platform"), report.platform);
    root.insert(QStringLiteral("qt"), qt);

    QJsonObject preferences;
    preferences.insert(QStringLiteral("antiAliasingMode"), report.antiAliasingMode);
    preferences.insert(QStringLiteral("requestedSamples"), report.requestedSamples);
    preferences.insert(QStringLiteral("msaaRequested"), report.msaaRequested);
    preferences.insert(QStringLiteral("lineSmoothingRequested"), report.lineSmoothingRequested);
    preferences.insert(QStringLiteral("defaultShapeLineWidth"), report.defaultShapeLineWidth);
    preferences.insert(QStringLiteral("defaultShapePointSize"), report.defaultShapePointSize);
    preferences.insert(QStringLiteral("renderCache"), report.renderCache);
    preferences.insert(QStringLiteral("useVbo"), report.useVbo);
    preferences.insert(QStringLiteral("useNewSelection"), report.useNewSelection);
    root.insert(QStringLiteral("preferences"), preferences);

    QJsonArray screens;
    for (const auto& screen : report.screens) {
        QJsonObject item;
        item.insert(QStringLiteral("name"), screen.name);
        item.insert(QStringLiteral("geometry"), screen.geometry);
        item.insert(QStringLiteral("availableGeometry"), screen.availableGeometry);
        item.insert(QStringLiteral("physicalSizeMm"), sizeToString(screen.physicalSizeMm));
        item.insert(QStringLiteral("devicePixelRatio"), screen.devicePixelRatio);
        item.insert(QStringLiteral("logicalDpiX"), screen.logicalDpiX);
        item.insert(QStringLiteral("logicalDpiY"), screen.logicalDpiY);
        item.insert(QStringLiteral("refreshRate"), screen.refreshRate);
        screens.append(item);
    }
    root.insert(QStringLiteral("screens"), screens);

    QJsonArray viewports;
    for (const auto& viewport : report.viewports) {
        QJsonObject item;
        item.insert(QStringLiteral("widgetClass"), viewport.widgetClass);
        item.insert(QStringLiteral("objectName"), viewport.objectName);
        item.insert(QStringLiteral("screen"), viewport.screenName);
        item.insert(QStringLiteral("size"), sizeToString(viewport.size));
        item.insert(QStringLiteral("devicePixelRatio"), viewport.devicePixelRatio);
        item.insert(QStringLiteral("requestedSamples"), viewport.requestedSamples);
        item.insert(
            QStringLiteral("widgetFormatSamples"),
            optionalIntToJson(viewport.widgetFormatSamples)
        );
        item.insert(
            QStringLiteral("contextFormatSamples"),
            optionalIntToJson(viewport.contextFormatSamples)
        );
        item.insert(
            QStringLiteral("contextProfile"),
            optionalStringToJson(viewport.contextProfile, !viewport.contextProfile.isEmpty())
        );
        item.insert(QStringLiteral("glSampleBuffers"), optionalIntToJson(viewport.glSampleBuffers));
        item.insert(QStringLiteral("glSamples"), optionalIntToJson(viewport.glSamples));
        item.insert(
            QStringLiteral("glMultisampleEnabled"),
            optionalBoolToJson(viewport.glMultisampleEnabled)
        );
        item.insert(
            QStringLiteral("glLineSmoothEnabled"),
            optionalBoolToJson(viewport.glLineSmoothEnabled)
        );
        item.insert(QStringLiteral("glBlendEnabled"), optionalBoolToJson(viewport.glBlendEnabled));
        item.insert(
            QStringLiteral("lineSmoothingBlendReady"),
            optionalBoolToJson(viewport.lineSmoothingBlendReady)
        );
        item.insert(
            QStringLiteral("glBlendSrcRgb"),
            optionalStringToJson(
                blendFactorToString(viewport.glBlendSrcRgb),
                viewport.glBlendSrcRgb.has_value()
            )
        );
        item.insert(
            QStringLiteral("glBlendDstRgb"),
            optionalStringToJson(
                blendFactorToString(viewport.glBlendDstRgb),
                viewport.glBlendDstRgb.has_value()
            )
        );
        item.insert(
            QStringLiteral("glBlendSrcAlpha"),
            optionalStringToJson(
                blendFactorToString(viewport.glBlendSrcAlpha),
                viewport.glBlendSrcAlpha.has_value()
            )
        );
        item.insert(
            QStringLiteral("glBlendDstAlpha"),
            optionalStringToJson(
                blendFactorToString(viewport.glBlendDstAlpha),
                viewport.glBlendDstAlpha.has_value()
            )
        );
        item.insert(
            QStringLiteral("glBlendEquationRgb"),
            optionalStringToJson(
                blendEquationToString(viewport.glBlendEquationRgb),
                viewport.glBlendEquationRgb.has_value()
            )
        );
        item.insert(
            QStringLiteral("glBlendEquationAlpha"),
            optionalStringToJson(
                blendEquationToString(viewport.glBlendEquationAlpha),
                viewport.glBlendEquationAlpha.has_value()
            )
        );
        item.insert(
            QStringLiteral("glDepthTestEnabled"),
            optionalBoolToJson(viewport.glDepthTestEnabled)
        );
        item.insert(QStringLiteral("glLineWidth"), optionalDoubleToJson(viewport.glLineWidth));
        item.insert(
            QStringLiteral("glAliasedLineWidthRangeMin"),
            optionalDoubleToJson(viewport.glAliasedLineWidthRangeMin)
        );
        item.insert(
            QStringLiteral("glAliasedLineWidthRangeMax"),
            optionalDoubleToJson(viewport.glAliasedLineWidthRangeMax)
        );
        item.insert(
            QStringLiteral("glLineSmoothHint"),
            lineSmoothHintToString(viewport.glLineSmoothHint)
        );
        item.insert(
            QStringLiteral("defaultFramebufferObject"),
            optionalUIntToJson(viewport.defaultFramebufferObject)
        );
        item.insert(QStringLiteral("drawFramebuffer"), optionalIntToJson(viewport.drawFramebuffer));
        item.insert(QStringLiteral("readFramebuffer"), optionalIntToJson(viewport.readFramebuffer));
        item.insert(
            QStringLiteral("vendor"),
            optionalStringToJson(viewport.vendor, !viewport.vendor.isEmpty())
        );
        item.insert(
            QStringLiteral("renderer"),
            optionalStringToJson(viewport.renderer, !viewport.renderer.isEmpty())
        );
        item.insert(
            QStringLiteral("version"),
            optionalStringToJson(viewport.version, !viewport.version.isEmpty())
        );
        item.insert(QStringLiteral("status"), viewport.status);
        viewports.append(item);
    }
    root.insert(QStringLiteral("viewports"), viewports);

    QJsonObject summary;
    bool lineSmoothingBlendReady = false;
    for (const auto& viewport : report.viewports) {
        lineSmoothingBlendReady = lineSmoothingBlendReady
            || viewport.lineSmoothingBlendReady.value_or(false);
    }
    summary.insert(QStringLiteral("msaaStatus"), msaaStatusToString(report.msaaStatus));
    summary.insert(QStringLiteral("msaaRequested"), report.msaaRequested);
    summary.insert(QStringLiteral("lineSmoothingRequested"), report.lineSmoothingRequested);
    summary.insert(QStringLiteral("lineSmoothingBlendReady"), lineSmoothingBlendReady);
    summary.insert(
        QStringLiteral("msaaActive"),
        report.msaaRequested && report.msaaStatus == GpuDiagnosticsReport::MsaaStatus::Active
    );
    summary.insert(
        QStringLiteral("antiAliasingActive"),
        report.msaaStatus == GpuDiagnosticsReport::MsaaStatus::Active
    );
    summary.insert(QStringLiteral("message"), report.summary);
    root.insert(QStringLiteral("summary"), summary);

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QString Gui::GpuDiagnostics::toText(const GpuDiagnosticsReport& report)
{
    QString text;
    QTextStream stream(&text);
    stream << "FreeCAD GPU diagnostics\n";
    stream << "MSAA status: " << msaaStatusToString(report.msaaStatus) << "\n";
    stream << "Summary: " << report.summary << "\n";
    stream << "Requested samples: " << report.requestedSamples << "\n";
    stream << "MSAA requested: " << (report.msaaRequested ? "true" : "false") << "\n";
    stream << "Line smoothing requested: " << (report.lineSmoothingRequested ? "true" : "false")
           << "\n";
    stream << "Anti-aliasing preference: " << report.antiAliasingMode << "\n";
    stream << "Default shape line width: " << report.defaultShapeLineWidth << "\n";
    stream << "Default shape point size: " << report.defaultShapePointSize << "\n";
    stream << "Render cache: " << (report.renderCache ? "enabled" : "disabled") << "\n";
    stream << "VBO: " << (report.useVbo ? "enabled" : "disabled") << "\n";
    stream << "New selection: " << (report.useNewSelection ? "enabled" : "disabled") << "\n";
    stream << "FreeCAD: " << report.freecadVersion << "\n";
    stream << "Qt: " << report.qtVersion << "\n";
    stream << "Platform: " << report.platform << "\n";

    for (std::size_t i = 0; i < report.viewports.size(); ++i) {
        const auto& viewport = report.viewports[i];
        stream << "Viewport " << (i + 1) << ": class=" << viewport.widgetClass
               << ", size=" << sizeToString(viewport.size) << ", screen=\"" << viewport.screenName
               << "\", widget samples=" << optionalIntToString(viewport.widgetFormatSamples)
               << ", context samples=" << optionalIntToString(viewport.contextFormatSamples)
               << ", context profile=" << viewport.contextProfile
               << ", GL_SAMPLE_BUFFERS=" << optionalIntToString(viewport.glSampleBuffers)
               << ", GL_SAMPLES=" << optionalIntToString(viewport.glSamples)
               << ", GL_MULTISAMPLE=" << optionalBoolToString(viewport.glMultisampleEnabled)
               << ", GL_LINE_SMOOTH=" << optionalBoolToString(viewport.glLineSmoothEnabled)
               << ", GL_BLEND=" << optionalBoolToString(viewport.glBlendEnabled)
               << ", LINE_SMOOTHING_BLEND_READY="
               << optionalBoolToString(viewport.lineSmoothingBlendReady)
               << ", GL_BLEND_SRC_RGB=" << blendFactorToString(viewport.glBlendSrcRgb)
               << ", GL_BLEND_DST_RGB=" << blendFactorToString(viewport.glBlendDstRgb)
               << ", GL_BLEND_SRC_ALPHA=" << blendFactorToString(viewport.glBlendSrcAlpha)
               << ", GL_BLEND_DST_ALPHA=" << blendFactorToString(viewport.glBlendDstAlpha)
               << ", GL_BLEND_EQUATION_RGB=" << blendEquationToString(viewport.glBlendEquationRgb)
               << ", GL_BLEND_EQUATION_ALPHA=" << blendEquationToString(viewport.glBlendEquationAlpha)
               << ", GL_DEPTH_TEST=" << optionalBoolToString(viewport.glDepthTestEnabled)
               << ", GL_LINE_WIDTH=" << optionalDoubleToString(viewport.glLineWidth)
               << ", GL_ALIASED_LINE_WIDTH_RANGE="
               << optionalDoubleToString(viewport.glAliasedLineWidthRangeMin) << "-"
               << optionalDoubleToString(viewport.glAliasedLineWidthRangeMax)
               << ", GL_LINE_SMOOTH_HINT=" << lineSmoothHintToString(viewport.glLineSmoothHint)
               << ", default FBO=" << optionalUIntToString(viewport.defaultFramebufferObject)
               << ", draw FBO=" << optionalIntToString(viewport.drawFramebuffer)
               << ", read FBO=" << optionalIntToString(viewport.readFramebuffer) << ", vendor=\""
               << viewport.vendor << "\", renderer=\"" << viewport.renderer << "\", version=\""
               << viewport.version << "\"\n";
    }

    return text;
}

QString Gui::GpuDiagnostics::msaaStatusToString(GpuDiagnosticsReport::MsaaStatus status)
{
    switch (status) {
        case GpuDiagnosticsReport::MsaaStatus::Active:
            return QObject::tr("Active");
        case GpuDiagnosticsReport::MsaaStatus::Inactive:
            return QObject::tr("Inactive");
        case GpuDiagnosticsReport::MsaaStatus::Unknown:
            return QObject::tr("Unknown");
    }
    return QObject::tr("Unknown");
}

void Gui::GpuDiagnosticsDialog::showDialog(QWidget* parent)
{
    const auto report = GpuDiagnostics::collect(parent);
    const auto json = GpuDiagnostics::toJson(report);
    const auto text = GpuDiagnostics::toText(report);

    QDialog dialog(parent);
    dialog.setWindowTitle(QObject::tr("GPU Diagnostics"));
    dialog.resize(820, 620);

    auto* layout = new QVBoxLayout(&dialog);
    auto* tabs = new QTabWidget(&dialog);
    tabs->addTab(createScrollableTab(createSummaryTab(report)), QObject::tr("Summary"));
    tabs->addTab(createScrollableTab(createViewportsTab(report)), QObject::tr("Viewports"));
    tabs->addTab(createScrollableTab(createScreensTab(report)), QObject::tr("Displays"));

    auto* exportText = new QPlainTextEdit(json);
    exportText->setReadOnly(true);
    exportText->setLineWrapMode(QPlainTextEdit::NoWrap);
    tabs->addTab(exportText, QObject::tr("JSON"));
    layout->addWidget(tabs);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto* copyJson = buttons->addButton(QObject::tr("Copy JSON"), QDialogButtonBox::ActionRole);
    auto* copyText = buttons->addButton(QObject::tr("Copy Text"), QDialogButtonBox::ActionRole);
    auto* saveJson = buttons->addButton(QObject::tr("Save JSON..."), QDialogButtonBox::ActionRole);
    QObject::connect(copyJson, &QPushButton::clicked, [&json]() {
        QApplication::clipboard()->setText(json);
    });
    QObject::connect(copyText, &QPushButton::clicked, [&text]() {
        QApplication::clipboard()->setText(text);
    });
    QObject::connect(saveJson, &QPushButton::clicked, [&dialog, &json]() {
        const auto fileName = QFileDialog::getSaveFileName(
            &dialog,
            QObject::tr("Save GPU Diagnostics"),
            QStringLiteral("freecad-gpu-diagnostics.json"),
            QObject::tr("JSON files (*.json);;All files (*)")
        );
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(
                &dialog,
                QObject::tr("Save GPU Diagnostics"),
                QObject::tr("Could not write the diagnostics report.")
            );
            return;
        }
        file.write(json.toUtf8());
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    dialog.exec();
}
