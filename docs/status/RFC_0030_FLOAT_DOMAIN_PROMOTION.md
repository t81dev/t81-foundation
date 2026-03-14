# RFC-0030 Float-Domain Promotion Note

Last Updated: 2026-03-14
Owner: @t81dev
Purpose: define the next design target for deterministic tensor math after the
phase-1 AI opcode closure work.

## Problem

The repo now has materially stronger deterministic arithmetic in tensor and AI
kernels, including deterministic fallback paths for `exp`, `sqrt`, `log`,
`softmax`, `silu`, `rmsnorm`, `rope`, `attention`, `qmatmul`, elementwise
division, and non-fixed `matmul` in deterministic builds.

What is still unresolved is not "more math cleanup." It is the policy for what
happens after deterministic float-domain computation completes.

Today the tensor surface mixes three concepts:

- storage form (`canonical_fixed_authoritative()` and canonical fixed cache)
- arithmetic discipline (host-float vs deterministic soft-math path)
- semantic class (`HostFloat`, `ExactInt`, `ExactTrit`)

Those are related, but they are not the same thing.

## Current Repo Behavior

The current implementation already demonstrates these distinctions:

- A tensor can have canonical fixed storage authority and still remain
  `HostFloat`.
- `strict_core_eligible()` is currently derived from `numeric_class` rather
  than directly from arithmetic provenance.
- Deterministic builds now avoid major raw `<cmath>` and AVX/FMA fallback
  lanes even when the result class remains `HostFloat`.
- Non-fixed `matmul` is deterministic in `T81_DETERMINISTIC` mode, but still
  returns a float-domain tensor and does not automatically promote to canonical
  exact semantics.

That means the repo no longer has a purely binary split of:

- `Exact*` = deterministic
- `HostFloat` = host-dependent

Instead, the real split is closer to:

- exact integer/trit tensors
- deterministic float-domain tensors
- host-tolerant float-domain tensors

## Proposed Model

RFC-0030 should define promotion rules around three independent axes.

### 1. Storage Axis

- `CanonicalFixed`
- `HostCacheOnly`

This answers: can the tensor be represented canonically in fixed storage?

### 2. Arithmetic Axis

- `StrictDeterministicMath`
- `HostTolerantMath`

This answers: was the result produced using deterministic math rules or host
numeric behavior?

### 3. Semantic Class Axis

- `ExactTrit`
- `ExactInt`
- `FloatDomain`

This answers: what kind of value does the tensor semantically contain?

Under this model, `HostFloat` is too overloaded as a name. It currently means
both "non-exact float-domain tensor" and, historically, "potentially host-
dependent math." RFC-0030 should decide whether to:

- keep `HostFloat` as the semantic class label but document that arithmetic
  provenance is separate, or
- replace it in follow-on work with a clearer float-domain class split.

## Recommended Short-Term Policy

Until a larger type/model change is accepted, use this rule:

- Promote to `ExactTrit` or `ExactInt` only when the operation is semantically
  exact and the tensor remains strict-core eligible by current rules.
- Keep non-exact float-domain results classified as `HostFloat` even when the
  arithmetic path is deterministic.
- Allow deterministic builds to strengthen arithmetic guarantees without
  implicitly promoting float-domain tensors into exact classes.

This matches the current repo trajectory and avoids silently changing the
meaning of `strict_core_eligible()`.

## Primary RFC-0030 Question

What should deterministic float-domain tensors mean in the public contract?

There are two credible paths:

### Option A: Keep Current Public Classes

- `ExactTrit`
- `ExactInt`
- `HostFloat`

Interpretation:

- `HostFloat` means "non-exact float-domain tensor"
- deterministic-vs-host-tolerant behavior is tracked by implementation mode,
  profile, or provenance, not by `numeric_class`

Pros:

- smallest surface change
- least test churn
- preserves current VM/helper predicates

Cons:

- the name `HostFloat` becomes increasingly misleading

### Option B: Introduce Deterministic Float-Domain Distinction

Potential future class split:

- `ExactTrit`
- `ExactInt`
- `DeterministicFloat`
- `HostFloat`

Pros:

- makes arithmetic provenance explicit
- aligns better with the deterministic profile language

Cons:

- larger API/test/spec change
- requires reevaluating `strict_core_eligible()`
- forces wider migration work across VM, tensor helpers, native decode, and
  conformance tests

## Recommendation

Use Option A for the next implementation phase.

That means:

- do not rename `HostFloat` yet
- do not auto-promote deterministic float-domain tensor results into exact
  classes
- explicitly document that deterministic arithmetic and semantic exactness are
  separate concerns

Then, if the naming or contract becomes too confusing, open a focused follow-on
RFC for numeric-class expansion rather than folding that change into RFC-0030
kernel work.

## Current Decision

Effective as of 2026-03-08:

- `strict_core_eligible()` remains tied to `numeric_class`
- no separate arithmetic-provenance bit is introduced at this time
- deterministic float-domain tensors continue to use `HostFloat`
- deterministic arithmetic strengthening may continue without changing the
  public tensor class model

This is an intentional deferral, not an omission.

## Reopen Conditions

Revisit the decision only if one of these becomes true:

1. a public API or conformance contract needs to distinguish deterministic
   float-domain tensors from host-tolerant ones
2. Axion policy/audit surfaces need arithmetic provenance as a first-class
   predicate
3. `strict_core_eligible()` starts blocking legitimate deterministic use cases
   that should not require exact-int promotion
4. the name `HostFloat` causes repeated spec or user confusion that cannot be
   addressed by documentation alone

## Concrete Next Step

The next work item should be a small contract pass, not a kernel rewrite:

1. document in spec/status materials that deterministic float-domain tensors
   can still be classified as `HostFloat`
2. identify whether any public API or conformance tests incorrectly assume
   `HostFloat` implies host-dependent arithmetic
3. only after that, decide whether `matmul` or other float-domain kernels need
   further promotion behavior changes

Status: items 1 and 2 are complete in the current repo checkpoint.
