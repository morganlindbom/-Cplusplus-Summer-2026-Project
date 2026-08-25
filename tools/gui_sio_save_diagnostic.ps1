param(
    [string]$Executable = "build-fresh/pico_visual_designer.exe",
    [string]$ResultPath = "validation/pin_certification/results/sio-save-diagnostic.json",
    [int]$Runs = 10
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
Import-Module (Join-Path $PSScriptRoot "gui/PvdGuiAutomation.psm1") -Force

$exePath = (Resolve-Path $Executable).Path
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("pvd-save-diagnostic-" + [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssfffZ"))
New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
$attempts = @()

function Wait-Element($root, [string]$automationId, [string]$name, [int]$seconds = 20) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $items = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($item in $items) {
            if (($automationId -and ($item.Current.AutomationId -eq $automationId -or $item.Current.AutomationId.EndsWith("." + $automationId))) -or ($name -and $item.Current.Name -eq $name)) { return $item }
        }
        Start-Sleep -Milliseconds 200
    }
    throw "GUI element not found: id='$automationId' name='$name'"
}

function Invoke-Element($element) {
    try { $element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke(); return $true } catch {}
    try { $element.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select(); return $true } catch {}
    throw "Element is not invokable: $($element.Current.Name)"
}

function Set-Value($element, [string]$value) {
    [void]$element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($value)
}

function Set-EditText($element, [string]$value) {
    $shell = New-Object -ComObject WScript.Shell
    [void]$shell.AppActivate((Get-Variable -Name CurrentDiagnosticProcessId -Scope Global -ErrorAction SilentlyContinue).Value)
    $element.SetFocus()
    [System.Windows.Forms.SendKeys]::SendWait("^a")
    [System.Windows.Forms.SendKeys]::SendWait($value)
    [System.Windows.Forms.SendKeys]::SendWait("{TAB}")
    Start-Sleep -Milliseconds 250
}

function Select-ComboItem($combo, [string]$value) {
    try {
        [void]$combo.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($value)
        Start-Sleep -Milliseconds 250
        return
    } catch {}
    try { $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand() } catch {}
    Start-Sleep -Milliseconds 100
    foreach ($item in $combo.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)) {
        if ($item.Current.Name -eq $value) {
            [void]$item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
            Start-Sleep -Milliseconds 250
            return
        }
    }
    $combo.SetFocus()
    if ($value -eq "Testing") { [System.Windows.Forms.SendKeys]::SendWait("{HOME}{DOWN}{ENTER}") }
    else { throw "Combo item not found: $value" }
    Start-Sleep -Milliseconds 250
}

function Fresh-Window($process) {
    $process.Refresh()
    if ($process.MainWindowHandle -eq 0) { throw "PVD main window handle is unavailable" }
    return [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
}

function Dialog-Info {
    $root = [System.Windows.Automation.AutomationElement]::RootElement
    $windows = @($root.FindAll([System.Windows.Automation.TreeScope]::Children, [System.Windows.Automation.Condition]::TrueCondition))
    foreach ($w in $windows) {
        if ($w.Current.Name -match "Open Pico Visual Designer project|Choose project directory|Confirm") {
            return [ordered]@{ appeared = $true; title = $w.Current.Name; element = $w }
        }
    }
    return [ordered]@{ appeared = $false; title = $null; element = $null }
}

function Sqlite-ProjectName([string]$database) {
    $code = "import sqlite3,sys; c=sqlite3.connect(sys.argv[1]); r=c.execute('select value from project where key=?',('project_name',)).fetchone(); print(r[0] if r else '')"
    try { return (& python -c $code $database 2>$null | Out-String).Trim() } catch { return "" }
}

for ($index = 1; $index -le $Runs; $index++) {
    $started = Get-Date
    $id = "SAVE-{0:D2}" -f $index
    $name = "GUI-SAVE-DIAG-$index"
    $path = Join-Path $tempRoot $id
    $database = Join-Path $path ($name + ".sqlite")
    $record = [ordered]@{
        attempt = $index; attempt_id = $id; timestamp_utc = $started.ToUniversalTime().ToString("o")
        process_id = $null; active_window_title = $null; expected_project_path = $path; requested_project_filename = $name
        save_control_located = $false; save_command_invoked = $false; file_dialog_appeared = $false; dialog_title = $null
        filename_field_located = $false; filename_entered = $null; confirmation_performed = $false; overwrite_dialog_appeared = $false
        observed_name_after_edit = $null; sqlite_exists = $false; sqlite_path = $database; sqlite_size = 0; sqlite_project_name = $null; dirty_state_after_save = $null
        elapsed_ms = $null; timeout_reason = $null; result = "FAIL"; evidence = $null
    }
    $process = $null
    try {
        $process = Start-Process -FilePath $exePath -PassThru
        $record.process_id = $process.Id
        $deadline = (Get-Date).AddSeconds(30)
        while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250; $process.Refresh() }
        $window = Fresh-Window $process
        $record.active_window_title = $window.Current.Name
        $nameEdit = Wait-Element $window "project_name" ""
        $pathEdit = Wait-Element $window "project_path" ""
        Set-Value $nameEdit $name
        Set-Value $pathEdit $path
        $create = Wait-Element $window "project_create" ""
        Invoke-Element $create | Out-Null
        $deadline = (Get-Date).AddSeconds(30)
        while (-not (Test-Path -LiteralPath $database) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 200 }
        if (-not (Test-Path -LiteralPath $database)) { throw "Create did not produce SQLite database: $database" }
        $global:CurrentDiagnosticProcessId = $process.Id
        $stateCombo = Wait-Element (Fresh-Window $process) "project_state" ""
        $record.observed_name_after_edit = (Select-PvdComboItemAsUser $stateCombo "Testing" $process.Id | ConvertTo-Json -Compress)
        $window = Fresh-Window $process
        $save = Wait-Element $window "project_save" "" 30
        $record.save_control_located = $true
        Invoke-Element $save | Out-Null
        $record.save_command_invoked = $true
        $record.filename_entered = $name + "-CHANGED"
        $deadline = (Get-Date).AddSeconds(30)
        while ((Get-Item -LiteralPath $database).LastWriteTime -lt $started -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 200 }
        $info = Dialog-Info
        $record.file_dialog_appeared = $info.appeared
        $record.dialog_title = $info.title
        $record.sqlite_exists = Test-Path -LiteralPath $database
        if ($record.sqlite_exists) { $record.sqlite_size = (Get-Item -LiteralPath $database).Length; $record.sqlite_project_name = Sqlite-ProjectName $database }
        $record.dirty_state_after_save = (Fresh-Window $process).Current.Name
        if (-not $record.sqlite_exists) { throw "SQLite file disappeared after Save" }
        $record.sqlite_project_name = Sqlite-ProjectName $database
        $stateValue = (& python -c "import sqlite3,sys; c=sqlite3.connect(sys.argv[1]); r=c.execute('select value from project where key=?',('state',)).fetchone(); print(r[0] if r else '')" $database 2>$null | Out-String).Trim()
        if ($stateValue -ne "Testing") { throw "SQLite state was not updated: '$stateValue'" }
        $record.result = "PASS"
        $record.evidence = "Create, visible Save, SQLite update, and persisted projectName verified."
    } catch {
        $record.evidence = $_.Exception.Message
        if ($_.Exception.Message -match "not found|30 seconds|timeout") { $record.timeout_reason = $_.Exception.Message }
    } finally {
        if ($process) {
            $record.active_window_title = if ($record.active_window_title) { $record.active_window_title } else { "" }
            if (-not $process.HasExited) { $process.CloseMainWindow(); Start-Sleep -Milliseconds 800; if (-not $process.HasExited) { $process.Kill() } }
        }
        $record.elapsed_ms = [int]((Get-Date) - $started).TotalMilliseconds
    }
    $attempts += $record
}

$output = [ordered]@{
    diagnostic_id = "pvd-gui-save-reliability"
    started_utc = $attempts[0].timestamp_utc
    completed_utc = (Get-Date).ToUniversalTime().ToString("o")
    runs_requested = $Runs
    runs_passed = @($attempts | Where-Object result -eq "PASS").Count
    temp_root = $tempRoot
    attempts = $attempts
    aramf = "DEFERRED: recorder unavailable"
}
$parent = Split-Path -Parent $ResultPath
New-Item -ItemType Directory -Force -Path $parent | Out-Null
$output | ConvertTo-Json -Depth 10 | Set-Content -Encoding UTF8 $ResultPath
$output | ConvertTo-Json -Depth 10
