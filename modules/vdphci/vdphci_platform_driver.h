#ifndef _VDPHCI_PLATFORM_DRIVER_H_
#define _VDPHCI_PLATFORM_DRIVER_H_

#include <linux/types.h>

struct vdphci_platform_data
{
    u8 num_ports;

    /*
     * Major number for devices. 0 for dynamic allocation.
     */
    int major;
};

extern const char vdphci_platform_driver_name[];

int vdphci_platform_driver_register(void);

void vdphci_platform_driver_unregister(void);

#endif
