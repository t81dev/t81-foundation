# RFC-0031: Deterministic AI Execution Contract

**Status:** accepted
**Type:** standards-track
**Applies-To:** RFC-0002, RFC-0003, RFC-0004, RFC-0025, RFC-0026, RFC-0027, RFC-00A0
**Created:** 2026-03-07
**Updated:** 2026-03-15
**Supersedes:** —
**Superseded-By:** —
**Discussion:** —

---

## Abstract

This RFC defines the deterministic execution contract for AI-related capabilities already introduced elsewhere in T81 architecture. It does not redefine tensor semantics, opcode surfaces, storage architecture, or governance foundations; instead, it composes them into a single promotion and execution contract for Deterministic Core Profile–compliant AI workloads.

---

## Scope and Non-Goals

### Scope

This RFC normatively defines:

1. **Deterministic AI arithmetic requirements** under the Deterministic Execution Contract (RFC-0002)
2. **Execution obligations** for existing AI opcodes (RFC-0026) under interpreter-first semantics
3. **Axion-governed model execution path** using existing verification machinery (RFC-0025, RFC-0003)
4. **Promotion contract** for `/experiments/ai` subsystems through RFC-00A0 lifecycle

### Non-Goals

This RFC does not:

- Redefine RFC-0004 tensor semantics
- Redefine RFC-0026 opcode syntax or opcode inventory  
- Replace RFC-00A0 promotion lifecycle
- Introduce nondeterministic execution paths into core
- Define new tensor model or storage architecture
- Extend cognition tier system
- Make performance promises or optimization claims

---

## Relationship to Existing RFCs

This RFC is **normatively subordinate** to the following RFCs and composes their requirements:

### RFC-0002: Deterministic Execution Contract
All AI execution MUST satisfy the bit-exact determinism guarantees defined in RFC-0002 §2. Any AI operation that violates the DEC MUST result in a Determinism Fault.

### RFC-0003: Axion Safety Model  
AI model loading and execution MUST be supervised by Axion according to the safety model defined in RFC-0003. All privileged AI operations MUST emit the required Axion events.

### RFC-0004: Canonical Tensor Semantics
AI tensor operations MUST conform to the canonical tensor model defined in RFC-0004. No AI operation may introduce non-canonical tensor representations.

### RFC-0025: Policy-Gated Tensor Loading
AI model loading MUST use the TLOADHASH instruction and policy verification defined in RFC-0025. Model hashes MUST be validated against the `allowed-tensor-hashes` policy directive.

### RFC-0026: AI-Native Inference Opcodes
AI inference MUST use the opcode surface defined in RFC-0026. This RFC defines execution semantics for those opcodes under DCP but does not introduce new opcodes.

### RFC-0027: Spec-as-Executable
AI execution contract MUST include spec-as-executable conformance programs as required by RFC-0027. All AI execution rules MUST be verifiable through T81Lang programs.

### RFC-00A0: AI Experiment Sandbox
AI subsystem promotion MUST follow the lifecycle defined in RFC-00A0. No AI feature may bypass the experimental → extension → core promotion pipeline.

---

## Deterministic AI Arithmetic Contract

### Fixed-Point Semantics

All AI arithmetic operations MUST use the fixed-point semantics defined in RFC-0004 §3.2. Operations MUST:

- Produce bit-exact results for identical inputs across all platforms
- Use canonical rounding modes (round toward negative infinity)
- Handle overflow according to RFC-0004 §3.2.3 deterministic overflow rules

### Soft-Float Requirements

AI operations involving floating-point arithmetic MUST:

- Use T81Float soft-float implementation exclusively (RFC-0026 §5.15.1)
- Never use hardware FPU instructions for any AI computation
- Produce identical results across x86-64, ARM64, and other supported platforms

### Quantization Invariants

AI quantization operations MUST satisfy:

```
dequantize(quantize(x, scale), scale) == x  // for all x in representable range
```

Quantization scale factors MUST be stored as canonical T81Float values. Quantization MUST be deterministic: identical inputs with identical scales produce identical quantized outputs.

### Overflow and Underflow Behavior

AI arithmetic MUST handle overflow/underflow according to RFC-0004 §3.2.3:

- Overflow to canonical saturation values
- Underflow to canonical zero
- No silent wrapping or undefined behavior
- All overflow events MUST emit Axion trace events

---

## Execution Contract for Existing AI Opcodes

### Interpreter-First Semantics

The T81VM interpreter implementation is the **canonical reference** for AI opcode execution. All JIT implementations MUST produce bit-exact results identical to the interpreter for identical inputs and VM state.

### ATTN Opcode Execution

ATTN (RFC-0026 §5.15.1) MUST satisfy:

- Use T81Float soft-float for all intermediate computations
- Apply canonical square-root algorithm defined in RFC-0004 §4.2
- Emit `attn guard` Axion event with shape metadata before execution
- Produce deterministic output for identical Q, K, V tensors across all platforms

### QMATMUL Opcode Execution

QMATMUL (RFC-0026 §5.15.2) MUST satisfy:

- Perform dequantization before multiplication (canonical order)
- Use T81Float accumulation with deterministic rounding
- Verify weight tensor provenance via Axion before execution
- Emit `qmatmul guard` Axion event with policy validation result

### WLOAD Opcode Execution

WLOAD (RFC-0026 §5.15.3) currently satisfies only the phase-1 audited
materialization contract. Full policy-gated CanonFS-backed loading remains a
follow-on promotion item and therefore is not yet a repo-level invariant.

Promotion target:

- Validate model hash against RFC-0025 `allowed-tensor-hashes` policy
- Use CanonFS content-addressed storage exclusively
- Emit `meta slot axion event segment=meta action=WeightLoad` before materializing tensor handle
- Fail with SecurityFault if policy validation denies access

### EMBED, GATHER, SCATTER Execution

EMBED, GATHER, SCATTER opcodes (RFC-0026 §5.15.4-5.15.6) MUST satisfy:

- Perform deterministic bounds checking before any memory access
- Maintain deterministic ordering for gather/scatter operations
- Detect and prevent SCATTER aliasing as specified in RFC-0026 §5.15.6
- Emit appropriate Axion guard events for each operation type

### Deterministic Memory Model

AI tensor memory MUST follow RFC-0002 §5 deterministic memory model:

- All tensor allocations MUST be recorded in Axion trace
- Tensor pool MUST be deterministic (RFC-0025 §3.3.4)
- No implicit memory compaction during AI execution
- All tensor handle assignments MUST be reproducible

---

## Axion-Governed Model Execution

### Model Verification Flow

AI model execution MUST follow this verification sequence:

```
1. Model hash lookup in Axion policy
2. CanonFS content verification (RFC-0025)
3. Tensor deserialization with canonical format validation
4. Axion policy gate approval
5. Model handle allocation in deterministic tensor pool
6. Execution trace emission with model metadata
```

### Policy Enforcement Requirements

Axion MUST enforce the following policies for AI execution:

- **allowed-tensor-hashes**: Whitelist of permitted model hashes
- **max-tensor-rank**: Maximum tensor rank per cognitive tier
- **ai-execution-enabled**: Boolean flag enabling AI operations
- **determinism-validation-required**: Whether to validate determinism automatically

### Event Emission Requirements

AI execution MUST emit the following Axion events:

- `model_load success|failure hash=<hash> reason=<reason>`
- `attn guard shape=<shape> tier=<tier>`
- `qmatmul guard policy=<allow|deny> scale=<scale>`
- `tensor_alloc rank=<rank> size=<size> handle=<handle>`

All events MUST follow the canonical format defined in RFC-0003 §4.

---

## Promotion Contract for `/experiments/ai`

### Lifecycle Adherence

AI subsystems MUST follow the promotion lifecycle defined in RFC-00A0:

```
/experiments/ai
↓ determinism validation
↓ Axion integration  
↓ core primitives
↓ ISA promotion
```

### Stage 1: Experiment Requirements

Experimental AI features MUST satisfy:

- Location in `/experiments/ai/` directory structure
- No modifications to protected core directories
- Opt-in build flags via `T81_ENABLE_AI_EXPERIMENTS`
- Basic functionality demonstration
- No determinism regressions in existing tests

### Stage 2: Extension Requirements

AI extensions MUST satisfy:

- Stable API surface with backward compatibility
- Comprehensive test coverage including determinism validation
- Full RFC-0027 spec-as-executable conformance programs
- Documentation complete with normative requirements
- Integration with existing Axion policy framework

### Stage 3: Core Integration Requirements

AI core integration MUST satisfy:

- Proven architectural value with measurable benefits
- No determinism regression across all platforms
- Performance benefits demonstrated through benchmarks
- Full RFC approval process completion
- Integration with existing opcode surface without conflicts

### Determinism Validation Gates

Each promotion stage MUST require:

- Cross-platform determinism validation (x86-64, ARM64)
- Axion trace completeness verification
- Spec-as-executable program compliance
- No new nondeterministic code paths

---

## Conformance and Verification

### Spec-as-Executable Requirements

AI execution MUST include the following spec programs per RFC-0027:

```
spec/conformance/ai/
  attn-determinism.t81           # ATTN produces bit-exact output
  qmatmul-scale-order.t81        # QMATMUL dequantize-then-multiply ordering
  wload-policy-gate.t81          # WLOAD with invalid policy → SecurityFault
  embed-bounds-check.t81          # EMBED bounds checking enforcement
  scatter-aliasing-detect.t81    # SCATTER aliasing detection
```

Each program MUST:

- Reference the normative RFC section it validates
- Use `@axion_verify` on `main()` for trace recording
- Produce single boolean pass/fail result
- Include AI-derivable metadata for automated testing

### Cross-Platform Determinism Tests

AI implementations MUST pass determinism validation:

- Execute identical AI workloads on x86-64 and ARM64
- Compare all tensor outputs bit-for-bit
- Verify identical Axion trace sequences
- Validate identical fault behavior for error conditions

### Automated Validation

CI pipeline MUST include:

- `spec_conformance_ai` target running all AI spec programs
- Cross-platform determinism matrix testing
- Axion trace completeness validation
- Promotion gate compliance verification

---

## Security Considerations

### Model Supply Chain Security

AI model loading MUST enforce:

- Cryptographic hash verification via RFC-0025
- CanonFS content-addressed storage exclusively
- Axion policy validation before model materialization
- Audit trail for all model load operations

### Determinism as Security Property

Any deviation from deterministic AI execution MUST be treated as a security violation:

- Nondeterministic results trigger SecurityFault
- Axion MUST terminate execution on determinism violations
- All determinism violations MUST be recorded in security audit log

### Resource Protection

AI execution MUST respect resource limits:

- Tensor rank limits per cognitive tier
- Memory allocation ceilings enforced by Axion
- No bypass of policy-based resource constraints

---

## Implementation Guidance

### Phase 1: Foundation (2026-04-01)

- Implement deterministic AI arithmetic validation tests
- Add Axion event emission for AI opcodes
- Create spec-as-executable conformance programs
- Establish cross-platform determinism test matrix

### Phase 2: Integration (2026-05-01)

- Integrate RFC-0025 policy-gated loading with AI opcodes
- Implement promotion gate validation automation
- Add CI targets for AI determinism validation
- Complete Axion governance integration

### Phase 3: Production (2026-06-01)

- Full RFC-00A0 promotion pipeline implementation
- Cross-platform determinism validation in CI
- Complete spec-as-executable conformance suite
- Documentation and developer tooling

### Acceptance Criteria

- All AI spec programs pass on x86-64 and ARM64
- Cross-platform determinism validation passes for ATTN, QMATMUL, WLOAD
- Axion trace completeness verified for AI execution paths
- Promotion gate automation enforces RFC-00A0 lifecycle
- No determinism regressions in existing test suite

---

## Conclusion

This RFC establishes the deterministic execution contract that enables AI capabilities while preserving the T81 Foundation's core identity as a bit-exact deterministic computing substrate. By composing existing RFCs rather than redefining them, it provides a clear path for AI development that maintains architectural integrity and safety guarantees.

The contract ensures that AI inference, regardless of complexity or model size, produces identical results across all supported platforms while remaining fully supervised by the Axion safety kernel. This preserves the deterministic foundation that enables verifiable computation and safe cognitive reasoning.
