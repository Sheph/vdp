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

#include "vdp_py_usb_urb.h"
#include "vdp_py_usb_error.h"

static void vdp_py_usb_urb_dealloc(struct vdp_py_usb_urb* self)
{
    vdp_usb_free_urb(self->urb);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_urb_complete(struct vdp_py_usb_urb* self)
{
    vdp_usb_result res = vdp_usb_complete_urb(self->urb);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef vdp_py_usb_urb_methods[] =
{
    { "complete", (PyCFunction)vdp_py_usb_urb_complete, METH_NOARGS, "Complete the URB" },
    { NULL }
};

static PyObject* vdp_py_usb_urb_get_id(struct vdp_py_usb_urb* self, void* closure)
{
    return PyLong_FromLong(self->urb->id);
}

static PyObject* vdp_py_usb_urb_get_type(struct vdp_py_usb_urb* self, void* closure)
{
    return PyLong_FromLong(self->urb->type);
}

static PyGetSetDef vdp_py_usb_urb_getset[] =
{
    { "id", (getter)vdp_py_usb_urb_get_id, NULL, "sequence id" },
    { "type", (getter)vdp_py_usb_urb_get_type, NULL, "type" },
    { NULL }
};

static PyTypeObject vdp_py_usb_urbtype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.URB", /* tp_name */
    sizeof(struct vdp_py_usb_urb), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_urb_dealloc, /* tp_dealloc */
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
    "vdpusb URB", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    vdp_py_usb_urb_methods, /* tp_methods */
    0, /* tp_members */
    vdp_py_usb_urb_getset, /* tp_getset */
};

void vdp_py_usb_urb_init(PyObject* module)
{
    if (PyType_Ready(&vdp_py_usb_urbtype) < 0) {
        return;
    }

    Py_INCREF(&vdp_py_usb_urbtype);
    PyModule_AddObject(module, "URB", (PyObject*)&vdp_py_usb_urbtype);
}

PyObject* vdp_py_usb_urb_new(struct vdp_usb_urb* urb)
{
    return NULL;
}

struct vdp_py_usb_urb* vdp_py_usb_urb_check(PyObject* obj)
{
    return NULL;
}
