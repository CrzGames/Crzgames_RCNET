#!/bin/bash
set -euo pipefail

echo -e "\e[32m\nGenerating Unix Makefiles project for Linux x64...\e[0m"

for build_type in Debug Release; do
  echo -e "\e[32m\nBuilding $build_type...\e[0m"

  # Configure and build the project
  cmake -S . -B "build/linux/x64/$build_type" \
    -G "Unix Makefiles" \
    -DRCNET_ARCH="x64" \
    -DCMAKE_BUILD_TYPE=$build_type

  cmake --build "build/linux/x64/$build_type" --parallel 8
done

# Final message
echo -e "\033[32m\nRCNET libraries for Linux x64 (Debug and Release) generated successfully.\nGo to build/linux/x64.\n\033[0m"