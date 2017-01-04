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
#include "libusb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

struct proxy_device;

static int done = 0;
static struct vdp_usb_device* vdp_devs[5];
static struct proxy_device* proxy_devs[5];

struct proxy_device
{
    libusb_device_handle* handle;
    struct vdp_usb_gadget* gadget;
};

static void proxy_gadget_ep_enable(struct vdp_usb_gadget_ep* ep, int value)
{
}

static void proxy_gadget_ep_enqueue(struct vdp_usb_gadget_ep* ep, struct vdp_usb_gadget_request* request)
{
    request->status = vdp_usb_urb_status_completed;
    request->complete(request);
    request->destroy(request);
}

static void proxy_gadget_ep_dequeue(struct vdp_usb_gadget_ep* ep, struct vdp_usb_gadget_request* request)
{
    request->status = vdp_usb_urb_status_unlinked;
    request->complete(request);
    request->destroy(request);
}

static vdp_usb_urb_status proxy_gadget_ep_clear_stall(struct vdp_usb_gadget_ep* ep)
{
    return vdp_usb_urb_status_completed;
}

static void proxy_gadget_ep_destroy(struct vdp_usb_gadget_ep* ep)
{
}

static void proxy_gadget_interface_enable(struct vdp_usb_gadget_interface* interface, int value)
{
}

static void proxy_gadget_interface_destroy(struct vdp_usb_gadget_interface* interface)
{
}

static void proxy_gadget_config_enable(struct vdp_usb_gadget_config* config, int value)
{
}

static void proxy_gadget_config_destroy(struct vdp_usb_gadget_config* config)
{
}

static void proxy_gadget_reset(struct vdp_usb_gadget* gadget, int start)
{
}

static void proxy_gadget_power(struct vdp_usb_gadget* gadget, int on)
{
}

static void proxy_gadget_set_address(struct vdp_usb_gadget* gadget, vdp_u32 address)
{
}

static void proxy_gadget_destroy(struct vdp_usb_gadget* gadget)
{
}

static struct vdp_usb_gadget_ep* create_proxy_gadget_ep(const struct libusb_endpoint_descriptor* desc,
    vdp_usb_gadget_ep_dir dir)
{
    static const struct vdp_usb_gadget_ep_ops ops =
    {
        .enable = proxy_gadget_ep_enable,
        .enqueue = proxy_gadget_ep_enqueue,
        .dequeue = proxy_gadget_ep_dequeue,
        .clear_stall = proxy_gadget_ep_clear_stall,
        .destroy = proxy_gadget_ep_destroy
    };

    struct vdp_usb_gadget_ep_caps caps;

    memset(&caps, 0, sizeof(caps));

    caps.address = VDP_USB_URB_ENDPOINT_NUMBER(desc->bEndpointAddress);
    caps.dir = dir;
    caps.type = desc->bmAttributes;
    caps.max_packet_size = desc->wMaxPacketSize;
    caps.interval = desc->bInterval;

    return vdp_usb_gadget_ep_create(&caps, &ops, NULL);
}

static struct vdp_usb_gadget_interface* create_proxy_gadget_interface(const struct libusb_interface_descriptor* desc)
{
    static const struct vdp_usb_gadget_interface_ops ops =
    {
        .enable = proxy_gadget_interface_enable,
        .destroy = proxy_gadget_interface_destroy
    };
    struct vdp_usb_gadget_interface_caps caps;
    uint8_t i, j, num_endpoints = 0;
    struct vdp_usb_gadget_interface* interface = NULL;

    memset(&caps, 0, sizeof(caps));

    caps.number = desc->bInterfaceNumber;
    caps.alt_setting = desc->bAlternateSetting;
    caps.klass = desc->bInterfaceClass;
    caps.subklass = desc->bInterfaceSubClass;
    caps.protocol = desc->bInterfaceProtocol;
    caps.description = desc->iInterface;
    caps.descriptors = NULL;
    caps.endpoints = malloc(sizeof(*caps.endpoints) * ((int)desc->bNumEndpoints + 1));

    if (!caps.endpoints) {
        return NULL;
    }

    for (i = 0; i < desc->bNumEndpoints; ++i) {
        uint8_t number = VDP_USB_URB_ENDPOINT_NUMBER(desc->endpoint[i].bEndpointAddress);
        int found = 0;

        for (j = 0; j < num_endpoints; ++j) {
            if (caps.endpoints[j]->caps.address == number) {
                found = 1;
                break;
            }
        }

        if (found) {
            continue;
        }

        vdp_usb_gadget_ep_dir dir = VDP_USB_URB_ENDPOINT_IN(desc->endpoint[i].bEndpointAddress) ?
            vdp_usb_gadget_ep_in : vdp_usb_gadget_ep_out;

        for (j = i + 1; j < desc->bNumEndpoints; ++j) {
            if (VDP_USB_URB_ENDPOINT_NUMBER(desc->endpoint[j].bEndpointAddress) == number) {
                dir |= VDP_USB_URB_ENDPOINT_IN(desc->endpoint[j].bEndpointAddress) ?
                    vdp_usb_gadget_ep_in : vdp_usb_gadget_ep_out;
            }
        }

        caps.endpoints[num_endpoints] = create_proxy_gadget_ep(&desc->endpoint[i], dir);
        if (!caps.endpoints[num_endpoints]) {
            goto out2;
        }

        ++num_endpoints;
    }

    caps.endpoints[num_endpoints] = NULL;

    interface = vdp_usb_gadget_interface_create(&caps, &ops, NULL);

    if (interface) {
        goto out1;
    }

out2:
    for (i = 0; i < num_endpoints; ++i) {
        vdp_usb_gadget_ep_destroy(caps.endpoints[i]);
    }
out1:
    free(caps.endpoints);

    return interface;
}

static struct vdp_usb_gadget_config* create_proxy_gadget_config(const struct libusb_config_descriptor* desc)
{
    static const struct vdp_usb_gadget_config_ops ops =
    {
        .enable = proxy_gadget_config_enable,
        .destroy = proxy_gadget_config_destroy
    };
    struct vdp_usb_gadget_config_caps caps;
    uint8_t i;
    int num_interfaces = 0, j;
    struct vdp_usb_gadget_config* config = NULL;

    memset(&caps, 0, sizeof(caps));

    for (i = 0; i < desc->bNumInterfaces; ++i) {
        num_interfaces += desc->interface[i].num_altsetting;
    }

    caps.number = desc->bConfigurationValue;
    caps.attributes = desc->bmAttributes;
    caps.max_power = desc->MaxPower;
    caps.description = desc->iConfiguration;
    caps.interfaces = malloc(sizeof(*caps.interfaces) * (num_interfaces + 1));

    if (!caps.interfaces) {
        return NULL;
    }

    num_interfaces = 0;

    for (i = 0; i < desc->bNumInterfaces; ++i) {
        for (j = 0; j < desc->interface[i].num_altsetting; ++j) {
            caps.interfaces[num_interfaces] = create_proxy_gadget_interface(&desc->interface[i].altsetting[j]);
            if (!caps.interfaces[num_interfaces]) {
                goto out2;
            }
            ++num_interfaces;
        }
    }

    caps.interfaces[num_interfaces] = NULL;

    config = vdp_usb_gadget_config_create(&caps, &ops, NULL);

    if (config) {
        goto out1;
    }

out2:
    for (j = 0; j < num_interfaces; ++j) {
        vdp_usb_gadget_interface_destroy(caps.interfaces[j]);
    }
out1:
    free(caps.interfaces);

    return config;
}

static struct vdp_usb_gadget* create_proxy_gadget(const struct libusb_device_descriptor* dev_desc,
    struct libusb_config_descriptor** config_descs)
{
    static const struct vdp_usb_gadget_ops ops =
    {
        .reset = proxy_gadget_reset,
        .power = proxy_gadget_power,
        .set_address = proxy_gadget_set_address,
        .destroy = proxy_gadget_destroy
    };
    struct vdp_usb_gadget_caps caps;
    struct libusb_endpoint_descriptor ep0_desc;
    uint8_t i, num_configs = 0;
    struct vdp_usb_gadget* gadget = NULL;

    memset(&caps, 0, sizeof(caps));
    memset(&ep0_desc, 0, sizeof(ep0_desc));

    ep0_desc.bEndpointAddress = 0;
    ep0_desc.bmAttributes = VDP_USB_ENDPOINT_XFER_CONTROL;
    ep0_desc.wMaxPacketSize = dev_desc->bMaxPacketSize0;
    ep0_desc.bInterval = 0;

    caps.bcd_usb = dev_desc->bcdUSB;
    caps.bcd_device = dev_desc->bcdDevice;
    caps.klass = dev_desc->bDeviceClass;
    caps.subklass = dev_desc->bDeviceSubClass;
    caps.protocol = dev_desc->bDeviceProtocol;
    caps.vendor_id = dev_desc->idVendor;
    caps.product_id = dev_desc->idProduct;
    caps.manufacturer = dev_desc->iManufacturer;
    caps.product = dev_desc->iProduct;
    caps.serial_number = dev_desc->iSerialNumber;
    caps.string_tables = NULL;
    caps.configs = malloc(sizeof(*caps.configs) * ((int)dev_desc->bNumConfigurations + 1));

    if (!caps.configs) {
        return NULL;
    }

    caps.endpoint0 = create_proxy_gadget_ep(&ep0_desc, vdp_usb_gadget_ep_inout);

    if (!caps.endpoint0) {
        goto out1;
    }

    for (i = 0; i < dev_desc->bNumConfigurations; ++i) {
        caps.configs[num_configs] = create_proxy_gadget_config(config_descs[i]);
        if (!caps.configs[num_configs]) {
            goto out2;
        }

        ++num_configs;
    }

    caps.configs[num_configs] = NULL;

    gadget = vdp_usb_gadget_create(&caps, &ops, NULL);

    if (gadget) {
        goto out1;
    }

out2:
    for (i = 0; i < num_configs; ++i) {
        vdp_usb_gadget_config_destroy(caps.configs[i]);
    }
    vdp_usb_gadget_ep_destroy(caps.endpoint0);
out1:
    free(caps.configs);

    return gadget;
}

static struct proxy_device* proxy_device_create(libusb_device_handle* handle)
{
    struct proxy_device* proxy_dev;
    struct libusb_device_descriptor dev_desc;
    int res;
    libusb_device* dev = libusb_get_device(handle);
    uint8_t i;
    struct libusb_config_descriptor** config_descs;

    proxy_dev = malloc(sizeof(*proxy_dev));
    if (!proxy_dev) {
        goto fail1;
    }

    proxy_dev->handle = handle;

    res = libusb_get_device_descriptor(dev, &dev_desc);
    if (res != LIBUSB_SUCCESS) {
        printf("error getting device descriptor\n");
        goto fail2;
    }

    config_descs = malloc(sizeof(*config_descs) * (int)dev_desc.bNumConfigurations);
    if (!config_descs) {
        printf("cannot allocate mem for config descs\n");
        goto fail2;
    }

    memset(config_descs, 0,
        sizeof(*config_descs) * (int)dev_desc.bNumConfigurations);

    for (i = 0; i < dev_desc.bNumConfigurations; ++i) {
        res = libusb_get_config_descriptor(dev, i, &config_descs[i]);
        if (res != LIBUSB_SUCCESS) {
            printf("error getting config descriptor\n");
            goto fail3;
        }
    }

    proxy_dev->gadget = create_proxy_gadget(&dev_desc, config_descs);
    if (!proxy_dev->gadget) {
        printf("cannot create proxy gadget\n");
        goto fail3;
    }

    for (i = 0; i < dev_desc.bNumConfigurations; ++i) {
        libusb_free_config_descriptor(config_descs[i]);
    }
    free(config_descs);

    return proxy_dev;

fail3:
    for (i = 0; i < dev_desc.bNumConfigurations; ++i) {
        libusb_free_config_descriptor(config_descs[i]);
    }
    free(config_descs);
fail2:
    free(proxy_dev);
fail1:

    return NULL;
}

static void proxy_device_destroy(struct proxy_device* proxy)
{
    vdp_usb_gadget_destroy(proxy->gadget);
    libusb_close(proxy->handle);
    free(proxy);
}

static int hotplug_callback_attach(libusb_context* ctx, libusb_device* dev, libusb_hotplug_event event, void* user_data)
{
    int i;
    vdp_usb_result vdp_res;

    printf("device attached: %d:%d\n",
        libusb_get_bus_number(dev), libusb_get_port_number(dev));

    for (i = 0; i < sizeof(proxy_devs)/sizeof(proxy_devs[0]); ++i) {
        if (!proxy_devs[i]) {
            libusb_device_handle* handle;
            int res;

            res = libusb_open(dev, &handle);
            if (res != LIBUSB_SUCCESS) {
                printf("error opening device\n");
                break;
            }

            proxy_devs[i] = proxy_device_create(handle);
            if (!proxy_devs[i]) {
                libusb_close(handle);
                break;
            }

            vdp_res = vdp_usb_device_attach(vdp_devs[i]);
            if (vdp_res != vdp_usb_success) {
                printf("failed to attach device: %s\n", vdp_usb_result_to_str(vdp_res));
                proxy_device_destroy(proxy_devs[i]);
                proxy_devs[i] = NULL;
            }

            break;
        }
    }

    return 0;
}

static int hotplug_callback_detach(libusb_context* ctx, libusb_device* dev, libusb_hotplug_event event, void* user_data)
{
    int i;

    printf("device detached: %d:%d\n",
        libusb_get_bus_number(dev), libusb_get_port_number(dev));

    for (i = 0; i < sizeof(proxy_devs)/sizeof(proxy_devs[0]); ++i) {
        if (proxy_devs[i]) {
            libusb_device* other_dev = libusb_get_device(proxy_devs[i]->handle);
            if ((libusb_get_bus_number(dev) == libusb_get_bus_number(other_dev)) &&
                (libusb_get_port_number(dev) == libusb_get_port_number(other_dev))) {
                proxy_device_destroy(proxy_devs[i]);
                proxy_devs[i] = NULL;
                vdp_usb_device_detach(vdp_devs[i]);
                break;
            }
        }
    }

    return 0;
}

static void sig_handler(int signum)
{
    done = 1;
}

int main(int argc, char* argv[])
{
    libusb_hotplug_callback_handle hp[2];
    int product_id, vendor_id;
    int res;
    libusb_device** devs;
    ssize_t cnt;
    struct vdp_usb_context* ctx;
    vdp_usb_result vdp_res;
    int i;

    signal(SIGINT, &sig_handler);

    if (argc < 3) {
        printf("usage: vdpusb-proxy <vendor_id> <product_id>\n");
        res = 1;
        goto out1;
    }

    vendor_id = (int)strtol(argv[1], NULL, 16);
    product_id = (int)strtol(argv[2], NULL, 16);

    res = libusb_init(NULL);
    if (res != 0) {
        printf("failed to initialise libusb: %s\n", libusb_error_name(res));
        res = 1;
        goto out1;
    }

    vdp_res = vdp_usb_init(stdout, vdp_log_debug, &ctx);
    if (vdp_res != vdp_usb_success) {
        printf("failed to initialise vdpusb: %s\n", vdp_usb_result_to_str(vdp_res));
        res = 1;
        goto out2;
    }

    for (i = 0; i < sizeof(vdp_devs)/sizeof(vdp_devs[0]); ++i) {
        vdp_res = vdp_usb_device_open(ctx, i, &vdp_devs[i]);
        if (vdp_res != vdp_usb_success) {
            res = 1;
            goto out3;
        }
    }

    if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        printf("libusb hotplug capabilites are not supported on this platform\n");
        res = 1;
        goto out3;
    }

    res = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0, vendor_id,
        product_id, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback_attach, NULL, &hp[0]);
    if (res != LIBUSB_SUCCESS) {
        printf("error registering callback 0\n");
        res = 1;
        goto out3;
    }

    res = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, vendor_id,
        product_id, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback_detach, NULL, &hp[1]);
    if (res != LIBUSB_SUCCESS) {
        printf("error registering callback 1\n");
        res = 1;
        goto out3;
    }

    printf("waiting for %04x:%04x\n", vendor_id, product_id);

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt > 0) {
        libusb_device* dev;
        i = 0;

        while ((dev = devs[i++]) != NULL) {
            struct libusb_device_descriptor desc;
            res = libusb_get_device_descriptor(dev, &desc);
            if ((res == 0) && (desc.idVendor == vendor_id) && (desc.idProduct == product_id)) {
                hotplug_callback_attach(NULL, dev, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
            }
        }

        libusb_free_device_list(devs, 1);
    }

    while (1) {
        res = libusb_handle_events(NULL);
        if (res != 0) {
            printf("libusb_handle_events() failed: %s\n", libusb_error_name(res));
            break;
        }
    }

    for (i = 0; i < sizeof(proxy_devs)/sizeof(proxy_devs[0]); ++i) {
        if (proxy_devs[i]) {
            proxy_device_destroy(proxy_devs[i]);
            vdp_usb_device_detach(vdp_devs[i]);
        }
    }

    res = 0;

out3:
    for (i = 0; i < sizeof(vdp_devs)/sizeof(vdp_devs[0]); ++i) {
        if (vdp_devs[i]) {
            vdp_usb_device_close(vdp_devs[i]);
        }
    }
    vdp_usb_cleanup(ctx);
out2:
    libusb_exit(NULL);
out1:

    return res;
}
