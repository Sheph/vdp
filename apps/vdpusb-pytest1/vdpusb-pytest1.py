import vdp;
import vdp.usb;

if __name__ == "__main__":
	ctx = vdp.usb.Context();
	try:
		print("Test (%d, %d)" % ctx.get_device_range());
	except vdp.usb.Error, e:
		print("Error: %s %d" % (e, e.result));
