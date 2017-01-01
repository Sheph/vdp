#ifndef _VDP_USB_FILTER_H_
#define _VDP_USB_FILTER_H_

#include "vdp/usb_util.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vdp_usb_filter_ops
{
    vdp_usb_urb_status (*get_device_descriptor)(void* /*user_data*/,
        struct vdp_usb_device_descriptor* /*descriptor*/);

    vdp_usb_urb_status (*get_qualifier_descriptor)(void* /*user_data*/,
        struct vdp_usb_qualifier_descriptor* /*descriptor*/);

    vdp_usb_urb_status (*get_config_descriptor)(void* /*user_data*/,
        vdp_u8 /*index*/,
        struct vdp_usb_config_descriptor* /*descriptor*/,
        const struct vdp_usb_descriptor_header*** /*other*/);

    vdp_usb_urb_status (*set_address)(void* /*user_data*/,
        vdp_u16 /*address*/);
};

/*
 * Filters standard device requests and calls appropriate callbacks from
 * 'ops'. Updates 'urb' status, transfer_buffer and actual_length, but does
 * not complete 'urb'.
 */
int vdp_usb_filter(struct vdp_usb_urb* urb, struct vdp_usb_filter_ops* ops,
    void* user_data);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
