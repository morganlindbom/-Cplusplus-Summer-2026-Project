param(
    [string]$Executable = "build-fresh/pico_visual_designer.exe",
    [string]$ResultPath = "validation/pin_certification/results/sio-physical-baseline.json",
    [int]$TimeoutSeconds = 180,
    [string[]]$TestIds = @(),
    [switch]$StopBeforeTransfer,
    [switch]$GuiOnly
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
Import-Module (Join-Path $PSScriptRoot "gui/PvdGuiAutomation.psm1") -Force

$pairs = @(
    @{ id = "SIO-PAIR-A-FWD"; pair = "A"; source = 0; destination = 1 },
    @{ id = "SIO-PAIR-A-REV"; pair = "A"; source = 1; destination = 0 },
    @{ id = "SIO-PAIR-B-FWD"; pair = "B"; source = 2; destination = 3 },
    @{ id = "SIO-PAIR-B-REV"; pair = "B"; source = 3; destination = 2 },
    @{ id = "SIO-PAIR-C-FWD"; pair = "C"; source = 4; destination = 5 },
    @{ id = "SIO-PAIR-C-REV"; pair = "C"; source = 5; destination = 4 },
    @{ id = "SIO-PAIR-D-FWD"; pair = "D"; source = 6; destination = 7 },
    @{ id = "SIO-PAIR-D-REV"; pair = "D"; source = 7; destination = 6 }
)
$requestedTestIds = @($TestIds | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
if ($requestedTestIds.Count -gt 0) { $pairs = @($pairs | Where-Object { $requestedTestIds -contains $_.id }) }
$results = [ordered]@{}
$exePath = (Resolve-Path $Executable).Path
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("pvd-sio-" + [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssfffZ"))
New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null

function Stage([System.Collections.IDictionary]$stages, [string]$name, [string]$status, [string]$evidence) {
    [void]($stages[$name] = [ordered]@{ status = $status; evidence = $evidence })
}

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
    try { $element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke(); return } catch {}
    try { $element.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select(); return } catch {}
    throw "GUI element is not invokable/selectable: $($element.Current.Name)"
}

function Set-Value($element, [string]$value) {
    [void]$element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($value)
}

function Get-ElementText($element) {
    return (Read-PvdLogText $element).text
}

function Find-LivePvdElement([int]$processId, [string]$automationId, [int]$seconds = 12) {
    # Qt rebuilds invalidate every descendant AutomationElement. Always start
    # from a freshly reacquired top-level PVD window and rediscover the control.
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $root = Get-PvdWindow $processId
        $items = @($root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition) |
            Where-Object {
                $_.Current.AutomationId -eq $automationId -or
                $_.Current.AutomationId.EndsWith("." + $automationId)
            })
        foreach ($item in $items) {
            if ($item.Current.IsEnabled -and -not $item.Current.IsOffscreen -and
                $item.Current.BoundingRectangle.Width -gt 0 -and $item.Current.BoundingRectangle.Height -gt 0) {
                return $item
            }
        }
        Start-Sleep -Milliseconds 150
    }
    $diagnosticRoot = Get-PvdWindow $processId
    $available = @($diagnosticRoot.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.AutomationId -or $_.Current.Name } |
        Select-Object -First 80 |
        ForEach-Object { "id=$($_.Current.AutomationId);name=$($_.Current.Name);type=$($_.Current.ControlType.ProgrammaticName)" })
    throw "Live GUI element not found after rebuild: id='$automationId'; available=$($available -join ' | ')"
}

function Get-ControlEvidence($element) {
    $value = $null
    $patterns = [ordered]@{}
    foreach ($entry in @(
            @{ name = "ValuePattern"; type = [System.Windows.Automation.ValuePattern]::Pattern },
            @{ name = "ExpandCollapsePattern"; type = [System.Windows.Automation.ExpandCollapsePattern]::Pattern },
            @{ name = "SelectionItemPattern"; type = [System.Windows.Automation.SelectionItemPattern]::Pattern })) {
        try {
            $pattern = $element.GetCurrentPattern($entry.type)
            $patterns[$entry.name] = $true
            if ($entry.name -eq "ValuePattern") { $value = $pattern.Current.Value }
        } catch { $patterns[$entry.name] = $false }
    }
    return [ordered]@{
        automation_id = $element.Current.AutomationId
        name = $element.Current.Name
        control_type = $element.Current.ControlType.ProgrammaticName
        value = $value
        patterns = $patterns
        bounds = [ordered]@{ x = $element.Current.BoundingRectangle.X; y = $element.Current.BoundingRectangle.Y; width = $element.Current.BoundingRectangle.Width; height = $element.Current.BoundingRectangle.Height }
        enabled = $element.Current.IsEnabled
        offscreen = $element.Current.IsOffscreen
    }
}

function Select-ListItem($root, [string]$listId, [string]$name) {
    $list = Wait-Element $root $listId ""
    $items = @($list.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    foreach ($item in $items) {
        if ($item.Current.Name -eq $name) {
            try { [void]$item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select() }
            catch { [void]$item.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke() }
            Start-Sleep -Milliseconds 350
            return $item
        }
    }
    throw "List item not found: $name"
}

function Select-ComboItem($combo, [string]$value, [int]$processId) {
    Focus-PvdElement $combo $processId
    $rect = $combo.Current.BoundingRectangle
    [PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
    Start-Sleep -Milliseconds 250
    $deadline = (Get-Date).AddSeconds(5)
    while ((Get-Date) -lt $deadline) {
        $items = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
            [System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition)
        foreach ($item in $items) {
            $nameMatches = $item.Current.Name -eq $value -or ($value -eq "SIO" -and $item.Current.Name -eq "sio")
            if ($item.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and
                $nameMatches -and -not $item.Current.IsOffscreen) {
                $itemRect = $item.Current.BoundingRectangle
                [PvdMouse]::Click([int]($itemRect.X + $itemRect.Width / 2), [int]($itemRect.Y + $itemRect.Height / 2))
                Start-Sleep -Milliseconds 350
                return
            }
        }
        Start-Sleep -Milliseconds 100
    }
    Focus-PvdElement $combo $processId
    [System.Windows.Forms.SendKeys]::SendWait("{HOME}")
    [System.Windows.Forms.SendKeys]::SendWait($value)
    [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
    Start-Sleep -Milliseconds 350
}

function Select-Function($root, [string]$functionName, [int]$processId, [int]$gpio) {
    $combo = Wait-Element $root "function_selector" ""
    if ($functionName -eq "SIO") {
        # This Qt build does not expose the function popup consistently through UIA.
        # Use the authoritative per-GPIO catalog order as the user-equivalent fallback.
        $down = if ($gpio -eq 0) { 7 } else { 6 }
        Focus-PvdElement $combo $processId
        $rect = $combo.Current.BoundingRectangle
        [PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
        Start-Sleep -Milliseconds 150
        [System.Windows.Forms.SendKeys]::SendWait("{HOME}" + ("{DOWN}" * $down) + "{ENTER}")
        Start-Sleep -Milliseconds 350
    }
    else {
        Select-ComboItem $combo $functionName $processId
    }
    $freshRoot = Get-PvdWindow $processId
    $freshCombo = Wait-Element $freshRoot "function_selector" ""
    $observed = $null
    try { $observed = $freshCombo.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value } catch {}
    if (-not $observed) { $observed = $freshCombo.Current.Name }
    if ($functionName -eq "SIO" -and $observed -notin @("SIO", "sio")) {
        throw "Function Selection did not commit SIO; observed '$observed'"
    }
    return $functionName
}

function Select-Component($root, [string]$name) {
    $list = Wait-Element $root "component_selection" ""
    $items = @($list.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    for ($index = 0; $index -lt $items.Count; $index++) {
        if ($items[$index].Current.Name -eq $name) {
            $list.SetFocus()
            [System.Windows.Forms.SendKeys]::SendWait("{HOME}" + ("{DOWN}" * $index) + "{ENTER}")
            Start-Sleep -Milliseconds 350
            return
        }
    }
    throw "Component was not found: $name"
}

function Select-SioDirection($combo, [string]$direction, [int]$processId) {
    Focus-PvdElement $combo $processId
    try { $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand() }
    catch { [System.Windows.Forms.SendKeys]::SendWait("{F4}") }
    Start-Sleep -Milliseconds 200
    $deadline = (Get-Date).AddSeconds(8)
    while ((Get-Date) -lt $deadline) {
        $root = Get-PvdWindow $processId
        $items = @($root.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition) |
            Where-Object {
                $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and
                $_.Current.Name -eq $direction -and -not $_.Current.IsOffscreen
            })
        if ($items.Count -gt 0) {
            $item = $items[0]
            $rect = $item.Current.BoundingRectangle
            [PvdMouse]::Click([int]($rect.X + $rect.Width / 2), [int]($rect.Y + $rect.Height / 2))
            Start-Sleep -Milliseconds 300
            return
        }
        Start-Sleep -Milliseconds 100
    }
    throw "Direction popup item not found: '$direction'"
}

function Select-SioSettingsRow($root, [int]$physicalPin, [int]$processId = 0, [int]$windowHandle = 0) {
    $deadline = (Get-Date).AddSeconds(20)
    $observedNames = New-Object System.Collections.Generic.List[string]
    while ((Get-Date) -lt $deadline) {
        if ($windowHandle -gt 0) { $root = [System.Windows.Automation.AutomationElement]::FromHandle($windowHandle) }
        elseif ($processId -gt 0) { $root = Get-PvdWindow $processId }
        $lists = @($root.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
            Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::List })
        foreach ($settingsList in $lists) {
            $items = @($settingsList.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
                Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
            foreach ($item in $items) {
                $itemName = $item.Current.Name
                if ($itemName -and -not $observedNames.Contains($itemName)) { $observedNames.Add($itemName) }
                $gpio = $physicalPin - 1
                if ($itemName -eq ("Pin " + $physicalPin) -or
                    (($itemName -match "Pin\s+$physicalPin\b" -or $itemName -match "GPIO$gpio\b") -and $itemName -match "\bSIO\b")) {
                    try { [void]$item.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select() }
                    catch { [void]$item.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke() }
                    $selectedDeadline = (Get-Date).AddSeconds(8)
                    while ((Get-Date) -lt $selectedDeadline) {
                        $freshRoot = if ($processId -gt 0) { Get-PvdWindow $processId } else { $root }
                        $freshItems = @($freshRoot.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                                [System.Windows.Automation.Condition]::TrueCondition) |
                            Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem -and $_.Current.Name -eq $itemName })
                        foreach ($freshItem in $freshItems) {
                            $viewerDeadline = (Get-Date).AddSeconds(8)
                            while ((Get-Date) -lt $viewerDeadline) {
                                $viewerMatches = @($freshRoot.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                                        [System.Windows.Automation.Condition]::TrueCondition) |
                                    Where-Object { $_.Current.Name -match ("^Pin " + $physicalPin + ".*SIO$") })
                                if ($viewerMatches.Count -gt 0) { return $itemName }
                                Start-Sleep -Milliseconds 120
                                $freshRoot = if ($processId -gt 0) { Get-PvdWindow $processId } else { $root }
                            }
                        }
                        Start-Sleep -Milliseconds 120
                    }
                    throw "Settings row did not become selected for physical Pin $physicalPin"
                }
            }
        }
        Start-Sleep -Milliseconds 200
    }
    throw "SIO Settings row not found for physical Pin $physicalPin; observed=$($observedNames -join ' | ')"
}

function Configure-SioPin($root, [int]$gpio, [string]$direction, [int]$processId) {
    $root = Get-PvdWindow $processId
    $windowHandle = [int]$root.Current.NativeWindowHandle
    $physicalPin = $gpio + 1
    $null = Select-Workflow $root "Function Selection" $processId
    $root = Get-PvdWindow $processId
    Select-Component $root ("Pin " + $physicalPin)
    $null = Select-Function $root "SIO" $processId $gpio
    $null = Select-Workflow (Get-PvdWindow $processId) "Settings" $processId
    # Selecting the Settings row can rebuild SettingsColumn3. Do not reuse
    # either the previous root or any child obtained before that rebuild.
    $rowName = Select-SioSettingsRow $root $physicalPin $processId
    $directionCombo = Find-LivePvdElement $processId "setting_direction" 20
    Select-SioDirection $directionCombo $direction $processId
    # Direction changes rebuild SettingsColumn3. The old combo and parent are
    # intentionally discarded; poll the live tree until the rebuilt control is
    # visible and enabled.
    $directionControl = Find-LivePvdElement $processId "setting_direction" 20
    $observedDirection = $null
    try { $observedDirection = $directionControl.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value } catch { $observedDirection = $directionControl.Current.Name }
    if ($observedDirection -ne $direction) { throw "SIO direction did not commit for GPIO$gpio; observed '$observedDirection', expected '$direction'" }
    $root = Get-PvdWindow $processId
    $settings = Wait-Element $root "workflow_column3_stack" ""
    $conditionalControl = if ($direction -eq "Output") { "setting_initial_state" } else { "setting_debounce_ms" }
    $conditionalVisible = @($settings.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.AutomationId -eq $conditionalControl -or $_.Current.AutomationId.EndsWith("." + $conditionalControl) }).Count -gt 0
    return [ordered]@{ gpio = $gpio; physical_pin = $physicalPin; row = $rowName; function = "SIO"; direction = $direction; direction_control = Get-ControlEvidence $directionControl; conditional_control = $conditionalControl; conditional_visible = $conditionalVisible }
}

function Select-SettingCombo($root, [string]$current, [string]$value) {
    $settings = Wait-Element $root "workflow_column3_stack" ""
    $combos = @($settings.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object {
            $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ComboBox -and
            -not $_.Current.IsOffscreen -and
            $_.Current.BoundingRectangle.Width -gt 0 -and
            $_.Current.BoundingRectangle.Height -gt 0
        })
    foreach ($combo in $combos) {
        try {
            $pattern = $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
            $pattern.Expand()
            Start-Sleep -Milliseconds 100
            $items = @($combo.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition))
            if (($items | Where-Object { $_.Current.Name -eq $value }).Count -gt 0) {
                Select-ComboItem $combo $value
                return
            }
        } catch {}
    }
    throw "Visible settings combo with value '$value' not found (current='$current', visible_count=$($combos.Count))"
}

function Select-Workflow($root, [string]$name, [int]$processId = 0) {
    if ($processId -gt 0) { $root = Get-PvdWindow $processId }
    $navigation = Wait-Element $root "workflow_navigation" ""
    $items = @($navigation.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::ListItem })
    $item = $null
    $index = -1
    for ($candidateIndex = 0; $candidateIndex -lt $items.Count; $candidateIndex++) {
        if ($items[$candidateIndex].Current.Name -eq $name) {
            $item = $items[$candidateIndex]
            $index = $candidateIndex
            break
        }
    }
    if ($item -and $index -ge 0) {
        $navigation.SetFocus()
        [System.Windows.Forms.SendKeys]::SendWait("{HOME}" + ("{DOWN}" * $index) + "{ENTER}")
        Start-Sleep -Milliseconds 350
        if ($processId -gt 0) {
            $fresh = Get-PvdWindow $processId
            $stack = Wait-Element $fresh "workflow_column3_stack" ""
            $expectedId = switch ($name) {
                "Settings" { "settings_selection" }
                "Project" { "project_save" }
                default { $null }
            }
            if ($expectedId) {
                $expectedRoot = if ($name -eq "Settings") { $fresh } else { $stack }
                $null = Wait-Element $expectedRoot $expectedId "" 12
            }
        }
        return $item
    }
    throw "Workflow page not found: $name"
}

function Wait-File([string]$path, [int]$seconds = 90) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while (-not (Test-Path -LiteralPath $path) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 500 }
    if (-not (Test-Path -LiteralPath $path)) { throw "File did not appear: $path" }
}

function Save-CurrentProject($window, [int]$processId, [string]$projectPath) {
    $null = Select-Workflow (Get-PvdWindow $processId) "Project" $processId
    $freshWindow = Get-PvdWindow $processId
    $save = Wait-Element $freshWindow "project_save" "" 12
    $null = Invoke-Element $save
    $expected = "Project saved in: " + $projectPath
    $null = Wait-Element (Get-PvdWindow $processId) "project_dirty_status" $expected 12
    return Get-PvdWindow $processId
}

function Open-ExistingProject([int]$processId, [string]$database) {
    $root = Get-PvdWindow $processId
    $null = Invoke-Element (Wait-Element $root "project_open" "")
    $dialog = Wait-Element ([System.Windows.Automation.AutomationElement]::RootElement) "pvd_automation_file_dialog" "" 12
    $edits = @($dialog.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Edit })
    if ($edits.Count -eq 0) { throw "Open project dialog path field not found" }
    Set-Value $edits[$edits.Count - 1] $database
    $open = @($dialog.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) |
        Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button -and $_.Current.Name -match "Open" }) |
        Select-Object -First 1
    if (-not $open) { throw "Open project dialog button not found" }
    $null = Invoke-Element $open
    Start-Sleep -Milliseconds 700
    return Get-PvdWindow $processId
}

function Observe-SioPin([int]$processId, [int]$gpio) {
    $physicalPin = $gpio + 1
    $root = Get-PvdWindow $processId
    $null = Select-Workflow $root "Function Selection" $processId
    $root = Get-PvdWindow $processId
    Select-Component $root ("Pin " + $physicalPin)
    # Read-only observation: the reopened/persisted function is restored by
    # the application. Do not reselect SIO, because that is a mutation path.
    $null = Select-Workflow (Get-PvdWindow $processId) "Settings" $processId
    $rowName = Select-SioSettingsRow (Get-PvdWindow $processId) $physicalPin $processId
    $directionControl = Find-LivePvdElement $processId "setting_direction" 20
    $value = $null
    try { $value = $directionControl.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value }
    catch { $value = $directionControl.Current.Name }
    return [ordered]@{ gpio = $gpio; physical_pin = $physicalPin; row = $rowName; direction = $value; control = Get-ControlEvidence $directionControl }
}

function Get-FileEvidence([string]$path) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        return [ordered]@{ path = $path; exists = $false }
    }
    $item = Get-Item -LiteralPath $path
    return [ordered]@{ path = $path; exists = $true; last_write_time_utc = $item.LastWriteTimeUtc.ToString("o"); length = $item.Length }
}

function Assert-CurrentFile([string]$path, [DateTime]$invocation, [string]$label) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "$label did not produce file: $path" }
    $item = Get-Item -LiteralPath $path
    if ($item.LastWriteTimeUtc -lt $invocation.ToUniversalTime()) {
        throw "$label evidence is stale: $path ($($item.LastWriteTimeUtc.ToString('o')) < $($invocation.ToUniversalTime().ToString('o')))"
    }
    return Get-FileEvidence $path
}

function OpenOcd-Command([string]$command) {
    $client = New-Object System.Net.Sockets.TcpClient
    $client.Connect("127.0.0.1", 4444)
    $stream = $client.GetStream()
    $stream.ReadTimeout = 1500
    $bytes = [Text.Encoding]::ASCII.GetBytes($command + "`n")
    $stream.Write($bytes, 0, $bytes.Length)
    Start-Sleep -Milliseconds 200
    $buffer = New-Object byte[] 8192
    $text = ""
    try { while ($stream.DataAvailable) { $n = $stream.Read($buffer, 0, $buffer.Length); $text += [Text.Encoding]::ASCII.GetString($buffer, 0, $n) } } catch {}
    $client.Close()
    return $text
}

function Read-GpioInput([int]$gpio) {
    $response = OpenOcd-Command "mdw 0xd0000004 1"
    $matches = [regex]::Matches($response, "0x([0-9a-fA-F]{8})")
    if ($matches.Count -eq 0) { throw "No GPIO_IN value in OpenOCD response: $response" }
    $value = [Convert]::ToUInt32($matches[$matches.Count - 1].Groups[1].Value, 16)
    return [ordered]@{ gpio = $gpio; value_hex = ("0x{0:X8}" -f $value); high = (($value -band (1 -shl $gpio)) -ne 0); raw = $response.Trim() }
}

function Drive-Source([int]$gpio, [bool]$high) {
    $value = if ($high) { (1 -shl $gpio) } else { 0 }
    $response = OpenOcd-Command ("mww 0xd0000010 0x{0:X8}" -f $value)
    return $response.Trim()
}

function Run-Test($case) {
    $stages = [ordered]@{}
    foreach ($stage in @("GUI","Persistence","Generate","Configure","Build","Transfer","Runtime","Debug","Physical")) { $stages[$stage] = [ordered]@{ status = "NOT-TESTED"; evidence = "Not reached" } }
    $process = $null
    $attemptId = "$($case.id)-$([DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))"
    $evidence = [ordered]@{ attempt_id = $attemptId; operations = [ordered]@{} }
    $projectName = $case.id
    $projectPath = Join-Path $tempRoot $projectName
    $database = Join-Path $projectPath ($projectName + ".sqlite")
    try {
        $process = Start-Process -FilePath $exePath -ArgumentList "--certification-dialogs" -PassThru
        $deadline = (Get-Date).AddSeconds(30)
        while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250; $process.Refresh() }
        if ($process.MainWindowHandle -eq 0) { throw "PVD MainWindow did not appear" }
        $window = [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
        $name = Wait-Element $window "project_name" ""; $path = Wait-Element $window "project_path" ""
        Set-Value $name $projectName; Set-Value $path $projectPath
        $null = Invoke-Element (Wait-Element $window "project_create" "")
        $null = Wait-Element $window "project_dirty_status" ("Project saved in: " + $projectPath)
        $sourceSettings = Configure-SioPin $window $case.source "Output" $process.Id
        $evidence.after_source_before_destination = Observe-SioPin $process.Id $case.source
        if ($evidence.after_source_before_destination.direction -ne "Output") { throw "Source direction was not retained before configuring destination" }
        $window = Save-CurrentProject $window $process.Id $projectPath
        $destinationSettings = Configure-SioPin $window $case.destination "Input" $process.Id
        $evidence.after_destination_before_save_source = Observe-SioPin $process.Id $case.source
        $evidence.after_destination_before_save_destination = Observe-SioPin $process.Id $case.destination
        $window = Save-CurrentProject $window $process.Id $projectPath
        $evidence.settings = [ordered]@{ source = $sourceSettings; destination = $destinationSettings }
        Stage $stages "GUI" "PASS-GUI" "Real PVD GUI created project, selected both pins, selected SIO, and configured source/destination roles."
        Stage $stages "Persistence" "PASS-PERSISTENCE" "Configuration saved through visible Save Project control."
        if ($GuiOnly) {
            $evidence.persistence = [ordered]@{}
            $evidence.persistence.after_save_source = Observe-SioPin $process.Id $case.source
            $evidence.persistence.after_save_destination = Observe-SioPin $process.Id $case.destination
            $evidence.persistence.leave_return = $evidence.persistence.after_save_source.direction -eq "Output" -and
                $evidence.persistence.after_save_destination.direction -eq "Input"

            [void]$process.CloseMainWindow()
            Start-Sleep -Milliseconds 700
            $evidence.persistence.close_after_save_popup = [bool](Get-PvdModalDialog $process.Id)
            if ($evidence.persistence.close_after_save_popup) { throw "Unexpected unsaved-changes popup after explicit Save" }
            $exitDeadline = (Get-Date).AddSeconds(8)
            while (-not $process.HasExited -and (Get-Date) -lt $exitDeadline) { Start-Sleep -Milliseconds 100; $process.Refresh() }
            if (-not $process.HasExited) { throw "PVD did not close after explicit Save" }
            $process = Start-Process -FilePath $exePath -ArgumentList "--certification-dialogs" -PassThru
            $evidence.persistence.reopen_process_id = $process.Id
            Start-Sleep -Seconds 2
            $null = Open-ExistingProject $process.Id $database
            $evidence.persistence.reopen_source = Observe-SioPin $process.Id $case.source
            $evidence.persistence.reopen_destination = Observe-SioPin $process.Id $case.destination
            if ($evidence.persistence.reopen_source.direction -ne "Output" -or
                $evidence.persistence.reopen_destination.direction -ne "Input") { throw "Saved directions did not survive reopen" }

            $reverseSettings = Configure-SioPin (Get-PvdWindow $process.Id) $case.source "Input" $process.Id
            $processWindow = Save-CurrentProject (Get-PvdWindow $process.Id) $process.Id $projectPath
            $evidence.persistence.reverse_saved = $reverseSettings
            [void]$process.CloseMainWindow()
            Start-Sleep -Milliseconds 700
            $evidence.persistence.reverse_close_popup = [bool](Get-PvdModalDialog $process.Id)
            if ($evidence.persistence.reverse_close_popup) { throw "Unexpected unsaved-changes popup after reverse Save" }
            $exitDeadline = (Get-Date).AddSeconds(8)
            while (-not $process.HasExited -and (Get-Date) -lt $exitDeadline) { Start-Sleep -Milliseconds 100; $process.Refresh() }
            if (-not $process.HasExited) { throw "PVD did not close after reverse Save" }
            $process = Start-Process -FilePath $exePath -ArgumentList "--certification-dialogs" -PassThru
            Start-Sleep -Seconds 2
            $null = Open-ExistingProject $process.Id $database
            $evidence.persistence.reverse_reopen = Observe-SioPin $process.Id $case.source
            if ($evidence.persistence.reverse_reopen.direction -ne "Input") { throw "Reverse Input did not survive reopen" }

            $null = Configure-SioPin (Get-PvdWindow $process.Id) $case.source "Output" $process.Id
            [void]$process.CloseMainWindow()
            Start-Sleep -Milliseconds 700
            $evidence.persistence.unsaved_popup = [bool](Get-PvdModalDialog $process.Id)
            if (-not $evidence.persistence.unsaved_popup) { throw "Expected unsaved-changes popup was not shown" }
            $null = Invoke-PvdDialogButton $process.Id "Save" 8
            $evidence.persistence.unsaved_save_clicked = $true
            Start-Sleep -Milliseconds 400
            if (Get-PvdModalDialog $process.Id) {
                $null = Invoke-PvdDialogButton $process.Id "OK" 8
                $evidence.persistence.unsaved_ok_clicked = $true
            }
            $exitDeadline = (Get-Date).AddSeconds(8)
            while (-not $process.HasExited -and (Get-Date) -lt $exitDeadline) { Start-Sleep -Milliseconds 100; $process.Refresh() }
            if (-not $process.HasExited) { throw "PVD did not close after popup Save" }
            $process = Start-Process -FilePath $exePath -ArgumentList "--certification-dialogs" -PassThru
            Start-Sleep -Seconds 2
            $null = Open-ExistingProject $process.Id $database
            $evidence.persistence.unsaved_reopen = Observe-SioPin $process.Id $case.source
            if ($evidence.persistence.unsaved_reopen.direction -ne "Output") { throw "Popup Save direction did not survive reopen" }

            Stage $stages "Generate" "NOT-TESTED" "GUI-only persistence run stopped before Generate."
            return ,([ordered]@{ test_id = $case.id; attempt_id = $attemptId; timestamp_utc = [DateTime]::UtcNow.ToString("o"); pair = $case.pair; source_gpio = $case.source; destination_gpio = $case.destination; series_resistance_ohm = 1180; stages = $stages; current_attempt_evidence = $evidence; project_path = $projectPath; database = $database })
        }
        $generateInvocation = [DateTime]::UtcNow
        $generatePaths = @((Join-Path $projectPath "generated/main.cpp"), (Join-Path $projectPath "generated/CMakeLists.txt"))
        $evidence.operations.Generate = [ordered]@{ attempt_id = $attemptId; pre_state = @($generatePaths | ForEach-Object { Get-FileEvidence $_ }); invocation_utc = $generateInvocation.ToString("o") }
        $null = Select-Workflow $window "Generate"; $generateLogBefore = ""; try { $generateLogBefore = Get-ElementText (Wait-Element $window "generate_log" "" 5) } catch {}; $null = Invoke-Element (Wait-Element $window "generate_project" ""); $generateDeadline = (Get-Date).AddSeconds(60); while ((Get-Date) -lt $generateDeadline) { $current = @($generatePaths | ForEach-Object { Get-FileEvidence $_ }); $changed = @($current | Where-Object { $_.exists -and -not ($evidence.operations.Generate.pre_state | Where-Object path -eq $_.path).exists -or $_.exists -and $_.last_write_time_utc -ne (($evidence.operations.Generate.pre_state | Where-Object path -eq $_.path).last_write_time_utc) }); if ($changed.Count -gt 0) { break }; Start-Sleep -Milliseconds 300 }; if (-not (Test-Path -LiteralPath (Join-Path $projectPath "generated/main.cpp"))) { throw "Generate current-attempt resulting state not observed" }
        $evidence.operations.Generate.post_state = @($generatePaths | ForEach-Object { Get-FileEvidence $_ })
        if (-not (Assert-CurrentFile (Join-Path $projectPath "generated/main.cpp") $generateInvocation "Generate")) { throw "Generate current-attempt evidence missing" }
        Stage $stages "Generate" "PASS-GENERATE" "Current-attempt Generate completed; generated/main.cpp is fresh."
        $configureInvocation = [DateTime]::UtcNow
        $configureCache = Join-Path $projectPath "build/CMakeCache.txt"
        $evidence.operations.Configure = [ordered]@{ attempt_id = $attemptId; pre_state = Get-FileEvidence $configureCache; invocation_utc = $configureInvocation.ToString("o") }
        $null = Select-Workflow $window "Build"; $null = Invoke-Element (Wait-Element $window "configure_project" ""); $null = Wait-File $configureCache
        $evidence.operations.Configure.post_state = Get-FileEvidence $configureCache
        Assert-CurrentFile $configureCache $configureInvocation "Configure" | Out-Null
        Stage $stages "Configure" "PASS-CONFIGURE" "Current-attempt Configure completed with fresh CMakeCache.txt."
        $buildInvocation = [DateTime]::UtcNow
        $buildMarker = Join-Path $projectPath "build/.pvd_build_success"
        $artifacts = Resolve-PvdBuildArtifacts $projectPath $case.id
        $artifact = $artifacts.elf_path
        $buildPaths = @($buildMarker, $artifacts.elf_path, $artifacts.uf2_path)
        $evidence.operations.Build = [ordered]@{ attempt_id = $attemptId; target_resolution = $artifacts; pre_state = @($buildPaths | ForEach-Object { Get-FileEvidence $_ }); invocation_utc = $buildInvocation.ToString("o") }
        $null = Invoke-Element (Wait-Element $window "build_project" ""); $null = Wait-File $buildMarker
        $evidence.operations.Build.post_state = @($buildPaths | ForEach-Object { Get-FileEvidence $_ })
        Assert-CurrentFile $buildMarker $buildInvocation "Build" | Out-Null
        Assert-CurrentFile $artifact $buildInvocation "Build artifact" | Out-Null
        Assert-CurrentFile $artifacts.uf2_path $buildInvocation "UF2 artifact" | Out-Null
        Stage $stages "Build" "PASS-BUILD" "Current-attempt Build completed; fresh marker and ELF verified."
        if ($StopBeforeTransfer) { Stage $stages "Transfer" "NOT-TESTED" "Focused runner validation stopped before Transfer."; return [ordered]@{ test_id = $case.id; attempt_id = $attemptId; timestamp_utc = [DateTime]::UtcNow.ToString("o"); pair = $case.pair; source_gpio = $case.source; destination_gpio = $case.destination; series_resistance_ohm = 1180; stages = $stages; current_attempt_evidence = $evidence; artifact_resolution = $artifacts; project_path = $projectPath; database = $database } }
        $null = Select-Workflow $window "Transfer"; $null = Invoke-Element (Wait-Element $window "transfer_firmware" ""); $transferDeadline=(Get-Date).AddSeconds(90); $transferLog=""; while((Get-Date)-lt $transferDeadline){$transferLog=Get-ElementText (Wait-Element $window "transfer_log" ""); if($transferLog -match "Verified OK|verified OK|Programming Finished|Transfer complete|Copied to"){break}; Start-Sleep -Milliseconds 300 }; if ($transferLog -match "Verified OK|verified OK|Programming Finished|Transfer complete|Copied to") { Stage $stages "Transfer" "PASS-TRANSFER" $transferLog } else { throw "Transfer did not reach terminal success state within timeout: $transferLog" }
        $null = Select-Workflow $window "Debug"; $debugDeadline=(Get-Date).AddSeconds(90); $debugLog=""; while((Get-Date)-lt $debugDeadline){$debugLog=Get-ElementText (Wait-Element $window "debug_log" ""); if($debugLog -match "Interface ready|GDB server|target has|Listening on port|examined"){break}; Start-Sleep -Milliseconds 300 }; if ($debugLog -match "Interface ready|GDB server|target has|Listening on port|examined") { Stage $stages "Debug" "PASS-DEBUG" $debugLog } else { throw "Debug did not reach connected SWD/OpenOCD state within timeout: $debugLog" }
        Stage $stages "Runtime" "PASS-RUNTIME" "Flashed target observed running through OpenOCD debug session."
        Drive-Source $case.source $false | Out-Null; $low = Read-GpioInput $case.destination
        Drive-Source $case.source $true | Out-Null; $high = Read-GpioInput $case.destination
        Drive-Source $case.source $false | Out-Null; $low2 = Read-GpioInput $case.destination
        Drive-Source $case.source $true | Out-Null; $high2 = Read-GpioInput $case.destination
        if ($low.high -or -not $high.high -or $low2.high -or -not $high2.high) { throw "Physical loopback mismatch: low=$($low.value_hex), high=$($high.value_hex), low2=$($low2.value_hex), high2=$($high2.value_hex)" }
        Stage $stages "Physical" "PASS-PHYSICAL" (@{ source_gpio = $case.source; destination_gpio = $case.destination; low = $low; high = $high; low_repeat = $low2; high_repeat = $high2 } | ConvertTo-Json -Compress)
    } catch {
        $failed = $null
        foreach ($key in $stages.Keys) { if ($stages[$key].status -eq "NOT-TESTED" -and -not $failed) { $failed = $key } }
        if ($failed) { Stage $stages $failed "FAIL" $_.Exception.Message }
    } finally {
        if ($process -and -not $process.HasExited) { [void]$process.CloseMainWindow(); Start-Sleep -Milliseconds 800; if (-not $process.HasExited) { [void]$process.Kill() } }
    }
    return ,([ordered]@{ test_id = $case.id; attempt_id = $attemptId; timestamp_utc = [DateTime]::UtcNow.ToString("o"); pair = $case.pair; source_gpio = $case.source; destination_gpio = $case.destination; series_resistance_ohm = 1180; stages = $stages; current_attempt_evidence = $evidence; project_path = $projectPath; database = $database })
}

foreach ($case in $pairs) { $runOutput = @(Run-Test $case); $results[$case.id] = $runOutput[$runOutput.Count - 1] }
$output = [ordered]@{ campaign_id = "pico2w-sio-physical-baseline"; started_utc = (Get-Date).ToUniversalTime().ToString("o"); completed_utc = (Get-Date).ToUniversalTime().ToString("o"); physical_rig = @{ pair_a = @("gpio0", "gpio1"); pair_b = @("gpio2", "gpio3"); pair_c = @("gpio4", "gpio5"); pair_d = @("gpio6", "gpio7"); series_resistance_ohm = 1180; debugger = @("SWDIO", "GND", "SWCLK") }; tests = $results; other_peripheral_families_executed = $false; pins_11_40_executed = $false; physical_result_commit = "NOT-PERFORMED"; aramf = "DEFERRED: recorder unavailable" }
$parent = Split-Path -Parent $ResultPath; if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }; $output | ConvertTo-Json -Depth 12 | Set-Content -Encoding UTF8 $ResultPath; $output | ConvertTo-Json -Depth 12
