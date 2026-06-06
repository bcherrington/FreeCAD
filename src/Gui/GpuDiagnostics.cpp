// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "GpuDiagnostics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
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
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>

#include "Application.h"
#include "Document.h"
#include "Multisample.h"
#include "ViewParams.h"
#include "ViewProvider.h"

#if HAVE_CONFIG_H
# include <config.h>
# ifdef HAVE_GL_GL_H
#  include <GL/gl.h>
# endif
#endif

namespace
{

constexpr std::size_t maxRenderProbes = 64;

std::mutex renderProbeMutex;
std::vector<Gui::GpuDiagnosticsRenderProbe> renderProbes;
bool renderProbesTruncated = false;

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

std::optional<double> propertyDoubleValue(const App::PropertyContainer* container, const char* name)
{
    if (!container) {
        return std::nullopt;
    }

    const auto* property = container->getPropertyByName(name);
    if (!property) {
        return std::nullopt;
    }
    if (property->isDerivedFrom<App::PropertyFloat>()) {
        return static_cast<const App::PropertyFloat*>(property)->getValue();
    }

    return std::nullopt;
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

double pointLineDistance(double px, double py, double x1, double y1, double x2, double y2)
{
    const double dx = x2 - x1;
    const double dy = y2 - y1;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared <= 0.0) {
        const double pointDx = px - x1;
        const double pointDy = py - y1;
        return std::sqrt(pointDx * pointDx + pointDy * pointDy);
    }

    const double t = std::clamp(((px - x1) * dx + (py - y1) * dy) / lengthSquared, 0.0, 1.0);
    const double cx = x1 + t * dx;
    const double cy = y1 + t * dy;
    const double pointDx = px - cx;
    const double pointDy = py - cy;
    return std::sqrt(pointDx * pointDx + pointDy * pointDy);
}

Gui::GpuDiagnosticsVisualAaProbe analyzeVisualAaImage(
    const QString& name,
    const QString& status,
    const QImage& source,
    int requestedSamples,
    const std::optional<int>& actualSamples,
    bool lineSmoothingEnabled,
    bool blendEnabled
)
{
    Gui::GpuDiagnosticsVisualAaProbe probe;
    probe.name = name;
    probe.status = status;
    probe.width = source.width();
    probe.height = source.height();
    probe.requestedSamples = requestedSamples;
    probe.actualSamples = actualSamples;
    probe.lineSmoothingEnabled = lineSmoothingEnabled;
    probe.blendEnabled = blendEnabled;

    const QImage image = source.convertToFormat(QImage::Format_RGB32);
    std::array<bool, 256> luminanceValues {};
    double coveredLuminanceSum = 0.0;
    for (int y = 0; y < image.height(); ++y) {
        const auto* scanLine = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const int luminance = qGray(scanLine[x]);
            luminanceValues[static_cast<std::size_t>(luminance)] = true;
            if (luminance < 250) {
                ++probe.coveredPixelCount;
                coveredLuminanceSum += luminance;
                if (luminance <= 8) {
                    ++probe.darkPixelCount;
                }
                else {
                    ++probe.intermediatePixelCount;
                }
            }
        }
    }

    probe.distinctLuminanceCount = static_cast<int>(
        std::count(luminanceValues.begin(), luminanceValues.end(), true)
    );
    if (probe.coveredPixelCount > 0) {
        probe.intermediatePixelRatio = static_cast<double>(probe.intermediatePixelCount)
            / probe.coveredPixelCount;
        probe.meanCoveredLuminance = coveredLuminanceSum / probe.coveredPixelCount;
    }

    return probe;
}

QImage makeScreenSpaceReferenceImage()
{
    constexpr int probeSize = 64;
    constexpr double x1 = 7.25;
    constexpr double y1 = 54.75;
    constexpr double x2 = 56.75;
    constexpr double y2 = 9.25;

    QImage image(probeSize, probeSize, QImage::Format_RGB32);
    image.fill(Qt::white);

    for (int y = 0; y < image.height(); ++y) {
        auto* scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const double distance = pointLineDistance(x + 0.5, y + 0.5, x1, y1, x2, y2);
            const double coverage = std::clamp(1.5 - distance, 0.0, 1.0);
            const int value = static_cast<int>(std::round(255.0 * (1.0 - coverage)));
            scanLine[x] = qRgb(value, value, value);
        }
    }

    return image;
}

Gui::GpuDiagnosticsVisualAaProbe makeUnavailableVisualAaProbe(
    const QString& name,
    const QString& status,
    int requestedSamples,
    bool lineSmoothingEnabled,
    bool blendEnabled
)
{
    Gui::GpuDiagnosticsVisualAaProbe probe;
    probe.name = name;
    probe.status = status;
    probe.requestedSamples = requestedSamples;
    probe.lineSmoothingEnabled = lineSmoothingEnabled;
    probe.blendEnabled = blendEnabled;
    return probe;
}

Gui::GpuDiagnosticsVisualAaProbe renderGlLineVisualAaProbe(
    const QString& name,
    int requestedSamples,
    bool lineSmoothingEnabled,
    bool blendEnabled,
    bool multisampleEnabled
)
{
    constexpr int probeSize = 64;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::Depth);
    format.setSamples(requestedSamples);

    QOpenGLFramebufferObject fbo(probeSize, probeSize, format);
    if (!fbo.isValid()) {
        return makeUnavailableVisualAaProbe(
            name,
            QObject::tr("Framebuffer unavailable"),
            requestedSamples,
            lineSmoothingEnabled,
            blendEnabled
        );
    }

    GLint previousViewport[4] = {0, 0, 0, 0};
    GLint previousMatrixMode = GL_MODELVIEW;
    const bool previousLineSmooth = glIsEnabled(GL_LINE_SMOOTH) == GL_TRUE;
    const bool previousBlend = glIsEnabled(GL_BLEND) == GL_TRUE;
    const bool previousMultisample = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;
    const bool previousDepthTest = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
    GLint previousBlendSrc = 0;
    GLint previousBlendDst = 0;
    GLint previousLineSmoothHint = 0;
    GLfloat previousLineWidth = 1.0F;
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);
    glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDst);
    glGetIntegerv(GL_LINE_SMOOTH_HINT, &previousLineSmoothHint);
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);

    fbo.bind();
    glViewport(0, 0, probeSize, probeSize);
    glDisable(GL_DEPTH_TEST);
    glClearColor(1.0F, 1.0F, 1.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, probeSize, 0.0, probeSize, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (multisampleEnabled) {
        glEnable(GL_MULTISAMPLE);
    }
    else {
        glDisable(GL_MULTISAMPLE);
    }
    if (lineSmoothingEnabled) {
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    }
    else {
        glDisable(GL_LINE_SMOOTH);
    }
    if (blendEnabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_BLEND);
    }

    glLineWidth(1.0F);
    glColor3f(0.0F, 0.0F, 0.0F);
    glBegin(GL_LINES);
    glVertex2f(7.25F, 54.75F);
    glVertex2f(56.75F, 9.25F);
    glEnd();
    glFlush();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(previousMatrixMode);

    const QImage image = fbo.toImage(false);
    fbo.release();

    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    glLineWidth(previousLineWidth);
    glBlendFunc(static_cast<GLenum>(previousBlendSrc), static_cast<GLenum>(previousBlendDst));
    glHint(GL_LINE_SMOOTH_HINT, static_cast<GLenum>(previousLineSmoothHint));
    previousDepthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    previousMultisample ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);
    previousBlend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    previousLineSmooth ? glEnable(GL_LINE_SMOOTH) : glDisable(GL_LINE_SMOOTH);

    return analyzeVisualAaImage(
        name,
        QObject::tr("Available"),
        image,
        requestedSamples,
        fbo.format().samples(),
        lineSmoothingEnabled,
        blendEnabled
    );
}

std::vector<Gui::GpuDiagnosticsVisualAaProbe> collectVisualAaProbes(QOpenGLWidget* glWidget)
{
    std::vector<Gui::GpuDiagnosticsVisualAaProbe> probes;
    if (!glWidget || !glWidget->context()) {
        return probes;
    }

    glWidget->makeCurrent();
    probes.push_back(
        renderGlLineVisualAaProbe(QStringLiteral("gl-lines-aliased"), 0, false, false, false)
    );
    probes.push_back(
        renderGlLineVisualAaProbe(QStringLiteral("gl-lines-line-smooth"), 0, true, true, false)
    );
    probes.push_back(
        renderGlLineVisualAaProbe(QStringLiteral("gl-lines-msaa-8x"), 8, false, false, true)
    );
    probes.push_back(analyzeVisualAaImage(
        QStringLiteral("screen-space-reference"),
        QObject::tr("CPU reference"),
        makeScreenSpaceReferenceImage(),
        0,
        std::nullopt,
        false,
        false
    ));
    glWidget->doneCurrent();

    return probes;
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

QWidget* createObjectStylesTab(const Gui::GpuDiagnosticsReport& report)
{
    auto* tab = new QWidget;
    auto* layout = new QVBoxLayout(tab);

    if (report.objectStyles.empty()) {
        layout->addWidget(new QLabel(QObject::tr("No active document object styles were found.")));
        layout->addStretch();
        return tab;
    }

    if (report.objectStylesTruncated) {
        layout->addWidget(new QLabel(
            QObject::tr("Object style diagnostics were truncated to the first 200 objects.")
        ));
    }

    for (std::size_t index = 0; index < report.objectStyles.size(); ++index) {
        const auto& object = report.objectStyles[index];
        auto* form = new QFormLayout;
        addFormRow(form, QObject::tr("Object"), QString::number(index + 1));
        addFormRow(form, QObject::tr("Document"), object.documentName);
        addFormRow(form, QObject::tr("Name"), object.objectName);
        addFormRow(form, QObject::tr("Label"), object.label);
        addFormRow(form, QObject::tr("Object type"), object.objectType);
        addFormRow(form, QObject::tr("View provider"), object.viewProviderType);
        addFormRow(form, QObject::tr("Visible"), object.visible ? QObject::tr("Yes") : QObject::tr("No"));
        addFormRow(form, QObject::tr("Display mode"), object.activeDisplayMode);
        addFormRow(form, QObject::tr("ViewObject line width"), optionalDoubleToString(object.lineWidth));
        addFormRow(form, QObject::tr("ViewObject point size"), optionalDoubleToString(object.pointSize));
        layout->addLayout(form);
    }

    layout->addStretch();
    return tab;
}

QWidget* createRenderProbesTab(const Gui::GpuDiagnosticsReport& report)
{
    auto* tab = new QWidget;
    auto* layout = new QVBoxLayout(tab);

    if (report.renderProbes.empty()) {
        layout->addWidget(new QLabel(QObject::tr("No BRep edge render probes have been captured.")));
        layout->addStretch();
        return tab;
    }

    if (report.renderProbesTruncated) {
        layout->addWidget(new QLabel(
            QObject::tr("BRep edge render probes were truncated to the first 64 snapshots.")
        ));
    }

    for (std::size_t index = 0; index < report.renderProbes.size(); ++index) {
        const auto& probe = report.renderProbes[index];
        auto* form = new QFormLayout;
        addFormRow(form, QObject::tr("Probe"), QString::number(index + 1));
        addFormRow(form, QObject::tr("Node"), probe.node);
        addFormRow(form, QObject::tr("Stage"), probe.stage);
        addFormRow(form, QObject::tr("Coordinate indices"), QString::number(probe.coordIndexCount));
        addFormRow(
            form,
            QObject::tr("Action smoothing"),
            probe.actionSmoothing ? QObject::tr("Enabled") : QObject::tr("Disabled")
        );
        addFormRow(
            form,
            QObject::tr("Delayed paths"),
            probe.renderingDelayedPaths ? QObject::tr("Yes") : QObject::tr("No")
        );
        addFormRow(form, QObject::tr("GL samples"), optionalIntToString(probe.glSamples));
        addFormRow(form, QObject::tr("GL sample buffers"), optionalIntToString(probe.glSampleBuffers));
        addFormRow(form, QObject::tr("GL multisample"), optionalBoolToString(probe.glMultisampleEnabled));
        addFormRow(form, QObject::tr("GL line smooth"), optionalBoolToString(probe.glLineSmoothEnabled));
        addFormRow(form, QObject::tr("GL blend"), optionalBoolToString(probe.glBlendEnabled));
        addFormRow(
            form,
            QObject::tr("Line smoothing blend ready"),
            optionalBoolToString(probe.lineSmoothingBlendReady)
        );
        addFormRow(form, QObject::tr("Blend source RGB"), blendFactorToString(probe.glBlendSrcRgb));
        addFormRow(form, QObject::tr("Blend destination RGB"), blendFactorToString(probe.glBlendDstRgb));
        addFormRow(form, QObject::tr("Blend source alpha"), blendFactorToString(probe.glBlendSrcAlpha));
        addFormRow(
            form,
            QObject::tr("Blend destination alpha"),
            blendFactorToString(probe.glBlendDstAlpha)
        );
        addFormRow(form, QObject::tr("Line width"), optionalDoubleToString(probe.glLineWidth));
        addFormRow(form, QObject::tr("Line smooth hint"), lineSmoothHintToString(probe.glLineSmoothHint));
        addFormRow(form, QObject::tr("Draw FBO"), optionalIntToString(probe.drawFramebuffer));
        addFormRow(form, QObject::tr("Read FBO"), optionalIntToString(probe.readFramebuffer));
        addFormRow(form, QObject::tr("Renderer"), probe.renderer);
        layout->addLayout(form);
    }

    layout->addStretch();
    return tab;
}

QWidget* createVisualAaProbesTab(const Gui::GpuDiagnosticsReport& report)
{
    auto* tab = new QWidget;
    auto* layout = new QVBoxLayout(tab);

    if (report.visualAaProbes.empty()) {
        layout->addWidget(new QLabel(QObject::tr("No visual anti-aliasing probes were captured.")));
        layout->addStretch();
        return tab;
    }

    for (std::size_t index = 0; index < report.visualAaProbes.size(); ++index) {
        const auto& probe = report.visualAaProbes[index];
        auto* form = new QFormLayout;
        addFormRow(form, QObject::tr("Probe"), QString::number(index + 1));
        addFormRow(form, QObject::tr("Name"), probe.name);
        addFormRow(form, QObject::tr("Status"), probe.status);
        addFormRow(form, QObject::tr("Size"), QStringLiteral("%1x%2").arg(probe.width).arg(probe.height));
        addFormRow(form, QObject::tr("Requested samples"), QString::number(probe.requestedSamples));
        addFormRow(form, QObject::tr("Actual samples"), optionalIntToString(probe.actualSamples));
        addFormRow(
            form,
            QObject::tr("Line smoothing"),
            probe.lineSmoothingEnabled ? QObject::tr("Enabled") : QObject::tr("Disabled")
        );
        addFormRow(
            form,
            QObject::tr("Blending"),
            probe.blendEnabled ? QObject::tr("Enabled") : QObject::tr("Disabled")
        );
        addFormRow(form, QObject::tr("Covered pixels"), QString::number(probe.coveredPixelCount));
        addFormRow(
            form,
            QObject::tr("Intermediate pixels"),
            QString::number(probe.intermediatePixelCount)
        );
        addFormRow(form, QObject::tr("Dark pixels"), QString::number(probe.darkPixelCount));
        addFormRow(
            form,
            QObject::tr("Distinct luminance values"),
            QString::number(probe.distinctLuminanceCount)
        );
        addFormRow(
            form,
            QObject::tr("Intermediate pixel ratio"),
            QString::number(probe.intermediatePixelRatio, 'f', 4)
        );
        addFormRow(
            form,
            QObject::tr("Mean covered luminance"),
            QString::number(probe.meanCoveredLuminance, 'f', 2)
        );
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

    QOpenGLWidget* visualProbeWidget = nullptr;
    int visualProbeWidgetArea = 0;
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
            const int widgetArea = glWidget->size().width() * glWidget->size().height();
            if (widgetArea > visualProbeWidgetArea && glWidget->size().width() >= 64
                && glWidget->size().height() >= 64) {
                visualProbeWidget = glWidget;
                visualProbeWidgetArea = widgetArea;
            }

            glWidget->makeCurrent();
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
            glWidget->doneCurrent();
        }
        else {
            viewport.status = QObject::tr("OpenGL context is not initialized");
        }

        report.viewports.push_back(viewport);
    }

    report.visualAaProbes = collectVisualAaProbes(visualProbeWidget);

    if (Application::Instance) {
        constexpr std::size_t maxObjectStyles = 200;
        if (auto* guiDocument = Application::Instance->activeDocument()) {
            if (auto* appDocument = guiDocument->getDocument()) {
                const auto& objects = appDocument->getObjects();
                report.objectStyles.reserve(std::min(objects.size(), maxObjectStyles));
                for (const auto* object : objects) {
                    if (!object) {
                        continue;
                    }
                    if (report.objectStyles.size() >= maxObjectStyles) {
                        report.objectStylesTruncated = true;
                        break;
                    }

                    GpuDiagnosticsObjectStyle style;
                    style.documentName = QString::fromUtf8(appDocument->getName());
                    style.objectName = QString::fromUtf8(object->getNameInDocument());
                    style.label = QString::fromStdString(object->Label.getStrValue());
                    style.objectType = QString::fromLatin1(object->getTypeId().getName());

                    if (const auto* viewProvider = guiDocument->getViewProvider(object)) {
                        style.viewProviderType = QString::fromLatin1(
                            viewProvider->getTypeId().getName()
                        );
                        style.visible = viewProvider->isShow();
                        style.activeDisplayMode = QString::fromStdString(
                            viewProvider->getActiveDisplayMode()
                        );
                        style.lineWidth = propertyDoubleValue(viewProvider, "LineWidth");
                        style.pointSize = propertyDoubleValue(viewProvider, "PointSize");
                    }
                    else {
                        style.viewProviderType = QObject::tr("Unavailable");
                        style.activeDisplayMode = QObject::tr("Unavailable");
                    }

                    report.objectStyles.push_back(style);
                }
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(renderProbeMutex);
        report.renderProbes = renderProbes;
        report.renderProbesTruncated = renderProbesTruncated;
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
        bool hasEdgeProbe = false;
        bool hasEdgeActionSmoothing = false;
        bool hasEdgeLineSmoothing = false;
        bool hasEdgeLineSmoothingWithBlend = false;
        bool hasEdgeSamples = false;
        for (const auto& probe : report.renderProbes) {
            hasEdgeProbe = true;
            hasEdgeActionSmoothing = hasEdgeActionSmoothing || probe.actionSmoothing;
            hasEdgeLineSmoothing = hasEdgeLineSmoothing || probe.glLineSmoothEnabled.value_or(false);
            hasEdgeLineSmoothingWithBlend = hasEdgeLineSmoothingWithBlend
                || probe.lineSmoothingBlendReady.value_or(false);
            hasEdgeSamples = hasEdgeSamples || probe.glSamples.value_or(0) > 0;
        }

        if (report.lineSmoothingRequested && hasLineSmoothing && hasLineSmoothingWithBlend) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QObject::tr(
                "Line smoothing was requested and at least one OpenGL viewport reports "
                "GL_LINE_SMOOTH and GL_BLEND enabled."
            );
        }
        else if (report.lineSmoothingRequested && hasEdgeLineSmoothingWithBlend) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QObject::tr(
                "Line smoothing was requested and a BRep edge render probe reports GL_LINE_SMOOTH "
                "and GL_BLEND enabled during edge drawing."
            );
        }
        else if (report.lineSmoothingRequested && hasLineSmoothing) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QObject::tr(
                "Line smoothing was requested and GL_LINE_SMOOTH is enabled, but no OpenGL "
                "viewport reports GL_BLEND enabled. Alpha line smoothing may be ineffective."
            );
        }
        else if (report.lineSmoothingRequested && hasEdgeLineSmoothing) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            report.summary = QObject::tr(
                "Line smoothing was requested and a BRep edge render probe reports GL_LINE_SMOOTH "
                "during edge drawing, but not GL_BLEND. Alpha line smoothing may be ineffective."
            );
        }
        else if (report.lineSmoothingRequested && hasEdgeProbe && hasEdgeActionSmoothing) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Inactive;
            report.summary = QObject::tr(
                "Line smoothing was requested and Coin edge render action smoothing is enabled, "
                "but BRep edge render probes do not report GL_LINE_SMOOTH enabled during edge "
                "drawing."
            );
        }
        else if (report.lineSmoothingRequested) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Inactive;
            report.summary
                = QObject::tr("Line smoothing was requested, but no OpenGL viewport reports GL_LINE_SMOOTH enabled.");
        }
        else if (report.msaaRequested && hasActualSamples) {
            report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Active;
            if (hasEdgeProbe && !hasEdgeSamples) {
                report.summary = QObject::tr(
                    "MSAA was requested and the viewport reports active samples, but captured BRep "
                    "edge render probes do not report active GL samples during edge drawing."
                );
            }
            else {
                report.summary = QObject::tr(
                    "MSAA was requested and at least one OpenGL viewport reports active samples."
                );
            }
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
    report.objectStylesTruncated = false;
    report.renderProbesTruncated = false;

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
        item.insert(QStringLiteral("vendor"), viewport.vendor);
        item.insert(QStringLiteral("renderer"), viewport.renderer);
        item.insert(QStringLiteral("version"), viewport.version);
        item.insert(QStringLiteral("status"), viewport.status);
        viewports.append(item);
    }
    root.insert(QStringLiteral("viewports"), viewports);

    QJsonArray objectStyles;
    for (const auto& style : report.objectStyles) {
        QJsonObject item;
        item.insert(QStringLiteral("documentName"), style.documentName);
        item.insert(QStringLiteral("objectName"), style.objectName);
        item.insert(QStringLiteral("label"), style.label);
        item.insert(QStringLiteral("objectType"), style.objectType);
        item.insert(QStringLiteral("viewProviderType"), style.viewProviderType);
        item.insert(QStringLiteral("visible"), style.visible);
        item.insert(QStringLiteral("activeDisplayMode"), style.activeDisplayMode);
        item.insert(QStringLiteral("lineWidth"), optionalDoubleToJson(style.lineWidth));
        item.insert(QStringLiteral("pointSize"), optionalDoubleToJson(style.pointSize));
        objectStyles.append(item);
    }
    root.insert(QStringLiteral("objectStyles"), objectStyles);
    root.insert(QStringLiteral("objectStylesTruncated"), report.objectStylesTruncated);

    QJsonArray renderProbesJson;
    for (const auto& probe : report.renderProbes) {
        QJsonObject item;
        item.insert(QStringLiteral("node"), probe.node);
        item.insert(QStringLiteral("stage"), probe.stage);
        item.insert(QStringLiteral("coordIndexCount"), probe.coordIndexCount);
        item.insert(QStringLiteral("actionSmoothing"), probe.actionSmoothing);
        item.insert(QStringLiteral("renderingDelayedPaths"), probe.renderingDelayedPaths);
        item.insert(QStringLiteral("glSampleBuffers"), optionalIntToJson(probe.glSampleBuffers));
        item.insert(QStringLiteral("glSamples"), optionalIntToJson(probe.glSamples));
        item.insert(
            QStringLiteral("glMultisampleEnabled"),
            optionalBoolToJson(probe.glMultisampleEnabled)
        );
        item.insert(
            QStringLiteral("glLineSmoothEnabled"),
            optionalBoolToJson(probe.glLineSmoothEnabled)
        );
        item.insert(QStringLiteral("glBlendEnabled"), optionalBoolToJson(probe.glBlendEnabled));
        item.insert(
            QStringLiteral("lineSmoothingBlendReady"),
            optionalBoolToJson(probe.lineSmoothingBlendReady)
        );
        item.insert(
            QStringLiteral("glBlendSrcRgb"),
            optionalStringToJson(
                blendFactorToString(probe.glBlendSrcRgb),
                probe.glBlendSrcRgb.has_value()
            )
        );
        item.insert(
            QStringLiteral("glBlendDstRgb"),
            optionalStringToJson(
                blendFactorToString(probe.glBlendDstRgb),
                probe.glBlendDstRgb.has_value()
            )
        );
        item.insert(
            QStringLiteral("glBlendSrcAlpha"),
            optionalStringToJson(
                blendFactorToString(probe.glBlendSrcAlpha),
                probe.glBlendSrcAlpha.has_value()
            )
        );
        item.insert(
            QStringLiteral("glBlendDstAlpha"),
            optionalStringToJson(
                blendFactorToString(probe.glBlendDstAlpha),
                probe.glBlendDstAlpha.has_value()
            )
        );
        item.insert(QStringLiteral("glLineWidth"), optionalDoubleToJson(probe.glLineWidth));
        item.insert(QStringLiteral("glLineSmoothHint"), lineSmoothHintToString(probe.glLineSmoothHint));
        item.insert(QStringLiteral("drawFramebuffer"), optionalIntToJson(probe.drawFramebuffer));
        item.insert(QStringLiteral("readFramebuffer"), optionalIntToJson(probe.readFramebuffer));
        item.insert(QStringLiteral("vendor"), probe.vendor);
        item.insert(QStringLiteral("renderer"), probe.renderer);
        item.insert(QStringLiteral("version"), probe.version);
        renderProbesJson.append(item);
    }
    root.insert(QStringLiteral("renderProbes"), renderProbesJson);
    root.insert(QStringLiteral("renderProbesTruncated"), report.renderProbesTruncated);

    QJsonArray visualAaProbesJson;
    for (const auto& probe : report.visualAaProbes) {
        QJsonObject item;
        item.insert(QStringLiteral("name"), probe.name);
        item.insert(QStringLiteral("status"), probe.status);
        item.insert(QStringLiteral("width"), probe.width);
        item.insert(QStringLiteral("height"), probe.height);
        item.insert(QStringLiteral("requestedSamples"), probe.requestedSamples);
        item.insert(QStringLiteral("actualSamples"), optionalIntToJson(probe.actualSamples));
        item.insert(QStringLiteral("lineSmoothingEnabled"), probe.lineSmoothingEnabled);
        item.insert(QStringLiteral("blendEnabled"), probe.blendEnabled);
        item.insert(QStringLiteral("coveredPixelCount"), probe.coveredPixelCount);
        item.insert(QStringLiteral("intermediatePixelCount"), probe.intermediatePixelCount);
        item.insert(QStringLiteral("darkPixelCount"), probe.darkPixelCount);
        item.insert(QStringLiteral("distinctLuminanceCount"), probe.distinctLuminanceCount);
        item.insert(QStringLiteral("intermediatePixelRatio"), probe.intermediatePixelRatio);
        item.insert(QStringLiteral("meanCoveredLuminance"), probe.meanCoveredLuminance);
        visualAaProbesJson.append(item);
    }
    root.insert(QStringLiteral("visualAaProbes"), visualAaProbesJson);

    QJsonObject summary;
    bool lineSmoothingBlendReady = false;
    for (const auto& viewport : report.viewports) {
        lineSmoothingBlendReady = lineSmoothingBlendReady
            || viewport.lineSmoothingBlendReady.value_or(false);
    }
    bool edgeLineSmoothingBlendReady = false;
    bool edgeLineSmoothing = false;
    bool edgeActionSmoothing = false;
    bool edgeSamples = false;
    for (const auto& probe : report.renderProbes) {
        edgeLineSmoothingBlendReady = edgeLineSmoothingBlendReady
            || probe.lineSmoothingBlendReady.value_or(false);
        edgeLineSmoothing = edgeLineSmoothing || probe.glLineSmoothEnabled.value_or(false);
        edgeActionSmoothing = edgeActionSmoothing || probe.actionSmoothing;
        edgeSamples = edgeSamples || probe.glSamples.value_or(0) > 0;
    }
    summary.insert(QStringLiteral("msaaStatus"), msaaStatusToString(report.msaaStatus));
    summary.insert(QStringLiteral("msaaRequested"), report.msaaRequested);
    summary.insert(QStringLiteral("lineSmoothingRequested"), report.lineSmoothingRequested);
    summary.insert(QStringLiteral("lineSmoothingBlendReady"), lineSmoothingBlendReady);
    summary.insert(QStringLiteral("edgeRenderProbeCount"), static_cast<int>(report.renderProbes.size()));
    summary.insert(QStringLiteral("edgeActionSmoothing"), edgeActionSmoothing);
    summary.insert(QStringLiteral("edgeLineSmoothing"), edgeLineSmoothing);
    summary.insert(QStringLiteral("edgeLineSmoothingBlendReady"), edgeLineSmoothingBlendReady);
    summary.insert(QStringLiteral("edgeSamples"), edgeSamples);
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

    for (std::size_t i = 0; i < report.objectStyles.size(); ++i) {
        const auto& style = report.objectStyles[i];
        stream << "Object " << (i + 1) << ": document=\"" << style.documentName << "\", name=\""
               << style.objectName << "\", label=\"" << style.label << "\", object type=\""
               << style.objectType << "\", view provider=\"" << style.viewProviderType
               << "\", visible=" << (style.visible ? "true" : "false") << ", display mode=\""
               << style.activeDisplayMode
               << "\", line width=" << optionalDoubleToString(style.lineWidth)
               << ", point size=" << optionalDoubleToString(style.pointSize) << "\n";
    }
    if (report.objectStylesTruncated) {
        stream << "Object style diagnostics were truncated.\n";
    }

    for (std::size_t i = 0; i < report.renderProbes.size(); ++i) {
        const auto& probe = report.renderProbes[i];
        stream << "BRep edge render probe " << (i + 1) << ": node=\"" << probe.node
               << "\", stage=\"" << probe.stage << "\", coord indices=" << probe.coordIndexCount
               << ", action smoothing=" << (probe.actionSmoothing ? "true" : "false")
               << ", delayed paths=" << (probe.renderingDelayedPaths ? "true" : "false")
               << ", GL_SAMPLE_BUFFERS=" << optionalIntToString(probe.glSampleBuffers)
               << ", GL_SAMPLES=" << optionalIntToString(probe.glSamples)
               << ", GL_MULTISAMPLE=" << optionalBoolToString(probe.glMultisampleEnabled)
               << ", GL_LINE_SMOOTH=" << optionalBoolToString(probe.glLineSmoothEnabled)
               << ", GL_BLEND=" << optionalBoolToString(probe.glBlendEnabled)
               << ", LINE_SMOOTHING_BLEND_READY="
               << optionalBoolToString(probe.lineSmoothingBlendReady)
               << ", GL_BLEND_SRC_RGB=" << blendFactorToString(probe.glBlendSrcRgb)
               << ", GL_BLEND_DST_RGB=" << blendFactorToString(probe.glBlendDstRgb)
               << ", GL_BLEND_SRC_ALPHA=" << blendFactorToString(probe.glBlendSrcAlpha)
               << ", GL_BLEND_DST_ALPHA=" << blendFactorToString(probe.glBlendDstAlpha)
               << ", GL_LINE_WIDTH=" << optionalDoubleToString(probe.glLineWidth)
               << ", GL_LINE_SMOOTH_HINT=" << lineSmoothHintToString(probe.glLineSmoothHint)
               << ", draw FBO=" << optionalIntToString(probe.drawFramebuffer)
               << ", read FBO=" << optionalIntToString(probe.readFramebuffer) << ", vendor=\""
               << probe.vendor << "\", renderer=\"" << probe.renderer << "\", version=\""
               << probe.version << "\"\n";
    }
    if (report.renderProbesTruncated) {
        stream << "BRep edge render probes were truncated.\n";
    }

    for (std::size_t i = 0; i < report.visualAaProbes.size(); ++i) {
        const auto& probe = report.visualAaProbes[i];
        stream << "Visual AA probe " << (i + 1) << ": name=\"" << probe.name << "\", status=\""
               << probe.status << "\", size=" << probe.width << "x" << probe.height
               << ", requested samples=" << probe.requestedSamples
               << ", actual samples=" << optionalIntToString(probe.actualSamples)
               << ", line smoothing=" << (probe.lineSmoothingEnabled ? "true" : "false")
               << ", blend=" << (probe.blendEnabled ? "true" : "false")
               << ", covered pixels=" << probe.coveredPixelCount
               << ", intermediate pixels=" << probe.intermediatePixelCount
               << ", dark pixels=" << probe.darkPixelCount
               << ", distinct luminance values=" << probe.distinctLuminanceCount
               << ", intermediate pixel ratio="
               << QString::number(probe.intermediatePixelRatio, 'f', 4)
               << ", mean covered luminance=" << QString::number(probe.meanCoveredLuminance, 'f', 2)
               << "\n";
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

void Gui::GpuDiagnostics::recordRenderProbe(const GpuDiagnosticsRenderProbe& probe)
{
    std::lock_guard<std::mutex> lock(renderProbeMutex);
    if (renderProbes.size() >= maxRenderProbes) {
        renderProbesTruncated = true;
        return;
    }
    renderProbes.push_back(probe);
}

void Gui::GpuDiagnostics::clearRenderProbes()
{
    std::lock_guard<std::mutex> lock(renderProbeMutex);
    renderProbes.clear();
    renderProbesTruncated = false;
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
    tabs->addTab(createScrollableTab(createObjectStylesTab(report)), QObject::tr("Objects"));
    tabs->addTab(createScrollableTab(createRenderProbesTab(report)), QObject::tr("Edge Render"));
    tabs->addTab(createScrollableTab(createVisualAaProbesTab(report)), QObject::tr("Visual AA"));

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
