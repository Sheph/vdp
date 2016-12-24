# Add module
macro(add_linux_kernel_module MOD_NAME MOD_SOURCES MOD_HEADERS MOD_EXTRA_C_FLAGS)
    set(MOD_OBJS "")
    set(MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/${MOD_NAME}.o")
    list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/.${MOD_NAME}.o.cmd")
    list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/${MOD_NAME}.mod.c")
    list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/${MOD_NAME}.mod.o")
    list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/.${MOD_NAME}.mod.o.cmd")
    list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/.${MOD_NAME}.ko.cmd")
    list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/Module.symvers")
    list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/modules.order")
    list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/.tmp_versions")
    foreach (MOD_SOURCE_FILE ${MOD_SOURCES})
        get_filename_component(MOD_SOURCE_FILENAME_WE ${MOD_SOURCE_FILE} NAME_WE)
        set(MOD_OBJS ${MOD_OBJS} ${MOD_SOURCE_FILENAME_WE}.o)
        list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/${MOD_SOURCE_FILENAME_WE}.o")
        list(APPEND MOD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/build/.${MOD_SOURCE_FILENAME_WE}.o.cmd")
    endforeach()
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build)
    foreach (MOD_FILE ${MOD_SOURCES} ${MOD_HEADERS})
        get_filename_component(MOD_FILE ${MOD_FILE} ABSOLUTE)
        get_filename_component(MOD_FILENAME ${MOD_FILE} NAME)
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${MOD_FILE} ${CMAKE_CURRENT_BINARY_DIR}/build/${MOD_FILENAME})
    endforeach()
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.kernel ${CMAKE_CURRENT_BINARY_DIR}/build/Makefile)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build/${MOD_NAME}.ko
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/build ${CMAKE_MAKE_PROGRAM} -C "${KERNEL_BUILD_DIR}" M="${CMAKE_CURRENT_BINARY_DIR}/build" MOD_NAME="${MOD_NAME}" MOD_EXTRA_C_FLAGS="${MOD_EXTRA_C_FLAGS}" MOD_OBJS="${MOD_OBJS}" modules
        DEPENDS ${MOD_SOURCES} ${MOD_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.kernel
        IMPLICIT_DEPENDS C ${MOD_SOURCES} ${MOD_HEADERS}
    )
    add_custom_target(
        ${MOD_NAME} ALL
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/build/${MOD_NAME}.ko ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MOD_NAME}.ko
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/build/${MOD_NAME}.ko
    )
    get_directory_property(CLEAN_FILES ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(
        PROPERTIES
        ADDITIONAL_MAKE_CLEAN_FILES
        "${CLEAN_FILES};${MOD_GENERATED};${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MOD_NAME}.ko"
    )
    include_directories("${KERNEL_SOURCE_DIR}/include")
    include_directories("${KERNEL_SOURCE_DIR}/arch/x86/include")
    include_directories("${KERNEL_BUILD_DIR}/include")
    add_definitions(-D__KERNEL__ -DMODULE)
    add_definitions(${MOD_EXTRA_C_FLAGS})
endmacro(add_linux_kernel_module)
