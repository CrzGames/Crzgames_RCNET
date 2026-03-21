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
  cmake --build "%BUILD_DIR%" --config Debug --parallel 8
) else (
  cmake --build "%BUILD_DIR%" --config Debug --target "%TARGET%" --parallel 8
)

if errorlevel 1 (
  echo [ERROR] Debug build failed.
  exit /b 1
)

echo [OK   ] Debug build completed.
