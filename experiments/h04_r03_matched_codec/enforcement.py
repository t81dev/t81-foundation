"""Memory controls and observations with explicit coverage, never fake RSS claims.

Kernel memory.max is an accounted-memory guard, NOT an exact RSS limiter.
tracemalloc observes traced transient peaks, NOT every native allocation and NOT
pre-allocation enforcement. Neither adapter may qualify a production trial.
"""
import os
from pathlib import Path
import platform
import tracemalloc

from .measurement_engine import RSS_LIMIT, SCRATCH_LIMIT, ProtocolError


class UnresolvedEnforcement(ProtocolError):
    pass


def _read(path, name):
    # A replaced symlink is rejected at open, not just during path inspection.
    flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
    descriptor = os.open(Path(path) / name, flags)
    with os.fdopen(descriptor) as stream:
        return stream.read(65536).strip()


def _key_values(value):
    fields = {}
    for line in value.splitlines():
        key, number = line.split()
        if key in fields:
            raise ProtocolError("duplicate kernel counter")
        fields[key] = int(number)
        if fields[key] < 0:
            raise ProtocolError("negative kernel counter")
    return fields


def cgroup_snapshot(path):
    """Retain raw kernel control text and counters, including after worker death.

    This reader also accepts regular files in tests: it alone proves no kernel
    enforcement. The producer and verified cgroup filesystem must be retained.
    """
    names = ("memory.max", "memory.swap.max", "memory.current", "memory.peak",
             "memory.events.local", "memory.oom.group", "cgroup.procs", "cgroup.events", "pids.max")
    raw = {name: _read(path, name) for name in names}
    return {"raw": raw, "accounted_current_bytes": int(raw["memory.current"]),
            "accounted_peak_bytes": int(raw["memory.peak"]),
            "events": _key_values(raw["memory.events.local"]),
            "populated": _key_values(raw["cgroup.events"])["populated"],
            "pids": [int(p) for p in raw["cgroup.procs"].split()],
            "scope": "cgroup-accounted-memory-not-process-RSS"}


def validate_cgroup_controls(snapshot, *, empty):
    raw = snapshot["raw"]
    if (snapshot["accounted_current_bytes"] < 0 or snapshot["accounted_peak_bytes"] < 0
            or snapshot["populated"] not in (0, 1)):
        raise ProtocolError("invalid kernel memory counters")
    if not {"max", "oom", "oom_kill", "oom_group_kill"} <= snapshot["events"].keys():
        raise ProtocolError("missing required OOM counters")
    if raw["memory.max"] != str(RSS_LIMIT) or raw["memory.swap.max"] != "0":
        raise ProtocolError("dedicated 32 MiB accounted-memory guard / zero swap required")
    if raw["memory.oom.group"] != "1" or raw["pids.max"] != "1":
        raise ProtocolError("group OOM kill and single-task cgroup required")
    if empty and (snapshot["populated"] != 0 or snapshot["pids"]):
        raise ProtocolError("cgroup is not empty")
    if snapshot["accounted_peak_bytes"] < snapshot["accounted_current_bytes"]:
        raise ProtocolError("inconsistent memory peak")
    if snapshot["accounted_peak_bytes"] > RSS_LIMIT:
        raise ProtocolError("accounted-memory guard overshoot")
    if any(snapshot["events"].get(k, 0) for k in ("max", "oom", "oom_kill", "oom_group_kill")):
        raise ProtocolError("pre-existing or observed memory guard failure")


class KernelMemoryGuard:
    """Read/verify a separately provisioned dedicated cgroup v2 leaf.

    No mounts, kernel settings or cgroups are created/modified implicitly. No
    attach-after-exec path is offered: migration would miss previous charges.
    Native CLONE_INTO_CGROUP/bootstrap coverage is an unresolved prerequisite.
    """
    def __init__(self, path):
        if platform.system() != "Linux":
            raise UnresolvedEnforcement("Linux cgroup v2 required")
        supplied = Path(path)
        resolved = supplied.resolve(strict=True)
        root = Path("/sys/fs/cgroup")
        if supplied.absolute() != resolved or resolved == root or root not in resolved.parents:
            raise ProtocolError("non-symlink dedicated cgroup leaf required")
        mounts = Path("/proc/self/mountinfo").read_text().splitlines()
        if not any(line.split()[4] == str(root) and " - cgroup2 " in line for line in mounts):
            raise UnresolvedEnforcement("verified cgroup2 mount required")
        if any(item.is_dir() for item in resolved.iterdir()):
            raise ProtocolError("cgroup leaf with no descendants required")
        self.path = resolved
        self.before = cgroup_snapshot(resolved)
        validate_cgroup_controls(self.before, empty=True)
        # Fresh allocation history is needed; never reset memory.peak silently.
        if self.before["accounted_peak_bytes"] != 0:
            raise ProtocolError("fresh cgroup history required")

    def evidence(self):
        after = cgroup_snapshot(self.path)
        failure = None
        try:
            validate_cgroup_controls(after, empty=False)
        except ProtocolError as exc:
            failure = str(exc)
        return {"before": self.before, "after": after,
                "kernel_guard_configured_before": True, "current_control_failure": failure,
                "exact_RSS_enforcement": False,
                "native_scratch_coverage": False, "continuous_enforcement": False}

    def enter(self, phase):
        # Existing engine API cannot accept accounted memory as peak_rss_bytes.
        raise UnresolvedEnforcement("exact continuous RSS and full scratch coverage unresolved")

    def leave(self, phase):
        raise UnresolvedEnforcement("kernel counters are not per-phase RSS proof")


class TracedScratch:
    """Detect even freed transient traced allocations; report limited scope.

    A peak over the cap invalidates the observation. It does not prove the
    allocation was prevented or that untraced native memory stayed below cap.
    """
    def begin(self):
        if tracemalloc.is_tracing():
            raise ProtocolError("exclusive tracemalloc ownership required")
        tracemalloc.start(1)
        tracemalloc.reset_peak()

    def finish(self):
        if not tracemalloc.is_tracing():
            raise ProtocolError("scratch tracing was disabled")
        current, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()
        return {"traced_current_bytes": current, "traced_peak_bytes": peak,
                "limit_bytes": SCRATCH_LIMIT, "limit_exceeded": peak > SCRATCH_LIMIT,
                "coverage": "CPython-tracemalloc-domains-only", "continuous_enforcement": False,
                "native_coverage_complete": False}


class ScratchReservations:
    """Pre-allocation budget enforcement for explicitly registered allocations.

    The caller must reserve before allocation, release only after actual free,
    and route every allocation through this interface. Current codec/interpreter
    coverage is incomplete; this is not a production memory backend.
    """
    def __init__(self, journal):
        self.journal, self.live, self.peak = journal, {}, 0

    def reserve(self, allocation, size):
        if not isinstance(allocation, str) or not allocation or allocation in self.live:
            raise ProtocolError("unique allocation identity required")
        if type(size) is not int or size < 0:
            raise ProtocolError("nonnegative byte reservation required")
        total = sum(self.live.values()) + size
        event = {"kind": "scratch_reserve", "allocation": allocation,
                 "size": size, "requested_live_bytes": total, "refused": total > SCRATCH_LIMIT}
        self.journal.append(event)  # durable refusal before caller can allocate
        if total > SCRATCH_LIMIT:
            raise ProtocolError("scratch reservation exceeds 64 KiB")
        self.live[allocation] = size
        self.peak = max(self.peak, total)

    def release(self, allocation):
        if allocation not in self.live:
            raise ProtocolError("unknown or duplicate allocation release")
        self.journal.append({"kind": "scratch_release", "allocation": allocation})
        del self.live[allocation]

    def evidence(self):
        return {"reserved_peak_bytes": self.peak, "outstanding": dict(self.live),
                "coverage": "explicit-reservations-only", "continuous_enforcement": False}
