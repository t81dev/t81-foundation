# VM Monolith Decomposition Plan

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [VM Monolith Decomposition Plan](#vm-monolith-decomposition-plan)
  - [Objective](#objective)
  - [Constraints](#constraints)
  - [Target Decomposition (Current Baseline)](#target-decomposition-current-baseline)
  - [Phase Plan](#phase-plan)
  - [Verification Gates Per Phase](#verification-gates-per-phase)
  - [Risk Register](#risk-register)
  - [Exit Criteria](#exit-criteria)
  - [Exit Evidence](#exit-evidence)

<!-- T81-TOC:END -->


Date: 2026-02-26  
Scope: `core/vm/vm.cpp`  
Status: Completed (Phase A+B+C+D+E+F complete)

## Objective
Reduce regression blast radius and improve assurance depth by decomposing `core/vm/vm.cpp` into testable modules while preserving byte-for-byte runtime behavior on DCP surfaces.

## Constraints
- No opcode semantic changes during extraction phases.
- All existing VM/Axion/trace/determinism tests must remain green at each phase.
- New modules must keep deterministic trap strings and trace reason strings stable.
- Any behavior-changing fixes must land in separate commits after decomposition steps.
- Pre-dispatch policy denies must remain observable in Axion logs (no silent deny paths).

## Target Decomposition (Current Baseline)
1. `core/vm/vm.cpp`  
Purpose: dispatch integration, trap routing, and opcode-family orchestration.
2. `core/vm/value_ops.cpp`  
Purpose: scalar arithmetic helpers.
3. `core/vm/memory_segments.cpp`  
Purpose: memory/stack helpers and layout-aware checks.
4. `core/vm/policy_trace_bridge.cpp`  
Purpose: syscall context assembly, deterministic reason formatting, Axion event recording.
5. `core/vm/tensor_helpers.cpp`  
Purpose: tensor decode/load/compute kernels and checked tensor trap mapping helpers.
6. `core/vm/runtime_state_helpers.cpp`  
Purpose: system-register synchronization and deterministic lineage/entropy/constitutional signatures.
7. `core/vm/gc_helpers.cpp`  
Purpose: GC mark/sweep traversal and heap compaction helpers.
8. `core/vm/tier_limits.cpp`  
Purpose: cognitive-tier limit/threshold helpers.

## Phase Plan
1. Phase A: Extraction scaffolding
- Introduce internal headers under `core/vm/internal/`.
- Move pure helper functions first (no opcode handler movement).
- Add compile-time include boundaries for each helper module.
2. Phase B: Stateless handler extraction
- Move deterministic arithmetic/comparison handlers to `vm_value_ops.cpp`.
- Keep exact control flow and trap code paths.
3. Phase C: Memory and stack extraction
- Move `Load/Store/Push/Pop/Stack*` helpers to `vm_memory_segments.cpp`.
- Preserve segment event strings.
4. Phase D: Policy and trace bridge extraction
- Move policy syscall context assembly + trace emission helpers.
- Add explicit deterministic signature tests for trace reasons.
5. Phase E: Tensor and extended opcode extraction
- Move tensor and weights handlers.
- Keep non-DCP boundaries unchanged.
 - Completed: shared native tensor decode path (`WeightsTensorHandle` promotion + `TLoadHash` decode) extracted to `tensor_helpers`; CanonFS tensor-object parsing, decode-or-fail gate, hash-ref parsing, and CanonFS fetch/decode status classification moved out of `vm.cpp`; tensor compute paths (`TSqrt`, `TExp`, `TSiLU`, `TSoftmax`, `TVecAdd/TVecMul`, `TTranspose`, `TMatMul`, `TTenDot`, `TRMSNorm`, `TRoPE`) now call helper modules; `TGet/TSet/TNew/TID` core operations and tensor shape-compatibility predicates are helper-centralized.
6. Phase F: Final dispatch slimming
- Reduce `vm.cpp` to dispatch integration and module wiring.
 - Completed: tensor checked trap-routing (`TVec*`, `TTranspose`, `TMatMul`, `TTenDot`, `TGet`, `TSet`) now helper-driven; system-register/signature computations extracted to `runtime_state_helpers`; GC mark/sweep and heap compaction extracted to `gc_helpers`; Axion event push/meta-slot/structured-recording extracted to `policy_trace_bridge`; segment-kind resolution moved to `memory_segments` helper.

## Verification Gates Per Phase
- `ctest --test-dir build -R "t81_vm_.*|vm_.*|axion_.*|jit_.*|determinism.*|canonfs_.*"`
- `scripts/ci/run_determinism_slice.sh build`
- `python3 scripts/ci/check_tisc_freeze_integrity.py`
- `python3 scripts/governance/check_spec_code_alignment_baseline.py`

## Risk Register
1. Trace reason drift due to string formatting movement.
Mitigation: preserve helper for canonical string assembly and add snapshot assertions.
2. Trap mapping drift due to handler extraction order.
Mitigation: freeze trap expectations with semantic family tests.
3. Hidden coupling between policy checks and dispatch order.
Mitigation: keep per-instruction policy hook call order unchanged and assert via policy matrix tests.
4. Performance regressions from over-fragmentation.
Mitigation: benchmark after each major phase and keep hot helpers inline where needed.

## Exit Criteria
- `core/vm/vm.cpp` reduced to integration layer with materially lower cyclomatic concentration.
- No regressions in determinism and governance gate suites.
- Updated architecture docs reflect new module boundaries with evidence links.
- Pre-dispatch deny-path observability remains locked by explicit VM tests.
- Tensor shape-fault behavior remains locked by explicit VM trap-conformance tests.

## Exit Evidence
- `core/vm/vm.cpp` reduced from 5294 to 5021 LOC in this decomposition cycle; high-risk helper clusters moved to dedicated modules.
- Determinism and governance gates passed on post-Phase-F baseline:
  - `scripts/ci/run_determinism_slice.sh build`
  - `python3 scripts/ci/check_tisc_freeze_integrity.py`
  - `python3 scripts/governance/check_spec_code_alignment_baseline.py`
  - focused VM/Axion/CanonFS/JIT conformance suites (`ctest` regex slices)
- Pre-dispatch deny-path observability remains covered by `t81_vm_predispatch_policy_deny_logging_test`.
- Tensor shape and access trap conformance remains covered by `t81_vm_tensor_shape_faults_test` and `t81_vm_tensor_get_set_conformance_test`.
