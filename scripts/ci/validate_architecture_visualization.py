#!/usr/bin/env python3
"""
validate_architecture_visualization.py
Deterministic validation script ensuring referential integrity, source existence,
line bounds, semantic compatibility, and schema correctness for docs/architecture/visualization/ datasets.
"""

import json
import os
import sys
import hashlib

DATA_DIR = "docs/architecture/visualization"
REQUIRED_FILES = [
    "architecture_layers.json",
    "architecture_nodes.json",
    "architecture_edges.json",
    "architecture_interfaces.json",
    "architecture_gaps.json",
    "execution_paths.json",
    "architecture_evidence.json",
    "visualization_layout.json",
    "subsystem_summary.json",
    "coverage_manifest.json"
]

VALID_REFERENCE_TYPES = {
    "FUNCTION_DEFINITION",
    "CALL_SITE",
    "DECLARATION",
    "COMMENT",
    "DESCRIPTIVE_FILE"
}

VALID_RELATIONSHIP_TYPES = {
    "containment",
    "function_invocation",
    "instruction_dispatch",
    "runtime_data_flow",
    "compile_time_dependency",
    "link_dependency",
    "test_coverage",
    "research_or_evidence_relationship",
    "architectural_design_relationship"
}

VALID_DERIVATION_METHODS = {
    "MANUALLY_CURATED",
    "STATICALLY_EXTRACTED"
}

VALID_EXECUTION_PROFILES = {"HOSTED", "FREESTANDING", "EMULATED", "NONE"}
VALID_TEST_COVERAGE = {"DEDICATED_SUITE", "SMOKE_ONLY", "NONE"}
VALID_QUALIFICATION = {"QUALIFIED", "QUALIFIED_WITH_LIMITATIONS", "EXPLORATORY", "UNVERIFIED"}
VALID_PRODUCTION_STATUS = {"PRODUCTION_CORE", "EXPERIMENTAL_FRONTIER", "RESEARCH_PROTOTYPE", "DESIGN_ONLY", "GOVERNANCE_AND_ASSURANCE"}

# Semantic compatibility check between reference_type and relationship_type
# (e.g. COMMENT, DECLARATION, or DESCRIPTIVE_FILE cannot substantiate runtime function_invocation or runtime_data_flow)
RUNTIME_RELATIONSHIPS = {"function_invocation", "runtime_data_flow"}

def load_json(name, base_dir=DATA_DIR):
    path = os.path.join(base_dir, name)
    if not os.path.exists(path):
        raise FileNotFoundError(f"Missing required visualization file: {path}")
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)

def check_file_and_line(filepath, line, context_desc):
    if not os.path.exists(filepath):
        raise FileNotFoundError(f"{context_desc}: Referenced file does not exist: {filepath}")
    if line is not None:
        if not isinstance(line, int) or line < 1:
            raise ValueError(f"{context_desc}: Line number must be a positive integer, got: {line}")
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            total_lines = sum(1 for _ in f)
        if line > total_lines:
            raise ValueError(f"{context_desc}: Line {line} out of bounds for {filepath} (total lines: {total_lines})")

def validate(data_dir=DATA_DIR):
    print(f"== Validating Architecture Visualization Datasets ({data_dir}) ==")

    # 1. Check all required files exist and parse as valid JSON
    data = {}
    for rf in REQUIRED_FILES:
        data[rf] = load_json(rf, base_dir=data_dir)
        print(f" [PASS] Loaded {rf}")

    layers_data = data["architecture_layers.json"]
    nodes_data = data["architecture_nodes.json"]
    edges_data = data["architecture_edges.json"]
    ifaces_data = data["architecture_interfaces.json"]
    paths_data = data["execution_paths.json"]
    layout_data = data["visualization_layout.json"]
    manifest_data = data["coverage_manifest.json"]

    layer_ids = set()
    for l in layers_data.get("layers", []):
        lid = l["id"]
        if lid in layer_ids:
            raise ValueError(f"Duplicate layer ID: {lid}")
        layer_ids.add(lid)
    assert len(layer_ids) == 7, f"Expected 7 layers, found {len(layer_ids)}"

    # 2. Validate Nodes
    node_ids = set()
    for n in nodes_data.get("nodes", []):
        nid = n["id"]
        if nid in node_ids:
            raise ValueError(f"Duplicate node ID found: {nid}")
        node_ids.add(nid)
        if n["layer_id"] not in layer_ids:
            raise ValueError(f"Node {nid} references unknown layer {n['layer_id']}")

        # Validate maturity fields and evidence
        mat = n.get("maturity", {})
        if mat.get("execution") not in VALID_EXECUTION_PROFILES:
            raise ValueError(f"Node {nid} has invalid execution profile: {mat.get('execution')}")
        if mat.get("test_coverage") not in VALID_TEST_COVERAGE:
            raise ValueError(f"Node {nid} has invalid test coverage: {mat.get('test_coverage')}")
        if mat.get("qualification") not in VALID_QUALIFICATION:
            raise ValueError(f"Node {nid} has invalid qualification: {mat.get('qualification')}")
        if mat.get("production_status") not in VALID_PRODUCTION_STATUS:
            raise ValueError(f"Node {nid} has invalid production status: {mat.get('production_status')}")

        # Check maturity evidence presence
        ev_mat = n.get("maturity_evidence", {})
        if not ev_mat:
            raise ValueError(f"Node {nid} missing maturity_evidence dictionary")
        if mat.get("qualification") in ["QUALIFIED", "QUALIFIED_WITH_LIMITATIONS"]:
            if not ev_mat.get("qualification_evidence"):
                raise ValueError(f"Node {nid} claims qualification without qualification_evidence citation")

        # Validate that cited source files exist and have bounds
        for sp in n.get("source_paths", []):
            check_file_and_line(sp, None, f"Node {nid}")

    print(f" [PASS] Validated {len(node_ids)} nodes and their source file paths.")

    # 3. Validate Edges & Referential Integrity
    edge_ids = set()
    for e in edges_data.get("edges", []):
        eid = e["id"]
        if eid in edge_ids:
            raise ValueError(f"Duplicate edge ID found: {eid}")
        edge_ids.add(eid)
        if e["source_id"] not in node_ids:
            raise ValueError(f"Edge {eid} references unknown source_id: {e['source_id']}")
        if e["dest_id"] not in node_ids:
            raise ValueError(f"Edge {eid} references unknown dest_id: {e['dest_id']}")

        rtype = e.get("relationship_type")
        if rtype not in VALID_RELATIONSHIP_TYPES:
            raise ValueError(f"Edge {eid} has invalid relationship_type: {rtype}")

        dmethod = e.get("derivation_method")
        if dmethod not in VALID_DERIVATION_METHODS:
            raise ValueError(f"Edge {eid} missing or invalid derivation_method: {dmethod}")

        # Validate edge source file evidence and line bounds
        ev = e.get("evidence", {})
        if "source_file" in ev and ev["source_file"]:
            sf = ev["source_file"]
            line = ev.get("line")
            reftype = ev.get("reference_type")
            if reftype not in VALID_REFERENCE_TYPES:
                raise ValueError(f"Edge {eid} evidence has invalid reference_type: {reftype}")
            check_file_and_line(sf, line, f"Edge {eid}")

            # Semantic compatibility enforcement:
            # COMMENT, DECLARATION, or DESCRIPTIVE_FILE cannot be used as proof of runtime invocation or dataflow
            if rtype in RUNTIME_RELATIONSHIPS and reftype in ["COMMENT", "DECLARATION", "DESCRIPTIVE_FILE"]:
                raise ValueError(
                    f"Edge {eid}: Semantic incompatibility — {reftype} reference cannot substantiate runtime {rtype}"
                )

    print(f" [PASS] Validated {len(edge_ids)} edges, derivation methods, and referential integrity.")

    # 4. Validate Interfaces
    for iface in ifaces_data.get("interfaces", []):
        if iface["provider_node_id"] not in node_ids:
            raise ValueError(f"Interface provider not in nodes: {iface['provider_node_id']}")
        for c in iface.get("consumer_node_ids", []):
            if c not in node_ids:
                raise ValueError(f"Interface consumer not in nodes: {c}")

    print(f" [PASS] Validated {len(ifaces_data.get('interfaces', []))} interfaces.")

    # 5. Validate Execution Paths
    for p in paths_data.get("paths", []):
        tmethod = p.get("tracing_method")
        if tmethod not in ["STATICALLY_TRACED", "DYNAMICALLY_OBSERVED"]:
            raise ValueError(f"Execution path {p['id']} has invalid tracing_method: {tmethod}")

        # If pathway or step claims DYNAMICALLY_OBSERVED, require valid trace evidence
        if tmethod == "DYNAMICALLY_OBSERVED":
            rtev = p.get("runtime_trace_evidence")
            if not rtev:
                raise ValueError(f"Path {p['id']} claims DYNAMICALLY_OBSERVED without runtime_trace_evidence")

        for s in p.get("steps", []):
            if s["module"] not in node_ids:
                raise ValueError(f"Execution path step references unknown node: {s['module']}")
            check_file_and_line(s["file"], s.get("line"), f"Path {p['id']} Step {s.get('step_number')}")

            # Step observation status
            obs = s.get("observation_status")
            if obs not in ["STATICALLY_SUPPORTED", "DYNAMICALLY_OBSERVED", "NOT_ESTABLISHED"]:
                raise ValueError(f"Path {p['id']} Step {s.get('step_number')} invalid observation_status: {obs}")
            if obs == "DYNAMICALLY_OBSERVED":
                rtev = p.get("runtime_trace_evidence")
                if not rtev or not os.path.exists(rtev.get("trace_identity", "")):
                    raise ValueError(f"Path {p['id']} step claims DYNAMICALLY_OBSERVED without verified trace evidence")
                # Verify trace hash
                trace_file = rtev["trace_identity"]
                expected_sha = rtev.get("sha256")
                actual_sha = hashlib.sha256(open(trace_file, "rb").read()).hexdigest()
                if expected_sha != actual_sha:
                    raise ValueError(f"Path {p['id']} trace SHA-256 mismatch: expected {expected_sha}, got {actual_sha}")

    print(f" [PASS] Validated {len(paths_data.get('paths', []))} execution pathways and trace provenance.")

    # 6. Validate Layout Positions
    positions = layout_data.get("node_positions", {})
    for nid in node_ids:
        if nid not in positions:
            raise ValueError(f"Node {nid} missing 3D layout coordinates in visualization_layout.json")

    print(f" [PASS] Validated 3D layout coordinates for all nodes.")

    # 7. Validate Coverage Manifest
    rep_dirs = manifest_data.get("repository_directories", {})
    mapped = rep_dirs.get("mapped", {})
    unmapped = rep_dirs.get("unmapped", {})
    assert len(mapped) + len(unmapped) == rep_dirs.get("total_top_level_directories"), "Directory census mismatch in coverage manifest"
    print(f" [PASS] Validated coverage manifest ({len(mapped)} mapped, {len(unmapped)} unmapped directories).")

    print("== All Visualization Data Invariants Verified Successfully! ==")

if __name__ == "__main__":
    validate()
