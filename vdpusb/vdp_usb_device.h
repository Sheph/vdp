#ifndef _VDP_USB_DEVICE_H_
#define _VDP_USB_DEVICE_H_

#include "vdp/usb.h"

struct vdp_usb_context;

struct vdp_usb_device
{
    struct vdp_usb_context* context;

    vdp_u8 device_number;

    vdp_fd fd;
};

#endif
