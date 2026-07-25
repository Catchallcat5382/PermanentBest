@echo off
setlocal EnableExtensions EnableDelayedExpansion

title Permanent Best - Build and Install v1.1.2
color 06
cls

echo ============================================================
echo  PERMANENT BEST - BUILD AND INSTALL v1.1.2
echo ============================================================
echo.
echo Builds, packages, verifies, and installs Permanent Best.
echo The old firee.goldenbest mod is safely renamed to .disabled
echo to prevent both mods from editing the same progress label.
echo Nothing is deleted.
echo.

for %%I in ("%~dp0.") do set "SOURCE_DIR=%%~fI"
set "BUILD_ROOT=%LOCALAPPDATA%\PermanentBestBuild\build-v112"
set "PACKAGE_STAGE=%BUILD_ROOT%\manual-package"

set "GAME_ROOT=F:\SteamLibrary\steamapps\common\Geometry Dash"
set "GAME_EXE=%GAME_ROOT%\GeometryDash.exe"

set "NINJA_EXE=%LOCALAPPDATA%\Microsoft\WinGet\Links\ninja.exe"
set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
set "CLANG_EXE=C:\Program Files\LLVM\bin\clang.exe"
set "CLANGXX_EXE=C:\Program Files\LLVM\bin\clang++.exe"
set "GIT_BIN=C:\Program Files\Git\cmd"
set "GEODE_SDK=%LOCALAPPDATA%\GeodeSDK\Geode"
set "CPM_SOURCE_CACHE=%LOCALAPPDATA%\GeodeSDK\cpm-cache"

set "MOD_ID=catchallcat5382.permanent-best"
set "TARGET_NAME=PermanentBest"
set "BUILT_MOD=%BUILD_ROOT%\%MOD_ID%.geode"
set "LATEST_DIR=%SOURCE_DIR%\releases\latest"
set "LATEST_MOD=%LATEST_DIR%\PermanentBest.geode"
set "MODS_DIR=%GAME_ROOT%\geode\mods"
set "UNZIPPED_DIR=%GAME_ROOT%\geode\unzipped\%MOD_ID%"
set "INSTALLED_MOD=%MODS_DIR%\%MOD_ID%.geode"
set "OLD_GOLDEN_MOD=%MODS_DIR%\firee.goldenbest.geode"
set "OLD_GOLDEN_DISABLED=%MODS_DIR%\firee.goldenbest.geode.disabled"
set "OLD_GOLDEN_UNZIPPED=%GAME_ROOT%\geode\unzipped\firee.goldenbest"

if not exist "%SOURCE_DIR%\CMakeLists.txt" goto project_missing
if not exist "%SOURCE_DIR%\mod.json" goto modjson_missing
if not exist "%SOURCE_DIR%\src\" goto source_missing
powershell -NoProfile -ExecutionPolicy Bypass -File "%SOURCE_DIR%\scripts\Normalize-Metadata.ps1"
if errorlevel 1 goto metadata_failed
if not exist "%GAME_EXE%" goto game_missing
if not exist "%NINJA_EXE%" goto missing_ninja
if not exist "%CMAKE_EXE%" goto missing_cmake
if not exist "%CLANG_EXE%" goto missing_clang
if not exist "%CLANGXX_EXE%" goto missing_clang
if not exist "%GIT_BIN%\git.exe" goto missing_git
if not exist "%GEODE_SDK%\VERSION" goto missing_sdk

set "GIT_DIR="
set "GIT_WORK_TREE="
set "GIT_INDEX_FILE="
set "GIT_OBJECT_DIRECTORY="
set "GIT_ALTERNATE_OBJECT_DIRECTORIES="
set "SOURCE_GIT=%SOURCE_DIR:\=/%"
set "GIT_CONFIG_COUNT=1"
set "GIT_CONFIG_KEY_0=safe.directory"
set "GIT_CONFIG_VALUE_0=%SOURCE_GIT%"
set "PATH=C:\Program Files\CMake\bin;C:\Program Files\LLVM\bin;%GIT_BIN%;%PATH%"

if not exist "%CPM_SOURCE_CACHE%" mkdir "%CPM_SOURCE_CACHE%"
if not exist "%BUILD_ROOT%" mkdir "%BUILD_ROOT%"

tasklist /FI "IMAGENAME eq GeometryDash.exe" 2>nul | find /I "GeometryDash.exe" >nul
if not errorlevel 1 goto game_open

echo Using Geode SDK: %GEODE_SDK%
echo Source: %SOURCE_DIR%
echo Build cache: %BUILD_ROOT%
echo.

if exist "%BUILD_ROOT%\CMakeCache.txt" del /F /Q "%BUILD_ROOT%\CMakeCache.txt"
if exist "%BUILD_ROOT%\CMakeFiles\" rmdir /S /Q "%BUILD_ROOT%\CMakeFiles"

"%CMAKE_EXE%" ^
  -S "%SOURCE_DIR%" ^
  -B "%BUILD_ROOT%" ^
  -G Ninja ^
  "-DCMAKE_BUILD_TYPE=RelWithDebInfo" ^
  "-DCMAKE_C_COMPILER=%CLANG_EXE%" ^
  "-DCMAKE_CXX_COMPILER=%CLANGXX_EXE%" ^
  "-DCMAKE_MAKE_PROGRAM=%NINJA_EXE%" ^
  "-DGEODE_CLI:FILEPATH=%NINJA_EXE%" ^
  "-DGEODE_DISABLE_CLI_CALLS=ON" ^
  "-DCMAKE_SUPPRESS_REGENERATION=ON"
if errorlevel 1 goto configure_failed

"%CMAKE_EXE%" --build "%BUILD_ROOT%" --target %TARGET_NAME% --parallel
if errorlevel 1 goto build_failed

set "BUILT_DLL="
for /r "%BUILD_ROOT%" %%D in (%TARGET_NAME%.dll) do if not defined BUILT_DLL set "BUILT_DLL=%%~fD"
for /r "%BUILD_ROOT%" %%D in (lib%TARGET_NAME%.dll) do if not defined BUILT_DLL set "BUILT_DLL=%%~fD"
if not defined BUILT_DLL goto dll_missing

echo Compiled DLL: !BUILT_DLL!

if exist "%PACKAGE_STAGE%\" rmdir /S /Q "%PACKAGE_STAGE%"
mkdir "%PACKAGE_STAGE%"
copy /Y "%SOURCE_DIR%\mod.json" "%PACKAGE_STAGE%\mod.json" >nul
copy /Y "!BUILT_DLL!" "%PACKAGE_STAGE%\%MOD_ID%.dll" >nul
for %%F in (logo.png about.md changelog.md support.md) do if exist "%SOURCE_DIR%\%%F" copy /Y "%SOURCE_DIR%\%%F" "%PACKAGE_STAGE%\%%F" >nul
if exist "%SOURCE_DIR%\resources\" (
    mkdir "%PACKAGE_STAGE%\resources\%MOD_ID%" >nul 2>&1
    xcopy "%SOURCE_DIR%\resources\*" "%PACKAGE_STAGE%\resources\%MOD_ID%\" /E /I /Y /Q >nul
)

if exist "%BUILT_MOD%" del /F /Q "%BUILT_MOD%"
pushd "%PACKAGE_STAGE%"
"%CMAKE_EXE%" -E tar cf "%BUILT_MOD%" --format=zip .
set "PACKAGE_RESULT=!ERRORLEVEL!"
popd
if not "!PACKAGE_RESULT!"=="0" goto package_failed
if not exist "%BUILT_MOD%" goto package_failed

"%CMAKE_EXE%" -E tar tf "%BUILT_MOD%" >"%BUILD_ROOT%\package-contents.txt"
if errorlevel 1 goto package_verify_failed
find /I "mod.json" < "%BUILD_ROOT%\package-contents.txt" >nul || goto package_verify_failed
find /I "%MOD_ID%.dll" < "%BUILD_ROOT%\package-contents.txt" >nul || goto package_verify_failed

if not exist "%LATEST_DIR%" mkdir "%LATEST_DIR%"
del /F /Q "%LATEST_DIR%\*" >nul 2>&1
copy /Y "%BUILT_MOD%" "%LATEST_MOD%" >nul || goto latest_copy_failed

if not exist "%MODS_DIR%" mkdir "%MODS_DIR%"
if exist "%OLD_GOLDEN_MOD%" (
    echo Disabling old Golden Best package to prevent conflicts...
    if exist "%OLD_GOLDEN_DISABLED%" del /F /Q "%OLD_GOLDEN_DISABLED%"
    move /Y "%OLD_GOLDEN_MOD%" "%OLD_GOLDEN_DISABLED%" >nul || goto old_mod_disable_failed
)
if exist "%OLD_GOLDEN_UNZIPPED%\" rmdir /S /Q "%OLD_GOLDEN_UNZIPPED%"

if exist "%INSTALLED_MOD%" del /F /Q "%INSTALLED_MOD%"
if exist "%UNZIPPED_DIR%\" rmdir /S /Q "%UNZIPPED_DIR%"
copy /Y "%BUILT_MOD%" "%INSTALLED_MOD%" >nul || goto install_failed

for %%A in ("%BUILT_MOD%") do set "BUILT_SIZE=%%~zA"
for %%A in ("%INSTALLED_MOD%") do set "INSTALLED_SIZE=%%~zA"
if not "!BUILT_SIZE!"=="!INSTALLED_SIZE!" goto install_failed

color 0A
echo.
echo ============================================================
echo  PERMANENT BEST INSTALLED SUCCESSFULLY
echo ============================================================
echo Installed: %INSTALLED_MOD%
echo Package:   %LATEST_MOD%
echo Size:      !INSTALLED_SIZE! bytes
echo.
echo Open Geode ^> Permanent Best ^> Settings to customize it.
echo.
pause
exit /b 0

:project_missing
echo ERROR: CMakeLists.txt is missing.& goto fail
:modjson_missing
echo ERROR: mod.json is missing.& goto fail
:source_missing
echo ERROR: src folder is missing.& goto fail
:metadata_failed
echo ERROR: mod.json validation or UTF-8 normalization failed.& goto fail
:game_missing
echo ERROR: GeometryDash.exe was not found at %GAME_EXE%.& goto fail
:missing_ninja
echo ERROR: Ninja was not found at %NINJA_EXE%.& goto fail
:missing_cmake
echo ERROR: CMake was not found at %CMAKE_EXE%.& goto fail
:missing_clang
echo ERROR: LLVM Clang was not found.& goto fail
:missing_git
echo ERROR: Git was not found at %GIT_BIN%.& goto fail
:missing_sdk
echo ERROR: Geode SDK v5.8.2 was not found at %GEODE_SDK%.& goto fail
:game_open
echo ERROR: Close Geometry Dash before building.& goto fail
:configure_failed
echo ERROR: CMake configuration failed.& goto fail
:build_failed
echo ERROR: C++ compilation failed.& goto fail
:dll_missing
echo ERROR: The compiled DLL could not be located.& goto fail
:package_failed
echo ERROR: The .geode package could not be created.& goto fail
:package_verify_failed
echo ERROR: The .geode package failed verification.& goto fail
:latest_copy_failed
echo ERROR: Could not update releases\latest.& goto fail
:old_mod_disable_failed
echo ERROR: Could not safely disable firee.goldenbest.& goto fail
:install_failed
echo ERROR: Could not install or verify the package.& goto fail

:fail
color 0C
echo.
echo Build/install stopped. Read the error above.
echo.
pause
exit /b 1
