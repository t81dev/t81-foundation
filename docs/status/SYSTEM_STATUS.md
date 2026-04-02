# System Status

Status: Active
Last Updated: 2026-04-01
Owner: Status / Governance

## Purpose

Provide a concise operational view of subsystem maturity and governance-critical
status boundaries.

## Classification Note

- **DCP / verified deterministic surface** means the surface is explicitly inside the deterministic-core boundary and backed by registry/enforcement evidence.
- **Governed non-DCP** means the surface is policy-bounded and architecturally important, but not entitled to full deterministic-surface claims.
- **Experimental / non-DCP** means the surface remains outside release-grade deterministic guarantees unless later promoted.

## Component Health

| Component | Maturity | Compliance Posture | Evidence Surface |
| :--- | :--- | :--- | :--- |
| **TISC ISA** | Stable | Frozen boundary enforced | Tests + freeze governance docs |
| **T81VM** | Stable | DCP / verified deterministic surface for interpreter execution on the current supported-platform matrix; broader VM-adjacent acceleration remains governed separately | Tests + implementation matrix + `docs/records/audits/T81VM_STABLE_PROMOTION_EVIDENCE_2026-03-15.md` |
| **Axion Governance Kernel** | Stable | Governed non-DCP overall; bounded evidence exists, but the broader governance/kernel surface is not automatically a verified deterministic surface | Tests + implementation matrix + `docs/records/audits/AXION_STABLE_PROMOTION_EVIDENCE_2026-03-15.md` |
| **T81Lang** | Stable | Governed non-DCP overall; language-spec stability is closed, but compiler/toolchain-wide deterministic promotion remains bounded by explicit evidence | `docs/records/audits/T81LANG_STABLE_PROMOTION_EVIDENCE_2026-03-16.md` + implementation matrix |
| **Data Types** | Implemented (Stable) | Frozen boundary enforced; determinism audit completed 2026-02-27 — `Cell` overflow UB fixed, `T81Float` signed-zero canonicalized, `T81Map`/`T81Set` type enforcement hardened. | `docs/reports/determinism_types_audit.md` + `tests/determinism/` + `tests/cpp/` |
| **T81Graph** | Beta | Governed non-DCP; useful and implemented, but not yet a promoted verified deterministic surface as a whole | Implementation matrix + `docs/records/audits/BG-09_IMPLEMENTATION_EVIDENCE_T81GRAPH_SERIALIZATION.md` |
| **Cognitive Tiers** | Concept / Experimental | Experimental / non-DCP / non-verified unless promoted through governance | `docs/status/EXPERIMENTAL_SURFACE_INVENTORY.md`, `spec/cognitive-tiers.md` |
| **Hanoi VM** | Concept | Experimental / non-DCP / non-verified unless promoted through governance | Experimental inventory and specs |

## Program Risks (Current)

- Spec-implementation drift on draft and experimental surfaces.
- Determinism overclaim risk outside registry-backed verified surfaces.
- CI portability churn across Windows and ARM64 lanes.
- Governance and handoff maintenance risk if control-surface docs are not refreshed alongside code.

## Operational Notes (2026-04-01)

- **Status docs refreshed to match the current repo shape** — stale March snapshot language,
  dead control references, and benchmark-lane drift are being removed so `/docs/status`
  stays usable as an active control surface rather than a historical note set.
- **RFC-00D1 remains the clearest draft-to-code bridge, but with a narrower claim** —
  the current CanonFS interchange JSON/result/provenance/policy-profile seed is stable
  enough to build against, while the broader RFC remains draft and still excludes wider
  format/symlink/archive decisions.
- **CI posture is now simpler** — deterministic-profile enforcement is consolidated into
  `ci.yml`, `runtime-contract.yml` remains a distinct cross-repo contract lane, and
  `bench.yml` is the single push-sensitive benchmark workflow.

## Operational Notes (2026-03-26)

- **RFC-00D1 moved from draft-only to an implementation seed with real surface area** —
  CanonFS import/export flows now exist in the CLI and core library, with schema artifacts,
  runtime validation, CI JSON contract checks, module tests, and policy-profile recording.
- **Handoff path tightened** — `README`, `HANDOFF`, `ROADMAP`, `BUILDABLE_NEXT_STEPS`,
  project-profile guidance, and `AGENTS.md` now present a more consistent newcomer and
  contributor story.
- **Current operational focus is narrower than subsystem breadth** — keep `main` stable,
  keep portability regressions closed, and make RFC-00D1 easier for another engineer to
  extend without needing maintainer memory.

## Operational Notes (2026-03-05)

- CI recovery completed on `main` after restoring `include/t81/support/expected.hpp`
  fallback implementation (`57f1a96c`).
- Format gate reliability hardened by increasing `actions/checkout` history depth
  in `format.yml` (`b20934be`), preventing full-tree fallback checks on push.
- Required CI contexts are passing on the reference candidate (`57f1a96c`).

## Operational Notes (2026-03-06)

- Main CI currently red on Windows configure lanes due sparse-checkout omissions;
  patch queued to include `benchmarks/` and `spec/` in Windows sparse checkout.
- Determinism hardening queued with a same-machine repeatability gate that
  enforces bit-identical `vm_workload_determinism_signatures.log` across reruns.
- Cross-arch bit-identity gates for `t3k` and `t81lang` promoted into required
  quality-gate dependencies.

## Operational Notes (2026-03-16)

- T81Lang language-spec stability review closed (spec v1.3); this should not be
  read as blanket DCP promotion for the entire toolchain.
- RFC-0015 agentic constructs accepted: first-class `agent`/`behavior` declarations,
  `AGENT_INVOKE` opcode, `infer` sugar; 16/16 assertions pass; tisc-spec §5.16 added.
- RFC-0011 (Grammar Modernization) and RFC-00A2 (AI Benchmark Spec) accepted.
- Full suite: 363/363 tests passing (100%).

## Operational Notes (2026-03-22)

- **Ternary-Native Inference: Track K/L execution performance milestone reached** — native
  unary fast paths (`TExp`, `TSiLU`, `TSoftmax`) now carry the correct `HostFloat` numeric class,
  and `ops::matmul` has a dedicated IEEE float path for HostFloat inputs that skips both
  `deterministic_fma` round-trips and eager DFixed canonical-fixed cache builds. The chained
  `WeightsLoad → TExp → TMatMul` path drops from ~4 160 ms/iter to 0.0073 ms at 64 elements
  (10–873× vs binary BigInt reference at sizes 64–4096). This removes the primary result-
  representation gating factor identified in RFC-00BB §6.3 for progression toward
  `native_supported` for dense decoder families.
- **Test count updated to 406/406** — all passing on AArch64. +1 from `axion_agent_invoke_policy_test`
  (9 assertions, [AI-01..05]) closing the Axion runtime-integration evidence gap.
- **TernaryOS ARMv8 QEMU boot milestone reached** — `BOOTAA64.EFI` executed under
  QEMU `virt,accel=hvf` (Apple Silicon HVF); all 10 contract files verified including
  Phase 5 startup, CanonStore recovery (20 entries), display/network round-trips, and
  durable session state. `hal_main_result=0`, `kernel_boot_ready_slice=complete`,
  `phase=5`, `shell_mode=typed-builtins`. Evidence:
  `docs/records/audits/TERNARYOS_ARMV8_QEMU_BOOT_EVIDENCE_2026-03-22.md`.

## Operational Notes (2026-03-19)

- RFC-0042 through RFC-0053 now form the draft governance chain for backend
  equivalence, conformance, packed-trit stabilization, memory, scheduling, JIT
  lowering, deterministic-surface boundaries, arithmetic semantics, vector
  semantics, heterogeneous acceleration, dataflow, and distributed execution.
- Public status documents are being aligned so they distinguish verified DCP
  surfaces from governed non-DCP and experimental surfaces.
- Axion epoch scheduler/audit parity is now CI-enforced via the dedicated
  `axion-epoch-determinism` lane, covering pooled-vs-unbounded kernel submit
  equivalence for execution and audit outcomes in the experimental TernaryOS +
  DPE boundary.

## Control References

- `docs/status/PROJECT_CONTROL_CENTER.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/status/ACTIVE_RISKS.md`
- `docs/status/T81LANG_PROMOTION_GATE.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`
- `spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md`

## Versioning Statement

This status document is descriptive and must remain aligned with the authority
model and governance artifacts.
