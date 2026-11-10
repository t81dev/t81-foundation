"""
test_architecture_engineering_diagnostics.py
Comprehensive regression and negative test suite for T81-ARCH-ENG-002 Phase 1:
Architecture-Guided Engineering Diagnostics and Drift Detection.

Verifies:
1. Positive baseline: validates visualization datasets and component_evidence_index.json.
2. Negative drift cases: missing files, broken edges, invalid interface references, stale line bounds, unsupported reference types.
3. Evidence classification enforcement: rejection of invalid classifications and unsupported VERIFIED promotions.
4. Referential integrity: component_evidence_index resolves to valid node IDs and real repo artifacts.
5. Bash arithmetic defect regression: reproduces `set -e` / `((COMPLETED_TESTS++))` failure and verifies safe alternative.
6. Python optimization safety: fail-closed execution under both normal Python and `python -O`.
7. Fail-closed behavior: malformed/incomplete evidence yields nonzero exit code and explicit exceptions.
8. Determinism: identical inputs produce identical diagnostic outputs across repeated invocations.
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
# 2. Negative Drift Tests
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
    # Put an impossible line number
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

# ---------------------------------------------------------------------------
# 3. Evidence Classification Enforcement Tests
# ---------------------------------------------------------------------------

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
# 4. Bash Arithmetic Regression Tests (`set -e` + `((COMPLETED_TESTS++))`)
# ---------------------------------------------------------------------------

def test_bash_arithmetic_evaluation_defect_reproduction():
    """
    Subprocess test reproducing the exact Bash defect in automated_verification.sh:
    When COMPLETED_TESTS=0, `((COMPLETED_TESTS++))` returns exit status 1.
    Under `set -e`, this immediately crashes the script.
    """
    script_defect = """#!/usr/bin/env bash
set -e
TOTAL_TESTS=5
COMPLETED_TESTS=0
tmp_file=$(mktemp)
[ -f "$tmp_file" ] && ((COMPLETED_TESTS++))
rm -f "$tmp_file"
echo "COMPLETED_REACHED"
"""
    res = subprocess.run(["bash", "-c", script_defect], capture_output=True, text=True)
    assert res.returncode == 1, "Expected bash script to fail closed with exit code 1"
    assert "COMPLETED_REACHED" not in res.stdout

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
# 5. Python Optimization and Fail-Closed Safety Tests
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

# ---------------------------------------------------------------------------
# 6. Determinism Test
# ---------------------------------------------------------------------------

def test_deterministic_drift_detection_repeated_runs():
    """Verifies that multiple drift detection invocations produce bit-for-bit identical results."""
    run1 = detect_drift(DATA_DIR)
    run2 = detect_drift(DATA_DIR)
    assert json.dumps(run1, sort_keys=True) == json.dumps(run2, sort_keys=True)
