// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef GUI_GPUDIAGNOSTICS_H
#define GUI_GPUDIAGNOSTICS_H

#include <optional>
#include <vector>

#include <QSize>
#include <QSizeF>
#include <QString>

#include <FCGlobal.h>

class QWidget;

namespace Gui
{

struct GuiExport GpuDiagnosticsScreen
{
    QString name;
    QString geometry;
    QString availableGeometry;
    QSizeF physicalSizeMm;
    qreal devicePixelRatio {0.0};
    qreal logicalDpiX {0.0};
    qreal logicalDpiY {0.0};
    qreal refreshRate {0.0};
};

struct GuiExport GpuDiagnosticsViewport
{
    QString widgetClass;
    QString objectName;
    QString screenName;
    QSize size;
    qreal devicePixelRatio {0.0};
    int requestedSamples {0};
    std::optional<int> widgetFormatSamples;
    std::optional<int> contextFormatSamples;
    std::optional<int> glSampleBuffers;
    std::optional<int> glSamples;
    std::optional<bool> glMultisampleEnabled;
    std::optional<bool> glLineSmoothEnabled;
    std::optional<bool> glBlendEnabled;
    std::optional<bool> lineSmoothingBlendReady;
    std::optional<bool> glDepthTestEnabled;
    std::optional<int> glBlendSrcRgb;
    std::optional<int> glBlendDstRgb;
    std::optional<int> glBlendSrcAlpha;
    std::optional<int> glBlendDstAlpha;
    std::optional<int> glBlendEquationRgb;
    std::optional<int> glBlendEquationAlpha;
    std::optional<double> glLineWidth;
    std::optional<double> glAliasedLineWidthRangeMin;
    std::optional<double> glAliasedLineWidthRangeMax;
    std::optional<int> glLineSmoothHint;
    std::optional<unsigned int> defaultFramebufferObject;
    std::optional<int> drawFramebuffer;
    std::optional<int> readFramebuffer;
    QString vendor;
    QString renderer;
    QString version;
    QString status;
};

struct GuiExport GpuDiagnosticsObjectStyle
{
    QString documentName;
    QString objectName;
    QString label;
    QString objectType;
    QString viewProviderType;
    QString activeDisplayMode;
    bool visible {false};
    std::optional<double> lineWidth;
    std::optional<double> pointSize;
};

struct GuiExport GpuDiagnosticsRenderProbe
{
    QString node;
    QString stage;
    int coordIndexCount {0};
    bool actionSmoothing {false};
    bool renderingDelayedPaths {false};
    std::optional<int> glSampleBuffers;
    std::optional<int> glSamples;
    std::optional<bool> glMultisampleEnabled;
    std::optional<bool> glLineSmoothEnabled;
    std::optional<bool> glBlendEnabled;
    std::optional<bool> lineSmoothingBlendReady;
    std::optional<int> glBlendSrcRgb;
    std::optional<int> glBlendDstRgb;
    std::optional<int> glBlendSrcAlpha;
    std::optional<int> glBlendDstAlpha;
    std::optional<double> glLineWidth;
    std::optional<int> glLineSmoothHint;
    std::optional<int> drawFramebuffer;
    std::optional<int> readFramebuffer;
    QString vendor;
    QString renderer;
    QString version;
};

struct GuiExport GpuDiagnosticsVisualAaProbe
{
    QString name;
    QString status;
    int width {0};
    int height {0};
    int requestedSamples {0};
    std::optional<int> actualSamples;
    bool lineSmoothingEnabled {false};
    bool blendEnabled {false};
    int coveredPixelCount {0};
    int intermediatePixelCount {0};
    int darkPixelCount {0};
    int distinctLuminanceCount {0};
    double intermediatePixelRatio {0.0};
    double meanCoveredLuminance {0.0};
};

struct GuiExport GpuDiagnosticsReport
{
    enum class MsaaStatus
    {
        Active,
        Inactive,
        Unknown
    };

    QString schema;
    QString freecadVersion;
    QString qtVersion;
    QString platform;
    QString antiAliasingMode;
    int defaultShapeLineWidth {0};
    int defaultShapePointSize {0};
    bool renderCache {false};
    bool useVbo {false};
    bool useNewSelection {false};
    int requestedSamples {0};
    bool msaaRequested {false};
    bool lineSmoothingRequested {false};
    MsaaStatus msaaStatus {MsaaStatus::Unknown};
    QString summary;
    std::vector<GpuDiagnosticsScreen> screens;
    std::vector<GpuDiagnosticsViewport> viewports;
    std::vector<GpuDiagnosticsObjectStyle> objectStyles;
    std::vector<GpuDiagnosticsRenderProbe> renderProbes;
    std::vector<GpuDiagnosticsVisualAaProbe> visualAaProbes;
    bool objectStylesTruncated {false};
    bool renderProbesTruncated {false};
};

class GuiExport GpuDiagnostics
{
public:
    static GpuDiagnosticsReport collect(QWidget* parent = nullptr);
    static GpuDiagnosticsReport makeReportForTesting(
        int requestedSamples,
        const std::vector<std::optional<int>>& viewportSamples
    );
    static QString toJson(const GpuDiagnosticsReport& report);
    static QString toText(const GpuDiagnosticsReport& report);
    static QString msaaStatusToString(GpuDiagnosticsReport::MsaaStatus status);
    static void recordRenderProbe(const GpuDiagnosticsRenderProbe& probe);
    static void clearRenderProbes();
};

class GuiExport GpuDiagnosticsDialog
{
public:
    static void showDialog(QWidget* parent = nullptr);
};

}  // namespace Gui

#endif  // GUI_GPUDIAGNOSTICS_H
