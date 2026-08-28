# application_exit_openocd_selftest.ps1
param([string]$Executable = "build-current/pico_visual_designer.exe", [string]$Database = "PICO2W.sqlite")
$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "PvdGuiAutomation.psm1") -Force
function C($r,[string]$id){$r.FindAll([System.Windows.Automation.TreeScope]::Descendants,[System.Windows.Automation.Condition]::TrueCondition)|Where-Object{$_.Current.AutomationId -eq $id -or $_.Current.AutomationId.EndsWith("."+$id)}|Select-Object -First 1}
function I($x){[void]$x.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()}
function P{[bool](Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue)}
function O{@(Get-CimInstance Win32_Process -Filter "Name='openocd.exe'" -ErrorAction SilentlyContinue)}
$p=$null
try {
    Get-Process pico_visual_designer,openocd,arm-none-eabi-gdb -ErrorAction SilentlyContinue|Stop-Process -Force -ErrorAction SilentlyContinue
    $p=Start-Process -FilePath (Resolve-Path $Executable).Path -ArgumentList "--certification-dialogs" -PassThru
    Start-Sleep -Seconds 2
    $w=Get-PvdWindow $p.Id; I (C $w "project_open")
    $d=Wait-PvdElement (Get-PvdWindow $p.Id) "pvd_automation_file_dialog" ""
    $f=C $d "fileNameEdit"; $f.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue((Resolve-Path $Database).Path)
    $b=@($d.FindAll([System.Windows.Automation.TreeScope]::Descendants,[System.Windows.Automation.Condition]::TrueCondition)|Where-Object{$_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and $_.Current.Name -eq "Open"})|Select-Object -First 1; I $b
    Start-Sleep -Seconds 2; Navigate-PvdPage $p.Id "Debug"|Out-Null; Start-Sleep -Seconds 2
    $w=Get-PvdWindow $p.Id; I (C $w "debug_start")
    $deadline=(Get-Date).AddSeconds(20); while(-not(P) -and (Get-Date)-lt $deadline){Start-Sleep -Milliseconds 250}
    $ocd=@(O); "PVD_PID=$($p.Id)"; "OPENOCD_BEFORE_EXIT=$($ocd.ProcessId -join ',')"; "PORT_BEFORE_EXIT=$(P)"
    $p.CloseMainWindow(); $deadline=(Get-Date).AddSeconds(10); while(-not $p.HasExited -and (Get-Date)-lt $deadline){Start-Sleep -Milliseconds 250;$p.Refresh()}
    Start-Sleep -Seconds 2
    $left=@(O); $children=@(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue|Where-Object{$_.ParentProcessId -eq $p.Id -and $_.Name -eq "conhost.exe"})
    "PVD_EXITED=$($p.HasExited)"; "OPENOCD_AFTER_EXIT=$($left.ProcessId -join ',')"; "PORT_AFTER_EXIT=$(P)"; "CONHOST_AFTER_EXIT=$($children.ProcessId -join ',')"
} finally {
    if($p -and -not $p.HasExited){$p.Kill()}
}
