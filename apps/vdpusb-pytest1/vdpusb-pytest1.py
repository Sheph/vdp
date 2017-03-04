import vdp;
import vdp.usb;

if __name__ == "__main__":
	ctx = vdp.usb.Context();
	try:
		print("Device range: (%d, %d)" % ctx.get_device_range());
		device = ctx.open_device(0);
		device.attach(vdp.usb.SPEED_HIGH);
		device.detach();
		device.close();
	except vdp.usb.Error, e:
		print("Error: %s %d" % (e, e.result));
