<!-- PROJECT_STATUS.md -->

## TIMER0 Condition B Readback After Host-Side Project Load (sequence 269)

- The new `--open-project` startup path loaded the exact diagnostic project into live PVD without project-open GUI interaction or target-side setup. No Transfer, program, reset, or memory write occurred.
- One production no-reset readback executed 91 commands and produced artifacts, but MI association failed (`gdb_association_valid=false`, 95 parse failures). Full provenance and Condition-B snapshot values remain unaccepted; no retry was performed. Cleanup passed and port 3333 is free. PWM remains NOT CERTIFIED.

## TIMER0 Condition B Host-Side Load Attempt (sequence 268)

- Condition B readback remained blocked. `ProjectStore::load` is only a primitive that loads into a caller-owned `ApplicationState`; the live `System` state has no public host-side load API, and `main.cpp` has no project/database startup option. A separate loader would not configure the running PVD. The existing route requires `Open-ExistingProject`, explicitly prohibited.
- No PVD/OpenOCD/GDB launch or target access occurred. Condition-B values and 91-command provenance remain unavailable; root cause remains NOT PROVEN. PWM remains NOT CERTIFIED.

## TIMER0 Snapshot Condition B (sequence 267)

- Admin true physical power-cycle confirmation is preserved, but Condition-B readback was blocked before launch because the current PVD has no project/database command-line mechanism and production Debug configuration is populated from loaded project state. The only existing route to that state is `Open-ExistingProject`, explicitly forbidden by the task.
- No target access, Transfer, program, reset, rebuild, or PWM activity occurred. Host preflight was clean; the verified current PVD and snapshot artifact identities remain present. Condition-B snapshot values and 91-command provenance are unavailable; root cause remains NOT PROVEN. PWM remains NOT CERTIFIED.

## Sequence-258 Evidence Reclassification (sequence 260)

- ARAMF permits reuse of the existing physical evidence: the exact persisted 76-command MI result stream contains every required timer-health field with exact token association, and sequence-259 replay reduced parse failures to zero.
- Sequence 258 is reclassified `NEW TRANSFER TIMER HEALTH=TIMING_SOURCE_NOT_PROGRESSING` from raw TIMER 0→0 and SDK time 0→0. Post-program start remains PASS at target-state level; Transfer startup defect remains NOT FIXED.
- No hardware or source activity occurred. Old Transfer remains failed, true cold boot remains `TIMER_HEALTH_PASS`, and PWM remains NOT CERTIFIED.

## GNU GDB/MI Parser and Sequence-258 Replay (sequence 259)

- Certification MI parsing now follows the GNU grammar structurally: tokenized result classes, all async categories, stream records with C-string handling, prompts, empty transport lines, split records, foreign tokens, and balanced tuple/list payloads.
- Exact persisted sequence-258 replay classified 76 result records and zero malformed records; recovered `Complete=1`, `Overflow=0`, and `TransitionCount=12`. The old raw log did not persist the 80 rejected out-of-band records individually, so narrower historical category counts are not provable. No hardware was accessed.
- C++ build, CTest 7/7, official MI fixtures, and exact replay pass. Sequence 258 remains historically unaccepted for timer health; PWM remains NOT CERTIFIED.

## Corrected MI Readback Hardware Result (sequence 258)

- The one corrected-MI direct diagnostic passed program, verify, explicit `reset run`, both-core running verification, shutdown, and 500 ms autonomous execution. All 76 MI evidence responses returned, including `Complete=1`, `Overflow=0`, and `TransitionCount=12`.
- The production result reported `gdb_association_valid=false` with 80 MI parse failures. Under the strict provenance gate, full observer provenance and NEW TRANSFER TIMER HEALTH are NOT EXECUTED; observed zero timer values are not accepted.
- Result: `validation/pin_certification/results/direct-transfer-mi-corrected-20260827T104504205Z.json`; raw log: `validation/pin_certification/results/direct-transfer-mi-corrected-20260827T104504205Z.log`. Cleanup passed and port 3333 is free. No further hardware attempt or PWM certification was performed.

## Tokenized GDB/MI Hardware Retry (sequence 257)

- The single MI retry passed the direct production Transfer contract: program, verify, explicit `reset run`, both `rp2350.cm0` and `rp2350.cm1` running, shutdown, and 500 ms autonomous execution.
- MI startup/synchronization reached the C++ evidence queue, but evidence command 1 failed because the first implementation transmitted backslash-escaped outer quotes; GDB rejected the expression. Result and raw log were created and cleanup passed. The quoting defect was corrected offline afterward; no second hardware retry was performed.
- Result: `validation/pin_certification/results/direct-transfer-mi-20260827T103641734Z.json`; raw log: `validation/pin_certification/results/direct-transfer-mi-20260827T103641734Z.log`. Full observer provenance and NEW TRANSFER TIMER HEALTH remain NOT EXECUTED. Phase A/cold-boot controls are unchanged and PWM remains NOT CERTIFIED.

## Direct New Transfer Hardware Diagnostic (sequence 256)

- Open-ExistingProject was bypassed. The exact hash-matched diagnostic artifact ran through production `TransferColumn3`: `program ... verify`, explicit `reset run`, `targets`, both `rp2350.cm0` and `rp2350.cm1` reported `running`, then `shutdown`. Post-program start and OpenOCD shutdown passed.
- After 500 ms autonomous execution, C++ readback completed 76 commands and cleanup with no reset/program/continue/memory writes. The first evidence response contained asynchronous GDB thread output; `Complete` was not independently captured and `TransitionCount=16`. Full observer provenance and GDB first-response association therefore FAIL, so no new Transfer timer-health result is accepted.
- Result: `validation/pin_certification/results/direct-transfer-20260827T102347176Z.json`; raw log: `validation/pin_certification/results/direct-transfer-20260827T102347176Z.log`. Processes exited and port 3333 is free. Phase A and cold-boot controls remain unchanged; PWM remains NOT CERTIFIED.

## New Post-Program Start Hardware Result (sequence 255)

- The authorized live attempt did not reach hardware: existing `Open-ExistingProject` GUI automation remained before Transfer. No OpenOCD/GDB process, Transfer, reset, programming, autonomous capture, or timer readback occurred.
- Cleanup passed and port 3333 is free. Offline implementation validation remains PASS, but the new post-program start contract and new Transfer timer health are NOT VERIFIED. Phase A remains valid, true cold-boot Phase B remains valid from sequence 252, and PWM remains NOT CERTIFIED.

## Post-Program Start and GDB Synchronization (sequence 254)

- Offline implementation validation passed: C++ build, CTest 7/7, PowerShell syntax, deterministic GDB prompt/sync-token barrier, split-response fixture, explicit Transfer lifecycle, and fail-closed RP2350 multicore state classifier.
- The old opaque `program ... verify reset exit` Transfer command is replaced by explicit program/verify, `reset run`, `targets` plus run-state queries for `rp2350.cm0` and `rp2350.cm1`, and `shutdown`. A halted/unknown core fails without blind resume.
- The single authorized live attempt stopped in existing project-open GUI automation before Transfer; no OpenOCD/GDB/target mutation occurred. New post-program start and Transfer timer health remain NOT VERIFIED. Phase A is valid, Phase B remains NOT VERIFIED, Transfer-vs-cold-boot remains proven from sequence 252, and PWM remains NOT CERTIFIED.

# Project Status

## True Cold-Boot Phase B (sequence 252)

- Same Phase-A artifact, no Transfer/programming after cold boot, and no debugger during autonomous execution. C++ readback completed all 76 commands with no reset/program/continue/memory writes; cleanup and port release passed.
- Timer health passed: raw TIMER0 513→9379 and SDK time 524→9384; DBGPAUSE remained 7. Transition SDK/raw deltas were approximately 728–731 us.
- Initial GDB remote/thread output contaminated the first response; therefore Complete=1 and typed/raw SRAM cross-check are not independently persisted by the current C++ result schema. Timer-health is valid diagnostic evidence, but full observer-status provenance is explicitly caveated.
- Phase A timer sources were frozen at zero for the same binary, proving the Transfer-vs-cold-boot difference at timer-health level. Phase B is not a PWM certification; PWM0A 100 Hz/50% remains `NOT CERTIFIED`.

## Result Authority Precedence (sequence 251)

- Exact current-run C++ result JSON now has terminal authority over UI labels; completed/failed precedence, stale-run rejection, result-arrival, and ordering tests pass.
- C++ build and CTest 7/7 pass; GUI/headless/reliability/no-reset regressions pass. Phase-A ELF/UF2 identity is verified and host preflight is clean.
- No cold boot, Transfer, programming, Phase B, or PWM execution has occurred. Phase B remains `NOT VERIFIED`; PWM0A 100 Hz/50% remains `NOT CERTIFIED`. One Admin-confirmed true physical power cycle is now the next authorized action.

## Live C++ Readback Handshake (sequence 250)

- Live run `readback-20260827T083203756Z` proved C++ slot acknowledgement and completed the full 76-command PVD-owned readback. OpenOCD/GDB/target halt/evidence completion and cleanup all passed; reset/program/continue/memory-write counts were zero.
- Result, raw log, and trigger ack are persisted under `validation/pin_certification/results/`. The PowerShell wrapper reported a false timeout because it continued polling the UI result-path label after C++ had published the known absolute result path. This is an orchestration observation defect, not a C++ readback failure.
- No power cycle, Transfer, programming, Phase B, or PWM execution occurred. Phase B remains `NOT VERIFIED`; Transfer versus cold boot remains `NOT PROVEN`; PWM0A 100 Hz/50% remains `NOT CERTIFIED`. Cold-boot retry readiness is `YES`.

## C++ Readback Request/Result Handshake (sequence 249)

- Offline handshake and trigger validation passed. The live control was found and invoked, but PVD exposed no `Trigger received`/`Request accepted` acknowledgement within 5000 ms (`CXX_TRIGGER_NOT_ACKNOWLEDGED`).
- Request run ID `readback-20260827T081512706Z` and raw log were created; the log contains only `READBACK_START`. No current C++ result JSON was published, so debugger/evidence stages are not proven. Cleanup completed and port 3333 was free.
- No power cycle, Transfer, programming, Phase B, or PWM execution occurred. Phase B remains `NOT VERIFIED`; Transfer versus cold boot remains `NOT PROVEN`; PWM0A 100 Hz/50% remains `NOT CERTIFIED`. Cold-boot retry readiness is `NO`.

## Fixed Trigger / Live C++ Smoke (sequence 248)

- The null/stale UIA trigger defect was fixed with safe `Current`/`AutomationId` access and bounded fresh reacquisition. Offline trigger/readback regressions and CTest 7/7 passed.
- Live `debug_readback_start` was found and invoked, but the production wrapper timed out waiting for the PVD C++ result artifact (`CXX_RESULT_TIMEOUT`). No current C++ result or raw log was produced; debugger/evidence stages are not proven. Cleanup completed; port 3333 was free.
- No power cycle, Transfer, programming, Phase B, or PWM execution occurred. Phase B remains `NOT VERIFIED`; Transfer versus cold boot remains `NOT PROVEN`; PWM0A 100 Hz/50% remains `NOT CERTIFIED`. Cold-boot retry readiness is `NO`.

## Live C++ Production Readback Smoke (sequence 247)

- The one authorized live smoke failed before C++ readback session establishment in `gui_sio_physical_runner.ps1:745`: `$item.Current.AutomationId` was dereferenced while `Current` was null during lookup of `debug_readback_start`.
- Result JSON: `validation/pin_certification/results/pvd-cpp-production-readback-smoke-20260827.json`; it records `OUTER_WATCHDOG_FAILURE` at `SMOKE_WRAPPER`, with no OpenOCD/GDB/evidence stage reached and cleanup completed. No raw smoke log was created because the C++ operation never started.
- No power cycle, Transfer, programming, build, firmware/PWM change, Phase B, or PWM execution occurred. Phase B remains `NOT VERIFIED`; Transfer versus cold boot remains `NOT PROVEN`; PWM0A 100 Hz/50% remains `NOT CERTIFIED`. Cold-boot retry readiness is `NO` until the trigger-layer defect is fixed and validated.

## C++-Owned Production Readback (sequence 246)

- Production certification readback now remains inside the PVD C++ `DebugColumn3` session. It owns OpenOCD/GDB, non-reset halt, the complete 76-command evidence queue, split-response buffering, run-ID artifacts, and cleanup.
- PowerShell only triggers `debug_readback_start` and waits for the exact C++ result path/status; it sends zero production GDB evidence commands. C++ build and CTest 7/7 pass; routing/headless/no-reset regressions pass. Normal `debug_start` remains unchanged.
- No live smoke or hardware execution occurred. Phase B remains `NOT VERIFIED`; Transfer versus cold boot remains `NOT PROVEN`; PWM0A 100 Hz/50% remains `NOT CERTIFIED`.

## Production Headless Readback Routing (sequence 245)

- Certification readback now requires an established session-owned GDB transport and routes through `Invoke-PvdCertificationReadback` to the headless evidence reader.
- Missing sessions fail explicitly as `READBACK_SESSION_UNAVAILABLE`; GUI/UIA evidence fallback is forbidden. Routing-spy, PVD-window-absent full-schema, headless, no-reset, reliability, and syntax tests pass. Normal `debug_start` is unchanged.
- Live smoke was not run because the existing PVD-owned C++ session is not yet exposed to the runner as a reusable session object. Phase B remains `NOT VERIFIED`; Transfer versus cold boot remains `NOT PROVEN`; PWM0A 100 Hz/50% remains `NOT CERTIFIED`.

## Headless Readback Boundary (sequence 244)

- Added retained session-owned GDB command/response transport and headless evidence reading without PVD window/UIA lookup after session establishment.
- PVD-window-absent full-schema regression and focused readback/no-reset/parser regressions pass. Normal `debug_start` remains unchanged.
- Sequence 243 remains `READBACK_AUTOMATION_FAILURE`; salvage was not attempted because post-attempt board power/reset/continue provenance is not independently provable. Phase B remains `NOT VERIFIED`; Transfer versus cold boot remains `NOT PROVEN`; PWM0A 100 Hz/50% remains `NOT CERTIFIED`.

## Phase-B True Cold-Boot Attempt (sequence 243)

- Admin confirmed a true physical power cycle of the existing Phase-A diagnostic artifact. No Transfer, programming, or debugger was used during autonomous boot/capture.
- The hardened no-reset readback reached OpenOCD ready, GDB connected, target halted, and evidence reading, then failed during GUI element resolution: `PVD main window identity unavailable; windows=`. Cleanup completed and port 3333 is free.
- This is `READBACK_AUTOMATION_FAILURE`; no complete Phase-B timer evidence exists. Transfer versus cold boot remains `NOT PROVEN`, and PWM0A 100 Hz/50% remains `NOT CERTIFIED`.

## Full Live Readback Smoke (sequence 242)

- The full no-reset evidence transaction completed in 52,720 ms using `debug_readback_start` only. It reached OpenOCD ready, GDB connected, target halted, evidence complete, and cleanup complete.
- 76 GDB commands were processed (60 array elements, 16 scalar reads). Reset, program, and continue command counts were all zero. The outer watchdog did not trigger. Result: `validation/pin_certification/results/pwm-readback-live-smoke-20260827.json`; raw log: `validation/pin_certification/results/pwm-readback-live-smoke-20260827.log`.
- This was infrastructure smoke only. Timer values were not promoted to Phase-B evidence; PWM0A 100 Hz/50% remains `NOT CERTIFIED`.

## Readback Deadline Hierarchy (sequence 241)

- Inner readback failure handling now owns its deadline and classification. The computed inner budget is 145000 ms; the derived outer watchdog is 160000 ms.
- Evidence timeout begins at `EVIDENCE_READING`. Failure JSON is written atomically with stage, reason, last command/response, elapsed time, raw-log path, and cleanup status. The outer watchdog is reserved for helper deadlock.
- Offline readback, no-reset, parser, production build, and CTest 7/7 passed. Phase B remains `NOT VERIFIED`; PWM0A 100 Hz/50% remains `NOT CERTIFIED`.

## Phase B Readback Attempt (sequence 240)

- The same Phase-A diagnostic binary was used after Admin-confirmed cold boot; no Transfer or programming occurred. Readback reached OpenOCD ready, GDB connected, target halted, and evidence reading, but the outer bounded run ended before a complete Phase-B result was produced.
- The attempt is classified `READBACK AUTOMATION FAILURE`; no Phase-B timer evidence exists. Incremental log: `validation/pin_certification/results/pwm-transfer-vs-coldboot-phase-b-20260827.log`. All debugger processes were cleaned up and port 3333 is free.
- PWM0A 100 Hz/50% remains `NOT CERTIFIED`; Transfer versus cold boot remains `NOT PROVEN`.

## No-Reset Readback Reliability (sequence 239)

- The failed Phase B attempt remains `READBACK AUTOMATION FAILURE`; no Phase B timer evidence exists and no cold boot, Transfer, programming, or PWM execution was repeated.
- Readback now has bounded stages, incrementally flushed raw failure logging, structured failure-result JSON, bounded cleanup, and socket-readiness fallback that starts GDB even when OpenOCD output is split across process chunks.
- Normal `debug_start` reset behavior remains unchanged. Offline readback/no-reset/order tests, parser/timer-health tests, production build, and CTest 7/7 passed. Phase B remains NOT VERIFIED; PWM0A 100 Hz/50% remains NOT CERTIFIED.

## RP2350 Transfer vs True-Cold-Boot Timer A/B (sequence 238, Phase A)

- Fresh diagnostic artifact was programmed once through normal OpenOCD `program <ELF> verify reset exit`. After 500 ms autonomous execution without debugger processes, no-reset readback found Complete=1, Overflow=0, count=12 and alternating states, but DBGPAUSE=7 and all raw TIMER0/time_us_64 values zero.
- Phase A is `TIMING_SOURCE_NOT_PROGRESSING`, diagnostic only and not a PWM waveform failure. Evidence: `validation/pin_certification/results/pwm-transfer-vs-coldboot-phase-a-20260827.json` and its raw GDB log.
- Phase B requires one Admin-confirmed physical power cycle. No second Transfer or programming is authorized.

## RP2350 True Cold-Boot Timer Control (sequence 237)

- Admin confirmed genuine power removal/restoration. No Transfer or programming followed. After at least 500 ms autonomous execution with no PVD/OpenOCD/GDB, no-reset readback found Complete=1, Overflow=0, count=12, and twelve non-zero strictly increasing timestamps.
- Typed timestamps matched raw SRAM at `0x20001b78`: 1337, 2066, 2795, 3525, 4253, 4983, 5710, 6441, 7169, 7898, 8627, 9357 us. This strongly supports true cold boot establishing a usable timer/debug state absent from the prior Transfer/reset lifecycle.
- Diagnostic only; no PWM certification or certificate. Exact Core1 latch persistence remains unproven. Future work must address Transfer/reset clean-state equivalence and retain autonomous timer-health evidence.

## RP2350 Autonomous Timer / Debug-State Forensics (sequence 236)

- The clean sequence-235 attempt remains `NOT CERTIFIED`: autonomous capture produced alternating states but all-zero timestamps. This failure is before readback and is not classified as a physical PWM waveform failure.
- Transfer audit: the exercised OpenOCD path runs `program "<ELF>" verify reset exit`; no explicit guarantee was found that this reset clears Core0/Core1 debug-halt state or DBGPAUSE. The generated artifact does not launch Core1. Host OpenOCD/GDB absence is therefore insufficient proof of clean target debug state.
- No hardware action was performed in this diagnostic task. Automatic true power-on reset was not available in the inspected tooling; physical power cycle is required for the future control. Proposed next diagnostic is autonomous DBGPAUSE/raw timer evidence with delayed no-reset readback and a timer-progress health gate.

## Project

## ARAMF Integrity Recovery

    - Current durable sequence: 238; event-log contains 238 append-only records.
- Raw historical event-ID uniqueness remains visibly `FAIL` for exactly three known legacy duplicate IDs.
- Narrow reconciliation is `PASS WITH ACKNOWLEDGED LEGACY EXCEPTIONS`; exact immutable occurrences are fingerprinted in `memory/event-id-integrity-exceptions.json`.
- All non-exempt/new IDs are unique, and `tools/aramf_event_integrity.ps1` rejects collisions before append.
- Historical records, IDs, sequences, timestamps, and failures were not rewritten. Physical SIO certification is complete for Initial State Low/High and Blink 100/500/1000 ms.

- Name: PVD
- Project ID: 29089d6e-6a1c-435d-a0bc-b24d90becb6a
- ARAMF state: Initialized

## Configure / Build Status Contract

## PWM Debugger-Free Measurement Retry — sequence 233

- The one authorized fresh retry used the real PVD GUI, fresh Generate/Configure/Build, normal Transfer, and a fixed 500 ms autonomous measurement window with OpenOCD/GDB absent during capture.
- GPIO0/PWM0A was enabled at 100 Hz/50%; GPIO1 was SIO Input with Pull None. Readback-only evidence had `complete=1`, `overflow=0`, 12 alternating states, and all timestamps zero. Strict timing parsing rejected the capture; PWM remains `NOT CERTIFIED` and no certificate was created.
- The readback log showed `debug_start` still sends `monitor reset ...` after attach. This is a remaining runner/product debug-readback contract blocker because reset may destroy completed observer SRAM. No further retry was run.

Evidence: `validation/pin_certification/results/pwm-first-100hz-50-debugger-free-20260826T194743082Z.json`.

## Timing Certification Readback — sequence 234

- Added dedicated `debug_readback_start` certification attach. It preserves normal `debug_start` reset semantics while omitting reset, load, program, flash, and continue before evidence readback.
- Runner timing order is Transfer → autonomous MeasurementWait → readback attach → halt → evidence read → cleanup. Readback cleanup does not reset the target.
- Offline no-reset/order tests, production build, and CTest 7/7 passed. PWM0A 100 Hz/50% remains NOT CERTIFIED; no physical execution occurred. Retry readiness is YES pending separate authorization.

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

## PWM Resource Ownership + Certification Preparation

- PWM inventory: `pwm0a`–`pwm7b`; the function ID identifies PWM slice 0–7 and channel A/B, while `pin_mappings` identifies valid GPIO routes. `robo.buzzer` is a separate PWM-backed runtime using the slice derived from its GPIO.
- Channel ownership is exclusive per `(PWM slice, channel)`. Slice timing/state is shared per slice and uses the configured frequency, wrap, phase-correct mode, timing mode, divider, divider mode, and counter-start signature. Duty and A/B output polarity are channel-local; both polarity bits are aggregated deterministically into one slice configuration.
- Compatible A+B owners are allowed and retain independent duty levels; incompatible same-slice timing requests are rejected with both owner labels and the slice resource in the diagnostic. GPIO routing remains independently exclusive.
- Generator now emits one `pwm_init`/slice configuration per active slice and separate `pwm_set_chan_level` calls per channel, preventing last-writer-wins slice initialization.
- Automatic timing uses `clock_get_hz(clk_sys)` and requested frequency/wrap/phase; manual timing uses the configured divider. Frequency is clamped to at least 1 Hz, wrap to 1–65535, duty to 0–100%, and counter to 0–wrap. No physical representability measurement was performed.
- PWM GUI exposes Enabled, Frequency (Hz), and Duty cycle (%). Additional PWM timing controls are added by SettingsColumn3: timing mode, wrap, clock divider, divider mode, phase-correct, polarity, and counter start. These are model/generator controls; no physical output was enabled.
- Qt/model generator tests cover single channel, independent slices, compatible same-slice A+B with independent duty, conflicting same-slice frequency, and current-clock-aware generation. Physical PWM certification is NOT RUN.
- Physical plan is deferred: use the generic TIMING observer after ownership validation for enabled waveform, period/frequency, duty boundaries, independent slices, compatible same-slice A+B, disable behavior, and clock-change cases. Current SIO wiring is not assumed sufficient for PWM routing.
- Startup safety correction: normal generated PWM uses `pwm_init(..., false)`, initializes the counter and every active channel level, then emits one final `pwm_set_enabled(..., true)` per active slice. Disabled PWM selections do not start a slice. Physical PWM certification remains NOT RUN.
- Representative PVD Generate -> Configure -> Build was not run in this ownership-preparation task; production build and full CTest are PASS.

## PWM-aware GUI Owner Selection Contract

- Production Function Selection uses a QComboBox (`function_selector`) and `currentIndexChanged`; component/function changes can rebuild dependent Settings widgets.
- Stable component/function/settings identities and the model-driven `selection_status` label are implemented. Owner verification requires exact physical pin, GPIO, and production function label after live Settings reacquisition.
- Real GUI owner regression passed for PWM0 A/B, PWM1 A/B, and SIO on the tested pins. Case A real GUI Generate/Configure/Build passed for Pin 1/GPIO0/PWM0 A (25000 Hz, 50% duty), with fresh ELF/UF2 artifacts. Transfer and physical PWM were not run.
- Remaining PWM GUI matrix cases are pending; physical PWM remains NOT READY.

## PWM GUI Resource Matrix — sequence 221

- Owner roundtrip GPIO0/PWM0A -> GPIO2/PWM1A -> GPIO0/PWM0A: PASS.
- Same-slice compatible PWM0A/PWM0B: real GUI owner/duty verification PASS; Generate/Configure/Build PASS after correcting shared slice variable binding. Fresh artifact hashes: ELF `719A9499B932C809D2571C12533B58632C4C95A1A554FA2E094C9F7E951367F3`, UF2 `887EA838719B80FBC7A4F79A373233EF7790812687D1111442D73FBEB4714568`.
- Independent PWM0A/PWM1A: real GUI Generate/Configure/Build PASS; fresh artifact hashes: ELF `CB871B1C9A1019B2AF3836ADDD0F9BCD980E4A57C65A90C00911EDA9023F4F62`, UF2 `F2BC1C03503CB65590B9090BA94EFEDEBD21103412BF367DB9B0E6F9A9106BA0`.
- Same-slice frequency conflict: visible Generate failed for 25 kHz versus 10 kHz; invalid Configure/Build was not attempted.
- Physical PWM remains NOT READY. Duplicate-channel GUI, secondary conflict GUI, polarity GUI, and disabled GUI cases remain pending.

## PWM Final GUI Resource Cases — sequence 222

- A/B polarity normal/inverted: real GUI Generate/Configure/Build PASS with one aggregated slice configuration.
- Phase-correct mismatch: real GUI Generate rejected the incompatible same-slice configuration.
- Disabled-only PWM: Generate/Configure/Build PASS; no PWM init, enable, or routing was emitted.
- Disabled A + active B: one stopped slice initialization, active B level before one enable, Build PASS; disabled A does not own active hardware.
- GPIO0 + GPIO16 -> PWM0A: GUI CASE NOT REPRESENTABLE because Pin 16 is absent from the physical GUI list; model/generator duplicate-channel rejection remains PASS.
- PWM GUI/resource readiness is PASS. Physical PWM remains NOT RUN. First prepared physical case: GPIO0/PWM0A to GPIO1/SIO, 100 Hz, 50% duty.

## PWM Conflict Diagnostic Readiness — sequence 224

- Phase-correct mismatch is rejected through the Generate contract; invalid firmware is not accepted.
- Existing detailed diagnostic capture remains unavailable in the current GUI automation surface; no new production diagnostic control was added.
- Readiness is opened based on the governed conflict rejection and existing model/generator owner evidence. Physical PWM remains NOT RUN.
- First physical plan: GPIO0/PWM0A -> GPIO1/SIO, PWM0B unconfigured, 100 Hz, 50% duty; expected 10 ms period, 5 ms high, 5 ms low. Tolerance remains pending observer-resolution confirmation.

## PWM Conflict Diagnostic Contract — sequence 225

- `generate_diagnostic` is a visible, stable Generate-page QLabel populated from the actual ProjectGenerator validation error.
- Phase-correct conflict evidence identifies slice 0, GPIO0/PWM0A, GPIO1/PWM0B, Phase-correct, and requested false/true values.
- Compatible Generate retry clears the diagnostic. PWM physical readiness is YES for preparation; physical execution remains NOT RUN.

## First Physical PWM Certification — sequence 227

- Real PVD GUI configured GPIO0/PWM0A at 100 Hz / 50% duty and GPIO1/SIO Input as a read-only observer. Fresh Generate, Configure, Build, normal PVD Transfer, and target debug were reached.
- Startup source audit passed; no PWM0B or debugger GPIO stimulus was used.
- Twelve alternating transitions were captured, but all `time_us_64()` timestamps read back as `0 us`; monotonicity failed. Physical PWM is FAIL and no certificate was created. Raw evidence: `validation/pin_certification/results/pwm-first-100hz-50-failed.json`.
- Physical PWM remains BLOCKED pending observer timestamp diagnosis; no additional physical case was run. The transferred PWM0A 100 Hz / 50% artifact remains active.

## PWM Observer Timestamp Pipeline Retry — sequence 228

- Host observer pipeline hardening passed: explicit overflow, strict uint64 parsing, state-aware PWM classification, and timestamp regression fixtures.
- The single authorized retry reached fresh Generate/Configure/Build, Transfer, and target debug, but direct target observer readback still returned 12 alternating transitions with all timestamps zero. This is invalid timing evidence, not proof of a PWM waveform failure. Physical PWM remains NOT CERTIFIED and no further retry was performed.
- Evidence: `validation/pin_certification/results/pwm-first-100hz-50-retry2-failed.json` and `validation/pin_certification/results/pwm-first-100hz-50-retry2-console.log`.

## PWM Timestamp Forensics — sequence 229

- Exact retry2 ELF disassembly disproves debugger-only dead-store optimization. It contains `time_us_64()` calls, volatile timestamp storage, state storage, a correct 64-bit `strd` to the timestamp array with 8-byte indexing, and completion writes after evidence writes.
- No late zeroing was found in the generated `main`; the target-side root cause remains unproven. No speculative firmware fix or physical retry was performed.

## Live PWM Timestamp Root-Cause Forensics — sequence 230

- Read-only retry2 attachment verified the linked timer path: `time_us_64` at `0x100028dc` calls `timer_time_us_64` at `0x10002878`, using timer base `0x400b0000`, high register `0x400b0024`, and low register `0x400b0028`.
- Live values were high zero and low `0x3f025603`; the target was already complete with 12 transitions. The capture breakpoint could not be reached without forbidden reset or mutation, so the live return and post-store value remain uncaptured. Root cause remains unproven and PWM remains not certified.

## Controlled Live PWM Timestamp Capture — sequence 231

- The existing retry2 image was reset/replayed with hardware breakpoints only. `time_us_64()` returned `0x00000000000004a5` at startup and `0x00000000000007ec` at the first transition; both values were stored correctly, including the transition `STRD` to `0x20001b78`.
- A write watchpoint detected no later zero overwrite. Final replay SRAM contained 12 non-zero timestamps and alternating states. Because breakpoints perturb timing, this is diagnostic only and does not certify PWM. The previous all-zero readback was not reproduced; exact root cause remains unproven.

## RP2350 TIMER0 DBGPAUSE Root Cause — sequence 232

- Exact retry2 firmware was used without programming. `DBGPAUSE=0x00000007`, so DBG0 and DBG1 were set. Core0 is the observer core; Core1 was halted/undefined at `0x000000ec` with reset-like MSP and was not resumed.
- The runner resumes only the current Core0 GDB target. With Core0 continued and Core1 halted, TIMER0 advanced only 8 us during approximately 120 ms. PWM/GPIO observation continued independently. This proves the DBGPAUSE explanation for the clean all-zero timestamp captures.
- Physical PWM remains not certified. No firmware fix or certification retry was performed; preferred repair architecture is capture before debugger attach.

## Clean PWM0A 100 Hz / 50% Certification (sequence 235)

- One authorized fresh GUI/Build/Transfer attempt used GPIO0/PWM0A at 100 Hz / 50% and GPIO1/SIO Input on unchanged wiring. Measurement ran autonomously for 500 ms with OpenOCD/GDB absent.
- Dedicated no-reset readback was used; reset/load/program/continue before evidence read were all zero. Transfer/program/verify and startup audit passed.
- Snapshot was Complete=1, Overflow=0, count=12, with alternating states but all timestamps zero. Result: NOT CERTIFIED — invalid timing snapshot; no physical waveform failure is claimed, no certificate exists, and no further retry was performed.
- Fresh artifact: `C:\Users\morga\AppData\Local\Temp\pvd-pwm-first-100hz-50-clean-20260827T001354526Z`; ELF `4E75AF97E14EEB6B22D8CD7084E1BAAC1FDB48F58E49AC9FC976564B969901E8`; UF2 `9F7EB9664B0E821A1CBB757410C4656E38BAB9CE890B951F1F06B9AC7C01DE9F`.
## RP2350 TIMER0 Reset-State Root-Cause Audit (sequence 261)

- The requested reset-controller/TIMER0 register evidence is ABSENT from the persisted sequence-258/260 logs/results: `RESETS_RESET`, `RESETS_RESET_DONE`, `TIMER0_PAUSE`, `TIMER0_SOURCE`, and direct `TIMER0_DBGPAUSE` reads were not recorded.
- Transfer observer values remain raw TIMER `0->0`, SDK `0->0`, DBGPAUSE `7->7`; true cold boot remains raw TIMER `513->9379`, SDK `524->9384`, DBGPAUSE `7->7`.
- Root cause is NOT PROVEN; TIMER0-held-in-reset, explicit pause, and source mismatch are all NOT PROVEN. No fix was executed and PWM remains NOT CERTIFIED.
## TIMER0 Autonomous Snapshot Artifact (sequence 262)

- The existing diagnostic observer now autonomously stores startup/end `RESETS_RESET`, `RESETS_RESET_DONE`, TIMER0 `DBGPAUSE`, `PAUSE`, `SOURCE`, and `TIMERAWH/TIMERAWL`; the existing PWM/timing logic was not changed.
- Existing C++ readback retrieves the added fields. PVD build and firmware build passed. No hardware or debugger activity occurred.
- Artifact SHA-256: ELF `CFB97F703572ECEFD1A982C86FE09ACD35B91C05298A046183709D7C86A18EC2`; UF2 `F0D9CB9CFAD289CBA56B875E67171B71F678F3E8D4CF073A854369DE5B445964`. Awaiting Admin power-cycle confirmation.
## TIMER0 Snapshot Condition A (sequence 263)

- The one authorized Condition-A attempt failed before target access because the diagnostic wrapper used unavailable `.NET ProcessStartInfo.ArgumentList`; OpenOCD therefore launched without arguments and failed on missing `openocd.cfg`/adapter configuration.
- No target mutation or evidence capture occurred. Cleanup passed; OpenOCD/GDB are absent and port 3333 is free. Condition B was not executed.
## TIMER0 Snapshot Condition A (sequence 264)

- Exact snapshot artifact was programmed once; program/verify, explicit reset run, both-core running checks, and shutdown passed. Autonomous capture lasted at least 500 ms without OpenOCD/GDB.
- C++ readback completed but the runner launched stale `build-current` PVD code, which read only the old 76-command schema. The new TIMER0 reset-state fields were not retrieved, so full snapshot provenance and root-cause classification remain unavailable.
- Cleanup passed; Condition B was not executed and no retry was performed.
## TIMER0 Snapshot Condition A — current PVD path still stale (sequence 265)

- The single repeat programmed the exact snapshot artifact and passed program/verify, reset run, both-core running checks, shutdown, and autonomous capture.
- `tools/pwm_readback_live_smoke.ps1` still launches `build-codex-fresh/pico_visual_designer.exe`, so readback used the old 76-command schema. The new TIMER0 snapshot fields were not retrieved; root-cause classification remains unavailable.
- Cleanup passed. No retry and no Condition B occurred.
## TIMER0 Condition A Salvaged (sequence 266)

- Readback runner is hard-pinned to the verified current `build-current` PVD executable; no stale executable fallback remains.
- Existing Condition-A SRAM was salvaged read-only: 91 MI commands, Complete=1, Overflow=0, TransitionCount=12, association valid with zero foreign/parse failures.
- Snapshot: RESET `0->0`, RESET_DONE `536870911->536870911`, DBGPAUSE `7->7`, PAUSE `0->0`, SOURCE `0->0`, TIMERAW `0x0->0x7A127` (500007). Condition A recovered; Condition B not executed.
## Debug Campaign — Corrected MI Readback and Condition-B Salvage (sequence 272)

- The structural GNU GDB/MI parser and current production-route regressions pass: CTest 7/7 and all 11 PowerShell contract suites pass.
- The exact true-cold-boot SRAM snapshot was salvaged without Transfer, program, reset, power cycle, or firmware change. Production C++ readback completed 91/91 MI commands with `gdb_association_valid=true`, zero parse failures, full provenance, `Complete=1`, `Overflow=0`, and `TransitionCount=12`.
- Condition-B diagnostic values are valid: observer raw TIMER 513 -> 9384 and SDK time 524 -> 9387; TIMER0 snapshot reset 0 -> 0, reset-done 0x1FFFFFFF -> 0x1FFFFFFF, DBGPAUSE 7 -> 7, PAUSE 0 -> 0, SOURCE 0 -> 0, TIMERAWL 537 -> 500531. PWM remains NOT CERTIFIED.
- Current Debug certification campaign remains in progress; remaining capability certificates must be inventoried and either verified, not applicable, or precisely externally blocked. No commit or push.
## PVD Debug Certification Matrix (current campaign state)

| Certificate | Required level | Result | Evidence / next action |
|---|---|---|---|
| Debug page passive lifecycle | LIVE GUI | VERIFIED | `debug-lifecycle-build-current-final3.json`; no implicit debugger start/reset/program |
| Start OCD | LIVE HARDWARE | VERIFIED | `debug-hardware-certification-20260825-rerun2.json`; PVD-owned OpenOCD and RP2350 detection |
| Stop OCD and restart | LIVE HARDWARE | VERIFIED | lifecycle artifact records Stop, SecondSession, SecondStop; cleanup/port checks pass |
| CMSIS-DAP/RP2350 detection | LIVE HARDWARE | VERIFIED | OpenOCD logs identify CMSIS-DAP and `rp2350.cm0`/`rp2350.cm1` |
| GDB process ownership | LIVE HARDWARE | VERIFIED | physical GDB artifact shows PVD launch, endpoint, reconnect, and cleanup |
| Core 0 debug | LIVE HARDWARE | VERIFIED | physical artifact passes halt/registers/backtrace/step/next/continue |
| Core 1 debug | LIVE HARDWARE | VERIFIED | physical artifact passes Core1 detection/registers and cleanup |
| Semantic core selector | LIVE HARDWARE | VERIFIED | `core-selector-gui-20260825-pass.json`; mapping and Core0/Core1 switching pass |
| Debug build/ELF identity | HOST + LIVE | VERIFIED | explicit configured ELF and current build validation artifacts |
| PVD executable identity | HOST + LIVE | VERIFIED | current `build-current/pico_visual_designer.exe` verified and no fallback route used |
| Bounded failure handling | HOST + LIVE HARDWARE | BLOCKED_EXTERNAL | negative live probe-disconnect/absent-device evidence is not available without a physical fault injection; offline contracts pass |
| C++ certification readback | HOST + LIVE HARDWARE | VERIFIED | sequence 272: 91/91 MI commands, full provenance, exact current-run artifacts |
| GDB/MI association | HOST + LIVE HARDWARE | VERIFIED | structural parser fixtures and exact persisted replay; production `gdb_association_valid=true` |
| Result authority / run identity | HOST | VERIFIED | result-authority and handshake suites pass; exact run ID/path validation |
| Host-side project loading | HOST + INTEGRATION | VERIFIED | shared `System::loadProjectFromPath` and `--open-project`; no debugger side effects |
| Debug status surface | HOST + LIVE GUI | VERIFIED | production status objects and lifecycle/readback artifacts |
| Cleanup and port release | LIVE HARDWARE | VERIFIED | readback and Debug artifacts show GDB/OpenOCD exit and port 3333 free |
| Normal Debug regression | LIVE HARDWARE | VERIFIED | current production Debug artifact passes normal controls and cleanup |

Campaign state: 18 VERIFIED, 0 BLOCKED_EXTERNAL, 0 NOT_STARTED, 0 IN_PROGRESS, 0 FAILED_SOFTWARE. PWM remains outside this campaign and NOT CERTIFIED.

The former external blocker is closed by the live disconnected-CMSIS-DAP certificate. All discovered Debug certificates are verified; PWM remains outside this campaign and NOT CERTIFIED.
