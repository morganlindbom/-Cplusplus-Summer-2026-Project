param(
    [string]$Executable = "build-fresh/pico_visual_designer.exe",
    [Parameter(Mandatory = $true)][string]$Database
)

$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "PvdGuiAutomation.psm1") -Force

function Find-PvdDialog([string]$pattern, [int]$seconds = 15) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $windows = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($window in $windows) {
            if ($window.Current.ControlType -eq [System.Windows.Automation.ControlType]::Window -and $window.Current.Name -match $pattern) { return $window }
        }
        Start-Sleep -Milliseconds 150
    }
    throw "Dialog not found: $pattern"
}

function Open-PvdProject([int]$processId, [string]$database) {
    $window = Get-PvdWindow $processId
    Invoke-PvdButton (Find-PvdControlFresh $processId "project_open" "") $processId | Out-Null
    $dialog = Find-PvdDialog "pico_visual_designer\.exe$"
    $edits = @($dialog.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Edit })
    $openedViaLocation = $false
    if ($edits.Count -gt 0) {
        $edit = $edits[$edits.Count - 1]
        try { [void]$edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($database) }
        catch {
            $edit.SetFocus()
            [System.Windows.Forms.SendKeys]::SendWait("^a")
            [System.Windows.Forms.SendKeys]::SendWait($database)
        }
    } else {
        $shell = New-Object -ComObject WScript.Shell
        [void]$shell.AppActivate($processId)
        Start-Sleep -Milliseconds 150
        [System.Windows.Forms.SendKeys]::SendWait("^l")
        [System.Windows.Forms.SendKeys]::SendWait($database)
        [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
        $openedViaLocation = $true
    }
    $open = @($dialog.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and $_.Current.Name -match "Open" }) |
        Select-Object -First 1
    if (-not $openedViaLocation -and $open) { Invoke-PvdButton $open $processId | Out-Null }
    $deadline = (Get-Date).AddSeconds(15)
    while ((Get-Date) -lt $deadline) {
        $dialogs = @([System.Windows.Automation.AutomationElement]::RootElement.FindAll(
            [System.Windows.Automation.TreeScope]::Children,
            [System.Windows.Automation.Condition]::TrueCondition) |
            Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Window -and $_.Current.Name -match "pico_visual_designer\.exe$" })
        if ($dialogs.Count -eq 0) { break }
        Start-Sleep -Milliseconds 150
    }
}

function Get-VisiblePvdControls($window) {
    @($window.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object {
            -not $_.Current.IsOffscreen -and
            $_.Current.BoundingRectangle.Width -gt 0 -and
            $_.Current.BoundingRectangle.Height -gt 0
        } |
        ForEach-Object {
            [ordered]@{
                automation_id = $_.Current.AutomationId
                name = $_.Current.Name
                control_type = $_.Current.ControlType.ProgrammaticName
                enabled = $_.Current.IsEnabled
                offscreen = $_.Current.IsOffscreen
            }
        })
}

$process = Start-Process -FilePath (Resolve-Path $Executable).Path -PassThru
try {
    $deadline = (Get-Date).AddSeconds(30)
    while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 200
        $process.Refresh()
    }
    Open-PvdProject $process.Id $Database
    $initialPage = (Get-PvdCurrentPage (Get-PvdWindow $process.Id)).name
    $pages = [ordered]@{}
    foreach ($pageName in @("Project", "Generate", "Build", "Transfer", "Debug")) {
        $page = Navigate-PvdPage $process.Id $pageName
        $window = Get-PvdWindow $process.Id
        $allControls = @($window.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition))
        $controls = @(Get-VisiblePvdControls $window)
        $pages[$pageName] = [ordered]@{
            current_page = $page.name
            transfer_firmware = @($controls | Where-Object { $_.automation_id -match "transfer_firmware" -or $_.name -eq "Transfer Firmware" })
            transfer_firmware_all = @($allControls | Where-Object { $_.Current.AutomationId -match "transfer_firmware" -or $_.Current.Name -eq "Transfer Firmware" } | ForEach-Object { [ordered]@{ automation_id = $_.Current.AutomationId; name = $_.Current.Name; control_type = $_.Current.ControlType.ProgrammaticName; enabled = $_.Current.IsEnabled; offscreen = $_.Current.IsOffscreen } })
            transfer_log = @($controls | Where-Object { $_.automation_id -match "transfer_log" -or $_.name -eq "" -and $_.control_type -match "Edit" })
            major_controls = @($controls | Where-Object { $_.automation_id -match "project_|generate_|configure_|build_|transfer_|debug_|workflow_navigation" -or $_.name -match "Generate|Configure|Build|Transfer|Debug|Project" })
        }
    }
    [ordered]@{
        reopened_initial_page = $initialPage
        pages = $pages
    } | ConvertTo-Json -Depth 10
}
finally {
    if ($process -and -not $process.HasExited) {
        $process.CloseMainWindow()
        Start-Sleep -Milliseconds 500
        if (-not $process.HasExited) { $process.Kill() }
    }
}
