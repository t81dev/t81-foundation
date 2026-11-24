#!/usr/bin/env python3
"""Validate live C++ t81_provider serialized JSON against committed draft JSON Schemas."""
from __future__ import annotations

import json
import subprocess
import sys
import unittest
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
CAPS_SCHEMA_PATH = REPO_ROOT / "docs/research/proposals/provider-capabilities-v1.schema.json"
EVID_SCHEMA_PATH = REPO_ROOT / "docs/research/proposals/provider-evidence-v1.schema.json"


def _check_type(val: Any, expected: str | list[str], path: str) -> None:
    allowed = expected if isinstance(expected, list) else [expected]
    for typ in allowed:
        if typ == "null" and val is None:
            return
        if typ == "boolean" and isinstance(val, bool):
            return
        if typ == "integer" and isinstance(val, int) and not isinstance(val, bool):
            return
        if typ == "string" and isinstance(val, str):
            return
        if typ == "array" and isinstance(val, list):
            return
        if typ == "object" and isinstance(val, dict):
            return
    raise AssertionError(f"{path}: expected type {allowed}, got {type(val).__name__} ({val!r})")


def validate_against_schema(instance: Any, schema: dict[str, Any], path: str = "$") -> None:
    if "const" in schema:
        if instance != schema["const"]:
            raise AssertionError(f"{path}: expected const {schema['const']!r}, got {instance!r}")
    if "enum" in schema:
        if instance not in schema["enum"]:
            raise AssertionError(f"{path}: {instance!r} not in enum {schema['enum']!r}")
    if "type" in schema:
        _check_type(instance, schema["type"], path)
    if isinstance(instance, str) and "minLength" in schema:
        if len(instance) < schema["minLength"]:
            raise AssertionError(f"{path}: string length {len(instance)} < {schema['minLength']}")
    if isinstance(instance, int) and not isinstance(instance, bool) and "minimum" in schema:
        if instance < schema["minimum"]:
            raise AssertionError(f"{path}: {instance} < minimum {schema['minimum']}")
    if isinstance(instance, list) and "items" in schema:
        for idx, item in enumerate(instance):
            validate_against_schema(item, schema["items"], f"{path}[{idx}]")
    if isinstance(instance, dict):
        props = schema.get("properties", {})
        for req_key in schema.get("required", []):
            if req_key not in instance:
                raise AssertionError(f"{path}: missing required key {req_key!r}")
        if schema.get("additionalProperties") is False:
            extra = set(instance.keys()) - set(props.keys())
            if extra:
                raise AssertionError(f"{path}: unexpected additionalProperties {sorted(extra)}")
        for k, v in instance.items():
            if k in props:
                validate_against_schema(v, props[k], f"{path}.{k}")


class ProviderSchemaConformanceTest(unittest.TestCase):
    def _find_binary(self) -> Path:
        candidates = [
            REPO_ROOT / "build/t81_provider_contract_test",
            REPO_ROOT / "build/t81_provider_contract_test.exe",
            REPO_ROOT / "build/Release/t81_provider_contract_test.exe",
        ]
        for cand in candidates:
            if cand.exists():
                return cand
        self.fail(f"t81_provider_contract_test binary not found in {[str(c) for c in candidates]}")

    def test_live_serialized_outputs_conform_to_schemas(self) -> None:
        binary = self._find_binary()
        proc = subprocess.run(
            [str(binary), "--dump-json"],
            capture_output=True,
            text=True,
            check=True,
            timeout=30,
        )
        dumped = json.loads(proc.stdout)

        caps_schema = json.loads(CAPS_SCHEMA_PATH.read_text(encoding="utf-8"))
        evid_schema = json.loads(EVID_SCHEMA_PATH.read_text(encoding="utf-8"))

        validate_against_schema(dumped["capabilities"], caps_schema, "$.capabilities")
        validate_against_schema(dumped["evidence"], evid_schema, "$.evidence")

        # Validate draft RFC-00D4 6-primitive projection structure
        adb = dumped["rfc00d4"]
        for req_top in (
            "schema_version",
            "bundle_id",
            "timestamp_utc",
            "meaning",
            "evidence",
            "method",
            "authority",
            "accountability",
            "challengeability",
        ):
            self.assertIn(req_top, adb)
        self.assertEqual(adb["schema_version"], "1.0.0-draft")
        self.assertIn(adb["authority"]["verdict"], ("ALLOW", "DENY", "WARN"))


if __name__ == "__main__":
    unittest.main()
