@echo off
setlocal
cd /d "%~dp0"
title Best Bar - Publish Release
color 06
cls
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Release.ps1"
if errorlevel 1 pause
