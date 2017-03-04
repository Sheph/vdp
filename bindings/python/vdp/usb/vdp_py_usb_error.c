/*
 * Copyright (c) 2017, Stanislav Vorobiov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "vdp_py_usb_error.h"

static PyObject* vdp_py_usb_error = NULL;

static char vdp_py_usb_error_doc[] = "vdpusb result wrapper exception";

static PyObject* vdp_py_usb_error_init_obj(PyObject* self, PyObject* args)
{
    vdp_usb_result res = vdp_usb_success;
    int status;
    PyObject* parent_init = NULL;
    PyObject* call_res = NULL;

    if (!PyArg_ParseTuple(args, "Oi:__init__", &self, &res)) {
        return NULL;
    }

    switch (res) {
    case vdp_usb_nomem:
    case vdp_usb_misuse:
    case vdp_usb_unknown:
    case vdp_usb_not_found:
    case vdp_usb_busy:
    case vdp_usb_protocol_error:
        break;
    default:
        assert(0);
        return NULL;
    }

    status = PyObject_SetAttrString(self, "result", PyLong_FromLong(res));
    if (status < 0) {
        return NULL;
    }

    parent_init = PyObject_GetAttrString(PyExc_Exception, "__init__");
    if (!parent_init) {
        goto error;
    }

    call_res = PyObject_CallFunction(parent_init, "Os", self, vdp_usb_result_to_str(res));
    if (!call_res) {
        goto error;
    }

    Py_DECREF(parent_init);

    return call_res;

error:
    Py_XDECREF(parent_init);
    Py_XDECREF(call_res);

    return NULL;
}

static PyMethodDef vdp_py_usb_error_methods[] =
{
    { "__init__", vdp_py_usb_error_init_obj, METH_VARARGS },
    { NULL, NULL }
};

void vdp_py_usb_error_init(PyObject* module)
{
    PyObject* dict = PyDict_New();
    PyMethodDef* methods = vdp_py_usb_error_methods;
    PyObject* s;
    int status;

    if (!dict) {
        return;
    }

    while (methods->ml_name) {
        /* get a wrapper for the built-in function */
        PyObject* func = PyCFunction_New(methods, NULL);
        PyObject* meth;
        if (!func) {
            goto error;
        }
        meth = PyMethod_New(func, NULL, vdp_py_usb_error);
        Py_DECREF(func);
        if (!meth) {
            goto error;
        }
        PyDict_SetItemString(dict, methods->ml_name, meth);
        Py_DECREF(meth);
        ++methods;
    }

    s = PyString_FromString(vdp_py_usb_error_doc);
    if (!s) {
        goto error;
    }
    status = PyDict_SetItemString(dict, "__doc__", s);
    Py_DECREF(s);
    if (status == -1) {
        goto error;
    }

    vdp_py_usb_error = PyErr_NewException("usb.Error",
        PyExc_Exception, dict);

    if (!vdp_py_usb_error) {
        goto error;
    }

    PyModule_AddObject(module, "Error", vdp_py_usb_error);

    return;

error:
    Py_DECREF(dict);
    return;
}

void vdp_py_usb_error_set(vdp_usb_result res)
{
    PyObject* obj = Py_BuildValue("i", res);
    if (obj) {
        PyErr_SetObject(vdp_py_usb_error, obj);
        Py_DECREF(obj);
    }
}
