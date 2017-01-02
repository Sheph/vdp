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
#include <linux/usb.h>
#include "vdphci-common.h"
#include "vdphci_platform_driver.h"
#include "vdphci_hcd.h"
#include "debug.h"
#include "print.h"

const char vdphci_platform_driver_name[] = VDPHCI_NAME "_hcd";

static int vdphci_platform_driver_probe(struct platform_device* pdev)
{
    int ret;
    struct usb_hcd* hcd;

    dprintk("probing controller \"%s\"\n", dev_name(&pdev->dev));

    ret = vdphci_hcd_add(&pdev->dev,
        dev_name(&pdev->dev),
        (const struct vdphci_platform_data*)pdev->dev.platform_data,
        &hcd);

    if (ret == 0) {
        dprintk("controller \"%s\" added\n", dev_name(&pdev->dev));
    } else {
        dprintk("error adding controller \"%s\": %d\n", dev_name(&pdev->dev), ret);
    }

    return ret;
}

static int vdphci_platform_driver_remove(struct platform_device* pdev)
{
    struct usb_hcd* hcd;

    dprintk("removing controller \"%s\"\n", dev_name(&pdev->dev));

    hcd = platform_get_drvdata(pdev);

    vdphci_hcd_remove(hcd);

    platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct platform_driver vdphci_platform_driver =
{
    .probe = vdphci_platform_driver_probe,
    .remove = vdphci_platform_driver_remove,
    .driver =
    {
        .name = (char*)vdphci_platform_driver_name,
        .owner = THIS_MODULE,
    },
};

int vdphci_platform_driver_register(void)
{
    if (usb_disabled()) {
        return -ENODEV;
    }

    return platform_driver_register(&vdphci_platform_driver);
}

void vdphci_platform_driver_unregister(void)
{
    platform_driver_unregister(&vdphci_platform_driver);
}
