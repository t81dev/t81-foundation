# RFC-00BB: Native Model Architecture Compatibility

**Status:** proposed
**Type:** standards-track
**Applies-To:** tooling model conversion, native `.t81w` artifact contract, RFC-0034 execution path
**Created:** 2026-03-18
**Author:** @t81dev
**Depends on:** RFC-0034 (T81-Native AI Inference), RFC-00BA (llama.cpp GGUF Ingestion Bridge), RFC-0025 (Policy-Gated Tensor Loading via CanonFS)
**Blocks:** broad native conversion support for llama.cpp-compatible model families

---

## 1. Summary

This RFC defines how T81 should decide which llama.cpp-compatible model families can be converted into native `.t81w` artifacts for RFC-0034 execution.

The bridge RFC (RFC-00BA) answers how T81 reads GGUF models without reimplementing llama.cpp loaders. This RFC answers a different question: once T81 can read a model, what does it mean to support that model natively?

Native support in this RFC means:

- T81 can identify the model architecture
- T81 can map the required tensors into a T81-native representation
- T81 can reject unsupported features deterministically
- T81 can emit `.t81w` artifacts with clear provenance and compatibility metadata

For native-ternary SafeTensors sources, the same rule applies: T81 must identify a supported ternary profile or fail closed instead of treating arbitrary SafeTensors payloads as natively executable.

This RFC does not require that every model family supported by llama.cpp is immediately executable through RFC-0034 native inference. It defines the compatibility framework that allows T81 to grow toward that goal without making vague or misleading support claims.

## 2. Motivation

`llama.cpp` supports a broad and evolving set of model families. Those families differ materially:

- tensor naming conventions
- attention variants
- rotary embedding schemes
- feed-forward block structure
- MoE routing
- multimodal projector paths
- tokenizer and vocabulary metadata

Reading those models through llama.cpp is not enough to claim native support inside T81.

T81 needs a formal compatibility policy so that:

- supported families are explicit
- unsupported features fail cleanly
- conversion behavior is deterministic
- future llama.cpp updates do not silently widen or narrow native support

## 3. Goals

- Define the meaning of native model-family support in T81
- Separate ingestion compatibility from native execution compatibility
- Establish deterministic architecture detection and feature gating
- Require explicit per-architecture mapping rules
- Provide a path to incremental expansion across model families

## 4. Non-Goals

- Immediate support for every llama.cpp-compatible architecture
- Replacing llama.cpp as the authoritative model-family parser
- Standardizing tokenizer/runtime chat behavior in this RFC
- Defining multimodal execution semantics in the first compatibility pass

## 5. Proposal

### 5.1 Two Levels of Support

T81 distinguishes two kinds of compatibility:

#### 5.1.1 Ingestion Compatibility

A model is ingestion-compatible if T81 can:

- open it through RFC-00BA's bridge
- enumerate tensors and metadata
- inspect architecture identifiers

This is a bridge-level claim only. It does not imply native `.t81w` conversion or RFC-0034 execution.

#### 5.1.2 Native Compatibility

A model is native-compatible if T81 can:

- detect the architecture deterministically
- map all required tensors and metadata into a supported native representation
- reject or quarantine unsupported features explicitly
- produce a `.t81w` artifact with enough metadata for RFC-0034 execution

### 5.2 Compatibility Matrix

Each architecture family must be classified in one of these states:

- `unsupported`
- `ingestion_only`
- `experimental_native`
- `native_supported`

The compatibility matrix must be documented and versioned in-repo.

Current in-repo matrix as of 2026-03-18:

| Architecture | State | Profile | Notes |
| :--- | :--- | :--- | :--- |
| `bitnet` via SafeTensors | `experimental_native` | `bitnet-b1.58-v1` | Native ternary `I8` SafeTensors import path; explicit `bitnet` CLI format and metadata-based BitNet detection are supported |
| dense SafeTensors (`F16`/`BF16`/`F32`) | `experimental_native` | `native-dense-v1` fallback | Generic SafeTensors import can quantize dense float tensors into native balanced ternary `.t81w`; known architectures reuse explicit dense profiles only when metadata and tensor-name structure agree, otherwise import fails closed or falls back to `native-dense-v1` |
| `gemma` | `experimental_native` | `gemma-dense-v1` | Dense decoder-only profile admitted in code and covered by fixture tests; real repo-model smoke evidence is still pending because no gemma GGUF is checked in |
| `llama` | `experimental_native` | `llama-dense-v1` | First promoted dense decoder-only profile; bridge-backed GGUF import converts to native balanced ternary `.t81w`; verified on synthetic fixture and real TinyLlama import path |
| `mistral` | `experimental_native` | `mistral-dense-v1` | Dense decoder-only profile admitted in code and covered by fixture tests; real repo-model smoke evidence is still pending because no mistral GGUF is checked in |
| `phi3` | `experimental_native` | `phi3-dense-v1` | Second dense decoder-only profile; bridge-backed GGUF import is accepted in code and covered by fixture tests; real large-model smoke evidence still pending |
| `qwen2` | `experimental_native` | `qwen2-dense-v1` | Third dense decoder-only profile; bridge-backed GGUF import is accepted in code and covered by fixture tests; real repo-model smoke evidence is still pending |
| other llama.cpp architectures | `ingestion_only` or `unsupported` | — | Architecture may be readable through the bridge, but no native compatibility claim is made until a profile is specified and tested |

### 5.3 Required Per-Architecture Spec

An architecture may not be labeled `experimental_native` or `native_supported` without a documented mapping that covers:

- architecture identifier(s) from GGUF metadata
- required tensor names or rename rules
- required scalar metadata
- attention and position-encoding assumptions
- feed-forward shape assumptions
- unsupported optional features
- conversion-time rejection conditions

### 5.4 Conversion Contract

For each supported architecture, conversion must define:

1. tensor selection
2. tensor normalization or reshaping rules
3. ternary/native quantization policy
4. emitted compatibility metadata in `.t81w`
5. failure conditions for partial or incompatible models

T81 must fail closed if a model does not match the declared mapping.

### 5.5 Initial Scope

The first native architecture wave should be intentionally narrow.

Recommended order:

1. llama-family decoder-only dense models
2. mistral-compatible dense models
3. phi-family dense models
4. qwen-family dense models

Deferred initially:

- MoE architectures
- multimodal architectures
- recurrent/state-space hybrids
- architecture families requiring execution semantics not represented in RFC-0034

Implemented so far:

1. `gemma-dense-v1`
2. `llama-dense-v1`
3. `mistral-dense-v1`
4. `phi3-dense-v1`
5. `qwen2-dense-v1`

Not yet implemented:

1. bert-family profile
2. MoE and multimodal profiles

### 5.6 Metadata and Provenance

Converted `.t81w` artifacts should record:

- source architecture family
- source GGUF checksum
- bridge/backend revision used for import
- compatibility profile identifier
- rejected or omitted feature flags, if any

This allows T81 to distinguish:

- “artifact was produced from a known native profile”
- from “artifact was merely imported from a readable GGUF”

## 6. Determinism and Safety

### 6.1 Deterministic Mapping

Architecture detection and tensor selection must not depend on host heuristics beyond model metadata and tensor inventory.

Same input GGUF plus same compatibility profile must produce the same `.t81w`.

### 6.2 Fail-Closed Behavior

If required metadata or tensors are missing, ambiguous, or unsupported:

- conversion must fail
- the failure must identify the architecture profile and missing requirement

No best-effort fallback may silently claim native support.

### 6.3 Runtime Scope

This RFC governs conversion compatibility only.

Whether the resulting `.t81w` provides high-quality prompt inference, throughput parity, or architecture-complete behavior is a separate evidence question and must be proven per family.

Current execution evidence note:

- Import/profile admission is no longer the only gating issue for native model
  families in this RFC's scope.
- Local ARM64 benchmark evidence now shows that plain native weights load is
  measurable, and after lazy canonical-fixed construction the packed native
  decode path is measurable in the same release harness as the llama.cpp
  dequantization baseline. The dominant remaining local gap is deterministic
  exponentiation over the promoted canonical-fixed tensor. A benchmark-only
  direct packed-native `exp` path that still returns a canonical-fixed tensor
  does not materially improve throughput, while a raw-float result path is
  already competitive with the current llama.cpp substrate baseline. That
  points to result representation, not import/profile admission, as the next
  gating factor for moving dense families toward `native_supported`. A first VM
  fast-path set following that result is now in place for native balanced-trit
  ops (`TExp`, `TQUANT`, `TACT`, `TERNACCUM`, `TSiLU`, `TSoftmax`, `TRMSNorm`,
  `TRoPE`, `TWEMBED`, `TWMATMUL`, `TATTN`)
  and has materially improved measured interpreter-path execution, but it does
  not yet eliminate the broader need for lighter-weight native result handling
  across the remaining native surface. The matched VM native-vs-binary
  comparisons are now positive at roughly `62x-67x` for the unary set, `11.99x`
  for the first higher-level `TRMSNorm` loop, `4.07x` for `TRoPE` after
  coefficient caching, `769.63x` for `TWEMBED` after direct packed row
  extraction, and a scale-dependent `TWMATMUL` result that is now only slightly
  negative at `64` (`0.84x`) before crossing to `11.84x` at `256` and
  `570.42x` at `4096`. `TATTN` is now also materially positive after bypassing
  native `K` promotion, reaching `110970.48x` at `256` in the current VM-path
  comparison, and `TQUANT` reaches `6259.43x` at `64` once its dispatch order
  stops forcing promotion first. `TACT` behaves similarly at `64`
  (`6834.34x`) because balanced native trits stay in-domain for both supported
  activation modes, while `TERNACCUM` reaches `80.57x` with a direct scalar
  ternary dot-product path into `BigInt`. That is enough to show
  execution-side progress beyond mere import admission, but it also makes clear
  that different kernel classes will need different optimization styles.
- That means additional profiles can still be added under this RFC, but moving
  any family from `experimental_native` toward `native_supported` now depends
  not only on conversion correctness and rejection discipline, but also on
  improved post-load execution behavior for native tensors.

## 7. Alternatives Considered

### 7.1 Claim Native Support for Any Model llama.cpp Can Load

Rejected.

Loadability is not equivalent to meaningful native execution support.

### 7.2 Keep Compatibility Undocumented and Evolve Ad Hoc

Rejected.

That would create constant ambiguity in CLI behavior and artifact expectations.

### 7.3 Restrict T81 to a Single Architecture Forever

Rejected.

Too limiting for the intended model-ingestion roadmap.

## 8. Implementation Plan

### 8.1 Phase 1: Compatibility Framework

- add native compatibility state model
- define compatibility matrix document
- add deterministic architecture detection helpers

### 8.2 Phase 2: First Architecture Profile

- [x] implement one dense llama-family profile
- [x] add conversion tests against a real llama-family GGUF
- [x] emit profile metadata in `.t81w`

### 8.3 Phase 3: Expansion

- [~] add mistral/phi/qwen dense profiles
- [x] add explicit rejection tests for unsupported advanced features

Current status:

- `gemma-dense-v1` is now implemented in code and covered by fixture tests
- `mistral-dense-v1` is now implemented in code and covered by fixture tests
- `phi3-dense-v1` is now implemented in code and covered by fixture tests
- `qwen2-dense-v1` is now implemented in code and covered by fixture tests
- bert-family or other non-decoder-only profiles are now the next explicit negative examples

### 8.4 Phase 4: Evidence

- run end-to-end conversion plus RFC-0034 execution evidence for each promoted family
- document known quality and coverage limits

Current evidence state on 2026-03-18:

- real TinyLlama GGUF conversion into native `.t81w` is verified in-repo
- synthetic fixture coverage exists for `gemma`, `mistral`, `phi3`, and `qwen2`
- native execution evidence now exists for the generic `.t81w` path via
  `docs/records/status-history/NATIVE_WEIGHTS_EXECUTION_EVIDENCE_2026-03-18.md`
- the next step for this RFC is not another profile name alone; it is
  family-by-family execution evidence after conversion, starting with the
  already-admitted dense decoder families

Status 2026-03-18: proposed in-repo. The compatibility framework, explicit
profile matrix, deterministic rejection rules, and first real native-conversion
path are implemented. The remaining step before `accepted` is tighter
family-specific end-to-end execution evidence beyond generic native `.t81w`
execution coverage.

## 9. Open Questions

1. Should compatibility profile identifiers live inside `.t81w` headers or sidecar records?
2. How much architecture metadata should be embedded versus recomputed at load time?
3. When a family has many variants, should T81 support a generic profile or named subprofiles?
4. Should unsupported optional tensors trigger failure or be recorded as omitted features on a per-profile basis?

## 10. Acceptance Criteria

This RFC moves from `draft` to `proposed` when:

- a compatibility-state model exists
- architecture detection rules are documented
- at least one target family is specified at the profile level

This RFC can move from `proposed` to `accepted` when:

- at least one standard llama.cpp-supported architecture is natively convertible to `.t81w`
- conversion behavior is covered by automated tests
- compatibility metadata is recorded in emitted artifacts or equivalent provenance records
- unsupported features fail deterministically and descriptively
- at least one promoted architecture has end-to-end native execution evidence,
  not just conversion evidence

---

This RFC keeps T81 honest about the difference between reading a model and truly supporting it natively.
