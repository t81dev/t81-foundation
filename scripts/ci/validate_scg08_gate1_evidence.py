#!/usr/bin/env python3
"""
Validate SCG-08 Gate 1 machine-readable evidence artifact against evidence_schema.json.
Stdlib-only, fail-closed validation.
Includes rigorous negative test mode (--test-negative).
"""

import copy
import json
import math
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from datetime import datetime, timedelta

SHA_REGEX = re.compile(r"^[0-9a-f]{40}$")
ISO8601_REGEX = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$")

EXPECTED_SPECIFICATION_VERSION = "T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06"

# Only the keywords used by this evidence schema, plus boolean extra-property
# admission, are supported. Unknown constraints must never be silently ignored.
SUPPORTED_SCHEMA_KEYWORDS = {
    "$schema", "$id", "title", "description", "type", "enum", "required",
    "properties", "items", "pattern", "additionalProperties",
}


def fail(msg: str):
    print(f"[SCG-08 SCHEMA VALIDATION ERROR] {msg}", file=sys.stderr)
    sys.exit(1)


def compute_expected_w_ops(counters: dict) -> int:
    return (
        counters["op_alu_add_sub"]
        + (3 * counters["op_alu_mul"])
        + counters["op_table_lookup"]
        + counters["op_branch_cond"]
        + counters["op_norm_pass"]
    )


def reject_json_constant(value):
    raise ValueError(f"Nonfinite JSON constant: {value}")


def parse_finite_float(value):
    result = float(value)
    if not math.isfinite(result):
        raise ValueError(f"JSON number outside finite float range: {value}")
    return result


def load_json(stream):
    # Python otherwise accepts NaN/Infinity tokens, including in extra fields
    # that the schema intentionally permits without interpreting their contents.
    return json.load(stream, parse_constant=reject_json_constant, parse_float=parse_finite_float)


def validate_evidence(evidence_path: Path, schema_path: Path):
    if not evidence_path.is_file():
        fail(f"Evidence file not found: {evidence_path}")
    if not schema_path.is_file():
        fail(f"Schema file not found: {schema_path}")

    try:
        with open(evidence_path, "r", encoding="utf-8") as f:
            data = load_json(f)
    except Exception as e:
        fail(f"Failed to parse evidence JSON: {e}")

    try:
        with open(schema_path, "r", encoding="utf-8") as f:
            schema = load_json(f)
    except Exception as e:
        fail(f"Failed to parse schema JSON: {e}")

    validate_dict(data, schema)
    print(f"[SCG-08 SCHEMA VALIDATION PASSED] Artifact {evidence_path.name} passes supported schema checks and domain rules for {schema.get('$id')}")


def validate_schema_shape(value, schema: dict, path: str = "evidence"):
    """Check this schema's types, string enums, required/properties/items,
    ASCII-compatible regex patterns and boolean additionalProperties.

    Domain constraints below remain additional checks; this is not a general
    JSON Schema implementation or general ECMAScript regex engine. Integer
    counters require Python ints (not bools or integral floats).
    """
    if not isinstance(schema, dict):
        fail(f"{path}: schema must be an object")
    unsupported = set(schema) - SUPPORTED_SCHEMA_KEYWORDS
    if unsupported:
        fail(f"{path}: unsupported schema keywords: {sorted(unsupported)}")
    if "additionalProperties" in schema and type(schema["additionalProperties"]) is not bool:
        fail(f"{path}: only boolean additionalProperties is supported")
    if "enum" in schema and (
        not isinstance(schema["enum"], list)
        or not all(isinstance(option, str) for option in schema["enum"])
    ):
        fail(f"{path}: only string enum values are supported")
    expected = schema.get("type")
    valid_types = {
        "object": isinstance(value, dict),
        "array": isinstance(value, list),
        "string": isinstance(value, str),
        "integer": type(value) is int,
        "number": type(value) in (int, float),
        "boolean": type(value) is bool,
    }
    if expected is not None and not valid_types.get(expected, False):
        fail(f"{path} must be {expected}")
    if type(value) is float and not math.isfinite(value):
        fail(f"{path} must be finite")
    if "enum" in schema and value not in schema["enum"]:
        fail(f"{path} is outside the schema enum")
    if "pattern" in schema and isinstance(value, str):
        try:
            matches = re.search(schema["pattern"], value)
        except (TypeError, re.error) as exc:
            fail(f"{path}: invalid supported regex pattern: {exc}")
        if matches is None:
            fail(f"{path} does not match the schema pattern")
    if isinstance(value, dict):
        for key in schema.get("required", []):
            if key not in value:
                fail(f"Missing required property: {path}.{key}")
        properties = schema.get("properties", {})
        for key, item in value.items():
            if key in properties:
                validate_schema_shape(item, properties[key], f"{path}.{key}")
            elif schema.get("additionalProperties") is False:
                fail(f"Unexpected property: {path}.{key}")
    elif isinstance(value, list) and "items" in schema:
        for idx, item in enumerate(value):
            validate_schema_shape(item, schema["items"], f"{path}[{idx}]")


def validate_dict(data: dict, schema: dict):
    validate_schema_shape(data, schema)
    if not isinstance(data, dict):
        fail("Top-level evidence artifact must be a JSON object")

    # Top-level required properties
    for req in schema.get("required", []):
        if req not in data:
            fail(f"Missing required top-level property: {req}")

    if data.get("schema_id") != "t81.scg08.carry-experiment.v1":
        fail(f"Invalid schema_id: {data.get('schema_id')}")

    if data.get("experiment_id") != "EXP-SCG08-01":
        fail(f"Invalid experiment_id: {data.get('experiment_id')}")

    if data.get("specification_version") != EXPECTED_SPECIFICATION_VERSION:
        fail(f"Invalid specification_version: {data.get('specification_version')} (expected {EXPECTED_SPECIFICATION_VERSION})")

    if data.get("status") not in [
        "READY_FOR_INDEPENDENT_GATE1_IMPLEMENTATION_REVIEW",
        "CORRECTIONS_REQUIRED",
        "BLOCKED",
    ]:
        fail(f"Invalid status: {data.get('status')}")

    # timestamp_utc validation
    ts = data.get("timestamp_utc")
    if not isinstance(ts, str) or not ISO8601_REGEX.fullmatch(ts):
        fail(f"Invalid timestamp_utc format: {ts} (must be valid ISO-8601 UTC timestamp)")
    try:
        parsed_ts = datetime.fromisoformat(ts.replace("Z", "+00:00"))
    except ValueError:
        fail(f"Invalid timestamp_utc calendar value: {ts}")
    if parsed_ts.utcoffset() != timedelta(0):
        fail("timestamp_utc must have UTC offset zero")

    # git_context
    gc = data.get("git_context")
    if not isinstance(gc, dict):
        fail("Invalid git_context: must be an object")
    required_git_props = ["baseline_sha", "evaluated_implementation_sha"]
    for req_prop in required_git_props:
        if req_prop not in gc:
            fail(f"Invalid git_context: missing required property {req_prop}")
    for sha_key in required_git_props:
        sha_val = gc[sha_key]
        if not isinstance(sha_val, str) or not SHA_REGEX.fullmatch(sha_val):
            fail(f"Invalid {sha_key}: {sha_val} (must be non-null 40-character hexadecimal SHA)")
    if "evidence_publication_sha" in gc:
        pub_sha = gc["evidence_publication_sha"]
        if not isinstance(pub_sha, str) or not SHA_REGEX.fullmatch(pub_sha):
            fail(f"Invalid evidence_publication_sha: {pub_sha!r} (must be a non-null 40-character hexadecimal SHA)")

    # trial_budget
    tb = data.get("trial_budget")
    if not isinstance(tb, dict):
        fail("trial_budget must be an object")
    for req in ["max_cumulative_trials_ceiling", "actual_cumulative_trials_executed", "budget_enforced_fail_closed"]:
        if req not in tb:
            fail(f"Missing trial_budget property: {req}")
    if type(tb["max_cumulative_trials_ceiling"]) is not int or tb["max_cumulative_trials_ceiling"] <= 0:
        fail("max_cumulative_trials_ceiling must be a positive integer")
    if type(tb["actual_cumulative_trials_executed"]) is not int or tb["actual_cumulative_trials_executed"] < 0:
        fail("actual_cumulative_trials_executed must be a non-negative integer")
    if type(tb["budget_enforced_fail_closed"]) is not bool or not tb["budget_enforced_fail_closed"]:
        fail("budget_enforced_fail_closed must be boolean true")
    if tb["actual_cumulative_trials_executed"] > tb["max_cumulative_trials_ceiling"]:
        fail(f"Actual executed trials ({tb['actual_cumulative_trials_executed']}) exceeded budget ceiling ({tb['max_cumulative_trials_ceiling']})")

    # tier1_markov
    t1 = data.get("tier1_markov")
    if not isinstance(t1, dict):
        fail("tier1_markov must be an object")
    for k in ["ternary", "binary"]:
        if k not in t1:
            fail(f"Missing tier1_markov.{k}")
        sub = t1[k]
        if not isinstance(sub, dict):
            fail(f"tier1_markov.{k} must be an object")
        for prop in ["total_digits", "f_gen_given_0", "f_trans", "f_occ", "f_cont_given_act", "mean_run_length"]:
            if prop not in sub:
                fail(f"Missing property {prop} in tier1_markov.{k}")
            val = sub[prop]
            if prop == "total_digits":
                if type(val) is not int or val <= 0:
                    fail(f"Property {prop} in tier1_markov.{k} must be positive int")
            else:
                if not isinstance(val, (int, float)) or type(val) is bool:
                    fail(f"Property {prop} in tier1_markov.{k} must be float/number")
                if val < 0.0:
                    fail(f"Property {prop} in tier1_markov.{k} cannot be negative")

    # tier2_work
    t2 = data.get("tier2_work")
    if not isinstance(t2, dict):
        fail("tier2_work must be an object")
    bands = t2.get("evaluated_bands")
    if not isinstance(bands, list) or len(bands) != 5:
        fail("tier2_work.evaluated_bands must be a list of 5 evaluated bands")

    required_band_props = [
        "bit_width",
        "operand_trits",
        "add_trits_min",
        "mul_trits_min",
        "ternary_limbs_op",
        "ternary_limbs_add",
        "ternary_limbs_mul",
        "counters_add_ternary",
        "counters_mul_ternary",
        "counters_add_binary",
        "counters_mul_binary",
        "w_ops_add_ternary",
        "w_ops_mul_ternary",
        "w_ops_add_binary",
        "w_ops_mul_binary",
    ]
    expected_bit_widths = [64, 128, 256, 512, 1024]
    seen_bit_widths = []

    for idx, b in enumerate(bands):
        if not isinstance(b, dict):
            fail(f"evaluated_bands element {idx} must be an object")
        for prop in required_band_props:
            if prop not in b:
                fail(f"Missing band property: {prop} in band {idx}")

        # Enforce strict integer type on scalar properties (rejecting bools, nulls, strings, floats)
        scalar_props = [
            "bit_width",
            "operand_trits",
            "add_trits_min",
            "mul_trits_min",
            "ternary_limbs_op",
            "ternary_limbs_add",
            "ternary_limbs_mul",
            "w_ops_add_ternary",
            "w_ops_mul_ternary",
            "w_ops_add_binary",
            "w_ops_mul_binary",
        ]
        for prop in scalar_props:
            val = b[prop]
            if type(val) is not int or val < 0:
                fail(f"Band {b.get('bit_width')} property {prop} must be a non-negative integer (found {type(val).__name__}: {val!r})")

        # Validate primitive counter objects and reconcile with weighted total W_ops
        counter_mappings = [
            ("counters_add_ternary", "w_ops_add_ternary"),
            ("counters_mul_ternary", "w_ops_mul_ternary"),
            ("counters_add_binary", "w_ops_add_binary"),
            ("counters_mul_binary", "w_ops_mul_binary"),
        ]
        for c_key, w_key in counter_mappings:
            c_obj = b[c_key]
            if not isinstance(c_obj, dict):
                fail(f"Band {b.get('bit_width')} {c_key} must be an object")
            for prim in ["op_alu_add_sub", "op_alu_mul", "op_table_lookup", "op_branch_cond", "op_norm_pass"]:
                if prim not in c_obj:
                    fail(f"Missing primitive counter {prim} in {c_key}")
                if type(c_obj[prim]) is not int or c_obj[prim] < 0:
                    fail(f"Primitive counter {prim} in {c_key} must be a non-negative integer")
            expected_w = compute_expected_w_ops(c_obj)
            actual_w = b[w_key]
            if expected_w != actual_w:
                fail(f"Band {b.get('bit_width')} weighted total {w_key} ({actual_w}) does not match measured counters formula ({expected_w})")

        seen_bit_widths.append(b["bit_width"])

    if seen_bit_widths != expected_bit_widths:
        fail(f"evaluated_bands must cover bit widths {expected_bit_widths}, found {seen_bit_widths}")

    # oracle_verification
    ov = data.get("oracle_verification")
    if not isinstance(ov, dict):
        fail("oracle_verification must be an object")
    for prop in ["oracle_type", "dimensions_verified", "all_dimensions_passed"]:
        if prop not in ov:
            fail(f"Missing oracle_verification property: {prop}")
    if not isinstance(ov["oracle_type"], str) or len(ov["oracle_type"]) == 0:
        fail("oracle_type must be a non-empty string")
    if not isinstance(ov["dimensions_verified"], list) or len(ov["dimensions_verified"]) != 5:
        fail("dimensions_verified must list all 5 dimensions")
    for d in ov["dimensions_verified"]:
        if not isinstance(d, str) or len(d) == 0:
            fail("Each dimension in dimensions_verified must be a non-empty string")
    if ov["all_dimensions_passed"] is not True:
        fail("all_dimensions_passed must be boolean true")


def run_negative_tests(schema_path: Path):
    """Verify that malformed records, null SHAs, and invalid types fail closed."""
    print("[SCG-08 NEGATIVE TEST SUITE] Running negative schema tests...")

    base_valid = {
        "schema_id": "t81.scg08.carry-experiment.v1",
        "experiment_id": "EXP-SCG08-01",
        "specification_version": EXPECTED_SPECIFICATION_VERSION,
        "status": "READY_FOR_INDEPENDENT_GATE1_IMPLEMENTATION_REVIEW",
        "timestamp_utc": "2026-11-10T22:30:00Z",
        "git_context": {
            "baseline_sha": "dbf107fda83a3755fa21d6b95caa3eb8ac450dd3",
            "evaluated_implementation_sha": "2dd3cc62e526942cdcbed6d100770c08c26ab2fe",
        },
        "trial_budget": {
            "max_cumulative_trials_ceiling": 1000,
            "actual_cumulative_trials_executed": 179,
            "budget_enforced_fail_closed": True,
        },
        "tier1_markov": {
            "ternary": {
                "total_digits": 100,
                "f_gen_given_0": 0.24,
                "f_trans": 0.18,
                "f_occ": 0.29,
                "f_cont_given_act": 0.42,
                "mean_run_length": 1.61,
            },
            "binary": {
                "total_digits": 100,
                "f_gen_given_0": 0.27,
                "f_trans": 0.15,
                "f_occ": 0.47,
                "f_cont_given_act": 0.71,
                "mean_run_length": 3.13,
            },
        },
        "tier2_work": {
            "evaluated_bands": [
                {
                    "bit_width": bw,
                    "operand_trits": 41,
                    "add_trits_min": 42,
                    "mul_trits_min": 81,
                    "ternary_limbs_op": 2,
                    "ternary_limbs_add": 2,
                    "ternary_limbs_mul": 3,
                    "counters_add_ternary": {
                        "op_alu_add_sub": 2, "op_alu_mul": 0, "op_table_lookup": 76, "op_branch_cond": 2, "op_norm_pass": 0
                    },
                    "counters_mul_ternary": {
                        "op_alu_add_sub": 841, "op_alu_mul": 841, "op_table_lookup": 0, "op_branch_cond": 2461, "op_norm_pass": 1
                    },
                    "counters_add_binary": {
                        "op_alu_add_sub": 2, "op_alu_mul": 0, "op_table_lookup": 0, "op_branch_cond": 2, "op_norm_pass": 0
                    },
                    "counters_mul_binary": {
                        "op_alu_add_sub": 9, "op_alu_mul": 4, "op_table_lookup": 0, "op_branch_cond": 8, "op_norm_pass": 0
                    },
                    "w_ops_add_ternary": 80,
                    "w_ops_mul_ternary": 5826,
                    "w_ops_add_binary": 4,
                    "w_ops_mul_binary": 29,
                }
                for bw in [64, 128, 256, 512, 1024]
            ]
        },
        "oracle_verification": {
            "oracle_type": "ExactBigIntOracle",
            "dimensions_verified": ["dim1", "dim2", "dim3", "dim4", "dim5"],
            "all_dimensions_passed": True,
        },
    }

    negative_cases = [
        ("null evaluated_implementation_sha", lambda d: d["git_context"].__setitem__("evaluated_implementation_sha", None)),
        ("invalid short evaluated_implementation_sha", lambda d: d["git_context"].__setitem__("evaluated_implementation_sha", "abc123")),
        ("null evidence_publication_sha", lambda d: d["git_context"].__setitem__("evidence_publication_sha", None)),
        ("non-string evidence_publication_sha", lambda d: d["git_context"].__setitem__("evidence_publication_sha", 12345)),
        ("invalid publication sha format", lambda d: d["git_context"].__setitem__("evidence_publication_sha", "invalid-sha")),
        ("malformed timestamp", lambda d: d.__setitem__("timestamp_utc", "invalid-time")),
        ("wrong specification version", lambda d: d.__setitem__("specification_version", "v1.0")),
        ("budget ceiling exceeded", lambda d: d["trial_budget"].__setitem__("actual_cumulative_trials_executed", 1001)),
        ("budget boolean False", lambda d: d["trial_budget"].__setitem__("budget_enforced_fail_closed", False)),
        ("missing tier2 band", lambda d: d["tier2_work"]["evaluated_bands"].pop()),
        ("mismatched weighted total W_ops", lambda d: d["tier2_work"]["evaluated_bands"][0].__setitem__("w_ops_add_ternary", 999)),
        ("missing primitive counter", lambda d: d["tier2_work"]["evaluated_bands"][0]["counters_add_ternary"].pop("op_alu_add_sub")),
        ("boolean instead of number", lambda d: d["tier1_markov"]["ternary"].__setitem__("f_occ", True)),
        ("oracle passed False", lambda d: d["oracle_verification"].__setitem__("all_dimensions_passed", False)),
        ("oracle dimensions missing", lambda d: d["oracle_verification"].__setitem__("dimensions_verified", ["dim1"])),
        ("operand_trits is null", lambda d: d["tier2_work"]["evaluated_bands"][0].__setitem__("operand_trits", None)),
        ("ternary_limbs_op is not integer", lambda d: d["tier2_work"]["evaluated_bands"][0].__setitem__("ternary_limbs_op", "not an integer")),
        ("bit_width boolean True", lambda d: d["tier2_work"]["evaluated_bands"][0].__setitem__("bit_width", True)),
        ("operand_trits float", lambda d: d["tier2_work"]["evaluated_bands"][0].__setitem__("operand_trits", 41.5)),
        ("negative operand_trits", lambda d: d["tier2_work"]["evaluated_bands"][0].__setitem__("operand_trits", -1)),
        ("invalid calendar timestamp", lambda d: d.__setitem__("timestamp_utc", "2026-99-99T25:61:61Z")),
        ("non-UTC timestamp", lambda d: d.__setitem__("timestamp_utc", "2026-11-10T22:30:00+01:00")),
        ("SHA trailing newline", lambda d: d["git_context"].__setitem__("evaluated_implementation_sha", "a" * 40 + "\n")),
        ("nonfinite metric", lambda d: d["tier1_markov"]["ternary"].__setitem__("f_occ", float("nan"))),
        ("negative counter", lambda d: d["tier2_work"]["evaluated_bands"][0]["counters_add_binary"].__setitem__("op_alu_mul", -1)),
        ("boolean counter", lambda d: d["tier2_work"]["evaluated_bands"][0]["counters_add_binary"].__setitem__("op_alu_mul", False)),
        ("nested counter", lambda d: d["tier2_work"]["evaluated_bands"][0]["counters_add_binary"].__setitem__("op_alu_mul", {})),
        ("publication SHA trailing newline", lambda d: d["git_context"].__setitem__("evidence_publication_sha", "a" * 40 + "\n")),
        ("nonfinite infinite metric", lambda d: d["tier1_markov"]["ternary"].__setitem__("f_occ", float("inf"))),
    ]

    with open(schema_path, "r", encoding="utf-8") as f:
        schema = load_json(f)

    # First verify base_valid passes
    validate_dict(base_valid, schema)

    for desc, mutator in negative_cases:
        test_doc = copy.deepcopy(base_valid)
        mutator(test_doc)
        failed_as_expected = False
        try:
            validate_dict(test_doc, schema)
        except SystemExit as exc:
            failed_as_expected = exc.code == 1

        if not failed_as_expected:
            print(f"[NEGATIVE TEST FAILED] Case '{desc}' unexpectedly passed validation!", file=sys.stderr)
            sys.exit(1)
        print(f"  ✓ Case '{desc}' properly rejected fail-closed")

    print("[SCG-08 NEGATIVE TEST SUITE] All negative test cases passed successfully.")


def run_json_file_tests(evidence_path: Path, schema_path: Path):
    """Exercise file parsing and CLI rejection separately from dict fixtures."""
    valid_text = evidence_path.read_text(encoding="utf-8")
    valid = json.loads(valid_text)
    nonfinite = copy.deepcopy(valid)
    nonfinite["tier1_markov"]["ternary"]["f_occ"] = float("nan")
    nested = copy.deepcopy(valid)
    nested["tier2_work"]["evaluated_bands"][0]["counters_add_binary"]["op_alu_mul"] = {}
    cases = [
        ("valid", valid_text.encode(), 0),
        ("truncated", b'{"schema_id":', 1),
        ("trailing data", valid_text.encode() + b'{}', 1),
        ("invalid UTF-8", b'\xff', 1),
        ("top-level null", b'null', 1),
        ("top-level array", b'[]', 1),
        ("missing properties", b'{}', 1),
        ("nested scalar object", json.dumps(nested).encode(), 1),
        ("NaN declared field", json.dumps(nonfinite).encode(), 1),
    ]
    for token in ("NaN", "Infinity", "-Infinity", "1e999"):
        # Even uninterpreted extra properties must contain parseable finite JSON.
        text = valid_text.rstrip()[:-1] + ', "extra": ' + token + '}'
        cases.append((f"nonfinite extra field {token}", text.encode(), 1))
    extra = copy.deepcopy(valid)
    extra["extra"] = {"nested": [1, "permitted"]}
    cases.append(("permitted finite extras", json.dumps(extra).encode(), 0))
    schema_text = schema_path.read_text(encoding="utf-8")
    unsupported = json.loads(schema_text)
    unsupported["minimum"] = 0
    schema_cases = [
        ("truncated schema", b'{"type":', 1),
        ("invalid UTF-8 schema", b'\xff', 1),
        ("null schema", b'null', 1),
        ("array schema", b'[]', 1),
        ("unsupported schema constraint", json.dumps(unsupported).encode(), 1),
    ]
    file_cases = [(label, payload, schema_text.encode(), expected)
                  for label, payload, expected in cases]
    file_cases += [(label, valid_text.encode(), payload, expected)
                   for label, payload, expected in schema_cases]
    with tempfile.TemporaryDirectory(prefix="scg08-json-files-") as scratch:
        path = Path(scratch) / "evidence.json"
        schema_file = Path(scratch) / "schema.json"
        for label, payload, schema_payload, expected in file_cases:
            path.write_bytes(payload)
            schema_file.write_bytes(schema_payload)
            command = [sys.executable]
            if sys.flags.optimize:
                command.append("-O")
            command += [str(Path(__file__).resolve()), str(path), str(schema_file)]
            result = subprocess.run(command, capture_output=True, text=True, timeout=10)
            if result.returncode != expected or "Traceback" in result.stderr:
                fail(f"JSON-file case {label}: exit {result.returncode}, expected {expected}: {result.stderr}")
            if expected and (
                "[SCG-08 SCHEMA VALIDATION ERROR]" not in result.stderr
                or "[SCG-08 SCHEMA VALIDATION PASSED]" in result.stdout
            ):
                fail(f"JSON-file case {label} did not use the intended rejection path")
            print(f"  JSON-file case '{label}' passed (exit {expected})")
    print(f"[SCG-08 JSON FILE TEST SUITE] All {len(file_cases)} cases passed.")


def main():
    repo_root = Path(__file__).resolve().parent.parent.parent
    evidence_path = repo_root / "experiments" / "scg08_arithmetic_work" / "gate1_evidence_record.json"
    schema_path = repo_root / "experiments" / "scg08_arithmetic_work" / "evidence_schema.json"

    if "--test-negative" in sys.argv:
        run_negative_tests(schema_path)
        return
    if "--test-json-files" in sys.argv:
        run_json_file_tests(evidence_path, schema_path)
        return

    if len(sys.argv) > 1 and not sys.argv[1].startswith("--"):
        evidence_path = Path(sys.argv[1])
    if len(sys.argv) > 2 and not sys.argv[2].startswith("--"):
        schema_path = Path(sys.argv[2])

    validate_evidence(evidence_path, schema_path)


if __name__ == "__main__":
    main()
