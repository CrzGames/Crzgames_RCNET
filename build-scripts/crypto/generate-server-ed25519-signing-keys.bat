@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PY_SCRIPT=%SCRIPT_DIR%generate_server_ed25519_keys.py"

if not exist "%PY_SCRIPT%" (
  echo Error: Python script not found: %PY_SCRIPT%
  exit /b 1
)

where python >nul 2>&1
if %errorlevel%==0 (
  python "%PY_SCRIPT%"
  exit /b %errorlevel%
)

where py >nul 2>&1
if %errorlevel%==0 (
  py -3 "%PY_SCRIPT%"
  exit /b %errorlevel%
)

echo Error: Python was not found in PATH. Install Python 3 and retry.
exit /b 1
