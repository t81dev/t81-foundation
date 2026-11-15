# BASE-01 Reference Performance Baseline: Workload Discovery, Eligibility Audit, and Measurement Readiness (Phase A)

**Document:** `docs/research/base01/BASE01_WORKLOAD_AUDIT.md`
**Mission:** BASE-01 Phase A — Workload Discovery, Eligibility Audit, and Measurement Readiness
**Status:** READ-ONLY INVESTIGATION & REPRODUCIBILITY AUDIT (PHASE A ONLY)
**Date:** 2026-11-03
**Auditor:** Jules (AI Software Engineer)
**Authorization Boundary:** STRICTLY NO PERFORMANCE MEASUREMENTS, PROTOCOL ACTIVATIONS, OR MERGES

---

## 1. Executive Summary & Source Inspection

### 1.1 Mission & Objectives
The goal of BASE-01 is to establish the groundwork for T81 Foundation's first consolidated, reproducible conventional binary-host performance baseline. T81's historical performance findings are distributed across diverse experiments, architectural configurations, and research protocols. Phase A establishes a rigorous inventory of existing candidate workloads across four core domains, assesses their eligibility against frozen research boundaries, evaluates correctness oracles, and designs a reproducible measurement methodology without executing performance benchmarks or claiming ternary hardware superiority.

### 1.2 Exact Repository Revision Pins
- **Audited Repository:** `https://github.com/t81dev/t81-foundation`
- **Inspected Base HEAD Commit (`origin/main` & working tree):** `cda9135d36144d4de2771af174e52ac7b30a83c9`
  - Commit message: `Merge PR #684: independent H04-R03 enforcement feasibility audit`
  - Integration status: Clean, up-to-date with `origin/main`.
- **C1-M1 Draft Reference Commit (PR #686):** `ac7397aa5f8751d34adcb6ce2d38e3141e65b36c`
  - Branch: `origin/codex/c1-m1-block-policy`
  - PR title: `fix(canonfs): authorize block-backed metadata mutations`
  - Commit status: **DRAFT / UNMERGED**. Adds Axion policy checks to CanonFS block-backed mutations (`Publish`, `Revoke`, `Repair`). Tested only on macOS ARM64 Apple Clang 21; Linux and Windows remain unvalidated. In accordance with Section 2 of the prompt, PR #686 is **not** merged and must **not** be silently included in the reference baseline.

### 1.3 Mandatory Research Boundaries & Freeze Constraints
In strict accordance with Section 3:
1. **CMP3 (EXP-T81-COMP3-01):** Comparison experiment in `experiments/cmp3_decision_advantage/` evaluating three-state comparator branches against binary jumps. PMU counters and execution remain unapproved and blocked. No CMP3 workloads, comparator definitions (`cmp3_s64`), PMU scripts, or frozen configurations are reused or measured under BASE-01.
2. **H04-R03 Phase C:** Matched codec study in `experiments/h04_r03_matched_codec/` (Candidate A B729-µ5). The correctness phase (Phase B) is accepted, but Phase C performance execution is explicitly frozen (`--run-measurement` halts fail-closed with code 1; Linux feasibility audit `H04_R03_ENFORCEMENT_FEASIBILITY_AUDIT.md` classified stock Linux RSS enforcement as unresolved/blocked). Protected datasets (`1188f12c...`) and query sequences (`02e5f3da...`) are not repurposed or measured.
3. **Closed & Modeled Investigations (H01, H02, H03, H05):**
   - **H01:** Physical ternary storage evidence audit (Investigation 10A) disposition is `PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE`. No physical silicon claims are made.
   - **H02:** Balanced ternary arithmetic CPU closure (`docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`) proved scalar ternary arithmetic on binary CPUs suffers net slowdowns compared to native binary ALUs. BASE-01 measures Foundation's hosted software operations on conventional binary hosts without reopening arithmetic speedup competitions.
   - **H05:** Native ternary proof-of-advantage architecture study (ADR-0007) is `FEASIBILITY BLOCKED FOR SIMULATION PREPARATION`.
   - **Duotronic / Fabric:** Independent research components; excluded from Foundation reference baselines.

---

## 2. Existing Benchmark and Test Infrastructure Inventory

T81 Foundation possesses compiled benchmark targets and testing infrastructure under `benchmarks/` and `tests/`:

1. **Google Benchmark Runner (`benchmark_runner`):**
   - Declared in `benchmarks/CMakeLists.txt`: compiled executable target is `./build/benchmarks/benchmark_runner`.
   - Included suites: `BM_LimbMul.cpp`, `BM_TensorMatMul.cpp`, `BM_CanonFS.cpp`, `BM_LlamaGguf.cpp`, `BM_LlamaKernels.cpp`, `BM_NativeWeightsExecution.cpp`, `BM_TensorPromotion.cpp`, `BM_Lexer.cpp`, `BM_PackedTritVector.cpp`, `BM_TritwiseWorkloads.cpp`, `BM_DispatchOverhead.cpp`, `BM_VMSimulation.cpp`, `BM_DeterminismValidation.cpp`, `BM_GovernedWorkflows.cpp`, `BM_InferenceComparison.cpp`, `BM_Base81SIMD.cpp`.
2. **CTest Infrastructure:**
   - 158 static literal `add_test()` targets covering unit arithmetic, ISA encoding, VM dispatch, CanonFS drivers, and determinism.
3. **Experimental Harnesses:**
   - Standalone runners in `experiments/` with distinct governance boundaries.

---

## 3. Workload Eligibility Audit

Every candidate workload has been evaluated across the 18 mandatory criteria defined in Section 5.

### 3.1 Domain A: Arithmetic

#### Workload 1: `WL-ARITH-CODEC-I64`
1. **Identifier:** `WL-ARITH-CODEC-I64`
2. **Domain:** Domain A — Arithmetic
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** Header `include/t81/ternary/arith.hpp`, test target `ternary_arith_test` (`tests/cpp/ternary_arith_test.cpp`).
5. **Purpose & Operation:** Measures scalar Horner MSB-first balanced ternary conversion (`encode_i64` and `decode_i64`) for signed 64-bit integers on a binary host CPU.
6. **Input Definition:** Deterministic set of 100,000 signed 64-bit integers spanning zero, small values ($\pm 1, \pm 2, \dots$), powers of 3 up to $3^{40}$, boundary cancellations, and saturation extrema (`INT64_MAX`, `INT64_MIN`).
7. **Fixture Origin & Identity:** Generated via deterministic standard linear-congruential sequence with fixed seed `0x5438315F41524954` (`"T81_ARIT"`).
8. **Correctness Oracle:** `tests/cpp/ternary_arith_test.cpp` round-trip identity assertions: `decode_i64(encode_i64(x)) == x` for all $x \in [-2^{63}, 2^{63}-1]$, plus exact saturation bounds verified under Horner MSB evaluation.
9. **Proposed Build Command:** `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target t81_ternary_arith_test`
10. **Proposed Execution Command:** `./build/t81_ternary_arith_test` (for verification); harness execution via dedicated Phase B benchmark driver.
11. **Proposed Measurement Method:** High-resolution monotonic timer (`std::chrono::steady_clock`), measuring nanoseconds per integer round-trip conversion without dynamic memory allocation in the inner loop.
12. **Units & Primary Metric:** Nanoseconds per conversion (latency) and operations per second (throughput).
13. **Warm-up & Repetition:** 10,000 unmeasured warm-up iterations; 30 repetitions of 100,000 iterations each; reporting median, interquartile range (IQR), and coefficient of variation (CV).
14. **Environment Requirements:** Conventional x86_64 or AArch64 host, single thread pinned to an isolated physical core, Linux/macOS.
15. **Historical Overlap:** Relates to H02 arithmetic investigations. PR #675 fixed Horner evaluation; verified to not make radix superiority claims.
16. **Frozen-Protocol Compatibility:** Compatible. Does not touch CMP3, H04-R03, or PMU counters.
17. **Sources of Noise or Bias:** Small input size causing loop unrolling or compiler auto-vectorization differences across compilers; mitigated by `benchmark::DoNotOptimize`.
18. **Eligibility Decision & Rationale:** **ELIGIBLE**. Pure hosted arithmetic conversion function with verified mathematical oracle, non-allocating execution path, and zero frozen protocol conflicts.

#### Workload 2: `WL-ARITH-LIMB-MUL`
1. **Identifier:** `WL-ARITH-LIMB-MUL`
2. **Domain:** Domain A — Arithmetic
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** Header `include/t81/types/T81Limb.hpp`, executable `benchmark_runner` (suite `BM_LimbMul.cpp`).
5. **Purpose & Operation:** Measures multi-tryte limb Booth multiplication (`T81Limb::operator*`) on a conventional binary host.
6. **Input Definition:** Fixed 27-tryte limb operands populated deterministically via `a.set_tryte(i, (i % 27) - 13)` and `b.set_tryte(i, ((i + 7) % 27) - 13)` across batches of 4,096 operations.
7. **Fixture Origin & Identity:** Static procedural formula in `benchmarks/BM_LimbMul.cpp`.
8. **Correctness Oracle:** `tests/cpp/test_t81_limb_booth_mul.cpp` and `tests/cpp/test_t81_limb_mul_wide_high.cpp`.
9. **Proposed Build Command:** `cmake -B build -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_BENCHMARKS=ON && cmake --build build --target benchmark_runner`
10. **Proposed Execution Command:** `./build/benchmarks/benchmark_runner --benchmark_filter='^BM_LimbMul_Booth$' --benchmark_min_time=1.0s`
11. **Proposed Measurement Method:** Google Benchmark CPU and wall-clock execution timing per iteration batch.
12. **Units & Primary Metric:** Nanoseconds per limb multiplication (ns/op) and Mops/sec.
13. **Warm-up & Repetition:** Google Benchmark automatic warm-up; minimum 10 measured trial repetitions; target CV $\le 3\%$.
14. **Environment Requirements:** Conventional binary host, single thread pinned, CPU frequency scaling governor set to performance.
15. **Historical Overlap:** Investigated under H02 and closed PRs #544–#549. Historical closure established that software limb multiplication is slower than binary hardware multiplier ALUs.
16. **Frozen-Protocol Compatibility:** Compatible as a characterization of Foundation software limb ops on binary hardware, provided it is **not** presented as a competitive speedup claim over 64-bit integer ALUs.
17. **Sources of Noise or Bias:** Pre-existing binary comparator `BM_LimbMul_Booth_Binary` in `BM_LimbMul.cpp` compares 27-tryte software emulation against single-cycle hardware `unsigned __int128` multiplication, introducing semantic mismatch.
18. **Eligibility Decision & Rationale:** **ELIGIBLE** (isolated limb characterization only; binary comparator excluded to avoid invalid semantic contrast).

#### Workload 3: `WL-ARITH-SWAR-TRITWISE`
1. **Identifier:** `WL-ARITH-SWAR-TRITWISE`
2. **Domain:** Domain A — Arithmetic
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** `include/t81/tritwise/tritwise.hpp`, `benchmarks/BM_TritwiseWorkloads.cpp`, test `tests/cpp/test_swar.cpp`.
5. **Purpose & Operation:** Measures packed 2-bit-per-trit SWAR logic operations (`BM_MaskApply_Library` vs `BM_PatternMatch_Library`) across vector lengths $N \in [256, 65536]$.
6. **Input Definition:** Pseudorandom ternary vectors drawn with fixed MT19937 seed 42 over $\{-1, 0, 1\}$.
7. **Fixture Origin & Identity:** Procedural generation in `benchmarks/BM_TritwiseWorkloads.cpp`.
8. **Correctness Oracle:** `tests/cpp/test_tritwise_backend_equivalence.cpp` (verifying SWAR bitwise transformations match scalar and reference models).
9. **Proposed Build Command:** `cmake -B build -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_BENCHMARKS=ON && cmake --build build --target benchmark_runner`
10. **Proposed Execution Command:** `./build/benchmarks/benchmark_runner --benchmark_filter='^BM_(MaskApply|PatternMatch)_Library/'`
11. **Proposed Measurement Method:** Google Benchmark timer with `benchmark::DoNotOptimize`.
12. **Units & Primary Metric:** Microseconds ($\mu\text{s}$) per vector operation; throughput in Megatrits/sec.
13. **Warm-up & Repetition:** Standard Google Benchmark repetition policy; 15 trials per vector length.
14. **Environment Requirements:** Conventional binary x86_64 / AArch64 host; ensure `T81_TRITWISE_PROFILE` is disabled.
15. **Historical Overlap:** Investigated in historical benchmark logs (`benchmarks/results/archive/benchmark_results_release.txt`). Historical claims (1.48 $\mu\text{s}$) were corrected to 1.07 $\mu\text{s}$ (library) in prior audits.
16. **Frozen-Protocol Compatibility:** Fully compatible.
17. **Sources of Noise or Bias:** `T81_TRITWISE_PROFILE` compiler flag alters execution; must be asserted OFF.
18. **Eligibility Decision & Rationale:** **ELIGIBLE**. Validated software SWAR primitive with strict equivalence testing.

---

### 3.2 Domain B: TISC / HanoiVM

#### Workload 4: `WL-VM-TISC-DETERMINISM-TIERS`
1. **Identifier:** `WL-VM-TISC-DETERMINISM-TIERS`
2. **Domain:** Domain B — TISC / HanoiVM
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** `vm/vm.cpp`, `tests/cpp/vm_workload_determinism_tiers_test.cpp`, executable `build/tests/cpp/vm_workload_determinism_tiers_test`.
5. **Purpose & Operation:** Evaluates TISC virtual machine interpreter dispatch latency and state transitions across multi-tier deterministic computational workloads (arithmetic accumulation loop, register permutation, memory indexing).
6. **Input Definition:** Pre-encoded deterministic TISC programs (`t81::tisc::Program`) constructed in `vm_workload_determinism_tiers_test.cpp` executed up to 5,000 steps.
7. **Fixture Origin & Identity:** C++ source construction in `tests/cpp/vm_workload_determinism_tiers_test.cpp`.
8. **Correctness Oracle:** Exact 64-bit state signature verification (`sig_mix` over all 48 registers, register tags, PC, SP, trace length, and Axion log count) matching frozen expected signatures.
9. **Proposed Build Command:** `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target t81_vm_workload_determinism_tiers_test`
10. **Proposed Execution Command:** `./build/t81_vm_workload_determinism_tiers_test` (for test validation); wrapped in Phase B harness for execution timing.
11. **Proposed Measurement Method:** In-process monotonic timer measuring milliseconds per 10,000 interpreter instruction cycles; verification that state hash matches expected constant after every run.
12. **Units & Primary Metric:** Microseconds per 1,000 TISC instructions ($\mu\text{s}/1\text{k-insn}$) and total run duration.
13. **Warm-up & Repetition:** 5 warm-up iterations; 20 measured repetitions; CV check $\le 3\%$.
14. **Environment Requirements:** Linux/macOS binary host, single thread.
15. **Historical Overlap:** Relates to H03 and H07. Independent from CMP3 and H04-R03.
16. **Frozen-Protocol Compatibility:** Compatible. Uses standard hosted interpreter (`make_interpreter_vm()`), not CMP3 branch microbenchmarks.
17. **Sources of Noise or Bias:** Dynamic heap allocation during trace/axion_log generation.
18. **Eligibility Decision & Rationale:** **ELIGIBLE**. Provides an established deterministic execution oracle with zero external model dependencies.

#### Workload 5: `WL-VM-HANOI-COMMAND-SURFACE`
1. **Identifier:** `WL-VM-HANOI-COMMAND-SURFACE`
2. **Domain:** Domain B — TISC / HanoiVM
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** `experimental/hanoi/in_memory_kernel.cpp`, `tests/cpp/hanoi_command_surface_test.cpp`.
5. **Purpose & Operation:** Measures latency of Hanoi in-memory kernel lifecycle operations: `boot()`, `status()`, `optimize()`, `simulate()`, `snapshot()`, and `rollback()`.
6. **Input Definition:** Fixed sequence of operations: boot, memory compaction, GC optimization, 3-step simulation (`load`, `compute`, `store`), snapshot creation, and root rollback.
7. **Fixture Origin & Identity:** Defined in `tests/cpp/hanoi_command_surface_test.cpp` per RFC-0000 §7.
8. **Correctness Oracle:** Return status booleans, `active_snapshots` count verification, and trace verification.
9. **Proposed Build Command:** `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target t81_hanoi_command_surface_test`
10. **Proposed Execution Command:** `./build/t81_hanoi_command_surface_test`
11. **Proposed Measurement Method:** Wall-clock duration per complete kernel lifecycle sequence.
12. **Units & Primary Metric:** Microseconds ($\mu\text{s}$) per complete command sequence.
13. **Warm-up & Repetition:** 10 warm-up runs, 50 measured repetitions.
14. **Environment Requirements:** Conventional binary host, single thread.
15. **Historical Overlap:** Originates from Hanoi CWEB lineage (Stage B of Master Continuity Charter). Experimental surface, but hosted implementation in C++.
16. **Frozen-Protocol Compatibility:** Compatible; completely isolated from frozen experiments.
17. **Sources of Noise or Bias:** In-memory string allocations in trace generation.
18. **Eligibility Decision & Rationale:** **ELIGIBLE**. Exercises HanoiVM kernel lifecycle commands under hosted execution.

#### Workload 6: `WL-VM-CMP3-DECISION`
1. **Identifier:** `WL-VM-CMP3-DECISION`
2. **Domain:** Domain B — TISC / HanoiVM
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** `experiments/cmp3_decision_advantage/`
5. **Purpose & Operation:** Hardware PMU measurement of CMP3 three-state branch vs binary jump cascade.
6. **Input Definition:** Frozen synthetic traces in CMP3 protocol.
7. **Fixture Origin & Identity:** `experiments/cmp3_decision_advantage/protocol/r1/`
8. **Correctness Oracle:** N/A (frozen).
9. **Proposed Build Command:** N/A
10. **Proposed Execution Command:** N/A
11. **Proposed Measurement Method:** N/A
12. **Units & Primary Metric:** Instructions / branch mispredictions.
13. **Warm-up & Repetition:** N/A
14. **Environment Requirements:** Dedicated bare-metal host with PMU.
15. **Historical Overlap:** Hypotheses H05 / EXP-T81-COMP3-01.
16. **Frozen-Protocol Compatibility:** **INCOMPATIBLE & FROZEN**. Protocol Section 7 forbids execution without explicit maintainer Ed25519 authorization.
17. **Sources of Noise or Bias:** Unresolved compiler lowering artifacts.
18. **Eligibility Decision & Rationale:** **EXCLUDED**. Explicitly forbidden by Section 3 of the prompt.

---

### 3.3 Domain C: CanonFS

#### Workload 7: `WL-CANONFS-MEM-THROUGHPUT`
1. **Identifier:** `WL-CANONFS-MEM-THROUGHPUT`
2. **Domain:** Domain C — CanonFS
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** `fs/`, `benchmarks/BM_CanonFS.cpp`, test `tests/cpp/canonfs_driver_test.cpp`.
5. **Purpose & Operation:** Evaluates in-memory content-addressed block storage write and read throughput (`write_object` and `read_object_bytes`) with SHA3/CanonHash computation.
6. **Input Definition:** Bounded synthetic data blocks of sizes $4\,\text{KiB}, 64\,\text{KiB}, 1\,\text{MiB}$, filled deterministically via fixed seed PRNG.
7. **Fixture Origin & Identity:** Generated in `benchmarks/BM_CanonFS.cpp`.
8. **Correctness Oracle:** Content-addressable hash invariance (writing identical bytes returns identical `CanonHash`) and byte-for-byte read-back equality (`read == written`).
9. **Proposed Build Command:** `cmake -B build -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_BENCHMARKS=ON && cmake --build build --target benchmark_runner`
10. **Proposed Execution Command:** `./build/benchmarks/benchmark_runner --benchmark_filter='^BM_CanonFS_(Write|Read)Throughput_InMemory'`
11. **Proposed Measurement Method:** Google Benchmark throughput tracking (`SetBytesProcessed`).
12. **Units & Primary Metric:** Megabytes per second ($\text{MB/s}$) and microseconds per operation.
13. **Warm-up & Repetition:** Google Benchmark auto-scaling; minimum 15 iterations.
14. **Environment Requirements:** Conventional binary host, single thread.
15. **Historical Overlap:** Subsystem performance tracking; independent from H04-R03.
16. **Frozen-Protocol Compatibility:** Compatible.
17. **Sources of Noise or Bias:** Memory allocator fragmentation on large blocks; mitigated by fixed block sizing.
18. **Eligibility Decision & Rationale:** **ELIGIBLE**. Stable, non-persistent baseline for CanonFS content-addressing throughput.

#### Workload 8: `WL-CANONFS-PERSISTENT-IO`
1. **Identifier:** `WL-CANONFS-PERSISTENT-IO`
2. **Domain:** Domain C — CanonFS
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** `fs/persistent_driver.cpp`, `benchmarks/BM_CanonFS.cpp`, test `tests/cpp/canonfs_persistent_driver_test.cpp`.
5. **Purpose & Operation:** Measures filesystem-backed persistent storage write, directory indexing, and read-back latency under governed Axion policy checks.
6. **Input Definition:** Controlled payload files written to temporary directory on host filesystem (`fs::temp_directory_path()`).
7. **Fixture Origin & Identity:** Dynamically generated test blobs with SHA3-256 integrity tags.
8. **Correctness Oracle:** `tests/cpp/canonfs_persistent_driver_test.cpp` (verifying across separate driver instances that persistent storage reloads and matches).
9. **Proposed Build Command:** `cmake -B build -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_BENCHMARKS=ON && cmake --build build --target benchmark_runner`
10. **Proposed Execution Command:** `./build/benchmarks/benchmark_runner --benchmark_filter='^BM_CanonFS_.*_Persistent'`
11. **Proposed Measurement Method:** Google Benchmark wall-clock timing with filesystem sync barriers.
12. **Units & Primary Metric:** Latency per block flush ($\mu\text{s}$) and persistent write throughput ($\text{MB/s}$).
13. **Warm-up & Repetition:** 5 warm-up iterations; 20 repetitions with pre-allocated temporary directories.
14. **Environment Requirements:** Host filesystem with POSIX write/fsync semantics.
15. **Historical Overlap:** RFC-0021 persistent storage architecture.
16. **Frozen-Protocol Compatibility:** Compatible.
17. **Sources of Noise or Bias:** Host disk I/O caching and filesystem journal latency.
18. **Eligibility Decision & Rationale:** **CONDITIONAL**. Requires a dedicated non-shared scratch filesystem and explicit tmpfs/disk characterization before benchmarking.

#### Workload 9: `WL-CANONFS-BLOCK-BACKED-POLICY`
1. **Identifier:** `WL-CANONFS-BLOCK-BACKED-POLICY`
2. **Domain:** Domain C — CanonFS
3. **Repository & Commit:** PR #686 commit `ac7397aa5f8751d34adcb6ce2d38e3141e65b36c` (diverged from `cda9135`)
4. **Source & Existing Target:** `fs/block_backed_driver.cpp`, `tests/cpp/canonfs_policy_boundary_test.cpp`.
5. **Purpose & Operation:** Measures Axion policy hook overhead on block-backed mutations (`Publish`, `Revoke`, `Repair`).
6. **Input Definition:** Mock block device operations with allow and deny policy scripts.
7. **Fixture Origin & Identity:** `tests/cpp/canonfs_policy_boundary_test.cpp`.
8. **Correctness Oracle:** Axion hook interception counts and device write prevention checks.
9. **Proposed Build Command:** Dependent on unmerged PR #686 branch.
10. **Proposed Execution Command:** Dependent on unmerged PR #686 branch.
11. **Proposed Measurement Method:** Policy hook latency delta.
12. **Units & Primary Metric:** Nanoseconds per authorization check.
13. **Warm-up & Repetition:** N/A
14. **Environment Requirements:** macOS ARM64 / Linux x86_64.
15. **Historical Overlap:** C1 Architecture Convergence / PR #686.
16. **Frozen-Protocol Compatibility:** N/A (unmerged code).
17. **Sources of Noise or Bias:** Unmerged branch dependencies.
18. **Eligibility Decision & Rationale:** **EXCLUDED / CONDITIONAL ON MERGE**. Per Section 2 of prompt, PR #686 is a draft and must not be merged or silently included in the reference baseline. If evaluated in future phases, it requires explicit maintainer merge authorization.

---

### 3.4 Domain D: Resource Consumption

#### Workload 10: `WL-RES-RUSAGE-METRICS`
1. **Identifier:** `WL-RES-RUSAGE-METRICS`
2. **Domain:** Domain D — Resource Consumption
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** POSIX standard library API (`sys/resource.h` / Python `resource` module); harness integration in `tests/cpp/test_resource_monitoring.cpp`.
5. **Purpose & Operation:** Accurately measures process-level peak resident set size (`ru_maxrss`), user CPU time (`ru_utime`), system CPU time (`ru_stime`), and wall-clock duration for any executed workload.
6. **Input Definition:** Standardized observation harness wrapper wrapping workload executions.
7. **Fixture Origin & Identity:** OS kernel resource accounting counters via `getrusage(RUSAGE_SELF, ...)`.
8. **Correctness Oracle:** Monotonic increase checks: $\Delta t_{\text{wall}} \ge 0$, $\Delta t_{\text{cpu}} \ge 0$, $\text{ru\_maxrss} > 0$.
9. **Proposed Build Command:** Standard C++20 / Python 3.12 standard library (no external dependencies).
10. **Proposed Execution Command:** Integrated into Phase B benchmark executor.
11. **Proposed Measurement Method:** Pre- and post-execution sampling of `struct rusage` and `std::chrono::steady_clock::now()`.
12. **Units & Primary Metric:** Kilobytes (KB) for peak RSS, seconds (with microsecond resolution) for CPU/wall time.
13. **Warm-up & Repetition:** 1 sampling call per benchmark trial invocation.
14. **Environment Requirements:** POSIX-compliant host (Linux `ru_maxrss` in KiB; macOS `ru_maxrss` in bytes — normalization required).
15. **Historical Overlap:** Connects directly to H04-R03 enforcement feasibility findings (`H04_R03_ENFORCEMENT_FEASIBILITY_AUDIT.md`), which highlighted that `ru_maxrss` measures high-water mark page table allocation rather than instantaneous RSS.
16. **Frozen-Protocol Compatibility:** Compatible.
17. **Sources of Noise or Bias:** OS kernel page reclamation, lazy page allocation, platform unit divergence (KiB on Linux vs Bytes on Darwin).
18. **Eligibility Decision & Rationale:** **ELIGIBLE**. Essential, non-intrusive resource observation primitive for all host measurements.

#### Workload 11: `WL-RES-ARTIFACT-SIZE`
1. **Identifier:** `WL-RES-ARTIFACT-SIZE`
2. **Domain:** Domain D — Resource Consumption
3. **Repository & Commit:** `https://github.com/t81dev/t81-foundation` @ `cda9135d36144d4de2771af174e52ac7b30a83c9`
4. **Source & Existing Target:** File system inspection of built binaries (`t81`, `benchmark_runner`) and test fixtures.
5. **Purpose & Operation:** Quantifies static serialized disk footprint, binary section sizes (`.text`, `.rodata`, `.data`, `.bss`), and content-addressed storage object sizes.
6. **Input Definition:** Built compilation artifacts in `build/` directory.
7. **Fixture Origin & Identity:** Compiler output binaries.
8. **Correctness Oracle:** Byte-exact file sizing via `std::filesystem::file_size` and SHA-256 hash digests.
9. **Proposed Build Command:** `cmake --build build --target t81 benchmark_runner`
10. **Proposed Execution Command:** `size build/t81` / `ls -ln build/t81` / `sha256sum build/t81`
11. **Proposed Measurement Method:** Direct stat inspection and ELF/Mach-O section header parsing.
12. **Units & Primary Metric:** Exact byte counts.
13. **Warm-up & Repetition:** 1 deterministic measurement post-build.
14. **Environment Requirements:** Host filesystem.
15. **Historical Overlap:** Connects to artifact density tracking in H03 and H04.
16. **Frozen-Protocol Compatibility:** Compatible.
17. **Sources of Noise or Bias:** Compiler debug symbols; requires consistent `-DCMAKE_BUILD_TYPE=Release` without debug info for comparison.
18. **Eligibility Decision & Rationale:** **ELIGIBLE**. Completely deterministic, zero-noise measurement.

---

## 4. Workload Classification Summary

| Workload ID | Domain | Description | Decision | Key Rationale / Boundary |
|---|---|---|---|---|
| `WL-ARITH-CODEC-I64` | A (Arith) | `encode_i64` / `decode_i64` round-trip | **ELIGIBLE** | Pure host scalar codec; verified mathematical oracle; no freeze conflict. |
| `WL-ARITH-LIMB-MUL` | A (Arith) | `T81Limb` Booth multiplication | **ELIGIBLE** | Self-contained software limb op; binary ALU comparison excluded. |
| `WL-ARITH-SWAR-TRITWISE` | A (Arith) | SWAR packed tritwise operations | **ELIGIBLE** | Deterministic logic vector operations; equivalence validated. |
| `WL-VM-TISC-DETERMINISM-TIERS` | B (VM) | TISC interpreter determinism run | **ELIGIBLE** | 48-register deterministic hash oracle; no CMP3 overlap. |
| `WL-VM-HANOI-COMMAND-SURFACE` | B (VM) | Hanoi in-memory kernel lifecycle | **ELIGIBLE** | Hosted RFC-0000 §7 command verification; independent. |
| `WL-VM-CMP3-DECISION` | B (VM) | CMP3 PMU branch comparator | **EXCLUDED** | Frozen protocol; requires unobtained maintainer Ed25519 signing. |
| `WL-CANONFS-MEM-THROUGHPUT` | C (CanonFS) | In-memory block write/read | **ELIGIBLE** | High-throughput content-addressed hashing; clean oracle. |
| `WL-CANONFS-PERSISTENT-IO` | C (CanonFS) | Persistent directory write/read | **CONDITIONAL** | Requires dedicated host scratch filesystem to bound I/O noise. |
| `WL-CANONFS-BLOCK-BACKED-POLICY`| C (CanonFS) | Block driver Axion mutations | **EXCLUDED** | Draft PR #686 unmerged; Linux/Windows unvalidated. |
| `WL-RES-RUSAGE-METRICS` | D (Resource) | POSIX `getrusage` RSS & CPU time | **ELIGIBLE** | Non-invasive host process accounting; platform-normalized. |
| `WL-RES-ARTIFACT-SIZE` | D (Resource) | Binary ELF/Mach-O section sizing | **ELIGIBLE** | Exact deterministic static bytes; zero variance. |

---

## 5. Proposed Phase B Measurement Methodology

For eligible workloads, the following protocol must be strictly observed in any separately authorized Phase B execution:

1. **Host Qualification & Environmental Controls:**
   - Single isolated physical core via CPU pinning (`taskset -c 2` on Linux).
   - Frequency governor locked to `performance` mode; turbo boost disabled where supported to eliminate dynamic thermal throttling.
   - Host platform identification recorded: CPU model, L1/L2/L3 cache topology, total RAM, OS kernel version, architecture (`uname -a`).
2. **Build Configuration:**
   - Compiler: GCC 13+ or Clang 18+ (recorded explicitly).
   - Flags: `-O3 -DNDEBUG -std=c++20 (or -std=c++23 if T81_USE_CXX23=ON)`.
   - Feature flags: `T81_TRITWISE_PROFILE=OFF`, `T81_ENABLE_DPE=OFF` (for baseline core).
3. **Trial Repetition & Statistical Aggregation:**
   - Pre-measurement discard: minimum 5 unmeasured warm-up iterations.
   - Measurement iterations: $K = 30$ independent runs per workload.
   - Robust statistics: Report Median, Interquartile Range (IQR), Minimum, Maximum, and Coefficient of Variation ($\text{CV} = \sigma / \mu$).
   - Stability Gate: If $\text{CV} > 5.0\%$, the trial series is flagged as unstable/inconclusive and must not be accepted as a baseline reference.
4. **Memory Measurement Normalization:**
   - Process RSS collected via `getrusage(RUSAGE_SELF)`.
   - Normalization: Linux reports `ru_maxrss` in kilobytes; Darwin (macOS) reports in bytes. Phase B harness must normalize all metrics to kibibytes ($\text{KiB}$).
   - Clear documentation that `ru_maxrss` represents process lifetime peak page allocation, not instantaneous live heap.
5. **Fail-Closed Verification:**
   - Every measurement trial must execute its associated correctness oracle before or after the timing loop.
   - Any assertion failure or hash mismatch immediately aborts the run, invalidating all collected timings.

---

## 6. Recommended Minimal Phase B Workload Set

To ensure rapid, defensible, and reliable baselining without expanding into unmaintainable complexity, the recommended minimal Phase B workload package consists of **four primary workloads** (one from each domain):

1. **Domain A:** `WL-ARITH-CODEC-I64` (Balanced ternary Horner integer conversion latency & throughput)
2. **Domain B:** `WL-VM-TISC-DETERMINISM-TIERS` (TISC interpreter execution latency with 64-bit state signature oracle)
3. **Domain C:** `WL-CANONFS-MEM-THROUGHPUT` (In-memory content-addressed block write/read throughput)
4. **Domain D:** `WL-RES-RUSAGE-METRICS` + `WL-RES-ARTIFACT-SIZE` (Integrated process resource consumption and artifact sizing)

This minimal set provides 100% domain coverage, possesses byte-exact or mathematical correctness oracles, avoids filesystem jitter, and maintains zero overlap with any frozen or closed research protocol.
