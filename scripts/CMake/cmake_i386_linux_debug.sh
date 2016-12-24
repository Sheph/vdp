#!/bin/bash
DIR_NAME=`basename ${PWD}`
mkdir -p ../${DIR_NAME}_build_i386_debug
cd ../${DIR_NAME}_build_i386_debug
cmake -DCMAKE_BUILD_TYPE=Debug -D_ECLIPSE_VERSION=4.3 -G "Eclipse CDT4 - Unix Makefiles" ../${DIR_NAME}
