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
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/elements/SoCoordinateElement.h>
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
#include <Gui/GpuDiagnostics.h>
#include <Base/Color.h>
#include "SoBrepEdgeSet.h"
#include "ViewProviderExt.h"

#include <Gui/Inventor/So3DAnnotation.h>


using namespace PartGui;

SO_NODE_SOURCE(SoBrepEdgeSet)

struct SoBrepEdgeSet::SelContext: Gui::SoFCSelectionContextEx
{
    std::vector<int32_t> hl, sl;
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

static void renderOverlayLines(
    SoGLRenderAction* action,
    SoIndexedLineSet* lineSet,
    const int32_t* indices,
    int numIndices,
    const Base::Color& color
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
    SoDepthBufferElement::set(state, FALSE, FALSE, SoDepthBufferElement::ALWAYS, SbVec2f(0.0f, 1.0f));

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
    const SbColor& color
)
{
    renderOverlayLines(
        action,
        lineSet,
        indices,
        numIndices,
        Base::Color(color[0], color[1], color[2], 1.0f)
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
            group.color
        );
    }
}

static QString glString(GLenum name)
{
    const auto* value = glGetString(name);
    return value ? QString::fromLatin1(reinterpret_cast<const char*>(value)) : QString();
}

class ScopedLineSmoothing
{
public:
    explicit ScopedLineSmoothing(bool enable)
        : active(enable)
    {
        if (!active) {
            return;
        }

        wasLineSmooth = glIsEnabled(GL_LINE_SMOOTH) == GL_TRUE;
        wasBlend = glIsEnabled(GL_BLEND) == GL_TRUE;
        glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
        glGetIntegerv(GL_LINE_SMOOTH_HINT, &lineSmoothHint);

        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    }

    ~ScopedLineSmoothing()
    {
        if (!active) {
            return;
        }

        glBlendFunc(static_cast<GLenum>(blendSrcRgb), static_cast<GLenum>(blendDstRgb));
        glHint(GL_LINE_SMOOTH_HINT, static_cast<GLenum>(lineSmoothHint));

        if (wasBlend) {
            glEnable(GL_BLEND);
        }
        else {
            glDisable(GL_BLEND);
        }

        if (wasLineSmooth) {
            glEnable(GL_LINE_SMOOTH);
        }
        else {
            glDisable(GL_LINE_SMOOTH);
        }
    }

private:
    bool active {false};
    bool wasLineSmooth {false};
    bool wasBlend {false};
    GLint blendSrcRgb {0};
    GLint blendDstRgb {0};
    GLint lineSmoothHint {0};
};

enum class EdgeAaDiagnosticMode
{
    Disabled,
    Overlay,
    Only,
    DebugOnly,
    Hide,
};

EdgeAaDiagnosticMode edgeAaDiagnosticMode()
{
    const char* value = std::getenv("FREECAD_EDGE_AA_DIAGNOSTIC");
    if (!value) {
        return EdgeAaDiagnosticMode::Disabled;
    }

    const std::string_view mode(value);
    if (mode == "hide") {
        return EdgeAaDiagnosticMode::Hide;
    }
    if (mode == "screen-space-only") {
        return EdgeAaDiagnosticMode::Only;
    }
    if (mode == "screen-space-debug") {
        return EdgeAaDiagnosticMode::DebugOnly;
    }
    if (mode == "screen-space" || mode == "screen-space-overlay") {
        return EdgeAaDiagnosticMode::Overlay;
    }

    return EdgeAaDiagnosticMode::Disabled;
}

bool isScreenSpaceOnlyMode(EdgeAaDiagnosticMode mode)
{
    return mode == EdgeAaDiagnosticMode::Only || mode == EdgeAaDiagnosticMode::DebugOnly;
}

struct ProjectedPoint
{
    double x {0.0};
    double y {0.0};
    double z {0.0};
    bool valid {false};
};

ProjectedPoint projectPoint(
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
    return {
        viewport[0] + (ndcX + 1.0) * viewport[2] * 0.5,
        viewport[1] + (ndcY + 1.0) * viewport[3] * 0.5,
        ndcZ,
        ndcX >= -1.25 && ndcX <= 1.25 && ndcY >= -1.25 && ndcY <= 1.25
    };
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
        previousShadeModel = 0;
        previousDepthMask = GL_TRUE;
        glGetIntegerv(GL_SHADE_MODEL, &previousShadeModel);
        glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
        glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSrc);
        glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDst);

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
        glBlendFunc(static_cast<GLenum>(previousBlendSrc), static_cast<GLenum>(previousBlendDst));
        glShadeModel(static_cast<GLenum>(previousShadeModel));

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
    GLint previousBlendSrc {0};
    GLint previousBlendDst {0};
    GLboolean previousDepthMask {GL_TRUE};
    bool previousDepthTest {false};
    bool previousBlend {false};
    bool previousLighting {false};
    bool previousTexture2D {false};
    bool previousCullFace {false};
    bool previousLineSmooth {false};
};

void renderScreenSpaceEdgeAaSegment(
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
    const bool debug = mode == EdgeAaDiagnosticMode::DebugOnly;
    const std::array<double, 4> offsets = debug ? std::array<double, 4> {-2.0, -0.5, 0.5, 2.0}
                                                : std::array<double, 4> {-1.05, -0.25, 0.25, 1.05};
    const std::array<double, 4> alphas = debug ? std::array<double, 4> {0.0, 0.85, 0.85, 0.0}
                                               : std::array<double, 4> {0.0, 0.45, 0.45, 0.0};
    const GLfloat red = debug ? 1.0F : 0.0F;
    const GLfloat green = 0.0F;
    const GLfloat blue = debug ? 0.0F : 0.0F;
    const double z1 = -std::clamp(first.z, -1.0, 1.0);
    const double z2 = -std::clamp(second.z, -1.0, 1.0);

    glBegin(GL_TRIANGLE_STRIP);
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        const double offset = offsets[i];
        const auto alpha = static_cast<GLfloat>(alphas[i]);
        glColor4f(red, green, blue, alpha);
        glVertex3d(first.x + nx * offset, first.y + ny * offset, z1);
        glVertex3d(second.x + nx * offset, second.y + ny * offset, z2);
    }
    glEnd();
}

void renderScreenSpaceEdgeAaOverlay(
    SoGLRenderAction* action,
    const SoCoordinateElement* coords,
    const int32_t* indices,
    int numIndices,
    EdgeAaDiagnosticMode mode
)
{
    if (!action || !coords || !indices || numIndices <= 0) {
        return;
    }

    GLdouble modelView[16];
    GLdouble projection[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    ScopedScreenSpaceEdgeAaState state(viewport);

    int32_t previous = -1;
    for (int i = 0; i < numIndices; ++i) {
        const int32_t current = indices[i];
        if (current < 0) {
            previous = -1;
            continue;
        }
        if (previous >= 0 && previous < coords->getNum() && current < coords->getNum()) {
            const auto first = projectPoint(coords->get3(previous), modelView, projection, viewport);
            const auto second = projectPoint(coords->get3(current), modelView, projection, viewport);
            renderScreenSpaceEdgeAaSegment(first, second, mode);
        }
        previous = current;
    }
}

static void recordEdgeRenderProbe(SoGLRenderAction* action, const char* stage, int coordIndexCount)
{
    if (!action) {
        return;
    }

    Gui::GpuDiagnosticsRenderProbe probe;
    probe.node = QStringLiteral("SoBrepEdgeSet");
    probe.stage = QString::fromLatin1(stage);
    probe.coordIndexCount = coordIndexCount;
    probe.actionSmoothing = action->isSmoothing();
    probe.renderingDelayedPaths = action->isRenderingDelayedPaths();

    GLint sampleBuffers = 0;
    GLint samples = 0;
    GLint drawFramebuffer = 0;
    GLint readFramebuffer = 0;
    GLint lineSmoothHint = 0;
    GLint blendSrcRgb = 0;
    GLint blendDstRgb = 0;
    GLint blendSrcAlpha = 0;
    GLint blendDstAlpha = 0;
    GLfloat lineWidth = 0.0F;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);
    glGetIntegerv(GL_SAMPLES, &samples);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
    glGetIntegerv(GL_LINE_SMOOTH_HINT, &lineSmoothHint);
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);
    glGetFloatv(GL_LINE_WIDTH, &lineWidth);

    probe.glSampleBuffers = sampleBuffers;
    probe.glSamples = samples;
    probe.glMultisampleEnabled = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;
    probe.glLineSmoothEnabled = glIsEnabled(GL_LINE_SMOOTH) == GL_TRUE;
    probe.glBlendEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
    probe.lineSmoothingBlendReady = probe.glLineSmoothEnabled.value_or(false)
        && probe.glBlendEnabled.value_or(false);
    probe.glBlendSrcRgb = blendSrcRgb;
    probe.glBlendDstRgb = blendDstRgb;
    probe.glBlendSrcAlpha = blendSrcAlpha;
    probe.glBlendDstAlpha = blendDstAlpha;
    probe.glLineWidth = static_cast<double>(lineWidth);
    probe.glLineSmoothHint = lineSmoothHint;
    probe.drawFramebuffer = drawFramebuffer;
    probe.readFramebuffer = readFramebuffer;
    probe.vendor = glString(GL_VENDOR);
    probe.renderer = glString(GL_RENDERER);
    probe.version = glString(GL_VERSION);

    Gui::GpuDiagnostics::recordRenderProbe(probe);
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

    if (ctx && ctx->highlightIndex == std::numeric_limits<int>::max()) {
        if (ctx->selectionIndex.empty() || ctx->isSelectAll()) {
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

        const auto edgeAaMode = edgeAaDiagnosticMode();
        if (edgeAaMode == EdgeAaDiagnosticMode::Hide) {
            recordEdgeRenderProbe(action, "clarify-hidden", this->coordIndex.getNum());
        }
        else if (isScreenSpaceOnlyMode(edgeAaMode)) {
            recordEdgeRenderProbe(action, "clarify-before-screen-space-only", this->coordIndex.getNum());
        }
        else {
            ScopedLineSmoothing lineSmoothing(action->isSmoothing());
            recordEdgeRenderProbe(action, "clarify-before-inherited", this->coordIndex.getNum());
            inherited::GLRender(action);
            recordEdgeRenderProbe(action, "clarify-after-inherited", this->coordIndex.getNum());
        }
        if (edgeAaMode != EdgeAaDiagnosticMode::Disabled && edgeAaMode != EdgeAaDiagnosticMode::Hide) {
            renderScreenSpaceEdgeAaOverlay(
                action,
                SoCoordinateElement::getInstance(action->getState()),
                this->coordIndex.getValues(0),
                this->coordIndex.getNum(),
                edgeAaMode
            );
            if (isScreenSpaceOnlyMode(edgeAaMode)) {
                recordEdgeRenderProbe(
                    action,
                    "clarify-after-screen-space-only",
                    this->coordIndex.getNum()
                );
            }
        }

        state->pop();
    }
    else {
        const auto edgeAaMode = edgeAaDiagnosticMode();
        if (edgeAaMode == EdgeAaDiagnosticMode::Hide) {
            recordEdgeRenderProbe(action, "hidden", this->coordIndex.getNum());
        }
        else if (isScreenSpaceOnlyMode(edgeAaMode)) {
            recordEdgeRenderProbe(action, "before-screen-space-only", this->coordIndex.getNum());
        }
        else {
            ScopedLineSmoothing lineSmoothing(action->isSmoothing());
            recordEdgeRenderProbe(action, "before-inherited", this->coordIndex.getNum());
            inherited::GLRender(action);
            recordEdgeRenderProbe(action, "after-inherited", this->coordIndex.getNum());
        }
        if (edgeAaMode != EdgeAaDiagnosticMode::Disabled && edgeAaMode != EdgeAaDiagnosticMode::Hide) {
            renderScreenSpaceEdgeAaOverlay(
                action,
                SoCoordinateElement::getInstance(action->getState()),
                this->coordIndex.getValues(0),
                this->coordIndex.getNum(),
                edgeAaMode
            );
            if (isScreenSpaceOnlyMode(edgeAaMode)) {
                recordEdgeRenderProbe(action, "after-screen-space-only", this->coordIndex.getNum());
            }
        }
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
            highlightColor.getValue()
        );
    }
    const int selNum = selectionCoordIndex.getNum();
    if (selNum > 0) {
        renderOverlayLines(
            action,
            overlayLineSet,
            selectionCoordIndex.getValues(0),
            selNum,
            selectionColor.getValue()
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
                ctx->highlightColor
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
                renderOverlayLines(action, overlayLineSet, ctx->hl.data(), num, ctx->highlightColor);
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
                ctx->selectionColor
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
                renderOverlayLines(action, overlayLineSet, ctx->sl.data(), num, ctx->selectionColor);
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
