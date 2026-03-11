#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PY_SCRIPT="${SCRIPT_DIR}/generate_server_ed25519_keys.py"

if [[ ! -f "${PY_SCRIPT}" ]]; then
  echo "Error: Python script not found: ${PY_SCRIPT}" >&2
  exit 1
fi

if command -v python3 >/dev/null 2>&1; then
  python3 "${PY_SCRIPT}"
elif command -v python >/dev/null 2>&1; then
  python "${PY_SCRIPT}"
else
  echo "Error: python/python3 not found in PATH. Install Python 3 and retry." >&2
  exit 1
fi
