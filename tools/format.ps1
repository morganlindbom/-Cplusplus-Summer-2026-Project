param(
    [ValidateSet("gui", "system", "generator", "all")]
    [string]$Group = "all",
    [switch]$Check
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$groups = @{
    gui = @(
        "src/systems/mainwindow/MainWindow.cpp",
        "src/systems/mainwindow/workflow/Workflow.cpp",
        "src/systems/mainwindow/function_selection/function_selection_column3/FunctionSelectionColumn3.cpp",
        "src/systems/mainwindow/project/project_column3/ProjectColumn3.cpp",
        "src/systems/mainwindow/dialog/AutomationFileDialogService.cpp",
        "src/systems/mainwindow/dialog/NativeFileDialogService.cpp",
        "src/systems/mainwindow/pio/pio_column3/PioColumn3.cpp",
        "src/systems/mainwindow/code/code_column3/CodeColumn3.cpp",
        "src/systems/mainwindow/function_selection/function_selection_column2/FunctionSelectionColumn2.cpp",
        "src/systems/mainwindow/generate/generate_column3/GenerateColumn3.cpp",
        "src/systems/mainwindow/build/build_column3/BuildColumn3.cpp",
        "src/systems/mainwindow/transfer/transfer_column3/TransferColumn3.cpp",
        "src/systems/mainwindow/debug/debug_column3/DebugColumn3.cpp",
        "src/systems/mainwindow/settings/settings_column3/SettingsColumn3.cpp"
    )
    system = @("src/systems/System.cpp")
    generator = @("src/systems/generation/ProjectGenerator.cpp")
}
$files = if ($Group -eq "all") { @($groups.gui + $groups.system + $groups.generator) } else { $groups[$Group] }

$formatter = $null
if (Test-Path "C:\msys64\ucrt64\bin\clang-format.exe") {
    $formatter = "C:\msys64\ucrt64\bin\clang-format.exe"
}
if (-not $formatter) {
    $formatter = (Get-Command clang-format -ErrorAction SilentlyContinue).Source
}
if (-not $formatter) {
    $scripts = (& python -c "import sysconfig; print(sysconfig.get_path('scripts', 'nt_user'))").Trim()
    $candidate = Join-Path $scripts "clang-format.exe"
    if (Test-Path $candidate) { $formatter = $candidate }
}
if (-not $formatter) { throw "clang-format was not found. Install the pinned project tool with: python -m pip install --user clang-format==18.1.8" }

Push-Location $root
try {
    & $formatter --version
    $arguments = @("--style=file")
    if ($Check) { $arguments += "--dry-run", "--Werror" } else { $arguments += "-i" }
    $action = if ($Check) { "Checking " } else { "Formatting " }
    foreach ($file in $files) {
        Write-Host ($action + $file)
        & $formatter @arguments $file
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
} finally {
    Pop-Location
}
