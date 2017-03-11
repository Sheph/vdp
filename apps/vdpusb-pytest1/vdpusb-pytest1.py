import vdp;
import vdp.usb;
import select;

class TestFilter(vdp.usb.Filter):
	def __init__(self):
		vdp.usb.Filter.__init__(self);

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
	except vdp.usb.Error, e:
		print("Error: %s" % e);
