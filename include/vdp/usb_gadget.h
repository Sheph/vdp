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

#ifndef _VDP_USB_GADGET_H_
#define _VDP_USB_GADGET_H_

#include "vdp/usb_util.h"
#include "vdp/list.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * USB Gadget API.
 *
 * The entities here are: request, caps, endpoint, interface, config and gadget.
 *
 * reguests are allocated by the gadget api and passed to endpoints, it's your
 * responsibility to call request->complete in order to complete the underlying
 * URB and then request->destroy to free memory.
 *
 * caps are passed when constructing an object, they're always deep-copied into
 * object. if there're other objects in caps then their ownership is also passed
 * to the object being constructed (only on successful construction). e.g.
 * interface will take ownership of all endpoints, thus, when calling
 * vdp_usb_gadget_interface_destroy it'll automatically destroy all endpoints.
 */

/*
 * USB Gadget Request.
 * @{
 */

typedef enum
{
    vdp_usb_gadget_request_standard = VDP_USB_REQUESTTYPE_TYPE_STANDARD,
    vdp_usb_gadget_request_class = VDP_USB_REQUESTTYPE_TYPE_CLASS,
    vdp_usb_gadget_request_vendor = VDP_USB_REQUESTTYPE_TYPE_VENDOR,
    vdp_usb_gadget_request_reserved = VDP_USB_REQUESTTYPE_TYPE_RESERVED
} vdp_usb_gadget_request_type;

typedef enum
{
    vdp_usb_gadget_request_device = VDP_USB_REQUESTTYPE_RECIPIENT_DEVICE,
    vdp_usb_gadget_request_interface = VDP_USB_REQUESTTYPE_RECIPIENT_INTERFACE,
    vdp_usb_gadget_request_endpoint = VDP_USB_REQUESTTYPE_RECIPIENT_ENDPOINT,
    vdp_usb_gadget_request_other = VDP_USB_REQUESTTYPE_RECIPIENT_OTHER
} vdp_usb_gadget_request_recipient;

struct vdp_usb_gadget_control_setup
{
    vdp_usb_gadget_request_type type;
    vdp_usb_gadget_request_recipient recipient;
    vdp_u32 request;
    vdp_u32 value;
    vdp_u32 index;
};

typedef enum
{
    vdp_usb_gadget_request_zero_packet = VDP_USB_URB_ZERO_PACKET
} vdp_usb_gadget_request_flag;

struct vdp_usb_gadget_request
{
    struct vdp_list entry;

    vdp_u32 id;

    int in;

    vdp_u32 flags;

    struct vdp_usb_gadget_control_setup setup_packet;

    vdp_byte* transfer_buffer;

    vdp_u32 transfer_length;

    vdp_u32 actual_length;

    vdp_u32 number_of_packets;

    vdp_u32 interval_us;

    vdp_usb_urb_status status;

    struct vdp_usb_iso_packet* iso_packets;

    vdp_usb_result (*complete)(struct vdp_usb_gadget_request* /*request*/);

    void (*destroy)(struct vdp_usb_gadget_request* /*request*/);
};

/*
 * @}
 */

/*
 * USB Gadget Endpoint.
 * @{
 */

struct vdp_usb_gadget_ep;

struct vdp_usb_gadget_ep_ops
{
    void (*enable)(struct vdp_usb_gadget_ep* /*ep*/, int /*value*/);

    void (*enqueue)(struct vdp_usb_gadget_ep* /*ep*/, struct vdp_usb_gadget_request* /*request*/);
    void (*dequeue)(struct vdp_usb_gadget_ep* /*ep*/, struct vdp_usb_gadget_request* /*request*/);

    vdp_usb_urb_status (*clear_stall)(struct vdp_usb_gadget_ep* /*ep*/);

    void (*destroy)(struct vdp_usb_gadget_ep* /*ep*/);
};

typedef enum
{
    vdp_usb_gadget_ep_in = (1 << 0),
    vdp_usb_gadget_ep_out = (1 << 1),
    vdp_usb_gadget_ep_inout = vdp_usb_gadget_ep_in | vdp_usb_gadget_ep_out
} vdp_usb_gadget_ep_dir;

typedef enum
{
    vdp_usb_gadget_ep_control = VDP_USB_ENDPOINT_XFER_CONTROL,
    vdp_usb_gadget_ep_iso = VDP_USB_ENDPOINT_XFER_ISO,
    vdp_usb_gadget_ep_bulk = VDP_USB_ENDPOINT_XFER_BULK,
    vdp_usb_gadget_ep_int = VDP_USB_ENDPOINT_XFER_INT
} vdp_usb_gadget_ep_type;

struct vdp_usb_gadget_ep_caps
{
    vdp_u32 address;
    vdp_usb_gadget_ep_dir dir;
    vdp_usb_gadget_ep_type type;
    vdp_u32 max_packet_size;
    vdp_u32 interval;
};

struct vdp_usb_gadget_ep
{
    struct vdp_usb_gadget_ep_caps caps;
    void* priv;

    int active;
    int stalled;

    struct vdp_list requests;
};

struct vdp_usb_gadget_ep* vdp_usb_gadget_ep_create(const struct vdp_usb_gadget_ep_caps* caps,
    const struct vdp_usb_gadget_ep_ops* ops, void* priv);

void vdp_usb_gadget_ep_destroy(struct vdp_usb_gadget_ep* ep);

/*
 * @}
 */

/*
 * USB Gadget Interface.
 * @{
 */

struct vdp_usb_gadget_interface;

struct vdp_usb_gadget_interface_ops
{
    void (*enable)(struct vdp_usb_gadget_interface* /*interface*/, int /*value*/);

    void (*destroy)(struct vdp_usb_gadget_interface* /*interface*/);
};

struct vdp_usb_gadget_interface_caps
{
    vdp_u32 number;
    vdp_u32 alt_setting;
    vdp_u32 klass;
    vdp_u32 subklass;
    vdp_u32 protocol;
    int description;
    struct vdp_usb_descriptor_header** descriptors;
    struct vdp_usb_gadget_ep** endpoints;
};

struct vdp_usb_gadget_interface
{
    struct vdp_usb_gadget_interface_caps caps;
    void* priv;

    int active;
};

struct vdp_usb_gadget_interface* vdp_usb_gadget_interface_create(const struct vdp_usb_gadget_interface_caps* caps,
    const struct vdp_usb_gadget_interface_ops* ops, void* priv);

void vdp_usb_gadget_interface_destroy(struct vdp_usb_gadget_interface* interface);

/*
 * @}
 */

/*
 * USB Gadget Configuration.
 * @{
 */

struct vdp_usb_gadget_config;

struct vdp_usb_gadget_config_ops
{
    void (*enable)(struct vdp_usb_gadget_config* /*config*/, int /*value*/);

    void (*destroy)(struct vdp_usb_gadget_config* /*config*/);
};

typedef enum
{
    vdp_usb_gadget_config_att_one       = VDP_USB_CONFIG_ATT_ONE,
    vdp_usb_gadget_config_att_selfpower = VDP_USB_CONFIG_ATT_SELFPOWER,
    vdp_usb_gadget_config_att_wakeup    = VDP_USB_CONFIG_ATT_WAKEUP,
    vdp_usb_gadget_config_att_battery   = VDP_USB_CONFIG_ATT_BATTERY
} vdp_usb_gadget_config_att;

struct vdp_usb_gadget_config_caps
{
    vdp_u32 number;
    vdp_u32 attributes;
    vdp_u32 max_power;
    int description;
    struct vdp_usb_gadget_interface** interfaces;
};

struct vdp_usb_gadget_config
{
    struct vdp_usb_gadget_config_caps caps;
    void* priv;

    int active;
};

struct vdp_usb_gadget_config* vdp_usb_gadget_config_create(const struct vdp_usb_gadget_config_caps* caps,
    const struct vdp_usb_gadget_config_ops* ops, void* priv);

void vdp_usb_gadget_config_destroy(struct vdp_usb_gadget_config* config);

/*
 * @}
 */

/*
 * USB Gadget.
 * @{
 */

struct vdp_usb_gadget;

struct vdp_usb_gadget_ops
{
    void (*reset)(struct vdp_usb_gadget* /*gadget*/, int /*start*/);

    void (*power)(struct vdp_usb_gadget* /*gadget*/, int /*on*/);

    void (*set_address)(struct vdp_usb_gadget* /*gadget*/, vdp_u32 /*address*/);

    void (*destroy)(struct vdp_usb_gadget* /*gadget*/);
};

struct vdp_usb_gadget_caps
{
    vdp_u32 bcd_usb;
    vdp_u32 bcd_device;
    vdp_u32 klass;
    vdp_u32 subklass;
    vdp_u32 protocol;
    vdp_u32 vendor_id;
    vdp_u32 product_id;
    int manufacturer;
    int product;
    int serial_number;
    struct vdp_usb_string_table* string_tables;
    struct vdp_usb_gadget_ep* endpoint0;
    struct vdp_usb_gadget_config** configs;
};

struct vdp_usb_gadget
{
    struct vdp_usb_gadget_caps caps;
    void* priv;

    vdp_u32 address;
};

struct vdp_usb_gadget* vdp_usb_gadget_create(const struct vdp_usb_gadget_caps* caps,
    const struct vdp_usb_gadget_ops* ops, void* priv);

void vdp_usb_gadget_event(struct vdp_usb_gadget* gadget, struct vdp_usb_event* event);

void vdp_usb_gadget_destroy(struct vdp_usb_gadget* gadget);

/*
 * @}
 */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
