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

static struct vdp_usb_gadget_ep* create_proxy_gadget_ep(const struct libusb_endpoint_descriptor* desc)
{
    // TODO
    return NULL;
}

static struct vdp_usb_gadget_interface* create_proxy_gadget_interface(const struct libusb_interface_descriptor* desc)
{
    // TODO
    return NULL;
}

static struct vdp_usb_gadget_config* create_proxy_gadget_config(const struct libusb_config_descriptor* desc)
{
    // TODO
    return NULL;
}

static struct vdp_usb_gadget* create_proxy_gadget(const struct libusb_device_descriptor* dev_desc,
    struct libusb_config_descriptor** config_descs)
{
    // TODO
    return NULL;
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
