// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <App/Document.h>
#include <App/DocumentPy.h>
#include <App/DocumentObjectPy.h>

#include "MotionSuspension.h"


namespace Assembly
{
namespace
{
App::Document& documentArgument(const Py::Tuple& args)
{
    PyObject* value = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "O!", &App::DocumentPy::Type, &value)) {
        throw Py::Exception();
    }
    auto* document = static_cast<App::DocumentPy*>(value);
    if (!document->isValid()) {
        throw Py::RuntimeError("The document has been closed");
    }
    return *document->getDocumentPtr();
}

class MotionSuspensionContext: public Py::PythonExtension<MotionSuspensionContext>
{
public:
    static void initType()
    {
        behaviors().name("AssemblyApp.AutomaticMotionSuspension");
        behaviors().doc("Single-use context disabling automatic assembly motion in a document.");
        behaviors().supportGetattr();
        add_varargs_method("__enter__", &MotionSuspensionContext::enter);
        add_varargs_method("__exit__", &MotionSuspensionContext::exit);
        // Python's with statement looks up special methods on the type, bypassing
        // the instance getattr dispatcher used by the older PyCXX extension API.
        static PyMethodDef methods[] = {
            {"__enter__", enterSlot, METH_VARARGS, nullptr},
            {"__exit__", exitSlot, METH_VARARGS, nullptr},
            {nullptr, nullptr, 0, nullptr}
        };
        behaviors().type_object()->tp_methods = methods;
        behaviors().readyType();
    }

    explicit MotionSuspensionContext(App::Document& document)
        : documentObject(document.getPyObject(), true)
    {}

private:
    static PyObject* enterSlot(PyObject* self, PyObject* args)
    {
        try {
            return Py::new_reference_to(
                static_cast<MotionSuspensionContext*>(self)->enter(Py::Tuple(args))
            );
        }
        catch (const Py::Exception&) {
            return nullptr;
        }
        catch (const std::bad_alloc&) {
            return PyErr_NoMemory();
        }
    }

    static PyObject* exitSlot(PyObject* self, PyObject* args)
    {
        try {
            return Py::new_reference_to(
                static_cast<MotionSuspensionContext*>(self)->exit(Py::Tuple(args))
            );
        }
        catch (const Py::Exception&) {
            return nullptr;
        }
    }

    Py::Object enter(const Py::Tuple& args)
    {
        if (!PyArg_ParseTuple(args.ptr(), "")) {
            throw Py::Exception();
        }
        if (entered) {
            throw Py::RuntimeError("A motion suspension context can only be entered once");
        }
        auto* document = static_cast<App::DocumentPy*>(documentObject.ptr());
        if (!document->isValid()) {
            throw Py::RuntimeError("The document has been closed");
        }
        suspension = std::make_unique<AutomaticMotionSuspension>(*document->getDocumentPtr());
        entered = true;
        return Py::Object(this);
    }

    Py::Object exit(const Py::Tuple& args)
    {
        PyObject* exceptionType;
        PyObject* exception;
        PyObject* traceback;
        if (!PyArg_ParseTuple(args.ptr(), "OOO", &exceptionType, &exception, &traceback)) {
            throw Py::Exception();
        }
        suspension.reset();
        return Py::Boolean(false);
    }

    Py::Object documentObject;
    std::unique_ptr<AutomaticMotionSuspension> suspension;
    bool entered = false;
};
}  // namespace

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("AssemblyApp")
    {
        MotionSuspensionContext::initType();
        add_varargs_method(
            "suspendAutomaticMotion",
            &Module::suspendAutomaticMotion,
            "Return a single-use context suspending automatic motion in a document."
        );
        add_varargs_method(
            "isAutomaticMotionSuspended",
            &Module::isAutomaticMotionSuspended,
            "Return whether automatic assembly motion is suspended in a document."
        );
        add_varargs_method(
            "_deferGroundedRelease",
            &Module::deferGroundedRelease,
            "Internal: defer local grounded-joint status release until admission."
        );
        initialize("This module is the Assembly module.");  // register with Python
    }

private:
    Py::Object suspendAutomaticMotion(const Py::Tuple& args)
    {
        return Py::asObject(new MotionSuspensionContext(documentArgument(args)));
    }

    Py::Object isAutomaticMotionSuspended(const Py::Tuple& args)
    {
        return Py::Boolean(Assembly::isAutomaticMotionSuspended(&documentArgument(args)));
    }

    Py::Object deferGroundedRelease(const Py::Tuple& args)
    {
        PyObject* documentValue = nullptr;
        PyObject* targetValue = nullptr;
        if (!PyArg_ParseTuple(
                args.ptr(),
                "O!O!",
                &App::DocumentPy::Type,
                &documentValue,
                &App::DocumentObjectPy::Type,
                &targetValue
            )) {
            throw Py::Exception();
        }
        auto* document = static_cast<App::DocumentPy*>(documentValue);
        auto* target = static_cast<App::DocumentObjectPy*>(targetValue);
        if (!document->isValid() || !target->Base::PyObjectBase::isValid()) {
            throw Py::RuntimeError("The document or grounded object has been deleted");
        }
        Assembly::deferGroundedRelease(*document->getDocumentPtr(), target->getDocumentObjectPtr());
        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Assembly
