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

static int ptr_array_count(void** arr)
{
    void** iter;
    int cnt = 0;

    for (iter = arr; iter && *iter; ++iter) {
        ++cnt;
    }

    return cnt;
}

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
        epi->descriptor_out.bLength = VDP_USB_DT_ENDPOINT_SIZE;
        epi->descriptor_out.bDescriptorType = VDP_USB_DT_ENDPOINT;
        epi->descriptor_out.bEndpointAddress = VDP_USB_ENDPOINT_OUT_ADDRESS(caps->address);
        epi->descriptor_out.bmAttributes = caps->type;
        epi->descriptor_out.wMaxPacketSize = vdp_cpu_to_u16le(caps->max_packet_size);
        epi->descriptor_out.bInterval = caps->interval;
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

struct vdp_usb_gadget_interfacei
{
    struct vdp_usb_gadget_interface interface;

    struct vdp_usb_gadget_interface_ops ops;

    struct vdp_usb_interface_descriptor descriptor;
};

struct vdp_usb_gadget_interface* vdp_usb_gadget_interface_create(const struct vdp_usb_gadget_interface_caps* caps,
    const struct vdp_usb_gadget_interface_ops* ops, void* priv)
{
    struct vdp_usb_gadget_interfacei* interfacei;
    int i;

    assert(caps);
    assert(ops);

    interfacei = malloc(sizeof(*interfacei));

    if (interfacei == NULL) {
        goto fail1;
    }

    memset(interfacei, 0, sizeof(*interfacei));

    memcpy(&interfacei->interface.caps, caps, sizeof(*caps));

    interfacei->interface.caps.descriptors =
        malloc(sizeof(caps->descriptors[0]) * (ptr_array_count((void**)caps->descriptors) + 1));

    if (interfacei->interface.caps.descriptors == NULL) {
        goto fail2;
    }

    for (i = 0; caps->descriptors && caps->descriptors[i]; ++i) {
        interfacei->interface.caps.descriptors[i] = malloc(caps->descriptors[i]->bLength);

        if (interfacei->interface.caps.descriptors[i] == NULL) {
            for (--i; i >= 0; --i) {
                free(interfacei->interface.caps.descriptors[i]);
            }
            goto fail3;
        }

        memcpy(interfacei->interface.caps.descriptors[i],
            caps->descriptors[i], caps->descriptors[i]->bLength);
    }

    interfacei->interface.caps.descriptors[i] = NULL;

    interfacei->interface.caps.endpoints =
        malloc(sizeof(caps->endpoints[0]) * (ptr_array_count((void**)caps->endpoints) + 1));

    if (interfacei->interface.caps.endpoints == NULL) {
        goto fail4;
    }

    interfacei->interface.caps.endpoints[ptr_array_count((void**)caps->endpoints)] = NULL;

    memcpy(interfacei->interface.caps.endpoints,
        caps->endpoints,
        sizeof(caps->endpoints[0]) * ptr_array_count((void**)caps->endpoints));

    interfacei->interface.priv = priv;
    interfacei->interface.active = 0;

    memcpy(&interfacei->ops, ops, sizeof(*ops));

    interfacei->descriptor.bLength = sizeof(interfacei->descriptor);
    interfacei->descriptor.bDescriptorType = VDP_USB_DT_INTERFACE;
    interfacei->descriptor.bInterfaceNumber = caps->number;
    interfacei->descriptor.bAlternateSetting = caps->alt_setting;
    interfacei->descriptor.bNumEndpoints = ptr_array_count((void**)caps->endpoints);
    interfacei->descriptor.bInterfaceClass = caps->klass;
    interfacei->descriptor.bInterfaceSubClass = caps->subklass;
    interfacei->descriptor.bInterfaceProtocol = caps->protocol;
    interfacei->descriptor.iInterface = caps->description;

    return &interfacei->interface;

fail4:
    for (i = 0; interfacei->interface.caps.descriptors[i]; ++i) {
        free(interfacei->interface.caps.descriptors[i]);
    }
fail3:
    free(interfacei->interface.caps.descriptors);
fail2:
    free(interfacei);
fail1:

    return NULL;
}

void vdp_usb_gadget_interface_destroy(struct vdp_usb_gadget_interface* interface)
{
    struct vdp_usb_gadget_interfacei* interfacei;
    int i;

    if (!interface) {
        return;
    }

    interfacei = vdp_containerof(interface, struct vdp_usb_gadget_interfacei, interface);

    interfacei->ops.destroy(interface);

    for (i = 0; interface->caps.endpoints[i]; ++i) {
        vdp_usb_gadget_ep_destroy(interface->caps.endpoints[i]);
    }

    free(interface->caps.endpoints);

    for (i = 0; interface->caps.descriptors[i]; ++i) {
        free(interface->caps.descriptors[i]);
    }

    free(interface->caps.descriptors);

    free(interfacei);
}

struct vdp_usb_gadget_configi
{
    struct vdp_usb_gadget_config config;

    struct vdp_usb_gadget_config_ops ops;

    struct vdp_usb_config_descriptor descriptor;

    struct vdp_usb_descriptor_header** other;
};

struct vdp_usb_gadget_config* vdp_usb_gadget_config_create(const struct vdp_usb_gadget_config_caps* caps,
    const struct vdp_usb_gadget_config_ops* ops, void* priv)
{
    struct vdp_usb_gadget_configi* configi;
    int i, j, cnt = 0;

    assert(caps);
    assert(ops);

    configi = malloc(sizeof(*configi));

    if (configi == NULL) {
        goto fail1;
    }

    memset(configi, 0, sizeof(*configi));

    memcpy(&configi->config.caps, caps, sizeof(*caps));

    configi->config.caps.interfaces =
        malloc(sizeof(caps->interfaces[0]) * (ptr_array_count((void**)caps->interfaces) + 1));

    if (configi->config.caps.interfaces == NULL) {
        goto fail2;
    }

    configi->config.caps.interfaces[ptr_array_count((void**)caps->interfaces)] = NULL;

    memcpy(configi->config.caps.interfaces,
        caps->interfaces,
        sizeof(caps->interfaces[0]) * ptr_array_count((void**)caps->interfaces));

    configi->config.priv = priv;
    configi->config.active = 0;

    memcpy(&configi->ops, ops, sizeof(*ops));

    configi->descriptor.bLength = sizeof(configi->descriptor);
    configi->descriptor.bDescriptorType = VDP_USB_DT_CONFIG;
    configi->descriptor.bNumInterfaces = ptr_array_count((void**)caps->interfaces);
    configi->descriptor.bConfigurationValue = caps->number;
    configi->descriptor.iConfiguration = caps->description;
    configi->descriptor.bmAttributes = caps->attributes;
    configi->descriptor.bMaxPower = caps->max_power;

    for (i = 0; configi->config.caps.interfaces[i]; ++i) {
        struct vdp_usb_gadget_interface* interface = configi->config.caps.interfaces[i];
        cnt += 1 + ptr_array_count((void**)interface->caps.descriptors);
        for (j = 0; interface->caps.endpoints[j]; ++j) {
            struct vdp_usb_gadget_ep* ep = interface->caps.endpoints[j];
            ++cnt;
            if ((ep->caps.type != vdp_usb_gadget_ep_control) &&
                (ep->caps.dir == vdp_usb_gadget_ep_inout)) {
                ++cnt;
            }
        }
    }

    configi->other = malloc(sizeof(configi->other[0]) * (cnt + 1));

    if (configi->other == NULL) {
        goto fail3;
    }

    cnt = 0;

    for (i = 0; configi->config.caps.interfaces[i]; ++i) {
        struct vdp_usb_gadget_interface* interface = configi->config.caps.interfaces[i];
        struct vdp_usb_gadget_interfacei* interfacei =
            vdp_containerof(interface, struct vdp_usb_gadget_interfacei, interface);

        configi->other[cnt++] = (struct vdp_usb_descriptor_header*)&interfacei->descriptor;
        memcpy(&configi->other[cnt],
            interface->caps.descriptors,
            ptr_array_count((void**)interface->caps.descriptors));
        cnt += ptr_array_count((void**)interface->caps.descriptors);

        for (j = 0; interface->caps.endpoints[j]; ++j) {
            struct vdp_usb_gadget_ep* ep = interface->caps.endpoints[j];
            struct vdp_usb_gadget_epi* epi =
                vdp_containerof(ep, struct vdp_usb_gadget_epi, ep);

            if (ep->caps.type == vdp_usb_gadget_ep_control) {
                configi->other[cnt++] = (struct vdp_usb_descriptor_header*)&epi->descriptor_out;
            } else {
                if ((ep->caps.dir & vdp_usb_gadget_ep_in) != 0) {
                    configi->other[cnt++] = (struct vdp_usb_descriptor_header*)&epi->descriptor_in;
                }
                if ((ep->caps.dir & vdp_usb_gadget_ep_out) != 0) {
                    configi->other[cnt++] = (struct vdp_usb_descriptor_header*)&epi->descriptor_out;
                }
            }
        }
    }

    configi->other[cnt] = NULL;

    return &configi->config;

fail3:
    free(configi->config.caps.interfaces);
fail2:
    free(configi);
fail1:

    return NULL;
}

void vdp_usb_gadget_config_destroy(struct vdp_usb_gadget_config* config)
{
    struct vdp_usb_gadget_configi* configi;
    int i;

    if (!config) {
        return;
    }

    configi = vdp_containerof(config, struct vdp_usb_gadget_configi, config);

    configi->ops.destroy(config);

    for (i = 0; config->caps.interfaces[i]; ++i) {
        vdp_usb_gadget_interface_destroy(config->caps.interfaces[i]);
    }

    free(configi->other);

    free(config->caps.interfaces);

    free(configi);
}

struct vdp_usb_gadgeti
{
    struct vdp_usb_gadget gadget;

    struct vdp_usb_gadget_ops ops;

    struct vdp_usb_device_descriptor descriptor;
};

struct vdp_usb_gadget* vdp_usb_gadget_create(const struct vdp_usb_gadget_caps* caps,
    const struct vdp_usb_gadget_ops* ops, void* priv)
{
    struct vdp_usb_gadgeti* gadgeti;
    int i, j;

    assert(caps);
    assert(ops);

    gadgeti = malloc(sizeof(*gadgeti));

    if (gadgeti == NULL) {
        goto fail1;
    }

    memset(gadgeti, 0, sizeof(*gadgeti));

    memcpy(&gadgeti->gadget.caps, caps, sizeof(*caps));

    for (i = 0;
        caps->string_tables && caps->string_tables[i].strings != NULL;
        ++i) {}

    gadgeti->gadget.caps.string_tables = malloc(sizeof(gadgeti->gadget.caps.string_tables[0]) * (i + 1));

    if (gadgeti->gadget.caps.string_tables == NULL) {
        goto fail2;
    }

    memset(gadgeti->gadget.caps.string_tables, 0, sizeof(gadgeti->gadget.caps.string_tables[0]) * (i + 1));

    for (i = 0;
        caps->string_tables && caps->string_tables[i].strings != NULL;
        ++i) {
        const struct vdp_usb_string* strings = caps->string_tables[i].strings;

        for (j = 0; strings && strings[j].str != NULL; ++j) {}

        gadgeti->gadget.caps.string_tables[i].strings =
            malloc(sizeof(gadgeti->gadget.caps.string_tables[i].strings[0]) * (j + 1));

        if (gadgeti->gadget.caps.string_tables[i].strings == NULL) {
            goto fail3;
        }

        gadgeti->gadget.caps.string_tables[i].language_id = caps->string_tables[i].language_id;

        memset((void*)gadgeti->gadget.caps.string_tables[i].strings, 0,
            sizeof(gadgeti->gadget.caps.string_tables[i].strings[0]) * (j + 1));

        for (j = 0; strings && strings[j].str != NULL; ++j) {
            struct vdp_usb_string* tmp =
                (struct vdp_usb_string*)&gadgeti->gadget.caps.string_tables[i].strings[j];
            tmp->index = strings[j].index;
            tmp->str = strdup(strings[j].str);
            if (tmp->str == NULL) {
                goto fail3;
            }
        }
    }

    gadgeti->gadget.caps.configs =
        malloc(sizeof(caps->configs[0]) * (ptr_array_count((void**)caps->configs) + 1));

    if (gadgeti->gadget.caps.configs == NULL) {
        goto fail3;
    }

    gadgeti->gadget.caps.configs[ptr_array_count((void**)caps->configs)] = NULL;

    memcpy(gadgeti->gadget.caps.configs,
        caps->configs,
        sizeof(caps->configs[0]) * ptr_array_count((void**)caps->configs));

    gadgeti->gadget.priv = priv;
    gadgeti->gadget.address = 0;

    memcpy(&gadgeti->ops, ops, sizeof(*ops));

    gadgeti->descriptor.bLength = sizeof(gadgeti->descriptor);
    gadgeti->descriptor.bDescriptorType = VDP_USB_DT_DEVICE;
    gadgeti->descriptor.bcdUSB = vdp_cpu_to_u16le(caps->bcd_usb);
    gadgeti->descriptor.bDeviceClass = caps->klass;
    gadgeti->descriptor.bDeviceSubClass = caps->subklass;
    gadgeti->descriptor.bDeviceProtocol = caps->protocol;
    gadgeti->descriptor.bMaxPacketSize0 = caps->endpoint0 ? caps->endpoint0->caps.max_packet_size : 64;
    gadgeti->descriptor.idVendor = vdp_cpu_to_u16le(caps->vendor_id);
    gadgeti->descriptor.idProduct = vdp_cpu_to_u16le(caps->product_id);
    gadgeti->descriptor.bcdDevice = vdp_cpu_to_u16le(caps->bcd_device);
    gadgeti->descriptor.iManufacturer = caps->manufacturer;
    gadgeti->descriptor.iProduct = caps->product;
    gadgeti->descriptor.iSerialNumber = caps->serial_number;
    gadgeti->descriptor.bNumConfigurations = ptr_array_count((void**)caps->configs);

    return &gadgeti->gadget;

fail3:
    for (i = 0; gadgeti->gadget.caps.string_tables[i].strings != NULL; ++i) {
        const struct vdp_usb_string* strings = gadgeti->gadget.caps.string_tables[i].strings;
        for (j = 0; strings[j].str != NULL; ++j) {
            free((void*)strings[j].str);
        }
        free((void*)strings);
    }
    free(gadgeti->gadget.caps.string_tables);
fail2:
    free(gadgeti);
fail1:

    return NULL;
}

void vdp_usb_gadget_event(struct vdp_usb_gadget* gadget, struct vdp_usb_event* event)
{
    switch (event->type) {
    case vdp_usb_event_none: {
        break;
    }
    case vdp_usb_event_signal: {
        break;
    }
    case vdp_usb_event_urb: {
        break;
    }
    case vdp_usb_event_unlink_urb: {
        break;
    }
    default:
        assert(0);
        break;
    }
}

void vdp_usb_gadget_destroy(struct vdp_usb_gadget* gadget)
{
    struct vdp_usb_gadgeti* gadgeti;
    int i, j;

    if (!gadget) {
        return;
    }

    gadgeti = vdp_containerof(gadget, struct vdp_usb_gadgeti, gadget);

    gadgeti->ops.destroy(gadget);

    for (i = 0; gadget->caps.configs[i]; ++i) {
        vdp_usb_gadget_config_destroy(gadget->caps.configs[i]);
    }

    vdp_usb_gadget_ep_destroy(gadget->caps.endpoint0);

    free(gadget->caps.configs);

    for (i = 0; gadget->caps.string_tables[i].strings != NULL; ++i) {
        const struct vdp_usb_string* strings = gadget->caps.string_tables[i].strings;
        for (j = 0; strings[j].str != NULL; ++j) {
            free((void*)strings[j].str);
        }
        free((void*)strings);
    }
    free(gadget->caps.string_tables);

    free(gadgeti);
}
