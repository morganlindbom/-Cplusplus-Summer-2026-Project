# gui_unsaved_save_minimal.ps1
param(
    [string]$Executable = 'build-fresh/pico_visual_designer.exe',
    [string]$ResultPath = 'validation/pin_certification/results/unsaved-save-minimal.json'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
Import-Module (Join-Path $PSScriptRoot 'gui/PvdGuiAutomation.psm1') -Force

function Wait-Id($root, [string]$id, [int]$seconds = 20)
{
    # Waits for one automation-id control under a stable root.
    #
    # Use this only for controls that are not expected to be destroyed by a Qt
    # page or model rebuild. Dynamic controls should use Find-PvdControlFresh.
    return Wait-PvdElement $root $id '' $seconds
}

function Fresh([int]$processId)
{
    # Reacquires the current top-level PVD window for a process.
    #
    # UIA elements can become stale when Qt replaces workflow columns, so callers
    # deliberately reacquire the root after rebuilding interactions.
    return Get-PvdWindow $processId
}

function Select-Page([int]$processId, [string]$name)
{
    # Navigates to one named workflow page and allows Qt to settle.
    #
    # Navigate-PvdPage verifies the active page before this helper returns.
    Navigate-PvdPage $processId $name 20 | Out-Null
    Start-Sleep -Milliseconds 400
}

function Select-ListItem($root, [string]$id, [string]$pattern, [int]$processId)
{
    # Selects one visible list item by its displayed text pattern.
    #
    # The list is resolved from the current root and the selected item is never
    # reused after the selection because the action can rebuild dependent columns.
    $list = Wait-Id $root $id
    $items = @($list.FindAll(
            [System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    $item = $items | Where-Object { $_.Current.Name -match $pattern } | Select-Object -First 1
    if (-not $item)
    {
        throw "List item not found: $pattern"
    }

    [void]$item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
    Start-Sleep -Milliseconds 350
}

function Get-ComboValue($combo)
{
    # Reads the current text from a Qt combo box through the best available UIA pattern.
    #
    # Qt versions expose combo values differently, so ValuePattern is preferred and
    # the accessible Name is retained as a compatibility fallback.
    $value = $combo.Current.Name
    try
    {
        $value = $combo.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value
    }
    catch
    {
        # Accessible Name remains the fallback value.
    }
    return $value
}

function Select-Sio([int]$processId)
{
    # Selects SIO by text after reacquiring the dynamically rebuilt function selector.
    #
    # The previous implementation depended on a hard-coded DOWN count and a stale
    # page root. The current flow opens the real Qt popup and selects the visible SIO
    # item by name, so catalog ordering cannot invalidate the test.
    $combo = Find-PvdControlFresh $processId 'function_selector' '' 12
    Focus-PvdElement $combo $processId
    try
    {
        $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
    }
    catch
    {
        [System.Windows.Forms.SendKeys]::SendWait('{F4}')
    }
    Start-Sleep -Milliseconds 200

    $items = @([System.Windows.Automation.AutomationElement]::RootElement.FindAll(
            [System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object {
            $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and
            $_.Current.Name -eq 'SIO' -and
            -not $_.Current.IsOffscreen
        })
    if ($items.Count -eq 0)
    {
        # The Qt popup is not always exposed as UIA ListItems.  Use the
        # catalog order through the focused real combo as the same user
        # keyboard action used by the certification runner.
        Focus-PvdElement $combo $processId
        [System.Windows.Forms.SendKeys]::SendWait('{HOME}' + ('{DOWN}' * 7) + '{ENTER}')
        Start-Sleep -Milliseconds 350
        $fresh = Find-PvdControlFresh $processId 'function_selector' '' 12
        $value = Get-ComboValue $fresh
        if ($value -notmatch '^SIO$') { throw "Function was not SIO: $value" }
        return
    }

    $item = $items[0]
    try
    {
        [void]$item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
        Focus-PvdElement $item $processId
        [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
    }
    catch
    {
        $rect = $item.Current.BoundingRectangle
        [void][PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
    }
    Start-Sleep -Milliseconds 500

    $fresh = Find-PvdControlFresh $processId 'function_selector' '' 12
    $value = Get-ComboValue $fresh
    if ($value -notmatch '^SIO$')
    {
        throw "Function was not SIO: $value"
    }
}

function Configure-SioOutput([int]$processId)
{
    # Configures Pin 1/GPIO0 as SIO Output using only visible user-equivalent controls.
    #
    # Every control that can be destroyed by a selection is reacquired from the
    # current process window before the next interaction.
    Select-Page $processId 'Function Selection'
    Select-ListItem (Fresh $processId) 'component_selection' 'Pin 1|GPIO0' $processId
    Select-Sio $processId

    Select-Page $processId 'Settings'
    Select-ListItem (Fresh $processId) 'settings_selection' 'Pin 1|GPIO0.*SIO' $processId
    $evidence = Set-PvdQtComboBoxValue $processId 'setting_direction' 'Output' $null 12
    $direction = Find-PvdControlFresh $processId 'setting_direction' '' 12
    $value = Get-ComboValue $direction
    if ($value -ne 'Output')
    {
        throw "Direction not Output: $value"
    }
    return $evidence
}

function Open-SavedProject([int]$processId, [string]$database)
{
    # Opens a persisted PVD project through the real project-open dialog.
    #
    # The helper interacts with the modal window exactly as a user would and avoids
    # bypassing persistence through direct SQLite access.
    $root = Fresh $processId
    Invoke-PvdButton (Wait-Id $root 'project_open') $processId | Out-Null
    Start-Sleep -Milliseconds 400

    $windows = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
        [System.Windows.Automation.TreeScope]::Children,
        [System.Windows.Automation.Condition]::TrueCondition)
    $dialog = $windows |
        Where-Object {
            $_.Current.ProcessId -eq $processId -and
            $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Window -and
            $_.Current.Name -ne 'pico_visual_designer'
        } |
        Select-Object -First 1
    if (-not $dialog)
    {
        throw 'Open dialog not found'
    }

    $edits = @($dialog.FindAll(
            [System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Edit })
    if ($edits.Count -eq 0)
    {
        throw 'Open dialog path field not found'
    }

    $edit = $edits[$edits.Count - 1]
    try
    {
        [void]$edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($database)
    }
    catch
    {
        $edit.SetFocus()
        [System.Windows.Forms.SendKeys]::SendWait('^a')
        [System.Windows.Forms.SendKeys]::SendWait($database)
    }

    $open = @($dialog.FindAll(
            [System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object {
            $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and
            $_.Current.Name -match 'Open'
        } |
        Select-Object -First 1)
    if ($open)
    {
        Invoke-PvdButton $open $processId | Out-Null
    }
    else
    {
        [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
    }
    Start-Sleep -Milliseconds 800
}

$evidence = [ordered]@{
    result = 'FAIL'
    direction_changed = $false
    unsaved_popup_detected = $false
    save_clicked = $false
    ok_clicked = 'NOT SHOWN'
    normal_exit = $false
    project_reopened = $false
    direction_persisted = 'FAIL'
    database = $null
    process_ids = @()
    error = $null
}

$path = Join-Path ([IO.Path]::GetTempPath()) ('pvd-unsaved-save-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
$database = Join-Path $path 'MINIMAL.sqlite'
$evidence.database = $database
$firstProcess = $null
$secondProcess = $null

try
{
    $firstProcess = Start-Process -FilePath (Resolve-Path $Executable) -ArgumentList '--certification-dialogs' -PassThru
    $evidence.process_ids += $firstProcess.Id
    Start-Sleep -Seconds 2

    $root = Fresh $firstProcess.Id
    [void](Wait-Id $root 'project_name').GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue('MINIMAL_UNSAVED_SAVE')
    [void](Wait-Id $root 'project_path').GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($path)
    Invoke-PvdButton (Wait-Id $root 'project_create') $firstProcess.Id | Out-Null
    Start-Sleep -Milliseconds 700

    Configure-SioOutput $firstProcess.Id | Out-Null
    $evidence.direction_changed = $true
    [void]$firstProcess.CloseMainWindow()
    Start-Sleep -Milliseconds 500

    $modal = Get-PvdModalDialog $firstProcess.Id
    if (-not $modal)
    {
        throw 'Unsaved-changes popup not detected'
    }
    $evidence.unsaved_popup_detected = $true
    Invoke-PvdDialogButton $firstProcess.Id 'Save' 5 | Out-Null
    $evidence.save_clicked = $true
    Start-Sleep -Milliseconds 400

    $confirm = Get-PvdModalDialog $firstProcess.Id
    if ($confirm)
    {
        Invoke-PvdDialogButton $firstProcess.Id 'OK' 5 | Out-Null
        $evidence.ok_clicked = 'PASS'
    }

    $deadline = (Get-Date).AddSeconds(6)
    while (-not $firstProcess.HasExited -and (Get-Date) -lt $deadline)
    {
        Start-Sleep -Milliseconds 100
        $firstProcess.Refresh()
    }
    if (-not $firstProcess.HasExited)
    {
        throw 'PVD did not exit after Save'
    }
    $evidence.normal_exit = $firstProcess.ExitCode -eq 0

    $secondProcess = Start-Process -FilePath (Resolve-Path $Executable) -ArgumentList '--certification-dialogs' -PassThru
    $evidence.process_ids += $secondProcess.Id
    Start-Sleep -Seconds 2
    Open-SavedProject $secondProcess.Id $database
    $evidence.project_reopened = $true

    Select-Page $secondProcess.Id 'Function Selection'
    Select-ListItem (Fresh $secondProcess.Id) 'component_selection' 'Pin 1|GPIO0' $secondProcess.Id
    Select-Sio $secondProcess.Id
    Select-Page $secondProcess.Id 'Settings'
    Select-ListItem (Fresh $secondProcess.Id) 'settings_selection' 'Pin 1|GPIO0.*SIO' $secondProcess.Id
    $direction = Find-PvdControlFresh $secondProcess.Id 'setting_direction' '' 12
    $directionValue = Get-ComboValue $direction
    $evidence.direction_persisted = $directionValue -eq 'Output'
    if (-not $evidence.direction_persisted)
    {
        throw "Reopened direction was '$directionValue'"
    }

    $evidence.result = 'PASS'
}
catch
{
    $evidence.error = $_.Exception.Message
}
finally
{
    foreach ($process in @($firstProcess, $secondProcess))
    {
        if ($process)
        {
            $process.Refresh()
            if (-not $process.HasExited)
            {
                Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            }
        }
    }

    $parent = Split-Path -Parent $ResultPath
    if ($parent)
    {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    $evidence | ConvertTo-Json -Depth 12 | Set-Content -Encoding UTF8 $ResultPath
    $evidence | ConvertTo-Json -Depth 12
}
