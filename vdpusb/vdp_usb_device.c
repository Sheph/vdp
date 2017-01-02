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

#include "vdp_usb_device.h"
#include "vdp_usb_context.h"
#include "vdp_usb_urbi.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

static vdp_usb_result vdp_usb_device_translate_io_error(int error)
{
    switch (error) {
    case EINVAL: {
        return vdp_usb_protocol_error;
    }
    default:
        return vdp_usb_unknown;
    }
}

static void vdp_usb_device_complete_unprocessed_urb(struct vdp_usb_device* device, vdp_u32 seq_num)
{
    char buff[sizeof(struct vdphci_devent_header) + sizeof(struct vdphci_devent_urb)];
    struct vdphci_devent_header header;
    struct vdphci_devent_urb urb;

    assert(device);
    if (!device) {
        return;
    }

    memset(&header, 0, sizeof(header));
    memset(&urb, 0, sizeof(urb));

    header.type = vdphci_devent_type_urb;
    urb.seq_num = seq_num;
    urb.status = vdphci_urb_status_unprocessed;

    memcpy(&buff[0], &header, sizeof(header));
    memcpy(&buff[0] + sizeof(header), &urb, vdp_offsetof(struct vdphci_devent_urb, data.buff));

    if (write(device->fd, &buff[0], sizeof(header) + vdp_offsetof(struct vdphci_devent_urb, data.buff)) == -1) {
        int error = errno;

        VDP_USB_LOG_ERROR(device->context, "device %d: cannot complete urb: %s (%d)",
            device->device_number, strerror(error), error);
    }
}

vdp_usb_result vdp_usb_device_open(struct vdp_usb_context* context,
    vdp_u8 device_number,
    struct vdp_usb_device** device)
{
    char device_path[255];
    int error;

    assert(context && device);
    if (!context || !device) {
        return vdp_usb_misuse;
    }

    int written = snprintf(&device_path[0],
        sizeof(device_path),
        "/dev/" VDPHCI_DEVICE_PREFIX "%d",
        device_number);

    if ((written >= sizeof(device_path)) || (written <= 0)) {
        VDP_USB_LOG_CRITICAL(context, "device file path is too long");

        return vdp_usb_unknown;
    }

    *device = malloc(sizeof(**device));

    if (*device == NULL) {
        return vdp_usb_nomem;
    }

    memset(*device, 0, sizeof(**device));

    (*device)->context = context;

    (*device)->device_number = device_number;

    (*device)->fd = open(device_path, O_RDWR);

    error = errno;

    if ((*device)->fd == -1) {
        free(*device);
        *device = NULL;

        if (error == EBUSY) {
            VDP_USB_LOG_ERROR(context, "device %d is busy", device_number);

            return vdp_usb_busy;
        } else if ((error == ENOENT) || (error == EISDIR) || (error == ENXIO) || (error == ENODEV)) {
            VDP_USB_LOG_ERROR(context, "device %d does not exist", device_number);

            return vdp_usb_not_found;
        } else {
            VDP_USB_LOG_ERROR(context, "device %d cannot be opened: %s (%d)", device_number, strerror(error), error);

            return vdp_usb_unknown;
        }
    }

    VDP_USB_LOG_DEBUG(context, "device %d opened", device_number);

    return vdp_usb_success;
}

void vdp_usb_device_close(struct vdp_usb_device* device)
{
    assert(device);
    if (!device) {
        return;
    }

    close(device->fd);
    device->fd = -1;

    VDP_USB_LOG_DEBUG(device->context, "device %d closed", device->device_number);

    free(device);
}

vdp_usb_result vdp_usb_device_attach(struct vdp_usb_device* device)
{
    char buff[sizeof(struct vdphci_devent_header) + sizeof(struct vdphci_devent_signal)];
    struct vdphci_devent_header header;
    struct vdphci_devent_signal event;

    assert(device);
    if (!device) {
        return vdp_usb_misuse;
    }

    memset(&header, 0, sizeof(header));
    memset(&event, 0, sizeof(event));

    header.type = vdphci_devent_type_signal;
    event.signal = vdphci_dsignal_attached;

    memcpy(&buff[0], &header, sizeof(header));
    memcpy(&buff[0] + sizeof(header), &event, sizeof(event));

    if (write(device->fd, &buff[0], sizeof(buff)) == -1) {
        int error = errno;

        VDP_USB_LOG_ERROR(device->context, "device %d: cannot attach device: %s (%d)",
            device->device_number, strerror(error), error);

        return vdp_usb_device_translate_io_error(error);
    } else {
        VDP_USB_LOG_DEBUG(device->context, "device %d attached", device->device_number);

        return vdp_usb_success;
    }
}

vdp_usb_result vdp_usb_device_detach(struct vdp_usb_device* device)
{
    char buff[sizeof(struct vdphci_devent_header) + sizeof(struct vdphci_devent_signal)];
    struct vdphci_devent_header header;
    struct vdphci_devent_signal event;

    assert(device);
    if (!device) {
        return vdp_usb_misuse;
    }

    memset(&header, 0, sizeof(header));
    memset(&event, 0, sizeof(event));

    header.type = vdphci_devent_type_signal;
    event.signal = vdphci_dsignal_detached;

    memcpy(&buff[0], &header, sizeof(header));
    memcpy(&buff[0] + sizeof(header), &event, sizeof(event));

    if (write(device->fd, &buff[0], sizeof(buff)) == -1) {
        int error = errno;

        VDP_USB_LOG_ERROR(device->context, "device %d: cannot detach device: %s (%d)",
            device->device_number, strerror(error), error);

        return vdp_usb_device_translate_io_error(error);
    } else {
        VDP_USB_LOG_DEBUG(device->context, "device %d detached", device->device_number);

        return vdp_usb_success;
    }
}

vdp_usb_result vdp_usb_device_wait_event(struct vdp_usb_device* device, vdp_fd* fd)
{
    assert(device);
    if (!device) {
        return vdp_usb_misuse;
    }

    if (fd) {
        *fd = device->fd;
    }

    return vdp_usb_success;
}

vdp_usb_result vdp_usb_device_get_event(struct vdp_usb_device* device, struct vdp_usb_event* event)
{
    ssize_t num_read = 0;
    vdp_usb_result res = vdp_usb_unknown;
    char* buff = NULL;
    size_t buff_size = 0;

    assert(device);
    assert(event);

    if (!device) {
        return vdp_usb_misuse;
    }

    if (!event) {
        return vdp_usb_misuse;
    }

    memset(event, 0, sizeof(*event));

    /*
     * We assume that most events (especially URBs) will fit into 4K, if not
     * we'll increase the buffer
     */

    assert(sizeof(struct vdphci_hevent_header) <= 4096);

    buff_size = 4096;

    buff = malloc(buff_size);

    if (!buff) {
        return vdp_usb_nomem;
    }

    while (1) {
        struct vdphci_hevent_header* header = NULL;
        struct vdphci_hevent_signal* signal_event = NULL;
        struct vdphci_hevent_unlink_urb* unlink_urb_event = NULL;
        struct vdp_usb_urbi* urbi = NULL;

        num_read = read(device->fd, buff, buff_size);

        if (num_read == -1) {
            int error = errno;

            VDP_USB_LOG_ERROR(device->context, "device %d: error reading event: %s (%d)",
                device->device_number, strerror(error), error);

            res = vdp_usb_device_translate_io_error(error);

            goto out_free_buff;
        }

        if (num_read == 0) {
            /*
             * No event pending
             */

            event->type = vdp_usb_event_none;

            res = vdp_usb_success;

            goto out_free_buff;
        }

        if (num_read < sizeof(*header)) {
            VDP_USB_LOG_ERROR(device->context, "device %d: bad event header",
                device->device_number);

            res = vdp_usb_protocol_error;

            goto out_free_buff;
        }

        header = (struct vdphci_hevent_header*)buff;

        switch (header->type) {
        case vdphci_hevent_type_signal: {
            if (header->length != sizeof(*signal_event)) {
                VDP_USB_LOG_ERROR(device->context, "device %d: signal event header reports bad length - %d",
                    device->device_number, header->length);

                res = vdp_usb_protocol_error;

                goto out_free_buff;
            }

            if (buff_size < (sizeof(*header) + sizeof(*signal_event))) {
                break;
            }

            if (num_read != (sizeof(*header) + sizeof(*signal_event))) {
                VDP_USB_LOG_ERROR(device->context, "device %d: bad signal event length - %d",
                    device->device_number, num_read);

                res = vdp_usb_protocol_error;

                goto out_free_buff;
            }

            signal_event = (struct vdphci_hevent_signal*)(buff + sizeof(*header));

            switch (signal_event->signal) {
            case vdphci_hsignal_reset_start: {
                event->data.signal.type = vdp_usb_signal_reset_start;
                break;
            }
            case vdphci_hsignal_reset_end: {
                event->data.signal.type = vdp_usb_signal_reset_end;
                break;
            }
            case vdphci_hsignal_power_on: {
                event->data.signal.type = vdp_usb_signal_power_on;
                break;
            }
            case vdphci_hsignal_power_off: {
                event->data.signal.type = vdp_usb_signal_power_off;
                break;
            }
            default:
                VDP_USB_LOG_ERROR(device->context, "device %d: bad signal type - %d",
                    device->device_number, signal_event->signal);

                res = vdp_usb_protocol_error;

                goto out_free_buff;
            }

            event->type = vdp_usb_event_signal;

            res = vdp_usb_success;

            goto out_free_buff;
        }
        case vdphci_hevent_type_urb: {
            if (header->length < vdp_offsetof(struct vdphci_hevent_urb, data.buff)) {
                VDP_USB_LOG_ERROR(device->context, "device %d: urb event header reports bad length - %d",
                    device->device_number, header->length);

                res = vdp_usb_protocol_error;

                goto out_free_buff;
            }

            if (buff_size < (sizeof(*header) + header->length)) {
                break;
            }

            if (num_read != (sizeof(*header) + header->length)) {
                VDP_USB_LOG_ERROR(device->context, "device %d: bad urb event length - %d",
                    device->device_number, num_read);

                res = vdp_usb_protocol_error;

                goto out_free_buff;
            }

            res = vdp_usb_urbi_create(device, buff, num_read, &urbi);

            if (res != vdp_usb_success) {
                /*
                 * Since we did read the urb and we're not giving it to the user
                 * because of errors we must complete it here.
                 */

                struct vdphci_hevent_urb* urb = (struct vdphci_hevent_urb*)(buff + sizeof(*header));

                vdp_usb_device_complete_unprocessed_urb(device, urb->seq_num);

                goto out_free_buff;
            }

            event->type = vdp_usb_event_urb;
            event->data.urb = &urbi->urb;

            goto out_keep_buff;
        }
        case vdphci_hevent_type_unlink_urb: {
            if (header->length != sizeof(*unlink_urb_event)) {
                VDP_USB_LOG_ERROR(device->context, "device %d: unlink urb event header reports bad length - %d",
                    device->device_number, header->length);

                res = vdp_usb_protocol_error;

                goto out_free_buff;
            }

            if (buff_size < (sizeof(*header) + sizeof(*unlink_urb_event))) {
                break;
            }

            if (num_read != (sizeof(*header) + sizeof(*unlink_urb_event))) {
                VDP_USB_LOG_ERROR(device->context, "device %d: bad unlink urb event length - %d",
                    device->device_number, num_read);

                res = vdp_usb_protocol_error;

                goto out_free_buff;
            }

            unlink_urb_event = (struct vdphci_hevent_unlink_urb*)(buff + sizeof(*header));

            event->type = vdp_usb_event_unlink_urb;
            event->data.unlink_urb.id = unlink_urb_event->seq_num;

            res = vdp_usb_success;

            goto out_free_buff;
        }
        default:
            VDP_USB_LOG_ERROR(device->context, "device %d: bad event type - %d",
                device->device_number, header->type);

            res = vdp_usb_protocol_error;

            goto out_free_buff;
        }

        buff_size = sizeof(*header) + header->length;

        free(buff);

        buff = malloc(buff_size);

        if (!buff) {
            res = vdp_usb_nomem;

            goto out_free_buff;
        }
    }

out_free_buff:
    if (buff) {
        free(buff);
    }

out_keep_buff:
    return res;
}

vdp_usb_result vdp_usb_complete_urb(struct vdp_usb_urb* urb)
{
    struct vdp_usb_urbi* urbi = NULL;
    vdp_usb_result res = vdp_usb_unknown;
    vdp_u32 event_size = 0;

    assert(urb);

    if (!urb) {
        return vdp_usb_misuse;
    }

    urbi = vdp_containerof(urb, struct vdp_usb_urbi, urb);

    res = vdp_usb_urbi_update(urbi);

    if (res != vdp_usb_success) {
        return res;
    }

    event_size = vdp_usb_urbi_get_effective_size(urbi) -
        vdp_offsetof(struct vdp_usb_urbi, devent_header);

    if (write(urbi->device->fd, &urbi->devent_header, event_size) == -1) {
        int error = errno;

        VDP_USB_LOG_ERROR(urbi->device->context, "device %d: cannot complete urb: %s (%d)",
            urbi->device->device_number, strerror(error), error);

        return vdp_usb_device_translate_io_error(error);
    } else {
        return vdp_usb_success;
    }
}

void vdp_usb_free_urb(struct vdp_usb_urb* urb)
{
    struct vdp_usb_urbi* urbi;

    assert(urb);
    if (!urb) {
        return;
    }

    urbi = vdp_containerof(urb, struct vdp_usb_urbi, urb);

    vdp_usb_urbi_destroy(urbi);
}
