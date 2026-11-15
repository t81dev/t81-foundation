"""Exercise real CLI policy binding and record output/artifact rejection evidence."""

import argparse
import hashlib
import json
import os
import platform
import subprocess
import sys
import tempfile
from pathlib import Path


def snapshot(root):
    result = {}
    for path in sorted(root.rglob("*")):
        name = str(path.relative_to(root))
        if path.is_symlink():
            result[name] = ["symlink", os.readlink(path)]
        elif path.is_dir():
            result[name] = ["directory"]
        else:
            result[name] = ["file", hashlib.sha256(path.read_bytes()).hexdigest()]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--t81-bin", type=Path, required=True)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    binary = args.t81_bin.resolve()
    records, failures = [], []
    fixtures = {
        "sentinel.t81": "fn main() -> i32 { print(47); return 0; }\n",
        "allow.apl": "(policy (tier 1) (max-instructions 1000))\n",
        "deny.apl": "(policy (max-instructions 0))\n",
        "malformed.apl": "(policy (max-instructions nope))\n",
        "empty.apl": "",
        "overflow.apl": "(policy (max-instructions 99999999999999999999999))\n",
        "trailing.apl": "(policy (tier 1)) @\n",
    }
    with tempfile.TemporaryDirectory(prefix="t81 policy boundary ") as scratch:
        root = Path(scratch)
        for name, text in fixtures.items():
            (root / name).write_text(text, encoding="utf-8")
        (root / "read-error.apl").mkdir()
        child_tmp = root / "child-tmp"
        child_tmp.mkdir()
        env = os.environ.copy()
        env.update({name: str(child_tmp) for name in ("TMPDIR", "TMP", "TEMP")})
        # Do not let an unrelated caller's configured store escape the fixture.
        env.pop("T81_CANONFS_ROOT", None)

        def run(label, argv, expected, required=(), success=False, trace=None,
                report=False, required_stdout=(), required_any=(), json_error=False):
            command = [str(binary), *map(str, argv)]
            before = snapshot(root)
            previous_trace = trace.read_bytes() if trace and trace.exists() else None
            result = subprocess.run(command, cwd=root, env=env, input="", capture_output=True,
                                    text=True, encoding="utf-8", errors="replace", timeout=30)
            after = snapshot(root)
            errors = []
            if result.returncode != expected:
                errors.append(f"exit {result.returncode}, expected {expected}")
            diagnostic_output = result.stderr
            if json_error:
                try:
                    error_json = json.loads(result.stdout)
                    if (set(error_json) != {"schema", "ok", "error"}
                            or error_json["schema"] != "t81.error.v1"
                            or error_json["ok"] is not False
                            or not isinstance(error_json["error"], str)):
                        errors.append("invalid CLI JSON error contract")
                    else:
                        diagnostic_output = error_json["error"]
                except (ValueError, TypeError):
                    errors.append("missing single CLI JSON error response")
            if success:
                completion = "Trace written to " if trace else "Program terminated normally"
                if "47" not in result.stdout.splitlines() or completion not in result.stdout:
                    errors.append("missing sentinel or completion diagnostic")
            elif not report and not json_error and result.stdout:
                errors.append("unexpected stdout before rejection")
            for text in required:
                if text not in diagnostic_output:
                    errors.append(f"missing diagnostic: {text}")
            if required_any and not any(text in diagnostic_output for text in required_any):
                errors.append(f"missing one of diagnostics: {required_any}")
            for text in required_stdout:
                if text not in result.stdout:
                    errors.append(f"missing stdout diagnostic: {text}")
            changed = sorted(name for name in before.keys() | after.keys()
                             if before.get(name) != after.get(name))
            allowed = [str(trace.relative_to(root))] if success and trace else []
            if any(name not in allowed for name in changed):
                errors.append(f"unintended fixture/output/temp artifacts: {changed}")
            if trace:
                if success and (not trace.exists() or not trace.read_bytes()):
                    errors.append("missing successful trace")
                elif not success:
                    if previous_trace is None and trace.exists():
                        errors.append("rejected execution created a trace")
                    elif previous_trace is not None and (
                            not trace.exists() or trace.read_bytes() != previous_trace):
                        errors.append("rejected execution modified pre-existing output")
            records.append({"case": label, "command": command, "cwd": str(root),
                            "exit_code": result.returncode, "expected_exit_code": expected,
                            "stdout": result.stdout, "stderr": result.stderr,
                            "expected_json_error": json_error,
                            "artifact_changes": changed, "allowed_changes": allowed,
                            "trace_preexisting": previous_trace is not None,
                            "trace_exists": trace.exists() if trace else None,
                            "trace_text": trace.read_text(encoding="utf-8")
                            if trace and trace.exists() else None, "failures": errors})
            failures.extend(f"{label}: {error}" for error in errors)

        tisc = root / "sentinel.tisc"
        command = [str(binary), "code", "build", str(root / "sentinel.t81"), "-o", str(tisc)]
        result = subprocess.run(command, cwd=root, env=env, input="", capture_output=True,
                                text=True, encoding="utf-8", errors="replace", timeout=30)
        records.append({"case": "compile", "command": command, "cwd": str(root),
                        "exit_code": result.returncode, "stdout": result.stdout,
                        "stderr": result.stderr})
        if result.returncode or not tisc.exists():
            raise RuntimeError(f"fixture compilation failed: {records[-1]}")
        tisc_hash = hashlib.sha256(tisc.read_bytes()).hexdigest()
        surfaces = [("vm", "run", tisc), ("vm", "trace", tisc),
                    ("code", "run", tisc), ("code", "run", root / "sentinel.t81")]
        policies = [
            ("allow", 0, ()),
            ("missing", 1, ("Could not open policy file:",)),
            ("malformed", 1, ("Could not parse policy file:",)),
            ("empty", 1, ("Could not parse policy file:",)),
            ("overflow", 1, ("Could not parse policy file:",)),
            ("trailing", 1, ("Could not parse policy file:",)),
            ("read-error", 1, ()),
            ("deny", 13, ("Execution trapped: SecurityFault",
                          "Instruction count limit exceeded: count=1 limit=0",
                          "compute executed: no")),
        ]
        read_diagnostics = ("Could not open policy file:", "Could not read policy file:")
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
                    if trace:
                        argv += ["-o", trace]
                    run(label, argv, code, diagnostic, success=code == 0, trace=trace,
                        required_any=read_diagnostics if policy == "read-error" else ())
            trace = root / f"{surface}-none.trace" if action == "trace" else None
            argv = [domain, action, source] + (["-o", trace] if trace else [])
            run(f"{surface}-no-policy", argv, 0, success=True, trace=trace)
            run(f"{surface}-unsupported-prefix",
                ["--policy", root / "allow.apl", domain, action, source], 1)
            run(f"{surface}-missing-policy-value", [domain, action, source, "--policy"], 1,
                ("Missing argument after --policy",))
            run(f"{surface}-empty-policy-argument",
                [domain, action, source, "--policy", ""] + (["-o", trace] if trace else []),
                1, ("Could not open policy file:",))
            for policy, code, diagnostic in policies[1:]:
                for preexisting in (False, True):
                    label = f"{surface}-{policy}-output-{preexisting}"
                    trace = root / f"{label}.trace"
                    if preexisting:
                        trace.write_text("pre-existing output must survive\n", encoding="utf-8")
                    run(label, [domain, action, source, "--policy", root / f"{policy}.apl",
                                "-o", trace], code, diagnostic, trace=trace,
                        required_any=read_diagnostics if policy == "read-error" else ())

        # Restoring the domain policy binding also affects these existing VM paths.
        # Invalid explicit policies must not reach stepping, state runs or the debugger.
        vm_actions = ["debug", "until", "step", "regs", "stack", "mem", "state",
                      "profile", "explain-trap"]
        for action in vm_actions:
            extra = [] if action == "debug" else ["--json"]
            if action == "until":
                extra += ["--pc", "1", "--count", "1"]
            elif action == "mem":
                extra += ["--addr", "0"]
            for policy, _, diagnostic in policies[1:-1]:
                run(f"vm-{action}-{policy}-preflight",
                    ["vm", action, tisc, "--policy", root / f"{policy}.apl", *extra],
                    1, diagnostic,
                    required_any=read_diagnostics if policy == "read-error" else (),
                    json_error=action != "debug")
            for policy in (None, "allow"):
                option = ["--policy", root / "allow.apl"] if policy else []
                run(f"vm-{action}-{policy or 'none'}-control",
                    ["vm", action, tisc, *option, *extra], 0, report=True)
            if action != "debug":
                # Existing inspector commands report traps with exit 1 (not run's 13).
                run(f"vm-{action}-zero-budget", ["vm", action, tisc, "--policy",
                    root / "deny.apl", *extra], 1, report=True,
                    required_stdout=("SecurityFault",))

    receipt = {"environment": {"platform": platform.platform(), "machine": platform.machine(),
                               "python": sys.version}, "binary": str(binary),
               "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
               "test_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "fixture_text": fixtures, "compiled_fixture_sha256": tisc_hash,
               "isolated_child_temp": True, "inherited_canonfs_root_removed": True,
               "records": records, "failures": failures}
    if args.receipt:
        args.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    for failure in failures:
        print(failure, file=sys.stderr)
    print(f"{'FAIL' if failures else 'PASS'}: {len(records)} CLI processes; {len(failures)} failed checks")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
