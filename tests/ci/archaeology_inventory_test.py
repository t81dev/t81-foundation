#!/usr/bin/env python3
"""Regression tests for repository archaeology inventory and classification.

Verifies:
1. Investigation 10A disposition is preserved as inconclusive / evaluation blocked
   due to unverified literature data, rather than claiming physical silicon falsification.
2. H04-R02 claim preserves exact conditional numerical results (AT=1210, AB=1250 at r=1.60,
   crossover 5/3 under 10-bit Tier-0 control) and Classification D methodological audit reclassification
   without universal advantage, universal disadvantage, or hardware superiority claims.
3. No file receives a Tier A rating without explicit pinned execution provenance across
   experiments/, tests/, scripts, historical reports, and generated artifacts.
4. Unit tests for has_pinned_execution_provenance(): complete qualifying receipts attain Tier A,
   incomplete receipts and generic source/docs cannot.
5. RFC authority classification derives from authentic Status metadata rather than
   filename assumptions (e.g., verifying RFC-00C8 is recognized as Accepted / NORMATIVE_CONTRACT).
"""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))

from tools.archaeology_classifier import has_pinned_execution_provenance, classify_artifact


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


def test_h04_r02_limitations_and_exact_numerical_results() -> None:
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

    # Exact conditional numerical verification:
    # boundary_oscillation with 10-bit Tier-0 control has AT=1210 < AB=1250 at r=1.60, crossover 5/3,
    # with 6-trit unequal native capacity domain limitation.
    assert "1210" in contradiction, f"Expected exact AT=1210 in CLM-003: {contradiction}"
    assert "1250" in contradiction, f"Expected exact AB=1250 in CLM-003: {contradiction}"
    assert "1.60" in contradiction, f"Expected r=1.60 in CLM-003: {contradiction}"
    assert "5/3" in contradiction, f"Expected crossover 5/3 in CLM-003: {contradiction}"
    assert "boundary_oscillation" in contradiction, f"Expected boundary_oscillation workload in CLM-003: {contradiction}"
    assert "unequal" in contradiction.lower() or "smaller" in contradiction.lower(), contradiction

    # Must not assert universal advantage or universal negative/hardware superiority
    assert "universal advantage" in contradiction.lower() and "not demonstrate" in contradiction.lower()


def test_no_unsupported_tier_a_assignments() -> None:
    """Verifies that no file receives a Tier A rating without explicit pinned execution provenance.
    Rejects unsupported Tier A promotions across experiments/, tests/, scripts/, historical reports,
    and generated artifacts.
    """
    files_path = REPO_ROOT / "docs/records/inventories/repository-archaeology/FILES.jsonl"
    assert files_path.exists(), f"Missing {files_path}"

    unsupported_tier_a = []
    with open(files_path, "r", encoding="utf-8") as f:
        for line in f:
            rec = json.loads(line)
            p = rec.get("path", "")
            t = rec.get("evidence_tier")
            if t == "Tier A":
                # Ensure every file marked Tier A actually qualifies with pinned execution provenance
                if not has_pinned_execution_provenance(p):
                    unsupported_tier_a.append((p, rec))

    assert len(unsupported_tier_a) == 0, (
        f"Found {len(unsupported_tier_a)} files with unsupported Tier A assignments! "
        f"Examples: {unsupported_tier_a[:5]}"
    )


def test_has_pinned_execution_provenance_unit() -> None:
    """Unit tests for has_pinned_execution_provenance():
    Verifies that complete qualifying receipts can attain Tier A, and incomplete receipts cannot.
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        # 1. Negative: Source code file (.cpp) even with complete info must NOT be promoted to Tier A
        cpp_file = tmp_path / "test.cpp"
        cpp_file.write_text("""
        // Execution Receipt
        // Git commit: f775680aba7db200691c0f4e2a3174b0e7e51581
        // Environment: Linux x86_64 toolchain
        // Command: cmake --build build
        // Input: input.bin
        // Output: result.bin
        // Scope: unit test
        """, encoding="utf-8")
        assert not has_pinned_execution_provenance(str(cpp_file)), "Source code .cpp should not be Tier A receipt"

        # 2. Negative: Missing commit SHA
        incomplete_file1 = tmp_path / "receipt_no_sha.json"
        incomplete_file1.write_text(json.dumps({
            "receipt": "execution receipt",
            "environment": "linux x86_64",
            "command": "python3 run.py",
            "input": "data.raw",
            "output": "result.json",
            "scope": "run test"
        }), encoding="utf-8")
        assert not has_pinned_execution_provenance(str(incomplete_file1)), "Missing SHA should fail"

        # 3. Negative: Missing commands
        incomplete_file2 = tmp_path / "receipt_no_cmd.json"
        incomplete_file2.write_text(json.dumps({
            "receipt": "execution receipt",
            "commit": "f775680aba7db200691c0f4e2a3174b0e7e51581",
            "environment": "linux x86_64",
            "input": "data.raw",
            "output": "result.json",
            "scope": "run test"
        }), encoding="utf-8")
        assert not has_pinned_execution_provenance(str(incomplete_file2)), "Missing command should fail"

        # 4. Positive: Complete qualifying execution receipt
        complete_receipt = tmp_path / "qualifying_receipt.json"
        complete_receipt.write_text(json.dumps({
            "execution_receipt": "verified",
            "provenance_commit": "f775680aba7db200691c0f4e2a3174b0e7e51581",
            "environment": "Linux x86_64 release toolchain",
            "command": "cmake --build build && python3 runner.py",
            "input_source": "dataset_v1.tar.gz",
            "output_result": "observed_measurements.json",
            "scope": "reproducible benchmark verification"
        }), encoding="utf-8")
        assert has_pinned_execution_provenance(str(complete_receipt)), "Complete qualifying receipt must pass"


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
    test_h04_r02_limitations_and_exact_numerical_results()
    print(" - test_h04_r02_limitations_and_exact_numerical_results: PASSED")
    test_no_unsupported_tier_a_assignments()
    print(" - test_no_unsupported_tier_a_assignments: PASSED")
    test_has_pinned_execution_provenance_unit()
    print(" - test_has_pinned_execution_provenance_unit: PASSED")
    test_rfc_authority_classification()
    print(" - test_rfc_authority_classification: PASSED")
    print("All archaeology inventory regression tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
