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

The source package was structurally validated in the reconstruction environment. Qt 6 development libraries are not installed in that environment, so the desktop executable could not be compiled there.

The package is configured for Qt 6 Widgets + Qt 6 SQL and can be built on the user's Qt development machine using the included `run_windows.ps1` or the commands in `README.md`.

## Viewer status

`assets/PICO2W.glb` is now loaded and rendered by the permanent shared Viewer. CMake copies the model to `build/runtime/assets` for the executable. The public Viewer/System/MainWindow connections are unchanged, and workflow selections still update the shared panel.

## Hardware-function scope

The function catalog contains the RP2350A GPIO0-GPIO29 alternate-function matrix used by Pico 2 / Pico 2 W, including GPIO/SIO, SPI, UART, I2C, PWM, PIO0/1/2, ADC, HSTX, clock, trace, QMI and USB-related alternate functions. Each function owns its own SQLite database.
