# direct_gdb_physical_cert.ps1
param([string]$Executable = "build-current/pico_visual_designer.exe", [string]$Database = "PICO2W.sqlite")
$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "PvdGuiAutomation.psm1") -Force
function C($r,[string]$id){$r.FindAll([System.Windows.Automation.TreeScope]::Descendants,[System.Windows.Automation.Condition]::TrueCondition)|Where-Object{$_.Current.AutomationId -eq $id -or $_.Current.AutomationId.EndsWith("."+$id)}|Select-Object -First 1}
function I($x){[void]$x.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()}
function P{[bool](Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue)}
$p=$null
try {
    Get-Process pico_visual_designer,openocd,arm-none-eabi-gdb -ErrorAction SilentlyContinue|Stop-Process -Force -ErrorAction SilentlyContinue
    $p=Start-Process -FilePath (Resolve-Path $Executable).Path -ArgumentList "--certification-dialogs" -PassThru
    Start-Sleep -Seconds 2; $w=Get-PvdWindow $p.Id; I (C $w "project_open")
    $d=Wait-PvdElement (Get-PvdWindow $p.Id) "pvd_automation_file_dialog" ""
    $f=C $d "fileNameEdit"; $f.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue((Resolve-Path $Database).Path)
    $b=@($d.FindAll([System.Windows.Automation.TreeScope]::Descendants,[System.Windows.Automation.Condition]::TrueCondition)|Where-Object{$_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and $_.Current.Name -eq "Open"})|Select-Object -First 1; I $b
    Start-Sleep -Seconds 2; Navigate-PvdPage $p.Id "Debug"|Out-Null; Start-Sleep -Seconds 2
    $w=Get-PvdWindow $p.Id; I (C $w "debug_start")
    $deadline=(Get-Date).AddSeconds(20); while(-not(P) -and (Get-Date)-lt $deadline){Start-Sleep -Milliseconds 250}
    if(-not(P)){throw "OpenOCD port 3333 did not become ready."}
    $g=(Get-ChildItem "$env:USERPROFILE/.pico-sdk/toolchain" -Recurse -Filter arm-none-eabi-gdb.exe|Select-Object -First 1).FullName
    $elf=(Resolve-Path "build/PICO2W.elf").Path
    $si=New-Object System.Diagnostics.ProcessStartInfo
    $si.FileName=$g; $si.Arguments='--batch --quiet --nx "'+$elf+'" -ex "set pagination off" -ex "set confirm off" -ex "target extended-remote localhost:3333" -ex "monitor reset halt" -ex "info registers" -ex "break main" -ex "step" -ex "next" -ex "backtrace" -ex "quit"'
    $si.UseShellExecute=$false; $si.CreateNoWindow=$true; $si.RedirectStandardOutput=$true; $si.RedirectStandardError=$true
    $gdb=New-Object System.Diagnostics.Process; $gdb.StartInfo=$si; $started=$gdb.Start(); "PVD_PID=$($p.Id)"; "GDB=$g"; "ELF=$elf"; "GDB_STARTED=$started"
    if(-not $gdb.WaitForExit(45000)){ $gdb.Kill(); throw "GDB timed out." }
    "GDB_EXIT=$($gdb.ExitCode)"; "GDB_STDOUT"; $gdb.StandardOutput.ReadToEnd(); "GDB_STDERR"; $gdb.StandardError.ReadToEnd()
    $w=Get-PvdWindow $p.Id; I (C $w "debug_stop"); Start-Sleep -Seconds 3
} finally {
    if($p -and -not $p.HasExited){$p.CloseMainWindow();Start-Sleep -Seconds 2;if(-not $p.HasExited){$p.Kill()}}
    Get-Process openocd,arm-none-eabi-gdb -ErrorAction SilentlyContinue|Stop-Process -Force -ErrorAction SilentlyContinue
}
