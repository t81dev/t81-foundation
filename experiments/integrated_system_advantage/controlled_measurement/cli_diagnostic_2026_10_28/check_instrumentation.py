"""Actual recorder checks, not task/performance observations. Retains all attempts."""
import base64
import json
from pathlib import Path
import subprocess
import sys
import time
import worker

out = Path(sys.argv[1]).resolve()
out.mkdir(parents=True, exist_ok=False)
timings = out / "nested.jsonl"
worker.install(timings)
journal = worker.apparatus.Journal(out / "attempts.jsonl", create=True)
cases = [([sys.executable, "-c", "print('observer-ok')"], None, 10),
         ([sys.executable, "-c", "import sys;sys.stdout.buffer.write(b'\\xff');sys.exit(7)"], worker.apparatus.LaunchFailure, 10),
         ([str(out / "missing-executable")], FileNotFoundError, 10),
         ([sys.executable, "-c", "import time;time.sleep(5)"], subprocess.TimeoutExpired, 0.02)]
for sequence, (command, expected, timeout) in enumerate(cases, 1):
    start = time.perf_counter_ns()
    try:
        raw = worker.apparatus.launch(command, journal, context={"operation":"recorder_selfcheck"}, timeout=timeout)
        if expected or raw != b"observer-ok\n":
            raise ValueError("Unexpected success/output")
    except BaseException as error:
        if expected is None or not isinstance(error, expected):
            raise
    end = time.perf_counter_ns()
    record = json.loads(timings.read_text().splitlines()[-1])
    if not (record["sequence"] == sequence and start <= record["start_ns"] < record["end_ns"] <= end):
        raise ValueError("Clock boundary mismatch")
    if (record["error"] is None) != (expected is None):
        raise ValueError("Error retention mismatch")
events = [json.loads(line) for line in journal.path.read_text().splitlines()]
if len(events) != 8 or base64.b64decode(events[3]["stdout_base64"]) != b"\xff":
    raise ValueError("Original journal/output retention changed")
print("4 recorder checks passed: success, nonzero/lossless output, spawn failure, timeout; not scientific observations")
