@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM Usage:
REM   windows-launch-100-clients.bat [count] [path_to_exe]
REM Examples:
REM   windows-launch-100-clients.bat
REM   windows-launch-100-clients.bat 50
REM   windows-launch-100-clients.bat 100 "C:\path\to\rcnet_example_client.exe"

set "COUNT=100"
if not "%~1"=="" set "COUNT=%~1"

set "EXE=%~dp0..\..\build\windows\x64\Debug\rcnet_example_client.exe"
if not "%~2"=="" set "EXE=%~2"

if not exist "%EXE%" (
    echo [ERROR] Client executable not found: "%EXE%"
    exit /b 1
)

set /a LAST=COUNT-1
if %LAST% LSS 0 (
    echo [ERROR] Invalid count: %COUNT%
    exit /b 1
)

echo Launching %COUNT% client instances...
echo Executable: "%EXE%"
echo.

for /L %%I in (0,1,%LAST%) do (
    start "rcnet_client_%%I" "%EXE%" --account-index %%I
)

echo Done.
exit /b 0
