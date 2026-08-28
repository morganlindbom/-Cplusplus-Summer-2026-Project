# process_lifecycle_selftest.ps1
param(
    [string]$Executable = "build-current/pico_visual_designer.exe",
    [string]$Database = "PICO2W.sqlite",
    [string]$ResultPath = "validation/pin_certification/results/process-lifecycle-selftest.json"
)

$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "PvdGuiAutomation.psm1") -Force

function Get-Control($Root, [string]$AutomationId) {
    return $Root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.AutomationId -eq $AutomationId -or $_.Current.AutomationId.EndsWith("." + $AutomationId) } |
        Select-Object -First 1
}

function Invoke-Control($Control) {
    [void]$Control.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
}

function Read-ControlText($Control) {
    try { return $Control.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value }
    catch { return $Control.Current.Name }
}

function Get-PvdChildren([int]$ParentProcessId) {
    @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
        Where-Object { $_.ParentProcessId -eq $ParentProcessId -and $_.Name -in @("openocd.exe", "arm-none-eabi-gdb.exe", "conhost.exe") })
}

function Get-OpenOcd {
    @(Get-CimInstance Win32_Process -Filter "Name='openocd.exe'" -ErrorAction SilentlyContinue)
}

function Test-Port3333 {
    [bool](Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue)
}

function Wait-Until([scriptblock]$Condition, [int]$Seconds = 10) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $deadline) {
        if (& $Condition) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

$process = $null
$result = [ordered]@{ result = "FAIL"; stages = [ordered]@{} }
try {
    foreach ($name in @("pico_visual_designer", "openocd", "arm-none-eabi-gdb")) {
        Get-Process $name -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    }
    Start-Sleep -Milliseconds 700
    $result.initial = [ordered]@{
        pvd = [bool](Get-Process pico_visual_designer -ErrorAction SilentlyContinue)
        openocd = [bool](Get-Process openocd -ErrorAction SilentlyContinue)
        gdb = [bool](Get-Process arm-none-eabi-gdb -ErrorAction SilentlyContinue)
        port3333 = Test-Port3333
    }

    $exe = (Resolve-Path $Executable).Path
    $result.executable = $exe
    $item = Get-Item $exe
    $result.timestamp = $item.LastWriteTimeUtc.ToString("o")
    $result.size = $item.Length
    $process = Start-Process -FilePath $exe -ArgumentList "--certification-dialogs" -PassThru
    $result.pvd_pid = $process.Id
    $result.stages.Launch = "PASS"
    Start-Sleep -Seconds 2

    $window = Get-PvdWindow $process.Id
    Invoke-Control (Get-Control $window "project_open")
    $dialog = Wait-PvdElement (Get-PvdWindow $process.Id) "pvd_automation_file_dialog" ""
    $field = Get-Control $dialog "fileNameEdit"
    $field.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue((Resolve-Path $Database).Path)
    $open = @($dialog.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and $_.Current.Name -eq "Open" }) | Select-Object -First 1
    Invoke-Control $open
    Start-Sleep -Seconds 2
    $result.stages.ProjectLoad = "PASS"

    Navigate-PvdPage $process.Id "Debug" | Out-Null
    Start-Sleep -Seconds 10
    $result.stages.DebugPagePassivity = if ((Get-OpenOcd).Count -eq 0 -and -not (Test-Port3333)) { "PASS" } else { "FAIL" }
    Navigate-PvdPage $process.Id "Build" | Out-Null
    Navigate-PvdPage $process.Id "Debug" | Out-Null
    Start-Sleep -Seconds 3
    $result.stages.ReturnPassivity = if ((Get-OpenOcd).Count -eq 0 -and -not (Test-Port3333)) { "PASS" } else { "FAIL" }

    $window = Get-PvdWindow $process.Id
    $controls = @{}
    foreach ($id in @("debug_status", "debug_start", "debug_stop", "debug_log")) { $controls[$id] = Get-Control $window $id }
    $result.initial_buttons = [ordered]@{ start = $controls.debug_start.Current.IsEnabled; stop = $controls.debug_stop.Current.IsEnabled }
    Invoke-Control $controls.debug_start
    $failureSeen = Wait-Until { (Read-ControlText $controls.debug_log) -match "unable to find|OpenOCD exited|OpenOCD stopped unexpectedly" -or (Get-OpenOcd).Count -eq 0 } 15
    Start-Sleep -Seconds 2
    $result.failure_log = Read-ControlText $controls.debug_log
    $result.failure_status = Read-ControlText $controls.debug_status
    $result.failure_children = @(Get-PvdChildren $process.Id)
    $result.stages.NoHardwareFailure = if ($failureSeen -and (Get-OpenOcd).Count -eq 0 -and -not (Test-Port3333)) { "PASS" } else { "FAIL" }
    $result.failure_buttons = [ordered]@{ start = $controls.debug_start.Current.IsEnabled; stop = $controls.debug_stop.Current.IsEnabled }
    $result.orphan_conhost = @($result.failure_children | Where-Object { $_.Name -eq "conhost.exe" }).Count
    $result.stages.ConsoleSuppression = if ($result.orphan_conhost -eq 0) { "PASS" } else { "FAIL" }
    $result.hardware = "NOT TESTED - HARDWARE NOT PRESENT OR CMSIS-DAP FAILURE PATH"
    $result.stages.HardwareStartStop = "NOT TESTED"
    $result.stages.Restart = "NOT TESTED"
    $result.stages.ApplicationExitCleanup = "NOT TESTED"
    $result.result = if (($result.stages.Values | Where-Object { $_ -eq "FAIL" }).Count -eq 0) { "PARTIAL" } else { "FAIL" }
} catch {
    $result.failure_boundary = $_.Exception.Message
} finally {
    if ($process -and -not $process.HasExited) {
        try { $process.CloseMainWindow(); Start-Sleep -Milliseconds 700 } catch {}
        if (-not $process.HasExited) { $process.Kill() }
    }
}
$parent = Split-Path -Parent $ResultPath
if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
$result | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $ResultPath
$result | ConvertTo-Json -Depth 8
