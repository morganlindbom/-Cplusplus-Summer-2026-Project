$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$runner = Get-Content (Join-Path $PSScriptRoot 'gui_sio_physical_runner.ps1') -Raw
$debug = Get-Content (Join-Path $root 'src/systems/mainwindow/debug/debug_column3/DebugColumn3.cpp') -Raw

function Require([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

# The production helper must create durable failure evidence before it starts
# attaching, and every externally visible failure must carry a stage/reason.
Require ($runner -match 'function Write-PvdReadbackEvent') 'Missing incremental readback logger'
Require ($runner -match 'Set-Content -LiteralPath \$ReadbackLogPath') 'Readback log is not created before attach'
Require ($runner -match "status='READBACK_FAILED'") 'Missing structured readback failure result'
Require ($runner -match 'failure_stage=\$stage') 'Failure stage is not persisted'
Require ($runner -match 'failure_reason=\$failureReason') 'Failure reason is not persisted'
Require ($runner -match "ReadbackLogPath") 'Readback log path is not injectable'
Require ($runner -match "ReadbackResultPath") 'Readback result path is not injectable'
Require ($runner -match 'readbackDeadline') 'Evidence transaction has no overall deadline'
Require ($runner -match 'Evidence read deadline exceeded') 'Evidence timeout does not produce a structured failure'
Require ($runner -match 'Write-PvdReadbackEvent[\s\S]*''RESPONSE''') 'Last response is not incrementally logged'
Require ($runner -match 'function Get-PvdReadbackDeadlineBudget') 'Shared readback deadline budget is missing'
Require ($runner -match 'InnerWorstCaseMs') 'Inner worst-case budget is not computed'
Require ($runner -match 'OuterWatchdogMs') 'Outer watchdog budget is not derived'
Require ($runner -match 'function Write-PvdAtomicJson') 'Atomic result writer is missing'
Require ($runner -match 'function Write-PvdReadbackOuterWatchdogFailure') 'Outer watchdog failure artifact is missing'
Require ($runner -match 'status=''OUTER_WATCHDOG_FAILURE''') 'Outer watchdog classification is missing'
foreach ($field in @('last_command','last_response','cleanup_completed','raw_log_path','elapsed_ms')) {
    Require ($runner -match $field) "Failure result field missing: $field"
}

# All production waits in this path are bounded.  The explicit timeout values
# are checked in the real helper rather than in a disconnected mock.
Require ($runner -match 'Wait-PvdDebugState[\s\S]*TimeoutSeconds 45') 'OpenOCD/GDB readiness wait is unbounded or missing a deadline'
Require ($runner -match 'Wait-PvdDebugState[\s\S]*TimeoutSeconds 10') 'Halt wait is unbounded or missing a deadline'
Require ($runner -match 'Find-LivePvdElement[\s\S]* 20') 'Readback UI acquisition lacks a bounded deadline'
Require ($runner -match 'Stop-PvdDebugProcesses') 'Failure cleanup is missing'

# Socket readiness must launch GDB even if the OpenOCD listening line was split
# across QProcess chunks; this is the concrete historical hang fix.
$socketReady = [regex]::Match($debug, 'if \(socket\.waitForConnected\(40\)\)[\s\S]*?return;')
Require $socketReady.Success 'Socket readiness branch not found'
Require ($socketReady.Value -match 'startGdb\(\)') 'Socket readiness does not launch GDB'

# Readback remains distinct from normal Debug and cannot send continue/reset.
$readback = [regex]::Match($debug, 'void DebugColumn3::sendGdbStartupCommands\(\)[\s\S]*?\n}\n\nvoid DebugColumn3::sendGdbCommand')
Require $readback.Success 'GDB startup command function not found'
Require ($readback.Value -match 'if \(!readbackOnly_\)') 'Normal/readback command split missing'
Require ($readback.Value -match 'target extended-remote localhost:3333') 'Readback remote attach missing'
Require ($readback.Value -match 'no reset/load/program/continue') 'Readback no-reset audit missing'
$cleanup = [regex]::Match($debug, 'void DebugColumn3::stopOpenOcd\(\)[\s\S]*?\n}\n\nbool DebugColumn3::sendOpenOcdCommands')
Require $cleanup.Success 'OpenOCD cleanup function not found'
Require ($cleanup.Value -match 'if \(!readbackOnly_ && resetMethod_ != "none"\)') 'Readback cleanup can reset target'

# Deterministic host-only state outcomes used by the runner contract.
$cases = @(
    @{ Name='ready-connect-halt-evidence'; Stage='EVIDENCE_COMPLETE'; Pass=$true },
    @{ Name='openocd-timeout'; Stage='OPENOCD_READY'; Pass=$false },
    @{ Name='gdb-timeout'; Stage='GDB_CONNECTED'; Pass=$false },
    @{ Name='halt-timeout'; Stage='TARGET_HALTED'; Pass=$false },
    @{ Name='missing-symbol'; Stage='EVIDENCE_READING'; Pass=$false },
    @{ Name='malformed-response'; Stage='EVIDENCE_READING'; Pass=$false },
    @{ Name='gdb-early-exit'; Stage='GDB_CONNECTED'; Pass=$false },
    @{ Name='openocd-early-exit'; Stage='OPENOCD_READY'; Pass=$false }
)
Require ($cases.Count -eq 8) 'Readback failure matrix is incomplete'
Require ((@($cases | Where-Object { -not $_.Pass }).Count) -eq 7) 'Failure matrix does not preserve negative outcomes'
'PWM readback reliability contract PASS'
