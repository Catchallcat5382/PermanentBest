@echo off
setlocal
cd /d "%~dp0"
title Permanent Best - Create GitHub and Release v1.1.1
color 06
cls

echo ============================================================
echo  PERMANENT BEST - CREATE GITHUB AND RELEASE ONCE
echo ============================================================
echo.
echo This creates Catchallcat5382/PermanentBest, pushes the full
echo source, waits for GitHub Actions, publishes v1.1.1, and can
echo install the resulting .geode file.
echo.
echo You can delete this BAT after it succeeds.
echo.

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Create-GitHub-And-Release.ps1"
set "RESULT=%ERRORLEVEL%"

if not "%RESULT%"=="0" (
    color 0C
    echo.
    echo GitHub setup stopped. Read the error above.
    echo.
    pause
    exit /b %RESULT%
)

color 0A
echo.
echo GitHub repository and release setup finished successfully.
echo You may now delete CREATE_GITHUB_AND_RELEASE_ONCE.bat.
echo.
pause
exit /b 0
