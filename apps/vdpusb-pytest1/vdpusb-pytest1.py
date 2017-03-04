import vdp;
import vdp.usb;

if __name__ == "__main__":
	ctx = vdp.usb.Context();
	try:
		evt = vdp.usb.SignalEvent(vdp.usb.SIGNAL_RESET_END);
		print("Evt %d %d" % (evt.type, evt.signal));

		print("Device range: (%d, %d)" % ctx.get_device_range());
		device = ctx.open_device(0);
		device.attach(vdp.usb.SPEED_HIGH);
		device.detach();
		device.close();
	except vdp.usb.Error, e:
		print("Error: %s %d" % (e, e.result));
