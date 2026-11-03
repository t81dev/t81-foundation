# BASE-01 Reference Performance Baseline: Phase A Gate Decision

**Document:** `docs/research/base01/BASE01_GATE_DECISION.md`
**Mission:** BASE-01 Phase A — Workload Discovery, Eligibility Audit, and Measurement Readiness
**Protocol:** BASE-01
**Version:** 0.1.0-draft
**Date:** 2026-11-03
**Auditor:** Jules (AI Software Engineer)
**Evaluated Repository:** `https://github.com/t81dev/t81-foundation`
**Evaluated HEAD Commit:** `cda9135d36144d4de2771af174e52ac7b30a83c9`
**Diverged Reference PR:** PR #686 draft commit `ac7397aa5f8751d34adcb6ce2d38e3141e65b36c` (unmerged, unvalidated on Linux/Windows)

---

## 1. Overall Gate Disposition

$$\mathbf{CONDITIONALLY\ READY}$$

### Definitive Gate Justification
Phase A source discovery and workload eligibility auditing demonstrate that T81 Foundation contains an independently scoped, reproducible, and mathematically verified minimal workload set spanning all four required baseline domains (Arithmetic, TISC/HanoiVM, CanonFS, and Resource Consumption) that does **not** conflict with frozen research protocols or claim native ternary hardware advantage.

However, execution readiness is classified as **CONDITIONALLY READY** rather than fully ready for immediate execution because five bounded, non-trivial engineering prerequisites must be satisfied and signed off before any Phase B measurement trials may be launched:
1. **Dedicated Phase B Runner Scaffolding:** Construction of an isolated benchmark harness that embeds the pre-measurement correctness oracle assertions and enforces fail-closed execution without relying on unvalidated interactive shells or ad-hoc scripts.
2. **Host Environment Qualification & Frequency Lockdown:** Enforcement of strict host controls (single physical core pinning via `taskset`, locking CPU governor to `performance`, disabling dynamic thermal boost, and recording kernel/cache hardware topology).
3. **Dedicated Non-Contended I/O Scratch Boundary:** Provisioning of a dedicated local scratch mount for persistent storage baselines (`WL-CANONFS-PERSISTENT-IO`) to isolate benchmark timings from host filesystem write queuing and background journaling.
4. **Platform Resource Metric Normalization:** Embedding platform unit handling into the resource observer (`WL-RES-RUSAGE-METRICS`) to normalize Linux `ru_maxrss` (KiB) and Darwin/macOS `ru_maxrss` (bytes) into canonical kibibytes ($\text{KiB}$).
5. **Separate Maintainer Authorization Gate:** An explicit maintainer cryptographic or written sign-off authorizing the activation of Phase B measurement trials. **This Phase A gate decision does not authorize Phase B benchmarking.**

---

## 2. Reconciled Workload Candidate Inventory

Across the 11 candidate workloads discovered, audited, and cataloged in `BASE01_WORKLOAD_AUDIT.md` and `BASE01_MANIFEST.json`, every workload ID appears exactly once and classifications are strictly reconciled:

- **Total Inspected Candidates:** 11
- **ELIGIBLE:** 8 (72.7%)
- **CONDITIONAL:** 1 (9.1%)
- **EXCLUDED:** 2 (18.2%)
- **UNKNOWN:** 0 (0.0%)

### 2.1 Eligible Workload IDs (8)
1. **`WL-ARITH-CODEC-I64`** (Domain A — Arithmetic): Scalar Horner MSB-first balanced ternary conversion round-trip for signed 64-bit integers on a binary host CPU. Correctness oracle: `tests/cpp/ternary_arith_test.cpp`.
2. **`WL-ARITH-LIMB-MUL`** (Domain A — Arithmetic): 27-tryte software limb Booth multiplication (`T81Limb::operator*`). Isolated software characterization only; binary hardware ALU comparator is excluded.
3. **`WL-ARITH-SWAR-TRITWISE`** (Domain A — Arithmetic): Packed 2-bit-per-trit SWAR logic operations across vector sizes $N \in [256, 65536]$. Correctness oracle: `tests/cpp/test_tritwise_backend_equivalence.cpp`.
4. **`WL-VM-TISC-DETERMINISM-TIERS`** (Domain B — TISC / HanoiVM): TISC virtual machine interpreter execution latency and state determinism across multi-tier programs. Correctness oracle: 64-bit state signature hash over 48 registers, tags, PC, SP, trace, and Axion log count.
5. **`WL-VM-HANOI-COMMAND-SURFACE`** (Domain B — TISC / HanoiVM): Hanoi in-memory kernel lifecycle operations (`boot`, `status`, `optimize`, `simulate`, `snapshot`, `rollback`). Correctness oracle: RFC-0000 §7 assertions in `tests/cpp/hanoi_command_surface_test.cpp`.
6. **`WL-CANONFS-MEM-THROUGHPUT`** (Domain C — CanonFS): In-memory content-addressed block write and read throughput with CanonHash SHA3 verification. Correctness oracle: Hash invariance and byte-exact read-back in `tests/cpp/canonfs_driver_test.cpp`.
7. **`WL-RES-RUSAGE-METRICS`** (Domain D — Resource Consumption): Process-level peak resident set size (`ru_maxrss`), user/system CPU time, and monotonic wall time via POSIX `getrusage`. Non-intrusive host resource accounting.
8. **`WL-RES-ARTIFACT-SIZE`** (Domain D — Resource Consumption): Static compiled binary disk footprint and ELF/Mach-O section header breakdown (`.text`, `.rodata`, `.data`, `.bss`). Zero variance.

### 2.2 Conditional Workload IDs (1)
1. **`WL-CANONFS-PERSISTENT-IO`** (Domain C — CanonFS): Filesystem-backed persistent directory block write, fsync, and read-back latency under Axion policy. **Condition:** Requires provisioning a dedicated non-shared scratch filesystem and explicit tmpfs/disk characterization to bound background I/O noise before benchmarking.

### 2.3 Excluded Workload IDs (2)
1. **`WL-VM-CMP3-DECISION`** (Domain B — TISC / HanoiVM): Hardware PMU measurement of CMP3 three-state branch vs binary jumps. **Reason:** Frozen experiment (EXP-T81-COMP3-01 / H05). Unapproved protocol amendment requires formal maintainer Ed25519 signing and R01–R11 implementation repairs. PMU counters remain blocked.
2. **`WL-CANONFS-BLOCK-BACKED-POLICY`** (Domain C — CanonFS): Axion policy hook overhead on block-backed mutations (`Publish`, `Revoke`, `Repair`). **Reason:** Introduced in draft PR #686 (commit `ac7397aa5f8751d34adcb6ce2d38e3141e65b36c`). Unmerged, diverged from `origin/main`, and unvalidated on Linux/Windows. Must not be silently included in the reference baseline.

---

## 3. Four-Domain Minimal Phase B Workload Set

To ensure defensible, rapid, and reproducible baselining without expanding into unmaintainable complexity, the recommended minimal Phase B execution set consists of the following primary workloads:

| Domain | Workload ID | Operation Measured | Correctness Oracle | Measurement Boundary |
|---|---|---|---|---|
| **Domain A (Arithmetic)** | `WL-ARITH-CODEC-I64` | Horner MSB-first `encode_i64` / `decode_i64` | `decode_i64(encode_i64(x)) == x` on $[-2^{63}, 2^{63}-1]$ | In-register scalar conversion loop; zero dynamic allocations in inner loop; nanoseconds per operation. |
| **Domain B (TISC/HanoiVM)** | `WL-VM-TISC-DETERMINISM-TIERS` | TISC interpreter dispatch loop (5,000 steps) | 64-bit state signature hash over 48 registers + PC/SP | In-memory bytecode execution; isolated from CMP3; microseconds per 1,000 instructions. |
| **Domain C (CanonFS)** | `WL-CANONFS-MEM-THROUGHPUT` | Content-addressed `write_object` & `read_object_bytes` | CanonHash invariance & byte read equality | In-memory CAS driver; $4\,\text{KiB}, 64\,\text{KiB}, 1\,\text{MiB}$ block scaling; Megabytes per second ($\text{MB/s}$). |
| **Domain D (Resource)** | `WL-RES-RUSAGE-METRICS` & `WL-RES-ARTIFACT-SIZE` | Process peak RSS, CPU time, and binary sizes | POSIX `getrusage` monotonic invariants & file stat SHA-256 | Process-level wrapper around test runs; normalized KiB RSS and byte-exact static binary section sizing. |

### Correctness Oracle & Measurement Boundaries
- **Pre-Execution Correctness Verification:** Every measurement trial must execute its associated unit test or mathematical verification oracle prior to the timed loop. Any failure halts immediately with non-zero exit code, discarding all timing records.
- **Strict Conventional Hardware Scope:** All workloads characterize Foundation's software implementation running on conventional binary host ALUs and operating system abstractions. No workload claims or infers a native ternary hardware advantage.

---

## 4. Remaining Requirements for Phase B Measurement Activation

Before Phase B benchmarking may be activated, the following engineering requirements must be completed:

### 4.1 Runner Scaffolding & Validation
- Implement a dedicated C++20 / Python Phase B runner (e.g. `benchmarks/base01_runner.cpp` or `scripts/benchmarks/base01_runner.py`) that executes the 4 minimal workloads under uniform harness contracts.
- Runner must support dry-run validation (`--dry-run` or `--validate-only`) that executes all correctness oracles and prints workload metadata without recording performance measurements.

### 4.2 Host Environment Identification & Configuration Controls
- Hardware characterization capture: Host CPU model, microarchitecture, clock frequencies, L1d/L1i/L2/L3 cache sizes, RAM size, OS kernel release (`uname -a`), and compiler toolchain versions (`gcc --version` / `clang --version`).
- Operating system execution environment lockdown:
  - Process execution pinned to physical core 2 (`taskset -c 2`).
  - CPU frequency governor asserted to `performance` mode.
  - Verification that system load average $\le 0.10$ before launching trials.

### 4.3 Raw Measurement & Metadata Retention
- Durable JSON receipt generation: Every measurement trial must write an immutable run receipt recording host metadata, git commit SHA, compiler flags, iteration timestamps, raw wall-clock timings ($K=30$), CPU times, peak RSS, and calculated statistics (Median, IQR, CV).
- Stability Gate: Enforcement of the noise threshold: if $\text{CV} = \sigma / \mu > 5.0\%$, the run is automatically flagged `UNSTABLE / INCONCLUSIVE`.

### 4.4 Explicit Maintainer Authorization Gate
- Benchmark execution must remain fail-closed behind an explicit CLI authorization switch (e.g. `--authorize-base01-measurement`) and require maintainer sign-off.
- No automated CI workflow or agent may launch performance benchmarking without explicit human approval.

---

## 5. Scope Boundaries: Prohibited Phase B Actions

Phase B is strictly bounded to the characterization of Foundation software on conventional binary hosts. **Phase B must NOT execute:**
1. Any PMU hardware event collection or branch misprediction counting (`WL-VM-CMP3-DECISION`).
2. Any H04-R03 Phase C performance measurement trials or protected dataset reuse.
3. Any physical SPICE simulations or ternary circuit modeling.
4. Any comparison claiming ternary hardware speedup over native binary 64-bit integer ALUs.
5. Any merge of unvalidated draft PRs (specifically PR #686).

---

## 6. Recommended Next Engineering Action

The recommended next concrete engineering action is:
**Author the non-measuring Phase B harness scaffolding and dry-run validator (`--validate-only`) to confirm environment detection and correctness oracle pass-rates across Linux and macOS, while keeping performance measurement disabled pending maintainer sign-off.**
