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

#include "vdp/usb_gadget.h"
#include "vdp/byte_order.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct vdp_usb_gadget_epi
{
    struct vdp_usb_gadget_ep ep;

    struct vdp_usb_gadget_ep_ops ops;

    struct vdp_usb_endpoint_descriptor descriptor_in;
    struct vdp_usb_endpoint_descriptor descriptor_out;
};

struct vdp_usb_gadget_ep* vdp_usb_gadget_ep_create(const struct vdp_usb_gadget_ep_caps* caps,
    const struct vdp_usb_gadget_ep_ops* ops, void* priv)
{
    struct vdp_usb_gadget_epi* epi;

    assert(caps);
    assert(ops);

    epi = malloc(sizeof(*epi));

    if (epi == NULL) {
        return NULL;
    }

    memset(epi, 0, sizeof(*epi));

    memcpy(&epi->ep.caps, caps, sizeof(*caps));
    epi->ep.priv = priv;
    epi->ep.stalled = 0;

    memcpy(&epi->ops, ops, sizeof(*ops));

    if ((caps->dir & vdp_usb_gadget_ep_in) != 0) {
        epi->descriptor_in.bLength = VDP_USB_DT_ENDPOINT_SIZE;
        epi->descriptor_in.bDescriptorType = VDP_USB_DT_ENDPOINT;
        epi->descriptor_in.bEndpointAddress = VDP_USB_ENDPOINT_IN_ADDRESS(caps->address);
        epi->descriptor_in.bmAttributes = caps->type;
        epi->descriptor_in.wMaxPacketSize = vdp_cpu_to_u16le(caps->max_packet_size);
        epi->descriptor_in.bInterval = caps->interval;
    }

    if ((caps->dir & vdp_usb_gadget_ep_out) != 0) {
        epi->descriptor_in.bLength = VDP_USB_DT_ENDPOINT_SIZE;
        epi->descriptor_in.bDescriptorType = VDP_USB_DT_ENDPOINT;
        epi->descriptor_in.bEndpointAddress = VDP_USB_ENDPOINT_OUT_ADDRESS(caps->address);
        epi->descriptor_in.bmAttributes = caps->type;
        epi->descriptor_in.wMaxPacketSize = vdp_cpu_to_u16le(caps->max_packet_size);
        epi->descriptor_in.bInterval = caps->interval;
    }

    return &epi->ep;
}

void vdp_usb_gadget_ep_destroy(struct vdp_usb_gadget_ep* ep)
{
    struct vdp_usb_gadget_epi* epi;

    if (!ep) {
        return;
    }

    epi = vdp_containerof(ep, struct vdp_usb_gadget_epi, ep);

    epi->ops.destroy(ep);

    free(epi);
}
