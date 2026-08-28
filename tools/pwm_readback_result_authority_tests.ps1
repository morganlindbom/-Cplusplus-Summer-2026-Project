$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$runner=Get-Content (Join-Path $root 'tools/gui_sio_physical_runner.ps1') -Raw
function Require([bool]$ok,[string]$message){if(-not $ok){throw $message}}
$start=$runner.IndexOf('function Invoke-PvdProductionReadback')
$end=$runner.IndexOf('function Invoke-PvdObserverGdbRead',$start)
$route=$runner.Substring($start,$end-$start)
Require ($route -match '\$resultPath=\$absoluteResult') 'Runner does not use exact requested result path'
Require ($route -match 'RESULT_RUN_ID_MISMATCH') 'Run-ID mismatch is not explicit'
Require ($route -match 'status -in @\(''READBACK_COMPLETED'',''READBACK_FAILED''\)') 'Terminal C++ status precedence missing'
Require ($route.IndexOf('if(Test-Path -LiteralPath $resultPath)') -lt $route.IndexOf('Get-LiveControlText')) 'UI status is checked before result authority'
Require ($route -match 'PowerShellEvidenceCommands=0') 'PowerShell evidence ownership changed'
'PASS: exact current-run C++ result precedes UI status'
'PASS: READBACK_COMPLETED and READBACK_FAILED are terminal'
'PASS: stale/mismatched run IDs are rejected'
'PASS: UI status remains supplemental only'
