#ifndef _VDP_USB_GADGET_H_
#define _VDP_USB_GADGET_H_

#include "vdp/usb_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * USB Gadget Endpoint.
 * @{
 */

struct vdp_usb_gadget_ep;

struct vdp_usb_gadget_ep_ops
{
    void (*enqueue)(struct vdp_usb_gadget_ep* /*ep*/, struct vdp_usb_urb* /*urb*/);
    void (*dequeue)(struct vdp_usb_gadget_ep* /*ep*/, vdp_u32 /*urb_id*/);

    vdp_usb_urb_status (*clear_stall)(struct vdp_usb_gadget_ep* /*ep*/);

    void (*destroy)(struct vdp_usb_gadget_ep* /*ep*/);
};

typedef enum
{
    vdp_usb_gadget_ep_in = 0,
    vdp_usb_gadget_ep_out = 1,
    vdp_usb_gadget_ep_inout = 2
} vdp_usb_gadget_ep_dir;

typedef enum
{
    vdp_usb_gadget_ep_control = VDP_USB_ENDPOINT_XFER_CONTROL,
    vdp_usb_gadget_ep_iso = VDP_USB_ENDPOINT_XFER_ISO,
    vdp_usb_gadget_ep_bulk = VDP_USB_ENDPOINT_XFER_BULK,
    vdp_usb_gadget_ep_int = VDP_USB_ENDPOINT_XFER_INT
} vdp_usb_gadget_ep_type;

struct vdp_usb_gadget_ep_caps
{
    vdp_u32 address;
    vdp_usb_gadget_ep_dir dir;
    vdp_usb_gadget_ep_type type;
    vdp_u32 max_packet_size;
    vdp_u32 interval;
};

struct vdp_usb_gadget_ep
{
    struct vdp_usb_gadget_ep_caps caps;
    struct vdp_usb_gadget_ep_ops* ops;
    void* priv;
};

struct vdp_usb_gadget_ep* vdp_usb_gadget_ep_create(const struct vdp_usb_gadget_ep_caps* caps,
    struct vdp_usb_gadget_ep_ops* ops, void* priv);

int vdp_usb_gadget_ep_stalled(struct vdp_usb_gadget_ep* ep);

void vdp_usb_gadget_ep_destroy(struct vdp_usb_gadget_ep* ep);

/*
 * @}
 */

/*
 * USB Gadget Interface.
 * @{
 */

struct vdp_usb_gadget_interface;

struct vdp_usb_gadget_interface_ops
{
    void (*enable)(struct vdp_usb_gadget_interface* /*interface*/, int /*value*/);

    void (*destroy)(struct vdp_usb_gadget_interface* /*interface*/);
};

struct vdp_usb_gadget_interface_caps
{
    vdp_u32 number;
    vdp_u32 alt_setting;
    vdp_u32 klass;
    vdp_u32 subklass;
    vdp_u32 protocol;
    int description;
    const struct vdp_usb_descriptor_header** descriptors;
    struct vdp_usb_gadget_ep** endpoints;
};

struct vdp_usb_gadget_interface
{
    struct vdp_usb_gadget_interface_caps caps;
    struct vdp_usb_gadget_interface_ops* ops;
    void* priv;
};

struct vdp_usb_gadget_interface* vdp_usb_gadget_interface_create(const struct vdp_usb_gadget_interface_caps* caps,
    struct vdp_usb_gadget_interface_ops* ops, void* priv);

int vdp_usb_gadget_interface_active(struct vdp_usb_gadget_interface* interface);

void vdp_usb_gadget_interface_destroy(struct vdp_usb_gadget_interface* interface);

/*
 * @}
 */

/*
 * USB Gadget Configuration.
 * @{
 */

struct vdp_usb_gadget_config;

struct vdp_usb_gadget_config_ops
{
    void (*enable)(struct vdp_usb_gadget_config* /*config*/, int /*value*/);

    void (*destroy)(struct vdp_usb_gadget_config* /*config*/);
};

typedef enum
{
    vdp_usb_gadget_config_att_one       = VDP_USB_CONFIG_ATT_ONE,
    vdp_usb_gadget_config_att_selfpower = VDP_USB_CONFIG_ATT_SELFPOWER,
    vdp_usb_gadget_config_att_wakeup    = VDP_USB_CONFIG_ATT_WAKEUP,
    vdp_usb_gadget_config_att_battery   = VDP_USB_CONFIG_ATT_BATTERY
} vdp_usb_gadget_config_att;

struct vdp_usb_gadget_config_caps
{
    vdp_u32 number;
    vdp_u32 attributes;
    vdp_u32 max_power;
    int description;
    struct vdp_usb_gadget_interface** interfaces;
};

struct vdp_usb_gadget_config
{
    struct vdp_usb_gadget_config_caps caps;
    struct vdp_usb_gadget_config_ops* ops;
    void* priv;
};

struct vdp_usb_gadget_config* vdp_usb_gadget_config_create(const struct vdp_usb_gadget_config_caps* caps,
    struct vdp_usb_gadget_config_ops* ops, void* priv);

int vdp_usb_gadget_config_active(struct vdp_usb_gadget_config* config);

void vdp_usb_gadget_config_destroy(struct vdp_usb_gadget_config* config);

/*
 * @}
 */

/*
 * USB Gadget.
 * @{
 */

struct vdp_usb_gadget;

struct vdp_usb_gadget_ops
{
    void (*reset)(struct vdp_usb_gadget* /*gadget*/, int /*start*/);

    void (*power)(struct vdp_usb_gadget* /*gadget*/, int /*on*/);

    void (*destroy)(struct vdp_usb_gadget* /*gadget*/);
};

struct vdp_usb_gadget_caps
{
    vdp_u32 bcd_usb;
    vdp_u32 bcd_device;
    vdp_u32 klass;
    vdp_u32 subklass;
    vdp_u32 protocol;
    vdp_u32 vendor_id;
    vdp_u32 product_id;
    int manufacturer;
    int product;
    const struct vdp_usb_string_table* string_tables;
    struct vdp_usb_gadget_ep* endpoint0;
    struct vdp_usb_gadget_config** configs;
};

struct vdp_usb_gadget
{
    struct vdp_usb_gadget_caps caps;
    struct vdp_usb_gadget_ops* ops;
    void* priv;
};

struct vdp_usb_gadget* vdp_usb_gadget_create(const struct vdp_usb_gadget_caps* caps,
    struct vdp_usb_gadget_ops* ops, void* priv);

void vdp_usb_gadget_event(struct vdp_usb_gadget* gadget, struct vdp_usb_event* event);

void vdp_usb_gadget_destroy(struct vdp_usb_gadget* gadget);

/*
 * @}
 */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
