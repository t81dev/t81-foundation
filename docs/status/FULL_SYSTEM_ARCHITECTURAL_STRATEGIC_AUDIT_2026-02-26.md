# Full-System Architectural & Strategic Audit

Date: 2026-02-26  
Revision: Post-remediation refresh + conformance sprint Phase 3 closure + VM decomposition Phase F closure + rerun on baseline `40488752`  
Scope: `/src`, `/include`, `/spec`, `/docs`, `/book`, CI workflows, governance files, capability contract, opcode/ISA surfaces, VM execution model, Axion policy enforcement, determinism gates, benchmarks, tests, multilingual alignment, roadmaps, release notes.

## Rerun Delta (Baseline `40488752`)

- Added and expanded Phase-3 conformance matrices:
  - `tests/cpp/vm_fault_family_determinism_matrix_test.cpp`
  - `tests/cpp/vm_tloadhash_decodefault_determinism_matrix_test.cpp`
  - `tests/cpp/vm_mixed_workload_conformance_matrix_test.cpp`
  - `tests/cpp/canonfs_read_verify_env_contract_test.cpp`
- Closed remaining planned Phase-3 matrix gaps:
  - deterministic `TLOADHASH` classification coverage for `InvalidHash` vs `CanonFsMiss` vs malformed-object `DecodeFault`
  - mixed workload deterministic deny-path branch coverage via policy instruction-budget gate
  - Axion clause-ordering conformance invariants with deterministic event-signature equivalence checks
- Reduced VM dispatch concentration on Axion opcodes:
  - extracted `AxCheck`/`AxReport`/blocked-privileged-Axion handling into dedicated dispatch lambdas in `core/vm/vm.cpp`
  - preserved fail-closed trap semantics and deterministic Axion event emission behavior
- Reduced VM dispatch concentration on blocked-neural and bitwise opcode families:
  - extracted `TNeuralFwd/TNeuralBwd` blocked handling and `BitAnd/BitOr/BitXor/BitNot/BitShl/BitShr/BitUShr` execution into dedicated dispatch lambdas in `core/vm/vm.cpp`
  - preserved decode-fault guards, fail-closed neural deny semantics, and bitwise register/tag/flag behavior
- Expanded CanonFS integrity and env-contract checks:
  - default read-verify behavior when `T81_CANONFS_READ_VERIFY` is unset
  - explicit env override contract coverage
- Reduced VM dispatch-path concentration by extracting trace/event logging helpers:
  - `core/vm/internal/policy_trace_bridge.hpp`
  - `core/vm/policy_trace_bridge.cpp`
  - wrapper simplification in `core/vm/vm.cpp`
- Prior Phase-2 controls remain in force:
  - VM/Axion conformance matrices (`vm_state_transition_conformance_matrix_test`, `axion_policy_allow_deny_determinism_test`)
  - workload benchmark guardrail in CI
  - translation semantic heading-parity CI gate
- Verification snapshot for rerun baseline:
  - `scripts/ci/run_determinism_slice.sh build`: PASS
  - governance/doc/spec gates: PASS
  - test inventory: `ctest --test-dir build -N` => **280 tests**

## Executive Summary
The repository is a substantial implemented system, not a paper design: compiler, ISA encoding, VM interpreter, threaded trace execution, Axion policy engine, CanonFS, and broad CI/test infrastructure are present. Governance maturity materially improved in this remediation cycle: previously warning-level rows were promoted to machine checks (API lock, spec-code baseline, translation staleness/semantic alignment, benchmark regression, overclaim guardrails), and trace-mode policy granularity is now per-instruction fail-closed.

Determinism claims are defensible for bounded, registry/DCP-defined surfaces. They are not defensible as universal guarantees across all runtime/backend surfaces. Tensor/backend and other non-DCP paths remain explicitly outside cross-platform bit-exact scope. CanonFS now re-verifies hash identity on read by default, reducing storage tamper exposure.

Architectural drift is reduced but not eliminated. The primary residual drift class is documentation consistency depth: root multilingual README section-level parity is now machine-checked, but full semantic parity across all multilingual/non-normative docs is not fully machine-proved. Complexity concentration in VM core remains a maintenance risk.

Current classification remains **Deterministic Runtime Candidate**. The system is stronger than a research prototype and has credible governance/test discipline, but does not yet meet pre-production infrastructure standards due to remaining assurance-depth and boundary-hardening gaps.

---

## 1. Architectural Integrity

### 1.1 Drift Matrix (Spec vs Implementation)

| Area | Spec/Doc Claim | Implementation Evidence | Assessment |
|---|---|---|---|
| Layer stack | Lang -> TISC -> VM -> Axion -> CanonFS | Compiler/IR/VM/Axion/CanonFS paths present | Aligned |
| ISA encoding | Canonical fixed-width 13-byte encoding | `core/isa/encoding.cpp` + freeze check script | Aligned |
| Register contract | `R0-R80` architectural window | VM uses larger implementation bank with bounded architectural window | Aligned (bounded extension model) |
| Trace governance | Policy-governed execution | JIT trace now applies per-instruction policy callback and fail-closed deny | Aligned |
| Unimplemented privileged/async/neural ops | Must not be permissive | Fail-closed `SecurityFault` behavior with tests and registry docs | Aligned |
| CanonFS integrity | Content-addressed integrity | Write hash identity enforced; read re-verify enforced by default (`T81_CANONFS_READ_VERIFY=0` override) | Aligned |

### 1.2 Layer Violation Analysis
- Layering is mostly respected in code paths.
- Experimental surfaces share namespace proximity with stable surfaces; governance boundaries are carried by status/registry and fail-closed behavior rather than strict package-level isolation.
- Non-normative docs still require ongoing semantic coherence maintenance against capability-contract boundaries.

### 1.3 Architectural Risk Score

| Metric | Score (1-10) | Basis |
|---|---:|---|
| Architectural Risk | 4.6 | Major contradictions remain remediated; rerun baseline adds conformance and governance depth, residual risk concentrated in VM control concentration and full-proof gaps |

---

## 2. Determinism Validation

### 2.1 Determinism Confidence Rating
**Moderate (system-wide), Strong (bounded DCP/verified surfaces).**

### 2.2 Failure Surface Analysis
- Bit-exact guarantees are enforced on selected surfaces via CI repro gates (`t81lang`, `t3k`, determinism slice).
- Workload-level determinism now includes micro/meso/mixed/policy-heavy/tensor-access signature tiers with reproducible log artifact output (`build/artifacts/vm_workload_determinism_signatures.log`).
- Trace mode now enforces policy per instruction (prior boundary-only gap closed).
- Float path hardening improved: strict deterministic float profile is defaulted (`T81_STRICT_DETERMINISTIC_FLOAT=ON` -> `T81_DETERMINISTIC`).
- Non-deterministic APIs/surfaces still exist by contract (time, scheduler, allocator addresses, non-DCP tensor/backend).
- CanonFS read path now enforces read-time re-hash verification by default; diagnostic override exists and must remain bounded.

### 2.3 Determinism Threat Map

| Threat | Severity | Likelihood | Current Control |
|---|---|---|---|
| Non-DCP tensor/backend cross-platform drift | High | Medium | Explicit DCP exclusion and capability-contract non-guarantee |
| Host/toolchain variability outside bounded surfaces | Medium | Medium | CI gates + multi-platform matrix |
| CanonFS read-path tamper exposure | Low-Medium | Low-Medium | Default read re-verify + deterministic mismatch faults |
| Overclaim reintroduction in active docs | Medium | Low | CI overclaim guardrail scripts |
| Full reduction-order/proof completeness | Medium | Indeterminate | Partial tests; no full formal closure evidence |

### 2.4 CI Determinism Gate Strength
- Strong for bounded targets.
- Not sufficient to claim universal runtime/backend bit identity.

---

## 3. Instruction Set Coherence (TISC)

### 3.1 Findings
- Opcode registry consistency is strong; freeze integrity script validates canonical range/structure.
- Stub semantics were hardened to fail-closed behavior and mapped to tests.
- Extension pressure remains due to reserved range and evolving experimental surfaces.

### 3.2 ISA Maturity Stage
**Near-Freeze** (Frozen core profile with bounded experimental extension surfaces).

### 3.3 Recommended Next Action
Publish explicit dual profile contract:
1. Frozen Core ISA (strictly versioned and guaranteed)
2. Experimental Extension ISA (explicitly non-DCP unless promoted)

---

## 4. VM & Execution Engine

### 4.1 Findings
- Interpreter is substantial and policy-aware.
- Trace engine is threaded (not native machine-code JIT).
- Trace execution now policy-checks per instruction and fail-closes on deny.
- Deterministic scheduling and broad regression coverage exist.
- VM helper concentration risk was reduced further in Phase F (`runtime_state_helpers`, `gc_helpers`, Axion event bridge extraction), but `core/vm/vm.cpp` remains high-complexity at integration layer scale.

### 4.2 Deliverables

| Metric | Score | Interpretation |
|---|---:|---|
| Runtime Integrity | 7.8/10 | Strong control posture with improved behavioral conformance coverage; bounded by non-DCP surfaces and VM control concentration |
| Production Readiness | Candidate-stage | Not pre-production infrastructure |

---

## 5. Axion Governance & Enforcement

### 5.1 Findings
- Policy parse failures and unknown clauses are fail-closed.
- Deny behavior coverage exists for `AXCHECK` and `AXREPORT`.
- Per-instruction policy in trace mode closes prior enforcement granularity gap.
- Governance matrix is now substantially machine-mapped to CI checks.

### 5.2 Deliverables

| Metric | Rating |
|---|---|
| Governance Strength | Moderate-Strong |
| Risk Classification | Medium |

### 5.3 Missing Enforcement Surfaces
- Behavioral spec-code conformance now includes new executable invariant suites, but remains incomplete at full subsystem and workload breadth.
- Policy behavior coverage now includes a clause matrix suite (`axion_policy_conformance_matrix_test`) in addition to fail-closed parser/invariant tests.
- VM behavioral coverage now includes explicit `TLOADHASH` conformance checks for policy deny (`SecurityFault`), CanonFS miss (`BoundsFault`), tensor-load success, and malformed-object decode-fault paths (`t81_vm_tloadhash_conformance_test`).
- `TLOADHASH` ambiguous payload-layout handling now fails closed (`DecodeFault`) instead of silently no-oping on shape/payload-equal objects.
- VM Phase-E decomposition advanced further: CanonFS tensor object parse+decode gate for `TLOADHASH` is now encapsulated in tensor helpers instead of inline in `vm.cpp`.
- `TLOADHASH` hash normalization/CanonRef parsing is now encapsulated in tensor helpers, reducing VM dispatch-path parsing logic concentration.
- `TLOADHASH` CanonFS fetch/decode result classification (invalid hash vs miss vs decode fault) is now centralized in helper code, leaving VM dispatch focused on trap/event mapping.
- Pre-dispatch Axion deny paths now emit explicit Axion events before `SecurityFault`, improving auditability of fail-closed policy enforcement.
- Tensor unary `TExp` runtime path is now helper-encapsulated and backed by explicit VM-level output assertions in `t81_vm_tensor_test`.
- Additional VM tensor opcode compute paths (`TSiLU`, `TSoftmax`, `TVecAdd/TVecMul`, `TTranspose`, `TMatMul`, `TTenDot`, `TRMSNorm`, `TRoPE`) now route through helper modules; VM-level conformance assertions were expanded for these kernels.
- `TLOADHASH` conformance now also covers malformed CanonTensor headers (invalid format tag and rank overflow) as explicit `DecodeFault` cases.
- Pre-dispatch deny-event observability is now covered by a dedicated VM regression test (`t81_vm_predispatch_policy_deny_logging_test`), reducing risk of silent deny regressions.
- Tensor shape-compatibility checks for `TVec*`, `TMatMul`, `TRMSNorm`, and `TRoPE` are now helper-centralized, reducing branch-complexity concentration in VM dispatch.
- VM tensor trap conformance now includes explicit fault expectations for `TSoftmax`, `TTranspose`, `TRoPE`, `TRMSNorm`, `TVecAdd`, `TMatMul`, `TTenDot`, `TGet`, and `TSet` mismatch/OOB/type paths (`t81_vm_tensor_shape_faults_test`).
- Tensor helper compatibility predicates now have direct regression coverage (`t81_vm_tensor_helper_predicates_test`) to lock shape/compatibility contract behavior independent of dispatch wiring.
- `TGet`/`TSet` success-path and type-behavior conformance is now explicitly covered (`t81_vm_tensor_get_set_conformance_test`) in addition to fault-path coverage.
- Phase F dispatch-slimming closure landed: tensor checked trap-routing, system-register/signature helpers, GC helpers, and Axion event-bridge helpers are extracted from `vm.cpp`.
- Formal proof depth for governance/security invariants is incomplete.
- Host-level containment remains intentionally out-of-scope.

---

## 6. Documentation vs Reality

### 6.1 Overstatement Map
- Root README claim posture was normalized to bounded determinism language.
- Release note caveat language was updated to current fail-closed and strict-float default posture.
- Active doc overclaim reintroduction is now CI-guarded.
- Multilingual alignment is now checked for naming, metadata, staleness, root semantic markers, and required root README section headings; full deep semantic equivalence remains **Indeterminate**.

### 6.2 Documentation Credibility Score

| Metric | Score (1-10) |
|---|---:|
| Documentation Credibility | 8.3 |

### 6.3 Required Corrections
1. Extend multilingual semantic checks beyond root README heading parity into deeper section/body semantic equivalence across doc sets.
2. Continue reducing duplicated/parallel claim surfaces where equivalent contract statements appear in multiple docs.

---

## 7. Code Quality & Engineering Discipline

### 7.1 Findings
- Build/test/toolchain discipline is strong and actively enforced.
- CI quality gate requires key jobs (spec/docs, build/test, determinism slice, static analysis, sanitizers, fuzzing, benchmarks, tritwise determinism).
- Test inventory is broad (280 tests currently discovered by CTest).
- Benchmark regression gating is now required in CI and now includes a VM workload dispatch/native ratio guardrail in addition to SIMD-focused checks.
- Static analysis coverage improved but not full-repo exhaustive.

### 7.2 Deliverables

| Dimension | Assessment |
|---|---|
| Engineering Maturity | Emerging System |

Refactor priority ranking:
1. Continue VM integration-layer decomposition to reduce residual control concentration in `core/vm/vm.cpp`.
2. Expand spec-code conformance from baseline file mapping to behavioral invariant checks.
3. Expand CanonFS integrity coverage to additional corruption/recovery scenarios (beyond current read-verify/tamper tests).
4. Expand benchmark gates beyond SIMD slices to representative workload families.
5. Continue contract-surface deduplication across docs/status/release narratives.

---

## 8. Strategic Position Assessment

### 8.1 Classification
**Deterministic Runtime Candidate**

### 8.2 Why
- Real implemented stack with strong CI/governance hardening.
- Determinism claims are bounded and increasingly enforced.
- Remaining gaps are assurance breadth/depth and non-DCP promotion hardening, not absence of core system capability.

### 8.3 If Development Stopped Today
A technically serious deterministic runtime platform with unusually strong governance/test scaffolding, remembered as a credible candidate architecture that did not fully cross into pre-production assurance completeness.

---

## 9. Hard Truth

### 9.1 Most Serious Structural Risks (Top 5)
1. Non-DCP tensor/backend surfaces remain outside universal bit-exact guarantees.
2. VM core complexity concentration increases regression and maintainability risk.
3. Spec-code assurance is still baseline-oriented, not full behavioral conformance coverage.
4. Multilingual semantic parity is machine-checked at root section depth, but not yet full-depth across all docs.
5. Workload-level determinism validation breadth improved but remains narrower than full production workload space.

### 9.2 Most Valuable Strengths (Top 5)
1. High-rigor CI with hard-fail governance and determinism gates.
2. End-to-end implemented pipeline (compiler -> ISA -> VM -> policy).
3. Fail-closed posture on high-risk unimplemented opcode surfaces.
4. Explicit capability boundaries and DCP/non-DCP taxonomy.
5. Large, active automated test surface (270+ tests), including new semantic/policy/canonfs/determinism conformance suites.

### 9.3 Single Most Important Next Move
Execute **Behavioral Conformance Expansion Sprint (Phase 3)**: scale the new VM/Axion/CanonFS matrix suites from representative slices to subsystem-complete invariant families and workload strata, while reducing `core/vm/vm.cpp` control concentration.

---

## Evidence Snapshot (Non-Exhaustive)
- CI/workflow enforcement: `.github/workflows/ci.yml`
- Governance matrix: `docs/governance/ENFORCEMENT_MATRIX.md`
- Capability boundaries: `docs/reference/CAPABILITY_CONTRACT.md`
- DCP boundary: `docs/product/DETERMINISTIC_CORE_PROFILE.md`
- VM/trace policy integration: `core/vm/vm.cpp`, `runtime/jit/jit_compiler.cpp`, `include/t81/jit/jit.hpp`
- VM decomposition plan baseline: `docs/status/VM_MONOLITH_DECOMPOSITION_PLAN_2026-02-26.md`
- Freeze integrity gate: `scripts/ci/check_tisc_freeze_integrity.py`
- Translation governance: `scripts/governance/check_translation_*.py`
- Workload benchmark gate: `scripts/ci/check_vm_workload_benchmark_regression.py`
- Test inventory: `ctest --test-dir build -N` (280 tests)

## Audit Notes
- Items with insufficient direct evidence are explicitly marked **Indeterminate**.
- This report is descriptive and evidence-based; normative authority remains `/spec`.
