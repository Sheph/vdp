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
#include "vdp/usb_hid.h"
#include "vdp/byte_order.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

static int done = 0;

static void print_error(vdp_usb_result res, const char* fmt, ...)
{
    if (fmt) {
        printf("error: ");
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf(" (%s)\n", vdp_usb_result_to_str(res));
    } else {
        printf("error: %s\n", vdp_usb_result_to_str(res));
    }
}

#pragma pack(1)
static const vdp_u8 test_report_descriptor[] =
{
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x08,        //     Usage Maximum (0x08)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x08,        //     Report Count (8)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x00,        //     Report Count (0)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)
    0x09, 0x40,        //     Usage (0x40)
    0x95, 0x02,        //     Report Count (2)
    0x75, 0x08,        //     Report Size (8)
    0x15, 0x81,        //     Logical Minimum (129)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (129)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x16, 0x01, 0x80,  //     Logical Minimum (32769)
    0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
};
#pragma pack()

static void ep0_enable(struct vdp_usb_gadget_ep* ep, int value)
{
    printf("ep0 enable = %d\n", value);
}

static void ep0_enqueue(struct vdp_usb_gadget_ep* ep, struct vdp_usb_gadget_request* request)
{
    request->status = vdp_usb_urb_status_stall;

    if (ep->stalled) {
        request->complete(request);
        request->destroy(request);
        return;
    }

    if (request->setup_packet.request == VDP_USB_REQUEST_GET_DESCRIPTOR) {
        if ((request->setup_packet.type == vdp_usb_gadget_request_standard) &&
            (request->setup_packet.recipient == vdp_usb_gadget_request_interface) &&
            request->in) {
            switch (request->setup_packet.value >> 8) {
            case VDP_USB_HID_DT_REPORT:
                printf("ep0 get_hid_report_descriptor %u\n", request->id);
                request->actual_length = vdp_min(request->transfer_length, sizeof(test_report_descriptor));
                memcpy(request->transfer_buffer, test_report_descriptor, request->actual_length);
                request->status = vdp_usb_urb_status_completed;
                break;
            default:
                break;
            }
        }
    } else if (request->setup_packet.request == VDP_USB_HID_REQUEST_SET_IDLE) {
        if ((request->setup_packet.type == vdp_usb_gadget_request_class) &&
            (request->setup_packet.recipient == vdp_usb_gadget_request_interface) &&
            !request->in) {
            printf("ep0 set_idle %u\n", request->id);
            request->status = vdp_usb_urb_status_completed;
        }
    }

    if (request->status == vdp_usb_urb_status_stall) {
        printf("ep0 %u stalled\n", request->id);
    }

    request->complete(request);
    request->destroy(request);
}

static void ep0_dequeue(struct vdp_usb_gadget_ep* ep, struct vdp_usb_gadget_request* request)
{
    printf("ep0 dequeue %u\n", request->id);
    request->status = vdp_usb_urb_status_unlinked;
    request->complete(request);
    request->destroy(request);
}

static vdp_usb_urb_status ep0_clear_stall(struct vdp_usb_gadget_ep* ep)
{
    printf("ep0 clear stall\n");
    return vdp_usb_urb_status_completed;
}

static void ep0_destroy(struct vdp_usb_gadget_ep* ep)
{
    printf("ep0 destroy\n");
}

static void ep1_enable(struct vdp_usb_gadget_ep* ep, int value)
{
    printf("ep1 enable = %d\n", value);
}

static void ep1_enqueue(struct vdp_usb_gadget_ep* ep, struct vdp_usb_gadget_request* request)
{
    request->status = vdp_usb_urb_status_stall;

    if (ep->stalled) {
        request->complete(request);
        request->destroy(request);
        return;
    }

    if (request->transfer_length >= 8) {
        printf("ep1 %u\n", request->id);

        usleep(request->interval_us);

        request->transfer_buffer[0] = 0;
        request->transfer_buffer[1] = 0;
        request->transfer_buffer[2] = 0;
        request->transfer_buffer[3] = 0;

        // X:
        request->transfer_buffer[4] = 0;
        request->transfer_buffer[5] = 0;

        // Y:
        request->transfer_buffer[6] = 0;
        request->transfer_buffer[7] = 0;

        request->actual_length = 8;
        request->status = vdp_usb_urb_status_completed;
    }

    if (request->status == vdp_usb_urb_status_stall) {
        printf("ep1 %u stalled\n", request->id);
    }

    request->complete(request);
    request->destroy(request);
}

static void ep1_dequeue(struct vdp_usb_gadget_ep* ep, struct vdp_usb_gadget_request* request)
{
    printf("ep1 dequeue %u\n", request->id);
    request->status = vdp_usb_urb_status_unlinked;
    request->complete(request);
    request->destroy(request);
}

static vdp_usb_urb_status ep1_clear_stall(struct vdp_usb_gadget_ep* ep)
{
    printf("ep1 clear stall\n");
    return vdp_usb_urb_status_completed;
}

static void ep1_destroy(struct vdp_usb_gadget_ep* ep)
{
    printf("ep1 destroy\n");
}

static void iface_enable(struct vdp_usb_gadget_interface* interface, int value)
{
    printf("iface (%u, %u) enable = %d\n",
        interface->caps.number, interface->caps.alt_setting, value);
}

static void iface_destroy(struct vdp_usb_gadget_interface* interface)
{
    printf("iface (%u, %u) destroy\n",
        interface->caps.number, interface->caps.alt_setting);
}

static void cfg_enable(struct vdp_usb_gadget_config* config, int value)
{
    printf("cfg %u enable = %d\n", config->caps.number, value);
}

static void cfg_destroy(struct vdp_usb_gadget_config* config)
{
    printf("cfg %u destroy\n", config->caps.number);
}

static void gadget_reset(struct vdp_usb_gadget* gadget, int start)
{
    printf("gadget reset %d\n", start);
}

static void gadget_power(struct vdp_usb_gadget* gadget, int on)
{
    printf("gadget power %d\n", on);
}

static void gadget_set_address(struct vdp_usb_gadget* gadget, vdp_u32 address)
{
    printf("gadget set address %u\n", address);
}

static void gadget_destroy(struct vdp_usb_gadget* gadget)
{
    printf("gadget destroy\n");
}

static struct vdp_usb_gadget* create_gadget()
{
    struct vdp_usb_gadget_ep_caps ep0_caps =
    {
        .address = 0,
        .dir = vdp_usb_gadget_ep_inout,
        .type = vdp_usb_gadget_ep_control,
        .max_packet_size = 64,
        .interval = 0
    };
    struct vdp_usb_gadget_ep_ops ep0_ops =
    {
        .enable = ep0_enable,
        .enqueue = ep0_enqueue,
        .dequeue = ep0_dequeue,
        .clear_stall = ep0_clear_stall,
        .destroy = ep0_destroy
    };
    struct vdp_usb_gadget_ep_caps ep1_caps =
    {
        .address = 1,
        .dir = vdp_usb_gadget_ep_in,
        .type = vdp_usb_gadget_ep_int,
        .max_packet_size = 8,
        .interval = 7
    };
    struct vdp_usb_gadget_ep_ops ep1_ops =
    {
        .enable = ep1_enable,
        .enqueue = ep1_enqueue,
        .dequeue = ep1_dequeue,
        .clear_stall = ep1_clear_stall,
        .destroy = ep1_destroy
    };
    struct vdp_usb_hid_descriptor hid_descriptor =
    {
        .bLength = sizeof(struct vdp_usb_hid_descriptor),
        .bDescriptorType = VDP_USB_HID_DT_HID,
        .bcdHID = vdp_cpu_to_u16le(0x0110),
        .bCountryCode = 0,
        .bNumDescriptors = 1,
        .desc[0].bDescriptorType = VDP_USB_HID_DT_REPORT,
        .desc[0].wDescriptorLength = sizeof(test_report_descriptor)
    };
    struct vdp_usb_descriptor_header* iface_descriptors[2] = { (struct vdp_usb_descriptor_header*)&hid_descriptor, NULL };
    struct vdp_usb_gadget_ep* iface_eps[2] = { NULL, NULL };
    struct vdp_usb_gadget_interface_caps iface_caps =
    {
        .number = 0,
        .alt_setting = 0,
        .klass = VDP_USB_CLASS_HID,
        .subklass = VDP_USB_SUBCLASS_BOOT,
        .protocol = VDP_USB_PROTOCOL_MOUSE,
        .description = 0,
        .descriptors = iface_descriptors,
        .endpoints = iface_eps
    };
    struct vdp_usb_gadget_interface_ops iface_ops =
    {
        .enable = iface_enable,
        .destroy = iface_destroy
    };
    struct vdp_usb_gadget_interface* ifaces[2] = { NULL, NULL };
    struct vdp_usb_gadget_config_caps cfg_caps =
    {
        .number = 1,
        .attributes = vdp_usb_gadget_config_att_one | vdp_usb_gadget_config_att_wakeup,
        .max_power = 49,
        .description = 0,
        .interfaces = ifaces
    };
    struct vdp_usb_gadget_config_ops cfg_ops =
    {
        .enable = cfg_enable,
        .destroy = cfg_destroy
    };
    struct vdp_usb_string us_strings[] =
    {
        {1, "Logitech"},
        {2, "USB-PS/2 Optical Mouse"},
        {0, NULL},
    };
    struct vdp_usb_string_table string_tables[] =
    {
        {0x0409, us_strings},
        {0, NULL},
    };
    struct vdp_usb_gadget_config* cfgs[2] = { NULL, NULL };
    struct vdp_usb_gadget_caps caps =
    {
        .bcd_usb = 0x0200,
        .bcd_device = 0x3000,
        .klass = 0,
        .subklass = 0,
        .protocol = 0,
        .vendor_id = 0x046d,
        .product_id = 0xc051,
        .manufacturer = 1,
        .product = 2,
        .serial_number = 0,
        .string_tables = string_tables,
        .endpoint0 = 0,
        .configs = cfgs
    };
    struct vdp_usb_gadget_ops ops =
    {
        .reset = gadget_reset,
        .power = gadget_power,
        .set_address = gadget_set_address,
        .destroy = gadget_destroy
    };

    caps.endpoint0 = vdp_usb_gadget_ep_create(&ep0_caps, &ep0_ops, NULL);
    iface_eps[0] = vdp_usb_gadget_ep_create(&ep1_caps, &ep1_ops, NULL);
    ifaces[0] = vdp_usb_gadget_interface_create(&iface_caps, &iface_ops, NULL);
    cfgs[0] = vdp_usb_gadget_config_create(&cfg_caps, &cfg_ops, NULL);

    return vdp_usb_gadget_create(&caps, &ops, NULL);
}

static int run(int device_num)
{
    int ret = 1;
    vdp_usb_result vdp_res;
    struct vdp_usb_context* context = NULL;
    struct vdp_usb_device* device = NULL;
    vdp_u8 device_lower, device_upper;

    vdp_res = vdp_usb_init(stdout, vdp_log_debug, &context);

    if (vdp_res != vdp_usb_success) {
        print_error(vdp_res, "cannot initialize context");

        return 1;
    }

    vdp_res = vdp_usb_get_device_range(context, &device_lower, &device_upper);

    if (vdp_res == vdp_usb_success) {
        printf("device range is: %d - %d\n", device_lower, device_upper);
    }

    vdp_res = vdp_usb_device_open(context, (vdp_u8)device_num, &device);

    if (vdp_res != vdp_usb_success) {
        print_error(vdp_res, "cannot open device #%d", device_num);

        goto out1;
    }

    vdp_res = vdp_usb_device_attach(device);

    if (vdp_res != vdp_usb_success) {
        print_error(vdp_res, "cannot attach device #%d", device_num);

        goto out2;
    }

    struct vdp_usb_gadget* gadget = create_gadget();

    while (!done) {
        int io_res;
        fd_set read_fds;
        vdp_fd fd;
        struct vdp_usb_event event;

        vdp_res = vdp_usb_device_wait_event(device, &fd);

        if (vdp_res != vdp_usb_success) {
            print_error(vdp_res, "device #%d wait for event failed", device_num);

            goto out2;
        }

        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        io_res = select(fd + 1, &read_fds, NULL, NULL, NULL);

        if (io_res < 0) {
            printf("select error: %s\n", strerror(errno));

            goto out2;
        }

        assert(io_res == 1);

        vdp_res = vdp_usb_device_get_event(device, &event);

        if (vdp_res != vdp_usb_success) {
            print_error(vdp_res, "cannot get event from device #%d", device_num);

            goto out2;
        }

        vdp_usb_gadget_event(gadget, &event);
    };

    vdp_usb_gadget_destroy(gadget);
    gadget = NULL;

    vdp_res = vdp_usb_device_detach(device);

    if (vdp_res != vdp_usb_success) {
        print_error(vdp_res, "cannot detach device #%d", device_num);

        goto out2;
    }

    ret = 0;

out2:
    vdp_usb_gadget_destroy(gadget);
    vdp_usb_device_close(device);
out1:
    vdp_usb_cleanup(context);

    return ret;
}

static void sig_handler(int signum)
{
    done = 1;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, &sig_handler);

    if (argc < 2) {
        printf("usage: vdpusb-mouse2 <port>\n");
        return 1;
    }

    return run(atoi(argv[1]));
}
