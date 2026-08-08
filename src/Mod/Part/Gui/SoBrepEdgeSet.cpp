// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <FCConfig.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QSurface>
#include <QVector2D>
#include <QVector4D>
#include <QWindow>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoTextureEnabledElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/actions/SoSearchAction.h>

#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Gui/Selection/Selection.h>
#include <Base/Color.h>
#include "SoBrepEdgeSet.h"
#include "ProjectedSegmentDeduplication.h"
#include "ScreenSpaceEdgeGeometry.h"
#include "ViewProviderExt.h"

#include <Gui/Inventor/So3DAnnotation.h>


using namespace PartGui;

SO_NODE_SOURCE(SoBrepEdgeSet)

struct SoBrepEdgeSet::SelContext: Gui::SoFCSelectionContextEx
{
    std::vector<int32_t> hl, sl;
};

/// Controls how B-rep overlay primitives interact with the scene depth buffer.
enum class OverlayDepthMode
{
    /// Keep normal occlusion so committed selection does not expose hidden geometry.
    RespectDepth,
    /// Render above model geometry for hover and preselection feedback.
    DrawOnTop,
};

static void applyOverlayPrimitiveState(SoState* state, SoNode* node)
{
    if (!state || !node) {
        return;
    }

    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoTextureEnabledElement::set(state, node, false);
    SoMaterialBindingElement::set(state, SoMaterialBindingElement::OVERALL);
    SoOverrideElement::setMaterialBindingOverride(state, node, true);
}

static void applyOverlayDepthState(SoState* state, OverlayDepthMode depthMode)
{
    switch (depthMode) {
        case OverlayDepthMode::DrawOnTop:
            SoDepthBufferElement::set(
                state,
                FALSE,
                FALSE,
                SoDepthBufferElement::ALWAYS,
                SbVec2f(0.0f, 1.0f)
            );
            return;
        case OverlayDepthMode::RespectDepth:
            SoDepthBufferElement::set(
                state,
                TRUE,
                FALSE,
                SoDepthBufferElement::LEQUAL,
                SbVec2f(0.0f, 1.0f)
            );
            return;
    }
}

static void renderOverlayLines(
    SoGLRenderAction* action,
    SoIndexedLineSet* lineSet,
    const int32_t* indices,
    int numIndices,
    const Base::Color& color,
    OverlayDepthMode depthMode
)
{
    if (!action || !lineSet || !indices || numIndices <= 0) {
        return;
    }

    // Match the legacy GL path by drawing each edge segment independently.
    std::vector<int32_t> lineIndices;
    lineIndices.reserve(static_cast<size_t>(numIndices) * 3);

    int32_t previous = -1;
    for (int i = 0; i < numIndices; i++) {
        const int32_t current = indices[i];
        if (current < 0) {
            previous = -1;
            continue;
        }
        if (previous >= 0) {
            lineIndices.push_back(previous);
            lineIndices.push_back(current);
            lineIndices.push_back(-1);
        }
        previous = current;
    }

    if (lineIndices.empty()) {
        return;
    }

    auto state = action->getState();
    state->push();

    applyOverlayPrimitiveState(state, lineSet);
    applyOverlayDepthState(state, depthMode);

    const SbColor sbColor(color.r, color.g, color.b);
    const float transparency = std::max(0.0f, 1.0f - color.a);
    const bool hasTransparency = transparency > 0.0f;
    if (hasTransparency) {
        SoShapeStyleElement::setTransparencyType(state, SoGLRenderAction::BLEND);
        SoLazyElement::setTransparencyType(state, SoGLRenderAction::BLEND);
    }

    SoLazyElement::setEmissive(state, &sbColor);
    uint32_t packedColor = sbColor.getPackedValue(transparency);
    SoLazyElement::setPacked(state, lineSet, 1, &packedColor, hasTransparency);

    // setValues() does not shrink the field, so rewrite the overlay index
    // array to the exact size to avoid stale segments from the previous
    // highlight.
    lineSet->coordIndex.setNum(static_cast<int>(lineIndices.size()));
    int32_t* coordIndex = lineSet->coordIndex.startEditing();
    std::copy(lineIndices.begin(), lineIndices.end(), coordIndex);
    lineSet->coordIndex.finishEditing();
    lineSet->GLRender(action);

    state->pop();
}

static void renderOverlayLines(
    SoGLRenderAction* action,
    SoIndexedLineSet* lineSet,
    const int32_t* indices,
    int numIndices,
    const SbColor& color,
    OverlayDepthMode depthMode
)
{
    renderOverlayLines(
        action,
        lineSet,
        indices,
        numIndices,
        Base::Color(color[0], color[1], color[2], 1.0f),
        depthMode
    );
}

static void renderColorOverrides(
    SoGLRenderAction* action,
    SoIndexedLineSet* lineSet,
    const int32_t* indices,
    int numIndices,
    const std::map<int, Base::Color>& colors
)
{
    if (!action || !lineSet || !indices || numIndices <= 0 || colors.empty()) {
        return;
    }

    struct ColorGroup
    {
        Base::Color color;
        std::vector<int32_t> indices;
    };

    std::map<uint32_t, ColorGroup> colorGroups;
    const auto wildcard = colors.find(-1);

    int lineIndex = 0;
    for (int i = 0; i < numIndices; ++lineIndex) {
        const int sectionStart = i;
        while (i < numIndices && indices[i] >= 0) {
            ++i;
        }

        const Base::Color* color = nullptr;
        auto it = colors.find(lineIndex);
        if (it != colors.end()) {
            color = &it->second;
        }
        else if (wildcard != colors.end()) {
            color = &wildcard->second;
        }

        if (color) {
            const SbColor sbColor(color->r, color->g, color->b);
            const uint32_t key = sbColor.getPackedValue(std::max(0.0f, 1.0f - color->a));
            auto& group = colorGroups[key];
            if (group.indices.empty()) {
                group.color = *color;
            }
            group.indices.insert(group.indices.end(), indices + sectionStart, indices + i);
            group.indices.push_back(-1);
        }

        if (i < numIndices && indices[i] < 0) {
            ++i;
        }
    }

    for (const auto& [_, group] : colorGroups) {
        renderOverlayLines(
            action,
            lineSet,
            group.indices.data(),
            static_cast<int>(group.indices.size()),
            group.color,
            OverlayDepthMode::DrawOnTop
        );
    }
}

enum class EdgeAaDiagnosticMode
{
    Disabled,
    Hide,
    LineSmooth,
    LineSmoothOff,
    DeduplicateScreenSpace,
    ShaderScreenSpace,
    ShaderScreenSpaceDeduplicated,
    ShaderScreenSpaceOverlay,
    ScreenSpaceDebug,
    ScreenSpaceOnly,
    ScreenSpaceOverlay,
    SuppressOverlays,
};

static EdgeAaDiagnosticMode edgeAaDiagnosticMode()
{
    static const EdgeAaDiagnosticMode mode = [] {
        const char* value = std::getenv("FREECAD_EDGE_AA_DIAGNOSTIC");
        if (!value) {
            return EdgeAaDiagnosticMode::Disabled;
        }

        const std::string_view configuredMode(value);
        if (configuredMode == "hide") {
            return EdgeAaDiagnosticMode::Hide;
        }
        if (configuredMode == "line-smooth") {
            return EdgeAaDiagnosticMode::LineSmooth;
        }
        if (configuredMode == "line-smooth-off") {
            return EdgeAaDiagnosticMode::LineSmoothOff;
        }
        if (configuredMode == "dedup-screen-space") {
            return EdgeAaDiagnosticMode::DeduplicateScreenSpace;
        }
        if (configuredMode == "shader-screen-space") {
            return EdgeAaDiagnosticMode::ShaderScreenSpace;
        }
        if (configuredMode == "shader-screen-space-dedup") {
            return EdgeAaDiagnosticMode::ShaderScreenSpaceDeduplicated;
        }
        if (configuredMode == "shader-screen-space-overlay") {
            return EdgeAaDiagnosticMode::ShaderScreenSpaceOverlay;
        }
        if (configuredMode == "screen-space-debug") {
            return EdgeAaDiagnosticMode::ScreenSpaceDebug;
        }
        if (configuredMode == "screen-space-only") {
            return EdgeAaDiagnosticMode::ScreenSpaceOnly;
        }
        if (configuredMode == "screen-space" || configuredMode == "screen-space-overlay") {
            return EdgeAaDiagnosticMode::ScreenSpaceOverlay;
        }
        if (configuredMode == "suppress-overlays") {
            return EdgeAaDiagnosticMode::SuppressOverlays;
        }

        return EdgeAaDiagnosticMode::Disabled;
    }();
    return mode;
}

static bool isScreenSpaceOnlyMode(EdgeAaDiagnosticMode mode)
{
    return mode == EdgeAaDiagnosticMode::ScreenSpaceDebug
        || mode == EdgeAaDiagnosticMode::ScreenSpaceOnly
        || mode == EdgeAaDiagnosticMode::DeduplicateScreenSpace;
}

static bool isLineSmoothComparisonMode(EdgeAaDiagnosticMode mode)
{
    return mode == EdgeAaDiagnosticMode::LineSmooth || mode == EdgeAaDiagnosticMode::LineSmoothOff;
}

static bool isShaderScreenSpaceOnlyMode(EdgeAaDiagnosticMode mode)
{
    return mode == EdgeAaDiagnosticMode::ShaderScreenSpace
        || mode == EdgeAaDiagnosticMode::ShaderScreenSpaceDeduplicated;
}

static bool isShaderScreenSpaceMode(EdgeAaDiagnosticMode mode)
{
    return isShaderScreenSpaceOnlyMode(mode)
        || mode == EdgeAaDiagnosticMode::ShaderScreenSpaceOverlay;
}

static double edgeAaDeduplicationTolerance()
{
    static const double tolerance = [] {
        const char* value = std::getenv("FREECAD_EDGE_AA_DEDUP_TOLERANCE_PX");
        if (!value) {
            return 0.5;
        }

        char* end = nullptr;
        const double parsed = std::strtod(value, &end);
        if (end != value && *end == '\0' && std::isfinite(parsed) && parsed > 0.0) {
            return parsed;
        }
        return 0.5;
    }();
    return tolerance;
}

static bool edgeAaDeduplicationStatsEnabled()
{
    const char* value = std::getenv("FREECAD_EDGE_AA_DIAGNOSTIC_STATS");
    return value && std::string_view(value) != "0";
}

static double edgeAaShaderLogicalWidth()
{
    static const double width = [] {
        const char* value = std::getenv("FREECAD_EDGE_AA_SHADER_WIDTH_PX");
        if (!value) {
            return 1.5;
        }

        char* end = nullptr;
        const double parsed = std::strtod(value, &end);
        if (end != value && *end == '\0' && std::isfinite(parsed) && parsed > 0.0) {
            return parsed;
        }
        return 1.5;
    }();
    return width;
}

static double currentDevicePixelRatio()
{
    auto* context = QOpenGLContext::currentContext();
    auto* surface = context ? context->surface() : nullptr;
    if (surface && surface->surfaceClass() == QSurface::Window) {
        const double ratio = static_cast<QWindow*>(surface)->devicePixelRatio();
        if (std::isfinite(ratio) && ratio > 0.0) {
            return ratio;
        }
    }
    return 1.0;
}

struct ProjectedPoint
{
    double x {0.0};
    double y {0.0};
    double z {0.0};
    bool valid {false};
};

static ProjectedPoint projectPoint(
    const SbVec3f& point,
    const GLdouble* modelView,
    const GLdouble* projection,
    const GLint* viewport
)
{
    const double object[4] = {point[0], point[1], point[2], 1.0};
    double eye[4] = {0.0, 0.0, 0.0, 0.0};
    double clip[4] = {0.0, 0.0, 0.0, 0.0};

    for (int row = 0; row < 4; ++row) {
        eye[row] = modelView[row] * object[0] + modelView[4 + row] * object[1]
            + modelView[8 + row] * object[2] + modelView[12 + row] * object[3];
    }
    for (int row = 0; row < 4; ++row) {
        clip[row] = projection[row] * eye[0] + projection[4 + row] * eye[1]
            + projection[8 + row] * eye[2] + projection[12 + row] * eye[3];
    }

    if (std::abs(clip[3]) <= 1.0e-9) {
        return {};
    }

    const double ndcX = clip[0] / clip[3];
    const double ndcY = clip[1] / clip[3];
    const double ndcZ = clip[2] / clip[3];
    const ProjectedPoint projected {
        viewport[0] + (ndcX + 1.0) * viewport[2] * 0.5,
        viewport[1] + (ndcY + 1.0) * viewport[3] * 0.5,
        ndcZ,
        true,
    };
    if (!std::isfinite(projected.x) || !std::isfinite(projected.y) || !std::isfinite(projected.z)) {
        return {};
    }
    return projected;
}

class ScopedScreenSpaceEdgeAaState
{
public:
    explicit ScopedScreenSpaceEdgeAaState(const GLint* viewport)
    {
        glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);
        previousDepthTest = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
        previousBlend = glIsEnabled(GL_BLEND) == GL_TRUE;
        previousLighting = glIsEnabled(GL_LIGHTING) == GL_TRUE;
        previousTexture2D = glIsEnabled(GL_TEXTURE_2D) == GL_TRUE;
        previousCullFace = glIsEnabled(GL_CULL_FACE) == GL_TRUE;
        previousLineSmooth = glIsEnabled(GL_LINE_SMOOTH) == GL_TRUE;
        glGetIntegerv(GL_SHADE_MODEL, &previousShadeModel);
        glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
        glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSrcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSrcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDstAlpha);
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        glGetFloatv(GL_CURRENT_COLOR, previousColor.data());
        if (auto* context = QOpenGLContext::currentContext()) {
            openGlFunctions = context->functions();
        }

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, viewport[2], 0.0, viewport[3], -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LINE_SMOOTH);
        if (openGlFunctions) {
            openGlFunctions->glUseProgram(0);
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glShadeModel(GL_SMOOTH);
        glDepthMask(GL_FALSE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }

    ~ScopedScreenSpaceEdgeAaState()
    {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(previousMatrixMode);

        glDepthMask(previousDepthMask);
        glDepthFunc(static_cast<GLenum>(previousDepthFunc));
        if (openGlFunctions) {
            openGlFunctions->glBlendFuncSeparate(
                static_cast<GLenum>(previousBlendSrcRgb),
                static_cast<GLenum>(previousBlendDstRgb),
                static_cast<GLenum>(previousBlendSrcAlpha),
                static_cast<GLenum>(previousBlendDstAlpha)
            );
            openGlFunctions->glUseProgram(static_cast<GLuint>(previousProgram));
        }
        else {
            glBlendFunc(
                static_cast<GLenum>(previousBlendSrcRgb),
                static_cast<GLenum>(previousBlendDstRgb)
            );
        }
        glShadeModel(static_cast<GLenum>(previousShadeModel));
        glColor4fv(previousColor.data());

        previousDepthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        previousBlend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        previousLighting ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
        previousTexture2D ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
        previousCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        previousLineSmooth ? glEnable(GL_LINE_SMOOTH) : glDisable(GL_LINE_SMOOTH);
    }

private:
    GLint previousMatrixMode {GL_MODELVIEW};
    GLint previousShadeModel {GL_SMOOTH};
    GLint previousDepthFunc {GL_LESS};
    GLint previousBlendSrcRgb {GL_ONE};
    GLint previousBlendDstRgb {GL_ZERO};
    GLint previousBlendSrcAlpha {GL_ONE};
    GLint previousBlendDstAlpha {GL_ZERO};
    GLint previousProgram {0};
    GLboolean previousDepthMask {GL_TRUE};
    std::array<GLfloat, 4> previousColor {1.0F, 1.0F, 1.0F, 1.0F};
    QOpenGLFunctions* openGlFunctions {nullptr};
    bool previousDepthTest {false};
    bool previousBlend {false};
    bool previousLighting {false};
    bool previousTexture2D {false};
    bool previousCullFace {false};
    bool previousLineSmooth {false};
};

class ScopedLineSmoothingState
{
public:
    explicit ScopedLineSmoothingState(bool enabled)
    {
        previousLineSmooth = glIsEnabled(GL_LINE_SMOOTH) == GL_TRUE;
        previousBlend = glIsEnabled(GL_BLEND) == GL_TRUE;
        glGetIntegerv(GL_LINE_SMOOTH_HINT, &previousLineSmoothHint);
        glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSrcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSrcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDstAlpha);
        if (auto* context = QOpenGLContext::currentContext()) {
            openGlFunctions = context->functions();
        }

        enabled ? glEnable(GL_LINE_SMOOTH) : glDisable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    ~ScopedLineSmoothingState()
    {
        glHint(GL_LINE_SMOOTH_HINT, static_cast<GLenum>(previousLineSmoothHint));
        if (openGlFunctions) {
            openGlFunctions->glBlendFuncSeparate(
                static_cast<GLenum>(previousBlendSrcRgb),
                static_cast<GLenum>(previousBlendDstRgb),
                static_cast<GLenum>(previousBlendSrcAlpha),
                static_cast<GLenum>(previousBlendDstAlpha)
            );
        }
        else {
            glBlendFunc(
                static_cast<GLenum>(previousBlendSrcRgb),
                static_cast<GLenum>(previousBlendDstRgb)
            );
        }

        previousBlend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        previousLineSmooth ? glEnable(GL_LINE_SMOOTH) : glDisable(GL_LINE_SMOOTH);
    }

private:
    GLint previousLineSmoothHint {GL_DONT_CARE};
    GLint previousBlendSrcRgb {GL_ONE};
    GLint previousBlendDstRgb {GL_ZERO};
    GLint previousBlendSrcAlpha {GL_ONE};
    GLint previousBlendDstAlpha {GL_ZERO};
    QOpenGLFunctions* openGlFunctions {nullptr};
    bool previousLineSmooth {false};
    bool previousBlend {false};
};

struct ShaderScreenSpaceRenderResult
{
    bool shaderReady {false};
    std::size_t inputSegments {0};
    std::size_t rejectedSegments {0};
    std::size_t duplicateSegments {0};
    std::size_t outputSegments {0};
    std::size_t vertexCount {0};
    double devicePixelRatio {1.0};
    double logicalWidth {1.5};
    double physicalWidth {1.5};
    long long elapsedMicroseconds {0};
};

struct VertexAttributeState
{
    GLint enabled {GL_FALSE};
    GLint size {4};
    GLint type {GL_FLOAT};
    GLint normalized {GL_FALSE};
    GLint stride {0};
    GLint bufferBinding {0};
    void* pointer {nullptr};
};

static VertexAttributeState captureVertexAttributeState(QOpenGLFunctions* functions, GLuint index)
{
    VertexAttributeState state;
    functions->glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &state.enabled);
    functions->glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &state.size);
    functions->glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &state.type);
    functions->glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &state.normalized);
    functions->glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &state.stride);
    functions->glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &state.bufferBinding);
    functions->glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &state.pointer);
    return state;
}

static void restoreVertexAttributeState(
    QOpenGLFunctions* functions,
    GLuint index,
    const VertexAttributeState& state
)
{
    functions->glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(state.bufferBinding));
    functions->glVertexAttribPointer(
        index,
        state.size,
        static_cast<GLenum>(state.type),
        static_cast<GLboolean>(state.normalized),
        state.stride,
        state.pointer
    );
    state.enabled == GL_TRUE ? functions->glEnableVertexAttribArray(index)
                             : functions->glDisableVertexAttribArray(index);
}

static QOpenGLShaderProgram* screenSpaceEdgeShaderProgram()
{
    auto* context = QOpenGLContext::currentContext();
    if (!context) {
        return nullptr;
    }

    constexpr auto objectName = "FreeCAD_BRepEdgeAaDiagnosticShader";
    if (auto* existing = context->findChild<QOpenGLShaderProgram*>(
            QString::fromLatin1(objectName),
            Qt::FindDirectChildrenOnly
        )) {
        return existing;
    }

    auto* program = new QOpenGLShaderProgram(context);
    program->setObjectName(QString::fromLatin1(objectName));
    constexpr auto vertexShader = R"(
#version 120
attribute vec3 vertexPosition;
attribute vec3 edgeCoordinates;
uniform vec2 viewportOrigin;
uniform vec2 viewportSize;
varying vec3 edgeData;

void main()
{
    vec2 viewportPosition = (vertexPosition.xy - viewportOrigin) / viewportSize;
    gl_Position = vec4(viewportPosition * 2.0 - 1.0, vertexPosition.z, 1.0);
    edgeData = edgeCoordinates;
}
)";
    constexpr auto fragmentShader = R"(
#version 120
uniform float halfWidth;
uniform float feather;
uniform vec4 edgeColor;
varying vec3 edgeData;

void main()
{
    float axialDistance = max(max(-edgeData.x, edgeData.x - edgeData.z), 0.0);
    float distanceToSegment = length(vec2(axialDistance, edgeData.y));
    float coverage = 1.0 - smoothstep(halfWidth, halfWidth + feather, distanceToSegment);
    gl_FragColor = vec4(edgeColor.rgb, edgeColor.a * coverage);
}
)";

    const bool compiled = program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader)
        && program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
    program->bindAttributeLocation("vertexPosition", 0);
    program->bindAttributeLocation("edgeCoordinates", 1);
    if (!compiled || !program->link()) {
        SoDebugError::postWarning(
            "SoBrepEdgeSet::screenSpaceEdgeShaderProgram",
            "%s",
            program->log().toUtf8().constData()
        );
        delete program;
        return nullptr;
    }
    return program;
}

static bool renderShaderScreenSpaceBatch(const Detail::ScreenSpaceEdgeBatch& batch, const GLint* viewport)
{
    if (batch.vertices.empty() || viewport[2] <= 0 || viewport[3] <= 0) {
        return false;
    }

    auto* context = QOpenGLContext::currentContext();
    auto* functions = context ? context->functions() : nullptr;
    auto* program = screenSpaceEdgeShaderProgram();
    if (!functions || !program || !program->bind()) {
        return false;
    }

    GLint previousArrayBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
    const auto positionState = captureVertexAttributeState(functions, 0);
    const auto edgeState = captureVertexAttributeState(functions, 1);

    QOpenGLBuffer vertexBuffer(QOpenGLBuffer::VertexBuffer);
    if (!vertexBuffer.create() || !vertexBuffer.bind()) {
        program->release();
        return false;
    }
    vertexBuffer.allocate(
        batch.vertices.data(),
        static_cast<int>(batch.vertices.size() * sizeof(Detail::ScreenSpaceEdgeVertex))
    );

    program->enableAttributeArray(0);
    program->setAttributeBuffer(
        0,
        GL_FLOAT,
        offsetof(Detail::ScreenSpaceEdgeVertex, x),
        3,
        sizeof(Detail::ScreenSpaceEdgeVertex)
    );
    program->enableAttributeArray(1);
    program->setAttributeBuffer(
        1,
        GL_FLOAT,
        offsetof(Detail::ScreenSpaceEdgeVertex, along),
        3,
        sizeof(Detail::ScreenSpaceEdgeVertex)
    );
    program->setUniformValue(
        "viewportOrigin",
        QVector2D(static_cast<float>(viewport[0]), static_cast<float>(viewport[1]))
    );
    program->setUniformValue(
        "viewportSize",
        QVector2D(static_cast<float>(viewport[2]), static_cast<float>(viewport[3]))
    );
    program->setUniformValue("halfWidth", static_cast<float>(batch.halfWidthPhysical));
    program->setUniformValue("feather", static_cast<float>(batch.featherPhysical));
    program->setUniformValue("edgeColor", QVector4D(0.0F, 0.0F, 0.0F, 0.82F));

    functions->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batch.vertices.size()));

    vertexBuffer.release();
    program->release();
    restoreVertexAttributeState(functions, 0, positionState);
    restoreVertexAttributeState(functions, 1, edgeState);
    functions->glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
    return true;
}

static void renderScreenSpaceEdgeAaSegment(
    const ProjectedPoint& first,
    const ProjectedPoint& second,
    EdgeAaDiagnosticMode mode
)
{
    if (!first.valid || !second.valid) {
        return;
    }

    const double dx = second.x - first.x;
    const double dy = second.y - first.y;
    const double length = std::sqrt(dx * dx + dy * dy);
    if (length <= 1.0e-6) {
        return;
    }

    const double nx = -dy / length;
    const double ny = dx / length;
    const bool debug = mode == EdgeAaDiagnosticMode::ScreenSpaceDebug;
    const std::array<double, 4> offsets = debug ? std::array<double, 4> {-2.0, -0.5, 0.5, 2.0}
                                                : std::array<double, 4> {-1.05, -0.25, 0.25, 1.05};
    const std::array<double, 4> alphas = debug ? std::array<double, 4> {0.0, 0.85, 0.85, 0.0}
                                               : std::array<double, 4> {0.0, 0.45, 0.45, 0.0};
    const GLfloat red = debug ? 1.0F : 0.0F;
    const double z1 = -std::clamp(first.z, -1.0, 1.0);
    const double z2 = -std::clamp(second.z, -1.0, 1.0);

    glBegin(GL_TRIANGLE_STRIP);
    for (std::size_t index = 0; index < offsets.size(); ++index) {
        const double offset = offsets[index];
        const auto alpha = static_cast<GLfloat>(alphas[index]);
        glColor4f(red, 0.0F, 0.0F, alpha);
        glVertex3d(first.x + nx * offset, first.y + ny * offset, z1);
        glVertex3d(second.x + nx * offset, second.y + ny * offset, z2);
    }
    glEnd();
}

static void renderScreenSpaceEdgeAa(
    SoGLRenderAction* action,
    const int32_t* indices,
    int numIndices,
    EdgeAaDiagnosticMode mode
)
{
    if (!action || !indices || numIndices <= 0) {
        return;
    }

    const auto* coordinates = SoCoordinateElement::getInstance(action->getState());
    if (!coordinates) {
        return;
    }

    SoCacheElement::invalidate(action->getState());

    GLdouble modelView[16];
    GLdouble projection[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    ScopedScreenSpaceEdgeAaState state(viewport);
    if (mode == EdgeAaDiagnosticMode::DeduplicateScreenSpace) {
        std::vector<Detail::ProjectedSegment> projectedSegments;
        int32_t previous = -1;
        for (int index = 0; index < numIndices; ++index) {
            const int32_t current = indices[index];
            if (current < 0) {
                previous = -1;
                continue;
            }
            if (previous >= 0) {
                Detail::ProjectedSegment segment;
                if (previous < coordinates->getNum() && current < coordinates->getNum()) {
                    const auto first
                        = projectPoint(coordinates->get3(previous), modelView, projection, viewport);
                    const auto second
                        = projectPoint(coordinates->get3(current), modelView, projection, viewport);
                    segment = {
                        first.x,
                        first.y,
                        first.z,
                        second.x,
                        second.y,
                        second.z,
                        first.valid && second.valid,
                    };
                }
                projectedSegments.push_back(segment);
            }
            previous = current;
        }

        const double tolerance = edgeAaDeduplicationTolerance();
        const Detail::ProjectedViewport projectedViewport {
            static_cast<double>(viewport[0]),
            static_cast<double>(viewport[1]),
            static_cast<double>(viewport[0] + viewport[2]),
            static_cast<double>(viewport[1] + viewport[3]),
        };
        const auto deduplicated
            = Detail::deduplicateProjectedSegments(projectedSegments, projectedViewport, tolerance);
        for (const auto& segment : deduplicated.segments) {
            renderScreenSpaceEdgeAaSegment(
                {segment.firstX, segment.firstY, segment.firstZ, true},
                {segment.secondX, segment.secondY, segment.secondZ, true},
                mode
            );
        }
        if (edgeAaDeduplicationStatsEnabled()) {
            std::fprintf(
                stderr,
                "FREECAD_EDGE_AA_DEDUP_STATS input=%zu rejected=%zu duplicate=%zu output=%zu "
                "tolerance_px=%.3f\n",
                deduplicated.stats.inputSegments,
                deduplicated.stats.rejectedSegments,
                deduplicated.stats.duplicateSegments,
                deduplicated.stats.outputSegments,
                tolerance
            );
        }
        return;
    }

    int32_t previous = -1;
    for (int index = 0; index < numIndices; ++index) {
        const int32_t current = indices[index];
        if (current < 0) {
            previous = -1;
            continue;
        }
        if (previous >= 0 && previous < coordinates->getNum() && current < coordinates->getNum()) {
            const auto first
                = projectPoint(coordinates->get3(previous), modelView, projection, viewport);
            const auto second
                = projectPoint(coordinates->get3(current), modelView, projection, viewport);
            renderScreenSpaceEdgeAaSegment(first, second, mode);
        }
        previous = current;
    }
}

static ShaderScreenSpaceRenderResult renderShaderScreenSpaceEdgeAa(
    SoGLRenderAction* action,
    const int32_t* indices,
    int numIndices,
    EdgeAaDiagnosticMode mode
)
{
    ShaderScreenSpaceRenderResult result;
    if (!action || !indices || numIndices <= 0 || !isShaderScreenSpaceMode(mode)) {
        return result;
    }

    const auto* coordinates = SoCoordinateElement::getInstance(action->getState());
    if (!coordinates) {
        return result;
    }

    SoCacheElement::invalidate(action->getState());
    GLdouble modelView[16];
    GLdouble projection[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    std::vector<Detail::ProjectedSegment> projectedSegments;
    int32_t previous = -1;
    for (int index = 0; index < numIndices; ++index) {
        const int32_t current = indices[index];
        if (current < 0) {
            previous = -1;
            continue;
        }
        if (previous >= 0) {
            Detail::ProjectedSegment segment;
            if (previous < coordinates->getNum() && current < coordinates->getNum()) {
                const auto first
                    = projectPoint(coordinates->get3(previous), modelView, projection, viewport);
                const auto second
                    = projectPoint(coordinates->get3(current), modelView, projection, viewport);
                segment = {
                    first.x,
                    first.y,
                    first.z,
                    second.x,
                    second.y,
                    second.z,
                    first.valid && second.valid,
                };
            }
            projectedSegments.push_back(segment);
        }
        previous = current;
    }

    result.inputSegments = projectedSegments.size();
    const Detail::ProjectedViewport projectedViewport {
        static_cast<double>(viewport[0]),
        static_cast<double>(viewport[1]),
        static_cast<double>(viewport[0] + viewport[2]),
        static_cast<double>(viewport[1] + viewport[3]),
    };
    const bool deduplicate = mode != EdgeAaDiagnosticMode::ShaderScreenSpace;
    std::vector<Detail::ProjectedSegment> selectedSegments;
    if (deduplicate) {
        const auto deduplicated = Detail::deduplicateProjectedSegments(
            projectedSegments,
            projectedViewport,
            edgeAaDeduplicationTolerance()
        );
        selectedSegments = deduplicated.segments;
        result.rejectedSegments = deduplicated.stats.rejectedSegments;
        result.duplicateSegments = deduplicated.stats.duplicateSegments;
    }
    else {
        selectedSegments.reserve(projectedSegments.size());
        for (const auto& segment : projectedSegments) {
            if (Detail::isProjectedSegmentRenderable(segment, projectedViewport)) {
                selectedSegments.push_back(segment);
            }
            else {
                ++result.rejectedSegments;
            }
        }
    }
    result.outputSegments = selectedSegments.size();

    result.devicePixelRatio = currentDevicePixelRatio();
    result.logicalWidth = edgeAaShaderLogicalWidth();
    auto batch = Detail::buildScreenSpaceEdgeBatch(
        selectedSegments,
        result.logicalWidth,
        result.devicePixelRatio
    );
    result.rejectedSegments += batch.rejectedSegments;
    result.vertexCount = batch.vertices.size();
    result.physicalWidth = result.logicalWidth * result.devicePixelRatio;

    const auto start = std::chrono::steady_clock::now();
    {
        ScopedScreenSpaceEdgeAaState state(viewport);
        result.shaderReady = renderShaderScreenSpaceBatch(batch, viewport);
    }
    result.elapsedMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(
                                     std::chrono::steady_clock::now() - start
    )
                                     .count();

    if (edgeAaDeduplicationStatsEnabled()) {
        std::fprintf(
            stderr,
            "FREECAD_EDGE_AA_SHADER_STATS input=%zu rejected=%zu duplicate=%zu output=%zu "
            "vertices=%zu draw_calls=%d dpr=%.3f width_logical=%.3f width_physical=%.3f "
            "dedup=%d shader_ready=%d elapsed_us=%lld\n",
            result.inputSegments,
            result.rejectedSegments,
            result.duplicateSegments,
            result.outputSegments,
            result.vertexCount,
            result.shaderReady ? 1 : 0,
            result.devicePixelRatio,
            result.logicalWidth,
            result.physicalWidth,
            deduplicate ? 1 : 0,
            result.shaderReady ? 1 : 0,
            result.elapsedMicroseconds
        );
    }
    return result;
}

void SoBrepEdgeSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepEdgeSet, SoIndexedLineSet, "IndexedLineSet");
}

SoBrepEdgeSet::SoBrepEdgeSet()
    : selContext(std::make_shared<SelContext>())
    , selContext2(std::make_shared<SelContext>())
{
    SO_NODE_CONSTRUCTOR(SoBrepEdgeSet);
    SO_NODE_ADD_FIELD(highlightCoordIndex, (0));
    SO_NODE_ADD_FIELD(selectionCoordIndex, (0));
    SO_NODE_ADD_FIELD(highlightColor, (SbColor(1.0f, 0.0f, 0.0f)));
    SO_NODE_ADD_FIELD(selectionColor, (SbColor(0.0f, 0.6f, 0.0f)));

    highlightCoordIndex.setNum(0);
    selectionCoordIndex.setNum(0);
    overlayLineSet = new SoIndexedLineSet;
    overlayLineSet->ref();
}

SoBrepEdgeSet::~SoBrepEdgeSet()
{
    if (overlayLineSet) {
        overlayLineSet->unref();
        overlayLineSet = nullptr;
    }
}

void SoBrepEdgeSet::GLRender(SoGLRenderAction* action)
{
    auto state = action->getState();
    selCounter.checkRenderCache(state);

    const auto diagnosticMode = edgeAaDiagnosticMode();
    const auto renderBaseEdges = [&] {
        if (isLineSmoothComparisonMode(diagnosticMode)) {
            ScopedLineSmoothingState lineSmoothing(diagnosticMode == EdgeAaDiagnosticMode::LineSmooth);
            inherited::GLRender(action);
            return;
        }
        inherited::GLRender(action);
    };
    if (diagnosticMode == EdgeAaDiagnosticMode::Hide) {
        return;
    }
    if (isScreenSpaceOnlyMode(diagnosticMode)) {
        renderScreenSpaceEdgeAa(
            action,
            this->coordIndex.getValues(0),
            this->coordIndex.getNum(),
            diagnosticMode
        );
        return;
    }
    if (isShaderScreenSpaceOnlyMode(diagnosticMode)) {
        const auto shaderResult = renderShaderScreenSpaceEdgeAa(
            action,
            this->coordIndex.getValues(0),
            this->coordIndex.getNum(),
            diagnosticMode
        );
        if (!shaderResult.shaderReady) {
            renderScreenSpaceEdgeAa(
                action,
                this->coordIndex.getValues(0),
                this->coordIndex.getNum(),
                EdgeAaDiagnosticMode::ScreenSpaceOnly
            );
        }
        return;
    }
    if (diagnosticMode == EdgeAaDiagnosticMode::SuppressOverlays) {
        renderBaseEdges();
        return;
    }

    SelContextPtr ctx2;
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext<SelContext>(this, selContext, ctx2);
    if (ctx2 && ctx2->selectionIndex.empty() && ctx2->colors.empty()) {
        return;
    }


    bool hasContextHighlight = ctx && !ctx->hl.empty();
    bool hasFaceHighlight = viewProvider && viewProvider->isFaceHighlightActive();
    bool hasAnyHighlight = hasContextHighlight || hasFaceHighlight;

    if (Gui::Selection().isClarifySelectionActive()
        && !Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths && hasAnyHighlight) {
        // if we are using clarifyselection - add this to delayed paths with priority
        // as we want to get this rendered on top of everything
        if (viewProvider) {
            viewProvider->setFaceHighlightActive(true);
        }
        Gui::SoDelayedAnnotationsElement::addDelayedPath(
            action->getState(),
            action->getCurPath()->copy(),
            200
        );
        return;
    }

    if (selContext2->checkGlobal(ctx)) {
        if (selContext2->isSelectAll()) {
            selContext2->sl.clear();
            selContext2->sl.push_back(-1);
        }
        else if (ctx) {
            selContext2->sl = ctx->sl;
        }
        if (selContext2->highlightIndex == std::numeric_limits<int>::max()) {
            selContext2->hl.clear();
            selContext2->hl.push_back(-1);
        }
        else if (ctx) {
            selContext2->hl = ctx->hl;
        }
        ctx = selContext2;
    }

    bool hasColorOverride = (ctx2 && !ctx2->colors.empty());

    if (ctx && ctx->highlightIndex == std::numeric_limits<int>::max() && !ctx->isSelectAll()) {
        if (ctx->selectionIndex.empty()) {
            if (ctx2) {
                ctx2->selectionColor = ctx->highlightColor;
                renderSelection(action, ctx2);
            }
            else {
                renderHighlight(action, ctx);
            }
        }
        else {
            if (!action->isRenderingDelayedPaths()) {
                renderSelection(action, ctx);
            }
            if (ctx2) {
                ctx2->selectionColor = ctx->highlightColor;
                renderSelection(action, ctx2);
            }
            else {
                renderHighlight(action, ctx);
            }
            if (action->isRenderingDelayedPaths()) {
                renderSelection(action, ctx);
            }
        }
        return;
    }

    if (!action->isRenderingDelayedPaths()) {
        renderHighlight(action, ctx);
    }
    if (ctx && !ctx->selectionIndex.empty()) {
        if (ctx->isSelectAll()) {
            if (ctx2) {
                ctx2->selectionColor = ctx->selectionColor;
                renderSelection(action, ctx2);
            }
            else if (ctx->isSelectAll()) {
                renderSelection(action, ctx);
            }
            if (action->isRenderingDelayedPaths()) {
                renderHighlight(action, ctx);
            }
            return;
        }
        if (!action->isRenderingDelayedPaths()) {
            renderSelection(action, ctx);
        }
    }
    if (hasColorOverride) {
        renderColorOverrides(
            action,
            overlayLineSet,
            this->coordIndex.getValues(0),
            this->coordIndex.getNum(),
            ctx2->colors
        );
    }
    else if (ctx2 && !ctx2->selectionIndex.empty()) {
        renderSelection(action, ctx2, false);
    }
    else if (
        Gui::Selection().isClarifySelectionActive()
        && !Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths && hasAnyHighlight
    ) {
        state->push();
        SoDepthBufferElement::set(state, FALSE, FALSE, SoDepthBufferElement::ALWAYS, SbVec2f(0.0f, 1.0f));

        renderBaseEdges();

        state->pop();
    }
    else {
        renderBaseEdges();
    }

    if (diagnosticMode == EdgeAaDiagnosticMode::ShaderScreenSpaceOverlay) {
        renderShaderScreenSpaceEdgeAa(
            action,
            this->coordIndex.getValues(0),
            this->coordIndex.getNum(),
            diagnosticMode
        );
    }

    // Workaround for #0000433
    // #if !defined(FC_OS_WIN32)
    if (!action->isRenderingDelayedPaths()) {
        renderHighlight(action, ctx);
    }
    if (ctx && !ctx->selectionIndex.empty()) {
        renderSelection(action, ctx);
    }
    if (action->isRenderingDelayedPaths()) {
        renderHighlight(action, ctx);
    }
    // #endif

    // Optional overlay rendering for deterministic tests (and programmatic usage).
    const int hlNum = highlightCoordIndex.getNum();
    if (hlNum > 0) {
        renderOverlayLines(
            action,
            overlayLineSet,
            highlightCoordIndex.getValues(0),
            hlNum,
            highlightColor.getValue(),
            OverlayDepthMode::DrawOnTop
        );
    }
    const int selNum = selectionCoordIndex.getNum();
    if (selNum > 0) {
        renderOverlayLines(
            action,
            overlayLineSet,
            selectionCoordIndex.getValues(0),
            selNum,
            selectionColor.getValue(),
            OverlayDepthMode::DrawOnTop
        );
    }
    if (diagnosticMode == EdgeAaDiagnosticMode::ScreenSpaceOverlay) {
        renderScreenSpaceEdgeAa(
            action,
            this->coordIndex.getValues(0),
            this->coordIndex.getNum(),
            diagnosticMode
        );
    }
}

void SoBrepEdgeSet::GLRenderBelowPath(SoGLRenderAction* action)
{
    inherited::GLRenderBelowPath(action);
}

void SoBrepEdgeSet::getBoundingBox(SoGetBoundingBoxAction* action)
{

    SelContextPtr ctx2 = Gui::SoFCSelectionRoot::getSecondaryActionContext<SelContext>(action, this);
    if (!ctx2 || (ctx2->sl.size() == 1 && ctx2->sl[0] < 0)) {
        inherited::getBoundingBox(action);
        return;
    }

    if (ctx2->sl.empty()) {
        return;
    }

    auto state = action->getState();
    auto coords = SoCoordinateElement::getInstance(state);
    const SbVec3f* coords3d = coords->getArrayPtr3();

    if (!validIndexes(coords, ctx2->sl)) {
        return;
    }

    SbBox3f bbox;

    int32_t i;
    const int32_t* cindices = &ctx2->sl[0];
    const int32_t* end = cindices + ctx2->sl.size();
    while (cindices < end) {
        bbox.extendBy(coords3d[*cindices++]);
        i = (cindices < end) ? *cindices++ : -1;
        while (i >= 0) {
            bbox.extendBy(coords3d[i]);
            i = cindices < end ? *cindices++ : -1;
        }
    }
    if (!bbox.isEmpty()) {
        action->extendBy(bbox);
    }
}

void SoBrepEdgeSet::renderHighlight(SoGLRenderAction* action, SelContextPtr ctx)
{
    if (!ctx || ctx->highlightIndex < 0) {
        return;
    }

    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(action->getState());
    if (!coords) {
        return;
    }

    int num = (int)ctx->hl.size();
    if (num > 0) {
        if (ctx->hl[0] < 0) {
            renderOverlayLines(
                action,
                overlayLineSet,
                this->coordIndex.getValues(0),
                this->coordIndex.getNum(),
                ctx->highlightColor,
                OverlayDepthMode::DrawOnTop
            );
        }
        else {
            if (!validIndexes(coords, ctx->hl)) {
                SoDebugError::postWarning(
                    "SoBrepEdgeSet::renderHighlight",
                    "highlightIndex out of range"
                );
            }
            else {
                renderOverlayLines(
                    action,
                    overlayLineSet,
                    ctx->hl.data(),
                    num,
                    ctx->highlightColor,
                    OverlayDepthMode::DrawOnTop
                );
            }
        }
    }
}

void SoBrepEdgeSet::renderSelection(SoGLRenderAction* action, SelContextPtr ctx, bool /*push*/)
{
    if (!ctx) {
        return;
    }

    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(action->getState());
    if (!coords) {
        return;
    }

    int num = (int)ctx->sl.size();
    if (num > 0) {
        if (ctx->sl[0] < 0) {
            renderOverlayLines(
                action,
                overlayLineSet,
                this->coordIndex.getValues(0),
                this->coordIndex.getNum(),
                ctx->selectionColor,
                OverlayDepthMode::RespectDepth
            );
        }
        else {
            if (!validIndexes(coords, ctx->sl)) {
                SoDebugError::postWarning(
                    "SoBrepEdgeSet::renderSelection",
                    "selectionIndex out of range"
                );
            }
            else {
                renderOverlayLines(
                    action,
                    overlayLineSet,
                    ctx->sl.data(),
                    num,
                    ctx->selectionColor,
                    OverlayDepthMode::RespectDepth
                );
            }
        }
    }
}

bool SoBrepEdgeSet::validIndexes(const SoCoordinateElement* coords, const std::vector<int32_t>& pts) const
{
    for (int32_t it : pts) {
        if (it >= coords->getNum()) {
            return false;
        }
    }
    return true;
}

void SoBrepEdgeSet::doAction(SoAction* action)
{
    if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
        Gui::SoHighlightElementAction* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
        selCounter.checkAction(hlaction);
        if (!hlaction->isHighlighted()) {
            SelContextPtr ctx
                = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
            if (ctx) {
                ctx->highlightIndex = -1;
                ctx->hl.clear();
                touch();
            }
            return;
        }
        const SoDetail* detail = hlaction->getElement();
        if (!detail) {
            SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
            ctx->highlightColor = hlaction->getColor();
            ctx->highlightIndex = std::numeric_limits<int>::max();
            ctx->hl.clear();
            ctx->hl.push_back(-1);
            touch();
            return;
        }

        if (!detail->isOfType(SoLineDetail::getClassTypeId())) {
            SelContextPtr ctx
                = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
            if (ctx) {
                ctx->highlightIndex = -1;
                ctx->hl.clear();
                touch();
            }
            return;
        }

        SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
        ctx->highlightColor = hlaction->getColor();
        int index = static_cast<const SoLineDetail*>(detail)->getLineIndex();
        const int32_t* cindices = this->coordIndex.getValues(0);
        int numcindices = this->coordIndex.getNum();

        ctx->hl.clear();
        for (int section = 0, i = 0; i < numcindices; i++) {
            if (cindices[i] < 0) {
                if (++section > index) {
                    break;
                }
            }
            else if (section == index) {
                ctx->hl.push_back(cindices[i]);
            }
        }
        if (!ctx->hl.empty()) {
            ctx->highlightIndex = index;
        }
        else {
            ctx->highlightIndex = -1;
        }
        touch();
        return;
    }
    else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
        Gui::SoSelectionElementAction* selaction = static_cast<Gui::SoSelectionElementAction*>(action);

        switch (selaction->getType()) {
            case Gui::SoSelectionElementAction::Color:
                if (selaction->isSecondary()) {
                    const auto& colors = selaction->getColors();

                    // Case 1: The color map is empty. This is a "clear" command.
                    if (colors.empty()) {
                        // We must find and remove any existing secondary context for this node.
                        if (Gui::SoFCSelectionRoot::removeActionContext(action, this)) {
                            touch();
                        }
                        return;
                    }

                    // Case 2: The color map is NOT empty. This is a "set color" command.
                    static std::string element("Edge");
                    bool hasEdgeColors = false;
                    for (const auto& [name, color] : colors) {
                        if (name.empty() || boost::starts_with(name, element)) {
                            hasEdgeColors = true;
                            break;
                        }
                    }

                    if (hasEdgeColors) {
                        auto ctx = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action, this);
                        selCounter.checkAction(selaction, ctx);
                        ctx->selectAll();

                        if (ctx->setColors(colors, element)) {
                            touch();
                        }
                    }
                }
                return;
            case Gui::SoSelectionElementAction::None: {
                if (selaction->isSecondary()) {
                    if (Gui::SoFCSelectionRoot::removeActionContext(action, this)) {
                        touch();
                    }
                }
                else {
                    SelContextPtr ctx
                        = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
                    if (ctx) {
                        ctx->selectionIndex.clear();
                        ctx->sl.clear();
                        ctx->colors.clear();
                        touch();
                    }
                }
                return;
            }
            case Gui::SoSelectionElementAction::All: {
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
                selCounter.checkAction(selaction, ctx);
                ctx->selectionColor = selaction->getColor();
                ctx->selectionIndex.clear();
                ctx->selectionIndex.insert(-1);  // all
                ctx->sl.clear();
                ctx->sl.push_back(-1);
                touch();
                return;
            }
            case Gui::SoSelectionElementAction::Append:
            case Gui::SoSelectionElementAction::Remove: {
                const SoDetail* detail = selaction->getElement();
                if (!detail || !detail->isOfType(SoLineDetail::getClassTypeId())) {
                    if (selaction->isSecondary()) {
                        // For secondary context, a detail of different type means
                        // the user may want to partial render only other type of
                        // geometry. So we call below to obtain a action context.
                        // If no secondary context exist, it will create an empty
                        // one, and an empty secondary context inhibites drawing
                        // here.
                        auto ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
                        selCounter.checkAction(selaction, ctx);
                        touch();
                    }
                    return;
                }
                int index = static_cast<const SoLineDetail*>(detail)->getLineIndex();
                SelContextPtr ctx;
                if (selaction->getType() == Gui::SoSelectionElementAction::Append) {
                    ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
                    selCounter.checkAction(selaction, ctx);
                    ctx->selectionColor = selaction->getColor();
                    if (ctx->isSelectAll()) {
                        ctx->selectionIndex.clear();
                    }
                    if (!ctx->selectionIndex.insert(index).second) {
                        return;
                    }
                }
                else {
                    ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
                    if (!ctx || !ctx->removeIndex(index)) {
                        return;
                    }
                }
                ctx->sl.clear();
                if (!ctx->selectionIndex.empty()) {
                    const int32_t* cindices = this->coordIndex.getValues(0);
                    int numcindices = this->coordIndex.getNum();
                    auto it = ctx->selectionIndex.begin();
                    for (int section = 0, i = 0; i < numcindices; i++) {
                        if (section == *it) {
                            ctx->sl.push_back(cindices[i]);
                        }
                        if (cindices[i] < 0) {
                            if (++section > *it) {
                                if (++it == ctx->selectionIndex.end()) {
                                    break;
                                }
                            }
                        }
                    }
                }
                touch();
                break;
            }
            default:
                break;
        }
        return;
    }

    inherited::doAction(action);
}

SoDetail* SoBrepEdgeSet::createLineSegmentDetail(
    SoRayPickAction* action,
    const SoPrimitiveVertex* v1,
    const SoPrimitiveVertex* v2,
    SoPickedPoint* pp
)
{
    SoDetail* detail = inherited::createLineSegmentDetail(action, v1, v2, pp);
    SoLineDetail* line_detail = static_cast<SoLineDetail*>(detail);
    int index = line_detail->getLineIndex();
    line_detail->setPartIndex(index);
    return detail;
}
