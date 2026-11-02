"""Frozen H04-R03 measurement infrastructure. No executable measurement entry point.

The public execution gate is unconditional. Internal lifecycle instrumentation is
exercised only with injected clocks in deterministic tests. See MEASUREMENT_ENGINE.md.
"""
from array import array
import gc
import hashlib
import json
from pathlib import Path
import platform
import random
import statistics
import struct
import time

from .benchmark_harness import ARMS, compute_bootstrap_ci, evaluate_decision_rules
from .codec import ARM_ENCODERS, ARM_DECODERS, compute_expected_total_bytes, random_access
from .oracle import L_B10, L_T243, L_B729, L_B729_u5
from .workload import (
    WORKLOAD_N, WORKLOAD_K, WORKLOAD_A, WORKLOAD_ACCESS_MODE,
    CANONICAL_DATASET_SHA256, CANONICAL_QUERY_SHA256, DATASET_SEED, QUERY_SEED,
)

INTEGRATION_SHA = "d8e1376a7a77902cc26269c6e8d4991bcde117a7"
PREREGISTRATION_SHA = "8e44c0f667c62e0d4143bb93169fc3235d5ebd66"
PHASES = (
    "input_mapping", "frame_allocation", "encoding", "retention_boundary",
    "frame_validation", "random_access", "output_and_checksum", "cleanup",
)
RSS_LIMIT = 32 * 1024 * 1024
SCRATCH_LIMIT = 64 * 1024
CONFIGURATION = {
    "n": WORKLOAD_N, "q": WORKLOAD_N, "k": WORKLOAD_K, "alignment": WORKLOAD_A,
    "access_mode": WORKLOAD_ACCESS_MODE, "arms": ARMS, "paired_blocks": 30,
    "bootstrap_resamples": 10000, "bootstrap_seed": 42,
    "dataset_seed": DATASET_SEED, "query_seed": QUERY_SEED,
    "dataset_sha256": CANONICAL_DATASET_SHA256, "query_sha256": CANONICAL_QUERY_SHA256,
}
LENGTHS = {"B10": L_B10, "T243": L_T243, "B729": L_B729, "B729-u5": L_B729_u5}


class ProtocolError(ValueError):
    pass


def run_measurements(*args, **kwargs):
    """No argument, JSON field, environment variable or qualification lifts this gate."""
    raise ProtocolError("Phase C execution FROZEN; separate human authorization required")


def canonical_json(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True,
                      allow_nan=False) + "\n"


def paired_orders(seed):
    """Reproducible uniform permutations; seed is recorded, never selected from outcomes."""
    rng = random.Random(seed)
    orders = []
    for _ in range(30):
        order = list(ARMS)
        rng.shuffle(order)
        orders.append(order)
    return orders


def verify_inputs(input_blob, query_blob):
    """Preflight only: verify frozen bytes BEFORE Phase 1; never time generation.

    Use the existing generators/serializers to prepare files outside trials.
    Compact little-endian buffers avoid retaining Python lists during trials.
    """
    if len(input_blob) != 2 * WORKLOAD_N or len(query_blob) != 4 * WORKLOAD_N:
        raise ProtocolError("frozen input lengths required")
    if hashlib.sha256(input_blob).hexdigest() != CANONICAL_DATASET_SHA256:
        raise ProtocolError("dataset digest mismatch")
    if hashlib.sha256(query_blob).hexdigest() != CANONICAL_QUERY_SHA256:
        raise ProtocolError("query digest mismatch")
    queries = array("I")
    if queries.itemsize != 4:
        raise ProtocolError("32-bit query storage unavailable")
    queries.frombytes(query_blob)
    import sys
    if sys.byteorder != "little":
        queries.byteswap()
    return queries


def qualify_host(snapshot, pins):
    """Pure fail-closed checks. Evidence supplied by collect_host; no host mutation.

    Isolation, exclusive allocation, kernel LTS status and cache reset cannot be
    established from sysfs alone. They require separately reviewed operator evidence.
    """
    failures = []
    def require(condition, reason):
        if not condition:
            failures.append(reason)
    require(snapshot.get("system") == "Linux" and snapshot.get("machine") == "x86_64",
            "Linux x86_64 required")
    require(bool(pins.get("kernel")) and snapshot.get("kernel") == pins.get("kernel"),
            "exact approved LTS kernel pin required")
    require(bool(pins.get("lts_evidence")), "operator LTS evidence required")
    cpus = snapshot.get("cpus", {})
    cores = {(v.get("package"), v.get("core")) for v in cpus.values()
             if v.get("package") is not None and v.get("core") is not None}
    require(len(cores) >= 4, "at least four physical cores required")
    affinity = snapshot.get("affinity", [])
    require(bool(affinity) and affinity == pins.get("affinity"), "approved affinity required")
    isolated = snapshot.get("isolated", [])
    require(bool(affinity) and set(affinity) <= set(isolated), "affinity must use isolated CPUs")
    for cpu in affinity:
        info = cpus.get(str(cpu), {})
        require(info.get("governor") == "performance", f"cpu{cpu}: performance governor required")
        low, high = info.get("min_khz"), info.get("max_khz")
        require(isinstance(low, int) and low > 0 and low == high == pins.get("frequency_khz"),
                f"cpu{cpu}: pinned min/max frequency required")
        caches = info.get("caches", [])
        require({c.get("level") for c in caches} >= {1, 2, 3}, f"cpu{cpu}: L1/L2/L3 required")
        require(bool(caches) and all(c.get("size") and c.get("type")
                and c.get("shared_cpu_list") and isinstance(c.get("line_bytes"), int)
                and c["line_bytes"] > 0 for c in caches), f"cpu{cpu}: incomplete cache topology")
        require(caches == pins.get("caches", {}).get(str(cpu)), f"cpu{cpu}: cache pin mismatch")
    turbo = snapshot.get("turbo", {})
    require(bool(turbo) and all(v == 0 for v in turbo.values()), "verified turbo disable required")
    require(snapshot.get("swap_bytes") == 0, "swap must be disabled")
    ceiling = snapshot.get("allocation_limit_bytes")
    require(isinstance(ceiling, int) and 0 < ceiling <= 16 * 1024**3,
            "finite runner allocation of at most 16 GiB required")
    require(bool(pins.get("dedicated_host_evidence")), "dedicated host evidence required")
    require(bool(pins.get("cache_reset_procedure")), "approved inter-process cache reset required")
    require(bool(pins.get("memory_enforcement_evidence")), "immediate RSS/scratch enforcement required")
    return {"qualified": not failures, "failures": failures, "snapshot": snapshot, "pins": pins}


def _cpu_list(value):
    result = []
    for part in value.strip().split(","):
        if part:
            endpoints = part.split("-")
            lo, hi = int(endpoints[0]), int(endpoints[-1])
            result.extend(range(lo, hi + 1))
    return sorted(set(result))


def collect_host(root=Path("/"), system=None, machine=None, affinity=None):
    """Read-only Linux probe; missing/unsupported fields remain unknown and fail checks."""
    def read(path):
        try:
            return (root / path).read_text().strip()
        except (OSError, UnicodeError):
            return None
    def integer(path):
        value = read(path)
        try:
            return int(value)
        except (TypeError, ValueError):
            return None
    import os
    if affinity is None:
        affinity = sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else []
    cpus = {}
    for directory in sorted((root / "sys/devices/system/cpu").glob("cpu[0-9]*")):
        if not directory.name[3:].isdigit():
            continue
        base = str(directory.relative_to(root))
        caches = []
        for cache in sorted((directory / "cache").glob("index*")):
            path = str(cache.relative_to(root))
            caches.append({"level": integer(path + "/level"), "type": read(path + "/type"),
                           "size": read(path + "/size"),
                           "line_bytes": integer(path + "/coherency_line_size"),
                           "shared_cpu_list": read(path + "/shared_cpu_list")})
        cpus[directory.name[3:]] = {
            "package": integer(base + "/topology/physical_package_id"),
            "core": integer(base + "/topology/core_id"),
            "governor": read(base + "/cpufreq/scaling_governor"),
            "min_khz": integer(base + "/cpufreq/scaling_min_freq"),
            "max_khz": integer(base + "/cpufreq/scaling_max_freq"), "caches": caches,
        }
    turbo = {}
    no_turbo = integer("sys/devices/system/cpu/intel_pstate/no_turbo")
    boost = integer("sys/devices/system/cpu/cpufreq/boost")
    if no_turbo is not None:
        turbo["intel_pstate"] = 1 - no_turbo
    if boost is not None:
        turbo["cpufreq"] = boost
    memory = {}
    for line in (read("proc/meminfo") or "").splitlines():
        fields = line.split()
        if len(fields) >= 2 and fields[0] in ("MemTotal:", "SwapTotal:"):
            memory[fields[0]] = int(fields[1]) * 1024
    # Resolve the current process's unified cgroup instead of assuming cgroup root.
    cgroup = None
    for line in (read("proc/self/cgroup") or "").splitlines():
        if line.startswith("0::/"):
            relative = line[4:]
            if ".." not in Path(relative).parts:
                cgroup = integer("sys/fs/cgroup/" + relative + "/memory.max")
    physical = memory.get("MemTotal:")
    limits = [v for v in (physical, cgroup) if isinstance(v, int) and v > 0]
    isolated = read("sys/devices/system/cpu/isolated")
    return {"system": system or platform.system(), "machine": machine or platform.machine(),
            "kernel": read("proc/sys/kernel/osrelease"), "cpus": cpus,
            "affinity": affinity, "isolated": _cpu_list(isolated) if isolated is not None else [],
            "turbo": turbo, "swap_bytes": memory.get("SwapTotal:"),
            "physical_bytes": physical, "cgroup_limit_bytes": cgroup,
            "allocation_limit_bytes": min(limits) if limits else None}


class _SignedView:
    """Bounded view passed to accepted block packers; no full decoded input copy.

    The inverse offset and packer's offset arithmetic are both charged to Phase 3.
    This adapter preserves the accepted block implementations exactly.
    """
    def __init__(self, mapped, start, stop):
        self.mapped, self.start, self.stop = mapped, start, stop
    def __len__(self):
        return self.stop - self.start
    def __getitem__(self, index):
        if isinstance(index, slice):
            return [self[i] for i in range(*index.indices(len(self))) ]
        if not 0 <= index < len(self):
            raise IndexError(index)
        return self.mapped[self.start + index] - 364


class _Lifecycle:
    """Eight-phase operations; production use remains blocked by run_measurements.

    Input blobs and queries must be owned compact buffers. No generator or digest
    calculation belongs inside the timed lifecycle. Checksum factory is xxHash64
    supplied by a pinned backend; fake factories are used only in unit tests.
    """
    def __init__(self, raw, queries, arm, checksum_factory, expected_checksum, n=WORKLOAD_N):
        if arm not in ARMS or len(raw) != 2 * n:
            raise ProtocolError("invalid arm or input length")
        self.raw, self.queries, self.arm = raw, queries, arm
        self.n, self.k = n, 64
        self.checksum_factory, self.expected_checksum = checksum_factory, expected_checksum
        self.mapped = self.frame = self.output = None
        self.query_sink = 0

    def input_mapping(self):
        self.mapped = array("H")
        if self.mapped.itemsize != 2:
            raise ProtocolError("16-bit mapping storage unavailable")
        for (x,) in struct.iter_unpack("<h", self.raw):
            if not -364 <= x <= 364:
                raise ProtocolError("input outside frozen domain")
            self.mapped.append(x + 364)

    def frame_allocation(self):
        self.frame = bytearray(compute_expected_total_bytes(self.n, self.k, 1, "RA-FIXED", self.arm))

    def encoding(self):
        struct.pack_into("<Q", self.frame, 0, self.n)
        stride = LENGTHS[self.arm](self.k)
        for start in range(0, self.n, self.k):
            block = ARM_ENCODERS[self.arm](_SignedView(self.mapped, start, min(start + self.k, self.n)))
            offset = 8 + (start // self.k) * stride
            self.frame[offset:offset + len(block)] = block

    def retention_boundary(self):
        self.raw = self.mapped = None
        gc.collect()
        # CPython's GIL is not a physical CPU/cache memory barrier. Production
        # qualification must establish a backend barrier; see documented blocker.

    def frame_validation(self):
        if struct.unpack_from("<Q", self.frame)[0] != self.n or len(self.frame) != compute_expected_total_bytes(
                self.n, self.k, 1, "RA-FIXED", self.arm):
            raise ProtocolError("frame geometry mismatch")
        # Stream canonical validation through bounded blocks, retaining no values.
        stride = LENGTHS[self.arm](self.k)
        for start in range(0, self.n, self.k):
            m = min(self.k, self.n - start)
            offset = 8 + (start // self.k) * stride
            ARM_DECODERS[self.arm](self.frame[offset:offset + LENGTHS[self.arm](m)], m)

    def random_access(self):
        # No query-result buffer or cache; scalar sink makes consumption explicit.
        sink = 0
        for index in self.queries:
            sink ^= random_access(self.frame, index, self.arm, self.k, 1, "RA-FIXED")
        self.query_sink = sink

    def output_and_checksum(self):
        # Same sequential reconstruction as accepted dry-run Phase 7. This is
        # NOT a retained copy of Phase 6's Q random-query results.
        self.output = bytearray(8 * self.n)
        stride = LENGTHS[self.arm](self.k)
        for start in range(0, self.n, self.k):
            m = min(self.k, self.n - start)
            offset = 8 + (start // self.k) * stride
            values = ARM_DECODERS[self.arm](self.frame[offset:offset + LENGTHS[self.arm](m)], m)
            for position, value in enumerate(values, start):
                struct.pack_into("<q", self.output, position * 8, value)
        checksum = self.checksum_factory()
        checksum.update(self.output)
        self.checksum = checksum.hexdigest()
        if self.checksum != self.expected_checksum:
            raise ProtocolError("witness checksum mismatch")

    def cleanup(self):
        self.raw = self.mapped = self.frame = self.output = self.queries = None
        gc.collect()


def _instrument(lifecycle, clock=time.perf_counter_ns, memory=None):
    """Time actual operations. Tests MUST inject a synthetic clock and memory monitor.

    memory is a backend monitor, not a sampled-RSS claim: enter/leave delimit
    phases and its enforcement must run during operations, including transients.
    Missing enforcement fails before a clock read or operation.
    """
    if memory is None:
        raise ProtocolError("continuous memory enforcement backend required")
    phases, resource_records = {}, []
    failure = None
    try:
        for phase in PHASES[:-1]:
            memory.enter(phase)
            start = clock()
            try:
                getattr(lifecycle, phase)()
            finally:
                elapsed = clock() - start
                if type(elapsed) is not int or elapsed <= 0:
                    raise ProtocolError("nonpositive or invalid clock interval")
                phases[phase] = elapsed
            record = memory.leave(phase)
            resource_records.append(record)
            if not _valid_memory(record) or record["peak_rss_bytes"] > RSS_LIMIT:
                raise ProtocolError("RSS limit exceeded")
            if phase == "random_access" and record["peak_scratch_bytes"] > SCRATCH_LIMIT:
                raise ProtocolError("query scratch limit exceeded")
    except Exception as exc:
        failure = f"{type(exc).__name__}: {exc}"
    finally:
        try:
            # Cleanup is mandatory even if the monitoring backend itself fails.
            try:
                memory.enter("cleanup")
                start = clock()
            except Exception:
                lifecycle.cleanup()
                raise
            try:
                lifecycle.cleanup()
            finally:
                elapsed = clock() - start
            if type(elapsed) is not int or elapsed <= 0:
                raise ProtocolError("invalid cleanup clock interval")
            phases["cleanup"] = elapsed
            record = memory.leave("cleanup")
            resource_records.append(record)
            if not _valid_memory(record) or record["peak_rss_bytes"] > RSS_LIMIT or not record["reclaimed"]:
                raise ProtocolError("cleanup reclamation or RSS failure")
        except Exception as exc:
            failure = (failure + "; " if failure else "") + f"cleanup: {type(exc).__name__}: {exc}"
    return {"valid": failure is None, "failure": failure, "phase_ns": phases,
            "lifecycle_ns": sum(phases[p] for p in PHASES[:7]) if failure is None else None,
            "query_ns": phases.get("random_access"), "resources": resource_records,
            "checksum": getattr(lifecycle, "checksum", None),
            "witness_checksum": getattr(lifecycle, "expected_checksum", None),
            "query_sink": getattr(lifecycle, "query_sink", None)}


def _valid_memory(record):
    return (isinstance(record, dict)
            and all(type(record.get(k)) is int and record[k] >= 0
                    for k in ("peak_rss_bytes", "peak_scratch_bytes"))
            and record.get("continuous_enforcement") is True)


def summarize(blocks):
    """Analyze exactly 30 complete paired blocks; incomplete/failed evidence is INVALID."""
    invalid = lambda reason: {"decision": {"classification": "INVALID", "reason": reason}}
    if len(blocks) != 30:
        return invalid("exactly 30 paired process blocks required")
    seen_processes = set()
    for block in blocks:
        if sorted(block.get("order", [])) != sorted(ARMS) or set(block.get("arms", {})) != set(ARMS):
            return invalid("missing, duplicate or extra comparison arm")
        for arm, trial in block["arms"].items():
            if not trial.get("valid"):
                return invalid(trial.get("failure") or "invalid trial")
            process = trial.get("process_identity")
            if not isinstance(process, str) or not process or process in seen_processes:
                return invalid("unique process identity required per arm and repetition")
            seen_processes.add(process)
            phases = trial.get("phase_ns", {})
            if set(phases) != set(PHASES) or any(type(v) is not int or v <= 0 for v in phases.values()):
                return invalid("missing or invalid phase timings")
            if trial.get("lifecycle_ns") != sum(phases[p] for p in PHASES[:7]):
                return invalid("lifecycle does not sum phases 1-7")
            if trial.get("query_ns") != phases["random_access"]:
                return invalid("query timing mismatch")
            resources = trial.get("resources", [])
            if len(resources) != 8 or any(not _valid_memory(r) or r["peak_rss_bytes"] > RSS_LIMIT for r in resources):
                return invalid("missing enforcement evidence or RSS breach")
            if resources[5]["peak_scratch_bytes"] > SCRATCH_LIMIT or not resources[-1].get("reclaimed"):
                return invalid("query scratch breach or unreclaimed buffers")
            if trial.get("checksum") != trial.get("witness_checksum") or not trial.get("checksum"):
                return invalid("missing or mismatched witness checksum")
    life = {a: [b["arms"][a]["lifecycle_ns"] for b in blocks] for a in ARMS}
    throughput = {a: [WORKLOAD_N / b["arms"][a]["query_ns"] for b in blocks] for a in ARMS}
    cv = {a: statistics.stdev(life[a]) / statistics.mean(life[a]) for a in ARMS}
    # Protocol invalidity takes precedence over noise, and noise over CI decisions.
    if any(v > .05 for v in cv.values()):
        return {"cv": cv, "decision": evaluate_decision_rules(cv, {}, {})}
    ci_life = {a: compute_bootstrap_ci(life[a], life["T243"]) for a in ARMS[1:]}
    ci_query = {a: compute_bootstrap_ci(throughput["T243"], throughput[a]) for a in ARMS[1:]}
    return {"cv": cv, "ci_life": ci_life, "ci_query": ci_query,
            "decision": evaluate_decision_rules(cv, ci_life, ci_query)}


def make_artifact(commit, environment, qualification, blocks, order_seed, *, synthetic=False):
    """Canonical content; contains raw observations, not only passing summaries.

    Synthetic artifacts cannot qualify as measured evidence. Commit identity is
    supplied by the controller at a clean pinned checkout, never read after trials.
    """
    if len(commit) != 40 or any(c not in "0123456789abcdef" for c in commit):
        raise ProtocolError("full commit SHA required")
    result = summarize(blocks)
    if not qualification.get("qualified"):
        result = {"decision": {"classification": "INVALID", "reason": "host not qualified"}}
    artifact = {"schema": "t81.h04-r03.measurement.v1", "evidence": "SYNTHETIC" if synthetic else "UNEXECUTED",
                "execution_frozen": True, "commit": commit, "candidate_a_integration": INTEGRATION_SHA,
                "preregistration_commit": PREREGISTRATION_SHA, "configuration": CONFIGURATION,
                "environment": environment, "qualification": qualification, "order_seed": order_seed,
                "blocks": blocks, "statistics": result}
    if [b.get("order") for b in blocks] != paired_orders(order_seed):
        artifact["statistics"] = {"decision": {"classification": "INVALID", "reason": "execution order mismatch"}}
    body = canonical_json(artifact)
    return body, hashlib.sha256(body.encode("ascii")).hexdigest()
