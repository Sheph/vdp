set(SRC
    main.c
)

add_executable(vdpusb-mouse2 ${SRC})
target_link_libraries(vdpusb-mouse2 vdpusb)
