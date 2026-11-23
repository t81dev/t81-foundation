#!/usr/bin/env python3
"""Fixture checks for scripts/governance/check_rfc_lifecycle_hygiene.py."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


def run_check(script: Path, root: Path | None = None) -> subprocess.CompletedProcess[str]:
    command = [sys.executable, str(script)]
    if root is not None:
        command.extend(["--root", str(root)])
    return subprocess.run(command, text=True, capture_output=True, check=False)


def fail(message: str, result: subprocess.CompletedProcess[str]) -> int:
    print(message)
    print("stdout:", result.stdout)
    print("stderr:", result.stderr)
    return 1


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    script = repo_root / "scripts/governance/check_rfc_lifecycle_hygiene.py"
    fixtures = repo_root / "tests/fixtures/governance/rfc_lifecycle_hygiene"
    if not script.is_file():
        print(f"missing checker: {script}")
        return 1

    missing = run_check(script, fixtures / "missing_rfc_id")
    if missing.returncode == 0:
        return fail("missing RFC id fixture was expected to fail", missing)
    if "rfc lifecycle hygiene check FAILED" not in missing.stdout:
        return fail("missing RFC id fixture did not report FAILED", missing)
    if "RFC-0099 exists as a file but is not in spec/rfcs/index.md: RFC-0099-unindexed.md" not in missing.stdout:
        return fail("missing RFC id fixture did not name RFC-0099", missing)
    if "duplicate ADR number" in missing.stdout:
        return fail("missing RFC id fixture also reported an ADR duplicate", missing)

    duplicate = run_check(script, fixtures / "duplicate_adr")
    if duplicate.returncode == 0:
        return fail("duplicate ADR fixture was expected to fail", duplicate)
    if "rfc lifecycle hygiene check FAILED" not in duplicate.stdout:
        return fail("duplicate ADR fixture did not report FAILED", duplicate)
    expected_adr = "duplicate ADR number 0007: ADR-0007-axion-ai-001.md, ADR-0007-other-claim.md"
    if expected_adr not in duplicate.stdout:
        return fail("duplicate ADR fixture did not name both ADR-0007 files", duplicate)
    if "exists as a file but is not in spec/rfcs/index.md" in duplicate.stdout:
        return fail("duplicate ADR fixture also reported a missing RFC id", duplicate)

    multiple = run_check(script, fixtures / "multiple_markdown")
    if multiple.returncode != 0:
        return fail("multiple markdown fixture was expected to pass", multiple)
    if "rfc lifecycle hygiene check PASSED" not in multiple.stdout:
        return fail("multiple markdown fixture did not pass", multiple)
    if "WARNING: RFC-0056 has 2 markdown files: RFC-0056-hardware-profile.md, RFC-0056-integration.md" not in multiple.stdout:
        return fail("multiple markdown fixture did not warn", multiple)
    if "rfc lifecycle hygiene check FAILED" in multiple.stdout:
        return fail("multiple markdown warning was treated as a failure", multiple)

    live = run_check(script)
    if live.returncode != 0:
        return fail("repository RFC lifecycle check failed", live)
    if "rfc lifecycle hygiene check PASSED" not in live.stdout:
        return fail("repository RFC lifecycle check did not pass", live)
    warned = set(re.findall(r"^WARNING: (RFC-\S+) has \d+ markdown files:", live.stdout, re.MULTILINE))
    if warned != {"RFC-0055", "RFC-0056"}:
        return fail(f"unexpected multiple-markdown warnings: {sorted(warned)}", live)

    print("check_rfc_lifecycle_hygiene_test: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
