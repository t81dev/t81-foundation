# Implementation Matrix

Status: Active
Last Updated: 2026-03-22
Owner: @t81dev

Alignment truth. One row per subsystem. No narrative.

Authority remains: `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.

Classification rule: implementation maturity is not the same thing as DCP / verified deterministic status. Promotion State reflects governance boundary status, not just feature completeness.

---

## Matrix

| Subsystem | Spec Reference | Spec Authority | Implementation Maturity | Promotion State | Spec-Impl Alignment | Drift Risk | Last Alignment Review | Owner | Target | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Data Types** | `spec/t81-data-types.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-03-06 | @t81dev | N/A (maintain) | Frozen DCP surface. Audit 2026-02-27: `Cell` signed-overflow UB fixed, `T81Float` signed-zero canonicalized, `T81Map`/`T81Set` enforcement hardened. Documentation reorganization completed 2026-03-06. |
| **TISC ISA** | `spec/tisc-spec.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-03-18 | @t81dev | N/A (maintain) | Frozen DCP surface. Freeze integrity enforced by `check_tisc_freeze_integrity.py`. RFC-0040 SWAR tensor opcodes `TNOT_SWAR`/`TAND_SWAR`/`TOR_SWAR` are now documented and covered in VM/JIT/spec/test surfaces. |
| **T81VM** | `spec/t81vm-spec.md` | Stable | Stable | **Verified (interpreter DCP scope)** | Low | Low | 2026-03-19 | @t81dev | N/A (maintain) | Interpreter execution on the current supported-platform matrix is a verified deterministic surface. Broader VM-adjacent acceleration, backend substitution, and future lowering paths remain governed by RFC-0042..RFC-0047 rather than implied by implementation maturity. Review record: `docs/records/audits/T81VM_STABLE_PROMOTION_EVIDENCE_2026-03-15.md`. |
| **T81Lang** | `spec/t81lang-spec.md` | **Stable** | Stable | **Governed non-DCP** | High | Low | 2026-03-19 | @t81dev | N/A (maintain) | Language-spec stability is closed, but the compiler/toolchain surface is not automatically a verified deterministic surface. Deterministic claims remain bounded to explicit evidence such as fixture-based compilation checks and registry-governed replay surfaces. Review record: `docs/records/audits/T81LANG_STABLE_PROMOTION_EVIDENCE_2026-03-16.md`. |
| **Axion Governance Kernel** | `spec/axion-kernel.md` | Stable | Stable | **Governed non-DCP** | Low | Low | 2026-03-22 | @t81dev | N/A (maintain) | Important governance/runtime surface with substantial evidence and closed milestone slices. Experimental kernel epoch scheduler/audit parity is now CI-enforced through `axion-epoch-determinism`, but the broader kernel/governance behavior is not automatically a verified deterministic surface. Runtime-integration evidence gap closed 2026-03-22: `axion_agent_invoke_policy_test` ([AI-01..05]) proves the policy engine is correctly wired to the `AgentInvoke` VM dispatch path via per-instruction `eval_axion_call`. Review record: `docs/records/audits/AXION_STABLE_PROMOTION_EVIDENCE_2026-03-15.md`. |
| **T81Graph** | Surface inventory (non-normative) | Draft | Beta | **Governed non-DCP** | Low | Low | 2026-03-19 | @t81dev | N/A (maintain) | VM opcode lowering and lang-side serialization are implemented with bounded evidence, but graph behavior is not yet promoted as a verified deterministic surface as a whole. |
| **DPE (Parallel Execution)** | RFC-DPE-0001–0009 | **Accepted** | Stable | **Governed non-DCP** | Low | Low | 2026-03-19 | @t81dev | N/A (maintain) | Accepted deterministic execution model with epoch/task/history/audit machinery in place. Kernel pooled-vs-unbounded epoch parity is now CI-enforced in the experimental TernaryOS lane, while whole-surface promotion remains governed by the newer equivalence, memory, and scheduling RFC chain rather than implied by implementation completeness alone. |
| **Cognitive Tiers** | `spec/cognitive-tiers.md` | Draft | Concept / Experimental | **Experimental** | Low | High | 2026-03-19 | @t81dev | 2026-06-15 | Experimental cognitive architecture with active implementation slices and tests. Experimental, non-DCP, non-verified unless promoted through governance. |
| **Benchmark Suite** | RFC-00A2 | **Accepted** | Stable | **Governed non-DCP** | Low | Low | 2026-03-19 | @t81dev | N/A (maintain) | Performance and determinism-evidence tooling is implemented and important, but benchmark infrastructure is an enforcement/support surface rather than itself a DCP deterministic execution surface. |
| **SIMD Tritwise Acceleration** | RFC-0041 | **Accepted** | Beta | **Governed non-DCP** | Medium | Medium | 2026-03-19 | @t81dev | 2026-04-15 | Core AVX2/NEON packed-trit kernels, threshold dispatch, tail fallback, and evidence are in place. Verified deterministic promotion depends on refreshed cross-architecture evidence and the RFC-0042 backend-equivalence chain. |
| **TernaryOS User Environment** | RFC-00B9 | **Accepted** | Beta | **Governed non-DCP** | Low | Medium | 2026-03-22 | @t81dev | 2026-06-15 | Acceptance coverage is implemented in the opt-in TernaryOS build. ARMv8 QEMU HVF boot evidence captured 2026-03-22: `BOOTAA64.EFI` executed; 10/10 contract files verified; `hal_main_result=0`, `kernel_boot_ready_slice=complete`, `phase=5`, `shell_mode=typed-builtins`; CanonStore 20 entries recovered; display/network round-trips confirmed. Evidence: `docs/records/audits/TERNARYOS_ARMV8_QEMU_BOOT_EVIDENCE_2026-03-22.md`. The experimental kernel lane hard-fails pooled-vs-unbounded epoch divergence through `axion-epoch-determinism`. This surface is not default-on and not a verified deterministic surface in the main release boundary. |
| **Cross-Platform Determinism CI** | GitHub Actions | Non-normative | Accepted | **Governed non-DCP** | Low | Low | 2026-03-19 | @t81dev | N/A (maintain) | Cross-platform and proof-lane evidence automation is an enforcement surface that supports DCP and governed non-DCP claims; it is not itself the certified deterministic surface. Current examples include the core determinism slice and the `axion-epoch-determinism` kernel lane. |
| **Hanoi VM** | `spec/rfcs/RFC-0000` | Draft | Alpha | **Experimental** | Low | Medium | 2026-03-19 | @t81dev | 2026-06-30 | Experimental kernel/runtime research surface with meaningful implementation progress, but still outside DCP and outside verified deterministic-surface status. |
| **Ternary-Native Inference** | RFC-0034 + RFC-0037 + RFC-00BB | **Accepted** | Beta | **Governed non-DCP** | Medium | Low | 2026-03-22 | @t81dev | 2026-06-15 | Track K (2026-03-22): native unary fast paths (`TExp`/`TSiLU`/`TSoftmax`) corrected to `HostFloat` numeric class; `strict_core_eligible()` guards added in 7 locations to prevent O(N) DFixed cache builds on float-backed tensors. Track L (2026-03-22): `ops::matmul` HostFloat IEEE fast path + lazy result construction via `from_host_float_data`; chained `WeightsLoad → TExp → TMatMul` now runs at 0.0073 ms/iter (64 elements; 10–873× faster than BigInt reference at sizes 64–4096). RFC-00BB §6.3 updated with execution evidence. Primary result-representation gating factor removed; `experimental_native → native_supported` progression unblocked for dense decoder families. |
| **Lattice Cryptography** | RFC-0038+0039 | **Accepted** | Stable | **Governed non-DCP** | Low | Low | 2026-03-19 | @t81dev | N/A (maintain) | Crypto opcode and stdlib work is implemented and stable in the product sense, but whole-vertical deterministic promotion should not be implied without surface-specific registry status. |
| **Governed FFI** | RFC-00B8 + RFC-0036 | **Accepted** | Beta | **Governed non-DCP** | Medium | Medium | 2026-03-19 | @t81dev | 2026-05-15 | VM/language bridge is implemented end to end with bounded evidence, but broader schema/sandbox/policy questions keep the FFI surface outside verified deterministic status. |
| **TUI Frontends** | RFC-0033 | **Accepted** | Beta | **Governed non-DCP** | Low | Low | 2026-03-19 | @t81dev | N/A (maintain) | Production-usable interfaces, but UI/runtime integration is not itself a verified deterministic surface. |
| **Governed llama.cpp** | `docs/records/archive/project-reports/llama-governed-repro.md` (guidance) | Non-normative | Experimental | **Governed non-DCP** | Medium | Medium | 2026-02-28 | @t81dev | 2026-04-30 | Classified governed non-DCP (DEC-003). Practical reproducibility only. Promotion requires governed AGI pipeline. |

---

## Governed AGI Surface Taxonomy

| Layer | Paths | Determinism Status | Promotion State | Governance Gate |
| :--- | :--- | :--- | :--- | :--- |
| Deterministic Substrate | `core/types`, `core/isa`, `core/vm`, `include/t81/**` | DCP / registry Verified | Verified | Freeze enforcement + DCP release discipline |
| Governance Kernel | `kernel/axion` | Governed non-DCP, scope-bounded | Governed non-DCP | Axion evidence milestones + incident-response; broader kernel behavior still requires explicit promotion before stronger deterministic claims |
| AGI Runtime / Research | `runtime/tracing`, `experimental/*`, cognitive tiers | Non-DCP unless promoted | Experimental | Governed AGI promotion pipeline |
| Governed Inference | `third_party/llama.cpp`, `tooling/model/`, CLI `llama-run` | Governed non-DCP | Experimental | Governed AGI pipeline + release boundary classification |

---

## Update Rules

1. `Last Alignment Review` must be updated whenever any implementation work touches that subsystem.
2. Any row with Drift Risk = High and no closure plan within 90 days must be escalated.
3. Registry status changes (Verified ↔ Partial) require a matrix update on the same PR.
4. See `GOVERNANCE_REVIEW_CADENCE.md §3` for the full registry update procedure.

---

## Cross-References

- `docs/status/DRIFT_DECOMPOSITION.md`
- `docs/status/FROZEN_CORE_PROFILE.md`
- `docs/status/HARDENING_BACKLOG.md`
- `docs/status/EXTENSION_PROFILE.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md`
- `docs/architecture/adr/`

## Versioning Statement

Descriptive control artifact; does not override `/spec` or freeze policy.
This matrix is descriptive and must not be read as granting DCP / verified deterministic status beyond the registry and governance boundary documents.
