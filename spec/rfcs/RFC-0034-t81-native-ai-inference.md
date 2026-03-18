# RFC-0034: T81-Native AI Inference

**Status:** proposed
**Type:** standards-track
**Applies-To:** `spec/tisc-spec.md` §5.17, `spec/t81-data-types.md` §11.9,
`spec/t81vm-spec.md`, `spec/t81lang-spec.md` §3–§4, `kernel/axion/`
**Created:** 2026-03-16
**Updated:** 2026-03-18
**Depends on:** RFC-0004 (Canonical Tensor Semantics), RFC-0017 (T81 Native),
RFC-0026 (AI-Native Inference Opcodes), RFC-0030 (Deterministic Math Subsystem),
RFC-0031 (Deterministic AI Execution Contract)
**Supersedes:** —
**Superseded-By:** —
**Discussion:** —

---

## 1. Summary

RFC-0026 introduced six AI inference opcodes (`ATTN`, `QMATMUL`, `WLOAD`,
`EMBED`, `GATHER`, `SCATTER`) that adapt conventional GPU-style float-quantized
inference to run deterministically on T81. Those opcodes are correct, accepted,
and load-bearing. They are not, however, *native* — they dequantize float
weights into T81Float for computation, relying on soft-float arithmetic for
correctness. The ternary substrate is invisible to them.

This RFC defines a parallel **T81-native inference path** in which model
weights, compute opcodes, and the underlying type system speak the same
language: balanced ternary `{−1, 0, +1}`. It introduces:

1. A **ternary-weight encoding** for T81W (`.t81w/ternary`) where each weight
   is a T81Qutrit stored as a 2-bit trit — no float representation exists in
   the model file.
2. A new **TISC opcode class §5.17** (`TWMATMUL`, `TQUANT`, `TATTN`,
   `TWEMBED`) that exploits ternary-weight arithmetic to eliminate floating-point
   multiply entirely from the hot path.
3. A **ternary accumulation model** (`TERNACCUM`) where the inner product
   reduces to integer add/subtract/skip over T81BigInt accumulators — no FPU
   rounding, no soft-float, exact integer result.
4. **Axion policy extensions** for ternary-weight provenance and shape
   contracts that are distinct from the float-quantized policy surface.
5. **T81Lang annotations** (`@ternary_inference`) enabling opt-in lowering
   to the new opcode class without modifying the float inference path.

RFC-0026 and this RFC coexist. Float-quantized and ternary-native inference are
both valid T81 paths; this RFC defines the preferred path for models
purpose-trained on the T81 substrate.

---

## 2. Motivation

### 2.1 The substrate mismatch

T81 is a balanced ternary machine. Its native scalar type is T81Qutrit
(`{−1, 0, +1}`). Its native integer is T81BigInt (balanced-ternary limbs). Its
ISA register file carries ternary-encoded values.

The inference opcodes in RFC-0026 operate on T81Float accumulators and
quantized-float weight tensors. The quantized weights are float-derived: they
started as IEEE 754 floats, were quantized to k-bit integers, and are
dequantized back to T81Float before computation. The ternary substrate
contributes nothing to that path.

This is a missed opportunity on two dimensions:

**Correctness.** Ternary-weight inner products over T81BigInt accumulators are
exact integer operations. There is no rounding, no accumulation error, no
soft-float bit-fiddling. The result is provably identical on every platform
with zero additional verification cost.

**Efficiency.** A ternary-weight matrix multiply is not a multiply. For each
weight `w ∈ {−1, 0, +1}` and activation `a`:

```text
w =  1  →  acc += a
w =  0  →  acc += 0  (skip)
w = −1  →  acc -= a
```

No multiplication instruction executes. On a ternary machine that represents
`w` as a 2-bit trit, this is the natural computation — not an optimization.

### 2.2 Ternary-weight networks are proven at scale

Ternary Weight Networks (Zhu et al., 2016) and the more recent 1-bit/1.58-bit
LLM results (Microsoft Research, 2024) demonstrate that models trained with
ternary weights can match full-precision quality at scale. The key finding is
that weights need more precision than activations, but `{−1, 0, +1}` is
sufficient for weights when training accounts for it.

T81 is in a unique position: the substrate is already ternary. A model trained
for T81 does not perform approximate ternary quantization of float weights — it
trains directly in the ternary weight domain, eliminating the float-to-ternary
approximation gap.

### 2.3 The audit and policy gap

RFC-0026 `WLOAD` verifies model provenance via hash (RFC-0025) and emits a
`WeightLoad` Axion event. But for float-quantized weights, Axion cannot enforce
*ternary* shape contracts — it does not know whether a weight tensor is
genuinely ternary or merely quantized-to-look-ternary. The distinction matters
for governance: a ternary-native model has a provably smaller representational
surface and is easier to audit.

This RFC introduces a ternary-weight policy surface that Axion can enforce at
load time with a deterministic domain check, not just a hash check.

### 2.4 Relationship to RFC-0026

This RFC does not modify, deprecate, or supersede RFC-0026. The two inference
paths serve different model classes:

| Property | RFC-0026 (float-quantized) | RFC-0034 (ternary-native) |
| :--- | :--- | :--- |
| Weight representation | quantized float → T81Float | T81Qutrit `{−1, 0, +1}` |
| Inner product | soft-float multiply-add | integer add/subtract/skip |
| Accumulator type | T81Float | T81BigInt |
| Rounding | yes (canonical mode) | none |
| Axion weight check | hash + float-shape | hash + trit-domain assertion |
| Training requirement | any float model | ternary-weight training |
| T81Lang annotation | (implicit; Tier 2+) | `@ternary_inference` |

---

## 3. Proposal

### 3.1 T81W Ternary Encoding (`.t81w/ternary`)

A `.t81w` file with encoding type `ternary` stores each weight as a 2-bit
balanced-trit encoding:

```text
00  →  0
01  →  +1
10  →  −1
11  →  (reserved; loading must raise a CanonFault)
```

Weights are packed 4 per byte, row-major, with a header block that includes:

- `magic`: `T81WTN` (6 bytes, distinguishes from float-quantized `T81WFQ`)
- `shape`: rank and dimension array (same as RFC-0004 shape encoding)
- `canon_hash`: CanonHash81 of the packed trit payload
- `scale_absent`: boolean flag; MUST be `true` for ternary-native encoding
  (the absence of a scale field is load-time verifiable)
- `axion_policy_slot`: optional reference to an Axion policy handle

This encoding is a distinct subtype of T81W. `WLOAD` with a `ternary` header
routes to the ternary policy gate. Passing a `ternary` handle to `QMATMUL`
(which expects float-quantized weights) MUST raise a `TypeFault`.

### 3.2 New TISC Opcode Class: §5.17 Ternary-Native Inference Operations

All opcodes in this class:

- operate on `T81Qutrit` weight handles and `T81Float` or `T81BigInt`
  activation handles
- are Tier 2+ only
- are subject to Axion pre-instruction shape and domain verification
- are lowered by the T81Lang compiler when `@ternary_inference` is present

#### 5.17.1 TWMATMUL — Ternary-Weight Matrix Multiply

```text
TWMATMUL  RD, R_ACT, R_WT
```

- `R_WT`: TernaryWeightHandle (T81Qutrit tensor, rank ≥ 2)
- `R_ACT`: TensorHandle of T81Float or T81BigInt activations
- `RD`: TensorHandle of T81BigInt accumulators

Semantics:

```text
for each output position (i, j):
  acc = 0
  for k:
    w = R_WT[i, k]      // w ∈ {−1, 0, +1}
    a = R_ACT[k, j]
    if   w ==  1: acc += a
    elif w == −1: acc -= a
    // w == 0: skip (no operation)
  RD[i, j] = acc
```

The accumulator is T81BigInt with deterministic overflow defined by RFC-0030
§4. No floating-point multiply executes. Axion MUST verify that `R_WT` is a
`.t81w/ternary` handle before execution; passing a float-quantized handle
raises a `TypeFault`.

Axion emits: `twmatmul guard shape=<shape> trit_density=<nonzero_frac>` —
the `trit_density` field records the fraction of non-zero weights and is
recorded for governance but does not affect execution.

#### 5.17.2 TQUANT — Quantize to Ternary

```text
TQUANT  RD, R_SRC, R_THR
```

- `R_SRC`: TensorHandle of T81Float activations
- `R_THR`: scalar T81Float threshold (must satisfy `R_THR > 0`)
- `RD`: T81Qutrit TensorHandle

Semantics:

```text
for each element x:
  if   x >  R_THR: RD[...] = +1
  elif x < −R_THR: RD[...] = −1
  else:             RD[...] =  0
```

Used at inference boundaries to quantize float activations before passing
them to a ternary-weight layer. The threshold `R_THR` is an explicit parameter
rather than a learned scale; it must be provided by the caller.

`R_THR ≤ 0` raises a `CanonFault`. The result is deterministic: identical
inputs with identical threshold produce bit-exact identical trit outputs.

Axion emits: `tquant guard threshold=<thr> sparsity=<zero_frac>` — the
sparsity field enables governance rules that quarantine anomalously sparse or
dense activations.

#### 5.17.3 TATTN — Ternary-Key/Query Attention

```text
TATTN  RD, R_Q, R_K, R_V
```

- `R_Q`, `R_K`: T81Qutrit TensorHandles (ternary-quantized query and key
  projections)
- `R_V`: T81Float TensorHandle (float value projection; not ternarized)
- `RD`: T81Float TensorHandle (attention output)

Semantics:

```text
scores = TWMATMUL(R_Q, R_K^T)     // T81BigInt scores; no multiply
scaled = scores / sqrt(d_k)       // soft-float scaling (RFC-0030)
weights = softmax(scaled)          // T81Float softmax
RD = weights · R_V                 // T81Float matmul for value projection
```

The key insight: the dominant cost of attention (the QKᵀ inner product) is
ternary-weight multiply-free. Only the value projection uses float. Axion
verifies that Q and K are genuine T81Qutrit handles; passing float tensors
raises a `TypeFault` rather than silently dequantizing.

This is distinct from RFC-0026 `ATTN`, which operates on float Q, K, V
throughout. `TATTN` and `ATTN` MUST NOT be substituted for each other.

Axion emits: `tattn guard q_shape=<shape> k_shape=<shape> v_shape=<shape>`

#### 5.17.4 TWEMBED — Ternary Embedding Lookup

```text
TWEMBED  RD, R_TABLE, R_IDX
```

- `R_TABLE`: TernaryWeightHandle (T81Qutrit embedding table, shape [vocab, dim])
- `R_IDX`: T81BigInt or Vector[T81BigInt] of token indices
- `RD`: T81Qutrit TensorHandle of gathered rows

Semantics: row gather from a ternary embedding table. The result is T81Qutrit
(not dequantized to float), allowing downstream TWMATMUL to operate on ternary
inputs without a dequantize round-trip.

Bounds check behavior is identical to RFC-0026 `EMBED`: out-of-range index
raises a `BoundsFault`. Axion emits: `twembed guard vocab=<n> dim=<d>`

#### 5.17.5 TERNACCUM — Ternary Reduction to Scalar

```text
TERNACCUM  RD, R_WT, R_ACT
```

A scalar variant of `TWMATMUL` for 1D dot-product reduction. `R_WT` and
`R_ACT` must be rank-1 T81Qutrit tensors of equal length. `RD` receives a
single T81BigInt result.

Intended for attention score computation in single-head, sequence-by-sequence
inference passes where constructing a full matrix is unnecessary.

Axion emits: `ternaccum guard length=<n>`

#### 5.17.6 TACT — Ternary Activation Function

```text
TACT  RD, R_SRC, R_MODE
```

- `R_SRC`: TensorHandle of T81Float activations (the post-accumulation,
  pre-nonlinearity tensor; typically the output of a normalization step applied
  to a `TWMATMUL` result)
- `R_MODE`: immediate byte selecting the activation mode
- `RD`: T81Qutrit TensorHandle

Defined modes:

| `R_MODE` | Name | Semantics |
| :--- | :--- | :--- |
| `0x01` | `TernaryStep` | `x > 0.5 → +1`; `x < −0.5 → −1`; otherwise `0` |
| `0x02` | `TanhQuantized` | `tanh(x) > 0.5 → +1`; `tanh(x) < −0.5 → −1`; otherwise `0` |

All threshold comparisons use T81Float comparison semantics (RFC-0030 §5).
The `tanh` function in `TanhQuantized` mode uses the `t81_soft_math` integer-backed
implementation (RFC-0030 §3); the result is bit-exact across platforms.
Undefined mode bytes raise a `CanonFault`.

**Relationship to `TQUANT`:** `TQUANT` is a general-purpose quantization
primitive with a caller-supplied threshold, intended for inference boundaries
between layers. `TACT` is a named activation family with canonical fixed
thresholds and built-in governance monitoring. `TernaryStep` mode is
semantically equivalent to `TQUANT` with `R_THR = 0.5`, but the two opcodes
serve different purposes: `TQUANT` quantizes arbitrary float data; `TACT`
applies a layer activation and triggers the Axion activation policy gate.

**Axion post-execute policy gate.** After `TACT` executes and `RD` is
populated, Axion evaluates the `activation-ceiling` policy directive (§3.3)
against the output trit distribution. The gate uses the same verdict model as
RFC-00B5 interrupt policy:

- `Allow` — execution continues normally; Axion emits `tact guard` event
- `Quarantine` — output distribution exceeded the policy ceiling; Axion emits
  `tact policy quarantine` event and suspends the thread pending supervisor
  acknowledgement; `RD` is NOT committed to the register file
- `Deny` — the thread is already quarantined for a prior activation violation;
  execution raises an `ActivationFault`

This is a post-execute gate on the output, not a trap before PC increment.
The PC advances only after the gate returns `Allow`. On `Quarantine` or `Deny`,
the PC does not advance, matching RFC-0003's fail-closed policy model.

Axion emits: `tact guard mode=<mode_name> sparsity=<zero_frac> verdict=<verdict>`
— emitted on every execution; `verdict` is `allow`, `quarantine`, or `deny`.

---

### 3.3 Axion Policy Extensions

Two new policy directives govern the ternary-native inference path:

**`allowed-ternary-model-hashes`** (list of CanonHash81 values)

Distinct from RFC-0025 `allowed-tensor-hashes`. Applies only to `.t81w/ternary`
loads. Separating the two lists allows an Axion policy to permit float-quantized
models but deny ternary-native ones (or vice versa) independently.

**`ternary-weight-domain-check`** (boolean, default `true`)

When `true`, Axion MUST verify at WLOAD time that every trit in the weight
payload is in `{−1, 0, +1}` and that no `11` (reserved) encoding is present.
This check is O(n) in weight count; operators may disable it for trusted
on-device models by setting this to `false`. The directive name records the
decision in the audit log regardless.

**`activation-ceiling`** (struct, optional; governs `TACT` post-execute gate)

When present, defines the policy ceiling applied after every `TACT` execution.
Fields:

```yaml
activation-ceiling:
  max-nonzero-fraction: <T81Float 0..1>   # fraction of non-zero trits in RD above which Quarantine fires
  mode-mask: [TernaryStep, TanhQuantized] # which modes are gated (default: all)
  scope: thread | process-group           # quarantine granularity (default: thread)
```

If `max-nonzero-fraction` is absent or zero, the gate is disabled and every
`TACT` execution returns `Allow`. The `scope` field controls whether a
`Quarantine` verdict suspends only the faulting thread or the entire
process-group (matching RFC-0003 §6.2 quarantine semantics).

This directive is the only RFC-0034 policy that operates *after* an opcode
executes rather than before it. The asymmetry is intentional: weight provenance
must be checked before a weight handle materializes; activation distribution
can only be evaluated after the activation is computed.

---

### 3.4 T81Lang: `@ternary_inference` Annotation

A new function annotation that instructs the compiler to lower
AI-related built-ins to the ternary-native opcode class instead of the
float-quantized class:

```t81lang
@ternary_inference
@tier(2)
fn inference_forward(weights: T81Qutrit[], input: T81Float[]) -> T81BigInt[] {
  let q_input = tquant(input, threshold = 0.5);
  return twmatmul(weights, q_input);
}
```

Lowering rules:

| Built-in call | Without `@ternary_inference` | With `@ternary_inference` |
| :--- | :--- | :--- |
| `matmul(w, a)` where `w: T81Qutrit[]` | `TypeFault` (wrong type) | `TWMATMUL` |
| `attention(q, k, v)` | `ATTN` | `TATTN` (if q, k are T81Qutrit) |
| `embed_lookup(table, idx)` | `EMBED` | `TWEMBED` (if table is T81Qutrit) |
| `quantize(src, thr)` | compile error | `TQUANT` |
| `activate(src, mode)` | compile error | `TACT` (Tier 2+ only) |

Using `@ternary_inference` on a function that calls float-only operations is
a compile-time warning, not an error. The annotation does not affect functions
that have no compatible ternary lowering.

---

## 4. Determinism / Safety Considerations

### 4.1 Exact integer arithmetic

`TWMATMUL` and `TERNACCUM` produce T81BigInt results. T81BigInt arithmetic is
defined as exact over the full integer domain (RFC-0017 §3). There are no
rounding modes, no overflow (BigInt is arbitrary precision), and no
platform-dependent behavior. The only determinism obligation is correct
balanced-trit encoding of inputs — which the ternary-weight domain check
(§3.3) enforces at load time.

This is a stronger determinism guarantee than RFC-0026 float-quantized
inference, which relies on RFC-0030 soft-float semantics to achieve
cross-platform bit-exactness.

### 4.2 TQUANT determinism

The threshold comparison `x > R_THR` / `x < −R_THR` uses T81Float comparison
semantics (RFC-0030 §5). The result is a trit — no floating-point arithmetic
is involved in the assignment. Given identical inputs and threshold, the trit
output is bit-exact. The sparsity field in the Axion event is computed after
execution and does not feed back into the computation.

### 4.3 TATTN soft-float surface

The value projection `weights · R_V` in `TATTN` uses T81Float multiplication
(RFC-0030). This is the only remaining soft-float surface in the ternary-native
path and inherits RFC-0030's determinism guarantees. A future RFC may introduce
a fully-ternary value projection for models that ternarize V as well; that
work is deferred.

### 4.4 Reserved trit encoding

The `11` encoding in `.t81w/ternary` is permanently reserved. A ternary model
file containing `11` bytes in the weight payload MUST be rejected at WLOAD with
a `CanonFault`. This prevents adversarially crafted model files from encoding
a fourth value into what appears to be a ternary weight stream.

### 4.5 TACT determinism

`TernaryStep` — the threshold comparisons (`x > 0.5`, `x < −0.5`) use T81Float
comparison semantics (RFC-0030 §5). Given identical input and mode, the trit
assignment is deterministic with no rounding involved in the assignment itself.

`TanhQuantized` — the `tanh` computation uses `t81_soft_math` (RFC-0030 §3),
which is integer-backed and produces bit-exact results across platforms. The
subsequent threshold comparison is identical to `TernaryStep`. The full chain
is deterministic.

The Axion post-execute gate evaluates `sparsity` (fraction of zero trits in
`RD`) after `RD` is populated. The sparsity calculation is a trit-count
operation over T81Qutrit data — exact integer arithmetic, no soft-float. The
policy verdict does not feed back into `RD`; it only controls whether `RD` is
committed and whether the PC advances.

### 4.6 No SCATTER equivalent

The ternary-native opcode class does not include a ternary scatter operation.
Sparse scatter over ternary weights has non-trivial aliasing semantics that are
not resolved in this RFC. Programs requiring scatter on ternary tensors should
use RFC-0026 `SCATTER` after dequantizing — or wait for a follow-on RFC.

---

## 5. Compatibility

### 5.1 No impact on RFC-0026

All six RFC-0026 opcodes are unmodified. The ternary-native opcodes occupy a
new §5.17 section in `spec/tisc-spec.md`. Opcode byte assignments for §5.17
MUST be reserved in `spec/tisc/opcode-registry.md` without conflicting with
§5.15 (RFC-0026 opcodes).

### 5.2 T81W format versioning

`.t81w/ternary` files are distinguished by the `T81WTN` magic prefix. Existing
`.t81w` loaders that do not recognize `T81WTN` MUST reject the file with a
`FormatFault` rather than silently misinterpreting it. This is enforced via
the magic check, which is the first operation in `WLOAD` dispatch.

### 5.3 T81Lang backward compatibility

The `@ternary_inference` annotation is strictly additive. Existing T81Lang
programs not using it are unaffected. Programs that explicitly annotate
`@ternary_inference` on functions calling float-quantized builtins produce
warnings, not errors, so annotation can be added incrementally.

### 5.4 Tier constraints

All six new opcodes (`TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`,
`TACT`) require Tier 2 or above, matching RFC-0026. Tier 1 programs attempting
to execute them receive a `TierFault`. This is consistent with the general rule
that inference is a supervised-tier operation. `TACT` additionally requires
that an Axion policy context is active; executing `TACT` with no ambient policy
handle returns `Allow` by default (the gate is open, not absent).

---

## 6. Implementation Plan

| Milestone | Scope | Notes |
| :--- | :--- | :--- |
| TN-M1 | `.t81w/ternary` format definition; `T81WTN` magic; header parser; domain check | Blocks all downstream milestones |
| TN-M2 | `TWMATMUL` — opcode encoding, VM dispatch, BigInt accumulator path | Reference implementation; test vectors on x86-64 and ARM64 |
| TN-M3 | `TQUANT` — threshold comparison, T81Qutrit output, Axion sparsity event | Relatively isolated; can parallel with TN-M2 |
| TN-M4 | `TERNACCUM` — scalar dot-product reduction; spec-as-executable conformance program | Simpler variant of TWMATMUL |
| TN-M5 | `TWEMBED` — row-gather from T81Qutrit table; bounds check; Axion event | After TWMATMUL stable |
| TN-M6 | `TATTN` — ternary QKᵀ via TWMATMUL + soft-float value projection | Requires TN-M2 and TN-M5 |
| TN-M7 | Axion policy extensions: `allowed-ternary-model-hashes`, `ternary-weight-domain-check` | Requires TN-M1 |
| TN-M8 | T81Lang `@ternary_inference` annotation; lowering table; compiler tests | Requires TN-M2/M3/M5/M6 |
| TN-M9 | Spec-as-executable conformance suite (`twmatmul-exact.t81`, `tquant-determinism.t81`, etc.) | Requires TN-M2–M6 |
| TN-M10 | `TACT` — `TernaryStep` and `TanhQuantized` modes; `activation-ceiling` Axion policy gate; `tact guard` event | Requires TN-M3 (TQUANT infrastructure); soft-float tanh from RFC-0030 |

Milestones do not carry fixed calendar targets at draft stage; targets will be
assigned at the `proposed` transition after dependency review.

## 6.1 Implementation Status and Remaining Work

Implementation status on 2026-03-18:

- All in-repo RFC-0034 implementation items are closed: opcode/runtime
  dispatch, Axion policy integration, `@ternary_inference` lowering, authored
  conformance programs, CanonFS-backed `.t81w` execution, and native model
  import/conversion support needed to feed the path.
- The native ternary execution path is now measured end to end on the local
  ARM64 host. Evidence is recorded in:
  - `docs/records/status-history/RFC_0034_TERNARY_INFERENCE_EVIDENCE_2026-03-18.md`
  - `docs/records/status-history/NATIVE_WEIGHTS_EXECUTION_EVIDENCE_2026-03-18.md`
- Current measured native-only smoke evidence:
  - `BM_NativeWeightsLoad_T81Native/64`: `372.09 Kops/s`, `172.67 µs`
  - `BM_NativeWeightsPromote_T81Native/64`: `37.42 ops/s`, `1.81 s`
  - `BM_NativeWeightsLoadAndExp_T81Native/64`: first `15.59 ops/s`, `4.24 s`;
    after direct `TExp` fast path plus host-float tensor construction:
    `696.30 ops/s`, `115.54 ms`
  - `BM_NativeWeightsLoadAndSiLU_T81Native/64`: `784.42 ops/s`, `93.37 ms`
  - `BM_NativeWeightsLoadAndSoftmax_T81Native/64`: `711.95 ops/s`, `92.08 ms`
  - `BM_NativeWeightsLoadAndRMSNorm_T81Native/64`: `226.04 ops/s`, `311.71 ms`
  - `BM_NativeWeightsLoadAndRoPE_T81Native/64`: after RoPE coefficient caching:
    `81.34 ops/s`, `836.23 ms`
  - First matched VM native-vs-binary comparison at `64`:
    - `BM_NativeWeightsLoadAndExp`: `62.43x` throughput, `62.60x` latency
    - `BM_NativeWeightsLoadAndSiLU`: `67.12x` throughput, `68.36x` latency
    - `BM_NativeWeightsLoadAndSoftmax`: `63.34x` throughput, `64.07x` latency
    - `BM_NativeWeightsLoadAndRMSNorm`: `11.99x` throughput, `11.65x` latency
    - `BM_NativeWeightsLoadAndRoPE`: `4.07x` throughput, `4.20x` latency
- Current same-build substrate comparison evidence in `build-llama` (`Release`,
  `T81_ENABLE_LLAMA_CPP=ON`):
  - `BM_LlamaGgufDequantize_Binary/8192`: `118.72 Mops/s`
  - `BM_LlamaGgufDequantizeAndExp_Binary/8192`: `409.60 Mops/s`
  - `BM_NativeWeightsDecode_T81Native/8192`: `41.58 Mops/s`, `196.79 µs`
  - `BM_NativeWeightsDecodeAndExp_T81Native/8192`: `90.22 Kops/s`, `228.99 ms`
  - `BM_NativeWeightsPackedExp_T81Native/8192`: about `85-97 Kops/s`,
    `100-195 ms`
  - `BM_NativeWeightsPackedExpRawFloat_T81Native/8192`: `372.36 Mops/s`,
    `23.13 µs`
  - `BM_LlamaGgufDequantizeAndExp_Binary/65536`: `348.60 Mops/s`, `188.33 µs`
  - `BM_NativeWeightsDecodeAndExp_T81Native/65536`: `643.54 Kops/s`,
    `209.14 ms`
  - `BM_NativeWeightsPackedExp_T81Native/65536`: `671.21 Kops/s`, `243.58 ms`
  - `BM_NativeWeightsPackedExpRawFloat_T81Native/65536`: `407.06 Mops/s`,
    `160.37 µs`

Interpretation:

- The native path is real, instrumented, and measurable. The remaining work is
  no longer import plumbing or missing opcode coverage.
- The immediate execution bottleneck is not plain native weight loading. The
  measured cost first concentrated in tensor materialization from the packed
  native representation, with deterministic exponentiation adding further cost
  on top. After switching canonical-fixed tensor construction to lazy host-cache
  materialization, packed native decode is now measurable in the same release
  harness as the llama.cpp baseline. The dominant remaining gap is now the
  deterministic exponentiation path over promoted canonical-fixed tensors.
  A direct packed-native benchmark path that still returns a canonical-fixed
  tensor does not materially improve throughput over `decode()` + `ops::exp()`,
  while a direct packed-native path that writes only a raw float buffer is fast
  enough to match or beat the current llama.cpp substrate baseline. That means
  the remaining hot cost is not trit classification but the current tensor
  result representation. Narrow VM fast paths for native unary ops on
  `BalancedTernary` weights, combined with host-float tensor construction that
  avoids eager canonical-cache synthesis, materially improved real interpreter
  paths including `WeightsLoad + TExp` and `WeightsLoad + TSiLU`. Even so, the
  broader benchmark evidence, now including `WeightsLoad + TSoftmax`,
  `WeightsLoad + TRMSNorm`, and `WeightsLoad + TRoPE`, still says result
  representation remains the next main optimization target.
  At the same time, the first matched VM native-vs-binary comparison now shows
  a real execution advantage rather than only a substrate advantage: these
  native unary loops are currently about `62x-67x` faster than the equivalent
  binary host-float interpreter loops at `64` elements, and the first
  higher-level `TRMSNorm` loop is about `12x` faster on that same VM-path
  comparison. `TRoPE` is now also materially positive after coefficient caching
  (`4.07x`), which narrows the set of obvious higher-level laggards but still
  leaves it well behind the unary set and `TRMSNorm`.
- Promotion work is therefore split into two tracks:
  1. evidence refresh on both reference platforms for the RFC status transition
  2. targeted optimization of the post-decode `TExp`-dominated native
     execution path, most likely by broadening the new host-float/native fast
     path approach across the remaining native unary surface or otherwise
     avoiding the current canonical-fixed tensor result path for common native
     workloads

---

## 7. Open Questions

**Q1 — Ternary activation path (full ternary inference)**
This RFC keeps activations as T81Float between layers (only weights are
ternary). A fully ternary inference path — ternary activations too — would
eliminate the remaining T81Float surface in `TATTN`. Is there a model training
regime for T81 that justifies full trit activations? Deferred to follow-on RFC.

**Q2 — TWMATMUL output type** *(resolved 2026-03-16)*
**Decision: pure T81BigInt is the normative accumulator; bounded fast path
deferred to TN-M2.**
The T81BigInt accumulator is the normative path. A `T81Int32` fast path for
dimensions d ≤ 729 (3⁶) may be introduced as an optimization at implementation
milestone TN-M2, provided it can be proven to produce bit-identical results to
the BigInt path for all inputs in the bounded range. Until that proof is in the
conformance suite, implementations MUST use the BigInt path. The fast path, if
added, requires a dimension-check guard and a fallback — not a separate opcode.

**Q3 — TQUANT threshold source**
The threshold `R_THR` is an explicit parameter. Some ternary quantization
schemes use a per-tensor learned threshold stored in the model file. Should
`.t81w/ternary` include an optional threshold field, or should all thresholds
come from the calling program? The current proposal requires caller-supplied
thresholds to keep the weight format pure.

**Q4 — Ternary KV cache**
A compressed KV cache where keys/queries are stored as ternary tensors would
reduce memory bandwidth during autoregressive inference. This is a runtime
memory management question more than an ISA question; it is deferred to the
Axion OS memory subsystem track.

**Q5 — Normalization layers**
Ternary-weight inference typically pairs with RMS normalization (not LayerNorm)
because RMS norm does not require a learned scale parameter per weight.
Should a `TWRMSNORM` opcode be part of this RFC or a separate follow-on?

**Q7 — Post-load tensor materialization cost**
Native weights loading is now benchmarked end to end and the current local
evidence first showed a large gap between plain `WeightsLoad`, promoted tensor
materialization, and `WeightsLoad + TExp`. The latest local evidence now shows
that lazy canonical-fixed construction materially reduced the decode side of
that gap, while deterministic exponentiation remains the dominant outlier. A
direct packed-native benchmark path that still returns a canonical-fixed tensor
does not materially improve throughput, while a raw-float result path does.
Should RFC-0034 continue to rely on promotion into generic tensor storage for
native model execution, or should a follow-on optimization effort introduce a
lighter-weight native result representation for common unary/native workloads?

**Q6 — TACT mode extensibility** *(resolved 2026-03-16)*
**Decision: mode bytes registered in `spec/tisc/opcode-registry.md` alongside
opcode bytes; no separate file.**
The `R_MODE` byte space is an extension point of the `TACT` opcode, not a
separate opcode family. It belongs in the same registry file as the opcode
byte assignments, under a `TACT Modes` subsection. A separate
`tact-mode-registry.md` would create split-ownership without benefit. Future
modes (`0x03`, `0x04`, …) require an RFC or RFC amendment; no mode byte may be
assigned without a normative semantic definition.

---

## 8. Acceptance Criteria

This RFC can move from `draft` to `proposed` when: *(met 2026-03-16)*

- ✅ The `.t81w/ternary` format header and domain check are specified normatively
  in `spec/t81-data-types.md §11.9`
- ✅ `spec/tisc-spec.md §5.17` stubs all six opcodes with encoding and semantics
- ✅ Q2 and Q6 resolved with documented decisions

This RFC can move from `proposed` to `accepted` when:

- ✅ All six opcodes are implemented in `core/vm/vm.cpp` and dispatch correctly
- `TWMATMUL` produces bit-exact results verified against reference vectors on
  x86-64 and ARM64
- Axion emits the correct guard events for `TWMATMUL`, `TQUANT`, `TATTN`,
  `TWEMBED`, and `TACT` (including `verdict=` field for all three outcomes)
- ✅ `allowed-ternary-model-hashes`, `ternary-weight-domain-check`, and
  `activation-ceiling` are implemented and tested in the interpreter/runtime
  path (`include/t81/axion/policy.hpp`, `core/vm/vm.cpp`,
  `tests/cpp/vm_rfc0034_ternary_native_test.cpp`)
- ✅ `TACT` `Quarantine` verdict: `RD` is not committed and PC does not advance;
  verified by test
- ✅ `TACT` `Deny` verdict: `ActivationFault` raised; verified by test
- ✅ T81Lang compiler supports `@ternary_inference` on Tier 2+ functions and
  lowers compatible AI call surfaces to §5.17 opcodes; current implementation
  covers compatible `std.tensor.matmul` → `TWMATMUL` and
  `std.tensor.attention` → `TATTN`, while the explicit `std.tnn.*` surface
  remains available for the full ternary opcode family
- ✅ Spec-as-executable conformance programs are authored under
  `spec/conformance/ai/`: `twmatmul-exact.t81`, `tquant-determinism.t81`,
  `tattn-ternary-qk.t81`, `twembed-bounds.t81`, `tact-step-determinism.t81`,
  `tact-quarantine-gate.t81`
- The conformance suite passes in CI on both reference platforms
- No determinism regressions in the existing 363/363 core tests

This RFC can move from `accepted` to `integrated` when:

- ✅ At least one end-to-end ternary-native inference fixture runs against a
  `.t81w/ternary` model file stored in CanonFS; current evidence is the
  CanonFS-backed `--weights-model sha3-256:...` path exercised by
  `tests/cpp/cli_contract_test.cpp` against
  `tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81`
- ✅ The `ternary-weight-domain-check` policy path is exercised in the
  integration test suite with both `true` and `false` directive values;
  covered by `tests/cpp/vm_rfc0034_ternary_native_test.cpp`

Remaining work for status promotion on 2026-03-18:

- The `proposed` → `accepted` transition still depends on refreshed
  cross-platform CI evidence for the reference-platform criteria above.
- Independent of status promotion, local runtime evidence now identifies
  the post-decode deterministic exponentiation path as the next optimization
  target for materially stronger native-execution claims. Current benchmark-only
  evidence strongly suggests that changing result representation will matter
  more than merely bypassing decode, because raw packed-native exponentiation
  into a plain float buffer is already competitive with the llama.cpp substrate
  baseline while canonical-fixed tensor result paths are not. A first set of VM
  fast paths in this direction is now implemented for native balanced-trit
  `TExp`, `TSiLU`, `TSoftmax`, `TRMSNorm`, and `TRoPE`; the measured
  interpreter-path gains are already visible for both the unary set and the
  first higher-level kernels. `TRoPE` still trails the unary set and
  `TRMSNorm`, but coefficient caching moved it out of the merely-marginal range.
