@echo off
cd  %~dp0

set OUT_DIR="./VS2017_Prj"
if not exist %OUT_DIR% (
    md %OUT_DIR%
)
cd %OUT_DIR%
cmake ..\..\..  -G"Visual Studio 15 2017"