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

#ifndef _VDPHCI_HCD_H_
#define _VDPHCI_HCD_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include "vdphci-common.h"
#include "vdphci_platform_driver.h"
#include "vdphci_device.h"
#include "vdphci_port.h"

struct vdphci_hcd
{
    /*
     * Fields below are set on 'vdphci_hcd_add' from platform data.
     */

    u8 num_ports;
    int major;

    /*
     * Fields below are set on HCD start and cleaned up on HCD stop, they use
     * the fields above.
     */

    spinlock_t lock;

    /*
     * 'bus_suspend' was called.
     */
    int suspended;

    /*
     * Begin of range of device numbers. Range is 'num_ports' long.
     */
    dev_t devno;

    /*
     * Virtual devices that user drives.
     */
    struct vdphci_device devices[VDPHCI_MAX_PORTS];

    /*
     * Virtual ports, managed by HCD.
     */
    struct vdphci_port ports[VDPHCI_MAX_PORTS];
};

#define vdphci_hcd_lock(hcd, flags) spin_lock_irqsave(&(hcd)->lock, flags)

#define vdphci_hcd_unlock(hcd, flags) spin_unlock_irqrestore(&(hcd)->lock, flags)

static inline struct vdphci_hcd* usb_hcd_to_vdphci_hcd(struct usb_hcd* hcd)
{
    return (struct vdphci_hcd *)(hcd->hcd_priv);
}

static inline struct usb_hcd* vdphci_hcd_to_usb_hcd(struct vdphci_hcd* hcd)
{
    return container_of((void*)hcd, struct usb_hcd, hcd_priv);
}

/*
 * Creates and adds new HCD to system. Sets driver data of 'controller' to '*hcd'.
 */
int vdphci_hcd_add(struct device* controller,
    const char* bus_name,
    const struct vdphci_platform_data* platform_data,
    struct usb_hcd** hcd);

void vdphci_hcd_remove(struct usb_hcd* hcd);

/*
 * Forces HCD to re-query port statuses.
 */
void vdphci_hcd_invalidate_ports(struct vdphci_hcd* hcd);

#endif
