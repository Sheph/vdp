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

#ifndef _VDP_USB_URBI_H_
#define _VDP_USB_URBI_H_

#include "vdp/usb.h"
#include "vdphci-common.h"

struct vdp_usb_device;

struct vdp_usb_urbi
{
    /*
     * Allocated urbi size.
     */
    vdp_u32 size;

    /*
     * The device from which this URB came.
     */
    struct vdp_usb_device* device;

    /*
     * Original hevent, i.e "hevent header + urb hevent"
     */
    const void* event;

    struct vdp_usb_urb urb;

    struct vdphci_devent_header devent_header;

    struct vdphci_devent_urb devent_urb;
};

/*
 * 'event' is a pointer to "hevent header + urb hevent" read from fd, if this function returns success
 * then 'event' shouldn't be freed by caller, it'll be freed in 'vdp_usb_urbi_destroy'
 */
vdp_usb_result vdp_usb_urbi_create(struct vdp_usb_device* device,
    const void* event,
    size_t event_size,
    struct vdp_usb_urbi** urbi);

/*
 * When user is done with the URB we must update 'devent_urb' fields from 'urb' fields,
 * then, we can send everything starting from 'devent_header' to the kernel.
 */
vdp_usb_result vdp_usb_urbi_update(struct vdp_usb_urbi* urbi);

/*
 * Useful size of an urbi. For example, for IN bulk transfer effective size will be:
 * urbi->size - (transfer_length - actual_length)
 */
vdp_u32 vdp_usb_urbi_get_effective_size(struct vdp_usb_urbi* urbi);

void vdp_usb_urbi_destroy(struct vdp_usb_urbi* urbi);

#endif
