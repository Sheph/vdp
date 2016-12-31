#include "vdp/usb_filter.h"
#include "vdp/byte_order.h"
#include <assert.h>
#include <string.h>

int vdp_usb_filter(struct vdp_usb_urb* urb, struct vdp_usb_filter_ops* ops,
    void* user_data)
{
    assert(urb);
    assert(ops);

    if (!urb || !ops) {
        return 0;
    }

    if ((urb->type != vdp_usb_urb_control) ||
        (VDP_USB_REQUESTTYPE_TYPE(urb->setup_packet->bRequestType) != VDP_USB_REQUESTTYPE_TYPE_STANDARD) ||
        (VDP_USB_URB_ENDPOINT_NUMBER(urb->endpoint_address) != 0)) {
        return 0;
    }

    switch (urb->setup_packet->bRequest) {
    case VDP_USB_REQUEST_GET_DESCRIPTOR: {
        if (!VDP_USB_URB_ENDPOINT_IN(urb->endpoint_address) ||
            (VDP_USB_REQUESTTYPE_RECIPIENT(urb->setup_packet->bRequestType) != VDP_USB_REQUESTTYPE_RECIPIENT_DEVICE) ||
            !VDP_USB_REQUESTTYPE_IN(urb->setup_packet->bRequestType)) {
            break;
        }

        vdp_u8 dt_type = vdp_u16le_to_cpu(urb->setup_packet->wValue) >> 8;
        vdp_u8 dt_index = vdp_u16le_to_cpu(urb->setup_packet->wValue) & 0xF;

        switch (dt_type) {
        case VDP_USB_DT_DEVICE:
            if ((dt_index == 0) && (urb->setup_packet->wIndex == 0)) {
                struct vdp_usb_device_descriptor descriptor;
                memset(&descriptor, 0, sizeof(descriptor));
                urb->status = ops->get_device_descriptor(user_data, &descriptor);
                if (urb->status == vdp_usb_urb_status_completed) {
                    urb->actual_length = vdp_usb_write_device_descriptor(&descriptor,
                        urb->transfer_buffer,
                        urb->transfer_length);
                }
                return 1;
            }
            break;
        case VDP_USB_DT_CONFIG:
            break;
        case VDP_USB_DT_STRING:
            break;
        case VDP_USB_DT_INTERFACE:
            break;
        case VDP_USB_DT_ENDPOINT:
            break;
        default:
            break;
        }
        break;
    }
    case VDP_USB_REQUEST_SET_ADDRESS: {
        if (!VDP_USB_URB_ENDPOINT_OUT(urb->endpoint_address) ||
            (VDP_USB_REQUESTTYPE_RECIPIENT(urb->setup_packet->bRequestType) != VDP_USB_REQUESTTYPE_RECIPIENT_DEVICE) ||
            !VDP_USB_REQUESTTYPE_OUT(urb->setup_packet->bRequestType) ||
            (urb->setup_packet->wIndex != 0) ||
            (urb->transfer_length != 0)) {
            break;
        }
        urb->status = ops->set_address(user_data, vdp_u16le_to_cpu(urb->setup_packet->wValue));
        return 1;
    }
    default:
        break;
    }

    return 0;
}
