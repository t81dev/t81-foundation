#!/usr/bin/env python3
"""Validate the checked-in state with the schema's supported stdlib-only subset.

This checks structure, not source authenticity, freshness, or authorization.
"""
import json
import re
import sys
from datetime import date, datetime
from pathlib import Path


def require(condition, message):
    if not condition:
        raise ValueError(message)


def json_equal(value, expected):
    # JSON booleans and numbers are distinct, unlike Python's False == 0.
    if isinstance(value, bool) != isinstance(expected, bool):
        return False
    return value == expected


def resolve_ref(ref, schema):
    require(isinstance(ref, str) and ref.startswith("#/$defs/"), f"unsupported $ref: {ref}")
    target_name = ref[len("#/$defs/"):]
    defs = schema.get("$defs", {})
    require(target_name in defs, f"unresolved $ref: {ref}")
    return defs[target_name]


def validate_source_revision(val, path="$"):
    """Enforce exact mutual exclusivity on source_revision_field:

    Must be an object with exactly one of:
      1. commit_sha (40 lowercase hex chars)
      2. inherit_from (non-empty string)
      3. value == None and non-empty reason string
    """
    require(isinstance(val, dict), f"{path}: expected object")
    require(len(val) > 0, f"{path}: empty source_revision is forbidden")

    has_commit = "commit_sha" in val
    has_inherit = "inherit_from" in val
    has_null_val = "value" in val
    has_reason = "reason" in val

    # Reason cannot exist alone
    if has_reason and not has_null_val:
        raise ValueError(f"{path}: reason-only source_revision is forbidden")

    # Mutual exclusivity between alternatives
    alternatives = [has_commit, has_inherit, has_null_val]
    require(sum(bool(a) for a in alternatives) == 1,
            f"{path}: source_revision must have exactly one valid alternative")

    if has_commit:
        require(set(val.keys()) == {"commit_sha"}, f"{path}: unexpected keys in commit_sha alternative")
        cs = val["commit_sha"]
        require(isinstance(cs, str) and re.fullmatch(r"^[0-9a-f]{40}$", cs) is not None,
                f"{path}: invalid commit_sha format")
    elif has_inherit:
        require(set(val.keys()) == {"inherit_from"}, f"{path}: unexpected keys in inherit_from alternative")
        ih = val["inherit_from"]
        require(isinstance(ih, str) and len(ih) > 0, f"{path}: empty inherit_from string")
    elif has_null_val:
        require(set(val.keys()) == {"value", "reason"}, f"{path}: unavailable source_revision must have value and reason only")
        require(val["value"] is None, f"{path}: expected null value for unavailable source_revision")
        rn = val["reason"]
        require(isinstance(rn, str) and len(rn.strip()) > 0, f"{path}: nonempty reason required")


def validate(value, rule, schema=None, path="$"):
    if schema is None:
        schema = rule

    if "$ref" in rule:
        resolved = resolve_ref(rule["$ref"], schema)
        validate(value, resolved, schema, path)
        return

    # Check if this rule is resolving source_revision_field
    if path.endswith(".source_revision") or path.endswith(".merge_sha"):
        validate_source_revision(value, path)
        return

    supported = {"$schema", "$defs", "$ref", "title", "description", "type", "const", "enum",
                 "required", "dependentRequired", "additionalProperties", "properties", "items",
                 "pattern", "format", "minimum", "maxLength", "minLength"}
    require(not (rule.keys() - supported), f"{path}: unsupported schema keywords")

    if "const" in rule:
        require(json_equal(value, rule["const"]),
                f"{path}: expected constant {rule['const']!r}")
    if "enum" in rule:
        require(any(json_equal(value, item) for item in rule["enum"]),
                f"{path}: invalid enum")

    typ = rule.get("type")
    if typ == "null":
        require(value is None, f"{path}: expected null")
    elif typ == "object":
        require(isinstance(value, dict), f"{path}: expected object")
        require(set(rule.get("required", [])) <= value.keys(),
                f"{path}: missing required keys")
        if "dependentRequired" in rule:
            for dep_key, req_keys in rule["dependentRequired"].items():
                if dep_key in value:
                    require(set(req_keys) <= value.keys(),
                            f"{path}: missing dependent required keys for {dep_key}")
        add_props = rule.get("additionalProperties", True)
        if add_props is False:
            require(not (value.keys() - rule.get("properties", {}).keys()),
                    f"{path}: unexpected keys")
        elif isinstance(add_props, dict):
            for k, v in value.items():
                if k not in rule.get("properties", {}):
                    validate(v, add_props, schema, f"{path}.{k}")
        for key, val in value.items():
            if key in rule.get("properties", {}):
                validate(val, rule["properties"][key], schema, path + "." + key)
    elif typ == "array":
        require(isinstance(value, list), f"{path}: expected array")
        if "items" in rule:
            for i, val in enumerate(value):
                validate(val, rule["items"], schema, f"{path}[{i}]")
    elif typ == "string":
        require(isinstance(value, str), f"{path}: expected string")
        if "pattern" in rule:
            require(re.search(rule["pattern"], value) is not None,
                    f"{path}: pattern mismatch")
        if "maxLength" in rule:
            require(len(value) <= rule["maxLength"], f"{path}: string too long")
        if "minLength" in rule:
            require(len(value) >= rule["minLength"], f"{path}: string too short")
        if "format" in rule:
            fmt = rule["format"]
            if fmt == "date":
                require(re.fullmatch(r"[0-9]{4}-[0-9]{2}-[0-9]{2}", value) is not None,
                        f"{path}: expected YYYY-MM-DD date")
                try:
                    date.fromisoformat(value)
                except ValueError as exc:
                    raise ValueError(f"{path}: invalid calendar date") from exc
            elif fmt == "date-time":
                # RFC 3339 / ISO 8601 with mandatory timezone offset
                require(re.fullmatch(r"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}(?:\.[0-9]+)?(?:Z|[+-][0-9]{2}:[0-9]{2})", value) is not None,
                        f"{path}: expected RFC 3339 date-time with timezone")
                try:
                    dt = datetime.fromisoformat(value.replace("Z", "+00:00"))
                    require(dt.tzinfo is not None, f"{path}: missing timezone")
                except ValueError as exc:
                    raise ValueError(f"{path}: invalid calendar date-time") from exc
            else:
                raise ValueError(f"{path}: unsupported format {fmt!r}")
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
