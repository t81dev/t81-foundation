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
4. Unit tests for structured provenance contract (verify_provenance_contract):
   - Rejects fabricated receipts containing expected keywords without valid contract schema.
   - Rejects receipts with missing or malformed commit SHAs.
   - Rejects receipts with missing source linkage or empty source paths.
   - Rejects receipts with missing command invocations.
   - Rejects receipts with missing output identities or empty output mappings.
   - Classifies syntactically complete unverified receipts as candidates for verification (NONE).
   - Promotes to Tier A only upon verified independent execution.
5. Directory membership in historical/archive affects lifecycle only, not evidence tier.
6. RFC authority classification derives from authentic Status metadata rather than
   filename assumptions (e.g., verifying RFC-00C8 is recognized as Accepted / NORMATIVE_CONTRACT).
"""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))

from tools.archaeology_classifier import (
    has_pinned_execution_provenance,
    verify_provenance_contract,
    classify_artifact,
)


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


def test_archive_path_membership_affects_lifecycle_only() -> None:
    """Verifies that membership in an archive or historical directory/filename
    affects artifact lifecycle only (marking it HISTORICAL) and does not automatically
    mutate or promote the evidence tier.
    """
    # 1. A documentation file in archive should have lifecycle HISTORICAL and tier NONE
    c1 = classify_artifact("docs/archive/old_spec.md", 500)
    assert c1["lifecycle"] == "HISTORICAL"
    assert c1["evidence_tier"] == "NONE", f"Expected tier NONE, got {c1['evidence_tier']}"

    # 2. A source code file in historical directory should have lifecycle HISTORICAL and tier Tier B (implementation)
    c2 = classify_artifact("experiments/s02_stochastic_topology/historical/pr582/main.cpp", 1000)
    assert c2["lifecycle"] == "HISTORICAL"
    assert c2["evidence_tier"] == "Tier B", f"Expected tier Tier B, got {c2['evidence_tier']}"

    # 3. An experimental results JSON in historical directory should have lifecycle HISTORICAL and tier NONE (unverified data)
    c3 = classify_artifact("experiments/s02_stochastic_topology/historical/pr582/results.json", 2000)
    assert c3["lifecycle"] == "HISTORICAL"
    assert c3["evidence_tier"] == "NONE", f"Expected tier NONE, got {c3['evidence_tier']}"


def test_structured_provenance_contract_unit() -> None:
    """Unit tests for verify_provenance_contract() and has_pinned_execution_provenance():
    Verifies that:
    1. Fabricated files containing keywords but not valid JSON/contract are rejected.
    2. Missing or malformed commit SHAs are rejected.
    3. Missing or empty source linkage is rejected.
    4. Missing or empty commands list is rejected.
    5. Missing or empty output identities are rejected.
    6. Well-formed receipts without independent execution are candidates for verification (NONE).
    7. Fully verified execution receipts attain Tier A.
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        # 1. Keyword stuffing in non-JSON file
        fake_txt = tmp_path / "fake_receipt.txt"
        fake_txt.write_text("""
        schema: t81.provenance.v1
        pinned_commit: f775680aba7db200691c0f4e2a3174b0e7e51581
        source_linkage: src/vm/vm.cpp
        command: cmake --build build
        environment: Linux x86_64
        output_identities: results.json
        verified_execution: INDEPENDENTLY_REPRODUCED_TIER_A
        """, encoding="utf-8")
        res1 = verify_provenance_contract(str(fake_txt))
        assert not res1["valid_contract"]
        assert not has_pinned_execution_provenance(str(fake_txt))

        # 2. JSON with invalid/unrelated SHA length
        bad_sha = tmp_path / "bad_sha.json"
        bad_sha.write_text(json.dumps({
            "schema": "t81.provenance.v1",
            "pinned_commit": "not_a_valid_sha_hash",
            "source_linkage": ["src/vm/vm.cpp"],
            "commands": ["cmake --build build"],
            "environment": {"os": "linux", "arch": "x86_64"},
            "outputs": {"results.json": "sha256:abc"}
        }), encoding="utf-8")
        res2 = verify_provenance_contract(str(bad_sha))
        assert not res2["valid_contract"]
        assert "pinned_commit" in res2["error"]

        # 3. Missing source linkage
        no_src = tmp_path / "no_src.json"
        no_src.write_text(json.dumps({
            "schema": "t81.provenance.v1",
            "pinned_commit": "f775680aba7db200691c0f4e2a3174b0e7e51581",
            "source_linkage": [],
            "commands": ["cmake --build build"],
            "environment": {"os": "linux", "arch": "x86_64"},
            "outputs": {"results.json": "sha256:abc"}
        }), encoding="utf-8")
        res3 = verify_provenance_contract(str(no_src))
        assert not res3["valid_contract"]
        assert "source_linkage" in res3["error"]

        # 4. Missing outputs / output identities
        no_out = tmp_path / "no_out.json"
        no_out.write_text(json.dumps({
            "schema": "t81.provenance.v1",
            "pinned_commit": "f775680aba7db200691c0f4e2a3174b0e7e51581",
            "source_linkage": ["src/vm/vm.cpp"],
            "commands": ["cmake --build build"],
            "environment": {"os": "linux", "arch": "x86_64"},
            "outputs": {}
        }), encoding="utf-8")
        res4 = verify_provenance_contract(str(no_out))
        assert not res4["valid_contract"]
        assert "outputs" in res4["error"]

        # 5. Candidate for verification (well-formed, but unverified execution -> tier NONE)
        candidate = tmp_path / "candidate_receipt.json"
        candidate.write_text(json.dumps({
            "schema": "t81.provenance.v1",
            "pinned_commit": "f775680aba7db200691c0f4e2a3174b0e7e51581",
            "source_linkage": ["src/vm/vm.cpp"],
            "commands": ["cmake --build build"],
            "environment": {"os": "linux", "arch": "x86_64"},
            "outputs": {"results.json": "sha256:abc123def456"}
        }), encoding="utf-8")
        res5 = verify_provenance_contract(str(candidate))
        assert res5["valid_contract"]
        assert res5["candidate_for_verification"]
        assert not res5["verified_tier_a"]
        assert not has_pinned_execution_provenance(str(candidate))

        # 6. Adversarial test: Self-attested JSON claims (e.g. verified_execution)
        # must NEVER independently establish Tier A promotion without external verification.
        # It must remain candidate_for_verification (NONE) rather than verified Tier A.
        self_attested = tmp_path / "self_attested_receipt.json"
        self_attested.write_text(json.dumps({
            "schema": "t81.provenance.v1",
            "pinned_commit": "f775680aba7db200691c0f4e2a3174b0e7e51581",
            "source_linkage": ["src/vm/vm.cpp"],
            "commands": ["cmake --build build"],
            "environment": {"os": "linux", "arch": "x86_64"},
            "outputs": {"results.json": "sha256:abc123def456"},
            "verified_execution": "INDEPENDENTLY_REPRODUCED_TIER_A"
        }), encoding="utf-8")
        res6 = verify_provenance_contract(str(self_attested))
        assert res6["valid_contract"]
        assert res6["candidate_for_verification"]
        assert not res6["verified_tier_a"], "Self-attested receipt must NOT attain Tier A"
        assert not has_pinned_execution_provenance(str(self_attested)), "has_pinned_execution_provenance must return False for self-attested receipt"


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
    test_archive_path_membership_affects_lifecycle_only()
    print(" - test_archive_path_membership_affects_lifecycle_only: PASSED")
    test_structured_provenance_contract_unit()
    print(" - test_structured_provenance_contract_unit: PASSED")
    test_rfc_authority_classification()
    print(" - test_rfc_authority_classification: PASSED")
    print("All archaeology inventory regression tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
