$ErrorActionPreference='Stop'
$runner=Get-Content (Join-Path $PSScriptRoot 'gui_sio_physical_runner.ps1') -Raw
$prefix=$runner.Substring(0,$runner.IndexOf('function Run-Test')).Replace('$PSScriptRoot', "(Join-Path (Get-Location) 'tools')")
Invoke-Expression $prefix
function Gdb([object[]]$v){$lines=@();for($i=0;$i -lt $v.Count;$i++){$lines+="`$$($i+1) = $($v[$i])"};$lines -join "`n"}
$observer=@{Mode='TIMING';Prefix='observer';MaxSamples=16;MaxTransitions=12}
$small=ConvertFrom-PvdObserverGdbOutput -RawText (Gdb @(1,1,0,4,1,1000,0,6000,1,11000,0,16000,0)) -Mode TIMING -Prefix observer -MaxSamples 16 -MaxTransitions 12
if($small.TransitionTimesUs[0] -ne 1000 -or $small.TransitionTimesUs[3] -ne 16000 -or $small.Overflow){throw 'small uint64 timestamp fixture failed'}
$large=ConvertFrom-PvdObserverGdbOutput -RawText (Gdb @(1,1,0,4,1,123456789012,0,123456794012,1,123456799012,0,123456804012,0)) -Mode TIMING -Prefix observer -MaxSamples 16 -MaxTransitions 12
if($large.TransitionTimesUs[0] -ne [UInt64]123456789012){throw 'large uint64 timestamp fixture failed'}
foreach($bad in @((Gdb @(1,1,0,3,1,1000,0,6000,1,5000,0)),(Gdb @(1,1,0,3,1,0,0,0,1,0,0)),(Gdb @(1,1,0,2,1,1000,0,6000)))){try{ConvertFrom-PvdObserverGdbOutput -RawText $bad -Mode TIMING -Prefix observer -MaxSamples 16 -MaxTransitions 12|Out-Null;throw 'invalid timestamp fixture accepted'}catch{if($_.Exception.Message -eq 'invalid timestamp fixture accepted'){throw}}}
$fifty=Get-PvdPwmTimingClassification ([UInt64[]](1,0,1,0,1,0,1,0,1,0,1,0)) ([UInt64[]](5000,10000,15000,20000,25000,30000,35000,40000,45000,50000,55000,60000))
if($fifty.MeanPeriodMs -ne 10 -or $fifty.MeanHighMs -ne 5 -or $fifty.MeanLowMs -ne 5 -or $fifty.FrequencyHz -ne 100 -or $fifty.DutyPercent -ne 50){throw '50 percent classifier fixture failed'}
$zeroStart=Get-PvdPwmTimingClassification ([UInt64[]](0,1,0,1,0,1)) ([UInt64[]](1000,3500,6000,13500,16000,23500))
if($zeroStart.MeanPeriodMs -ne 10 -or $zeroStart.MeanHighMs -ne 2.5 -or $zeroStart.MeanLowMs -ne 7.5){throw 'state-aware 25 percent classifier fixture failed'}
'PWM observer timestamp/parser/classifier tests PASS'
