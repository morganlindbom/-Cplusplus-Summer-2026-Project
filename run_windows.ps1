# run_windows.ps1
param(
    [string]$QtPrefix = "",
    [string]$Generator = "Ninja"
)

$ErrorActionPreference = "Stop"
$cmakeArgs = @("-S", ".", "-B", "build", "-G", $Generator)
if ($QtPrefix -ne "") {
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$QtPrefix"
}
cmake @cmakeArgs
cmake --build build
& .\build\pico_visual_designer.exe
