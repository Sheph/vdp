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

#include "vdp_usb_urbi.h"
#include "vdp_usb_device.h"
#include "vdp_usb_context.h"
#include "vdp/byte_order.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline struct vdphci_hevent_urb* urbi_get_hevent_urb(struct vdp_usb_urbi* urbi)
{
    return (struct vdphci_hevent_urb*)((const char*)urbi->event + sizeof(struct vdphci_hevent_header));
}

static int translate_urb_status(vdp_usb_urb_status status, vdphci_urb_status* res)
{
    assert(res);
    if (!res) {
        return 0;
    }

    switch (status) {
    case vdp_usb_urb_status_completed: {
        *res = vdphci_urb_status_completed;
        break;
    }
    case vdp_usb_urb_status_unlinked: {
        *res = vdphci_urb_status_unlinked;
        break;
    }
    case vdp_usb_urb_status_error: {
        *res = vdphci_urb_status_error;
        break;
    }
    case vdp_usb_urb_status_stall: {
        *res = vdphci_urb_status_stall;
        break;
    }
    case vdp_usb_urb_status_overflow: {
        *res = vdphci_urb_status_overflow;
        break;
    }
    case vdp_usb_urb_status_undefined:
    default:
        return 0;
    }

    return 1;
}

static void urbi_fill_common(struct vdp_usb_device* device,
    const void* event,
    struct vdphci_hevent_urb* urb,
    vdp_u32 urbi_size,
    struct vdp_usb_urbi* urbi)
{
    urbi->size = urbi_size;
    urbi->device = device;
    urbi->event = event;
    urbi->urb.id = urb->seq_num;

    switch (urb->type) {
    default: {
        assert(0);
    }
    case vdphci_urb_type_control: {
        urbi->urb.type = vdp_usb_urb_control;
        break;
    }
    case vdphci_urb_type_bulk: {
        urbi->urb.type = vdp_usb_urb_bulk;
        break;
    }
    case vdphci_urb_type_int: {
        urbi->urb.type = vdp_usb_urb_int;
        break;
    }
    case vdphci_urb_type_iso:
        urbi->urb.type = vdp_usb_urb_iso;
        break;
    }

    urbi->urb.flags = urb->flags;
    urbi->urb.endpoint_address = urb->endpoint_address;
    urbi->urb.setup_packet = NULL;
    urbi->urb.transfer_buffer = NULL;
    urbi->urb.transfer_length = urb->transfer_length;
    urbi->urb.actual_length = 0;
    urbi->urb.number_of_packets = urb->number_of_packets;
    urbi->urb.interval = urb->interval;
    urbi->urb.status = vdp_usb_urb_status_undefined;
    urbi->urb.iso_packets = NULL;

    urbi->devent_header.type = vdphci_devent_type_urb;

    urbi->devent_urb.seq_num = urb->seq_num;
    urbi->devent_urb.status = vdphci_urb_status_unprocessed;
    urbi->devent_urb.actual_length = 0;
}

static vdp_usb_result urbi_create_in_iso(struct vdp_usb_device* device,
    const void* event,
    struct vdphci_hevent_urb* urb,
    vdp_u32 urb_size,
    struct vdp_usb_urbi** urbi)
{
    vdp_u32 iso_packets_size = urb_size - vdp_offsetof(struct vdphci_hevent_urb, data.packets);
    vdp_u32 num_iso_packets = iso_packets_size / sizeof(struct vdphci_h_iso_packet);
    vdp_u32 i = 0;
    vdp_u32 actual_transfer_length = 0;
    vdp_u32 data_offset = 0;
    vdp_u32 tmp_size = 0;
    vdp_byte* current_iso_buff = NULL;

    /*
     * Validate the URB.
     */

    if ((num_iso_packets * sizeof(struct vdphci_h_iso_packet)) != iso_packets_size) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad iso packet data",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    if (num_iso_packets != urb->number_of_packets) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb, number of packets == %d, but it should be %d",
            device->device_number,
            urb->number_of_packets,
            num_iso_packets);
    }

    for (i = 0; i < num_iso_packets; ++i) {
        actual_transfer_length += urb->data.packets[i].length;
    }

    if (actual_transfer_length != urb->transfer_length) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb, transfer length == %d, but it should be %d",
            device->device_number,
            urb->transfer_length,
            actual_transfer_length);

        return vdp_usb_protocol_error;
    }

    /*
     * Allocate urbi.
     */

    data_offset = vdp_offsetof(struct vdp_usb_urbi, devent_urb) +
        vdp_offsetof(struct vdphci_devent_urb, data.packets) +
        (sizeof(struct vdphci_d_iso_packet) * num_iso_packets);

    tmp_size = data_offset + actual_transfer_length;

    *urbi = malloc(tmp_size);

    if (*urbi == NULL) {
        return vdp_usb_nomem;
    }

    memset(*urbi, 0, tmp_size);

    urbi_fill_common(device, event, urb, tmp_size, *urbi);

    tmp_size = sizeof(struct vdp_usb_iso_packet) * num_iso_packets;

    (*urbi)->urb.iso_packets = malloc(tmp_size);

    if ((*urbi)->urb.iso_packets == NULL) {
        free(*urbi);
        *urbi = NULL;

        return vdp_usb_nomem;
    }

    memset((*urbi)->urb.iso_packets, 0, tmp_size);

    /*
     * Fill urbi.urb
     */

    (*urbi)->urb.transfer_buffer = (vdp_byte*)(*urbi) + data_offset;

    current_iso_buff = (*urbi)->urb.transfer_buffer;

    for (i = 0; i < num_iso_packets; ++i) {
        (*urbi)->urb.iso_packets[i].buffer = current_iso_buff;
        (*urbi)->urb.iso_packets[i].length = urb->data.packets[i].length;
        (*urbi)->urb.iso_packets[i].actual_length = 0;
        (*urbi)->urb.iso_packets[i].status = vdp_usb_urb_status_undefined;

        current_iso_buff += urb->data.packets[i].length;
    }

    assert(((*urbi)->urb.transfer_buffer + (*urbi)->urb.transfer_length) == current_iso_buff);

    /*
     * Fill urbi.devent_urb
     */

    for (i = 0; i < num_iso_packets; ++i) {
        (*urbi)->devent_urb.data.packets[i].status = vdphci_urb_status_unprocessed;
        (*urbi)->devent_urb.data.packets[i].actual_length = 0;
    }

    return vdp_usb_success;
}

static vdp_usb_result urbi_create_in_control(struct vdp_usb_device* device,
    const void* event,
    struct vdphci_hevent_urb* urb,
    vdp_u32 urb_size,
    struct vdp_usb_urbi** urbi)
{
    vdp_u32 data_offset = 0;
    vdp_u32 tmp_size = 0;
    const struct vdp_usb_control_setup* setup = NULL;

    /*
     * Validate the URB.
     */

    if ((urb_size - vdp_offsetof(struct vdphci_hevent_urb, data.buff)) !=
        sizeof(struct vdp_usb_control_setup))
    {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb size, control setup packet is expected",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    setup = (const struct vdp_usb_control_setup*)(&urb->data.buff[0]);

    if ((vdp_u32)vdp_u16le_to_cpu(setup->wLength) != urb->transfer_length) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb size, control setup wLength != transfer_length",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    if (VDP_USB_REQUESTTYPE_OUT(setup->bRequestType)) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad control setup, bRequestType specifies an out transfer",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    /*
     * Allocate urbi.
     */

    data_offset = vdp_offsetof(struct vdp_usb_urbi, devent_urb) +
        vdp_offsetof(struct vdphci_devent_urb, data.buff);

    tmp_size = data_offset + urb->transfer_length;

    *urbi = malloc(tmp_size);

    if (*urbi == NULL) {
        return vdp_usb_nomem;
    }

    memset(*urbi, 0, tmp_size);

    urbi_fill_common(device, event, urb, tmp_size, *urbi);

    /*
     * Fill urbi.urb
     */

    (*urbi)->urb.setup_packet = setup;

    (*urbi)->urb.transfer_buffer = (vdp_byte*)(*urbi) + data_offset;

    return vdp_usb_success;
}

static vdp_usb_result urbi_create_in_other(struct vdp_usb_device* device,
    const void* event,
    struct vdphci_hevent_urb* urb,
    vdp_u32 urb_size,
    struct vdp_usb_urbi** urbi)
{
    vdp_u32 data_offset = 0;
    vdp_u32 tmp_size = 0;

    /*
     * Validate the URB.
     */

    if (urb_size != vdp_offsetof(struct vdphci_hevent_urb, data.buff)) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb size",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    /*
     * Allocate urbi.
     */

    data_offset = vdp_offsetof(struct vdp_usb_urbi, devent_urb) +
        vdp_offsetof(struct vdphci_devent_urb, data.buff);

    tmp_size = data_offset + urb->transfer_length;

    *urbi = malloc(tmp_size);

    if (*urbi == NULL) {
        return vdp_usb_nomem;
    }

    memset(*urbi, 0, tmp_size);

    urbi_fill_common(device, event, urb, tmp_size, *urbi);

    /*
     * Fill urbi.urb
     */

    (*urbi)->urb.transfer_buffer = (vdp_byte*)(*urbi) + data_offset;

    return vdp_usb_success;
}

static vdp_usb_result urbi_create_out_iso(struct vdp_usb_device* device,
    const void* event,
    struct vdphci_hevent_urb* urb,
    vdp_u32 urb_size,
    struct vdp_usb_urbi** urbi)
{
    vdp_u32 iso_packets_size = urb_size - vdp_offsetof(struct vdphci_hevent_urb, data.packets);
    vdp_u32 iso_data_size = 0;
    vdp_u32 actual_transfer_length = 0;
    vdp_u32 tmp_size = 0;
    vdp_u32 i = 0;

    /*
     * Validate the URB.
     */

    if ((urb->number_of_packets * sizeof(struct vdphci_h_iso_packet)) > iso_packets_size) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad iso packet data",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    iso_data_size = iso_packets_size - (urb->number_of_packets * sizeof(struct vdphci_h_iso_packet));

    if (urb->transfer_length != iso_data_size) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb, transfer length == %d, but it should be %d",
            device->device_number,
            urb->transfer_length,
            iso_data_size);

        return vdp_usb_protocol_error;
    }

    for (i = 0; i < urb->number_of_packets; ++i) {
        if ((actual_transfer_length + urb->data.packets[i].length) > iso_data_size) {
            VDP_USB_LOG_ERROR(device->context,
                "device %d: bad urb, iso packet end > iso_data_size",
                device->device_number);

            return vdp_usb_protocol_error;
        }

        actual_transfer_length += urb->data.packets[i].length;
    }

    if (urb->transfer_length != actual_transfer_length) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb, transfer length == %d, but it should be %d",
            device->device_number,
            urb->transfer_length,
            actual_transfer_length);

        return vdp_usb_protocol_error;
    }

    /*
     * Allocate urbi.
     */

    tmp_size = vdp_offsetof(struct vdp_usb_urbi, devent_urb) +
        vdp_offsetof(struct vdphci_devent_urb, data.packets) +
        (sizeof(struct vdphci_d_iso_packet) * urb->number_of_packets);

    *urbi = malloc(tmp_size);

    if (*urbi == NULL) {
        return vdp_usb_nomem;
    }

    memset(*urbi, 0, tmp_size);

    urbi_fill_common(device, event, urb, tmp_size, *urbi);

    tmp_size = sizeof(struct vdp_usb_iso_packet) * urb->number_of_packets;

    (*urbi)->urb.iso_packets = malloc(tmp_size);

    if ((*urbi)->urb.iso_packets == NULL) {
        free(*urbi);
        *urbi = NULL;

        return vdp_usb_nomem;
    }

    memset((*urbi)->urb.iso_packets, 0, tmp_size);

    /*
     * Fill urbi.urb
     */

    (*urbi)->urb.transfer_buffer = (vdp_byte*)(urb) +
        vdp_offsetof(struct vdphci_hevent_urb, data.packets) +
        (urb->number_of_packets * sizeof(struct vdphci_h_iso_packet));

    actual_transfer_length = 0;

    for (i = 0; i < urb->number_of_packets; ++i) {
        (*urbi)->urb.iso_packets[i].buffer = (*urbi)->urb.transfer_buffer + actual_transfer_length;
        (*urbi)->urb.iso_packets[i].length = urb->data.packets[i].length;
        (*urbi)->urb.iso_packets[i].actual_length = 0;
        (*urbi)->urb.iso_packets[i].status = vdp_usb_urb_status_undefined;

        actual_transfer_length += urb->data.packets[i].length;
    }

    /*
     * Fill urbi.devent_urb
     */

    for (i = 0; i < urb->number_of_packets; ++i) {
        (*urbi)->devent_urb.data.packets[i].status = vdphci_urb_status_unprocessed;
        (*urbi)->devent_urb.data.packets[i].actual_length = 0;
    }

    return vdp_usb_success;
}

static vdp_usb_result urbi_create_out_control(struct vdp_usb_device* device,
    const void* event,
    struct vdphci_hevent_urb* urb,
    vdp_u32 urb_size,
    struct vdp_usb_urbi** urbi)
{
    vdp_u32 tmp_size = 0;
    const struct vdp_usb_control_setup* setup = NULL;
    vdp_u32 actual_transfer_length = 0;

    /*
     * Validate the URB.
     */

    if ((urb_size - vdp_offsetof(struct vdphci_hevent_urb, data.buff)) <
        sizeof(struct vdp_usb_control_setup)) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb size, control setup packet is expected",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    actual_transfer_length = urb_size -
        vdp_offsetof(struct vdphci_hevent_urb, data.buff) -
        sizeof(struct vdp_usb_control_setup);

    if (actual_transfer_length != urb->transfer_length) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb size, transfer length != actual transfer length",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    setup = (const struct vdp_usb_control_setup*)(&urb->data.buff[0]);

    if ((vdp_u32)vdp_u16le_to_cpu(setup->wLength) != urb->transfer_length) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb size, control setup wLength != transfer_length",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    if (VDP_USB_REQUESTTYPE_IN(setup->bRequestType)) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad control setup, bRequestType specifies an in transfer",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    /*
     * Allocate urbi.
     */

    tmp_size = vdp_offsetof(struct vdp_usb_urbi, devent_urb) +
        vdp_offsetof(struct vdphci_devent_urb, data.buff);

    *urbi = malloc(tmp_size);

    if (*urbi == NULL) {
        return vdp_usb_nomem;
    }

    memset(*urbi, 0, tmp_size);

    urbi_fill_common(device, event, urb, tmp_size, *urbi);

    /*
     * Fill urbi.urb
     */

    (*urbi)->urb.setup_packet = setup;

    (*urbi)->urb.transfer_buffer = (vdp_byte*)setup + sizeof(struct vdp_usb_control_setup);

    return vdp_usb_success;
}

static vdp_usb_result urbi_create_out_other(struct vdp_usb_device* device,
    const void* event,
    struct vdphci_hevent_urb* urb,
    vdp_u32 urb_size,
    struct vdp_usb_urbi** urbi)
{
    vdp_u32 actual_transfer_length = urb_size - vdp_offsetof(struct vdphci_hevent_urb, data.buff);
    vdp_u32 tmp_size = 0;

    /*
     * Validate the URB.
     */

    if (actual_transfer_length != urb->transfer_length) {
        VDP_USB_LOG_ERROR(device->context,
            "device %d: bad urb size, transfer length != actual transfer length",
            device->device_number);

        return vdp_usb_protocol_error;
    }

    /*
     * Allocate urbi.
     */

    tmp_size = vdp_offsetof(struct vdp_usb_urbi, devent_urb) +
        vdp_offsetof(struct vdphci_devent_urb, data.buff);

    *urbi = malloc(tmp_size);

    if (*urbi == NULL) {
        return vdp_usb_nomem;
    }

    memset(*urbi, 0, tmp_size);

    urbi_fill_common(device, event, urb, tmp_size, *urbi);

    /*
     * Fill urbi.urb
     */

    (*urbi)->urb.transfer_buffer = (vdp_byte*)&urb->data.buff[0];

    return vdp_usb_success;
}

vdp_usb_result vdp_usb_urbi_create(struct vdp_usb_device* device,
    const void* event,
    size_t event_size,
    struct vdp_usb_urbi** urbi)
{
    struct vdphci_hevent_urb* urb;
    size_t urb_size;

    assert(device);
    assert(event);
    assert(urbi);
    assert(event_size >= sizeof(struct vdphci_hevent_header));

    urb = (struct vdphci_hevent_urb*)((const char*)event + sizeof(struct vdphci_hevent_header));
    urb_size = event_size - sizeof(struct vdphci_hevent_header);

    assert(urb_size >= vdp_offsetof(struct vdphci_hevent_urb, data.buff));

    /*
     * Ok, here's what we have: 'urb' and it's size in 'urb_size', the structure itself
     * must be validated below.
     */

    if (VDPHCI_USB_ENDPOINT_IN(urb->endpoint_address)) {
        switch (urb->type) {
        case vdphci_urb_type_control: {
            return urbi_create_in_control(device, event, urb, urb_size, urbi);
        }
        case vdphci_urb_type_bulk:
        case vdphci_urb_type_int: {
            return urbi_create_in_other(device, event, urb, urb_size, urbi);
        }
        case vdphci_urb_type_iso: {
            return urbi_create_in_iso(device, event, urb, urb_size, urbi);
        }
        default:
            VDP_USB_LOG_ERROR(device->context,
                "device %d: bad urb type",
                device->device_number);
            return vdp_usb_protocol_error;
        }
    } else {
        switch (urb->type) {
        case vdphci_urb_type_control: {
            return urbi_create_out_control(device, event, urb, urb_size, urbi);
        }
        case vdphci_urb_type_bulk:
        case vdphci_urb_type_int: {
            return urbi_create_out_other(device, event, urb, urb_size, urbi);
        }
        case vdphci_urb_type_iso: {
            return urbi_create_out_iso(device, event, urb, urb_size, urbi);
        }
        default:
            VDP_USB_LOG_ERROR(device->context,
                "device %d: bad urb type",
                device->device_number);
            return vdp_usb_protocol_error;
        }
    }
}

vdp_usb_result vdp_usb_urbi_update(struct vdp_usb_urbi* urbi)
{
    struct vdphci_hevent_urb* original_urb;
    vdp_u32 i;
    vdp_u32 actual_length;

    assert(urbi);

    if (!urbi) {
        return vdp_usb_misuse;
    }

    original_urb = urbi_get_hevent_urb(urbi);

    if (original_urb->type == vdphci_urb_type_iso) {
        vdphci_urb_status default_urb_status;

        int have_default_urb_status =
            translate_urb_status(urbi->urb.status, &default_urb_status);

        actual_length = 0;

        if (urbi->urb.number_of_packets != original_urb->number_of_packets) {
            VDP_USB_LOG_ERROR(urbi->device->context,
                "device %d: bad urb number_of_packets",
                urbi->device->device_number);
            return vdp_usb_misuse;
        }

        for (i = 0; i < urbi->urb.number_of_packets; ++i) {
            if (!translate_urb_status(urbi->urb.iso_packets[i].status, &urbi->devent_urb.data.packets[i].status)) {
                if (have_default_urb_status) {
                    urbi->devent_urb.data.packets[i].status = default_urb_status;
                } else {
                    VDP_USB_LOG_ERROR(urbi->device->context,
                        "device %d: bad iso packet status in packet #%d",
                        urbi->device->device_number,
                        i);
                    return vdp_usb_misuse;
                }
            }

            if (urbi->urb.iso_packets[i].actual_length > original_urb->data.packets[i].length) {
                VDP_USB_LOG_ERROR(urbi->device->context,
                    "device %d: bad iso packet actual length in packet #%d",
                    urbi->device->device_number,
                    i);
                return vdp_usb_misuse;
            }

            urbi->devent_urb.data.packets[i].actual_length = urbi->urb.iso_packets[i].actual_length;

            actual_length += urbi->urb.iso_packets[i].actual_length;
        }

        urbi->devent_urb.status = vdphci_urb_status_completed;
    } else {
        actual_length = urbi->urb.actual_length;

        if (!translate_urb_status(urbi->urb.status, &urbi->devent_urb.status)) {
            VDP_USB_LOG_ERROR(urbi->device->context,
                "device %d: bad urb status",
                urbi->device->device_number);
            return vdp_usb_misuse;
        }
    }

    if (actual_length > original_urb->transfer_length) {
        VDP_USB_LOG_ERROR(urbi->device->context,
            "device %d: bad urb actual length",
            urbi->device->device_number);
        return vdp_usb_misuse;
    }

    urbi->devent_urb.actual_length = actual_length;

    return vdp_usb_success;
}

vdp_u32 vdp_usb_urbi_get_effective_size(struct vdp_usb_urbi* urbi)
{
    struct vdphci_hevent_urb* original_urb;

    assert(urbi);
    if (!urbi) {
        return 0;
    }

    original_urb = urbi_get_hevent_urb(urbi);

    if (VDPHCI_USB_ENDPOINT_IN(original_urb->endpoint_address)) {
        switch (original_urb->type) {
        case vdphci_urb_type_control:
        case vdphci_urb_type_bulk:
        case vdphci_urb_type_int:
            return urbi->size - (original_urb->transfer_length - urbi->devent_urb.actual_length);
        default:
            return urbi->size;
        }
    } else {
        return urbi->size;
    }
}

void vdp_usb_urbi_destroy(struct vdp_usb_urbi* urbi)
{
    assert(urbi);
    if (!urbi) {
        return;
    }

    free(urbi->urb.iso_packets);
    free((void*)urbi->event);
    free(urbi);
}
