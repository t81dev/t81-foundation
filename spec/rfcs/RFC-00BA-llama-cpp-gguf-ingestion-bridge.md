# RFC-00BA: llama.cpp GGUF Ingestion Bridge

**Status:** accepted
**Type:** standards-track
**Applies-To:** tooling model import path, optional CLI integration, internal build graph
**Created:** 2026-03-18
**Author:** @t81dev
**Depends on:** RFC-0034 (T81-Native AI Inference), RFC-0025 (Policy-Gated Tensor Loading via CanonFS), RFC-00B8 (Governed Foreign Function Interface)
**Blocks:** native `.t81w` conversion from standard llama.cpp GGUF artifacts

---

## 1. Summary

This RFC defines a bounded integration contract for using vendored `llama.cpp` as T81's authoritative GGUF loader and quantized-tensor dequantization backend during model conversion.

The goal is not to make T81 runtime execution depend on `llama.cpp`. The goal is to avoid reimplementing every GGUF tensor format (`Q2_K`, `Q3_K`, `Q4_K`, `Q5_K`, `Q6_K`, future variants) inside T81 while still enabling conversion of common GGUF artifacts into native T81 `.t81w` tensors for RFC-0034 execution.

This RFC introduces a single internal bridge surface that:

- opens a GGUF model through `llama.cpp`
- enumerates tensor metadata
- dequantizes selected tensors to `float32`
- returns those tensors to T81 conversion code

Everything above that bridge remains T81-owned. Everything below it remains `llama.cpp`-owned.

## 2. Motivation

Current `weights import --format gguf` in T81 only supports native ternary `T3_K` GGUF tensors. Real GGUF models in the repository's `models/` directory are standard llama.cpp artifacts dominated by K-quantized tensor formats.

The current choices are:

1. reimplement all llama.cpp GGUF quantization loaders in T81
2. use llama.cpp as the authoritative loader and isolate it behind a narrow internal interface

Option 1 creates permanent maintenance debt. Every future llama.cpp update that adds, changes, or retires a tensor format would force T81 to duplicate that work.

Option 2 keeps T81 focused on its actual value:

- policy and provenance control
- deterministic native ternary execution
- CanonFS-backed artifacts
- conversion into `.t81w`

This RFC adopts option 2.

## 3. Goals

- Enable conversion of standard llama.cpp GGUF models into native `.t81w`
- Avoid implementing GGUF quantized tensor decoders independently in T81
- Localize all `llama.cpp`/`ggml` coupling to one internal bridge layer
- Preserve the ability to update vendored `llama.cpp` without invasive T81 rewrites
- Keep RFC-0034 runtime execution independent from `llama.cpp`

## 4. Non-Goals

- Making the T81 VM or JIT depend on `llama.cpp`
- Treating `llama.cpp` as part of the deterministic execution core
- Exposing llama.cpp internal types across T81 public headers
- Promising support for every future GGUF family on day one
- Replacing the existing `llama-run` governed inference path

## 5. Proposal

### 5.1 Architecture

The integration is split into three layers:

1. `llama.cpp` bridge
   - the only code allowed to include llama.cpp/ggml internal headers
   - responsible for GGUF opening, tensor enumeration, and dequantization

2. T81 conversion layer
   - consumes bridge output as plain metadata plus `float32` tensor rows
   - applies T81-native ternary quantization
   - writes `.t81w`

3. T81 runtime layer
   - consumes `.t81w` only
   - remains independent from llama.cpp

### 5.2 Internal Bridge Contract

The bridge surface is intentionally small:

```cpp
struct GgufTensorDescriptor {
  std::string name;
  std::vector<uint64_t> shape;
  std::string source_type;
  uint64_t element_count;
};

class GgufImportBridge {
public:
  static expected<std::unique_ptr<GgufImportBridge>, std::string> open(const std::filesystem::path&);

  virtual std::vector<GgufTensorDescriptor> list_tensors() const = 0;
  virtual expected<std::vector<float>, std::string> read_tensor_f32(std::string_view tensor_name) = 0;
  virtual ~GgufImportBridge() = default;
};
```

Rules:

- no llama.cpp types in public T81 headers
- no bridge symbols exposed as public install targets
- no T81 core/runtime code linked against the bridge unless explicitly optional

### 5.3 Build Gating

The bridge is available only when `T81_ENABLE_LLAMA_CPP=ON`.

Behavior:

- if llama support is enabled, `weights import --format gguf` may use the bridge for standard GGUF models
- if llama support is disabled, native `T3_K` GGUF import may still work through T81's direct parser
- if a standard GGUF requires llama-backed import and llama support is disabled, the CLI must fail with a specific build-gating message

### 5.4 Upgrade Boundary

Only one implementation unit may depend directly on llama.cpp internal loader details.

That unit owns:

- `llama.cpp` includes
- ggml tensor type inspection
- dequantization calls
- mapping llama tensor metadata into `GgufTensorDescriptor`

All other T81 code must use the bridge interface only.

This preserves a stable T81-side contract even if llama.cpp internals change.

### 5.5 Conversion Flow

For a standard GGUF import:

1. CLI resolves GGUF path
2. bridge opens model through llama.cpp
3. bridge enumerates tensors
4. T81 selects supported tensors for conversion
5. bridge dequantizes tensor data to `float32`
6. T81 quantizes to native ternary representation
7. T81 writes `.t81w`
8. resulting `.t81w` flows through RFC-0034 runtime and CanonFS paths

### 5.6 Fallback Strategy

The bridge contract must allow future replacement of the implementation without changing the importer API.

Allowed future backends:

- direct in-process llama.cpp bridge
- subprocess helper built from vendored llama.cpp
- external export tool producing the same intermediate tensor stream

This RFC standardizes the T81-side contract, not the irreversible choice of transport.

## 6. Determinism and Safety

### 6.1 Determinism Scope

GGUF ingestion through llama.cpp is not part of T81's deterministic execution core.

Determinism guarantees begin after conversion to native `.t81w`.

That means:

- dequantization may be implementation-defined relative to llama.cpp version
- imported `.t81w` must record provenance sufficient to identify source GGUF and bridge version
- runtime execution of resulting `.t81w` remains subject to existing RFC-0034 determinism and Axion policy rules

### 6.2 Provenance Requirements

At minimum, imported `.t81w` evidence should record:

- source GGUF path or CanonFS hash
- source GGUF checksum
- llama.cpp revision or vendored snapshot identifier
- bridge mode used for import
- tensor selection and quantization summary

### 6.3 Policy Boundary

This RFC does not allow arbitrary llama.cpp execution inside governed runtime paths.

The bridge is a tooling/import feature. It is not a VM escape hatch.

If future work exposes bridge-like functionality inside a governed runtime path, that work must explicitly route through Axion policy and likely depend on RFC-00B8.

## 7. Alternatives Considered

### 7.1 Reimplement All GGUF Loaders in T81

Rejected.

This creates recurring maintenance work for every llama.cpp quantization update and provides little T81-specific value.

### 7.2 Use `gguf-py` as the Primary Runtime Converter

Rejected as the primary path.

It is useful as a reference oracle, but a Python-first dependency is weaker for local CLI ergonomics and build integration.

### 7.3 Convert Only Native T3_K GGUF

Rejected as insufficient.

It does not solve the stated user need of converting real llama.cpp models from the repository `models/` directory.

## 8. Implementation Plan

### 8.1 Phase 1: Bridge Skeleton

- add internal GGUF import bridge interface
- implement llama.cpp-backed bridge behind `T81_ENABLE_LLAMA_CPP`
- add focused metadata enumeration tests

### 8.2 Phase 2: Float Export Path

- implement tensor dequantization via llama.cpp/ggml
- support initial standard formats proven by repository models
- add golden checks against known GGUF tensor inventories

### 8.3 Phase 3: Native Conversion

- pipe bridge-exported `float32` tensors into native ternary quantization
- write `.t81w`
- wire CLI `weights import --format gguf` to the bridge-backed path

### 8.4 Phase 4: Evidence and Provenance

- record import provenance
- add CanonFS-backed conversion evidence
- document supported formats and failure modes

Current status on 2026-03-18:

- the internal bridge interface exists
- the bridge is build-gated behind `T81_ENABLE_LLAMA_CPP`
- metadata enumeration and float export tests exist
- `weights import --format gguf` can convert standard non-`T3_K` GGUF models into
  native `.t81w`
- llama.cpp coupling remains localized to the bridge layer
- import output now records source GGUF checksum and bridge revision on the
  `ModelFile` surface and in `weights import` reporting
- real TinyLlama GGUF conversion is verified in-repo in addition to fixture
  coverage

Status 2026-03-18: accepted in-repo. Remaining work after acceptance is limited
to broader provenance persistence choices and future backend replacement
flexibility, not the core bridge contract itself.

## 9. Open Questions

1. Should the first bridge implementation use llama.cpp public APIs only, or may it include internal loader headers?
2. Should import provenance be embedded inside `.t81w`, or recorded only in sidecar evidence?
3. Which tensor families should be imported first for useful prompt-style inference after conversion?
4. Should the bridge expose per-tensor streaming to reduce peak memory pressure on large GGUF models?

## 10. Acceptance Criteria

This RFC moves from `draft` to `proposed` when:

- an internal bridge interface exists
- the bridge is build-gated behind `T81_ENABLE_LLAMA_CPP`
- at least one llama.cpp-backed metadata enumeration test passes
- no T81 public header exposes llama.cpp types

This RFC moves from `proposed` to `accepted` when:

- `weights import --format gguf` can convert at least one standard non-`T3_K` GGUF model into `.t81w`
- the import path is covered by automated tests
- provenance output records the source GGUF and llama.cpp bridge revision
- the integration remains localized to the bridge layer

---

This RFC exists to keep llama.cpp integration narrow, replaceable, and honest: llama.cpp owns GGUF loading, T81 owns governed conversion and native execution.
