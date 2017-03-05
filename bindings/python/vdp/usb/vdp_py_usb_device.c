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

#include "vdp_py_usb_device.h"
#include "vdp_py_usb_error.h"
#include "vdp_py_usb_event.h"

static void vdp_py_usb_device_dealloc(struct vdp_py_usb_device* self)
{
    vdp_usb_device_close(self->device);
    Py_DECREF(self->ctx);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_device_attach(struct vdp_py_usb_device* self, PyObject* args)
{
    int speed;

    if (!PyArg_ParseTuple(args, "i:attach", &speed)) {
        return NULL;
    }

    if (!vdp_usb_speed_validate(speed)) {
        PyErr_SetString(PyExc_ValueError, "invalid speed value");
        return NULL;
    }

    vdp_usb_result res = vdp_usb_device_attach(self->device, speed);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* vdp_py_usb_device_detach(struct vdp_py_usb_device* self)
{
    vdp_usb_result res = vdp_usb_device_detach(self->device);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* vdp_py_usb_device_get_fd(struct vdp_py_usb_device* self)
{
    vdp_fd fd;

    vdp_usb_result res = vdp_usb_device_wait_event(self->device, &fd);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    return PyLong_FromLong(fd);
}

static PyObject* vdp_py_usb_device_get_event(struct vdp_py_usb_device* self)
{
    struct vdp_usb_event event;

    vdp_usb_result res = vdp_usb_device_get_event(self->device, &event);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    return vdp_py_usb_event_new((PyObject*)self, &event);
}

static PyMethodDef vdp_py_usb_device_methods[] =
{
    { "attach", (PyCFunction)vdp_py_usb_device_attach, METH_VARARGS, "Attach the device to port" },
    { "detach", (PyCFunction)vdp_py_usb_device_detach, METH_NOARGS, "Detach from port" },
    { "get_fd", (PyCFunction)vdp_py_usb_device_get_fd, METH_NOARGS, "Returns event fd" },
    { "get_event", (PyCFunction)vdp_py_usb_device_get_event, METH_NOARGS, "Returns event" },
    { NULL }
};

static PyTypeObject vdp_py_usb_devicetype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.Device", /* tp_name */
    sizeof(struct vdp_py_usb_device), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_device_dealloc, /* tp_dealloc */
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
    "vdpusb Device", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    vdp_py_usb_device_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
};

void vdp_py_usb_device_init(PyObject* module)
{
    if (PyType_Ready(&vdp_py_usb_devicetype) < 0) {
        return;
    }

    Py_INCREF(&vdp_py_usb_devicetype);
    PyModule_AddObject(module, "Device", (PyObject*)&vdp_py_usb_devicetype);
}

PyObject* vdp_py_usb_device_new(PyObject* ctx, struct vdp_usb_device* device)
{
    struct vdp_py_usb_device* self = (struct vdp_py_usb_device*)PyObject_New(struct vdp_py_usb_device, &vdp_py_usb_devicetype);

    Py_INCREF(ctx);
    self->ctx = ctx;
    self->device = device;

    return (PyObject*)self;
}

struct vdp_py_usb_device* vdp_py_usb_device_check(PyObject* obj)
{
    return NULL;
}
