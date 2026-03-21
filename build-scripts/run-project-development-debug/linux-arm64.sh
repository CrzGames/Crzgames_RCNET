#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build/linux/arm64/Debug"
CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
TARGET="${1:-}"

if [[ ! -f "${CACHE_FILE}" ]]; then
  echo "[ERROR] Build directory not generated: ${BUILD_DIR}"
  echo "[INFO ] Run first: ./build-scripts/generate-project/linux-arm64.sh"
  exit 1
fi

echo -e "\e[32m[INFO ] Rebuilding Debug configuration (Linux arm64)...\e[0m"
if [[ -z "${TARGET}" ]]; then
  cmake --build "${BUILD_DIR}" --parallel 8
else
  cmake --build "${BUILD_DIR}" --target "${TARGET}" --parallel 8
fi

echo -e "\e[32m[OK   ] Debug build completed.\e[0m"
