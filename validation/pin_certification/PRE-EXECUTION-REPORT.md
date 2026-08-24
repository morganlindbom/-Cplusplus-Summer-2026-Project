# PRE-EXECUTION REPORT — PINS 1–10

Status: READY FOR REVIEW; physical execution has not started.

## A. Git state

```text
main...origin/main at ba1952780eed717cb1a24c606cf5fe506d535fac
?? validation/
```

The existing untracked `validation/` tree is intentional project work. No
files were deleted, reset, overwritten, or discarded. No commit or push was
performed for this protocol expansion.

## B–C. Test counts

- Existing canonical pins 1–5 protocol: 49 tests.
- New pins 1–10 protocol manifest: 111 total tests.
- New definitions: 62 tests: 44 pin 6–10 function/GND tests, 10 block-negative tests, and 8 SIO physical baseline cases.
- Existing pins 1–5 files remain byte-preserved; this is checked by `legacy_traceability.jsonc`.

## D. Physical pin map

| Pin | Identity | Type |
|---:|---|---|
| 1 | GPIO0 | GPIO |
| 2 | GPIO1 | GPIO |
| 3 | GND | GND |
| 4 | GPIO2 | GPIO |
| 5 | GPIO3 | GPIO |
| 6 | GPIO4 | GPIO |
| 7 | GPIO5 | GPIO |
| 8 | GND | GND |
| 9 | GPIO6 | GPIO |
| 10 | GPIO7 | GPIO |

Mapping authority: `src/systems/components/PicoPinMap.cpp`, with Pico 2 W
board selection and RP2350 SDK data recorded in `hardware_reference.jsonc`.

## E. GPIO function matrix summary

Every listed function is `hardware_supported=true`, `pvd_supported=true`,
`generator_supported=true`, `protocol_present=true`, and `should_test=true`.

| GPIO | PVD SQLite functions | Count |
|---:|---|---:|
| 0 | I2C0 SDA, PIO0, PIO1, PIO2, PWM0 A, QMI CS1n, SIO, SPI0 RX, UART0 TX, USB OVCUR DET | 10 |
| 1 | I2C0 SCL, PIO0, PIO1, PIO2, PWM0 B, SIO, SPI0 CSn, TRACECLK, UART0 RX, USB VBUS DET | 10 |
| 2 | I2C1 SDA, PIO0, PIO1, PIO2, PWM1 A, SIO, SPI0 SCK, TRACEDATA0, UART0 CTS, UART0 TX AUX, USB VBUS EN | 11 |
| 3 | I2C1 SCL, PIO0, PIO1, PIO2, PWM1 B, SIO, SPI0 TX, TRACEDATA1, UART0 RTS, UART0 RX AUX, USB OVCUR DET | 11 |
| 4 | I2C0 SDA, PIO0, PIO1, PIO2, PWM2 A, SIO, SPI0 RX, TRACEDATA2, UART1 TX, USB VBUS DET | 10 |
| 5 | I2C0 SCL, PIO0, PIO1, PIO2, PWM2 B, SIO, SPI0 CSn, TRACEDATA3, UART1 RX, USB VBUS EN | 10 |
| 6 | I2C1 SDA, PIO0, PIO1, PIO2, PWM3 A, SIO, SPI0 SCK, UART1 CTS, UART1 TX AUX, USB OVCUR DET | 10 |
| 7 | I2C1 SCL, PIO0, PIO1, PIO2, PWM3 B, SIO, SPI0 TX, UART1 RTS, UART1 RX AUX, USB VBUS DET | 10 |

No hardware-only, PVD-only, generator-only, protocol-missing, or uncertain
entries were found for GPIO0–GPIO7.

## F. GND pin tests

Pin 3 retains the legacy `P03-GND-CLASSIFICATION`,
`P03-GND-REJECT-FUNCTION`, and `P03-GND-PERSISTENCE` cases. Pin 8 adds:

- `P08-GND-CLASSIFICATION`
- `P08-GND-REJECT-FUNCTION`
- `P08-GND-PERSISTENCE`
- `P08-GND-GENERATOR`

These verify non-programmability, GUI rejection, persistence, and absence of
generated GPIO initialization. Pin 8 is explicitly `GND`, `gpio=null`, and
`programmable=false`.

## G. Installed physical rig

- Pair A: GPIO0 ↔ GPIO1, 1180 Ω series resistance
- Pair B: GPIO2 ↔ GPIO3, 1180 Ω series resistance
- Pair C: GPIO4 ↔ GPIO5, 1180 Ω series resistance
- Pair D: GPIO6 ↔ GPIO7, 1180 Ω series resistance
- Debugger: dedicated SWDIO, GND, SWCLK only
- Physical Pico header pins 1–3 are not used for debugger wiring

## H. First SIO batch

The first physical batch contains exactly these eight cases:

- `SIO-PAIR-A-FWD`: GPIO0 output → GPIO1 input
- `SIO-PAIR-A-REV`: GPIO1 output → GPIO0 input
- `SIO-PAIR-B-FWD`: GPIO2 output → GPIO3 input
- `SIO-PAIR-B-REV`: GPIO3 output → GPIO2 input
- `SIO-PAIR-C-FWD`: GPIO4 output → GPIO5 input
- `SIO-PAIR-C-REV`: GPIO5 output → GPIO4 input
- `SIO-PAIR-D-FWD`: GPIO6 output → GPIO7 input
- `SIO-PAIR-D-REV`: GPIO7 output → GPIO6 input

Each case verifies GUI selection, direction, initial low, high/low
propagation, readback, pull settings where valid, persistence, Generate,
Configure, Build, Transfer, Runtime, Debug, physical loopback, and cleanup.
The two pins are never configured as opposing outputs.

## I. Disabled or gated tests

All results remain `NOT-RUN`. Transfer, Runtime, and Debug are gated pending
review of this report and the physical execution workflow. Physical stages
remain represented with exact equipment boundaries:

- I2C: external open-drain target and valid pull-ups required; resistor links are not an I2C bus.
- UART: role validation is present; campaign-level TX/RX execution requires review before running.
- SPI: complete TX/RX/SCK/CSn wiring does not match the fixed pairs without rewiring.
- Trace: trace capture setup and dedicated trace equipment required.
- USB: USB power/control test fixture required.
- QMI/XIP: QMI/XIP-specific hardware and safe flash/PSRAM setup required.
- Exact PWM timing: oscilloscope or logic analyzer required; signal presence may be tested separately.
- Future groups 11–20, 21–30, and 31–40: not executed and not populated with fabricated tests/results.

## J. Confirmation

**NO PHYSICAL TEST EXECUTION HAS STARTED.**

This report is the execution gate. Stop here until it has been reviewed.
