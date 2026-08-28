$ErrorActionPreference='Stop'
$runner=Get-Content (Join-Path $PSScriptRoot 'gui_sio_physical_runner.ps1') -Raw
$prefix=$runner.Substring(0,$runner.IndexOf('function Run-Test')).Replace('$PSScriptRoot', "(Join-Path (Get-Location) 'tools')")
Invoke-Expression $prefix
$process=$null
try {
  $process=Start-Process -FilePath (Resolve-Path 'build-current/pico_visual_designer.exe').Path -ArgumentList '--certification-dialogs' -PassThru
  Start-Sleep -Milliseconds 1800
  Open-ExistingProject $process.Id 'C:\Users\morga\AppData\Local\Temp\pvd-pwm-first-100hz-50\PWM_FIRST_100HZ_50.sqlite'
  $null=Select-Workflow (Get-PvdWindow $process.Id) 'Debug' $process.Id
  $null=Invoke-Element (Find-LivePvdElement $process.Id 'debug_start' 20)
  $null=Wait-PvdDebugState -ProcessId $process.Id -Accepted @('Debug session ready','Remote debugging using localhost:3333','Remote debugging from host') -TimeoutSeconds 45
  $cmds=@(
    'print (int)observer_pwm0a_100hz_50_gpio1_transition_count',
    'print (unsigned long long)observer_pwm0a_100hz_50_gpio1_transition_us[0]',
    'print (unsigned long long)observer_pwm0a_100hz_50_gpio1_transition_us[1]',
    'print (unsigned long long)observer_pwm0a_100hz_50_gpio1_transition_us[2]',
    'print (unsigned long long)observer_pwm0a_100hz_50_gpio1_start_us',
    'print time_us_64()',
    'p/x &observer_pwm0a_100hz_50_gpio1_transition_us',
    'x/24wx &observer_pwm0a_100hz_50_gpio1_transition_us'
  )
  foreach($cmd in $cmds){ Send-PvdDebugCommand $process.Id $cmd; Start-Sleep -Milliseconds 350 }
  $log=Get-LiveControlText (Find-LivePvdElement $process.Id 'debug_log' 10)
  $log | Set-Content -LiteralPath 'validation/pin_certification/results/pwm-first-100hz-50-gdb-diagnostic.log' -Encoding UTF8
  $log
} finally {
  if($process){try{$null=Invoke-Element (Find-LivePvdElement $process.Id 'debug_stop' 10)}catch{};if(-not $process.HasExited){$process.CloseMainWindow();Start-Sleep -Milliseconds 600;if(-not $process.HasExited){$process.Kill()}}}
}
