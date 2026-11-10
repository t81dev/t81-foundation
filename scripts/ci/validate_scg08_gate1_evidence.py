#!/usr/bin/env python3
"""
Validate SCG-08 Gate 1 machine-readable evidence artifact against evidence_schema.json.
Stdlib-only, fail-closed validation.
"""

import json
import os
import sys
from pathlib import Path


def fail(msg: str):
    print(f"[SCG-08 SCHEMA VALIDATION ERROR] {msg}", file=sys.stderr)
    sys.exit(1)


def validate_evidence(evidence_path: Path, schema_path: Path):
    if not evidence_path.is_file():
        fail(f"Evidence file not found: {evidence_path}")
    if not schema_path.is_file():
        fail(f"Schema file not found: {schema_path}")

    try:
        with open(evidence_path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception as e:
        fail(f"Failed to parse evidence JSON: {e}")

    try:
        with open(schema_path, "r", encoding="utf-8") as f:
            schema = json.load(f)
    except Exception as e:
        fail(f"Failed to parse schema JSON: {e}")

    # Top-level required
    for req in schema.get("required", []):
        if req not in data:
            fail(f"Missing required top-level property: {req}")

    if data.get("schema_id") != "t81.scg08.carry-experiment.v1":
        fail(f"Invalid schema_id: {data.get('schema_id')}")

    if data.get("experiment_id") != "EXP-SCG08-01":
        fail(f"Invalid experiment_id: {data.get('experiment_id')}")

    if data.get("status") not in [
        "READY_FOR_INDEPENDENT_GATE1_IMPLEMENTATION_REVIEW",
        "CORRECTIONS_REQUIRED",
        "BLOCKED",
    ]:
        fail(f"Invalid status: {data.get('status')}")

    # git_context
    gc = data.get("git_context", {})
    if not isinstance(gc, dict) or "baseline_sha" not in gc or "head_sha" not in gc:
        fail("Invalid git_context: must contain baseline_sha and head_sha strings")

    # trial_budget
    tb = data.get("trial_budget", {})
    for req in ["max_cumulative_trials_ceiling", "actual_cumulative_trials_executed", "budget_enforced_fail_closed"]:
        if req not in tb:
            fail(f"Missing trial_budget property: {req}")
    if not isinstance(tb["max_cumulative_trials_ceiling"], int):
        fail("max_cumulative_trials_ceiling must be integer")
    if not isinstance(tb["actual_cumulative_trials_executed"], int):
        fail("actual_cumulative_trials_executed must be integer")
    if not isinstance(tb["budget_enforced_fail_closed"], bool) or not tb["budget_enforced_fail_closed"]:
        fail("budget_enforced_fail_closed must be boolean true")
    if tb["actual_cumulative_trials_executed"] > tb["max_cumulative_trials_ceiling"]:
        fail("Trial budget exceeded ceiling")

    # tier1_markov
    t1 = data.get("tier1_markov", {})
    for k in ["ternary", "binary"]:
        if k not in t1:
            fail(f"Missing tier1_markov.{k}")
        sub = t1[k]
        for prop in ["total_digits", "f_gen_given_0", "f_trans", "f_occ", "f_cont_given_act", "mean_run_length"]:
            if prop not in sub:
                fail(f"Missing property {prop} in tier1_markov.{k}")
            if prop == "total_digits" and not isinstance(sub[prop], int):
                fail(f"Property {prop} in tier1_markov.{k} must be int")
            elif prop != "total_digits" and not isinstance(sub[prop], (int, float)):
                fail(f"Property {prop} in tier1_markov.{k} must be float/number")

    # tier2_work
    t2 = data.get("tier2_work", {})
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
        "w_ops_add_ternary",
        "w_ops_mul_ternary",
        "w_ops_add_binary",
        "w_ops_mul_binary",
    ]
    for b in bands:
        for prop in required_band_props:
            if prop not in b:
                fail(f"Missing band property: {prop}")
            if not isinstance(b[prop], int):
                fail(f"Band property {prop} must be integer")

    # oracle_verification
    ov = data.get("oracle_verification", {})
    for prop in ["oracle_type", "dimensions_verified", "all_dimensions_passed"]:
        if prop not in ov:
            fail(f"Missing oracle_verification property: {prop}")
    if not isinstance(ov["oracle_type"], str) or len(ov["oracle_type"]) == 0:
        fail("oracle_type must be non-empty string")
    if not isinstance(ov["dimensions_verified"], list) or len(ov["dimensions_verified"]) != 5:
        fail("dimensions_verified must list all 5 dimensions")
    if ov["all_dimensions_passed"] is not True:
        fail("all_dimensions_passed must be true")

    print(f"[SCG-08 SCHEMA VALIDATION PASSED] Artifact {evidence_path.name} conforms to {schema.get('$id')}")


def main():
    repo_root = Path(__file__).resolve().parent.parent.parent
    evidence_path = repo_root / "experiments" / "scg08_arithmetic_work" / "gate1_evidence_record.json"
    schema_path = repo_root / "experiments" / "scg08_arithmetic_work" / "evidence_schema.json"

    if len(sys.argv) > 1:
        evidence_path = Path(sys.argv[1])
    if len(sys.argv) > 2:
        schema_path = Path(sys.argv[2])

    validate_evidence(evidence_path, schema_path)


if __name__ == "__main__":
    main()
