// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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
#include <boost/graph/topological_sort.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost_graph_adjacency_list.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Writer.h>
#include <CXX/Objects.hxx>

#include "PropertyExpressionEngine.h"
#include "ExpressionVisitors.h"
#include "private/RecomputeAdmission.h"


FC_LOG_LEVEL_INIT("App", true);

using namespace App;
using namespace Base;
using namespace boost;
namespace sp = std::placeholders;

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyExpressionContainer, App::PropertyXLinkContainer)

namespace
{
struct ExpressionAdmissionRecord
{
    std::set<RecomputeDetail::Generation> documents;
    std::set<const DocumentObject*> objects;
};
std::map<PropertyExpressionContainer*, ExpressionAdmissionRecord> expressionContainers;
}  // namespace

PropertyExpressionContainer::PropertyExpressionContainer()
{
    static const bool connected = [] {
        GetApplication().signalRelabelDocument.connect(
            PropertyExpressionContainer::slotRelabelDocument
        );
        GetApplication().signalMoveDynamicProperty.connect(
            PropertyExpressionContainer::slotMoveDynamicProperty
        );
        return true;
    }();
    (void)connected;
    RecomputeDetail::IndexLock lock;
    expressionContainers.emplace(this, ExpressionAdmissionRecord());
}

PropertyExpressionContainer::~PropertyExpressionContainer()
{
    RecomputeDetail::IndexLock lock;
    expressionContainers.erase(this);
}

void PropertyExpressionContainer::publishAdmissionTargets(const std::vector<DocumentObject*>& targets)
{
    ExpressionAdmissionRecord record;
    // Registry-only identity lookup: publication adds no admission or native read.
    // The caller supplies dependencies already computed by its graph update.
    for (auto* target : targets) {
        if (target) {
            record.objects.insert(target);
            const auto binding = RecomputeDetail::Coordinator::instance().bindObject(target);
            if (binding->documentGeneration) {
                record.documents.insert(binding->documentGeneration);
            }
        }
    }
    RecomputeDetail::IndexLock lock;
    expressionContainers[this] = std::move(record);
}

void PropertyExpressionContainer::copyAdmissionMetadata(const PropertyExpressionContainer& source)
{
    RecomputeDetail::IndexLock lock;
    expressionContainers[this] = expressionContainers.at(
        const_cast<PropertyExpressionContainer*>(&source)
    );
}

std::vector<DocumentObject*> PropertyExpressionContainer::admissionMetadataTargets() const
{
    RecomputeDetail::IndexLock lock;
    const auto& record = expressionContainers.at(const_cast<PropertyExpressionContainer*>(this));
    std::vector<DocumentObject*> result;
    for (auto* target : record.objects) {
        result.push_back(const_cast<DocumentObject*>(target));
    }
    return result;
}

void PropertyExpressionContainer::collectAdmissionTargets(std::vector<DocumentObject*>& targets) const
{
    PropertyXLinkContainer::collectAdmissionTargets(targets);
    const auto indexed = admissionMetadataTargets();
    targets.insert(targets.end(), indexed.begin(), indexed.end());
}

RecomputeDetail::PropertySnapshot PropertyExpressionContainer::snapshotForAdmission(
    const Document* document,
    const DocumentObject* object
)
{
    std::vector<Property*> interested;
    const auto binding = document ? RecomputeDetail::Coordinator::instance().bindDocument(document)
                                  : RecomputeDetail::BindingPtr {};
    {
        RecomputeDetail::IndexLock lock;
        for (const auto& entry : expressionContainers) {
            if ((document && entry.second.documents.contains(binding->documentGeneration))
                || (object && entry.second.objects.contains(object))) {
                interested.push_back(entry.first);
            }
        }
    }
    return RecomputeDetail::Coordinator::instance().snapshotProperties(interested);
}

void PropertyExpressionContainer::slotRelabelDocument(const App::Document& doc)
{
    if (doc.getOldLabel() != doc.Label.getValue()) {
        auto snapshot = snapshotForAdmission(&doc);
        for (auto* property : snapshot.properties()) {
            static_cast<PropertyExpressionContainer*>(property)->onRelabeledDocument(doc);
        }
    }
}

void PropertyExpressionContainer::handleDynamicPropertyRename(
    const App::Property& prop,
    const char* oldName
)
{
    auto snapshot
        = snapshotForAdmission(nullptr, dynamic_cast<const DocumentObject*>(prop.getContainer()));
    for (auto* property : snapshot.properties()) {
        static_cast<PropertyExpressionContainer*>(property)->onRenameDynamicProperty(prop, oldName);
    }
}

void PropertyExpressionContainer::slotMoveDynamicProperty(
    const App::Property& prop,
    const App::DocumentObject& targetObj
)
{
    auto snapshot
        = snapshotForAdmission(nullptr, dynamic_cast<const DocumentObject*>(prop.getContainer()));
    for (auto* property : snapshot.properties()) {
        static_cast<PropertyExpressionContainer*>(property)->onMoveDynamicProperty(prop, targetObj);
    }
}

///////////////////////////////////////////////////////////////////////////////////////

/* The cycle_detector struct is used by the boost graph routines to detect
 * cycles in the graph. */
struct cycle_detector: public boost::dfs_visitor<>
{
    cycle_detector(bool& has_cycle, int& src)
        : _has_cycle(has_cycle)
        , _src(src)
    {}

    template<class Edge, class Graph>
    void back_edge(Edge e, Graph& g)
    {
        _has_cycle = true;
        _src = source(e, g);
    }

protected:
    bool& _has_cycle;
    int& _src;
};

using DiGraph = boost::adjacency_list<boost::listS, boost::vecS, boost::directedS>;
struct PropertyExpressionEngine::Private
{
    // For some reason, MSVC has trouble with vector of scoped_connection if
    // defined in header, hence the private structure here.
    std::vector<fastsignals::scoped_connection> conns;
    std::unordered_map<std::string, std::vector<ObjectIdentifier>> propMap;

    /**
     * @brief Build a graph of all expressions in \a exprs.
     * @param exprs Expressions to use in graph
     * @param revNodes Map from int[nodeid] to ObjectIndentifer.
     * @param g Graph to update. May contain additional nodes than in revNodes, because of outside
     * dependencies.
     */
    static void buildGraph(
        const ExpressionMap& exprs,
        boost::unordered_map<int, App::ObjectIdentifier>& revNodes,
        DiGraph& g,
        ExecuteOption option = ExecuteAll
    )
    {
        boost::unordered_map<ObjectIdentifier, int> nodes;
        std::vector<Edge> edges;

        // Build data structure for graph
        for (const auto& expr : exprs) {
            if (option != ExecuteAll) {
                auto prop = expr.first.getProperty();
                if (!prop) {
                    throw Base::RuntimeError("Path does not resolve to a property.");
                }
                bool is_output = prop->testStatus(App::Property::Output)
                    || (prop->getType() & App::Prop_Output);
                if ((is_output && option == ExecuteNonOutput)
                    || (!is_output && option == ExecuteOutput)) {
                    continue;
                }
                if (option == ExecuteOnRestore && !prop->testStatus(Property::Transient)
                    && !(prop->getType() & Prop_Transient)
                    && !prop->testStatus(Property::EvalOnRestore)) {
                    continue;
                }
            }
            buildGraphStructures(expr.first, expr.second.expression, nodes, revNodes, edges);
        }

        // Create graph.
        g.clear();
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            add_vertex(g);
        }

        // Add edges to graph
        for (const auto& edge : edges) {
            add_edge(edge.first, edge.second, g);
        }

        // Check for cycles
        bool has_cycle = false;
        int src = -1;
        cycle_detector vis(has_cycle, src);
        depth_first_search(g, visitor(vis));

        if (has_cycle) {
            std::string s = revNodes[src].toString() + " reference creates a cyclic dependency.";

            throw Base::RuntimeError(s.c_str());
        }
    }

    static void buildGraphStructures(
        const ObjectIdentifier& path,
        const std::shared_ptr<Expression> expression,
        boost::unordered_map<ObjectIdentifier, int>& nodes,
        boost::unordered_map<int, ObjectIdentifier>& revNodes,
        std::vector<Edge>& edges
    )
    {
        /* Insert target property into nodes structure */
        if (nodes.find(path) == nodes.end()) {
            int s = nodes.size();

            revNodes[s] = path;
            nodes[path] = s;
        }
        else {
            revNodes[nodes[path]] = path;
        }

        /* Insert dependencies into nodes structure */
        ExpressionDeps deps;
        if (expression) {
            deps = expression->getDeps();
        }

        for (auto& dep : deps) {
            for (auto& info : dep.second) {
                if (info.first.empty()) {
                    continue;
                }
                for (auto& oid : info.second) {
                    ObjectIdentifier cPath(oid.canonicalPath());
                    if (nodes.find(cPath) == nodes.end()) {
                        int s = nodes.size();
                        nodes[cPath] = s;
                    }
                    edges.emplace_back(nodes[path], nodes[cPath]);
                }
            }
        }
    }
};

///////////////////////////////////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE(App::PropertyExpressionEngine, App::PropertyExpressionContainer)


PropertyExpressionEngine::PropertyExpressionEngine()
    : validator(0)
{}

PropertyExpressionEngine::~PropertyExpressionEngine() = default;

// fixme Should probably return something else than 0.
unsigned int PropertyExpressionEngine::getMemSize() const
{
    return 0;
}

Property* PropertyExpressionEngine::Copy() const
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    PropertyExpressionEngine* engine = new PropertyExpressionEngine();

    for (const auto& it : expressions) {
        ExpressionInfo info;
        if (it.second.expression) {
            info.expression = std::shared_ptr<Expression>(it.second.expression->copy());
        }
        engine->expressions[it.first] = info;
    }

    engine->validator = validator;
    mutation.include(admissionTargets(engine->expressions));
    engine->refreshAdmissionMetadata();

    return engine;
}

PropertyExpressionEngine::ExpressionMap PropertyExpressionEngine::copyExpressions() const
{
    ExpressionMap result;
    for (const auto& entry : expressions) {
        result.emplace(
            entry.first,
            ExpressionInfo(
                entry.second.expression ? std::shared_ptr<Expression>(entry.second.expression->copy())
                                        : nullptr
            )
        );
    }
    return result;
}

std::vector<DocumentObject*> PropertyExpressionEngine::admissionTargets(const ExpressionMap& values)
{
    std::vector<DocumentObject*> targets;
    for (const auto& entry : values) {
        targets.push_back(entry.first.getOwner());
        if (auto* owner = entry.first.getDocumentObject()) {
            targets.push_back(owner);
        }
        if (entry.second.expression) {
            targets.push_back(entry.second.expression->getOwner());
            std::map<DocumentObject*, bool> dependencies;
            std::map<std::pair<std::string, DocumentObject*>, bool> properties;
            entry.second.expression->getDepObjects(dependencies, nullptr, &properties);
            for (const auto& dependency : dependencies) {
                targets.push_back(dependency.first);
            }
            for (const auto& property : properties) {
                targets.push_back(property.first.second);
            }
        }
    }
    return targets;
}

void PropertyExpressionEngine::commitExpressions(ExpressionMap&& proposed, bool notify)
{
    commitExpressions(std::move(proposed), notify, {});
}

void PropertyExpressionEngine::commitExpressions(
    ExpressionMap&& proposed,
    bool notify,
    const std::set<ObjectIdentifier>& changedPaths
)
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    auto targets = admissionTargets(expressions);
    auto next = admissionTargets(proposed);
    targets.insert(targets.end(), next.begin(), next.end());
    mutation.include(targets);
    if (!notify) {
        expressions = std::move(proposed);
        refreshAdmissionMetadata();
        return;
    }
    std::set<ObjectIdentifier> paths = changedPaths;
    for (const auto& entry : expressions) {
        if (!proposed.contains(entry.first)) {
            paths.insert(entry.first);
        }
    }
    for (const auto& entry : proposed) {
        auto old = expressions.find(entry.first);
        if (old == expressions.end()) {
            paths.insert(entry.first);
            continue;
        }
        const auto& previous = old->second.expression;
        const auto& nextExpression = entry.second.expression;
        if (previous != nextExpression
            && (!previous || !nextExpression || !previous->isSame(*nextExpression))) {
            paths.insert(entry.first);
        }
    }
    if (paths.empty()) {
        return;
    }
    preflightAdmissionStorage();
    AtomicPropertyChange signaller(*this);
    expressions = std::move(proposed);
    for (const auto& path : paths) {
        expressionChanged(path);
    }
    signaller.tryInvoke();
}

void PropertyExpressionEngine::collectAdmissionTargets(std::vector<DocumentObject*>& targets) const
{
    PropertyExpressionContainer::collectAdmissionTargets(targets);
    const auto* owner = freecad_cast<DocumentObject*>(getContainer());
    if (!admissionStaging && (!owner || !owner->isAttachedToDocument())) {
        // Copy factories publish their committed footprint. A history-owned
        // model also retains its embedded engine after detachment, while its
        // native name is null. Both consume only the indexed identities.
        return;
    }
    // Live receivers and unpublished staging engines still discover their
    // current/prospective AST footprint under their caller's native scope.
    auto own = admissionTargets(expressions);
    targets.insert(targets.end(), own.begin(), own.end());
}

void PropertyExpressionEngine::refreshAdmissionMetadata()
{
    if (admissionStaging) {
        return;
    }
    publishAdmissionTargets(admissionTargets(expressions));
}

void PropertyExpressionEngine::hasSetValue()
{
    refreshAdmissionMetadata();
    auto* owner = freecad_cast<App::DocumentObject*>(getContainer());
    if (!owner || !owner->isAttachedToDocument() || owner->isRestoring() || testFlag(LinkDetached)) {
        PropertyExpressionContainer::hasSetValue();
        return;
    }

    std::map<App::DocumentObject*, bool> deps;
    std::map<std::pair<std::string, App::DocumentObject*>, bool> propDeps;
    std::vector<std::string> labels;
    unregisterElementReference();
    UpdateElementReferenceExpressionVisitor<PropertyExpressionEngine> v(*this);
    for (auto& e : expressions) {
        auto expr = e.second.expression;
        if (expr) {
            expr->getDepObjects(deps, &labels, &propDeps);
            if (!restoring) {
                expr->visit(v);
            }
        }
    }
    registerLabelReferences(std::move(labels));

    updateDeps(std::move(deps), &propDeps);

    if (pimpl) {
        pimpl->conns.clear();
        pimpl->propMap.clear();
    }
    // check if there is any hidden references
    bool hasHidden = false;
    for (auto& v : _Deps) {
        if (v.second) {
            hasHidden = true;
            break;
        }
    }
    if (hasHidden) {
        if (!pimpl) {
            pimpl = std::make_unique<Private>();
        }
        for (auto& e : expressions) {
            auto expr = e.second.expression;
            if (!expr) {
                continue;
            }
            for (auto& dep : expr->getIdentifiers()) {
                if (!dep.second) {
                    continue;
                }
                const ObjectIdentifier& var = dep.first;
                for (auto& vdep : var.getDep(true)) {
                    auto obj = vdep.first;
                    auto objName = obj->getFullName() + ".";
                    for (auto& propName : vdep.second) {
                        std::string key = objName + propName;
                        auto& propDeps = pimpl->propMap[key];
                        if (propDeps.empty()) {
                            // NOLINTBEGIN
                            if (!propName.empty()) {
                                pimpl->conns.emplace_back(obj->signalChanged.connect(
                                    std::bind(
                                        &PropertyExpressionEngine::slotChangedProperty,
                                        this,
                                        sp::_1,
                                        sp::_2
                                    )
                                ));
                            }
                            else {
                                pimpl->conns.emplace_back(obj->signalChanged.connect(
                                    std::bind(
                                        &PropertyExpressionEngine::slotChangedObject,
                                        this,
                                        sp::_1,
                                        sp::_2
                                    )
                                ));
                            }
                            // NOLINTEND
                        }
                        propDeps.push_back(e.first);
                    }
                }
            }
        }
    }

    PropertyExpressionContainer::hasSetValue();
}

void PropertyExpressionEngine::updateHiddenReference(const std::string& key)
{
    if (!pimpl) {
        return;
    }
    auto it = pimpl->propMap.find(key);
    if (it == pimpl->propMap.end()) {
        return;
    }
    for (auto& var : it->second) {
        auto it = expressions.find(var);
        if (it == expressions.end() || it->second.busy) {
            continue;
        }
        Property* myProp = var.getProperty();
        if (!myProp) {
            continue;
        }
        Base::StateLocker guard(it->second.busy);
        App::any value;
        try {
            value = it->second.expression->getValueAsAny();
            if (!isAnyEqual(value, myProp->getPathValue(var))) {
                myProp->setPathValue(var, value);
            }
        }
        catch (const RecomputeDetail::AdmissionRefusal&) {
            throw;
        }
        catch (Base::Exception& e) {
            e.reportException();
            FC_ERR(
                "Failed to evaluate property binding " << myProp->getFullName() << " on change of "
                                                       << key
            );
        }
        catch (std::bad_cast&) {
            FC_ERR(
                "Invalid type '" << value.type().name() << "' in property binding "
                                 << myProp->getFullName() << " on change of " << key
            );
        }
        catch (std::exception& e) {
            FC_ERR(e.what());
            FC_ERR(
                "Failed to evaluate property binding " << myProp->getFullName() << " on change of "
                                                       << key
            );
        }
    }
}

void PropertyExpressionEngine::slotChangedObject(const App::DocumentObject& obj, const App::Property&)
{
    updateHiddenReference(obj.getFullName());
}

void PropertyExpressionEngine::slotChangedProperty(const App::DocumentObject&, const App::Property& prop)
{
    updateHiddenReference(prop.getFullName());
}

void PropertyExpressionEngine::Paste(const Property& from)
{
    const auto& fromee = dynamic_cast<const PropertyExpressionEngine&>(from);
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    auto source = RecomputeDetail::Coordinator::instance().mutation(fromee);
    auto proposed = fromee.copyExpressions();
    auto targets = admissionTargets(proposed);
    mutation.include(targets);
    // Validator is part of the same admitted commit as the expression map.
    preflightAdmissionStorage();
    AtomicPropertyChange signaller(*this);
    expressions = std::move(proposed);
    validator = fromee.validator;
    for (const auto& entry : expressions) {
        expressionChanged(entry.first);
    }
    signaller.tryInvoke();
}

void PropertyExpressionEngine::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<ExpressionEngine count=\"" << expressions.size();
    if (PropertyExpressionContainer::_XLinks.empty()) {
        writer.Stream() << "\">" << std::endl;
        writer.incInd();
    }
    else {
        writer.Stream() << R"(" xlink="1">)" << std::endl;
        writer.incInd();
        PropertyExpressionContainer::Save(writer);
    }
    for (const auto& it : expressions) {
        std::string expression, comment;
        if (it.second.expression) {
            expression = it.second.expression->toString(true);
            comment = it.second.expression->comment;
        }

        writer.Stream() << writer.ind() << "<Expression path=\""
                        << Property::encodeAttribute(it.first.toString()) << "\" expression=\""
                        << Property::encodeAttribute(expression) << "\"";
        if (!comment.empty()) {
            writer.Stream() << " comment=\"" << Property::encodeAttribute(comment) << "\"";
        }
        writer.Stream() << "/>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</ExpressionEngine>" << std::endl;
}

void PropertyExpressionEngine::Restore(Base::XMLReader& reader)
{
    auto restoration = RecomputeDetail::Coordinator::instance().atomicForRestore(*this);
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    reader.readElement("ExpressionEngine");
    int count = reader.getAttribute<double>("count");

    if (reader.hasAttribute("xlink") && reader.getAttribute<bool>("xlink")) {
        PropertyExpressionContainer::Restore(reader);
    }

    restoredExpressions = std::make_unique<std::vector<RestoredExpression>>();
    restoredExpressions->reserve(count);
    for (int i = 0; i < count; ++i) {

        reader.readElement("Expression");
        restoredExpressions->emplace_back();
        auto& info = restoredExpressions->back();
        info.path = reader.getAttribute<const char*>("path");
        info.expr = reader.getAttribute<const char*>("expression");
        if (reader.hasAttribute("comment")) {
            info.comment = reader.getAttribute<const char*>("comment");
        }
    }

    reader.readEndElement("ExpressionEngine");
}

ObjectIdentifier PropertyExpressionEngine::canonicalPath(const ObjectIdentifier& oid) const
{
    DocumentObject* docObj = freecad_cast<DocumentObject*>(getContainer());

    // Am I owned by a DocumentObject?
    if (!docObj) {
        throw Base::RuntimeError("PropertyExpressionEngine must be owned by a DocumentObject.");
    }

    int ptype;
    Property* prop = oid.getProperty(&ptype);

    // oid pointing to a property...?
    if (!prop) {
        throw Base::RuntimeError(oid.resolveErrorString().c_str());
    }

    if (ptype || prop->getContainer() != getContainer()) {
        return oid;
    }

    // In case someone calls this with p pointing to a PropertyExpressionEngine for some reason
    if (prop->isDerivedFrom(PropertyExpressionEngine::classTypeId)) {
        return oid;
    }

    // Dispatch call to actual canonicalPath implementation
    return oid.canonicalPath();
}

size_t PropertyExpressionEngine::numExpressions() const
{
    return expressions.size();
}

void PropertyExpressionEngine::afterRestore()
{
    auto restoration = RecomputeDetail::Coordinator::instance().atomicForRestore(*this);
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    DocumentObject* docObj = freecad_cast<DocumentObject*>(getContainer());
    if (restoredExpressions && docObj) {
        preflightAdmissionStorage();
        Base::FlagToggler<bool> flag(restoring);
        AtomicPropertyChange signaller(*this);

        PropertyExpressionContainer::afterRestore();
        ObjectIdentifier::DocumentMapper mapper(this->_DocMap);

        for (auto& info : *restoredExpressions) {
            tryRestoreExpression(docObj, info);
        }
        signaller.tryInvoke();
    }
    restoredExpressions.reset();
}

void PropertyExpressionEngine::tryRestoreExpression(DocumentObject* docObj, const RestoredExpression& info)
{
    try {
        ObjectIdentifier path = ObjectIdentifier::parse(docObj, info.path);
        if (!info.expr.empty()) {
            std::shared_ptr<Expression> expression(Expression::parse(docObj, info.expr));
            if (expression) {
                expression->comment = info.comment;
            }
            setValue(path, expression);
        }
    }
    catch (const RecomputeDetail::AdmissionRefusal&) {
        throw;
    }
    catch (const Base::Exception& e) {
        FC_ERR("Failed to restore " << docObj->getFullName() << '.' << getName() << ": " << e.what());
    }
}

void PropertyExpressionEngine::onContainerRestored()
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    PropertyExpressionEngine staged;
    staged.admissionStaging = true;
    staged.expressions = copyExpressions();
    {
        UpdateElementReferenceExpressionVisitor<PropertyExpressionEngine> plan(staged);
        for (auto& entry : staged.expressions) {
            if (entry.second.expression) {
                entry.second.expression->visit(plan);
            }
        }
    }
    std::vector<DocumentObject*> plannedTargets;
    staged.collectAdmissionTargets(plannedTargets);
    mutation.include(plannedTargets);
    preflightAdmissionStorage();

    Base::FlagToggler<bool> flag(restoring);
    unregisterElementReference();
    UpdateElementReferenceExpressionVisitor<PropertyExpressionEngine> v(*this);
    for (auto& e : expressions) {
        auto expr = e.second.expression;
        if (expr) {
            expr->visit(v);
        }
    }
}

const boost::any PropertyExpressionEngine::getPathValue(const App::ObjectIdentifier& path) const
{
    // Get a canonical path
    ObjectIdentifier usePath(canonicalPath(path));

    auto i = expressions.find(usePath);
    if (i != expressions.end()) {
        return i->second;
    }

    return boost::any();
}

void PropertyExpressionEngine::setValue(const ObjectIdentifier& path, std::shared_ptr<Expression> expr)
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    ObjectIdentifier usePath(canonicalPath(path));
    const Property* prop = usePath.getProperty();
    prop->getPathValue(usePath);
    auto it = expressions.find(usePath);
    if (it != expressions.end()
        && (expr == it->second.expression
            || (expr && it->second.expression && expr->isSame(*it->second.expression)))) {
        return;
    }
    ExpressionMap proposed = expressions;
    if (expr) {
        proposed[usePath] = ExpressionInfo(expr);
        mutation.include(admissionTargets(proposed));
        std::string error = validateExpression(usePath, expr);
        if (!error.empty()) {
            throw Base::RuntimeError(error.c_str());
        }
    }
    else if (it != expressions.end()) {
        proposed.erase(usePath);
    }
    else {
        return;
    }
    commitExpressions(std::move(proposed));
}

std::vector<App::ObjectIdentifier> PropertyExpressionEngine::computeEvaluationOrder(ExecuteOption option)
{
    std::vector<App::ObjectIdentifier> evaluationOrder;
    boost::unordered_map<int, ObjectIdentifier> revNodes;
    DiGraph g;

    Private::buildGraph(expressions, revNodes, g, option);

    /* Compute evaluation order for expressions */
    std::vector<int> c;
    topological_sort(g, std::back_inserter(c));

    for (int i : c) {
        // we return the evaluation order for our properties, not the dependencies
        // the topo sort will contain node ids for both our props and their deps
        if (revNodes.find(i) != revNodes.end()) {
            evaluationOrder.push_back(revNodes[i]);
        }
    }

    return evaluationOrder;
}

DocumentObjectExecReturn* App::PropertyExpressionEngine::execute(ExecuteOption option, bool* touched)
{
    DocumentObject* docObj = freecad_cast<DocumentObject*>(getContainer());

    if (!docObj) {
        throw Base::RuntimeError("PropertyExpressionEngine must be owned by a DocumentObject.");
    }

    if (running) {
        return DocumentObject::StdReturn;
    }

    if (option == ExecuteOnRestore) {
        bool found = false;
        for (auto& e : expressions) {
            auto prop = e.first.getProperty();
            if (!prop) {
                continue;
            }
            if (prop->testStatus(App::Property::Transient) || (prop->getType() & App::Prop_Transient)
                || prop->testStatus(App::Property::EvalOnRestore)) {
                found = true;
                break;
            }
        }
        if (!found) {
            return DocumentObject::StdReturn;
        }
    }

    /* Resetter class, to ensure that the "running" variable gets set to false, even if
     * an exception is thrown.
     */

    class resetter
    {
    public:
        explicit resetter(bool& b)
            : _b(b)
        {
            _b = true;
        }
        ~resetter()
        {
            _b = false;
        }

    private:
        bool& _b;
    };

    resetter r(running);

    // Compute evaluation order
    std::vector<App::ObjectIdentifier> evaluationOrder = computeEvaluationOrder(option);
    std::vector<ObjectIdentifier>::const_iterator it = evaluationOrder.begin();

#ifdef FC_PROPERTYEXPRESSIONENGINE_LOG
    std::clog << "Computing expressions for " << getName() << std::endl;
#endif

    /* Evaluate the expressions, and update properties */
    for (; it != evaluationOrder.end(); ++it) {

        // Get property to update
        Property* prop = it->getProperty();

        if (!prop) {
            throw Base::RuntimeError("Path does not resolve to a property.");
        }

        DocumentObject* parent = freecad_cast<DocumentObject*>(prop->getContainer());

        /* Make sure property belongs to the same container as this PropertyExpressionEngine */
        if (parent != docObj) {
            throw Base::RuntimeError("Invalid property owner.");
        }

        /* Set value of property */
        App::any value;
        try {
            // Evaluate expression
            std::shared_ptr<App::Expression> expression = expressions[*it].expression;
            if (expression) {
                value = expression->getValueAsAny();

                // Enable value comparison for all expression bindings to reduce
                // unnecessary touch and recompute.
                //
                // This optimization is necessary for some hidden reference to
                // work because it introduce dependency loop. The repeativtive
                // recompute can be stopped if the expression evaluates the same
                // value.
                //
                // In the future, we can generalize the optimization to all
                // property modification, i.e. do not touch unless value change
                //
                // if (option == ExecuteOnRestore && prop->testStatus(Property::EvalOnRestore))
                {
                    if (isAnyEqual(value, prop->getPathValue(*it))) {
                        continue;
                    }
                    if (touched) {
                        *touched = true;
                    }
                }
                prop->setPathValue(*it, value);
            }
        }
        catch (const RecomputeDetail::AdmissionRefusal&) {
            throw;
        }
        catch (Base::Exception& e) {
            std::ostringstream ss;
            ss << e.what() << std::endl << "in property binding '" << prop->getFullName() << "'";
            e.setMessage(ss.str());
            throw;
        }
        catch (std::bad_cast&) {
            std::ostringstream ss;
            ss << "Invalid type '" << value.type().name() << "'";
            ss << "\nin property binding '" << prop->getFullName() << "'";
            throw Base::TypeError(ss.str().c_str());
        }
        catch (std::exception& e) {
            std::ostringstream ss;
            ss << e.what() << "\nin property binding '" << prop->getFullName() << "'";
            throw Base::RuntimeError(ss.str().c_str());
        }
    }
    return DocumentObject::StdReturn;
}

void PropertyExpressionEngine::getPathsToDocumentObject(
    DocumentObject* obj,
    std::vector<App::ObjectIdentifier>& paths
) const
{
    DocumentObject* owner = freecad_cast<DocumentObject*>(getContainer());

    if (!owner || owner == obj) {
        return;
    }

    for (auto& v : expressions) {
        if (!v.second.expression) {
            continue;
        }
        const auto& deps = v.second.expression->getDeps();
        auto it = deps.find(obj);
        if (it == deps.end()) {
            continue;
        }
        for (auto& dep : it->second) {
            paths.insert(paths.end(), dep.second.begin(), dep.second.end());
        }
    }
}

bool PropertyExpressionEngine::depsAreTouched() const
{
    for (auto& v : _Deps) {
        // v.second indicates if it is a hidden reference
        if (!v.second && v.first->isTouched()) {
            return true;
        }
    }
    return false;
}

std::string PropertyExpressionEngine::validateExpression(
    const ObjectIdentifier& path,
    std::shared_ptr<const Expression> expr
) const
{
    return validateExpressionPlan(path, std::move(expr), expressions);
}

std::string PropertyExpressionEngine::validateExpressionPlan(
    const ObjectIdentifier& path,
    std::shared_ptr<const Expression> expr,
    const ExpressionMap& baseline
) const
{
    std::string error;
    ObjectIdentifier usePath(canonicalPath(path));

    if (validator) {
        error = validator(usePath, expr);
        if (!error.empty()) {
            return error;
        }
    }

    // Get document object
    DocumentObject* pathDocObj = usePath.getDocumentObject();
    assert(pathDocObj);

    if (GetApplication().isFineGrainedRecomputeEnabled()) {
        // Fully mimics the else part except for taking into account input properties.
        auto inList = pathDocObj->getInListEx(true);

        std::map<DocumentObject*, bool> depObjects;
        std::map<std::pair<std::string, DocumentObject*>, bool> propDeps;
        expr->getDepObjects(depObjects, nullptr, &propDeps);

        for (const auto& [pair, hidden] : propDeps) {
            auto* docObj = pair.second;
            auto& propName = pair.first;
            if (!hidden && inList.contains(docObj) && !docObj->isInputProperty(propName)) {
                std::stringstream ss;
                ss << "cyclic reference to " << docObj->getFullName() << "." << propName;
                return ss.str();
            }
        }
    }
    else {
        auto inList = pathDocObj->getInListEx(true);
        for (auto& v : expr->getDepObjects()) {
            auto docObj = v.first;
            if (!v.second && inList.contains(docObj)) {
                std::stringstream ss;
                ss << "cyclic reference to " << docObj->getFullName();
                return ss.str();
            }
        }
    }


    // Check for internal document object dependencies

    // Copy current expressions
    ExpressionMap newExpressions = baseline;

    // Add expression in question
    std::shared_ptr<Expression> exprClone(expr->copy());
    newExpressions[usePath].expression = exprClone;

    // Build graph; an exception will be thrown if it is not a DAG
    try {
        boost::unordered_map<int, ObjectIdentifier> revNodes;
        DiGraph g;

        Private::buildGraph(newExpressions, revNodes, g);
    }
    catch (const RecomputeDetail::AdmissionRefusal&) {
        throw;
    }
    catch (const Base::Exception& e) {
        return e.what();
    }

    return {};
}

void PropertyExpressionEngine::renameExpressions(
    const std::map<ObjectIdentifier, ObjectIdentifier>& paths
)
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    ExpressionMap newExpressions;
    std::map<ObjectIdentifier, ObjectIdentifier> canonicalPaths;

    /* ensure input map uses canonical paths */
    for (const auto& path : paths) {
        canonicalPaths[canonicalPath(path.first)] = path.second;
    }

    for (auto i = expressions.begin(); i != expressions.end(); ++i) {
        auto j = canonicalPaths.find(i->first);

        // Renamed now?
        if (j != canonicalPaths.end()) {
            newExpressions[j->second] = i->second;
        }
        else {
            newExpressions[i->first] = i->second;
        }
    }

    mutation.include(admissionTargets(newExpressions));
    preflightAdmissionStorage();
    aboutToSetValue();
    expressions = std::move(newExpressions);
    for (auto i = expressions.begin(); i != expressions.end(); ++i) {
        expressionChanged(i->first);
    }

    hasSetValue();
}

void PropertyExpressionEngine::renameObjectIdentifiers(
    const std::map<ObjectIdentifier, ObjectIdentifier>& paths
)
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    auto proposed = copyExpressions();
    class RenamePlan: public ExpressionVisitor
    {
    public:
        RenamePlan(
            const std::map<ObjectIdentifier, ObjectIdentifier>& paths,
            const ObjectIdentifier& owner
        )
            : paths(paths)
            , owner(owner)
        {}
        void visit(Expression& node) override
        {
            renameObjectIdentifier(node, paths, owner);
        }

        void aboutToChange() override
        {
            modified = true;
        }
        bool modified = false;

    private:
        const std::map<ObjectIdentifier, ObjectIdentifier>& paths;
        const ObjectIdentifier& owner;
    };
    std::set<ObjectIdentifier> changedPaths;
    for (auto& entry : proposed) {
        if (entry.second.expression) {
            RenamePlan visitor(paths, entry.first);
            entry.second.expression->visit(visitor);
            if (visitor.modified) {
                changedPaths.insert(entry.first);
            }
        }
    }
    // A rewrite can change identifier owner while preserving its serialized
    // text. The visitor's actual edit, not text equality, decides publication.
    commitExpressions(std::move(proposed), true, changedPaths);
}

PyObject* PropertyExpressionEngine::getPyObject()
{
    Py::List list;
    for (const auto& it : expressions) {
        Py::Tuple tuple(2);
        tuple.setItem(0, Py::String(it.first.toString()));
        auto expr = it.second.expression;
        tuple.setItem(1, expr ? Py::String(expr->toString()) : Py::None());
        list.append(tuple);
    }
    return Py::new_reference_to(list);
}

void PropertyExpressionEngine::setPyObject(PyObject*)
{
    throw Base::RuntimeError("Property is read-only");
}

/* The policy implemented in the following function is to auto erase binding in
 * case linked object is gone. I think it is better to cause error and get
 * user's attention
 *
void PropertyExpressionEngine::breakLink(App::DocumentObject *obj, bool clear) {
    auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if(!owner)
        return;
    if(_Deps.count(obj)==0 && (!clear || obj!=owner || _Deps.empty()))
        return;
    AtomicPropertyChange signaler(*this);
    for(auto it=expressions.begin(),itNext=it;it!=expressions.end();it=itNext) {
        ++itNext;
        const auto &deps = it->second.expression->getDepObjects();
        if(clear) {
            // here means we are breaking all expression, except those that has
            // no depdenecy or self dependency
            if(deps.empty() || (deps.size()==1 && *deps.begin()==owner))
                continue;
        }else if(!deps.count(obj))
            continue;
        auto path = it->first;
        expressions.erase(it);
        expressionChanged(path);
    }
}
*/

bool PropertyExpressionEngine::adjustLink(const std::set<DocumentObject*>& inList)
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    auto* owner = dynamic_cast<DocumentObject*>(getContainer());
    if (!owner) {
        return false;
    }
    bool found = false;
    for (const auto& dependency : _Deps) {
        if (inList.contains(dependency.first)) {
            found = true;
            break;
        }
    }
    if (!found) {
        return false;
    }
    auto proposed = copyExpressions();
    for (auto& entry : proposed) {
        if (entry.second.expression) {
            entry.second.expression->adjustLinks(inList);
        }
    }
    commitExpressions(std::move(proposed));
    return true;
}

void PropertyExpressionEngine::updateElementReference(DocumentObject* feature, bool reverse, bool notify)
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    if (feature) {
        mutation.include({feature});
    }
    PropertyExpressionEngine staged;
    staged.admissionStaging = true;
    staged.expressions = copyExpressions();
    {
        UpdateElementReferenceExpressionVisitor<PropertyExpressionEngine> plan(staged, feature, reverse);
        for (auto& entry : staged.expressions) {
            if (entry.second.expression) {
                entry.second.expression->visit(plan);
            }
        }
    }
    std::vector<DocumentObject*> plannedTargets;
    staged.collectAdmissionTargets(plannedTargets);
    mutation.include(plannedTargets);
    preflightAdmissionStorage();
    // Only after the complete staged footprint is reserved may the original
    // visitor update live ASTs and element-registration metadata.

    (void)notify;
    if (!feature) {
        unregisterElementReference();
    }
    UpdateElementReferenceExpressionVisitor<PropertyExpressionEngine> v(*this, feature, reverse);
    for (auto& e : expressions) {
        if (e.second.expression) {
            e.second.expression->visit(v);
            if (v.changed()) {
                expressionChanged(e.first);
                v.reset();
            }
        }
    }
    if (feature && v.changed()) {
        auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
        if (owner) {
            owner->onUpdateElementReference(this);
        }
    }
}

bool PropertyExpressionEngine::referenceChanged() const
{
    return false;
}

Property* PropertyExpressionEngine::CopyOnImportExternal(
    const std::map<std::string, std::string>& nameMap
) const
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    std::unique_ptr<PropertyExpressionEngine> engine;
    for (auto it = expressions.begin(); it != expressions.end(); ++it) {
#ifdef BOOST_NO_CXX11_SMART_PTR
        std::shared_ptr<Expression> expr(it->second.expression->importSubNames(nameMap).release());
#else
        std::shared_ptr<Expression> expr(it->second.expression->importSubNames(nameMap));
#endif
        if (!expr && !engine) {
            continue;
        }
        if (!engine) {
            engine = std::make_unique<PropertyExpressionEngine>();
            for (auto it2 = expressions.begin(); it2 != it; ++it2) {
                engine->expressions[it2->first] = ExpressionInfo(
                    std::shared_ptr<Expression>(it2->second.expression->copy())
                );
            }
        }
        else if (!expr) {
            expr = it->second.expression;
        }
        engine->expressions[it->first] = ExpressionInfo(expr);
    }
    if (!engine) {
        return nullptr;
    }
    engine->validator = validator;
    mutation.include(admissionTargets(engine->expressions));
    engine->refreshAdmissionMetadata();
    return engine.release();
}

Property* PropertyExpressionEngine::CopyOnLabelChange(
    App::DocumentObject* obj,
    const std::string& ref,
    const char* newLabel
) const
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    std::unique_ptr<PropertyExpressionEngine> engine;
    for (auto it = expressions.begin(); it != expressions.end(); ++it) {
#ifdef BOOST_NO_CXX11_SMART_PTR
        std::shared_ptr<Expression> expr(
            it->second.expression->updateLabelReference(obj, ref, newLabel).release()
        );
#else
        std::shared_ptr<Expression> expr(
            it->second.expression->updateLabelReference(obj, ref, newLabel)
        );
#endif
        if (!expr && !engine) {
            continue;
        }
        if (!engine) {
            engine = std::make_unique<PropertyExpressionEngine>();
            for (auto it2 = expressions.begin(); it2 != it; ++it2) {
                ExpressionInfo info;
                if (it2->second.expression) {
                    info.expression = std::shared_ptr<Expression>(it2->second.expression->copy());
                }
                engine->expressions[it2->first] = info;
            }
        }
        else if (!expr) {
            expr = it->second.expression;
        }
        engine->expressions[it->first] = ExpressionInfo(expr);
    }
    if (!engine) {
        return nullptr;
    }
    engine->validator = validator;
    mutation.include(admissionTargets(engine->expressions));
    engine->refreshAdmissionMetadata();
    return engine.release();
}

Property* PropertyExpressionEngine::CopyOnLinkReplace(
    const App::DocumentObject* parent,
    App::DocumentObject* oldObj,
    App::DocumentObject* newObj
) const
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    std::unique_ptr<PropertyExpressionEngine> engine;
    for (auto it = expressions.begin(); it != expressions.end(); ++it) {
#ifdef BOOST_NO_CXX11_SMART_PTR
        std::shared_ptr<Expression> expr(
            it->second.expression->replaceObject(parent, oldObj, newObj).release()
        );
#else
        std::shared_ptr<Expression> expr(it->second.expression->replaceObject(parent, oldObj, newObj));
#endif
        if (!expr && !engine) {
            continue;
        }
        if (!engine) {
            engine = std::make_unique<PropertyExpressionEngine>();
            for (auto it2 = expressions.begin(); it2 != it; ++it2) {
                ExpressionInfo info;
                if (it2->second.expression) {
                    info.expression = std::shared_ptr<Expression>(it2->second.expression->copy());
                }
                engine->expressions[it2->first] = info;
            }
        }
        else if (!expr) {
            expr = it->second.expression;
        }
        engine->expressions[it->first] = ExpressionInfo(expr);
    }
    if (!engine) {
        return nullptr;
    }
    engine->validator = validator;
    mutation.include(admissionTargets(engine->expressions));
    engine->refreshAdmissionMetadata();
    return engine.release();
}

std::map<App::ObjectIdentifier, const App::Expression*> PropertyExpressionEngine::getExpressions() const
{
    std::map<App::ObjectIdentifier, const Expression*> res;
    for (auto& v : expressions) {
        res[v.first] = v.second.expression.get();
    }
    return res;
}

void PropertyExpressionEngine::setExpressions(
    std::map<App::ObjectIdentifier, App::ExpressionPtr>&& exprs
)
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    ExpressionMap proposed = expressions;
    for (auto& entry : exprs) {
        ObjectIdentifier path(canonicalPath(entry.first));
        path.getProperty()->getPathValue(path);
        if (entry.second) {
            proposed[path] = ExpressionInfo(std::shared_ptr<Expression>(std::move(entry.second)));
        }
        else {
            proposed.erase(path);
        }
    }
    mutation.include(admissionTargets(proposed));
    // Validate the entire final graph and every custom validator before any
    // live-map assignment, transaction notification, or expressionChanged signal.
    boost::unordered_map<int, ObjectIdentifier> nodes;
    DiGraph graph;
    Private::buildGraph(proposed, nodes, graph);
    for (const auto& entry : proposed) {
        if (entry.second.expression) {
            auto error = validateExpressionPlan(entry.first, entry.second.expression, proposed);
            if (!error.empty()) {
                throw Base::RuntimeError(error.c_str());
            }
        }
    }
    preflightAdmissionStorage();
    AtomicPropertyChange signaller(*this);
    commitExpressions(std::move(proposed));
    signaller.tryInvoke();
}

void PropertyExpressionEngine::onRelabeledDocument(const App::Document& doc)
{
    auto mutation = RecomputeDetail::Coordinator::instance().mutation(*this);
    auto proposed = copyExpressions();
    RelabelDocumentExpressionVisitor visitor(doc);
    for (auto& entry : proposed) {
        if (entry.second.expression) {
            entry.second.expression->visit(visitor);
        }
    }
    // Relabel remains deliberately silent; admission still covers the AST write.
    commitExpressions(std::move(proposed), false);
}

void PropertyExpressionEngine::onRenameDynamicProperty(const App::Property& prop, const char* oldName)
{
    ObjectIdentifier oldNameId = ObjectIdentifier(prop.getContainer(), std::string(oldName));
    ObjectIdentifier newNameId = ObjectIdentifier(prop);
    const std::map<ObjectIdentifier, ObjectIdentifier> paths = {
        {oldNameId, newNameId},
    };

    renameObjectIdentifiers(paths);
}

void PropertyExpressionEngine::onMoveDynamicProperty(
    const App::Property& prop,
    const DocumentObject& targetObj
)
{
    ObjectIdentifier oldNameId = ObjectIdentifier(prop);
    ObjectIdentifier newNameId = ObjectIdentifier(&targetObj, std::string(prop.getName()));
    const std::map<ObjectIdentifier, ObjectIdentifier> paths = {
        {oldNameId, newNameId},
    };

    renameObjectIdentifiers(paths);
}

void PropertyExpressionEngine::getLinksTo(
    std::vector<App::ObjectIdentifier>& identifiers,
    App::DocumentObject* obj,
    const char* subname,
    bool all
) const
{
    Expression::DepOption option = all ? Expression::DepOption::DepAll
                                       : Expression::DepOption::DepNormal;

    App::SubObjectT objT(obj, subname);
    auto sobj = objT.getSubObject();
    auto subElement = objT.getOldElementName();

    for (auto& [expressionId, expressionInfo] : expressions) {
        const auto& deps = expressionInfo.expression->getDeps(option);
        auto it = deps.find(obj);
        if (it == deps.end()) {
            continue;
        }
        auto [docObj, map] = *it;
        for (auto& [key, paths] : map) {
            if (!subname) {
                identifiers.push_back(expressionId);
                break;
            }
            if (std::ranges::any_of(paths, [subname, obj, sobj, &subElement](const auto& path) {
                    if (path.getSubObjectName() == subname) {
                        return true;
                    }

                    App::SubObjectT sobjT(obj, path.getSubObjectName().c_str());
                    return (sobjT.getSubObject() == sobj && sobjT.getOldElementName() == subElement);
                })) {
                identifiers.push_back(expressionId);
            }
        }
    }
}
