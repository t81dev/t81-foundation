"""Offline artifact consistency checks. Success NEVER establishes feasibility."""
import base64
import hashlib
import json
from pathlib import Path
import re


def verify(root):
    manifest = json.loads((root / "evidence-manifest.json").read_text())
    for name, expected in manifest["files"].items():
        relative = Path(name)
        if relative.is_absolute() or ".." in relative.parts or (root / relative).is_symlink():
            raise ValueError("unsafe evidence path")
        data = (root / relative).read_bytes()
        if len(data) != expected["bytes"] or hashlib.sha256(data).hexdigest() != expected["sha256"]:
            raise ValueError("artifact identity mismatch: " + name)
    matrix = json.loads((root / "requirements-to-evidence.json").read_text())
    expected_statuses = {"F1": "PARTIALLY DEMONSTRATED", "F2": "BLOCKED", "F3": "BLOCKED",
                         "F4": "PARTIALLY DEMONSTRATED", "F5": "NOT TESTED"}
    if {row["id"]: row["status"] for row in matrix["requirements"]} != expected_statuses:
        raise ValueError("requirement classification mismatch")
    if matrix["overall_disposition"] != "FEASIBILITY BLOCKED" or any(matrix[key] is not False for key in
            ("host_qualified", "phase_c_executed", "codec_arms_executed", "freeze_lifted", "independent_reproduction")):
        raise ValueError("unsupported readiness/execution claim")
    for row in matrix["requirements"] + matrix["scoped_observations"]:
        for name in row["evidence"]:
            if not (root / name.split("#")[0]).is_file():
                raise ValueError("missing evidence reference")
    log = (root / "evidence/diagnostics.final.log").read_text()
    if not log.endswith("H04_FEASIBILITY_DONE\n") or "NATIVE_BUILD_IDENTICAL=1" not in log:
        raise ValueError("incomplete diagnostic")
    pid = re.search(r'F1_FIRST_CHILD_MARKER child=(\d+) cgroup=0::/h04-feasibility\n', log)
    if pid is None or any(marker not in log for marker in
            (f"F1_PARENT_OBSERVATION child={pid[1]} cgroup=0::/h04-feasibility\n",
             f"F1_TARGET_PROCS {pid[1]}\n", f"F1_POST_EXEC pid={pid[1]} cgroup=0::/h04-feasibility\n",
             "F4_CHILD_WAIT exited=1 code=0 proc_absent=1", "F4_SHARED_AFTER_EXIT resident=1 value=90")):
        raise ValueError("containment/exit checkpoints mismatch")
    a = re.search(r'F2_RLIMIT_RSS rss_bytes=(\d+) limit_bytes=(\d+) survived=1', log)
    b = re.search(r'F2_SHARED rss_bytes=(\d+) cgroup_current_bytes=(\d+) limit_bytes=(\d+)', log)
    if not a or not b or not int(a[1]) > int(a[2]) == 33554432 or not int(b[1]) > int(b[3]) == 33554432 > int(b[2]):
        raise ValueError("RSS counterexamples missing")
    scratch = next(json.loads(line) for line in log.splitlines() if line.startswith('{') and 'requested_native_bytes' in line)
    if not scratch["traced_peak_bytes"] < scratch["scratch_limit_bytes"] == 65536 < scratch["requested_native_bytes"]:
        raise ValueError("native tracing counterexample missing")
    observations = {row["id"]: row for row in matrix["scoped_observations"]}
    if observations["F2-RLIMIT-counterexample"]["rss_bytes"] != int(a[1]):
        raise ValueError("matrix RLIMIT observation differs from log")
    if any(observations["F2-memcg-counterexample"][key] != value for key, value in
           (("rss_bytes", int(b[1])), ("cgroup_current_bytes", int(b[2])), ("limit_bytes", int(b[3])))):
        raise ValueError("matrix cgroup observation differs from log")
    if observations["F3-tracing-counterexample"]["traced_peak_bytes"] != scratch["traced_peak_bytes"]:
        raise ValueError("matrix scratch observation differs from log")
    if scratch["complete_coverage"] is not False:
        raise ValueError("partial instrumentation labeled complete")
    raw = (root / "evidence/successful-boot.raw.log").read_bytes().replace(b'\r', b'')
    for label, name in (("FINAL", "diagnostics.final.log"), ("FAILURE", "diagnostics.first-controls-failure.log")):
        match = re.search(rb'^H04_EXPORT_' + label.encode() + rb'_BEGIN\n([A-Za-z0-9+/=\n]+)^H04_EXPORT_' + label.encode() + rb'_END$', raw, re.M)
        if not match or base64.b64decode(b''.join(match[1].split()), validate=True) != (root / "evidence" / name).read_bytes():
            raise ValueError("serial export identity mismatch")
    for name in ("containment_probe.c", "native_scratch_probe.py"):
        digest = hashlib.sha256((root / name).read_bytes()).hexdigest()
        if digest + "  " + name not in log:
            raise ValueError("executed diagnostic source differs: " + name)
    script_digest = hashlib.sha256((root / "guest_diagnostics.sh").read_bytes()).hexdigest()
    if script_digest + "  guest_diagnostics.sh" not in (root / "evidence/virtual-host-gaps.log").read_text():
        raise ValueError("executed guest script source differs")
    return {"integrity_verified": True, "overall_disposition": "FEASIBILITY BLOCKED",
            "host_qualified": False, "phase_c_executed": False,
            "notice": "Artifact consistency only; not independent reproduction or proof of feasibility"}


if __name__ == "__main__":
    print(json.dumps(verify(Path(__file__).resolve().parent), sort_keys=True))
