param(
    [string]$Database = 'C:\Users\morga\AppData\Local\Temp\pvd-pwm-first-100hz-50-clean-20260827T005101711Z\PWM_FIRST_100HZ_50_RETRY2.sqlite',
    [string]$Uf2 = 'C:\Users\morga\AppData\Local\Temp\pvd-pwm-first-100hz-50-clean-20260827T005101711Z\build\PWM_FIRST_100HZ_50_RETRY2.uf2',
    [string]$Executable = 'build-current/pico_visual_designer.exe'
)
$ErrorActionPreference = 'Stop'
$root = (Get-Location).Path
$runnerText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'gui_sio_physical_runner.ps1') -Raw
$prefix = $runnerText.Substring(0, $runnerText.IndexOf('function Run-Test')).Replace('$PSScriptRoot', "(Join-Path (Get-Location) 'tools')")
Invoke-Expression $prefix

$runId = 'post-program-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$resultPath = Join-Path $root "validation/pin_certification/results/$runId.json"
$logPath = [IO.Path]::ChangeExtension($resultPath, '.log')
$observer = @{ ObservedGpio = 1; ObserverMode = 'TIMING'; ExpectedLevel = 0; MaxSamples = 16; MaxTransitions = 12; ObservationDurationMs = 5000; ExpectedIntervalMs = 5; ToleranceMs = 1; SymbolPrefix = 'observer_pwm0a_100hz_50_clean_gpio1'; DiagnosticTimer = $true; ArtifactIdentityPass = $true }
$pvd = $null
$started = [DateTime]::UtcNow
try {
    if (-not (Test-Path -LiteralPath $Database -PathType Leaf)) { throw "Database missing: $Database" }
    if (-not (Test-Path -LiteralPath $Uf2 -PathType Leaf)) { throw "UF2 missing: $Uf2" }
    if (@(Get-Process openocd,arm-none-eabi-gdb,pico_visual_designer -ErrorAction SilentlyContinue).Count -ne 0) { throw 'PVD/debugger process preflight failed' }
    if (@(Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue).Count -ne 0) { throw 'Port 3333 preflight failed' }
    $pvd = Start-Process -FilePath (Join-Path $root $Executable) -ArgumentList '--certification-dialogs' -PassThru
    $windowDeadline = (Get-Date).AddSeconds(30)
    while ($pvd.MainWindowHandle -eq 0 -and (Get-Date) -lt $windowDeadline) { Start-Sleep -Milliseconds 250; $pvd.Refresh() }
    if ($pvd.MainWindowHandle -eq 0) { throw 'PVD main window did not appear' }
    $null = Open-ExistingProject $pvd.Id $Database
    Start-Sleep -Milliseconds 700
    $target = Invoke-PvdObserverTargetRun -ProcessId $pvd.Id -Uf2 $Uf2 -Observer $observer -MeasurementWaitMs 500
    $final = [ordered]@{ run_id = $runId; status = if ($target.Observer.status) { $target.Observer.status } else { 'POST_PROGRAM_DIAGNOSTIC_COMPLETED' }; transfer = $target.Transfer; measurement = $target.MeasurementPhase; observer = $target.Observer; raw_text = $target.ObserverRawText; no_pwm_certification = $true; completed_utc = [DateTime]::UtcNow.ToString('o'); elapsed_ms = [int](([DateTime]::UtcNow - $started).TotalMilliseconds) }
    Write-PvdAtomicJson -Path $resultPath -Value $final
    $final | ConvertTo-Json -Depth 30
}
catch {
    Write-PvdAtomicJson -Path $resultPath -Value ([ordered]@{ run_id = $runId; status = 'POST_PROGRAM_DIAGNOSTIC_FAILED'; failure_reason = $_.Exception.Message; no_pwm_certification = $true; elapsed_ms = [int](([DateTime]::UtcNow - $started).TotalMilliseconds) })
    throw
}
finally {
    try { Stop-PvdDebugProcesses } catch {}
    if ($pvd -and -not $pvd.HasExited) { [void]$pvd.CloseMainWindow(); Start-Sleep -Milliseconds 800; if (-not $pvd.HasExited) { [void]$pvd.Kill() } }
    if ($pvd) { Get-Process -Id $pvd.Id -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue }
}
