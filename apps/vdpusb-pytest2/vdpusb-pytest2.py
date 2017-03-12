import vdp;
import vdp.usb;
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

class TestEndpoint0(vdp.usb.gadget.Endpoint):
    def __init__(self):
        vdp.usb.gadget.Endpoint.__init__(self, dict(
            address = 0,
            dir = vdp.usb.gadget.EP_INOUT,
            type = vdp.usb.gadget.EP_CONTROL,
            max_packet_size = 64,
            interval = 0));

class TestEndpoint1(vdp.usb.gadget.Endpoint):
    def __init__(self):
        vdp.usb.gadget.Endpoint.__init__(self, dict(
            address = 1,
            dir = vdp.usb.gadget.EP_IN,
            type = vdp.usb.gadget.EP_INT,
            max_packet_size = 8,
            interval = 7));

class TestInterface(vdp.usb.gadget.Interface):
    def __init__(self):
        vdp.usb.gadget.Interface.__init__(self, dict(
            number = 0,
            alt_setting = 0,
            klass = vdp.usb.CLASS_HID,
            subklass = vdp.usb.SUBCLASS_BOOT,
            protocol = vdp.usb.PROTOCOL_MOUSE,
            description = 0,
            descriptors = [(vdp.usb.HID_DT_HID, struct.pack("<HBBBH", 0x0110, 0, 1, vdp.usb.HID_DT_REPORT, len(test_report_descriptor)))],
            endpoints = [TestEndpoint1()]));

class TestConfig(vdp.usb.gadget.Config):
    def __init__(self):
        vdp.usb.gadget.Config.__init__(self, dict(
            number = 1,
            attributes = vdp.usb.gadget.CONFIG_ATT_ONE | vdp.usb.gadget.CONFIG_ATT_WAKEUP,
            max_power = 49,
            description = 0,
            interfaces = [TestInterface()]));

if __name__ == "__main__":
    ep0 = TestEndpoint0();
    cfg = TestConfig();
    print("Test");
