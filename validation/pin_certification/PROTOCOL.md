# Pico 2 W full-board pin certification protocol

This is the reusable protocol definition for four future physical groups:

1. pins 1–10 (current scope)
2. pins 11–20
3. pins 21–30
4. pins 31–40

The prior canonical pins 1–5 protocol remains at `validation/test_1_to_5/` and
is intentionally unchanged. `legacy_traceability.jsonc` records its file
hashes, test IDs, and count. The current manifest includes those 49 legacy
tests by reference and adds 62 new definitions, for 111 total protocol cases.

## Verified first block

| Physical pin | Board identity | Type |
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

The mapping is checked against `PicoPinMap.cpp`; alternate functions are
discovered from the PVD SQLite function databases. RP2350 SDK selector families
and the RP2350 GPIO Bank 0 table are recorded in `hardware_reference.jsonc`.

## Installed rig

- Pair A: GPIO0 ↔ GPIO1 through 1180 Ω
- Pair B: GPIO2 ↔ GPIO3 through 1180 Ω
- Pair C: GPIO4 ↔ GPIO5 through 1180 Ω
- Pair D: GPIO6 ↔ GPIO7 through 1180 Ω
- Debugger: dedicated SWDIO, GND, SWCLK interface only
- Physical header pins 1–3 are not used for debugger wiring

The SIO baseline has eight exact cases, two directions per pair:
`SIO-PAIR-A-FWD`, `SIO-PAIR-A-REV`, through `SIO-PAIR-D-FWD` and
`SIO-PAIR-D-REV`. Both sides are never configured as opposing outputs.

## Execution gate

This directory defines tests only. Every case and every stage starts at
`NOT-RUN`; no physical campaign has started. The first execution phase is the
SIO baseline. PWM, PIO, UART, SPI, I2C, trace, USB/QMI, and conflict cases stay
represented with explicit feasibility and equipment requirements.

I2C resistor links are not treated as an I2C bus. Physical I2C requires a
real open-drain target and valid pull-ups. Exact PWM timing requires an
oscilloscope or logic analyzer. Trace, USB, QMI, and analog/special functions
retain their special-equipment boundaries in the case metadata.
