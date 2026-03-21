#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build/macos/arm64"
CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
TARGET="${1:-}"

if [[ ! -f "${CACHE_FILE}" ]]; then
  echo "[ERROR] Build directory not generated: ${BUILD_DIR}"
  echo "[INFO ] Run first: ./build-scripts/generate-project/macos-arm64.sh"
  exit 1
fi

echo -e "\e[32m[INFO ] Rebuilding Debug configuration (macOS arm64)...\e[0m"
if [[ -z "${TARGET}" ]]; then
  cmake --build "${BUILD_DIR}" \
    --config Debug \
    --parallel 8 \
    -- \
    CODE_SIGNING_ALLOWED=NO \
    CODE_SIGNING_REQUIRED=NO
else
  cmake --build "${BUILD_DIR}" \
    --config Debug \
    --target "${TARGET}" \
    --parallel 8 \
    -- \
    CODE_SIGNING_ALLOWED=NO \
    CODE_SIGNING_REQUIRED=NO
fi

echo -e "\e[32m[OK   ] Debug build completed.\e[0m"
