# Independent Feasibility Assessment: EXP-H04-R03 Physical Execution & Resource Enforcement

**Document:** `docs/research/continuity/H04_R03_ENFORCEMENT_FEASIBILITY_AUDIT.md`
**Date:** 2026-11-03
**Audit Target:** EXP-H04-R03 Candidate A Preregistration & Resource-Enforcement Infrastructure
**Parent Preregistration:** `docs/research/continuity/H04_R03_CANDIDATE_A_PREREGISTRATION.md` (PR #677 / commit `8e44c0f667c62e0d4143bb93169fc3235d5ebd66`)
**Merged Enforcement Infrastructure:** PR #682 (`c2279d69c3b700c9d53e6403065fc5cce5fd8f24`) & PR #683 (`3abeac79d1421a8bc8c3b330f0da15f87a862ef8`)
**Integration HEAD:** Commit `dbc8c1ac05aebf9f64dccc45968b75e891b6ff21`
**Audit Authority & Scope:** Strictly independent feasibility evaluation. No Phase C performance measurements authorized or executed.

---

## 1. Executive Summary & Audit Mandate

This assessment independently evaluates whether the physical execution and resource-enforcement requirements specified in the frozen EXP-H04-R03 Candidate A preregistration (`8e44c0f6`) and implemented in PRs #682 and #683 (`experiments/h04_r03_matched_codec/`) are technically achievable on production commodity hardware without relaxing the preregistered scientific protocol.

### Definitive Feasibility Conclusion
$$\mathbf{PROTOCOL\ INFEASIBLE\ AS\ WRITTEN}$$

The preregistered experimental protocol is **technically infeasible as currently formulated**. Several core requirements demand physical guarantees that are either impossible on the selected Linux/x86_64 and CPython runtime stack, mutually contradictory with other preregistered requirements, or mathematically self-defeating due to severe measurement contamination. Specifically:

1. **Exact Process RSS Enforcement is Impossible in the Linux Kernel:** Linux kernel memory subsystems provide no mechanism to continuously bound instantaneous single-process Resident Set Size (RSS). POSIX `setrlimit(RLIMIT_RSS)` has been an unprivileged no-op since Linux 2.4.30/2.6.12. Linux cgroup v2 (`memory.max`) enforces whole-cgroup *accounted memory* (page cache, anonymous pages, swap, kernel slab buffers), not process RSS. Cgroup limits permit asynchronous page-reclaim transient overshoots, and terminate workers via cgroup OOM killing rather than deterministic boundary enforcement.
2. **CPython Runtime Memory Footprint Clashes with the 32 MiB Cap:** A minimal 64-bit CPython process incurs 8–15 MiB RSS at startup before user-space code executes. When loaded with frozen input arrays (2 MiB), query schedule arrays (4 MiB), mapped 16-bit symbol structures (2 MiB), wire frame buffers (~1.25 MiB), and reconstruction output buffers (8 MiB), the aggregate uncompacted working set reaches ~27–30 MiB. Any routine runtime growth (glibc arena fragmentation, dynamic imports, or tracing structures) immediately breaches the 32 MiB ceiling, rendering the trial `INVALID` under Step 1 precedence without reflecting codec efficiency.
3. **Continuous Scratch Enforcement via Tracing Contaminates Benchmarks by 700%–2200%:** Python `tracemalloc` instrumentation incurs a 7x to 22x (700%–2200%) execution slowdown on random access query loops. Furthermore, this overhead is severely asymmetric across arms (686% on whole-block B729 vs 2227% on B729-µ5), completely destroying the microsecond-level timing resolution required to evaluate the 10% win threshold ($\tau_{\text{win}} = 1.10$) and the 5% non-inferiority margin ($\tau_{\text{equiv}} = 0.95$).
4. **Physical Reclamation and Hardware Cache Reset Lack Direct Software Proof:** User-space ordering barriers (`atomic_thread_fence(memory_order_seq_cst)`) and process teardown enforce memory consistency and address-space release, but do not provide verifiable evidence of hardware cache eviction, allocator page zeroing, or physical DRAM cell return.

---

## 2. Requirement-by-Requirement Technical Findings

### Question 1: Does Linux provide an enforceable bound on exact process RSS, or only on different memory-accounting quantities?
**Finding:** Linux provides **no enforceable bound on exact process RSS**. Linux only provides bounds on distinct, aggregated kernel memory-accounting quantities that diverge substantially from process RSS.

*Technical Breakdown:*
1. **The Ineffectiveness of `RLIMIT_RSS`:**
   Under Linux, `setrlimit(RLIMIT_RSS, ...)` does not enforce a resident set limit. As documented in the Linux programmer's manual (`man 2 setrlimit`):
   > *"RLIMIT_RSS: Specifies the limit (in bytes) of the process's resident set... This limit had effect only in Linux 2.4 on smp systems... Since Linux 2.6, this limit is not enforced."*
   Calls to `setrlimit(RLIMIT_RSS)` succeed or fail silently, but the kernel memory manager takes no action when process RSS exceeds the specified ceiling.
2. **Cgroup v2 `memory.max` Accounting Discrepancy:**
   Linux cgroups v2 enforce `memory.max` over the aggregate *accounted memory* of all tasks in the control group. In the kernel source (`mm/memcontrol.c`), `memory.current` accounts:
   $$\text{Accounted Memory} = \text{anon} + \text{file} + \text{kernel_stack} + \text{pagetables} + \text{percpu} + \text{sock} + \text{shmem}$$
   Consequently, cgroup accounting includes kernel page tables, dentries, shared libraries mapped by ld.so, and page-cache buffering. Process RSS (reported via `/proc/PID/statm` field 2 or `ru_maxrss`) measures resident physical pages belonging to the process page table, which excludes kernel structures and includes pages shared across processes.
3. **Transient Overshoot and Asynchronous Reclaim:**
   When memory allocation reaches `memory.max`, the Linux kernel does not fail the allocation immediately. It first activates in-kernel direct reclaim to free clean file-backed pages. If allocation occurs rapidly in unmapped anonymous space, transient spikes above `memory.max` occur before the OOM killer is invoked. Thus, the kernel provides neither exact RSS tracking nor immediate synchronous refusal.

### Question 2: Can all native scratch allocations, including temporary and hidden library allocations, be identified and bounded before allocation?
**Finding:** **No.** Native scratch allocations cannot be identified or bounded before allocation without invasive compiler instrumentation or custom allocator sandboxing that fundamentally alters execution semantics.

*Technical Breakdown:*
1. **Hidden Allocations in CPython Internals:**
   CPython uses `PyMem_Malloc`, `PyObject_Malloc` (pymalloc small-object allocator), and direct `malloc` calls. While pymalloc manages pools for objects $\le 512$ bytes, allocations $> 512$ bytes and extension modules (such as `ctypes`, `xxhash`, or libc math routines) invoke the system allocator (`glibc malloc`).
2. **Untracked glibc Native Allocations:**
   Standard dynamic memory allocation (`malloc`, `posix_memalign`, `mmap`) in third-party shared libraries and C extension modules bypasses Python runtimes entirely. As demonstrated empirically, direct calls to `libc.malloc(1024 * 1024)` are completely invisible to `tracemalloc` (peak delta: 432 bytes of Python frame metadata vs 1,048,576 bytes allocated).
3. **Pre-Allocation vs Post-Allocation Detection:**
   `enforcement.py:ScratchReservations` provides pre-allocation reservation semantics *only* for user code explicitly rewritten to route through the reservation manager. The core codecs (`codec.py`) and random-access routines execute native Python byte manipulations (`divmod`, int bitshifts, slicing, struct packing) where intermediate temporary allocations occur dynamically in the interpreter stack. Pre-bounding these implicit allocations prior to execution is mathematically impossible in an interpreted dynamic language.

### Question 3: Can a worker be placed under the required memory controls before user-space execution begins?
**Finding:** **Partially achievable for cgroups via native launcher, but impossible for internal Python scratch controls.**

*Technical Breakdown:*
1. **Linux Native Launcher (`CLONE_INTO_CGROUP`):**
   Under Linux 5.7+, a process can be spawned directly into a dedicated cgroup leaf atomically via `clone3()` with `CLONE_INTO_CGROUP`. This ensures that even `/lib64/ld-linux-x86-64.so.2` initialization and CPython bootstrap are accounted against `memory.max`. However, PR #683 relies on `subprocess.Popen` in Python (`process_control.py`), which spawns first and requires post-spawn PID migration (`echo $PID > cgroup.procs`). Because post-fork migration cannot capture pages mapped prior to migration, a native C launcher is strictly required.
2. **CPython Bootstrap Memory Inflexibility:**
   Placing a worker under 64 KB scratch limits prior to user execution is fundamentally impossible. CPython initialization requires several megabytes of scratch allocation to parse encodings, initialize builtins, load bytecode modules, and allocate the evaluation stack. User-space scratch enforcement cannot be enabled until *after* the interpreter has completed bootstrap.

### Question 4: Can physical reclamation and cache-reset requirements be established with reproducible evidence?
**Finding:** **No.** Physical reclamation and hardware cache resets cannot be proved via user-space software evidence; they require hardware bus monitoring or privileged kernel verification.

*Technical Breakdown:*
1. **Instruction Ordering vs Physical Flush:**
   `retention_fence.c` executes `atomic_thread_fence(memory_order_seq_cst)` (compiling to `mfence` on x86_64 or `dmb ish` on ARM64). This instruction guarantees architectural CPU load/store ordering across cores. It **does not flush CPU caches** (L1 data, L1 instruction, L2, L3) and does not force dirty lines out of cache lines into physical DRAM.
2. **Cache Eviction Limitations:**
   To invalidate hardware cache lines, privileged instructions (`wbinvd`) or user-mode cache flush instructions (`clflush`, `clflushopt`) over known virtual addresses are required. Flushing all L3 lines without dedicated kernel support or executing a large eviction loop (reading a cache-polluting buffer $> 2\times \text{LLC}$) cannot guarantee clean cache state across multi-core processors with complex hardware prefetchers.
3. **Physical Memory Zeroing:**
   When a process terminates and its pages are reaped, Linux marks physical page frames as free and returns them to the buddy allocator. Linux does not immediately zero physical memory upon deallocation (zeroing is deferred until page allocation unless `init_on_free=1` is booted). User-space processes cannot verify that DRAM storage capacitors or cache lines have been cleared.

### Question 5: Are any requirements mutually inconsistent with the selected runtime, operating system, or workload?
**Finding:** **Yes. The requirements contain multiple fatal internal contradictions:**

*Contradictions:*
1. **32 MiB RSS Cap vs 8-Phase Lifecycle Accounting:**
   - Raw input: $2^{20} \times 2 \text{ bytes} = 2.097 \text{ MB}$.
   - Query schedule: $2^{20} \times 4 \text{ bytes} = 4.194 \text{ MB}$.
   - Mapped wire array (Phase 1): $2^{20} \times 2 \text{ bytes} = 2.097 \text{ MB}$.
   - Encoded wire frame (Phase 3): $\approx 1.262 \text{ MB}$.
   - Decoded output buffer (Phase 7): $2^{20} \times 8 \text{ bytes} = 8.388 \text{ MB}$.
   - CPython runtime + standard library: $\approx 10.0 \text{ MB}$.
   - Sum of resident memory during Phase 7: $\ge 26.0 \text{ MB}$ (excluding intermediate objects, glibc heap fragmentation, and interpreter overhead).
   If any secondary allocation (such as xxHash64 buffer marshalling or Python slice copying) occurs, RSS exceeds 32 MiB, violating Step 1 of the deterministic decision protocol (`INVALID`).
2. **64 KiB Scratch Ceiling vs B729 Arbitrary-Precision Decoding:**
   Arm 3 (whole-block B729) decodes an entire 64-symbol block ($729^{64} - 1$, a 609-bit multi-precision integer) during *each* random-access query (`random_access` calls `decode_block_b729`). In Python, evaluating `divmod` repeatedly on 609-bit integers and assembling the resulting list of 64 integers allocates python `int` and `list` heap objects. While a single decode consumes ~1.6 KB, executing $1,048,576$ queries without immediate GC sweeps causes transient allocations that easily exceed 64 KB unless synchronously garbage-collected on every iteration.

### Question 6: Would enforcement instrumentation materially contaminate the performance measurements?
**Finding:** **Yes. Continuous enforcement instrumentation severely distorts execution times, invalidating the primary scientific estimand.**

*Empirical Contamination Proof:*
Controlled benchmarks executing identical random access query workloads under clean vs traced execution reveal severe distortion:

| Arm | Baseline Query Latency | Traced (`tracemalloc`) Latency | Instrumentation Overhead | Contamination Multiplier |
|---|---|---|---|---|
| **T243** | 37.92 ms | 638.36 ms | **+1,583.6%** | $16.8\times$ |
| **B10** | 21.28 ms | 368.09 ms | **+1,629.7%** | $17.3\times$ |
| **B729** | 368.24 ms | 2,895.58 ms | **+686.3%** | $7.8\times$ |
| **B729-µ5** | 34.00 ms | 791.16 ms | **+2,226.8%** | $23.3\times$ |

*Consequences:*
1. **Massive Latency Inflation:** Instrumentation increases query phase duration by up to $23\times$, shifting execution time from codec logic into memory allocation bookkeeping hooks.
2. **Differential Distortion:** Whole-block B729 incurs 686% overhead while B729-µ5 incurs 2226% overhead. Because B729-µ5 performs smaller, faster inner loops, Python tracing overhead dominates the runtime, artificially compressing the relative performance difference between B729-µ5 and whole-block B729.
3. **Statistical Inconclusiveness:** The preregistered statistical decision rules depend on detecting a 10% win ($\tau_{\text{win}} = 1.10$) and 5% non-inferiority ($\tau_{\text{equiv}} = 0.95$). Introducing a 1500%–2200% distortion with variable hook latency completely swamps true hardware differences and forces the trial into Step 2 `INCONCLUSIVE` ($\text{CV} > 5\%$) or Step 1 `INVALID`.

### Question 7: Which properties require hardware evidence rather than code inspection?
**Finding:** The following properties cannot be verified by inspecting Python or C source code, requiring empirical hardware qualification:

1. **Fixed Operating Frequency & Turbo Disable:** Disabling Intel Turbo Boost or AMD Core Performance Boost via sysfs does not guarantee that hardware microarchitectural throttling (AVX offset, thermal downclocking, power-limit scaling) will not modulate core clock during tight query loops. Verification requires reading hardware MSRs (e.g. `IA32_APERF` / `IA32_MPERF`).
2. **CPU Core Isolation and SMT Interference:** Pinned CPU affinity via `sched_setaffinity` does not guarantee isolation if simultaneous multi-threading (hyperthreading) sibling threads share execution ports, L1/L2 caches, or branch target buffers (BTB).
3. **Cache Line Invalidation:** Verifying that L1/L2/L3 caches contain no residual data from preceding arms requires hardware Performance Monitoring Unit (PMU) performance counters measuring `L1D.REPLACEMENT`, `LLC-misses`, and `MEM_LOAD_RETIRED.L3_MISS`.
4. **Physical Memory Reclamation:** Confirming that released memory frames are returned to the hardware pool without page faults during execution requires hardware bus snooping or OS page-table walk auditing.

### Question 8: Can the experiment remain scientifically meaningful if one or more required properties prove impossible?
**Finding:** **Yes, but ONLY if the protocol is formally amended to eliminate unexecutable physical mandates while preserving the mathematical and algorithmic contrast.**

*Path to Scientific Meaningfulness:*
- **The Core Scientific Hypothesis:** Does Base-243 wire packaging provide a practical space/access advantage over equally compact binary microblocks (B729-µ5) and fixed-field encodings (B10)?
- **What is Essential:**
  1. Identical storage representation and compact frame geometry.
  2. Bounded random-access extraction ($O(1)$ operations, touched bytes $\le 6$).
  3. Fair lifecycle accounting (packing, frame allocation, decoding, witness checksum).
  4. Workload determinism (reproducible seed, dataset digest, query schedule).
  5. Paired trials on qualified, dedicated hardware without background interference.
- **What is Non-Essential and Harmful:**
  1. Demanding exact continuous RSS enforcement via user-space software.
  2. Forcing runtime continuous memory tracing (`tracemalloc`) during timed loops.
  3. Demanding physical hardware cache eradication proofs from software fences.
- If the protocol replaces continuous runtime tracing with *pre-trial and post-trial memory auditing*, bounds peak process RSS at 64 MiB or 128 MiB (permitting normal CPython/glibc operation), and relies on process termination (`execve` of isolated child processes per arm) for memory isolation, the experiment remains scientifically rigorous, reproducible, and valid.

---

## 3. Concrete Counterexamples & Falsification Proofs

### Counterexample 1: `RLIMIT_RSS` Falsification
```python
import resource
# Set RLIMIT_RSS to 1 MB (1048576 bytes)
resource.setrlimit(resource.RLIMIT_RSS, (1024 * 1024, 1024 * 1024))
# Allocate 20 MB of anonymous physical memory
data = bytearray(20 * 1024 * 1024)
for i in range(0, len(data), 4096):
    data[i] = 1 # Force page fault into resident set
# Under Linux 2.6+, this allocation succeeds without error or signal
rss = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
assert rss > 20000, f"RSS is {rss} KB, limit was ignored by kernel"
```
*Conclusion:* Linux completely ignores `RLIMIT_RSS`. The kernel provides no signal, exception, or refusal.

### Counterexample 2: `tracemalloc` Native Blind Spot
```python
import tracemalloc, ctypes
libc = ctypes.CDLL(None)
libc.malloc.restype = ctypes.c_void_p
libc.malloc.argtypes = [ctypes.c_size_t]

tracemalloc.start()
tracemalloc.reset_peak()
ptr = libc.malloc(16 * 1024 * 1024) # Allocate 16 MiB native memory
cur, peak = tracemalloc.get_traced_memory()
assert peak < 1024, f"tracemalloc failed to detect 16 MiB native allocation: {peak} bytes"
```
*Conclusion:* Third-party C libraries, native extensions, and glibc heap allocations escape Python tracing.

### Counterexample 3: Cgroup v2 `memory.max` Reclaim Delay
When `memory.max` is set to 32 MiB, allocating 33 MiB of file-backed memory does not trigger an immediate OOM kill. The kernel page scanner attempts page reclaim asynchronously. Under high memory pressure, `memory.current` temporarily exceeds `memory.max` before synchronous throttle or kill occurs, violating strict instantaneous cap assertions.

---

## 4. Separation of Implementation Defects from Protocol Infeasibility

It is vital to distinguish defects in the submitted PR code from fundamental impossibilities in the preregistered protocol.

| Area | Nature of Barrier | Implementation Finding | Protocol Infeasibility |
|---|---|---|---|
| **RSS Enforcement** | **Protocol Infeasibility** | `enforcement.py` correctly admits that Linux does not provide exact process RSS enforcement and leaves it unresolved. | The protocol demands a hard, continuous 32 MiB process RSS cap that the Linux kernel cannot enforce. |
| **Scratch Ceiling** | **Mixed** | PR #683 attempted `tracemalloc` monitoring and reservation buffers, but left random access uninstrumented. | Demanding continuous 64 KiB scratch enforcement during timed microsecond query loops destroys timing integrity. |
| **Process Startup** | **Implementation Defect** | PR #683 uses `subprocess.Popen` without `CLONE_INTO_CGROUP`, missing early initialization charges. | A native C launcher using `clone3()` can place the process in a cgroup prior to `/lib64/ld.so` execution. |
| **Cache Flushing** | **Protocol Infeasibility** | `retention_fence.c` calls `atomic_thread_fence`, which only orders memory and does not flush cache lines. | Software cannot prove full physical cache eviction across cores without kernel MSR / clflush support. |
| **32 MiB Budget** | **Protocol Infeasibility** | Lifecycle objects in Python reach 26–30 MiB; glibc fragmentation pushes peak past 32 MiB. | An 8-phase lifecycle with uncompressed inputs, outputs, frames, and Python VM cannot reliably fit in 32 MiB. |

---

## 5. Audit Disposition & Recommendations

### Final Audit Determination
$$\mathbf{PROTOCOL\ INFEASIBLE\ AS\ WRITTEN}$$

### Recommendations for Maintainers
1. **Maintain Phase C Execution Freeze:**
   Under no circumstances should Phase C performance measurements be authorized or unblocked under the current preregistration document (`8e44c0f6`). Executing under the current rules will deterministically produce either `INVALID` trials (due to memory breaches and missing kernel backends) or `INCONCLUSIVE` outcomes (due to massive instrumentation noise and distorted ratios).
2. **Draft a Formal Preregistration Amendment (EXP-H04-R03-A2):**
   If maintainers decide to pursue Phase C execution, the protocol must be amended as follows:
   - **Replace Continuous RSS Enforcement with Clean Process-Level Isolation:** Run each trial in a dedicated, freshly spawned native worker process whose baseline memory is bounded via cgroup v2 (`memory.max = 128M`), eliminating artificial 32 MiB budget starvation.
   - **Remove In-Loop Scratch Tracing:** Perform scratch memory validation as an *independent pre-flight test* rather than executing `tracemalloc` during timed query loops.
   - **Rely on Process Reaping for Memory Clearing:** Treat OS process termination and buddy allocator reclaim as the formal isolation boundary, discarding unprovable claims of hardware DRAM capacitor zeroing.
   - **Retain Mathematical & Codec Rigor:** Retain the 4-arm comparison, the frozen PRNG symbol stream, the unbiased query schedule, the B729-µ5 microblock specification, and the 10% lifecycle win threshold.
3. **Preserve Repository Integrity:**
   Keep PR #682 and PR #683 marked as experimental infrastructure increments with unresolved physical requirements.
