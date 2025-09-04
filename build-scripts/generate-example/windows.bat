@echo off
setlocal

REM Generate lib and executable for Windows x64 (Debug)

REM 1) mkdir -p dist/project-example/windows/x64
mkdir "dist\project-example\windows\x64" 2>nul

REM 2) cd dist/project-example/windows/x64
cd /d "dist\project-example\windows\x64" || (
  echo [ERREUR] Impossible d'entrer dans dist\project-example\windows\x64
  exit /b 1
)

REM 3) cmake ../../../.. -G "Visual Studio 17 2022" -A x64
cmake ..\..\..\.. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
  echo [ERREUR] Echec de la configuration CMake.
  exit /b 1
)

REM 4) cmake --build . (Debug via --config, car VS est multi-config)
cmake --build . --config Debug
if errorlevel 1 (
  echo [ERREUR] Echec du build Debug.
  exit /b 1
)

echo.
echo [OK] RCNETCore Windows x64 Debug genere avec succes.
echo.

endlocal
