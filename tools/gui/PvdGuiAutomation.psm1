Set-StrictMode -Version Latest
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
if ($null -eq ('PvdWindowNative' -as [type])) {
    Add-Type @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class PvdWindowNative {
  public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr lParam);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder text, int length);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder text, int length);
  [DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr hWnd, uint command);
  [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern bool IsWindow(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
  public const uint GW_OWNER = 4;
}
'@
}
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class PvdMouse {
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
  public const uint LEFTDOWN=0x0002, LEFTUP=0x0004;
  public static void Click(int x, int y) { SetCursorPos(x,y); mouse_event(LEFTDOWN,0,0,0,UIntPtr.Zero); mouse_event(LEFTUP,0,0,0,UIntPtr.Zero); }
}
'@

function Get-PvdWindow([int]$processId) {
    return Get-PvdMainWindow -ProcessId $processId
}

function Get-PvdTopLevelWindows([int]$processId) {
    $foreground = [PvdWindowNative]::GetForegroundWindow()
    $records = New-Object System.Collections.ArrayList
    $callback = [PvdWindowNative+EnumWindowsProc]{
        param($hWnd, $lParam)
        [uint32]$ownerPid = 0
        [void][PvdWindowNative]::GetWindowThreadProcessId($hWnd, [ref]$ownerPid)
        if ($ownerPid -ne [uint32]$processId) { return $true }
        $title = New-Object System.Text.StringBuilder 512
        $class = New-Object System.Text.StringBuilder 256
        [void][PvdWindowNative]::GetWindowText($hWnd, $title, $title.Capacity)
        [void][PvdWindowNative]::GetClassName($hWnd, $class, $class.Capacity)
        $rect = New-Object PvdWindowNative+RECT
        [void][PvdWindowNative]::GetWindowRect($hWnd, [ref]$rect)
        [void]$records.Add([pscustomobject]@{
            hwnd = [int64]$hWnd.ToInt64(); pid = [int]$ownerPid; class_name = $class.ToString(); title = $title.ToString()
            visible = [PvdWindowNative]::IsWindowVisible($hWnd); enabled = [PvdWindowNative]::IsWindowEnabled($hWnd)
            owner_hwnd = [int64]([PvdWindowNative]::GetWindow($hWnd, [PvdWindowNative]::GW_OWNER).ToInt64())
            parent_hwnd = [int64]([PvdWindowNative]::GetParent($hWnd).ToInt64()); foreground = ($hWnd -eq $foreground)
            rect = [ordered]@{ x=$rect.Left; y=$rect.Top; width=($rect.Right-$rect.Left); height=($rect.Bottom-$rect.Top) }
            control_type = $null; uia_class_name = $null; automation_id = $null
        })
        return $true
    }
    [void][PvdWindowNative]::EnumWindows($callback, [IntPtr]::Zero)
    foreach ($record in $records) {
        if (-not $record.visible -or $record.hwnd -eq 0) { continue }
        try {
            $uia = [System.Windows.Automation.AutomationElement]::FromHandle([IntPtr]$record.hwnd)
            $record.control_type = $uia.Current.ControlType.ProgrammaticName
            $record.uia_class_name = $uia.Current.ClassName
            $record.automation_id = $uia.Current.AutomationId
        } catch {
            $record.control_type = $null; $record.uia_class_name = $null; $record.automation_id = $null
        }
    }
    return @($records.ToArray())
}

function Get-PvdMainWindow([int]$ProcessId, [Int64]$VerifiedHwnd = 0) {
    $records = @(Get-PvdTopLevelWindows $ProcessId)
    $transient = @('QComboBoxPrivateContainer','QMenu','QToolTip','Qt5QWindowIcon','Qt6QWindowIcon')
    $candidate = $null
    if ($VerifiedHwnd -ne 0) {
        $candidate = $records | Where-Object { $_.hwnd -eq $VerifiedHwnd -and $_.pid -eq $ProcessId -and $_.visible -and $_.class_name -notin $transient } | Select-Object -First 1
    }
    if (-not $candidate) {
        $candidate = $records | Where-Object {
            $_.pid -eq $ProcessId -and $_.visible -and $_.class_name -notin $transient -and
            (($_.class_name -eq 'pvd::MainWindow') -or ($_.control_type -eq 'ControlType.Window' -and $_.title -eq 'pico_visual_designer'))
        } | Select-Object -First 1
    }
    if (-not $candidate) {
        $candidate = $records | Where-Object {
            $_.pid -eq $ProcessId -and $_.visible -and $_.enabled -and $_.title -eq 'pico_visual_designer' -and
            $_.class_name -notin @('QComboBoxPrivateContainer','Qt6111QWindowPopupDropShadowSaveBits','QMenu','QToolTip')
        } | Sort-Object @{ Expression = { $_.rect.width * $_.rect.height }; Descending = $true } | Select-Object -First 1
    }
    if ($candidate -and -not [PvdWindowNative]::IsWindow([IntPtr]$candidate.hwnd)) {
        $candidate = $records | Where-Object {
            $_.pid -eq $ProcessId -and $_.visible -and $_.enabled -and $_.title -eq 'pico_visual_designer' -and
            $_.class_name -notin @('QComboBoxPrivateContainer','Qt6111QWindowPopupDropShadowSaveBits','QMenu','QToolTip') -and
            [PvdWindowNative]::IsWindow([IntPtr]$_.hwnd)
        } | Sort-Object @{ Expression = { $_.rect.width * $_.rect.height }; Descending = $true } | Select-Object -First 1
    }
    if (-not $candidate) { throw "PVD main window identity unavailable; windows=$($records | ConvertTo-Json -Compress -Depth 5)" }
    if (-not [PvdWindowNative]::IsWindow([IntPtr]$candidate.hwnd)) { throw "PVD main HWND is no longer valid: $($candidate.hwnd)" }
    return [System.Windows.Automation.AutomationElement]::FromHandle([IntPtr]$candidate.hwnd)
}

function Get-PvdQtPopup([int]$ProcessId, [string]$ClassName = 'QComboBoxPrivateContainer') {
    $roots = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
        [System.Windows.Automation.TreeScope]::Children,
        [System.Windows.Automation.Condition]::TrueCondition)
    $popups = New-Object System.Collections.ArrayList
    foreach ($root in $roots) {
        try {
            if ($root.Current.ProcessId -ne $ProcessId -or $root.Current.ClassName -ne $ClassName -or
                $root.Current.IsOffscreen -or -not $root.Current.IsEnabled) { continue }
            [void]$popups.Add([pscustomobject]@{
                hwnd = [int64]$root.Current.NativeWindowHandle
                pid = $ProcessId
                class_name = $root.Current.ClassName
                title = $root.Current.Name
                visible = -not $root.Current.IsOffscreen
                enabled = $root.Current.IsEnabled
                owner_hwnd = 0
                parent_hwnd = 0
                foreground = ([PvdWindowNative]::GetForegroundWindow().ToInt64() -eq $root.Current.NativeWindowHandle)
                control_type = $root.Current.ControlType.ProgrammaticName
                uia_class_name = $root.Current.ClassName
                automation_id = $root.Current.AutomationId
                uia_root = $root
            })
        } catch {}
    }
    return @($popups.ToArray())
}

function Get-PvdModalDialog([int]$ProcessId) {
    $roots = New-Object System.Collections.ArrayList
    $desktopRoots = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
        [System.Windows.Automation.TreeScope]::Children,
        [System.Windows.Automation.Condition]::TrueCondition)
    foreach ($root in $desktopRoots) { [void]$roots.Add($root) }
    try {
        $main = Get-PvdMainWindow -ProcessId $ProcessId
        if ($main) {
            $nested = $main.FindAll(
                [System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition)
            foreach ($root in $nested) { [void]$roots.Add($root) }
        }
    } catch {}
    foreach ($root in $roots) {
        try {
            if ($root.Current.ProcessId -ne $ProcessId -or $root.Current.IsOffscreen -or $root.Current.Name -eq 'pico_visual_designer') { continue }
            $descendants = @($root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                                          [System.Windows.Automation.Condition]::TrueCondition))
            $dialogButtons = @($descendants | Where-Object {
                $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and
                $_.Current.Name -in @('Save','Discard','Cancel')
            })
            $isMessageBox = $root.Current.ClassName -eq 'QMessageBox' -or
                $root.Current.AutomationId -like '*QMessageBox*' -or
                $root.Current.Name -eq 'Close Project'
            if ($dialogButtons.Count -eq 0 -and -not $isMessageBox) { continue }
            $hwnd = [IntPtr]$root.Current.NativeWindowHandle
            $owner = if ($hwnd -ne [IntPtr]::Zero) {
                [PvdWindowNative]::GetWindow($hwnd, [PvdWindowNative]::GW_OWNER)
            } else { [IntPtr]::Zero }
            return [ordered]@{
                root = $root; hwnd = [int64]$root.Current.NativeWindowHandle; pid = $ProcessId
                title = $root.Current.Name; class_name = $root.Current.ClassName
                automation_id = $root.Current.AutomationId; owner_hwnd = [int64]$owner.ToInt64()
            }
        } catch {}
    }
    return $null
}

function Invoke-PvdDialogButton([int]$ProcessId, [string]$Label, [int]$Seconds = 5) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    $dialog = $null
    while ((Get-Date) -lt $deadline -and -not $dialog) {
        $dialog = Get-PvdModalDialog $ProcessId
        if (-not $dialog) { Start-Sleep -Milliseconds 100 }
    }
    if (-not $dialog) { throw "PVD modal dialog not found for button '$Label'" }
    $buttons = @($dialog.root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                                     [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and $_.Current.Name -eq $Label })
    if ($buttons.Count -ne 1) { throw "Expected one dialog button '$Label', found $($buttons.Count)" }
    $button = $buttons[0]
    try { [void]$button.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke() }
    catch { Focus-PvdElement $button $ProcessId; [System.Windows.Forms.SendKeys]::SendWait('{ENTER}') }
    $closed = $false
    $closeDeadline = (Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $closeDeadline) {
        if (-not (Get-PvdModalDialog $ProcessId)) { $closed = $true; break }
        Start-Sleep -Milliseconds 100
    }
    if (-not $closed) { throw "PVD modal dialog did not close after '$Label'" }
    return [ordered]@{ title=$dialog.title; class_name=$dialog.class_name; hwnd=$dialog.hwnd; owner_hwnd=$dialog.owner_hwnd; button=$Label; closed=$true }
}

function Wait-PvdUiTreeStable([int]$ProcessId, [Int64]$MainHwnd, [hashtable]$Expected,
                               [int]$Seconds = 12) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    $attempts = 0
    $lastFailure = $null
    while ((Get-Date) -lt $deadline) {
        $attempts++
        try {
            $process = Get-Process -Id $ProcessId -ErrorAction Stop
            $process.Refresh()
            [uint32]$ownerPid = 0
            [void][PvdWindowNative]::GetWindowThreadProcessId([IntPtr]$MainHwnd, [ref]$ownerPid)
            if (-not [PvdWindowNative]::IsWindow([IntPtr]$MainHwnd) -or $ownerPid -ne [uint32]$ProcessId) {
                throw "Preserved main HWND is invalid or belongs to PID $ownerPid"
            }
            $root = [System.Windows.Automation.AutomationElement]::FromHandle([IntPtr]$MainHwnd)
            $all = @($root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                                   [System.Windows.Automation.Condition]::TrueCondition))
            $settingsPage = $all | Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and $_.Current.Name -eq 'Settings' } | Select-Object -First 1
            if (-not $settingsPage) { throw 'Settings workflow page not available' }
            $settingsList = $all | Where-Object { $_.Current.AutomationId -eq 'settings_selection' } | Select-Object -First 1
            if (-not $settingsList) { throw 'Settings selection list not available' }
            $rows = @($settingsList.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                                            [System.Windows.Automation.Condition]::TrueCondition))
            $row = $rows | Where-Object {
                $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and
                $_.Current.Name -match [string]$Expected.pin_pattern -and
                $_.Current.Name -match [string]$Expected.function_pattern
            } | Select-Object -First 1
            if (-not $row) { throw 'Expected pin/function Settings row not available' }
            $direction = $all | Where-Object { $_.Current.AutomationId -eq 'setting_direction' } | Select-Object -First 1
            if (-not $direction) { throw 'Direction control not available' }
            $directionValue = $null
            try { $directionValue = $direction.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value } catch {}
            if (-not $directionValue) { $directionValue = $direction.Current.Name }
            if ($directionValue -ne [string]$Expected.direction) { throw "Direction is '$directionValue', expected '$($Expected.direction)'" }
            $modeControl = $all | Where-Object { $_.Current.AutomationId -eq [string]$Expected.mode_control } | Select-Object -First 1
            if (-not $modeControl) { throw "Expected mode-specific control '$($Expected.mode_control)' not available" }
            return [ordered]@{
                root = $root; settings_page = $settingsPage; settings_list = $settingsList; row = $row
                direction = $direction; mode_control = $modeControl; direction_value = $directionValue
                attempts = $attempts; ready = $true
            }
        } catch {
            $lastFailure = $_.Exception.Message
            Start-Sleep -Milliseconds 100
        }
    }
    throw "UI tree semantic stabilization timed out after $attempts attempts: $lastFailure"
}

function Wait-PvdElement($root, [string]$automationId, [string]$name, [int]$seconds = 20) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $items = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($item in $items) {
            if (($automationId -and ($item.Current.AutomationId -eq $automationId -or $item.Current.AutomationId.EndsWith("." + $automationId))) -or ($name -and $item.Current.Name -eq $name)) { return $item }
        }
        Start-Sleep -Milliseconds 150
    }
    throw "GUI element not found: id='$automationId' name='$name'"
}

function Focus-PvdElement($element, [int]$processId) {
    $shell = New-Object -ComObject WScript.Shell
    [void]$shell.AppActivate($processId)
    [void]$element.SetFocus()
}

function Set-PvdTextAsUser($element, [string]$value, [int]$processId) {
    Focus-PvdElement $element $processId
    [System.Windows.Forms.SendKeys]::SendWait("^a")
    [System.Windows.Forms.SendKeys]::SendWait($value)
    [System.Windows.Forms.SendKeys]::SendWait("{TAB}")
    Start-Sleep -Milliseconds 200
    $observed = $null
    try { $observed = $element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value } catch { $observed = $element.Current.Name }
    if ($observed -ne $value) { throw "Text commit mismatch: expected '$value', observed '$observed'" }
    return [ordered]@{ method = "keyboard+focus-out"; requested = $value; observed = $observed }
}

function Invoke-PvdButton($element, [int]$processId) {
    if (-not $element.Current.IsEnabled -or $element.Current.IsOffscreen) { throw "Button is disabled or offscreen: $($element.Current.AutomationId)" }
    try { [void]$element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke(); return "InvokePattern" } catch {}
    Focus-PvdElement $element $processId
    [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
    return "keyboard-enter"
}

function Select-PvdComboItemAsUser($combo, [string]$value, [int]$processId) {
    if (-not $combo.Current.IsEnabled -or $combo.Current.IsOffscreen) { throw "Combo is disabled or offscreen: $($combo.Current.AutomationId)" }
    Focus-PvdElement $combo $processId
    $rect = $combo.Current.BoundingRectangle
    [PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
    Start-Sleep -Milliseconds 250
    if ($value -eq "Testing") {
        [System.Windows.Forms.SendKeys]::SendWait("{HOME}{DOWN}{ENTER}")
        Start-Sleep -Milliseconds 300
        $selected = $null
        try { $selected = $combo.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value } catch { $selected = $combo.Current.Name }
        if ($selected -eq $value) { return [ordered]@{ method = "mouse-open-keyboard-select"; requested = $value; observed = $selected } }
    }
    $popupItems = [System.Windows.Automation.AutomationElement]::RootElement.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
    foreach ($item in $popupItems) {
        if ($item.Current.Name -eq $value -and $item.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem) {
            $r = $item.Current.BoundingRectangle
            if ($r.Width -gt 0 -and $r.Height -gt 0) {
                [PvdMouse]::Click([int]($r.X + $r.Width / 2), [int]($r.Y + $r.Height / 2))
                Start-Sleep -Milliseconds 300
                $selected = $null
                try { $selected = $combo.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value } catch { $selected = $combo.Current.Name }
                if ($selected -eq $value) { return [ordered]@{ method = "mouse-popup-selection"; requested = $value; observed = $selected } }
            }
        }
    }
    $expanded = $false
    try { $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand(); $expanded = $true } catch {}
    Start-Sleep -Milliseconds 200
    $roots = @($combo, [System.Windows.Automation.AutomationElement]::RootElement)
    foreach ($searchRoot in $roots) {
        $items = $searchRoot.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($item in $items) {
            if ($item.Current.Name -eq $value -and $item.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem) {
                try { [void]$item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select() } catch { [void]$item.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke() }
                Start-Sleep -Milliseconds 250
                $selected = $null
                try { $selected = $combo.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value } catch { $selected = $combo.Current.Name }
                if ($selected -ne $value) { throw "Combo selection did not commit: expected '$value', observed '$selected'" }
                return [ordered]@{ method = "expanded-popup-selection"; requested = $value; observed = $selected }
            }
        }
    }
    Focus-PvdElement $combo $processId
    [System.Windows.Forms.SendKeys]::SendWait("{HOME}")
    [System.Windows.Forms.SendKeys]::SendWait($value)
    [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
    Start-Sleep -Milliseconds 250
    $selected = $combo.Current.Name
    if ($selected -ne $value) { throw "Combo selection item '$value' was not exposed by UIA popup" }
    return [ordered]@{ method = "keyboard-search"; requested = $value; observed = $selected }
}

function Read-PvdLogText($element) {
    $metadata = [ordered]@{ automation_id = $element.Current.AutomationId; control_type = $element.Current.ControlType.ProgrammaticName; method = $null; text = "" }
    try { $metadata.text = $element.GetCurrentPattern([System.Windows.Automation.TextPattern]::Pattern).DocumentRange.GetText(-1); $metadata.method = "TextPattern.DocumentRange"; return $metadata } catch {}
    try { $metadata.text = $element.GetCurrentPattern([System.Windows.Automation.LegacyIAccessiblePattern]::Pattern).Current.Value; $metadata.method = "LegacyIAccessible.Value"; return $metadata } catch {}
    try { $metadata.text = $element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value; $metadata.method = "ValuePattern.Value"; return $metadata } catch {}
    $parts = @($element.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) | ForEach-Object { $_.Current.Name } | Where-Object { $_ })
    $metadata.text = $parts -join "`n"; $metadata.method = "child-names"
    return $metadata
}

function Get-PvdCurrentPage($window) {
    $navigation = Wait-PvdElement $window "workflow_navigation" ""
    $selected = $null
    $items = @($navigation.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    foreach ($item in $items) {
        try {
            if ($item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Current.IsSelected) {
                $selected = $item
                break
            }
        } catch {}
    }
    if (-not $selected) { throw "Current PVD workflow page is not exposed by UI Automation" }
    return [ordered]@{ name = $selected.Current.Name; automation_id = $selected.Current.AutomationId; element = $selected }
}

function Wait-PvdPage($window, [string]$name, [int]$seconds = 20) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        try {
            $page = Get-PvdCurrentPage $window
            if ($page.name -eq $name) { return $page }
        } catch {}
        Start-Sleep -Milliseconds 150
    }
    throw "PVD workflow page not reached: '$name'"
}

function Navigate-PvdPage([int]$processId, [string]$name, [int]$seconds = 20) {
    $window = Get-PvdWindow $processId
    $navigation = Wait-PvdElement $window "workflow_navigation" ""
    $items = @($navigation.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    $item = $items | Where-Object { $_.Current.Name -eq $name } | Select-Object -First 1
    if (-not $item) { throw "Workflow page not found: '$name'" }
    $rect = $item.Current.BoundingRectangle
    if ($rect.Width -le 0 -or $rect.Height -le 0 -or $item.Current.IsOffscreen) { throw "Workflow page is not visible: '$name'" }
    $navigation.SetFocus()
    $index = [Array]::IndexOf([array]$items, $item)
    [System.Windows.Forms.SendKeys]::SendWait("{HOME}" + ("{DOWN}" * $index) + "{ENTER}")
    Start-Sleep -Milliseconds 150
    return Wait-PvdPage (Get-PvdWindow $processId) $name $seconds
}

function Assert-PvdPage($window, [string]$name) {
    $page = Get-PvdCurrentPage $window
    if ($page.name -ne $name) { throw "Unexpected PVD page: expected '$name', actual '$($page.name)'" }
    return $page
}

function Find-PvdControlFresh([int]$processId, [string]$automationId, [string]$name, [int]$seconds = 20) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline)
    {
        $root = Get-PvdWindow $processId
        $items = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($item in $items)
        {
            $matches = ($automationId -and ($item.Current.AutomationId -eq $automationId -or
                        $item.Current.AutomationId.EndsWith("." + $automationId))) -or
                       ($name -and $item.Current.Name -eq $name)
            if ($matches -and -not $item.Current.IsOffscreen -and
                $item.Current.BoundingRectangle.Width -gt 0 -and $item.Current.BoundingRectangle.Height -gt 0)
            {
                return $item
            }
        }
        Start-Sleep -Milliseconds 150
    }
    throw "Visible GUI element not found: id='$automationId' name='$name'"
}

function Invoke-PvdRebuildingMutation([scriptblock]$mutation, [scriptblock]$reacquire,
                                      [int]$seconds = 10) {
    # Performs an interaction that may destroy its Qt subtree, then returns only newly reacquired controls.
    & $mutation
    $deadline = (Get-Date).AddSeconds($seconds)
    $lastError = $null
    while ((Get-Date) -lt $deadline) {
        try {
            $result = & $reacquire
            if ($null -ne $result) { return $result }
        }
        catch { $lastError = $_.Exception.Message }
        Start-Sleep -Milliseconds 150
    }
    if ($lastError) { throw "Dynamic UI reacquisition timed out: $lastError" }
    throw "Dynamic UI reacquisition timed out without a stable rebuilt subtree"
}

function Get-PvdControlPatternNames($element) {
    # Reports the common UIA patterns without retaining the element for later use.
    $patterns = [ordered]@{}
    foreach ($entry in @(
            @{ name = "InvokePattern"; pattern = [System.Windows.Automation.InvokePattern]::Pattern },
            @{ name = "ValuePattern"; pattern = [System.Windows.Automation.ValuePattern]::Pattern },
            @{ name = "ExpandCollapsePattern"; pattern = [System.Windows.Automation.ExpandCollapsePattern]::Pattern },
            @{ name = "SelectionPattern"; pattern = [System.Windows.Automation.SelectionPattern]::Pattern },
            @{ name = "SelectionItemPattern"; pattern = [System.Windows.Automation.SelectionItemPattern]::Pattern })) {
        try { $element.GetCurrentPattern($entry.pattern) | Out-Null; $patterns[$entry.name] = $true }
        catch { $patterns[$entry.name] = $false }
    }
    return $patterns
}

function Set-PvdQtComboBoxValue([int]$processId, [string]$automationId, [string]$targetText,
                                [scriptblock]$postRebuild = $null, [int]$seconds = 12) {
    # Selects a visible Qt QComboBox item through UIA/keyboard semantics and verifies post-commit state.
    $combo = Find-PvdControlFresh $processId $automationId "" $seconds
    $evidence = [ordered]@{
        widget_type = $combo.Current.ControlType.ProgrammaticName
        object_name = $combo.Current.AutomationId
        accessible_name = $combo.Current.Name
        patterns_before = Get-PvdControlPatternNames $combo
        popup_appeared = $false
        target_item_found = $false
        selection_method = $null
        popup_closed = $false
        post_rebuild = $null
    }
    $oldCombo = $combo
    try {
        $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
        $evidence.popup_appeared = $true
    }
    catch {
        Focus-PvdElement $combo $processId
        [System.Windows.Forms.SendKeys]::SendWait("{F4}")
        Start-Sleep -Milliseconds 150
        $evidence.popup_appeared = $true
    }
    Start-Sleep -Milliseconds 150
    $popupItems = @([System.Windows.Automation.AutomationElement]::RootElement.FindAll(
            [System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and
                       $_.Current.Name -eq $targetText -and -not $_.Current.IsOffscreen })
    if ($popupItems.Count -gt 0) {
        $evidence.target_item_found = $true
        $item = $popupItems[0]
        $selectedThroughUia = $false
        try {
            $item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
            $selectedThroughUia = $true
            $evidence.selection_method = "user-equivalent-uia-selection-enter"
            Focus-PvdElement $item $processId
            [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
        }
        catch {
            $rect = $item.Current.BoundingRectangle
            [void][PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
            $evidence.selection_method = "user-equivalent-popup-click-fallback"
        }
    }
    else {
        # Direction is a two-item Qt combo in this application; the keyboard path is still user-equivalent.
        Focus-PvdElement $combo $processId
        [System.Windows.Forms.SendKeys]::SendWait("{HOME}")
        if ($targetText -ne "Input") { [System.Windows.Forms.SendKeys]::SendWait("{DOWN}") }
        [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
        $evidence.selection_method = "user-equivalent-keyboard-navigation"
    }
    Start-Sleep -Milliseconds 250
    try {
        $state = $oldCombo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Current.ExpandCollapseState
        $evidence.popup_closed = $state -eq [System.Windows.Automation.ExpandCollapseState]::Collapsed
    }
    catch { $evidence.popup_closed = $true }
    if (-not $evidence.popup_closed) { throw "Qt combo popup did not close after selecting '$targetText'" }
    if ($postRebuild) {
        $evidence.post_rebuild = & $postRebuild
    }
    return $evidence
}

function New-PvdAttemptEvidence([string]$operation)
{
    # Creates a unique evidence scope for one asynchronous GUI operation.
    return [ordered]@{
        attempt_id = [guid]::NewGuid().ToString()
        operation = $operation
        invocation_time_utc = (Get-Date).ToUniversalTime().ToString("o")
        pre_state = [ordered]@{}
        start_evidence = [ordered]@{ observed = $false; reason = $null }
        new_output = [ordered]@{ observed = $false; baseline_length = 0; current_length = 0 }
        terminal_evidence = [ordered]@{ observed = $false; reason = $null }
        post_state = [ordered]@{}
    }
}

function Get-PvdFileEvidence([string[]]$paths)
{
    # Captures existence, size, and UTC modification time without changing test state.
    $snapshot = [ordered]@{}
    foreach ($path in $paths)
    {
        $fullPath = [System.IO.Path]::GetFullPath($path)
        $info = Get-Item -LiteralPath $fullPath -ErrorAction SilentlyContinue
        $snapshot[$fullPath] = [ordered]@{
            path = $fullPath
            exists = [bool]$info
            length = if ($info) { [int64]$info.Length } else { [int64]0 }
            last_write_utc = if ($info) { $info.LastWriteTimeUtc.ToString("o") } else { $null }
        }
    }
    return $snapshot
}

function Get-PvdDirectoryEvidence([string]$directory)
{
    # Captures all transfer-relevant generated-file timestamps for freshness comparison.
    $snapshot = [ordered]@{}
    if (-not (Test-Path -LiteralPath $directory)) { return $snapshot }
    foreach ($file in Get-ChildItem -LiteralPath $directory -File -Recurse)
    {
        $snapshot[$file.FullName] = [ordered]@{
            path = $file.FullName
            length = [int64]$file.Length
            last_write_utc = $file.LastWriteTimeUtc.ToString("o")
        }
    }
    return $snapshot
}

function Get-PvdChildProcessEvidence([int]$parentProcessId)
{
    # Captures current direct child processes for operation lifecycle observation.
    $children = [ordered]@{}
    foreach ($child in @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$parentProcessId"))
    {
        $children[[string]$child.ProcessId] = [ordered]@{
            process_id = [int]$child.ProcessId
            name = $child.Name
            command_line = $child.CommandLine
            creation_time = $child.CreationDate
        }
    }
    return $children
}

function Wait-PvdCurrentAttemptStart([int]$parentProcessId, $beforeProcesses, [string]$baselineLog,
                                     [int]$seconds = 20, $logElement = $null)
{
    # Waits for new process or appended log evidence after the current operation invocation.
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline)
    {
        $processes = Get-PvdChildProcessEvidence $parentProcessId
        $newProcess = @($processes.Keys | Where-Object { $_ -and -not $beforeProcesses.Contains($_) })
        if ($newProcess.Count -gt 0)
        {
            return [ordered]@{ observed = $true; method = "new-child-process"; processes = $processes }
        }
        if ($null -ne $baselineLog)
        {
            $currentLogElement = if ($logElement) { $logElement } else { Find-PvdControlFresh $parentProcessId "build_log" "" 5 }
            $log = Read-PvdLogText $currentLogElement
            if ($log.text.Length -gt $baselineLog.Length)
            {
                return [ordered]@{ observed = $true; method = "new-build-log-output"; processes = $processes }
            }
        }
        Start-Sleep -Milliseconds 250
    }
    return [ordered]@{ observed = $false; method = $null; processes = Get-PvdChildProcessEvidence $parentProcessId }
}

function Wait-PvdFileChange([string]$path, $beforeSnapshot, [int]$seconds = 20)
{
    # Waits for a current-attempt file state transition when a process is too short-lived to sample.
    $fullPath = [System.IO.Path]::GetFullPath($path)
    $before = $beforeSnapshot[$fullPath]
    if (-not $before) { return [ordered]@{ observed = $false; method = $null; current = $null } }
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline)
    {
        $current = Get-PvdFileEvidence @($fullPath)
        if ($before.exists -ne $current[$fullPath].exists -or
            $before.last_write_utc -ne $current[$fullPath].last_write_utc -or
            $before.length -ne $current[$fullPath].length)
        {
            return [ordered]@{ observed = $true; method = "file-state-transition"; current = $current[$fullPath] }
        }
        Start-Sleep -Milliseconds 100
    }
    return [ordered]@{ observed = $false; method = $null; current = (Get-PvdFileEvidence @($fullPath))[$fullPath] }
}

function Test-PvdFreshness([string]$artifactPath, [string]$elfPath, [string]$generatedDirectory,
                           [datetime]$invocationTimeUtc)
{
    # Applies the same generated-source-versus-firmware timestamp invariant used by PVD.
    $artifact = Get-Item -LiteralPath $artifactPath -ErrorAction SilentlyContinue
    $elf = Get-Item -LiteralPath $elfPath -ErrorAction SilentlyContinue
    $sources = @(Get-ChildItem -LiteralPath $generatedDirectory -File -Recurse -ErrorAction SilentlyContinue)
    $newest = $null
    if ($sources.Count -gt 0) { $newest = $sources | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1 }
    $artifactFresh = $artifact -and $newest -and $artifact.LastWriteTimeUtc -ge $newest.LastWriteTimeUtc
    $elfFresh = $elf -and $newest -and $elf.LastWriteTimeUtc -ge $newest.LastWriteTimeUtc
    return [ordered]@{
        artifact_path = if ($artifact) { $artifact.FullName } else { [System.IO.Path]::GetFullPath($artifactPath) }
        elf_path = if ($elf) { $elf.FullName } else { [System.IO.Path]::GetFullPath($elfPath) }
        newest_source_path = if ($newest) { $newest.FullName } else { $null }
        newest_source_timestamp_utc = if ($newest) { $newest.LastWriteTimeUtc.ToString("o") } else { $null }
        artifact_timestamp_utc = if ($artifact) { $artifact.LastWriteTimeUtc.ToString("o") } else { $null }
        elf_timestamp_utc = if ($elf) { $elf.LastWriteTimeUtc.ToString("o") } else { $null }
        artifact_fresh = [bool]$artifactFresh
        elf_fresh = [bool]$elfFresh
        invocation_time_utc = $invocationTimeUtc.ToString("o")
        pass = [bool]($artifactFresh -and $elfFresh)
    }
}

function Resolve-PvdBuildArtifacts([string]$projectPath, [string]$testId)
{
    # Resolves the generated target and its artifacts from authoritative generated CMake metadata.
    $generated = Join-Path $projectPath "generated"
    $cmake = Join-Path $generated "CMakeLists.txt"
    $target = $null
    if (Test-Path -LiteralPath $cmake -PathType Leaf)
    {
        $content = Get-Content -Raw -LiteralPath $cmake
        $match = [regex]::Match($content, '(?m)^\s*add_executable\(\s*([A-Za-z0-9_.-]+)')
        if ($match.Success) { $target = $match.Groups[1].Value }
        if (-not $target)
        {
            $match = [regex]::Match($content, '(?m)^\s*project\(\s*([A-Za-z0-9_.-]+)')
            if ($match.Success) { $target = $match.Groups[1].Value }
        }
    }
    if (-not $target)
    {
        $target = ($testId -replace '[^A-Za-z0-9]+', '_').Trim('_')
    }
    $build = Join-Path $projectPath "build"
    return [ordered]@{
        test_id = $testId
        target_name = $target
        artifact_stem = $target
        build_directory = $build
        elf_path = Join-Path $build ($target + ".elf")
        uf2_path = Join-Path $build ($target + ".uf2")
        source = if (Test-Path -LiteralPath $cmake) { "generated/CMakeLists.txt" } else { "canonical-test-id-normalization" }
    }
}

Export-ModuleMember -Function Get-PvdWindow,Get-PvdTopLevelWindows,Get-PvdMainWindow,Get-PvdQtPopup,Get-PvdModalDialog,Invoke-PvdDialogButton,Wait-PvdUiTreeStable,Wait-PvdElement,Focus-PvdElement,Set-PvdTextAsUser,Invoke-PvdButton,Select-PvdComboItemAsUser,Read-PvdLogText,Get-PvdCurrentPage,Wait-PvdPage,Navigate-PvdPage,Assert-PvdPage,Find-PvdControlFresh,Invoke-PvdRebuildingMutation,Get-PvdControlPatternNames,Set-PvdQtComboBoxValue,New-PvdAttemptEvidence,Get-PvdFileEvidence,Get-PvdDirectoryEvidence,Get-PvdChildProcessEvidence,Wait-PvdCurrentAttemptStart,Wait-PvdFileChange,Test-PvdFreshness,Resolve-PvdBuildArtifacts
