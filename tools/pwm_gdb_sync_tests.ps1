$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$cppPath = Join-Path $root 'src/systems/mainwindow/debug/debug_column3/DebugColumn3.cpp'
$cpp = Get-Content -LiteralPath $cppPath -Raw

function Assert-Contract([bool]$condition, [string]$message) {
    if (-not $condition) { throw "FAIL: $message" }
}

Assert-Contract ($cpp.Contains('startReadbackSynchronization()')) 'readback must enter synchronization before evidence'
Assert-Contract ($cpp.Contains('set prompt %1')) 'session-specific GDB prompt must be configured'
Assert-Contract ($cpp.Contains('echo %1')) 'session-specific sync token must be sent'
Assert-Contract ($cpp.Contains('GDB_SYNC PASS')) 'sync success must be logged'
Assert-Contract ($cpp.Contains('readbackPrompt_')) 'evidence must use the unique prompt field'
Assert-Contract (-not $cpp.Contains('readbackResponseBuffer_.contains("(gdb)")')) 'generic gdb prompt must not delimit evidence'

# Deterministic split-chunk contract fixture.  Connection/thread output is present,
# but only the response after the command send and before the unique prompt is data.
$run = 'readback-test-1'
$prompt = "__PVD_GDB_${run}__"
$token = "__PVD_SYNC_${run}__"
$chunks = @(
    "Remote debugging using localhost:3333`nThread 1 notification`n(gdb) `n",
    "set prompt accepted`n$prompt",
    "echo $token`n$token`n$prompt",
    "print value`n`$1 = 42`n$prompt"
)
$buffer = ''
$syncPrompt = $false
$syncToken = $false
$evidence = $null
foreach ($chunk in $chunks) {
    $buffer += $chunk
    if (-not $syncPrompt -and $buffer.Contains($prompt)) {
        $syncPrompt = $true
        $buffer = ''
    }
    if ($syncPrompt -and -not $syncToken -and $buffer.Contains($token) -and $buffer.Contains($prompt)) {
        $syncToken = $true
        $buffer = ''
    }
    if ($syncToken -and $buffer.Contains($prompt)) {
        $evidence = $buffer
        break
    }
}
Assert-Contract $syncPrompt 'split unique prompt must be detected'
Assert-Contract $syncToken 'sync token must be paired with the unique prompt'
Assert-Contract ($null -ne $evidence -and $evidence.Contains('42') -and -not $evidence.Contains('Remote debugging')) 'initial output must not contaminate command one'
Write-Output 'PASS: GDB synchronization contract and split-response fixture'
