@echo off
setlocal
cd /d "%~dp0"
title Best Bar - Run Geometry Dash
color 06
cls
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Run-GD.ps1"
if errorlevel 1 pause
