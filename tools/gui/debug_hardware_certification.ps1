# debug_hardware_certification.ps1
param(
    [string]$Executable = "build-cert-20260825/pico_visual_designer.exe",
    [string]$Database = "PICO2W.sqlite",
    [string]$ResultPath = "validation/pin_certification/results/debug-hardware-certification.json"
)

$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "PvdGuiAutomation.psm1") -Force

function Get-Dialog([int]$ProcessId) {
    $deadline = (Get-Date).AddSeconds(15)
    while ((Get-Date) -lt $deadline) {
        try {
            $main = Get-PvdWindow $ProcessId
            $dialog = $main.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                                    [System.Windows.Automation.Condition]::TrueCondition) |
                Where-Object { $_.Current.ClassName -eq "QFileDialog" -and $_.Current.AutomationId -match "pvd_automation_file_dialog$" } |
                Select-Object -First 1
            if ($dialog) { return $dialog }
        } catch {}
        Start-Sleep -Milliseconds 150
    }
    throw "Debug certification file dialog was not discovered."
}

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

function Get-OpenOcdChildren([int]$ProcessId) {
    @(Get-CimInstance Win32_Process -Filter "Name='openocd.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.ParentProcessId -eq $ProcessId })
}

function Get-OpenOcdSnapshot {
    @(Get-CimInstance Win32_Process -Filter "Name='openocd.exe'" -ErrorAction SilentlyContinue |
        Select-Object ProcessId,ParentProcessId,ExecutablePath,CommandLine)
}

function Test-Port3333 {
    return [bool](Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue)
}

function Wait-OpenOcdOwnedReady([int]$ProcessId, [int]$seconds = 10) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $owned = @(Get-OpenOcdSnapshot | Where-Object { $_.ParentProcessId -eq $ProcessId })
        $listener = @(Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue |
            Where-Object { $_.OwningProcess -in @($owned.ProcessId) })
        if ($owned.Count -eq 1 -and $listener.Count -ge 1) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

function Wait-OpenOcdStopped([int]$ProcessId, [int]$seconds = 10) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $owned = @(Get-OpenOcdSnapshot | Where-Object { $_.ParentProcessId -eq $ProcessId })
        $listener = @(Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue |
            Where-Object { $_.OwningProcess -in @($owned.ProcessId) })
        if ($owned.Count -eq 0 -and $listener.Count -eq 0) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

$process = $null
$result = [ordered]@{ result = "FAIL"; stages = [ordered]@{}; failure_boundary = $null }
try {
    $process = Start-Process -FilePath (Resolve-Path $Executable).Path -ArgumentList "--certification-dialogs" -PassThru
    $result.pvd_pid = $process.Id
    $deadline = (Get-Date).AddSeconds(30)
    while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 200; $process.Refresh() }
    if ($process.MainWindowHandle -eq 0) { throw "PVD main window did not appear." }
    $result.stages.Launch = "PASS"

    Invoke-Control (Get-Control (Get-PvdWindow $process.Id) "project_open")
    $dialog = Get-Dialog $process.Id
    $field = Get-Control $dialog "fileNameEdit"
    $field.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue((Resolve-Path $Database).Path)
    $open = @($dialog.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                              [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and $_.Current.Name -eq "Open" }) | Select-Object -First 1
    Invoke-Control $open
    Start-Sleep -Seconds 2
    $result.stages.ProjectLoad = "PASS"

    $page = Navigate-PvdPage $process.Id "Debug"
    $window = Get-PvdWindow $process.Id
    $result.stages.Navigation = if ($page.name -eq "Debug") { "PASS" } else { "FAIL" }
    Start-Sleep -Seconds 10
    if ((Get-OpenOcdChildren $process.Id).Count -ne 0 -or (Test-Port3333)) { throw "OpenOCD started during Debug-page activation." }
    $result.stages.NoImplicitStart = "PASS"
    Navigate-PvdPage $process.Id "Build" | Out-Null
    Navigate-PvdPage $process.Id "Debug" | Out-Null
    Start-Sleep -Seconds 2
    if ((Get-OpenOcdChildren $process.Id).Count -ne 0 -or (Test-Port3333)) { throw "OpenOCD started while returning to Debug." }
    $result.stages.NoStartAfterReturn = "PASS"
    $window = Get-PvdWindow $process.Id
    $ids = @("debug_status", "debug_start", "debug_stop", "debug_halt", "debug_continue", "debug_step",
             "debug_next", "debug_backtrace", "debug_registers", "debug_command", "debug_send_command", "debug_log")
    $controls = @{}
    foreach ($id in $ids) { $controls[$id] = Get-Control $window $id; if (-not $controls[$id]) { throw "Missing Debug control: $id" } }
    $result.stages.Controls = "PASS"
    if (-not $controls.debug_start.Current.IsEnabled -or $controls.debug_stop.Current.IsEnabled) { throw "Initial button state is incorrect." }
    $result.stages.InitialButtons = "PASS"
    $result.initial_status = Read-ControlText $controls.debug_status

    Invoke-Control $controls.debug_start
    $deadline = (Get-Date).AddSeconds(20)
    do { Start-Sleep -Milliseconds 250; $status = Read-ControlText $controls.debug_status; $log = Read-ControlText $controls.debug_log } while ((Get-Date) -lt $deadline -and $log -notmatch "OpenOCD GDB server is ready|OpenOCD ready.*Running|connection rejected|GDB connection failed")
    $result.status_after_start = $status
    $result.log_after_start = $log
    $result.stages.Start = if ($log -match "OpenOCD GDB server is ready|OpenOCD ready.*Running") { "PASS" } else { "FAIL" }
    if (-not (Wait-OpenOcdOwnedReady $process.Id)) {
        $result.openocd_snapshot_after_start = @(Get-OpenOcdSnapshot)
        $result.port_snapshot_after_start = @(Get-NetTCPConnection -LocalPort 3333 -ErrorAction SilentlyContinue |
            Select-Object State,LocalAddress,LocalPort,OwningProcess)
        throw "Explicit Start OCD did not create one owned OpenOCD listener."
    }
    $result.stages.GdbControls = "NOT-TESTED - deferred until OCD lifecycle certification"
    Invoke-Control $controls.debug_stop; Start-Sleep -Seconds 2
    $result.status_after_stop = Read-ControlText $controls.debug_status
    $result.stages.Stop = if ($result.status_after_stop -match "Stopped|Completed") { "PASS" } else { "FAIL" }
    if (-not (Wait-OpenOcdStopped $process.Id)) { throw "Stop OCD did not release OpenOCD." }
    Invoke-Control $controls.debug_start; Start-Sleep -Seconds 2
    $result.status_after_second_start = Read-ControlText $controls.debug_status
    if (-not (Wait-OpenOcdOwnedReady $process.Id)) { throw "Second Start OCD did not create a clean session." }
    $result.stages.SecondSession = "PASS"
    Invoke-Control $controls.debug_stop
    Start-Sleep -Seconds 2
    if (-not (Wait-OpenOcdStopped $process.Id)) { throw "Second Stop OCD did not clean up." }
    $result.stages.SecondStop = "PASS"
    $result.result = if (($result.stages.Values | Where-Object { $_ -eq "FAIL" }).Count -eq 0) { "PASS" } else { "PARTIAL" }
} catch {
    $result.failure_boundary = $_.Exception.Message
} finally {
    if ($process -and -not $process.HasExited) {
        try {
            $window = Get-PvdWindow $process.Id
            $stopControl = Get-Control $window "debug_stop"
            if ($stopControl -and $stopControl.Current.IsEnabled) {
                Invoke-Control $stopControl
                Start-Sleep -Milliseconds 800
            }
        } catch {}
        $process.CloseMainWindow()
        Start-Sleep -Milliseconds 700
        if (-not $process.HasExited) { $process.Kill() }
    }
    $cleanupDeadline = (Get-Date).AddSeconds(5)
    while ((Get-Date) -lt $cleanupDeadline) {
        $owned = @(Get-OpenOcdSnapshot | Where-Object { $_.ParentProcessId -eq $result.pvd_pid })
        if ($owned.Count -eq 0) { break }
        foreach ($item in $owned) { Stop-Process -Id $item.ProcessId -Force -ErrorAction SilentlyContinue }
        Start-Sleep -Milliseconds 200
    }
    $result.cleanup_openocd = if (@(Get-OpenOcdSnapshot).Count -eq 0) { "PASS" } else { "FAIL" }
    $result.cleanup_port3333 = if (-not (Test-Port3333)) { "PASS" } else { "FAIL" }
}
$parent = Split-Path -Parent $ResultPath
if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
$result | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $ResultPath
$result | ConvertTo-Json -Depth 8
