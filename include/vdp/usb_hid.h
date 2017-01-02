/*
 * Copyright (c) 2017, Stanislav Vorobiov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
