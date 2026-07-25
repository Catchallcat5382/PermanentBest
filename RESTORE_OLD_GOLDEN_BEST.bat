@echo off
setlocal
set "MODS=F:\SteamLibrary\steamapps\common\Geometry Dash\geode\mods"
if exist "%MODS%\firee.goldenbest.geode.disabled" move /Y "%MODS%\firee.goldenbest.geode.disabled" "%MODS%\firee.goldenbest.geode" >nul
echo Old Golden Best has been restored.
pause
