"""Read-only checks, run from this candidate repository's root."""
import hashlib
import json
import re
import subprocess
from pathlib import Path

BASE = "4d5f3586c04a69c520e613166291c0974877628d"
AUDIT = Path(__file__).parent


def pinned(revision, path):
    return subprocess.check_output(["git", "show", f"{revision}:{path}"])


def assertions(source):
    return re.findall(r"SCG08_REQUIRE\((.*?)\);", source, re.S)


def norm(source):
    return re.sub(r"\s+", "", source)


path = Path("experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp")
before = assertions(pinned(BASE, path).decode())
after = assertions(path.read_text())
removed = [a for a in before if norm(a) not in {norm(b) for b in after}]
assert len(removed) == 6, removed
assert all(any(marker in a for marker in (
    "Pipe creation", "Fork failed", "WIFEXITED", "stderr_output.find"
)) for a in removed), removed

unchanged = {}
for name in ("exact_oracle_adapter.hpp", "tier1_markov_harness.hpp",
             "tier2_work_harness.hpp", "GATE1_IMPLEMENTATION_REPORT.md",
             "gate1_evidence_record.json"):
    path = Path("experiments/scg08_arithmetic_work") / name
    data = path.read_bytes()
    assert data == pinned(BASE, path), path
    unchanged[str(path)] = hashlib.sha256(data).hexdigest()
path = Path("spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md")
assert path.read_bytes() == pinned(BASE, path)
unchanged[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
assert subprocess.check_output(["git", "diff", BASE, "--", "include", "src"]) == b""

record = json.loads((AUDIT / "preservation.json").read_text())
for entry in record["files"]:
    data = Path(entry["retained_path"]).read_bytes()
    assert hashlib.sha256(data).hexdigest() == entry["sha256"]
    assert data == pinned(record["pr778_revision"], entry["source_path"])
print(json.dumps({
    "base_sha": BASE,
    "unchanged_sha256": unchanged,
    "production_diff_empty": True,
    "preserved_original_files": len(record["files"]),
    "retained_assertion_invocations_including_macro": len(after),
    "removed_posix_process_assertions": [re.sub(r"\s+", " ", a) for a in removed],
    "replacement_checks": "Python requires exact exit 1 and correct stderr diagnostic",
}, indent=2))
