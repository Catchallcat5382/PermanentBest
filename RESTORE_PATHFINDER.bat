@echo off
setlocal EnableExtensions
set "GAME_ROOT=F:\SteamLibrary\steamapps\common\Geometry Dash"
set "MODS_DIR=%GAME_ROOT%\geode\mods"
set "ACTIVE=%MODS_DIR%\camila314.pathfinder.geode"
set "DISABLED=%MODS_DIR%\camila314.pathfinder.geode.disabled"

tasklist /FI "IMAGENAME eq GeometryDash.exe" 2>nul | find /I "GeometryDash.exe" >nul
if not errorlevel 1 (
    echo Close Geometry Dash before running this file.
    pause
    exit /b 1
)

if not exist "%DISABLED%" (
    echo No disabled Pathfinder package was found.
    pause
    exit /b 0
)
if exist "%ACTIVE%" del /F /Q "%ACTIVE%"
move /Y "%DISABLED%" "%ACTIVE%" >nul

echo camila314.pathfinder was restored. It may crash again on your current setup.
pause
