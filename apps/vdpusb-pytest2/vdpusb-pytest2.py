import vdp;
import vdp.usb;

class TestEndpoint0(vdp.usb.gadget.Endpoint):
    def __init__(self):
        vdp.usb.gadget.Endpoint.__init__(self, dict(
            address = 0,
            dir = vdp.usb.gadget.EP_INOUT,
            type = vdp.usb.gadget.EP_CONTROL,
            max_packet_size = 64,
            interval = 0));

if __name__ == "__main__":
    endpoint0 = TestEndpoint0();
    print("Test");
