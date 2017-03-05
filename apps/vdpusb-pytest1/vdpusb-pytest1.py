import vdp;
import vdp.usb;
import select;

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
					elif isinstance(evt, vdp.usb.UnlinkURBEvent):
						print("Unlink URB Event %d" % evt.id);
		except KeyboardInterrupt:
			pass

		device.detach();
	except vdp.usb.Error, e:
		print("Error: %s" % e);
