______________________________________________________________________

# RFC-0008 — T81 Formal Verification Harness

Version 0.1 — Draft (Standards Track)\
Status: Draft\
Author: Verification Working Group\
Applies to: Spec Suite, Tooling, Axion, T81Lang

______________________________________________________________________

# 0. Summary

This RFCallows us to **prove** determinism properties end-to-end. It defines:

1. A canonical trace format (`.t81trace`) for proofs.
1. A reference checker that replays traces against the specs.
1. Integration hooks for SMT/proof assistants.

______________________________________________________________________

# 1. Motivation

T81’s specs insist on determinism, but we currently rely on conventional
unit/integration tests. We need a formal harness that:

- consumes the same TISC binaries the VM runs
- records execution traces with Axion metadata
- feeds them into repeatable proof pipelines

This ensures that new ISA or compiler features (e.g., Option handling) come
with machine-checkable guarantees.

______________________________________________________________________

# 2. Design / Specification

### 2.1 Trace Format

- `.t81trace` is a binary, canonicalized format containing:
  - instruction stream
  - register snapshots
  - memory digests
  - Axion verdicts
- Hash is base-81 encoded; identical traces yield identical hashes.

### 2.2 Reference Replayer

- `t81-verify` loads a program + trace and deterministicly replays it using
  the spec-defined semantics (not the production VM).
- Divergence → proof failure.
- Tooling outputs witnesses (counterexamples) for use in SMT solvers.

### 2.3 SMT Interface

- Provide a library that exports TISC semantics as logical predicates.
- Users can plug into Z3/Isabelle/etc. to prove invariants about their code.

### 2.4 Axion Hooks

- Axion can request proof artifacts for high-tier deployments.
- Failure to provide a matching `.t81trace` hash results in deterministic
  rejection at deployment time.

______________________________________________________________________

# 3. Rationale

- Formal verification is useless without canonical inputs; the trace format
  ensures identical behavior across machines.
- Replayer sits between the spec and VM to catch deviations early.
- SMT hooks encourage third parties to prove additional safety properties.

______________________________________________________________________

# 4. Backwards Compatibility

- Optional: legacy builds can ignore the harness.
- Once per-tier policies require proofs, deployments supply traces; the VM
  behavior is otherwise unchanged.

______________________________________________________________________

# 5. Security Considerations

- Verified builds reduce risk of compiler/VM bugs being exploited.
- Trace hashes help detect tampering in the supply chain.

______________________________________________________________________

# 6. Open Questions

1. Should the trace format embed compressed memory diffs or require full dumps?
1. How do we align trace semantics with future parallel extensions?
1. Should Axion require proofs for certain opcode classes by default?

______________________________________________________________________
