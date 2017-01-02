set(SRC
    main.c
)

add_executable(vdpusb-mouse1 ${SRC})
target_link_libraries(vdpusb-mouse1 vdpusb)
