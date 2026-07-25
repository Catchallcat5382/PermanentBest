@echo off
setlocal
set "MODS=F:\SteamLibrary\steamapps\common\Geometry Dash\geode\mods"
set "UNZIPPED=F:\SteamLibrary\steamapps\common\Geometry Dash\geode\unzipped\firee.goldenbest"
if exist "%MODS%\firee.goldenbest.geode" move /Y "%MODS%\firee.goldenbest.geode" "%MODS%\firee.goldenbest.geode.disabled" >nul
if exist "%UNZIPPED%\" rmdir /S /Q "%UNZIPPED%"
echo Old Golden Best is disabled. Nothing was deleted.
pause
