#include <linux/module.h>
#include <linux/init.h>
#include "print.h"

MODULE_AUTHOR("Stanislav Vorobiov");
MODULE_LICENSE("Dual BSD/GPL");

int vdphci_major = 0;

module_param(vdphci_major, int, S_IRUGO);

int vdphci_init(void)
{
    print_info("module loaded\n");

    return 0;
}

void vdphci_cleanup(void)
{
    print_info("module unloaded\n");
}

module_init(vdphci_init);
module_exit(vdphci_cleanup);
