Set-StrictMode -Version Latest

function Get-AramfEventRecords {
    param([Parameter(Mandatory)][string]$EventLogPath)
    if (-not (Test-Path -LiteralPath $EventLogPath)) { throw "Event log not found: $EventLogPath" }
    $records = @()
    foreach ($line in (Get-Content -LiteralPath $EventLogPath)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        try { $records += ($line | ConvertFrom-Json) }
        catch { throw "Invalid JSONL record in $EventLogPath" }
    }
    return $records
}

function Get-AramfLineSha256 {
    param([Parameter(Mandatory)][string]$Line)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($Line)
        return (($sha.ComputeHash($bytes) | ForEach-Object { $_.ToString('x2') }) -join '')
    } finally { $sha.Dispose() }
}

function Get-AramfEventIdSet {
    param([Parameter(Mandatory)][string]$EventLogPath)
    $set = New-Object 'System.Collections.Generic.HashSet[string]'
    foreach ($record in (Get-AramfEventRecords $EventLogPath)) {
        if ([string]::IsNullOrWhiteSpace([string]$record.eventId)) { throw 'Event record has no eventId' }
        [void]$set.Add([string]$record.eventId)
    }
    return $set
}

function New-AramfUniqueEventId {
    param([Parameter(Mandatory)][string]$EventLogPath)
    $existing = Get-AramfEventIdSet $EventLogPath
    do { $candidate = 'event-' + ([guid]::NewGuid().ToString()) } while ($existing.Contains($candidate))
    return $candidate
}

function Add-AramfEventRecord {
    param(
        [Parameter(Mandatory)][string]$EventLogPath,
        [Parameter(Mandatory)][hashtable]$Record
    )
    $existing = Get-AramfEventRecords $EventLogPath
    $ids = New-Object 'System.Collections.Generic.HashSet[string]'
    foreach ($item in $existing) { [void]$ids.Add([string]$item.eventId) }
    $id = [string]$Record.eventId
    if ([string]::IsNullOrWhiteSpace($id)) { throw 'New event requires eventId' }
    if ($ids.Contains($id)) { throw "Refusing colliding eventId: $id" }
    $max = 0
    foreach ($item in $existing) { if ([int]$item.sequenceNumber -gt $max) { $max = [int]$item.sequenceNumber } }
    if (-not $Record.ContainsKey('sequenceNumber')) { $Record.sequenceNumber = $max + 1 }
    if ([int]$Record.sequenceNumber -le $max) { throw 'New event sequence must be greater than current maximum' }
    $json = ($Record | ConvertTo-Json -Compress -Depth 20)
    Add-Content -LiteralPath $EventLogPath -Value $json -Encoding UTF8
    return $Record
}

function Test-AramfEventIntegrity {
    param(
        [Parameter(Mandatory)][string]$EventLogPath,
        [Parameter(Mandatory)][string]$ExceptionPath
    )
    $records = @(Get-AramfEventRecords $EventLogPath)
    $linesBySequence = @{}
    foreach ($line in (Get-Content -LiteralPath $EventLogPath)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        $parsed = $line | ConvertFrom-Json
        $linesBySequence[[int]$parsed.sequenceNumber] = [pscustomobject]@{ record = $parsed; sha256 = Get-AramfLineSha256 $line }
    }
    $exceptions = Get-Content -LiteralPath $ExceptionPath -Raw | ConvertFrom-Json
    $groups = @($records | Group-Object eventId | Where-Object { $_.Count -gt 1 })
    $registered = @{}
    foreach ($entry in $exceptions.exceptions) { $registered[[string]$entry.eventId] = $entry }
    $unknown = @()
    $badFingerprint = @()
    foreach ($group in $groups) {
        if (-not $registered.ContainsKey($group.Name)) { $unknown += $group.Name; continue }
        $expected = @($registered[$group.Name].occurrences)
        $actual = @($group.Group | ForEach-Object { [pscustomobject]@{ sequenceNumber = [int]$_.sequenceNumber; eventType = [string]$_.eventType } })
        if ($expected.Count -ne $actual.Count) { $badFingerprint += $group.Name; continue }
        foreach ($occurrence in $expected) {
            $found = $group.Group | Where-Object { [int]$_.sequenceNumber -eq [int]$occurrence.sequenceNumber -and [string]$_.eventType -eq [string]$occurrence.eventType }
            if ($null -eq $found -or $linesBySequence[[int]$occurrence.sequenceNumber].sha256 -ne [string]$occurrence.rawLineSha256) { $badFingerprint += $group.Name }
        }
    }
    $nonExempt = @($groups | Where-Object { -not $registered.ContainsKey($_.Name) })
    [pscustomobject]@{
        RawHistoricalUniqueness = ($groups.Count -eq 0)
        DuplicateCount = $groups.Count
        KnownLegacyReconciliation = ($unknown.Count -eq 0 -and $badFingerprint.Count -eq 0 -and $groups.Count -eq $registered.Count)
        UnknownDuplicates = $unknown
        BadFingerprints = $badFingerprint
        AllNonExemptUnique = ($nonExempt.Count -eq 0)
        IntegrityStatus = if ($unknown.Count -eq 0 -and $badFingerprint.Count -eq 0) { 'PASS WITH ACKNOWLEDGED LEGACY EXCEPTIONS' } else { 'FAIL' }
        HighestSequence = (($records | Measure-Object sequenceNumber -Maximum).Maximum)
        EventCount = $records.Count
    }
}
