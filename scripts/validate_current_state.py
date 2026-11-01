#!/usr/bin/env python3
"""Validate the checked-in state with the schema's supported stdlib-only subset.

This checks structure, not source authenticity, freshness, or authorization.
"""
import json
import re
import sys
from datetime import date
from pathlib import Path


def require(condition, message):
    if not condition:
        raise ValueError(message)


def json_equal(value, expected):
    # JSON booleans and numbers are distinct, unlike Python's False == 0.
    if isinstance(value, bool) != isinstance(expected, bool):
        return False
    return value == expected


def validate(value, rule, path="$"):
    supported = {"$schema", "title", "description", "type", "const", "enum",
                 "required", "additionalProperties", "properties", "items",
                 "pattern", "format", "minimum", "maxLength"}
    require(not (rule.keys() - supported), f"{path}: unsupported schema keywords")
    if "const" in rule:
        require(json_equal(value, rule["const"]),
                f"{path}: expected constant {rule['const']!r}")
    if "enum" in rule:
        require(any(json_equal(value, item) for item in rule["enum"]),
                f"{path}: invalid enum")
    typ = rule.get("type")
    if typ == "object":
        require(isinstance(value, dict), f"{path}: expected object")
        require(set(rule.get("required", [])) <= value.keys(),
                f"{path}: missing required keys")
        if rule.get("additionalProperties") is False:
            require(not (value.keys() - rule.get("properties", {}).keys()),
                    f"{path}: unexpected keys")
        for key, val in value.items():
            if key in rule.get("properties", {}):
                validate(val, rule["properties"][key], path + "." + key)
    elif typ == "array":
        require(isinstance(value, list), f"{path}: expected array")
        for i, val in enumerate(value):
            validate(val, rule["items"], f"{path}[{i}]")
    elif typ == "string":
        require(isinstance(value, str), f"{path}: expected string")
        if "pattern" in rule:
            require(re.search(rule["pattern"], value) is not None,
                    f"{path}: pattern mismatch")
        if "maxLength" in rule:
            require(len(value) <= rule["maxLength"], f"{path}: string too long")
        if "format" in rule:
            require(rule["format"] == "date", f"{path}: unsupported format")
            require(re.fullmatch(r"[0-9]{4}-[0-9]{2}-[0-9]{2}", value) is not None,
                    f"{path}: expected YYYY-MM-DD date")
            try:
                date.fromisoformat(value)
            except ValueError as exc:
                raise ValueError(f"{path}: invalid calendar date") from exc
    elif typ == "integer":
        require(type(value) is int or (type(value) is float and value.is_integer()),
                f"{path}: expected integer")
        require(value >= rule.get("minimum", value), f"{path}: below minimum")
    else:
        require(typ is None, f"{path}: unsupported schema type {typ!r}")


def reject_constant(value):
    raise ValueError(f"non-JSON constant: {value}")


def main():
    root = Path(__file__).resolve().parents[1]
    try:
        state = json.loads((root / "docs/ai-context/CURRENT_STATE.json").read_text(
            encoding="utf-8"), parse_constant=reject_constant)
        schema = json.loads((root / "docs/ai-context/CURRENT_STATE.schema.json").read_text(
            encoding="utf-8"), parse_constant=reject_constant)
        validate(state, schema)
    except (OSError, ValueError) as exc:
        print(f"CURRENT_STATE: invalid: {exc}", file=sys.stderr)
        return 1
    print("CURRENT_STATE: structure valid; provenance and freshness require live verification")
    return 0


if __name__ == "__main__":
    sys.exit(main())
