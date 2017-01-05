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

#ifndef _VDPHCI_COMMON_H_
#define _VDPHCI_COMMON_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This is driver name.
 */
#define VDPHCI_NAME "vdphci"

/*
 * Maximum number of ports allowed per HCD.
 */
#define VDPHCI_MAX_PORTS 10

/*
 * This is devfs device file prefix.
 */
#define VDPHCI_DEVICE_PREFIX "vdphcidev"

/*
 * Device control codes magic.
 */
#define VDPHCI_IOC_MAGIC 'V'

/*
 * Get info.
 */
struct vdphci_info
{
    int busnum;
    int portnum;
};

#define VDPHCI_IOC_GET_INFO _IOR(VDPHCI_IOC_MAGIC, 0, struct vdphci_info)

/*
 * HEvent related. HEvents are sent by HCD to device.
 */

typedef enum
{
    vdphci_hevent_type_signal = 0,
    vdphci_hevent_type_urb = 1,
    vdphci_hevent_type_unlink_urb = 2,
} vdphci_hevent_type;

/*
 * Sent before HEvent itself.
 */
struct vdphci_hevent_header
{
    /*
     * The type of event.
     */
    vdphci_hevent_type type;

    /*
     * Length of the event data.
     */
    __u32 length;
};

typedef enum
{
    vdphci_hsignal_reset_start = 0,
    vdphci_hsignal_reset_end = 1,
    vdphci_hsignal_power_on = 2,
    vdphci_hsignal_power_off = 3
} vdphci_hsignal;

/*
 * See USB 2.0 specification, section 9.1.1.
 * We're reporting signals instead of states because it's more natural, the physical device
 * just reacts upon external signals and changes state internally. For example, if
 * it's in 'configured' state and receives a reset signal it goes back to 'default' state.
 * As you can see the device actually received 'reset signal', not 'default state'.
 */
struct vdphci_hevent_signal
{
    vdphci_hsignal signal;
};

typedef enum
{
    vdphci_urb_type_control = 0,
    vdphci_urb_type_bulk = 1,
    vdphci_urb_type_int = 2,
    vdphci_urb_type_iso = 3
} vdphci_urb_type;

/*
 * Describes IN or OUT isochronous packet that's sent with URB.
 * We don't need an offset here since ISO packets themselves are stored one by one, each of
 * 'length' size
 */
struct vdphci_h_iso_packet
{
    /*
     * Data length.
     */
    __u32 length;
};

/*
 * User receives this event when the system submits an URB for the device.
 * Of course, physical device never receives URBs, it receives packets, but we're reporting
 * URBs both for performance reasons and in order to simplify user-mode processing, it's
 * a lot easier to process an URB than a bunch of packets.
 * Anyway, you can process this URB as if it was a bunch of packets, in case of control and
 * bulk transfers you can break the transfer down into packets of size 'wMaxPacketSize'
 * and process each separately (for example, communicate to some physical device); in case of
 * interrupt and isochronous transfers you can schedule next packet transfer once in
 * 'interval' (micro)frames.
 */
struct vdphci_hevent_urb
{
    /*
     * Every URB is identified by its sequence number.
     */
    __u32 seq_num;

    /*
     * Type of transfer.
     */
    vdphci_urb_type type;

    /*
     * Flags which describe how to carry out the transfer.
     */
    /*
     * Indicates that bulk OUT transfers
     * should always terminate with a short packet, even if it means adding an
     * extra zero length packet.
     */
#define VDPHCI_URB_ZERO_PACKET (1 << 0)
    __u32 flags;

    /*
     * Endpoint address as described in USB 2.0 specification, section 9.6.6.
     * Plus, for control endpoints direction bit is set, that sort of
     * eases the use
     */
#define VDPHCI_USB_ENDPOINT_IN(endpoint_address) ((endpoint_address) & 0x80)
#define VDPHCI_USB_ENDPOINT_OUT(endpoint_address) (!VDPHCI_USB_ENDPOINT_IN(endpoint_address))
#define VDPHCI_USB_ENDPOINT_NUMBER(endpoint_address) ((endpoint_address) & 0x0F)
    __u8 endpoint_address;

    /*
     * Total number of bytes to transfer. In case of isochronous mode - total number of
     * bytes in all packets.
     */
    __u32 transfer_length;

    /*
     * Isochronous mode only, size of 'packets' array at the end.
     */
    __u32 number_of_packets;

    /*
     * In case of interrupt and isochronous mode user should schedule
     * transfers at least once in 'interval' us
     */
    __u32 interval;

    union
    {
        /*
         * Use only in case of non-isochronous OUT transfers or control IN transfer,
         * in the latter case 'buff' must contain the setup packet.
         */
        char buff[1];

        /*
         * Isochronous mode only, the size of this array is 'number_of_packets'. In case
         * of OUT transfer this array is immediately followed by transfer data.
         */
        struct vdphci_h_iso_packet packets[1];
    } data;
};

/*
 * Every URB reported by the HCD can be unlinked, when user receives this event
 * he must terminate packet transfers for the URB identified by 'seq_num' and send URB DEvent
 * as soon as possible.
 */
struct vdphci_hevent_unlink_urb
{
    /*
     * Which URB to terminate.
     */
    __u32 seq_num;
};

/*
 * DEvent related. DEvents are sent by device to HCD.
 */

typedef enum
{
    vdphci_devent_type_signal = 0,
    vdphci_devent_type_urb = 1
} vdphci_devent_type;

/*
 * Sent before DEvent itself.
 */
struct vdphci_devent_header
{
    /*
     * The type of event.
     */
    vdphci_devent_type type;
};

typedef enum
{
    vdphci_dsignal_attached = 0,
    vdphci_dsignal_detached = 1
} vdphci_dsignal;

/*
 * Specifies either device was attached or detached.
 */
struct vdphci_devent_signal
{
    vdphci_dsignal signal;
};

/*
 * URB completion status.
 */
typedef enum
{
    vdphci_urb_status_completed = 0,  /* Completed successfully. */
    vdphci_urb_status_unlinked = 1,   /* URB was unlinked. */
    vdphci_urb_status_error = 2,      /* USB transfer error. */
    vdphci_urb_status_stall = 3,      /* Endpoint stalled or request not supported (for control transfers). */
    vdphci_urb_status_overflow = 4,   /* Endpoint returns more data than expected. */
    vdphci_urb_status_unprocessed = 5 /* Unable to process the URB, due to user-kernel protocol errors or insufficient memory */
} vdphci_urb_status;

/*
 * Describes IN or OUT isochronous packet reply.
 * We don't need an offset here since ISO packets themselves are stored one by one, each of
 * vdphci_h_iso_packet::length size
 */
struct vdphci_d_iso_packet
{
    /*
     * The status of this operation.
     */
    vdphci_urb_status status;

    /*
     * Number of bytes actually transfered.
     */
    __u32 actual_length;
};

/*
 * Users send this as a reply to 'vdphci_hevent_urb' or 'vdphci_hevent_urb_unlink'.
 */
struct vdphci_devent_urb
{
    /*
     * Every URB is identified by its sequence number.
     */
    __u32 seq_num;

    /*
     * Status for non-isochronous transfers.
     */
    vdphci_urb_status status;

    /*
     * Total number of bytes transfered. In case of isochronous mode - total number of
     * bytes in all packets.
     */
    __u32 actual_length;

    union
    {
        /*
         * Use only in case of non-isochronous IN transfers.
         */
        char buff[1];

        /*
         * Isochronous mode only, the size of this array is 'number_of_packets'. In case
         * of IN transfer this array is immediately followed by transfer data.
         */
        struct vdphci_d_iso_packet packets[1];
    } data;
};

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
