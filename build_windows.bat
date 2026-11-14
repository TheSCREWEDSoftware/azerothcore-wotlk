@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem  port/tc6x - 6.2.4 (21742) client
rem  Configures and builds this source tree (worldserver, authserver/bnetserver,
rem  scripts and all tools: map_extractor, vmap4_extractor, vmap4_assembler,
rem  mmaps_generator, dbimport), then extracts the client data if CLIENT is set.
rem  Run it from anywhere; the source is the folder this file is in.
rem ===========================================================================

rem --- settings -------------------------------------------------------------
set "SRC=%~dp0"
if "%SRC:~-1%"=="\" set "SRC=%SRC:~0,-1%"
rem Build folder
set "BUILD=F:\TC to AC\Build\6x"
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
set "CLIENT_LABEL=6.2.4 (21742)"
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

echo.
echo Build done. Map extraction for this client (CASC storage) is not available yet:
echo the extractors on this branch still read MPQ archives. Use extracted data from a
echo TrinityCore %CLIENT_LABEL% install meanwhile.
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
