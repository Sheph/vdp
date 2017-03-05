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

#include "vdp_py_usb_context.h"
#include "vdp_py_usb_error.h"
#include "vdp_py_usb_device.h"

static void vdp_py_usb_context_dealloc(struct vdp_py_usb_context* self)
{
    if (self->ctx) {
        vdp_usb_cleanup(self->ctx);
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static int vdp_py_usb_context_init_obj(struct vdp_py_usb_context* self, PyObject* args, PyObject* kwargs)
{
    vdp_usb_result res = vdp_usb_init(NULL, vdp_log_critical, &self->ctx);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return -1;
    }

    return 0;
}

static PyObject* vdp_py_usb_context_get_device_range(struct vdp_py_usb_context* self)
{
    vdp_u8 device_lower;
    vdp_u8 device_upper;

    vdp_usb_result res = vdp_usb_get_device_range(self->ctx, &device_lower, &device_upper);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    return Py_BuildValue("ii", (int)device_lower, (int)device_upper);
}

static PyObject* vdp_py_usb_context_open_device(struct vdp_py_usb_context* self, PyObject* args)
{
    int device_number;
    struct vdp_usb_device* device;

    if (!PyArg_ParseTuple(args, "i:open_device", &device_number)) {
        return NULL;
    }

    vdp_usb_result res = vdp_usb_device_open(self->ctx, device_number, &device);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    return vdp_py_usb_device_new((PyObject*)self, device);
}

static PyMethodDef vdp_py_usb_context_methods[] =
{
    { "get_device_range", (PyCFunction)vdp_py_usb_context_get_device_range, METH_NOARGS, "Get available device numbers" },
    { "open_device", (PyCFunction)vdp_py_usb_context_open_device, METH_VARARGS, "Open device" },
    { NULL }
};

static PyTypeObject vdp_py_usb_contexttype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.Context", /* tp_name */
    sizeof(struct vdp_py_usb_context), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_context_dealloc, /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT, /* tp_flags */
    "vdpusb Context", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    vdp_py_usb_context_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
};

void vdp_py_usb_context_init(PyObject* module)
{
    vdp_py_usb_contexttype.tp_new = PyType_GenericNew;
    vdp_py_usb_contexttype.tp_init = (initproc)vdp_py_usb_context_init_obj;
    if (PyType_Ready(&vdp_py_usb_contexttype) < 0) {
        return;
    }

    Py_INCREF(&vdp_py_usb_contexttype);
    PyModule_AddObject(module, "Context", (PyObject*)&vdp_py_usb_contexttype);
}

PyObject* vdp_py_usb_context_new(struct vdp_usb_context* ctx)
{
    return NULL;
}

struct vdp_py_usb_context* vdp_py_usb_context_check(PyObject* obj)
{
    return NULL;
}
