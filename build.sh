#!/usr/bin/env bash
set -e

BUILD_DIR="build"
EXECUTABLE_NAME="raycast-thesis"

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$BUILD_DIR" -j$(sysctl -n hw.logicalcpu 2>/dev/null || nproc)

cd "$BUILD_DIR"
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    ./"$EXECUTABLE_NAME.exe"
else
    ./"$EXECUTABLE_NAME"
fi