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

#include <linux/platform_device.h>
#include "vdphci_controllers.h"
#include "vdphci_platform_driver.h"
#include "debug.h"

extern int vdphci_major;

static struct vdphci_platform_data vdphci_controller_pdata =
{
    .num_ports = 5,
    .major = 0
};

static void vdphci_controller_release(struct device *dev)
{
    dprintk("vdphci controller %p released\n", (void*)to_platform_device(dev));
}

static struct platform_device vdphci_controller =
{
    .name = (char*)vdphci_platform_driver_name,
    .id = 0,
    .dev =
    {
        .release = vdphci_controller_release,
        .platform_data = &vdphci_controller_pdata
    }
};

int vdphci_controllers_add(void)
{
    int ret;

    vdphci_controller_pdata.major = vdphci_major;

    ret = platform_device_register(&vdphci_controller);

    if (ret == 0) {
        dprintk("vdphci controller %p added\n", (void*)&vdphci_controller);
    }

    return ret;
}

void vdphci_controllers_remove(void)
{
    platform_device_unregister(&vdphci_controller);
}
