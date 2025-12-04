______________________________________________________________________

# RFC-0019 — Axion Match & Loop Metadata Enforcement

Version 0.1 — Draft (Standards Track)\
Status: Draft\
Author: T81 Foundation Compiler Council\
Applies to: Axion Kernel, T81VM, TISC, T81Lang

______________________________________________________________________

# 0. Summary

This RFC mandates that Axion MUST receive structured metadata describing loop annotations and enum guards so the Trace Subsystem (DTS) and Verification Subsystem (VS) (see `spec/axion-kernel.md` §1.1‑1.7) can reason about guard-aware execution. It ties that metadata into the Axion Policy Language (APL, RFC-0009) by requiring policies to reference the same trace descriptors and enumerated variant IDs.

______________________________________________________________________

# 1. Motivation

Axion’s responsibilities (§1.1–§1.7 of `spec/axion-kernel.md`) highlight its central role for determinism, safety, and metadata hosting. Recent compiler cables already emit `(policy …)` loops for Axion, but match/enum guard events lack the same formal contract. Without a canonical payload (variant id, enum id, guard hit/miss, loop annotation) in Axion logs, deterministic auditing, guards, and policy enforcement cannot verify guard coverage or metadata hints. RFC-0009 introduces APL but does not prescribe what metadata must be paired with the policy. This RFC fills that gap.

______________________________________________________________________

# 2. Design

## 2.1 Extended Metadata Contract

Axion-aware compilers/CLI MUST embed two classes of metadata into the Axion trace payload stored in `t81::tisc::Program`:

1. **Loop guards** described by `(policy (loop (id …) (file …) (line …) (column …) (annotated true|false) (bound …)))`.
2. **Structural match metadata** with entries like `(match-metadata (match (scrutinee Enum) (arms (arm (enum-id E) (variant-id V) (variant VariantName) ...))))`.

Every metadata record MUST include:

 * The Axion policy tier(s) that triggered emission (respects RFC-0009’s `tier`/`allow-opcode` semantics).
 * The canonical enum id/variant id pair (see `include/t81/enum_meta.hpp`) so Axion can reason deterministically about guards across compilations.
 * Payload type names to verify that Axion’s type invariant (spec §1.2) sees the declared payload type.

## 2.2 VM Trace Events

Axion’s trace log (DTS) MUST receive events for Axion-aware opcodes such as `ENUM_IS_VARIANT`, `ENUM_UNWRAP_PAYLOAD`, `OPTION_UNWRAP`, `RESULT_UNWRAP_OK`, and `RESULT_UNWRAP_ERR`. Each event must:

1. Pack the encoded variant id (enum id + variant id).
2. Include the guard result (pass/fail) and the payload value/tag (when unwrapping).
3. Reference the active policy (per RFC-0009) and, ideally, include the metadata reason string (`AxionEvent.verdict.reason`) describing enum/loop context (e.g., “enum guard enum=Color variant=Blue match=pass payload=i32”).

This flow allows VS/CRS to correlate guard success with metadata obligations, verifying that no guard path is missing an arm (per spec axioms) and enabling Axion to deny transmissions that violate determinism (spec §1.1).

## 2.3 Policy Language Link

APL’s `allow-opcode`/`deny-opcode` clauses (RFC-0009 §2.2) now expect the same metadata naming and variant IDs emitted to the Axion log. Policy authors may reference `(allow-opcode ENUM_UNWRAP_PAYLOAD)` and assert that Axion logs must handshake with `(match-metadata ...)` entries for the referenced enum id/variant combos. This linkage assures that Axion’s Constraint Resolution Subsystem (CRS) and Recursion Control Subsystem (RCS) know whether guard coverage aligns with policy-level expectations.

______________________________________________________________________

# 3. Security & Determinism Implications

 * **Determinism** – Encoded ids prevent Axion from relying on order-dependent strings; the variant id encoding is canonical and ABI-stable.
 * **Security** – Axion can detect if metadata is absent or malformed and trigger a deterministic trap (per spec §1.2 and §1.6).
 * **Policy compliance** – Tie-in to RFC-0009 ensures that policies that refer to guard coverage cannot be spoofed by malformed metadata.

______________________________________________________________________

# 4. Deployment & Backward Compatibility

Legacy binaries that lack match metadata continue to run; Axion treats the metadata as optional but logs warnings (DTS) and denies guard-specific policies. The compiler should emit warnings when metadata is missing. Tools like CLI/ir-inspector (see `docs/guides/cli-toolkit.md`) should expose metadata fulfillment.

______________________________________________________________________

# 5. Open Questions

1. Should Axion verify payload types beyond requiring their names (e.g., canonical handles or `t81::tisc::TypeAliasMetadata`)?
2. Do loops annotated with `@bounded(loop(...))` require Axion to accept guard expressions as policies?
3. How should Axion handle match metadata when multiple enums share variant ids? (Encoded ids should prevent collisions.)

______________________________________________________________________

