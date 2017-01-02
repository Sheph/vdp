Virtual Device Platform (https://github.com/Sheph/vdp)

=========================

1. About
-------------------

The goal of this project is to create a crossplatform (at least linux and windows)
framework for implementing various types of devices using various programming
languages. e.g. one can create a fully functional USB camera using
only python scripts that can take data from files, some other device, etc.
or, one can create a USB printer device that's actually located elsewhere
and is available over network (similar to how usbip project does this, but
in our case usbip functionality is just one use case). In general, you just
use one simple crossplatform API, create virtual devices and make them do
whatever you want, easily.

2. Status
-------------------

Just starting. Now it's only USB with C API on linux, but there're plans to
also implement PCI, HMDI/VGA and other types of devices. Windows porting
and other languages (python, lua, etc.) support is also planned. There're also
plans to make more sample apps (for now it's just a simple USB HID mouse), usbip-like
sample will be one of them.

3. Building and running
-------------------

1. run scripts/CMake/cmake_i386_linux_xxx.sh from this directory, set
-DKERNEL_SOURCE_DIR and -DKERNEL_BUILD_DIR to desired linux kernel sources or remove
those lines if you want to use current Ubuntu kernel.

2. run out/bin/vdphci-load.sh, it'll insert virtual USB host controller driver into your system.

3. run ./vdpusb-mouse1 1, it'll insert virtual USB mouse into your host controller, your system will
be able to recognize and use that mouse.

4. you can modify test_process_int_urb in apps/vdpusb-mouse1/main.c, play around with X and Y
values, e.g. setting both to 0x01, 0x00 will make mouse pointer slowly move down and right.
