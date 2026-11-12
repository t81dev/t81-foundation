"""
test_architecture_engineering_diagnostics.py
Comprehensive regression and negative test suite for T81-ARCH-ENG-002 Phase 1:
Architecture-Guided Engineering Diagnostics and Drift Detection.

Verifies:
1. Positive baseline: validates visualization datasets and component_evidence_index.json.
2. Mandatory evidence index: validator fails closed if component_evidence_index.json is missing,
   and succeeds only when --allow-missing-evidence-index is provided.
3. Summary reconciliation: detects and rejects total_components mismatch and classification_summary count drift.
4. Classification integrity & prerequisites: prevents unsupported promotion to VERIFIED without receipts,
   without test files, or with unverified node maturity qualification.
5. Layer ID referential integrity: detects component layer_id mismatch against nodes and layers.
6. Negative drift cases: missing files, broken edges, invalid interface references, stale line bounds, unsupported reference types.
7. Bash arithmetic defect regression: reproduces `set -e` / `((COMPLETED_TESTS++))` failure and verifies safe alternative.
8. Python optimization safety: fail-closed execution under both normal Python and `python -O`.
9. Determinism: identical inputs produce identical diagnostic outputs across repeated invocations.
"""

import copy
import json
import os
import shutil
import subprocess
import sys
import tempfile
import pytest

import importlib.util

SCRIPT_PATH = os.path.join("scripts", "ci", "validate_architecture_visualization.py")
DATA_DIR = os.path.join("docs", "architecture", "visualization")

def _load_validator_module():
    spec = importlib.util.spec_from_file_location("val_mod", SCRIPT_PATH)
    val_mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(val_mod)
    return val_mod

val_mod = _load_validator_module()
detect_drift = val_mod.detect_drift
validate = val_mod.validate
collect_baseline_inventory = getattr(val_mod, "collect_baseline_inventory", None)
check_git_ancestry = getattr(val_mod, "check_git_ancestry", None)
VALID_EVIDENCE_RELATIONSHIPS = val_mod.VALID_EVIDENCE_RELATIONSHIPS
VALID_REFERENCE_TYPES = val_mod.VALID_REFERENCE_TYPES

@pytest.fixture
def isolated_dataset():
    """Provides a temporary, isolated copy of the architecture visualization dataset."""
    with tempfile.TemporaryDirectory() as tmpdir:
        for fname in os.listdir(DATA_DIR):
            src = os.path.join(DATA_DIR, fname)
            if os.path.isfile(src):
                shutil.copy(src, os.path.join(tmpdir, fname))
        yield tmpdir

# ---------------------------------------------------------------------------
# 1. Positive Baseline Tests
# ---------------------------------------------------------------------------

def test_positive_visualization_and_evidence_index_validation():
    """Positive test: existing datasets and component_evidence_index validate cleanly."""
    res = subprocess.run([sys.executable, SCRIPT_PATH, "--drift-check"], capture_output=True, text=True)
    assert res.returncode == 0, f"Validator failed:\n{res.stdout}\n{res.stderr}"
    assert "All Visualization Data Invariants Verified Successfully!" in res.stdout
    assert "Zero drift detected against repository tree." in res.stdout

def test_positive_detect_drift_clean_baseline():
    """Positive test: detect_drift reports zero anomalies on baseline tree."""
    anomalies = detect_drift(DATA_DIR)
    assert sum(len(v) for v in anomalies.values()) == 0, f"Unexpected baseline drift: {anomalies}"

def test_evidence_index_completeness_and_schema():
    """Positive test: component_evidence_index covers all 35 nodes and adheres to schema."""
    index_path = os.path.join(DATA_DIR, "component_evidence_index.json")
    assert os.path.exists(index_path), "component_evidence_index.json missing from visualization dir"
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    assert data["schema_version"] == "1.0.0"
    assert data["baseline_commit"] == "9a3e307ad21135749d35a4f1a6a0d5820d0cc698"
    assert data["total_components"] == 35

    summary = data["classification_summary"]
    assert summary["VERIFIED"] == 9
    assert summary["STATICALLY_SUPPORTED"] == 22
    assert summary["UNVERIFIED"] == 3
    assert summary["NOT_MAPPED"] == 1
    assert sum(summary.values()) == 35

# ---------------------------------------------------------------------------
# 2. Mandatory Evidence Index Tests
# ---------------------------------------------------------------------------

def test_negative_missing_mandatory_evidence_index(isolated_dataset):
    """Validator must fail closed if component_evidence_index.json is missing by default."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    os.remove(index_path)

    with pytest.raises(FileNotFoundError, match="Missing required visualization file: .*component_evidence_index.json"):
        validate(isolated_dataset, allow_missing_evidence_index=False)

def test_legacy_mode_allows_missing_evidence_index(isolated_dataset):
    """Explicit legacy flag --allow-missing-evidence-index allows missing index."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    os.remove(index_path)

    # Should validate cleanly when flag is True
    validate(isolated_dataset, allow_missing_evidence_index=True)

# ---------------------------------------------------------------------------
# 3. Summary Reconciliation Tests
# ---------------------------------------------------------------------------

def test_negative_summary_reconciliation_total_components_mismatch(isolated_dataset):
    """Summary reconciliation must reject mismatch between declared total_components and actual component count."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["total_components"] = 999
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    with pytest.raises(ValueError, match="Census mismatch in component_evidence_index.json: total_components=999"):
        validate(isolated_dataset)

def test_negative_summary_reconciliation_category_count_mismatch(isolated_dataset):
    """Summary reconciliation must reject mismatch in classification_summary counts."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    # Alter declared VERIFIED count
    data["classification_summary"]["VERIFIED"] = 99
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    with pytest.raises(ValueError, match="Summary reconciliation discrepancy for VERIFIED: declared 99, recomputed 9"):
        validate(isolated_dataset)

# ---------------------------------------------------------------------------
# 4. Classification Integrity & VERIFIED Prerequisites Tests
# ---------------------------------------------------------------------------

def test_negative_verified_promotion_without_receipts(isolated_dataset):
    """Reject unsupported promotion to VERIFIED when component lacks receipt_files."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Promote an unverified node to VERIFIED without receipts
    for c in data["components"]:
        if c["component_id"] == "sub.layer.1_lang.frontend":
            c["relationship_classification"] = "VERIFIED"
            c["receipt_files"] = []
            break
    data["classification_summary"]["VERIFIED"] += 1
    data["classification_summary"]["STATICALLY_SUPPORTED"] -= 1

    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    with pytest.raises(ValueError, match="sub.layer.1_lang.frontend: VERIFIED classification requires at least one authoritative qualification receipt"):
        validate(isolated_dataset)

def test_negative_verified_promotion_with_unqualified_node_maturity(isolated_dataset):
    """Reject promotion to VERIFIED when underlying node maturity qualification is not QUALIFIED or QUALIFIED_WITH_LIMITATIONS."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Promote node with valid receipt, but node maturity in architecture_nodes.json is UNVERIFIED
    for c in data["components"]:
        if c["component_id"] == "sub.layer.1_lang.frontend":
            c["relationship_classification"] = "VERIFIED"
            c["receipt_files"] = ["docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md"]
            break
    data["classification_summary"]["VERIFIED"] += 1
    data["classification_summary"]["STATICALLY_SUPPORTED"] -= 1

    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    with pytest.raises(ValueError, match="Unsupported promotion to VERIFIED. Node maturity qualification is 'UNVERIFIED'"):
        validate(isolated_dataset)

def test_negative_component_layer_id_mismatch(isolated_dataset):
    """Reject component layer_id mismatch against declared architecture node."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["components"][0]["layer_id"] = "layer.7_assurance"  # Wrong layer
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    with pytest.raises(ValueError, match="layer_id mismatch"):
        validate(isolated_dataset)

# ---------------------------------------------------------------------------
# 5. Negative Drift Tests
# ---------------------------------------------------------------------------

def test_negative_drift_missing_source_path(isolated_dataset):
    """Negative drift test: detects missing source paths in nodes."""
    nodes_path = os.path.join(isolated_dataset, "architecture_nodes.json")
    with open(nodes_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["nodes"][0]["source_paths"].append("nonexistent/phantom/file.cpp")
    with open(nodes_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    drift = detect_drift(isolated_dataset)
    assert len(drift["missing_source_paths"]) == 1
    assert drift["missing_source_paths"][0]["path"] == "nonexistent/phantom/file.cpp"

def test_negative_drift_stale_line_reference(isolated_dataset):
    """Negative drift test: detects stale/out-of-bounds line numbers in execution paths."""
    paths_path = os.path.join(isolated_dataset, "execution_paths.json")
    with open(paths_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["paths"][0]["steps"][0]["line"] = 999999
    with open(paths_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    drift = detect_drift(isolated_dataset)
    assert len(drift["stale_line_references"]) == 1
    assert drift["stale_line_references"][0]["line"] == 999999

def test_negative_drift_unsupported_reference_type(isolated_dataset):
    """Negative drift test: detects unsupported reference type strings."""
    edges_path = os.path.join(isolated_dataset, "architecture_edges.json")
    with open(edges_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["edges"][0]["evidence"]["reference_type"] = "INVALID_SPECULATIVE_REF"
    with open(edges_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    drift = detect_drift(isolated_dataset)
    assert len(drift["unsupported_reference_types"]) == 1
    assert drift["unsupported_reference_types"][0]["reference_type"] == "INVALID_SPECULATIVE_REF"

def test_negative_drift_dangling_interface_reference(isolated_dataset):
    """Negative drift test: detects interface provider/consumer not present in nodes."""
    ifaces_path = os.path.join(isolated_dataset, "architecture_interfaces.json")
    with open(ifaces_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["interfaces"][0]["provider_node_id"] = "sub.layer.999_phantom"
    with open(ifaces_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    drift = detect_drift(isolated_dataset)
    assert len(drift["record_inconsistencies"]) >= 1
    assert any("sub.layer.999_phantom" in item["issue"] for item in drift["record_inconsistencies"])

def test_evidence_index_rejects_invalid_classification(isolated_dataset):
    """Evidence index must reject invalid relationship classification strings."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["components"][0]["relationship_classification"] = "PROMOTED_BY_AI"
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    with pytest.raises(ValueError, match="invalid relationship_classification: PROMOTED_BY_AI"):
        validate(isolated_dataset)

def test_evidence_index_rejects_missing_node_coverage(isolated_dataset):
    """Evidence index must account for 100% of declared architecture nodes."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    popped = data["components"].pop(0)
    data["total_components"] -= 1
    rel = popped["relationship_classification"]
    data["classification_summary"][rel] -= 1
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    with pytest.raises(ValueError, match=f"Component evidence index missing entries for nodes: \\['{popped['component_id']}'\\]"):
        validate(isolated_dataset)

def test_evidence_index_rejects_nonexistent_test_or_receipt(isolated_dataset):
    """Evidence index must reject cited test files or receipts that do not exist."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["components"][0]["test_files"].append("tests/cpp/phantom_nonexistent_test.cpp")
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    with pytest.raises(FileNotFoundError, match="test file does not exist: tests/cpp/phantom_nonexistent_test.cpp"):
        validate(isolated_dataset)

# ---------------------------------------------------------------------------
# 6. Bash Arithmetic Regression Tests (`set -e` + `((COMPLETED_TESTS++))`)
# ---------------------------------------------------------------------------

def test_bash_arithmetic_evaluation_defect_reproduction():
    """Postincrement returns 1 at zero; errexit covers it starting in Bash 4.1.

    Bash 3.2 (macOS system Bash) does not apply errexit to arithmetic commands.
    Check both the status defect and each version's actual exit behavior.
    """
    version = subprocess.check_output(
        ["bash", "-c", 'echo "${BASH_VERSINFO[0]}.${BASH_VERSINFO[1]}"'], text=True
    )
    bash_version = tuple(map(int, version.strip().split(".")))
    status = subprocess.run(
        ["bash", "-c", 'n=0; ((n++)); result=$?; echo "$result:$n"'],
        capture_output=True, text=True
    )
    assert status.returncode == 0
    assert status.stdout.strip() == "1:1"
    script_defect = """set -e
COMPLETED_TESTS=0
[ -n yes ] && ((COMPLETED_TESTS++))
echo "COMPLETED_REACHED: $COMPLETED_TESTS"
"""
    res = subprocess.run(["bash", "-c", script_defect], capture_output=True, text=True)
    expected_exit = 1 if bash_version >= (4, 1) else 0
    assert res.returncode == expected_exit, f"Bash {version.strip()}: {res.stderr}"
    if expected_exit:
        assert "COMPLETED_REACHED" not in res.stdout
    else:
        assert res.stdout.strip() == "COMPLETED_REACHED: 1"


def test_bash_arithmetic_safe_alternative_verification():
    """
    Subprocess test verifying the safe corrective alternative:
    `COMPLETED_TESTS=$((COMPLETED_TESTS + 1))` succeeds under `set -e`.
    """
    script_safe = """#!/usr/bin/env bash
set -e
TOTAL_TESTS=5
COMPLETED_TESTS=0
tmp_file=$(mktemp)
if [ -f "$tmp_file" ]; then
    COMPLETED_TESTS=$((COMPLETED_TESTS + 1))
fi
rm -f "$tmp_file"
echo "COMPLETED_REACHED: $COMPLETED_TESTS"
"""
    res = subprocess.run(["bash", "-c", script_safe], capture_output=True, text=True)
    assert res.returncode == 0, f"Safe script unexpectedly failed:\n{res.stderr}"
    assert "COMPLETED_REACHED: 1" in res.stdout

# ---------------------------------------------------------------------------
# 7. Python Optimization and Fail-Closed Safety Tests
# ---------------------------------------------------------------------------

def test_python_optimized_mode_validation():
    """Verify validator operates fail-closed under `python -O` (assert-independent)."""
    cmd = [sys.executable, "-O", SCRIPT_PATH, "--drift-check"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    assert res.returncode == 0, f"Validator failed under python -O:\n{res.stdout}\n{res.stderr}"
    assert "All Visualization Data Invariants Verified Successfully!" in res.stdout
    assert "Zero drift detected against repository tree." in res.stdout

def test_python_optimized_mode_rejects_corrupted_evidence_index(isolated_dataset):
    """Verify python -O mode rejects malformed evidence index entries with nonzero exit code."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["components"][0]["relationship_classification"] = "CORRUPTED_VALUE"
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    cmd = [
        sys.executable,
        "-O",
        "-c",
        f"import sys; sys.path.insert(0, 'scripts/ci'); import validate_architecture_visualization as v; v.validate('{isolated_dataset}')"
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    assert res.returncode != 0
    assert "invalid relationship_classification: CORRUPTED_VALUE" in res.stderr

def test_python_optimized_mode_rejects_missing_mandatory_index(isolated_dataset):
    """Verify python -O mode rejects missing mandatory component_evidence_index.json."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    os.remove(index_path)

    cmd = [
        sys.executable,
        "-O",
        "-c",
        f"import sys; sys.path.insert(0, 'scripts/ci'); import validate_architecture_visualization as v; v.validate('{isolated_dataset}')"
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    assert res.returncode != 0
    assert "Missing required visualization file" in res.stderr

# ---------------------------------------------------------------------------
# 8. Determinism Test
# ---------------------------------------------------------------------------

def test_deterministic_drift_detection_repeated_runs():
    """Verifies that multiple drift detection invocations produce bit-for-bit identical results."""
    run1 = detect_drift(DATA_DIR)
    run2 = detect_drift(DATA_DIR)
    assert json.dumps(run1, sort_keys=True) == json.dumps(run2, sort_keys=True)

# ---------------------------------------------------------------------------
# 9. Provenance and Baseline Inventory Tests
# ---------------------------------------------------------------------------

def test_baseline_inventory_collection():
    """Verifies collect_baseline_inventory catalogs all 13 datasets with baselines, timestamps, and scopes."""
    assert collect_baseline_inventory is not None
    inv_data = collect_baseline_inventory(DATA_DIR, comparison_ref="HEAD")
    items = inv_data["inventory"]
    assert len(items) == 13

    files = {it["filename"] for it in items}
    assert "architecture_layers.json" in files
    assert "component_evidence_index.json" in files
    assert "research_programs.json" in files
    assert "pull_requests_snapshot.json" in files

    distinct_baselines = {it["recorded_baseline"] for it in items if it["recorded_baseline"] != "Not recorded"}
    assert "4f96cb24358bfd260e67387b2c4db2797b01236f" in distinct_baselines
    assert "9a3e307ad21135749d35a4f1a6a0d5820d0cc698" in distinct_baselines
    assert "6c9eaf1a2fc8368cd06f6dddbbdcd78e6606c0ce" in distinct_baselines

    for it in items:
        assert it["scope"] != "Not recorded"
        assert it["ancestry_status"] in ["IDENTICAL", "ANCESTOR", "DIVERGENT", "UNAVAILABLE"]
        if it["ancestry_status"] in ["IDENTICAL", "ANCESTOR"]:
            assert isinstance(it["commits_behind"], int)
            assert it["commits_behind"] >= 0
        else:
            assert it["commits_behind"] is None

def test_git_ancestry_edge_cases(tmp_path):
    """Use a local commit graph, independent of checkout history or shallow clones."""
    def git(*args, input=None):
        try:
            return subprocess.check_output(
                ["git", "-c", "user.name=Test", "-c", "user.email=test@example.invalid", *args],
                cwd=tmp_path, input=input, text=True, stderr=subprocess.PIPE
            ).strip()
        except (OSError, subprocess.CalledProcessError) as exc:
            pytest.fail(f"Git fixture execution failed: {exc}")

    git("init")
    tree = git("mktree", input="")
    root = git("commit-tree", tree, input="root\n")
    head = git("commit-tree", tree, "-p", root, input="head\n")
    sibling = git("commit-tree", tree, "-p", root, input="sibling\n")
    git("update-ref", "refs/heads/comparison", head)
    ref = "refs/heads/comparison"

    assert check_git_ancestry(head, ref, tmp_path) == ("IDENTICAL", 0, head)
    assert check_git_ancestry(root, ref, tmp_path) == ("ANCESTOR", 1, head)
    assert check_git_ancestry(sibling, ref, tmp_path) == ("DIVERGENT", None, head)
    assert check_git_ancestry("0" * 40, ref, tmp_path) == ("UNAVAILABLE", None, head)
    assert check_git_ancestry("Not recorded", ref, tmp_path) == ("UNAVAILABLE", None, None)
    assert check_git_ancestry(root, "refs/nonexistent/phantom_ref", tmp_path) == ("UNAVAILABLE", None, None)


def test_missing_metadata_normalizes_to_not_recorded(isolated_dataset):
    """Verifies that omitted baseline and timestamp keys normalize to 'Not recorded' without exceptions."""
    layers_path = os.path.join(isolated_dataset, "architecture_layers.json")
    with open(layers_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data.pop("baseline_commit", None)
    with open(layers_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    inv = collect_baseline_inventory(isolated_dataset, comparison_ref="HEAD")
    layers_item = next(it for it in inv["inventory"] if it["filename"] == "architecture_layers.json")
    assert layers_item["recorded_baseline"] == "Not recorded"
    assert layers_item["recorded_timestamp"] == "Not recorded"
    assert layers_item["ancestry_status"] == "UNAVAILABLE"
    assert layers_item["commits_behind"] is None

def test_drift_detects_missing_component_evidence_paths(isolated_dataset):
    """Verifies detect_drift reports missing test_files and receipt_files cited in component_evidence_index.json."""
    index_path = os.path.join(isolated_dataset, "component_evidence_index.json")
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Inject nonexistent test and receipt files
    data["components"][0]["test_files"].append("tests/cpp/nonexistent_test_drift.cpp")
    data["components"][0]["receipt_files"].append("docs/research/nonexistent_receipt_drift.md")
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump(data, f)

    anomalies = detect_drift(isolated_dataset)
    missing = anomalies["missing_source_paths"]
    missing_paths = [m["path"] for m in missing]

    assert "tests/cpp/nonexistent_test_drift.cpp" in missing_paths
    assert "docs/research/nonexistent_receipt_drift.md" in missing_paths

    test_ctx = next(m["context"] for m in missing if m["path"] == "tests/cpp/nonexistent_test_drift.cpp")
    assert "test_files" in test_ctx
    receipt_ctx = next(m["context"] for m in missing if m["path"] == "docs/research/nonexistent_receipt_drift.md")
    assert "receipt_files" in receipt_ctx

def test_baseline_inventory_separated_from_integrity_failures():
    """Verifies that having mixed historical baselines does not cause validate() or detect_drift() to report drift."""
    # Positive validation passes cleanly even though baselines are 47, 33, and 24 commits behind HEAD
    anomalies = detect_drift(DATA_DIR)
    assert sum(len(v) for v in anomalies.values()) == 0, f"Expected 0 drift anomalies, got: {anomalies}"
