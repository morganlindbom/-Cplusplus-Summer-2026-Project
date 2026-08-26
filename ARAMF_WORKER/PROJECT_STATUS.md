<!-- PROJECT_STATUS.md -->

# Project Status

## Project

## ARAMF Integrity Recovery

- Current durable sequence: 204; event-log contains 204 append-only records.
- Raw historical event-ID uniqueness remains visibly `FAIL` for exactly three known legacy duplicate IDs.
- Narrow reconciliation is `PASS WITH ACKNOWLEDGED LEGACY EXCEPTIONS`; exact immutable occurrences are fingerprinted in `memory/event-id-integrity-exceptions.json`.
- All non-exempt/new IDs are unique, and `tools/aramf_event_integrity.ps1` rejects collisions before append.
- Historical records, IDs, sequences, timestamps, and failures were not rewritten. Physical SIO certification is complete for Initial State Low/High and Blink 100/500/1000 ms.

- Name: PVD
- Project ID: 29089d6e-6a1c-435d-a0bc-b24d90becb6a
- ARAMF state: Initialized

## Configure / Build Status Contract

- Visible status control: PASS (`QLabel`, objectName `build_status`).
- Initial state: `Idle`.
- Canonical runtime states: ConfigureRunning, ConfigureCompleted, ConfigureFailed,
  BuildRunning, BuildCompleted, BuildFailed.
- The label is user-visible and is the primary GUI synchronization point; `build_log`
  remains diagnostic only.
- Fresh Configure -> Build validation: PASS. Transfer was not run for this task.

## SIO Physical Certification

- Initial State Low: PHYSICAL PASS; Initial State High: PHYSICAL PASS.
- Blink 100 ms: PHYSICAL PASS; Blink 500 ms: PHYSICAL PASS; Blink 1000 ms: PHYSICAL PASS.
- Blink physical scope is limited to 100/500/1000 ms; full 10-60000 ms coverage is not claimed.
- Existing Blink 500 and Blink 1000 physical result files were valid and adopted append-only; no redundant rerun was performed.
- Final active artifact: `C:\Users\morga\AppData\Local\Temp\pvd-sio-20260826T042152311Z\SIO-PAIR-D-FWD` (Blink 1000 runtime).

## Implemented

- ARAMF control-plane structure generated.

## Verified

- Generation completed for the selected output products.

## PVD Debugging Certification

Current state: PARTIAL

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

## SIO Software/Generator Closeout (current task)

- SIO catalog exposes Direction, Pull (`None`, `Pull-up`, `Pull-down`), Initial State (`Low`, `High`), Blink Enable, Blink Interval, Drive Strength (`2`, `4`, `8`, `12` mA), Slew (`Slow`, `Fast`), and Input-only `debounce_ms`.
- Production build and extended `sio_model` generator tests: PASS. Verified mappings include `gpio_set_pulls`, `gpio_set_drive_strength`, `gpio_set_slew_rate`, initial latch-before-output-enable, Blink interval runtime, and no SIO runtime consumer for debounce.
- Runner accepts parameterized source drive strength/slew and destination Pull using the existing live-control path.
- GUI matrix: COMPLETE through the real Qt SettingsColumn3 widgets and signal/model path. Pull, Drive Strength, and Slew transitions, fresh persistence, and multi-pin isolation pass. Windows UIA native Qt popup enumeration remains unreliable and is not product evidence.
- Stable combo identities: `setting_pull`, `setting_drive_strength`, and `setting_slew_rate`.
- Qt integration target: `sio_model`; it exercises real QComboBox controls, `currentTextChanged`, `SettingsColumn3::settingChanged`, ApplicationState propagation, ProjectStore reload, and owner isolation. Qt6::Test is not installed, so no fake replacement widgets or UIA-only assertions were added.
- Manual Pull-up oracle: PASS (visible user path, Save, close, fresh reopen). Product signal path remains `QComboBox::currentTextChanged` -> `SettingsColumn3::settingChanged` -> `System.cpp` -> `state_.selections`.
- Pull physical: DEFERRED; drive-strength physical: DEFERRED pending instrumentation; slew physical: DEFERRED pending oscilloscope; debounce is UI/model-only for SIO. IRQ/events, input hysteresis, and input enable are not exposed.
- SIO software/GUI closeout: COMPLETE WITH DOCUMENTED PHYSICAL DEFERRALS. Pull physical validation is deferred pending board-bias/topology proof; Drive Strength physical validation requires current instrumentation; Slew physical validation requires oscilloscope edge-rate measurement. PWM is the next recommendation only and was not started.

- Pin 1 / GPIO0 function selection Disabled -> SIO: PASS.
- Verified live owner: physical Pin 1, GPIO0, selection `pin_1`.
- Verified function state: UI `SIO`; authoritative model function `sio`.
- Verified SIO retention: PASS through T+0 ms, T+100 ms, T+500 ms, T+1000 ms, and T+2000 ms.
- Real-GUI SIO Direction Input -> Output: PASS.
- Verified Pin 1 / GPIO0 / SIO direction: UI `Output`; authoritative model `Output`.
- Verified Direction retention: PASS through T+0 ms, T+100 ms, T+500 ms, T+1000 ms, and T+2000 ms.
- GUI multi-pin isolation: PASS after correcting the diagnostic’s Settings-row navigation to visible mouse selection and verifying authoritative model checkpoints.
- Prior failure was an automation wrong-owner write: the attempted Pin 2 Input write was accepted as `pin_1` Output -> Input. No production model corruption was proven.
- Intended state: Pin 1 / GPIO0 / SIO = Output; Pin 2 / GPIO1 / SIO = Input.
- Previous blocked observation remains historical: Pin 1 = Input; Pin 2 = Input.
- Project save after final lifecycle change: PASS. Real GUI Save persisted separate pin_1/pin_2 SIO selections and directions.
- Reopen: PASS. Same project reopened in a fresh PVD process.
- Dirty state after Save: PASS. Status changed from unsaved to Project saved.; no unsaved dialog appeared after explicit Save.
- Unsaved-changes dialog: PASS. Current PVD-owned `Close Project` dialog exposed Save, Discard, and Cancel.
- Cancel: PASS. Application remained open and dirty Input runtime state remained active.
- Save from dialog: PASS. Input persisted after fresh reopen.
- Discard: PASS. Unsaved Input was absent after fresh reopen; persisted Output remained.
- SIO physical runner: existing `tools/gui_sio_physical_runner.ps1` inspected; no new runner required.
- Pin 1 current-wiring physical certification: ROBO-PICO ONBOARD GPIO0 STATUS LED IDENTIFIED. Cytron ROBO-PICO manufacturer documentation establishes GPIO0/Grove Port 1 onboard status indication and intended digital-output operation; exact onboard resistor value is not fabricated.
- Current physical wiring (Admin-corrected): GPIO0↔GPIO1 (Physical Pin 1↔2), GPIO2↔GPIO3 (Physical Pin 4↔5), GPIO4↔GPIO5 (Physical Pin 6↔7), and GPIO6↔GPIO7 (Physical Pin 9↔10). These installed loopbacks must not be removed or rewired.
- The prior GPIO0↔GPIO7 / Physical Pin 1↔10 interpretation is historical only and is superseded; prior events remain unchanged.
- Current Pin 1 safety target: Physical Pin 1/GPIO0 SIO Output; Physical Pin 2/GPIO1 SIO Input/non-driving.
- GUI owner isolation: PASS. The physical runner now uses visible Settings-row mouse selection, live owner reacquisition, and mandatory physical-pin/GPIO/selection-ID gates.
- Authoritative persisted model: PASS — pin_1=sio/Output and pin_2=sio/Input after the owner-gated GUI-only retest.
- Generated GPIO0/GPIO1 static safety audit: PASS. Fresh PVD Generate produced GPIO1 SIO input/non-driving semantics and GPIO0 latch-low-before-output semantics with no GPIO1 output, mask, override, peripheral, or multicore drive path.
- Electrical safety gate: PASS for generated static safety. Pin 1/GPIO0 and Pin 2/GPIO1 physical certification: PASS. Pin 4/GPIO2 SIO Output and Pin 5/GPIO3 SIO Input physical loopback: PASS with firmware samples 0 -> 1 -> 0.
- Historical operational boundary: Generate-only runner fall-through reached Configure and failed Build; preserved in sequence 52.
- Current intentional artifact gate: Configure completed and Build completed through the real PVD GUI. Current `.pvd_build_success`, ELF, and UF2 are present and current in the stimulus-enabled project.
- Existing PVD SIO stimulus capability: PERIODIC TOGGLE AVAILABLE — Pin 1 GPIO0 `blink_enabled=true`, initial Low, interval 500 ms generates repeated LOW/HIGH transitions.
- Bounded physical-certification stimulus: ADMIN-AUTHORIZED DIRECT C++ TEST STIMULUS PREPARED. The generated certification artifact contains LOW 500 ms -> HIGH 1000 ms -> LOW and leaves GPIO0 LOW; this is certification-only code, not a new PVD SIO feature.
- Post-build source identity and GPIO0/GPIO1 static safety: PASS. No Transfer or GPIO drive occurred.
- Generate/Configure/Build/Transfer for GPIO0/GPIO1, GPIO2/GPIO3, GPIO4/GPIO5, and GPIO6/GPIO7: PASS. GPIO6/GPIO7 firmware readback was 0 -> 1 -> 0 with pass flag true; the current D artifact is SIO_PAIR_D_FWD and the GPIO7 path remained non-driving.
- The prior GPIO7 experiment reached GUI/Generate/Configure/Build in a temporary project but is not valid current-wiring certification; no Transfer or GPIO drive was performed.

## Current Blocker

- Current reverse baseline: GPIO1 Output -> GPIO0 Input physical loopback PASS with firmware readback 0 -> 1 -> 0. Pair A bidirectional SIO is PASS. The forward GPIO0 Output / GPIO1 Input certificates remain unchanged.

- Current blocker: NONE for the four installed forward-direction SIO loopback pairs. GPIO6/GPIO7 physical loopback is PASS; Pins 3 and 8 are GND/not applicable. Reverse SIO directions and other peripheral functions remain untested.

- Pin 1/GPIO0 SIO Output: PASS — Admin-observed LED OFF -> ON -> OFF. Pin 2/GPIO1 SIO Input: PASS — firmware readback 0 -> 1 -> 0. Pin 4/GPIO2 SIO Output: PASS. Pin 5/GPIO3 SIO Input: PASS. Pin 6/GPIO4 SIO Output: PASS. Pin 7/GPIO5 SIO Input: PASS.

## Next Project Task

- GPIO4 <-> GPIO5 physical loopback: PASS (firmware GPIO5 readback 0 -> 1 -> 0; current artifact SIO_PAIR_C_FWD; Transfer PASS).
- GPIO6 <-> GPIO7 physical loopback: PASS (firmware GPIO7 readback 0 -> 1 -> 0; current artifact SIO_PAIR_D_FWD; Transfer PASS).
- GPIO1 -> GPIO0 reverse physical loopback: PASS (firmware GPIO0 readback 0 -> 1 -> 0; current artifact SIO_PAIR_A_REV; Transfer PASS).
- Pair A bidirectional SIO: PASS.
- Next task: GPIO3 Output -> GPIO2 Input reverse certification. Do not start it automatically.
- The broader hardware-resource ownership and conflict certification campaign for GPIO, PWM, PIO, UART, SPI, I2C, and multicore resources remains future work.

## ARAMF Lifecycle

Current state: Finalized

## Test Certification

- Current durable evidence: `certification/certificates.jsonl`
- Current resolved state: `certification/current-certification-state.json`
- No certificate is issued without applicable evidence.

## Latest Agent Task

- Task: GPIO1/GPIO0 reverse SIO physical certification
- Status: COMPLETED for the first reverse baseline pair; Pair A now has bidirectional SIO physical evidence.
- Result: GPIO1 Output -> GPIO0 Input was Configure/Build/Transfer verified with firmware GPIO0 readback 0 -> 1 -> 0. New reverse-direction certificates exist for GPIO1 Output and GPIO0 Input; all forward certificates remain unchanged. Next reverse pair is GPIO3 Output -> GPIO2 Input.
- Root cause: diagnostic automation selected the wrong live Settings owner; the attempted Pin 2 direction write targeted `pin_1`.
- Corrected visible-row navigation and authoritative model tracing proved Pin 1 Output / Pin 2 Input isolation through repeated navigation.
- Normal PVD executable rebuilt successfully; no production behavior correction or certification certificate was created.

## Remaining Reverse SIO Batch Status

- B-REV GPIO3 Output -> GPIO2 Input: Configure/Build/Transfer completed; firmware GPIO2 samples were 0 -> 1 -> 0, but the first readback automation requested `gpio2_loopback_pass` instead of the actual reverse symbol `gpio2_reverse_loopback_pass`. Certification remains pending corrected pass-symbol observation.
- C-REV GPIO5 Output -> GPIO4 Input: Configure/Build/Transfer PASS; read-only GDB firmware samples `0 -> 1 -> 0`, `gpio4_reverse_loopback_pass=1`. Pair evidence is PASS, but certificates were not issued before the batch stop.
- D-REV GPIO7 Output -> GPIO6 Input: fresh generated source prepared, but Configure/Build did not reach a verifiable terminal result. No Transfer or GPIO drive occurred.
- Current blocker: B-REV corrected firmware-pass-symbol observation and D-REV Configure/Build verification are unresolved. The unattended reverse batch is stopped; no further pair or capability family was started.
- Current wiring remains unchanged: GPIO0 <-> GPIO1, GPIO2 <-> GPIO3, GPIO4 <-> GPIO5, GPIO6 <-> GPIO7.
- Next task: resume B-REV readback with `gpio2_reverse_loopback_pass`, then complete/validate B-REV before deciding whether D-REV may proceed.

## Final Reverse SIO Reconciliation

- B-REV GPIO3 Output -> GPIO2 Input: CANONICAL PHYSICAL PASS; certificates `cert-b1c3d5f7-9a02-4e6c-8b0d-2f4a6c819e7d` and `cert-c2d4f6a8-0b13-5f7d-9c1e-3a5b7d920f8e`.
- C-REV GPIO5 Output -> GPIO4 Input: CANONICAL PHYSICAL PASS; certificates `cert-d3e5f7a9-1b24-6c8e-0d2f-4a6c8e103f9b` and `cert-e4f6a8b0-2c35-7d9f-1e3a-5b7d9f214a0c`.
- D-REV GPIO7 Output -> GPIO6 Input: CANONICAL PHYSICAL PASS; certificates `cert-f5a7c9b1-3d46-8e0a-2f4b-6c8e0a325b1d` and `cert-a6b8d0c2-4e57-9f1b-3a5c-7e9f1b436c2e`.
- All three results are ADMIN-ATTESTED EXISTING PHYSICAL RUN EVIDENCE adopted during reconciliation; no new hardware run occurred.
- PINS 1–10 CURRENT WIRING SIO BIDIRECTIONAL PHYSICAL CERTIFICATION: PASS. GPIO0–GPIO7 each have SIO Input and Output physical PASS. Pins 3 and 8 are GND / NOT APPLICABLE.
- Current wiring unchanged. Scope excludes PWM, PIO, UART, SPI, I2C, ADC, and other alternate functions.
- Next capability boundary: inventory remaining supported SIO settings and alternate-function/IRQ capabilities from current PVD definitions; no next capability was started.

## Inventory Result

- Implemented SIO settings: `direction` (Input|Output), `pull` (None|Pull-up|Pull-down), `debounce_ms`, `initial_state` (Low|High), `blink_enabled`, `blink_interval_ms`, `drive_strength` (2|4|8|12 mA), and `slew_rate` (Slow|Fast).
- Generator mappings: `gpio_set_pulls`, deterministic initial `gpio_put`/`gpio_set_dir`, generated blink runtime, `gpio_set_drive_strength`, and `gpio_set_slew_rate`.
- GPIO IRQ/event configuration is not supported by the current PVD catalog/generator.
- Class-A candidates requiring a dedicated objective harness: Initial State Low/High, Pull-up, Pull-down, and generated Blink transitions/interval. No physical certification was claimed because the existing runner lacks contract-compliant timing/readback for these settings.
- Class-B: GUI/model/persistence/generator/build verification for those SIO settings.
- Class-C: physical drive-strength current, slew edge-rate, input hysteresis, and debounce timing; instrumentation or a dedicated measurement harness is required.
- Alternate-function families present for future inventory: PWM, UART, SPI, I2C, and PIO. No alternate-function test was executed.
- Current blocker: remaining SIO-setting physical tests need an objective PVD-generated observation/timing harness. No safety incident occurred.

## Generic GPIO Certification Observer

- Implemented in `tools/gui_sio_physical_runner.ps1` with parameterized `ObservedGpio`, `ObserverMode`, `ExpectedLevel`, `MaxSamples`, `MaxTransitions`, `ObservationDurationMs`, `ExpectedIntervalMs`, `ToleranceMs`, and `SymbolPrefix`.
- Modes `LEVEL`, `TRANSITIONS`, and `TIMING` are implemented using fixed-size volatile storage, `gpio_get()`, and target `time_us_64()` timestamps.
- Observer is read-only relative to the feature under test, bounded, non-blocking, and polled after PVD runtime handlers in the main loop.
- Focused self-tests PASS for GPIO1/GPIO7 level observation, GPIO3 transitions, GPIO5 timing, custom symbols, bounds, read-only rules, and invalid mode rejection.
- Representative observer-injected generated project compiled; real PVD Configure and Build passed and the PVD success marker was present. No Transfer or GPIO execution occurred.
- Current blocker resolved: GENERIC GPIO OBSERVER IMPLEMENTED AND BUILD-VALIDATED.
- Next task: use the generic observer for Initial State Low/High and PVD-generated Blink physical tests. Pull remains physically deferred pending board-bias isolation.

## Initial State / Blink Physical Batch

- Infrastructure status: PASS for observer generation, marker-bounded read-only validation, visible build status synchronization, SIO GUI orchestration, Generate/Inject/Build, generic Transfer/target/readback/parser, and the LEVEL infrastructure smoke.
- Product capability status: Initial State Low/High and Blink 100/500/1000 physical certification are PHYSICAL PASS.
- Blink 500/1000 were valid existing physical results and were adopted append-only during resume; no duplicate physical run was performed.
- Physical scope is limited to Blink 100/500/1000 ms; the full 10-60000 ms domain is not claimed.
- Next task: select the next authorized SIO/PWM certification family after reviewing remaining boundaries.
