#!/usr/bin/env python3
"""
validate_architecture_visualization.py
Deterministic validation script ensuring referential integrity, source existence,
and schema correctness for docs/architecture/visualization/ datasets.
"""

import json
import os
import sys

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
    "subsystem_summary.json"
]

def load_json(name):
    path = os.path.join(DATA_DIR, name)
    if not os.path.exists(path):
        raise FileNotFoundError(f"Missing required visualization file: {path}")
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)

def validate():
    print("== Validating Architecture Visualization Datasets ==")

    # 1. Check all required files exist and parse as valid JSON
    data = {}
    for rf in REQUIRED_FILES:
        data[rf] = load_json(rf)
        print(f" [PASS] Loaded {rf}")

    layers_data = data["architecture_layers.json"]
    nodes_data = data["architecture_nodes.json"]
    edges_data = data["architecture_edges.json"]
    ifaces_data = data["architecture_interfaces.json"]
    paths_data = data["execution_paths.json"]
    layout_data = data["visualization_layout.json"]

    layer_ids = set(l["id"] for l in layers_data.get("layers", []))
    assert len(layer_ids) == 7, f"Expected 7 layers, found {len(layer_ids)}"

    # 2. Validate Nodes
    node_ids = set()
    for n in nodes_data.get("nodes", []):
        nid = n["id"]
        assert nid not in node_ids, f"Duplicate node ID found: {nid}"
        node_ids.add(nid)
        assert n["layer_id"] in layer_ids, f"Node {nid} references unknown layer {n['layer_id']}"
        # Validate that cited source files exist
        for sp in n.get("source_paths", []):
            assert os.path.exists(sp), f"Node {nid} cites non-existent source file: {sp}"

    print(f" [PASS] Validated {len(node_ids)} nodes and their source file paths.")

    # 3. Validate Edges & Referential Integrity
    edge_ids = set()
    for e in edges_data.get("edges", []):
        eid = e["id"]
        assert eid not in edge_ids, f"Duplicate edge ID found: {eid}"
        edge_ids.add(eid)
        assert e["source_id"] in node_ids, f"Edge {eid} references unknown source_id: {e['source_id']}"
        assert e["dest_id"] in node_ids, f"Edge {eid} references unknown dest_id: {e['dest_id']}"
        # Validate edge source file evidence
        ev = e.get("evidence", {})
        if "source_file" in ev and ev["source_file"]:
            assert os.path.exists(ev["source_file"]), f"Edge {eid} cites non-existent source file: {ev['source_file']}"

    print(f" [PASS] Validated {len(edge_ids)} edges and referential integrity.")

    # 4. Validate Interfaces
    for iface in ifaces_data.get("interfaces", []):
        assert iface["provider_node_id"] in node_ids, f"Interface provider not in nodes: {iface['provider_node_id']}"
        for c in iface.get("consumer_node_ids", []):
            assert c in node_ids, f"Interface consumer not in nodes: {c}"

    print(f" [PASS] Validated {len(ifaces_data.get('interfaces', []))} interfaces.")

    # 5. Validate Execution Paths
    for p in paths_data.get("paths", []):
        for s in p.get("steps", []):
            assert s["module"] in node_ids, f"Execution path step references unknown node: {s['module']}"
            assert os.path.exists(s["file"]), f"Execution path step cites non-existent file: {s['file']}"

    print(f" [PASS] Validated {len(paths_data.get('paths', []))} execution pathways.")

    # 6. Validate Layout Positions
    positions = layout_data.get("node_positions", {})
    for nid in node_ids:
        assert nid in positions, f"Node {nid} missing 3D layout coordinates in visualization_layout.json"

    print(f" [PASS] Validated 3D layout coordinates for all nodes.")
    print("== All Visualization Data Invariants Verified Successfully! ==")

if __name__ == "__main__":
    validate()
