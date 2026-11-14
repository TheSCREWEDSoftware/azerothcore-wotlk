@echo off
rem Opens cmake-gui on this source tree (port/sf548, 5.4.8 (18414)) with the build folder next to it.
rem Tools (map/vmap/mmap extractors, dbimport) are on by default (TOOLS_BUILD=all).
set "SRC=%~dp0"
if "%SRC:~-1%"=="\" set "SRC=%SRC:~0,-1%"
start "" cmake-gui -S "%SRC%" -B "%SRC%\..\Build-sf548"
