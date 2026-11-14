@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem  port/sf548 - 5.4.8 (18414) client
rem  Configures and builds this source tree (worldserver, authserver/bnetserver,
rem  scripts and all tools: map_extractor, vmap4_extractor, vmap4_assembler,
rem  mmaps_generator, dbimport), then extracts the client data if CLIENT is set.
rem  Run it from anywhere; the source is the folder this file is in.
rem ===========================================================================

rem --- settings -------------------------------------------------------------
set "SRC=%~dp0"
if "%SRC:~-1%"=="\" set "SRC=%SRC:~0,-1%"
rem Build folder
set "BUILD=F:\TC to AC\Build\548"
rem RelWithDebInfo, Release or Debug
set "CONFIG=RelWithDebInfo"
rem Client folder (the one with the Wow executable and Data\). Empty = build only.
set "CLIENT="
rem 1 = also generate mmaps (slow), 0 = dbc/maps/vmaps only
set "MMAPS=1"
rem 1 = open cmake-gui on this source/build pair instead of building here
set "GUI=0"
rem Extra cmake options, e.g. -DBOOST_ROOT=C:/local/boost_1_87_0 -DOPENSSL_ROOT_DIR=...
set "EXTRA_CMAKE="
set "CLIENT_LABEL=5.4.8 (18414)"
rem ---------------------------------------------------------------------------

if "!GUI!"=="1" (
    start "" cmake-gui -S "!SRC!" -B "!BUILD!"
    goto :done
)

echo === configure ===
cmake -S "!SRC!" -B "!BUILD!" -DTOOLS_BUILD=all -DSCRIPTS=static -DMODULES=static !EXTRA_CMAKE!
if errorlevel 1 goto :fail

echo.
echo === build (!CONFIG!) ===
cmake --build "!BUILD!" --config !CONFIG! --parallel
if errorlevel 1 goto :fail

set "BIN=!BUILD!\bin\!CONFIG!"
if not exist "!BIN!\worldserver.exe" set "BIN=!BUILD!\bin"

if "!CLIENT!"=="" (
    echo.
    echo Build done. Set CLIENT at the top of this file to also extract dbc/maps/vmaps/mmaps.
    goto :done
)
if not exist "!CLIENT!\Data" (
    echo CLIENT "!CLIENT!" has no Data folder.
    goto :fail
)

set "DATA=!BUILD!\data"
if not exist "!DATA!" mkdir "!DATA!"
pushd "!DATA!"

echo.
echo === dbc, maps and cameras ===
"!BIN!\map_extractor.exe" -i "!CLIENT!" -o "!DATA!"
if errorlevel 1 goto :failpop

echo.
echo === vmaps ===
"!BIN!\vmap4_extractor.exe" -d "!CLIENT!\Data"
if errorlevel 1 goto :failpop
if not exist vmaps mkdir vmaps
"!BIN!\vmap4_assembler.exe" Buildings vmaps
if errorlevel 1 goto :failpop
rmdir /s /q Buildings

if "!MMAPS!"=="1" (
    echo.
    echo === mmaps - this can take hours ===
    if not exist mmaps mkdir mmaps
    copy /y "!BIN!\mmaps-config.yaml" . >nul 2>&1
    "!BIN!\mmaps_generator.exe"
    if errorlevel 1 goto :failpop
)
popd

echo.
echo Extracted data is in "!DATA!". Set DataDir in worldserver.conf to that folder.
goto :done

:failpop
popd
:fail
echo.
echo FAILED - see the output above.
endlocal
pause
exit /b 1

:done
endlocal
pause
exit /b 0
