set(SRC
    main.c
    vdphci_platform_driver.c
    vdphci_controllers.c
    vdphci_hcd.c
    vdphci_device.c
    vdphci_port.c
    vdphci_direct_io.c
)

set(HDRS
    debug.h
    print.h
    vdphci_platform_driver.h
    vdphci_controllers.h
    vdphci_hcd.h
    vdphci_device.h
    vdphci_port.h
    vdphci_direct_io.h
)

set(VDPHCI_C_FLAGS -Wall -I${VDP_INCLUDE_DIR})

if (VDP_BUILD_TYPE STREQUAL "debug")
    set(VDPHCI_C_FLAGS ${VDPHCI_C_FLAGS} -O -g -DDEBUG)
else ()
    set(VDPHCI_C_FLAGS ${VDPHCI_C_FLAGS} -Os -DNDEBUG)
endif ()

add_linux_kernel_module(vdphci "${SRC}" "${HDRS}" "${VDPHCI_C_FLAGS}")

add_custom_command(
    TARGET vdphci
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/vdphci-load.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/vdphci-unload.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

get_directory_property(CLEAN_FILES ADDITIONAL_MAKE_CLEAN_FILES)

set_directory_properties(
    PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES
    "${CLEAN_FILES};${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdphci-load.sh;${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdphci-unload.sh"
)
