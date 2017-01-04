set(SOURCES
    core.c
    descriptor.c
    hotplug.c
    io.c
    strerror.c
    sync.c
    os/linux_netlink.c
    os/linux_udev.c
    os/linux_usbfs.c
    os/poll_posix.c
    os/threads_posix.c
)

include_directories(.)

add_library(usb-1.0 STATIC ${SOURCES})
