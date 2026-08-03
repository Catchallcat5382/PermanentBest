@echo off
setlocal EnableExtensions
set "GAME_ROOT=F:\SteamLibrary\steamapps\common\Geometry Dash"
set "MODS_DIR=%GAME_ROOT%\geode\mods"
set "ACTIVE=%MODS_DIR%\camila314.pathfinder.geode"
set "DISABLED=%MODS_DIR%\camila314.pathfinder.geode.disabled"
set "UNZIPPED=%GAME_ROOT%\geode\unzipped\camila314.pathfinder"

tasklist /FI "IMAGENAME eq GeometryDash.exe" 2>nul | find /I "GeometryDash.exe" >nul
if not errorlevel 1 (
    echo Close Geometry Dash before running this file.
    pause
    exit /b 1
)

if exist "%ACTIVE%" (
    if exist "%DISABLED%" del /F /Q "%DISABLED%"
    move /Y "%ACTIVE%" "%DISABLED%" >nul
)
if exist "%UNZIPPED%\" rmdir /S /Q "%UNZIPPED%"

echo camila314.pathfinder is disabled. Its package was renamed, not deleted.
pause
