param(
    [string]$Executable = 'build-fresh/pico_visual_designer.exe',
    [string]$ResultPath = 'validation/pin_certification/results/unsaved-dialog-runtime.json',
    [ValidateSet('Discard','Cancel')][string]$Action = 'Discard'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
Import-Module (Join-Path $PSScriptRoot 'gui/PvdGuiAutomation.psm1') -Force

function Dump-Uia($element) {
    return [ordered]@{
        name=$element.Current.Name; automation_id=$element.Current.AutomationId
        control_type=$element.Current.ControlType.ProgrammaticName; class_name=$element.Current.ClassName
        framework_id=$element.Current.FrameworkId; process_id=$element.Current.ProcessId
        native_window_handle=$element.Current.NativeWindowHandle; enabled=$element.Current.IsEnabled
        offscreen=$element.Current.IsOffscreen
        bounds=[ordered]@{x=$element.Current.BoundingRectangle.X;y=$element.Current.BoundingRectangle.Y;width=$element.Current.BoundingRectangle.Width;height=$element.Current.BoundingRectangle.Height}
    }
}

function Get-Ancestry($element) {
    $walker = [System.Windows.Automation.TreeWalker]::ControlViewWalker
    $items = @(); $current = $element
    for ($i=0; $i -lt 12 -and $current; $i++) {
        $items += ,(Dump-Uia $current)
        try { $current = $walker.GetParent($current) } catch { $current = $null }
    }
    return $items
}

function Get-PvdUiaElements([int]$ProcessId) {
    $all = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition)
    return @($all | Where-Object { $_.Current.ProcessId -eq $ProcessId -and
        $_.Current.ControlType -in @([System.Windows.Automation.ControlType]::Window,
          [System.Windows.Automation.ControlType]::Pane,[System.Windows.Automation.ControlType]::Text,
          [System.Windows.Automation.ControlType]::Button) } | ForEach-Object { Dump-Uia $_ })
}

function Set-Text($root, [string]$id, [string]$value, [int]$processId) {
    $element = Wait-PvdElement $root $id '' 15
    [void]$element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($value)
    Focus-PvdElement $element $processId
    [System.Windows.Forms.SendKeys]::SendWait('{TAB}')
    Start-Sleep -Milliseconds 300
}

$process = Start-Process -FilePath (Resolve-Path $Executable) -ArgumentList '--certification-dialogs' -PassThru
$evidence = [ordered]@{ process_id=$process.Id; action=$Action; before_close=$null; after_close=$null; dialog_text=$null; buttons=@(); dialog_ancestors=@{}; resolver=$null; process_alive_after_close=$false; exit_code=$null }
try {
    $deadline=(Get-Date).AddSeconds(30)
    while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250; $process.Refresh() }
    $root=Get-PvdWindow $process.Id
    $name = Wait-PvdElement $root 'project_name' '' 15
    $path = Wait-PvdElement $root 'project_path' '' 15
    [void]$name.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue('UNSAVED_DIALOG_DIAGNOSTIC')
    [void]$path.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue((Join-Path ([IO.Path]::GetTempPath()) ('pvd-unsaved-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))))
    Focus-PvdElement $path $process.Id; [System.Windows.Forms.SendKeys]::SendWait('{TAB}')
    [void](Wait-PvdElement $root 'project_create' '' 15).GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    Start-Sleep -Milliseconds 700
    $root=Get-PvdWindow $process.Id
    Set-Text $root 'project_name' 'UNSAVED_DIALOG_DIRTY' $process.Id
    $mainHwnd=[int64]$root.Current.NativeWindowHandle
    $evidence.before_close=[ordered]@{ main_hwnd=$mainHwnd; top_level_windows=@(Get-PvdTopLevelWindows $process.Id); uia_elements=@(Get-PvdUiaElements $process.Id); foreground_hwnd=[int64]([PvdWindowNative]::GetForegroundWindow().ToInt64()) }
    [void]$process.CloseMainWindow()
    Start-Sleep -Milliseconds 700
    $process.Refresh()
    $evidence.after_close=[ordered]@{ top_level_windows=@(Get-PvdTopLevelWindows $process.Id); uia_elements=@(Get-PvdUiaElements $process.Id); foreground_hwnd=[int64]([PvdWindowNative]::GetForegroundWindow().ToInt64()) }
    $elements=[System.Windows.Automation.AutomationElement]::RootElement.FindAll([System.Windows.Automation.TreeScope]::Descendants,[System.Windows.Automation.Condition]::TrueCondition)
    $text=$elements | Where-Object { $_.Current.ProcessId -eq $process.Id -and $_.Current.Name -eq 'The project has unsaved changes.' } | Select-Object -First 1
    if ($text) { $evidence.dialog_text=Dump-Uia $text; $evidence.dialog_ancestors.text=Get-Ancestry $text }
    foreach($label in @('Save','Discard','Cancel')) {
        $button=$elements | Where-Object { $_.Current.ProcessId -eq $process.Id -and $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and $_.Current.Name -eq $label } | Select-Object -First 1
        if ($button) { $evidence.buttons += [ordered]@{label=$label; element=(Dump-Uia $button); ancestors=Get-Ancestry $button} }
    }
    try {
        $resolved = Get-PvdModalDialog $process.Id
        if ($resolved) {
            $evidence.resolver = [ordered]@{found=$true; title=$resolved.title; class_name=$resolved.class_name; automation_id=$resolved.automation_id; hwnd=$resolved.hwnd; owner_hwnd=$resolved.owner_hwnd}
            $evidence.dialog_action = Invoke-PvdDialogButton -ProcessId $process.Id -Label $Action -Seconds 5
            $process.Refresh()
            $evidence.process_alive_after_action = -not $process.HasExited
            if ($Action -eq 'Cancel' -and -not $evidence.process_alive_after_action) { throw 'PVD exited after Cancel' }
            if ($Action -eq 'Cancel') {
                [void]$process.CloseMainWindow()
                Start-Sleep -Milliseconds 300
                $evidence.cleanup_action = Invoke-PvdDialogButton -ProcessId $process.Id -Label 'Discard' -Seconds 5
            }
        } else { $evidence.resolver = [ordered]@{found=$false} }
    } catch { $evidence.resolver_error=$_.Exception.Message }
    $process.Refresh(); $evidence.process_alive_after_close=-not $process.HasExited; if ($process.HasExited) { $evidence.exit_code=$process.ExitCode }
} catch { $evidence.error=$_.Exception.Message }
finally {
    $process.Refresh(); if ($process.HasExited) { $evidence.exit_code=$process.ExitCode }
    $parent=Split-Path -Parent $ResultPath; if($parent){New-Item -ItemType Directory -Force -Path $parent|Out-Null}
    $evidence|ConvertTo-Json -Depth 20|Set-Content -Encoding UTF8 $ResultPath
    $evidence|ConvertTo-Json -Depth 20
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
}
