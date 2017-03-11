set(SRC
    vdp/__init__.py
    vdp/usb/vdp_py_usb.c
    vdp/usb/vdp_py_usb_context.c
    vdp/usb/vdp_py_usb_context.h
    vdp/usb/vdp_py_usb_device.c
    vdp/usb/vdp_py_usb_device.h
    vdp/usb/vdp_py_usb_event.c
    vdp/usb/vdp_py_usb_event.h
    vdp/usb/vdp_py_usb_urb.c
    vdp/usb/vdp_py_usb_urb.h
    vdp/usb/vdp_py_usb_error.c
    vdp/usb/vdp_py_usb_error.h
    vdp/usb/vdp_py_usb_filter.c
    vdp/usb/vdp_py_usb_filter.h
)

set(VDP_PY_CFLAGS '-Wall')

if (VDP_BUILD_TYPE STREQUAL "debug")
    set(VDP_PY_CFLAGS ${VDP_PY_CFLAGS},'-O0','-g','-UNDEBUG')
else ()
    set(VDP_PY_CFLAGS ${VDP_PY_CFLAGS},'-DNDEBUG','-O2')
endif ()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in" "${CMAKE_CURRENT_BINARY_DIR}/setup.py")

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp"
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/setup.py" build --build-lib "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/python2.7/site-packages"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp"
    DEPENDS ${SRC} vdpusb)

add_custom_target(vdp_py ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

get_directory_property(CLEAN_FILES ADDITIONAL_MAKE_CLEAN_FILES)
set_directory_properties(
    PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES
    "${CLEAN_FILES};${CMAKE_CURRENT_BINARY_DIR}/build;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/python2.7/site-packages/vdp"
)
