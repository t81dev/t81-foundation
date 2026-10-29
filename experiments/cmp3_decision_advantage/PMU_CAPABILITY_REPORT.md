# PMU Capability Assessment Report (EXP-T81-COMP3-01/r1)

**Date:** 2026-10-29
**Target Environment:** Linux devbox `6.8.0 #1 SMP PREEMPT_DYNAMIC x86_64`
**Probe Status:** DISCARDED / BLOCKED — `PMU_UNAVAILABLE`

---

## 1. PMU Implementation Overview

The C++ benchmark harness (`benchmark.cpp`) implements hardware performance counter accounting using the Linux native `perf_event_open` system call (`syscall(__NR_perf_event_open, ...)`).

### Required Hardware Counters:
1. `PERF_COUNT_HW_INSTRUCTIONS` (Leader event)
2. `PERF_COUNT_HW_BRANCH_MISSES` (Grouped event)

### Hardware Counter Configuration:
```cpp
perf_event_attr attr{};
attr.type = PERF_TYPE_HARDWARE;
attr.size = sizeof(attr);
attr.config = config;
attr.exclude_kernel = 1;
attr.exclude_hv = 1;
attr.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
```

---

## 2. Capability Probe Execution & Findings

A no-workload PMU probe was executed within the non-performance validation boundary:
```bash
$ /tmp/cmp3_val_gxx/optimized/benchmark --probe-pmu
PASS PCG=4 BOUNDARY=300 POLICY=120 GENERATED=150000 DETERMINISM=30000
BLOCKED — PMU_UNAVAILABLE
```

### Analysis & Categorization:
- **System Call Behavior:** `perf_event_open` returned `-1` (failed).
- **Categorization:** **Kernel configuration restriction / Container virtualization restriction.** In this virtualized container environment, hardware PMU counters are not exposed or permitted by the host kernel security policy (`/proc/sys/kernel/perf_event_paranoid` or container cgroup restrictions).
- **Prohibited Counter Values:** Any temporary probe values are strictly discarded and never logged or treated as experimental data.

---

## 3. Methodological Enforcement & Protocol Adherence

1. **No Counter Substitution:** Wall-clock timing or `rdtsc` cycles are **not** substituted for hardware instruction or branch-miss counters.
2. **No Host Security Bypass:** Host security settings or container privileges were **not** modified or bypassed.
3. **Fail-Closed Execution:** When PMU counters are unavailable, benchmark execution halts immediately with `BLOCKED — PMU_UNAVAILABLE`. Production performance measurement remains blocked.

---

## 4. Final Disposition Impact

Because the target Linux host does not expose hardware PMU instruction and branch-miss counters, experimental performance measurement cannot proceed.

The overall experiment disposition remains:

**MEASUREMENT BLOCKED — PMU_UNAVAILABLE / TRUSTED RUNNER INCOMPLETE.**
