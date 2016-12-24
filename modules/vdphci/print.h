#ifndef _VDPHCI_PRINT_H_
#define _VDPHCI_PRINT_H_

#include <linux/kernel.h>
#include "vdphci-common.h"

#define print_info(fmt, args...) printk(KERN_INFO VDPHCI_NAME ": " fmt, ## args)

#define print_error(fmt, args...) printk(KERN_ERR VDPHCI_NAME ": " fmt, ## args)

#endif
