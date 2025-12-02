#!/usr/bin/env bash
set -e

BUILD_DIR="build"

if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
    cmake -S . -B "$BUILD_DIR" -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
else
    cmake -S . -B "$BUILD_DIR"
fi

cmake --build "$BUILD_DIR" -j"$( (nproc 2>/dev/null) || (sysctl -n hw.logicalcpu 2>/dev/null) )"