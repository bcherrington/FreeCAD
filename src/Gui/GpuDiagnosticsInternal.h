// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef GUI_GPUDIAGNOSTICSINTERNAL_H
#define GUI_GPUDIAGNOSTICSINTERNAL_H

#include <QObject>

#include "GpuDiagnostics.h"

namespace Gui::GpuDiagnosticsInternal
{

inline void classifyReport(GpuDiagnosticsReport& report)
{
    if (report.viewports.empty()) {
        report.msaaStatus = GpuDiagnosticsReport::MsaaStatus::Unknown;
        report.summary = QObject::tr("No OpenGL viewport was available for MSAA diagnostics.");
        return;
    }

    bool hasActualSamples = false;
    bool hasZeroSamples = false;
    bool hasLineSmoothing = false;
    bool hasLineSmoothingWithBlend = false;
    for (const auto& viewport : report.viewports) {
        const auto actualSamples = viewport.glSamples.value_or(
            viewport.contextFormatSamples.value_or(-1)
        );
        hasActualSamples = hasActualSamples || actualSamples > 0;
        hasZeroSamples = hasZeroSamples || actualSamples == 0;
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
        report.summary = QObject::tr(
            "Line smoothing was requested, but no OpenGL viewport reports GL_LINE_SMOOTH "
            "enabled."
        );
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

}  // namespace Gui::GpuDiagnosticsInternal

#endif  // GUI_GPUDIAGNOSTICSINTERNAL_H
