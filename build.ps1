& "C:\Program Files\CMake\bin\cmake.exe" -S . -B build -G Ninja `
  -D CMAKE_MAKE_PROGRAM=C:/Users/Iriya/AppData/Local/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe/ninja.exe `
  -D CMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64 `
  -D CMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe `
  -D CMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& "C:\Program Files\CMake\bin\cmake.exe" --build build --parallel
exit $LASTEXITCODE
