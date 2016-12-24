#!/bin/bash
DIR_NAME=`basename ${PWD}`
mkdir -p ../${DIR_NAME}_build_i386_debug
cd ../${DIR_NAME}_build_i386_debug
cmake -DCMAKE_BUILD_TYPE=Debug -D_ECLIPSE_VERSION=4.3 \
-DKERNEL_SOURCE_DIR=$HOME/kernel-src/linux-`uname -r | cut -f 1 -d-` \
-DKERNEL_BUILD_DIR=$HOME/kernel-src/linux-`uname -r | cut -f 1 -d-`/debian/build/build-generic \
-G "Eclipse CDT4 - Unix Makefiles" ../${DIR_NAME}
