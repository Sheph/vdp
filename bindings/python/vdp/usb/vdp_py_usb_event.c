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
#include "vdp_py_usb_urb.h"
#include "structmember.h"

static int vdp_py_usb_event_init_obj(struct vdp_py_usb_event* self, PyObject* args, PyObject* kwargs)
{
    self->event.type = vdp_usb_event_none;

    return 0;
}

static void vdp_py_usb_event_dealloc(struct vdp_py_usb_event* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_event_get_type(struct vdp_py_usb_event* self, void* closure)
{
    return PyLong_FromLong(self->event.type);
}

static PyGetSetDef vdp_py_usb_event_getset[] =
{
    { "type", (getter)vdp_py_usb_event_get_type, NULL, "type" },
    { NULL }
};

static PyTypeObject vdp_py_usb_eventtype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.Event", /* tp_name */
    sizeof(struct vdp_py_usb_event), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_event_dealloc, /* tp_dealloc */
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
    vdp_py_usb_event_getset, /* tp_getset */
};

static int vdp_py_usb_event_signal_init_obj(struct vdp_py_usb_event_common* self, PyObject* args, PyObject* kwargs)
{
    int sig;

    if (vdp_py_usb_eventtype.tp_init((PyObject*)self, args, kwargs) < 0) {
        return -1;
    }

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

static int vdp_py_usb_event_urb_init_obj(struct vdp_py_usb_event_urb* self, PyObject* args, PyObject* kwargs)
{
    PyObject* urb_obj = NULL;
    struct vdp_py_usb_urb* py_urb = NULL;
    struct vdp_py_usb_urb_wrapper* urb_wrapper;

    if (vdp_py_usb_eventtype.tp_init((PyObject*)self, args, kwargs) < 0) {
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O", &urb_obj)) {
        return -1;
    }

    py_urb = vdp_py_usb_urb_check(urb_obj);
    if (!py_urb) {
        PyErr_SetString(PyExc_TypeError, "urb object is expected");
        return -1;
    }

    urb_wrapper = (struct vdp_py_usb_urb_wrapper*)py_urb->urb_wrapper;

    self->base.event.type = vdp_usb_event_urb;
    self->base.event.data.urb = urb_wrapper->urb;

    Py_INCREF(urb_obj);
    self->urb = urb_obj;

    return 0;
}

static void vdp_py_usb_event_urb_dealloc(struct vdp_py_usb_event_urb* self)
{
    Py_XDECREF(self->urb);
    vdp_py_usb_eventtype.tp_dealloc((PyObject*)self);
}

static PyMemberDef vdp_py_usb_event_urb_members[] =
{
    { (char *)"urb", T_OBJECT_EX, offsetof(struct vdp_py_usb_event_urb, urb), READONLY, (char *)"urb" },
    { NULL }
};

static PyTypeObject vdp_py_usb_event_urbtype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.URBEvent", /* tp_name */
    sizeof(struct vdp_py_usb_event_urb), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_event_urb_dealloc, /* tp_dealloc */
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
    "vdpusb URBEvent", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    vdp_py_usb_event_urb_members, /* tp_members */
    0, /* tp_getset */
};

static int vdp_py_usb_event_unlink_urb_init_obj(struct vdp_py_usb_event_common* self, PyObject* args, PyObject* kwargs)
{
    vdp_u32 id;

    if (vdp_py_usb_eventtype.tp_init((PyObject*)self, args, kwargs) < 0) {
        return -1;
    }

    if (!PyArg_ParseTuple(args, "I", &id)) {
        return -1;
    }

    self->base.event.type = vdp_usb_event_unlink_urb;
    self->base.event.data.unlink_urb.id = id;

    return 0;
}

static PyObject* vdp_py_usb_event_unlink_urb_get_id(struct vdp_py_usb_event_common* self, void* closure)
{
    return PyLong_FromLong(self->base.event.data.unlink_urb.id);
}

static PyGetSetDef vdp_py_usb_event_unlink_urb_getset[] =
{
    { "id", (getter)vdp_py_usb_event_unlink_urb_get_id, NULL, "id" },
    { NULL }
};

static PyTypeObject vdp_py_usb_event_unlink_urbtype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.UnlinkURBEvent", /* tp_name */
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
    "vdpusb UnlinkURBEvent", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    0, /* tp_members */
    vdp_py_usb_event_unlink_urb_getset, /* tp_getset */
};

void vdp_py_usb_event_init(PyObject* module)
{
    vdp_py_usb_eventtype.tp_new = PyType_GenericNew;
    vdp_py_usb_eventtype.tp_init = (initproc)vdp_py_usb_event_init_obj;
    if (PyType_Ready(&vdp_py_usb_eventtype) < 0) {
        return;
    }

    vdp_py_usb_event_signaltype.tp_init = (initproc)vdp_py_usb_event_signal_init_obj;
    vdp_py_usb_event_signaltype.tp_base = &vdp_py_usb_eventtype;
    if (PyType_Ready(&vdp_py_usb_event_signaltype) < 0) {
        return;
    }

    vdp_py_usb_event_urbtype.tp_init = (initproc)vdp_py_usb_event_urb_init_obj;
    vdp_py_usb_event_urbtype.tp_base = &vdp_py_usb_eventtype;
    if (PyType_Ready(&vdp_py_usb_event_urbtype) < 0) {
        return;
    }

    vdp_py_usb_event_unlink_urbtype.tp_init = (initproc)vdp_py_usb_event_unlink_urb_init_obj;
    vdp_py_usb_event_unlink_urbtype.tp_base = &vdp_py_usb_eventtype;
    if (PyType_Ready(&vdp_py_usb_event_unlink_urbtype) < 0) {
        return;
    }

    Py_INCREF(&vdp_py_usb_eventtype);
    PyModule_AddObject(module, "Event", (PyObject*)&vdp_py_usb_eventtype);

    Py_INCREF(&vdp_py_usb_event_signaltype);
    PyModule_AddObject(module, "SignalEvent", (PyObject*)&vdp_py_usb_event_signaltype);

    Py_INCREF(&vdp_py_usb_event_urbtype);
    PyModule_AddObject(module, "URBEvent", (PyObject*)&vdp_py_usb_event_urbtype);

    Py_INCREF(&vdp_py_usb_event_unlink_urbtype);
    PyModule_AddObject(module, "UnlinkURBEvent", (PyObject*)&vdp_py_usb_event_unlink_urbtype);
}

PyObject* vdp_py_usb_event_new(PyObject* device, const struct vdp_usb_event* event)
{
    switch (event->type) {
    case vdp_usb_event_none: {
        struct vdp_py_usb_event* self = (struct vdp_py_usb_event*)PyObject_New(struct vdp_py_usb_event, &vdp_py_usb_eventtype);

        memcpy(&self->event, event, sizeof(self->event));

        return (PyObject*)self;
    }
    case vdp_usb_event_signal: {
        struct vdp_py_usb_event_common* self = (struct vdp_py_usb_event_common*)PyObject_New(struct vdp_py_usb_event_signal, &vdp_py_usb_event_signaltype);

        memcpy(&self->base.event, event, sizeof(self->base.event));

        return (PyObject*)self;
    }
    case vdp_usb_event_urb: {
        struct vdp_py_usb_event_urb* self = (struct vdp_py_usb_event_urb*)PyObject_New(struct vdp_py_usb_event_urb, &vdp_py_usb_event_urbtype);

        memcpy(&self->base.event, event, sizeof(self->base.event));
        self->urb = vdp_py_usb_urb_new(device, event->data.urb);

        return (PyObject*)self;
    }
    case vdp_usb_event_unlink_urb: {
        struct vdp_py_usb_event_common* self = (struct vdp_py_usb_event_common*)PyObject_New(struct vdp_py_usb_event_unlink_urb, &vdp_py_usb_event_unlink_urbtype);

        memcpy(&self->base.event, event, sizeof(self->base.event));

        return (PyObject*)self;
    }
    default:
        assert(0);
        return NULL;
    }
}

struct vdp_py_usb_event* vdp_py_usb_event_check(PyObject* obj)
{
    if (!PyObject_IsInstance(obj, (PyObject*)&vdp_py_usb_eventtype)) {
        return NULL;
    }
    return (struct vdp_py_usb_event*)obj;
}
