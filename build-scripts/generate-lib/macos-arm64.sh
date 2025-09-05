#!/bin/bash

# Generate lib for macOS arm64 architecture (Debug + Release)

mkdir -p dist/lib/macos/arm64
cd dist/lib/macos/arm64

# Debug build
cmake ../../../.. -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DRCNET_BUILD_EXAMPLE=OFF
cmake --build .

# Release build
cmake ../../../.. -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DRCNET_BUILD_EXAMPLE=OFF
cmake --build .

echo -e "\033[32m\n Generate lib RCNETCore for macOS arm64 (min macOS 15.0) in Release/Debug successfully, go to the dist/lib/macos/arm64 directory...\n\033[0m"
