param([string[]]$Files)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$tidy = (Get-Command clang-tidy -ErrorAction SilentlyContinue).Source
if (-not $tidy -and (Test-Path "C:\msys64\ucrt64\bin\clang-tidy.exe")) {
    $tidy = "C:\msys64\ucrt64\bin\clang-tidy.exe"
}
if (-not $tidy) {
    Write-Error "clang-tidy is not installed; lint cannot run. Formatting remains available through tools/format.ps1."
    exit 2
}
if (-not $Files -or $Files.Count -eq 0) {
    $Files = @("src/systems/System.cpp", "src/systems/generation/ProjectGenerator.cpp")
}
Push-Location $root
try {
    foreach ($file in $Files) {
        & $tidy -p build-fresh $file
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
} finally { Pop-Location }
