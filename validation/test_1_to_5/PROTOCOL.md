# Canonical pins 1-5 validation protocol

This directory defines the canonical, executable-later protocol for physical pins 1-5. It is a protocol definition, not a certification result and not a campaign launch. All test results are initialized to `NOT-RUN`.

## Scope and pin mapping

| Physical pin | PVD GPIO identity | Protocol meaning |
|---:|---:|---|
| 1 | GPIO0 | PVD catalog functions and negative validation |
| 2 | GPIO1 | PVD catalog functions and negative validation |
| 3 | GND | ground classification, rejection, persistence; no programmable function |
| 4 | GPIO2 | PVD catalog functions and negative validation |
| 5 | GPIO3 | PVD catalog functions and negative validation |

The mapping is taken from `src/systems/components/PicoPinMap.cpp`. Function cases are taken from the per-function SQLite files under `src/systems/components/pin_functions/`; the generator's function list is intentionally not duplicated here as a second Source of Truth. The hardware comparison is recorded in `hardware_reference.jsonc` and is based on the official [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf), the [Pico SDK hardware API](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html), and the SDK's [RP2350 GPIO table](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_gpio/include/hardware/gpio.h).

## Canonical execution chain

Each test case contains all stages, in this order:

`Launch -> Navigation -> Selection -> Settings -> Save/Persistence -> Generate -> Configure -> Build -> Transfer -> Runtime -> Debug -> Cleanup`

Stages are verified through the real PVD GUI path where the workflow exists. A stage cannot be marked `PASS` unless it was executed and verified. Transfer, Runtime, and Debug are currently explicitly disabled by policy because the current certification setup does not permit new wiring or external hardware. Cleanup remains part of every case.

The initial campaign is not started by creating these files. A future runner must stop at the first blocking failure, leave later stages `NOT-RUN`, preserve the exact failure stage and reason, and write results through the existing certification/ARAMF recorder path.

## Result vocabulary

Allowed stage results are `NOT-RUN`, `PASS`, `FAIL`, `BLOCKED`, `NOT-APPLICABLE`, and `REQUIRES-EXTERNAL-HARDWARE`. No result in this protocol claims physical or electrical certification.

## Function coverage

The protocol includes every function currently exposed by PVD's pin-function databases for GPIO0-3, including SIO, PWM, PIO, UART, SPI, I2C, XIP/QMI, USB, and CoreSight Trace entries. The official table and PVD catalog match for all GPIO0-3 mux entries. PVD uses two naming aliases: `qmi_cs1n` for the official `XIP_CS1n / QMI CS1n` entry, and `uart0_tx_aux`/`uart0_rx_aux` for the official GPIO2/GPIO3 F11 UART0 TX/RX entries using the SDK's `GPIO_FUNC_UART_AUX` selector family. These aliases are explicit in every generated test case and in `hardware_reference.jsonc`. Requirements are category-specific and are recorded in each test case. GPIO0-3 also have an explicit invalid-function negative case retained for the campaign.

Pin 3 is handled separately as GND. Its protocol verifies classification, rejection of programmable functions, and persistence of the non-programmable identity. Generate, Configure, and Build are explicitly `should_test: false` and `NOT-RUN` for GND because they are not applicable.

## Capability discrepancies

The RP2350 hardware reference has now been resolved for GPIO0-GPIO3. No hardware-only, PVD-only conflict, protocol-missing, protocol-extra, generator-gap, or uncertain mux entries were found. The future campaign must still preserve the discrepancy classes for later pins and classify every discrepancy as:

1. hardware-supported and PVD-supported;
2. hardware-supported but PVD-not-supported;
3. PVD-exposed but hardware capability uncertain; or
4. not applicable to the board/pin.

Any future case in categories 2 or 3 must be represented and disabled through the existing project Source of Truth before certification; it must not be fabricated as a PASS in this protocol.

## Files

- `protocol.jsonc`: protocol identity, stage policy, Source-of-Truth references, and discrepancy policy.
- `hardware_reference.jsonc`: authoritative GPIO0-GPIO3 hardware inventory and comparison classification.
- `manifest.jsonc`: flat index and counts generated from the five pin documents.
- `schema/test_case.schema.jsonc`: structural contract for test cases and stage records.
- `pins/*.jsonc`: complete per-pin test definitions.
- `generate_protocol.py`: deterministic protocol/manifest generator.
- `validate_protocol.py`: structural self-validation; it does not launch PVD or touch hardware.
- `results/README.md`: result handling and evidence rules.

## Explicit non-actions

The pins 1-5 campaign was not run. No PVD project was changed, no hardware was rewired, no external device was added, no certification PASS was fabricated, and no commit or push is part of this task.
