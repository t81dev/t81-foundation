"""Real-CLI policy metadata tests and separately labeled fake-backend argv tests."""

import argparse
import hashlib
import json
import os
import platform
import subprocess
import sys
import tempfile
from pathlib import Path


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bash", type=Path, default=Path("/bin/bash"))
    parser.add_argument("--wrapper", type=Path, default=Path(__file__).resolve().parents[2]
                        / "skills/policy-checker/validate-policy.sh")
    parser.add_argument("--t81-bin", type=Path, help="Omit to run fake-backend tests only")
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    bash = args.bash.resolve()
    wrapper = args.wrapper.resolve()
    records, failures = [], []
    version = subprocess.run([str(bash), "--version"], capture_output=True, text=True, check=True)
    with tempfile.TemporaryDirectory(prefix="t81 skill boundary ") as scratch:
        root = Path(scratch)
        fake_bin, real_bin, fixtures = (root / name for name in ("fake-bin", "real-bin", "inputs"))
        for directory in (fake_bin, real_bin, fixtures):
            directory.mkdir()
        # A fixed executable captures argv; it never interprets a command string.
        fake = fake_bin / "t81"
        fake.write_text(f'#!{sys.executable}\nimport json,os,sys\n'
                        'print(json.dumps({"fake_argv": sys.argv[1:]}))\n'
                        'sys.exit(int(os.environ.get("T81_TEST_BACKEND_EXIT", "0")))\n')
        fake.chmod(0o755)
        if args.t81_bin:
            real = args.t81_bin.resolve()
            (real_bin / "t81").symlink_to(real)
        texts = {"good.apl": "(policy (tier 1) (max-instructions 1000))\n",
                 "deny.apl": "(policy (max-instructions 0))\n",
                 "bad.apl": "(policy (tier nope))\n", "empty.apl": "",
                 "invalid.axionb": "not a compiled policy"}
        names = ["space name.apl", "single'quote.apl", 'double"quote.apl',
                 'raw;|&<>*?[]{}!.apl', 'literal$(printf AUDIT_CANARY).apl',
                 'dollar$(printf CANARY > canary-dollar).apl',
                 'backtick`printf CANARY > canary-backtick`.apl', '$HOME (data).apl',
                 'quote";printf CANARY > canary-quote;printf ".apl', '--help.apl']
        for name in names:
            texts[name] = texts["good.apl"]
        for name, text in texts.items():
            (fixtures / name).write_text(text, encoding="utf-8")
        (fixtures / "directory.apl").mkdir()
        (fixtures / "broken.apl").symlink_to(fixtures / "absent-target")
        (fixtures / "linked.apl").symlink_to(fixtures / "good.apl")
        os.mkfifo(fixtures / "fifo.apl")
        unreadable = fixtures / "unreadable.apl"
        unreadable.write_text(texts["good.apl"], encoding="utf-8")
        unreadable.chmod(0)
        unreadable_testable = not os.access(unreadable, os.R_OK)
        # Snapshot only regular files/directories/symlinks; never read a FIFO.
        def observe():
            result = {}
            for p in sorted(root.rglob("*")):
                name = str(p.relative_to(root))
                if p.is_symlink():
                    result[name] = {"kind": "symlink", "target": os.readlink(p)}
                elif p.is_dir():
                    result[name] = {"kind": "dir"}
                elif p.is_file():
                    try:
                        digest = sha(p)
                    except PermissionError:
                        digest = "unreadable"
                    result[name] = {"kind": "file", "sha256": digest,
                                    "size": p.stat().st_size, "mode": p.stat().st_mode}
                else:
                    result[name] = {"kind": "special"}
            return result

        def run(backend, label, argv, code, expected_path=None, valid=None, backend_exit=0):
            env = os.environ.copy()
            # Ambient shell startup hooks and function exports are outside this boundary.
            env.pop("BASH_ENV", None)
            env.pop("ENV", None)
            for key in list(env):
                if key.startswith("BASH_FUNC_"):
                    del env[key]
            env.update(PATH=str(fake_bin if backend == "fake" else real_bin) + os.pathsep + os.defpath,
                       HOME=str(root / "home-must-not-be-created"),
                       T81_CANONFS_ROOT=str(root / "store-must-not-be-created"),
                       T81_TEST_BACKEND_EXIT=str(backend_exit))
            if backend == "absent":
                env["PATH"] = str(root / "no-cli")
            command = [str(bash), str(wrapper), *map(str, argv)]
            before = observe()
            proc = subprocess.run(command, cwd=fixtures, env=env, capture_output=True,
                                  text=True, encoding="utf-8", errors="replace", timeout=10)
            after = observe()
            changed = {p: {"before": before.get(p), "after": after.get(p)}
                       for p in before.keys() | after.keys() if before.get(p) != after.get(p)}
            errors = []
            if proc.returncode != code:
                errors.append(f"exit {proc.returncode}, expected {code}")
            if changed:
                errors.append("unexpected persistent filesystem change")
            if expected_path is not None:
                try:
                    output = json.loads(proc.stdout)
                    if backend == "fake":
                        expected = ["policy", "validate", str(expected_path), "--json"]
                        if output != {"fake_argv": expected}:
                            errors.append("argv differs from fixed intended dispatch")
                    elif (output.get("schema") != "t81.policy-validate.v1"
                          or output.get("valid") is not valid
                          or output.get("policy") != str(expected_path)):
                        errors.append("native schema/validity/input identity mismatch")
                except (ValueError, TypeError):
                    errors.append("missing expected JSON output")
            elif proc.stdout:
                errors.append("backend unexpectedly reached on rejected arguments")
            records.append({"backend": backend, "case": label, "command": command,
                            "cwd": str(fixtures), "environment_overrides": {
                                key: env[key] for key in ("PATH", "HOME", "T81_CANONFS_ROOT", "T81_TEST_BACKEND_EXIT")},
                            "exit_code": proc.returncode, "expected_exit_code": code,
                            "stdout": proc.stdout, "stderr": proc.stderr,
                            "filesystem_changes": changed, "failures": errors})
            failures.extend(f"{backend}/{label}: {error}" for error in errors)
            # Remove only harness-observed canaries/store paths before the next case.
            for path in sorted(after.keys() - before.keys(), key=len, reverse=True):
                p = root / path
                if p.is_dir():
                    p.rmdir()
                else:
                    p.unlink()

        if args.t81_bin:
            command = [str(real), "policy", "compile", str(fixtures / "good.apl"),
                       "-o", str(fixtures / "good.axionb")]
            proc = subprocess.run(command, cwd=fixtures, capture_output=True, text=True,
                                  encoding="utf-8", errors="replace", timeout=10)
            records.append({"backend": "fixture-setup", "case": "compile-policy", "command": command,
                            "cwd": str(fixtures), "exit_code": proc.returncode,
                            "stdout": proc.stdout, "stderr": proc.stderr,
                            "output_sha256": sha(fixtures / "good.axionb") if proc.returncode == 0 else None})
            if proc.returncode != 0:
                raise RuntimeError(f"policy fixture compilation failed: {records[-1]}")

        backends = ["fake"] + (["real"] if args.t81_bin else [])
        for backend in backends:
            run(backend, "normal", ["good.apl"], 0, "./good.apl", True)
            run(backend, "json-before", ["--json", "good.apl"], 0, "./good.apl", True)
            run(backend, "json-after", ["good.apl", "--json"], 0, "./good.apl", True)
            run(backend, "absolute", [fixtures / "good.apl"], 0, fixtures / "good.apl", True)
            run(backend, "symlink-explicitly-allowed", ["linked.apl"], 0, "./linked.apl", True)
            for name in names:
                run(backend, "literal:" + name, ["--", name], 0, "./" + name, True)
            for label, argv in [
                ("missing", []), ("empty-path", [""]), ("empty-after-separator", ["--", ""]),
                ("strict-unsupported", ["good.apl", "--strict"]),
                ("policy-unsupported", ["good.apl", "--policy", "missing.apl"]),
                ("output-unsupported", ["good.apl", "--output", "must-not-exist"]),
                ("store-unsupported", ["import", "good.apl", "--canonfs-root", str(root / "store")]),
                ("boolean-value", ["good.apl", "--json", "true"]),
                ("boolean-assignment", ["good.apl", "--json=true"]),
                ("duplicate-flag", ["good.apl", "--json", "--json"]),
                ("extra-positional", ["good.apl", "bad.apl"]),
                ("unsupported-operation", ["simulate", "good.apl"]),
                ("legacy-dispatch-injection", ["import", names[5]]),
                ("legacy-unquoted-operation", ['$(printf CANARY > canary-operation)', "good.apl"]),
                ("legacy-quote-breakout", ["import", names[8]]),
                ("no-separator-leading-dash", ["--help.apl"]),
                ("separator-missing", ["--"]), ("separator-extra", ["--", "good.apl", "bad.apl"]),
                ("wrong-suffix", ["program.tisc"]), ("missing-path", ["missing.apl"]),
                ("directory", ["directory.apl"]), ("broken-symlink", ["broken.apl"]),
                ("fifo", ["fifo.apl"])]:
                run(backend, label, argv, 2)
            if unreadable_testable:
                run(backend, "unreadable-file", ["unreadable.apl"], 2)
            if backend == "fake":
                run(backend, "exit-propagation", ["good.apl"], 23, "./good.apl", backend_exit=23)
            else:
                run(backend, "compiled-policy", ["good.axionb"], 0, "./good.axionb", True)
                for name in ("bad.apl", "empty.apl", "invalid.axionb"):
                    run(backend, "parse-failure:" + name, [name], 1, "./" + name, False)
                # A zero execution budget is valid metadata, not an execution request.
                run(backend, "deny-policy-is-data", ["deny.apl"], 0, "./deny.apl", True)
        run("absent", "missing-cli", ["good.apl"], 127)
        receipt = {"environment": {"platform": platform.platform(), "machine": platform.machine(),
                                   "python": sys.version, "bash_version_command": [str(bash), "--version"],
                                   "bash_version_exit": version.returncode, "bash_version_stdout": version.stdout,
                                   "bash_version_stderr": version.stderr},
                   "wrapper": str(wrapper), "wrapper_sha256": sha(wrapper),
                   "test_sha256": sha(__file__), "fixture_text": texts,
                   "binary": str(args.t81_bin.resolve()) if args.t81_bin else None,
                   "binary_sha256": sha(args.t81_bin) if args.t81_bin else None,
                   "skipped": [] if unreadable_testable else ["unreadable-file: current user can read chmod 000 file"],
                   "records": records, "failures": failures}
        unreadable.chmod(0o600)
    if args.receipt:
        args.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    for failure in failures:
        print(failure, file=sys.stderr)
    print(f"{'FAIL' if failures else 'PASS'}: {len(records)} process receipts; "
          f"{len(failures)} failed checks; real CLI {'included' if args.t81_bin else 'NOT RUN'}")
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
