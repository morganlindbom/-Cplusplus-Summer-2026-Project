$ErrorActionPreference = 'Stop'
$runner = Get-Content (Join-Path $PSScriptRoot 'gui_sio_physical_runner.ps1') -Raw
$debug = Get-Content (Join-Path (Split-Path $PSScriptRoot) 'src/systems/mainwindow/debug/debug_column3/DebugColumn3.cpp') -Raw
$header = Get-Content (Join-Path (Split-Path $PSScriptRoot) 'src/systems/mainwindow/debug/debug_column3/DebugColumn3.hpp') -Raw

if (-not $runner.Contains("`$readbackButton = if (`$ReadbackOnly) { 'debug_readback_start' } else { 'debug_start' }")) { throw 'Runner does not select the dedicated readback control' }
if ($runner -notmatch "Invoke-PvdObserverGdbRead[\s\S]*ReadbackOnly[\s\S]*debug_halt") { throw 'Timing readback path missing attach/halt sequence' }
$readbackStart = $runner.IndexOf("} else {`n            # Timing evidence")
$readbackEnd = $runner.IndexOf('foreach($c in (New-PvdObserverGdbReadCommands', $readbackStart)
if ($readbackStart -lt 0 -or $readbackEnd -lt $readbackStart) { throw 'Readback branch boundaries missing' }
$readbackBlock = $runner.Substring($readbackStart, $readbackEnd - $readbackStart)
if ($readbackBlock.Contains('debug_continue') -or $readbackBlock.Contains("GdbCommand 'continue'")) { throw 'Readback path continues before evidence' }
if ($debug -notmatch 'void DebugColumn3::startReadbackSession') { throw 'Dedicated production readback entry point missing' }
if ($debug -notmatch 'if \(!readbackOnly_\)\s*\{[\s\S]*resetMonitorCommand') { throw 'Normal reset branch missing' }
if ($debug -notmatch 'Certification readback attach commands sent: target extended-remote only') { throw 'Readback command audit missing' }
if ($debug -notmatch 'if \(!readbackOnly_ && resetMethod_ != "none"\)') { throw 'Readback cleanup still resets target' }
if ($header -notmatch 'startReadbackSession|readbackOnly_|readbackStartButton_') { throw 'Readback API/member contract missing' }
if ($runner -notmatch 'Transfer[\s\S]*MeasurementWaitMs[\s\S]*Invoke-PvdProductionReadback') { throw 'Measurement/readback order contract missing' }
'PWM no-reset readback contract PASS'
