#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./macos-launch-clients.sh [count] [path_to_exe_or_app] [start_index]
#
# Examples:
#   ./macos-launch-clients.sh
#   ./macos-launch-clients.sh 200
#   ./macos-launch-clients.sh 400 "/path/to/rcnet_example_client"
#   ./macos-launch-clients.sh 400 "/path/to/rcnet_example_client.app" 1000

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

COUNT="${1:-100}"
EXE="${2:-${REPO_ROOT}/build/macos/arm64/Debug/rcnet_example_client}"
START_INDEX="${3:-0}"

if ! [[ "${COUNT}" =~ ^[0-9]+$ ]]; then
  echo "[ERROR] Invalid count: ${COUNT}"
  exit 1
fi

if ! [[ "${START_INDEX}" =~ ^[0-9]+$ ]]; then
  echo "[ERROR] Invalid start_index: ${START_INDEX}"
  exit 1
fi

if [[ "${COUNT}" -le 0 ]]; then
  echo "[ERROR] Count must be > 0"
  exit 1
fi

if [[ ! -e "${EXE}" ]]; then
  echo "[ERROR] Client executable/app not found: ${EXE}"
  exit 1
fi

LAST_INDEX=$((START_INDEX + COUNT - 1))

echo "Launching ${COUNT} client instances..."
echo "Target: ${EXE}"
echo "Account index range: ${START_INDEX}..${LAST_INDEX}"
echo

for ((i = 0; i < COUNT; ++i)); do
  idx=$((START_INDEX + i))

  if [[ "${EXE}" == *.app ]]; then
    open -n "${EXE}" --args --account-index "${idx}"
  else
    "${EXE}" --account-index "${idx}" >/dev/null 2>&1 &
  fi
done

echo "Done."
