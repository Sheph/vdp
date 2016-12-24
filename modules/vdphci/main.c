#include <linux/module.h>
#include <linux/init.h>
#include "print.h"
#include "vdphci_platform_driver.h"
#include "vdphci_controllers.h"

MODULE_AUTHOR("Stanislav Vorobiov");
MODULE_LICENSE("Dual BSD/GPL");

int vdphci_major = 0;

module_param(vdphci_major, int, S_IRUGO);

int vdphci_init(void)
{
    int ret = vdphci_platform_driver_register();

    if (ret != 0) {
        return ret;
    }

    ret = vdphci_controllers_add();

    if (ret != 0) {
        vdphci_platform_driver_unregister();

        return ret;
    }

    print_info("module loaded\n");

    return 0;
}

void vdphci_cleanup(void)
{
    vdphci_controllers_remove();

    vdphci_platform_driver_unregister();

    print_info("module unloaded\n");
}

module_init(vdphci_init);
module_exit(vdphci_cleanup);
