import vdp;
import vdp.usb;
import select;
import struct;

test_report_descriptor = bytearray([
    0x05, 0x01,     # Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,     # Usage (Mouse)
    0xA1, 0x01,     # Collection (Application)
    0x09, 0x01,     #   Usage (Pointer)
    0xA1, 0x00,     #   Collection (Physical)
    0x05, 0x09,     #    Usage Page (Button)
    0x19, 0x01,     #    Usage Minimum (0x01)
    0x29, 0x08,     #    Usage Maximum (0x08)
    0x15, 0x00,     #    Logical Minimum (0)
    0x25, 0x01,     #    Logical Maximum (1)
    0x95, 0x08,     #    Report Count (8)
    0x75, 0x01,     #    Report Size (1)
    0x81, 0x02,     #    Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x00,     #    Report Count (0)
    0x81, 0x03,     #    Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  #     Usage Page (Vendor Defined 0xFF00)
    0x09, 0x40,     #    Usage (0x40)
    0x95, 0x02,     #    Report Count (2)
    0x75, 0x08,     #    Report Size (8)
    0x15, 0x81,     #    Logical Minimum (129)
    0x25, 0x7F,     #    Logical Maximum (127)
    0x81, 0x02,     #    Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,     #    Usage Page (Generic Desktop Ctrls)
    0x09, 0x38,     #    Usage (Wheel)
    0x15, 0x81,     #    Logical Minimum (129)
    0x25, 0x7F,     #    Logical Maximum (127)
    0x75, 0x08,     #    Report Size (8)
    0x95, 0x01,     #    Report Count (1)
    0x81, 0x06,     #    Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x30,     #    Usage (X)
    0x09, 0x31,     #    Usage (Y)
    0x16, 0x01, 0x80,  #     Logical Minimum (32769)
    0x26, 0xFF, 0x7F,  #     Logical Maximum (32767)
    0x75, 0x10,     #    Report Size (16)
    0x95, 0x02,     #    Report Count (2)
    0x81, 0x06,     #    Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,           #   End Collection
    0xC0,           # End Collection
]);

class TestFilter(vdp.usb.Filter):
    def __init__(self):
        vdp.usb.Filter.__init__(self);

    def get_device_descriptor(self):
        return dict(
            bDescriptorType = vdp.usb.DT_DEVICE,
            bcdUSB = 0x0200,
            bDeviceClass = 0,
            bDeviceSubClass = 0,
            bDeviceProtocol = 0,
            bMaxPacketSize0 = 64,
            idVendor = 0x046d,
            idProduct = 0xc051,
            bcdDevice = 0x3000,
            iManufacturer = 1,
            iProduct = 2,
            iSerialNumber = 0,
            bNumConfigurations = 1);

    def get_qualifier_descriptor(self):
        return dict(
            bDescriptorType = vdp.usb.DT_QUALIFIER,
            bcdUSB = 0x0200,
            bDeviceClass = 0,
            bDeviceSubClass = 0,
            bDeviceProtocol = 0,
            bMaxPacketSize0 = 64,
            bNumConfigurations = 1);

    def get_config_descriptor(self, index):
        print("get_config_descriptor(%d)" % (index,));
        config_descriptor = dict( \
            bDescriptorType = vdp.usb.DT_CONFIG,
            bNumInterfaces = 1,
            bConfigurationValue = 1,
            iConfiguration = 0,
            bmAttributes = vdp.usb.CONFIG_ATT_ONE | vdp.usb.CONFIG_ATT_WAKEUP,
            bMaxPower = 49);
        return (config_descriptor,
            [(vdp.usb.DT_INTERFACE, struct.pack("<BBBBBBB", 0, 0, 1, vdp.usb.CLASS_HID, vdp.usb.SUBCLASS_BOOT, vdp.usb.PROTOCOL_MOUSE, 0)),
            (vdp.usb.HID_DT_HID, struct.pack("<HBBBH", 0x0110, 0, 1, vdp.usb.HID_DT_REPORT, len(test_report_descriptor))),
            (vdp.usb.DT_ENDPOINT, struct.pack("<BBHB", 0x81, vdp.usb.ENDPOINT_XFER_INT, 8, 7))]);

    def get_string_descriptor(self):
        return [(0x0409, [(1, "Logitech"), (2, "USB-PS/2 Optical Mouse")])];

    def set_address(self, address):
        print("set_address(%d)" % (address,));
        return vdp.usb.URB_STATUS_COMPLETED;

filter = TestFilter();

if __name__ == "__main__":
    ctx = vdp.usb.Context();
    try:
        print("Device range: (%d, %d)" % ctx.get_device_range());
        device = ctx.open_device(0);
        device.attach(vdp.usb.SPEED_HIGH);
        fd = device.get_fd();

        try:
            while True:
                rd, wr, er = select.select([fd], [], [])
                if rd:
                    evt = device.get_event();
                    if isinstance(evt, vdp.usb.SignalEvent):
                        print("Signal Event %d" % evt.signal);
                    elif isinstance(evt, vdp.usb.URBEvent):
                        print("URB Event %d %d" % (evt.urb.id, evt.urb.type));
                        if evt.urb.type == vdp.usb.URB_CONTROL:
                            print("- Control: request_type = 0x%X, request = 0x%X, value = %d, index = %d" %
                                (evt.urb.setup_packet.request_type,
                                evt.urb.setup_packet.request,
                                evt.urb.setup_packet.value,
                                evt.urb.setup_packet.index));
                        if not filter.filter(evt.urb):
                            evt.urb.status = vdp.usb.URB_STATUS_UNLINKED;
                        evt.urb.complete();
                    elif isinstance(evt, vdp.usb.UnlinkURBEvent):
                        print("Unlink URB Event %d" % evt.id);
        except KeyboardInterrupt:
            pass

        device.detach();
    except Exception, e:
        print("Error: %s" % e);
