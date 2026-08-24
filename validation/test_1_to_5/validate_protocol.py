# validation/test_1_to_5/validate_protocol.py
"""Validate the canonical pins 1-5 protocol without executing any test."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).parent
STAGES = ["launch", "navigation", "selection", "settings", "save_persistence", "generate", "configure", "build", "transfer", "runtime", "debug", "cleanup"]
PIN_FILES = {1: "pin_01_gpio0.jsonc", 2: "pin_02_gpio1.jsonc", 3: "pin_03_gnd.jsonc", 4: "pin_04_gpio2.jsonc", 5: "pin_05_gpio3.jsonc"}


def load_jsonc(path: Path) -> dict:
    """Load the restricted JSONC used by the protocol."""
    lines = [line for line in path.read_text(encoding="utf-8").splitlines() if not line.lstrip().startswith("//")]
    return json.loads("\n".join(lines))


def validate() -> list[str]:
    """Return all protocol consistency errors."""
    errors = []
    manifest = load_jsonc(ROOT / "manifest.jsonc")
    hardware_reference = load_jsonc(ROOT / "hardware_reference.jsonc")
    seen = []
    all_tests = []
    for pin, filename in PIN_FILES.items():
        path = ROOT / "pins" / filename
        if not path.exists():
            errors.append(f"missing pin file: {path}")
            continue
        document = load_jsonc(path)
        if document.get("physical_pin") != pin:
            errors.append(f"wrong physical pin in {filename}")
        expected_gpio = {1: 0, 2: 1, 3: None, 4: 2, 5: 3}[pin]
        if document.get("gpio") != expected_gpio:
            errors.append(f"wrong GPIO mapping in {filename}")
        for test in document.get("tests", []):
            all_tests.append(test)
            test_id = test.get("test_id")
            if not test_id or test_id in seen:
                errors.append(f"duplicate or missing test id: {test_id}")
            seen.append(test_id)
            if test.get("result") is not None:
                errors.append(f"top-level result must not replace stage results: {test_id}")
            if not isinstance(test.get("should_test"), bool):
                errors.append(f"missing should_test: {test_id}")
            for field in ("hardware_function_name", "sdk_selector_family", "discrepancy_class"):
                if not test.get(field):
                    errors.append(f"missing {field}: {test_id}")
            if not isinstance(test.get("generator_support_verified"), bool):
                errors.append(f"missing generator_support_verified: {test_id}")
            if not test.get("should_test") and not test.get("disabled_reason"):
                errors.append(f"disabled test has no reason: {test_id}")
            stages = test.get("stages", {})
            if list(stages) != STAGES:
                errors.append(f"incomplete stage list: {test_id}")
            for stage in STAGES:
                value = stages.get(stage, {})
                if not isinstance(value.get("should_test"), bool) or value.get("result") != "NOT-RUN":
                    errors.append(f"stage is not explicit and NOT-RUN: {test_id}/{stage}")
                if not value.get("should_test") and not value.get("disabled_reason"):
                    errors.append(f"disabled stage has no reason: {test_id}/{stage}")
    manifest_ids = [entry.get("test_id") for entry in manifest.get("tests", [])]
    if manifest_ids != seen:
        errors.append("manifest test index does not match pin documents")
    if manifest.get("test_count") != len(seen):
        errors.append("manifest test_count is incorrect")
    if manifest.get("stage_count") != len(STAGES):
        errors.append("manifest stage_count is incorrect")
    if hardware_reference.get("hardware_only") or hardware_reference.get("pvd_only_conflicts") or hardware_reference.get("protocol_missing_before_correction") or hardware_reference.get("protocol_extras") or hardware_reference.get("generator_gaps") or hardware_reference.get("uncertain"):
        errors.append("hardware reference contains unresolved discrepancy entries")
    for gpio in range(4):
        inventory = hardware_reference.get("gpio", {}).get(str(gpio), [])
        inventory_ids = {entry.get("pvd_function_id") for entry in inventory}
        protocol_ids = {test.get("hardware_function") for test in all_tests if test.get("gpio") == gpio and test.get("category") != "Negative"}
        if inventory_ids != protocol_ids:
            errors.append(f"hardware/protocol inventory mismatch for GPIO{gpio}")
        if any(not all(entry.get(field) is True for field in ("hardware_supported", "pvd_supported", "protocol_present")) or entry.get("discrepancy_class") != "MATCH" for entry in inventory):
            errors.append(f"unresolved hardware classification for GPIO{gpio}")
    return errors


def main() -> int:
    """Run validation and report a machine-readable success result."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    errors = validate()
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1
    print("PASS: canonical pins 1-5 protocol is structurally consistent")
    print("PASS: no campaign execution was performed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
