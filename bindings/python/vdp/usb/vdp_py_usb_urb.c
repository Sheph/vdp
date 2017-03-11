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
#include "vdp/byte_order.h"
#include "structmember.h"

struct vdp_py_usb_control_setup
{
    PyObject_HEAD
    struct vdp_usb_control_setup control_setup;
};

struct vdp_py_usb_iso_packet
{
    PyObject_HEAD
    PyObject* urb_wrapper;
    struct vdp_usb_iso_packet* iso_packet;
};

static void vdp_py_usb_urb_wrapper_dealloc(struct vdp_py_usb_urb_wrapper* self)
{
    vdp_usb_free_urb(self->urb);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyTypeObject vdp_py_usb_urb_wrappertype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.URBWrapper", /* tp_name */
    sizeof(struct vdp_py_usb_urb_wrapper), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_urb_wrapper_dealloc, /* tp_dealloc */
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
    "vdpusb URBWrapper", /* tp_doc */
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

static void vdp_py_usb_urb_dealloc(struct vdp_py_usb_urb* self)
{
    struct vdp_py_usb_urb_wrapper* urb_wrapper =
        (struct vdp_py_usb_urb_wrapper*)self->urb_wrapper;

    if (!self->completed) {
        urb_wrapper->urb->status = vdp_usb_urb_status_unlinked;
        vdp_usb_complete_urb(urb_wrapper->urb);
    }
    Py_DECREF(self->urb_wrapper);
    Py_DECREF(self->device);
    Py_DECREF(self->setup_packet);
    Py_DECREF(self->iso_packet_list);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_urb_complete(struct vdp_py_usb_urb* self)
{
    struct vdp_py_usb_urb_wrapper* urb_wrapper =
        (struct vdp_py_usb_urb_wrapper*)self->urb_wrapper;

    vdp_usb_result res = vdp_usb_complete_urb(urb_wrapper->urb);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    self->completed = 1;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef vdp_py_usb_urb_methods[] =
{
    { "complete", (PyCFunction)vdp_py_usb_urb_complete, METH_NOARGS, "Complete the URB" },
    { NULL }
};

static PyMemberDef vdp_py_usb_urb_members[] =
{
    { (char *)"setup_packet", T_OBJECT_EX, offsetof(struct vdp_py_usb_urb, setup_packet), READONLY, (char *)"setup packet" },
    { (char *)"iso_packets", T_OBJECT_EX, offsetof(struct vdp_py_usb_urb, iso_packet_list), READONLY, (char *)"ISO packets" },
    { NULL }
};

static PyObject* vdp_py_usb_urb_get_id(struct vdp_py_usb_urb* self, void* closure)
{
    struct vdp_py_usb_urb_wrapper* urb_wrapper =
        (struct vdp_py_usb_urb_wrapper*)self->urb_wrapper;

    return PyLong_FromLong(urb_wrapper->urb->id);
}

static PyObject* vdp_py_usb_urb_get_type(struct vdp_py_usb_urb* self, void* closure)
{
    struct vdp_py_usb_urb_wrapper* urb_wrapper =
        (struct vdp_py_usb_urb_wrapper*)self->urb_wrapper;

    return PyLong_FromLong(urb_wrapper->urb->type);
}

static PyObject* vdp_py_usb_urb_get_status(struct vdp_py_usb_urb* self, void* closure)
{
    struct vdp_py_usb_urb_wrapper* urb_wrapper =
        (struct vdp_py_usb_urb_wrapper*)self->urb_wrapper;

    return PyLong_FromLong(urb_wrapper->urb->status);
}

static int vdp_py_usb_urb_set_status(struct vdp_py_usb_urb* self, PyObject* value, void* closure)
{
    struct vdp_py_usb_urb_wrapper* urb_wrapper =
        (struct vdp_py_usb_urb_wrapper*)self->urb_wrapper;

    if (value && PyInt_Check(value)) {
        int status = PyInt_AsLong(value);

        if (!vdp_usb_urb_status_validate(status)) {
            PyErr_SetString(PyExc_ValueError, "invalid status value");
            return -1;
        }

        urb_wrapper->urb->status = status;

        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "value is not numeric");
    return -1;
}

static PyGetSetDef vdp_py_usb_urb_getset[] =
{
    { "id", (getter)vdp_py_usb_urb_get_id, NULL, "sequence id" },
    { "type", (getter)vdp_py_usb_urb_get_type, NULL, "type" },
    { "status", (getter)vdp_py_usb_urb_get_status, (setter)vdp_py_usb_urb_set_status, "status" },
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
    vdp_py_usb_urb_members, /* tp_members */
    vdp_py_usb_urb_getset, /* tp_getset */
};

static void vdp_py_usb_control_setup_dealloc(struct vdp_py_usb_control_setup* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_control_setup_get_request_type(struct vdp_py_usb_control_setup* self, void* closure)
{
    return PyLong_FromLong(self->control_setup.bRequestType);
}

static PyObject* vdp_py_usb_control_setup_get_request(struct vdp_py_usb_control_setup* self, void* closure)
{
    return PyLong_FromLong(self->control_setup.bRequest);
}

static PyObject* vdp_py_usb_control_setup_get_value(struct vdp_py_usb_control_setup* self, void* closure)
{
    return PyLong_FromLong(vdp_u16le_to_cpu(self->control_setup.wValue));
}

static PyObject* vdp_py_usb_control_setup_get_index(struct vdp_py_usb_control_setup* self, void* closure)
{
    return PyLong_FromLong(vdp_u16le_to_cpu(self->control_setup.wIndex));
}

static PyGetSetDef vdp_py_usb_control_setup_getset[] =
{
    { "request_type", (getter)vdp_py_usb_control_setup_get_request_type, NULL, "request type" },
    { "request", (getter)vdp_py_usb_control_setup_get_request, NULL, "request" },
    { "value", (getter)vdp_py_usb_control_setup_get_value, NULL, "value" },
    { "index", (getter)vdp_py_usb_control_setup_get_index, NULL, "index" },
    { NULL }
};

static PyTypeObject vdp_py_usb_control_setuptype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.ControlSetup", /* tp_name */
    sizeof(struct vdp_py_usb_control_setup), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_control_setup_dealloc, /* tp_dealloc */
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
    "vdpusb ControlSetup", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    0, /* tp_members */
    vdp_py_usb_control_setup_getset, /* tp_getset */
};

static void vdp_py_usb_iso_packet_dealloc(struct vdp_py_usb_iso_packet* self)
{
    Py_DECREF(self->urb_wrapper);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_iso_packet_get_status(struct vdp_py_usb_iso_packet* self, void* closure)
{
    return PyLong_FromLong(self->iso_packet->status);
}

static int vdp_py_usb_iso_packet_set_status(struct vdp_py_usb_iso_packet* self, PyObject* value, void* closure)
{
    if (value && PyInt_Check(value)) {
        int status = PyInt_AsLong(value);

        if (!vdp_usb_urb_status_validate(status)) {
            PyErr_SetString(PyExc_ValueError, "invalid status value");
            return -1;
        }

        self->iso_packet->status = status;

        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "value is not numeric");
    return -1;
}

static PyGetSetDef vdp_py_usb_iso_packet_getset[] =
{
    { "status", (getter)vdp_py_usb_iso_packet_get_status, (setter)vdp_py_usb_iso_packet_set_status, "status" },
    { NULL }
};

static PyTypeObject vdp_py_usb_iso_packettype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.ISOPacket", /* tp_name */
    sizeof(struct vdp_py_usb_iso_packet), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_iso_packet_dealloc, /* tp_dealloc */
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
    "vdpusb ISOPacket", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    0, /* tp_members */
    vdp_py_usb_iso_packet_getset, /* tp_getset */
};

void vdp_py_usb_urb_init(PyObject* module)
{
    if (PyType_Ready(&vdp_py_usb_urb_wrappertype) < 0) {
        return;
    }

    if (PyType_Ready(&vdp_py_usb_urbtype) < 0) {
        return;
    }

    if (PyType_Ready(&vdp_py_usb_control_setuptype) < 0) {
        return;
    }

    if (PyType_Ready(&vdp_py_usb_iso_packettype) < 0) {
        return;
    }

    Py_INCREF(&vdp_py_usb_urb_wrappertype);
    PyModule_AddObject(module, "URBWrapper", (PyObject*)&vdp_py_usb_urb_wrappertype);

    Py_INCREF(&vdp_py_usb_urbtype);
    PyModule_AddObject(module, "URB", (PyObject*)&vdp_py_usb_urbtype);

    Py_INCREF(&vdp_py_usb_control_setuptype);
    PyModule_AddObject(module, "ControlSetup", (PyObject*)&vdp_py_usb_control_setuptype);

    Py_INCREF(&vdp_py_usb_iso_packettype);
    PyModule_AddObject(module, "ISOPacket", (PyObject*)&vdp_py_usb_iso_packettype);
}

PyObject* vdp_py_usb_urb_new(PyObject* device, struct vdp_usb_urb* urb)
{
    struct vdp_py_usb_urb* self = (struct vdp_py_usb_urb*)PyObject_New(struct vdp_py_usb_urb, &vdp_py_usb_urbtype);
    struct vdp_py_usb_control_setup* setup_packet;
    struct vdp_py_usb_urb_wrapper* urb_wrapper;
    vdp_u32 i;

    Py_INCREF(device);
    self->device = device;
    urb_wrapper = (struct vdp_py_usb_urb_wrapper*)PyObject_New(struct vdp_py_usb_urb_wrapper, &vdp_py_usb_urb_wrappertype);
    urb_wrapper->urb = urb;
    self->urb_wrapper = (PyObject*)urb_wrapper;
    self->completed = 0;

    setup_packet = (struct vdp_py_usb_control_setup*)PyObject_New(struct vdp_py_usb_control_setup, &vdp_py_usb_control_setuptype);
    self->setup_packet = (PyObject*)setup_packet;

    if (urb->type == vdp_usb_urb_control) {
        memcpy(&setup_packet->control_setup,
            urb->setup_packet, sizeof(setup_packet->control_setup));
    } else {
        memset(&setup_packet->control_setup,
            0, sizeof(setup_packet->control_setup));
    }

    self->iso_packet_list = PyList_New(0);

    if (urb->type == vdp_usb_urb_iso) {
        for (i = 0; i < urb->number_of_packets; ++i) {
            struct vdp_py_usb_iso_packet* iso_packet =
                (struct vdp_py_usb_iso_packet*)PyObject_New(struct vdp_py_usb_iso_packet, &vdp_py_usb_iso_packettype);

            Py_INCREF((PyObject*)urb_wrapper);
            iso_packet->urb_wrapper = (PyObject*)urb_wrapper;
            iso_packet->iso_packet = &urb->iso_packets[i];

            PyList_Append(self->iso_packet_list, (PyObject*)iso_packet);
            Py_DECREF((PyObject*)iso_packet);
        }
    }

    return (PyObject*)self;
}

struct vdp_py_usb_urb* vdp_py_usb_urb_check(PyObject* obj)
{
    if (!PyObject_IsInstance(obj, (PyObject*)&vdp_py_usb_urbtype)) {
        return NULL;
    }
    return (struct vdp_py_usb_urb*)obj;
}
