# CROSSROADS: The Strategic Future of the T81 Foundation

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CROSSROADS: The Strategic Future of the T81 Foundation](#crossroads-the-strategic-future-of-the-t81-foundation)
  - [1. The Determinism vs. Performance Frontier](#1-the-determinism-vs-performance-frontier)
  - [2. The Compilation Horizon: Interpreter vs. JIT](#2-the-compilation-horizon-interpreter-vs-jit)
  - [3. Hardware Sovereignty: Emulation vs. Silicon](#3-hardware-sovereignty-emulation-vs-silicon)
  - [4. Ecosystem Positioning: General Purpose vs. High-Stakes](#4-ecosystem-positioning-general-purpose-vs-high-stakes)
  - [Summary of Strategic Commitments](#summary-of-strategic-commitments)

<!-- T81-TOC:END -->


> **Status:** Conceptual / Strategic
> **Last Updated:** February 10, 2026
> **Purpose:** This document outlines the major strategic forks in the road ("crossroads") facing the T81 Foundation as it moves beyond v1.0. It analyzes the tensions between competing goals and proposes the path forward.

---

## 1. The Determinism vs. Performance Frontier

**The Tension:**
T81's core value proposition is **strict, bit-exact determinism** across all platforms (x86, ARM, RISC-V). However, modern hardware achieves performance through localized non-determinism (out-of-order execution, speculative fetching, varied floating-point unit implementations).
- *Path A (Pure Purity):* Remain 100% software-defined (like `dmath`), sacrificing 10x-100x performance for absolute guarantees.
- *Path B (Hardware Speed):* Allow hardware floating-point and vector instructions, accepting that traces may diverge slightly across architectures.

**The Chosen Path: Tiered Strictness**
We reject the false dichotomy. T81 will implement a **Tiered Execution Model**:
1.  **Strict Mode (Default for Audit):** Uses software-defined numerics (`dmath`, `T81BigInt`). Guaranteed bit-exact everywhere. Slow but safe.
2.  **Accelerator Mode (Opt-in):** Allows specific, strictly-defined hardware accelerations (e.g., AVX-512 integer ops) that have been formally verified to match the software model.
3.  **Loose Mode (Development only):** Allows raw hardware floating point for rapid prototyping/training, explicitly flagged in the Axion trace as "Non-Canonical".

**Future Milestone:**
- **Formal Verification of SIMD Kernels:** Proving that our AVX2/NEON integer implementations are mathematically identical to the scalar reference.

---

## 2. The Compilation Horizon: Interpreter vs. JIT

**The Tension:**
The current HanoiVM interpreter is simple, auditable, and easy to port. However, it hits a performance ceiling. A JIT (Just-In-Time) compiler offers speed but introduces massive complexity and new vectors for non-determinism (memory layout, cache side-channels, varying code generation).
- *Path A (Interpreter Only):* Accept the speed limit. Keep the TCB (Trusted Computing Base) small.
- *Path B (Aggressive JIT):* Use LLVM/AsmJit to compile hot paths, risking "compiler divergence" bugs.

**The Chosen Path: Deterministic Trace JIT**
We will pursue a **Trace-based JIT** (similar to LuaJIT 2.0 or Firefox's IonMonkey) rather than a Method JIT, but with a unique constraint: **The generated machine code must be functionally identical across runs.**
- **The "Verifiable JIT" Challenge:** We aim to explore a JIT where the *generation* of machine code is itself a deterministic process recorded in the Axion trace.
- **Guardrails:** The JIT will fallback to the interpreter immediately upon detecting any deviation or undefined behavior.

---

## 3. Hardware Sovereignty: Emulation vs. Silicon

**The Tension:**
T81 is a ternary (base-3) architecture running on binary (base-2) silicon. This emulation incurs a constant overhead.
- *Path A (Software Forever):* Optimize for binary hardware. Treat "ternary" as just a logical abstraction.
- *Path B (Hardware Manifestation):* Actively push for FPGA or ASIC implementations of T81 native opcodes.

**The Chosen Path: The Hardware Abstraction Layer (HAL)**
We will formalize the boundary between the VM and the metal.
- **Short Term:** Continued optimization of the binary-to-ternary emulation layer (SIMD bit-slicing).
- **Medium Term:** First-party support for FPGA offloading. We will release a reference Verilog/VHDL core for the TISC instruction set.
- **Long Term:** If ternary memory devices (e.g., RRAM, phase-change memory) mature, T81 will be the "Day 0" runtime ready to run on them natively.

---

## 4. Ecosystem Positioning: General Purpose vs. High-Stakes

**The Tension:**
Should T81 try to compete with Python or Rust as a general-purpose language?
- *Path A (General Adoption):* Add web servers, UI libraries, and extensive system bindings to attract a mass audience.
- *Path B (Niche Dominance):* Focus exclusively on "High-Stakes Compute" (Governance, AI Safety, Financial Audit).

**The Chosen Path: The "Secure Core" Strategy**
T81 is **NOT** a general-purpose replacement. It is designed to be the embedded **"Secure Core"** of a larger system.
- We will prioritize **FFI (Foreign Function Interface)** and **Embedding** scenarios.
- A typical app will write UI/Networking in Python/JS and the "Business Logic / Policy Engine" in T81.
- Success Metric: T81 is invisible but omnipresent in critical infrastructure, not the language you use to write a to-do list app.

---

## Summary of Strategic Commitments

| Domain | Strategy | Key Deliverable |
| :--- | :--- | :--- |
| **Numerics** | Software-Defined Strictness | `dmath` completion & AVX formal verification |
| **Execution** | Deterministic Trace JIT | Experimental JIT branch (2026 Q3) |
| **Hardware** | Emulation + FPGA Reference | Open Source TISC Verilog Core |
| **Adoption** | Embedded "Secure Core" | High-performance Python/Rust bindings |

This document serves as a compass. When making architectural decisions, we point here: **Auditability is the Constraint; Performance is the Variable.**
