param(
    [ValidateSet('A','B','C','D')]
    [string]$Strategy = 'A',
    [string]$Executable = 'build-fresh/pico_visual_designer.exe',
    [string]$ResultPath = 'validation/pin_certification/results/sio-direction-diagnostic.json'
    ,[switch]$LifecycleOnly
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
Import-Module (Join-Path $PSScriptRoot 'gui/PvdGuiAutomation.psm1') -Force

function Get-Patterns($element) {
    return Get-PvdControlPatternNames $element
}

function Get-ComboValue($combo) {
    $value = $null
    try { $value = $combo.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value } catch {}
    if (-not $value) { $value = $combo.Current.Name }
    return $value
}

function Wait-Combo([int]$processId) {
    return Find-PvdControlFresh $processId 'setting_direction' '' 15
}

function Select-SioFunction($root, [int]$processId) {
    $combo = Wait-PvdElement $root 'function_selector' '' 15
    Focus-PvdElement $combo $processId
    $rect = $combo.Current.BoundingRectangle
    [PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
    Start-Sleep -Milliseconds 150
    [System.Windows.Forms.SendKeys]::SendWait('{HOME}' + ('{DOWN}' * 7) + '{ENTER}')
    Start-Sleep -Milliseconds 400
}

function Select-PinAndSettings($root, [int]$processId) {
    $navigation = Wait-PvdElement $root 'workflow_navigation' '' 15
    $pages = @($navigation.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    $functionPage = $pages | Where-Object { $_.Current.Name -eq 'Function Selection' } | Select-Object -First 1
    if (-not $functionPage) { throw 'Function Selection workflow page not found' }
    Focus-PvdElement $navigation $processId
    [System.Windows.Forms.SendKeys]::SendWait('{HOME}' + ('{DOWN}' * ($pages.IndexOf($functionPage))) + '{ENTER}')
    Start-Sleep -Milliseconds 400
    $root = Get-PvdWindow $processId
    $component = Wait-PvdElement $root 'component_selection' '' 15
    $items = @($component.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    $pin = $items | Where-Object { $_.Current.Name -eq 'Pin 1' } | Select-Object -First 1
    if (-not $pin) { throw 'Pin 1 not found' }
    $pinIndex = 0
    for ($i = 0; $i -lt $items.Count; $i++) { if ($items[$i].Current.Name -eq 'Pin 1') { $pinIndex = $i; break } }
    Focus-PvdElement $component $processId
    [System.Windows.Forms.SendKeys]::SendWait('{HOME}' + ('{DOWN}' * $pinIndex) + '{ENTER}')
    Start-Sleep -Milliseconds 400
    Start-Sleep -Milliseconds 300
    $root = Get-PvdWindow $processId
    Select-SioFunction $root $processId
    $root = Get-PvdWindow $processId
    $functionCombo = Wait-PvdElement $root 'function_selector' '' 15
    $functionValue = Get-ComboValue $functionCombo
    if ($functionValue -notin @('SIO','sio')) {
        throw "Function Selection did not commit SIO; observed '$functionValue'"
    }
    $navigation = Wait-PvdElement $root 'workflow_navigation' '' 15
    $pages = @($navigation.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    $settings = $pages | Where-Object { $_.Current.Name -eq 'Settings' } | Select-Object -First 1
    Focus-PvdElement $navigation $processId
    [System.Windows.Forms.SendKeys]::SendWait('{HOME}' + ('{DOWN}' * ($pages.IndexOf($settings))) + '{ENTER}')
    Start-Sleep -Milliseconds 450
    $settingsList = Wait-PvdElement (Get-PvdWindow $processId) 'settings_selection' '' 15
    $rows = @($settingsList.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    $row = $rows | Where-Object { $_.Current.Name -match 'Pin 1|GPIO0' -and $_.Current.Name -match 'SIO' } | Select-Object -First 1
    if (-not $row) { throw "Pin 1 / SIO Settings row not found; observed=$($rows.Current.Name -join ' | ')" }
    [void]$row.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
    Start-Sleep -Milliseconds 350
}

function Dump-Element($element) {
    return [ordered]@{
        name = $element.Current.Name
        automation_id = $element.Current.AutomationId
        control_type = $element.Current.ControlType.ProgrammaticName
        class_name = $element.Current.ClassName
        framework_id = $element.Current.FrameworkId
        enabled = $element.Current.IsEnabled
        offscreen = $element.Current.IsOffscreen
        focused = $element.Current.HasKeyboardFocus
        bounds = [ordered]@{ x=$element.Current.BoundingRectangle.X; y=$element.Current.BoundingRectangle.Y; width=$element.Current.BoundingRectangle.Width; height=$element.Current.BoundingRectangle.Height }
        native_window_handle = $element.Current.NativeWindowHandle
        patterns = Get-Patterns $element
    }
}

$process = Start-Process -FilePath (Resolve-Path $Executable) -ArgumentList '--certification-dialogs' -PassThru
$evidence = [ordered]@{ strategy=$Strategy; process_id=$process.Id; stages=[ordered]@{} }
try {
    $deadline = (Get-Date).AddSeconds(30)
    while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250; $process.Refresh() }
    $root = Get-PvdWindow $process.Id
    $name = Wait-PvdElement $root 'project_name' '' 15
    $path = Wait-PvdElement $root 'project_path' '' 15
    [void]$name.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue('SIO_DIRECTION_DIAGNOSTIC')
    $temp = Join-Path ([IO.Path]::GetTempPath()) ('pvd-direction-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
    [void]$path.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($temp)
    Invoke-PvdButton (Wait-PvdElement $root 'project_create' '' 15) $process.Id | Out-Null
    Start-Sleep -Milliseconds 500
    $root = Get-PvdWindow $process.Id
    Select-PinAndSettings $root $process.Id
    $combo = Wait-Combo $process.Id
    $evidence.combo_before = Dump-Element $combo
    $evidence.combo_before.value = Get-ComboValue $combo

    if ($LifecycleOnly) {
        $main = Get-PvdMainWindow -ProcessId $process.Id
        $mainHwnd = [int64]$main.Current.NativeWindowHandle
        $evidence.lifecycle = @()
        for ($attempt = 1; $attempt -le 10; $attempt++) {
            $before = Get-PvdMainWindow -ProcessId $process.Id -VerifiedHwnd $mainHwnd
            $comboAttempt = Wait-Combo $process.Id
            try { $comboAttempt.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand() } catch { throw "Attempt $attempt could not open Direction popup: $($_.Exception.Message)" }
            Start-Sleep -Milliseconds 150
            $popup = @(Get-PvdQtPopup -ProcessId $process.Id)
            if ($popup.Count -ne 1) {
                $windowDump = @(Get-PvdTopLevelWindows -ProcessId $process.Id | Select-Object hwnd,class_name,title,visible,enabled,owner_hwnd,parent_hwnd,foreground)
                throw "Attempt $attempt expected one Qt combo popup, found $($popup.Count); windows=$($windowDump | ConvertTo-Json -Compress -Depth 5)"
            }
            [System.Windows.Forms.SendKeys]::SendWait('{ESC}')
            Start-Sleep -Milliseconds 200
            $after = Get-PvdMainWindow -ProcessId $process.Id -VerifiedHwnd $mainHwnd
            $freshCombo = Wait-Combo $process.Id
            $evidence.lifecycle += [ordered]@{
                attempt = $attempt; main_hwnd_before = $mainHwnd; popup_hwnd = $popup[0].hwnd
                main_valid_during_popup = [PvdWindowNative]::IsWindow([IntPtr]$mainHwnd)
                main_hwnd_after = [int64]$after.Current.NativeWindowHandle
                same_main_hwnd = ([int64]$after.Current.NativeWindowHandle -eq $mainHwnd)
                direction_reacquired = ($null -ne $freshCombo)
            }
        }
        $evidence.result = if (@($evidence.lifecycle | Where-Object { -not $_.main_valid_during_popup -or -not $_.same_main_hwnd -or -not $_.direction_reacquired }).Count -eq 0) { 'PASS' } else { 'FAIL' }
        $parent = Split-Path -Parent $ResultPath; if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
        $evidence | ConvertTo-Json -Depth 12 | Set-Content -Encoding UTF8 $ResultPath
        $evidence | ConvertTo-Json -Depth 12
        return
    }

    $main = Get-PvdMainWindow -ProcessId $process.Id
    $mainHwnd = [int64]$main.Current.NativeWindowHandle
    $timeline = [ordered]@{}
    $timeline.T0_input_started = (Get-Date).ToUniversalTime().ToString('o')
    try { $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand(); $evidence.popup_opened=$true } catch { $evidence.popup_opened=$false }
    $timeline.T1_popup_open = (Get-Date).ToUniversalTime().ToString('o')
    Start-Sleep -Milliseconds 150
    $evidence.popup_windows = @([System.Windows.Automation.AutomationElement]::RootElement.FindAll([System.Windows.Automation.TreeScope]::Children,[System.Windows.Automation.Condition]::TrueCondition) | ForEach-Object { Dump-Element $_ })
    $evidence.popup_items = @([System.Windows.Automation.AutomationElement]::RootElement.FindAll([System.Windows.Automation.TreeScope]::Descendants,[System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem } | ForEach-Object { Dump-Element $_ })

    if ($Strategy -eq 'A') {
        $item = @([System.Windows.Automation.AutomationElement]::RootElement.FindAll([System.Windows.Automation.TreeScope]::Descendants,[System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and $_.Current.Name -eq 'Output' }) | Select-Object -First 1
        [void]$item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
    } elseif ($Strategy -eq 'B') {
        Focus-PvdElement $combo $process.Id; [System.Windows.Forms.SendKeys]::SendWait('{DOWN}')
    } elseif ($Strategy -eq 'C') {
        Focus-PvdElement $combo $process.Id; [System.Windows.Forms.SendKeys]::SendWait('{DOWN}{ENTER}')
    } else {
        $item = @([System.Windows.Automation.AutomationElement]::RootElement.FindAll([System.Windows.Automation.TreeScope]::Descendants,[System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and $_.Current.Name -eq 'Output' }) | Select-Object -First 1
        $r = $item.Current.BoundingRectangle
        [PvdMouse]::Click([int]($r.X + $r.Width/2), [int]($r.Y + $r.Height/2))
    }
    $timeline.T2_item_input = (Get-Date).ToUniversalTime().ToString('o')
    $popupDeadline = (Get-Date).AddSeconds(5)
    while ((Get-Date) -lt $popupDeadline -and @(Get-PvdQtPopup -ProcessId $process.Id).Count -gt 0) { Start-Sleep -Milliseconds 100 }
    $timeline.T3_popup_closed = (Get-Date).ToUniversalTime().ToString('o')
    $process.Refresh()
    $evidence.process_alive_after = -not $process.HasExited
    if (-not $process.HasExited) {
        $timeline.T4_main_hwnd_valid = (Get-Date).ToUniversalTime().ToString('o')
        $evidence.main_hwnd = $mainHwnd
        $evidence.main_hwnd_valid = [PvdWindowNative]::IsWindow([IntPtr]$mainHwnd)
        $timeline.T5_ui_root = $null; $timeline.T6_settings_row = $null; $timeline.T7_direction = $null; $timeline.T8_direction_target = $null; $timeline.T9_mode_control = $null
        $stable = Wait-PvdUiTreeStable -ProcessId $process.Id -MainHwnd $mainHwnd -Expected @{ pin_pattern='Pin 1|GPIO0'; function_pattern='SIO'; direction='Output'; mode_control='setting_initial_state' } -Seconds 8
        $timeline.T5_ui_root = (Get-Date).ToUniversalTime().ToString('o')
        $timeline.T6_settings_row = $timeline.T5_ui_root
        $timeline.T7_direction = $timeline.T5_ui_root
        $timeline.T8_direction_target = $timeline.T5_ui_root
        $timeline.T9_mode_control = $timeline.T5_ui_root
        $evidence.timeline = $timeline
        $evidence.semantic_state = [ordered]@{ expected_pin='Pin 1/GPIO0'; observed_pin=$stable.row.Current.Name; function='SIO'; direction=$stable.direction_value; mode_control=$stable.mode_control.Current.AutomationId; attempts=$stable.attempts }
        $evidence.result = 'PASS'
    } else { $evidence.result='FAIL'; $evidence.exit_code=$process.ExitCode }
} catch { $evidence.result='FAIL'; $evidence.error=$_.Exception.Message }
finally {
    if ($process -and -not $process.HasExited) {
        [void]$process.CloseMainWindow()
        Start-Sleep -Milliseconds 250
        if (-not $process.HasExited) {
            try { $evidence.cleanup = Invoke-PvdDialogButton -ProcessId $process.Id -Label 'Discard' -Seconds 5 }
            catch { $evidence.cleanup_error = $_.Exception.Message }
        }
        $exitDeadline = (Get-Date).AddSeconds(5)
        while (-not $process.HasExited -and (Get-Date) -lt $exitDeadline) { Start-Sleep -Milliseconds 100; $process.Refresh() }
        $evidence.process_exited = $process.HasExited
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
    }
}
$parent = Split-Path -Parent $ResultPath; if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
$evidence | ConvertTo-Json -Depth 12 | Set-Content -Encoding UTF8 $ResultPath
$evidence | ConvertTo-Json -Depth 12
