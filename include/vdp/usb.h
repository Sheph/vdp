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

#ifndef _VDP_USB_H_
#define _VDP_USB_H_

#include "vdp/types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * VDP USB result codes.
 * @{
 */

typedef enum
{
    vdp_usb_success = 0,    ///< Success, no error.
    vdp_usb_nomem,          ///< Not enough memory.
    vdp_usb_misuse,         ///< Interface misuse.
    vdp_usb_unknown,        ///< Some unknown error.
    vdp_usb_not_found,      ///< Entity not found.
    vdp_usb_busy,           ///< Device is busy.
    vdp_usb_protocol_error  ///< Kernel mode - user mode protocol error, bad kernel module version ?
} vdp_usb_result;

const char* vdp_usb_result_to_str(vdp_usb_result res);

/*
 * @}
 */

/*
 * Context must be created before working with devices.
 * @{
 */

struct vdp_usb_context;

/*
 * Initialize VDP USB system.
 * if 'log_file' is NULL then logging will be turned off.
 */
vdp_usb_result vdp_usb_init(FILE* log_file,
    vdp_log_level log_level,
    struct vdp_usb_context** context);

/*
 * Cleanup must be the final step, you should close all opened devices before this call.
 */
void vdp_usb_cleanup(struct vdp_usb_context* context);

/*
 * Get lower and upper device number in system. Returns vdp_usb_not_found if no devices are found
 * in the system.
 */
vdp_usb_result vdp_usb_get_device_range(struct vdp_usb_context* context, vdp_u8* device_lower, vdp_u8* device_upper);

/*
 * @}
 */

/*
 * Device open/close, 'device_number' is zero based. Any particular device can be opened only once.
 * @{
 */

struct vdp_usb_device;

vdp_usb_result vdp_usb_device_open(struct vdp_usb_context* context,
    vdp_u8 device_number,
    struct vdp_usb_device** device);

void vdp_usb_device_close(struct vdp_usb_device* device);

/*
 * @}
 */

/*
 * Device control.
 * @{
 */

typedef enum
{
    vdp_usb_speed_low = 0,
    vdp_usb_speed_full = 1,
    vdp_usb_speed_high = 2
} vdp_usb_speed;

/*
 * Inform the system that the device has attached and that it's ready to react upon
 * events.
 */
vdp_usb_result vdp_usb_device_attach(struct vdp_usb_device* device,
    vdp_usb_speed speed);

/*
 * Inform the system that the device was detached and can't
 * handle new events.
 */
vdp_usb_result vdp_usb_device_detach(struct vdp_usb_device* device);

int vdp_usb_device_get_busnum(struct vdp_usb_device* device);
int vdp_usb_device_get_portnum(struct vdp_usb_device* device);

/*
 * @}
 */

/*
 * Device events.
 * @{
 */

typedef enum
{
    vdp_usb_event_none = 0,
    vdp_usb_event_signal = 1,
    vdp_usb_event_urb = 2,
    vdp_usb_event_unlink_urb = 3
} vdp_usb_event_type;

typedef enum
{
    vdp_usb_signal_reset_start = 0,
    vdp_usb_signal_reset_end = 1,
    vdp_usb_signal_power_on = 2,
    vdp_usb_signal_power_off = 3
} vdp_usb_signal_type;

const char* vdp_usb_signal_type_to_str(vdp_usb_signal_type signal_type);

struct vdp_usb_signal
{
    vdp_usb_signal_type type;
};

/*
 * Same as VDP_USB_ENDPOINT_XFER_XXX.
 */
typedef enum
{
    vdp_usb_urb_control = 0,
    vdp_usb_urb_iso = 1,
    vdp_usb_urb_bulk = 2,
    vdp_usb_urb_int = 3
} vdp_usb_urb_type;

const char* vdp_usb_urb_type_to_str(vdp_usb_urb_type urb_type);

typedef enum
{
    vdp_usb_urb_status_undefined = 0,  /* Not yet determined. */
    vdp_usb_urb_status_completed = 1,  /* Completed successfully. */
    vdp_usb_urb_status_unlinked = 2,   /* URB was unlinked. */
    vdp_usb_urb_status_error = 3,      /* USB transfer error. */
    vdp_usb_urb_status_stall = 4,      /* Endpoint stalled or request not supported (for control transfers). */
    vdp_usb_urb_status_overflow = 5    /* Endpoint returns more data than expected. */
} vdp_usb_urb_status;

struct vdp_usb_iso_packet
{
    vdp_byte* buffer;

    vdp_u32 length;

    vdp_u32 actual_length;

    vdp_usb_urb_status status;
};

#pragma pack(1)
struct vdp_usb_control_setup
{
    vdp_u8 bRequestType;

    vdp_u8 bRequest;

    vdp_u16 wValue;

    vdp_u16 wIndex;

    vdp_u16 wLength;
};
#pragma pack()

#define VDP_USB_REQUESTTYPE_IN(bRequestType) ((bRequestType) & 0x80)
#define VDP_USB_REQUESTTYPE_OUT(bRequestType) (!VDP_USB_REQUESTTYPE_IN(bRequestType))
#define VDP_USB_REQUESTTYPE_TYPE(bRequestType) (((bRequestType) >> 5) & 0x03)
#define VDP_USB_REQUESTTYPE_RECIPIENT(bRequestType) ((bRequestType) & 0x1F)

#define VDP_USB_REQUESTTYPE_TYPE_STANDARD 0x00
#define VDP_USB_REQUESTTYPE_TYPE_CLASS    0x01
#define VDP_USB_REQUESTTYPE_TYPE_VENDOR   0x02
#define VDP_USB_REQUESTTYPE_TYPE_RESERVED 0x03

#define VDP_USB_REQUESTTYPE_RECIPIENT_DEVICE    0x00
#define VDP_USB_REQUESTTYPE_RECIPIENT_INTERFACE 0x01
#define VDP_USB_REQUESTTYPE_RECIPIENT_ENDPOINT  0x02
#define VDP_USB_REQUESTTYPE_RECIPIENT_OTHER     0x03
#define VDP_USB_REQUESTTYPE_RECIPIENT_PORT      0x04
#define VDP_USB_REQUESTTYPE_RECIPIENT_RPIPE     0x05

#define VDP_USB_REQUEST_GET_STATUS          0x00
#define VDP_USB_REQUEST_CLEAR_FEATURE       0x01
#define VDP_USB_REQUEST_SET_FEATURE         0x03
#define VDP_USB_REQUEST_SET_ADDRESS         0x05
#define VDP_USB_REQUEST_GET_DESCRIPTOR      0x06
#define VDP_USB_REQUEST_SET_DESCRIPTOR      0x07
#define VDP_USB_REQUEST_GET_CONFIGURATION   0x08
#define VDP_USB_REQUEST_SET_CONFIGURATION   0x09
#define VDP_USB_REQUEST_GET_INTERFACE       0x0A
#define VDP_USB_REQUEST_SET_INTERFACE       0x0B
#define VDP_USB_REQUEST_SYNCH_FRAME         0x0C
#define VDP_USB_REQUEST_SET_ENCRYPTION      0x0D
#define VDP_USB_REQUEST_GET_ENCRYPTION      0x0E
#define VDP_USB_REQUEST_SET_HANDSHAKE       0x0F
#define VDP_USB_REQUEST_GET_HANDSHAKE       0x10
#define VDP_USB_REQUEST_SET_CONNECTION      0x11
#define VDP_USB_REQUEST_SET_SECURITY_DATA   0x12
#define VDP_USB_REQUEST_GET_SECURITY_DATA   0x13
#define VDP_USB_REQUEST_SET_WUSB_DATA       0x14
#define VDP_USB_REQUEST_LOOPBACK_DATA_WRITE 0x15
#define VDP_USB_REQUEST_LOOPBACK_DATA_READ  0x16
#define VDP_USB_REQUEST_SET_INTERFACE_DS    0x17

const char* vdp_usb_request_type_direction_to_str(vdp_u8 bRequestType);

const char* vdp_usb_request_type_type_to_str(vdp_u8 bRequestType);

const char* vdp_usb_request_type_recipient_to_str(vdp_u8 bRequestType);

const char* vdp_usb_request_to_str(vdp_u8 bRequest);

#define VDP_USB_URB_ENDPOINT_IN(bEndpointAddress) ((bEndpointAddress) & 0x80)
#define VDP_USB_URB_ENDPOINT_OUT(bEndpointAddress) (!VDP_USB_URB_ENDPOINT_IN(bEndpointAddress))
#define VDP_USB_URB_ENDPOINT_NUMBER(bEndpointAddress) ((bEndpointAddress) & 0x0F)

struct vdp_usb_urb
{
    vdp_u32 id;

    vdp_usb_urb_type type;

#define VDP_USB_URB_ZERO_PACKET (1 << 0)
    vdp_u32 flags;

    vdp_u8 endpoint_address;

    const struct vdp_usb_control_setup* setup_packet;

    vdp_byte* transfer_buffer;

    vdp_u32 transfer_length;

    vdp_u32 actual_length;

    vdp_u32 number_of_packets;

    vdp_u32 interval;

    vdp_usb_urb_status status;

    struct vdp_usb_iso_packet* iso_packets;
};

/*
 * Dump URB details to 'buff'. 'buff_size' is the size of the 'buff' in bytes.
 * Note that 'buff' should not be very large (perhaps 512 bytes would do), since
 * URB's payload doesn't get dumped.
 */
void vdp_usb_urb_to_str(const struct vdp_usb_urb* urb, char* buff, size_t buff_size);

struct vdp_usb_unlink_urb
{
    vdp_u32 id;
};

struct vdp_usb_event
{
    vdp_usb_event_type type;

    union
    {
        struct vdp_usb_signal signal;
        struct vdp_usb_urb* urb;
        struct vdp_usb_unlink_urb unlink_urb;
    } data;
};

/*
 * Instruct the device to wait for incoming event. Returned 'fd' can be
 * used in 'select'/'WaitForXXXObject' to wait for event. Once it's set, you can call
 * vdp_usb_device_get_event.
 */
vdp_usb_result vdp_usb_device_wait_event(struct vdp_usb_device* device, vdp_fd* fd);

/*
 * Returns the event.
 * Note that 'event.type' can be 'vdp_usb_event_none' on return, in this case you'll have to wait for event again.
 * 'none' events can happen, for example, if event has arrived but when we tried to get its data
 * it wasn't there. Such situation is possible when system is canceling URBs, for example.
 */
vdp_usb_result vdp_usb_device_get_event(struct vdp_usb_device* device, struct vdp_usb_event* event);

/*
 * Complete an URB event. Though it's not required, URBs should be completed in the same order
 * in which they were received since it's more natural for
 * the device to process the URBs one by one.
 */
vdp_usb_result vdp_usb_complete_urb(struct vdp_usb_urb* urb);

/*
 * Frees the URB returned by vdp_usb_device_get_event.
 * Must be called when you're done with the URB.
 */
void vdp_usb_free_urb(struct vdp_usb_urb* urb);

/*
 * @}
 */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
