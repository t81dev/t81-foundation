# T81 Foundation: Critical Evidence Gaps & Actionable Priorities

**Document Identifier:** T81-BASELINE-001-GAPS
**Repository Baseline Commit:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Tree SHA:** `68fa5529b9d8244ba31c0f543006dee9283b248f`
**Date:** November 2026
**Status:** Authoritative Engineering & Research Guidance

---

## 1. Ten Most Consequential Evidence Gaps

Based on the exhaustive audit of repository source, tests, benchmarks, and research records, the following ten evidence gaps represent the most critical blockers to scientific validity, architectural integrity, and production readiness:

### 1. Physical Solid-State Storage & Device Characterization Gap (H01)
- **The Gap:** Investigation 10A demonstrated that all 10 historical solid-state memory citations (P01–P10) cited in earlier research were unverified in the primary literature. There is zero empirical physical device characterization data (noise margins, write endurance, sense-amp energy overhead) in the repository.
- **Consequence:** Claims of ternary storage density superiority over binary silicon remain ungrounded theoretical conjectures.

### 2. Binary Hardware Accelerator Competitiveness Deficit (H06)
- **The Gap:** On modern binary processors, INT8 SIMD instructions (e.g. ARM NEON `sdot`, Intel VNNI) achieve 72.2 Gops/s, outperforming T81 ternary NEON kernels (4.96 Gops/s) by 14.5x. Early claims of 74.6x speedups were artifacts of comparing against unvectorized FP32 scalar baselines.
- **Consequence:** Ternary neural network execution on conventional binary CPUs cannot compete with standard low-bit binary quantization without dedicated ternary hardware.

### 3. Asymmetric State-Domain Comparisons in Representation Benchmarks (H04)
- **The Gap:** In H04-R02, the apparent adaptive balanced ternary (AT) advantage on `boundary_oscillation` relied on comparing a 6-trit domain (729 states) against a 10-bit domain (1024 states) with metadata scaling defects (Classification D).
- **Consequence:** When capacity domains are strictly equalized, adaptive binary matches or outperforms adaptive ternary across all tested symbol cost ratios.

### 4. Recursive Cognition Coordination & Management Tax (>96% Overhead) (H08 / H09)
- **The Gap:** Experiment H09-E1 proved that recursive multi-tier cognitive agent loops (Arm C) incur an overhead penalty exceeding 96% of total compute, rendering them 7x to 66x more expensive than a simple centralized planner.
- **Consequence:** Cognitive tiers T4–T6 (T2187–T19683) remain speculative abstractions with severe negative scaling properties.

### 5. In-Memory CAS & Pipeline I/O Overhead Tax (CanonFS)
- **The Gap:** In microbenchmark `BM_CanonFS_ReadThroughput_InMemory`, CanonFS in-memory read requires 12.01 ms compared to 28.52 µs for an OS memory-mapped baseline—a 420x slowdown (0.00238x throughput ratio).
- **Consequence:** Content-addressed integrity hashing and policy enforcement impose a steep throughput penalty that must be optimized before high-throughput I/O adoption.

### 6. Public API Surface Lock & Digest Divergence
- **The Gap:** `docs/governance/PUBLIC_API_LOCK.json` locks v1.9.0 for `include/t81/**/*.hpp`, but its stored SHA-256 surface digest (`fa40679...`) diverged from the actual codebase digest (`0bddb4b...`) calculated across 252 public headers.
- **Consequence:** Automated CI semver checks risk false positives or governance gridlock unless reconciled via maintainer authority.

### 7. Missing or Stale Implementation Paths in Historical RFC Records
- **The Gap:** Historical RFC matrices referenced outdated source file paths (e.g., `lang/lexer.cpp` instead of `lang/frontend/lexer.cpp`, `src/policy/axion_policy.cpp` instead of `kernel/axion/policy_engine.cpp`, or unmerged `src/cognition/tier4.cpp`).
- **Consequence:** Downstream auditors and autonomous agents waste significant effort searching for non-existent source paths or assuming unimplemented features are active.

### 8. RFC-00D0 Base-81 TCP/IP Stack Implementation Gap
- **The Gap:** RFC-00D0 describes an ambitious Base-81-aware TCP/IP network stack with deterministic replay. However, zero production implementation or test code exists beyond preliminary protocol drafts.
- **Consequence:** Premature attempts to implement full networking breadth risk introducing non-deterministic jitter into the core runtime.

### 9. Freestanding Userland Hardware Portability Boundaries (TernaryOS)
- **The Gap:** TernaryOS freestanding EL0 bring-up (RFC-00BC–00CE) is currently verified only under virtualized QEMU and VirtualBox harnesses, lacking verification on bare-metal ARM64 or x86_64 silicon.
- **Consequence:** Pager, scheduler, and interrupt isolation contracts remain hypervisor-dependent rather than proven bare-metal OS mechanisms.

### 10. Checked Floating-Point Conversion Boundary Verification (RFC-00D5)
- **The Gap:** Conversion between balanced ternary `T81Float` and IEEE 754 binary64 exhibits edge-case precision and rounding discrepancies under extreme transcendental ranges. RFC-00D5 remains in draft status.
- **Consequence:** Numerical reproducibility across heterogeneous CPU architectures remains vulnerable to host libm drift unless checked conversion boundaries are fully standardized.

---

## 2. Actionable, Evidence-Backed Priorities

To maximize engineering impact while respecting the research continuity charter, development must be directed exclusively toward finishable, verifiable lanes:

### Priority 1: Solidify and Harden RFC-00D1 CanonFS Interchange
- **Rationale:** RFC-00D1 is the most mature draft-to-code bridge in the repository, with operational CLI subcommands (`canonfs import`, `canonfs export`), comprehensive JSON schemas, and active CI validation.
- **Action Items:**
  - Tighten error reporting and add explicit negative tests for malformed foreign filesystem manifests.
  - Optimize the CAS read path to reduce the 420x throughput overhead gap against memory-mapped baselines.
  - Finalize golden import/export fixtures under `examples/`.

### Priority 2: Execute EXP-H04-R03 (Phase B Deterministic Correctness)
- **Rationale:** Resolves the H04-R02 Classification D controversy by enforcing strictly matched 729-state domains ($[-364, +364]$) across all 4 arms.
- **Action Items:**
  - Enforce pre-allocation bounds and block-framing assertions.
  - Validate O(1)-space bounded-block random access via deterministic touch-count assertions without reconstructing offset tables.
  - Strictly prohibit speculative performance or hardware superiority claims.

### Priority 3: Maintain Operational Stability of Deterministic Core Profile (DCP)
- **Rationale:** Deterministic execution and Axion policy enforcement are the crown jewels of T81 Foundation—empirically verified, bit-exact, and tested across architectures.
- **Action Items:**
  - Keep CI passing and operationally boring across Linux, macOS, and Windows.
  - Freeze ISA v1.9.0 opcode definitions and reject opportunistic opcode expansions.
  - Finalize RFC-00D5 checked floating-point conversion boundaries to prevent numerical drift.

### Non-Priorities (Explicitly Deprioritized Lanes)
- **Do NOT expand Cognitive Tiers (T4–T6):** H09-E1 confirmed a >96% coordination tax; halt speculative multi-agent recursion.
- **Do NOT build full TCP/IP stack (RFC-00D0):** Maintain network wire compatibility as design-led; do not write broad freestanding networking stacks.
- **Do NOT claim ternary AI inference superiority on binary silicon:** INT8 SIMD is empirically 14.5x faster; restrict ternary neural claims to dedicated hardware research.
