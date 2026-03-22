# RFC-0032: AI Subsystem Promotion Pathway

**Status:** accepted
**Type:** standards-track
**Applies-To:** `spec/tisc-spec.md`, `spec/t81vm-spec.md`, `spec/supplemental/axion-policy-grammar.md`, `spec/supplemental/canonfs-spec.md`, `RFC-0002`, `RFC-0003`, `RFC-0004`, `RFC-0025`, `RFC-0026`, `RFC-0031`, `RFC-00A0`
**Created:** 2026-03-07
**Updated:** 2026-03-15
**Supersedes:** —
**Superseded-By:** —
**Discussion:** —

---

## 1. Abstract

This RFC translates the findings of the T81 Foundation AI Promotion Audit Report into a normative engineering specification. It defines deterministic promotion requirements, architectural placement, and governance integration for ten identified AI experimental components. Components are classified into three promotion dispositions: **PROMOTE** (ready for core integration as-is), **MODIFY THEN PROMOTE** (requires remediation of determinism or governance defects before integration), and **KEEP EXPERIMENTAL** (permanently or indefinitely excluded from deterministic core due to irremediable non-determinism). The RFC establishes a five-phase promotion roadmap, normative determinism gates for each phase, and clear references to the Axion governance kernel, CanonFS content-addressed model loading, and the determinism test suite.

---

## 2. Motivation

The T81 Foundation's experimental AI subsystem (`experiments/ai/`) contains implementations of ternary quantization, LLM backend adaption, model provenance management, AI-native policy hooks, benchmarking infrastructure, deterministic evidence collection, and developer UX tooling. These components were developed under the RFC-00A0 sandbox regime and have reached sufficient maturity for formal audit. An AI Promotion Audit was conducted across all ten components (see §5 Scope for the full inventory). The audit surfaced four systematic defects across components slated for core promotion:

1. **Floating-point contamination** — `ternary_codec.cpp` embeds `float`/`double` metric computations (`std::sqrt`, `std::log10`) on the core quantization path.
2. **External ML runtime dependency** — `backend_adapter.cpp` links against `llama.cpp` and `onnx_runtime`, both of which introduce non-deterministic floating-point results and platform-dependent execution order.
3. **Incomplete Axion integration** — `model_manager.cpp` implements its own ad hoc hash verification rather than using the TISC `TLOADHASH` instruction and the Axion `allowed-tensor-hashes` policy gate defined in RFC-0025.
4. **Simulation artefacts in CLI** — `t81_ai_cli.cpp` uses mock data and `std::this_thread::sleep_for` as simulation scaffolding that is incompatible with the Deterministic Core Profile.

Without a formal promotion specification these defects risk admission into the core, violating the bit-exact determinism contract defined in RFC-0002 and undermining the supply-chain security guarantees of RFC-0025.

---

## 3. Scope

### 3.1 Components Under Review

This RFC covers the following components identified in the AI Promotion Audit Report:

| ID | Source Path | Audit Disposition | Target Location |
| :--- | :--- | :--- | :--- |
| C-01 | `experiments/ai/quantization/ternary_codec.cpp` | MODIFY THEN PROMOTE | `core/math/quantization/` |
| C-02 | `experiments/ai/llm_backend/backend_adapter.cpp` | MODIFY THEN PROMOTE | `core/vm/ai_backend/` |
| C-03 | `experiments/ai/model_provenance/model_manager.cpp` | MODIFY THEN PROMOTE | Axion subsystem / CanonFS |
| C-04 | `experiments/ai/policy_hooks/axion_hooks.cpp` | PROMOTE | Axion subsystem |
| C-05 | `experiments/ai/benchmarks/benchmark_runner.cpp` | KEEP EXPERIMENTAL | `experiments/ai/benchmarks/` |
| C-06 | `experiments/ai/determinism/evidence_collector.cpp` | PROMOTE | `tests/determinism/` |
| C-07 | `experiments/ai/ux_tools/t81_ai_cli.cpp` | MODIFY THEN PROMOTE | `tools/cli/ai/` |
| C-08 | `experiments/ai/ux_tools/t81_ai_minimal.cpp` | KEEP EXPERIMENTAL | `experiments/ai/ux_tools/` |
| C-09 | `experiments/ai/sandbox/promotion_gates.cpp` | KEEP EXPERIMENTAL | `experiments/ai/sandbox/` |
| C-10 | `experiments/ai/opcodes/IMPLEMENTATION_REPORT.md` | PROMOTE | `docs/architecture/` |

### 3.2 Non-Goals

This RFC does not:

- Redefine TISC opcode semantics already specified in RFC-0026.
- Modify the TLOADHASH instruction encoding or fault model defined in RFC-0025.
- Extend the Axion policy language grammar beyond what is already specified in RFC-0022.
- Introduce new tensor data types beyond those defined in RFC-0004.
- Define performance targets or latency guarantees for any promoted component.
- Mandate promotion timelines beyond the phase targets in §11.

---

## 4. Architectural Context

The T81 architecture defines a layered determinism stack:

```
T81Lang (source)
    ↓  lang/frontend — lexer / parser / semantic analysis
TISC Bytecode
    ↓  kernel/axion — policy enforcement intercept
T81VM — deterministic interpreter (core/vm/)
    ↓
Foundation: TISC ISA + ternary data types  [FROZEN per major version]
```

AI subsystem components promoted to core MUST integrate at one of three defined integration surfaces:

- **ISA surface** — new or extended TISC opcodes specified in `spec/tisc-spec.md` and implemented in `core/isa/` and `core/vm/`.
- **Axion surface** — policy hooks, governance events, and model-load verification integrated into `kernel/axion/`.
- **CanonFS surface** — content-addressed model artifact storage accessed exclusively through `TLOADHASH` as specified in RFC-0025.

No promoted component may introduce a fourth integration surface without a separate RFC approved by the Architecture Review process.

External ML runtimes (`llama.cpp`, `onnx_runtime`, and similar) are **permanently prohibited** from the deterministic core. They may remain as opt-in adapters in `experiments/ai/` subject to an explicit `T81_ENABLE_LLAMA_CPP` CMake flag, and they MUST be absent from all promoted source paths.

---

## 5. Promotion Disposition Rules

### 5.1 PROMOTE (Ready for Core Integration)

A component with disposition PROMOTE satisfies all of the following preconditions at the time of audit:

1. All execution paths are deterministic: given identical input state, output is bit-exact across x86-64 and ARM64.
2. The component does not use hardware floating-point instructions, `std::chrono`, OS-level timing, or thread-sleep calls on any path reachable from the promoted entry point.
3. The component integrates with, or is coherent with, the Axion governance surface (RFC-0003, RFC-0022) as appropriate to its function.
4. The component has no direct or transitive dependency on external ML runtimes.
5. The component has no dependency on mock or stub data structures on any promoted code path.

Components C-04 (`axion_hooks.cpp`) and C-06 (`evidence_collector.cpp`) satisfy these preconditions and MAY be promoted immediately upon RFC acceptance without further code modification, subject to the acceptance criteria in §13.

Component C-10 (`IMPLEMENTATION_REPORT.md`) is a documentation artifact, not a binary component. It MUST be promoted to `docs/architecture/` as the normative reference for AI opcode Phase 1 conformance evidence. Its claims regarding ATTN, QMATMUL, and EMBED determinism MUST be verified against the conformance suite defined in RFC-0027 before promotion is closed.

### 5.2 MODIFY THEN PROMOTE

A component with disposition MODIFY THEN PROMOTE is architecturally sound in structure and intent but contains one or more determinism-violating or governance-bypassing defects that MUST be remediated before promotion. Remediation requirements are normative and defined per component in §7.

Promotion of a MODIFY THEN PROMOTE component is gated on passing all determinism validation tests in `tests/determinism/` for the modified component, as specified in §8.4.

### 5.3 KEEP EXPERIMENTAL

A component with disposition KEEP EXPERIMENTAL is permanently or indefinitely excluded from the deterministic core. Such components:

- MUST remain in `experiments/ai/` or a subdirectory thereof.
- MUST be gated behind `T81_ENABLE_AI_EXPERIMENTS` (OFF by default).
- MUST NOT be linked against by any target in the `OFF`-by-default build configuration.
- MUST NOT be referenced by path or symbol from any promoted component.

Disposition KEEP EXPERIMENTAL does not prevent future re-audit if the component is fundamentally restructured. A re-audit request requires a new RFC or a formal amendment to this RFC.

---

## 6. Promotion Requirements for AI Components

### 6.1 C-01 — `ternary_codec.cpp` (MODIFY THEN PROMOTE)

**Defect:** The codec computes MSE and PSNR quality metrics using `float`, `double`, `std::sqrt`, and `std::log10`. These expressions are on a code path compiled into the codec translation unit and violate the prohibition on hardware floating-point in the Deterministic Core Profile (RFC-0002 §3).

**Normative remediation requirements:**

- All floating-point metric computations (MSE, PSNR, and any derived metrics) MUST be removed from the core quantization codec implementation.
- If quality metrics are required for offline tooling or diagnostics, they MUST be relocated to a separate translation unit in `tools/diagnostics/` that is explicitly excluded from the deterministic core build targets and gated behind `T81_BUILD_DIAGNOSTICS`.
- The promoted codec MUST use only integer or fixed-point arithmetic for all quantization, packing, and unpacking operations.
- The canonical Base-81 packing algorithm MUST produce bit-exact output for identical input trits across all supported platforms and compilers.
- Promoted target: `core/math/quantization/ternary_codec.cpp`.

### 6.2 C-02 — `backend_adapter.cpp` (MODIFY THEN PROMOTE)

**Defect:** The adapter delegates inference execution to `llama.cpp` and `onnx_runtime`. Both runtimes use hardware FPU instructions, can exhibit platform-dependent rounding, and may schedule operations in non-deterministic order. They cannot be made deterministic without replacing their execution engine.

**Normative remediation requirements:**

- All `#include` directives, link dependencies, and call sites referencing `llama.cpp` or `onnx_runtime` MUST be removed from the promoted translation unit.
- The promoted backend adapter MUST dispatch all inference operations exclusively to the T81 VM interpreter via the TISC AI-native opcode surface (RFC-0026 §5.15): `ATTN`, `QMATMUL`, `WLOAD`, `EMBED`, `GATHER`, `SCATTER`.
- The adapter MUST NOT implement its own tensor arithmetic. All tensor computation MUST be expressed as TISC instruction sequences dispatched through the T81 VM.
- The adapter interface MUST be Axion-mediated: each inference request MUST invoke the Axion pre-execution hook before dispatching to the VM.
- An optional `T81_ENABLE_LLAMA_CPP` build flag MAY retain the llama.cpp dispatch path in `experiments/ai/llm_backend/` for research purposes. This flag MUST NOT be set in any CI configuration used to validate promoted components.
- Promoted target: `core/vm/ai_backend/backend_adapter.cpp`.

### 6.3 C-03 — `model_manager.cpp` (MODIFY THEN PROMOTE)

**Defect:** The model manager implements its own hash verification logic that does not invoke the `TLOADHASH` TISC instruction or the Axion `allowed-tensor-hashes` policy gate (RFC-0025 §3). This creates a model loading path that bypasses the cryptographic supply-chain gate.

**Normative remediation requirements:**

- All ad hoc hash verification logic MUST be removed from `model_manager.cpp`.
- Model loading MUST be performed exclusively via the `TLOADHASH` TISC instruction as specified in RFC-0025 §3.3. No model weight data MAY be materialized in the tensor pool without prior `TLOADHASH` policy verification.
- The model manager MUST validate model artifact hashes against the `allowed-tensor-hashes` list in the active Axion policy. An unverified or policy-denied hash MUST result in a `SecurityFault (POLICY_VIOLATION)` — the model handle MUST NOT be materialized.
- The model manager MUST emit the canonical Axion trace event `TLOADHASH success|failure hash=<hash> reason=<reason>` on every load attempt.
- Model artifacts MUST be stored in and retrieved from CanonFS exclusively. Direct filesystem access to model weight files outside of CanonFS is prohibited on promoted code paths.
- Promoted target: Axion subsystem (`kernel/axion/`) and CanonFS integration layer (`fs/`).

### 6.4 C-04 — `axion_hooks.cpp` (PROMOTE)

**Disposition rationale:** The component implements Axion policy event callbacks for AI-specific events. It follows deterministic policy evaluation logic, emits canonical Axion events, and has no external ML runtime dependencies or floating-point execution.

**Normative integration requirements on promotion:**

- The promoted file MUST register its AI event hooks through the canonical Axion hook registration API defined in `kernel/axion/policy_engine.cpp`.
- Hook identifiers MUST be assigned from the reserved AI event namespace. Concrete identifiers MUST be registered in `spec/supplemental/axion-event-registry.md`.
- All hook implementations MUST be side-effect-free with respect to VM state: hooks MAY read VM state for policy evaluation but MUST NOT modify tensor pool contents, register values, or program counter.
- Promoted target: `kernel/axion/ai_hooks.cpp`.

### 6.5 C-05 — `benchmark_runner.cpp` (KEEP EXPERIMENTAL)

**Disposition rationale:** The component measures latency, tokens-per-second, and memory usage via `std::chrono` and OS hardware information queries. These measurements are fundamentally hardware-dependent and cannot be made deterministic without replacing them with something other than a performance benchmark. No remediation path exists for deterministic promotion.

**Normative constraints while experimental:**

- MUST remain in `experiments/ai/benchmarks/`.
- MUST be gated behind `T81_ENABLE_AI_EXPERIMENTS`.
- MUST NOT be linked against by any promoted target.
- Any deterministic step-count or instruction-count proxy metrics introduced in this component for internal use MUST NOT be surfaced as latency or performance claims.

### 6.6 C-06 — `evidence_collector.cpp` (PROMOTE)

**Disposition rationale:** The component collects and verifies deterministic execution evidence using SHA-256 and deterministic metric comparison. It does not use hardware timing, external runtimes, or floating-point on promoted paths.

**Normative integration requirements on promotion:**

- The promoted component MUST be integrated into the determinism test suite as a first-class validation driver.
- Evidence collected MUST include: CanonHash-81 of the VM trace, Axion event sequence hash, and cross-platform register-state fingerprint at program termination.
- The evidence schema MUST be documented in `tests/determinism/README.md` and MUST be stable across patch releases.
- Promoted target: `tests/determinism/evidence_collector.cpp`.

### 6.7 C-07 — `t81_ai_cli.cpp` (MODIFY THEN PROMOTE)

**Defects:**
1. Uses mock data structures that do not invoke actual promoted subsystems.
2. Uses `std::this_thread::sleep_for` as a simulation delay on user-facing code paths.

**Normative remediation requirements:**

- All mock data structures MUST be replaced with calls to the actual promoted subsystems (C-01, C-02, C-03, C-04) once those components have completed promotion.
- All `std::this_thread::sleep_for` calls MUST be removed unconditionally. Progress indication MUST be expressed as deterministic step-count output, not wall-clock time.
- The promoted CLI MUST not invoke any component with disposition KEEP EXPERIMENTAL.
- Promoted target: `tools/cli/ai/t81_ai_cli.cpp`. Promotion of this component is explicitly **dependent** on completion of Phase 4 (§11) since it wraps the AI backend.

### 6.8 C-08 — `t81_ai_minimal.cpp` (KEEP EXPERIMENTAL)

**Disposition rationale:** The component uses mock data and hardcoded responses throughout. There is no structurally sound core to promote; the component is a stub. No remediation path short of a complete rewrite (which would constitute a new component) is defined.

### 6.9 C-09 — `promotion_gates.cpp` (KEEP EXPERIMENTAL)

**Disposition rationale:** The component validates experiment promotion using simulated CI checks. It is a sandbox validation utility, not a runtime component. Its function is useful for local development but is explicitly not part of the runtime core. The authoritative promotion gate is the CI pipeline defined in `tests/determinism/` and the RFC acceptance criteria in §13, not a runtime binary.

### 6.10 C-10 — `IMPLEMENTATION_REPORT.md` (PROMOTE)

**Disposition rationale:** The document reports Phase 1 conformance evidence for AI-native opcodes ATTN, QMATMUL, and EMBED. It contains architectural decisions and conformance data that MUST be preserved in the normative documentation tree.

**Normative integration requirements on promotion:**

- The document MUST be promoted to `docs/architecture/ai-opcode-phase1-conformance.md`.
- Claims in the document regarding deterministic execution of ATTN, QMATMUL, and EMBED MUST be cross-referenced against the spec-as-executable conformance programs required by RFC-0027 and RFC-0031.
- The document MUST not be modified after promotion without an RFC amendment or a new conformance report covering the modified claims.

---

## 7. Deterministic Execution Requirements

All components promoted to the deterministic core MUST satisfy the following requirements. Violation of any requirement constitutes a determinism defect that MUST block promotion.

### 7.1 Arithmetic

- All promoted code paths MUST use integer or T81Float soft-float arithmetic exclusively (RFC-0004 §3.2, RFC-0031 §Deterministic AI Arithmetic Contract).
- Hardware FPU instructions MUST NOT appear in any promoted translation unit. This includes calls to `std::sqrt`, `std::log`, `std::pow`, `std::exp`, and any other `<cmath>` function that may lower to an SSE or NEON floating-point instruction.
- Quantization operations MUST satisfy the round-trip invariant: `dequantize(quantize(x, scale), scale) == x` for all `x` in the representable range (RFC-0031).

### 7.2 Time and System State

- Promoted code paths MUST NOT read wall-clock time (`std::chrono`, `clock_gettime`, `gettimeofday`), query OS hardware topology, or call `std::this_thread::sleep_for`.
- Promoted code paths MUST NOT read process environment variables, kernel version strings, or CPU feature flags at runtime to branch on execution behavior.

### 7.3 External Dependencies

- Promoted translation units MUST NOT link against `llama.cpp`, `onnx_runtime`, or any other external ML runtime.
- Promoted translation units MUST NOT call into `dlopen`/`LoadLibrary` or equivalent dynamic dispatch mechanisms.

### 7.4 Tensor Pool Determinism

- All tensor allocations in promoted components MUST follow the strictly append-only tensor pool model defined in RFC-0025 §3.3.4.
- Handle assignment MUST be reproducible: given identical program inputs and identical execution order, every tensor handle MUST receive an identical numeric value across all executions.

### 7.5 Cross-Platform Bit-Exactness

- Promoted AI components MUST produce bit-exact results on x86-64 and ARM64 reference platforms for identical inputs and initial VM state.
- CI MUST validate bit-exactness via the cross-platform determinism matrix target `spec_determinism_ai_cross_platform` before any promoted component may merge to the main branch.

---

## 8. Axion Governance Integration

### 8.1 Mandatory Pre-Execution Verification

The Axion kernel MUST intercept and evaluate the following AI-specific events before the corresponding VM operation may proceed. A `DENY` verdict MUST result in a `SecurityFault (POLICY_VIOLATION)` that terminates the current VM execution frame.

| Event | Trigger | Required Policy Check |
| :--- | :--- | :--- |
| `ai_model_load` | `TLOADHASH` instruction | `allowed-tensor-hashes` whitelist |
| `attn_guard` | `ATTN` opcode dispatch | Q/K/V shape compatibility; tier ≥ 2 |
| `qmatmul_guard` | `QMATMUL` opcode dispatch | Weight tensor provenance; scale canonical range |
| `ai_exec_gate` | Entry into promoted AI backend (C-02) | `ai-execution-enabled` policy flag |

### 8.2 Axion Event Emission Requirements

Promoted AI components MUST emit the following canonical Axion trace events. Event format MUST conform to RFC-0003 §4 and RFC-0020.

```
model_load   success|failure  hash=<sha3-256:hex64>  reason=<string>
attn_guard   shape=<Q_shape>x<K_shape>x<V_shape>  tier=<int>
qmatmul_guard  policy=allow|deny  scale=<fixed-point>  wt_hash=<sha3-256:hex64>
tensor_alloc  rank=<int>  size=<int>  handle=<int>
ai_exec_gate  backend=t81vm  policy=allow|deny
```

Events MUST be emitted before the associated operation executes, not after.

### 8.3 Policy Language Requirements

The active Axion policy (RFC-0022) MUST include the following directives for AI execution to proceed:

- `(allowed-tensor-hashes [...])` — non-empty list of `sha3-256:`-prefixed hashes of permitted model weight `CanonObject`s.
- `(ai-execution-enabled true)` — explicit opt-in to AI opcode execution.
- `(max-tensor-rank <int>)` — per-cognitive-tier maximum tensor rank; MUST be set to a finite value.

Absence of any required directive MUST result in `SecurityFault (POLICY_VIOLATION)` on the first AI operation attempted.

### 8.4 Hook Registration

`axion_hooks.cpp` (C-04), once promoted to `kernel/axion/ai_hooks.cpp`, MUST register its callbacks through the canonical registration API before the VM begins execution. Hook registration MUST be idempotent and MUST not modify any VM state or tensor pool.

---

## 9. CanonFS Model Provenance Requirements

### 9.1 Exclusive Storage Path

All AI model weight artifacts consumed by promoted components MUST be stored in CanonFS as `CanonObject`s serialized per the format defined in RFC-0025 §3.2.1. Direct filesystem path loading of raw weight files is prohibited on promoted code paths.

### 9.2 Hash-First Access

Model weight data MUST only be materialized in the tensor pool following successful `TLOADHASH` policy verification (RFC-0025 §3.3.3). No promoted component may cache, copy, or otherwise hold a reference to weight data that has not passed `TLOADHASH` verification in the current execution session.

### 9.3 Audit Trail

Every model load attempt MUST produce a CanonFS audit record containing:

- `sha3-256` hash of the requested `CanonObject`
- Axion `ALLOW` or `DENY` verdict and verbatim reason string
- Session-scoped tensor handle assigned (on `ALLOW`) or `null` (on `DENY`)
- Monotonic VM instruction counter at time of load

The audit record MUST be written to the CanonFS audit log before the load completes. On `DENY`, the audit record MUST be written before the `SecurityFault` is raised.

### 9.4 Integrity Verification on Load

The VM MUST re-verify the `CanonObject` SHA-3 digest against the policy-whitelist value after fetching from CanonFS storage and before deserializing the tensor header. A digest mismatch after fetch MUST raise `BoundsFault (CANONFS_MISS)` rather than silently loading corrupted data.

---

## 10. Promotion Roadmap

The following five phases are derived directly from the AI Promotion Audit Report. Phases are sequential: a phase MUST NOT begin until all acceptance criteria for the preceding phase are met.

### Phase 1 — Deterministic Tensor Primitives and Codecs

**Target components:** C-01 (`ternary_codec.cpp`)

**Required actions:**
1. Remove or relocate all floating-point metric computations (MSE, PSNR) from `ternary_codec.cpp` to `tools/diagnostics/`.
2. Verify that the remaining codec produces bit-exact Base-81 packing output on x86-64 and ARM64 using the cross-platform determinism harness.
3. Add dedicated determinism tests for the promoted codec to `tests/determinism/codec/`.
4. Promote the modified codec to `core/math/quantization/ternary_codec.cpp`.

**Phase 1 gate criteria:** All tests in `tests/determinism/codec/` pass. No `float` or `double` symbols appear in the promoted translation unit (verified by `nm` or equivalent symbol analysis in CI).

### Phase 2 — AI Opcode Integration

**Target components:** C-10 (`IMPLEMENTATION_REPORT.md`)

**Required actions:**
1. Promote `IMPLEMENTATION_REPORT.md` to `docs/architecture/ai-opcode-phase1-conformance.md`.
2. Integrate the AI-native inference opcodes (ATTN, QMATMUL, EMBED) into the core ISA (`core/isa/opcodes.hpp`, `core/isa/binary_emitter.cpp`) and VM dispatcher (`vm/vm.cpp`) in accordance with RFC-0026 §5.15.
3. Add conformance programs per RFC-0027: `spec/conformance/ai/attn-determinism.t81`, `spec/conformance/ai/qmatmul-scale-order.t81`, `spec/conformance/ai/embed-bounds-check.t81`.

**Phase 2 gate criteria:** All three conformance programs execute without fault and produce expected deterministic outputs on both reference platforms. ATTN and QMATMUL results are bit-exact across platforms.

### Phase 3 — Axion Model Governance and Policy

**Target components:** C-03 (`model_manager.cpp`), C-04 (`axion_hooks.cpp`)

**Required actions:**
1. Remove ad hoc hash verification from `model_manager.cpp`; replace all model loading with `TLOADHASH` invocation.
2. Promote modified `model_manager.cpp` to the Axion subsystem and CanonFS integration layer.
3. Promote `axion_hooks.cpp` to `kernel/axion/ai_hooks.cpp`; register AI event hooks.
4. Add Axion policy tests confirming that a model with a non-whitelisted hash raises `SecurityFault (POLICY_VIOLATION)`.
5. Add CanonFS integration test confirming that a missing artifact raises `BoundsFault (CANONFS_MISS)`.

**Phase 3 gate criteria:** All Axion AI event strings defined in §8.2 appear in trace output for a reference inference run. Policy-denial and CanonFS-miss fault tests pass. C-10 conformance document cross-references are resolved.

### Phase 4 — Deterministic VM Backend

**Target components:** C-02 (`backend_adapter.cpp`)

**Required actions:**
1. Remove all `llama.cpp` and `onnx_runtime` includes and link dependencies.
2. Rewrite the dispatch path to invoke the T81 VM via TISC AI-native opcodes (RFC-0026 §5.15), building on promoted Phase 2 and Phase 3 infrastructure.
3. Add the Axion `ai_exec_gate` pre-execution hook call.
4. Execute a full end-to-end inference test against a CanonFS-stored `T81W` model artifact, verifying bit-exact output on both reference platforms.
5. Promote to `core/vm/ai_backend/backend_adapter.cpp`.

**Phase 4 gate criteria:** The promoted backend produces bit-exact inference output on x86-64 and ARM64. The CI target `spec_determinism_ai_cross_platform` passes. Zero references to `llama.cpp` or `onnx_runtime` appear in the promoted build graph.

### Phase 5 — CLI Exposure and Determinism Testing

**Target components:** C-06 (`evidence_collector.cpp`), C-07 (`t81_ai_cli.cpp`)

**Required actions:**
1. Promote `evidence_collector.cpp` to `tests/determinism/evidence_collector.cpp`; document the evidence schema in `tests/determinism/README.md`.
2. Remove all mock data and `std::this_thread::sleep_for` calls from `t81_ai_cli.cpp`.
3. Wire `t81_ai_cli.cpp` to the actual promoted subsystems from Phases 1–4.
4. Add CLI integration tests that invoke the promoted CLI against a reference CanonFS model store and verify the Axion trace output.
5. Promote `t81_ai_cli.cpp` to `tools/cli/ai/t81_ai_cli.cpp`.

**Phase 5 gate criteria:** CLI integration tests pass. Evidence collector produces a stable, documented schema. CLI produces no mock output and no timing-dependent behavior.

---

## 11. Determinism Risk Analysis

### 11.1 External ML Runtime Dependency (HIGH)

**Risk:** `backend_adapter.cpp` (C-02) links against `llama.cpp` and `onnx_runtime`. These runtimes use hardware FPU, may use SIMD paths that differ by CPU microarchitecture, and can schedule operations in non-deterministic order.

**Severity:** Critical. If admitted to core without remediation, this would break the Deterministic Core Profile for all inference workloads.

**Remediation:** Mandatory as part of Phase 4. The `T81_ENABLE_LLAMA_CPP` flag MUST remain `OFF` in all CI configurations that validate promoted components.

### 11.2 Hardware and Time-Dependent Behavior (HIGH)

**Risk:** `benchmark_runner.cpp` (C-05) and `t81_ai_cli.cpp` (C-07, pre-remediation) use `std::chrono`, OS hardware queries, and `std::this_thread::sleep_for`. Any such call that reaches a promoted code path constitutes a determinism violation.

**Severity:** Critical for C-07 if promoted without remediation. Contained for C-05 by KEEP EXPERIMENTAL disposition.

**Remediation:** `benchmark_runner.cpp` remains experimental permanently. `t81_ai_cli.cpp` MUST have all timing calls removed as a Phase 5 requirement.

### 11.3 Floating-Point Contamination (HIGH)

**Risk:** `ternary_codec.cpp` (C-01) uses `float`, `double`, `std::sqrt`, and `std::log10` on the codec path. These produce platform-dependent results and violate RFC-0002 §3.

**Severity:** High. Admitted to `core/math/quantization/` without remediation, this would contaminate every downstream quantization consumer.

**Remediation:** Mandatory as part of Phase 1. Post-promotion CI MUST verify absence of floating-point symbols via static analysis.

### 11.4 Model Loading without Axion Gate (MEDIUM)

**Risk:** `model_manager.cpp` (C-03) performs its own hash verification outside the Axion governance path. This creates a supply-chain attack vector: a model that passes the ad hoc check may not match the policy-whitelisted artifact.

**Severity:** Medium architectural risk (the component is experimental today) escalating to High if promoted without remediation.

**Remediation:** Mandatory as part of Phase 3. TLOADHASH integration is the normative resolution.

### 11.5 Stub Components in Experimental Layer (LOW)

**Risk:** `t81_ai_minimal.cpp` (C-08) and `promotion_gates.cpp` (C-09) use hardcoded mock responses and simulated CI, respectively. If a promoted component acquired a transitive dependency on these stubs, it would import non-determinism.

**Severity:** Low, as both are permanently KEEP EXPERIMENTAL and gated behind the opt-in build flag.

**Remediation:** Enforced by RFC-00A0 §4 and §8.2 of this RFC. CI MUST verify that no promoted target has a link or include dependency on these files.

---

## 12. Implementation Guidance

### 12.1 Build System Integration

Promoted components MUST be added to the appropriate CMake library target:

| Component | Promoted Path | CMake Target |
| :--- | :--- | :--- |
| C-01 (codec) | `core/math/quantization/` | `t81_core` |
| C-02 (backend) | `core/vm/ai_backend/` | `t81_vm` |
| C-03 (model mgr) | `kernel/axion/` + `fs/` | `t81_axion` |
| C-04 (hooks) | `kernel/axion/ai_hooks.cpp` | `t81_axion` |
| C-06 (evidence) | `tests/determinism/` | `t81_determinism_tests` |
| C-07 (CLI) | `tools/cli/ai/` | `t81_tool_cli` |
| C-10 (doc) | `docs/architecture/` | N/A (documentation) |

No promoted component SHOULD be added to a CMake target that also links against the experimental opt-in targets without explicit build-system separation.

### 12.2 Static Analysis Gates

CI MUST enforce the following static checks on all promoted translation units before merge:

1. **No FP symbols:** `nm -u` on the object file MUST NOT list any symbol from `<cmath>` or `libm` that can resolve to a hardware FPU operation.
2. **No external ML runtime symbols:** `nm -u` MUST NOT list any symbol with prefix `llama_`, `onnx_`, or `OrtApi`.
3. **No sleep/timing symbols:** `nm -u` MUST NOT list `sleep_for`, `clock_gettime`, `gettimeofday`, or `std::chrono`.

### 12.3 Conformance Program Requirements

Per RFC-0027, each promotion phase MUST produce at least one spec-as-executable conformance program. Programs MUST:

- Reside in `spec/conformance/ai/`.
- Use `@axion_verify` on `main()`.
- Produce a single boolean pass/fail output.
- Include a comment block citing the normative RFC section validated.

Minimum required conformance programs across all five phases:

```
spec/conformance/ai/codec-base81-roundtrip.t81       # Phase 1 — C-01
spec/conformance/ai/attn-determinism.t81             # Phase 2 — RFC-0026 §5.15.1
spec/conformance/ai/qmatmul-scale-order.t81          # Phase 2 — RFC-0026 §5.15.2
spec/conformance/ai/embed-bounds-check.t81           # Phase 2 — RFC-0026 §5.15.4
spec/conformance/ai/tloadhash-policy-gate.t81        # Phase 3 — RFC-0025 §3.3.3
spec/conformance/ai/model-load-audit-trail.t81       # Phase 3 — §9.3
spec/conformance/ai/backend-no-fpu-path.t81          # Phase 4 — §7.1
spec/conformance/ai/cli-no-mock-output.t81           # Phase 5 — §6.7
```

---

## 13. Backwards Compatibility

This RFC does not modify any existing TISC opcode encoding, T81Lang syntax, Axion policy grammar, or CanonFS object format. All changes are additive:

- Promoted components add new source files to existing CMake targets; they do not replace or remove existing files.
- The `T81_ENABLE_AI_EXPERIMENTS` flag and experimental directory structure defined in RFC-00A0 are preserved unchanged.
- Existing T81Lang programs that do not use AI-native opcodes are unaffected.
- The KEEP EXPERIMENTAL components remain available behind the opt-in flag for existing users of the experimental build configuration.

---

## 14. Security Considerations

### 14.1 Model Supply Chain

The most significant security surface introduced by AI subsystem promotion is model weight loading. This RFC addresses this by mandating exclusive use of `TLOADHASH` and the Axion `allowed-tensor-hashes` gate (RFC-0025). No weight data may be materialized outside this verification chain.

A consequence of the C-03 remediation requirement is that any deployment configuration that previously loaded models through the ad hoc verification path in the experimental `model_manager.cpp` MUST be migrated to `TLOADHASH`-based loading before deploying a build that includes the promoted model manager. Deployments that do not migrate MUST retain the experimental build configuration.

### 14.2 Determinism as a Security Property

Under the T81 security model (RFC-0003), non-deterministic execution is treated as a fault condition rather than benign undefined behavior. This RFC's remediation requirements eliminate the primary sources of non-determinism in the audited AI components. Any deviation from bit-exact determinism in a promoted component MUST be treated as a `SecurityFault` and MUST be recorded in the Axion security audit log.

### 14.3 CLI Attack Surface

`t81_ai_cli.cpp` (C-07), once promoted, exposes the AI backend to user-supplied inputs. The promoted CLI MUST:

- Validate all user-supplied model path arguments as CanonFS `sha3-256:` hash strings before passing them to `TLOADHASH`. Passing unsanitized path strings to the VM is prohibited.
- Sanitize token input sequences for EMBED lookups to prevent out-of-bounds index injection.
- Propagate all `SecurityFault`, `BoundsFault`, and `DecodeFault` conditions to the CLI exit code without swallowing them in error-handling logic.

### 14.4 Permanent Exclusion of External Runtimes

The prohibition on `llama.cpp`, `onnx_runtime`, and similar runtimes in promoted targets is a security control in addition to a determinism control. External runtimes introduce third-party code with independent vulnerability surfaces into the T81 core. This prohibition is normative and MUST NOT be relaxed by any future RFC without a full security review.

---

## 15. References

- [RFC-0002](RFC-0002-deterministic-execution-contract.md) — Deterministic Execution Contract
- [RFC-0003](RFC-0003-axion-safety-model.md) — Axion Safety Model
- [RFC-0004](RFC-0004-canonical-tensor-semantics.md) — Canonical Tensor Semantics
- [RFC-0022](RFC-0022-axion-policy-language.md) — Axion Policy Language Evolution
- [RFC-0025](RFC-0025-policy-gated-tensor-loading.md) — Policy-Gated Tensor Loading via CanonFS
- [RFC-0026](RFC-0026-ai-native-inference-opcodes.md) — AI-Native Inference Opcodes (ATTN, QMATMUL, WLOAD, EMBED, GATHER, SCATTER)
- [RFC-0027](RFC-0027-spec-as-executable.md) — Spec-as-Executable Conformance Model
- [RFC-0031](RFC-0031-deterministic-ai-execution-contract.md) — Deterministic AI Execution Contract
- [RFC-00A0](RFC-00A0-ai-experiment-sandbox.md) — AI Experiment Sandbox and Repository Boundaries
- `spec/supplemental/canonfs-spec.md` — CanonFS Content-Addressed Storage Specification
- `spec/supplemental/axion-policy-grammar.md` — Axion Policy Language Grammar
- `spec/tisc-spec.md` — TISC Instruction Set Architecture Specification
- `spec/t81vm-spec.md` — T81 Virtual Machine Specification
- `docs/architecture/OVERVIEW.md` — T81 Architecture Authority Document
- AI Promotion Audit Report — canonical evidence base for this RFC (internal)
