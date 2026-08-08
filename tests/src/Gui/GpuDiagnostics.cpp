// SPDX-License-Identifier: LGPL-2.1-or-later

#include <optional>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <gtest/gtest.h>

#include "Gui/GpuDiagnostics.h"
#include "Gui/GpuDiagnosticsInternal.h"

namespace
{

Gui::GpuDiagnosticsReport makeReport(
    int requestedSamples,
    const std::vector<std::optional<int>>& viewportSamples
)
{
    Gui::GpuDiagnosticsReport report;
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
    report.useVbo = true;
    report.useNewSelection = true;

    for (const auto& samples : viewportSamples) {
        Gui::GpuDiagnosticsViewport viewport;
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

    Gui::GpuDiagnosticsInternal::classifyReport(report);
    return report;
}

}  // namespace

TEST(GpuDiagnostics, RequestedSamplesWithZeroActualSamplesIsInactive)
{
    const auto report = makeReport(8, {0});

    EXPECT_EQ(report.msaaStatus, Gui::GpuDiagnosticsReport::MsaaStatus::Inactive);
    EXPECT_TRUE(report.summary.contains(QStringLiteral("zero active samples")));
}

TEST(GpuDiagnostics, RequestedSamplesWithActualSamplesIsActive)
{
    const auto report = makeReport(8, {4});

    EXPECT_EQ(report.msaaStatus, Gui::GpuDiagnosticsReport::MsaaStatus::Active);
    EXPECT_TRUE(report.summary.contains(QStringLiteral("active samples")));
}

TEST(GpuDiagnostics, MissingViewportIsUnknown)
{
    const auto report = makeReport(8, {});

    EXPECT_EQ(report.msaaStatus, Gui::GpuDiagnosticsReport::MsaaStatus::Unknown);
    EXPECT_TRUE(report.summary.contains(QStringLiteral("No OpenGL viewport")));
}

TEST(GpuDiagnostics, LineSmoothingIsReportedSeparatelyFromMsaa)
{
    auto report = makeReport(1, {0});
    report.viewports.front().glLineSmoothEnabled = true;
    report.viewports.front().glBlendEnabled = false;
    report.viewports.front().lineSmoothingBlendReady = false;

    const auto json = Gui::GpuDiagnostics::toJson(report);
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    ASSERT_TRUE(document.isObject());

    const auto root = document.object();
    const auto preferences = root.value(QStringLiteral("preferences")).toObject();
    EXPECT_FALSE(preferences.value(QStringLiteral("msaaRequested")).toBool());
    EXPECT_TRUE(preferences.value(QStringLiteral("lineSmoothingRequested")).toBool());

    const auto summary = root.value(QStringLiteral("summary")).toObject();
    EXPECT_FALSE(summary.value(QStringLiteral("msaaRequested")).toBool());
    EXPECT_TRUE(summary.value(QStringLiteral("lineSmoothingRequested")).toBool());
    EXPECT_FALSE(summary.value(QStringLiteral("lineSmoothingBlendReady")).toBool());
    EXPECT_FALSE(summary.value(QStringLiteral("msaaActive")).toBool());
    EXPECT_TRUE(summary.value(QStringLiteral("antiAliasingActive")).toBool());
    EXPECT_TRUE(
        summary.value(QStringLiteral("message")).toString().contains(QStringLiteral("GL_BLEND"))
    );
}

TEST(GpuDiagnostics, JsonExportIncludesSchemaSummaryAndUnavailableFields)
{
    auto report = makeReport(8, {std::nullopt});

    const auto json = Gui::GpuDiagnostics::toJson(report);
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    ASSERT_TRUE(document.isObject());

    const auto root = document.object();
    EXPECT_EQ(
        root.value(QStringLiteral("schema")).toString(),
        QStringLiteral("org.freecad.gpu-diagnostics.v1")
    );

    const auto preferences = root.value(QStringLiteral("preferences")).toObject();
    EXPECT_EQ(preferences.value(QStringLiteral("requestedSamples")).toInt(), 8);
    EXPECT_TRUE(preferences.value(QStringLiteral("msaaRequested")).toBool());
    EXPECT_FALSE(preferences.value(QStringLiteral("lineSmoothingRequested")).toBool());
    EXPECT_TRUE(preferences.contains(QStringLiteral("defaultShapeLineWidth")));

    const auto summary = root.value(QStringLiteral("summary")).toObject();
    EXPECT_EQ(summary.value(QStringLiteral("msaaStatus")).toString(), QStringLiteral("Unknown"));

    const auto viewports = root.value(QStringLiteral("viewports")).toArray();
    ASSERT_EQ(viewports.size(), 1);
    const auto viewport = viewports.at(0).toObject();
    EXPECT_TRUE(viewport.contains(QStringLiteral("widgetFormatSamples")));
    EXPECT_TRUE(viewport.value(QStringLiteral("widgetFormatSamples")).isNull());
    EXPECT_TRUE(viewport.contains(QStringLiteral("contextFormatSamples")));
    EXPECT_TRUE(viewport.value(QStringLiteral("contextFormatSamples")).isNull());
    EXPECT_TRUE(viewport.contains(QStringLiteral("glSamples")));
    EXPECT_TRUE(viewport.value(QStringLiteral("glSamples")).isNull());
    EXPECT_TRUE(viewport.contains(QStringLiteral("glMultisampleEnabled")));
    EXPECT_TRUE(viewport.value(QStringLiteral("glMultisampleEnabled")).isNull());
    EXPECT_TRUE(viewport.contains(QStringLiteral("glLineWidth")));
    EXPECT_TRUE(viewport.value(QStringLiteral("glLineWidth")).isNull());
    EXPECT_TRUE(viewport.contains(QStringLiteral("glBlendSrcRgb")));
    EXPECT_TRUE(viewport.value(QStringLiteral("glBlendSrcRgb")).isNull());
    EXPECT_TRUE(viewport.contains(QStringLiteral("lineSmoothingBlendReady")));
    EXPECT_TRUE(viewport.value(QStringLiteral("lineSmoothingBlendReady")).isNull());
    EXPECT_TRUE(viewport.value(QStringLiteral("contextProfile")).isNull());
    EXPECT_TRUE(viewport.value(QStringLiteral("vendor")).isNull());
    EXPECT_TRUE(viewport.value(QStringLiteral("renderer")).isNull());
    EXPECT_TRUE(viewport.value(QStringLiteral("version")).isNull());

    EXPECT_FALSE(root.contains(QStringLiteral("objectStyles")));
    EXPECT_FALSE(root.contains(QStringLiteral("renderProbes")));
    EXPECT_FALSE(root.contains(QStringLiteral("visualAaProbes")));
}
