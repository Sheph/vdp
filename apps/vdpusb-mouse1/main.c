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

#include "vdp/usb.h"
#include "vdp/usb_filter.h"
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

static struct vdp_usb_interface_descriptor test_interface_descriptor =
{
    .bLength = sizeof(struct vdp_usb_interface_descriptor),
    .bDescriptorType = VDP_USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = VDP_USB_CLASS_HID,
    .bInterfaceSubClass = VDP_USB_SUBCLASS_BOOT,
    .bInterfaceProtocol = VDP_USB_PROTOCOL_MOUSE,
    .iInterface = 0
};

static struct vdp_usb_hid_descriptor test_hid_descriptor =
{
    .bLength = sizeof(struct vdp_usb_hid_descriptor),
    .bDescriptorType = VDP_USB_HID_DT_HID,
    .bcdHID = 0x0110,
    .bCountryCode = 0,
    .bNumDescriptors = 1,
    .desc[0].bDescriptorType = VDP_USB_HID_DT_REPORT,
    .desc[0].wDescriptorLength = sizeof(test_report_descriptor)
};

static struct vdp_usb_endpoint_descriptor test_endpoint_descriptor =
{
    .bLength = VDP_USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = VDP_USB_DT_ENDPOINT,
    .bEndpointAddress = VDP_USB_ENDPOINT_IN_ADDRESS(1),
    .bmAttributes = VDP_USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize = 8,
    .bInterval = 7
};

static struct vdp_usb_descriptor_header *test_descriptors[] =
{
    (struct vdp_usb_descriptor_header *)&test_interface_descriptor,
    (struct vdp_usb_descriptor_header *)&test_hid_descriptor,
    (struct vdp_usb_descriptor_header *)&test_endpoint_descriptor,
    NULL,
};

static const struct vdp_usb_string test_us_strings[] =
{
    {1, "Logitech"},
    {2, "USB-PS/2 Optical Mouse"},
    {0, NULL},
};

static const struct vdp_usb_string_table test_string_tables[] =
{
    {0x0409, test_us_strings},
    {0, NULL},
};

static int test_process_control_urb(struct vdp_usb_urb* urb, int device_num)
{
    switch (urb->setup_packet->bRequest) {
    case VDP_USB_REQUEST_GET_DESCRIPTOR: {
        if (VDP_USB_REQUESTTYPE_TYPE(urb->setup_packet->bRequestType) != VDP_USB_REQUESTTYPE_TYPE_STANDARD) {
            break;
        }

        vdp_u8 dt_type = vdp_u16le_to_cpu(urb->setup_packet->wValue) >> 8;

        switch (dt_type) {
        case VDP_USB_HID_DT_REPORT:
            printf("get_hid_report_descriptor on device #%d\n", device_num);
            urb->status = vdp_usb_urb_status_completed;
            urb->actual_length = vdp_min(urb->transfer_length, sizeof(test_report_descriptor));
            memcpy(urb->transfer_buffer, test_report_descriptor, urb->actual_length);
            return 1;
        default:
            break;
        }
        break;
    }
    case VDP_USB_HID_REQUEST_SET_IDLE: {
        if (VDP_USB_REQUESTTYPE_TYPE(urb->setup_packet->bRequestType) != VDP_USB_REQUESTTYPE_TYPE_CLASS) {
            break;
        }
        printf("set_idle on device #%d\n", device_num);
        urb->status = vdp_usb_urb_status_completed;
        urb->actual_length = 0;
        return 1;
    }
    default:
        break;
    }
    return 0;
}

static int test_process_int_urb(struct vdp_usb_urb* urb, int device_num)
{
    if (urb->transfer_length < 8) {
        urb->status = vdp_usb_urb_status_completed;
        return 0;
    }

    urb->transfer_buffer[0] = 0;
    urb->transfer_buffer[1] = 0;
    urb->transfer_buffer[2] = 0;
    urb->transfer_buffer[3] = 0;

    // X:
    urb->transfer_buffer[4] = 0;
    urb->transfer_buffer[5] = 0;

    // Y:
    urb->transfer_buffer[6] = 0;
    urb->transfer_buffer[7] = 0;

    urb->actual_length = 8;
    urb->status = vdp_usb_urb_status_completed;
    return 0;
}

static vdp_usb_urb_status test_get_device_descriptor(void* user_data,
    struct vdp_usb_device_descriptor* descriptor)
{
    int device_num = (vdp_uintptr)user_data;

    printf("get_device_descriptor on device #%d\n", device_num);

    descriptor->bLength = sizeof(*descriptor);
    descriptor->bDescriptorType = VDP_USB_DT_DEVICE;
    descriptor->bcdUSB = vdp_cpu_to_u16le(0x0200);
    descriptor->bDeviceClass = 0;
    descriptor->bDeviceSubClass = 0;
    descriptor->bDeviceProtocol = 0;
    descriptor->bMaxPacketSize0 = 64;
    descriptor->idVendor = vdp_cpu_to_u16le(0x046d);
    descriptor->idProduct = vdp_cpu_to_u16le(0xc051);
    descriptor->bcdDevice = vdp_cpu_to_u16le(0x3000);
    descriptor->iManufacturer = 1;
    descriptor->iProduct = 2;
    descriptor->iSerialNumber = 0;
    descriptor->bNumConfigurations = 1;

    return vdp_usb_urb_status_completed;
}

static vdp_usb_urb_status test_get_qualifier_descriptor(void* user_data,
    struct vdp_usb_qualifier_descriptor* descriptor)
{
    int device_num = (vdp_uintptr)user_data;

    printf("get_qualifier_descriptor on device #%d\n", device_num);

    descriptor->bLength = sizeof(*descriptor);
    descriptor->bDescriptorType = VDP_USB_DT_QUALIFIER;
    descriptor->bcdUSB = vdp_cpu_to_u16le(0x0200);
    descriptor->bDeviceClass = 0;
    descriptor->bDeviceSubClass = 0;
    descriptor->bDeviceProtocol = 0;
    descriptor->bMaxPacketSize0 = 64;
    descriptor->bNumConfigurations = 1;
    descriptor->bRESERVED = 0;

    return vdp_usb_urb_status_completed;
}

static vdp_usb_urb_status test_get_config_descriptor(void* user_data,
    vdp_u8 index,
    struct vdp_usb_config_descriptor* descriptor,
    struct vdp_usb_descriptor_header*** other)
{
    int device_num = (vdp_uintptr)user_data;

    printf("get_config_descriptor(%d) on device #%d\n", (int)index, device_num);

    descriptor->bLength = sizeof(*descriptor);
    descriptor->bDescriptorType = VDP_USB_DT_CONFIG;
    descriptor->bNumInterfaces = 1;
    descriptor->bConfigurationValue = 1;
    descriptor->iConfiguration = 0;
    descriptor->bmAttributes = VDP_USB_CONFIG_ATT_ONE | VDP_USB_CONFIG_ATT_WAKEUP;
    descriptor->bMaxPower = 49;

    *other = test_descriptors;

    return vdp_usb_urb_status_completed;
}

static vdp_usb_urb_status test_get_string_descriptor(void* user_data,
    const struct vdp_usb_string_table** tables)
{
    int device_num = (vdp_uintptr)user_data;

    printf("get_string_descriptor on device #%d\n", device_num);

    *tables = test_string_tables;

    return vdp_usb_urb_status_completed;
}

static vdp_usb_urb_status test_set_address(void* user_data,
    vdp_u16 address)
{
    int device_num = (vdp_uintptr)user_data;

    printf("set_address(%d) on device #%d\n", (int)address, device_num);

    return vdp_usb_urb_status_completed;
}

static vdp_usb_urb_status test_set_configuration(void* user_data,
    vdp_u8 configuration)
{
    int device_num = (vdp_uintptr)user_data;

    printf("set_configuration(%d) on device #%d\n", (int)configuration, device_num);

    return vdp_usb_urb_status_completed;
}

static vdp_usb_urb_status test_get_status(void* user_data,
    vdp_u8 recipient, vdp_u8 index, vdp_u16* status)
{
    int device_num = (vdp_uintptr)user_data;

    printf("get_status on device #%d\n", device_num);

    switch (recipient) {
    case VDP_USB_REQUESTTYPE_RECIPIENT_DEVICE:
    case VDP_USB_REQUESTTYPE_RECIPIENT_INTERFACE:
    case VDP_USB_REQUESTTYPE_RECIPIENT_ENDPOINT:
        return vdp_usb_urb_status_completed;
    default:
        return vdp_usb_urb_status_stall;
    }
}

static vdp_usb_urb_status test_enable_feature(void* user_data,
    vdp_u8 recipient, vdp_u8 index, vdp_u16 feature, int enable)
{
    int device_num = (vdp_uintptr)user_data;

    printf("enable_feature on device #%d\n", device_num);

    switch (recipient) {
    case VDP_USB_REQUESTTYPE_RECIPIENT_DEVICE:
    case VDP_USB_REQUESTTYPE_RECIPIENT_INTERFACE:
        return vdp_usb_urb_status_stall;
    case VDP_USB_REQUESTTYPE_RECIPIENT_ENDPOINT:
        if ((feature != 0) || enable) {
            return vdp_usb_urb_status_stall;
        }
        return vdp_usb_urb_status_completed;
    default:
        return vdp_usb_urb_status_stall;
    }
}

static vdp_usb_urb_status test_get_interface(void* user_data,
    vdp_u8 interface, vdp_u8* alt_setting)
{
    int device_num = (vdp_uintptr)user_data;

    printf("get_interface on device #%d\n", device_num);

    *alt_setting = 0;
    return vdp_usb_urb_status_completed;
}

static vdp_usb_urb_status test_set_interface(void* user_data,
    vdp_u8 interface, vdp_u8 alt_setting)
{
    int device_num = (vdp_uintptr)user_data;

    printf("set_interface on device #%d\n", device_num);

    return vdp_usb_urb_status_completed;
}

static vdp_usb_urb_status test_set_descriptor(void* user_data,
    vdp_u16 value, vdp_u16 index, const vdp_byte* data,
    vdp_u32 len)
{
    int device_num = (vdp_uintptr)user_data;

    printf("set_descriptor on device #%d\n", device_num);

    return vdp_usb_urb_status_stall;
}

static struct vdp_usb_filter_ops test_filter_ops =
{
    .get_device_descriptor = test_get_device_descriptor,
    .get_qualifier_descriptor = test_get_qualifier_descriptor,
    .get_config_descriptor = test_get_config_descriptor,
    .get_string_descriptor = test_get_string_descriptor,
    .set_address = test_set_address,
    .set_configuration = test_set_configuration,
    .get_status = test_get_status,
    .enable_feature = test_enable_feature,
    .get_interface = test_get_interface,
    .set_interface = test_set_interface,
    .set_descriptor = test_set_descriptor
};

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

    vdp_res = vdp_usb_device_attach(device, vdp_usb_speed_high);

    if (vdp_res != vdp_usb_success) {
        print_error(vdp_res, "cannot attach device #%d", device_num);

        goto out2;
    }

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

        switch (event.type) {
        case vdp_usb_event_none: {
            printf("event on device #%d: none\n", device_num);
            break;
        }
        case vdp_usb_event_signal: {
            printf("event on device #%d: signal %s\n", device_num, vdp_usb_signal_type_to_str(event.data.signal.type));
            break;
        }
        case vdp_usb_event_urb: {
            char urb_str[512];

            vdp_usb_urb_to_str(event.data.urb, urb_str, sizeof(urb_str));

            printf("event on device #%d: urb %s\n", device_num, &urb_str[0]);
            if (!vdp_usb_filter(event.data.urb, &test_filter_ops, (void*)(vdp_uintptr)device_num)) {
                if (event.data.urb->type == vdp_usb_urb_control) {
                    if (!test_process_control_urb(event.data.urb, device_num)) {
                        event.data.urb->status = vdp_usb_urb_status_completed;
                    }
                } else if (event.data.urb->type == vdp_usb_urb_int) {
                    usleep(event.data.urb->interval);
                    if (!test_process_int_urb(event.data.urb, device_num)) {
                        event.data.urb->status = vdp_usb_urb_status_completed;
                    }
                } else {
                    event.data.urb->status = vdp_usb_urb_status_stall;
                }
            }
            vdp_usb_complete_urb(event.data.urb);
            vdp_usb_free_urb(event.data.urb);
            break;
        }
        case vdp_usb_event_unlink_urb: {
            printf("event on device #%d: unlink urb\n", device_num);
            break;
        }
        default:
            printf("event on device #%d: unknown\n", device_num);
            break;
        }
    };

    vdp_res = vdp_usb_device_detach(device);

    if (vdp_res != vdp_usb_success) {
        print_error(vdp_res, "cannot detach device #%d", device_num);

        goto out2;
    }

    ret = 0;

out2:
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
        printf("usage: vdpusb-mouse1 <port>\n");
        return 1;
    }

    return run(atoi(argv[1]));
}
