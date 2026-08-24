# tools/gui_persistence_proof.ps1
param(
    [string]$Executable = "build-fresh/pico_visual_designer.exe",
    [string]$ResultPath = "certification/results/gui-persistence-proof.json",
    [int]$TimeoutSeconds = 30
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms

$stages = [ordered]@{}
foreach ($stage in @("Launch", "Create", "ChangeSetting", "Dirty", "Save", "Reload", "Restore")) {
    $stages[$stage] = [ordered]@{ status = "NOT-TESTED"; reason = "Not reached" }
}
$process = $null
$projectPath = Join-Path ([System.IO.Path]::GetTempPath()) "pvd-persistence-proof"
$databasePath = Join-Path $projectPath "GUI_PERSISTENCE_PROOF.sqlite"

function Set-Stage([string]$name, [string]$status, [string]$reason) {
    $stages[$name] = [ordered]@{ status = $status; reason = $reason }
}

function Wait-Element($root, [string]$automationId, [string]$name, [int]$seconds = 15) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $items = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($item in $items) {
            if (($automationId -and ($item.Current.AutomationId -eq $automationId -or $item.Current.AutomationId.EndsWith("." + $automationId))) -or ($name -and $item.Current.Name -eq $name)) {
                return $item
            }
        }
        Start-Sleep -Milliseconds 250
    }
    throw "GUI control not found: id='$automationId', name='$name'"
}

function Set-Value($element, [string]$value) {
    $pattern = $element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
    $pattern.SetValue($value)
}

function Wait-Window([string]$name, [int]$seconds = 15) {
    $deadline = (Get-Date).AddSeconds($seconds)
    $condition = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $name)
    while ((Get-Date) -lt $deadline) {
        $window = [System.Windows.Automation.AutomationElement]::RootElement.FindFirst([System.Windows.Automation.TreeScope]::Children, $condition)
        if ($window) { return $window }
        Start-Sleep -Milliseconds 250
    }
    throw "Window not found: $name"
}

function Wait-ProjectDialog([int]$processId, [string]$mainWindowName, [int]$seconds = 15) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        foreach ($candidate in [System.Windows.Automation.AutomationElement]::RootElement.FindAll([System.Windows.Automation.TreeScope]::Children, [System.Windows.Automation.Condition]::TrueCondition)) {
            if ($candidate.Current.Name -and $candidate.Current.Name -ne $mainWindowName -and
                ($candidate.Current.ProcessId -eq $processId -or $candidate.Current.ControlType -eq [System.Windows.Automation.ControlType]::Window)) {
                return $candidate
            }
        }
        Start-Sleep -Milliseconds 250
    }
    throw "Project file dialog was not found"
}

function Invoke-Element($element) {
    $element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
}

function Select-ComboValue($combo, [string]$value) {
    try {
        $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
        foreach ($item in $combo.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)) {
            if ($item.Current.Name -eq $value) {
                $item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
                return
            }
        }
    } catch {}
    $combo.SetFocus()
    if ($value -eq "C") {
        [System.Windows.Forms.SendKeys]::SendWait("{HOME}{DOWN}{ENTER}")
        return
    }
    throw "Combo value not found: $value"
}

function Toggle-CheckBox($checkBox) {
    $checkBox.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Toggle()
}

try {
    if (Test-Path $projectPath) { Remove-Item -LiteralPath $projectPath -Recurse -Force | Out-Null }
    New-Item -ItemType Directory -Force -Path $projectPath | Out-Null
    $exe = (Resolve-Path $Executable).Path
    $process = Start-Process -FilePath $exe -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 250
        $process.Refresh()
    }
    if ($process.MainWindowHandle -eq 0) { throw "PVD MainWindow did not appear" }
    $window = [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
    Set-Stage Launch PASS "Real PVD executable started"

    $name = Wait-Element $window "project_name" ""
    $path = Wait-Element $window "project_path" ""
    Set-Value $name "GUI_PERSISTENCE_PROOF"
    Set-Value $path $projectPath
    Invoke-Element (Wait-Element $window "project_create" "")
    Wait-Element $window "project_dirty_status" "Project saved in: $projectPath" | Out-Null
    Set-Stage Create PASS "Project created through the visible PVD Create Project control"

    Toggle-CheckBox (Wait-Element $window "project_runtime_diagnostics" "")
    Set-Stage ChangeSetting PASS "Runtime diagnostics changed through the visible project settings control"
    $dirty = Wait-Element $window "project_dirty_status" ""
    if ($dirty.Current.Name -ne "Unsaved project changes.") { throw "Dirty state was not visible after project setting change; actual='$($dirty.Current.Name)'" }
    Set-Stage Dirty PASS "Central dirty state was visible in the project status control"

    Invoke-Element (Wait-Element $window "project_save" "")
    Wait-Element $window "project_dirty_status" "Project saved in: $projectPath" | Out-Null
    Set-Stage Save PASS "Project saved through the visible PVD Save Project control"

    Invoke-Element (Wait-Element $window "project_open" "")
    Start-Sleep -Milliseconds 500
    [System.Windows.Forms.SendKeys]::SendWait("^a")
    [System.Windows.Forms.SendKeys]::SendWait($databasePath)
    [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
    Start-Sleep -Milliseconds 750
    Wait-Element $window "project_dirty_status" "Project saved." | Out-Null
    Set-Stage Reload PASS "Project reopened through the visible PVD Open Project workflow"

    $diagnostics = Wait-Element $window "project_runtime_diagnostics" ""
    $toggleState = $diagnostics.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Current.ToggleState
    if ($toggleState -ne [System.Windows.Automation.ToggleState]::Off) { throw "Runtime diagnostics change was not restored after reload" }
    Set-Stage Restore PASS "Runtime diagnostics disabled state was restored after reload"
}
catch {
    foreach ($key in $stages.Keys) {
        if ($stages[$key].status -eq "NOT-TESTED") {
            Set-Stage $key FAIL $_.Exception.Message
            break
        }
    }
}
finally {
    if ($process -and -not $process.HasExited) {
        $process.CloseMainWindow()
        Start-Sleep -Milliseconds 500
        if (-not $process.HasExited) { $process.Kill() }
    }
    $overall = if (($stages.Values | Where-Object { $_.status -eq "FAIL" }).Count -gt 0) { "FAIL" } elseif (($stages.Values | Where-Object { $_.status -eq "PASS" }).Count -eq $stages.Count) { "PASS-GUI" } else { "NOT-TESTED" }
    $result = [ordered]@{
        caseId = "gui-persistence-proof"
        database = $databasePath
        stages = $stages
        overall = $overall
        processModel = "separate PVD executable via Windows UI Automation"
        fullPinsCampaign = "NOT-STARTED"
    }
    $parent = Split-Path -Parent $ResultPath
    if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
    $result | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $ResultPath
    Write-Output ($result | ConvertTo-Json -Depth 8)
}
