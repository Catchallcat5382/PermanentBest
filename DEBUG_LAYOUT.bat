@echo off
setlocal
cd /d "%~dp0"
title Best Bar - Layout Debug
color 0E
cls
echo ============================================================
echo  BEST BAR - LIVE LAYOUT DEBUG
ECHO ============================================================
echo.
echo Drag the gold progress bar in-game. The diamond moves with it.
echo The position saves automatically and remains after debug closes.
echo Close Geometry Dash to turn debug mode off.
echo.
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Run-Layout-Debug.ps1"
if errorlevel 1 pause
