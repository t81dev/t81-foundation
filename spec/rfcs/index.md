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
| RFC-0034 | T81-Native AI Inference | accepted | In-repo implementation closure is complete: opcode/runtime path, Axion policy surface, `@ternary_inference` lowering, conformance programs, CanonFS-backed `.t81w` execution evidence, and native benchmark evidence are all in place; a first native VM fast-path set (`TExp`, `TQUANT`, `TACT`, `TERNACCUM`, `TSiLU`, `TSoftmax`, `TRMSNorm`, `TRoPE`, `TWEMBED`, `TWMATMUL`, `TATTN`) for balanced-trit weights is now implemented, with matched VM native-vs-binary wins recorded at roughly `62x-67x` for the unary set, `6259.43x` for `TQUANT`, `6834.34x` for `TACT`, `80.57x` for `TERNACCUM`, `11.99x` for `TRMSNorm`, `4.07x` for `TRoPE`, `769.63x` for `TWEMBED`, a scale-dependent `TWMATMUL` crossover from `0.84x` at `64` to `11.84x` at `256` and `570.42x` at `4096`, and `TATTN` at `110970.48x` on the current `256`-trit VM comparison; remaining work is cross-platform evidence refresh plus broader optimization of the post-decode result-representation path |
| RFC-0040 | SWAR Formalization | accepted | In-repo implementation closure is complete: stable SWAR API, explicit TISC opcodes (`0xD5`-`0xD7`), Setun bridge mnemonics, VM/JIT/CanonFS coverage, Axion trace/policy coverage, performance evidence, and `experimental/packed_trit_vector.hpp` deprecation wording all in place (2026-03-22); remaining `stable`-promotion item: refreshed x86_64 cross-architecture evidence (pending CI runner) |
| RFC-0041 | SIMD Formalization | accepted | In-repo implementation closure complete: AVX2/NEON kernels, `t81/simd/simd.hpp` facade, migration guide, ARM64 evidence (2026-03-18 + 2026-03-22 snapshots), formal deprecation wording (`#pragma message` guard on direct experimental include), and RFC-0041 evidence note all in place; remaining `stable`-promotion item: x86_64 evidence refresh (pending CI runner) |
| RFC-0042 | Deterministic Backend Equivalence Contract | accepted | Defines scalar as the canonical oracle and makes scalar ↔ SWAR ↔ SIMD ↔ future backend substitution a governed equivalence surface rather than an optimization-only convention |
| RFC-0043 | Deterministic Conformance and Validation Framework | accepted | Defines how determinism claims are proven: corpus classes, replay artifacts, backend matrix, breach classification, and CI enforcement model |
| RFC-0044 | Stable Packed Trit Vector Interface | accepted | Stabilizes the shared packed-trit substrate under SWAR and SIMD so backend governance no longer depends on `experimental` representation assumptions |
| RFC-0045 | Deterministic Memory Model | accepted | Defines canonical memory visibility, segment semantics, handle identity, aliasing constraints, and the relationship between immediate VM mutation and deferred epoch commit |
| RFC-0046 | Deterministic Scheduling and Execution Ordering | accepted | Defines the constitutional scheduling rule across program order, dependency order, canonical commit order, and future multi-core/distributed execution |
| RFC-0047 | Deterministic JIT and Lowering Rules | accepted | Constrains Trace-JIT and future lowering-based acceleration so optimization remains interpreter-equivalent, policy-boundary-safe, and subordinate to backend/memory/scheduling governance |
| RFC-0048 | Deterministic Surface Definition and Governance Boundaries | accepted | Defines the constitutional classes for DCP, governed non-DCP, experimental, and out-of-scope surfaces, plus the promotion/demotion rules that control deterministic claims |
| RFC-0049 | Canonical Ternary Arithmetic Semantics | accepted | Defines the mathematical oracle for addition, subtraction, multiplication, comparison, carry, normalization, and overflow so every governed backend shares one arithmetic contract |
| RFC-0050 | Vectorized Ternary Operations for TISC | accepted | Defines when vector execution is an ISA/VM semantic surface versus an internal backend optimization, including lane ordering, width, trace, and fallback rules |
| RFC-0051 | Deterministic Heterogeneous Acceleration | accepted | Constrains GPU and other accelerator backends through explicit kernel-class, transfer, reduction, fallback, and promotion rules so heterogeneous execution cannot silently expand the deterministic surface |
| RFC-0052 | Canonical Dataflow and State-Driven Execution | accepted | Defines canonical dependency graph, state identity, readiness, propagation, and CanonFS-linked execution rules above DPE |
| RFC-0053 | Distributed Deterministic Execution Protocol | accepted | Defines cross-node state identity, global commit order, conflict resolution, replay artifacts, and failure behavior for future distributed execution |
| RFC-0054 | CanonFS Object Identity and Persistence Contract | accepted | Reconciles CanonFS object identity, persistent-driver guarantees, capability semantics, parity semantics, and user-visible reference naming |
| RFC-0055 | Native Ternary Hardware Target and Interop Contract | accepted | Separates native ternary hardware targets from accelerator-only targets; defines TISC semantic authority, hardware interop obligations, and promotion rules for future external ternary platforms |
| RFC-0056 | Google Axion-T81 Integration Contract | accepted | Contract specifies Axion as a Verified Lowering Target (RFC-0055): TISC-to-ARM64 lowering, Titanium offload governance (RFC-0048 boundary classification), cloud monitoring separation (§5.2), and multi-instance/multi-region determinism via RFC-0053 epoch protocol; all 4 open questions resolved (2026-03-22); Phase 1–3 implementation is post-acceptance work |
| RFC-00B0 | Axion HAL Specification | accepted | First non-hosted promotion path and HAL contract for the Axion OS stack |
| RFC-00B1 | Ternary MMU | accepted | TVA layout, radix page table, and MMU fault model for Axion |
| RFC-00B2 | Device Driver Architecture | accepted | Phase 4 storage/display/network boundary and guest-device model |
| RFC-00B3 | Axion Governance Kernel Architecture | accepted | All §8 acceptance criteria met; two open questions resolved (capability model, device registry) |
| RFC-00B4 | Userland Service Contract | accepted | Stable service/runtime request and result boundary before syscall and ABI widening |
| RFC-00B5 | Governed Event Interrupt Model | accepted | Slices 26–28 complete: WaitForDevice, interrupt policy gate, unhandled IRQ governance (UnhandledInterruptDropped); 3214/3214 assertions; all open questions resolved or formally deferred (2026-03-22): nesting resolved by design, audit surface stabilized (6 events), priority classes and tooling deferred to future RFCs |
| RFC-00B6 | Minimal Syscall and Capability Boundary | accepted | All §8 criteria met; open question resolved (Slice 22): `ClaimDevice`/`ReleaseDevice`/`QueryDevice` wired as `KernelCallKind` entries with ownership enforcement; proved by `[AC-22d-01..08]` |
| RFC-00B7 | Pager Service ABI | accepted | `PagerService` capability + `RequestPageMapping` / `WaitForPagerHandoff` / `ResumePageFaultedThread` fully implemented and tested |
| RFC-00B8 | Governed Foreign Function Interface | accepted | RFC-0036 language surface and VM bridge implemented end to end: integer/string/double/bytes/mixed/`StringVectorHandle`/`IntVectorHandle` call shapes proved; `required_capabilities` wired to `SyscallContext::trace_reasons`; canonical `ffi_call`/`ffi_quarantine`/`ffi_policy_deny`/`ffi_capability`/`ffi_error` audit events in Axion event registry; sandbox boundary resolved (Q6: policy-mediated gate is normative per T81 determinism requirements); 405/405 pass (2026-03-22); post-acceptance: additional ecosystem bindings and `stable` promotion evidence |
| RFC-00B9 | TernaryOS User Environment Standard | accepted | `t81-init`, session model, `t81sh`, service registry, TTY contract, and Axion gates (`BootService`/`SessionCreate`/`ServiceSpawn`/`ShellExec`) are implemented with AC-1..15 passing; `T81_ENABLE_TERNARYOS` is now default-ON as of 2026-03-22 |
| RFC-00BA | llama.cpp GGUF Ingestion Bridge | accepted | The narrow llama.cpp-backed GGUF bridge is now accepted in-repo: build-gated metadata enumeration, float export, bridge-backed `weights import --format gguf` conversion, real TinyLlama import evidence, and source-hash/bridge-revision reporting are all implemented while runtime execution remains independent from llama.cpp |
| RFC-00BB | Native Model Architecture Compatibility | accepted | Compatibility-state/profile gating active for all 7 profiles; real TinyLlama `GGUF -> .t81w -> VM TWEMBED` proof for `llama-dense-v1`; synthetic `GGUF -> .t81w -> VM TWEMBED` execution evidence added for `gemma`, `mistral`, `phi3`, `qwen2` (2026-03-22, `gguf_import_bridge_test.cpp`); post-acceptance: real GGUF execution for non-llama families once reference models are available |
| RFC-00C8 | Concurrent Fault Isolation | accepted | Phase 19 validates the previously untested `fs_sched_fault_handler()` context-switch branch: faulting `tid=7` is contained while healthy `tid=6` continues through `WaitForDevice(30)` and exits cleanly |
| RFC-00C9 | EL0 Fault Evidence Query | accepted | Freezes the minimal post-mortem read path over retained `fault_ec` and `fault_far`, strengthening Phase 18/19 proof without widening the EL0 ABI |
| RFC-00CA | EL0 Fault Summary Query | accepted | Activates reserved KernelCall ordinal 21 in the freestanding bridge and proves an EL0 sibling can query retained fault summary state immediately after fault-handler handoff |
| RFC-00CB | EL0 Fault Detail Query | accepted | Activates reserved KernelCall ordinal 15 in the freestanding bridge and proves an EL0 sibling can query retained `subject_tid` / `fault_ec` / `fault_far` detail immediately after fault-handler handoff |
| RFC-00CC | EL0 Fault Acknowledgement and Drain | draft | Defines the next fault-lifecycle step: explicit acknowledgement and deterministic draining of retained freestanding fault state |

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
