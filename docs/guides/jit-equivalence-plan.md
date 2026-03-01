# JIT Equivalence Plan

**Status:** **Experimental / Planned**
**Reference:** `spec/t81vm-spec.md`
**Objective:** Formalize the criteria for adopting Just-In-Time compilation into the Verified Core.

This document defines the rigorous equivalence requirements that any JIT implementation (e.g., Trace JIT, Method JIT) MUST satisfy before being enabled by default or considered "stable".

## 1. Formal Equivalence Requirement

The JIT is considered **equivalent** to the Interpreter if and only if:

For every valid TISC program $P$ and initial state $S_0$:
$$ Trace(Interpreter(P, S_0)) \equiv Trace(JIT(P, S_0)) $$

Where:
*   $Trace$ is the sequence of user-visible side effects (memory writes, register updates, faults).
*   $\equiv$ denotes **bit-exact identity** of all state values.

**Constraint:** The JIT MUST NOT introduce any observable nondeterminism (e.g., from speculation, optimization reordering, or hardware flags).

## 2. State Transition Equality

A single instruction execution $S_i \to S_{i+1}$ is equivalent if:
1.  **Register State**: `R[0..80]` are identical.
2.  **Memory State**: `MEM` contents are identical.
3.  **Flags**: `FLAGS` register is identical.
4.  **Faults**: Any fault raised is identical in type and code.

## 3. Verification Strategy: Hash-Based Equivalence

To verify equivalence without storing full traces:

1.  **Canonical State Hashing**:
    Define a hash function $H(S)$ that uniquely fingerprints the VM state.

    $$ H(S_i) = \text{Hash}(R, PC, SP, FLAGS, \text{ModifiedMemory}) $$

2.  **Checkpoint Verification**:
    At regular intervals (e.g., every basic block or function call), both Interpreter and JIT compute $H(S_i)$.

    The JIT is valid if:
    $$ \forall i, H_{Interp}(S_i) == H_{JIT}(S_i) $$

3.  **Divergence Detection**:
    If $H_{Interp} \neq H_{JIT}$, the JIT MUST abort and fall back to the interpreter (Deoptimization), or fault if in a strict verification mode.

## 4. Required Future Tests

Before JIT can be marked **Verified**:

1.  **Fuzzing Campaign**:
    Differential fuzzing between Interpreter and JIT on 1 billion random valid instruction sequences.

2.  **Corner Case Suite**:
    Explicit tests for:
    *   Self-modifying code (should be impossible/fault).
    *   Precise exception handling (faults must occur at exact instruction boundary).
    *   Floating-point edge cases (NaN payloads, denormals).

3.  **Hardware Diversity**:
    Verify equivalence on x86-64 (AVX2/AVX512) and ARM64 (NEON).

## 5. Preconditions for Stabilization

The JIT will transition from **Experimental** to **Stable** only when:

1.  Equivalence is formally proven via the Hash-Based strategy in CI.
2.  No regression in determinism is observed across platforms.
3.  The performance benefit is >2x (otherwise complexity cost is unjustified).
