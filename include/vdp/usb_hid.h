#ifndef _VDP_USB_HID_H_
#define _VDP_USB_HID_H_

#include "vdp/usb.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * USB HID definitions
 * @{
 */

#define VDP_USB_CLASS_HID 3

#define VDP_USB_SUBCLASS_BOOT       1
#define VDP_USB_PROTOCOL_KEYBOARD   1
#define VDP_USB_PROTOCOL_MOUSE      2

#define VDP_USB_HID_REQUEST_GET_REPORT      0x01
#define VDP_USB_HID_REQUEST_GET_IDLE        0x02
#define VDP_USB_HID_REQUEST_GET_PROTOCOL    0x03
#define VDP_USB_HID_REQUEST_SET_REPORT      0x09
#define VDP_USB_HID_REQUEST_SET_IDLE        0x0A
#define VDP_USB_HID_REQUEST_SET_PROTOCOL    0x0B

#define VDP_USB_HID_DT_HID          0x21
#define VDP_USB_HID_DT_REPORT       0x22
#define VDP_USB_HID_DT_PHYSICAL     0x23

#pragma pack(1)
struct vdp_usb_hid_class_descriptor
{
    vdp_u8 bDescriptorType;
    vdp_u16 wDescriptorLength;
};

struct vdp_usb_hid_descriptor
{
    vdp_u8 bLength;
    vdp_u8 bDescriptorType;
    vdp_u16 bcdHID;
    vdp_u8 bCountryCode;
    vdp_u8 bNumDescriptors;
    struct vdp_usb_hid_class_descriptor desc[1];
};
#pragma pack()

/*
 * @}
 */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
