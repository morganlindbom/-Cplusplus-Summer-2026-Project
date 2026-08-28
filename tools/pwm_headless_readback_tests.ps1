$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$runner=Get-Content (Join-Path $root 'tools/gui_sio_physical_runner.ps1') -Raw
$prefix=$runner.Substring(0,$runner.IndexOf('function Run-Test')).Replace('$PSScriptRoot',"(Join-Path (Get-Location) 'tools')")
Invoke-Expression $prefix
function Require([bool]$ok,[string]$message){if(-not $ok){throw $message}}
Require ($runner -match 'function New-PvdHeadlessReadbackSession') 'Headless session factory missing'
$hs=$runner.IndexOf('function Invoke-PvdHeadlessObserverGdbRead');$he=$runner.IndexOf('function Invoke-PvdCertificationReadback',$hs)
$headless=$runner.Substring($hs,$he-$hs)
Require ($headless -notmatch 'Find-LivePvdElement|Get-PvdWindow|Get-LiveControlText|AutomationElement') 'Headless evidence path depends on GUI'
Require ($headless -match 'Invoke-ReadbackGdbCommand') 'Headless path does not use direct command transport'
Require ($runner -match 'function Invoke-PvdCertificationReadback') 'Production certification readback entrypoint missing'
$production=$runner.Substring($runner.IndexOf('function Invoke-PvdCertificationReadback'),$runner.IndexOf('function Invoke-PvdObserverGdbRead',$runner.IndexOf('function Invoke-PvdCertificationReadback'))-$runner.IndexOf('function Invoke-PvdCertificationReadback'))
Require ($production -match 'Invoke-PvdHeadlessObserverGdbRead') 'Production entrypoint does not route to headless reader'
Require ($runner -match 'READBACK_SESSION_UNAVAILABLE') 'Missing explicit no-session failure'
Require ($runner -match 'function Invoke-PvdProductionReadback') 'Orchestration-only production entrypoint missing'
Require ($runner -match 'Invoke-PvdProductionReadback -ProcessId') 'Timing runner does not use production readback entrypoint'
$psroute=$runner.Substring($runner.IndexOf('function Invoke-PvdProductionReadback'),$runner.IndexOf('function Invoke-PvdObserverGdbRead',$runner.IndexOf('function Invoke-PvdProductionReadback'))-$runner.IndexOf('function Invoke-PvdProductionReadback'))
Require ($psroute -notmatch 'Invoke-PvdObserverGdbRead|Send-PvdDebugCommand') 'Production route falls back to PowerShell evidence reader'
$observer=@{ObserverMode='TIMING';SymbolPrefix='observer_headless';MaxSamples=16;MaxTransitions=12}
$responses=@('$1 = 1','$2 = 1','$3 = 12','$4 = 0','$5 = 1','$6 = 1000','$7 = 0','$8 = 1','$9 = 2000','$10 = 0','$11 = 1','$12 = 3000','$13 = 0','$14 = 1','$15 = 4000','$16 = 0','$17 = 1','$18 = 5000','$19 = 0','$20 = 1','$21 = 6000','$22 = 0','$23 = 1','$24 = 7000','$25 = 0','$26 = 1','$27 = 8000','$28 = 0','$29 = 1','$30 = 9000','$31 = 0','$32 = 1','$33 = 10000','$34 = 0','$35 = 1','$36 = 11000','$37 = 0','$38 = 1','$39 = 12000','$40 = 0','$41 = 0')
$session=[pscustomobject]@{Next=0;Responses=$responses}
$session | Add-Member ScriptMethod SendCommand { param($c) $this.Next++ }
$session | Add-Member ScriptMethod ReceiveResponse { param($d) $this.Responses[$this.Next-1] }
$session | Add-Member ScriptMethod Cleanup { }
$log=Join-Path $env:TEMP 'pvd-headless-readback-test.log';$json=[IO.Path]::ChangeExtension($log,'.json')
$result=Invoke-PvdHeadlessObserverGdbRead -Session $session -Observer $observer -ReadbackLogPath $log -ReadbackResultPath $json -ReadbackTimeoutSeconds 30
Require (Test-Path $log) 'Headless raw log missing'
Require ($result.ReadbackDiagnostics.GuiResolutionCalls -eq 0) 'Headless path consulted GUI'
Require ($result.ReadbackDiagnostics.EvidenceReadCompleted) 'Headless schema did not complete'
Require ((Get-Content $log -Raw) -notmatch 'PVD main window|UIA|window identity') 'Headless log contains GUI dependency'
'PASS: readback evidence does not require a PVD window'
'PASS: headless evidence path uses retained session transport'
