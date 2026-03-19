______________________________________________________________________

# RFC-0009 — Axion Policy Language (APL)

Version 0.1 — Standards Track\
Status: Superseded\
Updated: 2026-03-15\
Author: Axion Governance Council\
Applies to: Axion, T81VM, T81Lang, Cognitive Tiers
Superseded-By: RFC-0022
Supersession note: RFC-0022 is the preferred forward evolution path and supersedes this RFC.

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

Axion policies may also assert the segment trace strings described in [RFC-0020](RFC-0020-axion-segment-trace.md). The runtime now logs each stack, heap, tensor, and meta transition as a `verdict.reason` such as `stack frame allocated stack addr=243 size=16`, `heap block freed heap addr=512 size=32`, `tensor slot allocated tensor addr=5`, `meta slot axion event segment=meta addr=1283`, `AxRead guard segment=stack addr=42`, or `AxSet guard segment=heap addr=128`. Policies can use:

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

# 7. Acceptance Criteria

| ID | Criterion | Evidence |
| :--- | :--- | :--- |
| [A-0009-01] | APL s-expression parser implemented and rejects malformed input | `parse_policy()` in `include/t81/axion/policy.hpp`; `t81_vm_policy_parse_fail_closed_test` |
| [A-0009-02] | `tier` directive enforced: programs exceeding the policy tier are denied | `axion_recursion_guardrails_test`, `spec_conformance_axion-kernel_tier-supervision-invariant` |
| [A-0009-03] | `allow-opcode` / `deny-opcode` enforcement: denied opcodes trigger `SecurityFault` deterministically | `t81_test_axion_opcodes`, `t81_vm_axreport_policy_deny_fail_closed_test` |
| [A-0009-04] | Fail-closed on parse error: a policy that cannot be parsed halts execution | `t81_vm_policy_parse_fail_closed_test` |
| [A-0009-05] | AI hook interception subject to active APL policy | `axion_ai_hooks_test` |
| [A-0009-06] | Policy evaluation is deterministic: same policy + input → identical verdict sequence | `axion_policy_allow_deny_determinism_test` |
| [A-0009-07] | CanonFS hook applies the active policy to executable objects at load time | `canonfs_hook.cpp` `parse_policy()` call; `canonfs_axion_trace_test` |

______________________________________________________________________

## Acceptance Note (2026-03-15)

All seven criteria above are met as of this date.

The APL s-expression syntax `(policy (tier N) (allow-opcode X) …)` specified in §2.1 is fully implemented in `include/t81/axion/policy.hpp` (`PolicyLexer` → `parse_policy()`). The `policy.hpp` parser handles `tier`, `max-stack`, `max-trace`, `allow-opcode`, `deny-opcode`, `require-proof`, and the guard/loop metadata predicates added in §2.5. Legacy JSON policies noted in §4 are translated at load time via `canonfs_hook.cpp`.

Key implementation mapping:

- **§2.1 Syntax / §2.2 Semantics** — `include/t81/axion/policy.hpp`: `PolicyLexer`, `parse_policy()`, `Policy` struct.
- **§2.3 VM Integration** — `kernel/axion/policy_engine.cpp`: `PolicyEngine::evaluate()` intercepts every TISC opcode; `SecurityFault` on deny.
- **§2.4 Compiler Tooling** — `t81_vm_policy_parse_fail_closed_test` validates build-time rejection of policy-violating programs.
- **§2.5 Guard & Loop Predicates** — `policy.hpp` extended predicates; `axion_ai_hooks_test` verifies runtime guard interception.
- **§3 Rationale** — confirmed: deterministic s-expression evaluation avoids whitespace/ordering spoofing (§5 Security Considerations satisfied).

Suite status at acceptance: **49/49 axion + ethics + tier + policy tests passing**.

______________________________________________________________________
