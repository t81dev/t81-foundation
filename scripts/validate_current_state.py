#!/usr/bin/env python3
"""Validate T81 non-normative operational state using Python standard library."""
import json
import re
from datetime import date
from pathlib import Path

root = Path(__file__).resolve().parents[1]
state = json.loads((root / "docs/ai-context/CURRENT_STATE.json").read_text())
schema = json.loads((root / "docs/ai-context/CURRENT_STATE.schema.json").read_text())

def validate(value, rule, path="$"):
    if "const" in rule:
        assert value == rule["const"], f"{path}: expected constant {rule['const']!r}"
    if "enum" in rule:
        assert value in rule["enum"], f"{path}: invalid enum"
    typ = rule.get("type")
    if typ == "object":
        assert isinstance(value, dict), f"{path}: expected object"
        assert set(rule.get("required", [])) <= value.keys(), f"{path}: missing required keys"
        if rule.get("additionalProperties") is False:
            assert not (value.keys() - rule.get("properties", {}).keys()), f"{path}: unexpected keys"
        for key, val in value.items():
            if key in rule.get("properties", {}):
                validate(val, rule["properties"][key], path + "." + key)
    elif typ == "array":
        assert isinstance(value, list), f"{path}: expected array"
        for i, val in enumerate(value):
            validate(val, rule["items"], f"{path}[{i}]")
    elif typ == "string":
        assert isinstance(value, str), f"{path}: expected string"
        if "pattern" in rule:
            assert re.fullmatch(rule["pattern"], value), f"{path}: pattern mismatch"
        if rule.get("format") == "date":
            date.fromisoformat(value)
    elif typ == "integer":
        assert type(value) is int, f"{path}: expected integer"
        assert value >= rule.get("minimum", value), f"{path}: below minimum"

validate(state, schema)
assert state["purpose"].startswith("Non-normative"), "state must not grant authority"
print("CURRENT_STATE: structure valid; provenance and freshness require live verification")
