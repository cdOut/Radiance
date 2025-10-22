#!/usr/bin/env bash
set -e

BUILD_DIR="build"
EXECUTABLE_NAME="app"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

echo "Configuring with CMake..."
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug

echo "Building project..."
cmake --build "$BUILD_DIR" -j$(sysctl -n hw.logicalcpu 2>/dev/null || nproc)

echo "Running $EXECUTABLE_NAME..."
cd "$BUILD_DIR"
./"$EXECUTABLE_NAME"