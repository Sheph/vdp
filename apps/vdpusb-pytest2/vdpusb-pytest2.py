import vdp;
import vdp.usb;

class TestEndpoint0(vdp.usb.gadget.Endpoint):
    def __init__(self):
        vdp.usb.gadget.Endpoint.__init__(self);

if __name__ == "__main__":
    endpoint0 = TestEndpoint0();
    print("Test");
