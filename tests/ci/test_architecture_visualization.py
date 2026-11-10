"""
test_architecture_visualization.py
Pytest suite verifying the structural integrity and fail-closed validation of the
T81 architecture visualization datasets.
"""

import subprocess
import sys
import os
import json
import shutil
import tempfile
import pytest

SCRIPT_PATH = os.path.join("scripts", "ci", "validate_architecture_visualization.py")
DATA_DIR = os.path.join("docs", "architecture", "visualization")

def test_architecture_visualization_integrity():
    """Positive test: existing datasets pass all validation invariants."""
    res = subprocess.run([sys.executable, SCRIPT_PATH], capture_output=True, text=True)
    assert res.returncode == 0, f"Validation script failed:\n{res.stdout}\n{res.stderr}"
    assert "All Visualization Data Invariants Verified Successfully!" in res.stdout

def run_validation_in_dir(tmpdir):
    """Helper to run validator against a copy of the dataset."""
    import importlib.util
    spec = importlib.util.spec_from_file_location("val_mod", SCRIPT_PATH)
    val_mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(val_mod)
    val_mod.validate(data_dir=tmpdir)

@pytest.fixture
def temp_dataset():
    """Provides an isolated copy of the visualization dataset."""
    with tempfile.TemporaryDirectory() as tmpdir:
        for fname in os.listdir(DATA_DIR):
            src = os.path.join(DATA_DIR, fname)
            if os.path.isfile(src):
                shutil.copy(src, os.path.join(tmpdir, fname))
        yield tmpdir

def test_negative_invalid_source_line(temp_dataset):
    """Negative test: line number out of bounds fails validation."""
    edges_path = os.path.join(temp_dataset, "architecture_edges.json")
    with open(edges_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["edges"][0]["evidence"]["line"] = 99999999
    with open(edges_path, "w", encoding="utf-8") as f:
        json.dump(data, f)
    with pytest.raises(ValueError, match="out of bounds"):
        run_validation_in_dir(temp_dataset)

def test_negative_nonexistent_source_file(temp_dataset):
    """Negative test: nonexistent source file fails validation."""
    edges_path = os.path.join(temp_dataset, "architecture_edges.json")
    with open(edges_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["edges"][0]["evidence"]["source_file"] = "nonexistent/file/path.cpp"
    with open(edges_path, "w", encoding="utf-8") as f:
        json.dump(data, f)
    with pytest.raises(FileNotFoundError, match="Referenced file does not exist"):
        run_validation_in_dir(temp_dataset)

def test_negative_nonexistent_node_id(temp_dataset):
    """Negative test: edge referencing nonexistent node ID fails validation."""
    edges_path = os.path.join(temp_dataset, "architecture_edges.json")
    with open(edges_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["edges"][0]["source_id"] = "sub.nonexistent.fake_node"
    with open(edges_path, "w", encoding="utf-8") as f:
        json.dump(data, f)
    with pytest.raises(ValueError, match="references unknown source_id"):
        run_validation_in_dir(temp_dataset)

def test_negative_unsupported_maturity_claim(temp_dataset):
    """Negative test: node claiming QUALIFIED without qualification_evidence citation fails validation."""
    nodes_path = os.path.join(temp_dataset, "architecture_nodes.json")
    with open(nodes_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["nodes"][0]["maturity"]["qualification"] = "QUALIFIED"
    data["nodes"][0]["maturity_evidence"]["qualification_evidence"] = ""
    with open(nodes_path, "w", encoding="utf-8") as f:
        json.dump(data, f)
    with pytest.raises(ValueError, match="claims qualification without qualification_evidence"):
        run_validation_in_dir(temp_dataset)

def test_negative_trace_hash_mismatch(temp_dataset):
    """Negative test: execution pathway trace hash mismatch fails validation."""
    paths_path = os.path.join(temp_dataset, "execution_paths.json")
    with open(paths_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["paths"][1]["runtime_trace_evidence"]["sha256"] = "0000000000000000000000000000000000000000000000000000000000000000"
    with open(paths_path, "w", encoding="utf-8") as f:
        json.dump(data, f)
    with pytest.raises(ValueError, match="trace SHA-256 mismatch"):
        run_validation_in_dir(temp_dataset)

def test_negative_semantic_incompatibility(temp_dataset):
    """Negative test: runtime function invocation supported only by comment fails validation."""
    edges_path = os.path.join(temp_dataset, "architecture_edges.json")
    with open(edges_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    for e in data["edges"]:
        if e["relationship_type"] == "function_invocation":
            e["evidence"]["reference_type"] = "COMMENT"
            break
    with open(edges_path, "w", encoding="utf-8") as f:
        json.dump(data, f)
    with pytest.raises(ValueError, match="Semantic incompatibility"):
        run_validation_in_dir(temp_dataset)
