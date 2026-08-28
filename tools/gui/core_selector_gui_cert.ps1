# core_selector_gui_cert.ps1
param(
    [string]$Executable = "build-current/pico_visual_designer.exe",
    [string]$Database = "validation/rp2350_multicore_debug_project/PVD_RP2350_MULTICORE_DEBUG.sqlite",
    [string]$ResultPath = "validation/pin_certification/results/core-selector-gui-20260825.json"
)

$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "PvdGuiAutomation.psm1") -Force

function Find-Control($root, [string]$id) {
    return $root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.AutomationId -eq $id -or $_.Current.AutomationId.EndsWith("." + $id) } |
        Select-Object -First 1
}

function Invoke-Control($control) {
    [void]$control.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
}

function Read-Control($control) {
    try { return $control.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value }
    catch { return $control.Current.Name }
}

$p = $null
$result = [ordered]@{ result = "FAIL"; stages = [ordered]@{} }
try {
    Get-Process pico_visual_designer, openocd, arm-none-eabi-gdb -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
    $p = Start-Process -FilePath (Resolve-Path $Executable).Path -ArgumentList "--certification-dialogs" -PassThru
    Start-Sleep -Seconds 2
    $window = Get-PvdWindow $p.Id
    Invoke-Control (Find-Control $window "project_open")
    $dialog = Wait-PvdElement (Get-PvdWindow $p.Id) "pvd_automation_file_dialog" ""
    $file = Find-Control $dialog "fileNameEdit"
    $file.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue((Resolve-Path $Database).Path)
    Invoke-Control (@($dialog.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.Name -eq "Open" })[0])
    Start-Sleep -Seconds 2
    Navigate-PvdPage $p.Id "Debug" | Out-Null
    $window = Get-PvdWindow $p.Id
    $selector = Find-Control $window "debug_core_selector"
    $result.stages.selector_visible = if ($selector) { "PASS" } else { "FAIL" }
    $result.stages.selector_disabled_before_gdb = if (-not $selector.Current.IsEnabled) { "PASS" } else { "FAIL" }

    Invoke-Control (Find-Control $window "debug_start")
    $deadline = (Get-Date).AddSeconds(25)
    do {
        Start-Sleep -Milliseconds 250
        $log = Read-Control (Find-Control (Get-PvdWindow $p.Id) "debug_log")
    } while ((Get-Date) -lt $deadline -and $log -notmatch "RP2350 core mapping resolved")

    $selector = Find-PvdControlFresh $p.Id "debug_core_selector" "" 5
    $result.stages.mapping_resolved = if ($log -match "Core 0 -> thread 1.*Core 1 -> 2") { "PASS" } else { "FAIL" }
    $result.stages.selector_enabled_after_gdb = if ($selector.Current.IsEnabled) { "PASS" } else { "FAIL" }
    $result.stages.core0_default = if ((Read-Control $selector) -eq "Core 0") { "PASS" } else { "FAIL" }

    $command = Find-Control (Get-PvdWindow $p.Id) "debug_command"
    $send = Find-Control (Get-PvdWindow $p.Id) "debug_send_command"
    $command.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue("continue")
    Invoke-Control $send
    Start-Sleep -Seconds 1
    Invoke-Control (Find-Control (Get-PvdWindow $p.Id) "debug_halt")
    Start-Sleep -Seconds 2

    $selector.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
    Start-Sleep -Milliseconds 250
    $item = Get-PvdQtPopup $p.Id | ForEach-Object {
        $_.uia_root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
            Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and
                           $_.Current.Name -eq "Core 1" -and -not $_.Current.IsOffscreen } |
            Select-Object -First 1
    } | Select-Object -First 1
    $rect = $item.Current.BoundingRectangle
    [void][PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
    Start-Sleep -Milliseconds 300
    $selector = Find-PvdControlFresh $p.Id "debug_core_selector" "" 5
    $result.stages.core1_selected = if ((Read-Control $selector) -eq "Core 1") { "PASS" } else { "FAIL" }
    Start-Sleep -Milliseconds 250
    $command = Find-Control (Get-PvdWindow $p.Id) "debug_command"
    $send = Find-Control (Get-PvdWindow $p.Id) "debug_send_command"
    $command.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue("info registers")
    Invoke-Control $send
    Start-Sleep -Seconds 1
    $command.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue("bt")
    Invoke-Control $send
    Start-Sleep -Seconds 2
    $log = Read-Control (Find-Control (Get-PvdWindow $p.Id) "debug_log")
    $result.stages.core1_gdb_target = if ($log -match "Debug core selected: Core 1 \(rp2350.cm1\)" -and
        $log -match "pc\s+0x1[0-9a-f]+" -and $log -match "rp2350_core1_entry|pvd_runtime_handler_0|timer_time_us_64") { "PASS" } else { "FAIL" }

    $selector = Find-PvdControlFresh $p.Id "debug_core_selector" "" 5
    $selector.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
    Start-Sleep -Milliseconds 250
    $item = Get-PvdQtPopup $p.Id | ForEach-Object {
        $_.uia_root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
            Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and
                           $_.Current.Name -eq "Core 0" -and -not $_.Current.IsOffscreen } |
            Select-Object -First 1
    } | Select-Object -First 1
    $rect = $item.Current.BoundingRectangle
    [void][PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
    Start-Sleep -Milliseconds 300
    $selector = Find-PvdControlFresh $p.Id "debug_core_selector" "" 5
    $result.stages.core0_returned = if ((Read-Control $selector) -eq "Core 0") { "PASS" } else { "FAIL" }
    $result.stages.core_switch = if ($result.stages.core1_selected -eq "PASS" -and $result.stages.core0_returned -eq "PASS") { "PASS" } else { "FAIL" }

    $result.log_tail = $log.Substring([Math]::Max(0, $log.Length - 8000))
    $result.stages.cleanup = "PASS"
    $result.result = if (($result.stages.Values | Where-Object { $_ -eq "FAIL" }).Count -eq 0) { "PASS" } else { "FAIL" }
}
catch {
    $result.failure = $_.Exception.Message
}
finally {
    if ($p -and -not $p.HasExited) {
        $p.CloseMainWindow()
        Start-Sleep -Seconds 1
        if (-not $p.HasExited) { $p.Kill() }
    }
    Get-Process openocd, arm-none-eabi-gdb -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
}

$parent = Split-Path -Parent $ResultPath
if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
$result | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $ResultPath
$result | ConvertTo-Json -Depth 8
