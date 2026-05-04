$qtRoot = "C:\Qt\6.8.3\mingw_64"
$mingwRoot = "C:\Qt\Tools\mingw1310_64"
$cmakeRoot = "C:\Program Files\CMake\bin"
$ninjaRoot = "C:\Users\Iriya\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe"

$env:Qt6_DIR = Join-Path $qtRoot "lib\cmake\Qt6"
$env:CMAKE_PREFIX_PATH = $qtRoot
$env:Path = "$($qtRoot)\bin;$($mingwRoot)\bin;$cmakeRoot;$ninjaRoot;$env:Path"

Write-Host "Qt environment loaded."
Write-Host "Qt root: $qtRoot"
Write-Host "MinGW root: $mingwRoot"
