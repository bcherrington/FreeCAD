// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2002 Jürgen Riegel
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <cassert>
#include <algorithm>
#include <array>
#include <tuple>

#include <atomic>
#include <Base/Tools.h>
#include <Base/Writer.h>
#include <CXX/Objects.hxx>

#include "Property.h"
#include "ObjectIdentifier.h"
#include "PropertyContainer.h"
#include "PropertyLinks.h"
#include "Document.h"
#include "DocumentObject.h"
#include "private/RecomputeAdmission.h"


using namespace App;


//**************************************************************************
//**************************************************************************
// Property
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::Property, Base::Persistence)

//**************************************************************************
// Construction/Destruction

static std::atomic<int64_t> _PropID;

// Here is the implementation! Description should take place in the header file!
Property::Property()
    : _id(++_PropID)
{
    RecomputeDetail::Coordinator::instance().registerProperty(this);
}

Property::~Property()
{
    RecomputeDetail::Coordinator::instance().retireProperty(this);
}

const char* Property::getName() const
{
    return myName ? myName : "";
}

bool Property::hasName() const
{
    return isValidName(myName);
}

bool Property::isValidName(const char* name)
{
    return !Base::Tools::isNullOrEmpty(name);
}

std::string Property::getFullName() const
{
    std::string name;
    if (myName) {
        if (father) {
            name = father->getFullName() + ".";
        }
        else {
            name = "?.";
        }
        name += myName;
    }
    else {
        return "?";
    }
    return name;
}

std::string Property::getFileName(const char* postfix, const char* prefix) const
{
    std::ostringstream ss;
    if (prefix) {
        ss << prefix;
    }
    if (!myName) {
        ss << "Property";
    }
    else {
        std::string name = getFullName();
        auto pos = name.find('#');
        if (pos == std::string::npos) {
            ss << name;
        }
        else {
            ss << (name.c_str() + pos + 1);
        }
    }
    if (postfix) {
        ss << postfix;
    }
    return ss.str();
}

// clang-format off
static constexpr auto mapProps = std::to_array<std::tuple<Property::Status, PropertyType>>({
    {App::Property::PropReadOnly,    Prop_ReadOnly},
    {App::Property::PropHidden,      Prop_Hidden},
    {App::Property::PropInput,       Prop_Input},
    {App::Property::PropOutput,      Prop_Output},
    {App::Property::PropTransient,   Prop_Transient},
    {App::Property::PropNoRecompute, Prop_NoRecompute},
    {App::Property::PropNoPersist,   Prop_NoPersist}
});
// clang-format on

short Property::getType() const
{
    short type = 0;
    for (const auto& [propertyStatus, propertyType] : mapProps) {
        if (testStatus(propertyStatus)) {
            type |= propertyType;
        }
    }
    return type;
}

void Property::syncType(unsigned type)
{
    for (const auto& [propertyStatus, propertyType] : mapProps) {
        if (type & propertyType) {
            StatusBits.set((size_t)propertyStatus);
        }
    }
}

const char* Property::getGroup() const
{
    return father->getPropertyGroup(this);
}

const char* Property::getDocumentation() const
{
    return father->getPropertyDocumentation(this);
}

void Property::setContainer(PropertyContainer* father)
{
    auto& admission = RecomputeDetail::Coordinator::instance();
    auto mutation = admission.mutation(
        {dynamic_cast<DocumentObject*>(this->father), dynamic_cast<DocumentObject*>(father)},
        {dynamic_cast<Document*>(this->father), dynamic_cast<Document*>(father)});
    admission.registerProperty(this, dynamic_cast<DocumentObject*>(father), dynamic_cast<Document*>(father));
    this->father = father;
}

void Property::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    path.setValue(value);
}

const boost::any Property::getPathValue(const ObjectIdentifier& path) const
{
    return path.getValue();
}

void Property::getPaths(std::vector<ObjectIdentifier>& paths) const
{
    paths.emplace_back(getContainer(), getName());
}

void Property::getPathsForCompletion(
    std::vector<ObjectIdentifier>& paths,
    const std::string&
) const
{
    getPaths(paths);
}

ObjectIdentifier Property::canonicalPath(const ObjectIdentifier& p) const
{
    return p;
}

namespace App
{
/*!
 * \brief The PropertyCleaner struct
 * Make deleting dynamic property safer by postponing its destruction.
 *
 * Dynamic property can be removed at any time, even during triggering of
 * onChanged() signal of the removing property. This patch introduced
 * static function Property::destroy() to make it safer by queueing any
 * removed property, and only deleting them when no onChanged() call is
 * active.
 */
struct PropertyCleaner
{
    explicit PropertyCleaner(Property* p) : prop(p)
    {
        RecomputeDetail::IndexLock lock;
        ++_PropCleanerCounter;
    }
    ~PropertyCleaner()
    {
        {
            RecomputeDetail::IndexLock lock;
            if (--_PropCleanerCounter) {
                return;
            }
        }
        drain(prop);
    }
    static void drain(Property* excluded = nullptr)
    {
        std::vector<Property*> pending;
        {
            RecomputeDetail::IndexLock lock;
            if (_PropCleanerCounter) {
                return;
            }
            pending.swap(_RemovedProps);
        }
        auto& admission = RecomputeDetail::Coordinator::instance();
        for (auto* property : pending) {
            bool retained = property == excluded;
            if (!retained) {
                try {
                    // Edges were retired under the original writer scope.
                    // This phase releases only storage after old snapshots end;
                    // it must not inspect AST/native owners that may be gone.
                    retained = !admission.retireProperty(property);
                    if (!retained) {
                        property->father = nullptr;
                        delete property;
                    }
                }
                catch (const RecomputeDetail::AdmissionRefusal&) {
                    retained = true;
                }
            }
            if (retained) {
                RecomputeDetail::IndexLock lock;
                _RemovedProps.push_back(property);
            }
        }
        bool remains;
        {
            RecomputeDetail::IndexLock lock;
            remains = !_RemovedProps.empty();
        }
        if (remains) {
            admission.requestCleanup();
        }
    }
    static void add(Property* property)
    {
        static const bool registered = [] {
            RecomputeDetail::Coordinator::instance().setPropertyDrain([] { drain(); });
            return true;
        }();
        (void)registered;
        {
            RecomputeDetail::IndexLock lock;
            if (std::find(_RemovedProps.begin(), _RemovedProps.end(), property) == _RemovedProps.end()) {
                _RemovedProps.push_back(property);
            }
        }
        RecomputeDetail::Coordinator::instance().requestCleanup();
    }

    Property* prop;
    static std::vector<Property*> _RemovedProps;
    static int _PropCleanerCounter;
};
}  // namespace App

std::vector<Property*> PropertyCleaner::_RemovedProps;
int PropertyCleaner::_PropCleanerCounter = 0;

void Property::destroy(Property* p)
{
    if (p) {
        // Is it necessary to nullify the container? May cause crash if any
        // onChanged() caller assumes a non-null container.
        //
        // p->setContainer(0);

        auto mutation = RecomputeDetail::Coordinator::instance().mutation(*p);
        if (auto* link = dynamic_cast<PropertyLinkBase*>(p)) {
            link->retireAdmissionEdges();
        }
        // Retirement is sticky even while a previous snapshot retains storage.
        RecomputeDetail::Coordinator::instance().retireProperty(p);
        PropertyCleaner::add(p);
    }
}

bool Property::enableNotify(bool enable)
{
    bool isNotify = isNotifyEnabled();

    if (enable) {
        StatusBits.reset(DisableNotify);
    }
    else {
        StatusBits.set(DisableNotify);
    }
    return isNotify;
}

bool Property::isNotifyEnabled() const
{
    return !StatusBits.test(DisableNotify);
}

void Property::touch()
{
    std::optional<RecomputeDetail::MutationScope> mutation;
    if (dynamic_cast<const PropertyLinkBase*>(this)) {
        mutation.emplace(RecomputeDetail::Coordinator::instance().mutation(*this));
    }
    PropertyCleaner guard(this);
    if (father && isNotifyEnabled()) {
        father->onEarlyChange(this);
        father->onChanged(this);
    }
    StatusBits.set(Touched);
}

void Property::setReadOnly(bool readOnly)
{
    this->setStatus(App::Property::ReadOnly, readOnly);
}

void Property::hasSetValue()
{
    PropertyCleaner guard(this);
    if (father) {
        if (isNotifyEnabled()) {
            father->onChanged(this);
        }
        if (!testStatus(Busy)) {
            Base::BitsetLocker<decltype(StatusBits)> guard(StatusBits, Busy);
            signalChanged(*this);
        }
    }
    StatusBits.set(Touched);
}

void Property::aboutToSetValue()
{
    if (father) {
        father->onBeforeChange(this);
    }
}

void Property::verifyPath(const ObjectIdentifier& p) const
{
    p.verify(*this);
}

Property* Property::Copy() const
{
    // have to be reimplemented by a subclass!
    assert(0);
    return nullptr;
}

void Property::Paste(const Property& /*from*/)
{
    // have to be reimplemented by a subclass!
    assert(0);
}

void Property::setStatusValue(unsigned long status)
{
    // clang-format off
    static const unsigned long mask =
         (1<<PropDynamic)
        |(1<<PropNoRecompute)
        |(1<<PropReadOnly)
        |(1<<PropTransient)
        |(1<<PropOutput)
        |(1<<PropHidden)
        |(1<<PropNoPersist)
        |(1<<Busy);
    // clang-format on

    status &= ~mask;
    status |= StatusBits.to_ulong() & mask;
    unsigned long oldStatus = StatusBits.to_ulong();
    StatusBits = decltype(StatusBits)(status);

    if (father) {
        static unsigned long _signalMask = (1 << ReadOnly) | (1 << Hidden);
        if ((status & _signalMask) != (oldStatus & _signalMask)) {
            father->onPropertyStatusChanged(*this, oldStatus);
        }
    }
}

void Property::setStatus(Status pos, bool on)
{
    auto bits = StatusBits;
    bits.set(pos, on);
    setStatusValue(bits.to_ulong());
}

bool Property::isSame(const Property& other) const
{
    if (&other == this) {
        return true;
    }
    if (other.getTypeId() != getTypeId() || getMemSize() != other.getMemSize()) {
        return false;
    }

    Base::StringWriter writer, writer2;
    Save(writer);
    other.Save(writer2);
    return writer.getString() == writer2.getString();
}

//**************************************************************************
//**************************************************************************
// PropertyListsBase
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void PropertyListsBase::_setPyObject(PyObject* value)
{
    std::vector<int> indices;
    std::vector<PyObject*> vals;
    Py::Object pySeq;

    if (PyDict_Check(value)) {
        Py::Dict dict(value);
        auto size = dict.size();
        vals.reserve(size);
        indices.reserve(size);
        int listSize = getSize();
        for (auto it = dict.begin(); it != dict.end(); ++it) {
            const auto& item = *it;
            PyObject* key = item.first.ptr();
            if (!PyLong_Check(key)) {
                throw Base::TypeError("expect key type to be integer");
            }
            long idx = PyLong_AsLong(key);
            if (idx < -1 || idx > listSize) {
                throw Base::ValueError("index out of bound");
            }
            if (idx == -1 || idx == listSize) {
                idx = listSize;
                ++listSize;
            }
            indices.push_back(idx);
            vals.push_back(item.second.ptr());
        }
    }
    else {
        if (PySequence_Check(value)) {
            pySeq = value;
        }
        else {
            PyObject* iter = PyObject_GetIter(value);
            if (iter) {
                Py::Object pyIter(iter, true);
                pySeq = Py::asObject(PySequence_Fast(iter, ""));
            }
            else {
                PyErr_Clear();
                vals.push_back(value);
            }
        }
        if (!pySeq.isNone()) {
            Py::Sequence seq(pySeq);
            vals.reserve(seq.size());
            for (auto it = seq.begin(); it != seq.end(); ++it) {
                vals.push_back((*it).ptr());
            }
        }
    }
    setPyValues(vals, indices);
}


//**************************************************************************
//**************************************************************************
// PropertyLists
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyLists, App::Property)
