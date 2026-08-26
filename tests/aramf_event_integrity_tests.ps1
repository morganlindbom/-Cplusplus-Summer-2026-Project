$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\tools\aramf_event_integrity.ps1')
$temp = Join-Path ([IO.Path]::GetTempPath()) ('aramf-event-test-' + [guid]::NewGuid().ToString('N') + '.jsonl')
try {
    $a = @{eventId='event-a'; eventType='TEST_RESULT'; sequenceNumber=1; status='PASS'}
    $b = @{eventId='event-b'; eventType='TEST_RESULT'; sequenceNumber=2; status='PASS'}
    @($a,$b) | ForEach-Object { ($_ | ConvertTo-Json -Compress) } | Set-Content -LiteralPath $temp -Encoding UTF8
    $id = New-AramfUniqueEventId $temp
    if ($id -in @('event-a','event-b')) { throw 'Generated ID collided' }
    try { Add-AramfEventRecord $temp @{eventId='event-a';eventType='TEST_RESULT';sequenceNumber=3} | Out-Null; throw 'Collision was accepted' } catch { if ($_.Exception.Message -notmatch 'colliding eventId') { throw } }
    Add-AramfEventRecord $temp @{eventId=$id;eventType='TEST_RESULT'} | Out-Null
    $id2 = New-AramfUniqueEventId $temp
    if ($id -eq $id2) { throw 'Sequential IDs were not distinct' }
    $known = @{eventId='event-legacy';eventType='TEST_RESULT';sequenceNumber=4;status='FAIL'}
    Add-AramfEventRecord $temp $known | Out-Null
    $legacy = @(Get-AramfEventRecords $temp | Where-Object eventId -eq 'event-legacy')
    if ($legacy.Count -ne 1) { throw 'Legacy fixture setup failed' }
    $known2 = @{eventId='event-legacy';eventType='VALIDATION_RESULT';sequenceNumber=5;status='FAIL'}
    Add-Content -LiteralPath $temp -Value ($known2 | ConvertTo-Json -Compress) -Encoding UTF8
    $legacyLine = ($known | ConvertTo-Json -Compress)
    $legacyHash = Get-AramfLineSha256 $legacyLine
    $legacyHash2 = Get-AramfLineSha256 ($known2 | ConvertTo-Json -Compress)
    $exceptionPath = Join-Path ([IO.Path]::GetTempPath()) ('aramf-exception-' + [guid]::NewGuid().ToString('N') + '.json')
    try {
        @{exceptions=@(@{eventId='event-legacy';occurrences=@(@{sequenceNumber=4;eventType='TEST_RESULT';rawLineSha256=$legacyHash},@{sequenceNumber=5;eventType='VALIDATION_RESULT';rawLineSha256=$legacyHash2})})} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $exceptionPath -Encoding UTF8
        $exact = Test-AramfEventIntegrity $temp $exceptionPath
        if (-not $exact.KnownLegacyReconciliation) { throw 'Exact exception was not accepted' }
        @{eventId='event-legacy';eventType='TEST_RESULT';sequenceNumber=4;status='ALTERED'} | ConvertTo-Json -Compress | Set-Content -LiteralPath $temp -Encoding UTF8
        $altered = Test-AramfEventIntegrity $temp $exceptionPath
        if ($altered.KnownLegacyReconciliation) { throw 'Altered legacy payload was accepted' }
        @(@{eventId='event-unknown';eventType='TEST_RESULT';sequenceNumber=6},@{eventId='event-unknown';eventType='TEST_RESULT';sequenceNumber=7}) | ForEach-Object { Add-Content -LiteralPath $temp -Value ($_ | ConvertTo-Json -Compress) -Encoding UTF8 }
        $unknown = Test-AramfEventIntegrity $temp $exceptionPath
        if ($unknown.UnknownDuplicates -notcontains 'event-unknown') { throw 'Unknown duplicate was not rejected' }
    } finally { if (Test-Path -LiteralPath $exceptionPath) { Remove-Item -LiteralPath $exceptionPath -Force } }
    $actual = Test-AramfEventIntegrity (Join-Path $PSScriptRoot '..\ARAMF_WORKER\memory\event-log.jsonl') (Join-Path $PSScriptRoot '..\ARAMF_WORKER\memory\event-id-integrity-exceptions.json')
    if ($actual.IntegrityStatus -ne 'PASS WITH ACKNOWLEDGED LEGACY EXCEPTIONS') { throw 'Known legacy reconciliation failed' }
    if (-not $actual.AllNonExemptUnique) { throw 'Non-exempt duplicate detected' }
    'PASS: unique generation and collision rejection'
    'PASS: known historical duplicate accepted only by exact reconciliation'
    'PASS: raw duplicate remains visible and unknown duplicates are not exempt'
} finally { if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force } }
