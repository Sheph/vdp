set(SRC
    main.c
)

add_executable(vdpusb-test ${SRC})
target_link_libraries(vdpusb-test vdpusb)
