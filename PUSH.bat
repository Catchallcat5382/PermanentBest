@echo off
setlocal
cd /d "%~dp0"
title Best Bar - Push and Install
color 06
cls
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Push.ps1"
if errorlevel 1 pause
