#!/usr/bin/env python3
"""
validate_architecture_visualization.py
Deterministic validation script ensuring referential integrity, source existence,
line bounds, semantic compatibility, trace provenance, and schema correctness
for docs/architecture/visualization/ datasets.
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
    "coverage_manifest.json",
    "component_evidence_index.json",
    "research_programs.json",
    "pull_requests_snapshot.json"
]

DATASET_SCOPES = {
    "architecture_layers.json": "7-layer architectural taxonomy (L1 Language to L7 Assurance)",
    "architecture_nodes.json": "35 architectural components across 7 platform layers",
    "architecture_edges.json": "32 technical dependency edges and source line evidence anchors",
    "architecture_interfaces.json": "6 public system-level ABI and API interface contracts",
    "architecture_gaps.json": "4 documented architectural and target-system gaps",
    "execution_paths.json": "9 statically traced execution pathways and dispatch sequences",
    "architecture_evidence.json": "Primary architectural citations and unresolved questions",
    "visualization_layout.json": "3D isometric layout coordinates and layer elevations",
    "subsystem_summary.json": "Aggregated subsystem graph (13 subsystems, 11 summary edges)",
    "coverage_manifest.json": "Top-level directory representation census (18/35 directories, 51.43%)",
    "component_evidence_index.json": "Evidence, test files, and continuity qualification receipts across 35 components",
    "research_programs.json": "Historical research progress snapshot across 6 programs and H01-H08 mappings",
    "pull_requests_snapshot.json": "Point-in-time snapshot of 18 cataloged PRs mapped to subsystems"
}

VALID_REFERENCE_TYPES = {
    "FUNCTION_DEFINITION",
    "CALL_SITE",
    "DISPATCH_SITE",
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

VALID_EVIDENCE_RELATIONSHIPS = {"VERIFIED", "STATICALLY_SUPPORTED", "UNVERIFIED", "NOT_MAPPED"}

RUNTIME_RELATIONSHIPS = {"function_invocation", "runtime_data_flow"}

REQUIRED_TRACE_PROVENANCE_FIELDS = [
    "trace_identity",
    "sha256",
    "producing_command",
    "producing_test",
    "pinned_baseline",
    "observation_scope"
]

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

def validate(data_dir=DATA_DIR, allow_missing_evidence_index=False):
    print(f"== Validating Architecture Visualization Datasets ({data_dir}) ==")

    # 1. Check all required files exist and parse as valid JSON
    data = {}
    files_to_check = [f for f in REQUIRED_FILES if not (f == "component_evidence_index.json" and allow_missing_evidence_index)]
    for rf in files_to_check:
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
    if len(layer_ids) != 7:
        raise ValueError(f"Expected 7 layers, found {len(layer_ids)}")

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

# Check maturity evidence presence and reject tautological claims
        ev_mat = n.get("maturity_evidence", {})
        if not ev_mat:
            raise ValueError(f"Node {nid} missing maturity_evidence dictionary")

        prod_ev = ev_mat.get("production_evidence", "").strip()
        if not prod_ev or prod_ev.lower().startswith("audited production_status:"):
            raise ValueError(f"Node {nid} contains self-referential or tautological production_evidence: '{prod_ev}'")

        qual_ev = ev_mat.get("qualification_evidence", "").strip()
        if mat.get("qualification") in ["QUALIFIED", "QUALIFIED_WITH_LIMITATIONS"]:
            if not qual_ev or qual_ev.lower() in ["qualified", "qualified with limitations", "unverified", "exploratory"]:
                raise ValueError(f"Node {nid} claims qualification without concrete inspectable qualification_evidence citation")

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

    # 5. Validate Execution Paths (Strict Dynamic Trace Enforcement)
    for p in paths_data.get("paths", []):
        tmethod = p.get("tracing_method")
        if tmethod not in ["STATICALLY_TRACED", "DYNAMICALLY_OBSERVED"]:
            raise ValueError(f"Execution path {p['id']} has invalid tracing_method: {tmethod}")

        has_dynamic_step = any(s.get("observation_status") == "DYNAMICALLY_OBSERVED" for s in p.get("steps", []))

        # Enforce trace validation whenever either the pathway OR any step claims DYNAMICALLY_OBSERVED
        if tmethod == "DYNAMICALLY_OBSERVED" or has_dynamic_step or p.get("runtime_trace_evidence"):
            rtev = p.get("runtime_trace_evidence")
            if not rtev:
                raise ValueError(
                    f"Path {p['id']} claims dynamic trace observation (pathway={tmethod}, has_dynamic_step={has_dynamic_step}) "
                    "but lacks runtime_trace_evidence provenance block"
                )
            for req_field in REQUIRED_TRACE_PROVENANCE_FIELDS:
                if req_field not in rtev or not str(rtev[req_field]).strip():
                    raise ValueError(f"Path {p['id']} runtime_trace_evidence missing required field: {req_field}")

            trace_file = rtev["trace_identity"]
            if not os.path.exists(trace_file):
                raise FileNotFoundError(f"Path {p['id']} trace file not found: {trace_file}")

            expected_sha = rtev.get("sha256")
            actual_sha = hashlib.sha256(open(trace_file, "rb").read()).hexdigest()
            if expected_sha != actual_sha:
                raise ValueError(f"Path {p['id']} trace SHA-256 mismatch: expected {expected_sha}, got {actual_sha}")

        for s in p.get("steps", []):
            if s["module"] not in node_ids:
                raise ValueError(f"Execution path step references unknown node: {s['module']}")
            check_file_and_line(s["file"], s.get("line"), f"Path {p['id']} Step {s.get('step_number')}")

            # Dispatch action semantics: control-flow switch/dispatch sites must be classified as DISPATCH_SITE
            action_lower = s.get("action", "").lower()
            if ("switch" in action_lower or "dispatch" in action_lower) and s.get("reference_type") in {"FUNCTION_DEFINITION", "CALL_SITE"}:
                # Verify if target line is a switch/dispatch statement in source
                try:
                    with open(s["file"], "r", encoding="utf-8") as src_f:
                        src_lines = src_f.readlines()
                    target_line = src_lines[s["line"] - 1].strip()
                    if target_line.startswith("switch") or "switch " in target_line:
                        raise ValueError(
                            f"Path {p['id']} Step {s.get('step_number')}: control flow dispatch switch statement at {s['file']}:{s['line']} must be classified as DISPATCH_SITE, not {s.get('reference_type')}"
                        )
                except (IndexError, FileNotFoundError):
                    pass


            # Do not allow COMMENT to serve as an executable path step anchor
            s_reftype = s.get("reference_type")
            if s_reftype == "COMMENT":
                raise ValueError(
                    f"Path {p['id']} Step {s.get('step_number')}: COMMENT reference cannot serve as execution step anchor"
                )

            obs = s.get("observation_status")
            if obs not in ["STATICALLY_SUPPORTED", "DYNAMICALLY_OBSERVED", "NOT_ESTABLISHED"]:
                raise ValueError(f"Path {p['id']} Step {s.get('step_number')} invalid observation_status: {obs}")

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
    if len(mapped) + len(unmapped) != rep_dirs.get("total_top_level_directories"):
        raise ValueError("Directory census mismatch in coverage manifest")
    cov_metrics = manifest_data.get("coverage_metrics", {})
    if "source_code_coverage_percentage" in cov_metrics:
        raise ValueError("Coverage manifest uses incorrect key 'source_code_coverage_percentage'; must be 'top_level_directory_representation_percentage'")
    if "top_level_directory_representation_percentage" not in cov_metrics:
        raise ValueError("Coverage manifest missing 'top_level_directory_representation_percentage'")

    print(f" [PASS] Validated coverage manifest ({len(mapped)} mapped, {len(unmapped)} unmapped directories).")

    # 8. Validate Component Evidence Index (Mandatory by default)
    evidence_index_file = "component_evidence_index.json"
    evidence_path = os.path.join(data_dir, evidence_index_file)
    if not os.path.exists(evidence_path):
        if not allow_missing_evidence_index:
            raise FileNotFoundError(
                f"Missing mandatory evidence index: {evidence_path}. "
                "Phase 1 requires component_evidence_index.json. Use --allow-missing-evidence-index for legacy mode."
            )
    else:
        ev_data = load_json(evidence_index_file, base_dir=data_dir)
        validate_component_evidence_index(ev_data, nodes_data.get("nodes", []), layer_ids)
        print(" [PASS] Validated component evidence index, summary reconciliation, and relationship classifications.")

    print("== All Visualization Data Invariants Verified Successfully! ==")

def validate_component_evidence_index(data, nodes, layer_ids):
    if not isinstance(data, dict):
        raise ValueError("component_evidence_index.json must be a JSON object")
    if "components" not in data:
        raise ValueError("component_evidence_index.json missing 'components' field")

    comps = data["components"]
    if not isinstance(comps, list):
        raise ValueError("'components' field must be a list")

    node_map = {n["id"]: n for n in nodes}
    node_ids = set(node_map.keys())

    # Summary reconciliation
    total_components = data.get("total_components")
    if total_components != len(comps):
        raise ValueError(
            f"Census mismatch in component_evidence_index.json: total_components={total_components} does not match len(components)={len(comps)}"
        )

    summary = data.get("classification_summary")
    if not isinstance(summary, dict):
        raise ValueError("component_evidence_index.json missing or invalid 'classification_summary' dictionary")

    recomputed_counts = {rel: 0 for rel in VALID_EVIDENCE_RELATIONSHIPS}

    seen_ids = set()
    for c in comps:
        cid = c.get("component_id")
        if not cid:
            raise ValueError("Component evidence entry missing 'component_id'")
        if cid in seen_ids:
            raise ValueError(f"Duplicate component_id in evidence index: {cid}")
        seen_ids.add(cid)
        if cid not in node_ids:
            raise ValueError(f"Component evidence references unknown node_id: {cid}")

        # Validate layer_id against layers
        layer_id = c.get("layer_id")
        if layer_id not in layer_ids:
            raise ValueError(f"Component {cid} references invalid layer_id: {layer_id}")
        if layer_id != node_map[cid]["layer_id"]:
            raise ValueError(f"Component {cid} layer_id mismatch: expected {node_map[cid]['layer_id']}, got {layer_id}")

        rel = c.get("relationship_classification")
        if rel not in VALID_EVIDENCE_RELATIONSHIPS:
            raise ValueError(f"Component {cid} has invalid relationship_classification: {rel}")
        recomputed_counts[rel] += 1

        test_files = c.get("test_files", [])
        receipt_files = c.get("receipt_files", [])

        # Check files referenced in evidence exist
        for tf in test_files:
            if not os.path.exists(tf):
                raise FileNotFoundError(f"Component {cid}: test file does not exist: {tf}")
        for rf in receipt_files:
            if not os.path.exists(rf):
                raise FileNotFoundError(f"Component {cid}: receipt file does not exist: {rf}")

        # Classification Integrity: Enforce prerequisites for VERIFIED promotion
        # VERIFIED requires:
        # 1. Non-empty receipt_files citing authoritative qualification receipts
        # 2. Non-empty test_files demonstrating executable test coverage
        # 3. Corresponding node maturity qualification must be QUALIFIED or QUALIFIED_WITH_LIMITATIONS
        if rel == "VERIFIED":
            if not receipt_files:
                raise ValueError(
                    f"Component {cid}: VERIFIED classification requires at least one authoritative qualification receipt in receipt_files"
                )
            if not test_files:
                raise ValueError(
                    f"Component {cid}: VERIFIED classification requires executable test verification in test_files"
                )
            node_qual = node_map[cid].get("maturity", {}).get("qualification")
            if node_qual not in ["QUALIFIED", "QUALIFIED_WITH_LIMITATIONS"]:
                raise ValueError(
                    f"Component {cid}: Unsupported promotion to VERIFIED. Node maturity qualification is '{node_qual}', must be QUALIFIED or QUALIFIED_WITH_LIMITATIONS"
                )

    # Reconcile summary counts
    for rel, expected_count in recomputed_counts.items():
        declared_count = summary.get(rel, 0)
        if declared_count != expected_count:
            raise ValueError(
                f"Summary reconciliation discrepancy for {rel}: declared {declared_count}, recomputed {expected_count}"
            )

    # Enforce all node_ids are accounted for in the evidence index
    missing_nodes = node_ids - seen_ids
    if missing_nodes:
        raise ValueError(f"Component evidence index missing entries for nodes: {sorted(missing_nodes)}")

def detect_drift(data_dir=DATA_DIR):
    """
    Performs deterministic architecture drift detection against the current repository tree.
    Returns a dictionary of drift anomalies:
      - missing_source_paths
      - stale_line_references
      - unsupported_reference_types
      - record_inconsistencies
    """
    anomalies = {
        "missing_source_paths": [],
        "stale_line_references": [],
        "unsupported_reference_types": [],
        "record_inconsistencies": []
    }

    nodes_data = load_json("architecture_nodes.json", base_dir=data_dir)
    edges_data = load_json("architecture_edges.json", base_dir=data_dir)
    paths_data = load_json("execution_paths.json", base_dir=data_dir)
    gaps_data = load_json("architecture_gaps.json", base_dir=data_dir)
    ifaces_data = load_json("architecture_interfaces.json", base_dir=data_dir)

    node_ids = {n["id"]: n for n in nodes_data.get("nodes", [])}

    # 1. Check Node source paths
    for nid, node in node_ids.items():
        for sp in node.get("source_paths", []):
            if not os.path.exists(sp):
                anomalies["missing_source_paths"].append({
                    "context": f"Node {nid}",
                    "path": sp
                })

    # 2. Check Edge references
    for e in edges_data.get("edges", []):
        eid = e["id"]
        ev = e.get("evidence", {})
        sf = ev.get("source_file")
        if sf:
            if not os.path.exists(sf):
                anomalies["missing_source_paths"].append({
                    "context": f"Edge {eid}",
                    "path": sf
                })
            else:
                reftype = ev.get("reference_type")
                if reftype not in VALID_REFERENCE_TYPES:
                    anomalies["unsupported_reference_types"].append({
                        "context": f"Edge {eid}",
                        "reference_type": reftype,
                        "file": sf
                    })
                lstart = ev.get("line_start")
                lend = ev.get("line_end")
                with open(sf, "r", encoding="utf-8", errors="replace") as f:
                    total_lines = sum(1 for _ in f)
                if lstart is not None and (lstart < 1 or lstart > total_lines):
                    anomalies["stale_line_references"].append({
                        "context": f"Edge {eid}",
                        "file": sf,
                        "line": lstart,
                        "total_lines": total_lines
                    })
                if lend is not None and (lend < 1 or lend > total_lines):
                    anomalies["stale_line_references"].append({
                        "context": f"Edge {eid}",
                        "file": sf,
                        "line": lend,
                        "total_lines": total_lines
                    })

    # 3. Check Execution Paths
    for p in paths_data.get("paths", []):
        pid = p["id"]
        for s in p.get("steps", []):
            snum = s.get("step_number")
            sf = s.get("file")
            line = s.get("line")
            reftype = s.get("reference_type")
            ctx = f"Path {pid} Step {snum}"

            if not os.path.exists(sf):
                anomalies["missing_source_paths"].append({
                    "context": ctx,
                    "path": sf
                })
            else:
                with open(sf, "r", encoding="utf-8", errors="replace") as f:
                    total_lines = sum(1 for _ in f)
                if line is not None and (line < 1 or line > total_lines):
                    anomalies["stale_line_references"].append({
                        "context": ctx,
                        "file": sf,
                        "line": line,
                        "total_lines": total_lines
                    })

            if reftype not in VALID_REFERENCE_TYPES:
                anomalies["unsupported_reference_types"].append({
                    "context": ctx,
                    "reference_type": reftype,
                    "file": sf
                })

            if s.get("module") not in node_ids:
                anomalies["record_inconsistencies"].append({
                    "context": ctx,
                    "issue": f"Referenced module {s.get('module')} not found in nodes"
                })

    # 4. Check Gaps
    for g in gaps_data.get("gaps", []):
        gid = g["id"]
        ev = g.get("evidence", {})
        sf = ev.get("source_file")
        line = ev.get("line")
        ctx = f"Gap {gid}"
        if sf:
            if not os.path.exists(sf):
                anomalies["missing_source_paths"].append({
                    "context": ctx,
                    "path": sf
                })
            elif line is not None:
                with open(sf, "r", encoding="utf-8", errors="replace") as f:
                    total_lines = sum(1 for _ in f)
                if line < 1 or line > total_lines:
                    anomalies["stale_line_references"].append({
                        "context": ctx,
                        "file": sf,
                        "line": line,
                        "total_lines": total_lines
                    })

    # 5. Check Interfaces
    for iface in ifaces_data.get("interfaces", []):
        iid = iface["id"]
        prov = iface.get("provider_node_id")
        if prov not in node_ids:
            anomalies["record_inconsistencies"].append({
                "context": f"Interface {iid}",
                "issue": f"Provider node {prov} not found in nodes"
            })
        for c in iface.get("consumer_node_ids", []):
            if c not in node_ids:
                anomalies["record_inconsistencies"].append({
                    "context": f"Interface {iid}",
                    "issue": f"Consumer node {c} not found in nodes"
                })

    # 6. Check Component Evidence Index Paths
    evidence_index_file = "component_evidence_index.json"
    ev_path = os.path.join(data_dir, evidence_index_file)
    if os.path.exists(ev_path):
        ev_data = load_json(evidence_index_file, base_dir=data_dir)
        for c in ev_data.get("components", []):
            cid = c.get("component_id", "unknown")
            for tf in c.get("test_files", []):
                if not os.path.exists(tf):
                    anomalies["missing_source_paths"].append({
                        "context": f"Component {cid} test_files",
                        "path": tf
                    })
            for rf in c.get("receipt_files", []):
                if not os.path.exists(rf):
                    anomalies["missing_source_paths"].append({
                        "context": f"Component {cid} receipt_files",
                        "path": rf
                    })

    return anomalies

def check_git_ancestry(baseline_sha, comparison_ref="HEAD", repo_root="."):
    """
    Evaluates Git ancestry relationship between baseline_sha and comparison_ref.
    Returns: (ancestry_status, commits_behind, resolved_comparison_sha)
      - ancestry_status: 'IDENTICAL' | 'ANCESTOR' | 'DIVERGENT' | 'UNAVAILABLE'
      - commits_behind: int (only if 'ANCESTOR' or 'IDENTICAL'), otherwise None
      - resolved_comparison_sha: full SHA string or None
    DIVERGENT means merge-base exited 1 (confirmed non-ancestry).
    UNAVAILABLE includes command failures and unavailable/invalid commit counts,
    even when ancestry was confirmed; ANCESTOR always includes an integer count.
    Handles missing Git, shallow clones, or unknown refs gracefully without raising exceptions.
    """
    import subprocess

    if not baseline_sha or baseline_sha == "Not recorded":
        return ("UNAVAILABLE", None, None)

    try:
        resolved_sha = subprocess.check_output(
            ["git", "rev-parse", comparison_ref],
            cwd=repo_root,
            stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()
    except Exception:
        return ("UNAVAILABLE", None, None)

    if baseline_sha == resolved_sha:
        return ("IDENTICAL", 0, resolved_sha)

    # Check if baseline_sha exists as a commit object in git
    try:
        subprocess.check_call(
            ["git", "cat-file", "-e", f"{baseline_sha}^{{commit}}"],
            cwd=repo_root,
            stderr=subprocess.DEVNULL
        )
    except Exception:
        return ("UNAVAILABLE", None, resolved_sha)

    # Check if baseline_sha is an ancestor of comparison_ref
    try:
        subprocess.check_call(
            ["git", "merge-base", "--is-ancestor", baseline_sha, comparison_ref],
            cwd=repo_root,
            stderr=subprocess.DEVNULL
        )
    except subprocess.CalledProcessError as exc:
        status = "DIVERGENT" if exc.returncode == 1 else "UNAVAILABLE"
        return (status, None, resolved_sha)
    except OSError:
        return ("UNAVAILABLE", None, resolved_sha)

    # Ancestry is confirmed. A counting failure is not evidence of divergence.
    try:
        count_str = subprocess.check_output(
            ["git", "rev-list", "--count", f"{baseline_sha}..{comparison_ref}"],
            cwd=repo_root,
            stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()
        return ("ANCESTOR", int(count_str), resolved_sha)
    except (subprocess.CalledProcessError, OSError, ValueError):
        return ("UNAVAILABLE", None, resolved_sha)


def collect_baseline_inventory(data_dir=DATA_DIR, comparison_ref="HEAD", repo_root="."):
    """
    Collects recorded baseline and timestamp metadata across all 13 required visualization datasets.
    Missing metadata is explicitly normalized to 'Not recorded'.
    Evaluates Git ancestry against comparison_ref when available.
    """
    inventory = []
    for fname in REQUIRED_FILES:
        fpath = os.path.join(data_dir, fname)
        if not os.path.exists(fpath):
            inventory.append({
                "filename": fname,
                "recorded_baseline": "Not recorded",
                "recorded_timestamp": "Not recorded",
                "scope": DATASET_SCOPES.get(fname, "Not recorded"),
                "ancestry_status": "UNAVAILABLE",
                "commits_behind": None
            })
            continue

        try:
            with open(fpath, "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception:
            data = {}

        baseline = (
            data.get("baseline_commit") or
            data.get("authoritative_baseline_commit") or
            data.get("pinned_baseline") or
            "Not recorded"
        )
        timestamp = (
            data.get("snapshot_timestamp") or
            data.get("generated_at") or
            data.get("verification_timestamp") or
            data.get("timestamp") or
            "Not recorded"
        )
        scope = DATASET_SCOPES.get(fname, data.get("description", "Not recorded"))

        ancestry_status, commits_behind, resolved_sha = check_git_ancestry(
            baseline, comparison_ref=comparison_ref, repo_root=repo_root
        )

        inventory.append({
            "filename": fname,
            "recorded_baseline": baseline,
            "recorded_timestamp": timestamp,
            "scope": scope,
            "ancestry_status": ancestry_status,
            "commits_behind": commits_behind
        })

    import subprocess
    try:
        resolved_comp_sha = subprocess.check_output(
            ["git", "rev-parse", comparison_ref],
            cwd=repo_root,
            stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()
    except Exception:
        resolved_comp_sha = None

    return {
        "comparison_ref": comparison_ref,
        "resolved_comparison_sha": resolved_comp_sha,
        "inventory": inventory
    }

def print_baseline_inventory(inv_data):
    ref = inv_data.get("comparison_ref", "HEAD")
    sha = inv_data.get("resolved_comparison_sha")
    sha_str = f" ({sha})" if sha else " (Git unavailable or unresolved)"
    print(f"\n== Dataset Baseline & Provenance Inventory ==")
    print(f"Comparison Reference: {ref}{sha_str}")
    print("-" * 115)
    print(f"{'Filename':<32} {'Recorded Baseline':<42} {'Timestamp':<22} {'Ancestry':<12} {'Behind':<8}")
    print("-" * 115)
    distinct_baselines = set()
    for item in inv_data["inventory"]:
        fname = item["filename"]
        b = item["recorded_baseline"]
        t = item["recorded_timestamp"]
        anc = item["ancestry_status"]
        behind = str(item["commits_behind"]) if item["commits_behind"] is not None else "-"
        if b != "Not recorded":
            distinct_baselines.add(b)
        print(f"{fname:<32} {b:<42} {t:<22} {anc:<12} {behind:<8}")
    print("-" * 115)
    print(f"Total datasets cataloged: {len(inv_data['inventory'])}")
    print(f"Distinct historical baselines: {len(distinct_baselines)}")
    print("Note: Datasets represent recorded offline research observations. Mixed historical baselines")
    print("or snapshot age do not constitute integrity defects or invalidate historical qualification.\n")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Validate Architecture Visualization Datasets and Detect Drift")
    parser.add_argument("--data-dir", default=DATA_DIR, help="Path to visualization dataset directory")
    parser.add_argument("--drift-check", action="store_true", help="Perform architecture drift detection")
    parser.add_argument("--allow-missing-evidence-index", action="store_true", help="Explicit legacy mode permitting omitted component_evidence_index.json")
    parser.add_argument("--comparison-ref", default="HEAD", help="Git reference for ancestry comparison (default: HEAD)")
    parser.add_argument("--report-inventory", action="store_true", help="Output dataset baseline provenance inventory")
    args = parser.parse_args()

    validate(args.data_dir, allow_missing_evidence_index=args.allow_missing_evidence_index)
    if args.drift_check or args.report_inventory:
        inv_data = collect_baseline_inventory(data_dir=args.data_dir, comparison_ref=args.comparison_ref)
        print_baseline_inventory(inv_data)

    if args.drift_check:
        drift = detect_drift(args.data_dir)
        total_anomalies = sum(len(v) for v in drift.values())
        print(f"== Architecture Drift Detection Results ==")
        print(f"Missing source paths: {len(drift['missing_source_paths'])}")
        print(f"Stale line references: {len(drift['stale_line_references'])}")
        print(f"Unsupported reference types: {len(drift['unsupported_reference_types'])}")
        print(f"Record inconsistencies: {len(drift['record_inconsistencies'])}")
        if total_anomalies > 0:
            print("\nDrift detected:")
            for category, items in drift.items():
                if items:
                    print(f"  {category}:")
                    for item in items:
                        print(f"    - {item}")
            sys.exit(1)
        else:
            print(" [PASS] Zero drift detected against repository tree.")
