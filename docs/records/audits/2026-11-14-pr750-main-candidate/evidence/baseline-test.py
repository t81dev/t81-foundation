"""Exercise real CLI policy binding; optional JSON receipts retain every process result."""

import argparse
import hashlib
import json
import platform
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--t81-bin", type=Path, required=True)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    binary = args.t81_bin.resolve()
    records = []
    failures = []
    fixtures = {
        "sentinel.t81": "fn main() -> i32 { print(47); return 0; }\n",
        "allow.apl": "(policy (tier 1) (max-instructions 1000))\n",
        # Pre-dispatch counts start at 1: zero budget denies every instruction.
        "deny.apl": "(policy (max-instructions 0))\n",
        "malformed.apl": "(policy (max-instructions nope))\n",
        "empty.apl": "",
    }
    with tempfile.TemporaryDirectory(prefix="t81 policy boundary ") as scratch:
        root = Path(scratch)
        for name, content in fixtures.items():
            (root / name).write_text(content, encoding="utf-8")

        def run(label, argv, expected, required=(), success=False, trace=None):
            command = [str(binary), *map(str, argv)]
            result = subprocess.run(command, cwd=root, capture_output=True, text=True,
                                    encoding="utf-8", errors="replace", timeout=30)
            errors = []
            if result.returncode != expected:
                errors.append(f"exit {result.returncode}, expected {expected}")
            if success:
                completion = "Trace written to " if trace is not None else "Program terminated normally"
                if "47" not in result.stdout.splitlines() or completion not in result.stdout:
                    errors.append("missing sentinel or completion diagnostic")
            elif result.stdout:
                errors.append("unexpected stdout before rejection")
            for text in required:
                if text not in result.stderr:
                    errors.append(f"missing diagnostic: {text}")
            if trace is not None:
                if success and (not trace.exists() or not trace.read_bytes()):
                    errors.append("missing successful trace")
                elif not success and trace.exists():
                    errors.append("rejected execution created a trace")
            records.append({"case": label, "command": command, "cwd": str(root),
                            "exit_code": result.returncode, "expected_exit_code": expected,
                            "stdout": result.stdout, "stderr": result.stderr,
                            "trace_exists": trace.exists() if trace is not None else None,
                            "trace_text": trace.read_text(encoding="utf-8")
                            if trace is not None and trace.exists() else None,
                            "failures": errors})
            failures.extend(f"{label}: {error}" for error in errors)

        tisc = root / "sentinel.tisc"
        # Compilation is setup, not execution. Its output is also retained.
        command = [str(binary), "code", "build", str(root / "sentinel.t81"), "-o", str(tisc)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True,
                                encoding="utf-8", errors="replace", timeout=30)
        records.append({"case": "compile", "command": command, "cwd": str(root),
                        "exit_code": result.returncode, "stdout": result.stdout, "stderr": result.stderr})
        if result.returncode != 0 or not tisc.exists():
            raise RuntimeError(f"fixture compilation failed: {records[-1]}")
        tisc_hash = hashlib.sha256(tisc.read_bytes()).hexdigest()
        surfaces = [("vm", "run", tisc), ("vm", "trace", tisc),
                    ("code", "run", tisc), ("code", "run", root / "sentinel.t81")]
        policies = [("allow", 0, ()), ("missing", 1, ("Could not open policy file:",)),
                    ("malformed", 1, ("Could not parse policy file:",)),
                    ("empty", 1, ("Could not parse policy file:",)),
                    ("deny", 13, ("Execution trapped: SecurityFault",
                                  "Instruction count limit exceeded: count=1 limit=0",
                                  "compute executed: no"))]
        for domain, action, source in surfaces:
            surface = f"{domain}-{action}-{source.suffix[1:]}"
            for placement in ("domain", "action", "trailing"):
                for policy, code, diagnostic in policies:
                    label = f"{surface}-{placement}-{policy}"
                    option = ["--policy", root / f"{policy}.apl"]
                    if placement == "domain":
                        argv = [domain, *option, action, source]
                    elif placement == "action":
                        argv = [domain, action, *option, source]
                    else:
                        argv = [domain, action, source, *option]
                    trace = root / f"{label}.trace" if action == "trace" else None
                    if trace is not None:
                        argv += ["-o", trace]
                    run(label, argv, code, diagnostic, success=code == 0, trace=trace)
            trace = root / f"{surface}-none.trace" if action == "trace" else None
            argv = [domain, action, source]
            if trace is not None:
                argv += ["-o", trace]
            run(f"{surface}-no-policy", argv, 0, success=True, trace=trace)
            # Options before the top-level domain are unsupported by the existing CLI.
            run(f"{surface}-unsupported-prefix",
                ["--policy", root / "allow.apl", domain, action, source], 1)
            run(f"{surface}-missing-policy-value", [domain, action, source, "--policy"], 1,
                ("Missing argument after --policy",))

    receipt = {"environment": {"platform": platform.platform(), "machine": platform.machine(),
                               "python": sys.version},
               "binary": str(binary), "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
               "test_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "fixture_text": fixtures, "compiled_fixture_sha256": tisc_hash,
               "records": records, "failures": failures}
    if args.receipt:
        args.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    for failure in failures:
        print(failure, file=sys.stderr)
    print(f"{'FAIL' if failures else 'PASS'}: {len(records)} CLI processes; {len(failures)} failed checks")
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
