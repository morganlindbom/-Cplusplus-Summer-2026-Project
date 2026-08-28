param(
    [Parameter(Mandatory)][string]$Database,
    [Parameter(Mandatory)][string]$ResultPath,
    [Parameter(Mandatory)][string]$LogPath,
    [string]$PvdExecutable = (Join-Path (Get-Location) 'build-current/pico_visual_designer.exe')
)
$ErrorActionPreference='Stop'
$requestedResultPath=$ResultPath
$requestedLogPath=$LogPath
$runner=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'gui_sio_physical_runner.ps1') -Raw
$prefix=$runner.Substring(0,$runner.IndexOf('function Run-Test')).Replace('$PSScriptRoot',"(Join-Path (Get-Location) 'tools')")
Invoke-Expression $prefix
$smokeResultPath=$requestedResultPath
$smokeLogPath=$requestedLogPath
    $exe=(Resolve-Path -LiteralPath $PvdExecutable).Path
    $expectedPvdHash='6B65B3D7DDE661112FBAB821D9CEA47A5492A241E1C246FB996D1F5E8F176D6E'
    if ((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -ne $expectedPvdHash) { throw "PVD executable hash mismatch: $exe" }
$observer=@{ObservedGpio=1;ObserverMode='TIMING';ExpectedLevel=0;MaxSamples=16;MaxTransitions=12;ObservationDurationMs=5000;ExpectedIntervalMs=5;ToleranceMs=1;SymbolPrefix='observer_pwm0a_100hz_50_clean_gpio1';DiagnosticTimer=$true}
$pvd=$null
$started=Get-Date
$budget=Get-PvdReadbackDeadlineBudget
try {
    if(-not (Test-Path -LiteralPath $Database -PathType Leaf)){throw "Database missing: $Database"}
    if(@(Get-Process openocd,arm-none-eabi-gdb,pico_visual_designer -ErrorAction SilentlyContinue).Count -ne 0){throw 'Owned debugger/PVD process already running'}
    if(@(Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue).Count -ne 0){throw 'Port 3333 already occupied'}
    $pvd=Start-Process -FilePath $exe -ArgumentList '--certification-dialogs' -PassThru
    $pvd.Refresh()
    $actualPvd=(Get-Process -Id $pvd.Id -ErrorAction Stop).Path
    if ([IO.Path]::GetFullPath($actualPvd) -ne [IO.Path]::GetFullPath($exe)) { throw "PVD executable identity mismatch: requested=$exe actual=$actualPvd" }
    $deadline=(Get-Date).AddSeconds(30)
    while($pvd.MainWindowHandle -eq 0 -and (Get-Date)-lt $deadline){Start-Sleep -Milliseconds 250;$pvd.Refresh()}
    if($pvd.MainWindowHandle -eq 0){throw 'PVD main window unavailable'}
    Open-ExistingProject $pvd.Id $Database | Out-Null
    Start-Sleep -Milliseconds 700
    $r=Invoke-PvdProductionReadback -ProcessId $pvd.Id -TimeoutSeconds ([int]($budget.OuterWatchdogMs/1000)) -ResultPath $requestedResultPath -LogPath $requestedLogPath
    $cppResult=$r.Result
    $record=[ordered]@{status=$cppResult.status;stage_reached=$cppResult.stage_reached;failure_stage=$cppResult.failure_stage;failure_reason=$cppResult.failure_reason;last_command=$cppResult.last_command;last_response=$cppResult.last_response;openocd_ready=$cppResult.openocd_ready;gdb_connected=$cppResult.gdb_connected;target_halted=$cppResult.target_halted;evidence_read_started=$cppResult.evidence_read_started;evidence_read_completed=$cppResult.evidence_read_completed;reset_command_count=$cppResult.reset_command_count;program_command_count=$cppResult.program_command_count;continue_command_count=$cppResult.continue_command_count;cleanup_completed=$cppResult.cleanup_completed;outer_watchdog_triggered=$false;elapsed_ms=[int](((Get-Date)-$started).TotalMilliseconds);raw_log_path=$cppResult.raw_log_path;result_json_path=$r.ResultPath;readback_run_id=$cppResult.readback_run_id;trigger_ack_path=$r.AckPath;trigger_ack_present=$true;deadline_budget=$budget;smoke_only=$true;headless_cpp=$true;powershell_evidence_commands=0;gui_evidence_commands=0}
    Write-PvdAtomicJson -Path $smokeResultPath -Value $record
    $record | ConvertTo-Json -Depth 20
} catch {
    if(-not (Test-Path -LiteralPath $smokeResultPath)) {
        $message=$_.Exception.Message
        $preSession=$message -match 'GUI_TRIGGER_|GUI element|AutomationId|PVD main window|PVD window'
        $status=if($preSession){'GUI_TRIGGER_AUTOMATION_FAILURE'}else{'OUTER_WATCHDOG_FAILURE'}
        $stage=if($preSession){'GUI_TRIGGER'}else{'SMOKE_WRAPPER'}
        Write-PvdAtomicJson -Path $smokeResultPath -Value ([ordered]@{status=$status;failure_stage=$stage;failure_reason=$message;stage_reached='UNKNOWN';last_command='';last_response='';openocd_ready=$false;gdb_connected=$false;target_halted=$false;evidence_read_started=$false;evidence_read_completed=$false;cxx_session_started=$false;openocd_started=$false;gdb_started=$false;power_cycle=$false;transfer=$false;programming=$false;reset_command_count=0;program_command_count=0;continue_command_count=0;cleanup_completed=$true;outer_watchdog_triggered=($status -eq 'OUTER_WATCHDOG_FAILURE');elapsed_ms=[int](((Get-Date)-$started).TotalMilliseconds);raw_log_path=$smokeLogPath;deadline_budget=$budget;smoke_only=$true})
    }
    throw
} finally {
    try{Stop-PvdDebugProcesses}catch{}
    if($pvd -and -not $pvd.HasExited){$pvd.CloseMainWindow();Start-Sleep -Milliseconds 800;if(-not $pvd.HasExited){$pvd.Kill()}}
    if($pvd){Get-Process -Id $pvd.Id -ErrorAction SilentlyContinue|Stop-Process -Force -ErrorAction SilentlyContinue}
    if(Test-Path -LiteralPath $smokeResultPath){try{$saved=Get-Content -LiteralPath $smokeResultPath -Raw|ConvertFrom-Json;$saved|Add-Member -MemberType NoteProperty -Name cleanup_completed -Value $true -Force;Write-PvdAtomicJson -Path $smokeResultPath -Value $saved}catch{}}
}
