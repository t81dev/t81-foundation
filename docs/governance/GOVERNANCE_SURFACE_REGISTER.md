# Governance Surface Register

Last Updated: 2026-03-19  
Owner: @t81dev

**Purpose:** one operational source of truth for surface class assignment.

This register classifies major T81 surfaces into exactly one of the classes
defined by
[RFC-0048-deterministic-surface-definition-and-governance-boundaries.md](/spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md):

1. Deterministic Core Profile (DCP)
2. Governed non-DCP
3. Experimental
4. Out of scope

## Authority

Classification precedence:

1. `/spec/**`
2. this register plus [DETERMINISM_SURFACE_REGISTRY.md](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md)
3. governance/status documents
4. descriptive README text

If a lower-authority document disagrees with this register, the lower-authority
document must be corrected.

## Class Definitions

### Deterministic Core Profile (DCP)

Use only when the surface has:

- normative semantics
- explicit deterministic boundary
- executable proof
- CI enforcement
- merge-blocking regression treatment

### Governed non-DCP

Use when the surface is:

- architecturally important
- policy-bounded or governance-bounded
- evidenced in a limited or scoped way
- not entitled to full deterministic claims

### Experimental

Use when the surface is:

- under active design or validation
- unstable in contract or boundary
- not ready for stability or equivalence claims

### Out of Scope

Use when the behavior is intentionally excluded from deterministic or governance
claims for the current release.

## Surface Register

| Surface | Primary Paths | Class | Current Claim Boundary | Evidence / Notes |
| :--- | :--- | :--- | :--- | :--- |
| Data Types | `core/types/`, `include/t81/` | DCP | Verified deterministic core | Frozen core path; canonical encoding and numeric contract tests |
| TISC ISA | `core/isa/`, `spec/tisc-spec.md` | DCP | Verified deterministic core | Frozen ISA semantics; freeze-integrity and VM determinism gates |
| T81VM Interpreter | `core/vm/` | DCP | Verified deterministic core for interpreter execution | Verified interpreter path only; broader acceleration is outside this class |
| Soft-Float / Deterministic Math | `core/types/`, `t81_soft_math` surfaces | DCP | Verified deterministic arithmetic boundary | Soft-float and arithmetic contract tests |
| Canonical Numeric / Wire Encoding | `core/types/`, TISC binary IO | DCP | Verified deterministic binary format | Canonical numeric contract and IO determinism tests |
| T81Lang Frontend / Toolchain | `lang/frontend/`, CLI compile/build flows | Governed non-DCP | Stable language surface, partial deterministic emission evidence only | Fixture-bounded reproducibility; not a full DCP compiler surface |
| Axion Governance Kernel | `kernel/axion/`, `spec/axion-kernel.md` | Governed non-DCP | Policy/governance surface with bounded proof | Important and stable in product sense; not a verified deterministic surface as a whole |
| CanonFS | `include/t81/canonfs/`, `fs/`, `spec/supplemental/canonfs-spec.md` | Governed non-DCP | Deterministic object-store contract with current-release narrowing | Spec/impl reconciliation in progress through RFC-0054; not DCP |
| DPE | `include/t81/dpe/`, `src/dpe/` | Governed non-DCP | Accepted deterministic execution model outside DCP | Stable implementation, accepted RFC series, evidence bounded to governed scope |
| Benchmark Suite | `benchmarks/` | Governed non-DCP | Evidence and performance support surface | Important enforcement/support tool, not itself a DCP execution surface |
| TUI Frontends | `tooling/tui/`, `t81 studio`, `t81 agent` | Governed non-DCP | Product-facing UI surface with bounded evidence | Usable, but not a deterministic core surface |
| T81Graph | `include/t81/types/T81Graph.hpp`, `include/t81/cog/v1/` | Governed non-DCP | Implemented graph surface with bounded evidence | Narrow symbolic-runtime subset relocated; whole surface not promoted to DCP |
| Ternary-Native Inference | RFC-0034 / RFC-0037 related runtime and tooling paths | Governed non-DCP | Evidence-backed but broader than current verified boundary | Promotion remains surface-specific |
| Governed FFI | RFC-00B8 / RFC-0036 related paths | Governed non-DCP | End-to-end bridge with bounded evidence | Schema/sandbox/policy hardening still open |
| TernaryOS User Environment | `include/t81/axion/userenv/`, `include/t81/axion/shell/`, `src/axion/userenv/`, `src/axion/shell/` | Governed non-DCP | Accepted RFC-00B9 slice in opt-in TernaryOS build | Stable public boundary extracted from `userland/experimental/`; still outside DCP |
| Axion OS / TernaryOS Kernel Lane | `userland/experimental/hal/`, `kernel/`, `mmu/`, `sched/`, `ipc/`, `dev/` | Governed non-DCP | Governed experimental kernel track only | CI-enforced epoch parity and boot evidence exist, but whole runtime remains experimental in layout and scope |
| SIMD / SWAR / Alternate Backend Execution Beyond Interpreter Scope | acceleration, JIT, lowering, backend-specific paths | Governed non-DCP | Must inherit equivalence rules; not promoted by implementation alone | Governed by RFC-0042 through RFC-0047 direction |
| Cognitive Tiers | `experimental/tiers/`, `experimental/tiers/cog/` | Experimental | Experimental only | No whole-surface promotion |
| Hanoi VM | `experimental/hanoi/` | Experimental | Experimental only | Alpha maturity; promotion blockers still open |
| Distributed Compute | `experimental/distributed/` | Experimental | Experimental only | Non-deterministic by design at current stage |
| Remaining Experimental Headers | `include/t81/experimental/` | Experimental | Experimental only | Must not be treated as stable public contract |
| Examples / Notebooks | `examples/`, `notebooks/` | Experimental | Illustrative only | No stability or determinism claim |
| Hosted / External AI adapters | llama.cpp bridge and similar external adapter paths | Governed non-DCP | Practical reproducibility only | Controlled, but outside verified deterministic scope |
| Wall-clock timing | profiling, elapsed time, host clocks | Out of scope | Never deterministic | Must not be represented as part of deterministic guarantees |
| Network timing / packet arrival order | network IO behavior | Out of scope | Never deterministic | Transport timing is excluded from deterministic claims |
| Hardware FPU behavior | host floating-point outside `T81Float` | Out of scope | Never deterministic | Use soft-float deterministic path instead |
| Real-time scheduling | host scheduler timing | Out of scope | Never deterministic | Scheduler latency is excluded from guarantees |
| Unsupported accelerators / hardware-specific execution | unverified device backends | Out of scope | Never deterministic for current release | No implicit promotion by proximity |

## Immediate Interpretation Rules

- Being inside `include/t81/` does not by itself grant DCP status.
- Being outside `experimental/` does not by itself grant DCP status.
- Being RFC-accepted does not by itself grant DCP status.
- Build-gated and opt-in surfaces may still be governed non-DCP.
- Experimental code may have tests and still remain experimental.

## Promotion Notes

### Promotion to Governed non-DCP

Requires at minimum:

- explicit boundary
- ownership in governance docs
- evidence that the surface is more than a prototype
- overclaim-safe docs

### Promotion to DCP

Requires at minimum:

- normative semantics
- explicit deterministic boundary
- executable replay/conformance evidence
- CI enforcement
- threat-model coverage
- registry update

## Current Review Priorities

1. CanonFS contract closure through RFC-0054.
2. Complete post-extraction docs and internal cleanup for the RFC-00B9 user-environment boundary.
3. Continued CLI surface pruning so public commands reflect supported classes.
4. Replay-bundle evidence for core governed workflows.

## Cross-References

- [DETERMINISM_SURFACE_REGISTRY.md](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md)
- [FROZEN_CORE_PROFILE.md](/docs/status/FROZEN_CORE_PROFILE.md)
- [IMPLEMENTATION_MATRIX.md](/docs/status/IMPLEMENTATION_MATRIX.md)
- [EXTENSION_PROFILE.md](/docs/status/EXTENSION_PROFILE.md)
- [SPEC_AUTHORITY_MODEL.md](/docs/governance/SPEC_AUTHORITY_MODEL.md)
- [RFC-0048-deterministic-surface-definition-and-governance-boundaries.md](/spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md)
