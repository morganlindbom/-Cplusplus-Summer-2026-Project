<!-- PROJECT_STATUS.md -->
# Project Status

## Reconstruction status

This package is the first complete runnable reconstruction baseline of Pico Visual Designer under the new object-oriented System/MainWindow architecture.

Implemented workflow objects:

- Project
- Function Selection
- Settings
- C++ / C Code
- ASM PIO Code
- Generate
- Build
- Transfer
- Debug

Shared MainWindow objects:

- Workflow
- Viewer

Every workflow owns an independent Column 2 object and Column 3 object.

## Database status

- 1 governing `system.sqlite`
- 20 MainWindow/component SQLite databases
- 64 independent RP2350 pin-function SQLite databases
- Per-project runtime state is stored in `project.sqlite`

Validation result in the reconstruction environment:

```text
UI/system databases checked: 21
Function databases checked: 64
PASS
```

## Build verification status

The desktop application has been built and exercised in the Windows development
environment.

Current results:

```text
PVD desktop build: PASS
CTest: 2/2 PASS
  database_integrity: PASS
  multicore_generator: PASS
```

The project remains configured for Qt 6 Widgets + Qt 6 SQL and can be built
using the included `run_windows.ps1` or the commands in `README.md`.

## Current multicore certification status

The RP2350A Core 1 setting is the master switch. Execution-core selection is
stored per function and generated runtime behavior uses shared Core 0/Core 1
dispatchers. Hardware initialization remains on Core 0 where required, before
Core 1 is launched.

### Verified PASS

- Function Selection UI selection behavior.
- GPIO0 persistence.
- GPIO1 persistence.
- Save, navigation, and project-state persistence.
- UI generation and generated-file refresh.
- CMake configure through PVD.
- Build through PVD.
- Transfer through PVD using the current project ELF.
- Transfer log verification from the authoritative `QPlainTextEdit`, including
  programming completion, verification success, and target reset.
- Build-artifact freshness checks using the successful-build marker and source
  timestamps.
- Core 0 GPIO runtime debugger proof.
- Core 1 onboard LED runtime debugger proof.
- Buzzer runtime on Core 0.
- Buzzer runtime on Core 1.
- Clock-aware buzzer PWM generation using `clock_get_hz(clk_sys)`.
- Non-blocking buzzer scheduling in the cooperative dispatcher.
- Reverse assignment: onboard LED on Core 0 and buzzer on Core 1.
- Multiple different function types on Core 1: onboard LED, GPIO0, and buzzer.
- Physical simultaneous operation of the multi-function Core 1 firmware.
- Core 1 debugger proof for the LED, GPIO0, and buzzer handlers.
- ROBO-PICO board-startup NeoPixel sanitation when the feature is disabled.
- Disabled NeoPixel runtime-handler absence.
- Disabled NeoPixel absence from Core 0 and Core 1 dispatchers.
- Temporary NeoPixel sanitation PIO resource release.
- Debug Stop restoring target execution with `reset run` before OpenOCD shutdown.
- Durable lessons recorded in `AGENT.md`.

### Implemented but not fully certified

- Resource validation covers the implemented GPIO, PWM, PIO, serial, ADC,
  CYW43, board-fixed, and shared-state model, but the complete deliberate
  conflict matrix has not been rerun through the UI after the final changes.
- PIO sanitation and normal PIO allocation are implemented and tested in the
  relevant workflow, but the full cross-peripheral PIO conflict matrix remains
  incomplete.
- Transfer stale-artifact protection is implemented; additional negative UI
  scenarios such as a failed build followed by a transfer attempt remain to be
  exercised as a dedicated test case.

### NOT TESTED or remaining work

- Full UI Test F matrix for every representative GPIO, PWM, PIO, UART, SPI,
  I2C, ADC, and board-specific conflict combination.
- Complete physical regression coverage for ADC, UART, SPI, and I2C functions.
- Reverse-assignment physical/debugger coverage for every supported function
  type beyond the verified LED, GPIO, and buzzer combinations.
- Full production certification report for every supported peripheral.

## Latest verified change

```text
Commit: b9ae569 Complete multicore runtime certification fixes
Branch: main
```

The working tree was clean after the commit and the changes were pushed to
`origin/main`.

## Viewer status

`assets/PICO2W.glb` is now loaded and rendered by the permanent shared Viewer. CMake copies the model to `build/runtime/assets` for the executable. The public Viewer/System/MainWindow connections are unchanged, and workflow selections still update the shared panel.

## Hardware-function scope

The function catalog contains the RP2350A GPIO0-GPIO29 alternate-function matrix used by Pico 2 / Pico 2 W, including GPIO/SIO, SPI, UART, I2C, PWM, PIO0/1/2, ADC, HSTX, clock, trace, QMI and USB-related alternate functions. Each function owns its own SQLite database.
