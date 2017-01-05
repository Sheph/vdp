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

#include <linux/slab.h>
#include "vdphci_port.h"
#include "debug.h"

#ifdef DEBUG

static const char* vdphci_port_status_bit_to_str(u32 status_bit)
{
    switch (status_bit) {
    case USB_PORT_STAT_CONNECTION: return "CONNECTION";
    case USB_PORT_STAT_ENABLE: return "ENABLE";
    case USB_PORT_STAT_SUSPEND: return "SUSPEND";
    case USB_PORT_STAT_OVERCURRENT: return "OVERCURRENT";
    case USB_PORT_STAT_RESET: return "RESET";
    case USB_PORT_STAT_L1: return "L1";
    case USB_PORT_STAT_POWER: return "POWER";
    case USB_PORT_STAT_LOW_SPEED: return "LOW_SPEED";
    case USB_PORT_STAT_HIGH_SPEED: return "HIGH_SPEED";
    case USB_PORT_STAT_TEST: return "TEST";
    case USB_PORT_STAT_INDICATOR: return "INDICATOR";
    case (USB_PORT_STAT_C_CONNECTION << 16): return "C_CONNECTION";
    case (USB_PORT_STAT_C_ENABLE << 16): return "C_ENABLE";
    case (USB_PORT_STAT_C_SUSPEND << 16): return "C_SUSPEND";
    case (USB_PORT_STAT_C_OVERCURRENT << 16): return "C_OVERCURRENT";
    case (USB_PORT_STAT_C_RESET << 16): return "C_RESET";
    case (USB_PORT_STAT_C_L1 << 16): return "C_L1";
    default: return NULL;
    }
}

#endif

#define seq_num_after(a, b) \
    (typecheck(u32, a) && typecheck(u32, b) && ((s32)(b) - (s32)(a) < 0))

#define seq_num_before(a, b) seq_num_after(b, a)

#define seq_num_after_eq(a, b)  \
    ( typecheck(u32, a) && typecheck(u32, b) && \
      ((s32)(a) - (s32)(b) >= 0) )

#define seq_num_before_eq(a, b) seq_num_after_eq(b, a)

static void vdphci_port_advance_current_urb_khevent(struct vdphci_port* port)
{
    if (port->current_urb_khevent) {
        if (list_is_last(&port->current_urb_khevent->list, &port->urb_list)) {
            port->current_urb_khevent = NULL;
        } else {
            port->current_urb_khevent =
                container_of(port->current_urb_khevent->list.next,
                    struct vdphci_khevent_urb,
                    list);
        }
    }
}

static void vdphci_port_khevent_signal_free(struct vdphci_khevent_signal* event)
{
    list_del(&event->list);

    kfree(event);
}

static void vdphci_port_khevent_urb_free(struct vdphci_khevent_urb* event)
{
    list_del(&event->list);

    kfree(event);
}

static void vdphci_port_khevent_unlink_urb_free(struct vdphci_khevent_unlink_urb* event)
{
    list_del(&event->list);

    kfree(event);
}

static void vdphci_port_khevent_signal_enqueue(struct vdphci_port* port,
    vdphci_hsignal hsignal)
{
    struct vdphci_khevent_signal* event;

    event = kzalloc(sizeof(*event), GFP_ATOMIC);

    if (event == NULL) {
        /*
         * No memory for signal event, too bad, but we can't do anything,
         * just skip the reporting
         */

        return;
    }

    event->type = vdphci_hevent_type_signal;
    INIT_LIST_HEAD(&event->list);
    event->signal = hsignal;

    list_add_tail(&event->list, &port->signal_list);

    wake_up(&port->khevent_wq);
}

/*
 * Dequeues 'event' and completes the corresponding URB if haven't been reported
 * to the user yet, otherwise adds an 'unlink urb' event.
 * In case of system resource shortage we won't be able to add new element to the
 * 'unlink urb' list, in that case we simply complete the URB.
 */
static void vdphci_port_khevent_urb_dequeue(struct vdphci_port* port,
    struct vdphci_khevent_urb* event,
    struct list_head* giveback_list)
{
    struct vdphci_khevent_unlink_urb* unlink_urb_event;

    if (port->current_urb_khevent &&
        seq_num_after_eq(event->seq_num, port->current_urb_khevent->seq_num)) {
        /*
         * The URB being dequeued is the one not reported yet to the user, so
         * we can just remove that corresponding URB from the list and complete it.
         */

        vdphci_port_khevent_urb_remove(port, event, giveback_list);

        return;
    }

    if (event->khevent_unlink_urb != NULL) {
        /*
         * There's an 'unlink urb' entry already, no need for another one.
         */

        return;
    }

    unlink_urb_event = kzalloc(sizeof(*unlink_urb_event), GFP_ATOMIC);

    if (unlink_urb_event == NULL) {
        /*
         * Resource shortage, just complete the URB.
         */

        vdphci_port_khevent_urb_remove(port, event, giveback_list);

        return;
    }

    unlink_urb_event->type = vdphci_hevent_type_unlink_urb;
    INIT_LIST_HEAD(&unlink_urb_event->list);
    unlink_urb_event->khevent_urb = event;

    list_add_tail(&unlink_urb_event->list, &port->unlink_urb_list);

    event->khevent_unlink_urb = unlink_urb_event;

    wake_up(&port->khevent_wq);
}

/*
 * Typically called when device gets detached or on port cleanup.
 */
static void vdphci_port_flush_all_khevents(struct vdphci_port* port, struct list_head* giveback_list)
{
    struct vdphci_khevent_signal *signal_event, *signal_event_tmp;
    struct vdphci_khevent_urb *urb_event, *urb_event_tmp;

    list_for_each_entry_safe(signal_event, signal_event_tmp, &port->signal_list, list) {
        vdphci_port_khevent_signal_free(signal_event);
    }

    list_for_each_entry_safe(urb_event, urb_event_tmp, &port->urb_list, list) {
        urb_event->urb->status = -ENODEV;

        vdphci_port_khevent_urb_remove(port, urb_event, giveback_list);
    }

    BUG_ON(!list_empty(&port->unlink_urb_list));
    BUG_ON(port->current_urb_khevent);
}

/*
 * Unlink all URBs, can happen during reset or power off. In this case
 * user will receive unlink events for all URBs that he currently processes, all
 * future URBs will be completed with 'canceled' status.
 * Typically, after all this the caller should add a 'signal' khevent and since
 * 'signal' khevents are reported only when there's no 'unlink urb' khevents
 * the user will see something like this: all his URBs are killed and
 * a signal has arrived followed by further events, just what we wanted !
 */
static void vdphci_port_unlink_all_urbs(struct vdphci_port* port, struct list_head* giveback_list)
{
    struct vdphci_khevent_urb *urb_event, *tmp;

    list_for_each_entry_safe(urb_event, tmp, &port->urb_list, list) {
        urb_event->urb->status = -ECONNRESET;

        vdphci_port_khevent_urb_dequeue(port, urb_event, giveback_list);
    }
}

void vdphci_port_init(u8 number, struct vdphci_port* port)
{
    memset(port, 0, sizeof(*port));

    port->number = number;

    INIT_LIST_HEAD(&port->signal_list);
    INIT_LIST_HEAD(&port->urb_list);
    INIT_LIST_HEAD(&port->unlink_urb_list);

    init_waitqueue_head(&port->khevent_wq);

    /*
     * For testing sequence number wrap.
     */
    port->seq_num = 0xFFFFFF00;
}

void vdphci_port_cleanup(struct vdphci_port* port)
{
    struct list_head giveback_list;

    INIT_LIST_HEAD(&giveback_list);

    vdphci_port_flush_all_khevents(port, &giveback_list);

    vdphci_port_giveback_urbs(&giveback_list);

    memset(port, 0, sizeof(*port));
}

#ifdef DEBUG

char* vdphci_port_status_to_str(u32 status)
{
    char* res;
    int i, len = 0;

    for (i = 0; i < sizeof(status)*8; i++) {
        if (status & (1 << i)) {
            const char* str = vdphci_port_status_bit_to_str(1 << i);

            if (str) {
                len += strlen(str) + 1;
            }
        }
    }

    res = kzalloc(len + 1, GFP_ATOMIC);

    if (res == NULL) {
        return NULL;
    }

    for (i = 0; i < sizeof(status)*8; i++) {
        if (status & (1 << i)) {
            const char* str = vdphci_port_status_bit_to_str(1 << i);

            if (str) {
                strcat(res, str);
                strcat(res, " ");
            }
        }
    }

    len = strlen(res);

    if (len > 0) {
        /*
         * Remove leading space.
         */

        res[len - 1] = '\0';
    }

    return res;
}

void vdphci_port_free_status_str(char* str)
{
    kfree(str);
}

#endif

int vdphci_port_urb_enqueue(struct vdphci_port* port, struct urb* urb, u32* seq_num)
{
    struct vdphci_khevent_urb* event;

    event = kzalloc(sizeof(*event), GFP_ATOMIC);

    if (event == NULL) {
        return -ENOMEM;
    }

    event->type = vdphci_hevent_type_urb;
    INIT_LIST_HEAD(&event->list);
    event->seq_num = port->seq_num++;
    event->urb = urb;
    urb->hcpriv = event;

    list_add_tail(&event->list, &port->urb_list);

    if (!port->current_urb_khevent) {
        /*
         * All urbs have been processed (but probably not completed)
         * set this urb as current urb.
         */
        port->current_urb_khevent = event;
    }

    wake_up(&port->khevent_wq);

    if (seq_num) {
        *seq_num = event->seq_num;
    }

    return 0;
}

void vdphci_port_urb_dequeue(struct vdphci_port* port, struct urb* urb, struct list_head* giveback_list)
{
    struct vdphci_khevent_urb* event = urb->hcpriv;

    if (!event) {
        return;
    }

    vdphci_port_khevent_urb_dequeue(port, event, giveback_list);
}

struct vdphci_khevent* vdphci_port_khevent_current(struct vdphci_port* port)
{
    if (!list_empty(&port->unlink_urb_list)) {
        return (struct vdphci_khevent*)list_first_entry(&port->unlink_urb_list,
            struct vdphci_khevent_unlink_urb,
            list);
    } else if (!list_empty(&port->signal_list)) {
        return (struct vdphci_khevent*)list_first_entry(&port->signal_list,
            struct vdphci_khevent_signal,
            list);
    } else {
        return (struct vdphci_khevent*)port->current_urb_khevent;
    }
}

void vdphci_port_khevent_proceed(struct vdphci_port* port)
{
    if (!list_empty(&port->unlink_urb_list)) {
        struct vdphci_khevent_unlink_urb* event =
            list_first_entry(&port->unlink_urb_list,
                struct vdphci_khevent_unlink_urb,
                list);

        struct vdphci_khevent_urb* urb_event = event->khevent_urb;

        BUG_ON(urb_event == NULL);

        if (urb_event != NULL) {
            BUG_ON(urb_event->khevent_unlink_urb != event);
            urb_event->khevent_unlink_urb = NULL;
        }

        vdphci_port_khevent_unlink_urb_free(event);
    } else if (!list_empty(&port->signal_list)) {
        struct vdphci_khevent_signal* event =
            list_first_entry(&port->signal_list,
                struct vdphci_khevent_signal,
                list);

        vdphci_port_khevent_signal_free(event);

        return;
    } else {
        vdphci_port_advance_current_urb_khevent(port);
    }
}

struct vdphci_khevent_urb* vdphci_port_khevent_urb_find(struct vdphci_port* port,
    u32 seq_num)
{
    struct vdphci_khevent_urb* event;

    if (port->current_urb_khevent &&
        (seq_num_after_eq(seq_num, port->current_urb_khevent->seq_num))) {
        /*
         * 'current_urb_khevent' is the first urb event that is not returned to the user,
         * but the user wants an event older than 'current_urb_khevent' or the
         * 'current_urb_khevent' itself, act as nothing was found.
         */

        return NULL;
    }

    list_for_each_entry(event, &port->urb_list, list) {
        if (event->seq_num == seq_num) {
            return event;
        } else if (seq_num_before(seq_num, event->seq_num)) {
            /*
             * All other events are later in time, break.
             */

            break;
        }
    }

    return NULL;
}

void vdphci_port_khevent_urb_remove(struct vdphci_port* port,
    struct vdphci_khevent_urb* event,
    struct list_head* giveback_list)
{
    struct vdphci_khevent_unlink_urb* unlink_urb_event = event->khevent_unlink_urb;

    if (unlink_urb_event) {
        /*
         * Free 'unlink urb' event first.
         */

        vdphci_port_khevent_unlink_urb_free(unlink_urb_event);

        event->khevent_unlink_urb = NULL;
    }

    if (port->current_urb_khevent == event) {
        /*
         * we're freeing current urb, advance it.
         */

        vdphci_port_advance_current_urb_khevent(port);
    }

    event->urb->hcpriv = NULL;

    usb_hcd_unlink_urb_from_ep(bus_to_hcd(event->urb->dev->bus), event->urb);

    list_move_tail(&event->list, giveback_list);
}

void vdphci_port_update(struct vdphci_port* port, struct list_head* giveback_list)
{
    port->enabled = 0;

    if ((port->status & USB_PORT_STAT_POWER) == 0) {
        port->status = 0;
    } else if (port->device_attached) {
        port->status |= USB_PORT_STAT_CONNECTION | USB_PORT_STAT_HIGH_SPEED;

        if ((port->old_status & USB_PORT_STAT_CONNECTION) == 0) {
            port->status |= (USB_PORT_STAT_C_CONNECTION << 16);
        }

        if ((port->status & USB_PORT_STAT_ENABLE) == 0) {
            port->status &= ~USB_PORT_STAT_SUSPEND;
        } else if ((port->status & USB_PORT_STAT_SUSPEND) == 0 &&
            !port->hcd_suspended) {
            port->enabled = 1;
        }
    } else {
        port->status &= ~(USB_PORT_STAT_CONNECTION |
            USB_PORT_STAT_ENABLE |
            USB_PORT_STAT_LOW_SPEED |
            USB_PORT_STAT_HIGH_SPEED |
            USB_PORT_STAT_SUSPEND);
        if ((port->old_status & USB_PORT_STAT_CONNECTION) != 0) {
            port->status |= (USB_PORT_STAT_C_CONNECTION << 16);
        }
    }

    if ((port->status & USB_PORT_STAT_ENABLE) == 0 || port->enabled) {
        port->resuming = 0;
    }

    if (port->device_attached) {
        if ((port->old_status & USB_PORT_STAT_RESET) == 0 &&
            (port->status & USB_PORT_STAT_RESET) != 0) {
            /*
             * Reset start
             */

            vdphci_port_unlink_all_urbs(port, giveback_list);

            vdphci_port_khevent_signal_enqueue(port, vdphci_hsignal_reset_start);
        } else if ((port->old_status & USB_PORT_STAT_RESET) != 0 &&
            (port->status & USB_PORT_STAT_RESET) == 0) {
            /*
             * Reset end
             */

            vdphci_port_khevent_signal_enqueue(port, vdphci_hsignal_reset_end);
        }

        if ((port->old_status & USB_PORT_STAT_POWER) == 0 &&
            (port->status & USB_PORT_STAT_POWER) != 0) {
            /*
             * Power on
             */

            vdphci_port_khevent_signal_enqueue(port, vdphci_hsignal_power_on);
        } else if ((port->old_status & USB_PORT_STAT_POWER) != 0 &&
            (port->status & USB_PORT_STAT_POWER) == 0) {
            /*
             * Power off
             */

            vdphci_port_unlink_all_urbs(port, giveback_list);

            vdphci_port_khevent_signal_enqueue(port, vdphci_hsignal_power_off);
        }
    }

    if ((port->old_status & USB_PORT_STAT_CONNECTION) != 0 &&
        (port->status & USB_PORT_STAT_CONNECTION) == 0) {
        /*
         * Device detached, flush all events
         */

        vdphci_port_flush_all_khevents(port, giveback_list);
    }

    port->old_status = port->status;
}

void vdphci_port_giveback_urbs(struct list_head* list)
{
    struct vdphci_khevent_urb *urb_event, *tmp;

    list_for_each_entry_safe(urb_event, tmp, list, list) {
        struct usb_hcd *hcd;

        BUG_ON(!urb_event->urb);
        BUG_ON(urb_event->khevent_unlink_urb);
        BUG_ON(urb_event->urb->hcpriv);

        hcd = bus_to_hcd(urb_event->urb->dev->bus);

        dprintk("%s: giving back %u with status %d\n",
            hcd->self.bus_name,
            urb_event->seq_num,
            (urb_event->urb->unlinked ? urb_event->urb->unlinked : urb_event->urb->status));

        usb_hcd_giveback_urb(hcd, urb_event->urb, urb_event->urb->status);

        vdphci_port_khevent_urb_free(urb_event);
    }
}
