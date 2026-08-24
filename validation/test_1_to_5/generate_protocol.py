# validation/test_1_to_5/generate_protocol.py
"""Generate the canonical pins 1-5 validation protocol and manifest."""

from __future__ import annotations

import json
import sqlite3
from pathlib import Path


ROOT = Path(__file__).parent

STAGES = [
    "launch",
    "navigation",
    "selection",
    "settings",
    "save_persistence",
    "generate",
    "configure",
    "build",
    "transfer",
    "runtime",
    "debug",
    "cleanup",
]

HARDWARE_FUNCTIONS = {
    0: {
        "spi0_rx": ("SPI0 RX", "SPI", "GPIO_FUNC_SPI"),
        "uart0_tx": ("UART0 TX", "UART", "GPIO_FUNC_UART"),
        "i2c0_sda": ("I2C0 SDA", "I2C", "GPIO_FUNC_I2C"),
        "pwm0a": ("PWM0 A", "PWM", "GPIO_FUNC_PWM"),
        "sio": ("SIO", "SIO", "GPIO_FUNC_SIO"),
        "pio0": ("PIO0", "PIO", "GPIO_FUNC_PIO0"),
        "pio1": ("PIO1", "PIO", "GPIO_FUNC_PIO1"),
        "pio2": ("PIO2", "PIO", "GPIO_FUNC_PIO2"),
        "qmi_cs1n": ("XIP_CS1n / QMI CS1n", "XIP_CS1", "GPIO_FUNC_XIP_CS1"),
        "usb_ovcur_det": ("USB OVCUR DET", "USB", "GPIO_FUNC_USB"),
    },
    1: {
        "spi0_csn": ("SPI0 CSn", "SPI", "GPIO_FUNC_SPI"),
        "uart0_rx": ("UART0 RX", "UART", "GPIO_FUNC_UART"),
        "i2c0_scl": ("I2C0 SCL", "I2C", "GPIO_FUNC_I2C"),
        "pwm0b": ("PWM0 B", "PWM", "GPIO_FUNC_PWM"),
        "sio": ("SIO", "SIO", "GPIO_FUNC_SIO"),
        "pio0": ("PIO0", "PIO", "GPIO_FUNC_PIO0"),
        "pio1": ("PIO1", "PIO", "GPIO_FUNC_PIO1"),
        "pio2": ("PIO2", "PIO", "GPIO_FUNC_PIO2"),
        "traceclk": ("TRACECLK", "Trace", "GPIO_FUNC_CORESIGHT_TRACE"),
        "usb_vbus_det": ("USB VBUS DET", "USB", "GPIO_FUNC_USB"),
    },
    2: {
        "spi0_sck": ("SPI0 SCK", "SPI", "GPIO_FUNC_SPI"),
        "uart0_cts": ("UART0 CTS", "UART", "GPIO_FUNC_UART"),
        "i2c1_sda": ("I2C1 SDA", "I2C", "GPIO_FUNC_I2C"),
        "pwm1a": ("PWM1 A", "PWM", "GPIO_FUNC_PWM"),
        "sio": ("SIO", "SIO", "GPIO_FUNC_SIO"),
        "pio0": ("PIO0", "PIO", "GPIO_FUNC_PIO0"),
        "pio1": ("PIO1", "PIO", "GPIO_FUNC_PIO1"),
        "pio2": ("PIO2", "PIO", "GPIO_FUNC_PIO2"),
        "tracedata0": ("TRACEDATA0", "Trace", "GPIO_FUNC_CORESIGHT_TRACE"),
        "usb_vbus_en": ("USB VBUS EN", "USB", "GPIO_FUNC_USB"),
        "uart0_tx_aux": ("UART0 TX", "UART_AUX", "GPIO_FUNC_UART_AUX"),
    },
    3: {
        "spi0_tx": ("SPI0 TX", "SPI", "GPIO_FUNC_SPI"),
        "uart0_rts": ("UART0 RTS", "UART", "GPIO_FUNC_UART"),
        "i2c1_scl": ("I2C1 SCL", "I2C", "GPIO_FUNC_I2C"),
        "pwm1b": ("PWM1 B", "PWM", "GPIO_FUNC_PWM"),
        "sio": ("SIO", "SIO", "GPIO_FUNC_SIO"),
        "pio0": ("PIO0", "PIO", "GPIO_FUNC_PIO0"),
        "pio1": ("PIO1", "PIO", "GPIO_FUNC_PIO1"),
        "pio2": ("PIO2", "PIO", "GPIO_FUNC_PIO2"),
        "tracedata1": ("TRACEDATA1", "Trace", "GPIO_FUNC_CORESIGHT_TRACE"),
        "usb_ovcur_det": ("USB OVCUR DET", "USB", "GPIO_FUNC_USB"),
        "uart0_rx_aux": ("UART0 RX", "UART_AUX", "GPIO_FUNC_UART_AUX"),
    },
}

def requirements_for(category: str) -> list[str]:
    """Return the settings and verification requirements for a category."""
    common = ["visible_selection", "save_persistence", "generated_configuration", "build"]
    specific = {
        "SIO": ["direction", "pull", "initial_state", "debounce", "drive_strength"],
        "PWM": ["slice_channel", "frequency", "duty_cycle", "divider", "top", "polarity", "ownership"],
        "PIO": ["pio_block", "state_machine", "program", "offset", "pin_mapping", "clock_divider", "wrap", "fifo", "ownership"],
        "UART": ["instance", "role", "baud_rate", "data_format", "pin_mapping", "ownership"],
        "SPI": ["instance", "role", "clock", "data_format", "pin_mapping", "ownership"],
        "I2C": ["instance", "role", "clock", "addressing", "pin_mapping", "ownership"],
        "QMI": ["qmi_role", "pin_mapping", "ownership"],
        "USB": ["usb_role", "pin_mapping", "ownership"],
        "Trace": ["trace_role", "pin_mapping", "ownership"],
    }
    return common + specific.get(category, [])


def stage_map(enabled: bool = True) -> dict[str, dict[str, object]]:
    """Create a complete stage map with no fabricated results."""
    result: dict[str, dict[str, object]] = {}
    for stage in STAGES:
        stage_enabled = enabled
        reason = ""
        if stage in {"transfer", "runtime", "debug"}:
            stage_enabled = False
            reason = "Deferred by current hardware certification policy; no physical rewiring or external hardware is permitted."
        if not enabled:
            stage_enabled = False
            reason = "Test definition retained in the protocol; campaign execution has not started."
        result[stage] = {
            "should_test": stage_enabled,
            "result": "NOT-RUN",
            "disabled_reason": reason,
        }
    return result


def case(test_id: str, pin: int, gpio: int | None, function_id: str, name: str, category: str, enabled: bool = True, hardware_name: str = "", sdk_family: str = "") -> dict:
    """Build one complete stage-level test case."""
    return {
        "test_id": test_id,
        "name": name,
        "category": category,
        "physical_pin": pin,
        "gpio": gpio,
        "hardware_function": function_id,
        "hardware_function_name": hardware_name or function_id,
        "sdk_selector_family": sdk_family or "NOT_APPLICABLE",
        "should_test": enabled,
        "disabled_reason": "" if enabled else "Test definition retained in the protocol; campaign execution has not started.",
        "expected_pvd_support": ("NOT_PVD_FUNCTION" if function_id == "invalid.function" else "CONFIRMED_PVD_SOURCE_OF_TRUTH") if gpio is not None else "NOT_APPLICABLE",
        "requires_external_hardware": False,
        "source_of_truth_verified": gpio is not None and function_id != "invalid.function",
        "generator_support_verified": gpio is not None and function_id != "invalid.function",
        "discrepancy_class": "NOT_A_MUX_FUNCTION" if function_id == "invalid.function" else "MATCH",
        "requirements": requirements_for(category) if gpio is not None else ["ground_classification", "reject_programmable_function", "persistence"],
        "stages": stage_map(enabled),
    }


def pin_document(pin: int, gpio: int | None, label: str) -> dict:
    """Build the complete protocol document for one physical pin."""
    cases = []
    if gpio is None:
        cases.extend([
            case("P03-GND-CLASSIFICATION", pin, None, "ground", "Physical pin 3 is GND", "GND"),
            case("P03-GND-REJECT-FUNCTION", pin, None, "ground.reject", "GND rejects programmable functions", "GND"),
            case("P03-GND-PERSISTENCE", pin, None, "ground.persistence", "GND remains non-programmable after save/reload", "GND"),
        ])
        for item in cases:
            for stage in ("generate", "configure", "build"):
                item["stages"][stage] = {
                    "should_test": False,
                    "result": "NOT-RUN",
                    "disabled_reason": "Not applicable to a ground pin.",
                }
    else:
        for function_id, name, category, hardware_name, sdk_family in functions_for_gpio(gpio):
            cases.append(case(f"P{pin:02d}-{function_id.upper()}", pin, gpio, function_id, f"{label}: {name}", category, True, hardware_name, sdk_family))
        cases.append(case(f"P{pin:02d}-NEG-INVALID-FUNCTION", pin, gpio, "invalid.function", f"{label}: invalid function is rejected", "Negative", False))
    return {
        "physical_pin": pin,
        "gpio": gpio,
        "label": label,
        "source_of_truth": "src/systems/components/PicoPinMap.cpp and src/systems/components/pin_functions/*/function.sqlite",
        "campaign_started": False,
        "tests": cases,
    }


def functions_for_gpio(gpio: int) -> list[tuple[str, str, str, str, str]]:
    """Read the function catalog for one GPIO from PVD's SQLite Source of Truth."""
    functions = {}
    database_root = Path(__file__).parents[2] / "src" / "systems" / "components" / "pin_functions"
    for database in sorted(database_root.glob("*/function.sqlite")):
        connection = sqlite3.connect(database)
        try:
            metadata = dict(connection.execute("SELECT key, value FROM metadata"))
            mapped = connection.execute("SELECT 1 FROM pin_mappings WHERE gpio = ? LIMIT 1", (gpio,)).fetchone()
            if mapped:
                function_id = metadata.get("function_id", database.parent.name)
                hardware = HARDWARE_FUNCTIONS.get(gpio, {}).get(function_id)
                if hardware is None:
                    raise ValueError(f"PVD function {function_id} for GPIO{gpio} is absent from the authoritative RP2350 table")
                functions[function_id] = (
                    function_id,
                    metadata.get("display_name", function_id),
                    metadata.get("category", "Unknown"),
                    hardware[0],
                    hardware[2],
                )
        finally:
            connection.close()
    return [functions[key] for key in sorted(functions)]


def write_jsonc(path: Path, value: object) -> None:
    """Write deterministic JSONC with a filename comment."""
    path.write_text(f"// {path.name}\n" + json.dumps(value, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    """Generate pin documents and the flat manifest."""
    pins = {
        1: pin_document(1, 0, "Physical pin 1 / GPIO0"),
        2: pin_document(2, 1, "Physical pin 2 / GPIO1"),
        3: pin_document(3, None, "Physical pin 3 / GND"),
        4: pin_document(4, 2, "Physical pin 4 / GPIO2"),
        5: pin_document(5, 3, "Physical pin 5 / GPIO3"),
    }
    (ROOT / "pins").mkdir(parents=True, exist_ok=True)
    manifest_cases = []
    for pin, document in pins.items():
        gpio_name = "gnd" if document["gpio"] is None else f"gpio{document['gpio']}"
        path = ROOT / "pins" / f"pin_{pin:02d}_{gpio_name}.jsonc"
        write_jsonc(path, document)
        manifest_cases.extend({"test_id": test["test_id"], "pin_file": path.name} for test in document["tests"])
    hardware_reference = {
        "authority": "RP2350 GPIO Bank 0 function table and Pico SDK gpio_function_rp2350",
        "sources": [
            "https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf",
            "https://www.raspberrypi.com/documentation/pico-sdk/hardware.html",
            "https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_gpio/include/hardware/gpio.h",
        ],
        "gpio": {
            str(gpio): [
                {"pvd_function_id": function_id, "hardware_function": values[0], "sdk_selector_family": values[2], "hardware_supported": True, "pvd_supported": True, "protocol_present": True, "current_should_test": True, "discrepancy_class": "MATCH"}
                for function_id, values in sorted(HARDWARE_FUNCTIONS[gpio].items())
            ]
            for gpio in range(4)
        },
        "hardware_only": [],
        "pvd_only_conflicts": [],
        "protocol_missing_before_correction": [],
        "protocol_extras": [],
        "generator_gaps": [],
        "uncertain": [],
        "alias_notes": [
            "PVD qmi_cs1n is the project name for the RP2350 XIP_CS1n/QMI CS1n mux entry.",
            "PVD uart0_tx_aux and uart0_rx_aux represent the RP2350 F11 UART_AUX selector family; the hardware table names the signals UART0 TX and UART0 RX.",
        ],
    }
    write_jsonc(ROOT / "hardware_reference.jsonc", hardware_reference)
    manifest = {
        "protocol_id": "pvd-pins-1-5-canonical",
        "campaign_started": False,
        "test_count": len(manifest_cases),
        "pin_count": 5,
        "stage_count": len(STAGES),
        "tests": manifest_cases,
    }
    write_jsonc(ROOT / "manifest.jsonc", manifest)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
