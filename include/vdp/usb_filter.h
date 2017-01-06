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

#ifndef _VDP_USB_FILTER_H_
#define _VDP_USB_FILTER_H_

#include "vdp/usb_util.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vdp_usb_filter_ops
{
    vdp_usb_urb_status (*get_device_descriptor)(void* /*user_data*/,
        struct vdp_usb_device_descriptor* /*descriptor*/);

    vdp_usb_urb_status (*get_qualifier_descriptor)(void* /*user_data*/,
        struct vdp_usb_qualifier_descriptor* /*descriptor*/);

    vdp_usb_urb_status (*get_config_descriptor)(void* /*user_data*/,
        vdp_u8 /*index*/,
        struct vdp_usb_config_descriptor* /*descriptor*/,
        struct vdp_usb_descriptor_header*** /*other*/);

    vdp_usb_urb_status (*get_string_descriptor)(void* /*user_data*/,
        const struct vdp_usb_string_table** /*tables*/);

    vdp_usb_urb_status (*set_address)(void* /*user_data*/,
        vdp_u16 /*address*/);

    vdp_usb_urb_status (*set_configuration)(void* /*user_data*/,
        vdp_u8 /*configuration*/);

    vdp_usb_urb_status (*get_status)(void* /*user_data*/,
        vdp_u8 /*recipient*/, vdp_u8 /*index*/, vdp_u16* /*status*/);

    vdp_usb_urb_status (*enable_feature)(void* /*user_data*/,
        vdp_u8 /*recipient*/, vdp_u8 /*index*/, vdp_u16 /*feature*/, int /*enable*/);

    vdp_usb_urb_status (*get_interface)(void* /*user_data*/,
        vdp_u8 /*interface*/, vdp_u8* /*alt_setting*/);

    vdp_usb_urb_status (*set_interface)(void* /*user_data*/,
        vdp_u8 /*interface*/, vdp_u8 /*alt_setting*/);

    vdp_usb_urb_status (*set_descriptor)(void* /*user_data*/,
        vdp_u16 /*value*/, vdp_u16 /*index*/, const vdp_byte* /*data*/,
        vdp_u32 /*len*/);
};

/*
 * Filters standard device requests and calls appropriate callbacks from
 * 'ops'. Updates 'urb' status, transfer_buffer and actual_length, but does
 * not complete 'urb'.
 */
int vdp_usb_filter(struct vdp_usb_urb* urb, struct vdp_usb_filter_ops* ops,
    void* user_data);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
