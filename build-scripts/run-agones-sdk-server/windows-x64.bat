@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..\..") do set "REPO_ROOT=%%~fI"

set "BIN_DIR=%REPO_ROOT%\agones-sdk-server-binaries"

if not exist "%BIN_DIR%\sdk-server.windows.amd64.exe" (
  echo [ERROR] sdk-server.windows.amd64.exe introuvable.
  echo [INFO ] Chemin attendu:
  echo        - %BIN_DIR%
  exit /b 1
)

set "GAMESERVER_NAME=my-local-game-server"
set "POD_NAMESPACE=staging-aetherroyale-agones-gameservers"
set "KUBECONFIG_PATH=C:\Users\Corentin\.kube\ovh.yaml"
set "ADDRESS=0.0.0.0"
set "GRACEFUL_TERMINATION=false"

echo [INFO ] Launching Agones SDK server from: %BIN_DIR%
"%BIN_DIR%\sdk-server.windows.amd64.exe" ^
  --gameserver-name "%GAMESERVER_NAME%" ^
  --pod-namespace "%POD_NAMESPACE%" ^
  --kubeconfig "%KUBECONFIG_PATH%" ^
  --address "%ADDRESS%" ^
  --graceful-termination=%GRACEFUL_TERMINATION%
