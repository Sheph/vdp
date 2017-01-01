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

static struct vdp_usb_endpoint_descriptor test_endpoint_descriptor =
{
    .bLength = sizeof(struct vdp_usb_endpoint_descriptor),
    .bDescriptorType = VDP_USB_DT_ENDPOINT,
    .bEndpointAddress = VDP_USB_ENDPOINT_IN_ADDRESS(1),
    .bmAttributes = VDP_USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize = 8,
    .bInterval = 10
};

static const struct vdp_usb_descriptor_header *test_descriptors[] =
{
    (const struct vdp_usb_descriptor_header *)&test_interface_descriptor,
    (const struct vdp_usb_descriptor_header *)&test_endpoint_descriptor,
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
    const struct vdp_usb_descriptor_header*** other)
{
    int device_num = (vdp_uintptr)user_data;

    printf("get_config_descriptor on device #%d\n", device_num);

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

static struct vdp_usb_filter_ops test_filter_ops =
{
    .get_device_descriptor = test_get_device_descriptor,
    .get_qualifier_descriptor = test_get_qualifier_descriptor,
    .get_config_descriptor = test_get_config_descriptor,
    .get_string_descriptor = test_get_string_descriptor,
    .set_address = test_set_address
};

static int cmd_dump_events(char* argv[])
{
    int ret = 1;
    vdp_usb_result vdp_res;
    struct vdp_usb_context* context = NULL;
    struct vdp_usb_device* device = NULL;
    int device_num = 0;
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

    device_num = atoi(argv[0]);

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

    while (1) {
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
                event.data.urb->status = vdp_usb_urb_status_completed;
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

typedef int (*command_handler)(char* argv[]);

const struct
{
    const char* cmd_name;
    const char* cmd_args;
    const char* cmd_description;
    unsigned int cmd_num_args;
    command_handler cmd_handler;
} commands[] =
{
    {"dump_events", " <device number>", "Dumps events from device #<device number>", 1, cmd_dump_events}
};

int main(int argc, char* argv[])
{
    int i, ret, handler_found = 0;

    ret = 1;

    if (argc > 1) {
        for (i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i) {
            if ((strcasecmp(argv[1], commands[i].cmd_name) == 0) && ((argc - 2) == commands[i].cmd_num_args)) {
                handler_found = 1;

                ret = commands[i].cmd_handler(argv + 2);

                break;
            }
        }
    }

    if (!handler_found) {
        size_t len;
        size_t max_len = 0;
        size_t j = 0;

        printf("usage: vdpusb-app <cmd> [args]\n");

        for (i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i) {
            len = strlen(commands[i].cmd_name) + strlen(commands[i].cmd_args);

            if (len > max_len) {
                max_len = len;
            }
        }

        printf("<cmd>:\n");

        for (i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i) {
            for (j = 0; j < strlen("usage: "); ++j) {
                printf(" ");
            }

            printf("%s%s", commands[i].cmd_name, commands[i].cmd_args);

            len = strlen(commands[i].cmd_name) + strlen(commands[i].cmd_args);

            for (j = 0; j < (max_len - len); ++j) {
                printf(" ");
            }

            printf(" - %s\n", commands[i].cmd_description);
        }
    }

    return ret;
}
