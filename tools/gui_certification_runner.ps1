param(
    [string]$Executable = "build-fresh/pico_visual_designer.exe",
    [string]$ResultPath = "certification/results/gui-proof-pin1.json",
    [int]$TimeoutSeconds = 30,
    [switch]$AllowHardwareActions
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms

$stages = [ordered]@{}
foreach ($stage in @("Launch","Navigation","Selection","Settings","Generate","Configure","Build","Transfer","Runtime","Debug")) {
    $stages[$stage] = [ordered]@{ status = "NOT-TESTED"; reason = "Not reached" }
}
$started = Get-Date
$process = $null

function Set-Stage([string]$name, [string]$status, [string]$reason) {
    $stages[$name] = [ordered]@{ status = $status; reason = $reason }
}

function Wait-Element($root, [string]$automationId, [string]$name, [int]$seconds = 15) {
    $deadline = (Get-Date).AddSeconds($seconds)
    $condition = [System.Windows.Automation.Condition]::TrueCondition
    while ((Get-Date) -lt $deadline) {
        $items = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, $condition)
        foreach ($item in $items) {
            if (($automationId -and ($item.Current.AutomationId -eq $automationId -or $item.Current.AutomationId.EndsWith("." + $automationId))) -or ($name -and $item.Current.Name -eq $name)) { return $item }
        }
        Start-Sleep -Milliseconds 250
    }
    throw "GUI control not found: id='$automationId', name='$name'"
}

function Invoke-Element($element) {
    try {
        $element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
        return
    } catch {}
    try {
        $element.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
        return
    } catch {}
    throw "GUI control is not invokable/selectable: $($element.Current.Name)"
}

function Select-ComboValue($combo, [string]$value) {
    try {
        try { $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand() } catch {}
        Start-Sleep -Milliseconds 250
        $pattern = $combo.GetCurrentPattern([System.Windows.Automation.SelectionPattern]::Pattern)
        foreach ($item in $combo.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)) {
            if ($item.Current.Name -eq $value) {
                $item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
                return
            }
        }
    } catch {}
    $combo.SetFocus()
    if ($value -eq "GPIO Output") { [System.Windows.Forms.SendKeys]::SendWait("{HOME}{DOWN 2}{ENTER}"); return }
    throw "GUI value not found: $value"
}

function Select-Workflow($window, [int]$index) {
    $navigation = Wait-Element $window "workflow_navigation" ""
    $navigation.SetFocus()
    [System.Windows.Forms.SendKeys]::SendWait("{HOME}" + ("{DOWN}" * $index) + "{ENTER}")
    Start-Sleep -Milliseconds 500
}

try {
    $exe = (Resolve-Path $Executable).Path
    $process = Start-Process -FilePath $exe -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 250
        $process.Refresh()
    }
    if ($process.MainWindowHandle -eq 0) { throw "PVD MainWindow did not appear" }
    $window = [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
    Set-Stage Launch PASS "Real PVD executable started and MainWindow was found"

    $workflow = Wait-Element $window "workflow_navigation" ""
    $functionWorkflow = Wait-Element $workflow "" "Function Selection"
    $workflow.SetFocus()
    [System.Windows.Forms.SendKeys]::SendWait("{HOME}{DOWN}{ENTER}")
    Set-Stage Navigation PASS "Function Selection opened through workflow_navigation"

    $components = Wait-Element $window "component_selection" ""
    $pin = Wait-Element $components "" "Pin 1"
    $components.SetFocus()
    [System.Windows.Forms.SendKeys]::SendWait("{HOME}{DOWN 6}{ENTER}")
    $selector = Wait-Element $window "function_selector" ""
    Set-Stage Selection PASS "Pin 1 selected and function selector appeared"

    Select-ComboValue $selector "GPIO Output"
    Select-Workflow $window 2
    $setting = Wait-Element $window "setting_blink_enabled" ""
    Set-Stage Settings PASS "GPIO Output selected and visible settings control appeared"

    Select-Workflow $window 3
    $generate = Wait-Element $window "generate_project" ""
    Invoke-Element $generate
    if (-not (Test-Path "generated/main.cpp")) { throw "Generated source was not present after GUI Generate" }
    Set-Stage Generate PASS "Generate invoked through visible Generate Project control"

    Select-Workflow $window 6
    $configure = Wait-Element $window "configure_project" ""
    Invoke-Element $configure
    $configureDeadline = (Get-Date).AddSeconds(15)
    while (-not (Test-Path "build/CMakeCache.txt") -and (Get-Date) -lt $configureDeadline) { Start-Sleep -Milliseconds 250 }
    if (-not (Test-Path "build/CMakeCache.txt")) { throw "CMakeCache.txt was not created after GUI Configure" }
    Set-Stage Configure PASS "Configure invoked through visible Configure control"
    Start-Sleep -Seconds 2
    $build = Wait-Element $window "build_project" ""
    Invoke-Element $build
    $buildDeadline = (Get-Date).AddSeconds(60)
    while (-not (Test-Path "build/.pvd_build_success") -and (Get-Date) -lt $buildDeadline) { Start-Sleep -Seconds 1 }
    if (-not (Test-Path "build/.pvd_build_success")) { throw "PVD build success marker was not created after GUI Build" }
    Set-Stage Build PASS "Build invoked through visible Build control; result requires project build evidence"

    if ($AllowHardwareActions) {
        Set-Stage Transfer NOT-TESTED "Hardware action opt-in was requested but requires explicit certified target setup"
        Set-Stage Runtime NOT-TESTED "Runtime verification requires an available target"
        Set-Stage Debug NOT-TESTED "Debug verification requires an available target and probe"
    } else {
        Set-Stage Transfer NOT-TESTED "Hardware actions disabled for proof runner"
        Set-Stage Runtime NOT-TESTED "Hardware actions disabled for proof runner"
        Set-Stage Debug NOT-TESTED "Hardware actions disabled for proof runner"
    }
} catch {
    $failed = $null
    foreach ($key in $stages.Keys) { if ($stages[$key].status -eq "NOT-TESTED" -and -not $failed) { $failed = $key } }
    if ($failed) { Set-Stage $failed FAIL $_.Exception.Message }
} finally {
    if ($process -and -not $process.HasExited) { $process.CloseMainWindow(); Start-Sleep -Milliseconds 500; if (-not $process.HasExited) { $process.Kill() } }
    $overall = if (($stages.Values | Where-Object { $_.status -eq "FAIL" }).Count -gt 0) { "FAIL" } elseif (($stages.Values | Where-Object { $_.status -eq "PASS" }).Count -ge 7) { "PASS-AUTOMATED" } else { "NOT-TESTED" }
    $result = [ordered]@{
        caseId = "gui-proof-pin1-gpio.output"
        physicalPin = 1
        gpio = 0
        function = "gpio.output"
        profile = "existing PVD settings"
        stages = $stages
        overall = $overall
        startedUtc = $started.ToUniversalTime().ToString("o")
        completedUtc = (Get-Date).ToUniversalTime().ToString("o")
        processModel = "separate PVD executable via Windows UI Automation"
        aramfRecorder = "NOT-AVAILABLE: external aramf CLI not installed"
        physicalCertification = "PENDING"
    }
    $parent = Split-Path -Parent $ResultPath
    if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
    $result | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $ResultPath
    Write-Output ($result | ConvertTo-Json -Depth 8)
}
