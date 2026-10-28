"""EXPLORATORY — NONCONFIRMATORY parent-side process interval observer.

The accepted apparatus launch/execute/Store/readback functions remain unchanged.
Only its subprocess namespace is proxied to observe Popen/communicate boundaries.
"""
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import types

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import apparatus

LABEL = "EXPLORATORY — NONCONFIRMATORY"


def install(path):
    original_launch = apparatus.launch
    current = {}
    sequence = 0

    class Process:
        def __init__(self, *args, **kwargs):
            current["start_utc"] = datetime.now(timezone.utc).isoformat()
            current["start_ns"] = time.perf_counter_ns()
            try:
                self.process = subprocess.Popen(*args, **kwargs)
                current["child_pid"] = self.process.pid
            except BaseException:
                current["end_ns"] = time.perf_counter_ns()
                current["returncode"] = None
                raise

        def communicate(self, *args, **kwargs):
            try:
                return self.process.communicate(*args, **kwargs)
            finally:
                # In timeout cleanup, the later communicate return supersedes this.
                current["end_ns"] = time.perf_counter_ns()
                current["returncode"] = self.process.returncode

        def __getattr__(self, key):
            return getattr(self.process, key)

    def observed_launch(command, journal, *, context=None, timeout=180):
        nonlocal sequence
        sequence += 1
        current.clear()
        error = None
        try:
            return original_launch(command, journal, context=context, timeout=timeout)
        except BaseException as failure:
            error = dict(error_type=type(failure).__name__, error=str(failure))
            raise
        finally:
            record = dict(label=LABEL, confirmatory=False, eligible_for_preregistered_dataset=False,
                          sequence=sequence, parent_pid=os.getpid(), command=[str(x) for x in command],
                          context=context or {}, error=error, **current)
            record["elapsed_ns"] = (current["end_ns"]-current["start_ns"]
                                    if "end_ns" in current and "start_ns" in current else None)
            # Extra diagnostic I/O lies outside nested intervals, inside outer time.
            with Path(path).open("a") as stream:
                stream.write(json.dumps(record, ensure_ascii=False) + "\n")
                stream.flush()
                os.fsync(stream.fileno())

    apparatus.subprocess = types.SimpleNamespace(Popen=Process, PIPE=subprocess.PIPE)
    apparatus.launch = observed_launch


if __name__ == "__main__":
    install(os.environ["T81_I08_DIAGNOSTIC_TIMINGS"])
    try:
        apparatus.main()
    except PermissionError as error:
        print(str(error), file=sys.stderr)
        sys.exit(13)
    except apparatus.LaunchFailure as error:
        print(str(error), file=sys.stderr)
        sys.exit(error.returncode if error.returncode in (1, 13) else 1)
