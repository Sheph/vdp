set(SRC
    vdp_usb.c
    vdp_usb_context.c
    vdp_usb_context.h
    vdp_usb_device.c
    vdp_usb_device.h
    vdp_usb_urbi.c
    vdp_usb_urbi.h
    vdp_usb_util.c
    vdp_usb_filter.c
    vdp_usb_gadget.c
)

add_library(vdpusb STATIC ${SRC})
target_link_libraries(vdpusb lwl)
