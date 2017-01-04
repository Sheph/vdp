set(SRC
    main.c
)

add_executable(vdpusb-proxy ${SRC})
target_link_libraries(vdpusb-proxy vdpusb usb-1.0 udev ${CMAKE_THREAD_LIBS_INIT})
