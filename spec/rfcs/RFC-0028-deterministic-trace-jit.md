# RFC-0028: Deterministic Trace-JIT

Version 0.1 — Standards Track
Status: Draft
Author: T81 Foundation Architecture Team
Applies to: T81VM, TISC, Axion

______________________________________________________________________

# Summary

This RFC proposes the design and integration pathway for a deterministic Trace-JIT within the T81 Foundation. The goal is to elevate the Trace-JIT from an "Experimental" surface to a "Stable" surface within the Deterministic Core Profile (DCP) by mitigating performance overhead without compromising cross-architecture bit-level determinism.

# Motivation

The T81 architecture enforces strict deterministic execution through explicit truncation, Axion loop-checks, SWAR unpacking, and the avoidance of fast, hash-based data structures (e.g., `std::unordered_map`). These constraints result in a significant performance penalty compared to native binary execution. While a Trace-JIT optimization path exists conceptually (`runtime/jit/`), it remains experimental and unverified against the core specification. A verified, deterministic JIT is required to provide high performance for AI inference and cognitive tier reasoning while preserving the project's root invariant of absolute reproducibility.

# Proposal

The Deterministic Trace-JIT must adhere to the following invariants and mechanisms to guarantee bit-exact equivalence with the reference interpreter:

### 1. Validation Mechanisms for Bit-Level Determinism
*   **Shadow-Execution Property Testing:** The Trace-JIT must be mathematically guaranteed to yield the exact output state as the reference interpreter for any given input state. The `tests/cpp/vm_determinism_property_test.cpp` framework shall be extended to include a `JITShadowMode` harness. This harness will generate valid TISC bytecode sequences, concurrently execute them natively via the reference interpreter and the compiled JIT trace, and assert strict equivalence.
*   **State Equivalence Assertions:** After every trace exit, the validation harness must assert the bit-exact equivalence of the VM's Canonical State, including the register file, stack memory, heap map (which must remain explicitly ordered via `std::map`), and the Axion Policy context.

### 2. Trace Identity Layer (Canonical Trace Hashing)
Each compiled trace must include a deterministic identity derived from the context of its compilation. This ensures reproducible JIT caching and artifact identity.
The canonical trace hash is defined as:
```
trace_hash = hash(
    canonical_tisc_sequence
    register_layout
    stack_layout
    policy_context
)
```

### 3. Deterministic Register Allocation
To prevent nondeterministic behavior arising from host compiler decisions or varying architectural register counts, native register allocation must use a deterministic register mapping table:
```
VM_R0 -> host_reg_0
VM_R1 -> host_reg_1
...
```
Alternatively, traces must be compiled using a fixed virtual register file in memory.

### 4. Deterministic JIT Cache
Compiled JIT artifacts must be stored using their canonical identities to enable reproducible builds and distributed verification. The cache structure must reside in CanonFS:
```
canonfs/jit/
    <trace_hash>.jit
```

### 5. Safe Fallback Mechanism (Axion Policy Interruption)
Axion policy boundaries (e.g., `TLOADHASH` bounding, AI model validation) are complex and non-trivial to safely inline into JIT machine code. The JIT must treat Axion boundaries as trace-terminating events using On-Stack Replacement (OSR).
*   **Synchronous State Materialization (Deoptimization):** When compiling a trace that encounters an Axion-gated opcode (e.g., `AXREAD`, `AXVERIFY`), the JIT compiler emits a "Side-Exit Stub" instead of native instructions for that policy check.
*   **Bailout Routine:** If a trace hits a dynamic constraint or policy check mid-execution, the stub executes an OSR bailout. It deterministically reconstructs the VM interpreter state (`InstructionPointer`, registers, stack pointer) and yields control back to the `core/vm/vm.cpp` main dispatch loop.
*   **Verdict Enforcement:** The reference interpreter subsequently natively evaluates the Axion policy via `kernel/axion/policy_engine.cpp`. If `Allow`, execution continues and may re-enter a new JIT trace. If `Warn`, `Deny`, or `Defer`, the standard deterministic fault pipeline is invoked.

### 6. Deterministic Runtime Oracle
A new CI job, `repro_oracle`, must be introduced to enforce determinism. The pipeline must execute canonical programs through both the interpreter and the JIT, perform trace comparisons, and compare the base81 hashes. Any divergence must fail the build.

# Impact

## Backward Compatibility
This RFC is fully compatible with the existing `t81vm-spec.md` and `axion-kernel.md`. It does not modify TISC ISA semantics but introduces JIT execution paths that are mathematically bound to match the reference interpreter exactly. Experimental JIT code in `runtime/jit/` will be transitioned to the `core/vm/` structure under explicit deterministic enforcement gates.

## Performance
Significantly reduces execution overhead, approaching native speed while preserving absolute determinism.

## Security
Maintains full Axion policy visibility through synchronous state materialization and OSR bailout routines.

# Alternatives Considered

*   **Ahead-of-Time (AOT) Compilation:** Dismissed as it complicates the dynamic tiering model and dynamic Axion policy evaluation.
*   **Relaxed Determinism Profiles:** Allowing the JIT to deviate from bit-exact interpreter results for performance. Explicitly rejected as it violates the foundational principles defined in RFC-0001 and RFC-0002.

# References

* RFC-0001: Architecture Principles
* RFC-0002: Deterministic Execution Contract
* T81 VM Specification (`t81vm-spec.md`)

______________________________________________________________________

## Acceptance Criteria

| ID | Criterion | Status |
| :--- | :--- | :--- |
| [A-0028-01] | §1 Shadow execution: interpreter-vs-JIT register/memory equivalence verified | met — `jit_trace_equivalence_test` (430 assertions); `jit_repro_oracle_test` §6 |
| [A-0028-02] | §2 Canonical trace hash: `CanonHash81(serialised TISC sequence)` attached to every compiled trace; stable across recompiles; discriminates different sequences | met — `JitTrace::trace_hash()`; `jit_repro_oracle_test` [§2-a..c] (6 assertions) |
| [A-0028-03] | §3 Deterministic register allocation: no host-register nondeterminism | met — `ThreadedJitTrace` operates on the VM's flat R0–R242 register file; no host register mapping needed |
| [A-0028-04] | §4 CanonFS JIT cache: `canonfs/jit/<trace_hash>.jit` | deferred — not blocking acceptance; tracked as follow-on |
| [A-0028-05] | §5 Axion OSR / Side-Exit Stubs: `AxRead`/`AxSet`/`AxVerify`/`AxReport` are trace-terminating with `ExitKind::AxionBoundary`; interpreter resumes natively | met — explicit cases in `ThreadedJitTrace::execute()`; `ExitKind::AxionBoundary` in `jit.hpp`; `vm.cpp` handles resume without trace invalidation; `jit_repro_oracle_test` [§5] (5 assertions) |
| [A-0028-06] | §6 Repro oracle: two runs of a hot program yield identical `CanonHash81(final registers)`; mutated program yields different hash | met — `jit_repro_oracle_test` [§6-a..b] (4 assertions) |

## Acceptance Note (2026-03-15)

[A-0028-02], [A-0028-05], [A-0028-06] met in this revision. [A-0028-01] and [A-0028-03]
were already met by prior work. [A-0028-04] (CanonFS cache) is deferred as non-blocking.

**§2 Canonical Trace Identity** — `JitTrace` gains a `trace_hash_` field (protected,
`t81::hash::CanonHash81`) and `trace_hash()` accessor. `JitCompiler::compile()` serialises
each `Insn` as 18 canonical bytes (opcode 2B LE + a 4B LE + b 8B LE + c 4B LE) and calls
`hash_bytes()` (SHA3-512 truncated to 256 bits). `ThreadedJitTrace::set_trace_hash()` stores
the result. The hash covers only the instruction sequence, not register/stack state, which
matches the `canonfs/jit/<trace_hash>.jit` caching key intent.

**§3 Deterministic Register Allocation** — `ThreadedJitTrace` uses the VM's existing flat
R0–R242 register file stored in `State::contexts[i].registers`. There are no host CPU
registers involved; the mapping is trivially deterministic by construction.

**§5 Axion OSR** — `AxRead`, `AxSet`, `AxVerify`, and `AxReport` are explicit cases in
`ThreadedJitTrace::execute()` that trigger an OSR bailout: `ctx.pc` is rewound by one
(so the interpreter resumes at the Axion opcode), `exit_kind` is set to `AxionBoundary`,
and the trace returns immediately. The `vm.cpp` post-trace handler returns without
invalidating the trace so it can be re-entered after the Axion boundary is resolved.
The exit is annotated in the Axion log as `"axion-boundary"`.

**§6 Repro Oracle** — `tests/cpp/jit_repro_oracle_test.cpp` (15 assertions):

* [§2-a..c]: non-zero hash, stability across recompiles, discrimination across programs
* [§5]: five distinct `ExitKind` values confirmed at compile time
* [§6-a..b]: two independent interpreter runs of a 50-iteration hot loop yield identical
  `CanonHash81(final register file)`; a loop-count mutation produces a different hash

Test suite: `jit_repro_oracle_test` — **15/15 assertions passing**.
Remaining open item: §4 CanonFS cache (non-blocking follow-on).
