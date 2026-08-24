"""Validate the reusable pins 1-10 protocol without executing hardware."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).parent
REPO = ROOT.parent.parent
LEGACY = REPO / "validation" / "test_1_to_5"
STAGES = ["launch", "navigation", "selection", "settings", "save_persistence", "generate", "configure", "build", "transfer", "runtime", "debug", "physical", "cleanup"]
PIN_MAP = {1: 0, 2: 1, 3: None, 4: 2, 5: 3, 6: 4, 7: 5, 8: None, 9: 6, 10: 7}


def load(path: Path) -> dict:
    return json.loads("\n".join(line for line in path.read_text(encoding="utf-8").splitlines() if not line.lstrip().startswith("//")))


def validate() -> list[str]:
    errors: list[str] = []
    protocol = load(ROOT / "protocol.jsonc")
    manifest = load(ROOT / "manifest.jsonc")
    hardware = load(ROOT / "hardware_reference.jsonc")
    traceability = load(ROOT / "legacy_traceability.jsonc")
    if protocol.get("campaign_started") or protocol.get("physical_certification_claimed"):
        errors.append("campaign must remain unstarted and uncertified")
    if protocol.get("canonical_stages") != STAGES:
        errors.append("canonical stage list mismatch")
    for pin, gpio in PIN_MAP.items():
        if pin <= 5:
            continue
        candidates = sorted((ROOT / "groups" / "pins_01_10").glob(f"pin_{pin:02d}_*.jsonc"))
        if not candidates:
            errors.append(f"missing pin document {pin}")
            continue
        document = load(candidates[0])
        if document.get("physical_pin") != pin or document.get("gpio") != gpio:
            errors.append(f"pin map mismatch for physical pin {pin}")
        if gpio is None and document.get("pin_type") != "GND":
            errors.append(f"GND type missing for physical pin {pin}")
        seen: set[str] = set()
        for test in document.get("tests", []):
            test_id = test.get("test_id")
            if not test_id or test_id in seen:
                errors.append(f"duplicate/missing test id in pin {pin}: {test_id}")
            seen.add(test_id)
            for field in ("hardware_supported", "pvd_supported", "generator_supported", "protocol_present", "should_test"):
                if not isinstance(test.get(field), bool):
                    errors.append(f"missing boolean {field}: {test_id}")
            stages = test.get("stages", {})
            if list(stages) != STAGES:
                errors.append(f"stage list mismatch: {test_id}")
            for stage in STAGES:
                value = stages.get(stage, {})
                if not isinstance(value.get("should_test"), bool) or value.get("result") != "NOT-RUN":
                    errors.append(f"stage not explicit NOT-RUN: {test_id}/{stage}")
                if not value.get("should_test") and not value.get("disabled_reason"):
                    errors.append(f"disabled stage has no reason: {test_id}/{stage}")
    for gpio in range(8):
        entries = hardware.get("gpio", {}).get(str(gpio), [])
        if not entries:
            errors.append(f"empty hardware matrix GPIO{gpio}")
        for entry in entries:
            for field in ("hardware_supported", "pvd_supported", "generator_supported", "protocol_present", "should_test"):
                if entry.get(field) is not True:
                    errors.append(f"unresolved capability matrix entry GPIO{gpio}/{entry.get('pvd_function_id')}/{field}")
    for path in (ROOT / "groups" / "pins_01_10").glob("*.jsonc"):
        document = load(path)
        for test in document.get("tests", []):
            if list(test.get("stages", {})) != STAGES:
                errors.append(f"stage list mismatch: {test.get('test_id')}")
    legacy_manifest = load(LEGACY / "manifest.jsonc")
    if traceability.get("test_count") != legacy_manifest.get("test_count") or traceability.get("test_ids") != [entry["test_id"] for entry in legacy_manifest["tests"]]:
        errors.append("legacy test IDs/count changed")
    for relative, digest in traceability.get("preserved_byte_hashes", {}).items():
        path = REPO / relative
        if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            errors.append(f"legacy file changed: {relative}")
    if manifest.get("test_count") != 111 or manifest.get("legacy_test_count") != 49 or manifest.get("new_test_count") != 62:
        errors.append("manifest count mismatch")
    return errors


if __name__ == "__main__":
    problems = validate()
    if problems:
        for problem in problems:
            print(f"ERROR: {problem}")
        raise SystemExit(1)
    print("PASS: reusable pins 1-10 protocol is structurally consistent")
    print("PASS: legacy pins 1-5 files are byte-preserved")
    print("PASS: no physical campaign execution was performed")
