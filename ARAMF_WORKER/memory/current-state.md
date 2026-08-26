<!-- current-state.md -->

# Current Project State

## Latest Durable Sequence

204

## Latest Production Development Event

8025fad7-675d-433d-ae96-26403f6816d3

## ARAMF Integrity Recovery

- Raw historical event-ID uniqueness: FAIL for exactly three known legacy duplicate IDs (six immutable occurrences).
- Legacy reconciliation: PASS under `memory/event-id-integrity-exceptions.json`, which fingerprints the exact sequence/event-type/line occurrences.
- All non-exempt event IDs: unique. Future event creation is collision-guarded by `tools/aramf_event_integrity.ps1`.
- Historical sequences 129-131 and 159-161, including 160/161/162, are preserved unchanged. Current durable sequence: 204.
- Certification readiness: READY under the current contract for future physical work; no physical execution occurred in this repair task.

## Latest Status Contract

- Build page exposes visible `QLabel` `build_status` with `Idle`, Configure running/completed/failed,
  and Build running/completed/failed states.
- Internal `BuildWorkflowState` is canonical; the label is presentation and automation synchronization.
- Fresh no-Transfer Configure/Build validation passed for Initial Low, Initial High, and Blink 100 ms.

## SIO Physical Certification

- Initial State Low: PHYSICAL PASS; Initial State High: PHYSICAL PASS.
- Blink 100 ms: PHYSICAL PASS; Blink 500 ms: PHYSICAL PASS; Blink 1000 ms: PHYSICAL PASS.
- Blink capability scope is limited to the measured 100/500/1000 ms values; the full 10-60000 ms domain is not claimed.
- Existing Blink 500/1000 evidence was inspected and adopted append-only; no duplicate physical run was performed during resume.
- Final active artifact: existing Blink 1000 artifact `C:\Users\morga\AppData\Local\Temp\pvd-sio-20260826T042152311Z\SIO-PAIR-D-FWD`.

## Latest Production Sequence

## SIO Software/Generator Matrix

- SIO settings: Direction Input/Output; Pull None/Pull-up/Pull-down; Initial State Low/High; Blink disabled/enabled with interval; Drive Strength 2/4/8/12 mA; Slew Slow/Fast; debounce_ms Input-only.
- Generator tests PASS for Pull, drive-strength, slew-rate mappings, initial latch ordering, Blink 100 ms runtime mapping, and confirmation that debounce has no SIO runtime consumer.
- Qt integration target `sio_model` passes with real SettingsColumn3/QComboBox widgets: Pull transitions and fresh Pull-up reload, Drive Strength 2/4/8/12, Slew Slow/Fast, signal/model propagation, and multi-pin isolation. The manual Pull-up visible-user oracle and Save/reopen also pass.
- Windows UIA native Qt popup enumeration is unreliable for certification; the runner's historical failures remain preserved and are not a product failure. No physical execution occurred in this closeout.
- SIO software/GUI family status: COMPLETE WITH DOCUMENTED PHYSICAL DEFERRALS. Pull physical, Drive Strength measurement, and Slew oscilloscope validation remain deferred; Debounce has no SIO runtime consumer.
- GPIO IRQ/events, input hysteresis, and input enable are not exposed.

5
