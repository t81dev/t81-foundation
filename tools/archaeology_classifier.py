import os
import re
from typing import Dict, Any, Tuple

def classify_artifact(path: str, size: int) -> Dict[str, Any]:
    parts = path.split('/')
    top_dir = parts[0]
    filename = parts[-1]
    ext = os.path.splitext(filename)[1].lower()

    # Default values
    subsystem = top_dir
    content_category = "OTHER"
    lifecycle = "CURRENT"
    authority = "DESCRIPTIVE"
    evidence_tier = "NONE"
    relevant_spec = None
    relevant_hypothesis = None
    historical_indicator = False
    generated_indicator = False
    proposed_disposition = "RETAIN"
    confidence = 0.95
    evidence_basis = "PATH_AND_METADATA"

    # Category determination
    if ext in ['.c', '.cpp', '.cc', '.h', '.hpp', '.inl']:
        content_category = "SOURCE_CODE"
    elif ext in ['.py', '.sh', '.bash', '.ps1', '.bat']:
        content_category = "SCRIPT"
    elif ext in ['.md', '.txt', '.rst', '.adoc']:
        content_category = "DOCUMENTATION"
    elif ext in ['.json', '.jsonl', '.yaml', '.yml', '.toml', '.tsv', '.csv']:
        content_category = "DATA_OR_CONFIG"
    elif ext in ['.cmake'] or filename in ['CMakeLists.txt', 'Makefile', 'BUILD.bazel']:
        content_category = "BUILD_CONFIG"
    elif ext in ['.t81', '.tisc', '.apl']:
        content_category = "T81_DOMAIN_ARTIFACT"
    elif ext in ['.patch', '.diff']:
        content_category = "PATCH"
    else:
        content_category = "BINARY_OR_RAW"

    # Subsystem refining
    if top_dir == "include":
        subsystem = "public_api"
    elif top_dir == "src":
        subsystem = "runtime_core"
    elif top_dir == "vm":
        subsystem = "virtual_machine"
    elif top_dir == "isa":
        subsystem = "tisc_isa"
    elif top_dir == "fs":
        subsystem = "canonfs"
    elif top_dir == "spec":
        subsystem = "specifications"
    elif top_dir == "docs":
        subsystem = "documentation"
    elif top_dir == "tests":
        subsystem = "tests"
    elif top_dir == "experiments":
        subsystem = "experiments"
    elif top_dir == "scripts":
        subsystem = "tooling_and_ci"
    elif top_dir == "tools":
        subsystem = "developer_tools"
    elif top_dir == "experimental":
        subsystem = "experimental_incubator"
    elif top_dir == "ternaryos":
        subsystem = "ternaryos_exploration"

    # Authority and Evidence Tier
    if top_dir == "spec":
        authority = "SPECIFICATION"
        if "rfcs" in parts:
            if "RFC-00" in filename:
                # Check draft RFCs
                if any(x in filename for x in ["RFC-00D", "RFC-00E", "RFC-00C8"]):
                    lifecycle = "DRAFT"
                    authority = "DRAFT_PROPOSAL"
                    evidence_tier = "Tier E"
                else:
                    authority = "NORMATIVE_CONTRACT"
                    lifecycle = "CURRENT"
    elif top_dir == "contracts" or "PUBLIC_API_LOCK" in filename:
        authority = "NORMATIVE_CONTRACT"
    elif top_dir == "tests":
        authority = "TEST_ASSERTION"
        evidence_tier = "Tier A"
    elif top_dir in ["src", "include", "vm", "fs", "isa", "kernel", "lang", "codec", "crypto", "drivers"]:
        authority = "IMPLEMENTATION"
        evidence_tier = "Tier B"
    elif top_dir == "experiments":
        authority = "EXPERIMENTAL_EVIDENCE"
        if "h04_r02" in path or "h04_r01" in path:
            relevant_hypothesis = "H04"
            evidence_tier = "Tier A"
        elif "cmp3" in path:
            relevant_hypothesis = "H-INIT-CMP3"
            evidence_tier = "Tier B"
        elif "durable_witness" in path:
            evidence_tier = "Tier A"
        elif "s02_stochastic" in path:
            relevant_hypothesis = "H02"
            evidence_tier = "Tier A"
        elif "h09_process" in path:
            relevant_hypothesis = "H09"
            evidence_tier = "Tier A"
    elif top_dir == "scripts":
        if "governance" in parts or "ci" in parts:
            authority = "GOVERNANCE_VALIDATOR"
        else:
            authority = "UTILITY"

    # Hypothesis association
    if "h01" in path.lower() or "investigation_10a" in path.lower():
        relevant_hypothesis = "H01"
    elif "h02" in path.lower() or "s02" in path.lower():
        relevant_hypothesis = "H02"
    elif "h03" in path.lower():
        relevant_hypothesis = "H03"
    elif "h04" in path.lower():
        relevant_hypothesis = "H04"
    elif "h05" in path.lower():
        relevant_hypothesis = "H05"
    elif "h06" in path.lower():
        relevant_hypothesis = "H06"
    elif "h07" in path.lower():
        relevant_hypothesis = "H07"
    elif "h08" in path.lower():
        relevant_hypothesis = "H08"

    # RFC association
    m = re.search(r"RFC-([0-9A-Za-z]+)", path)
    if m:
        relevant_spec = f"RFC-{m.group(1)}"

    # Lifecycle classification refining
    parts_lower = [p.lower() for p in parts]
    if "archive" in parts_lower or "historical" in parts_lower or "HISTORICAL" in filename:
        lifecycle = "HISTORICAL"
        historical_indicator = True
        evidence_tier = "Tier C" if evidence_tier == "NONE" else evidence_tier
    elif "provisional" in parts_lower or "draft" in parts_lower:
        lifecycle = "DRAFT"
    elif "generated" in parts_lower or filename.endswith(".log") or filename in ["test_record.json", "record_hash.txt", "test_input.txt"]:
        generated_indicator = True
        lifecycle = "GENERATED"
    elif "superseded" in parts_lower or "SUPERSEDED" in filename:
        lifecycle = "SUPERSEDED"

    # Root anomalies check
    if len(parts) == 1:
        if filename in ["test_input.txt", "test_record.json", "record_hash.txt", "build.log", "import.apl", "tiny-model-policy.apl", "symbolic_algebraic.tisc", "symbolic_algebraic_exported.tisc"]:
            lifecycle = "HISTORICAL" if not generated_indicator else "GENERATED"
            proposed_disposition = "RELOCATE_OR_CLEANUP"
            confidence = 0.90
            evidence_basis = "ROOT_ANOMALY"

    return {
        "subsystem": subsystem,
        "content_category": content_category,
        "lifecycle": lifecycle,
        "authority": authority,
        "evidence_tier": evidence_tier,
        "relevant_spec": relevant_spec,
        "relevant_hypothesis": relevant_hypothesis,
        "historical_indicator": historical_indicator,
        "generated_indicator": generated_indicator,
        "proposed_disposition": proposed_disposition,
        "confidence": confidence,
        "evidence_basis": evidence_basis
    }
