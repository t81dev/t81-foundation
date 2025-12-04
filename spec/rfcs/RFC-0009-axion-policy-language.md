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

### 2.5 Guard & Loop Metadata Predicates

APL now closes the loop on the guard metadata described in [RFC-0019](RFC-0019-axion-match-logging.md). The compiler embeds `(policy (loop ...))` hints, `(match-metadata ...)` s-expressions, and canonical enum/variant ids inside the `.tisc` header so Axion can pair `EnumIsVariant`/`EnumUnwrapPayload` events with policy expectations. Policy authors can express these obligations through dedicated metadata predicates:

```
(require-match-guard
  (enum Color)
  (variant Blue)
  (payload i32)
  (result pass))
```

`require-match-guard` asserts that the Axion trace contains a guard event for the given enum and variant with the expected payload chemistry and pass/fail result; it automatically lifts the `variant-id` and `enum-id` emitted by the compiler into the policy layer. Policies that allow `ENUM_UNWRAP_PAYLOAD` or `ENUM_IS_VARIANT` while also requiring specific guards can therefore prove that guard coverage matches Axion’s deterministic metadata.

Loop hints are exposed through `require-loop-hint`, for example:

```
(require-loop-hint
  (id 3)
  (annotated true)
  (bound 100))
```

This clause ensures the DTS saw a `(policy (loop (id 3) (file …) (line …) (column …) (bound 100) (annotated true)))` entry emitted by `format_loop_metadata` and that Axion can match it against the runtime guard trace before permitting high-tier opcodes inside an unbounded loop. The metadata/guard predicates stay optional so legacy binaries without the new instrumentation continue to run, but a missing guard or loop hint can trigger a deterministic `Policy Fault` if the policy explicitly `require`s it.

### 2.6 Segment Trace Predicates

Axion policies may also assert the segment trace strings described in [RFC-0013](RFC-0013-axion-segment-trace.md). The runtime now logs each stack, heap, tensor, and meta transition as a `verdict.reason` such as `stack frame allocated stack addr=243 size=16`, `heap block freed heap addr=512 size=32`, `tensor slot allocated tensor addr=5`, `meta slot axion event addr=1283`, `AxRead guard segment=stack addr=42`, or `AxSet guard segment=heap addr=128`. Policies can use:

```
(require-segment-event
  (segment tensor)
  (action "tensor slot allocated")
  (addr 5))
```

`require-segment-event` emits a deterministic `Policy Fault` if the Axion log lacks the recorded `verdict.reason`. Guard-dependent policies can similarly demand that `AxRead guard ...` or `AxSet guard ...` strings appear before approving privileged actions, tightly binding segment context to policy enforcement.

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
2. Do we need policy capabilities (e.g., allow certain opcodes only inside
   specific modules)?
3. How do we distribute signed policy bundles for offline deployments?

______________________________________________________________________
