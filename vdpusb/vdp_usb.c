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

#include "vdp/usb.h"
#include "vdp/byte_order.h"
#include <stdlib.h>
#include <assert.h>

int vdp_usb_signal_type_validate(int value)
{
    switch (value) {
    case vdp_usb_signal_reset_start:
    case vdp_usb_signal_reset_end:
    case vdp_usb_signal_power_on:
    case vdp_usb_signal_power_off:
        return 1;
    default:
        return 0;
    }
}

const char* vdp_usb_result_to_str(vdp_usb_result res)
{
    switch (res) {
    case vdp_usb_success: return "success";
    case vdp_usb_nomem: return "not enough memory";
    case vdp_usb_misuse: return "misuse";
    case vdp_usb_unknown: return "unknown error";
    case vdp_usb_not_found: return "entity not found";
    case vdp_usb_busy: return "device is busy";
    case vdp_usb_protocol_error: return "kernel-user protocol error";
    default: return "undefined error";
    };
}

const char* vdp_usb_signal_type_to_str(vdp_usb_signal_type signal_type)
{
    switch (signal_type) {
    case vdp_usb_signal_reset_start: return "reset_start";
    case vdp_usb_signal_reset_end: return "reset_end";
    case vdp_usb_signal_power_on: return "power_on";
    case vdp_usb_signal_power_off: return "power_off";
    default: return "undefined";
    };
}

const char* vdp_usb_urb_type_to_str(vdp_usb_urb_type urb_type)
{
    switch (urb_type) {
    case vdp_usb_urb_control: return "control";
    case vdp_usb_urb_bulk: return "bulk";
    case vdp_usb_urb_int: return "int";
    case vdp_usb_urb_iso: return "iso";
    default: return "undefined";
    };
}

const char* vdp_usb_request_type_direction_to_str(vdp_u8 bRequestType)
{
    if (VDP_USB_REQUESTTYPE_IN(bRequestType)) {
        return "in";
    } else {
        return "out";
    }
}

const char* vdp_usb_request_type_type_to_str(vdp_u8 bRequestType)
{
    switch (VDP_USB_REQUESTTYPE_TYPE(bRequestType)) {
    case VDP_USB_REQUESTTYPE_TYPE_STANDARD: return "standard";
    case VDP_USB_REQUESTTYPE_TYPE_CLASS: return "class";
    case VDP_USB_REQUESTTYPE_TYPE_VENDOR: return "vendor";
    case VDP_USB_REQUESTTYPE_TYPE_RESERVED: return "reserved";
    default: return "unknown";
    }
}

const char* vdp_usb_request_type_recipient_to_str(vdp_u8 bRequestType)
{
    switch (VDP_USB_REQUESTTYPE_RECIPIENT(bRequestType)) {
    case VDP_USB_REQUESTTYPE_RECIPIENT_DEVICE: return "device";
    case VDP_USB_REQUESTTYPE_RECIPIENT_INTERFACE: return "interface";
    case VDP_USB_REQUESTTYPE_RECIPIENT_ENDPOINT: return "endpoint";
    case VDP_USB_REQUESTTYPE_RECIPIENT_OTHER: return "other";
    case VDP_USB_REQUESTTYPE_RECIPIENT_PORT: return "port";
    case VDP_USB_REQUESTTYPE_RECIPIENT_RPIPE: return "rpipe";
    default: return "unknown";
    }
}

const char* vdp_usb_request_to_str(vdp_u8 bRequest)
{
    switch (bRequest) {
    case VDP_USB_REQUEST_GET_STATUS: return "VDP_USB_REQUEST_GET_STATUS";
    case VDP_USB_REQUEST_CLEAR_FEATURE: return "VDP_USB_REQUEST_CLEAR_FEATURE";
    case VDP_USB_REQUEST_SET_FEATURE: return "VDP_USB_REQUEST_SET_FEATURE";
    case VDP_USB_REQUEST_SET_ADDRESS: return "VDP_USB_REQUEST_SET_ADDRESS";
    case VDP_USB_REQUEST_GET_DESCRIPTOR: return "VDP_USB_REQUEST_GET_DESCRIPTOR";
    case VDP_USB_REQUEST_SET_DESCRIPTOR: return "VDP_USB_REQUEST_SET_DESCRIPTOR";
    case VDP_USB_REQUEST_GET_CONFIGURATION: return "VDP_USB_REQUEST_GET_CONFIGURATION";
    case VDP_USB_REQUEST_SET_CONFIGURATION: return "VDP_USB_REQUEST_SET_CONFIGURATION";
    case VDP_USB_REQUEST_GET_INTERFACE: return "VDP_USB_REQUEST_GET_INTERFACE";
    case VDP_USB_REQUEST_SET_INTERFACE: return "VDP_USB_REQUEST_SET_INTERFACE";
    case VDP_USB_REQUEST_SYNCH_FRAME: return "VDP_USB_REQUEST_SYNCH_FRAME";
    case VDP_USB_REQUEST_SET_ENCRYPTION: return "VDP_USB_REQUEST_SET_ENCRYPTION";
    case VDP_USB_REQUEST_GET_ENCRYPTION: return "VDP_USB_REQUEST_GET_ENCRYPTION";
    case VDP_USB_REQUEST_SET_HANDSHAKE: return "VDP_USB_REQUEST_SET_HANDSHAKE";
    case VDP_USB_REQUEST_GET_HANDSHAKE: return "VDP_USB_REQUEST_GET_HANDSHAKE";
    case VDP_USB_REQUEST_SET_CONNECTION: return "VDP_USB_REQUEST_SET_CONNECTION";
    case VDP_USB_REQUEST_SET_SECURITY_DATA: return "VDP_USB_REQUEST_SET_SECURITY_DATA";
    case VDP_USB_REQUEST_GET_SECURITY_DATA: return "VDP_USB_REQUEST_GET_SECURITY_DATA";
    case VDP_USB_REQUEST_SET_WUSB_DATA: return "VDP_USB_REQUEST_SET_WUSB_DATA";
    case VDP_USB_REQUEST_LOOPBACK_DATA_WRITE: return "VDP_USB_REQUEST_LOOPBACK_DATA_WRITE";
    case VDP_USB_REQUEST_LOOPBACK_DATA_READ: return "VDP_USB_REQUEST_LOOPBACK_DATA_READ";
    case VDP_USB_REQUEST_SET_INTERFACE_DS: return "VDP_USB_REQUEST_SET_INTERFACE_DS";
    default : return "VDP_USB_REQUEST_XXX";
    }
}

void vdp_usb_urb_to_str(const struct vdp_usb_urb* urb, char* buff, size_t buff_size)
{
    int ret;

    if (urb->type == vdp_usb_urb_int) {
        ret = snprintf(
            buff,
            buff_size,
            "id = %u, type = %s, flags = 0x%X, ep = 0x%.2X, interval = %u, buff = (%u)",
            urb->id,
            vdp_usb_urb_type_to_str(urb->type),
            urb->flags,
            urb->endpoint_address,
            urb->interval,
            urb->transfer_length);
    } else if (urb->type == vdp_usb_urb_iso) {
        ret = snprintf(
            buff,
            buff_size,
            "id = %u, type = %s, flags = 0x%X, ep = 0x%.2X, num_packets = %u, interval = %u, buff = (%u)",
            urb->id,
            vdp_usb_urb_type_to_str(urb->type),
            urb->flags,
            urb->endpoint_address,
            urb->number_of_packets,
            urb->interval,
            urb->transfer_length);
    } else if (urb->type == vdp_usb_urb_control) {
        if (VDP_USB_REQUESTTYPE_TYPE(urb->setup_packet->bRequestType) == VDP_USB_REQUESTTYPE_TYPE_STANDARD) {
            ret = snprintf(
                buff,
                buff_size,
                "id = %u, type = %s, flags = 0x%X, ep = 0x%.2X, bRequestType = %s:%s:%s, bRequest = %s, wValue = %u, wIndex = %u, buff = (%u)",
                urb->id,
                vdp_usb_urb_type_to_str(urb->type),
                urb->flags,
                urb->endpoint_address,
                vdp_usb_request_type_direction_to_str(urb->setup_packet->bRequestType),
                vdp_usb_request_type_type_to_str(urb->setup_packet->bRequestType),
                vdp_usb_request_type_recipient_to_str(urb->setup_packet->bRequestType),
                vdp_usb_request_to_str(urb->setup_packet->bRequest),
                vdp_u16le_to_cpu(urb->setup_packet->wValue),
                vdp_u16le_to_cpu(urb->setup_packet->wIndex),
                vdp_u16le_to_cpu(urb->setup_packet->wLength));
        } else {
            ret = snprintf(
                buff,
                buff_size,
                "id = %u, type = %s, flags = 0x%X, ep = 0x%.2X, bRequestType = %s:%s:%s, bRequest = 0x%X, wValue = %u, wIndex = %u, buff = (%u)",
                urb->id,
                vdp_usb_urb_type_to_str(urb->type),
                urb->flags,
                urb->endpoint_address,
                vdp_usb_request_type_direction_to_str(urb->setup_packet->bRequestType),
                vdp_usb_request_type_type_to_str(urb->setup_packet->bRequestType),
                vdp_usb_request_type_recipient_to_str(urb->setup_packet->bRequestType),
                (int)urb->setup_packet->bRequest,
                vdp_u16le_to_cpu(urb->setup_packet->wValue),
                vdp_u16le_to_cpu(urb->setup_packet->wIndex),
                vdp_u16le_to_cpu(urb->setup_packet->wLength));
        }
    } else {
        ret = snprintf(
            buff,
            buff_size,
            "id = %u, type = %s, flags = 0x%X, ep = 0x%.2X, buff = (%u)",
            urb->id,
            vdp_usb_urb_type_to_str(urb->type),
            urb->flags,
            urb->endpoint_address,
            urb->transfer_length);
    }

    if (ret <= 0) {
        buff[0] = '\0';
    } else {
        buff[buff_size - 1] = '\0';
    }
}
