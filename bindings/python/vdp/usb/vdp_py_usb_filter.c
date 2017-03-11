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

#include "vdp_py_usb_filter.h"
#include "vdp_py_usb_urb.h"

static vdp_usb_urb_status vdp_py_usb_filter_get_device_descriptor(void* user_data,
    struct vdp_usb_device_descriptor* descriptor)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_get_qualifier_descriptor(void* user_data,
    struct vdp_usb_qualifier_descriptor* descriptor)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_get_config_descriptor(void* user_data,
    vdp_u8 index,
    struct vdp_usb_config_descriptor* descriptor,
    struct vdp_usb_descriptor_header*** other)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_get_string_descriptor(void* user_data,
    const struct vdp_usb_string_table** tables)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_set_address(void* user_data,
    vdp_u16 address)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_set_configuration(void* user_data,
    vdp_u8 configuration)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_get_status(void* user_data,
    vdp_u8 recipient, vdp_u8 index, vdp_u16* status)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_enable_feature(void* user_data,
    vdp_u8 recipient, vdp_u8 index, vdp_u16 feature, int enable)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_get_interface(void* user_data,
    vdp_u8 interface, vdp_u8* alt_setting)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_set_interface(void* user_data,
    vdp_u8 interface, vdp_u8 alt_setting)
{
    return vdp_usb_urb_status_error;
}

static vdp_usb_urb_status vdp_py_usb_filter_set_descriptor(void* user_data,
    vdp_u16 value, vdp_u16 index, const vdp_byte* data,
    vdp_u32 len)
{
    return vdp_usb_urb_status_error;
}

static struct vdp_usb_filter_ops vdp_py_usb_filter_ops =
{
    .get_device_descriptor = vdp_py_usb_filter_get_device_descriptor,
    .get_qualifier_descriptor = vdp_py_usb_filter_get_qualifier_descriptor,
    .get_config_descriptor = vdp_py_usb_filter_get_config_descriptor,
    .get_string_descriptor = vdp_py_usb_filter_get_string_descriptor,
    .set_address = vdp_py_usb_filter_set_address,
    .set_configuration = vdp_py_usb_filter_set_configuration,
    .get_status = vdp_py_usb_filter_get_status,
    .enable_feature = vdp_py_usb_filter_enable_feature,
    .get_interface = vdp_py_usb_filter_get_interface,
    .set_interface = vdp_py_usb_filter_set_interface,
    .set_descriptor = vdp_py_usb_filter_set_descriptor
};

static int vdp_py_usb_filter_init_obj(struct vdp_py_usb_filter* self, PyObject* args, PyObject* kwargs)
{
    self->fn_get_device_descriptor = PyObject_GetAttrString((PyObject*)self, "get_device_descriptor");
    self->fn_get_qualifier_descriptor = PyObject_GetAttrString((PyObject*)self, "get_qualifier_descriptor");
    self->fn_get_config_descriptor = PyObject_GetAttrString((PyObject*)self, "get_config_descriptor");
    self->fn_get_string_descriptor = PyObject_GetAttrString((PyObject*)self, "get_string_descriptor");
    self->fn_set_address = PyObject_GetAttrString((PyObject*)self, "set_address");
    self->fn_set_configuration = PyObject_GetAttrString((PyObject*)self, "set_configuration");
    self->fn_get_status = PyObject_GetAttrString((PyObject*)self, "get_status");
    self->fn_enable_feature = PyObject_GetAttrString((PyObject*)self, "enable_feature");
    self->fn_get_interface = PyObject_GetAttrString((PyObject*)self, "get_interface");
    self->fn_set_interface = PyObject_GetAttrString((PyObject*)self, "set_interface");
    self->fn_set_descriptor = PyObject_GetAttrString((PyObject*)self, "set_descriptor");

    PyErr_Clear();

    return 0;
}

static void vdp_py_usb_filter_dealloc(struct vdp_py_usb_filter* self)
{
    Py_XDECREF(self->fn_get_device_descriptor);
    Py_XDECREF(self->fn_get_qualifier_descriptor);
    Py_XDECREF(self->fn_get_config_descriptor);
    Py_XDECREF(self->fn_get_string_descriptor);
    Py_XDECREF(self->fn_set_address);
    Py_XDECREF(self->fn_set_configuration);
    Py_XDECREF(self->fn_get_status);
    Py_XDECREF(self->fn_enable_feature);
    Py_XDECREF(self->fn_get_interface);
    Py_XDECREF(self->fn_set_interface);
    Py_XDECREF(self->fn_set_descriptor);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_filter_filter(struct vdp_py_usb_filter* self, PyObject* args)
{
    PyObject* urb_obj = NULL;
    struct vdp_py_usb_urb* py_urb = NULL;
    struct vdp_py_usb_urb_wrapper* urb_wrapper;

    if (!PyArg_ParseTuple(args, "O", &urb_obj)) {
        return NULL;
    }

    py_urb = vdp_py_usb_urb_check(urb_obj);
    if (!py_urb) {
        PyErr_SetString(PyExc_TypeError, "urb object is expected");
        return NULL;
    }

    urb_wrapper = (struct vdp_py_usb_urb_wrapper*)py_urb->urb_wrapper;

    if (vdp_usb_filter(urb_wrapper->urb, &vdp_py_usb_filter_ops, self)) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static PyMethodDef vdp_py_usb_filter_methods[] =
{
    { "filter", (PyCFunction)vdp_py_usb_filter_filter, METH_VARARGS, "Filter an URB" },
    { NULL }
};

static PyTypeObject vdp_py_usb_filtertype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.Filter", /* tp_name */
    sizeof(struct vdp_py_usb_filter), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_filter_dealloc, /* tp_dealloc */
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
    "vdpusb Filter", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    vdp_py_usb_filter_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
};

void vdp_py_usb_filter_init(PyObject* module)
{
    vdp_py_usb_filtertype.tp_new = PyType_GenericNew;
    vdp_py_usb_filtertype.tp_init = (initproc)vdp_py_usb_filter_init_obj;
    if (PyType_Ready(&vdp_py_usb_filtertype) < 0) {
        return;
    }

    Py_INCREF(&vdp_py_usb_filtertype);
    PyModule_AddObject(module, "Filter", (PyObject*)&vdp_py_usb_filtertype);
}
