#ifndef _VDP_USB_UTIL_H_
#define _VDP_USB_UTIL_H_

#include "vdp/usb.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * USB descriptor definitions
 * See USB 2.0 specification, section 9.6.
 * @{
 */

#define VDP_USB_DT_DEVICE    0x01
#define VDP_USB_DT_CONFIG    0x02
#define VDP_USB_DT_STRING    0x03
#define VDP_USB_DT_INTERFACE 0x04
#define VDP_USB_DT_ENDPOINT  0x05

#define VDP_USB_CLASS_PER_INTERFACE          0 /* for DeviceClass */
#define VDP_USB_CLASS_AUDIO                  1
#define VDP_USB_CLASS_COMM                   2
#define VDP_USB_CLASS_HID                    3
#define VDP_USB_CLASS_PHYSICAL               5
#define VDP_USB_CLASS_STILL_IMAGE            6
#define VDP_USB_CLASS_PRINTER                7
#define VDP_USB_CLASS_MASS_STORAGE           8
#define VDP_USB_CLASS_HUB                    9
#define VDP_USB_CLASS_CDC_DATA            0x0a
#define VDP_USB_CLASS_CSCID               0x0b /* chip+ smart card */
#define VDP_USB_CLASS_CONTENT_SEC         0x0d /* content security */
#define VDP_USB_CLASS_VIDEO               0x0e
#define VDP_USB_CLASS_WIRELESS_CONTROLLER 0xe0
#define VDP_USB_CLASS_MISC                0xef
#define VDP_USB_CLASS_APP_SPEC            0xfe
#define VDP_USB_CLASS_VENDOR_SPEC         0xff

#define VDP_USB_SUBCLASS_VENDOR_SPEC      0xff

#pragma pack(1)
struct vdp_usb_device_descriptor
{
    vdp_u8 bLength;
    vdp_u8 bDescriptorType;
    vdp_u16 bcdUSB;
    vdp_u8 bDeviceClass;
    vdp_u8 bDeviceSubClass;
    vdp_u8 bDeviceProtocol;
    vdp_u8 bMaxPacketSize0;
    vdp_u16 idVendor;
    vdp_u16 idProduct;
    vdp_u16 bcdDevice;
    vdp_u8 iManufacturer;
    vdp_u8 iProduct;
    vdp_u8 iSerialNumber;
    vdp_u8 bNumConfigurations;
};

#define VDP_USB_CONFIG_ATT_ONE       (1 << 7) /* must be set */
#define VDP_USB_CONFIG_ATT_SELFPOWER (1 << 6) /* self powered */
#define VDP_USB_CONFIG_ATT_WAKEUP    (1 << 5) /* can wakeup */
#define VDP_USB_CONFIG_ATT_BATTERY   (1 << 4) /* battery powered */

struct vdp_usb_config_descriptor
{
    vdp_u8 bLength;
    vdp_u8 bDescriptorType;
    vdp_u16 wTotalLength;
    vdp_u8 bNumInterfaces;
    vdp_u8 bConfigurationValue;
    vdp_u8 iConfiguration;
    vdp_u8 bmAttributes;
    vdp_u8 bMaxPower;
};

struct vdp_usb_interface_descriptor
{
    vdp_u8 bLength;
    vdp_u8 bDescriptorType;
    vdp_u8 bInterfaceNumber;
    vdp_u8 bAlternateSetting;
    vdp_u8 bNumEndpoints;
    vdp_u8 bInterfaceClass;
    vdp_u8 bInterfaceSubClass;
    vdp_u8 bInterfaceProtocol;
    vdp_u8 iInterface;
};

#define VDP_USB_DT_ENDPOINT_SIZE       7
#define VDP_USB_DT_ENDPOINT_AUDIO_SIZE 9 /* Audio extension */

#define VDP_USB_ENDPOINT_SYNC_NONE     (0 << 2)
#define VDP_USB_ENDPOINT_SYNC_ASYNC    (1 << 2)
#define VDP_USB_ENDPOINT_SYNC_ADAPTIVE (2 << 2)
#define VDP_USB_ENDPOINT_SYNC_SYNC     (3 << 2)

#define VDP_USB_ENDPOINT_XFER_CONTROL  0
#define VDP_USB_ENDPOINT_XFER_ISO      1
#define VDP_USB_ENDPOINT_XFER_BULK     2
#define VDP_USB_ENDPOINT_XFER_INT      3

struct vdp_usb_endpoint_descriptor
{
    vdp_u8 bLength;
    vdp_u8 bDescriptorType;
    vdp_u8 bEndpointAddress;
    vdp_u8 bmAttributes;
    vdp_u16 wMaxPacketSize;
    vdp_u8 bInterval;

    /*
     * NOTE: These two are ONLY in audio endpoints.
     * Use VDP_USB_DT_ENDPOINT*_SIZE in bLength, not sizeof.
     */

    vdp_u8 bRefresh;
    vdp_u8 bSynchAddress;
};

struct vdp_usb_string_descriptor
{
    vdp_u8 bLength;
    vdp_u8 bDescriptorType;
    vdp_u16 wData[1]; /* UTF-16LE encoded */
};
#pragma pack()

/*
 * These are helpers for easier string descriptor handling
 * @{
 */

struct vdp_usb_string
{
    vdp_u8 index;
    const char* str;
};

struct vdp_usb_string_table
{
    vdp_u16 language_id; /* in host byte order */
    struct vdp_usb_string* strings; /* string array terminated by {0, NULL} entry */
};

/*
 * @}
 */

/*
 * @}
 */

/*
 * USB descriptor helper functions
 * @{
 */

/*
 * Given string table array at 'tables' (terminated by {0, NULL} entry), language id and string index
 * writes the corresponding string descriptor to 'buff'.
 * Returns number of bytes written or 0 in case of error.
 */
vdp_u32 vdp_usb_write_string_descriptor(const struct vdp_usb_string_table* tables,
    vdp_u16 language_id,
    vdp_u8 index,
    vdp_byte* buff,
    vdp_u32 buff_size);

/*
 * Returns number of bytes written.
 */
vdp_u32 vdp_usb_write_device_descriptor(
    const struct vdp_usb_device_descriptor* descriptor,
    vdp_byte* buff,
    vdp_u32 buff_size);

/*
 * Returns number of bytes written.
 */
vdp_u32 vdp_usb_write_config_descriptor(
    const struct vdp_usb_config_descriptor* descriptor,
    vdp_byte* buff,
    vdp_u32 buff_size);

/*
 * Returns number of bytes written.
 */
vdp_u32 vdp_usb_write_interface_descriptor(
    const struct vdp_usb_interface_descriptor* descriptor,
    vdp_byte* buff,
    vdp_u32 buff_size);

/*
 * Returns number of bytes written.
 */
vdp_u32 vdp_usb_write_endpoint_descriptor(
    const struct vdp_usb_endpoint_descriptor* descriptor,
    vdp_byte* buff,
    vdp_u32 buff_size);

/*
 * @}
 */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif