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
