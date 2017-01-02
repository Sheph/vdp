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

#ifndef _VDPHCI_DEVICE_H_
#define _VDPHCI_DEVICE_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>

struct vdphci_hcd;

struct vdphci_port;

struct vdphci_device
{
    /*
     * HCD that created us.
     */

    struct vdphci_hcd* parent_hcd;

    /*
     * Port this device is connected to
     */

    struct vdphci_port* port;

    /*
     * Char device representing this device.
     * @{
     */
    struct cdev cdev;
    struct mutex cdev_mutex;

    /*
     * We only allow one opened file at a time. Note that opening a device doesn't mean
     * being ready to work with it, user might wanted to just check if device is busy or not,
     * device shouldn't report itself as connected until 'attached' is true.
     */
    int opened;
    /*
     * @}
     */
};

static inline struct vdphci_device* cdev_to_vdphci_device(struct cdev* dev)
{
    return container_of((void*)dev, struct vdphci_device, cdev);
}

int vdphci_device_init(struct vdphci_hcd* parent_hcd, struct vdphci_port* port, dev_t devno, struct vdphci_device* device);

void vdphci_device_cleanup(struct vdphci_device* device);

#endif
