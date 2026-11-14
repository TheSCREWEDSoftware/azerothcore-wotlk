@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem  port/tc434 - 4.3.4 (15595) client: extract dbc, maps, vmaps and mmaps
rem  Uses the tools built from this branch (AC_GUI_CMAKE.bat + Visual Studio)
rem  in F:\TC to AC\Build\434.
rem ===========================================================================

rem --- settings -------------------------------------------------------------
rem Client folder (the one with the Wow executable and Data\)
set "CLIENT="
rem Build folder used by AC_GUI_CMAKE.bat
set "BUILD=F:\TC to AC\Build\434"
rem Preferred build configuration of the tools (others are tried if missing)
set "CONFIG=RelWithDebInfo"
rem Output folder (point DataDir in worldserver.conf here)
set "DATA=F:\TC to AC\Build\434\data"
rem 1 = also generate mmaps (slow), 0 = dbc/maps/vmaps only
set "MMAPS=1"
set "CLIENT_LABEL=4.3.4 (15595)"
rem ---------------------------------------------------------------------------

if "!CLIENT!"=="" (
    echo Set CLIENT at the top of this file to your !CLIENT_LABEL! client folder.
    goto :fail
)
if not exist "!CLIENT!\Data" (
    echo CLIENT "!CLIENT!" has no Data folder.
    goto :fail
)

set "BIN="
for %%C in (!CONFIG! RelWithDebInfo Release Debug) do (
    if not defined BIN if exist "!BUILD!\bin\%%C\map_extractor.exe" set "BIN=!BUILD!\bin\%%C"
)
if not defined BIN (
    echo map_extractor.exe not found under "!BUILD!\bin". Build the solution in Visual Studio first.
    goto :fail
)
echo Tools: "!BIN!"

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
