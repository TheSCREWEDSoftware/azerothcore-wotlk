@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem  port/tc6x - 6.2.4 (21742) client: extract dbc, maps, vmaps and mmaps
rem  Uses the tools built from this branch (AC_GUI_CMAKE.bat + Visual Studio)
rem  in F:\TC to AC\Build\6x.
rem ===========================================================================

rem --- settings -------------------------------------------------------------
rem Client folder (the one with the Wow executable and Data\)
set "CLIENT="
rem Build folder used by AC_GUI_CMAKE.bat
set "BUILD=F:\TC to AC\Build\6x"
rem Preferred build configuration of the tools (others are tried if missing)
set "CONFIG=RelWithDebInfo"
rem Output folder (point DataDir in worldserver.conf here)
set "DATA=F:\TC to AC\Build\6x\data"
rem 1 = also generate mmaps (slow), 0 = dbc/maps/vmaps only
set "MMAPS=1"
set "CLIENT_LABEL=6.2.4 (21742)"
rem ---------------------------------------------------------------------------

echo Map extraction for the !CLIENT_LABEL! client (CASC storage) is not available on this
echo branch yet: its extractors still read MPQ archives.
goto :done

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
