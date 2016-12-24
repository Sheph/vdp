#!/bin/bash
DIR_NAME=`basename ${PWD}`
mkdir -p ../${DIR_NAME}_build_i386_release
cd ../${DIR_NAME}_build_i386_release
cmake -DCMAKE_BUILD_TYPE=Release -D_ECLIPSE_VERSION=4.3 -G "Eclipse CDT4 - Unix Makefiles" ../${DIR_NAME}
