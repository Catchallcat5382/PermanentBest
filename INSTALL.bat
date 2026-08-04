@echo off
setlocal EnableExtensions

title Best Bar - Install Existing Package v2.0.0-beta.7
color 06
cls

echo ============================================================
echo  BEST BAR - INSTALL EXISTING PACKAGE v2.0.0-beta.7
echo ============================================================
echo.
echo Installs the newest already-built Best Bar .geode package.
echo This does not rebuild, push, release, or change source code.
echo.

for %%I in ("%~dp0.") do set "SOURCE_DIR=%%~fI"

if not exist "%SOURCE_DIR%\scripts\Install.ps1" goto missing_script

powershell -NoProfile -ExecutionPolicy Bypass -File "%SOURCE_DIR%\scripts\Install.ps1"
if errorlevel 1 goto failed

color 0A
echo.
echo Installation finished successfully.
echo.
pause
exit /b 0

:missing_script
color 0C
echo ERROR: scripts\Install.ps1 is missing.
goto failed_message

:failed
color 0C

:failed_message
echo.
echo Installation stopped. Read the error above.
echo.
pause
exit /b 1
