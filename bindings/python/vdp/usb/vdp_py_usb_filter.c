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
#include "vdp/byte_order.h"

static int getint(const char* func, PyObject* obj, const char* name, int* res)
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
        PyErr_Format(PyExc_AttributeError, "%s: attribute '%s' not found", func, name);
        *res = 0;
        return 0;
    }

    if (!PyInt_Check(value_obj)) {
        Py_DECREF(value_obj);
        PyErr_Format(PyExc_TypeError, "%s: value of '%s' is not numeric", func, name);
        *res = 0;
        return 0;
    }

    value = PyInt_AsLong(value_obj);
    Py_DECREF(value_obj);

    return value;
}

static vdp_usb_urb_status ret_to_status(const char* fn_name, PyObject* ret)
{
    int status = PyInt_AsLong(ret);
    Py_DECREF(ret);

    if (!vdp_usb_urb_status_validate(status)) {
        PyErr_Format(PyExc_ValueError, "%s: invalid status value", fn_name);
        return vdp_usb_urb_status_stall;
    }

    return status;
}

static vdp_usb_urb_status vdp_py_usb_filter_get_device_descriptor(void* user_data,
    struct vdp_usb_device_descriptor* descriptor)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;
    int res = 1;

    if (!self->fn_get_device_descriptor) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "get_device_descriptor");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_get_device_descriptor, NULL);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (PyInt_Check(ret)) {
        return ret_to_status("get_device_descriptor", ret);
    }

    descriptor->bLength = sizeof(*descriptor);
    descriptor->bDescriptorType = getint("get_device_descriptor", ret, "bDescriptorType", &res);
    descriptor->bcdUSB = vdp_cpu_to_u16le(getint("get_device_descriptor", ret, "bcdUSB", &res));
    descriptor->bDeviceClass = getint("get_device_descriptor", ret, "bDeviceClass", &res);
    descriptor->bDeviceSubClass = getint("get_device_descriptor", ret, "bDeviceSubClass", &res);
    descriptor->bDeviceProtocol = getint("get_device_descriptor", ret, "bDeviceProtocol", &res);
    descriptor->bMaxPacketSize0 = getint("get_device_descriptor", ret, "bMaxPacketSize0", &res);
    descriptor->idVendor = vdp_cpu_to_u16le(getint("get_device_descriptor", ret, "idVendor", &res));
    descriptor->idProduct = vdp_cpu_to_u16le(getint("get_device_descriptor", ret, "idProduct", &res));
    descriptor->bcdDevice = vdp_cpu_to_u16le(getint("get_device_descriptor", ret, "bcdDevice", &res));
    descriptor->iManufacturer = getint("get_device_descriptor", ret, "iManufacturer", &res);
    descriptor->iProduct = getint("get_device_descriptor", ret, "iProduct", &res);
    descriptor->iSerialNumber = getint("get_device_descriptor", ret, "iSerialNumber", &res);
    descriptor->bNumConfigurations = getint("get_device_descriptor", ret, "bNumConfigurations", &res);

    Py_DECREF(ret);

    return res ? vdp_usb_urb_status_completed : vdp_usb_urb_status_stall;
}

static vdp_usb_urb_status vdp_py_usb_filter_get_qualifier_descriptor(void* user_data,
    struct vdp_usb_qualifier_descriptor* descriptor)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;
    int res = 1;

    if (!self->fn_get_qualifier_descriptor) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "get_qualifier_descriptor");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_get_qualifier_descriptor, NULL);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (PyInt_Check(ret)) {
        return ret_to_status("get_qualifier_descriptor", ret);
    }

    descriptor->bLength = sizeof(*descriptor);
    descriptor->bDescriptorType = getint("get_qualifier_descriptor", ret, "bDescriptorType", &res);
    descriptor->bcdUSB = vdp_cpu_to_u16le(getint("get_qualifier_descriptor", ret, "bcdUSB", &res));
    descriptor->bDeviceClass = getint("get_qualifier_descriptor", ret, "bDeviceClass", &res);
    descriptor->bDeviceSubClass = getint("get_qualifier_descriptor", ret, "bDeviceSubClass", &res);
    descriptor->bDeviceProtocol = getint("get_qualifier_descriptor", ret, "bDeviceProtocol", &res);
    descriptor->bMaxPacketSize0 = getint("get_qualifier_descriptor", ret, "bMaxPacketSize0", &res);
    descriptor->bNumConfigurations = getint("get_qualifier_descriptor", ret, "bNumConfigurations", &res);
    descriptor->bRESERVED = 0;

    Py_DECREF(ret);

    return res ? vdp_usb_urb_status_completed : vdp_usb_urb_status_stall;
}

static vdp_usb_urb_status vdp_py_usb_filter_get_config_descriptor(void* user_data,
    vdp_u8 index,
    struct vdp_usb_config_descriptor* descriptor,
    struct vdp_usb_descriptor_header*** other)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;
    PyObject* ret_descriptor = NULL;
    PyObject* ret_other = NULL;
    int res = 1;

    if (!self->fn_get_config_descriptor) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "get_config_descriptor");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_get_config_descriptor, "i", (int)index);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (PyInt_Check(ret)) {
        return ret_to_status("get_config_descriptor", ret);
    }

    if (PyTuple_Check(ret)) {
        if (!PyArg_ParseTuple(ret, "O|O:get_config_descriptor", &ret_descriptor, &ret_other)) {
            Py_DECREF(ret);
            return vdp_usb_urb_status_stall;
        }
    } else {
        ret_descriptor = ret;
    }

    if (ret_other) {
        Py_ssize_t cnt, i;

        if (!PyList_Check(ret_other)) {
            Py_DECREF(ret);
            PyErr_Format(PyExc_TypeError, "%s: value not a list", "get_config_descriptor");
            return vdp_usb_urb_status_stall;
        }

        cnt = PyList_Size(ret_other);

        if (cnt > 0) {
            for (i = 0; self->descriptors && self->descriptors[i]; ++i) {
                free(self->descriptors[i]);
            }
            free(self->descriptors);

            self->descriptors = malloc(sizeof(self->descriptors[0]) * (cnt + 1));

            for (i = 0; i < cnt; ++i) {
                PyObject* desc;
                int desc_type = 0;
                const char* desc_data = NULL;
                int desc_data_size = 0;

                self->descriptors[i] = NULL;

                desc = PyList_GetItem(ret_other, i);

                if (!PyTuple_Check(desc)) {
                    Py_DECREF(ret);
                    PyErr_Format(PyExc_TypeError, "%s: value not a tuple", "get_config_descriptor");
                    return vdp_usb_urb_status_stall;
                }

                if (!PyArg_ParseTuple(desc, "it#:get_config_descriptor", &desc_type, &desc_data, &desc_data_size)) {
                    Py_DECREF(ret);
                    return vdp_usb_urb_status_stall;
                }

                self->descriptors[i] = malloc(sizeof(*self->descriptors[0]) + desc_data_size);

                self->descriptors[i]->bDescriptorType = desc_type;
                self->descriptors[i]->bLength = sizeof(*self->descriptors[0]) + desc_data_size;
                memcpy(self->descriptors[i] + 1, desc_data, desc_data_size);
            }

            self->descriptors[i] = NULL;

            *other = self->descriptors;
        }
    }

    descriptor->bLength = sizeof(*descriptor);
    descriptor->bDescriptorType = getint("get_config_descriptor", ret_descriptor, "bDescriptorType", &res);
    descriptor->bNumInterfaces = getint("get_config_descriptor", ret_descriptor, "bNumInterfaces", &res);
    descriptor->bConfigurationValue = getint("get_config_descriptor", ret_descriptor, "bConfigurationValue", &res);
    descriptor->iConfiguration = getint("get_config_descriptor", ret_descriptor, "iConfiguration", &res);
    descriptor->bmAttributes = getint("get_config_descriptor", ret_descriptor, "bmAttributes", &res);
    descriptor->bMaxPower = getint("get_config_descriptor", ret_descriptor, "bMaxPower", &res);

    Py_DECREF(ret);

    return res ? vdp_usb_urb_status_completed : vdp_usb_urb_status_stall;
}

static vdp_usb_urb_status vdp_py_usb_filter_get_string_descriptor(void* user_data,
    const struct vdp_usb_string_table** tables)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;
    Py_ssize_t cnt, i, j;

    if (!self->fn_get_string_descriptor) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "get_string_descriptor");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_get_string_descriptor, NULL);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (PyInt_Check(ret)) {
        return ret_to_status("get_string_descriptor", ret);
    }

    if (!PyList_Check(ret)) {
        Py_DECREF(ret);
        PyErr_Format(PyExc_TypeError, "%s: value not a list", "get_string_descriptor");
        return vdp_usb_urb_status_stall;
    }

    cnt = PyList_Size(ret);

    if (cnt > 0) {
        for (i = 0; self->string_tables && self->string_tables[i].strings != NULL; ++i) {
            const struct vdp_usb_string* strings = self->string_tables[i].strings;
            for (j = 0; strings[j].str != NULL; ++j) {
                free((void*)strings[j].str);
            }
            free((void*)strings);
        }
        free(self->string_tables);

        self->string_tables = malloc(sizeof(self->string_tables[0]) * (cnt + 1));

        for (i = 0; i < cnt; ++i) {
            PyObject* table;
            int language_id = 0;
            PyObject* strings = NULL;
            Py_ssize_t strings_cnt;
            struct vdp_usb_string* tmp;

            self->string_tables[i].language_id = 0;
            self->string_tables[i].strings = NULL;

            table = PyList_GetItem(ret, i);

            if (!PyTuple_Check(table)) {
                Py_DECREF(ret);
                PyErr_Format(PyExc_TypeError, "%s: value not a tuple", "get_string_descriptor");
                return vdp_usb_urb_status_stall;
            }

            if (!PyArg_ParseTuple(table, "iO:get_string_descriptor", &language_id, &strings)) {
                Py_DECREF(ret);
                return vdp_usb_urb_status_stall;
            }

            if (!PyList_Check(strings)) {
                Py_DECREF(ret);
                PyErr_Format(PyExc_TypeError, "%s: value not a list", "get_string_descriptor");
                return vdp_usb_urb_status_stall;
            }

            strings_cnt = PyList_Size(strings);

            self->string_tables[i].language_id = language_id;
            self->string_tables[i].strings = malloc(sizeof(self->string_tables[i].strings[0]) * (strings_cnt + 1));

            self->string_tables[i + 1].language_id = 0;
            self->string_tables[i + 1].strings = NULL;

            for (j = 0; j < strings_cnt; ++j) {
                PyObject* str_obj;
                int index = 0;
                const char* str;
                tmp = (struct vdp_usb_string*)&self->string_tables[i].strings[j];

                tmp->index = 0;
                tmp->str = NULL;

                str_obj = PyList_GetItem(strings, j);

                if (!PyTuple_Check(str_obj)) {
                    Py_DECREF(ret);
                    PyErr_Format(PyExc_TypeError, "%s: value not a tuple", "get_string_descriptor");
                    return vdp_usb_urb_status_stall;
                }

                if (!PyArg_ParseTuple(str_obj, "is:get_string_descriptor", &index, &str)) {
                    Py_DECREF(ret);
                    return vdp_usb_urb_status_stall;
                }

                tmp->index = index;
                tmp->str = strdup(str);
            }

            tmp = (struct vdp_usb_string*)&self->string_tables[i].strings[j];

            tmp->index = 0;
            tmp->str = NULL;
        }

        self->string_tables[i].language_id = 0;
        self->string_tables[i].strings = NULL;

        *tables = self->string_tables;
    }

    Py_DECREF(ret);

    return vdp_usb_urb_status_completed;
}

static vdp_usb_urb_status vdp_py_usb_filter_set_address(void* user_data,
    vdp_u16 address)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;

    if (!self->fn_set_address) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "set_address");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_set_address, "i", (int)address);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (!PyInt_Check(ret)) {
        PyErr_SetString(PyExc_TypeError, "value is not numeric");
        Py_DECREF(ret);
        return vdp_usb_urb_status_stall;
    }

    return ret_to_status("set_address", ret);
}

static vdp_usb_urb_status vdp_py_usb_filter_set_configuration(void* user_data,
    vdp_u8 configuration)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;

    if (!self->fn_set_configuration) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "set_configuration");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_set_configuration, "i", (int)configuration);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (!PyInt_Check(ret)) {
        PyErr_SetString(PyExc_TypeError, "value is not numeric");
        Py_DECREF(ret);
        return vdp_usb_urb_status_stall;
    }

    return ret_to_status("set_configuration", ret);
}

static vdp_usb_urb_status vdp_py_usb_filter_get_status(void* user_data,
    vdp_u8 recipient, vdp_u8 index, vdp_u16* status)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;
    int ret_urb_status, ret_status = 0;

    if (!self->fn_get_status) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "get_status");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_get_status, "ii", (int)recipient, (int)index);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (PyInt_Check(ret)) {
        return ret_to_status("get_status", ret);
    }

    if (!PyTuple_Check(ret)) {
        Py_DECREF(ret);
        PyErr_Format(PyExc_TypeError, "%s: value not a tuple", "get_status");
        return vdp_usb_urb_status_stall;
    }

    if (!PyArg_ParseTuple(ret, "i|i:get_status", &ret_urb_status, &ret_status)) {
        Py_DECREF(ret);
        return vdp_usb_urb_status_stall;
    }

    if (!vdp_usb_urb_status_validate(ret_urb_status)) {
        Py_DECREF(ret);
        PyErr_Format(PyExc_ValueError, "%s: invalid status value", "get_status");
        return vdp_usb_urb_status_stall;
    }

    *status = ret_status;

    Py_DECREF(ret);

    return ret_urb_status;
}

static vdp_usb_urb_status vdp_py_usb_filter_enable_feature(void* user_data,
    vdp_u8 recipient, vdp_u8 index, vdp_u16 feature, int enable)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;

    if (!self->fn_enable_feature) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "enable_feature");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_enable_feature, "iiii", (int)recipient, (int)index, (int)feature, (int)enable);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (!PyInt_Check(ret)) {
        PyErr_SetString(PyExc_TypeError, "value is not numeric");
        Py_DECREF(ret);
        return vdp_usb_urb_status_stall;
    }

    return ret_to_status("enable_feature", ret);
}

static vdp_usb_urb_status vdp_py_usb_filter_get_interface(void* user_data,
    vdp_u8 interface, vdp_u8* alt_setting)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;
    int ret_urb_status, ret_alt_setting = 0;

    if (!self->fn_get_interface) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "get_interface");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_get_interface, "i", (int)interface);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (PyInt_Check(ret)) {
        return ret_to_status("get_interface", ret);
    }

    if (!PyTuple_Check(ret)) {
        Py_DECREF(ret);
        PyErr_Format(PyExc_TypeError, "%s: value not a tuple", "get_interface");
        return vdp_usb_urb_status_stall;
    }

    if (!PyArg_ParseTuple(ret, "i|i:get_interface", &ret_urb_status, &ret_alt_setting)) {
        Py_DECREF(ret);
        return vdp_usb_urb_status_stall;
    }

    if (!vdp_usb_urb_status_validate(ret_urb_status)) {
        Py_DECREF(ret);
        PyErr_Format(PyExc_ValueError, "%s: invalid status value", "get_interface");
        return vdp_usb_urb_status_stall;
    }

    *alt_setting = ret_alt_setting;

    Py_DECREF(ret);

    return ret_urb_status;
}

static vdp_usb_urb_status vdp_py_usb_filter_set_interface(void* user_data,
    vdp_u8 interface, vdp_u8 alt_setting)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;

    if (!self->fn_set_interface) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "set_interface");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_set_interface, "ii", (int)interface, (int)alt_setting);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (!PyInt_Check(ret)) {
        PyErr_SetString(PyExc_TypeError, "value is not numeric");
        Py_DECREF(ret);
        return vdp_usb_urb_status_stall;
    }

    return ret_to_status("set_interface", ret);
}

static vdp_usb_urb_status vdp_py_usb_filter_set_descriptor(void* user_data,
    vdp_u16 value, vdp_u16 index, const vdp_byte* data,
    vdp_u32 len)
{
    struct vdp_py_usb_filter* self = user_data;
    PyObject* ret;

    if (!self->fn_set_descriptor) {
        PyErr_Format(PyExc_AttributeError, "'%s' not found", "set_descriptor");
        return vdp_usb_urb_status_stall;
    }

    ret = PyObject_CallFunction(self->fn_set_descriptor, "iis#", (int)value, (int)index, data, (int)len);

    if (!ret) {
        return vdp_usb_urb_status_stall;
    }

    if (!PyInt_Check(ret)) {
        PyErr_SetString(PyExc_TypeError, "value is not numeric");
        Py_DECREF(ret);
        return vdp_usb_urb_status_stall;
    }

    return ret_to_status("set_descriptor", ret);
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
    int i, j;

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

    for (i = 0; self->descriptors && self->descriptors[i]; ++i) {
        free(self->descriptors[i]);
    }
    free(self->descriptors);

    for (i = 0; self->string_tables && self->string_tables[i].strings != NULL; ++i) {
        const struct vdp_usb_string* strings = self->string_tables[i].strings;
        for (j = 0; strings[j].str != NULL; ++j) {
            free((void*)strings[j].str);
        }
        free((void*)strings);
    }
    free(self->string_tables);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* vdp_py_usb_filter_filter(struct vdp_py_usb_filter* self, PyObject* args)
{
    PyObject* urb_obj = NULL;
    struct vdp_py_usb_urb* py_urb = NULL;
    struct vdp_py_usb_urb_wrapper* urb_wrapper;
    int res;

    if (!PyArg_ParseTuple(args, "O", &urb_obj)) {
        return NULL;
    }

    py_urb = vdp_py_usb_urb_check(urb_obj);
    if (!py_urb) {
        PyErr_SetString(PyExc_TypeError, "urb object is expected");
        return NULL;
    }

    urb_wrapper = (struct vdp_py_usb_urb_wrapper*)py_urb->urb_wrapper;

    res = vdp_usb_filter(urb_wrapper->urb, &vdp_py_usb_filter_ops, self);

    if (PyErr_Occurred()) {
        return NULL;
    }

    if (res) {
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
