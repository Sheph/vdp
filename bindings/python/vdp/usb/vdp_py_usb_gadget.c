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

#include "vdp_py_usb_gadget.h"
#include "vdp/usb_gadget.h"

struct vdp_py_usb_gadget_endpoint
{
    PyObject_HEAD
    PyObject* fn_enable;
    PyObject* fn_enqueue;
    PyObject* fn_dequeue;
    PyObject* fn_clear_stall;
    PyObject* fn_destroy;
};

static int vdp_py_usb_gadget_endpoint_init_obj(struct vdp_py_usb_gadget_endpoint* self, PyObject* args, PyObject* kwargs)
{
    self->fn_enable = PyObject_GetAttrString((PyObject*)self, "enable");
    self->fn_enqueue = PyObject_GetAttrString((PyObject*)self, "enqueue");
    self->fn_dequeue = PyObject_GetAttrString((PyObject*)self, "dequeue");
    self->fn_clear_stall = PyObject_GetAttrString((PyObject*)self, "clear_stall");
    self->fn_destroy = PyObject_GetAttrString((PyObject*)self, "destroy");

    PyErr_Clear();

    return 0;
}

static void vdp_py_usb_gadget_endpoint_dealloc(struct vdp_py_usb_gadget_endpoint* self)
{
    Py_XDECREF(self->fn_enable);
    Py_XDECREF(self->fn_enqueue);
    Py_XDECREF(self->fn_dequeue);
    Py_XDECREF(self->fn_clear_stall);
    Py_XDECREF(self->fn_destroy);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyTypeObject vdp_py_usb_gadget_endpointtype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.gadget.Endpoint", /* tp_name */
    sizeof(struct vdp_py_usb_gadget_endpoint), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_gadget_endpoint_dealloc, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_reserved */
    0, /* tp_repr */
    0, /* tp_as_number */
    0, /* tp_as_sequence */
    0, /* tp_as_mapping */
    0, /* tp_hash  */
    0, /* tp_call */
    0, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "vdpusb gadget Endpoint", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
};

static PyMethodDef vdp_py_usb_gadget_methods[] =
{
    { NULL }
};

void vdp_py_usb_gadget_init(PyObject* module)
{
    PyObject* module2 = Py_InitModule3("usb.gadget", vdp_py_usb_gadget_methods, "vdpusb gadget module");

    vdp_py_usb_gadget_endpointtype.tp_new = PyType_GenericNew;
    vdp_py_usb_gadget_endpointtype.tp_init = (initproc)vdp_py_usb_gadget_endpoint_init_obj;
    if (PyType_Ready(&vdp_py_usb_gadget_endpointtype) < 0) {
        return;
    }

    Py_INCREF(&vdp_py_usb_gadget_endpointtype);
    PyModule_AddObject(module2, "Endpoint", (PyObject*)&vdp_py_usb_gadget_endpointtype);

    Py_INCREF(module2);
    PyModule_AddObject(module, "gadget", module2);
}
