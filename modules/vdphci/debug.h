#ifndef _VDPHCI_DEBUG_H_
#define _VDPHCI_DEBUG_H_

#include <linux/compiler.h>
#include <linux/kernel.h>
#include "vdphci-common.h"

#ifdef DEBUG
#   define dprintk(fmt, args...) printk(KERN_DEBUG VDPHCI_NAME "::%s: " fmt, __FUNCTION__, ## args)
#else
#   define dprintk(fmt, args...)
#endif

#endif
