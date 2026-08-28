$ErrorActionPreference = 'Stop'
$runner = Get-Content (Join-Path $PSScriptRoot 'gui_sio_physical_runner.ps1') -Raw
$prefix = $runner.Substring(0, $runner.IndexOf('function Run-Test'))
$prefix = $prefix.Replace('$PSScriptRoot', "(Join-Path (Get-Location) 'tools')")
Invoke-Expression $prefix

$o = New-PvdObserverCode -ObservedGpio 1 -ObserverMode TIMING -ExpectedLevel 0 -MaxSamples 16 -MaxTransitions 12 -ObservationDurationMs 5000 -ExpectedIntervalMs 5000 -ToleranceMs 500 -SymbolPrefix timer_diag -DiagnosticTimer $true
foreach ($needle in @('timer_diag_startup_dbgpause','timer_diag_startup_raw_timer_64','timer_diag_end_raw_timer_64','timer_diag_timer_progress_valid','timer_diag_transition_raw_timer_64','timer0_hw->timerawh','timer0_hw->timerawl','timer0_hw->dbgpause')) { if (($o.declarations + $o.initialization + $o.poll) -notmatch [regex]::Escape($needle)) { throw "Diagnostic contract missing: $needle" } }
if ($o.poll.IndexOf('const uint64_t now_us') -gt $o.poll.IndexOf('const uint64_t raw_timer_64')) { throw 'SDK timestamp must precede raw timer capture' }
if ($o.poll.IndexOf('transition_raw_timer_64[i]') -lt 0 -or $o.poll.IndexOf('transition_count >= 12') -lt 0) { throw 'Per-transition raw timer evidence missing' }

function Get-TimerHealth([UInt64]$StartRaw,[UInt64]$EndRaw,[UInt64]$StartSdk,[UInt64]$EndSdk,[bool]$TransitionStateChanged,[UInt64[]]$TransitionRaw,[UInt64[]]$TransitionSdk) {
    if ($TransitionRaw.Count -ne $TransitionSdk.Count -or $TransitionRaw.Count -eq 0) { return 'OTHER_INVALID' }
    if ($EndRaw -le $StartRaw -and $TransitionStateChanged) { return 'TIMING_SOURCE_NOT_PROGRESSING' }
    if ($EndRaw -gt $StartRaw -and $EndSdk -le $StartSdk) { return 'SDK_TIME_NOT_PROGRESSING' }
    if ($EndRaw -gt $StartRaw -and $EndSdk -gt $StartSdk) {
        for ($i=1; $i -lt $TransitionRaw.Count; $i++) { if (($TransitionRaw[$i]-$TransitionRaw[$i-1]) -ne ($TransitionSdk[$i]-$TransitionSdk[$i-1])) { return 'TIMESTAMP_CAPTURE_MISMATCH' } }
        return 'TIMER_HEALTH_PASS'
    }
    return 'OTHER_INVALID'
}
if ((Get-TimerHealth 1000 101000 1000 101000 $false ([UInt64[]](1000,6000,11000)) ([UInt64[]](1000,6000,11000))) -ne 'TIMER_HEALTH_PASS') { throw 'timer pass fixture failed' }
if ((Get-TimerHealth 1000 1000 1000 1000 $true ([UInt64[]](1000,1000)) ([UInt64[]](1000,1000))) -ne 'TIMING_SOURCE_NOT_PROGRESSING') { throw 'frozen timer fixture failed' }
if ((Get-TimerHealth 1000 101000 1000 1000 $false ([UInt64[]](1000,6000)) ([UInt64[]](1000,6000))) -ne 'SDK_TIME_NOT_PROGRESSING') { throw 'frozen SDK fixture failed' }
if ((Get-TimerHealth 1000 101000 1000 101000 $false ([UInt64[]](1000,7000)) ([UInt64[]](1000,6000))) -ne 'TIMESTAMP_CAPTURE_MISMATCH') { throw 'mismatch fixture failed' }
if ((Get-TimerHealth 1000 101000 1000 101000 $false ([UInt64[]](1000)) ([UInt64[]](1000,6000))) -ne 'OTHER_INVALID') { throw 'length fixture failed' }
'PWM timer health/instrumentation tests PASS'
