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

#ifndef _VDPHCI_PORT_H_
#define _VDPHCI_PORT_H_

#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include "vdphci-common.h"

/*
 * Kernel part of HEvent. Use 'type' to cast it to what you need.
 */
struct vdphci_khevent
{
    vdphci_hevent_type type;

    struct list_head list;
};

struct vdphci_khevent_signal
{
    vdphci_hevent_type type;

    struct list_head list;

    vdphci_hsignal signal;
};

struct vdphci_khevent_unlink_urb;

/*
 * Used to link pending URB. Note that a pointer to this structure
 * is always stored in 'urb->hcpriv' !
 */
struct vdphci_khevent_urb
{
    vdphci_hevent_type type;

    struct list_head list;

    u32 seq_num;

    struct urb* urb;

    /*
     * Points to a corresponding entry in the 'unlink urb' list,
     * when completing and unlinking this urb we should also remove corresponding
     * 'unlink urb' event atomically.
     * If 'unlink urb' event has been read before the completion of this urb
     * then we should set this field to NULL.
     */
    struct vdphci_khevent_unlink_urb* khevent_unlink_urb;
};

struct vdphci_khevent_unlink_urb
{
    vdphci_hevent_type type;

    struct list_head list;

    /*
     * Pointer to urb khevent to unlink.
     */
    struct vdphci_khevent_urb* khevent_urb;
};

struct vdphci_port
{
    /*
     * For logging purposes.
     */
    u8 number;

    /*
     * True when parent HCD is suspended.
     */
    int hcd_suspended;

    /*
     * USB status and old status of this port.
     */
    u32 status;
    u32 old_status;

    /*
     * Port is USB enabled.
     */
    int enabled;

    /*
     * When we're resuming the port we must signal for 20ms and only after that we can state
     * that the port was actually resumed.
     * If we're resetting, 50ms signal must apply.
     * @{
     */
    int resuming;
    unsigned long re_timeout;
    /*
     * @}
     */

    /*
     * Device is attached to this port.
     */
    int device_attached;

    /*
     * HEvent related.
     * @{
     */

    /*
     * List of signals.
     */
    struct list_head signal_list;

    /*
     * List of pending URBs.
     */
    struct list_head urb_list;

    /*
     * List of URB unlink events.
     */
    struct list_head unlink_urb_list;

    /*
     * When both 'signal_list' and 'unlink_urb_list' are empty we'll return
     * this urb khevent if it's not NULL. If it's NULL we assume that 'urb_list'
     * has been processed totally.
     */
    struct vdphci_khevent_urb* current_urb_khevent;

    /*
     * Wait queue that is waken up when at least one event is available.
     */
    wait_queue_head_t khevent_wq;

    /*
     * Auto-incremented urb sequence number.
     */
    u32 seq_num;

    /*
     * @}
     */
};

void vdphci_port_init(u8 number, struct vdphci_port* port);

void vdphci_port_cleanup(struct vdphci_port* port);

/*
 * Return khevent queue that can be waited on for incoming khevents.
 */
static inline wait_queue_head_t* vdphci_port_get_khevent_wq(struct vdphci_port* port)
{
    return &port->khevent_wq;
}

#ifdef DEBUG

/*
 * Convert port status to string, be sure to free the string after use.
 * @{
 */
char* vdphci_port_status_to_str(u32 status);
void vdphci_port_free_status_str(char* str);
/*
 * @}
 */

#endif

/*
 * All of the functions below must be called WITH HCD lock being held
 * @{
 */

static inline u32 vdphci_port_get_status(struct vdphci_port* port)
{
    return port->status;
}

static inline u32 vdphci_port_check_status_bits(struct vdphci_port* port, u32 bits)
{
    return (port->status & bits);
}

static inline void vdphci_port_set_status_bits(struct vdphci_port* port, u32 bits)
{
    port->status |= bits;
}

static inline void vdphci_port_reset_status_bits(struct vdphci_port* port, u32 bits)
{
    port->status &= ~bits;
}

static inline void vdphci_port_set_resuming(struct vdphci_port* port, int resuming)
{
    port->resuming = !!resuming;
}

static inline int vdphci_port_is_resuming(struct vdphci_port* port)
{
    return port->resuming;
}

static inline void vdphci_port_set_re_timeout(struct vdphci_port* port, int timeout_ms)
{
    port->re_timeout = jiffies + msecs_to_jiffies(timeout_ms);
}

static inline unsigned long vdphci_port_get_re_timeout(struct vdphci_port* port)
{
    return port->re_timeout;
}

static inline void vdphci_port_set_device_attached(struct vdphci_port* port, int attached)
{
    port->device_attached = !!attached;
}

static inline int vdphci_port_is_device_attached(struct vdphci_port* port)
{
    return port->device_attached;
}

static inline void vdphci_port_set_hcd_suspended(struct vdphci_port* port, int suspended)
{
    port->hcd_suspended = !!suspended;
}

static inline int vdphci_port_is_hcd_suspended(struct vdphci_port* port)
{
    return port->hcd_suspended;
}

static inline int vdphci_port_is_enabled(struct vdphci_port* port)
{
    return port->enabled;
}

/*
 * Add an urb khevent and signal waiters. 'seq_num' (can be NULL) receives assigned
 * sequence number just for debugging purposes.
 */
int vdphci_port_urb_enqueue(struct vdphci_port* port, struct urb* urb, u32* seq_num);

/*
 * Add an urb unlink khevent and signal waiters.
 * 'giveback_list' is a list head that'll contain urbs to giveback. After this call
 * HCD lock must be released and 'vdphci_port_giveback_urbs' must be called on that list.
 * Note that this function can be called multiple times with the same 'giveback_list'
 * but with different urbs, it'll add urbs to the end of the list so you can giveback
 * all urbs at once later.
 */
void vdphci_port_urb_dequeue(struct vdphci_port* port, struct urb* urb, struct list_head* giveback_list);

/*
 * Return currently pending khevent if any, otherwise NULL.
 * Pointer returned can be considered valid only until next call to some
 * 'vdphci_port_xxx' function.
 */
struct vdphci_khevent* vdphci_port_khevent_current(struct vdphci_port* port);

/*
 * Proceed to the next khevent, if any.
 */
void vdphci_port_khevent_proceed(struct vdphci_port* port);

/*
 * Find urb khevent by sequence number. Returns NULL if not found.
 * Pointer returned can be considered valid only until next call to some
 * 'vdphci_port_xxx' function.
 */
struct vdphci_khevent_urb* vdphci_port_khevent_urb_find(struct vdphci_port* port,
    u32 seq_num);

/*
 * Remove urb khevent referenced by 'event' from queue.
 * 'event' should be obtained from the call to 'vdphci_port_khevent_urb_find'.
 * 'giveback_list' is a list head that'll contain urbs to giveback. After this call
 * HCD lock must be released and 'vdphci_port_giveback_urbs' must be called on that list.
 * Note that this function can be called multiple times with the same 'giveback_list'
 * but with different urbs, it'll add urbs to the end of the list so you can giveback
 * all urbs at once later.
 */
void vdphci_port_khevent_urb_remove(struct vdphci_port* port,
    struct vdphci_khevent_urb* event,
    struct list_head* giveback_list);

/*
 * Update port status.
 * 'giveback_list' is a list head that'll contain urbs to giveback. After this call
 * HCD lock must be released and 'vdphci_port_giveback_urbs' must be called on that list.
 * Note that this function can be called multiple times with the same 'giveback_list'
 * but with different ports, it'll add urbs to the end of the list so you can giveback
 * all urbs at once later.
 */
void vdphci_port_update(struct vdphci_port* port, struct list_head* giveback_list);

/*
 * @}
 */

/*
 * All of the functions below must be called WITHOUT HCD lock being held
 * @{
 */

void vdphci_port_giveback_urbs(struct list_head* list);

/*
 * @}
 */

#endif
