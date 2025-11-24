______________________________________________________________________

# RFC-0009 — Axion Policy Language (APL)

Version 0.1 — Draft (Standards Track)\
Status: Draft\
Author: Axion Governance Council\
Applies to: Axion, T81VM, T81Lang, Cognitive Tiers

______________________________________________________________________

# 0. Summary

This RFC introduces the **Axion Policy Language (APL)**, a declarative DSL
used to describe recursion limits, shape bounds, and symbolic constraints that
Axion enforces at runtime. It defines syntax, semantics, and integration points
with the VM and compiler.

______________________________________________________________________

# 1. Motivation

Axion currently consumes handwritten JSON-like policies. They are hard to audit
and lack formal semantics. APL provides:

- canonical syntax checked by the spec suite
- deterministic evaluation semantics
- direct references to tier concepts in `spec/cognitive-tiers.md`

______________________________________________________________________

# 2. Design / Specification

### 2.1 Syntax

APL is a minimal s-expression language:

```
(policy
  (tier 3)
  (max-stack 59049)
  (allow-opcode TVECADD)
  (deny-shape (> dims 4))
  (require-proof hash:XYZ))
```

- Symbols and literals are base-81 strings; numbers are balanced ternary.
- Policies compile to deterministic bytecode consumed by Axion.

### 2.2 Semantics

- `tier n` sets the maximum cognitive tier; Axion rejects programs exceeding it.
- `max-stack` and `max-trace` enforce resource ceilings.
- `allow-opcode` / `deny-opcode` whitelist/blacklist instructions with canonical
  ordering to avoid policy conflicts.
- `deny-shape` uses predicates referencing tensor metadata defined in RFC-0004.
- `require-proof` ties policies to RFC-0008 artifacts.

### 2.3 VM Integration

- APL bytecode is embedded into the program header.
- VM exposes `AxPolicy` syscalls for reading the active policy.
- Policy violations trigger deterministic `Trap::SecurityFault`.

### 2.4 Compiler Tooling

- `t81c` validates policies during build, rejecting programs whose source-level
  annotations contradict the active policy.
- Macros allow `@requires_policy(policy_name)` on functions.

______________________________________________________________________

# 3. Rationale

- S-expressions are easy to parse deterministically and align with symbolic
  reasoning goals.
- Explicit op/shape controls give Axion precise levers without baking policies
  into code.
- Tight integration with proofs ensures high-tier deployments can be audited.

______________________________________________________________________

# 4. Backwards Compatibility

- Legacy JSON policies continue to work until sunset; the runtime translates
  them into APL bytecode.
- Programs without embedded policies inherit the deployment default.

______________________________________________________________________

# 5. Security Considerations

- Canonical syntax prevents policy spoofing via whitespace or ordering tricks.
- Embedding proof hashes ensures the running binary matches the verified one.

______________________________________________________________________

# 6. Open Questions

1. Should we support policy composition (e.g., include other policies)?
1. Do we need policy capabilities (e.g., allow certain opcodes only inside
   specific modules)?
1. How do we distribute signed policy bundles for offline deployments?

______________________________________________________________________
