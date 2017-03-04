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

#include "vdp_py_usb_event.h"
#include "vdp_py_usb_error.h"

static void vdp_py_usb_event_base_dealloc(struct vdp_py_usb_event_base* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_event_base_get_type(struct vdp_py_usb_event_base* self, void* closure)
{
    return PyLong_FromLong(self->event.type);
}

static PyGetSetDef vdp_py_usb_event_base_getset[] =
{
    { "type", (getter)vdp_py_usb_event_base_get_type, NULL, "type" },
    { NULL }
};

static PyTypeObject vdp_py_usb_event_basetype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.Event", /* tp_name */
    sizeof(struct vdp_py_usb_event_base), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_event_base_dealloc, /* tp_dealloc */
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
    "vdpusb Event", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    0, /* tp_members */
    vdp_py_usb_event_base_getset, /* tp_getset */
};

static int vdp_py_usb_event_signal_init_obj(struct vdp_py_usb_event_common* self, PyObject* args, PyObject* kwargs)
{
    int sig;

    if (!PyArg_ParseTuple(args, "i", &sig)) {
        return -1;
    }

    if (!vdp_usb_signal_type_validate(sig)) {
        PyErr_SetString(PyExc_ValueError, "invalid signal value");
        return -1;
    }

    self->base.event.type = vdp_usb_event_signal;
    self->base.event.data.signal.type = sig;

    return 0;
}

static PyObject* vdp_py_usb_event_signal_get_signal(struct vdp_py_usb_event_common* self, void* closure)
{
    return PyLong_FromLong(self->base.event.data.signal.type);
}

static PyGetSetDef vdp_py_usb_event_signal_getset[] =
{
    { "signal", (getter)vdp_py_usb_event_signal_get_signal, NULL, "signal" },
    { NULL }
};

static PyTypeObject vdp_py_usb_event_signaltype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.SignalEvent", /* tp_name */
    sizeof(struct vdp_py_usb_event_common), /* tp_basicsize */
    0, /* tp_itemsize */
    0, /* tp_dealloc */
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
    "vdpusb SignalEvent", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    0, /* tp_members */
    vdp_py_usb_event_signal_getset, /* tp_getset */
};

void vdp_py_usb_event_init(PyObject* module)
{
    if (PyType_Ready(&vdp_py_usb_event_basetype) < 0) {
        return;
    }

    vdp_py_usb_event_signaltype.tp_new = PyType_GenericNew;
    vdp_py_usb_event_signaltype.tp_init = (initproc)vdp_py_usb_event_signal_init_obj;
    vdp_py_usb_event_signaltype.tp_base = &vdp_py_usb_event_basetype;
    if (PyType_Ready(&vdp_py_usb_event_signaltype) < 0) {
        return;
    }

    Py_INCREF(&vdp_py_usb_event_basetype);
    PyModule_AddObject(module, "Event", (PyObject*)&vdp_py_usb_event_basetype);

    Py_INCREF(&vdp_py_usb_event_signaltype);
    PyModule_AddObject(module, "SignalEvent", (PyObject*)&vdp_py_usb_event_signaltype);
}
