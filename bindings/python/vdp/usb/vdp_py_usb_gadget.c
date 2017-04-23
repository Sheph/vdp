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
#include "vdp_py_usb_device.h"
#include "vdp_py_usb_error.h"
#include "vdp_py_usb_urb.h"
#include "vdp/usb_gadget.h"
#include "structmember.h"

static int getint(PyObject* obj, const char* name, int* res)
{
    PyObject* name_obj;
    PyObject* value_obj;
    int value;

    if (!*res) {
        return 0;
    }

    name_obj = PyString_FromString(name);
    value_obj = PyObject_GetItem(obj, name_obj);
    Py_DECREF(name_obj);
    if (!value_obj) {
        PyErr_Format(PyExc_AttributeError, "attribute '%s' not found", name);
        *res = 0;
        return 0;
    }

    if (!PyInt_Check(value_obj)) {
        Py_DECREF(value_obj);
        PyErr_Format(PyExc_TypeError, "value of '%s' is not numeric", name);
        *res = 0;
        return 0;
    }

    value = PyInt_AsLong(value_obj);
    Py_DECREF(value_obj);

    return value;
}

static int getenum(PyObject* obj, const char* name, int (*validate)(int), int* res)
{
    PyObject* name_obj;
    PyObject* value_obj;
    int value;

    if (!*res) {
        return 0;
    }

    name_obj = PyString_FromString(name);
    value_obj = PyObject_GetItem(obj, name_obj);
    Py_DECREF(name_obj);
    if (!value_obj) {
        PyErr_Format(PyExc_AttributeError, "attribute '%s' not found", name);
        *res = 0;
        return 0;
    }

    if (!PyInt_Check(value_obj)) {
        Py_DECREF(value_obj);
        PyErr_Format(PyExc_TypeError, "value of '%s' is not numeric", name);
        *res = 0;
        return 0;
    }

    value = PyInt_AsLong(value_obj);
    Py_DECREF(value_obj);

    if (!validate(value)) {
        PyErr_Format(PyExc_ValueError, "invalid '%s' value", name);
        *res = 0;
        return 0;
    }

    return value;
}

static void free_descriptors(struct vdp_usb_descriptor_header** descriptors)
{
    int i;

    for (i = 0; descriptors && descriptors[i]; ++i) {
        free(descriptors[i]);
    }
    free(descriptors);
}

static struct vdp_usb_descriptor_header** get_descriptors(PyObject* obj, const char* name, int* res)
{
    struct vdp_usb_descriptor_header** descriptors = NULL;
    Py_ssize_t cnt, i;
    PyObject* name_obj;
    PyObject* value_obj;

    if (!*res) {
        return NULL;
    }

    name_obj = PyString_FromString(name);
    value_obj = PyObject_GetItem(obj, name_obj);
    Py_DECREF(name_obj);
    if (!value_obj || (value_obj == Py_None)) {
        PyErr_Clear();
        Py_XDECREF(value_obj);
        return NULL;
    }

    if (!PyList_Check(value_obj)) {
        PyErr_Format(PyExc_TypeError, "value of '%s' is not a list", name);
        goto error;
    }

    cnt = PyList_Size(value_obj);

    if (cnt < 1) {
        Py_DECREF(value_obj);
        return NULL;
    }

    descriptors = malloc(sizeof(descriptors[0]) * (cnt + 1));

    for (i = 0; i < cnt; ++i) {
        PyObject* desc;
        int desc_type = 0;
        const char* desc_data = NULL;
        int desc_data_size = 0;

        descriptors[i] = NULL;

        desc = PyList_GetItem(value_obj, i);

        if (!PyTuple_Check(desc)) {
            PyErr_Format(PyExc_TypeError, "value of '%s' elements are not tuples", name);
            goto error;
        }

        if (!PyArg_ParseTuple(desc, "it#", &desc_type, &desc_data, &desc_data_size)) {
            goto error;
        }

        descriptors[i] = malloc(sizeof(*descriptors[0]) + desc_data_size);

        descriptors[i]->bDescriptorType = desc_type;
        descriptors[i]->bLength = sizeof(*descriptors[0]) + desc_data_size;
        memcpy(descriptors[i] + 1, desc_data, desc_data_size);
    }

    descriptors[i] = NULL;

    Py_DECREF(value_obj);

    return descriptors;

error:
    free_descriptors(descriptors);
    Py_DECREF(value_obj);
    *res = 0;

    return NULL;
}

static void free_string_tables(struct vdp_usb_string_table* string_tables)
{
    int i, j;

    for (i = 0; string_tables && string_tables[i].strings != NULL; ++i) {
        const struct vdp_usb_string* strings = string_tables[i].strings;
        for (j = 0; strings[j].str != NULL; ++j) {
            free((void*)strings[j].str);
        }
        free((void*)strings);
    }
    free(string_tables);
}

static struct vdp_usb_string_table* get_string_tables(PyObject* obj, const char* name, int* res)
{
    struct vdp_usb_string_table* string_tables = NULL;
    Py_ssize_t cnt, i, j;
    PyObject* name_obj;
    PyObject* value_obj;

    if (!*res) {
        return NULL;
    }

    name_obj = PyString_FromString(name);
    value_obj = PyObject_GetItem(obj, name_obj);
    Py_DECREF(name_obj);
    if (!value_obj || (value_obj == Py_None)) {
        PyErr_Clear();
        Py_XDECREF(value_obj);
        return NULL;
    }

    if (!PyList_Check(value_obj)) {
        PyErr_Format(PyExc_TypeError, "value of '%s' is not a list", name);
        goto error;
    }

    cnt = PyList_Size(value_obj);

    if (cnt < 1) {
        Py_DECREF(value_obj);
        return NULL;
    }

    string_tables = malloc(sizeof(string_tables[0]) * (cnt + 1));

    for (i = 0; i < cnt; ++i) {
        PyObject* table;
        int language_id = 0;
        PyObject* strings = NULL;
        Py_ssize_t strings_cnt;
        struct vdp_usb_string* tmp;

        string_tables[i].language_id = 0;
        string_tables[i].strings = NULL;

        table = PyList_GetItem(value_obj, i);

        if (!PyTuple_Check(table)) {
            PyErr_Format(PyExc_TypeError, "value of '%s' elements are not tuples", name);
            goto error;
        }

        if (!PyArg_ParseTuple(table, "iO", &language_id, &strings)) {
            goto error;
        }

        if (!PyList_Check(strings)) {
            PyErr_Format(PyExc_TypeError, "value of '%s' elements are not tuples of language id and strings", name);
            goto error;
        }

        strings_cnt = PyList_Size(strings);

        string_tables[i].language_id = language_id;
        string_tables[i].strings = malloc(sizeof(string_tables[i].strings[0]) * (strings_cnt + 1));

        string_tables[i + 1].language_id = 0;
        string_tables[i + 1].strings = NULL;

        for (j = 0; j < strings_cnt; ++j) {
            PyObject* str_obj;
            int index = 0;
            const char* str;
            tmp = (struct vdp_usb_string*)&string_tables[i].strings[j];

            tmp->index = 0;
            tmp->str = NULL;

            str_obj = PyList_GetItem(strings, j);

            if (!PyTuple_Check(str_obj)) {
                PyErr_Format(PyExc_TypeError, "value of '%s' strings elements are not tuples", name);
                goto error;
            }

            if (!PyArg_ParseTuple(str_obj, "is", &index, &str)) {
                goto error;
            }

            tmp->index = index;
            tmp->str = strdup(str);
        }

        tmp = (struct vdp_usb_string*)&string_tables[i].strings[j];

        tmp->index = 0;
        tmp->str = NULL;
    }

    string_tables[i].language_id = 0;
    string_tables[i].strings = NULL;

    Py_DECREF(value_obj);

    return string_tables;

error:
    free_string_tables(string_tables);
    Py_DECREF(value_obj);
    *res = 0;

    return NULL;
}

struct vdp_py_usb_gadget_wrapper
{
    PyObject_HEAD
    PyObject* gadget_obj; // doesn't hold a reference! just a
                          // pointer that gets nulled when gadget object
                          // is destroyed.
    struct vdp_usb_gadget* gadget;

    PyObject* current_device; // doesn't hold a reference, points to a
                              // device that currently calls 'event' function.
};

struct vdp_py_usb_gadget_request_wrapper
{
    PyObject_HEAD
    PyObject* gadget_wrapper;
    struct vdp_usb_gadget_request* request;
};

static void vdp_py_usb_gadget_request_wrapper_dealloc(struct vdp_py_usb_gadget_request_wrapper* self)
{
    self->request->destroy(self->request);
    Py_DECREF(self->gadget_wrapper);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyTypeObject vdp_py_usb_gadget_request_wrappertype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.gadget.RequestWrapper", /* tp_name */
    sizeof(struct vdp_py_usb_gadget_request_wrapper), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_gadget_request_wrapper_dealloc, /* tp_dealloc */
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
    "vdpusb gadget RequestWrapper", /* tp_doc */
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

struct vdp_py_usb_gadget_request
{
    PyObject_HEAD
    PyObject* device;
    PyObject* request_wrapper;
    int completed;

    PyObject* raw_setup_packet;
    PyObject* setup_packet;
    PyObject* iso_packet_list;
};

static Py_ssize_t vdp_py_usb_gadget_request_getreadbuf(struct vdp_py_usb_gadget_request* self, Py_ssize_t index, const void** ptr)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    if (index != 0) {
        PyErr_SetString(PyExc_SystemError, "accessing non-existent segment");
        return -1;
    }

    *ptr = request_wrapper->request->transfer_buffer;
    return request_wrapper->request->transfer_length;
}

static Py_ssize_t vdp_py_usb_gadget_request_getwritebuf(struct vdp_py_usb_gadget_request* self, Py_ssize_t index, const void** ptr)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    if (index != 0) {
        PyErr_SetString(PyExc_SystemError, "accessing non-existent segment");
        return -1;
    }

    if (!request_wrapper->request->in) {
        return -1;
    }

    *ptr = request_wrapper->request->transfer_buffer;
    return request_wrapper->request->transfer_length;
}

static Py_ssize_t vdp_py_usb_gadget_request_getsegcount(struct vdp_py_usb_gadget_request* self, Py_ssize_t* lenp)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    if (lenp) {
        *lenp = request_wrapper->request->transfer_length;
    }

    return 1;
}

static Py_ssize_t vdp_py_usb_gadget_request_getcharbuffer(struct vdp_py_usb_gadget_request* self, Py_ssize_t index, const void **ptr)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    if (index != 0) {
        PyErr_SetString(PyExc_SystemError, "accessing non-existent segment");
        return -1;
    }

    *ptr = request_wrapper->request->transfer_buffer;
    return request_wrapper->request->transfer_length;
}

static int vdp_py_usb_gadget_request_getbuffer(struct vdp_py_usb_gadget_request* self, Py_buffer* view, int flags)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    return PyBuffer_FillInfo(view, (PyObject*)self,
        request_wrapper->request->transfer_buffer, request_wrapper->request->transfer_length,
        !request_wrapper->request->in, flags);
}

static PyBufferProcs vdp_py_usb_gadget_request_as_buffer =
{
    (readbufferproc)vdp_py_usb_gadget_request_getreadbuf,
    (writebufferproc)vdp_py_usb_gadget_request_getwritebuf,
    (segcountproc)vdp_py_usb_gadget_request_getsegcount,
    (charbufferproc)vdp_py_usb_gadget_request_getcharbuffer,
    (getbufferproc)vdp_py_usb_gadget_request_getbuffer,
    0,
};

static void vdp_py_usb_gadget_request_dealloc(struct vdp_py_usb_gadget_request* self)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    if (!self->completed) {
        request_wrapper->request->status = vdp_usb_urb_status_unlinked;
        request_wrapper->request->complete(request_wrapper->request);
    }
    Py_DECREF(self->request_wrapper);
    Py_DECREF(self->device);
    Py_DECREF(self->raw_setup_packet);
    Py_DECREF(self->setup_packet);
    Py_DECREF(self->iso_packet_list);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_gadget_request_complete(struct vdp_py_usb_gadget_request* self)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    vdp_usb_result res = request_wrapper->request->complete(request_wrapper->request);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    self->completed = 1;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef vdp_py_usb_gadget_request_methods[] =
{
    { "complete", (PyCFunction)vdp_py_usb_gadget_request_complete, METH_NOARGS, "Complete the Request" },
    { NULL }
};

static PyMemberDef vdp_py_usb_gadget_request_members[] =
{
    { (char *)"raw_setup_packet", T_OBJECT_EX, offsetof(struct vdp_py_usb_gadget_request, raw_setup_packet), READONLY, (char *)"raw setup packet" },
    { (char *)"setup_packet", T_OBJECT_EX, offsetof(struct vdp_py_usb_gadget_request, setup_packet), READONLY, (char *)"setup packet" },
    { (char *)"iso_packets", T_OBJECT_EX, offsetof(struct vdp_py_usb_gadget_request, iso_packet_list), READONLY, (char *)"ISO packets" },
    { NULL }
};

static PyObject* vdp_py_usb_gadget_request_get_id(struct vdp_py_usb_gadget_request* self, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    return PyLong_FromLong(request_wrapper->request->id);
}

static PyObject* vdp_py_usb_gadget_request_get_is_in(struct vdp_py_usb_gadget_request* self, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    return PyLong_FromLong(request_wrapper->request->in);
}

static PyObject* vdp_py_usb_gadget_request_get_status(struct vdp_py_usb_gadget_request* self, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    return PyLong_FromLong(request_wrapper->request->status);
}

static int vdp_py_usb_gadget_request_set_status(struct vdp_py_usb_gadget_request* self, PyObject* value, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    if (value && PyInt_Check(value)) {
        int status = PyInt_AsLong(value);

        if (!vdp_usb_urb_status_validate(status)) {
            PyErr_SetString(PyExc_ValueError, "invalid status value");
            return -1;
        }

        request_wrapper->request->status = status;

        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "value is not numeric");
    return -1;
}

static PyObject* vdp_py_usb_gadget_request_get_transfer_length(struct vdp_py_usb_gadget_request* self, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    return PyLong_FromLong(request_wrapper->request->transfer_length);
}

static PyObject* vdp_py_usb_gadget_request_get_actual_length(struct vdp_py_usb_gadget_request* self, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    return PyLong_FromLong(request_wrapper->request->actual_length);
}

static int vdp_py_usb_gadget_request_set_actual_length(struct vdp_py_usb_gadget_request* self, PyObject* value, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    if (value && PyInt_Check(value)) {
        request_wrapper->request->actual_length = PyInt_AsLong(value);
        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "value is not numeric");
    return -1;
}

static PyObject* vdp_py_usb_gadget_request_get_interval_us(struct vdp_py_usb_gadget_request* self, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    return PyLong_FromLong(request_wrapper->request->interval_us);
}

static PyObject* vdp_py_usb_gadget_request_get_flags(struct vdp_py_usb_gadget_request* self, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    return PyLong_FromLong(request_wrapper->request->flags);
}

static PyObject* vdp_py_usb_gadget_request_get_number_of_packets(struct vdp_py_usb_gadget_request* self, void* closure)
{
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper =
        (struct vdp_py_usb_gadget_request_wrapper*)self->request_wrapper;

    return PyLong_FromLong(request_wrapper->request->number_of_packets);
}

static PyGetSetDef vdp_py_usb_gadget_request_getset[] =
{
    { "id", (getter)vdp_py_usb_gadget_request_get_id, NULL, "sequence id" },
    { "is_in", (getter)vdp_py_usb_gadget_request_get_is_in, NULL, "is_in" },
    { "status", (getter)vdp_py_usb_gadget_request_get_status, (setter)vdp_py_usb_gadget_request_set_status, "status" },
    { "transfer_length", (getter)vdp_py_usb_gadget_request_get_transfer_length, NULL, "transfer_length" },
    { "actual_length", (getter)vdp_py_usb_gadget_request_get_actual_length, (setter)vdp_py_usb_gadget_request_set_actual_length, "actual_length" },
    { "interval_us", (getter)vdp_py_usb_gadget_request_get_interval_us, NULL, "interval_us" },
    { "flags", (getter)vdp_py_usb_gadget_request_get_flags, NULL, "flags" },
    { "number_of_packets", (getter)vdp_py_usb_gadget_request_get_number_of_packets, NULL, "number_of_packets" },
    { NULL }
};

static PyTypeObject vdp_py_usb_gadget_requesttype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.gadget.Request", /* tp_name */
    sizeof(struct vdp_py_usb_gadget_request), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_gadget_request_dealloc, /* tp_dealloc */
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
    &vdp_py_usb_gadget_request_as_buffer, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_NEWBUFFER, /* tp_flags */
    "vdpusb gadget Request", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    vdp_py_usb_gadget_request_methods, /* tp_methods */
    vdp_py_usb_gadget_request_members, /* tp_members */
    vdp_py_usb_gadget_request_getset, /* tp_getset */
};

struct vdp_py_usb_gadget_control_setup
{
    PyObject_HEAD
    struct vdp_usb_gadget_control_setup control_setup;
};

static void vdp_py_usb_gadget_control_setup_dealloc(struct vdp_py_usb_gadget_control_setup* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_gadget_control_setup_get_type(struct vdp_py_usb_gadget_control_setup* self, void* closure)
{
    return PyLong_FromLong(self->control_setup.type);
}

static PyObject* vdp_py_usb_gadget_control_setup_get_recipient(struct vdp_py_usb_gadget_control_setup* self, void* closure)
{
    return PyLong_FromLong(self->control_setup.recipient);
}

static PyObject* vdp_py_usb_gadget_control_setup_get_request(struct vdp_py_usb_gadget_control_setup* self, void* closure)
{
    return PyLong_FromLong(self->control_setup.request);
}

static PyObject* vdp_py_usb_gadget_control_setup_get_value(struct vdp_py_usb_gadget_control_setup* self, void* closure)
{
    return PyLong_FromLong(self->control_setup.value);
}

static PyObject* vdp_py_usb_gadget_control_setup_get_index(struct vdp_py_usb_gadget_control_setup* self, void* closure)
{
    return PyLong_FromLong(self->control_setup.index);
}

static PyGetSetDef vdp_py_usb_gadget_control_setup_getset[] =
{
    { "type", (getter)vdp_py_usb_gadget_control_setup_get_type, NULL, "type" },
    { "recipient", (getter)vdp_py_usb_gadget_control_setup_get_recipient, NULL, "recipient" },
    { "request", (getter)vdp_py_usb_gadget_control_setup_get_request, NULL, "request" },
    { "value", (getter)vdp_py_usb_gadget_control_setup_get_value, NULL, "value" },
    { "index", (getter)vdp_py_usb_gadget_control_setup_get_index, NULL, "index" },
    { NULL }
};

static PyTypeObject vdp_py_usb_gadget_control_setuptype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.gadget.ControlSetup", /* tp_name */
    sizeof(struct vdp_py_usb_gadget_control_setup), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_gadget_control_setup_dealloc, /* tp_dealloc */
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
    "vdpusb gadget ControlSetup", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    0, /* tp_members */
    vdp_py_usb_gadget_control_setup_getset, /* tp_getset */
};

static PyObject* vdp_py_usb_gadget_request_new(PyObject* device,
    PyObject* gadget_wrapper,
    struct vdp_usb_gadget_request* request)
{
    struct vdp_py_usb_gadget_request* self =
        (struct vdp_py_usb_gadget_request*)PyObject_New(struct vdp_py_usb_gadget_request, &vdp_py_usb_gadget_requesttype);
    struct vdp_usb_control_setup control_setup;
    struct vdp_py_usb_gadget_request_wrapper* request_wrapper;
    struct vdp_py_usb_gadget_control_setup* setup_packet;
    vdp_u32 i;

    Py_INCREF(device);
    self->device = device;
    request_wrapper = (struct vdp_py_usb_gadget_request_wrapper*)PyObject_New(struct vdp_py_usb_gadget_request_wrapper, &vdp_py_usb_gadget_request_wrappertype);
    Py_INCREF(gadget_wrapper);
    request_wrapper->gadget_wrapper = gadget_wrapper;
    request_wrapper->request = request;
    self->request_wrapper = (PyObject*)request_wrapper;
    self->completed = 0;

    if (request->raw_setup_packet) {
        memcpy(&control_setup, request->raw_setup_packet, sizeof(control_setup));
    } else {
        memset(&control_setup, 0, sizeof(control_setup));
    }

    self->raw_setup_packet = vdp_py_usb_control_setup_new(&control_setup);

    setup_packet =
        (struct vdp_py_usb_gadget_control_setup*)PyObject_New(struct vdp_py_usb_gadget_control_setup, &vdp_py_usb_gadget_control_setuptype);

    if (request->raw_setup_packet) {
        memcpy(&setup_packet->control_setup, &request->setup_packet, sizeof(setup_packet->control_setup));
    } else {
        memset(&setup_packet->control_setup, 0, sizeof(setup_packet->control_setup));
    }

    self->setup_packet = (PyObject*)setup_packet;

    self->iso_packet_list = PyList_New(0);

    for (i = 0; i < request->number_of_packets; ++i) {
        PyObject* iso_packet = vdp_py_usb_iso_packet_new((PyObject*)request_wrapper,
            &request->iso_packets[i], !request->in);
        PyList_Append(self->iso_packet_list, iso_packet);
        Py_DECREF(iso_packet);
    }

    return (PyObject*)self;
}

struct vdp_py_usb_gadget_ep
{
    PyObject_HEAD

    struct vdp_usb_gadget_ep_caps caps;

    PyObject* caps_obj;

    struct vdp_usb_gadget_ep* ep;

    struct vdp_py_usb_gadget_wrapper* gadget_wrapper;
};

static int vdp_py_usb_gadget_ep_init_obj(struct vdp_py_usb_gadget_ep* self, PyObject* args, PyObject* kwargs)
{
    PyObject* caps;
    int res = 1;

    if (!PyArg_ParseTuple(args, "O", &caps)) {
        return -1;
    }

    self->caps.address = getint(caps, "address", &res);
    self->caps.dir = getenum(caps, "dir", &vdp_usb_gadget_ep_dir_validate, &res);
    self->caps.type = getenum(caps, "type", &vdp_usb_gadget_ep_type_validate, &res);
    if (self->caps.type == vdp_usb_gadget_ep_iso) {
        self->caps.sync = getenum(caps, "sync", &vdp_usb_gadget_ep_sync_validate, &res);
        self->caps.usage = getenum(caps, "usage", &vdp_usb_gadget_ep_usage_validate, &res);
    }
    self->caps.max_packet_size = getint(caps, "max_packet_size", &res);
    self->caps.interval = getint(caps, "interval", &res);
    self->caps.descriptors = get_descriptors(caps, "descriptors", &res);

    if (!res) {
        return -1;
    }

    PyErr_Clear();

    Py_INCREF(caps);
    self->caps_obj = caps;

    return 0;
}

static void vdp_py_usb_gadget_ep_dealloc(struct vdp_py_usb_gadget_ep* self)
{
    free_descriptors(self->caps.descriptors);

    Py_XDECREF(self->caps_obj);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static void vdp_py_usb_gadget_ep_enable(struct vdp_usb_gadget_ep* ep, int value)
{
    struct vdp_py_usb_gadget_ep* self = ep->priv;
    PyObject* ret = PyObject_CallMethod((PyObject*)self, "enable", "i", (int)value);

    Py_XDECREF(ret);

    if (!ret) {
        PyErr_Clear();
    }
}

static void vdp_py_usb_gadget_ep_enqueue(struct vdp_usb_gadget_ep* ep, struct vdp_usb_gadget_request* request)
{
    struct vdp_py_usb_gadget_ep* self = ep->priv;
    PyObject* request_obj;
    PyObject* ret;

    assert(self->gadget_wrapper);
    assert(self->gadget_wrapper->current_device);

    request_obj = vdp_py_usb_gadget_request_new(self->gadget_wrapper->current_device,
        (PyObject*)self->gadget_wrapper,
        request);
    request->priv = request_obj;

    ret = PyObject_CallMethod((PyObject*)self, "enqueue", "O", request_obj);

    Py_DECREF(request_obj);

    Py_XDECREF(ret);

    if (!ret) {
        PyErr_Clear();
    }
}

static void vdp_py_usb_gadget_ep_dequeue(struct vdp_usb_gadget_ep* ep, struct vdp_usb_gadget_request* request)
{
    struct vdp_py_usb_gadget_ep* self = ep->priv;
    PyObject* request_obj;
    PyObject* ret;

    assert(request->priv);

    request_obj = request->priv;

    ret = PyObject_CallMethod((PyObject*)self, "dequeue", "O", request_obj);

    Py_XDECREF(ret);

    if (!ret) {
        PyErr_Clear();
    }
}

static vdp_usb_urb_status vdp_py_usb_gadget_ep_clear_stall(struct vdp_usb_gadget_ep* ep)
{
    return vdp_usb_urb_status_completed;
}

static void vdp_py_usb_gadget_ep_destroy(struct vdp_usb_gadget_ep* ep)
{
    struct vdp_py_usb_gadget_ep* self = ep->priv;

    self->ep = NULL;
    self->gadget_wrapper = NULL;
}

static struct vdp_usb_gadget_ep_ops vdp_py_usb_gadget_ep_ops =
{
    .enable = vdp_py_usb_gadget_ep_enable,
    .enqueue = vdp_py_usb_gadget_ep_enqueue,
    .dequeue = vdp_py_usb_gadget_ep_dequeue,
    .clear_stall = vdp_py_usb_gadget_ep_clear_stall,
    .destroy = vdp_py_usb_gadget_ep_destroy
};

static struct vdp_usb_gadget_ep* vdp_py_usb_gadget_ep_construct(struct vdp_py_usb_gadget_ep* self)
{
    struct vdp_usb_gadget_ep* ep;

    if (self->ep) {
        PyErr_SetString(PyExc_RuntimeError, "endpoint is already in use");
        return NULL;
    }

    ep = vdp_usb_gadget_ep_create(&self->caps, &vdp_py_usb_gadget_ep_ops, self);

    if (!ep) {
        PyErr_SetString(PyExc_MemoryError, "cannot create endpoint");
        return NULL;
    }

    return ep;
}

static PyTypeObject vdp_py_usb_gadget_eptype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.gadget.Endpoint", /* tp_name */
    sizeof(struct vdp_py_usb_gadget_ep), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_gadget_ep_dealloc, /* tp_dealloc */
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

static struct vdp_py_usb_gadget_ep* vdp_py_usb_gadget_ep_check(PyObject* obj)
{
    if (!PyObject_IsInstance(obj, (PyObject*)&vdp_py_usb_gadget_eptype)) {
        return NULL;
    }
    return (struct vdp_py_usb_gadget_ep*)obj;
}

struct vdp_py_usb_gadget_interface
{
    PyObject_HEAD

    struct vdp_usb_gadget_interface_caps caps;

    PyObject* endpoint_list;

    PyObject* caps_obj;

    struct vdp_usb_gadget_interface* interface;
};

static int vdp_py_usb_gadget_interface_init_obj(struct vdp_py_usb_gadget_interface* self, PyObject* args, PyObject* kwargs)
{
    PyObject* caps;
    PyObject* name_obj;
    PyObject* endpoints_obj;
    int res = 1;
    Py_ssize_t cnt, i;

    if (!PyArg_ParseTuple(args, "O", &caps)) {
        return -1;
    }

    self->caps.number = getint(caps, "number", &res);
    self->caps.alt_setting = getint(caps, "alt_setting", &res);
    self->caps.klass = getint(caps, "klass", &res);
    self->caps.subklass = getint(caps, "subklass", &res);
    self->caps.protocol = getint(caps, "protocol", &res);
    self->caps.description = getint(caps, "description", &res);
    self->caps.descriptors = get_descriptors(caps, "descriptors", &res);

    if (!res) {
        return -1;
    }

    self->endpoint_list = PyList_New(0);

    name_obj = PyString_FromString("endpoints");
    endpoints_obj = PyObject_GetItem(caps, name_obj);
    Py_DECREF(name_obj);
    if (endpoints_obj && (endpoints_obj != Py_None)) {
        if (!PyList_Check(endpoints_obj)) {
            PyErr_Format(PyExc_TypeError, "value of '%s' is not a list", "endpoints");
            Py_DECREF(endpoints_obj);
            return -1;
        }

        cnt = PyList_Size(endpoints_obj);

        for (i = 0; i < cnt; ++i) {
            struct vdp_py_usb_gadget_ep* ep;

            PyObject* ep_obj = PyList_GetItem(endpoints_obj, i);

            ep = vdp_py_usb_gadget_ep_check(ep_obj);
            if (!ep) {
                PyErr_Format(PyExc_TypeError, "value of '%s' elements are not endpoints", "endpoints");
                Py_DECREF(endpoints_obj);
                return -1;
            }

            PyList_Append(self->endpoint_list, ep_obj);
        }
    }
    Py_XDECREF(endpoints_obj);

    PyErr_Clear();

    Py_INCREF(caps);
    self->caps_obj = caps;

    return 0;
}

static void vdp_py_usb_gadget_interface_dealloc(struct vdp_py_usb_gadget_interface* self)
{
    free_descriptors(self->caps.descriptors);

    Py_XDECREF(self->caps_obj);

    Py_XDECREF(self->endpoint_list);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static void vdp_py_usb_gadget_interface_enable(struct vdp_usb_gadget_interface* interface, int value)
{
    struct vdp_py_usb_gadget_interface* self = interface->priv;
    PyObject* ret = PyObject_CallMethod((PyObject*)self, "enable", "i", (int)value);

    Py_XDECREF(ret);

    if (!ret) {
        PyErr_Clear();
    }
}

static void vdp_py_usb_gadget_interface_destroy(struct vdp_usb_gadget_interface* interface)
{
    struct vdp_py_usb_gadget_interface* self = interface->priv;

    self->interface = NULL;
}

static struct vdp_usb_gadget_interface_ops vdp_py_usb_gadget_interface_ops =
{
    .enable = vdp_py_usb_gadget_interface_enable,
    .destroy = vdp_py_usb_gadget_interface_destroy
};

static struct vdp_usb_gadget_interface* vdp_py_usb_gadget_interface_construct(struct vdp_py_usb_gadget_interface* self)
{
    struct vdp_usb_gadget_interface_caps caps;
    Py_ssize_t cnt, i;
    struct vdp_usb_gadget_interface* interface;

    if (self->interface) {
        PyErr_SetString(PyExc_RuntimeError, "interface is already in use");
        return NULL;
    }

    memcpy(&caps, &self->caps, sizeof(caps));

    cnt = PyList_Size(self->endpoint_list);

    caps.endpoints = malloc(sizeof(caps.endpoints[0]) * (cnt + 1));

    for (i = 0; i < cnt; ++i) {
        struct vdp_py_usb_gadget_ep* ep =
            vdp_py_usb_gadget_ep_check(PyList_GetItem(self->endpoint_list, i));

        caps.endpoints[i] = vdp_py_usb_gadget_ep_construct(ep);
        if (!caps.endpoints[i]) {
            for (--i; i >= 0; --i) {
                vdp_usb_gadget_ep_destroy(caps.endpoints[i]);
            }
            free(caps.endpoints);
            return NULL;
        }
    }

    caps.endpoints[i] = NULL;

    interface = vdp_usb_gadget_interface_create(&caps, &vdp_py_usb_gadget_interface_ops, self);

    if (!interface) {
        PyErr_SetString(PyExc_MemoryError, "cannot create interface");
        for (i = 0; i < cnt; ++i) {
            vdp_usb_gadget_ep_destroy(caps.endpoints[i]);
        }
        free(caps.endpoints);
        return NULL;
    }

    free(caps.endpoints);

    return interface;
}

static PyTypeObject vdp_py_usb_gadget_interfacetype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.gadget.Interface", /* tp_name */
    sizeof(struct vdp_py_usb_gadget_interface), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_gadget_interface_dealloc, /* tp_dealloc */
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
    "vdpusb gadget Interface", /* tp_doc */
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

static struct vdp_py_usb_gadget_interface* vdp_py_usb_gadget_interface_check(PyObject* obj)
{
    if (!PyObject_IsInstance(obj, (PyObject*)&vdp_py_usb_gadget_interfacetype)) {
        return NULL;
    }
    return (struct vdp_py_usb_gadget_interface*)obj;
}

struct vdp_py_usb_gadget_config
{
    PyObject_HEAD

    struct vdp_usb_gadget_config_caps caps;

    PyObject* interface_list;

    PyObject* caps_obj;

    struct vdp_usb_gadget_config* config;
};

static int vdp_py_usb_gadget_config_init_obj(struct vdp_py_usb_gadget_config* self, PyObject* args, PyObject* kwargs)
{
    PyObject* caps;
    PyObject* name_obj;
    PyObject* interfaces_obj;
    int res = 1;
    Py_ssize_t cnt, i;

    if (!PyArg_ParseTuple(args, "O", &caps)) {
        return -1;
    }

    self->caps.number = getint(caps, "number", &res);
    self->caps.attributes = getint(caps, "attributes", &res);
    self->caps.max_power = getint(caps, "max_power", &res);
    self->caps.description = getint(caps, "description", &res);
    self->caps.descriptors = get_descriptors(caps, "descriptors", &res);

    if (!res) {
        return -1;
    }

    self->interface_list = PyList_New(0);

    name_obj = PyString_FromString("interfaces");
    interfaces_obj = PyObject_GetItem(caps, name_obj);
    Py_DECREF(name_obj);
    if (interfaces_obj && (interfaces_obj != Py_None)) {
        if (!PyList_Check(interfaces_obj)) {
            PyErr_Format(PyExc_TypeError, "value of '%s' is not a list", "interfaces");
            Py_DECREF(interfaces_obj);
            return -1;
        }

        cnt = PyList_Size(interfaces_obj);

        for (i = 0; i < cnt; ++i) {
            struct vdp_py_usb_gadget_interface* interface;

            PyObject* interface_obj = PyList_GetItem(interfaces_obj, i);

            interface = vdp_py_usb_gadget_interface_check(interface_obj);
            if (!interface) {
                PyErr_Format(PyExc_TypeError, "value of '%s' elements are not interfaces", "interfaces");
                Py_DECREF(interfaces_obj);
                return -1;
            }

            PyList_Append(self->interface_list, interface_obj);
        }
    }
    Py_XDECREF(interfaces_obj);

    PyErr_Clear();

    Py_INCREF(caps);
    self->caps_obj = caps;

    return 0;
}

static void vdp_py_usb_gadget_config_dealloc(struct vdp_py_usb_gadget_config* self)
{
    free_descriptors(self->caps.descriptors);

    Py_XDECREF(self->caps_obj);

    Py_XDECREF(self->interface_list);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static void vdp_py_usb_gadget_config_enable(struct vdp_usb_gadget_config* config, int value)
{
    struct vdp_py_usb_gadget_config* self = config->priv;
    PyObject* ret = PyObject_CallMethod((PyObject*)self, "enable", "i", (int)value);

    Py_XDECREF(ret);

    if (!ret) {
        PyErr_Clear();
    }
}

static void vdp_py_usb_gadget_config_destroy(struct vdp_usb_gadget_config* config)
{
    struct vdp_py_usb_gadget_config* self = config->priv;

    self->config = NULL;
}

static struct vdp_usb_gadget_config_ops vdp_py_usb_gadget_config_ops =
{
    .enable = vdp_py_usb_gadget_config_enable,
    .destroy = vdp_py_usb_gadget_config_destroy
};

static struct vdp_usb_gadget_config* vdp_py_usb_gadget_config_construct(struct vdp_py_usb_gadget_config* self)
{
    struct vdp_usb_gadget_config_caps caps;
    Py_ssize_t cnt, i;
    struct vdp_usb_gadget_config* config;

    if (self->config) {
        PyErr_SetString(PyExc_RuntimeError, "config is already in use");
        return NULL;
    }

    memcpy(&caps, &self->caps, sizeof(caps));

    cnt = PyList_Size(self->interface_list);

    caps.interfaces = malloc(sizeof(caps.interfaces[0]) * (cnt + 1));

    for (i = 0; i < cnt; ++i) {
        struct vdp_py_usb_gadget_interface* interface =
            vdp_py_usb_gadget_interface_check(PyList_GetItem(self->interface_list, i));

        caps.interfaces[i] = vdp_py_usb_gadget_interface_construct(interface);
        if (!caps.interfaces[i]) {
            for (--i; i >= 0; --i) {
                vdp_usb_gadget_interface_destroy(caps.interfaces[i]);
            }
            free(caps.interfaces);
            return NULL;
        }
    }

    caps.interfaces[i] = NULL;

    config = vdp_usb_gadget_config_create(&caps, &vdp_py_usb_gadget_config_ops, self);

    if (!config) {
        PyErr_SetString(PyExc_MemoryError, "cannot create config");
        for (i = 0; i < cnt; ++i) {
            vdp_usb_gadget_interface_destroy(caps.interfaces[i]);
        }
        free(caps.interfaces);
        return NULL;
    }

    free(caps.interfaces);

    return config;
}

static PyTypeObject vdp_py_usb_gadget_configtype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.gadget.Config", /* tp_name */
    sizeof(struct vdp_py_usb_gadget_config), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_gadget_config_dealloc, /* tp_dealloc */
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
    "vdpusb gadget Config", /* tp_doc */
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

static struct vdp_py_usb_gadget_config* vdp_py_usb_gadget_config_check(PyObject* obj)
{
    if (!PyObject_IsInstance(obj, (PyObject*)&vdp_py_usb_gadget_configtype)) {
        return NULL;
    }
    return (struct vdp_py_usb_gadget_config*)obj;
}

static void vdp_py_usb_gadget_wrapper_dealloc(struct vdp_py_usb_gadget_wrapper* self)
{
    vdp_usb_gadget_destroy(self->gadget);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyTypeObject vdp_py_usb_gadget_wrappertype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.gadget.GadgetWrapper", /* tp_name */
    sizeof(struct vdp_py_usb_gadget_wrapper), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_gadget_wrapper_dealloc, /* tp_dealloc */
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
    "vdpusb gadget GadgetWrapper", /* tp_doc */
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

struct vdp_py_usb_gadget
{
    PyObject_HEAD

    PyObject* caps_obj;

    PyObject* gadget_wrapper;
};

static void vdp_py_usb_gadget_reset(struct vdp_usb_gadget* gadget, int start)
{
}

static void vdp_py_usb_gadget_power(struct vdp_usb_gadget* gadget, int on)
{
}

static void vdp_py_usb_gadget_set_address(struct vdp_usb_gadget* gadget, vdp_u32 address)
{
}

static void vdp_py_usb_gadget_destroy(struct vdp_usb_gadget* gadget)
{
    struct vdp_py_usb_gadget_wrapper* gadget_wrapper =
        (struct vdp_py_usb_gadget_wrapper*)gadget->priv;

    assert(!gadget_wrapper->gadget_obj);

    gadget_wrapper->gadget = NULL;
}

static struct vdp_usb_gadget_ops vdp_py_usb_gadget_ops =
{
    .reset = vdp_py_usb_gadget_reset,
    .power = vdp_py_usb_gadget_power,
    .set_address = vdp_py_usb_gadget_set_address,
    .destroy = vdp_py_usb_gadget_destroy
};

static int vdp_py_usb_gadget_init_obj(struct vdp_py_usb_gadget* self, PyObject* args, PyObject* kwargs)
{
    struct vdp_py_usb_gadget_ep* endpoint0;
    struct vdp_usb_gadget_caps gadget_caps;
    PyObject* caps;
    PyObject* name_obj;
    PyObject* configs_obj;
    PyObject* endpoint0_obj;
    int res = 1;
    Py_ssize_t cnt = 0, i, j, k;
    struct vdp_py_usb_gadget_wrapper* gadget_wrapper;

    if (!PyArg_ParseTuple(args, "O", &caps)) {
        return -1;
    }

    gadget_caps.bcd_usb = getint(caps, "bcd_usb", &res);
    gadget_caps.bcd_device = getint(caps, "bcd_device", &res);
    gadget_caps.klass = getint(caps, "klass", &res);
    gadget_caps.subklass = getint(caps, "subklass", &res);
    gadget_caps.protocol = getint(caps, "protocol", &res);
    gadget_caps.vendor_id = getint(caps, "vendor_id", &res);
    gadget_caps.product_id = getint(caps, "product_id", &res);
    gadget_caps.manufacturer = getint(caps, "manufacturer", &res);
    gadget_caps.product = getint(caps, "product", &res);
    gadget_caps.serial_number = getint(caps, "serial_number", &res);
    gadget_caps.string_tables = get_string_tables(caps, "string_tables", &res);

    if (!res) {
        return -1;
    }

    name_obj = PyString_FromString("configs");
    configs_obj = PyObject_GetItem(caps, name_obj);
    Py_DECREF(name_obj);
    if (configs_obj && (configs_obj != Py_None)) {
        if (!PyList_Check(configs_obj)) {
            PyErr_Format(PyExc_TypeError, "value of '%s' is not a list", "configs");
            Py_DECREF(configs_obj);
            goto fail0;
        }

        cnt = PyList_Size(configs_obj);

        for (i = 0; i < cnt; ++i) {
            struct vdp_py_usb_gadget_config* config;

            PyObject* config_obj = PyList_GetItem(configs_obj, i);

            config = vdp_py_usb_gadget_config_check(config_obj);
            if (!config) {
                PyErr_Format(PyExc_TypeError, "value of '%s' elements are not configs", "configs");
                Py_DECREF(configs_obj);
                goto fail0;
            }
        }
    }

    gadget_caps.configs = malloc(sizeof(gadget_caps.configs[0]) * (cnt + 1));

    for (i = 0; i < cnt; ++i) {
        struct vdp_py_usb_gadget_config* config =
            vdp_py_usb_gadget_config_check(PyList_GetItem(configs_obj, i));

        gadget_caps.configs[i] = vdp_py_usb_gadget_config_construct(config);
        if (!gadget_caps.configs[i]) {
            Py_XDECREF(configs_obj);
            for (--i; i >= 0; --i) {
                vdp_usb_gadget_config_destroy(gadget_caps.configs[i]);
            }
            free(gadget_caps.configs);
            goto fail0;
        }
    }

    gadget_caps.configs[i] = NULL;

    Py_XDECREF(configs_obj);

    name_obj = PyString_FromString("endpoint0");
    endpoint0_obj = PyObject_GetItem(caps, name_obj);
    Py_DECREF(name_obj);
    if (!endpoint0_obj) {
        PyErr_Format(PyExc_AttributeError, "attribute '%s' not found", "endpoint0");
        goto fail1;
    }

    endpoint0 = vdp_py_usb_gadget_ep_check(endpoint0_obj);
    if (!endpoint0) {
        PyErr_Format(PyExc_TypeError, "value of '%s' is not endpoint", "endpoint0");
        goto fail1;
    }

    gadget_caps.endpoint0 = vdp_py_usb_gadget_ep_construct(endpoint0);
    if (!gadget_caps.endpoint0) {
        goto fail1;
    }

    gadget_wrapper = (struct vdp_py_usb_gadget_wrapper*)PyObject_New(struct vdp_py_usb_gadget_wrapper, &vdp_py_usb_gadget_wrappertype);

    gadget_wrapper->gadget = vdp_usb_gadget_create(&gadget_caps, &vdp_py_usb_gadget_ops, gadget_wrapper);
    gadget_wrapper->gadget_obj = (PyObject*)self;

    if (!gadget_wrapper->gadget) {
        Py_DECREF(gadget_wrapper);
        PyErr_SetString(PyExc_MemoryError, "cannot create gadget");
        goto fail2;
    }

    free(gadget_caps.configs);
    free_string_tables(gadget_caps.string_tables);

    endpoint0->ep = gadget_wrapper->gadget->caps.endpoint0;
    endpoint0->gadget_wrapper = gadget_wrapper;

    for (i = 0; gadget_wrapper->gadget->caps.configs && gadget_wrapper->gadget->caps.configs[i]; ++i) {
        struct vdp_usb_gadget_config* cfg = gadget_wrapper->gadget->caps.configs[i];
        ((struct vdp_py_usb_gadget_config*)cfg->priv)->config = cfg;
        for (j = 0; cfg->caps.interfaces && cfg->caps.interfaces[j]; ++j) {
            struct vdp_usb_gadget_interface* iface = cfg->caps.interfaces[j];
            ((struct vdp_py_usb_gadget_interface*)iface->priv)->interface = iface;
            for (k = 0; iface->caps.endpoints && iface->caps.endpoints[k]; ++k) {
                ((struct vdp_py_usb_gadget_ep*)iface->caps.endpoints[k]->priv)->ep = iface->caps.endpoints[k];
                ((struct vdp_py_usb_gadget_ep*)iface->caps.endpoints[k]->priv)->gadget_wrapper = gadget_wrapper;
            }
        }
    }

    PyErr_Clear();

    Py_INCREF(caps);
    self->caps_obj = caps;
    self->gadget_wrapper = (PyObject*)gadget_wrapper;

    return 0;

fail2:
    vdp_usb_gadget_ep_destroy(gadget_caps.endpoint0);
fail1:
    for (i = 0; i < cnt; ++i) {
        vdp_usb_gadget_config_destroy(gadget_caps.configs[i]);
    }
    free(gadget_caps.configs);
fail0:
    free_string_tables(gadget_caps.string_tables);

    return -1;
}

static void vdp_py_usb_gadget_dealloc(struct vdp_py_usb_gadget* self)
{
    if (self->gadget_wrapper) {
        struct vdp_py_usb_gadget_wrapper* gadget_wrapper =
            (struct vdp_py_usb_gadget_wrapper*)self->gadget_wrapper;
        gadget_wrapper->gadget_obj = NULL;
        Py_DECREF(self->gadget_wrapper);
    }
    Py_XDECREF(self->caps_obj);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_gadget_mevent(struct vdp_py_usb_gadget* self, PyObject* args)
{
    struct vdp_py_usb_gadget_wrapper* gadget_wrapper =
        (struct vdp_py_usb_gadget_wrapper*)self->gadget_wrapper;
    PyObject* device_obj = NULL;
    struct vdp_py_usb_device* py_device = NULL;
    struct vdp_usb_event event;
    vdp_usb_result res;

    if (!PyArg_ParseTuple(args, "O", &device_obj)) {
        return NULL;
    }

    py_device = vdp_py_usb_device_check(device_obj);
    if (!py_device) {
        PyErr_SetString(PyExc_TypeError, "device object is expected");
        return NULL;
    }

    res = vdp_usb_device_get_event(py_device->device, &event);

    if (res != vdp_usb_success) {
        vdp_py_usb_error_set(res);
        return NULL;
    }

    gadget_wrapper->current_device = device_obj;

    vdp_usb_gadget_event(gadget_wrapper->gadget, &event);

    gadget_wrapper->current_device = NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef vdp_py_usb_gadget_methods[] =
{
    { "event", (PyCFunction)vdp_py_usb_gadget_mevent, METH_VARARGS, "Process gadget event" },
    { NULL }
};

static PyTypeObject vdp_py_usb_gadgettype =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "usb.gadget.Gadget", /* tp_name */
    sizeof(struct vdp_py_usb_gadget), /* tp_basicsize */
    0, /* tp_itemsize */
    (destructor)vdp_py_usb_gadget_dealloc, /* tp_dealloc */
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
    "vdpusb gadget Gadget", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    vdp_py_usb_gadget_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
};

static PyMethodDef vdp_py_usb_gadget_module_methods[] =
{
    { NULL }
};

void vdp_py_usb_gadget_init(PyObject* module)
{
    PyObject* module2 = Py_InitModule3("usb.gadget", vdp_py_usb_gadget_module_methods, "vdpusb gadget module");

    PyModule_AddIntConstant(module2, "REQUEST_STANDARD", vdp_usb_gadget_request_standard);
    PyModule_AddIntConstant(module2, "REQUEST_CLASS", vdp_usb_gadget_request_class);
    PyModule_AddIntConstant(module2, "REQUEST_VENDOR", vdp_usb_gadget_request_vendor);
    PyModule_AddIntConstant(module2, "REQUEST_RESERVED", vdp_usb_gadget_request_reserved);

    PyModule_AddIntConstant(module2, "REQUEST_DEVICE", vdp_usb_gadget_request_device);
    PyModule_AddIntConstant(module2, "REQUEST_INTERFACE", vdp_usb_gadget_request_interface);
    PyModule_AddIntConstant(module2, "REQUEST_ENDPOINT", vdp_usb_gadget_request_endpoint);
    PyModule_AddIntConstant(module2, "REQUEST_OTHER", vdp_usb_gadget_request_other);

    PyModule_AddIntConstant(module2, "REQUEST_ZERO_PACKET", vdp_usb_gadget_request_zero_packet);

    PyModule_AddIntConstant(module2, "EP_IN", vdp_usb_gadget_ep_in);
    PyModule_AddIntConstant(module2, "EP_OUT", vdp_usb_gadget_ep_out);
    PyModule_AddIntConstant(module2, "EP_INOUT", vdp_usb_gadget_ep_inout);

    PyModule_AddIntConstant(module2, "EP_CONTROL", vdp_usb_gadget_ep_control);
    PyModule_AddIntConstant(module2, "EP_ISO", vdp_usb_gadget_ep_iso);
    PyModule_AddIntConstant(module2, "EP_BULK", vdp_usb_gadget_ep_bulk);
    PyModule_AddIntConstant(module2, "EP_INT", vdp_usb_gadget_ep_int);

    PyModule_AddIntConstant(module2, "EP_SYNC_NONE", vdp_usb_gadget_ep_sync_none);
    PyModule_AddIntConstant(module2, "EP_SYNC_ASYNC", vdp_usb_gadget_ep_sync_async);
    PyModule_AddIntConstant(module2, "EP_SYNC_ADAPTIVE", vdp_usb_gadget_ep_sync_adaptive);
    PyModule_AddIntConstant(module2, "EP_SYNC_SYNC", vdp_usb_gadget_ep_sync_sync);

    PyModule_AddIntConstant(module2, "EP_USAGE_DATA", vdp_usb_gadget_ep_usage_data);
    PyModule_AddIntConstant(module2, "EP_USAGE_FEEDBACK", vdp_usb_gadget_ep_usage_feedback);
    PyModule_AddIntConstant(module2, "EP_USAGE_IMPLICIT_FB", vdp_usb_gadget_ep_usage_implicit_fb);

    PyModule_AddIntConstant(module2, "CONFIG_ATT_ONE", vdp_usb_gadget_config_att_one);
    PyModule_AddIntConstant(module2, "CONFIG_ATT_SELFPOWER", vdp_usb_gadget_config_att_selfpower);
    PyModule_AddIntConstant(module2, "CONFIG_ATT_WAKEUP", vdp_usb_gadget_config_att_wakeup);
    PyModule_AddIntConstant(module2, "CONFIG_ATT_BATTERY", vdp_usb_gadget_config_att_battery);

    if (PyType_Ready(&vdp_py_usb_gadget_request_wrappertype) < 0) {
        return;
    }

    if (PyType_Ready(&vdp_py_usb_gadget_requesttype) < 0) {
        return;
    }

    if (PyType_Ready(&vdp_py_usb_gadget_control_setuptype) < 0) {
        return;
    }

    vdp_py_usb_gadget_eptype.tp_new = PyType_GenericNew;
    vdp_py_usb_gadget_eptype.tp_init = (initproc)vdp_py_usb_gadget_ep_init_obj;
    if (PyType_Ready(&vdp_py_usb_gadget_eptype) < 0) {
        return;
    }

    vdp_py_usb_gadget_interfacetype.tp_new = PyType_GenericNew;
    vdp_py_usb_gadget_interfacetype.tp_init = (initproc)vdp_py_usb_gadget_interface_init_obj;
    if (PyType_Ready(&vdp_py_usb_gadget_interfacetype) < 0) {
        return;
    }

    vdp_py_usb_gadget_configtype.tp_new = PyType_GenericNew;
    vdp_py_usb_gadget_configtype.tp_init = (initproc)vdp_py_usb_gadget_config_init_obj;
    if (PyType_Ready(&vdp_py_usb_gadget_configtype) < 0) {
        return;
    }

    if (PyType_Ready(&vdp_py_usb_gadget_wrappertype) < 0) {
        return;
    }

    vdp_py_usb_gadgettype.tp_new = PyType_GenericNew;
    vdp_py_usb_gadgettype.tp_init = (initproc)vdp_py_usb_gadget_init_obj;
    if (PyType_Ready(&vdp_py_usb_gadgettype) < 0) {
        return;
    }

    Py_INCREF(&vdp_py_usb_gadget_request_wrappertype);
    PyModule_AddObject(module2, "RequestWrapper", (PyObject*)&vdp_py_usb_gadget_request_wrappertype);

    Py_INCREF(&vdp_py_usb_gadget_requesttype);
    PyModule_AddObject(module2, "Request", (PyObject*)&vdp_py_usb_gadget_requesttype);

    Py_INCREF(&vdp_py_usb_gadget_control_setuptype);
    PyModule_AddObject(module2, "ControlSetup", (PyObject*)&vdp_py_usb_gadget_control_setuptype);

    Py_INCREF(&vdp_py_usb_gadget_eptype);
    PyModule_AddObject(module2, "Endpoint", (PyObject*)&vdp_py_usb_gadget_eptype);

    Py_INCREF(&vdp_py_usb_gadget_interfacetype);
    PyModule_AddObject(module2, "Interface", (PyObject*)&vdp_py_usb_gadget_interfacetype);

    Py_INCREF(&vdp_py_usb_gadget_configtype);
    PyModule_AddObject(module2, "Config", (PyObject*)&vdp_py_usb_gadget_configtype);

    Py_INCREF(&vdp_py_usb_gadget_wrappertype);
    PyModule_AddObject(module2, "GadgetWrapper", (PyObject*)&vdp_py_usb_gadget_wrappertype);

    Py_INCREF(&vdp_py_usb_gadgettype);
    PyModule_AddObject(module2, "Gadget", (PyObject*)&vdp_py_usb_gadgettype);

    Py_INCREF(module2);
    PyModule_AddObject(module, "gadget", module2);
}
