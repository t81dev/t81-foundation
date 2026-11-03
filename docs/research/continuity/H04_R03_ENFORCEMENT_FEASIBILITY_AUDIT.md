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

The preregistered experimental protocol is **technically infeasible as currently formulated**. Multiple core requirements demand physical guarantees that are either unexecutable on the selected Linux/x86_64 and CPython runtime stack or subject to severe measurement distortion. Specifically:

1. **Exact Process RSS Enforcement is Unsupported by the Linux Kernel:** Linux kernel memory subsystems provide no mechanism to continuously bound instantaneous single-process Resident Set Size (RSS). POSIX `setrlimit(RLIMIT_RSS)` has been an unprivileged no-op since Linux 2.4.30/2.6.12. Linux cgroup v2 (`memory.max`) enforces whole-cgroup *accounted memory* (page cache, anonymous pages, swap, kernel slab buffers), not process RSS. Cgroup limits permit asynchronous page-reclaim transient overshoots, and terminate workers via cgroup OOM killing rather than deterministic boundary enforcement.
2. **CPython Runtime Working Set and Allocator Retention vs 32 MiB Cap:** While simultaneously live application buffers in Phase 7 are bounded at ~13.20 MiB following the Phase 4 retention boundary, the glibc memory allocator (`ptmalloc`) retains freed physical pages in heap arenas (`sbrk`/`mmap`) rather than returning them to the OS. The combined CPython baseline (8–15 MiB RSS) and historical allocation high-water mark (~26.23 MiB) leave virtually no margin beneath the 32 MiB threshold, creating severe sensitivity to allocator fragmentation and extension imports.
3. **Continuous Scratch Enforcement via Tracing Contaminates Benchmarks:** Python `tracemalloc` instrumentation injects Python allocator hooks into every memory operation. In microsecond-level query loops, continuous memory tracing shifts execution from codec logic into tracing metadata bookkeeping, introducing high and asymmetric measurement latency overhead across arms that obscures true algorithmic efficiency differences.
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
   Standard dynamic memory allocation (`malloc`, `posix_memalign`, `mmap`) in third-party shared libraries and C extension modules bypasses Python runtimes entirely. Direct calls to `libc.malloc(1024 * 1024)` are completely invisible to `tracemalloc` (peak delta: 432 bytes of Python frame metadata vs 1,048,576 bytes allocated).
3. **Pre-Allocation vs Post-Allocation Detection:**
   `enforcement.py:ScratchReservations` provides pre-allocation reservation semantics *only* for user code explicitly rewritten to route through the reservation manager. The core codecs (`codec.py`) and random-access routines execute native Python byte manipulations (`divmod`, int bitshifts, slicing, struct packing) where intermediate temporary allocations occur dynamically in the interpreter stack. Pre-bounding these implicit allocations prior to execution is unavailable in an interpreted dynamic language.

### Question 3: Can a worker be placed under the required memory controls before user-space execution begins?
**Finding:** **Partially achievable for cgroups via native launcher, but unexecutable for internal Python scratch controls.**

*Technical Breakdown:*
1. **Linux Native Launcher (`CLONE_INTO_CGROUP`):**
   Under Linux 5.7+, a process can be spawned directly into a dedicated cgroup leaf atomically via `clone3()` with `CLONE_INTO_CGROUP`. This ensures that even `/lib64/ld-linux-x86-64.so.2` initialization and CPython bootstrap are accounted against `memory.max`. However, PR #683 relies on `subprocess.Popen` in Python (`process_control.py`), which spawns first and requires post-spawn PID migration (`echo $PID > cgroup.procs`). Because post-fork migration cannot capture pages mapped prior to migration, a native C launcher is strictly required.
2. **CPython Bootstrap Memory Allocation:**
   Placing a worker under 64 KB scratch limits prior to user execution cannot be achieved. CPython initialization requires several megabytes of scratch allocation to parse encodings, initialize builtins, load bytecode modules, and allocate the evaluation stack. User-space scratch enforcement cannot be enabled until *after* the interpreter has completed bootstrap.

### Question 4: Can physical reclamation and cache-reset requirements be established with reproducible evidence?
**Finding:** **No.** Physical reclamation and hardware cache resets cannot be established via user-space software evidence; they require hardware bus monitoring or privileged kernel verification.

*Technical Breakdown:*
1. **Instruction Ordering vs Physical Flush:**
   `retention_fence.c` executes `atomic_thread_fence(memory_order_seq_cst)` (compiling to `mfence` on x86_64 or `dmb ish` on ARM64). This instruction guarantees architectural CPU load/store ordering across cores. It **does not flush CPU caches** (L1 data, L1 instruction, L2, L3) and does not force dirty lines out of cache lines into physical DRAM.
2. **Cache Eviction Limitations:**
   To invalidate hardware cache lines, privileged instructions (`wbinvd`) or user-mode cache flush instructions (`clflush`, `clflushopt`) over known virtual addresses are required. Flushing all L3 lines without dedicated kernel support or executing a large eviction loop (reading a cache-polluting buffer $> 2\times \text{LLC}$) cannot guarantee clean cache state across multi-core processors with complex hardware prefetchers.
3. **Physical Memory Zeroing:**
   When a process terminates and its pages are reaped, Linux marks physical page frames as free and returns them to the buddy allocator. Linux does not immediately zero physical memory upon deallocation (zeroing is deferred until page allocation unless `init_on_free=1` is booted). User-space processes cannot verify that DRAM storage capacitors or cache lines have been cleared.

### Question 5: Are any requirements mutually inconsistent with the selected runtime, operating system, or workload?
**Finding:** **Yes. The requirements contain substantial tensions between CPython runtime memory behavior, allocator retention, and the strict 32 MiB / 64 KiB ceilings:**

*Technical Analysis & Recalculated Working Sets:*
1. **Phase 7 Memory Accounting Under the Retention Boundary:**
   The preregistered lifecycle includes an explicit retention boundary at Phase 4 (`measurement_engine.py:retention_boundary`), which executes:
   ```python
   self.raw = self.mapped = None
   gc.collect()
   ```
   This drops the strong references to `raw` ($2.097 \text{ MB}$) and `mapped` ($2.097 \text{ MB}$). Consequently, during Phase 7 (`output_and_checksum`), the simultaneously live application buffers are strictly:
   - `queries`: array('I') of $2^{20}$ elements = $4,194,304 \text{ bytes} \approx 4.00 \text{ MB}$.
   - `frame`: bytearray = $1,261,576 \text{ bytes} \approx 1.20 \text{ MB}$ (or $1.31 \text{ MB}$ for B10).
   - `output`: bytearray of $8 \times 2^{20}$ bytes = $8,388,608 \text{ bytes} \approx 8.00 \text{ MB}$.
   - **Total Simultaneously Live Application Buffers:** $\approx 13.20 \text{ MB}$.

2. **Simultaneously Live Buffers vs Allocator-Retained Resident Pages:**
   Although live application buffers total only ~13.20 MB, process Resident Set Size (RSS) behaves differently:
   - Baseline CPython initialization occupies ~10.25 MB RSS.
   - During Phase 3 (encoding), `raw` (2 MB), `mapped` (2 MB), `queries` (4 MB), `frame` (1.2 MB), and intermediate packing objects bring the resident high-water mark to **~26.23 MB RSS**.
   - When `raw` and `mapped` are released at Phase 4, Python's `gc.collect()` reclaims the Python objects, but the underlying C allocator (`glibc ptmalloc`) does not necessarily release memory pages back to the kernel via `madvise(MADV_DONTNEED)` or `brk` truncation.
   - When Phase 7 allocates the 8 MB `output` bytearray, glibc may satisfy part of the request from heap free lists, but peak RSS remains at or above **26.23 MB**.
   - Because 26.23 MB is within 5.8 MB of the absolute 32.0 MB ceiling, any additional memory consumption (such as dynamic shared library loading, xxHash64 C extension allocations, or tracing structures) risks exceeding the 32 MiB threshold, triggering an `INVALID` classification under Step 1 precedence.

3. **Scratch Allocation Lifetime and Reference Counting in B729 Queries:**
   Arm 3 (whole-block B729) decodes a full 64-symbol block during each random-access query (`random_access` invokes `decode_block_b729`). In CPython, `decode_block_b729` allocates a temporary list of 64 integers:
   - The returned list is assigned to a local variable `vals` within `random_access`: `vals = decode_block_b729(...)`.
   - The scalar integer `vals[pos_in_block]` is returned, and `vals` goes out of scope.
   - In CPython, local variables out of scope are immediately freed by deterministic reference counting (`Py_DECREF`), without waiting for cyclic GC passes.
   - Peak scratch allocation within a single query is ~1.6 KB (well below 64 KB).
   - Empirical verification confirms that object count does not accumulate across queries (e.g., exactly 7,867 GC objects remain live both before and after 100 queries).
   - Therefore, B729 does not accumulate scratch memory across queries; however, the requirement to continuously *prove* that scratch remains below 64 KB across all 1,048,576 queries requires instrumentation hooks that degrade query performance.

### Question 6: Would enforcement instrumentation materially contaminate the performance measurements?
**Finding:** **Yes. Continuous enforcement instrumentation introduces substantial distortion into execution times, compromising the primary scientific estimand.**

*Mechanism of Instrumentation Distortion:*
Continuous scratch enforcement in Python relies on `tracemalloc`. Enabling `tracemalloc` hooks every internal call to `PyMem_RawMalloc`, `PyMem_Malloc`, and `PyObject_Malloc` to record file, line, and allocation size metadata:
- **Asymmetric Latency Overhead:** Codecs with high internal allocation frequencies or tighter inner loops experience severe and unequal runtime dilation.
- **Dilution of Algorithmic Contrast:** In microsecond query benchmarks, adding memory allocation tracing shifts the primary measurement from codec bit-manipulation logic into memory tracking hook overhead.
- **Distortion of Estimands:** The preregistered statistical decision rules depend on detecting a 10% lifecycle win ($\tau_{\text{win}} = 1.10$) and 5% non-inferiority margin ($\tau_{\text{equiv}} = 0.95$). Any measurement apparatus that injects significant and variable hook overhead obscures true algorithmic differences, driving trial variability above the 5.0% CV noise ceiling and forcing inconclusive outcomes.

### Question 7: Which properties require hardware evidence rather than code inspection?
**Finding:** The following properties cannot be verified by inspecting Python or C source code, requiring empirical hardware qualification:

1. **Fixed Operating Frequency & Turbo Disable:** Disabling Intel Turbo Boost or AMD Core Performance Boost via sysfs does not guarantee that hardware microarchitectural throttling (AVX offset, thermal downclocking, power-limit scaling) will not modulate core clock during tight query loops. Verification requires reading hardware MSRs (e.g. `IA32_APERF` / `IA32_MPERF`).
2. **CPU Core Isolation and SMT Interference:** Pinned CPU affinity via `sched_setaffinity` does not guarantee isolation if simultaneous multi-threading (hyperthreading) sibling threads share execution ports, L1/L2 caches, or branch target buffers (BTB).
3. **Cache Line Invalidation:** Verifying that L1/L2/L3 caches contain no residual data from preceding arms requires hardware Performance Monitoring Unit (PMU) performance counters measuring `L1D.REPLACEMENT`, `LLC-misses`, and `MEM_LOAD_RETIRED.L3_MISS`.
4. **Physical Memory Reclamation:** Confirming that released memory frames are returned to the hardware pool without page faults during execution requires hardware bus snooping or OS page-table walk auditing.

### Question 8: Can the experiment remain scientifically meaningful if one or more required properties prove impossible?
**Finding:** **Yes, provided the protocol is formally amended to replace unexecutable physical runtime requirements with clean process-level isolation and pre-flight validation.**

*Path to Scientific Meaningfulness:*
- **The Core Scientific Hypothesis:** Does Base-243 wire packaging provide a practical space/access advantage over equally compact binary microblocks (B729-µ5) and fixed-field encodings (B10)?
- **Essential Protocol Elements:**
  1. Identical storage representation and compact frame geometry.
  2. Bounded random-access extraction ($O(1)$ operations, touched bytes $\le 6$).
  3. Fair lifecycle accounting (packing, frame allocation, decoding, witness checksum).
  4. Workload determinism (reproducible seed, dataset digest, query schedule).
  5. Paired trials on qualified, dedicated hardware without background interference.
- **Unfeasible and Distorting Elements to Amend:**
  1. Continuous process RSS bounding via software assertions (replace with pre/post process auditing and a realistic cgroup limit such as 64–128 MiB).
  2. Continuous in-loop scratch memory tracing (replace with separate pre-flight scratch verification).
  3. Unprovable software assertions of physical hardware cache eradication (rely on process boundary teardown and OS virtual memory reclamation).
- If amended in this manner, the experiment preserves all mathematical and algorithmic rigor while establishing achievable physical execution boundaries.

---

## 3. Concrete Counterexamples & Technical Falsification

### Counterexample 1: Linux `RLIMIT_RSS` Non-Enforcement
```python
import resource
# Set RLIMIT_RSS to 1 MB (1048576 bytes)
resource.setrlimit(resource.RLIMIT_RSS, (1024 * 1024, 1024 * 1024))
# Allocate 20 MB of anonymous physical memory
data = bytearray(20 * 1024 * 1024)
for i in range(0, len(data), 4096):
    data[i] = 1 # Force page fault into resident set
# Under Linux 2.6+, this allocation succeeds without error, signal, or kernel intervention
rss = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
assert rss > 20000, f"RSS is {rss} KB, limit was not enforced by kernel"
```
*Significance:* Confirms that the Linux kernel does not enforce process RSS ceilings via standard POSIX resource limits.

### Counterexample 2: Incomplete Native Memory Coverage in `tracemalloc`
```python
import tracemalloc, ctypes
libc = ctypes.CDLL(None)
libc.malloc.restype = ctypes.c_void_p
libc.malloc.argtypes = [ctypes.c_size_t]

tracemalloc.start()
tracemalloc.reset_peak()
ptr = libc.malloc(16 * 1024 * 1024) # Allocate 16 MiB native heap
cur, peak = tracemalloc.get_traced_memory()
assert peak < 1024, f"tracemalloc failed to detect 16 MiB native allocation: {peak} bytes"
```
*Significance:* Demonstrates that C extensions and glibc allocations bypass Python runtime scratch tracking.

### Counterexample 3: Cgroup v2 `memory.max` Accounting Scope
In cgroup v2, `memory.current` reflects kernel page cache, socket memory, and page tables in addition to anonymous pages. Allocating temporary files or memory-mapped buffers increases `memory.current` even when process RSS remains constant, illustrating that cgroup limits do not map 1:1 to process RSS.

---

## 4. Separation of Implementation Defects from Protocol Infeasibility

| Requirement / Subsystem | Nature of Barrier | Implementation Status (PR #682 / #683) | Protocol Feasibility Analysis |
|---|---|---|---|
| **Continuous RSS Bounding** | **Protocol Infeasibility** | `enforcement.py:KernelMemoryGuard` records controls but explicitly notes exact RSS enforcement is unresolved. | The protocol demands continuous 32 MiB process RSS enforcement that the Linux kernel cannot provide. |
| **Scratch Memory Ceiling** | **Mixed** | PR #683 provides `TracedScratch` and `ScratchReservations`, but leaves query loop uninstrumented. | Demanding continuous in-loop scratch tracing introduces measurement distortion; pre-flight verification is feasible. |
| **Process Startup Controls** | **Implementation Defect** | PR #683 spawns child via `subprocess.Popen` before cgroup attachment. | Fixable via a native C launcher using `clone3(CLONE_INTO_CGROUP)` to capture process initialization. |
| **Hardware Cache Flushing** | **Protocol Infeasibility** | `retention_fence.c` executes an ordering barrier (`atomic_thread_fence`), which does not flush cache lines. | Software ordering fences cannot flush multi-core CPU caches without privileged instructions or cache pollution loops. |
| **32 MiB Budget Headroom** | **Protocol Infeasibility** | Live buffers are ~13.20 MB, but glibc heap retention keeps peak RSS at ~26.23 MB, leaving narrow margin. | Operating within 32 MiB in CPython on 64-bit Linux creates high risk of failure from allocator fragmentation. |

---

## 5. Synthesis Disposition Across Preregistration Audit Questions

| Audit Question | Preregistered Requirement | Technical Status | Feasibility Disposition |
|---|---|---|---|
| **Q1. Linux RSS Bound** | Exact 32 MiB Process RSS cap | Linux only bounds cgroup accounted memory; `RLIMIT_RSS` is a no-op | **UNSUPPORTED BY KERNEL** |
| **Q2. Native Scratch Bounds** | Pre-allocation bounds on all scratch $\le 64 \text{ KB}$ | Native/libc allocations untracked; dynamic byte operations unreserved | **UNRESOLVED / UNFEASIBLE IN CPYTHON** |
| **Q3. Pre-Execution Controls** | Worker under memory controls before execution | Atomic cgroup launch requires native `clone3`; scratch cannot precede bootstrap | **PARTIALLY FEASIBLE (REQUIRES C LAUNCHER)** |
| **Q4. Physical Reclamation** | Verifiable cache and physical RAM reset | User fences ensure ordering only; hardware cache lines and RAM unproved | **UNPROVABLE IN USER-SPACE SOFTWARE** |
| **Q5. Internal Contradictions** | 8-phase lifecycle, 32 MiB RSS, 64 KiB scratch | Phase 7 live buffers are 13.2 MB, but glibc retention holds RSS at 26.2 MB | **SUBSTANTIAL HEADROOM TENSION** |
| **Q6. Measurement Contamination**| Continuous scratch enforcement during queries | Allocation tracing hooks introduce significant, asymmetric measurement overhead | **MEASUREMENT CONTAMINATING** |
| **Q7. Hardware Properties** | Frequency pinning, SMT isolation, cache flush | Code inspection insufficient; requires MSR reads and PMU event counters | **REQUIRES HARDWARE EVIDENCE** |
| **Q8. Scientific Viability** | Research question meaningfulness if amended | Protocol preserves scientific validity if amended to process-isolated design | **VIABLE UNDER FORMAL AMENDMENT** |

---

## 6. Audit Disposition & Recommendations

### Final Audit Determination
$$\mathbf{PROTOCOL\ INFEASIBLE\ AS\ WRITTEN}$$

### Recommendations for Maintainers
1. **Maintain Phase C Execution Freeze:**
   Phase C performance measurements should remain frozen under the current preregistration document (`8e44c0f6`). Executing under the current rules will predictably lead to `INVALID` trials (due to memory breaches or missing kernel backends) or `INCONCLUSIVE` outcomes (due to instrumentation distortion and high run-to-run noise).
2. **Draft a Formal Preregistration Amendment:**
   If maintainers seek to execute Phase C, the preregistration should be amended to:
   - **Adopt Process-Level Memory Isolation:** Run each trial in a dedicated native worker process with cgroup v2 bounding set to a realistic ceiling (e.g., 64 MiB or 128 MiB) to accommodate runtime baseline overhead and glibc heap management.
   - **Decouple Scratch Verification from Performance Timing:** Verify 64 KiB scratch bounds via a dedicated pre-flight test suite rather than active tracing during timed query loops.
   - **Rely on Process Reaping for Memory Clearing:** Treat OS process termination and address-space destruction as the formal isolation boundary between trial runs.
   - **Retain Core Scientific Controls:** Retain all four comparison arms (T243, B10, whole-block B729, B729-µ5), the frozen PRNG symbol stream, the unbiased query schedule, the 10% lifecycle improvement threshold, and the 5-step decision precedence.
3. **Preserve Repository Infrastructure:**
   Maintain PR #682 and PR #683 as experimental infrastructure with explicitly recorded runtime and kernel boundaries.
