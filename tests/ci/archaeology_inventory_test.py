#!/usr/bin/env python3
"""Regression tests for repository archaeology inventory and classification.

Verifies:
1. Investigation 10A disposition is preserved as inconclusive / evaluation blocked
   due to unverified literature data, rather than claiming physical silicon falsification.
2. H04-R02 claim preserves conditional limitations and audit reclassification.
3. Distinction between test assertion files and executed evidence runs.
4. RFC authority classification derives from authentic Status metadata rather than
   filename assumptions (e.g., verifying RFC-00C8 is recognized as Accepted / NORMATIVE_CONTRACT).
"""

from __future__ import annotations

import json
from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[2]


def test_investigation_10a_disposition() -> None:
    claims_path = REPO_ROOT / "docs/records/inventories/repository-archaeology/CLAIMS.jsonl"
    assert claims_path.exists(), f"Missing {claims_path}"

    clm_004 = None
    with open(claims_path, "r", encoding="utf-8") as f:
        for line in f:
            rec = json.loads(line)
            if rec.get("claim_id") == "CLM-004":
                clm_004 = rec
                break

    assert clm_004 is not None, "CLM-004 not found in CLAIMS.jsonl"
    verdict = clm_004["verdict"]
    contradiction = clm_004["contradiction_record"]

    # Verify that Investigation 10A is recognized as inconclusive / evaluation blocked,
    # not a false claim of experimental silicon falsification.
    assert "FALSIFIED_NO_GO" not in verdict, f"CLM-004 verdict improperly asserts falsification: {verdict}"
    assert "INCONCLUSIVE" in verdict or "BLOCKED" in verdict, f"CLM-004 verdict should reflect inconclusive/blocked status: {verdict}"
    assert "unverified" in contradiction.lower() or "blocked" in contradiction.lower(), contradiction


def test_h04_r02_limitations_and_audit_reclassification() -> None:
    claims_path = REPO_ROOT / "docs/records/inventories/repository-archaeology/CLAIMS.jsonl"
    clm_003 = None
    with open(claims_path, "r", encoding="utf-8") as f:
        for line in f:
            rec = json.loads(line)
            if rec.get("claim_id") == "CLM-003":
                clm_003 = rec
                break

    assert clm_003 is not None, "CLM-003 not found in CLAIMS.jsonl"
    contradiction = clm_003["contradiction_record"]
    assert "Classification D" in contradiction or "classification d" in contradiction.lower(), contradiction
    assert "audit" in contradiction.lower(), contradiction


def test_test_files_distinguished_from_executed_evidence() -> None:
    files_path = REPO_ROOT / "docs/records/inventories/repository-archaeology/FILES.jsonl"
    assert files_path.exists(), f"Missing {files_path}"

    test_files_count = 0
    test_files_with_tier_a = 0

    with open(files_path, "r", encoding="utf-8") as f:
        for line in f:
            rec = json.loads(line)
            p = rec.get("path", "")
            if p.startswith("tests/"):
                test_files_count += 1
                if rec.get("evidence_tier") == "Tier A":
                    test_files_with_tier_a += 1

    assert test_files_count > 0, "No test files found under tests/"
    assert test_files_with_tier_a == 0, (
        f"Found {test_files_with_tier_a} files under tests/ marked as Tier A executed evidence! "
        "Test definition files must not be conflated with executed evidence."
    )


def test_rfc_authority_classification() -> None:
    files_path = REPO_ROOT / "docs/records/inventories/repository-archaeology/FILES.jsonl"
    assert files_path.exists(), f"Missing {files_path}"

    rfc_map = {}
    with open(files_path, "r", encoding="utf-8") as f:
        for line in f:
            rec = json.loads(line)
            p = rec.get("path", "")
            if p.startswith("spec/rfcs/RFC-") and p.endswith(".md"):
                rfc_map[p] = rec

    # RFC-00C8 is accepted in markdown ("Status: Accepted")
    c8_path = "spec/rfcs/RFC-00C8-concurrent-fault-isolation.md"
    assert c8_path in rfc_map, f"Missing {c8_path} in inventory"
    c8_rec = rfc_map[c8_path]

    assert c8_rec["authority"] == "NORMATIVE_CONTRACT", (
        f"RFC-00C8 has authority {c8_rec['authority']}, expected NORMATIVE_CONTRACT"
    )
    assert c8_rec["lifecycle"] == "CURRENT", (
        f"RFC-00C8 has lifecycle {c8_rec['lifecycle']}, expected CURRENT"
    )


def main() -> int:
    print("Running archaeology inventory regression tests...")
    test_investigation_10a_disposition()
    print(" - test_investigation_10a_disposition: PASSED")
    test_h04_r02_limitations_and_audit_reclassification()
    print(" - test_h04_r02_limitations_and_audit_reclassification: PASSED")
    test_test_files_distinguished_from_executed_evidence()
    print(" - test_test_files_distinguished_from_executed_evidence: PASSED")
    test_rfc_authority_classification()
    print(" - test_rfc_authority_classification: PASSED")
    print("All archaeology inventory regression tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
