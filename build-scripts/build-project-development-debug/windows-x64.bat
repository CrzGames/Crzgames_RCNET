@echo off
setlocal

set "BUILD_DIR=build\windows\x64"
set "CACHE_FILE=%BUILD_DIR%\CMakeCache.txt"
set "TARGET=%~1"

if not exist "%CACHE_FILE%" (
  echo [ERROR] Build directory not generated: %BUILD_DIR%
  echo [INFO ] Run first: build-scripts\generate-project\windows-x64.bat
  exit /b 1
)

echo [INFO ] Rebuilding Debug configuration (Windows x64)...

if "%TARGET%"=="" (
  echo [INFO ] No target specified, building default development targets...
  call :build_target rcnet
  if errorlevel 1 exit /b 1
  call :build_target rcnet_example_server
  if errorlevel 1 exit /b 1
  call :build_target rcnet_example_client
  if errorlevel 1 exit /b 1
) else (
  call :build_target "%TARGET%"
  if errorlevel 1 exit /b 1
)

echo [OK   ] Debug build completed.
exit /b 0

:build_target
echo [INFO ] Building target %~1 (Debug)...
cmake --build "%BUILD_DIR%" --config Debug --target "%~1" --parallel 8
if errorlevel 1 (
  echo [ERROR] Debug build failed for target: %~1
  exit /b 1
)
exit /b 0
