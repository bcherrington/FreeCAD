// SPDX-License-Identifier: LGPL-2.1-or-later

#include <optional>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <gtest/gtest.h>

#include "Gui/GpuDiagnostics.h"

TEST(GpuDiagnostics, RequestedSamplesWithZeroActualSamplesIsInactive)
{
    const auto report = Gui::GpuDiagnostics::makeReportForTesting(8, {0});

    EXPECT_EQ(report.msaaStatus, Gui::GpuDiagnosticsReport::MsaaStatus::Inactive);
    EXPECT_TRUE(report.summary.contains(QStringLiteral("zero active samples")));
}

TEST(GpuDiagnostics, RequestedSamplesWithActualSamplesIsActive)
{
    const auto report = Gui::GpuDiagnostics::makeReportForTesting(8, {4});

    EXPECT_EQ(report.msaaStatus, Gui::GpuDiagnosticsReport::MsaaStatus::Active);
    EXPECT_TRUE(report.summary.contains(QStringLiteral("active samples")));
}

TEST(GpuDiagnostics, MissingViewportIsUnknown)
{
    const auto report = Gui::GpuDiagnostics::makeReportForTesting(8, {});

    EXPECT_EQ(report.msaaStatus, Gui::GpuDiagnosticsReport::MsaaStatus::Unknown);
    EXPECT_TRUE(report.summary.contains(QStringLiteral("No OpenGL viewport")));
}

TEST(GpuDiagnostics, LineSmoothingIsReportedSeparatelyFromMsaa)
{
    auto report = Gui::GpuDiagnostics::makeReportForTesting(1, {0});
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
    auto report = Gui::GpuDiagnostics::makeReportForTesting(8, {std::nullopt});

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

    EXPECT_TRUE(root.contains(QStringLiteral("objectStyles")));
    EXPECT_TRUE(root.value(QStringLiteral("objectStyles")).isArray());
    EXPECT_FALSE(root.value(QStringLiteral("objectStylesTruncated")).toBool());
}

TEST(GpuDiagnostics, JsonExportIncludesRenderProbes)
{
    auto report = Gui::GpuDiagnostics::makeReportForTesting(1, {0});

    Gui::GpuDiagnosticsRenderProbe probe;
    probe.node = QStringLiteral("SoBrepEdgeSet");
    probe.stage = QStringLiteral("before-inherited");
    probe.coordIndexCount = 42;
    probe.actionSmoothing = true;
    probe.renderingDelayedPaths = false;
    probe.glLineSmoothEnabled = true;
    probe.glBlendEnabled = false;
    probe.lineSmoothingBlendReady = false;
    report.renderProbes.push_back(probe);

    const auto json = Gui::GpuDiagnostics::toJson(report);
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    ASSERT_TRUE(document.isObject());

    const auto root = document.object();
    const auto probes = root.value(QStringLiteral("renderProbes")).toArray();
    ASSERT_EQ(probes.size(), 1);
    const auto exportedProbe = probes.at(0).toObject();
    EXPECT_EQ(exportedProbe.value(QStringLiteral("node")).toString(), QStringLiteral("SoBrepEdgeSet"));
    EXPECT_EQ(
        exportedProbe.value(QStringLiteral("stage")).toString(),
        QStringLiteral("before-inherited")
    );
    EXPECT_EQ(exportedProbe.value(QStringLiteral("coordIndexCount")).toInt(), 42);
    EXPECT_TRUE(exportedProbe.value(QStringLiteral("actionSmoothing")).toBool());
    EXPECT_TRUE(exportedProbe.value(QStringLiteral("glLineSmoothEnabled")).toBool());
    EXPECT_FALSE(exportedProbe.value(QStringLiteral("lineSmoothingBlendReady")).toBool());

    const auto summary = root.value(QStringLiteral("summary")).toObject();
    EXPECT_EQ(summary.value(QStringLiteral("edgeRenderProbeCount")).toInt(), 1);
    EXPECT_TRUE(summary.value(QStringLiteral("edgeActionSmoothing")).toBool());
    EXPECT_TRUE(summary.value(QStringLiteral("edgeLineSmoothing")).toBool());
    EXPECT_FALSE(summary.value(QStringLiteral("edgeLineSmoothingBlendReady")).toBool());
}

TEST(GpuDiagnostics, JsonExportIncludesVisualAaProbes)
{
    auto report = Gui::GpuDiagnostics::makeReportForTesting(1, {0});

    Gui::GpuDiagnosticsVisualAaProbe probe;
    probe.name = QStringLiteral("line-smoothing");
    probe.status = QStringLiteral("Available");
    probe.width = 64;
    probe.height = 64;
    probe.requestedSamples = 1;
    probe.actualSamples = 0;
    probe.lineSmoothingEnabled = true;
    probe.blendEnabled = true;
    probe.coveredPixelCount = 42;
    probe.intermediatePixelCount = 12;
    probe.darkPixelCount = 30;
    probe.distinctLuminanceCount = 5;
    probe.intermediatePixelRatio = 0.285714;
    probe.meanCoveredLuminance = 120.5;
    report.visualAaProbes.push_back(probe);

    const auto json = Gui::GpuDiagnostics::toJson(report);
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    ASSERT_TRUE(document.isObject());

    const auto probes = document.object().value(QStringLiteral("visualAaProbes")).toArray();
    ASSERT_EQ(probes.size(), 1);
    const auto exportedProbe = probes.at(0).toObject();
    EXPECT_EQ(exportedProbe.value(QStringLiteral("name")).toString(), QStringLiteral("line-smoothing"));
    EXPECT_EQ(exportedProbe.value(QStringLiteral("width")).toInt(), 64);
    EXPECT_TRUE(exportedProbe.value(QStringLiteral("lineSmoothingEnabled")).toBool());
    EXPECT_EQ(exportedProbe.value(QStringLiteral("intermediatePixelCount")).toInt(), 12);
    EXPECT_GT(exportedProbe.value(QStringLiteral("intermediatePixelRatio")).toDouble(), 0.28);
}
