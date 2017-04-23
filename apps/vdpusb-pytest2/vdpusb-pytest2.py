import vdp;
import vdp.usb;
import select;
import struct;
import time;
import signal;

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

    def enable(self, value):
        print("ep0 enable = %d" % value);

    def enqueue(self, request):
        request.status = vdp.usb.URB_STATUS_STALL;

        if request.setup_packet.request == vdp.usb.REQUEST_GET_DESCRIPTOR:
            if (request.setup_packet.type == vdp.usb.gadget.REQUEST_STANDARD) and \
            (request.setup_packet.recipient == vdp.usb.gadget.REQUEST_INTERFACE) and request.is_in:
                dt_type = request.setup_packet.value >> 8;
                if dt_type == vdp.usb.HID_DT_REPORT:
                    print("get_hid_report_descriptor()");
                    data = memoryview(request);
                    request.status = vdp.usb.URB_STATUS_COMPLETED;
                    request.actual_length = min(len(data), len(test_report_descriptor));
                    data[0:request.actual_length] = test_report_descriptor[0:request.actual_length];
        elif request.setup_packet.request == vdp.usb.HID_REQUEST_SET_IDLE:
            if (request.setup_packet.type == vdp.usb.gadget.REQUEST_CLASS) and \
            (request.setup_packet.recipient == vdp.usb.gadget.REQUEST_INTERFACE) and not request.is_in:
                print("set_idle()");
                request.status = vdp.usb.URB_STATUS_COMPLETED;

        request.complete();

    def dequeue(self, request):
        print("ep0 dequeue = %d" % request.id);

    def clear_stall(self):
        print("ep0 clear_stall");

class TestEndpoint1(vdp.usb.gadget.Endpoint):
    def __init__(self):
        vdp.usb.gadget.Endpoint.__init__(self, dict(
            address = 1,
            dir = vdp.usb.gadget.EP_IN,
            type = vdp.usb.gadget.EP_INT,
            max_packet_size = 8,
            interval = 7));

    def enable(self, value):
        print("ep1 enable = %d" % value);

    def enqueue(self, request):
        print("ep1 enqueue = %d" % request.id);
        time.sleep(float(request.interval_us) / 1000000);
        data = memoryview(request);

        if len(data) >= 8:
            data[0:8] = bytearray([
                0, 0, 0, 0,
                # X:
                0, 0,
                # Y:
                0, 0]);

            request.actual_length = 8;

        request.status = vdp.usb.URB_STATUS_COMPLETED;
        request.complete();

    def dequeue(self, request):
        print("ep1 dequeue = %d" % request.id);

    def clear_stall(self):
        print("ep1 clear_stall");
        return vdp.usb.URB_STATUS_COMPLETED;

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

    def enable(self, value):
        print("iface enable = %d" % value);

class TestConfig(vdp.usb.gadget.Config):
    def __init__(self):
        vdp.usb.gadget.Config.__init__(self, dict(
            number = 1,
            attributes = vdp.usb.gadget.CONFIG_ATT_ONE | vdp.usb.gadget.CONFIG_ATT_WAKEUP,
            max_power = 49,
            description = 0,
            interfaces = [TestInterface()]));

    def enable(self, value):
        print("config enable = %d" % value);

class TestGadget(vdp.usb.gadget.Gadget):
    def __init__(self):
        vdp.usb.gadget.Gadget.__init__(self, dict(
            bcd_usb = 0x0200,
            bcd_device = 0x3000,
            klass = 0,
            subklass = 0,
            protocol = 0,
            vendor_id = 0x046d,
            product_id = 0xc051,
            manufacturer = 1,
            product = 2,
            serial_number = 0,
            string_tables = [(0x0409, [(1, "Logitech"), (2, "USB-PS/2 Optical Mouse")])],
            configs = [TestConfig()],
            endpoint0 = TestEndpoint0()));

    def reset(self, start):
        print("gadget reset = %d" % start);

    def power(self, on):
        print("gadget power = %d" % on);

    def set_address(self, address):
        print("gadget set_address = %d" % address);

exiting = False;

def sig_handler(signum, frame):
    global exiting;
    exiting = True;

if __name__ == "__main__":
    signal.signal(signal.SIGINT, sig_handler);

    ctx = vdp.usb.Context();
    gadget = TestGadget();
    try:
        print("Device range: (%d, %d)" % ctx.get_device_range());
        device = ctx.open_device(0);
        device.attach(vdp.usb.SPEED_HIGH);
        fd = device.get_fd();

        while not exiting:
            rd, wr, er = select.select([fd], [], [])
            if rd:
                gadget.event(device);

        device.detach();
    except Exception, e:
        print("Error: %s" % e);
