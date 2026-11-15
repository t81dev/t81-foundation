"""Portable child-process checks adapted from PR #778 at 56ad0d57.

Fixtures, validators and accounting remain those of PR #779 at 4d5f3586.
An exit status alone is insufficient: the expected diagnostic must be on stderr.
"""

import subprocess
import sys


CASES = (
    "numerical-equality", "overflow-classification", "negative-product-bound",
    "canonical-trits", "stored-tryte-validation", "stored-tryte-decode",
    "stored-tryte-add", "stored-tryte-mul", "sign-handling",
    "binary-truncation", "ternary-truncation", "budget",
)
ORACLE_DIAGNOSTIC = "INVALID EXPERIMENT (CORRECTNESS DEFECT: ORACLE MISMATCH)"
BUDGET_DIAGNOSTIC = "GATE 1 CUMULATIVE TRIAL BUDGET EXCEEDED"


def verify_rejections(executable):
    for kind in CASES:
        try:
            result = subprocess.run(
                [executable, "--negative", kind], capture_output=True,
                text=True, encoding="utf-8", errors="replace", timeout=10,
                check=False, shell=False,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise SystemExit(f"{kind}: child execution failed: {exc}") from exc
        diagnostic = BUDGET_DIAGNOSTIC if kind == "budget" else ORACLE_DIAGNOSTIC
        if result.returncode != 1 or diagnostic not in result.stderr:
            raise SystemExit(f"{kind}: unexpected status/output: {result}")
        print(f"{kind}: rejected with exit 1 and required diagnostic")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: test_result_rejection.py EXECUTABLE")
    verify_rejections(sys.argv[1])
