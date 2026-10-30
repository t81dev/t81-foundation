#!/usr/bin/env python3
"""Inject failures into the checked-in CI shell contracts; no build/model runs.

Uses only the standard library. The small indentation reader deliberately
accepts just the job/default/run forms exercised here, not arbitrary YAML.
It is not a general workflow syntax validator; YAML/Actions checks are separate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import textwrap


ROOT = Path(__file__).resolve().parents[2]
JOBS = {
    "ci.yml": ("spec-and-docs", "warn-strict", "benchmarks"),
    "ai-experiments-ci.yml": ("test-ai-experiments",),
}


def job_text(workflow, job):
    source = (ROOT / ".github/workflows" / workflow).read_text()
    match = re.search(r"^  " + re.escape(job) + r":\s*\n", source, re.M)
    if match is None:
        raise ValueError(f"missing job: {workflow}/{job}")
    rest = source[match.end():]
    end = re.search(r"^  [\w-]+:\s*\n", rest, re.M)
    return rest[:end.start()] if end else rest


def shell_command(workflow, job):
    source = job_text(workflow, job)
    # Require the actual job default, not merely a shell on the test step.
    if not re.search(r"^    defaults:\n      run:\n        shell: bash\s*$", source, re.M):
        raise ValueError(f"{workflow}/{job}: explicit bash default required")
    # GitHub's documented invocation for `shell: bash`.
    return ["bash", "--noprofile", "--norc", "-e", "-o", "pipefail"]


def step_run(workflow, job, name):
    source = job_text(workflow, job)
    match = re.search(r"^( +)- name: " + re.escape(name) + r"\s*$", source, re.M)
    if match is None:
        raise ValueError(f"missing step: {name}")
    rest = source[match.end():]
    end = re.search(r"^" + re.escape(match[1]) + r"- ", rest, re.M)
    step = rest[:end.start()] if end else rest
    run = re.search(r"^( +)run: (.*)$", step, re.M)
    if run is None:
        raise ValueError(f"missing run: {name}")
    if run[2] == "|":
        body = step[run.end():].splitlines()
        lines = []
        for line in body:
            if line.strip() and len(line) - len(line.lstrip()) <= len(run[1]):
                break
            lines.append(line)
        return textwrap.dedent("\n".join(lines)).strip() + "\n"
    if run[2].startswith(("'", '"', ">")):
        raise ValueError(f"unsupported run scalar: {name}")
    return run[2] + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    results = []
    shells = {(w, j): shell_command(w, j) for w, jobs in JOBS.items() for j in jobs}

    def execute(name, workflow, job, script, expected, env=None, setup=None):
        directory = out / name
        directory.mkdir()
        for path in [".github/logs", "build/ai-rfc0025", "build/ai-vm-trace",
                     "build/ai-opcodes", "build/benchmarks", "bin"]:
            (directory / path).mkdir(parents=True, exist_ok=True)
        if setup:
            setup(directory)
        path = directory / "step.sh"
        path.write_text(script)
        environment = dict(os.environ, **(env or {}))
        environment["PATH"] = str(directory / "bin") + os.pathsep + os.environ["PATH"]
        environment["GITHUB_OUTPUT"] = str(directory / "outputs")
        command = shells[workflow, job] + [str(path)]
        process = subprocess.run(command, cwd=directory, env=environment, capture_output=True)
        (directory / "stdout.log").write_bytes(process.stdout)
        (directory / "stderr.log").write_bytes(process.stderr)
        row = dict(name=name, workflow=workflow, job=job, command=command,
                   expected_exit=expected, actual_exit=process.returncode,
                   passed=process.returncode == expected)
        results.append(row)
        return directory, row

    # Non-final pipeline producer failures must fail the step and stop its
    # following command, while tee retains stdout and stderr in both cases.
    for (workflow, job) in shells:
        for code in (0, 7, 23):
            script = (f"bash -c 'echo producer-out; echo producer-err >&2; exit {code}' "
                      "2>&1 | tee command.log\nprintf reached > following-command\n")
            directory, row = execute(f"{job}-producer-{code}", workflow, job, script, code)
            log = (directory / "command.log").read_text()
            row["combined_log_retained"] = "producer-out" in log and "producer-err" in log
            row["following_command_ran"] = (directory / "following-command").exists()
            row["passed"] &= row["combined_log_retained"] and row["following_command_ran"] == (code == 0)
        execute(f"{job}-tee-failure", workflow, job,
                "printf producer-out | tee absent-parent/command.log\n", 1)

    def mock_tools(directory):
        python = '''#!/usr/bin/env bash
echo "$*" >> calls.log
echo "validator stdout: $*"
echo "validator stderr: $*" >&2
if [[ "$1" == "${FAIL_CHECK:-}" ]]; then exit "${CHECK_EXIT:-17}"; fi
'''
        cmake = '''#!/usr/bin/env bash
echo "cmake $*" >> calls.log
if [[ "$*" == *"--target help"* ]]; then
  echo "cmake-help diagnostic" >&2
  if [[ "${TARGETS_PRESENT:-1}" == 1 ]]; then
    printf '%s\\n' t81_ai t81_vm_tloadhash_conformance_test t81_vm_trace_test canonfs_axion_trace_test t81_vm_ai_phase1_qmatmul_conformance_test
    # Exceeds a pipe buffer: grep -q must not terminate a live producer.
    for ((i=0; i<10000; i++)); do echo unrelated-target-$i; done
  fi
  exit "${HELP_EXIT:-0}"
fi
echo "build diagnostic"
exit "${BUILD_EXIT:-0}"
'''
        ctest = '''#!/usr/bin/env bash
echo ctest-stdout
echo ctest-stderr >&2
exit "${CTEST_EXIT:-0}"
'''
        for name, body in {"python3": python, "cmake": cmake, "ctest": ctest,
                           "nproc": "#!/usr/bin/env bash\necho 2\n"}.items():
            file = directory / "bin" / name
            file.write_text(body)
            file.chmod(0o755)
        binary = directory / "build/benchmarks/benchmark_runner"
        binary.write_text("#!/usr/bin/env bash\necho benchmark-diagnostic\nexit \"${BUILD_EXIT:-0}\"\n")
        binary.chmod(0o755)

    # Execute the unchanged multi-validator workflow body with controlled
    # producers. A failure in the middle must not be hidden by later successes.
    governance = step_run("ci.yml", "spec-and-docs", "Run governance policy structure/license/artifact checks")
    for label, check, code in [("pass", "", 0), ("first", "scripts/governance/check_root_structure.py", 17),
                               ("middle", "scripts/governance/check_license_policy.py", 19)]:
        directory, row = execute("governance-" + label, "ci.yml", "spec-and-docs", governance, code,
                                 dict(FAIL_CHECK=check, CHECK_EXIT=str(code)), mock_tools)
        calls = (directory / "calls.log").read_text().splitlines()
        row["validator_calls"] = calls
        row["log_retained"] = (directory / ".github/logs/spec-and-docs.log").is_file()
        row["passed"] &= row["log_retained"] and (len(calls) == 17 if not check else calls[-1] == check)

    for job, name in [("warn-strict", "Build (warnings-as-errors)"),
                      ("benchmarks", "Run benchmark runner"),
                      ("benchmarks", "Enforce SIMD benchmark non-regression thresholds")]:
        for code in (0, 23):
            execute(f"{job}-{len(results)}-{code}", "ci.yml", job,
                    step_run("ci.yml", job, name), code,
                    dict(BUILD_EXIT=str(code), FAIL_CHECK="scripts/ci/check_simd_regression.py",
                         CHECK_EXIT=str(code)), mock_tools)

    ai_steps = ["Build AI CLI", "Build RFC-0025 TLOADHASH conformance targets",
                "Collect VM trace-level evidence (RFC-00A1 runtime trace slice)",
                "Run RFC-0026 phase-1 opcode runtime conformance"]
    for index, name in enumerate(ai_steps):
        for variant, env, expected in [
            ("present", {}, 0), ("help-failed-with-matching-output", {"HELP_EXIT": "29"}, 29),
            ("absent", {"TARGETS_PRESENT": "0"}, 0 if index == 0 else 1),
            ("build-failed", {"BUILD_EXIT": "31"}, 31),
        ]:
            execute(f"ai-probe-{index}-{variant}", "ai-experiments-ci.yml", "test-ai-experiments",
                    step_run("ai-experiments-ci.yml", "test-ai-experiments", name), expected, env, mock_tools)

    for index, name in enumerate(["Run RFC-0025 TLOADHASH conformance tests", *ai_steps[2:]]):
        for code in (0, 37):
            directory, row = execute(f"ai-ctest-{index}-{code}", "ai-experiments-ci.yml", "test-ai-experiments",
                                     step_run("ai-experiments-ci.yml", "test-ai-experiments", name), code,
                                     dict(CTEST_EXIT=str(code)), mock_tools)
            logs = list((directory / "build").rglob("*.log"))
            row["ctest_log_retained"] = any("ctest-stdout" in p.read_text() and "ctest-stderr" in p.read_text() for p in logs)
            row["passed"] &= row["ctest_log_retained"]

    result = dict(cases=len(results), failures=sum(not r["passed"] for r in results), results=results,
                  workflow_sha256={w: hashlib.sha256((ROOT / ".github/workflows" / w).read_bytes()).hexdigest() for w in JOBS})
    (out / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(dict(cases=result["cases"], failures=result["failures"], evidence=str(out))))
    return int(result["failures"] != 0)


if __name__ == "__main__":
    raise SystemExit(main())
