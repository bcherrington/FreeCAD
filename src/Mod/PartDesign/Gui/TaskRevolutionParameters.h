// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#pragma once

#include <memory>
#include <string>

#include <QMetaObject>

#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include "TaskSketchBasedParameters.h"


class Ui_TaskRevolutionParameters;
class QAbstractButton;
class QComboBox;
class QLabel;
class QLineEdit;

namespace App
{
class Property;
}

namespace Gui
{
class QuantitySpinBox;
class RadialGizmo;
class Gizmo;
class ViewProvider;
class ViewProviderCoordinateSystem;
class AsyncPreviewSession;
}  // namespace Gui

namespace PartDesignGui
{
class ViewProviderRevolution;
class ViewProviderGroove;

class PartDesignGuiExport TaskRevolutionParameters: public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskRevolutionParameters(
        ViewProvider* RevolutionView,
        const char* pixname,
        const QString& title,
        QWidget* parent = nullptr
    );
    ~TaskRevolutionParameters() override;

    void apply() override;

    /**
     * @brief fillAxisCombo fills the combo and selects the item according to
     * current value of revolution object's axis reference.
     * @param forceRefill if true, the combo box will be completely refilled. If
     * false, the current value of revolution object's axis will be added to the
     * list (if necessary), and selected. If the list is empty, it will be refilled anyway.
     */
    void fillAxisCombo(bool forceRefill = false);
    void addAxisToCombo(
        App::DocumentObject* linkObj,
        const std::string& linkSubname,
        const QString& itemText
    );
    void flushPendingRecompute() override;
    void stopPendingRecompute() override;
    bool hasOutstandingRecompute() const override;
    void setDeferredClosePending(bool pending) override;
    Gui::AsyncPreviewSession* getAcceptedRecomputeProgressSession() override;
    void clearInteractiveSelection();

Q_SIGNALS:
    void recomputeSettled();

private Q_SLOTS:
    void onUpdateView(bool);
    void onAngleChanged(double);
    void onAngle2Changed(double);
    void onAxisChanged(int);
    void onReversed(bool);
    void onModeChangedSide1(int);
    void onModeChangedSide2(int);
    void onSidesModeChanged(int);

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void changeEvent(QEvent* event) override;
    void triggerPreviewRecompute() override;
    void getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;
    bool getReversed() const;
    int getMode() const;
    int getMode2() const;
    int getSidesMode() const;
    QString getFaceName(QLineEdit* lineEdit) const;
    void setupDialog();

    enum class SidesMode
    {
        OneSide,
        TwoSides,
        Symmetric,
    };

    enum class Side
    {
        First,
        Second,
    };

    enum class Mode
    {
        Angle,
        ThroughAll,
        ToLast = ThroughAll,
        ToFirst,
        ToFace,
        TwoAngles,
    };

private:
    struct SideController
    {
        QComboBox* changeMode = nullptr;
        QLabel* labelAngle = nullptr;
        Gui::QuantitySpinBox* angleEdit = nullptr;
        QAbstractButton* buttonFace = nullptr;
        QLineEdit* lineFaceName = nullptr;

        App::PropertyEnumeration* Type = nullptr;
        App::PropertyAngle* Angle = nullptr;
        App::PropertyLinkSub* UpToFace = nullptr;
    };

    SideController m_side1;
    SideController m_side2;

    SideController& getSideController(Side side)
    {
        return side == Side::First ? m_side1 : m_side2;
    }

    const SideController& getSideController(Side side) const
    {
        return side == Side::First ? m_side1 : m_side2;
    }

    App::PropertyEnumeration* propSideType;
    App::PropertyBool* propReversed;
    App::PropertyLinkSub* propReferenceAxis;

private:
    void createSideControllers();
    void setupSideDialog(SideController& side);
    void connectSignals();
    void updateUI(Side side);
    void updateWholeUI(Side side);
    void updateSideUI(const SideController& side, Mode mode, bool isParentVisible, bool setFocus);
    void translateModeList(QComboBox* box, int index);
    void translateSidesList(int index);
    void schedulePendingRecompute();
    void runImmediateRecompute();
    void requestRecompute(bool waitForCompletion);
    void updateRecomputeUi();
    // TODO: This is common with extrude. Maybe send to superclass.
    void translateFaceName(QLineEdit* lineEdit);
    void handleLineFaceNameClick(QLineEdit* lineEdit);
    void handleLineFaceNameNo(QLineEdit* lineEdit);
    void clearFaceName(QLineEdit* lineEdit);
    void onModeChanged(int index, Side side);
    void onButtonFace(bool pressed, Side side);
    void onFaceName(const QString& text, Side side);
    Gui::ViewProviderCoordinateSystem* getOriginView() const;

private:
    std::unique_ptr<Ui_TaskRevolutionParameters> ui;
    QWidget* proxy;
    bool selectionFace;
    bool isGroove;
    Side activeSelectionSide;
    double defaultGizmoMultFactor;

    /**
     * @brief axesInList is the list of links corresponding to axis combo; must
     * be kept in sync with the combo. A special value of zero-pointer link is
     * for "Select axis" item.
     *
     * It is a list of pointers, because properties prohibit assignment. Use new
     * when adding stuff, and delete when removing stuff.
     */
    std::vector<std::unique_ptr<App::PropertyLinkSub>> axesInList;

    std::unique_ptr<Gui::AsyncPreviewSession> asyncPreviewSession;

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::RadialGizmo* rotationGizmo = nullptr;
    Gui::RadialGizmo* rotationGizmo2 = nullptr;
    void setupGizmos(ViewProvider* vp);
    void setGizmoPositions();
};

class PartDesignGuiExport TaskDlgRevolutionBase: public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgRevolutionBase(PartDesignGui::ViewProvider* vp);
    bool accept() override;
    bool reject() override;

protected:
    TaskRevolutionParameters* parameter = nullptr;

private Q_SLOTS:
    void onParameterRecomputeSettled();
};

class PartDesignGuiExport TaskDlgRevolutionParameters: public TaskDlgRevolutionBase
{
    Q_OBJECT

public:
    explicit TaskDlgRevolutionParameters(PartDesignGui::ViewProviderRevolution* RevolutionView);
};

class PartDesignGuiExport TaskDlgGrooveParameters: public TaskDlgRevolutionBase
{
    Q_OBJECT

public:
    explicit TaskDlgGrooveParameters(PartDesignGui::ViewProviderGroove* GrooveView);
};

}  // namespace PartDesignGui
