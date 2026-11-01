import os
import re
from typing import Dict, Any, Tuple

SHA_RE = re.compile(r"\b[0-9a-f]{40}\b")

def has_pinned_execution_provenance(path: str) -> bool:
    """Verifies whether an artifact contains explicit, pinned, independently reproducible execution evidence:
    source SHA, commands, environment, inputs, outputs, execution receipts and proven scope.
    Source code, test definitions, build configurations, and generic patches are never promoted without this."""
    if not os.path.exists(path):
        return False
    # Executable source code, tests, build configurations, and generic patches are not execution receipts
    ext = os.path.splitext(path)[1].lower()
    if ext in ['.c', '.cpp', '.cc', '.h', '.hpp', '.inl', '.cmake', '.patch', '.diff']:
        return False

    try:
        with open(path, "r", encoding="utf-8", errors="ignore") as fp:
            content = fp.read(65536)
    except Exception:
        return False

    content_lower = content.lower()
    # 1. Pinned commit SHA (40-hex)
    if not SHA_RE.search(content):
        return False

    # 2. Execution receipts / execution provenance
    has_receipt = ("execution receipt" in content_lower or
                   "execution provenance" in content_lower or
                   "provenance receipt" in content_lower or
                   "execution_receipt" in content_lower or
                   "execution_provenance" in content_lower or
                   ("receipt" in content_lower and "provenance" in content_lower))
    if not has_receipt:
        return False

    # 3. Commands
    has_cmd = ("command" in content_lower or "$ " in content or "cmake --build" in content_lower or "python3 " in content_lower or "ctest" in content_lower)
    if not has_cmd:
        return False

    # 4. Environment
    has_env = ("environment" in content_lower or "platform" in content_lower or "toolchain" in content_lower or "linux" in content_lower or "darwin" in content_lower)
    if not has_env:
        return False

    # 5. Inputs & 6. Outputs
    has_io = (("input" in content_lower or "source" in content_lower) and
              ("output" in content_lower or "result" in content_lower))
    if not has_io:
        return False

    # 7. Proven scope
    has_scope = "scope" in content_lower
    if not has_scope:
        return False

    return True

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
