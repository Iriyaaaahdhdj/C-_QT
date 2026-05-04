@echo off
set "QT_ROOT=C:\Qt\6.8.3\mingw_64"
set "MINGW_ROOT=C:\Qt\Tools\mingw1310_64"
set "NINJA_EXE=C:\Users\Iriya\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe\ninja.exe"
set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"

"%CMAKE_EXE%" -S . -B build -G Ninja ^
  -D CMAKE_MAKE_PROGRAM=%NINJA_EXE% ^
  -D CMAKE_PREFIX_PATH=%QT_ROOT% ^
  -D CMAKE_C_COMPILER=%MINGW_ROOT%\bin\gcc.exe ^
  -D CMAKE_CXX_COMPILER=%MINGW_ROOT%\bin\g++.exe
if errorlevel 1 exit /b %errorlevel%

"%CMAKE_EXE%" --build build --parallel
exit /b %errorlevel%
