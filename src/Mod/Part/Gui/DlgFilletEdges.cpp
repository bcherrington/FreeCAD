// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <climits>
#include <sstream>
#include <QHeaderView>
#include <QItemDelegate>
#include <QItemSelectionModel>
#include <QLocale>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>
#include <QVBoxLayout>

#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopExp.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/details/SoLineDetail.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <App/ElementNamingUtils.h>
#include <Base/UnitsApi.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/AsyncRecomputeProgressDialog.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Window.h>
#include <Mod/Part/App/FeatureChamfer.h>
#include <Mod/Part/App/FeatureFillet.h>

#include "DlgFilletEdges.h"
#include "ui_DlgFilletEdges.h"
#include "SoBrepEdgeSet.h"
#include "SoBrepFaceSet.h"
#include "SoBrepPointSet.h"

FC_LOG_LEVEL_INIT("Part", true, true)

using namespace PartGui;
namespace sp = std::placeholders;

FilletRadiusDelegate::FilletRadiusDelegate(QObject* parent)
    : QItemDelegate(parent)
{}

QWidget* FilletRadiusDelegate::createEditor(
    QWidget* parent,
    const QStyleOptionViewItem& /* option */,
    const QModelIndex& index
) const
{
    if (index.column() < 1) {
        return nullptr;
    }

    Gui::QuantitySpinBox* editor = new Gui::QuantitySpinBox(parent);
    editor->setUnit(Base::Unit::Length);
    editor->setMinimum(0.0);
    editor->setMaximum(std::numeric_limits<int>::max());
    editor->setSingleStep(0.1);

    return editor;
}

void FilletRadiusDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    Base::Quantity value = index.model()->data(index, Qt::EditRole).value<Base::Quantity>();

    Gui::QuantitySpinBox* spinBox = static_cast<Gui::QuantitySpinBox*>(editor);
    spinBox->setValue(value);
}

void FilletRadiusDelegate::setModelData(
    QWidget* editor,
    QAbstractItemModel* model,
    const QModelIndex& index
) const
{
    Gui::QuantitySpinBox* spinBox = static_cast<Gui::QuantitySpinBox*>(editor);
    spinBox->interpretText();
    // double value = spinBox->value();
    // QString value = QStringLiteral("%1").arg(spinBox->value(),0,'f',2);
    // QString value = QLocale().toString(spinBox->value().getValue(),'f',Base::UnitsApi::getDecimals());
    Base::Quantity value = spinBox->value();

    model->setData(index, QVariant::fromValue<Base::Quantity>(value), Qt::EditRole);
}

void FilletRadiusDelegate::
    updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /* index */) const
{
    editor->setGeometry(option.rect);
}

// --------------------------------------------------------------

FilletRadiusModel::FilletRadiusModel(QObject* parent)
    : QStandardItemModel(parent)
{}

void FilletRadiusModel::updateCheckStates()
{
    // See
    // http://www.qtcentre.org/threads/18856-Checkboxes-in-Treeview-do-not-get-refreshed?s=b0fea2bfc66da1098413ae9f2a651a68&p=93201#post93201
    Q_EMIT layoutChanged();
}

Qt::ItemFlags FilletRadiusModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags fl = QStandardItemModel::flags(index);
    if (index.column() == 0) {
        fl = fl | Qt::ItemIsUserCheckable;
    }
    return fl;
}

bool FilletRadiusModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    bool ok = QStandardItemModel::setData(index, value, role);
    if (role == Qt::CheckStateRole) {
        Q_EMIT toggleCheckState(index);
    }
    return ok;
}

QVariant FilletRadiusModel::data(const QModelIndex& index, int role) const
{
    QVariant value = QStandardItemModel::data(index, role);
    if (role == Qt::DisplayRole && index.column() >= 1) {
        Base::Quantity q = value.value<Base::Quantity>();
        QString str = QString::fromStdString(q.getUserString());
        return str;
    }
    return value;
}

// --------------------------------------------------------------

namespace PartGui
{
class EdgeFaceSelection: public Gui::SelectionFilterGate
{
    bool allowEdge {true};
    App::DocumentObject*& object;

public:
    explicit EdgeFaceSelection(App::DocumentObject*& obj)
        : Gui::SelectionFilterGate(nullPointer())
        , object(obj)
    {}
    void selectEdges()
    {
        allowEdge = true;
    }
    void selectFaces()
    {
        allowEdge = false;
    }
    bool allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* sSubName) override
    {
        if (pObj != this->object) {
            return false;
        }
        if (Base::Tools::isNullOrEmpty(sSubName)) {
            return false;
        }
        std::string element(sSubName);
        if (allowEdge) {
            return element.substr(0, 4) == "Edge";
        }
        else {
            return element.substr(0, 4) == "Face";
        }
    }
};
class DlgFilletEdges::Private
{
public:
    App::DocumentObject* object;
    EdgeFaceSelection* selection;
    Part::FilletBase* fillet;
    App::DocumentObjectWeakPtrT candidate {nullptr};
    App::DocumentObjectWeakPtrT base {nullptr};
    std::unique_ptr<App::DocumentWeakPtrT> editDocument;
    int transactionId = 0;
    bool openedTransaction = false;
    bool initialCandidate = false;
    bool createdCandidate = false;
    bool creationPending = false;
    bool candidateRetired = false;
    bool sourceRetired = false;
    bool capturedVisibility = false;
    bool baseWasVisible = true;
    bool candidateWasVisible = true;
    QTimer* highlighttimer;
    FilletType filletType;
    std::vector<int> edge_ids;
    TopTools_IndexedMapOfShape all_edges;
    TopTools_IndexedMapOfShape all_faces;
    using Connection = fastsignals::connection;
    Connection connectApplicationDeletedObject;
    Connection connectApplicationDeletedDocument;

    class SelectionObjectCompare
    {
    public:
        App::DocumentObject* obj;
        explicit SelectionObjectCompare(App::DocumentObject* obj)
            : obj(obj)
        {}
        bool operator()(const Gui::SelectionObject& sel) const
        {
            return (sel.getObject() == obj);
        }
    };
};
}  // namespace PartGui

/* TRANSLATOR PartGui::DlgFilletEdges */

DlgFilletEdges::DlgFilletEdges(
    FilletType type,
    Part::FilletBase* fillet,
    QWidget* parent,
    Qt::WindowFlags fl
)
    : QWidget(parent, fl)
    , ui(new Ui_DlgFilletEdges())
    , d(new Private())
{
    ui->setupUi(this);
    setupConnections();

    ui->filletStartRadius->setMaximum(std::numeric_limits<int>::max());
    ui->filletStartRadius->setMinimum(0);
    ui->filletStartRadius->setUnit(Base::Unit::Length);

    ui->filletEndRadius->setMaximum(std::numeric_limits<int>::max());
    ui->filletEndRadius->setMinimum(0);
    ui->filletEndRadius->setUnit(Base::Unit::Length);

    d->object = nullptr;
    setSelectionGate();

    d->fillet = fillet;
    d->candidate = fillet;
    d->initialCandidate = fillet != nullptr;
    if (fillet) {
        d->editDocument = std::make_unique<App::DocumentWeakPtrT>(fillet->getDocument());
        d->base = fillet->Base.getValue();
        if (auto* source = d->base.get<App::DocumentObject>()) {
            if (auto* view = Gui::Application::Instance->getViewProvider(source)) {
                d->baseWasVisible = view->isShow();
                d->capturedVisibility = true;
            }
        }
        if (auto* view = Gui::Application::Instance->getViewProvider(fillet)) {
            d->candidateWasVisible = view->isShow();
        }
    }
    // NOLINTBEGIN
    d->connectApplicationDeletedObject = App::GetApplication().signalDeletedObject.connect(
        std::bind(&DlgFilletEdges::onDeleteObject, this, sp::_1)
    );
    d->connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(
        std::bind(&DlgFilletEdges::onDeleteDocument, this, sp::_1)
    );
    // NOLINTEND
    //  set tree view with three columns
    FilletRadiusModel* model = new FilletRadiusModel(this);
    connect(model, &FilletRadiusModel::toggleCheckState, this, &DlgFilletEdges::toggleCheckState);
    model->insertColumns(0, 3);

    // timer for highlighting
    d->highlighttimer = new QTimer(this);
    d->highlighttimer->setSingleShot(true);
    connect(d->highlighttimer, &QTimer::timeout, this, &DlgFilletEdges::onHighlightEdges);

    d->filletType = type;
    if (d->filletType == DlgFilletEdges::CHAMFER) {
        ui->parameterName->setTitle(tr("Chamfer parameters"));
        ui->labelfillet->setText(tr("Chamfer type"));
        ui->labelRadius->setText(tr("Length"));
        ui->filletType->setItemText(0, tr("Equal distance"));
        ui->filletType->setItemText(1, tr("Two distances"));

        model->setHeaderData(0, Qt::Horizontal, tr("Edges to chamfer"), Qt::DisplayRole);
        model->setHeaderData(1, Qt::Horizontal, tr("Size"), Qt::DisplayRole);
        model->setHeaderData(2, Qt::Horizontal, tr("Size2"), Qt::DisplayRole);
    }
    else {
        ui->parameterName->setTitle(tr("Fillet parameter"));
        ui->labelfillet->setText(tr("Fillet type"));
        model->setHeaderData(0, Qt::Horizontal, tr("Edges to fillet"), Qt::DisplayRole);
        model->setHeaderData(1, Qt::Horizontal, tr("Start radius"), Qt::DisplayRole);
        model->setHeaderData(2, Qt::Horizontal, tr("End radius"), Qt::DisplayRole);
    }
    ui->treeView->setRootIsDecorated(false);
    ui->treeView->setItemDelegate(new FilletRadiusDelegate(this));
    ui->treeView->setModel(model);

    QHeaderView* header = ui->treeView->header();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setDefaultAlignment(Qt::AlignLeft);
    header->setSectionsMovable(false);
    onFilletTypeActivated(0);
    findShapes();
    if (!d->editDocument && d->object) {
        d->editDocument = std::make_unique<App::DocumentWeakPtrT>(d->object->getDocument());
    }
    if (d->editDocument && !d->editDocument->expired()) {
        auto* document = **d->editDocument;
        const int id = document->getBookedTransactionID();
        if (document->hasPendingTransaction() || App::GetApplication().transactionIsActive(id)) {
            d->transactionId = id;
        }
    }
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgFilletEdges::~DlgFilletEdges()
{
    // no need to delete child widgets, Qt does it all for us
    d->connectApplicationDeletedDocument.disconnect();
    d->connectApplicationDeletedObject.disconnect();
    Gui::Selection().rmvSelectionGate();
}

void DlgFilletEdges::setupConnections()
{
    // clang-format off
    connect(ui->shapeObject, qOverload<int>(&QComboBox::activated),
            this, &DlgFilletEdges::onShapeObjectActivated);
    connect(ui->selectEdges, &QRadioButton::toggled,
            this, &DlgFilletEdges::onSelectEdgesToggled);
    connect(ui->selectFaces, &QRadioButton::toggled,
            this, &DlgFilletEdges::onSelectFacesToggled);
    connect(ui->selectAllButton, &QPushButton::clicked,
            this, &DlgFilletEdges::onSelectAllButtonClicked);
    connect(ui->selectNoneButton, &QPushButton::clicked,
            this, &DlgFilletEdges::onSelectNoneButtonClicked);
    connect(ui->filletType, qOverload<int>(&QComboBox::activated),
            this, &DlgFilletEdges::onFilletTypeActivated);
    connect(ui->filletStartRadius,
            qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged),
            this, &DlgFilletEdges::onFilletStartRadiusValueChanged);
    connect(ui->filletEndRadius,
            qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged),
            this, &DlgFilletEdges::onFilletEndRadiusValueChanged);
    // clang-format on
}

void DlgFilletEdges::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // no object selected in the combobox or no sub-element was selected
    if (!d->object || !msg.pSubName) {
        return;
    }
    if (msg.Type == Gui::SelectionChanges::AddSelection
        || msg.Type == Gui::SelectionChanges::RmvSelection) {
        // when adding a sub-element to the selection check
        // whether this is the currently handled object
        App::Document* doc = d->object->getDocument();
        std::string docname = doc->getName();
        std::string objname = d->object->getNameInDocument();
        if (docname == msg.pDocName && objname == msg.pObjectName) {
            QString subelement = QString::fromUtf8(msg.pSubName);
            if (subelement.startsWith(QLatin1String("Edge"))) {
                onSelectEdge(subelement, msg.Type);
            }
            else if (subelement.startsWith(QLatin1String("Face"))) {
                d->selection->selectEdges();
                onSelectEdgesOfFace(subelement, msg.Type);
                d->selection->selectFaces();
            }
        }
    }

    if (msg.Type != Gui::SelectionChanges::SetPreselect
        && msg.Type != Gui::SelectionChanges::RmvPreselect) {
        d->highlighttimer->start(20);
    }
}

void DlgFilletEdges::onHighlightEdges()
{
    Gui::ViewProvider* view = Gui::Application::Instance->getViewProvider(d->object);
    if (view) {
        // deselect all faces
        {
            SoSearchAction searchAction;
            searchAction.setType(PartGui::SoBrepFaceSet::getClassTypeId());
            searchAction.setInterest(SoSearchAction::FIRST);
            searchAction.apply(view->getRoot());
            SoPath* selectionPath = searchAction.getPath();
            if (selectionPath) {
                Gui::SoSelectionElementAction action(Gui::SoSelectionElementAction::None);
                action.apply(selectionPath);
            }
        }
        // deselect all points
        {
            SoSearchAction searchAction;
            searchAction.setType(PartGui::SoBrepPointSet::getClassTypeId());
            searchAction.setInterest(SoSearchAction::FIRST);
            searchAction.apply(view->getRoot());
            SoPath* selectionPath = searchAction.getPath();
            if (selectionPath) {
                Gui::SoSelectionElementAction action(Gui::SoSelectionElementAction::None);
                action.apply(selectionPath);
            }
        }
        // select the edges
        {
            SoSearchAction searchAction;
            searchAction.setType(PartGui::SoBrepEdgeSet::getClassTypeId());
            searchAction.setInterest(SoSearchAction::FIRST);
            searchAction.apply(view->getRoot());
            SoPath* selectionPath = searchAction.getPath();
            if (selectionPath) {
                ParameterGrp::handle hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup(
                    "View"
                );
                SbColor selectionColor(0.1f, 0.8f, 0.1f);
                unsigned long selection = (unsigned long)(selectionColor.getPackedValue());
                selection = hGrp->GetUnsigned("SelectionColor", selection);
                float transparency;
                selectionColor.setPackedValue((uint32_t)selection, transparency);

                // clear the selection first
                Gui::SoSelectionElementAction clear(Gui::SoSelectionElementAction::None);
                clear.apply(selectionPath);

                Gui::SoSelectionElementAction action(Gui::SoSelectionElementAction::Append);
                action.setColor(selectionColor);
                action.apply(selectionPath);

                QAbstractItemModel* model = ui->treeView->model();
                SoLineDetail detail;
                action.setElement(&detail);
                for (int i = 0; i < model->rowCount(); ++i) {
                    QVariant value = model->index(i, 0).data(Qt::CheckStateRole);
                    Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

                    // is item checked
                    if (checkState & Qt::Checked) {
                        // the index value of the edge
                        int id = model->index(i, 0).data(Qt::UserRole).toInt();
                        detail.setLineIndex(id - 1);
                        action.apply(selectionPath);
                    }
                }
            }
        }
    }
}

void DlgFilletEdges::onSelectEdge(const QString& subelement, int type)
{
    Gui::SelectionChanges::MsgType msgType = Gui::SelectionChanges::MsgType(type);
    QAbstractItemModel* model = ui->treeView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        int id = model->data(model->index(i, 0), Qt::UserRole).toInt();
        QString name = QStringLiteral("Edge%1").arg(id);
        if (name == subelement) {
            // ok, check the selected sub-element
            Qt::CheckState checkState
                = (msgType == Gui::SelectionChanges::AddSelection ? Qt::Checked : Qt::Unchecked);
            QVariant value(static_cast<int>(checkState));
            QModelIndex index = model->index(i, 0);
            model->setData(index, value, Qt::CheckStateRole);
            // select the item
            ui->treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
            QItemSelection selection(index, model->index(i, 1));
            ui->treeView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
            ui->treeView->update();
            break;
        }
    }
}

void DlgFilletEdges::onSelectEdgesOfFace(const QString& subelement, int type)
{
    bool ok;
    int index = subelement.mid(4).toInt(&ok);
    if (ok) {
        try {
            const TopoDS_Shape& face = d->all_faces.FindKey(index);
            TopTools_IndexedMapOfShape mapOfEdges;
            TopExp::MapShapes(face, TopAbs_EDGE, mapOfEdges);

            for (int j = 1; j <= mapOfEdges.Extent(); ++j) {
                TopoDS_Edge edge = TopoDS::Edge(mapOfEdges.FindKey(j));
                int id = d->all_edges.FindIndex(edge);
                QString name = QStringLiteral("Edge%1").arg(id);
                onSelectEdge(name, type);
                Gui::SelectionChanges::MsgType msgType = Gui::SelectionChanges::MsgType(type);
                if (msgType == Gui::SelectionChanges::AddSelection) {
                    Gui::Selection().addSelection(
                        d->object->getDocument()->getName(),
                        d->object->getNameInDocument(),
                        (const char*)name.toUtf8()
                    );
                }
            }
        }
        catch (Standard_Failure&) {
        }
    }
}

void DlgFilletEdges::onDeleteObject(const App::DocumentObject& obj)
{
    if (d->creationPending && !d->fillet && d->editDocument
        && **d->editDocument == obj.getDocument()) {
        // An earlier creation observer may remove the new object before our
        // observer sees it. Never bind a later object with the same name.
        d->candidateRetired = true;
    }
    if (d->object == &obj || *d->base == &obj) {
        d->sourceRetired = true;
        d->base.reset();
    }
    if (d->fillet == &obj) {
        // Unlike the undo-aware weak holder, retirement of an in-flight
        // candidate is permanent for this task, even if its address is reused.
        d->candidateRetired = true;
        d->candidate.reset();
        d->fillet = nullptr;
    }
    else if (d->fillet && d->fillet->Base.getValue() == &obj) {
        d->fillet = nullptr;
        d->object = nullptr;
        ui->shapeObject->setCurrentIndex(0);
        onShapeObjectActivated(0);
    }
    else if (d->object == &obj) {
        d->object = nullptr;
        ui->shapeObject->removeItem(ui->shapeObject->currentIndex());
        ui->shapeObject->setCurrentIndex(0);
        onShapeObjectActivated(0);
    }
    else {
        QString shape = QString::fromUtf8(obj.getNameInDocument());
        // start from the second item
        for (int i = 1; i < ui->shapeObject->count(); i++) {
            if (ui->shapeObject->itemData(i).toString() == shape) {
                ui->shapeObject->removeItem(i);
                break;
            }
        }
    }
}

void DlgFilletEdges::onDeleteDocument(const App::Document& doc)
{
    if (d->object && d->object->getDocument() == &doc) {
        d->sourceRetired = true;
        d->candidateRetired = true;
        d->candidate.reset();
        d->base.reset();
    }
    if (d->object) {
        if (d->object->getDocument() == &doc) {
            ui->shapeObject->setCurrentIndex(0);
            onShapeObjectActivated(0);
            setEnabled(false);
        }
    }
    else if (App::GetApplication().getActiveDocument() == &doc) {
        ui->shapeObject->setCurrentIndex(0);
        onShapeObjectActivated(0);
        setEnabled(false);
    }
}

void DlgFilletEdges::toggleCheckState(const QModelIndex& index)
{
    if (!d->object) {
        return;
    }
    QVariant check = index.data(Qt::CheckStateRole);
    int id = index.data(Qt::UserRole).toInt();
    QString name = QStringLiteral("Edge%1").arg(id);
    Qt::CheckState checkState = static_cast<Qt::CheckState>(check.toInt());

    bool block = this->blockSelection(true);

    // is item checked
    if (checkState & Qt::Checked) {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().addSelection(
            doc->getName(),
            d->object->getNameInDocument(),
            (const char*)name.toUtf8()
        );
    }
    else {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().rmvSelection(
            doc->getName(),
            d->object->getNameInDocument(),
            (const char*)name.toUtf8()
        );
    }

    this->blockSelection(block);
}

void DlgFilletEdges::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) {
        return;
    }

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType(
        Part::Feature::getClassTypeId()
    );
    int index = 1;
    int current_index = 0;
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end();
         ++it, ++index) {
        ui->shapeObject->addItem(QString::fromUtf8((*it)->Label.getValue()));
        ui->shapeObject->setItemData(index, QString::fromUtf8((*it)->getNameInDocument()));
        if (current_index == 0) {
            if (Gui::Selection().isSelected(*it)) {
                current_index = index;
            }
        }
    }

    // if only one object is in the document then simply use that
    if (objs.size() == 1) {
        current_index = 1;
    }

    if (current_index > 0) {
        ui->shapeObject->setCurrentIndex(current_index);
        onShapeObjectActivated(current_index);
    }

    // if an existing fillet object is given start the edit mode
    if (d->fillet) {
        setupFillet(objs);
    }
}

void DlgFilletEdges::setupFillet(const std::vector<App::DocumentObject*>& objs)
{
    App::DocumentObject* base = d->fillet->Base.getValue();
    const std::vector<Part::FilletElement>& e = d->fillet->Edges.getValues();
    const auto& subs = d->fillet->EdgeLinks.getShadowSubs();
    if (subs.size() != e.size()) {
        FC_ERR("edge link size mismatch");
        return;
    }
    std::set<std::string> subSet;
    for (auto& sub : subs) {
        subSet.insert(sub.newName.empty() ? sub.oldName : sub.newName);
    }

    if (auto it = std::ranges::find(objs, base); it != objs.end()) {
        // toggle visibility
        std::string tmp;
        Gui::ViewProvider* vp;
        vp = Gui::Application::Instance->getViewProvider(d->fillet);
        if (vp) {
            vp->hide();
        }
        vp = Gui::Application::Instance->getViewProvider(base);
        if (vp) {
            vp->show();
        }

        int current_index = (it - objs.begin()) + 1;
        ui->shapeObject->setCurrentIndex(current_index);
        onShapeObjectActivated(current_index);
        ui->shapeObject->setEnabled(false);

        double startRadius = 1;
        double endRadius = 1;
        bool twoRadii = false;

        std::vector<std::string> subElements;
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
        bool block = model->blockSignals(true);  // do not call toggleCheckState
        auto baseShape = Part::Feature::getTopoShape(
            base,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
        std::set<Part::FilletElement> elements;
        for (size_t i = 0; i < e.size(); ++i) {
            auto& sub = subs[i];
            if (sub.newName.empty()) {
                int idx = Data::indexOfElement(sub.oldName, "Edge");
                if (idx == 0) {
                    FC_WARN("missing element reference: " << sub.oldName);
                }
                else {
                    elements.insert(e[i]);
                }
                continue;
            }
            auto& ref = sub.newName;
            Part::TopoShape edge;
            try {
                edge = baseShape.getSubShape(ref.c_str());
            }
            catch (...) {
            }
            if (!edge.isNull()) {
                elements.insert(e[i]);
                continue;
            }
            FC_WARN("missing element reference: " << base->getFullName() << "." << ref);

            for (auto& mapped : Part::Feature::getRelatedElements(base, ref.c_str())) {
                tmp.clear();
                if (!subSet.insert(mapped.index.appendToStringBuffer(tmp)).second
                    || !subSet.insert(mapped.name.toString(0)).second) {
                    continue;
                }
                FC_WARN("guess element reference: " << ref << " -> " << mapped.index);
                elements.emplace(mapped.index.getIndex(), e[i].radius1, e[i].radius2);
            }
        }

        for (const auto& et : e) {
            auto it2 = std::ranges::find(d->edge_ids, et.edgeid);
            if (it2 != d->edge_ids.end()) {
                int index = it2 - d->edge_ids.begin();
                model->setData(model->index(index, 0), Qt::Checked, Qt::CheckStateRole);
                // model->setData(model->index(index, 1),
                // QVariant(QLocale().toString(et->radius1,'f',Base::UnitsApi::getDecimals())));
                // model->setData(model->index(index, 2),
                // QVariant(QLocale().toString(et->radius2,'f',Base::UnitsApi::getDecimals())));
                model->setData(
                    model->index(index, 1),
                    QVariant::fromValue<Base::Quantity>(Base::Quantity(et.radius1, Base::Unit::Length))
                );
                model->setData(
                    model->index(index, 2),
                    QVariant::fromValue<Base::Quantity>(Base::Quantity(et.radius2, Base::Unit::Length))
                );

                startRadius = et.radius1;
                endRadius = et.radius2;
                if (startRadius != endRadius) {
                    twoRadii = true;
                }

                int id = model->index(index, 0).data(Qt::UserRole).toInt();
                std::stringstream str;
                str << "Edge" << id;
                subElements.push_back(str.str());
            }
        }
        model->blockSignals(block);

        // #0002273
        if (twoRadii) {
            ui->filletType->setCurrentIndex(1);
            onFilletTypeActivated(1);
        }

        // #0001746
        ui->filletStartRadius->blockSignals(true);
        ui->filletStartRadius->setValue(startRadius);
        ui->filletStartRadius->blockSignals(false);
        ui->filletEndRadius->blockSignals(true);
        ui->filletEndRadius->setValue(endRadius);
        ui->filletEndRadius->blockSignals(false);

        App::Document* doc = d->object->getDocument();
        // get current selection and their sub-elements
        // std::vector<Gui::SelectionObject> selObj =
        // Gui::Selection().getSelectionEx(doc->getName());
        // std::vector<Gui::SelectionObject>::iterator selIt = std::find_if(selObj.begin(),
        // selObj.end(),
        //    Private::SelectionObjectCompare(d->object));


        /*
         * Edit: the following check is no longer necessary, as Gui::Selection
         * will do the check
         *
       // If sub-objects are already selected then only add the un-selected parts.
       // This is important to avoid recursive calls of rmvSelection() which
       // invalidates the internal iterator (#0002200).
       if (selIt != selObj.end()) {
           std::vector<std::string> selElements = selIt->getSubNames();
           std::sort(selElements.begin(), selElements.end());
           std::sort(subElements.begin(), subElements.end());

           std::vector<std::string> complementary;
           std::back_insert_iterator<std::vector<std::string> > biit(complementary);
           std::set_difference(subElements.begin(), subElements.end(), selElements.begin(),
       selElements.end(), biit); subElements = complementary;
       }
       */

        Gui::Selection().clearSelection(doc->getName());

        if (!subElements.empty()) {
            Gui::Selection().addSelections(doc->getName(), d->object->getNameInDocument(), subElements);
        }
    }
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgFilletEdges::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        int index = ui->shapeObject->currentIndex();
        // only get the items from index 1 on since the first one will be added automatically
        int count = ui->shapeObject->count() - 1;
        QStringList text;
        QList<QVariant> data;
        for (int i = 0; i < count; i++) {
            text << ui->shapeObject->itemText(i + 1);
            data << ui->shapeObject->itemData(i + 1);
        }

        ui->retranslateUi(this);
        for (int i = 0; i < count; i++) {
            ui->shapeObject->addItem(text.at(i));
            ui->shapeObject->setItemData(i + 1, data.at(i));
        }

        ui->shapeObject->setCurrentIndex(index);
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
        count = model->rowCount();
        for (int i = 0; i < count; i++) {
            int id = model->data(model->index(i, 0), Qt::UserRole).toInt();
            model->setData(model->index(i, 0), QVariant(tr("Edge%1").arg(id)));
        }
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgFilletEdges::onShapeObjectActivated(int itemPos)
{
    d->object = nullptr;
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
    model->removeRows(0, model->rowCount());

    QByteArray name = ui->shapeObject->itemData(itemPos).toByteArray();
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return;
    }
    App::DocumentObject* part = doc->getObject((const char*)name);
    if (part && part->isDerivedFrom<Part::Feature>()) {
        // An initially-null task may recover from removal of its source before
        // it has created a candidate. This explicit new selection starts a new
        // binding; it must not revive a retired candidate or document.
        if (!d->initialCandidate && !d->createdCandidate && !d->openedTransaction
            && (!d->editDocument || (!d->editDocument->expired() && **d->editDocument == doc))) {
            d->sourceRetired = false;
            d->candidateRetired = false;
            d->base = part;
            d->capturedVisibility = false;
            if (!d->editDocument) {
                d->editDocument = std::make_unique<App::DocumentWeakPtrT>(doc);
            }
        }
        d->object = part;
        TopoDS_Shape myShape = static_cast<Part::Feature*>(part)->Shape.getValue();

        d->all_edges.Clear();
        TopExp::MapShapes(myShape, TopAbs_EDGE, d->all_edges);

        d->all_faces.Clear();
        TopExp::MapShapes(myShape, TopAbs_FACE, d->all_faces);

        // build up map edge->face
        TopTools_IndexedDataMapOfShapeListOfShape edge2Face;
        TopExp::MapShapesAndAncestors(myShape, TopAbs_EDGE, TopAbs_FACE, edge2Face);
        TopTools_IndexedMapOfShape mapOfShape;
        TopExp::MapShapes(myShape, TopAbs_EDGE, mapOfShape);

        // populate the model
        d->edge_ids.clear();
        for (int i = 1; i <= edge2Face.Extent(); ++i) {
            // set the index value as user data to use it in accept()
            const TopTools_ListOfShape& los = edge2Face.FindFromIndex(i);
            if (los.Extent() == 2) {
                // set the index value as user data to use it in accept()
                const TopoDS_Shape& edge = edge2Face.FindKey(i);
                // Now check also the continuity to only allow C0-continious
                // faces
                const TopoDS_Shape& face1 = los.First();
                const TopoDS_Shape& face2 = los.Last();
                GeomAbs_Shape cont = BRep_Tool::Continuity(
                    TopoDS::Edge(edge),
                    TopoDS::Face(face1),
                    TopoDS::Face(face2)
                );
                if (cont == GeomAbs_C0) {
                    int id = mapOfShape.FindIndex(edge);
                    d->edge_ids.push_back(id);
                }
            }
        }

        model->insertRows(0, d->edge_ids.size());
        int index = 0;
        for (int id : d->edge_ids) {
            model->setData(model->index(index, 0), QVariant(tr("Edge%1").arg(id)));
            model->setData(model->index(index, 0), QVariant(id), Qt::UserRole);
            // model->setData(model->index(index, 1),
            // QVariant(QLocale().toString(1.0,'f',Base::UnitsApi::getDecimals())));
            // model->setData(model->index(index, 2),
            // QVariant(QLocale().toString(1.0,'f',Base::UnitsApi::getDecimals())));
            model->setData(
                model->index(index, 1),
                QVariant::fromValue<Base::Quantity>(Base::Quantity(1.0, Base::Unit::Length))
            );
            model->setData(
                model->index(index, 2),
                QVariant::fromValue<Base::Quantity>(Base::Quantity(1.0, Base::Unit::Length))
            );
            std::stringstream element;
            element << "Edge" << id;
            if (Gui::Selection().isSelected(part, element.str().c_str())) {
                model->setData(model->index(index, 0), Qt::Checked, Qt::CheckStateRole);
            }
            else {
                model->setData(model->index(index, 0), Qt::Unchecked, Qt::CheckStateRole);
            }
            index++;
        }
    }
}

void DlgFilletEdges::onSelectEdgesToggled(bool on)
{
    if (on) {
        d->selection->selectEdges();
    }
}

void DlgFilletEdges::onSelectFacesToggled(bool on)
{
    if (on) {
        d->selection->selectFaces();
    }
}

void DlgFilletEdges::onSelectAllButtonClicked()
{
    std::vector<std::string> subElements;
    FilletRadiusModel* model = static_cast<FilletRadiusModel*>(ui->treeView->model());
    bool block = model->blockSignals(true);  // do not call toggleCheckState
    for (int i = 0; i < model->rowCount(); ++i) {
        QModelIndex index = model->index(i, 0);

        // is not yet checked?
        QVariant check = index.data(Qt::CheckStateRole);
        Qt::CheckState state = static_cast<Qt::CheckState>(check.toInt());
        if (state == Qt::Unchecked) {
            int id = index.data(Qt::UserRole).toInt();
            std::stringstream str;
            str << "Edge" << id;
            subElements.push_back(str.str());
        }

        Qt::CheckState checkState = Qt::Checked;
        QVariant value(static_cast<int>(checkState));
        model->setData(index, value, Qt::CheckStateRole);
    }
    model->blockSignals(block);
    model->updateCheckStates();

    if (d->object) {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().addSelections(doc->getName(), d->object->getNameInDocument(), subElements);
    }
}

void DlgFilletEdges::onSelectNoneButtonClicked()
{
    FilletRadiusModel* model = static_cast<FilletRadiusModel*>(ui->treeView->model());
    bool block = model->blockSignals(true);  // do not call toggleCheckState
    for (int i = 0; i < model->rowCount(); ++i) {
        Qt::CheckState checkState = Qt::Unchecked;
        QVariant value(static_cast<int>(checkState));
        model->setData(model->index(i, 0), value, Qt::CheckStateRole);
    }
    model->blockSignals(block);
    model->updateCheckStates();

    if (d->object) {
        App::Document* doc = d->object->getDocument();
        Gui::Selection().clearSelection(doc->getName());
    }
}

void DlgFilletEdges::onFilletTypeActivated(int index)
{
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->treeView->model());
    if (index == 0) {
        if (d->filletType == DlgFilletEdges::CHAMFER) {
            model->setHeaderData(1, Qt::Horizontal, tr("Length"), Qt::DisplayRole);
        }
        else {
            model->setHeaderData(1, Qt::Horizontal, tr("Radius"), Qt::DisplayRole);
        }
        ui->treeView->hideColumn(2);
        ui->filletEndRadius->hide();
    }
    else {
        if (d->filletType == DlgFilletEdges::CHAMFER) {
            model->setHeaderData(1, Qt::Horizontal, tr("Start length"), Qt::DisplayRole);
        }
        else {
            model->setHeaderData(1, Qt::Horizontal, tr("Start radius"), Qt::DisplayRole);
        }
        ui->treeView->showColumn(2);
        ui->filletEndRadius->show();
    }

    ui->treeView->resizeColumnToContents(0);
    ui->treeView->resizeColumnToContents(1);
    ui->treeView->resizeColumnToContents(2);
}

void DlgFilletEdges::onFilletStartRadiusValueChanged(const Base::Quantity& radius)
{
    QAbstractItemModel* model = ui->treeView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        QVariant value = model->index(i, 0).data(Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

        // is item checked
        if (checkState & Qt::Checked) {
            model->setData(model->index(i, 1), QVariant::fromValue<Base::Quantity>(radius));
        }
    }
}

void DlgFilletEdges::onFilletEndRadiusValueChanged(const Base::Quantity& radius)
{
    QAbstractItemModel* model = ui->treeView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        QVariant value = model->index(i, 0).data(Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

        // is item checked
        if (checkState & Qt::Checked) {
            model->setData(model->index(i, 2), QVariant::fromValue<Base::Quantity>(radius));
        }
    }
}

const char* DlgFilletEdges::getFilletType() const
{
    return "Fillet";
}

bool DlgFilletEdges::accept()
{
    if (!d->object || d->sourceRetired || d->candidateRetired) {
        QMessageBox::warning(
            this,
            tr("No shape selected"),
            tr("No valid shape is selected.\n"
               "Select a valid shape in the drop-down box first.")
        );
        return false;
    }
    QPointer<DlgFilletEdges> guard(this);
    App::Document* activeDoc = d->object->getDocument();
    if (!activeDoc || (d->editDocument && **d->editDocument != activeDoc)
        || (d->initialCandidate && d->candidate.expired())) {
        return false;
    }
    if (!d->editDocument) {
        d->editDocument = std::make_unique<App::DocumentWeakPtrT>(activeDoc);
    }
    App::DocumentWeakPtrT documentGuard(activeDoc);
    QAbstractItemModel* model = ui->treeView->model();
    bool end_radius = !ui->treeView->isColumnHidden(2);
    bool todo = false;

    QString shape, type, name;
    std::string fillet = getFilletType();
    shape = QString::fromUtf8(d->object->getNameInDocument());
    type = QStringLiteral("Part::%1").arg(QString::fromUtf8(fillet.c_str()));

    if (d->fillet) {
        name = QString::fromUtf8(d->fillet->getNameInDocument());
    }
    else {
        name = QString::fromStdString(activeDoc->getUniqueObjectName(fillet.c_str()));
    }

    const QString documentCommand = QString::fromStdString(
        std::string("App.getDocument('") + activeDoc->getName() + "')"
    );
    const auto expectedType = d->filletType == FILLET ? Part::Fillet::getClassTypeId()
                                                      : Part::Chamfer::getClassTypeId();
    QString code = QStringLiteral("__fillets__ = []\n");
    for (int i = 0; i < model->rowCount(); ++i) {
        QVariant value = model->index(i, 0).data(Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());

        // is item checked
        if (checkState & Qt::Checked) {
            // the index value of the edge
            int id = model->index(i, 0).data(Qt::UserRole).toInt();
            Base::Quantity r1 = model->index(i, 1).data(Qt::EditRole).value<Base::Quantity>();
            Base::Quantity r2 = r1;
            if (end_radius) {
                r2 = model->index(i, 2).data(Qt::EditRole).value<Base::Quantity>();
            }
            code += QStringLiteral("__fillets__.append((%1,%2,%3))\n")
                        .arg(id)
                        .arg(r1.getValue(), 0, 'f', Base::UnitsApi::getDecimals())
                        .arg(r2.getValue(), 0, 'f', Base::UnitsApi::getDecimals());
            todo = true;
        }
    }

    if (!todo) {
        QMessageBox::warning(
            this,
            tr("No edge selected"),
            tr("No edge entity is checked to fillet.\n"
               "Check one or more edge entities first.")
        );
        return false;
    }

    code += QStringLiteral(
                "%2.%1.Edges = __fillets__\n"
                "del __fillets__\n"
    )
                .arg(name, documentCommand);

    const QString operationLabel = QString::fromStdString(fillet);
    auto& application = App::GetApplication();
    const auto transactionMatches = [&]() {
        return !documentGuard.expired() && d->transactionId != 0
            && activeDoc->getBookedTransactionID() == d->transactionId
            && application.transactionIsActive(d->transactionId)
            && (application.getGlobalTransaction() == 0
                || application.getGlobalTransaction() == d->transactionId);
    };
    const auto restoreBase = [&]() {
        if (auto* source = d->base.get<App::DocumentObject>()) {
            if (auto* view = Gui::Application::Instance->getViewProvider(source)) {
                view->show();
            }
        }
    };
    const auto closeTrackedTransaction = [&](bool abort) {
        if (!transactionMatches()) {
            return false;
        }
        const int id = d->transactionId;
        const bool closed = abort ? application.abortTransaction(id)
                                  : application.commitTransaction(id);
        if (!guard || documentGuard.expired()) {
            return closed;
        }
        if (!closed || application.transactionIsActive(id)
            || activeDoc->getBookedTransactionID() == id || d->transactionId != id) {
            Base::Console().error(
                "The fillet transaction could not be closed; the edit remains open.\n"
            );
            return false;
        }
        d->transactionId = 0;
        d->openedTransaction = false;
        return true;
    };
    try {
        if (d->transactionId == 0) {
            // An unrelated edit that appeared after this task opened is not ours.
            if (activeDoc->getBookedTransactionID() != 0 || activeDoc->hasPendingTransaction()
                || application.getGlobalTransaction() != 0) {
                Base::Console().error(
                    "Another transaction is active; finish it before applying the fillet.\n"
                );
                return false;
            }
            const int openedId = activeDoc->openTransaction(fillet.c_str());
            if (!guard || documentGuard.expired() || d->candidateRetired || d->sourceRetired) {
                return false;
            }
            d->transactionId = openedId;
            d->openedTransaction = openedId != 0;
        }
        if (!transactionMatches()) {
            Base::Console().error("The fillet edit transaction is no longer active.\n");
            return false;
        }
        if (!d->capturedVisibility) {
            d->base = d->object;
            if (auto* view = Gui::Application::Instance->getViewProvider(d->object)) {
                d->baseWasVisible = view->isShow();
            }
            d->capturedVisibility = true;
        }
        const auto candidateMatches = [&](bool allowUnboundBase = false) {
            if (!guard || documentGuard.expired() || d->candidateRetired || d->sourceRetired
                || d->candidate.expired() || d->base.expired()) {
                return false;
            }
            auto* candidate = d->candidate.get<Part::FilletBase>();
            auto* source = d->base.get<App::DocumentObject>();
            return candidate && candidate == d->fillet && candidate->getTypeId() == expectedType
                && candidate->getDocument() == activeDoc
                && activeDoc->getObject(name.toUtf8().constData()) == candidate
                && (candidate->Base.getValue() == source
                    || (allowUnboundBase && !candidate->Base.getValue()));
        };
        if (!d->fillet) {
            d->creationPending = true;
            bool captured = false;
            fastsignals::scoped_connection createdConnection = activeDoc->signalNewObject.connect(
                [&, guard](const App::DocumentObject& object) {
                    if (!guard || documentGuard.expired() || d->candidateRetired || d->sourceRetired) {
                        return;
                    }
                    // A nested or additional creation cannot establish the origin
                    // of this attempt. Refuse it instead of adopting by name.
                    if (captured || object.getTypeId() != expectedType
                        || name != QString::fromUtf8(object.getNameInDocument())) {
                        d->candidateRetired = true;
                        return;
                    }
                    auto* candidate = freecad_cast<Part::FilletBase*>(
                        const_cast<App::DocumentObject*>(&object)
                    );
                    if (!candidate
                        || (candidate->Base.getValue()
                            && candidate->Base.getValue() != d->base.get<App::DocumentObject>())) {
                        d->candidateRetired = true;
                        return;
                    }
                    captured = true;
                    d->fillet = candidate;
                    d->candidate = candidate;
                    d->createdCandidate = true;
                }
            );
            const QString creationCode
                = QStringLiteral("%3.addObject(\"%1\",\"%2\")\n").arg(type, name, documentCommand);
            Gui::Command::runCommand(Gui::Command::App, creationCode.toUtf8());
            if (!guard) {
                return false;
            }
            d->creationPending = false;
            if (!captured || !candidateMatches(true) || !transactionMatches()) {
                return false;
            }
        }
        // A command exception may leave the exact captured new candidate with
        // no Base. A retry can complete it without creating or adopting another.
        if (d->createdCandidate && !d->initialCandidate) {
            if (!candidateMatches(true) || !transactionMatches()) {
                return false;
            }
            if (!d->fillet->Base.getValue()) {
                const QString baseCode
                    = QStringLiteral("%3.%1.Base = %3.%2\n").arg(name, shape, documentCommand);
                Gui::Command::runCommand(Gui::Command::App, baseCode.toUtf8());
            }
        }
        if (!candidateMatches() || !transactionMatches()) {
            return false;
        }
        ui->shapeObject->setEnabled(false);
        {
            Gui::WaitCursor wc;
            if (!candidateMatches() || !transactionMatches()) {
                return false;
            }
            Gui::Command::runCommand(Gui::Command::App, code.toUtf8());
        }
        if (!candidateMatches() || !transactionMatches()) {
            return false;
        }
        auto* resultObject = d->candidate.get<Part::FilletBase>();
        App::DocumentObjectWeakPtrT resultGuard(resultObject);
        const auto outcome = Gui::runAsyncDocumentObjectRecomputeProgressDialog(
            this,
            operationLabel,
            tr("Computing %1...").arg(operationLabel.toLower()),
            resultObject,
            /*recursive=*/true,
            [resultObject]() { return Gui::recomputeDocumentObjectWithResult(resultObject, true); }
        );
        if (!candidateMatches() || resultGuard.expired()) {
            return false;
        }
        if (!outcome.success) {
            // Model errors remain editable. A borrowed creation/edit transaction
            // must also survive progress cancellation or an execution exception.
            restoreBase();
            if (!guard || documentGuard.expired()) {
                return false;
            }
            if (outcome.failure != App::RecomputeFailure::ModelError && d->openedTransaction
                && transactionMatches()) {
                if (!closeTrackedTransaction(true) || !guard || documentGuard.expired()) {
                    return false;
                }
                if (!d->initialCandidate) {
                    d->candidate.reset();
                    d->fillet = nullptr;
                    d->candidateRetired = false;
                    d->createdCandidate = false;
                    ui->shapeObject->setEnabled(true);
                }
            }
            if (!outcome.canceled) {
                Base::Console().error(
                    "%s\n",
                    outcome.message.empty() ? "Fillet or chamfer recompute failed"
                                            : outcome.message.c_str()
                );
            }
            return false;
        }
        if (!transactionMatches()) {
            return false;
        }
        if (!resultObject->isValid()) {
            const std::string status = resultObject->getStatusString();
            restoreBase();
            if (!candidateMatches() || resultGuard.expired()) {
                return false;
            }
            Base::Console().error("%s\n", status.c_str());
            return false;
        }
        if (auto* view = Gui::Application::Instance->getViewProvider(d->object)) {
            view->hide();
        }
        if (!candidateMatches() || resultGuard.expired()) {
            return false;
        }
        if (auto* view = Gui::Application::Instance->getViewProvider(resultObject)) {
            view->show();
        }
        if (!candidateMatches() || resultGuard.expired()) {
            return false;
        }
        Gui::Command::copyVisual(resultObject, "LineColor", d->object);
        if (!candidateMatches() || resultGuard.expired()) {
            return false;
        }
        Gui::Command::copyVisual(resultObject, "PointColor", d->object);
        if (!candidateMatches() || !transactionMatches() || resultGuard.expired()) {
            return false;
        }
        if (!closeTrackedTransaction(false)) {
            if (guard && !documentGuard.expired()) {
                restoreBase();
            }
            return false;
        }
        if (!guard || documentGuard.expired()) {
            return true;
        }
        if (activeDoc->getBookedTransactionID() != 0 || activeDoc->hasPendingTransaction()
            || application.getGlobalTransaction() != 0) {
            return false;
        }
    }
    catch (const Base::Exception& e) {
        if (!guard) {
            return false;
        }
        // Only the creation observer may bind a candidate. A partial command
        // retains that exact object on a borrowed transaction; never look it up
        // by name after an exception.
        d->creationPending = false;
        restoreBase();
        if (!guard || documentGuard.expired()) {
            return false;
        }
        if (d->openedTransaction && transactionMatches()) {
            if (!closeTrackedTransaction(true)) {
                e.reportException();
                return false;
            }
            if (guard && !documentGuard.expired()) {
                if (!d->initialCandidate) {
                    d->candidate.reset();
                    d->fillet = nullptr;
                    d->candidateRetired = false;
                    d->createdCandidate = false;
                    ui->shapeObject->setEnabled(true);
                }
            }
        }
        e.reportException();
        return false;
    }

    return true;
}

bool DlgFilletEdges::reject()
{
    QPointer<DlgFilletEdges> guard(this);
    auto* document = d->editDocument ? **d->editDocument : nullptr;
    App::DocumentWeakPtrT documentGuard(document);
    if (document && d->transactionId != 0) {
        const int global = App::GetApplication().getGlobalTransaction();
        const bool matches = document->getBookedTransactionID() == d->transactionId
            && (global == 0 || global == d->transactionId);
        if (!matches && (document->hasPendingTransaction() || global != 0)) {
            Base::Console().error("The fillet transaction changed; no unrelated edit was discarded.\n");
            return false;
        }
        if (matches) {
            const int id = d->transactionId;
            const bool closed = App::GetApplication().abortTransaction(id);
            if (!guard || documentGuard.expired()) {
                return closed;
            }
            if (!closed || App::GetApplication().transactionIsActive(id)
                || document->getBookedTransactionID() == id || d->transactionId != id) {
                Base::Console().error(
                    "The fillet transaction could not be discarded; the edit remains open.\n"
                );
                return false;
            }
        }
    }
    else if (
        App::GetApplication().getGlobalTransaction() != 0
        || (document && (document->getBookedTransactionID() != 0 || document->hasPendingTransaction()))
    ) {
        return false;
    }
    d->transactionId = 0;
    d->openedTransaction = false;
    if ((document && (document->getBookedTransactionID() != 0 || document->hasPendingTransaction()))
        || App::GetApplication().getGlobalTransaction() != 0) {
        return false;
    }
    if (auto* source = d->base.get<App::DocumentObject>()) {
        if (auto* view = Gui::Application::Instance->getViewProvider(source)) {
            view->setVisible(d->baseWasVisible);
        }
    }
    if (!guard || (document && documentGuard.expired())) {
        return true;
    }
    if (auto* candidate = d->candidate.get<App::DocumentObject>()) {
        if (auto* view = Gui::Application::Instance->getViewProvider(candidate)) {
            view->setVisible(d->candidateWasVisible);
        }
    }
    return true;
}

bool DlgFilletEdges::finishEditing()
{
    auto* document = d->editDocument ? **d->editDocument : nullptr;
    if (!document) {
        return true;
    }
    // resetEdit commits a booked transaction. Never let it finalize a newer edit.
    if (App::GetApplication().getGlobalTransaction() != 0 || document->getBookedTransactionID() != 0
        || document->hasPendingTransaction()) {
        return false;
    }
    auto* guiDocument = Gui::Application::Instance->getDocument(document);
    if (!guiDocument || !guiDocument->getInEdit()) {
        return true;
    }
    auto* candidate = d->candidate.get<App::DocumentObject>();
    if (!candidate
        || guiDocument->getInEdit() != Gui::Application::Instance->getViewProvider(candidate)) {
        return false;
    }
    guiDocument->resetEdit();
    return true;
}

void DlgFilletEdges::setSelectionGate()
{
    d->selection = new EdgeFaceSelection(d->object);
    Gui::Selection().addSelectionGate(d->selection);
}

// ---------------------------------------

FilletEdgesDialog::FilletEdgesDialog(
    DlgFilletEdges::FilletType type,
    Part::FilletBase* fillet,
    QWidget* parent,
    Qt::WindowFlags fl
)
    : QDialog(parent, fl)
{
    widget = new DlgFilletEdges(type, fillet, this);
    this->setWindowTitle(widget->windowTitle());

    QVBoxLayout* hboxLayout = new QVBoxLayout(this);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &FilletEdgesDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &FilletEdgesDialog::reject);

    hboxLayout->addWidget(widget);
    hboxLayout->addWidget(buttonBox);
}

FilletEdgesDialog::~FilletEdgesDialog() = default;

void FilletEdgesDialog::accept()
{
    QPointer<FilletEdgesDialog> guard(this);
    QPointer<DlgFilletEdges> widgetGuard(widget);
    if (widget->accept() && guard && widgetGuard && widgetGuard->finishEditing() && guard) {
        QDialog::accept();
    }
}

void FilletEdgesDialog::reject()
{
    QPointer<FilletEdgesDialog> guard(this);
    QPointer<DlgFilletEdges> widgetGuard(widget);
    if (widget->reject() && guard && (!widgetGuard || widgetGuard->finishEditing()) && guard) {
        QDialog::reject();
    }
}

// ---------------------------------------

TaskFilletEdges::TaskFilletEdges(Part::Fillet* fillet)
{
    widget = new DlgFilletEdges(DlgFilletEdges::FILLET, fillet);
    addTaskBox(Gui::BitmapFactory().pixmap("Part_Fillet"), widget);
}

TaskFilletEdges::~TaskFilletEdges()
{
    // automatically deleted in the sub-class
}

void TaskFilletEdges::open()
{}

void TaskFilletEdges::clicked(int)
{}

bool TaskFilletEdges::accept()
{
    QPointer<TaskFilletEdges> guard(this);
    QPointer<DlgFilletEdges> widgetGuard(widget);
    const bool ok = widget->accept();
    return ok && (!guard || !widgetGuard || widgetGuard->finishEditing());
}

bool TaskFilletEdges::reject()
{
    QPointer<TaskFilletEdges> guard(this);
    QPointer<DlgFilletEdges> widgetGuard(widget);
    if (!widget->reject()) {
        return false;
    }
    return !guard || !widgetGuard || widgetGuard->finishEditing();
}

// --------------------------------------------------------------

/* TRANSLATOR PartGui::DlgChamferEdges */

DlgChamferEdges::DlgChamferEdges(Part::FilletBase* chamfer, QWidget* parent, Qt::WindowFlags fl)
    : DlgFilletEdges(DlgFilletEdges::CHAMFER, chamfer, parent, fl)
{
    this->setWindowTitle(tr("Chamfer Edges"));
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgChamferEdges::~DlgChamferEdges() = default;

const char* DlgChamferEdges::getFilletType() const
{
    return "Chamfer";
}

TaskChamferEdges::TaskChamferEdges(Part::Chamfer* chamfer)
{
    widget = new DlgChamferEdges(chamfer);
    addTaskBox(Gui::BitmapFactory().pixmap("Part_Chamfer"), widget);
}

TaskChamferEdges::~TaskChamferEdges()
{
    // automatically deleted in the sub-class
}

void TaskChamferEdges::open()
{}

void TaskChamferEdges::clicked(int)
{}

bool TaskChamferEdges::accept()
{
    QPointer<TaskChamferEdges> guard(this);
    QPointer<DlgFilletEdges> widgetGuard(widget);
    const bool ok = widget->accept();
    return ok && (!guard || !widgetGuard || widgetGuard->finishEditing());
}

bool TaskChamferEdges::reject()
{
    QPointer<TaskChamferEdges> guard(this);
    QPointer<DlgFilletEdges> widgetGuard(widget);
    if (!widget->reject()) {
        return false;
    }
    return !guard || !widgetGuard || widgetGuard->finishEditing();
}

#include "moc_DlgFilletEdges.cpp"
