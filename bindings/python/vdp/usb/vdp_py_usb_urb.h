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

#ifndef _VDP_PY_USB_URB_H_
#define _VDP_PY_USB_URB_H_

#include <Python.h>
#include "vdp/usb.h"

struct vdp_py_usb_urb_wrapper
{
    PyObject_HEAD
    struct vdp_usb_urb* urb;
};

struct vdp_py_usb_control_setup
{
    PyObject_HEAD
    struct vdp_usb_control_setup control_setup;
};

struct vdp_py_usb_iso_packet
{
    PyObject_HEAD
    PyObject* wrapper;
    struct vdp_usb_iso_packet* iso_packet;
    int readonly;
};

struct vdp_py_usb_urb
{
    PyObject_HEAD
    PyObject* device;
    PyObject* urb_wrapper;
    int completed;

    PyObject* setup_packet;
    PyObject* iso_packet_list;
};

void vdp_py_usb_urb_init(PyObject* module);

PyObject* vdp_py_usb_control_setup_new(const struct vdp_usb_control_setup* control_setup);

PyObject* vdp_py_usb_iso_packet_new(PyObject* wrapper,
    struct vdp_usb_iso_packet* packet, int readonly);

PyObject* vdp_py_usb_urb_new(PyObject* device, struct vdp_usb_urb* urb);
struct vdp_py_usb_urb* vdp_py_usb_urb_check(PyObject* obj);

#endif
