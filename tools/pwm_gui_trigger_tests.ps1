$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$runnerPath = Join-Path $root 'tools/gui_sio_physical_runner.ps1'
$runner = Get-Content $runnerPath -Raw
$prefix = $runner.Substring(0, $runner.IndexOf('function Run-Test')).Replace('$PSScriptRoot', "(Join-Path (Get-Location) 'tools')")
Invoke-Expression $prefix
function Require([bool]$ok, [string]$message) { if (-not $ok) { throw $message } }

Require ($runner -match 'function Get-UiaAutomationIdSafe') 'Safe AutomationId helper missing'
Require ($runner -match 'GUI_TRIGGER_NOT_FOUND') 'Bounded trigger failure classification missing'
Require ($runner -match 'function Invoke-PvdProductionReadback[\s\S]*Select-Workflow[\s\S]*debug_readback_start') 'Production trigger does not enter Debug page before lookup'
$findStart = $runner.IndexOf('function Find-LivePvdElement')
$findEnd = $runner.IndexOf('function Get-PvdBuildUiStatus', $findStart)
$findPath = $runner.Substring($findStart, $findEnd - $findStart)
Require ($findPath -notmatch '\.Current\.AutomationId') 'Unsafe trigger-path AutomationId dereference remains'
Require ($findPath -match 'Get-UiaCurrentSafe') 'Trigger path does not use safe Current access'

Require ($null -eq (Get-UiaAutomationIdSafe $null)) 'Null element was not safely skipped'
$nullCurrent = [pscustomobject]@{ Current = $null }
Require ($null -eq (Get-UiaAutomationIdSafe $nullCurrent)) 'Null Current was not safely skipped'
$throwing = New-Object psobject
$throwing | Add-Member ScriptProperty Current { throw 'stale UIA property' }
Require ($null -eq (Get-UiaAutomationIdSafe $throwing)) 'Stale Current did not safely fail'

$rect = [pscustomobject]@{ Width = 10; Height = 10 }
$validCurrent = [pscustomobject]@{ AutomationId = 'debug_readback_start'; Name = 'Readback'; IsEnabled = $true; IsOffscreen = $false; BoundingRectangle = $rect }
$valid = [pscustomobject]@{ Current = $validCurrent }
$stale = [pscustomobject]@{ Current = $null }
$script:triggerRoots = @()
foreach ($itemSet in @(@($stale), @($valid))) {
    $r = [pscustomobject]@{}
    $captured = $itemSet
    $r | Add-Member ScriptMethod FindAll { param($scope, $condition) return $captured }
    $script:triggerRoots += $r
}
$script:triggerRootIndex = 0
function global:Get-PvdWindow([int]$processId) { $r = $script:triggerRoots[$script:triggerRootIndex]; if ($script:triggerRootIndex -lt ($script:triggerRoots.Count - 1)) { $script:triggerRootIndex++ }; return $r }
$found = Find-LivePvdElement 123 'debug_readback_start' 2
Require ($found.Current.AutomationId -eq 'debug_readback_start') 'Fresh reacquisition did not find exact trigger'
Require ($script:triggerRootIndex -eq 1) 'Reacquisition did not move past stale element'
'PASS: null, null-Current, and stale UIA properties are safe'
'PASS: bounded fresh reacquisition finds debug_readback_start'
'PASS: production trigger uses explicit GUI_TRIGGER failure classification'
