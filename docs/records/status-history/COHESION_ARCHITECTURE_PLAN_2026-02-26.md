# Cohesion Architecture Plan (Operational, CI-Enforceable)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Cohesion Architecture Plan (Operational, CI-Enforceable)](#cohesion-architecture-plan-operational-ci-enforceable)
  - [1) Canonical Authority Model](#1-canonical-authority-model)
    - [Authority hierarchy (current enforced model)](#authority-hierarchy-current-enforced-model)
    - [Conflict resolution rules](#conflict-resolution-rules)
  - [2) Deterministic Profile Boundary](#2-deterministic-profile-boundary)
    - [Deterministic Profile v1 schema (operational)](#deterministic-profile-v1-schema-operational)
    - [Enforcement mapping (rule -> CI)](#enforcement-mapping-rule-->-ci)
  - [3) Extension Surface Containment](#3-extension-surface-containment)
    - [Extension Activation Protocol](#extension-activation-protocol)
    - [Fail-closed invariants checklist](#fail-closed-invariants-checklist)
  - [4) Governance Enforcement Convergence](#4-governance-enforcement-convergence)
    - [Machine-verifiability matrix (current reality)](#machine-verifiability-matrix-current-reality)
    - [Governance hardening roadmap](#governance-hardening-roadmap)
  - [5) Spec ↔ Implementation Synchronization Mechanism](#5-spec-↔-implementation-synchronization-mechanism)
    - [Sync enforcement plan](#sync-enforcement-plan)
  - [6) Trace-Mode Integrity Strategy](#6-trace-mode-integrity-strategy)
    - [Trace integrity checklist](#trace-integrity-checklist)
    - [Equivalence proof surface map](#equivalence-proof-surface-map)
  - [7) Translation & Documentation Drift Prevention](#7-translation-&-documentation-drift-prevention)
    - [Documentation cohesion policy](#documentation-cohesion-policy)
    - [CI staleness enforcement logic](#ci-staleness-enforcement-logic)
  - [8) System Invariants (Non-Negotiable)](#8-system-invariants-non-negotiable)
  - [9) Cohesion Scoring Model](#9-cohesion-scoring-model)
    - [Quantitative model](#quantitative-model)
    - [Thresholds](#thresholds)
  - [Prioritized 90-Day Cohesion Execution Plan](#prioritized-90-day-cohesion-execution-plan)

<!-- T81-TOC:END -->


## 1) Canonical Authority Model

### Authority hierarchy (current enforced model)

| Rank | Authority class | Canonical artifacts | Enforcement state |
|---|---|---|---|
| A0 | Normative spec | `/spec/**` including `tisc-spec.md`, `t81vm-spec.md`, `determinism-profile.md` | Declared + partially machine-checked |
| A1 | Governance normative controls | `SPEC_AUTHORITY_MODEL.md`, `FREEZE_ENFORCEMENT.md`, `ENFORCEMENT_MATRIX.md` | Mostly machine-checked |
| A2 | Determinism boundary controls | `DETERMINISM_SURFACE_REGISTRY.md`, `DETERMINISTIC_CORE_PROFILE_v1.md`, `docs/product/DETERMINISTIC_CORE_PROFILE.md` | Partially machine-checked |
| A3 | Implementation-bound status | `VERIFIED_SURFACE_AUDIT.md`, `IMPLEMENTATION_MATRIX.md` | Checked for consistency markers |
| A4 | Derivative descriptive docs | `/docs/**`, `/book/**`, READMEs | Guardrail checks only |

### Conflict resolution rules

1. `spec/*` wins over all non-spec docs.
2. Governance docs cannot override `spec/*`; they can only constrain release/admission.
3. If code diverges from stable spec, code is defective unless spec is explicitly Draft.
4. Any semantic ISA change requires major version + freeze-break protocol.
5. If deterministic claim appears outside registry/DCP, claim is invalid.

## 2) Deterministic Profile Boundary

### Deterministic Profile v1 schema (operational)

```yaml
profile_id: deterministic-v1
encoding: tisc-fixed-13-byte
register_window:
  architectural: R0-R80
  non_portable_extension: R81+
included_subsystems:
  - core/isa
  - core/vm interpreter semantics
  - core/types canonical numeric + soft-float wrappers
  - canon serialization surfaces
excluded_subsystems:
  - runtime/jit (promotion-gated)
  - experimental/tiers/*
  - experimental/distributed/*
  - experimental/hanoi/*
  - third_party model runtime paths (non-DCP)
deterministic_opcode_subset:
  include: [0..173]
  exclude:
    - 123..126   # NSend/NRecv/VWait/VYield
    - 133..157   # Sym*, Refl*, Recurse/Contract/Entropy/Depth/Terminate, Merge/Gossip/TickSync/Coherence/DistSeal, Inf*
    - 159..161   # AxSign/AxLineage/AxCanon
    - 165..166   # TNeuralFwd/TNeuralBwd
fail_closed_required:
  - unknown opcode decode
  - excluded opcode execution
```

### Enforcement mapping (rule -> CI)

| Profile rule | Enforcer |
|---|---|
| ISA frozen (0..173, reserved range intact) | `scripts/ci/check_tisc_freeze_integrity.py` in `spec-and-docs` |
| Unknown opcode rejected | `core/isa/encoding.cpp` decode + `tisc_opcode_matrix_test` |
| Stub/privileged/neural/async fail-closed | `vm_stubbed_*_fail_closed_test`, `test_vm_neural_opcodes` |
| Determinism reproducibility | `t81lang_repro_gate.py`, `t3k_repro_gate.py`, `run_determinism_slice.sh` |
| Overclaim prevention | `check_overclaim_guardrails.py` |
| DCP boundary labels | `check_cognitive_tier_boundary.py` |

## 3) Extension Surface Containment

### Extension Activation Protocol

1. Stub opcode remains `Trap::SecurityFault` with explicit Axion deny reason.
2. Activation requires:
   - spec semantic section in `spec/tisc-spec.md`,
   - registry status upgrade entry,
   - deterministic replay evidence in CI,
   - cross-arch hash stability where applicable.
3. Until all evidence exists, status remains `Stub` and profile-excluded.
4. Any active execution path without completed protocol is a hard-fail violation.

### Fail-closed invariants checklist

- Unknown opcode never executes.
- Stub privileged opcode never executes.
- Stub async/network opcode never executes.
- Stub neural opcode never executes.
- Unknown policy clause is parse-denied.
- Policy deny on `AXREPORT` and `AXCHECK` traps immediately.

## 4) Governance Enforcement Convergence

### Machine-verifiability matrix (current reality)

| Policy area | Machine-verifiable now | Status |
|---|---|---|
| Freeze integrity, workflow pinning/permissions, root/docs/license/artifact hygiene, translation staleness, API semver lock, fail-closed opcode tests | Yes | Hard-fail in `ci.yml` |
| Spec-implementation semantic drift (`spec_impl_drift_check.py`) | Partially (high false positives) | Governance debt |
| DCP integrity job (`deterministic-core-profile-check`) | Yes but `continue-on-error` | Warning-only debt |
| Architecture invariant job | Yes but `continue-on-error` | Warning-only debt |
| Translation metadata header (`source_commit`, `translation_status`) | Declared but not enforced | Governance debt |
| Branch protection requiring cross-arch hash jobs | External setting, not repo-enforced | Governance debt |

### Governance hardening roadmap

1. Remove `continue-on-error` from DCP and architecture invariant jobs.
2. Wire calibrated `spec_impl_drift_check.py` as blocking for `spec/*` changes.
3. Replace `check_translation_metadata.py` with header metadata parser enforcement.
4. Make cross-arch bit-identity jobs dependencies of `quality-gate` (or explicitly required check contract in repo policy artifact).
5. Eliminate non-executable CI references (`make audit-governance` currently missing target).

## 5) Spec ↔ Implementation Synchronization Mechanism

### Sync enforcement plan

| Sync axis | Current check | Required closure |
|---|---|---|
| Spec opcode set ↔ opcode registry | Freeze integrity script parses `spec/tisc/opcode-registry.md` | Add check: registry values must equal `include/t81/isa/opcodes.hpp` numeric map |
| Opcode registry ↔ VM dispatch | Indirect via tests | Add check: enumerate `kAllOpcodes` and assert dispatch handling classification (`implemented`, `fail_closed`, `decode_fault`) |
| Encoding format | `core/isa/encoding.cpp` fixed 13-byte decode | Add spec text fingerprint check for 13-byte invariants |
| Register model guarantees | Spec says architectural R0-R80, impl has 243 | Add check: profile mode rejects writes/reads above R80 unless non-portable flag enabled |
| Documentation drift | baseline file-existence checks | Add semantic marker checks per opcode/status row |

## 6) Trace-Mode Integrity Strategy

### Trace integrity checklist

- Per-instruction policy evaluation occurs inside trace execution path.
- Trace entry/exit/deopt reasons are logged deterministically.
- JIT and interpreter produce equal final registers/tags/trace reasons for covered workloads.
- Policy deny during trace produces `SecurityFault` with deterministic reason.
- Trace mode remains non-DCP until equivalence surface is fully proven.

### Equivalence proof surface map

| Surface | Evidence |
|---|---|
| Boundary logging invariants | `jit_trace_equivalence_test`, `jit_tensor_trace_equivalence_test` |
| Per-instruction policy parity | `vm_jit_per_instruction_policy_test` |
| Deterministic execution parity samples | JIT equivalence tests + determinism slice |
| Full semantic equivalence for all opcodes | Not proven -> governance debt |

## 7) Translation & Documentation Drift Prevention

### Documentation cohesion policy

1. English remains canonical.
2. Translation staleness gate is blocking (`>30 days` or `>10 commits`).
3. Translation metadata header enforcement must be mandatory and parsed.
4. Claim drift gate scans all active docs surfaces.
5. Any deterministic/security claim without backing registry/policy artifact is blockable.

### CI staleness enforcement logic

- Keep `check_translation_staleness.py` as hard-fail.
- Upgrade metadata check from README link-token presence to per-file header validation.
- Add claim-to-evidence linkage check: deterministic claim requires reference token to registry/DCP.

## 8) System Invariants (Non-Negotiable)

| Invariant | CI hard-fail mapping |
|---|---|
| No silent permissive stub execution | fail-closed opcode tests in `build-and-test` |
| No unknown policy clause acceptance | `vm_policy_parse_fail_closed_test` |
| No tag-pinned workflows | `audit_workflow_actions.py --max-tagged 0 --max-unknown 0` |
| No ISA semantic changes without profile/version governance | `check_tisc_freeze_integrity.py` + release protocol artifacts |
| No deterministic claim without registry inclusion | `check_overclaim_guardrails.py` + new claim-link check (debt until added) |
| No undocumented API drift for `include/t81/**` | `check_public_api_semver.py` |
| No root structure drift | `check_root_structure.py` |

## 9) Cohesion Scoring Model

### Quantitative model

- `SpecImplAlignment` = passing sync checks / total sync checks.
- `GovernanceCoverage` = hard-fail governance rules / total governance rules.
- `DeterministicProfileCoverage` = enforced profile rules / declared profile rules.
- `CIEnforcementCoverage` = required structural gates in quality path / total structural gates.
- `TranslationSync` = passing translation pairs / total translation pairs.
- `CohesionScore` = weighted sum:
  - 30% SpecImplAlignment
  - 25% GovernanceCoverage
  - 20% DeterministicProfileCoverage
  - 15% CIEnforcementCoverage
  - 10% TranslationSync

### Thresholds

| Mode | CohesionScore | Hard requirements |
|---|---|---|
| Research | `>=60` | No critical security invariant failure |
| Candidate | `>=75` | No warning-only on DCP boundary jobs |
| Pre-Production | `>=85` | Spec drift blocking + translation metadata blocking active |
| Infrastructure | `>=95` | Zero governance debt in critical path, cross-arch gates required in quality gate |

---

## Prioritized 90-Day Cohesion Execution Plan

1. **Days 1-15 (Critical hardening)**
   - Remove warning-only status from `deterministic-core-profile-check` and `architecture-invariants`.
   - Fix CI target mismatch (`make audit-governance` replacement with executable script call).
   - Promote cross-arch identity jobs into quality-gate dependency chain.

2. **Days 16-35 (Spec/impl synchronization)**
   - Replace or repair `spec_impl_drift_check.py` with low-noise parser and wire as blocking for `spec/*` changes.
   - Add opcode tri-sync check: `spec/tisc-spec.md` ↔ `spec/tisc/opcode-registry.md` ↔ `include/t81/isa/opcodes.hpp` ↔ dispatch classification report.

3. **Days 36-55 (Deterministic profile purity)**
   - Commit deterministic opcode subset manifest and enforce deny-on-excluded in profile mode.
   - Add register-window gate enforcing R0-R80 portability contract.
   - Add claim-link checker for deterministic/security claims in docs.

4. **Days 56-75 (Trace integrity closure)**
   - Expand JIT equivalence corpus to include fault parity and policy parity checkpoints.
   - Define explicit non-DCP trace mode gate label and enforce in docs/status synchronization checks.

5. **Days 76-90 (Governance debt burn-down and scoring rollout)**
   - Enforce translation metadata headers per-file.
   - Generate cohesion score artifact in CI (`artifacts/ci_reports/cohesion_score.json`).
   - Set merge floor to Candidate-mode thresholds; block regression below floor.
