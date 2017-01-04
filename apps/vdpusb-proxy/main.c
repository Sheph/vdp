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

static int done = 0;

static int hotplug_callback_attach(libusb_context* ctx, libusb_device* dev, libusb_hotplug_event event, void* user_data)
{
    printf("device attached: %d:%d\n",
        libusb_get_bus_number(dev), libusb_get_port_number(dev));

    return 0;
}

static int hotplug_callback_detach(libusb_context* ctx, libusb_device* dev, libusb_hotplug_event event, void* user_data)
{
    printf("device detached: %d:%d\n",
        libusb_get_bus_number(dev), libusb_get_port_number(dev));

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

    signal(SIGINT, &sig_handler);

    if (argc < 3) {
        printf("usage: vdpusb-proxy <vendor_id> <product_id>\n");
        return 1;
    }

    vendor_id = (int)strtol(argv[1], NULL, 16);
    product_id = (int)strtol(argv[2], NULL, 16);

    res = libusb_init(NULL);
    if (res != 0) {
        printf("failed to initialise libusb: %s\n", libusb_error_name(res));
        return 1;
    }

    if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        printf("libusb hotplug capabilites are not supported on this platform\n");
        libusb_exit(NULL);
        return 1;
    }

    res = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0, vendor_id,
        product_id, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback_attach, NULL, &hp[0]);
    if (res != LIBUSB_SUCCESS) {
        printf("error registering callback 0\n");
        libusb_exit(NULL);
        return 1;
    }

    res = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, vendor_id,
        product_id, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback_detach, NULL, &hp[1]);
    if (res != LIBUSB_SUCCESS) {
        printf("error registering callback 1\n");
        libusb_exit(NULL);
        return 1;
    }

    printf("waiting for %04x:%04x\n", vendor_id, product_id);

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt > 0) {
        libusb_device* dev;
        int i = 0;

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

    libusb_exit(NULL);

    return 0;
}
