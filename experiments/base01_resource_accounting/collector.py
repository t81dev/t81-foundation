"""Bounded fixture accounting only. No T81 workloads or elapsed-time APIs."""
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import tempfile

try:
    import resource
except ImportError:  # Explicit Windows/other unsupported behavior.
    resource = None

SOURCE_PIN = "cda9135d36144d4de2771af174e52ac7b30a83c9"
FOUNDATION_PIN = "db0e7c8b976991396eed7d14ddc1e889e9c511c2"


class AccountingError(ValueError):
    def __init__(self, message, record=None):
        super().__init__(message)
        self.record = record


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def supported(system=None):
    system = platform.system() if system is None else system
    if system not in ("Linux", "Darwin") or resource is None or not all(
        hasattr(os, name) for name in ("wait4", "posix_spawn")
    ) or not all(hasattr(resource, name) for name in
                 ("getrusage", "RUSAGE_SELF", "RUSAGE_CHILDREN")):
        raise AccountingError("unsupported platform or accounting APIs")
    return system


def number(value):
    try:
        valid = not isinstance(value, bool) and isinstance(value, (int, float)) and math.isfinite(value) and value >= 0
    except OverflowError:
        valid = False
    if not valid:
        raise AccountingError("missing, negative, or malformed accounting value")
    return value


def normalize_rss(raw, system):
    number(raw)
    if system == "Linux":
        return raw, "KiB"
    if system == "Darwin":
        return raw / 1024, "bytes"
    raise AccountingError("unsupported RSS units")


def usage_record(usage, system, scope, pid):
    if usage is None:
        raise AccountingError("missing resource sample")
    try:
        user, kernel, rss = (number(getattr(usage, field)) for field in
                             ("ru_utime", "ru_stime", "ru_maxrss"))
    except AttributeError as exc:
        raise AccountingError("malformed resource sample") from exc
    kib, unit = normalize_rss(rss, system)
    return dict(scope=scope, observer_pid=os.getpid(), subject_pid=pid,
                user_cpu_seconds=user, system_cpu_seconds=kernel,
                maxrss_raw=rss, maxrss_raw_unit=unit, peak_rss_kib=kib,
                raw_rusage={name: getattr(usage, name) for name in dir(usage)
                            if name.startswith("ru_")})


def parent_snapshot(selector, system=None):
    system = supported(system)
    if system != platform.system():
        raise AccountingError("cannot sample another platform's accounting")
    # A string allowlist prevents OS-specific selector integers leaking through.
    if selector not in ("RUSAGE_SELF", "RUSAGE_CHILDREN"):
        raise AccountingError("invalid selector")
    scope = ("parent-process-all-threads" if selector == "RUSAGE_SELF"
             else "cumulative-waited-children-not-per-pid")
    return usage_record(resource.getrusage(getattr(resource, selector)), system,
                        scope, os.getpid() if selector == "RUSAGE_SELF" else None)


def validate_child(report, pid, ppid, system):
    if not isinstance(report, dict):
        raise AccountingError("missing or malformed child report")
    for key, expected in (("pid", pid), ("ppid", ppid), ("bytes", 65536), ("checksum", 8355840)):
        if type(report.get(key)) is not int or report[key] != expected:
            raise AccountingError("child identity or deterministic output mismatch")
    for prefix in ("user", "system"):
        sec, usec = report.get(prefix + "_seconds"), report.get(prefix + "_microseconds")
        if type(sec) is not int or type(usec) is not int or sec < 0 or not 0 <= usec < 1000000:
            raise AccountingError("malformed child timeval")
    normalize_rss(report.get("maxrss_raw"), system)
    return report


class Child:
    def __init__(self, pid, owner, output, mode):
        self.pid, self.owner, self.output, self.mode = pid, owner, output, mode
        self.state = "spawned"


class Collector:
    def __init__(self, build_dir, observations, system=None):
        self.system = supported(system)
        if self.system != platform.system():
            raise AccountingError("collector platform must match the running OS")
        self.owner = os.getpid()
        self.children = {}
        self.observations = observations
        self.binary = Path(build_dir).resolve() / "fixture"
        self.manifest = json.loads((Path(build_dir) / "build.json").read_text())
        if self.manifest["fixture_source_sha256"] != digest(Path(__file__).with_name("fixture.c")):
            raise AccountingError("fixture source identity mismatch")
        self.check_binary()

    def check_binary(self):
        if self.manifest["fixture_binary_sha256"] != digest(self.binary):
            raise AccountingError("fixture binary identity mismatch")

    def launch(self, mode="success"):
        if mode not in ("success", "exit7", "signal"):
            raise AccountingError("only fixed correctness fixture modes are permitted")
        if os.getpid() != self.owner:
            raise AccountingError("collector cannot transfer across fork")
        self.check_binary()
        parent_before = parent_snapshot("RUSAGE_SELF", self.system)
        aggregate_before = parent_snapshot("RUSAGE_CHILDREN", self.system)
        output = tempfile.TemporaryFile()
        try:
            pid = os.posix_spawn(str(self.binary), [str(self.binary), mode],
                                 {"LC_ALL": "C"}, file_actions=[
                                     (os.POSIX_SPAWN_DUP2, output.fileno(), 1)])
        except BaseException:
            output.close()
            raise
        child = Child(pid, self.owner, output, mode)
        child.parent_before = parent_before
        child.aggregate_before = aggregate_before
        self.children[pid] = child
        return child

    def reap(self, child):
        if (not isinstance(child, Child) or child.owner != os.getpid() or
                self.owner != os.getpid() or self.children.get(child.pid) is not child or
                child.state != "spawned"):
            raise AccountingError("unowned child or duplicate reap")
        record = dict(purpose="collector-correctness-only", pid=child.pid,
                      expected_parent_pid=child.owner, fixture_mode=child.mode,
                      lifecycle=["spawned"], performance_samples=[])
        self.observations.append(record)
        # Once attempted, ownership cannot be reused even on ECHILD or bad data.
        child.state = "reap-attempted"
        try:
            record["parent_self_before"] = child.parent_before
            record["aggregate_children_before"] = child.aggregate_before
            result = os.wait4(child.pid, 0)  # Exact positive PID only.
            child.state = "reaped"
            if not isinstance(result, tuple) or len(result) != 3:
                raise AccountingError("malformed wait4 result")
            pid, status, usage = result
            record.update(returned_pid=pid, wait_status_raw=status)
            record["lifecycle"].append("reaped")
            if pid != child.pid:
                raise AccountingError("wait4 returned an unexpected PID")
            if type(status) is not int or status < 0:
                raise AccountingError("malformed wait status")
            record["exit_code"] = os.WEXITSTATUS(status) if os.WIFEXITED(status) else None
            record["signal"] = os.WTERMSIG(status) if os.WIFSIGNALED(status) else None
            record["wait4_child"] = usage_record(usage, self.system, "exact-waited-child", pid)
            record["parent_self_after"] = parent_snapshot("RUSAGE_SELF", self.system)
            record["aggregate_children_after"] = parent_snapshot("RUSAGE_CHILDREN", self.system)
            child.output.seek(0)
            raw = child.output.read(4097)
            record["child_stdout"] = raw.decode("utf-8", errors="replace")
            if len(raw) > 4096:
                raise AccountingError("oversized fixture report")
            try:
                report = json.loads(raw)
            except (ValueError, UnicodeError) as exc:
                raise AccountingError("missing or malformed fixture JSON") from exc
            record["child_self_before_exit"] = validate_child(report, pid, child.owner, self.system)
            # Child self-snapshot precedes reporting/exit. One microsecond is the
            # timeval representation quantum, not a performance threshold.
            for prefix, field in (("user", "user_cpu_seconds"), ("system", "system_cpu_seconds")):
                snapshot = report[prefix + "_seconds"] + report[prefix + "_microseconds"] / 1000000
                if snapshot > record["wait4_child"][field] + 0.000001:
                    raise AccountingError("child CPU snapshot exceeds lifetime accounting")
            if report["maxrss_raw"] > record["wait4_child"]["maxrss_raw"]:
                raise AccountingError("child peak exceeds lifetime peak")
            if not os.WIFEXITED(status) or os.WEXITSTATUS(status) != 0:
                raise AccountingError("unexpected child termination")
            record["validation"] = "passed"
            return record
        except (AccountingError, OSError) as exc:
            record["validation"] = "rejected"
            record["error"] = str(exc)
            if isinstance(exc, OSError):
                record["errno"] = exc.errno
            raise AccountingError(str(exc), record) from exc
        finally:
            child.output.close()
