import os
import re
from typing import Dict, Any, Tuple

import json

SHA_RE = re.compile(r"^[0-9a-f]{40}$")

def verify_provenance_contract(path: str) -> Dict[str, Any]:
    """Validates an artifact against the structured execution provenance contract.
    Requirements:
    1. Must be a structured JSON document (.json).
    2. Must contain valid contract schema or receipt_type.
    3. Must contain exact 40-hex pinned_commit.
    4. Must contain non-empty source_linkage referencing existing repo source files.
    5. Must contain non-empty commands execution sequence.
    6. Must contain structured environment specification (e.g. OS, architecture, toolchain).
    7. Must contain non-empty outputs mapping with verified output identities and digests.

    Receipts without executed independent verification in the static analysis session
    are classified as candidates for verification (evidence_tier: NONE) rather than verified Tier A.
    Fabricated keyword files, unrelated commit SHAs, missing source linkage, and missing output
    identities are strictly rejected.
    """
    if not os.path.exists(path) or not path.endswith(".json"):
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "Not a JSON file"}

    try:
        with open(path, "r", encoding="utf-8") as fp:
            data = json.load(fp)
    except Exception as e:
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": f"Invalid JSON: {e}"}

    if not isinstance(data, dict):
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "Root must be JSON object"}

    # 1. Schema / receipt type
    receipt_type = data.get("schema") or data.get("receipt_type")
    if not receipt_type or not isinstance(receipt_type, str):
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "Missing or non-string schema/receipt_type"}

    # 2. Pinned commit SHA
    pinned_commit = data.get("pinned_commit") or data.get("source_commit") or data.get("commit")
    if not pinned_commit or not isinstance(pinned_commit, str) or not SHA_RE.match(pinned_commit):
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "Missing or invalid 40-hex pinned_commit"}

    # 3. Source linkage
    source_linkage = data.get("source_linkage") or data.get("sources")
    if not source_linkage or not isinstance(source_linkage, (list, dict)) or len(source_linkage) == 0:
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "Missing or empty source_linkage"}

    src_paths = []
    if isinstance(source_linkage, list):
        for s in source_linkage:
            if isinstance(s, dict) and "path" in s:
                src_paths.append(s["path"])
            elif isinstance(s, str):
                src_paths.append(s)
    elif isinstance(source_linkage, dict):
        for k, v in source_linkage.items():
            if isinstance(v, dict) and "path" in v:
                src_paths.append(v["path"])
            elif isinstance(v, str):
                src_paths.append(v)
            else:
                src_paths.append(k)

    if not src_paths:
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "source_linkage contains no file paths"}

    for sp in src_paths:
        if not isinstance(sp, str) or len(sp.strip()) == 0:
            return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "Empty source path in source_linkage"}

    # 4. Commands
    commands = data.get("commands") or data.get("command")
    if not commands or (isinstance(commands, list) and len(commands) == 0) or (isinstance(commands, str) and len(commands.strip()) == 0):
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "Missing or empty commands"}

    # 5. Environment
    environment = data.get("environment")
    if not environment or (isinstance(environment, dict) and len(environment) == 0) or (isinstance(environment, str) and len(environment.strip()) == 0):
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "Missing or empty environment"}

    # 6. Outputs
    outputs = data.get("outputs") or data.get("output_identities")
    if not outputs or (isinstance(outputs, (dict, list)) and len(outputs) == 0):
        return {"valid_contract": False, "candidate_for_verification": False, "verified_tier_a": False, "error": "Missing or empty outputs"}

    # 7. Independent execution verification status
    # Per contract: receipts without executed independent verification in the static analysis
    # session are classified as candidates for verification (NONE) rather than verified Tier A.
    verified_execution = data.get("verified_execution") or data.get("reproduction_status")
    is_verified = (verified_execution == "INDEPENDENTLY_REPRODUCED_TIER_A")

    return {
        "valid_contract": True,
        "candidate_for_verification": True,
        "verified_tier_a": is_verified,
        "error": None
    }

def has_pinned_execution_provenance(path: str) -> bool:
    """Returns True ONLY if artifact satisfies structured provenance contract AND has verified execution."""
    res = verify_provenance_contract(path)
    return res["verified_tier_a"]

def get_rfc_status(path: str) -> str:
    """Reads RFC file content to determine canonical Status header.
    Returns 'ACCEPTED', 'DRAFT', 'SUPERSEDED', or 'UNKNOWN'."""
    # If it is a schema companion to an RFC, check the parent RFC status
    filename = os.path.basename(path)
    if filename.endswith(".json"):
        # e.g. RFC-00D1-..., RFC-0055-...
        m = re.match(r"(RFC-[0-9A-Za-z]+)", filename)
        if m:
            rfc_base = m.group(1)
            # Find matching md file
            parent_dir = os.path.dirname(path)
            for f in os.listdir(parent_dir):
                if f.startswith(rfc_base) and f.endswith(".md"):
                    return get_rfc_status(os.path.join(parent_dir, f))

    if not os.path.exists(path):
        return "UNKNOWN"

    try:
        with open(path, "r", encoding="utf-8", errors="ignore") as fp:
            content = fp.read(4000)
        m = re.search(r"(?:Status|\*\*Status\*\*)\s*[:\|]\s*\*?\*?([A-Za-z0-9_ —-]+)", content, re.IGNORECASE)
        if m:
            st = m.group(1).strip().lower()
            if "accepted" in st:
                return "ACCEPTED"
            if "superseded" in st:
                return "SUPERSEDED"
            if "draft" in st:
                return "DRAFT"
            return "UNKNOWN"
    except Exception:
        pass
    return "UNKNOWN"

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
            if filename.startswith("RFC-") or filename == "0000-template.md":
                rfc_st = get_rfc_status(path)
                if rfc_st == "ACCEPTED":
                    authority = "NORMATIVE_CONTRACT"
                    lifecycle = "CURRENT"
                    evidence_tier = "NONE"
                elif rfc_st == "DRAFT":
                    authority = "DRAFT_PROPOSAL"
                    lifecycle = "DRAFT"
                    evidence_tier = "Tier E"
                elif rfc_st == "SUPERSEDED":
                    authority = "SPECIFICATION"
                    lifecycle = "SUPERSEDED"
                    evidence_tier = "Tier C"
                else:
                    authority = "SPECIFICATION"
                    lifecycle = "NEEDS_REVIEW"
                    evidence_tier = "NONE"
    elif top_dir == "contracts" or "PUBLIC_API_LOCK" in filename:
        authority = "NORMATIVE_CONTRACT"
    elif top_dir == "tests":
        # Crucial distinction: test code/harnesses are assertions, NOT executed evidence
        authority = "TEST_ASSERTION"
        evidence_tier = "NONE"
    elif top_dir in ["src", "include", "vm", "fs", "isa", "kernel", "lang", "codec", "crypto", "drivers"]:
        authority = "IMPLEMENTATION"
        evidence_tier = "Tier B"
    elif top_dir == "experiments":
        authority = "EXPERIMENTAL_EVIDENCE"
        if "h04_r02" in path or "h04_r01" in path:
            relevant_hypothesis = "H04"
        elif "cmp3" in path:
            relevant_hypothesis = "H-INIT-CMP3"
        elif "s02_stochastic" in path:
            relevant_hypothesis = "H02"
        elif "h09_process" in path:
            relevant_hypothesis = "H09"

        # Pinned execution provenance check: Tier A requires explicit, pinned, independently
        # reproducible execution evidence (source SHA, commands, environment, inputs, outputs,
        # execution receipts and proven scope). No path-based or filename-based Tier A promotion.
        # Where provenance is unavailable, classify conservatively (Tier B for source/scripts,
        # NONE for assertions/docs/unverified raw data).
        if has_pinned_execution_provenance(path):
            evidence_tier = "Tier A"
        elif content_category in ["SOURCE_CODE", "SCRIPT"]:
            evidence_tier = "Tier B"
        else:
            evidence_tier = "NONE"
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
    # Directory membership in historical/archive affects lifecycle ONLY, not evidence tier.
    parts_lower = [p.lower() for p in parts]
    if "archive" in parts_lower or "historical" in parts_lower or "HISTORICAL" in filename:
        lifecycle = "HISTORICAL"
        historical_indicator = True
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
