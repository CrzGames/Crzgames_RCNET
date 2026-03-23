#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BIN_DIR="${REPO_ROOT}/agones-sdk-server-binaries"
BIN_PATH="${BIN_DIR}/sdk-server.linux.amd64"

if [[ ! -f "${BIN_PATH}" ]]; then
  echo "[ERROR] Binaire introuvable: ${BIN_PATH}"
  exit 1
fi

if [[ ! -x "${BIN_PATH}" ]]; then
  chmod +x "${BIN_PATH}"
fi

# Changer ces valeurs si besoin
GAMESERVER_NAME="${GAMESERVER_NAME:-my-local-game-server}"
POD_NAMESPACE="${POD_NAMESPACE:-staging-aetherroyale-agones-gameservers}"
KUBECONFIG_PATH="${KUBECONFIG_PATH:-$HOME/.kube/ovh.yaml}"
ADDRESS="${ADDRESS:-0.0.0.0}"
GRACEFUL_TERMINATION="${GRACEFUL_TERMINATION:-false}"

echo "[INFO ] Launching Agones SDK server from: ${BIN_PATH}"
exec "${BIN_PATH}" \
  --gameserver-name "${GAMESERVER_NAME}" \
  --pod-namespace "${POD_NAMESPACE}" \
  --kubeconfig "${KUBECONFIG_PATH}" \
  --address "${ADDRESS}" \
  --graceful-termination="${GRACEFUL_TERMINATION}"
