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
