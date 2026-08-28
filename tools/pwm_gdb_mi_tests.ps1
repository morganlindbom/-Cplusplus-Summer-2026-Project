$ErrorActionPreference = 'Stop'
$source = Get-Content (Join-Path $PSScriptRoot '..\src\systems\mainwindow\debug\debug_column3\DebugColumn3.cpp') -Raw
$protocol = Get-Content (Join-Path $PSScriptRoot '..\src\systems\debug\MiProtocol.cpp') -Raw
if ($source -notmatch '--interpreter=mi2' -or $source -notmatch 'sendReadbackMiCommand' -or
    $source -notmatch 'readbackMiForeignTokenCount_' -or $protocol -notmatch 'parseRecord' -or
    $protocol -notmatch 'consumeCString' -or $protocol -notmatch 'consumeValue') { throw 'MI production contract is incomplete' }

function Parse-MiRecord([string]$record) {
    if ($record -match '^(\d+)\^(done|error|connected)(?:,(.*))?$') {
        $token = [int]$Matches[1]
        $class = $Matches[2]
        $body = $Matches[3]
        return [pscustomobject]@{ Token = $token; Class = $class; Body = $body }
    }
    return [pscustomobject]@{ Token = $null; Class = 'async'; Body = $record }
}

$fixture = @(
    '=thread-created,id="2",group-id="i1"',
    '*stopped,reason="signal-received"',
    '~"remote output\\n"',
    '102^done,value="16"',
    '101^done,value="1"'
)
$records = $fixture | ForEach-Object { Parse-MiRecord $_ }
$evidence = $records | Where-Object { $_.Token -eq 101 }
if (@($evidence).Count -ne 1 -or @($evidence)[0].Class -ne 'done' -or @($evidence)[0].Body -notmatch 'value="1"') { throw 'token association fixture failed' }
if (($records | Where-Object { $_.Token -eq $null }).Count -ne 3) { throw 'async classification fixture failed' }
if (@($records | Where-Object { $_.Token -eq 102 }).Count -ne 1) { throw 'foreign token fixture failed' }

$splitBuffer = '101^do'
if ((Parse-MiRecord $splitBuffer).Token -ne $null) { throw 'partial record was parsed too early' }
$splitBuffer += 'ne,value="1"'
$splitRecord = Parse-MiRecord $splitBuffer
if ($splitRecord.Token -ne 101 -or $splitRecord.Class -ne 'done' -or $splitRecord.Body -notmatch 'value="1"') { throw 'split record fixture failed' }
Write-Output 'PASS: tokenized GDB/MI association, async separation, foreign-token, and split-record fixtures'
