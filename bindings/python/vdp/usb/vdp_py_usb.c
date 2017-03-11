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

#include "vdp_py_usb_error.h"
#include "vdp_py_usb_urb.h"
#include "vdp_py_usb_context.h"
#include "vdp_py_usb_device.h"
#include "vdp_py_usb_event.h"
#include "vdp_py_usb_filter.h"
#include "vdp/usb_hid.h"

static PyMethodDef vdp_py_usb_methods[] =
{
    { NULL }
};

#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC initusb(void)
{
    PyObject* module = Py_InitModule3("usb", vdp_py_usb_methods, "vdpusb module");

    PyModule_AddIntConstant(module, "SUCCESS", vdp_usb_success);
    PyModule_AddIntConstant(module, "NOMEM", vdp_usb_nomem);
    PyModule_AddIntConstant(module, "MISUSE", vdp_usb_misuse);
    PyModule_AddIntConstant(module, "UNKNOWN", vdp_usb_unknown);
    PyModule_AddIntConstant(module, "NOT_FOUND", vdp_usb_not_found);
    PyModule_AddIntConstant(module, "BUSY", vdp_usb_busy);
    PyModule_AddIntConstant(module, "PROTOCOL_ERROR", vdp_usb_protocol_error);

    PyModule_AddIntConstant(module, "SPEED_LOW", vdp_usb_speed_low);
    PyModule_AddIntConstant(module, "SPEED_FULL", vdp_usb_speed_full);
    PyModule_AddIntConstant(module, "SPEED_HIGH", vdp_usb_speed_high);

    PyModule_AddIntConstant(module, "SIGNAL_RESET_START", vdp_usb_signal_reset_start);
    PyModule_AddIntConstant(module, "SIGNAL_RESET_END", vdp_usb_signal_reset_end);
    PyModule_AddIntConstant(module, "SIGNAL_POWER_ON", vdp_usb_signal_power_on);
    PyModule_AddIntConstant(module, "SIGNAL_POWER_OFF", vdp_usb_signal_power_off);

    PyModule_AddIntConstant(module, "URB_CONTROL", vdp_usb_urb_control);
    PyModule_AddIntConstant(module, "URB_ISO", vdp_usb_urb_iso);
    PyModule_AddIntConstant(module, "URB_BULK", vdp_usb_urb_bulk);
    PyModule_AddIntConstant(module, "URB_INT", vdp_usb_urb_int);

    PyModule_AddIntConstant(module, "URB_STATUS_UNDEFINED", vdp_usb_urb_status_undefined);
    PyModule_AddIntConstant(module, "URB_STATUS_COMPLETED", vdp_usb_urb_status_completed);
    PyModule_AddIntConstant(module, "URB_STATUS_UNLINKED", vdp_usb_urb_status_unlinked);
    PyModule_AddIntConstant(module, "URB_STATUS_ERROR", vdp_usb_urb_status_error);
    PyModule_AddIntConstant(module, "URB_STATUS_STALL", vdp_usb_urb_status_stall);
    PyModule_AddIntConstant(module, "URB_STATUS_OVERFLOW", vdp_usb_urb_status_overflow);

    PyModule_AddIntConstant(module, "DT_DEVICE", VDP_USB_DT_DEVICE);
    PyModule_AddIntConstant(module, "DT_CONFIG", VDP_USB_DT_CONFIG);
    PyModule_AddIntConstant(module, "DT_STRING", VDP_USB_DT_STRING);
    PyModule_AddIntConstant(module, "DT_INTERFACE", VDP_USB_DT_INTERFACE);
    PyModule_AddIntConstant(module, "DT_ENDPOINT", VDP_USB_DT_ENDPOINT);
    PyModule_AddIntConstant(module, "DT_QUALIFIER", VDP_USB_DT_QUALIFIER);

    PyModule_AddIntConstant(module, "CONFIG_ATT_ONE", VDP_USB_CONFIG_ATT_ONE);
    PyModule_AddIntConstant(module, "CONFIG_ATT_SELFPOWER", VDP_USB_CONFIG_ATT_SELFPOWER);
    PyModule_AddIntConstant(module, "CONFIG_ATT_WAKEUP", VDP_USB_CONFIG_ATT_WAKEUP);
    PyModule_AddIntConstant(module, "CONFIG_ATT_BATTERY", VDP_USB_CONFIG_ATT_BATTERY);

    PyModule_AddIntConstant(module, "DT_ENDPOINT_SIZE", VDP_USB_DT_ENDPOINT_SIZE);
    PyModule_AddIntConstant(module, "DT_ENDPOINT_AUDIO_SIZE", VDP_USB_DT_ENDPOINT_AUDIO_SIZE);

    PyModule_AddIntConstant(module, "ENDPOINT_SYNC_NONE", VDP_USB_ENDPOINT_SYNC_NONE);
    PyModule_AddIntConstant(module, "ENDPOINT_SYNC_ASYNC", VDP_USB_ENDPOINT_SYNC_ASYNC);
    PyModule_AddIntConstant(module, "ENDPOINT_SYNC_ADAPTIVE", VDP_USB_ENDPOINT_SYNC_ADAPTIVE);
    PyModule_AddIntConstant(module, "ENDPOINT_SYNC_SYNC", VDP_USB_ENDPOINT_SYNC_SYNC);

    PyModule_AddIntConstant(module, "ENDPOINT_USAGE_DATA", VDP_USB_ENDPOINT_USAGE_DATA);
    PyModule_AddIntConstant(module, "ENDPOINT_USAGE_FEEDBACK", VDP_USB_ENDPOINT_USAGE_FEEDBACK);
    PyModule_AddIntConstant(module, "ENDPOINT_USAGE_IMPLICIT_FB", VDP_USB_ENDPOINT_USAGE_IMPLICIT_FB);

    PyModule_AddIntConstant(module, "ENDPOINT_XFER_CONTROL", VDP_USB_ENDPOINT_XFER_CONTROL);
    PyModule_AddIntConstant(module, "ENDPOINT_XFER_ISO", VDP_USB_ENDPOINT_XFER_ISO);
    PyModule_AddIntConstant(module, "ENDPOINT_XFER_BULK", VDP_USB_ENDPOINT_XFER_BULK);
    PyModule_AddIntConstant(module, "ENDPOINT_XFER_INT", VDP_USB_ENDPOINT_XFER_INT);

    PyModule_AddIntConstant(module, "REQUESTTYPE_TYPE_STANDARD", VDP_USB_REQUESTTYPE_TYPE_STANDARD);
    PyModule_AddIntConstant(module, "REQUESTTYPE_TYPE_CLASS", VDP_USB_REQUESTTYPE_TYPE_CLASS);
    PyModule_AddIntConstant(module, "REQUESTTYPE_TYPE_VENDOR", VDP_USB_REQUESTTYPE_TYPE_VENDOR);
    PyModule_AddIntConstant(module, "REQUESTTYPE_TYPE_RESERVED", VDP_USB_REQUESTTYPE_TYPE_RESERVED);

    PyModule_AddIntConstant(module, "REQUESTTYPE_RECIPIENT_DEVICE", VDP_USB_REQUESTTYPE_RECIPIENT_DEVICE);
    PyModule_AddIntConstant(module, "REQUESTTYPE_RECIPIENT_INTERFACE", VDP_USB_REQUESTTYPE_RECIPIENT_INTERFACE);
    PyModule_AddIntConstant(module, "REQUESTTYPE_RECIPIENT_ENDPOINT", VDP_USB_REQUESTTYPE_RECIPIENT_ENDPOINT);
    PyModule_AddIntConstant(module, "REQUESTTYPE_RECIPIENT_OTHER", VDP_USB_REQUESTTYPE_RECIPIENT_OTHER);
    PyModule_AddIntConstant(module, "REQUESTTYPE_RECIPIENT_PORT", VDP_USB_REQUESTTYPE_RECIPIENT_PORT);
    PyModule_AddIntConstant(module, "REQUESTTYPE_RECIPIENT_RPIPE", VDP_USB_REQUESTTYPE_RECIPIENT_RPIPE);

    PyModule_AddIntConstant(module, "REQUEST_GET_STATUS", VDP_USB_REQUEST_GET_STATUS);
    PyModule_AddIntConstant(module, "REQUEST_CLEAR_FEATURE", VDP_USB_REQUEST_CLEAR_FEATURE);
    PyModule_AddIntConstant(module, "REQUEST_SET_FEATURE", VDP_USB_REQUEST_SET_FEATURE);
    PyModule_AddIntConstant(module, "REQUEST_SET_ADDRESS", VDP_USB_REQUEST_SET_ADDRESS);
    PyModule_AddIntConstant(module, "REQUEST_GET_DESCRIPTOR", VDP_USB_REQUEST_GET_DESCRIPTOR);
    PyModule_AddIntConstant(module, "REQUEST_SET_DESCRIPTOR", VDP_USB_REQUEST_SET_DESCRIPTOR);
    PyModule_AddIntConstant(module, "REQUEST_GET_CONFIGURATION", VDP_USB_REQUEST_GET_CONFIGURATION);
    PyModule_AddIntConstant(module, "REQUEST_SET_CONFIGURATION", VDP_USB_REQUEST_SET_CONFIGURATION);
    PyModule_AddIntConstant(module, "REQUEST_GET_INTERFACE", VDP_USB_REQUEST_GET_INTERFACE);
    PyModule_AddIntConstant(module, "REQUEST_SET_INTERFACE", VDP_USB_REQUEST_SET_INTERFACE);
    PyModule_AddIntConstant(module, "REQUEST_SYNCH_FRAME", VDP_USB_REQUEST_SYNCH_FRAME);
    PyModule_AddIntConstant(module, "REQUEST_SET_ENCRYPTION", VDP_USB_REQUEST_SET_ENCRYPTION);
    PyModule_AddIntConstant(module, "REQUEST_GET_ENCRYPTION", VDP_USB_REQUEST_GET_ENCRYPTION);
    PyModule_AddIntConstant(module, "REQUEST_SET_HANDSHAKE", VDP_USB_REQUEST_SET_HANDSHAKE);
    PyModule_AddIntConstant(module, "REQUEST_GET_HANDSHAKE", VDP_USB_REQUEST_GET_HANDSHAKE);
    PyModule_AddIntConstant(module, "REQUEST_SET_CONNECTION", VDP_USB_REQUEST_SET_CONNECTION);
    PyModule_AddIntConstant(module, "REQUEST_SET_SECURITY_DATA", VDP_USB_REQUEST_SET_SECURITY_DATA);
    PyModule_AddIntConstant(module, "REQUEST_GET_SECURITY_DATA", VDP_USB_REQUEST_GET_SECURITY_DATA);
    PyModule_AddIntConstant(module, "REQUEST_SET_WUSB_DATA", VDP_USB_REQUEST_SET_WUSB_DATA);
    PyModule_AddIntConstant(module, "REQUEST_LOOPBACK_DATA_WRITE", VDP_USB_REQUEST_LOOPBACK_DATA_WRITE);
    PyModule_AddIntConstant(module, "REQUEST_LOOPBACK_DATA_READ", VDP_USB_REQUEST_LOOPBACK_DATA_READ);
    PyModule_AddIntConstant(module, "REQUEST_SET_INTERFACE_DS", VDP_USB_REQUEST_SET_INTERFACE_DS);

    PyModule_AddIntConstant(module, "CLASS_HID", VDP_USB_CLASS_HID);
    PyModule_AddIntConstant(module, "SUBCLASS_BOOT", VDP_USB_SUBCLASS_BOOT);
    PyModule_AddIntConstant(module, "PROTOCOL_KEYBOARD", VDP_USB_PROTOCOL_KEYBOARD);
    PyModule_AddIntConstant(module, "PROTOCOL_MOUSE", VDP_USB_PROTOCOL_MOUSE);

    PyModule_AddIntConstant(module, "HID_REQUEST_GET_REPORT", VDP_USB_HID_REQUEST_GET_REPORT);
    PyModule_AddIntConstant(module, "HID_REQUEST_GET_IDLE", VDP_USB_HID_REQUEST_GET_IDLE);
    PyModule_AddIntConstant(module, "HID_REQUEST_GET_PROTOCOL", VDP_USB_HID_REQUEST_GET_PROTOCOL);
    PyModule_AddIntConstant(module, "HID_REQUEST_SET_REPORT", VDP_USB_HID_REQUEST_SET_REPORT);
    PyModule_AddIntConstant(module, "HID_REQUEST_SET_IDLE", VDP_USB_HID_REQUEST_SET_IDLE);
    PyModule_AddIntConstant(module, "HID_REQUEST_SET_PROTOCOL", VDP_USB_HID_REQUEST_SET_PROTOCOL);

    PyModule_AddIntConstant(module, "HID_DT_HID", VDP_USB_HID_DT_HID);
    PyModule_AddIntConstant(module, "HID_DT_REPORT", VDP_USB_HID_DT_REPORT);
    PyModule_AddIntConstant(module, "HID_DT_PHYSICAL", VDP_USB_HID_DT_PHYSICAL);

    vdp_py_usb_error_init(module);
    vdp_py_usb_context_init(module);
    vdp_py_usb_urb_init(module);
    vdp_py_usb_device_init(module);
    vdp_py_usb_event_init(module);
    vdp_py_usb_filter_init(module);
}
