# RFC-0026: AI-Native Inference Opcodes

**Status:** accepted
**Type:** standards-track
**Applies-To:** `spec/tisc-spec.md` §5, `spec/t81-data-types.md` §11, `spec/t81vm-spec.md`
**Created:** 2026-03-01
**Updated:** 2026-03-08
**Supersedes:** —
**Superseded-By:** —
**Discussion:** —

---

## Summary

Status note: accepted for the phase-1 opcode surface (`ATTN`, `QMATMUL`,
`EMBED`, `WLOAD`, `GATHER`, `SCATTER`) now present in the repo. Deterministic
math tightening for the current opcode/kernel surface is substantially closed;
remaining follow-on work is primarily `WLOAD` promotion/hardening review and
broader float-domain policy tracked under `RFC-0030`, not design uncertainty.

T81 is conceived by AI, for AI. This RFC defines a new TISC opcode class —
**AI-Native Inference Opcodes** — that elevates attention, quantized matrix
multiplication, weight loading, embedding lookup, and gather/scatter into
first-class ISA primitives. These operations are the bottleneck of every
practical AI inference workload; they belong in the ISA, not in user-space
T81Lang libraries.

---

## Motivation

The current TISC ISA has tensor operations (TLOAD, TSTORE, TMATMUL, etc.) and
a rich type system that includes `T81Tensor`, `T81Prob`, and `T81Qutrit`. What
is missing are the specific computational primitives that AI inference actually
bottlenecks on:

1. **Attention** — the core operation of transformer models; no native opcode
   exists; must be expressed as a sequence of TMATMUL + softmax + TMATMUL,
   which loses determinism guarantees across the composite.
2. **Quantized matrix multiply** — `T81W` weight format exists but there is no
   ISA-level opcode that natively multiplies a quantized weight tensor against
   an activation tensor; this forces a dequantize-then-multiply pattern that
   wastes memory bandwidth and prevents Axion from enforcing quantization
   policy at the instruction level.
3. **Weight loading** — `T81W` deserialization happens entirely in user space;
   the VM has no visibility into the weight load path, so Axion cannot enforce
   shape/precision/provenance policy on model weights at load time.
4. **Embedding lookup** — gather from a large embedding table is a fundamental
   AI primitive; the current LOAD opcode is address-based, not index-based
   into a handle-managed embedding store.
5. **Gather / Scatter** — sparse tensor operations required for mixture-of-experts
   and sparse attention; not expressible as dense TISC tensor ops without
   materializing the full dense tensor first.

Without these opcodes, T81 is a ternary VM that *can* run AI workloads, but
cannot claim to be *designed* for them. This RFC closes that gap.

---

## Proposal

### New Opcode Class: §5.15 AI-Native Inference Operations

All opcodes in this class operate on tensor handles (not raw register values).
All are Tier 2+ only. All are subject to Axion pre-instruction verification.

#### 5.15.1 ATTN — Scaled Dot-Product Attention

```
ATTN  RD, R_Q, R_K, R_V
```

- `R_Q`, `R_K`, `R_V`: TensorHandle registers for query, key, value
- `RD`: output TensorHandle
- Semantics: `RD := softmax(Q · Kᵀ / √dₖ) · V`
- All intermediate products use T81Float soft-float arithmetic (no hardware FPU)
- Axion MUST verify that Q, K, V share compatible head dimensions before execution
- Produces a deterministic `attn guard` AxionEvent with shape metadata

#### 5.15.2 QMATMUL — Quantized Matrix Multiply

```
QMATMUL  RD, R_ACT, R_WT, R_SCALE
```

- `R_ACT`: activation TensorHandle (T81Float or T81Uint)
- `R_WT`: weight TensorHandle (T81W format, quantized)
- `R_SCALE`: scalar register holding dequantization scale factor
- `RD`: output TensorHandle (T81Float)
- Semantics: `RD := dequantize(R_WT, R_SCALE) · R_ACT`
- Axion enforces that `R_WT` provenance matches the loaded model policy
- Shape mismatch raises a TypeFault; scale out of canonical range raises a CanonFault

#### 5.15.3 WLOAD — Weight Load with Policy Gate

```
WLOAD  RD, R_PATH, R_POLICY
```

- `R_PATH`: SymbolHandle identifying a CanonFS path to a `.t81w` file
- `R_POLICY`: AxionPolicyHandle (may be null, defaults to ambient policy)
- `RD`: T81W TensorHandle
- Axion MUST verify shape, precision, and provenance metadata before the weight
  data is accessible to subsequent instructions
- On policy denial: SecurityFault; weight handle is never materialized
- CanonFS emits `meta slot axion event segment=meta action=WeightLoad` for audit

#### 5.15.4 EMBED — Embedding Lookup

```
EMBED  RD, R_TABLE, R_IDX
```

- `R_TABLE`: TensorHandle for embedding table (shape: [vocab, dim])
- `R_IDX`: T81BigInt or Vector[T81BigInt] of token indices
- `RD`: TensorHandle of gathered rows (shape: [len(R_IDX), dim])
- Axion verifies index bounds before lookup; out-of-bounds raises BoundsFault
- Deterministic: given identical table and indices, output is bit-exact

#### 5.15.5 GATHER — Sparse Gather

```
GATHER  RD, R_SRC, R_IDX, R_AXIS
```

- Gathers slices from `R_SRC` along `R_AXIS` at positions `R_IDX`
- `R_AXIS`: immediate integer (0–rank-1)
- Equivalent to NumPy `take` semantics; deterministic ordering enforced

#### 5.15.6 SCATTER — Sparse Scatter-Add

```
SCATTER  RD, R_DST, R_IDX, R_SRC, R_AXIS
```

- Scatter-adds `R_SRC` values into `R_DST` at `R_IDX` positions along `R_AXIS`
- `RD` receives the updated tensor (R_DST is not mutated in-place)
- Non-determinism from concurrent scatter is prohibited: Axion MUST enforce
  that no two SCATTER ops alias the same destination indices in the same
  execution frame

---

## Determinism / Safety Considerations

**ATTN:** The scaled dot-product is fully deterministic when using T81Float
soft-float arithmetic. The `√dₖ` scaling factor MUST use the canonical T81Float
square-root algorithm defined in `spec/t81-data-types.md §4`. Hardware FPU
use is prohibited for this opcode.

**QMATMUL:** Dequantization order matters for determinism. The spec mandates
`dequantize-then-multiply` (not `multiply-then-dequantize`). The canonical
order is enforced by Axion through the `R_SCALE` policy gate.

**WLOAD:** Weight loading is the primary supply-chain attack surface for AI
systems. Axion's pre-instruction hook on WLOAD is the security gate for the
entire inference path. Weight provenance metadata (hash, shape, precision) MUST
be checked before the handle is materialized.

**EMBED/GATHER/SCATTER:** All index-based ops MUST perform bounds checking
before any memory access. No speculative execution of out-of-bounds indices.
SCATTER aliasing detection is required because undetected aliasing produces
nondeterministic accumulation order.

---

## Compatibility

These are new opcodes; no existing TISC programs are affected.

The opcode encoding range for this class is reserved from the existing
experimental opcode space. Concrete opcode byte assignments will be assigned
during integration, coordinated with `spec/tisc/opcode-registry.md`.

T81Lang lowering: the compiler MUST lower `@attention`, `@qmatmul`, and weight
load expressions to these opcodes when targeting Tier 2+. Tier 1 programs
attempting to use these opcodes receive a TierFault.

---

## Implementation Plan

| Milestone | Scope | Target |
| :--- | :--- | :--- |
| AI-M1 | TISC opcode encoding; VM dispatch stubs for ATTN, QMATMUL, WLOAD | 2026-04-01 |
| AI-M2 | Full ATTN semantics with T81Float soft-float path; Axion shape guard | 2026-04-15 |
| AI-M3 | QMATMUL with T81W dequantization; provenance policy gate | 2026-04-30 |
| AI-M4 | WLOAD CanonFS integration; `meta slot axion event ... action=WeightLoad` | 2026-05-15 |
| AI-M5 | EMBED, GATHER, SCATTER with bounds enforcement and aliasing detection | 2026-05-30 |
| AI-M6 | T81Lang lowering for `@attention` and `@qmatmul` annotations | 2026-06-15 |

---

## Open Questions

1. **ATTN masking:** Should ATTN accept an optional mask TensorHandle (for
   causal / padding masking), or should masking be expressed as a QMATMUL +
   ADD sequence?

2. **Multi-head ATTN:** Does ATTN operate on a single head, or should it
   accept a head-count immediate and execute all heads in one opcode? Single-head
   is simpler to specify deterministically; multi-head is more efficient.

3. **SCATTER atomicity:** In Tier 3+ concurrent execution, SCATTER aliasing
   detection may require a transactional memory model. This is deferred to a
   follow-on RFC.

4. **WLOAD streaming:** Large models may require streaming weight loads that
   exceed VM memory limits. A streaming variant (WLOAD_STREAM) is out of scope
   for this RFC.

---

## Acceptance Criteria

- `spec/tisc-spec.md §5.15` documents all six opcodes with normative encoding
  and semantics
- `spec/tisc/opcode-registry.md` assigns concrete byte values in the DCP range
- VM dispatches ATTN and QMATMUL with bit-exact results verified against
  reference vectors on x86-64 and ARM64
- Axion emits canonical `attn guard`, `qmatmul guard`, and
  `meta slot axion event ... action=WeightLoad` reason strings
- T81Lang compiler lowers `@attention` to ATTN in Tier 2+ programs
- All six opcodes present in `spec/tisc/opcode-unified-reference.md`
