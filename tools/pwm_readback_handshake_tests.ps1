$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$cpp=Get-Content (Join-Path $root 'src/systems/mainwindow/debug/debug_column3/DebugColumn3.cpp') -Raw
$hpp=Get-Content (Join-Path $root 'src/systems/mainwindow/debug/debug_column3/DebugColumn3.hpp') -Raw
$ps=Get-Content (Join-Path $root 'tools/gui_sio_physical_runner.ps1') -Raw
function Require([bool]$ok,[string]$message){if(-not $ok){throw $message}}
Require ($cpp -match 'connect\(readbackStart, &QPushButton::clicked, this, &DebugColumn3::startReadbackSession\)') 'Qt readback signal/slot connection missing'
Require ($cpp -match 'READBACK_TRIGGER_RECEIVED') 'C++ trigger acknowledgement marker missing'
Require ($cpp -match 'READBACK_REQUEST_ACCEPTED') 'C++ request acceptance marker missing'
Require ($cpp -match 'CXX_TRIGGER_RECEIVED') 'Durable C++ trigger ack event missing'
Require ($cpp -match 'active-readback-request\.json') 'Explicit request path missing'
Require ($cpp -match 'READBACK_REQUEST_INVALID') 'Invalid request classification missing'
Require ($cpp -match 'QJsonDocument::fromJson') 'C++ request parser missing'
Require ($cpp -match 'QDir\(\)\.mkpath') 'C++ output directory creation missing'
Require ($cpp -match 'readbackRequestAccepted_') 'Request acceptance state missing'
Require ($ps -match 'active-readback-request\.json') 'Runner does not publish request context'
Require ($ps -match 'ackPath') 'Runner does not publish trigger ack path'
Require ($ps -match 'CXX_TRIGGER_NOT_ACKNOWLEDGED') 'Trigger acknowledgement timeout missing'
Require ($ps -match 'readback_run_id -ne \$runId') 'Run ID validation missing'
Require ($ps -match 'PowerShellEvidenceCommands=0') 'PowerShell evidence ownership is not zero'
'PASS: Qt trigger acknowledgement and request acceptance contract'
'PASS: exact request/result/log identity is validated'
'PASS: pre-session status and output ownership are explicit'
