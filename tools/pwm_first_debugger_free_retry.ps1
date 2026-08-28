$ErrorActionPreference = 'Stop'
$runnerPath = Join-Path $PSScriptRoot 'gui_sio_physical_runner.ps1'
$runnerText = Get-Content -LiteralPath $runnerPath -Raw
$runMarker = 'function Run-Test'
$prefix = $runnerText.Substring(0, $runnerText.IndexOf($runMarker)).Replace('$PSScriptRoot', "(Join-Path (Get-Location) 'tools')")
Invoke-Expression $prefix
$exePath = (Resolve-Path (Join-Path (Get-Location) 'build-codex-fresh/pico_visual_designer.exe')).Path

# Some Qt accessibility nodes have no AutomationId. Keep this retry's
# rediscovery path null-safe without changing the product or timing flow.
function Wait-Element($root, [string]$automationId, [string]$name, [int]$seconds = 20) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        foreach ($item in @($root.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition))) {
            $current = $item.Current
            if (-not $current) { continue }
            $id = [string]$current.AutomationId
            $itemName = [string]$current.Name
            if (($automationId -and ($id -eq $automationId -or ($id -and $id.EndsWith('.' + $automationId)))) -or ($name -and $itemName -eq $name)) { return $item }
        }
        Start-Sleep -Milliseconds 150
    }
    throw "GUI element not found: id='$automationId' name='$name'"
}

function Find-LivePvdElement([int]$processId, [string]$automationId, [int]$seconds = 12) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $root = Get-PvdWindow $processId
        foreach ($item in @($root.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition))) {
            $current = $item.Current
            if (-not $current) { continue }
            $id = [string]$current.AutomationId
            $statusMatch = $automationId -eq 'selection_status' -and (($current.Name -match '^(Selection status:|Selected:)') -or ($current.HelpText -match '^Selected:') -or ($current.Description -match '^Selected:'))
            if (($id -eq $automationId -or ($id -and $id.EndsWith('.' + $automationId)) -or $statusMatch) -and $current.IsEnabled -and -not $current.IsOffscreen -and $current.BoundingRectangle.Width -gt 0 -and $current.BoundingRectangle.Height -gt 0) { return (,$item) }
        }
        Start-Sleep -Milliseconds 150
    }
    throw "Live GUI element not found after rebuild: id='$automationId'"
}

$sourceProject = 'C:\Users\morga\AppData\Local\Temp\pvd-pwm-first-100hz-50-retry2'
$sourceDb = Join-Path $sourceProject 'PWM_FIRST_100HZ_50_RETRY2.sqlite'
$stamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$projectPath = Join-Path ([IO.Path]::GetTempPath()) ("pvd-pwm-first-100hz-50-clean-" + $stamp)
$projectName = 'PWM_FIRST_100HZ_50_CLEAN'
$database = Join-Path $projectPath ($projectName + '.sqlite')
$resultPath = Join-Path (Join-Path (Get-Location) 'validation/pin_certification/results') ("pwm-first-100hz-50-debugger-free-" + $stamp + '.json')
$process = $null
$observer = @{
    ObservedGpio = 1; ObserverMode = 'TIMING'; ExpectedLevel = 0; MaxSamples = 16
    MaxTransitions = 12; ObservationDurationMs = 5000; ExpectedIntervalMs = 5
    ToleranceMs = 1; SymbolPrefix = 'observer_pwm0a_100hz_50_clean_gpio1'; DiagnosticTimer = $true
}

function Read-ComboValue([int]$ProcessId, [string]$Id) {
    return Get-LiveControlText (Find-LivePvdElement $ProcessId $Id 20)
}

function Read-IntControl([int]$ProcessId, [string]$Id) {
    return Get-PvdNumericValue (Find-LivePvdElement $ProcessId $Id 20)
}

function Set-BoolControl([int]$ProcessId, [string]$Id, [bool]$Want) {
    $control = Find-LivePvdElement $ProcessId $Id 20
    $pattern = $control.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern)
    $desired = if ($Want) { [System.Windows.Automation.ToggleState]::On } else { [System.Windows.Automation.ToggleState]::Off }
    if ($pattern.Current.ToggleState -ne $desired) { $pattern.Toggle(); Start-Sleep -Milliseconds 700 }
    $fresh = Find-LivePvdElement $ProcessId $Id 20
    $state = $fresh.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Current.ToggleState
    if ($state -ne $desired) { throw "Boolean control $Id did not commit desired state $Want" }
    return $fresh
}

function Set-IntControl([int]$ProcessId, [string]$Id, [int]$Want) {
    $control = Find-LivePvdElement $ProcessId $Id 20
    try { $control.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue([string]$Want) } catch {
        Focus-PvdElement $control $ProcessId
        [System.Windows.Forms.SendKeys]::SendWait('^a' + [string]$Want + '{ENTER}{TAB}')
    }
    Start-Sleep -Milliseconds 700
    $fresh = Find-LivePvdElement $ProcessId $Id 20
    if ((Get-PvdNumericValue $fresh) -ne $Want) { throw "$Id did not commit $Want" }
    return $fresh
}

function Get-PvdStatus([int]$ProcessId, [string]$Id) {
    return Get-LiveControlText (Find-LivePvdElement $ProcessId $Id 10)
}

try {
    if (-not (Test-Path -LiteralPath $sourceDb -PathType Leaf)) { throw "Retry2 database missing: $sourceDb" }
    New-Item -ItemType Directory -Force -Path $projectPath | Out-Null
    Copy-Item -LiteralPath $sourceDb -Destination $database
    $process = Start-Process -FilePath $exePath -ArgumentList '--certification-dialogs' -PassThru
    $deadline = (Get-Date).AddSeconds(30)
    while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250; $process.Refresh() }
    if ($process.MainWindowHandle -eq 0) { throw 'PVD main window did not appear' }
    Open-ExistingProject $process.Id $database | Out-Null

    # Reacquire the persisted, real GUI owners from the Settings page.  This
    # is intentionally read/verify first; the copied project already contains
    # the exact authorized PWM0A and SIO selections.
    Select-Workflow (Get-PvdWindow $process.Id) 'Settings' $process.Id | Out-Null
    $settingsRows = @((Get-PvdWindow $process.Id).FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    $row0 = $settingsRows | Where-Object { $_.Current.Name -match '^Pin 1.*PWM0 ?A$' } | Select-Object -First 1
    if (-not $row0) { throw "Persisted GUI owner row missing for Pin 1 / GPIO0 / PWM0A: $($settingsRows.Current.Name -join ' | ')" }
    $rr = $row0.Current.BoundingRectangle
    [PvdMouse]::Click([int]($rr.X + $rr.Width / 2), [int]($rr.Y + $rr.Height / 2)); Start-Sleep -Milliseconds 700
    $status0 = (Get-LiveControlText (Find-LivePvdElement $process.Id 'selection_status' 20)) -replace '^Selection status:\s*', ''
    if ($status0 -notmatch '^Selected: Pin 1 / GPIO0 / PWM0 A$') { throw "PWM0A owner status mismatch: $status0" }
    $owner0 = [ordered]@{ PhysicalPin=1;Gpio=0;Function='PWM0A';OwnerVerified=$true;SelectionStatus=$status0;FunctionVerified=$true;ModelVerified=$true }
    $settings0 = Find-LivePvdElement $process.Id 'setting_enabled' 20
    $enabled0 = Get-PvdToggleState $settings0
    $frequency0 = Read-IntControl $process.Id 'setting_frequency_hz'
    $duty0 = Read-IntControl $process.Id 'setting_duty_percent'
    if ($enabled0 -ne [System.Windows.Automation.ToggleState]::On -or $frequency0 -ne 100 -or $duty0 -ne 50) {
        throw "PWM0A settings mismatch: enabled=$enabled0 frequency=$frequency0 duty=$duty0"
    }
    $row1 = $settingsRows | Where-Object { $_.Current.Name -match '^Pin 2.*SIO$' } | Select-Object -First 1
    if (-not $row1) { throw "Persisted GUI owner row missing for Pin 2 / GPIO1 / SIO" }
    $rr = $row1.Current.BoundingRectangle
    [PvdMouse]::Click([int]($rr.X + $rr.Width / 2), [int]($rr.Y + $rr.Height / 2)); Start-Sleep -Milliseconds 700
    $status1 = (Get-LiveControlText (Find-LivePvdElement $process.Id 'selection_status' 20)) -replace '^Selection status:\s*', ''
    if ($status1 -notmatch '^Selected: Pin 2 / GPIO1 / SIO$') { throw "SIO owner status mismatch: $status1" }
    $owner1 = [ordered]@{ PhysicalPin=2;Gpio=1;Function='SIO';OwnerVerified=$true;SelectionStatus=$status1;FunctionVerified=$true;ModelVerified=$true }
    $direction = Read-ComboValue $process.Id 'setting_direction'
    $pull = Read-ComboValue $process.Id 'setting_pull'
    if ($direction -ne 'Input' -or $pull -ne 'None') { throw "GPIO1 SIO settings mismatch: direction=$direction pull=$pull" }
    $owner1Direction = Read-ComboValue $process.Id 'setting_direction'
    if ($owner1Direction -ne 'Input') { throw "GPIO1 direction mismatch: $owner1Direction" }
    $row0 = @((Get-PvdWindow $process.Id).FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and $_.Current.Name -match '^Pin 1.*PWM0 ?A$' }) | Select-Object -First 1
    if (-not $row0) { throw 'PWM0A owner row disappeared during roundtrip' }
    $rr = $row0.Current.BoundingRectangle
    [PvdMouse]::Click([int]($rr.X + $rr.Width / 2), [int]($rr.Y + $rr.Height / 2)); Start-Sleep -Milliseconds 700
    $status0 = (Get-LiveControlText (Find-LivePvdElement $process.Id 'selection_status' 20)) -replace '^Selection status:\s*', ''
    if ($status0 -notmatch '^Selected: Pin 1 / GPIO0 / PWM0 A$') { throw "PWM0A roundtrip status mismatch: $status0" }
    if ((Read-IntControl $process.Id 'setting_frequency_hz') -ne 100 -or (Read-IntControl $process.Id 'setting_duty_percent') -ne 50) { throw 'PWM0A settings changed during owner roundtrip' }
    Save-CurrentProject (Get-PvdWindow $process.Id) $process.Id $projectPath | Out-Null

    $generateInvocation = [DateTime]::UtcNow
    Select-Workflow (Get-PvdWindow $process.Id) 'Generate' $process.Id | Out-Null
    Invoke-Element (Find-LivePvdElement $process.Id 'generate_project' 20) | Out-Null
    $status = ''
    $deadline = (Get-Date).AddSeconds(120)
    while ((Get-Date) -lt $deadline) {
        $status = Get-PvdStatus $process.Id 'generate_status'
        if ($status -match 'Generate completed') { break }
        if ($status -match 'Generate failed|error|failed') { throw "Generate failed: $status" }
        Start-Sleep -Milliseconds 300
    }
    if ($status -notmatch 'Generate completed') { throw "Generate timeout: $status" }
    $resolved = Resolve-PvdGeneratedProject -ProjectPath $projectPath -TestId 'PWM_FIRST_100HZ_50_CLEAN'
    Assert-CurrentFile $resolved.MainCpp $generateInvocation 'Generate' | Out-Null
    $injection = Add-PvdGeneratedObserver -MainCpp $resolved.MainCpp -Observer $observer
    $sourceAudit = Test-PvdObserverSource -MainCpp $resolved.MainCpp -Observer $observer
    if (-not $sourceAudit.Pass) { throw 'Observer source audit failed' }

    $configureInvocation = [DateTime]::UtcNow
    Select-Workflow (Get-PvdWindow $process.Id) 'Build' $process.Id | Out-Null
    Invoke-Element (Find-LivePvdElement $process.Id 'configure_project' 20) | Out-Null
    $status = ''
    $deadline = (Get-Date).AddSeconds(180)
    while ((Get-Date) -lt $deadline) {
        $status = Get-PvdStatus $process.Id 'build_status'
        if ($status -eq 'Configure completed') { break }
        if ($status -match 'Configure failed|Build-system process error') { throw "Configure failed: $status" }
        Start-Sleep -Milliseconds 400
    }
    if ($status -ne 'Configure completed') { throw "Configure timeout: $status" }
    Assert-CurrentFile (Join-Path $projectPath 'build/CMakeCache.txt') $configureInvocation 'Configure' | Out-Null

    $buildInvocation = [DateTime]::UtcNow
    $artifacts = Resolve-PvdBuildArtifacts $projectPath 'PWM_FIRST_100HZ_50_CLEAN'
    Invoke-Element (Find-LivePvdElement $process.Id 'build_project' 20) | Out-Null
    $status = ''
    $deadline = (Get-Date).AddSeconds(360)
    while ((Get-Date) -lt $deadline) {
        $status = Get-PvdStatus $process.Id 'build_status'
        if ($status -match 'Build completed') { break }
        if ($status -match 'Build failed|Build-system process error') { throw "Build failed: $status" }
        Start-Sleep -Milliseconds 500
    }
    if ($status -ne 'Build completed') { throw "Build timeout: $status" }
    $marker = Assert-CurrentFile (Join-Path $projectPath 'build/.pvd_build_success') $buildInvocation 'Build'
    $elfEvidence = Assert-CurrentFile $artifacts.elf_path $buildInvocation 'ELF'
    $uf2Evidence = Assert-CurrentFile $artifacts.uf2_path $buildInvocation 'UF2'
    $observer.ArtifactIdentityPass = $true

    # This function performs exactly: normal Transfer, autonomous wait, then
    # readback-only attach/halt. No GDB/OpenOCD is started during the wait.
    $target = Invoke-PvdObserverTargetRun -ProcessId $process.Id -Uf2 $artifacts.uf2_path -Observer $observer -MeasurementWaitMs 500
    if (-not $target.Observer.Complete) { throw 'Autonomous observer capture incomplete' }
    if ($target.Observer.Overflow) { throw 'Autonomous observer overflowed' }
    if ($target.Observer.TransitionCount -ne 12) { throw "Expected 12 transitions, got $($target.Observer.TransitionCount)" }
    $rawObserverLog = [string]$target.ObserverRawText
    if ($observer.DiagnosticTimer) {
        $phaseARawPath = Join-Path (Join-Path (Get-Location) 'validation/pin_certification/results') ("pwm-transfer-timer-diagnostic-" + $stamp + '.log')
        $rawObserverLog | Set-Content -LiteralPath $phaseARawPath -Encoding UTF8
        $hashElf = (Get-FileHash -LiteralPath $artifacts.elf_path -Algorithm SHA256).Hash
        $hashUf2 = (Get-FileHash -LiteralPath $artifacts.uf2_path -Algorithm SHA256).Hash
        $output = [ordered]@{ test='PWM transfer timer diagnostic'; attempt_id=$stamp; project_path=$projectPath; generated_root=$resolved.GeneratedRoot; artifact=@{elf=$artifacts.elf_path;uf2=$artifacts.uf2_path;elf_sha256=$hashElf;uf2_sha256=$hashUf2;marker=$marker}; observer=$target.Observer; raw_log=$phaseARawPath; raw_gdb_log=$rawObserverLog; measurement_phase=$target.MeasurementPhase; transfer=$target.Transfer; physical_result='NOT CERTIFIED'; diagnostic_only=$true }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $resultPath) | Out-Null
        $output | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $resultPath -Encoding UTF8
        $output | ConvertTo-Json -Depth 20
        return
    }
    $classification = Get-PvdPwmTimingClassification -States ([UInt64[]]$target.Observer.TransitionStates) -TimestampsUs ([UInt64[]]$target.Observer.TransitionTimesUs)
    $periodPass = $classification.AllWithinTolerance -and $classification.MinPeriodMs -ge 9.5 -and $classification.MaxPeriodMs -le 10.5
    $highPass = $classification.MinHighMs -ge 4.5 -and $classification.MaxHighMs -le 5.5
    $lowPass = $classification.MinLowMs -ge 4.5 -and $classification.MaxLowMs -le 5.5
    $frequencyPass = $classification.FrequencyHz -ge 95 -and $classification.FrequencyHz -le 105
    $dutyPass = $classification.DutyPercent -ge 45 -and $classification.DutyPercent -le 55
    if (-not ($periodPass -and $highPass -and $lowPass -and $frequencyPass -and $dutyPass)) { throw "PWM timing outside declared tolerance: $($classification | ConvertTo-Json -Compress)" }
    $hashElf = (Get-FileHash -LiteralPath $artifacts.elf_path -Algorithm SHA256).Hash
    $hashUf2 = (Get-FileHash -LiteralPath $artifacts.uf2_path -Algorithm SHA256).Hash
    $output = [ordered]@{
        test='PWM0A GPIO0 -> GPIO1 SIO'; attempt_id=$stamp; project_path=$projectPath; generated_root=$resolved.GeneratedRoot
        artifact=@{ elf=$artifacts.elf_path; uf2=$artifacts.uf2_path; elf_sha256=$hashElf; uf2_sha256=$hashUf2; marker=$marker }
        owners=@{ gpio0_pwm0a=$owner0; gpio1_sio=$owner1; gpio1_direction=$owner1Direction }
        settings=@{ enabled=$enabled0.ToString(); frequency_hz=$frequency0; duty_percent=$duty0 }
        observer=@{ prefix=$observer.SymbolPrefix; injection=$injection; source_audit=$sourceAudit; result=$target.Observer; measurement_phase=$target.MeasurementPhase }
        classification=$classification; acceptance=@{ period=$periodPass; high=$highPass; low=$lowPass; frequency=$frequencyPass; duty=$dutyPass; physical_pass=$true }
        transfer=$target.Transfer; debugger_stimulus=$false; debug_cleanup='readback-only; debug_stop finally'; new_firmware=$true
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $resultPath) | Out-Null
    $output | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $resultPath -Encoding UTF8
    $output | ConvertTo-Json -Depth 20
} catch {
    $failure = [ordered]@{ test='PWM0A GPIO0 -> GPIO1 SIO'; attempt_id=$stamp; project_path=$projectPath; error=$_.Exception.Message; physical_result='NOT CERTIFIED' }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $resultPath) | Out-Null
    $failure | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $resultPath -Encoding UTF8
    throw
} finally {
    Stop-PvdDebugProcesses
    if ($process -and -not $process.HasExited) {
        [void]$process.CloseMainWindow(); Start-Sleep -Milliseconds 800
        if (-not $process.HasExited) { [void]$process.Kill() }
    }
    if ($process) { Get-Process -Id $process.Id -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue }
}
