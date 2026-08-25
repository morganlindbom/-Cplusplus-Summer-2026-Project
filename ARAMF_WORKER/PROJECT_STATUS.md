<!-- PROJECT_STATUS.md -->

# Project Status

## Project

- Name: PVD
- Project ID: 29089d6e-6a1c-435d-a0bc-b24d90becb6a
- ARAMF state: Initialized

## Implemented

- ARAMF control-plane structure generated.

## Verified

- Generation completed for the selected output products.

## PVD Debugging Certification

Current state: VERIFIED / CERTIFIED

### OpenOCD lifecycle

- Debug workflow entry does not start OpenOCD.
- Explicit Start OCD starts the PVD-owned OpenOCD process.
- Explicit Stop OCD performs orderly `reset run` and `shutdown` cleanup.
- CMSIS-DAP disconnect completes and the orange programming LED turns off.
- OpenOCD process ownership: PASS.
- No orphan OpenOCD process or console remains: PASS.

### Authoritative Debug build

- Development projects use Debug configuration.
- `CMAKE_BUILD_TYPE=Debug`.
- `PICO_DEOPTIMIZED_DEBUG=1` for the certified Debug project.
- Debug consumes the ELF produced by the authoritative current Build workflow.
- The stale Release ELF selection defect was identified and resolved.

### GDB and RP2350 multicore

- Physical GDB connection: PASS.
- GDB process ownership: PASS.
- Core 0 physical debugging: PASS.
- Core 1 physical debugging: PASS.
- Halt, Continue, Step, Next, Registers, Backtrace, and Breakpoint: PASS.
- Second debug session: PASS.
- `rp2350.cm0` detection and application execution: PASS.
- `rp2350.cm1` detection and application execution: PASS.
- Core 0 and Core 1 application contexts are independently identifiable.

### Debug Core selector

- Dedicated semantic Core 0/Core 1 selector: PASS.
- GDB thread IDs are resolved dynamically from active `info threads` output.
- Thread IDs are session-specific and are not treated as permanent core identity.
- Single-core projects expose only Core 0.
- Multicore projects expose Core 0 and Core 1.
- Mapping is rediscovered for each GDB session.
- Selector state and mapping reset when the session stops.

## Durable Debug Rules

1. Entering the Debug workflow must never start OpenOCD; only explicit Start OCD may start it.
2. Stop OCD must allow normal OpenOCD/CMSIS-DAP shutdown to complete before fallback termination.
3. Debug must consume the ELF produced by the authoritative current Build workflow.
4. Core 0 and Core 1 are semantic physical-core identities; GDB thread IDs are session details.
5. Core 1 application debugging is valid only when the generated application launches Core 1.
6. The certified Debug implementation is a frozen baseline and changes require a reproducible defect or explicit feature request.

## Latest Settings Lifecycle Verification

- Settings rebuild read-only: PASS.
- Stale editor protection: PASS.
- Stale queued callback rejection: PASS.
- 20x rapid navigation core test: PASS.
- Pin 1 core retention: PASS.
- Pin 2 core retention: PASS.
- CTest: 7/7 PASS.
- Configure: PASS.
- Build: PASS.
- Formatting: PASS.
- clang-tidy: PASS.
- Database validation: PASS.
- SQLite integrity: PASS.
- Previously certified Debug/OpenOCD/GDB/Core-selector baselines: PRESERVED.

## Current GUI Certification Boundary

- GUI multi-pin isolation: FAIL / NOT CERTIFIED.
- Intended state: Pin 1 / GPIO0 / SIO = Output; Pin 2 / GPIO1 / SIO = Input.
- Observed state: Pin 1 = Input; Pin 2 = Input.
- Project save after final lifecycle change: NOT CERTIFIED.
- Reopen: NOT CERTIFIED.
- Dirty state: NOT CERTIFIED.
- Unsaved save path: NOT CERTIFIED.
- SIO physical runner ready: NO.
- Pin 1-10 physical certification: BLOCKED.
- No Generate-for-hardware, Transfer, Debug hardware, or GPIO drive was performed during this blocked stage.

## Current Blocker

- Locate and certify the remaining real-GUI SIO live-state divergence causing the Pin 1 / Pin 2 multi-pin observation mismatch.

## Next Project Task

- First locate and certify the remaining real-GUI SIO live-state divergence.
- Only after that passes, certify project save/reopen, dirty state, the unsaved-dialog path, and Pin 1-10 current-wiring physical behavior.
- The broader hardware-resource ownership and conflict certification campaign for GPIO, PWM, PIO, UART, SPI, I2C, and multicore resources remains future work.

## ARAMF Lifecycle

Current state: Finalized
