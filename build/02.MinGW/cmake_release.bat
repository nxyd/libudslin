@echo off
cd %~dp0

::set PATH=C:\Program Files\CMake\bin;C:\MinGW\i686-8.1.0-release-posix-dwarf-rt_v6-rev0\mingw32\bin

set OUT_DIR=.\Release
if not exist %OUT_DIR% (
    md %OUT_DIR%
)
cd %OUT_DIR%
cmake ..\..\.. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release