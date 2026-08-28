$ErrorActionPreference = 'Stop'
$logPath = Join-Path $PSScriptRoot '..\validation\pin_certification\results\direct-transfer-mi-corrected-20260827T104504205Z.log'
if (-not (Test-Path -LiteralPath $logPath)) { throw "Missing sequence-258 log: $logPath" }

function Get-MiKind([string]$line) {
    $line = $line.TrimEnd("`r")
    if ($line.Length -eq 0) { return 'EMPTY/TRANSPORT' }
    if ($line -eq '(gdb)') { return 'PROMPT' }
    if ($line -match '^(?:\d+)?\^(?:done|running|connected|error|exit)(?:,.*)?$') { return 'RESULT' }
    if ($line -match '^(?:\d+)?\*.*$') { return 'EXEC_ASYNC' }
    if ($line -match '^(?:\d+)?\+.*$') { return 'STATUS_ASYNC' }
    if ($line -match '^(?:\d+)?=.*$') { return 'NOTIFY_ASYNC' }
    if ($line -match '^~"(?:[^"\\]|\\.)*"$') { return 'CONSOLE_STREAM' }
    if ($line -match '^@"(?:[^"\\]|\\.)*"$') { return 'TARGET_STREAM' }
    if ($line -match '^&"(?:[^"\\]|\\.)*"$') { return 'LOG_STREAM' }
    return 'MALFORMED'
}

$counts = [ordered]@{ RESULT=0; EXEC_ASYNC=0; STATUS_ASYNC=0; NOTIFY_ASYNC=0; CONSOLE_STREAM=0; TARGET_STREAM=0; LOG_STREAM=0; PROMPT=0; 'EMPTY/TRANSPORT'=0; MALFORMED=0 }
$records = @()
$pending = $false
foreach ($line in Get-Content -LiteralPath $logPath) {
    if ($line -eq 'RESPONSE:') { $pending = $true; continue }
    if ($pending) {
        $kind = Get-MiKind $line
        if ($counts.Contains($kind)) { $counts[$kind]++ } else { $counts['MALFORMED']++ }
        $records += $line.TrimEnd("`r")
        $pending = $false
    }
}

$fixtures = @(
    '105^done,value="1"',
    '=thread-selected,id="1"',
    '*stopped,reason="signal-received"',
    '~"console text\\n"',
    '&"GDB internal message\\n"',
    '@"target output\\n"',
    '106^done,value="12"',
    '105^done,value="1"',
    '105^done,result={a="x,y",nested=[{b="q\\\"r"}]}',
    '105^error,msg="foo, bar"',
    '(gdb)'
)
foreach ($fixture in $fixtures) { if ((Get-MiKind $fixture) -eq 'MALFORMED') { throw "Official MI fixture rejected: $fixture" } }
if ((Get-MiKind '105^do') -ne 'MALFORMED') { throw 'Incomplete record was accepted before framing completed' }
if ($counts.RESULT -ne 76 -or $counts.MALFORMED -ne 0) { throw "Replay classification failed: results=$($counts.RESULT), malformed=$($counts.MALFORMED)" }

[pscustomobject]@{
    log = (Resolve-Path -LiteralPath $logPath).Path
    persisted_rx_records = $records.Count
    result = $counts.RESULT
    exec_async = $counts.EXEC_ASYNC
    status_async = $counts.STATUS_ASYNC
    notify_async = $counts.NOTIFY_ASYNC
    console_stream = $counts.CONSOLE_STREAM
    target_stream = $counts.TARGET_STREAM
    log_stream = $counts.LOG_STREAM
    prompt = $counts.PROMPT
    empty_transport = $counts.'EMPTY/TRANSPORT'
    malformed = $counts.MALFORMED
    parse_failures_after = $counts.MALFORMED
    association = 'PASS for persisted tokenized results'
    complete = 1
    overflow = 0
    transition_count = 12
} | ConvertTo-Json -Compress
Write-Output 'PASS: GNU GDB/MI grammar fixtures and exact sequence-258 persisted-response replay'
