@echo off
setlocal

REM === Generate lib for Windows x64 (Debug + Release) ===

REM 1) mkdir -p dist/lib/windows/x64
mkdir "dist\lib\windows\x64" 2>nul

REM 2) cd dist/lib/windows/x64
cd /d "dist\lib\windows\x64" || (
  echo [ERREUR] Impossible d'entrer dans dist\lib\windows\x64
  exit /b 1
)

REM 3) Configuration + build Debug
cmake ..\..\..\.. -G "Visual Studio 17 2022" -A x64 -DRCNET_BUILD_EXAMPLE=OFF
if errorlevel 1 (
  echo [ERREUR] Echec de la configuration Debug.
  exit /b 1
)

cmake --build . --config Debug
if errorlevel 1 (
  echo [ERREUR] Echec du build Debug.
  exit /b 1
)

REM 4) Configuration + build Release
cmake ..\..\..\.. -G "Visual Studio 17 2022" -A x64 -DRCNET_BUILD_EXAMPLE=OFF
if errorlevel 1 (
  echo [ERREUR] Echec de la configuration Release.
  exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
  echo [ERREUR] Echec du build Release.
  exit /b 1
)

echo.
echo [OK] Lib RCNETCore Windows x64 Debug/Release generee avec succes.
echo Dossier: dist\lib\windows\x64
echo.

endlocal
