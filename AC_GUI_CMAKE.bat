@echo off
rem Opens cmake-gui on this source tree (port/tc434, 4.3.4 (15595)) with the build folder next to it.
rem Tools (map/vmap/mmap extractors, dbimport) are on by default (TOOLS_BUILD=all).
set "SRC=%~dp0"
if "%SRC:~-1%"=="\" set "SRC=%SRC:~0,-1%"
start "" cmake-gui -S "%SRC%" -B "%SRC%\..\Build-tc434"
