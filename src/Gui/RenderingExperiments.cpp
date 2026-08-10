/***************************************************************************
 *   Copyright (c) 2026                                                    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <Inventor/SbColor.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHideEvent>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QShowEvent>
#include <QSlider>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Material.h>
#include <App/Property.h>
#include <App/PropertyStandard.h>

#include <Base/Tools.h>

#include "Application.h"
#include "DockWindowManager.h"
#include "Document.h"
#include "RenderingExperiments.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"

using namespace Gui::Dialog;

namespace
{

constexpr float MinimumDeviation = 0.001F;
constexpr float MaximumDeviation = 5.0F;
constexpr float DefaultCustomDeviation = 0.2F;
constexpr float DefaultCustomAngularDegrees = 15.0F;

constexpr int IntensityMinimumPercent = 10;
constexpr int IntensityMaximumPercent = 200;
constexpr int IntensityDefaultPercent = 100;
constexpr int ShininessMinimumPercent = 0;
constexpr int ShininessMaximumPercent = 100;
constexpr int SpecularMinimumPercent = 0;
constexpr int SpecularMaximumPercent = 100;
constexpr int DefaultSpecularPercent = 70;
constexpr int DefaultShininessPercent = 90;
constexpr int ApplyDebounceMilliseconds = 75;

enum UpdateFlag : unsigned int
{
    UpdateLighting = 1U << 0,
    UpdateMaterials = 1U << 1,
    UpdateTessellation = 1U << 2
};

float clamp01(float value)
{
    return std::clamp(value, 0.0F, 1.0F);
}

float fromPercent(int value)
{
    return clamp01(Base::fromPercent(value));
}

SbColor scaledColor(const SbColor& color, float factor)
{
    const float r = clamp01(color[0] * factor);
    const float g = clamp01(color[1] * factor);
    const float b = clamp01(color[2] * factor);
    return {r, g, b};
}

App::PropertyFloat* asFloatProperty(App::Property* property)
{
    return dynamic_cast<App::PropertyFloat*>(property);
}

App::PropertyMaterialList* asMaterialProperty(Gui::ViewProvider* viewProvider)
{
    return viewProvider
        ? dynamic_cast<App::PropertyMaterialList*>(viewProvider->getPropertyByName("ShapeAppearance"))
        : nullptr;
}

QString describeObjectCount(int count, const QString& singular, const QString& plural)
{
    return count == 1 ? QStringLiteral("1 %1").arg(singular)
                      : QStringLiteral("%1 %2").arg(count).arg(plural);
}

enum class LightingPreset : std::uint8_t
{
    DiagnosticStudio = 0,
    Technical = 1,
    HighContrast = 2
};

enum class TessellationProfile : std::uint8_t
{
    Native = 0,
    Coarse = 1,
    Fine = 2,
    Custom = 3
};

struct LightState
{
    bool on {false};
    float intensity {0.0F};
    SbColor color;
    SbVec3f direction;
};

struct EnvironmentState
{
    SbColor ambientColor;
    float ambientIntensity {0.0F};
};

struct ViewerState
{
    LightState headlight;
    LightState backlight;
    LightState fillLight;
    EnvironmentState environment;
};

struct MaterialSnapshot
{
    std::string objectName;
    std::vector<App::Material> materials;
};

struct TessellationSnapshot
{
    std::string objectName;
    bool hasDeviation {false};
    double deviation {0.0};
    bool hasAngularDeflection {false};
    double angularDeflection {0.0};
};

struct TessellationTargets
{
    App::PropertyFloat* deviation {nullptr};
    App::PropertyFloat* angularDeflection {nullptr};
};

LightState captureLightState(const SoDirectionalLight* light)
{
    LightState state;
    if (!light) {
        return state;
    }

    state.on = light->on.getValue();
    state.intensity = light->intensity.getValue();
    state.color = light->color.getValue();
    state.direction = light->direction.getValue();
    return state;
}

void applyLightState(SoDirectionalLight* light, const LightState& state)
{
    if (!light) {
        return;
    }

    light->on.setValue(state.on);
    light->intensity.setValue(state.intensity);
    light->color.setValue(state.color);
    light->direction.setValue(state.direction);
}

ViewerState captureViewerState(Gui::View3DInventorViewer* viewer)
{
    ViewerState state;
    if (!viewer) {
        return state;
    }

    state.headlight = captureLightState(viewer->getHeadlight());
    state.backlight = captureLightState(viewer->getBacklight());
    state.fillLight = captureLightState(viewer->getFillLight());

    if (auto* environment = viewer->getEnvironment()) {
        state.environment.ambientColor = environment->ambientColor.getValue();
        state.environment.ambientIntensity = environment->ambientIntensity.getValue();
    }

    return state;
}

void applyViewerState(Gui::View3DInventorViewer* viewer, const ViewerState& state)
{
    if (!viewer) {
        return;
    }

    applyLightState(viewer->getHeadlight(), state.headlight);
    applyLightState(viewer->getBacklight(), state.backlight);
    applyLightState(viewer->getFillLight(), state.fillLight);

    if (auto* environment = viewer->getEnvironment()) {
        environment->ambientColor.setValue(state.environment.ambientColor);
        environment->ambientIntensity.setValue(state.environment.ambientIntensity);
    }

    viewer->redraw();
}

void requestProviderUpdate(Gui::ViewProvider* viewProvider)
{
    if (auto* documentObjectProvider = dynamic_cast<Gui::ViewProviderDocumentObject*>(viewProvider)) {
        documentObjectProvider->updateView();
    }
}

template<typename Callback>
void changeWithoutDocumentModification(App::Property* property, Callback&& callback)
{
    Base::ObjectStatusLocker<App::Property::Status, App::Property> noModify(
        App::Property::NoModify,
        property
    );
    std::invoke(std::forward<Callback>(callback));
}

}  // namespace

class RenderingExperiments::Private
{
public:
    explicit Private(RenderingExperiments* owner, Gui::View3DInventor* viewIn, App::Document* documentIn)
        : self(owner)
        , view(viewIn)
        , document(documentIn)
    {}

    struct LightingPresetState
    {
        LightState headlight;
        LightState backlight;
        LightState fillLight;
        EnvironmentState environment;
    };

    RenderingExperiments* self {nullptr};
    QPointer<Gui::View3DInventor> view;
    App::Document* document {nullptr};
    QPointer<QDockWidget> dockWidget;

    QCheckBox* masterEnabled {nullptr};
    QPushButton* showNativeButton {nullptr};
    QPushButton* resetButton {nullptr};
    QLabel* statusLabel {nullptr};

    QComboBox* lightingPreset {nullptr};
    QSlider* lightingIntensity {nullptr};
    QLabel* lightingIntensityValue {nullptr};

    QCheckBox* materialEnabled {nullptr};
    QSlider* specularStrength {nullptr};
    QLabel* specularStrengthValue {nullptr};
    QSlider* shininess {nullptr};
    QLabel* shininessValue {nullptr};

    QCheckBox* tessellationEnabled {nullptr};
    QComboBox* tessellationProfile {nullptr};
    QDoubleSpinBox* deviation {nullptr};
    QDoubleSpinBox* angularDeflection {nullptr};
    QTimer* applyTimer {nullptr};

    std::optional<ViewerState> capturedViewerState;
    std::map<std::string, MaterialSnapshot> capturedMaterials;
    std::map<std::string, TessellationSnapshot> capturedTessellation;

    bool experimentsApplied {false};
    bool nativeHoldActive {false};
    bool documentActive {true};
    bool isRestoring {false};
    bool discardOnHide {false};
    bool saveSuspended {false};
    bool nativeHoldDuringSave {false};
    bool materialsCaptured {false};
    bool tessellationCaptured {false};
    unsigned int pendingUpdates {0U};

    fastsignals::scoped_connection activeDocConnection;
    fastsignals::scoped_connection deleteDocConnection;
    fastsignals::scoped_connection startSaveConnection;
    fastsignals::scoped_connection finishSaveConnection;

    void buildUi()
    {
        auto* outerLayout = new QVBoxLayout(self);
        outerLayout->setContentsMargins(10, 10, 10, 10);
        outerLayout->setSpacing(10);

        auto* warning = new QLabel(
            QObject::tr(
                "Experimental viewport controls for this view only. They do not change saved "
                "preferences and are restored when the experiment stops."
            ),
            self
        );
        warning->setWordWrap(true);
        outerLayout->addWidget(warning);

        masterEnabled = new QCheckBox(QObject::tr("Enable experiments"), self);
        masterEnabled->setObjectName(QStringLiteral("RenderingExperimentsEnabled"));
        outerLayout->addWidget(masterEnabled);

        auto* buttonsLayout = new QGridLayout();
        buttonsLayout->setContentsMargins(0, 0, 0, 0);
        buttonsLayout->setHorizontalSpacing(8);

        showNativeButton = new QPushButton(QObject::tr("Show native"), self);
        showNativeButton->setObjectName(QStringLiteral("RenderingExperimentsShowNative"));
        showNativeButton->setAutoDefault(false);
        showNativeButton->setToolTip(
            QObject::tr("Press and hold to temporarily restore the native view.")
        );
        buttonsLayout->addWidget(showNativeButton, 0, 0);

        resetButton = new QPushButton(QObject::tr("Reset to native"), self);
        resetButton->setObjectName(QStringLiteral("RenderingExperimentsReset"));
        resetButton->setAutoDefault(false);
        buttonsLayout->addWidget(resetButton, 0, 1);
        buttonsLayout->setColumnStretch(2, 1);
        outerLayout->addLayout(buttonsLayout);

        auto* lightingBox = new QGroupBox(QObject::tr("Lighting"), self);
        auto* lightingLayout = new QGridLayout(lightingBox);
        lightingPreset = new QComboBox(lightingBox);
        lightingPreset->addItem(QObject::tr("Diagnostic studio"));
        lightingPreset->addItem(QObject::tr("Technical"));
        lightingPreset->addItem(QObject::tr("High contrast"));
        lightingLayout->addWidget(new QLabel(QObject::tr("Preset"), lightingBox), 0, 0);
        lightingLayout->addWidget(lightingPreset, 0, 1, 1, 2);

        lightingIntensity = new QSlider(Qt::Horizontal, lightingBox);
        lightingIntensity->setObjectName(QStringLiteral("RenderingExperimentsLightingIntensity"));
        lightingIntensity->setRange(IntensityMinimumPercent, IntensityMaximumPercent);
        lightingIntensity->setValue(IntensityDefaultPercent);
        lightingIntensityValue = new QLabel(lightingBox);
        lightingLayout->addWidget(new QLabel(QObject::tr("Intensity"), lightingBox), 1, 0);
        lightingLayout->addWidget(lightingIntensity, 1, 1);
        lightingLayout->addWidget(lightingIntensityValue, 1, 2);
        outerLayout->addWidget(lightingBox);

        auto* materialBox = new QGroupBox(QObject::tr("Material response"), self);
        auto* materialLayout = new QGridLayout(materialBox);
        materialEnabled = new QCheckBox(QObject::tr("Adjust ShapeAppearance"), materialBox);
        materialEnabled->setObjectName(QStringLiteral("RenderingExperimentsMaterialEnabled"));
        materialLayout->addWidget(materialEnabled, 0, 0, 1, 3);

        specularStrength = new QSlider(Qt::Horizontal, materialBox);
        specularStrength->setObjectName(QStringLiteral("RenderingExperimentsSpecularStrength"));
        specularStrength->setRange(SpecularMinimumPercent, SpecularMaximumPercent);
        specularStrength->setValue(DefaultSpecularPercent);
        specularStrengthValue = new QLabel(materialBox);
        materialLayout->addWidget(new QLabel(QObject::tr("Specular"), materialBox), 1, 0);
        materialLayout->addWidget(specularStrength, 1, 1);
        materialLayout->addWidget(specularStrengthValue, 1, 2);

        shininess = new QSlider(Qt::Horizontal, materialBox);
        shininess->setObjectName(QStringLiteral("RenderingExperimentsShininess"));
        shininess->setRange(ShininessMinimumPercent, ShininessMaximumPercent);
        shininess->setValue(DefaultShininessPercent);
        shininessValue = new QLabel(materialBox);
        materialLayout->addWidget(new QLabel(QObject::tr("Shininess"), materialBox), 2, 0);
        materialLayout->addWidget(shininess, 2, 1);
        materialLayout->addWidget(shininessValue, 2, 2);
        outerLayout->addWidget(materialBox);

        auto* tessellationBox = new QGroupBox(QObject::tr("Manual tessellation"), self);
        auto* tessellationLayout = new QGridLayout(tessellationBox);
        tessellationEnabled
            = new QCheckBox(QObject::tr("Override local view-provider values"), tessellationBox);
        tessellationEnabled->setObjectName(QStringLiteral("RenderingExperimentsTessellationEnabled"));
        tessellationLayout->addWidget(tessellationEnabled, 0, 0, 1, 2);

        tessellationProfile = new QComboBox(tessellationBox);
        tessellationProfile->setObjectName(QStringLiteral("RenderingExperimentsTessellationProfile"));
        tessellationProfile->addItem(QObject::tr("Native"));
        tessellationProfile->addItem(QObject::tr("Coarse"));
        tessellationProfile->addItem(QObject::tr("Fine"));
        tessellationProfile->addItem(QObject::tr("Custom"));
        tessellationLayout->addWidget(new QLabel(QObject::tr("Profile"), tessellationBox), 1, 0);
        tessellationLayout->addWidget(tessellationProfile, 1, 1);

        deviation = new QDoubleSpinBox(tessellationBox);
        deviation->setDecimals(4);
        deviation->setRange(MinimumDeviation, MaximumDeviation);
        deviation->setSingleStep(0.01);
        deviation->setValue(DefaultCustomDeviation);
        tessellationLayout->addWidget(new QLabel(QObject::tr("Deviation"), tessellationBox), 2, 0);
        tessellationLayout->addWidget(deviation, 2, 1);

        angularDeflection = new QDoubleSpinBox(tessellationBox);
        angularDeflection->setDecimals(2);
        angularDeflection->setRange(1.0, 90.0);
        angularDeflection->setSingleStep(1.0);
        angularDeflection->setSuffix(QObject::tr(" deg"));
        angularDeflection->setValue(DefaultCustomAngularDegrees);
        tessellationLayout
            ->addWidget(new QLabel(QObject::tr("Angular deflection"), tessellationBox), 3, 0);
        tessellationLayout->addWidget(angularDeflection, 3, 1);
        outerLayout->addWidget(tessellationBox);

        statusLabel = new QLabel(self);
        statusLabel->setWordWrap(true);
        outerLayout->addWidget(statusLabel);
        outerLayout->addStretch(1);

        applyTimer = new QTimer(self);
        applyTimer->setObjectName(QStringLiteral("RenderingExperimentsApplyTimer"));
        applyTimer->setInterval(ApplyDebounceMilliseconds);
        applyTimer->setSingleShot(true);

        updateValueLabels();
        updateControlStates();
        updateStatus();
    }

    void setupConnections()
    {
        activeDocConnection = App::GetApplication().signalActiveDocument.connect(
            std::bind(&Private::onActiveDocument, this, std::placeholders::_1)
        );
        deleteDocConnection = App::GetApplication().signalDeleteDocument.connect(
            std::bind(&Private::onDeleteDocument, this, std::placeholders::_1)
        );
        startSaveConnection = App::GetApplication().signalStartSaveDocument.connect(
            std::bind(&Private::onStartSaveDocument, this, std::placeholders::_1, std::placeholders::_2)
        );
        finishSaveConnection = App::GetApplication().signalFinishSaveDocument.connect(
            std::bind(&Private::onFinishSaveDocument, this, std::placeholders::_1, std::placeholders::_2)
        );

        QObject::connect(masterEnabled, &QCheckBox::toggled, self, [this](bool enabled) {
            if (enabled) {
                applyExperiments();
            }
            else {
                cancelPendingUpdates();
                restoreToCapturedState(false);
            }
            updateControlStates();
            updateStatus();
        });

        QObject::connect(showNativeButton, &QPushButton::pressed, self, [this] {
            if (!masterEnabled->isChecked() || nativeHoldActive || !experimentsApplied) {
                return;
            }

            cancelPendingUpdates();
            restoreToCapturedState(true);
            nativeHoldActive = true;
            updateStatus();
        });
        QObject::connect(showNativeButton, &QPushButton::released, self, [this] {
            releaseNativeHold();
        });

        QObject::connect(resetButton, &QPushButton::clicked, self, [this] {
            discardOnHide = false;
            restoreToCapturedState(false);
            if (masterEnabled->isChecked()) {
                masterEnabled->setChecked(false);
            }
            else {
                discardCapturedState();
                updateControlStates();
                updateStatus();
            }
        });

        QObject::connect(lightingPreset, qOverload<int>(&QComboBox::currentIndexChanged), self, [this] {
            applyUpdates(UpdateLighting);
        });
        QObject::connect(lightingIntensity, &QSlider::valueChanged, self, [this] {
            updateValueLabels();
            scheduleUpdates(UpdateLighting);
        });

        QObject::connect(materialEnabled, &QCheckBox::toggled, self, [this] {
            updateControlStates();
            applyUpdates(UpdateMaterials);
        });
        QObject::connect(specularStrength, &QSlider::valueChanged, self, [this] {
            updateValueLabels();
            scheduleUpdates(UpdateMaterials);
        });
        QObject::connect(shininess, &QSlider::valueChanged, self, [this] {
            updateValueLabels();
            scheduleUpdates(UpdateMaterials);
        });

        QObject::connect(tessellationEnabled, &QCheckBox::toggled, self, [this] {
            updateControlStates();
            applyUpdates(UpdateTessellation);
        });
        QObject::connect(tessellationProfile, qOverload<int>(&QComboBox::currentIndexChanged), self, [this] {
            updateControlStates();
            applyUpdates(UpdateTessellation);
        });
        QObject::connect(deviation, qOverload<double>(&QDoubleSpinBox::valueChanged), self, [this] {
            scheduleUpdates(UpdateTessellation);
        });
        QObject::connect(
            angularDeflection,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            self,
            [this] { scheduleUpdates(UpdateTessellation); }
        );
        QObject::connect(applyTimer, &QTimer::timeout, self, [this] {
            const unsigned int updates = std::exchange(pendingUpdates, 0U);
            applyUpdates(updates);
        });
    }

    void updateValueLabels() const
    {
        lightingIntensityValue->setText(QStringLiteral("%1%").arg(lightingIntensity->value()));
        specularStrengthValue->setText(QStringLiteral("%1%").arg(specularStrength->value()));
        shininessValue->setText(QStringLiteral("%1%").arg(shininess->value()));
    }

    void updateControlStates() const
    {
        const bool masterOn = masterEnabled->isChecked() && documentActive;
        showNativeButton->setEnabled(masterOn);
        resetButton->setEnabled(
            experimentsApplied || masterEnabled->isChecked() || capturedViewerState.has_value()
        );

        lightingPreset->setEnabled(masterOn);
        lightingIntensity->setEnabled(masterOn);

        materialEnabled->setEnabled(masterOn);
        const bool materialsOn = masterOn && materialEnabled->isChecked();
        specularStrength->setEnabled(materialsOn);
        shininess->setEnabled(materialsOn);

        tessellationEnabled->setEnabled(masterOn);
        const bool tessellationOn = masterOn && tessellationEnabled->isChecked();
        tessellationProfile->setEnabled(tessellationOn);
        const bool customProfile = tessellationOn
            && static_cast<TessellationProfile>(tessellationProfile->currentIndex())
                == TessellationProfile::Custom;
        deviation->setEnabled(customProfile);
        angularDeflection->setEnabled(customProfile);
    }

    LightingPresetState currentLightingPreset() const
    {
        LightingPresetState state;
        const float intensityFactor
            = std::max(0.1F, static_cast<float>(lightingIntensity->value()) / 100.0F);

        switch (static_cast<LightingPreset>(lightingPreset->currentIndex())) {
            case LightingPreset::DiagnosticStudio:
                state.headlight = {
                    true,
                    0.95F * intensityFactor,
                    SbColor(0.98F, 0.98F, 0.96F),
                    SbVec3f(0.12F, -0.18F, -0.98F)
                };
                state.backlight = {
                    true,
                    0.32F * intensityFactor,
                    SbColor(0.82F, 0.88F, 1.0F),
                    SbVec3f(-0.24F, -0.42F, -0.88F)
                };
                state.fillLight = {
                    true,
                    0.24F * intensityFactor,
                    SbColor(1.0F, 0.96F, 0.90F),
                    SbVec3f(0.36F, -0.56F, 0.74F)
                };
                state.environment = {SbColor(0.18F, 0.18F, 0.20F), 0.20F * intensityFactor};
                break;
            case LightingPreset::Technical:
                state.headlight = {
                    true,
                    1.05F * intensityFactor,
                    SbColor(1.0F, 1.0F, 1.0F),
                    SbVec3f(0.05F, -0.10F, -0.99F)
                };
                state.backlight = {
                    true,
                    0.14F * intensityFactor,
                    SbColor(0.88F, 0.90F, 0.94F),
                    SbVec3f(-0.25F, 0.15F, 0.95F)
                };
                state.fillLight = {
                    true,
                    0.35F * intensityFactor,
                    SbColor(0.92F, 0.95F, 1.0F),
                    SbVec3f(-0.42F, -0.18F, -0.89F)
                };
                state.environment = {SbColor(0.10F, 0.10F, 0.12F), 0.12F * intensityFactor};
                break;
            case LightingPreset::HighContrast:
                state.headlight = {
                    true,
                    1.30F * intensityFactor,
                    SbColor(1.0F, 1.0F, 1.0F),
                    SbVec3f(0.20F, -0.22F, -0.95F)
                };
                state.backlight
                    = {false, 0.0F, SbColor(0.0F, 0.0F, 0.0F), SbVec3f(-0.30F, 0.18F, 0.94F)};
                state.fillLight = {
                    true,
                    0.20F * intensityFactor,
                    SbColor(0.90F, 0.94F, 1.0F),
                    SbVec3f(-0.70F, -0.18F, -0.69F)
                };
                state.environment = {SbColor(0.02F, 0.02F, 0.02F), 0.05F * intensityFactor};
                break;
        }

        return state;
    }

    void applyLightingOverrides()
    {
        auto* viewer = currentViewer();
        if (!viewer) {
            return;
        }

        const auto preset = currentLightingPreset();
        ViewerState state = capturedViewerState.value_or(captureViewerState(viewer));
        state.headlight = preset.headlight;
        state.backlight = preset.backlight;
        state.fillLight = preset.fillLight;
        state.environment = preset.environment;
        applyViewerState(viewer, state);
    }

    TessellationTargets getTessellationTargets(Gui::ViewProvider* viewProvider) const
    {
        TessellationTargets targets;
        if (!viewProvider) {
            return targets;
        }

        targets.deviation = asFloatProperty(viewProvider->getPropertyByName("Deviation"));
        targets.angularDeflection = asFloatProperty(
            viewProvider->getPropertyByName("AngularDeflection")
        );
        return targets;
    }

    std::pair<double, double> currentTessellationValues() const
    {
        switch (static_cast<TessellationProfile>(tessellationProfile->currentIndex())) {
            case TessellationProfile::Native:
                return {0.0, 0.0};
            case TessellationProfile::Coarse:
                return {0.5, 28.0};
            case TessellationProfile::Fine:
                return {0.08, 6.0};
            case TessellationProfile::Custom:
                return {deviation->value(), angularDeflection->value()};
        }

        return {deviation->value(), angularDeflection->value()};
    }

    void ensureViewerStateCaptured()
    {
        if (!capturedViewerState.has_value()) {
            capturedViewerState = captureViewerState(currentViewer());
        }
    }

    void ensureMaterialsCaptured()
    {
        if (!materialsCaptured) {
            captureMaterials();
        }
    }

    void ensureTessellationCaptured()
    {
        if (!tessellationCaptured) {
            captureTessellation();
        }
    }

    void captureMaterials()
    {
        capturedMaterials.clear();
        materialsCaptured = true;
        if (!document) {
            return;
        }

        for (auto* object : document->getObjects()) {
            auto* viewProvider = Gui::Application::Instance->getViewProvider(object);
            auto* materialProperty = asMaterialProperty(viewProvider);
            if (!materialProperty) {
                continue;
            }

            MaterialSnapshot snapshot;
            snapshot.objectName = object->getNameInDocument();
            snapshot.materials = materialProperty->getValues();
            capturedMaterials.emplace(snapshot.objectName, std::move(snapshot));
        }
    }

    void captureTessellation()
    {
        capturedTessellation.clear();
        tessellationCaptured = true;
        if (!document) {
            return;
        }

        for (auto* object : document->getObjects()) {
            auto* viewProvider = Gui::Application::Instance->getViewProvider(object);
            const TessellationTargets targets = getTessellationTargets(viewProvider);
            if (!targets.deviation && !targets.angularDeflection) {
                continue;
            }

            TessellationSnapshot snapshot;
            snapshot.objectName = object->getNameInDocument();
            if (targets.deviation) {
                snapshot.hasDeviation = true;
                snapshot.deviation = targets.deviation->getValue();
            }
            if (targets.angularDeflection) {
                snapshot.hasAngularDeflection = true;
                snapshot.angularDeflection = targets.angularDeflection->getValue();
            }
            capturedTessellation.emplace(snapshot.objectName, std::move(snapshot));
        }
    }

    void discardCapturedState()
    {
        capturedViewerState.reset();
        capturedMaterials.clear();
        capturedTessellation.clear();
        materialsCaptured = false;
        tessellationCaptured = false;
        experimentsApplied = false;
        nativeHoldActive = false;
    }

    void cancelPendingUpdates()
    {
        pendingUpdates = 0U;
        if (applyTimer) {
            applyTimer->stop();
        }
    }

    void clearPendingUpdates(unsigned int updates)
    {
        pendingUpdates &= ~updates;
        if (pendingUpdates == 0U && applyTimer) {
            applyTimer->stop();
        }
    }

    void restoreMaterials()
    {
        if (!document) {
            return;
        }

        for (const auto& [name, snapshot] : capturedMaterials) {
            auto* object = document->getObject(name.c_str());
            auto* viewProvider = object ? Gui::Application::Instance->getViewProvider(object)
                                        : nullptr;
            auto* materialProperty = asMaterialProperty(viewProvider);
            if (!materialProperty) {
                continue;
            }

            changeWithoutDocumentModification(materialProperty, [&] {
                materialProperty->setValues(snapshot.materials);
            });
            requestProviderUpdate(viewProvider);
        }
    }

    void restoreTessellation()
    {
        if (!document) {
            return;
        }

        for (const auto& [name, snapshot] : capturedTessellation) {
            auto* object = document->getObject(name.c_str());
            auto* viewProvider = object ? Gui::Application::Instance->getViewProvider(object)
                                        : nullptr;
            const TessellationTargets targets = getTessellationTargets(viewProvider);

            if (snapshot.hasDeviation && targets.deviation) {
                changeWithoutDocumentModification(targets.deviation, [&] {
                    targets.deviation->setValue(snapshot.deviation);
                });
            }
            if (snapshot.hasAngularDeflection && targets.angularDeflection) {
                changeWithoutDocumentModification(targets.angularDeflection, [&] {
                    targets.angularDeflection->setValue(snapshot.angularDeflection);
                });
            }
            requestProviderUpdate(viewProvider);
        }
    }

    void restoreToCapturedState(bool keepCaptured)
    {
        if (isRestoring) {
            return;
        }

        cancelPendingUpdates();
        isRestoring = true;

        if (capturedViewerState.has_value()) {
            applyViewerState(currentViewer(), *capturedViewerState);
        }
        restoreMaterials();
        restoreTessellation();

        experimentsApplied = false;
        nativeHoldActive = false;

        if (!keepCaptured) {
            discardCapturedState();
        }

        isRestoring = false;
        updateControlStates();
        updateStatus();
    }

    void applyMaterialOverrides()
    {
        if (!document || !materialEnabled->isChecked()) {
            return;
        }

        const float specularLevel = fromPercent(specularStrength->value());
        const float shininessLevel = fromPercent(shininess->value());

        for (const auto& [name, snapshot] : capturedMaterials) {
            auto* object = document->getObject(name.c_str());
            auto* viewProvider = object ? Gui::Application::Instance->getViewProvider(object)
                                        : nullptr;
            auto* materialProperty = asMaterialProperty(viewProvider);
            if (!materialProperty) {
                continue;
            }

            auto updated = snapshot.materials;
            for (auto& material : updated) {
                const SbColor diffuse(
                    material.diffuseColor.r,
                    material.diffuseColor.g,
                    material.diffuseColor.b
                );
                const SbColor specular = scaledColor(diffuse, specularLevel);
                // A material UUID makes App::Material equality identity-only. The temporary
                // appearance must be an inline value so PropertyMaterialList observes this change.
                material.uuid.clear();
                material.specularColor = Base::Color(specular[0], specular[1], specular[2]);
                material.shininess = shininessLevel;
            }

            changeWithoutDocumentModification(materialProperty, [&] {
                materialProperty->setValues(updated);
            });
            requestProviderUpdate(viewProvider);
        }
    }

    void applyTessellationOverrides()
    {
        if (!document || !tessellationEnabled->isChecked()) {
            return;
        }

        if (static_cast<TessellationProfile>(tessellationProfile->currentIndex())
            == TessellationProfile::Native) {
            return;
        }

        const auto [deviationValue, angularValue] = currentTessellationValues();

        for (const auto& [name, snapshot] : capturedTessellation) {
            auto* object = document->getObject(name.c_str());
            auto* viewProvider = object ? Gui::Application::Instance->getViewProvider(object)
                                        : nullptr;
            const TessellationTargets targets = getTessellationTargets(viewProvider);

            if (snapshot.hasDeviation && targets.deviation) {
                changeWithoutDocumentModification(targets.deviation, [&] {
                    targets.deviation->setValue(deviationValue);
                });
            }
            if (snapshot.hasAngularDeflection && targets.angularDeflection) {
                changeWithoutDocumentModification(targets.angularDeflection, [&] {
                    targets.angularDeflection->setValue(angularValue);
                });
            }
            requestProviderUpdate(viewProvider);
        }
    }

    void scheduleUpdates(unsigned int updates)
    {
        if (!masterEnabled->isChecked() || !documentActive || nativeHoldActive) {
            return;
        }

        pendingUpdates |= updates;
        applyTimer->start();
    }

    void applyUpdates(unsigned int updates)
    {
        clearPendingUpdates(updates);
        if (updates == 0U || isRestoring || !documentActive || nativeHoldActive
            || !masterEnabled->isChecked()) {
            updateStatus();
            return;
        }

        if ((updates & UpdateLighting) != 0U) {
            ensureViewerStateCaptured();
            applyLightingOverrides();
        }

        if ((updates & UpdateMaterials) != 0U) {
            if (materialEnabled->isChecked()) {
                ensureMaterialsCaptured();
                applyMaterialOverrides();
            }
            else if (materialsCaptured) {
                restoreMaterials();
            }
        }

        if ((updates & UpdateTessellation) != 0U) {
            const bool nativeProfile = static_cast<TessellationProfile>(
                                           tessellationProfile->currentIndex()
                                       )
                == TessellationProfile::Native;
            if (tessellationEnabled->isChecked() && !nativeProfile) {
                ensureTessellationCaptured();
                applyTessellationOverrides();
            }
            else if (tessellationCaptured) {
                restoreTessellation();
            }
        }

        experimentsApplied = true;
        updateControlStates();
        updateStatus();
    }

    void applyExperiments()
    {
        if (isRestoring || !documentActive || nativeHoldActive) {
            updateStatus();
            return;
        }

        if (!masterEnabled->isChecked()) {
            updateStatus();
            return;
        }

        cancelPendingUpdates();
        applyUpdates(UpdateLighting | UpdateMaterials | UpdateTessellation);
    }

    Gui::View3DInventorViewer* currentViewer() const
    {
        return view ? view->getViewer() : nullptr;
    }

    int supportedMaterialObjectCount() const
    {
        if (capturedMaterials.empty()) {
            return 0;
        }
        return static_cast<int>(capturedMaterials.size());
    }

    int supportedTessellationObjectCount() const
    {
        if (capturedTessellation.empty()) {
            return 0;
        }
        return static_cast<int>(capturedTessellation.size());
    }

    void updateStatus() const
    {
        if (!view || !document) {
            statusLabel->setText(QObject::tr("Status: view or document is no longer available."));
            return;
        }

        const int materialCount = supportedMaterialObjectCount();
        const int tessellationCount = supportedTessellationObjectCount();

        if (!documentActive) {
            statusLabel->setText(
                QObject::tr("Status: native view restored while another document is active.")
            );
            return;
        }

        if (!masterEnabled->isChecked()) {
            statusLabel->setText(
                QObject::tr("Status: native view. Supported objects: %1, %2.")
                    .arg(describeObjectCount(
                        materialCount,
                        QObject::tr("material target"),
                        QObject::tr("material targets")
                    ))
                    .arg(describeObjectCount(
                        tessellationCount,
                        QObject::tr("tessellation target"),
                        QObject::tr("tessellation targets")
                    ))
            );
            return;
        }

        if (nativeHoldActive) {
            statusLabel->setText(
                QObject::tr("Status: holding native view while the button is pressed.")
            );
            return;
        }

        if (materialCount == 0 && tessellationCount == 0) {
            statusLabel->setText(
                QObject::tr("Status: experiments applied, but no active supported objects were found.")
            );
            return;
        }

        QStringList sections;
        sections.append(QObject::tr("lighting"));
        if (materialEnabled->isChecked()) {
            sections.append(QObject::tr("materials"));
        }
        if (tessellationEnabled->isChecked()) {
            sections.append(QObject::tr("manual tessellation"));
        }

        statusLabel->setText(
            QObject::tr("Status: experimental view active (%1). Supported objects: %2, %3.")
                .arg(sections.join(QStringLiteral(", ")))
                .arg(describeObjectCount(
                    materialCount,
                    QObject::tr("material target"),
                    QObject::tr("material targets")
                ))
                .arg(describeObjectCount(
                    tessellationCount,
                    QObject::tr("tessellation target"),
                    QObject::tr("tessellation targets")
                ))
        );
    }

    void releaseNativeHold()
    {
        if (!nativeHoldActive) {
            return;
        }

        nativeHoldActive = false;
        if (masterEnabled->isChecked() && documentActive) {
            applyExperiments();
        }
        else {
            updateStatus();
        }
    }

    void onActiveDocument(const App::Document& activeDocument)
    {
        const bool nowActive = (&activeDocument == document);
        if (documentActive == nowActive) {
            return;
        }

        documentActive = nowActive;
        if (!dockWidget) {
            return;
        }

        if (documentActive) {
            dockWidget->show();
            if (masterEnabled->isChecked()) {
                applyExperiments();
            }
            else {
                updateStatus();
            }
        }
        else {
            restoreToCapturedState(false);
            dockWidget->hide();
        }
    }

    void onDeleteDocument(const App::Document& deletedDocument)
    {
        if (&deletedDocument != document) {
            return;
        }

        document = nullptr;
        view.clear();
        if (dockWidget) {
            dockWidget->deleteLater();
        }
        else if (self) {
            self->deleteLater();
        }
    }

    void onStartSaveDocument(const App::Document& savedDocument, const std::string&)
    {
        if (&savedDocument != document || !masterEnabled->isChecked()
            || !capturedViewerState.has_value()) {
            return;
        }

        saveSuspended = true;
        nativeHoldDuringSave = nativeHoldActive;
        if (experimentsApplied) {
            restoreToCapturedState(true);
        }
        nativeHoldActive = nativeHoldDuringSave;
    }

    void onFinishSaveDocument(const App::Document& savedDocument, const std::string&)
    {
        if (&savedDocument != document || !saveSuspended) {
            return;
        }

        saveSuspended = false;
        nativeHoldActive = nativeHoldDuringSave;
        nativeHoldDuringSave = false;
        if (!nativeHoldActive && masterEnabled->isChecked() && documentActive) {
            applyExperiments();
        }
        else {
            updateStatus();
        }
    }
};

RenderingExperiments* RenderingExperiments::makeDockWidget(Gui::View3DInventor* view, App::Document* showOn)
{
    auto* widget = new RenderingExperiments(view, showOn);
    auto* dockManager = Gui::DockWindowManager::instance();
    QDockWidget* dockWidget
        = dockManager->addDockWindow("Rendering Experiments", widget, Qt::RightDockWidgetArea);
    dockWidget->setFeatures(
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
        | QDockWidget::DockWidgetClosable
    );
    dockWidget->show();
    widget->d->dockWidget = dockWidget;
    return widget;
}

RenderingExperiments::RenderingExperiments(Gui::View3DInventor* view, App::Document* showOn, QWidget* parent)
    : QWidget(parent)
    , d(new Private(this, view, showOn))
{
    setObjectName(QStringLiteral("RenderingExperimentsPanel"));
    setWindowTitle(tr("Rendering Experiments"));
    d->buildUi();
    d->setupConnections();

    if (const auto* active = App::GetApplication().getActiveDocument()) {
        d->documentActive = (active == showOn);
    }

    d->updateControlStates();
    d->updateStatus();
}

RenderingExperiments::~RenderingExperiments()
{
    d->activeDocConnection.disconnect();
    d->deleteDocConnection.disconnect();
    d->startSaveConnection.disconnect();
    d->finishSaveConnection.disconnect();
    d->restoreToCapturedState(false);
    delete d;
}

void RenderingExperiments::bindTo(Gui::View3DInventor* view, App::Document* showOn)
{
    if (d->view == view && d->document == showOn) {
        return;
    }

    d->restoreToCapturedState(false);
    d->view = view;
    d->document = showOn;
    d->documentActive = (App::GetApplication().getActiveDocument() == showOn);
    d->masterEnabled->setChecked(false);
    d->updateControlStates();
    d->updateStatus();
}

void RenderingExperiments::closeEvent(QCloseEvent* event)
{
    d->discardOnHide = true;
    d->restoreToCapturedState(false);
    QWidget::closeEvent(event);
    if (auto* dock = qobject_cast<QDockWidget*>(parent())) {
        dock->deleteLater();
    }
}

void RenderingExperiments::hideEvent(QHideEvent* event)
{
    if (d->discardOnHide) {
        d->discardOnHide = false;
        QWidget::hideEvent(event);
        return;
    }

    if (!d->documentActive) {
        d->restoreToCapturedState(true);
    }
    else if (d->masterEnabled->isChecked()) {
        d->restoreToCapturedState(false);
        d->masterEnabled->setChecked(false);
    }

    QWidget::hideEvent(event);
}

void RenderingExperiments::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (d->documentActive && d->masterEnabled->isChecked()) {
        d->applyExperiments();
    }
    else {
        d->updateStatus();
    }
}
