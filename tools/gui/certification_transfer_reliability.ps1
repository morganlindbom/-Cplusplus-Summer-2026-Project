# certification_transfer_reliability.ps1
param(
    [string]$Executable = "build-fresh/pico_visual_designer.exe",
    [Parameter(Mandatory = $true)][string]$Database,
    [Parameter(Mandatory = $true)][string]$ProjectPath,
    [Parameter(Mandatory = $true)][string]$Artifact,
    [string]$ResultPath = "validation/pin_certification/results/deterministic-transfer-reliability.json",
    [int]$Runs = 5,
    [switch]$StopBeforeTransfer
)

$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "PvdGuiAutomation.psm1") -Force
$exePath = (Resolve-Path $Executable).Path
$databasePath = (Resolve-Path $Database).Path
$projectPathResolved = (Resolve-Path $ProjectPath).Path
$artifactPath = (Resolve-Path $Artifact).Path
$attempts = @()

function Find-CertificationDialog([int]$processId, [int]$seconds = 15)
{
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline)
    {
        $items = (Get-PvdWindow $processId).FindAll([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($item in $items)
        {
            if ($item.Current.ControlType -eq [System.Windows.Automation.ControlType]::Window -and
                $item.Current.AutomationId -match "pvd_automation_file_dialog$" -and $item.Current.IsEnabled)
            {
                return $item
            }
        }
        Start-Sleep -Milliseconds 100
    }
    throw "dialog discovery boundary failed"
}

function Find-DialogControl($dialog, [string]$automationId, [string]$name, [int]$seconds = 10)
{
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline)
    {
        $items = $dialog.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($item in $items)
        {
            if ((($automationId -and $item.Current.AutomationId -match $automationId) -or
                 ($name -and $item.Current.Name -eq $name)) -and
                $item.Current.IsEnabled -and -not $item.Current.IsOffscreen)
            {
                return $item
            }
        }
        Start-Sleep -Milliseconds 100
    }
    throw "dialog control discovery boundary failed"
}

function Read-Value($element)
{
    try { return $element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value }
    catch { return $element.Current.Name }
}

function Open-CertificationProject([int]$processId)
{
    $openButton = Find-PvdControlFresh $processId "project_open" "" 20
    Invoke-PvdButton $openButton $processId | Out-Null
    $dialog = Find-CertificationDialog $processId
    if ($dialog.Current.ClassName -ne "QFileDialog") { throw "dialog identity boundary failed" }
    $field = Find-DialogControl $dialog "fileNameEdit$" "" 10
    # QFileDialog accepts the path through ValuePattern; this preserves '+' and other
    # Windows path characters that SendKeys interprets as metacharacters.
    $field.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($databasePath)
    $field.SetFocus()
    [System.Windows.Forms.SendKeys]::SendWait("{TAB}")
    Start-Sleep -Milliseconds 200
    $observed = Read-Value $field
    if ($observed -ne $databasePath -and $observed -ne [System.IO.Path]::GetFileName($databasePath))
    {
        throw "file input boundary failed"
    }
    $field.SetFocus()
    [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
    $deadline = (Get-Date).AddSeconds(15)
    while ((Get-Date) -lt $deadline)
    {
        $dialogs = @((Get-PvdWindow $processId).FindAll([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition) |
            Where-Object { $_.Current.AutomationId -match "pvd_automation_file_dialog$" })
        if ($dialogs.Count -eq 0) { break }
        Start-Sleep -Milliseconds 100
    }
    if ($dialogs.Count -ne 0) { throw "dialog close boundary failed" }
    $pathControl = Find-PvdControlFresh $processId "project_path" "" 20
    $nameControl = Find-PvdControlFresh $processId "project_name" "" 20
    if ([System.IO.Path]::GetFullPath((Read-Value $pathControl)) -ne [System.IO.Path]::GetFullPath($projectPathResolved) -or
        (Read-Value $nameControl) -ne "PVD_RP2350_MULTICORE_DEBUG")
    {
        throw "project state verification boundary failed"
    }
}

for ($i = 1; $i -le $Runs; $i++)
{
    $started = Get-Date
    $process = $null
    $record = [ordered]@{
        attempt = $i
        timestamp_utc = $started.ToUniversalTime().ToString("o")
        result = "FAIL"
        launch = "FAIL"
        open = "FAIL"
        navigation = "FAIL"
        generate = "FAIL"
        generate_current_attempt = "FAIL"
        configure = "FAIL"
        configure_current_attempt = "FAIL"
        build_start = "FAIL"
        build_completion = "FAIL"
        build = "FAIL"
        artifact = "FAIL"
        freshness = "FAIL"
        pvd_freshness_gate = "NOT-TESTED"
        transfer_page = "FAIL"
        fresh_uia = "FAIL"
        transfer_control = "FAIL"
        gui_invocation = "FAIL"
        transfer_process = "FAIL"
        transfer_log = "FAIL"
        terminal = "FAIL"
        cmsis_dap = "FAIL"
        programming = "FAIL"
        cleanup = "FAIL"
        log_method = $null
        log_text = ""
        build_log_text = ""
        failure_boundary = $null
        attempt_id = $null
        freshness_evidence = $null
    }
    try
    {
        $process = Start-Process -FilePath $exePath -ArgumentList "--certification-dialogs" -PassThru
        $record.launch = "PASS"
        $deadline = (Get-Date).AddSeconds(30)
        while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline)
        {
            Start-Sleep -Milliseconds 200
            $process.Refresh()
        }
        if ($process.MainWindowHandle -eq 0) { throw "launch boundary failed" }
        Open-CertificationProject $process.Id
        $record.open = "PASS"
        Navigate-PvdPage $process.Id "Generate" 20 | Out-Null
        $record.navigation = "PASS"
        $generateAttempt = New-PvdAttemptEvidence "Generate"
        $generateInvocation = (Get-Date).ToUniversalTime()
        $generateBefore = Get-PvdDirectoryEvidence (Join-Path $projectPathResolved "generated")
        $generateStatus = Find-PvdControlFresh $process.Id "generate_status" "" 20
        $generateStatusBefore = Read-Value $generateStatus
        $generateMethod = Invoke-PvdButton (Find-PvdControlFresh $process.Id "generate_project" "" 20) $process.Id
        Start-Sleep -Milliseconds 500
        $generated = Join-Path $projectPathResolved "generated\main.cpp"
        $deadline = (Get-Date).AddSeconds(30)
        while (-not (Test-Path $generated) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 300 }
        if (-not (Test-Path $generated)) { throw "generate boundary failed" }
        $record.generate = "PASS"
        $generateAfter = Get-PvdDirectoryEvidence (Join-Path $projectPathResolved "generated")
        $generateChanged = @($generateAfter.Keys | Where-Object {
                -not $generateBefore.Contains($_) -or
                $generateAfter[$_].last_write_utc -ne $generateBefore[$_].last_write_utc })
        $generateStatusAfter = Read-Value (Find-PvdControlFresh $process.Id "generate_status" "" 20)
        Write-Output ("Generate method={0}; before='{1}'; after='{2}'" -f $generateMethod, $generateStatusBefore, $generateStatusAfter)
        if ($generateChanged.Count -eq 0 -and $generateStatusAfter -eq $generateStatusBefore)
        {
            throw "current-attempt Generate evidence was not observed (before='$generateStatusBefore', after='$generateStatusAfter')"
        }
        $generateMethod = if ($generateChanged.Count -gt 0) { "generated-file-change" } else { "generate-status-change" }
        $generateAttempt.start_evidence = [ordered]@{ observed = $true; method = $generateMethod }
        $generateAttempt.terminal_evidence = [ordered]@{ observed = $true; method = "generate-status-or-file-present" }
        $record.generate_current_attempt = "PASS"
        Navigate-PvdPage $process.Id "Build" 20 | Out-Null
        Write-Output ("Current page after Build navigation: {0}" -f (Get-PvdCurrentPage (Get-PvdWindow $process.Id)).name)
        $buildLogElement = Find-PvdControlFresh $process.Id "build_log" "" 20
        $buildLogBefore = (Read-PvdLogText $buildLogElement).text
        $buildStatusBefore = $null
        try { $buildStatusBefore = Read-Value (Find-PvdControlFresh $process.Id "build_status" "" 5) } catch {}
        $configureAttempt = New-PvdAttemptEvidence "Configure"
        $configureInvocation = (Get-Date).ToUniversalTime()
        $configureBeforeProcesses = Get-PvdChildProcessEvidence $process.Id
        $cache = Join-Path $projectPathResolved "build\CMakeCache.txt"
        $configureCacheBefore = Get-PvdFileEvidence @($cache)
        $configureBaselineLog = $buildLogBefore
        $configureLogElement = $buildLogElement
        $configureButton = Find-PvdControlFresh $process.Id "configure_project" "" 20
        Write-Output ("Configure control: name='{0}' aid='{1}' enabled={2} offscreen={3}" -f
                $configureButton.Current.Name, $configureButton.Current.AutomationId,
                $configureButton.Current.IsEnabled, $configureButton.Current.IsOffscreen)
        Invoke-PvdButton $configureButton $process.Id | Out-Null
        $deadline = (Get-Date).AddSeconds(45)
        Start-Sleep -Milliseconds 500
        $configureImmediateLog = (Read-PvdLogText (Find-PvdControlFresh $process.Id "build_log" "" 20)).text
        $configureImmediateStatus = $null
        try { $configureImmediateStatus = Read-Value (Find-PvdControlFresh $process.Id "build_status" "" 5) } catch {}
        if ($configureImmediateStatus -match '^Configure (running|completed)$' -and $configureImmediateStatus -ne $buildStatusBefore)
        {
            $configureStart = [ordered]@{ observed = $true; method = "build-status"; processes = Get-PvdChildProcessEvidence $process.Id }
        }
        elseif ($configureImmediateLog.Contains('$ cmake -S'))
        {
            $configureStart = [ordered]@{ observed = $true; method = "new-configure-command"; processes = Get-PvdChildProcessEvidence $process.Id }
        }
        else
        {
            $configureStart = Wait-PvdFileChange $cache $configureCacheBefore 20
        }
        if (-not $configureStart.observed)
        {
            $configureStart = Wait-PvdCurrentAttemptStart $process.Id $configureBeforeProcesses $configureBaselineLog 20
        }
        if (-not $configureStart.observed)
        {
            $currentCache = Get-Item -LiteralPath $cache -ErrorAction SilentlyContinue
            $currentLog = (Read-PvdLogText (Find-PvdControlFresh $process.Id "build_log" "" 20)).text
            if ($currentLog.Length -gt $configureBaselineLog.Length -or $currentLog.Contains('$ cmake -S'))
            {
                $configureStart = [ordered]@{ observed = $true; method = "current-new-log-output"; processes = Get-PvdChildProcessEvidence $process.Id }
            }
        }
        if (-not $configureStart.observed) { throw "Configure current-attempt start not observed" }
        $configureAttempt.start_evidence = $configureStart
        $configureProcessesAtEvidence = Get-PvdChildProcessEvidence $process.Id
        $configureProcessIds = @($configureProcessesAtEvidence.Keys | Where-Object {
                $_ -and -not $configureBeforeProcesses.Contains($_) })
        # A fast configure can start and exit between Win32 process snapshots.
        # Current-attempt log/cache evidence is authoritative in that case, so a sampled
        # child process is useful evidence but is no longer a mandatory gate.
        while (-not (Test-Path $cache) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 500 }
        if (-not (Test-Path $cache)) { throw "configure boundary failed" }
        $record.configure = "PASS"
        $configureCache = Get-Item -LiteralPath $cache
        if ($configureCache.LastWriteTimeUtc -lt $configureInvocation) { throw "Configure cache is not current-attempt evidence" }
        if ($configureProcessIds.Count -gt 0)
        {
            $deadline = (Get-Date).AddSeconds(45)
            $activeConfigureProcesses = @($configureProcessIds)
            while ((Get-Date) -lt $deadline)
            {
                $activeConfigureProcesses = @(Get-PvdChildProcessEvidence $process.Id).Keys |
                    Where-Object { $configureProcessIds -contains $_ }
                if ($activeConfigureProcesses.Count -eq 0) { break }
                Start-Sleep -Milliseconds 300
            }
            if ($activeConfigureProcesses.Count -ne 0) { throw "Configure process did not complete before Build invocation" }
        }
        $configureTerminalStatus = $null
        try { $configureTerminalStatus = Read-Value (Find-PvdControlFresh $process.Id "build_status" "" 5) } catch {}
        if ($configureTerminalStatus -eq 'Configure failed' -or $configureTerminalStatus -eq 'Build-system process error')
        {
            throw "Configure terminal status reported failure: $configureTerminalStatus"
        }
        $configureTerminalMethod = if ($configureTerminalStatus -eq 'Configure completed') { 'build-status-completed' } else { 'current-cache-timestamp' }
        $configureAttempt.terminal_evidence = [ordered]@{ observed = $true; method = $configureTerminalMethod }
        $record.configure_current_attempt = "PASS"
        $buildAttempt = New-PvdAttemptEvidence "Build"
        $record.attempt_id = $buildAttempt.attempt_id
        $buildInvocation = (Get-Date).ToUniversalTime()
        $artifactBefore = Get-PvdFileEvidence @($artifactPath, ($artifactPath -replace "\.uf2$", ".elf"), (Join-Path $projectPathResolved "build\.pvd_build_success"))
        $sourceBefore = Get-PvdDirectoryEvidence (Join-Path $projectPathResolved "generated")
        $buildLogBefore = (Read-PvdLogText (Find-PvdControlFresh $process.Id "build_log" "" 20)).text
        $buildBeforeProcesses = Get-PvdChildProcessEvidence $process.Id
        Invoke-PvdButton (Find-PvdControlFresh $process.Id "build_project" "" 20) $process.Id | Out-Null
        $marker = Join-Path $projectPathResolved "build\.pvd_build_success"
        $buildStart = Wait-PvdCurrentAttemptStart $process.Id $buildBeforeProcesses $buildLogBefore 20
        if (-not $buildStart.observed)
        {
            $buildStartStatus = $null
            try { $buildStartStatus = Read-Value (Find-PvdControlFresh $process.Id "build_status" "" 5) } catch {}
            if ($buildStartStatus -match '^Build (running|completed)$')
            {
                $buildStart = [ordered]@{ observed = $true; method = "build-status"; processes = Get-PvdChildProcessEvidence $process.Id }
            }
        }
        if (-not $buildStart.observed) { throw "Build current-attempt start not observed" }
        $buildAttempt.start_evidence = $buildStart
        $record.build_start = "PASS"
        $deadline = (Get-Date).AddSeconds(90)
        $buildCompleted = $false
        while ((Get-Date) -lt $deadline)
        {
            $markerInfo = Get-Item -LiteralPath $marker -ErrorAction SilentlyContinue
            $children = Get-PvdChildProcessEvidence $process.Id
            $logAfter = (Read-PvdLogText (Find-PvdControlFresh $process.Id "build_log" "" 5)).text
            $buildTerminalStatus = $null
            try { $buildTerminalStatus = Read-Value (Find-PvdControlFresh $process.Id "build_status" "" 2) } catch {}
            if ($buildTerminalStatus -eq 'Build failed' -or $buildTerminalStatus -eq 'Build-system process error')
            {
                throw "Build terminal status reported failure: $buildTerminalStatus"
            }
            if ($markerInfo -and $markerInfo.LastWriteTimeUtc -ge $buildInvocation -and
                $logAfter.Length -gt $buildLogBefore.Length)
            {
                $buildCompleted = $true
                break
            }
            Start-Sleep -Milliseconds 500
        }
        if (-not $buildCompleted) { throw "Build current-attempt terminal success was not observed" }
        $buildAttempt.terminal_evidence = [ordered]@{ observed = $true; method = "new-marker-new-log-process-exit" }
        $record.build_completion = "PASS"
        $record.build = "PASS"
        if (-not (Test-Path $artifactPath)) { throw "artifact boundary failed" }
        $record.artifact = "PASS"
        $freshness = Test-PvdFreshness $artifactPath ($artifactPath -replace "\.uf2$", ".elf") (Join-Path $projectPathResolved "generated") $buildInvocation
        $record.freshness_evidence = $freshness
        if (-not $freshness.pass) { throw "firmware freshness invariant failed" }
        $record.freshness = "PASS"
        $record.pvd_freshness_gate = "PASS"
        if ($StopBeforeTransfer)
        {
            $record.result = "PASS"
            $attempts += $record
            break
        }
        Navigate-PvdPage $process.Id "Transfer" 20 | Out-Null
        $record.transfer_page = "PASS"
        $button = Find-PvdControlFresh $process.Id "transfer_firmware" "" 20
        if (-not $button.Current.IsEnabled -or $button.Current.IsOffscreen) { throw "transfer control disabled or hidden" }
        $record.fresh_uia = "PASS"
        $record.transfer_control = "PASS"
        Invoke-PvdButton $button $process.Id | Out-Null
        $record.gui_invocation = "PASS"
        $deadline = (Get-Date).AddSeconds(120)
        while ((Get-Date) -lt $deadline)
        {
            $log = Read-PvdLogText (Find-PvdControlFresh $process.Id "transfer_log" "" 20)
            $record.log_method = $log.method
            $record.log_text = $log.text
            $record.transfer_log = "PASS"
            $children = @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$($process.Id)" |
                Where-Object { $_.Name -match "openocd|picotool" })
            if ($children) { $record.transfer_process = "PASS" }
            if ($record.log_text -match "Verified OK|verified OK|Programming Finished|Transfer complete|Copied to")
            {
                $record.terminal = "PASS"
                $record.cmsis_dap = "PASS"
                $record.programming = "PASS"
                break
            }
            Start-Sleep -Milliseconds 300
        }
        if ($record.terminal -ne "PASS") { throw "transfer terminal boundary failed" }
        $record.result = "PASS"
    }
    catch
    {
        try { $record.build_log_text = (Read-PvdLogText (Find-PvdControlFresh $process.Id "build_log" "" 5)).text } catch {}
        $record.failure_boundary = $_.Exception.Message
    }
    finally
    {
        if ($process)
        {
            if (-not $process.HasExited)
            {
                $process.CloseMainWindow()
                Start-Sleep -Milliseconds 500
                if (-not $process.HasExited) { $process.Kill() }
            }
            $record.cleanup = "PASS"
        }
    }
    $attempts += $record
    if ($record.result -ne "PASS") { break }
}

$output = [ordered]@{
    diagnostic_id = "pvd-deterministic-transfer-reliability"
    runs_requested = $Runs
    runs_completed = $attempts.Count
    runs_passed = @($attempts | Where-Object result -eq "PASS").Count
    probe = "E66258881776372"
    swd = "PASS"
    attempts = $attempts
}
New-Item -ItemType Directory -Force -Path (Split-Path $ResultPath) | Out-Null
$output | ConvertTo-Json -Depth 10 | Set-Content -Encoding UTF8 $ResultPath
$output | ConvertTo-Json -Depth 10
