#!/bin/bash
set -euo pipefail

echo -e "\e[32m\nGenerating Unix Makefiles project for Linux arm64...\e[0m"

for build_type in Debug Release; do
  echo -e "\e[32m\nBuilding $build_type...\e[0m"

  # Configure and build the project
  cmake -S . -B "build/linux/arm64/$build_type" \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=$build_type \
    -DRCNET_ARCH=arm64

  cmake --build "build/linux/arm64/$build_type" --parallel 8
done

# Final message
echo -e "\033[32m\nRCNET libraries for Linux arm64 (Debug and Release) generated successfully.\nGo to build/linux/arm64.\n\033[0m"
