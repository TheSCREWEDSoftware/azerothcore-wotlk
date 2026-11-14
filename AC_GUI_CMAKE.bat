@echo off
rem Opens cmake-gui on this source tree (port/tc6x, 6.2.4 (21742)) with its build folder.
rem Tools (map/vmap/mmap extractors, dbimport) are on by default (TOOLS_BUILD=all).
set "SRC=%~dp0"
if "%SRC:~-1%"=="\" set "SRC=%SRC:~0,-1%"
start "" cmake-gui -S "%SRC%" -B "F:\TC to AC\Build\6x"
