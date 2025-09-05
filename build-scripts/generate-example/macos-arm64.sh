#!/bin/bash
# Generate lib and executable for macOS arm64 (Debug) with Xcode

set -e

# 1) mkdir -p dist/project-example/macos/arm64
mkdir -p dist/project-example/macos/arm64

# 2) cd dist/project-example/macos/arm64
cd dist/project-example/macos/arm64

# 3) cmake ../../../.. -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake ../../../.. -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0

# 4) cmake --build . --config Debug
cmake --build . --config Debug

echo -e "\033[32m\n[OK] RCNETCore macOS arm64 Debug generated successfully, go to dist/project-example/macos/arm64\n\033[0m"
