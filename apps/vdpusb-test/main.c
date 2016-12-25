#include "vdp/usb.h"
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
            event.data.urb->status = vdp_usb_urb_status_completed;
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
