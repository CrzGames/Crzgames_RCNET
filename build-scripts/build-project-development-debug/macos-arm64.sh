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
if [[ -n "${TARGET}" ]]; then
  targets=("${TARGET}")
else
  targets=("rcnet" "rcnet_example_server" "rcnet_example_client")
  echo -e "\e[32m[INFO ] No target specified, building default development targets...\e[0m"
fi

for t in "${targets[@]}"; do
  echo -e "\e[32m[INFO ] Building target ${t} (Debug)...\e[0m"
  cmake --build "${BUILD_DIR}" \
    --config Debug \
    --target "${t}" \
    --parallel 8 \
    -- \
    CODE_SIGNING_ALLOWED=NO \
    CODE_SIGNING_REQUIRED=NO
done

echo -e "\e[32m[OK   ] Debug build completed.\e[0m"
