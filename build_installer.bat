@echo off
setlocal
cd /d "%~dp0"

where cl >nul 2>nul
if errorlevel 1 (
  echo ERROR: run this script from "Developer Command Prompt for VS 2022".
  exit /b 1
)

set "ISCC="
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if "%ISCC%"=="" (
  echo ERROR: Inno Setup 6 not found. Install it with: winget install JRSoftware.InnoSetup
  exit /b 1
)

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

cmake --build build
if errorlevel 1 exit /b 1

"%ISCC%" installer\Revora.iss
if errorlevel 1 exit /b 1

echo.
echo DONE. Installer: %~dp0installer\Output
