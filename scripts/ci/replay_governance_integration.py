#!/usr/bin/env python3
"""Independent command-level replay of the retained 27-command governance inventory.

Runs every command separately; never interprets author-reported exit codes as
new execution results. Output is diagnostic evidence, not a passing quality gate.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

SOURCE = Path("docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default="governance-integration-replay.json")
    parser.add_argument("--timeout", type=int, default=180)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    source_bytes = SOURCE.read_bytes()
    inventory = json.loads(source_bytes)
    commands = inventory[:27]
    if len(commands) != 27 or len({tuple(row["command"]) for row in commands}) != 27:
        raise SystemExit("Invalid governance command inventory: expected 27 unique commands")
    if any(row["command"][0] != "python3" for row in commands):
        raise SystemExit("Unexpected non-Python command in retained inventory")
    commit = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True, text=True, check=True).stdout.strip()
    results = []
    for row in commands:
        cmd = row["command"]
        try:
            completed = subprocess.run(cmd, capture_output=True, text=True, timeout=args.timeout, check=False)
            result = {"command": cmd, "exit": completed.returncode, "stdout": completed.stdout, "stderr": completed.stderr, "timed_out": False}
        except subprocess.TimeoutExpired as exc:
            result = {"command": cmd, "exit": None, "stdout": (exc.stdout or b"").decode("utf-8", "replace") if isinstance(exc.stdout, bytes) else (exc.stdout or ""), "stderr": (exc.stderr or b"").decode("utf-8", "replace") if isinstance(exc.stderr, bytes) else (exc.stderr or ""), "timed_out": True}
        result["author_receipt_exit"] = row["exit"]
        result["exit_matches_author_receipt"] = result["exit"] == row["exit"] and not result["timed_out"]
        results.append(result)
        print(f'{len(results):02d}/27 exit={result["exit"]} previous={row["exit"]} match={result["exit_matches_author_receipt"]}: {" ".join(cmd)}', flush=True)
    record = {
        "schema": "t81-governance-independent-replay-v1",
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "commit": commit,
        "source": str(SOURCE),
        "source_sha256": hashlib.sha256(source_bytes).hexdigest(),
        "platform": platform.platform(),
        "python": sys.version,
        "ci": os.getenv("GITHUB_ACTIONS", "false"),
        "results": results,
        "passed": sum(r["exit"] == 0 for r in results),
        "failed": sum(r["exit"] not in (0, None) for r in results),
        "timed_out": sum(r["timed_out"] for r in results),
        "matches": sum(r["exit_matches_author_receipt"] for r in results),
    }
    Path(args.output).write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(f'REPLAY: {record["passed"]} pass, {record["failed"]} fail, {record["timed_out"]} timeout; {record["matches"]}/27 match retained exits')
    # Nonzero for mismatch, not for known validator failures. The artifact
    # records all failures explicitly; a matching replay is NOT governance GO.
    return 0 if record["matches"] == 27 else 1


if __name__ == "__main__":
    raise SystemExit(main())
