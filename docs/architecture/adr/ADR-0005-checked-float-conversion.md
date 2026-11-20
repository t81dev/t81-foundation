# ADR-0005: Additive checked conversion with an explicit numerical profile

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [ADR-0005: Additive checked conversion with an explicit numerical profile](#adr-0005-additive-checked-conversion-with-an-explicit-numerical-profile)
  - [Context](#context)
  - [Proposed decision](#proposed-decision)
  - [API strategies](#api-strategies)
  - [Implementation alternatives after approval](#implementation-alternatives-after-approval)
  - [Rounding and representation decision](#rounding-and-representation-decision)
  - [Migration and consequences](#migration-and-consequences)
  - [Approval record](#approval-record)

<!-- T81-TOC:END -->


Status: Proposed — maintainer approval pending
Date: 2026-10-22
Authority: Design recommendation only; RFC-00D5 is draft, production unchanged

## Context

PRs #551/#552 demonstrate both platform drift and identical incorrect values.
The normative spec requires explicit errors for invalid/lossy floating conversion;
current bare-value `noexcept` bridges cannot communicate every such outcome. A
new contract must specify the representable set before defining determinism.
`noexcept` itself does not preclude error-returning APIs.

## Proposed decision

Recommend additive checked conversion, exact-only by default, with an explicit
nearest-even option reporting rounded success. Use integer/rational computation
and a validated finite-value boundary. Retain existing conversion algorithms as
legacy behavior until a separately approved migration. Do not expand ISA semantics,
serialization claims or the verified surface registry through this design PR.

## API strategies

| Strategy | Viability and compatibility | Loss/error visibility | Recommendation |
| --- | --- | --- | --- |
| New checked result/error API | Additive minor API after approval; old callers untouched. `expected<Conversion<T>,Error>` can report exact/rounded value and flags or an error. Validated inputs prevent hidden raw-state normalization. | Explicit and local; no successful value on error. | Preferred for new governed callers. |
| Compatible legacy wrapper | Keep existing signatures **and old algorithm**; document host/range/rounding limits and exclude from new guarantees. It remains usable for existing workloads. | No new numerical guarantee; callers needing checked outcomes opt in. | Required compatibility lane until deprecation is approved. |
| Reimplement old signatures as wrappers around checked conversion | Source-compatible but numerically incompatible on ties/extremes; choosing zero/Inf/NaE on error erases categories, changes hashes and can still hide loss. | Insufficient unless callers accept a new versioned fallback policy. | Rejected for silent rollout; only a future explicit migration proposal. |
| Exceptions / errno / thread-local status | Exception propagation conflicts with current noexcept; global channels couple unrelated operations and replay. Changing noexcept/signature breaks contracts. | Error can be ignored or confused with another operation. | Rejected as the checked protocol. |

Recommended shape is conceptual, not an installed header. Expected success data
include value, exact/rounded classification and zero normalization flags. Error
categories have no partially usable output. A validated-value wrapper versus a
checked legacy-object adapter is a binding decision still requiring approval.

## Implementation alternatives after approval

| Candidate | Advantages | Risk / stopping test |
| --- | --- | --- |
| Exact integer rational with arbitrary-width limbs | Directly follows cross-product comparisons, handles binary64 extremes without host FPU or intermediate-range loss; easy to compare to Python oracle. | Allocation/resource policy and approved integer backend needed. Do not reuse unsafe BigInt assumptions or accidentally noexcept-terminate on resource failure. |
| Bounded fixed-width integer arithmetic | For admitted profiles, bounds on denominator/powers and intermediate products can be proven; can avoid heap allocation. | Must prove every intermediate bound for both directions and every parameter, not merely mantissa width. Stop on any unchecked overflow or unsupported format accidentally admitted. |
| Host long double / split pow / increased precision | Small patch, potentially useful as an off-core diagnostic control. | Still host-dependent log/rounding/range; fixes for unsigned scale alone do not settle ties/normalization. Rejected as correctness foundation. |

No implementation backend is selected by benchmarking in this PR. There is no
transcendental or inference-performance work. The exact reference defines outputs
independently of either future integer backend.

## Rounding and representation decision

A globally nearest search over the canonical finite set is required. At odd-radix
exponent boundaries, both endpoint mantissas can have the same parity; proposed
nearest-even is completed by smaller-exponent tie selection. Nearest-away and
truncation remain comparison alternatives with different exact outcomes. Agreeing
hashes or approximate tolerances cannot substitute for this numerical choice.

The old audit oracle is evidence for one nearest-candidate model; it is not prior
approval of the finite subset or a full checked API. The spec's base-81/BigInt
language, actual bounded base-3 fields, default zero state and raw decoder's invalid
code handling need explicit maintainer decisions before bridging legacy objects.

## Migration and consequences

1. Approve RFC-00D5 semantics and all pending decisions first.
2. Introduce checked boundaries without redirecting legacy calls.
3. Migrate explicit host intake/reporting callers with named loss/error policy.
4. Treat constructors, widening, numeric formatting and content-addressed results
   as separate compatibility review points; preserve old bytes and evidence.
5. Leave VM opcode behavior and existing float handle values untouched. Any VM
   adoption must preserve frozen semantics or take the formal change route.
6. Keep production conversion probes excluded from integer provenance until exact
   numerical conformance and later native evidence justify a separate promotion.

Residual risks: strict text validation differs from raw object acceptance;
exact-only will reject many familiar binary/ternary fractions; rounded success
requires caller acknowledgement; profile changes alter identities. These are
visible compatibility costs, not reasons to hide errors.

## Approval record

**Not approved for production implementation.** Maintainers must resolve all five
RFC-00D5 §11 decisions. This document records a proposed decision, not an accepted
ADR or a new determinism guarantee.

References: [RFC-00D5](../../../spec/rfcs/RFC-00D5-checked-floating-point-conversion.md),
[compatibility audit and plan](../../../experiments/checked_float_contract/PLAN.md).
