$ErrorActionPreference = 'Stop'
$runner = Get-Content (Join-Path $PSScriptRoot 'gui_sio_physical_runner.ps1') -Raw
foreach ($required in @('function Stop-PvdDebugProcesses','function Assert-PvdNoDebugSession','[switch]$ReadbackOnly','MeasurementPhase','OpenOCDDuringCapture=$false','GdbDuringCapture=$false','TargetAutonomous=$true')) {
    if ($runner -notmatch [regex]::Escape($required)) { throw "Missing debugger-free measurement contract: $required" }
}
$run = [regex]::Match($runner, 'function Invoke-PvdObserverTargetRun[\s\S]*?\nfunction Get-PvdObserverTimingClassification')
if (-not $run.Success) { throw 'Observer target run function not found' }
$text = $run.Value
$transfer = $text.IndexOf('Invoke-PvdNormalTransfer')
$measurement = $text.IndexOf('Start-Sleep -Milliseconds $MeasurementWaitMs')
$readback = $text.IndexOf('Invoke-PvdProductionReadback -ProcessId $ProcessId')
if ($transfer -lt 0 -or $measurement -lt 0 -or $readback -lt 0 -or -not ($transfer -lt $measurement -and $measurement -lt $readback)) { throw 'Invalid timing execution order' }
if ($text -match 'ReadbackOnly\).*debug_continue|ReadbackOnly\).*GdbCommand.*continue') { throw 'Readback-only path can continue target' }
'PWM debugger-free measurement order PASS'
