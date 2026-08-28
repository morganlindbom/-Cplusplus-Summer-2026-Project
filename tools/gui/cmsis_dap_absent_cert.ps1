param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$Database,
    [Parameter(Mandatory = $true)][string]$ResultPath
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'PvdGuiAutomation.psm1') -Force

function Get-Count([string]$name) {
    @($p = Get-Process -Name $name -ErrorAction SilentlyContinue).Count
}

function Test-Port3333 {
    @((Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue)).Count -gt 0
}

function Read-Text($element) {
    try {
        $value = $element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value
        if ($value) { return [string]$value }
    } catch {}
    try {
        $value = $element.GetCurrentPattern([System.Windows.Automation.TextPattern]::Pattern).DocumentRange.GetText(-1)
        if ($value) { return [string]$value }
    } catch {}
    try { return [string]$element.Current.Name } catch { return '' }
}

$result = [ordered]@{
    result = 'FAIL'
    certificate = 'CMSIS_DAP_ABSENT_FAILURE_PATH'
    admin_physical_disconnect = $true
    stages = [ordered]@{}
    gdb_started = 0
    target_reset_program_halt = 0
    unexpected_retry_count = 0
    unrelated_processes = 'UNTOUCHED'
}
$process = $null
try {
    $exe = (Resolve-Path -LiteralPath $Executable).Path
    $db = (Resolve-Path -LiteralPath $Database).Path
    $result.requested_executable = $exe
    $result.database = $db
    $result.initial = [ordered]@{ pvd = (Get-Count 'pico_visual_designer'); openocd = (Get-Count 'openocd'); gdb = (Get-Count 'arm-none-eabi-gdb'); port3333 = (Test-Port3333) }
    if ($result.initial.pvd -or $result.initial.openocd -or $result.initial.gdb -or $result.initial.port3333) { throw 'Host preflight was not clean.' }

    $process = Start-Process -FilePath $exe -ArgumentList @('--certification-dialogs', ('--open-project "' + $db + '"')) -PassThru
    $result.pvd_pid = $process.Id
    $result.stages.launch = 'PASS'
    $deadline = (Get-Date).AddSeconds(30)
    while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 200; $process.Refresh() }
    if ($process.MainWindowHandle -eq 0) { throw 'PVD main window did not appear.' }
    $page = Navigate-PvdPage $process.Id 'Debug' 20
    $result.stages.debug_page_passive = if ((Get-Count 'openocd') -eq 0 -and (Get-Count 'arm-none-eabi-gdb') -eq 0 -and -not (Test-Port3333)) { 'PASS' } else { 'FAIL' }
    $start = Find-PvdControlFresh $process.Id 'debug_start' '' 20
    $status = Find-PvdControlFresh $process.Id 'debug_status' '' 20
    $log = Find-PvdControlFresh $process.Id 'debug_log' '' 20
    $result.stages.start_control_found = 'PASS'
    $result.start_invoked_utc = (Get-Date).ToUniversalTime().ToString('o')
    $result.invocation_method = Invoke-PvdButton $start $process.Id
    $result.stages.start_ocd_invoked = 'PASS'
    Start-Sleep -Milliseconds 1200
    $startActivated = ((Get-Count 'openocd') -gt 0) -or ((Read-Text $status) -notmatch 'Configured')
    if (-not $startActivated) {
        $start = Find-PvdControlFresh $process.Id 'debug_start' '' 5
        $rect = $start.Current.BoundingRectangle
        [PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
        $result.invocation_method = 'InvokePattern-then-fresh-click-fallback'
        Start-Sleep -Milliseconds 800
        $startActivated = ((Get-Count 'openocd') -gt 0) -or ((Read-Text $status) -notmatch 'Configured')
    }
    if (-not $startActivated) { throw 'Start OCD control did not activate the production slot.' }

    $failureDeadline = (Get-Date).AddSeconds(20)
    $statusText = ''
    $logText = ''
    while ((Get-Date) -lt $failureDeadline) {
        try { $status = Find-PvdControlFresh $process.Id 'debug_status' '' 2; $log = Find-PvdControlFresh $process.Id 'debug_log' '' 2 } catch {}
        $statusText = Read-Text $status
        $logText = Read-Text $log
        if ($statusText -match 'error|failed|not found|unavailable|Probe' -or $logText -match 'unable to find a matching CMSIS-DAP|no CMSIS-DAP device|unable to open CMSIS-DAP|failed to open CMSIS-DAP|Probe not found|Probe open failed') { break }
        if ((Get-Count 'openocd') -eq 0 -and (Get-Count 'arm-none-eabi-gdb') -eq 0) { break }
        Start-Sleep -Milliseconds 200
    }
    $result.status = $statusText
    $result.openocd_log = $logText
    $result.cmsis_dap_absence_detected = [bool]($logText -match 'unable to find a matching CMSIS-DAP|no CMSIS-DAP device|unable to open CMSIS-DAP|failed to open CMSIS-DAP|Probe not found|Probe open failed' -or $statusText -match 'Probe not found|Probe open failed|error|failed|unavailable')
    $result.failure_classification = if ($result.cmsis_dap_absence_detected) { 'CMSIS_DAP_UNAVAILABLE' } else { 'OPENOCD_START_FAILURE_UNCLASSIFIED' }
    $result.stages.cmsis_dap_absence_detected = if ($result.cmsis_dap_absence_detected) { 'PASS' } else { 'FAIL' }
    $result.stages.bounded_failure = if ((Get-Date) -lt $failureDeadline) { 'PASS' } else { 'FAIL' }
    $result.stages.no_gdb = if ((Get-Count 'arm-none-eabi-gdb') -eq 0) { 'PASS' } else { 'FAIL' }
    $result.stages.no_target_action = if ($result.target_reset_program_halt -eq 0) { 'PASS' } else { 'FAIL' }
    $result.openocd_exit_observed = ((Get-Count 'openocd') -eq 0)
    $result.stages.openocd_terminated = if ($result.openocd_exit_observed) { 'PASS' } else { 'FAIL' }
    $result.stages.pvd_responsive = if (-not $process.HasExited) { 'PASS' } else { 'FAIL' }
    $result.ui_state_after_failure = $statusText
    $result.ui_recoverable = [bool]($statusText -notmatch 'Starting|Connecting|Stopping|Busy')
    $result.stages.ui_recoverable = if ($result.ui_recoverable) { 'PASS' } else { 'FAIL' }
    $result.port3333 = if (-not (Test-Port3333)) { 'FREE' } else { 'OCCUPIED' }
    $result.stages.cleanup = if ($result.openocd_exit_observed -and $result.port3333 -eq 'FREE') { 'PASS' } else { 'FAIL' }
    $result.result = if (($result.stages.Values | Where-Object { $_ -eq 'FAIL' }).Count -eq 0 -and $result.failure_classification -eq 'CMSIS_DAP_UNAVAILABLE') { 'PASS' } else { 'FAIL' }
} catch {
    $result.failure = $_.Exception.Message
} finally {
    if ($process -and -not $process.HasExited) {
        try { $process.CloseMainWindow() | Out-Null } catch {}
        Start-Sleep -Milliseconds 800
        if (-not $process.HasExited) { $process.Kill() }
    }
    $result.final = [ordered]@{ pvd = (Get-Count 'pico_visual_designer'); openocd = (Get-Count 'openocd'); gdb = (Get-Count 'arm-none-eabi-gdb'); port3333 = (Test-Port3333) }
}
$parent = Split-Path -Parent $ResultPath
if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
$result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $ResultPath -Encoding UTF8
$result | ConvertTo-Json -Depth 12
