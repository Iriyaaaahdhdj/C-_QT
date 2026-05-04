@echo off
set "QT_ROOT=C:\Qt\6.8.3\mingw_64"
set "MINGW_ROOT=C:\Qt\Tools\mingw1310_64"
set "CMAKE_ROOT=C:\Program Files\CMake\bin"
set "NINJA_ROOT=C:\Users\Iriya\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe"

set "Qt6_DIR=%QT_ROOT%\lib\cmake\Qt6"
set "CMAKE_PREFIX_PATH=%QT_ROOT%"
set "PATH=%QT_ROOT%\bin;%MINGW_ROOT%\bin;%CMAKE_ROOT%;%NINJA_ROOT%;%PATH%"

echo Qt environment loaded.
echo Qt root: %QT_ROOT%
echo MinGW root: %MINGW_ROOT%
