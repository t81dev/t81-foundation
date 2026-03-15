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
| RFC-0000 | T81 Base-81 Ternary Computing Stack | draft | Foundational umbrella document |
| RFC-0001 | Architecture Principles | draft | Candidate for partial integration into `t81-overview.md` |
| RFC-0002 | Deterministic Execution Contract | draft | Cross-layer invariants |
| RFC-0003 | Axion Safety Model | draft | Axion threat/safety model |
| RFC-0004 | Canonical Tensor Semantics | draft | Tensor semantics harmonization |
| RFC-0005 | TISC v0.4 Extensions | draft | ISA extension planning |
| RFC-0006 | Deterministic GC | draft | VM reclaim semantics |
| RFC-0007 | T81Lang Standard Library | draft | Stdlib contract definition |
| RFC-0008 | Formal Verification Harness | superseded | Superseded by RFC-0027 (spec-as-executable is the concrete realization) |
| RFC-0009 | Axion Policy Language (APL) | draft | Superseded path by RFC-0022 once accepted |
| RFC-0010 | TISC Float/Fraction Ops | accepted | Accepted with caveat: host `double` path for `FDIV`/transcendentals is not yet cross-arch bit-deterministic |
| RFC-0011 | T81Lang Grammar Modernization | draft | Grammar evolution proposal |
| RFC-0012 | Ternary Tensor Quantization | superseded | Superseded by RFC-0026 (QMATMUL covers quantized matmul path) |
| RFC-0013 | Ternary Matmul | superseded | Superseded by RFC-0026 (QMATMUL) |
| RFC-0014 | Neural Primitives | superseded | Superseded by RFC-0026 (ATTN + EMBED cover the neural primitive surface) |
| RFC-0015 | Agentic Constructs | draft | First-class agent model |
| RFC-0016 | SIMD Limb | proposed | SIMD arithmetic proposal |
| RFC-0017 | Introduce T81 Native | proposed | Register-native type proposal |
| RFC-0018 | T81 Native SIMD Arithmetic | proposed | Follow-on SIMD arithmetic details |
| RFC-0019 | Axion Match Logging | accepted | Runtime/spec/CLI depend on canonical match metadata and guard-audit strings |
| RFC-0020 | Axion Segment Trace | accepted | Runtime/spec/CLI depend on canonical segment-trace strings |
| RFC-0021 | Tier4 Cognition | draft | Tier-4 reflection/cognition proposal |
| RFC-0022 | Axion Policy Language Evolution | accepted | CLI/compiler/runtime policy surface is active; supersedes RFC-0009 in practice |
| RFC-0023 | T81Lang Print Canonical Runtime | draft | Deterministic print/runtime surface |
| RFC-0024 | C++23 Wording Alignment | draft | Documentation/process wording alignment |
| RFC-0025 | Policy-Gated Tensor Loading via CanonFS | accepted | `TLOADHASH` + `allowed-tensor-hashes` are active; operational hardening continues |
| RFC-0026 | AI-Native Inference Opcodes | accepted | Phase-1 opcode surface is implemented; remaining follow-on work is narrow `WLOAD` promotion review plus RFC-0030 float-domain policy |
| RFC-0027 | Spec-as-Executable Conformance Model | accepted | Conformance suite is wired into CMake/CTest; optional annotation follow-ons remain |
| RFC-0028 | Deterministic Trace JIT | draft | Trace-JIT deterministic execution model |
| RFC-0029 | T81Lang Feature Registry Drift Prevention | draft | Feature registry consistency mechanisms |
| RFC-0030 | Deterministic Math Subsystem | draft | Canonical arithmetic operations |
| RFC-0031 | Deterministic AI Execution Contract | draft | Composes RFC-0002/0003/0004/0025/0026/00A0 into a single AI execution contract |
| RFC-0032 | AI Subsystem Promotion Pathway | proposed | Normative promotion specification for `experiments/ai/` components; 5-phase roadmap |
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
| RFC-DPE-0003 | Epoch Execution and Canonical Commit | accepted | [DPE-03-01..04] met (Slice 15); kernel wiring + [AC-22s] (Slice 16); [DPE-03-06] met via KernelEpochPolicyGate + EpochAbortedPolicyFault audit event (Slice 17); [DPE-03-05] deferred (RFC-0030); [DPE-07-01..06] met (Slice 21): SubmitEpoch KernelCallKind ABI boundary fully wired through axion_kernel_call() |
| RFC-DPE-0004 | DAG-Ordered Multi-Task Epoch Execution | accepted | [DPE-04-01..04] met (Slice 18): topological_sort_epoch() + DpeTaskInputSnapshot + predecessor delta propagation; proved by `t81_dpe_epoch_dag_test` |
| RFC-DPE-0005 | Level-Parallel Epoch Execution | accepted | [DPE-05-01..04] met (Slice 19): topological_levels_epoch() + std::thread level dispatch + noexcept fallback; proved by `t81_dpe_epoch_parallel_test` |
| RFC-DPE-0006 | Bounded Thread Pool for Epoch Execution | accepted | [DPE-06-01..04] met (Slice 20): DpeThreadPool (N workers, queue, idle-wait, shutdown) + optional pool param in axion_kernel_submit_epoch(); proved by `t81_dpe_thread_pool_test` |
| RFC-DPE-0007 | Epoch Execution Timeout | accepted | [DPE-08-01..06] met (Slice 23): optional timeout_ms param on axion_kernel_submit_epoch(); per-level steady_clock check; Aborted_Timeout → RetryLater/EpochTimedOut at syscall boundary; proved by `t81_ternaryos_epoch_timeout_test` |
| RFC-DPE-0008 | Epoch Audit Events | accepted | [DPE-09-01..06] met (Slice 24): EpochSubmitted/EpochCommitted/EpochAborted wired into axion_kernel_submit_epoch(); epoch_audit_submissions/commits/aborts counters + last_epoch_audit_kind/sequence retained state; proved by `t81_ternaryos_epoch_audit_test` |

## Experimental RFCs (A-series)

| RFC | Title | Status | Notes |
| :--- | :--- | :--- | :--- |
| RFC-00A0 | AI Experiment Sandbox | draft | Formal boundaries for AI experimentation in `/experiments/ai/` |
| RFC-00A1 | Deterministic Evidence Protocol | draft | Evidence collection and verification for AI determinism |
| RFC-00A2 | AI Benchmark Specification | draft | Standardized AI performance benchmarks |
| RFC-00A3 | Model Artifact Provenance | draft | Model supply chain provenance tracking |
| RFC-00A4 | Ternary Quantization Codec | draft | Ternary-native quantization codecs |
| RFC-00A5 | LLM Backend Adapter | draft | Engine-agnostic LLM backend interface |
| RFC-00A6 | Axion Policy Hooks | draft | Policy enforcement hooks for AI operations |
| RFC-00A7 | UX Integration | draft | User experience integration for AI features |
| RFC-00A8 | AI Native VM Opcodes | draft | Additional AI-specific VM opcodes |

## Active Consolidations

- APL track: RFC-0022 is the forward path; RFC-0009 is retained for provenance until formal supersession.
- AI-native track: RFC-0026 supersedes RFC-0012, RFC-0013, RFC-0014. Those are closed for new edits.
- Conformance track: RFC-0027 supersedes RFC-0008. Spec-as-executable is the concrete realization.

## Authoring

- Start from `spec/rfcs/template.md`.
- Update this index whenever RFC status changes.
