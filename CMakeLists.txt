cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0014 NEW)

project(VDP)
set(CMAKE_VERBOSE_MAKEFILE ON)

#default config

if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug Release" FORCE)
endif ()

string(TOLOWER "${CMAKE_BUILD_TYPE}" VDP_BUILD_TYPE)

if (NOT KERNEL_VERSION)
    execute_process(COMMAND uname -r OUTPUT_VARIABLE KERNEL_VERSION)
    string(STRIP ${KERNEL_VERSION} KERNEL_VERSION)
endif()

if (NOT KERNEL_SOURCE_DIR)
    set(KERNEL_SOURCE_DIR /lib/modules/${KERNEL_VERSION}/build CACHE STRING "Kernel source dir" FORCE)
endif()

if (NOT KERNEL_BUILD_DIR)
    set(KERNEL_BUILD_DIR /lib/modules/${KERNEL_VERSION}/build CACHE STRING "Kernel build dir" FORCE)
endif()

unset(KERNEL_VERSION)

#platform setup

message(STATUS "Kernel source dir - " ${KERNEL_SOURCE_DIR})
message(STATUS "Kernel build dir - " ${KERNEL_BUILD_DIR})

#testing

enable_testing()

#additional modules

set(CMAKE_MODULE_PATH "${VDP_SOURCE_DIR}/CMake")

include (LinuxKernelMacros)

#out dir

set(VDP_OUT_DIR ${VDP_BINARY_DIR}/out)

#set includes/libs

set(VDP_INCLUDE_DIRECTORIES "${VDP_SOURCE_DIR}/include")
include_directories(${VDP_INCLUDE_DIRECTORIES})

#fix output directories

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${VDP_OUT_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${VDP_OUT_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${VDP_OUT_DIR}/lib)

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -D_REENTRANT")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -DPIC")
endif ()

add_subdirectory(modules)
