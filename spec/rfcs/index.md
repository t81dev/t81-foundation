# T81 RFC Index

This index tracks RFC status and intended disposition.

## Status Legend

- `draft`: active authoring
- `proposed`: under formal review
- `accepted`: design accepted, pending/ongoing integration
- `integrated`: merged into normative specs
- `superseded`: replaced by a newer RFC
- `rejected`: closed without adoption

## RFC Catalog

| RFC | Title | Status | Notes |
| :--- | :--- | :--- | :--- |
| RFC-0000 | T81 Base-81 Ternary Computing Stack | accepted | Umbrella RFC: all component RFCs (0001–0033, 00B0–00B7, DPE-0001–0009) accepted; all 23/23 AC met |
| RFC-0001 | Architecture Principles | accepted | All §1–§3 principles satisfied: deterministic execution (RFC-0002), canonical data (tensor/float/fraction), ternary-native ISA, safe recursion (RFC-0003 RCS), no-UB policy, privileged boundary mediation; 49/49 axion+ethics+tier tests pass |
| RFC-0002 | Deterministic Execution Contract | accepted | Cross-layer invariants proven: 27-program conformance suite, EvidenceCollector, full stack; §11 fulfilled |
| RFC-0003 | Axion Safety Model | accepted | All 9 criteria met: AXREAD/AXSET/AXVERIFY mediation, fail-closed policy parse, deterministic audit log, tier-supervision invariant, instruction ceiling, CanonFS observability, AI-model hooks, interrupt rate-limit; 49/49 axion+ethics+tier+policy tests pass |
| RFC-0004 | Canonical Tensor Semantics | accepted | §2.1–2.5 met: immutable shape tuples, 1-based handle pool, TVECADD/TMATMUL/elementwise with shape guards, IR lowering, Axion shape-metadata hooks; 30+ tensor tests pass |
| RFC-0005 | TISC v0.4 Extensions | accepted | All 7 criteria met: structural opcodes (46–49), VLoad/VStore/VAdd/VFma, ChkShape, ReadIsaVersion, Axion trace; 10/10 tests pass |
| RFC-0006 | Deterministic GC | accepted | All 6 criteria met: mark_and_sweep, compact_heap, GcSafepoint opcode, byte-threshold (3^12), Axion events, policy veto; 8/8 tests pass |
| RFC-0007 | T81Lang Standard Library | accepted | All 7 criteria met: arith/tensor/option/result/axsafe_io modules in `kBuiltinTable`; MakeOptionSome/None/MakeResultOk/Err opcodes in TISC; tier+purity rules enforced by SA; ISA-version freeze replaces per-module versioning |
| RFC-0008 | Formal Verification Harness | superseded | Superseded by RFC-0027 (spec-as-executable is the concrete realization) |
| RFC-0009 | Axion Policy Language (APL) | superseded | Superseded by RFC-0022; s-expression policy syntax, `PolicyEngine::evaluate()`, and guard/loop predicates all implemented; forward path is RFC-0022 |
| RFC-0010 | TISC Float/Fraction Ops | accepted | Caveat resolved: `FDIV` is IEEE 754 exactly-rounded; transcendentals route through `t81_soft_math` (RFC-0030); no host libm dependency |
| RFC-0011 | T81Lang Grammar Modernization | accepted | All 7 AC met: annotations, generics, expanded types, var/break/continue, record/enum, @module namespacing, legacy artifact superseded; realized via RFC-0003/0004/0007/0015/0029 (2026-03-16) |
| RFC-0012 | Ternary Tensor Quantization | superseded | Superseded by RFC-0026 (QMATMUL covers quantized matmul path) |
| RFC-0013 | Ternary Matmul | superseded | Superseded by RFC-0026 (QMATMUL) |
| RFC-0014 | Neural Primitives | superseded | Superseded by RFC-0026 (ATTN + EMBED cover the neural primitive surface) |
| RFC-0015 | Agentic Constructs | accepted | First-class `agent`/`behavior` declarations, `AGENT_INVOKE` opcode, Axion audit; 9/9 AC met (2026-03-15) |
| RFC-0016 | SIMD Limb | superseded | Superseded by RFC-0017; `t81::simd` namespace retained for internal helpers only |
| RFC-0017 | Introduce T81 Native | accepted | All 6 criteria met: `t81::T81` struct, negation (AVX2+scalar), addition (carry-map+prefix scan), conversion, master header, fallback guard; 43 assertions pass |
| RFC-0018 | T81 Native SIMD Arithmetic | accepted | All 6 criteria met: AVX2 carry-map addition, scalar fallback, multiplication, subtraction, benchmarks, AddEntry table; proved by `t81_native_property_test` + `t81_simd_add_helpers_test` |
| RFC-0019 | Axion Match Logging | accepted | Runtime/spec/CLI depend on canonical match metadata and guard-audit strings |
| RFC-0020 | Axion Segment Trace | accepted | Runtime/spec/CLI depend on canonical segment-trace strings |
| RFC-0021 | Tier4 Cognition | accepted | All 10 criteria met: Tier4Loop observe/reflect/refine, SelfModel (81-entry ring buffer), RecursiveImprovementBounds, TierAwarePlanner, promotion heuristics, VM tier limits, Axion CheckTier enforcement; 4 test suites pass |
| RFC-0022 | Axion Policy Language Evolution | accepted | CLI/compiler/runtime policy surface is active; supersedes RFC-0009 in practice |
| RFC-0023 | T81Lang Print Canonical Runtime | accepted | All 5 criteria met: `Opcode::Print` + `State::printed_output`; IR lowering via binary_emitter; Int/Bool/Float/Fraction/Symbol rendering; determinism verified by `vm_print_test` + `e2e_print_runtime_test` |
| RFC-0024 | C++23 Wording Alignment | accepted | All 3 criteria met: no normative changes, consistent C++23 wording, CI unaffected; 344/344 tests pass |
| RFC-0025 | Policy-Gated Tensor Loading via CanonFS | accepted | `TLOADHASH` + `allowed-tensor-hashes` are active; operational hardening continues |
| RFC-0026 | AI-Native Inference Opcodes | accepted | Phase-1 opcode surface implemented; `WLOAD` promotion review resolved; RFC-0030 float-domain policy complete |
| RFC-0027 | Spec-as-Executable Conformance Model | accepted | Conformance suite is wired into CMake/CTest; optional annotation follow-ons remain |
| RFC-0028 | Deterministic Trace JIT | accepted | All 6 criteria met: §2 trace_hash, §3 flat register file, §4 CanonFS JIT cache (JitTraceCache; 19/19), §5 AxionBoundary OSR, §6 repro oracle (15/15) |
| RFC-0029 | T81Lang Feature Registry Drift Prevention | accepted | All 6 criteria met: `kBuiltinTable` as single source, tier-gate enforcement in SA, distributed builtins blocked in DCP, effect-surface blocking in `@pure`/Tier≤1, VM `DecodeFault` safeguard, `spec/t81lang_features.md` feature registry |
| RFC-0030 | Deterministic Math Subsystem | accepted | `t81_soft_math` integer-backed implementation covers all transcendentals; `FDiv` is IEEE 754 exactly-rounded; RFC-0010 caveat resolved |
| RFC-0031 | Deterministic AI Execution Contract | accepted | All 5 RFC-0032 promotion phases complete; conformance programs authored; phase_status advanced to spec_conformant |
| RFC-0032 | AI Subsystem Promotion Pathway | accepted | All 5 phases complete: ternary codec, doc, Axion hooks, T81VmBackend, EvidenceCollector + AI CLI; conformance suite closed |
| RFC-0033 | Dual TUI Frontends | accepted | All 4 phases complete: FTXUI infra, `t81 studio` (7 views, palette, REPL), `t81 agent` (14 slash cmds, session save/load), CI snapshot test (11/11 assertions) |
| RFC-00B0 | Axion HAL Specification | accepted | First non-hosted promotion path and HAL contract for the Axion OS stack |
| RFC-00B1 | Ternary MMU | accepted | TVA layout, radix page table, and MMU fault model for Axion |
| RFC-00B2 | Device Driver Architecture | accepted | Phase 4 storage/display/network boundary and guest-device model |
| RFC-00B3 | Axion Kernel Architecture | accepted | All §8 acceptance criteria met; two open questions resolved (capability model, device registry) |
| RFC-00B4 | Userland Service Contract | accepted | Stable service/runtime request and result boundary before syscall and ABI widening |
| RFC-00B5 | Governed Event Interrupt Model | accepted | Formalizes event-driven interrupt handling and no trap-return opcode requirement for the current Axion path |
| RFC-00B6 | Minimal Syscall and Capability Boundary | accepted | All §8 criteria met; open question resolved (Slice 22): `ClaimDevice`/`ReleaseDevice`/`QueryDevice` wired as `KernelCallKind` entries with ownership enforcement; proved by `[AC-22d-01..08]` |
| RFC-00B7 | Pager Service ABI | accepted | `PagerService` capability + `RequestPageMapping` / `WaitForPagerHandoff` / `ResumePageFaultedThread` fully implemented and tested |

## DPE RFCs (Deterministic Parallel Execution series)

| RFC | Title | Status | Notes |
| :--- | :--- | :--- | :--- |
| RFC-DPE-0001 | Deterministic Parallel Execution — Vision and Motivation | accepted | Informational; vocabulary and principles for the DPE series |
| RFC-DPE-0002 | TISC Task Graph Primitives | accepted | All §5 criteria met; DpeTaskRunner + [DPE-02-05] (single-task epoch ≡ direct TISC execution) proved in Slice 14 via `task_runner_test` |
| RFC-DPE-0003 | Epoch Execution and Canonical Commit | accepted | [DPE-03-01..04] met (Slice 15); kernel wiring + [AC-22s] (Slice 16); [DPE-03-06] met via KernelEpochPolicyGate + EpochAbortedPolicyFault audit event (Slice 17); [DPE-03-05] met: DeltaRecord::word_tags + FloatHandle canonical serialization + intern_float() + t81_dpe_float_reduction_test 21/21; all 6/6 criteria met; [DPE-07-01..06] met (Slice 21): SubmitEpoch KernelCallKind ABI boundary fully wired through axion_kernel_call() |
| RFC-DPE-0004 | DAG-Ordered Multi-Task Epoch Execution | accepted | [DPE-04-01..04] met (Slice 18): topological_sort_epoch() + DpeTaskInputSnapshot + predecessor delta propagation; proved by `t81_dpe_epoch_dag_test` |
| RFC-DPE-0005 | Level-Parallel Epoch Execution | accepted | [DPE-05-01..04] met (Slice 19): topological_levels_epoch() + std::thread level dispatch + noexcept fallback; proved by `t81_dpe_epoch_parallel_test` |
| RFC-DPE-0006 | Bounded Thread Pool for Epoch Execution | accepted | [DPE-06-01..04] met (Slice 20): DpeThreadPool (N workers, queue, idle-wait, shutdown) + optional pool param in axion_kernel_submit_epoch(); proved by `t81_dpe_thread_pool_test` |
| RFC-DPE-0007 | Epoch Execution Timeout | accepted | [DPE-08-01..06] met (Slice 23): optional timeout_ms param on axion_kernel_submit_epoch(); per-level steady_clock check; Aborted_Timeout → RetryLater/EpochTimedOut at syscall boundary; proved by `t81_ternaryos_epoch_timeout_test` |
| RFC-DPE-0008 | Epoch Audit Events | accepted | [DPE-09-01..06] met (Slice 24): EpochSubmitted/EpochCommitted/EpochAborted wired into axion_kernel_submit_epoch(); epoch_audit_submissions/commits/aborts counters + last_epoch_audit_kind/sequence retained state; proved by `t81_ternaryos_epoch_audit_test` |
| RFC-DPE-0009 | Epoch History Ring | accepted | [DPE-10-01..06] met (Slice 25): EpochHistoryRecord + kEpochHistoryCapacity=8 ring in EpochRuntimeState; populated on commit; evicts oldest on overflow; exposed as vector in KernelRuntimeStatusView; proved by `t81_ternaryos_epoch_history_test` |

## Experimental RFCs (A-series)

| RFC | Title | Status | Notes |
| :--- | :--- | :--- | :--- |
| RFC-00A0 | AI Experiment Sandbox | superseded | Superseded by RFC-0032; 3-stage promotion lifecycle executed to completion |
| RFC-00A1 | Deterministic Evidence Protocol | superseded | Superseded by RFC-0032 §5; `evidence-schema-v1` (FNV-1a, key=value, no timing) replaces SHA-256/JSON/wall-clock design |
| RFC-00A2 | AI Benchmark Specification | accepted | 6/6 AC met: VM throughput benchmarks, CanonHash81 determinism validation (`score=1.0`), Google Benchmark JSON output, `t81 internal benchmark` CLI; `BM_DeterminismValidation.cpp` added (2026-03-16) |
| RFC-00A3 | Model Artifact Provenance | accepted | Implemented as TLOADHASH + `allowed_tensor_hashes` + `model_load` Axion event (RFC-0025) |
| RFC-00A4 | Ternary Quantization Codec | accepted | `quantize_threshold()` + `pack_ternary_to_base81()` promoted per RFC-0032 Phase 1; float metrics removed |
| RFC-00A5 | LLM Backend Adapter | superseded | Superseded by RFC-0032 Phase 4; `T81VmBackend` (deterministic VM-only) replaces llama.cpp/ONNX vision |
| RFC-00A6 | Axion Policy Hooks | accepted | `AIHookEngine` + `PolicyEngine` live per RFC-0032 Phase 3; event registry in `spec/supplemental/axion-event-registry.md` |
| RFC-00A7 | UX Integration | superseded | Superseded by RFC-0033 (`t81 agent`/`t81 studio`) + existing CLI; `t81 ai` command hierarchy not implemented and not on roadmap |
| RFC-00A8 | AI Native VM Opcodes | superseded | Superseded by RFC-0026; ATTN/QMATMUL/EMBED/WLOAD live in TISC ISA |

## Active Consolidations

- SIMD track: RFC-0017 is the accepted register-native type (`t81::T81`); RFC-0016 is formally superseded. RFC-0018 documents the add/mul arithmetic implementation.
- APL track: RFC-0022 is the accepted forward path; RFC-0009 is formally superseded.
- AI-native track: RFC-0026 supersedes RFC-0012, RFC-0013, RFC-0014. Those are closed for new edits.
- Conformance track: RFC-0027 supersedes RFC-0008. Spec-as-executable is the concrete realization.
- Float determinism: RFC-0030 closes the RFC-0010 FDIV/transcendental caveat. RFC-0010 is fully accepted with no outstanding caveats.

## Authoring

- Start from `spec/rfcs/template.md`.
- Update this index whenever RFC status changes.
