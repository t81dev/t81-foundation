# Active Risks

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Active Risks](#active-risks)
  - [Open Risks](#open-risks)
  - [Closed Risks](#closed-risks)

<!-- T81-TOC:END -->


Last Updated: 2026-04-01
Owner: @t81dev
Cadence: Weekly refresh

No prose. If a risk needs an essay, escalate it.

## Open Risks

| ID | Risk | Severity | Owner | Mitigation | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| R-05 | Governed non-DCP and draft surfaces could outpace boundary, registry, and evidence updates | Medium | @t81dev | Keep public claims bounded to registry-backed surfaces, refresh handoff/status docs as RFC-00D0 and RFC-00D1 evolve, and continue classification-correct wording in README/status materials | Monitoring |
| R-06 | Documentation and handoff drift could reintroduce maintainer-memory dependence | Medium | @t81dev | Keep `README`, `HANDOFF`, `BUILDABLE_NEXT_STEPS`, `AGENTS`, and `/docs/status` aligned whenever priority lanes change | Monitoring |
| R-07 | CI portability churn could keep `main` operationally noisy across Windows and ARM64 lanes | Medium | @t81dev | Treat Windows/MSVC, Windows/clang-cl, and Linux/ARM64 regressions as immediate-fix items; verify portability-sensitive changes before PR | Monitoring |
| R-08 | RFC-00D1 could stall in a half-implemented state if the stable seed boundary and remaining negative-path behavior are not kept explicit | Medium | @t81dev | Keep RFC-00D1 focused on finishable hardening slices: examples, negative tests where ambiguity remains, schema/CLI alignment, and stable-seed boundary clarity | Monitoring |
| R-09 | Benchmark variability could create false signal in `vm workload gate` guardrail | Low | @t81dev | Alert threshold >5% regression; revisit threshold only after repeated same-environment divergence | Monitoring |

## Closed Risks

| ID | Risk | Resolution | Closed |
| :--- | :--- | :--- | :--- |
| **R-01** | **Determinism overclaim — external summaries omit registry boundary language** | `check_determinism_claims.py` CI gate implemented and wired to enforce registry-bounded claims; all external summaries appended with boundary link | **2026-03-10** |
| **R-16** | **VM OOB register-index crashes** — SymLoad, ReflCap, ReflJustify accessed `register_tags[insn.b]` without `reg_ok()` guard, discovered by fuzz_vm | Added `!reg_ok(insn.b)` guard to all three dispatch cases; `85a0b438` | **2026-03-10** |
| **R-17** | **binary_io OOM on corrupt/empty .tisc** — length-prefix read without sanity check allowed attacker-controlled allocation up to 2⁶⁴ elements (→ OOM-kill, exit 137) | `read_checked_size()` helper added; throws on EOF or count > 16M; `85a0b438` | **2026-03-10** |
| **R-18** | **3 TLOADHASH tests failing** — `t81_vm_tloadhash_conformance_test`, `t81_vm_tloadhash_canonical_fixed_test`, `t81_vm_tloadhash_decodefault_determinism_matrix_test` appearing as SEGFAULTs in CI | Root cause was stale CMake generator mismatch (Unix Makefiles vs Ninja) in asio/googlebenchmark/ftxui subbuilds. After clearing stale subbuild caches and reconfiguring with the default Ninja preset, all 3 tests pass. Not a code defect. 344/344 passing. | **2026-03-15** |
| R-19 | Test failures impacting release readiness — 5 tests failing (98.5% success rate) | Primary failures fixed; 344/344 passing. R-18 also closed after identifying the stale CMake generator mismatch root cause. | 2026-03-15 |
| **R-02** | **Axion maturity posture delayed governance closure** | **Resolved through the 2026-03-10 review cycle; later boundary work now classifies Axion as governed non-DCP rather than implicitly DCP-adjacent** | **2026-03-10** |
| R-20 | CodeQL push trigger missing on `main` — required context not populated | `ad6c2777` added push trigger to `codeql.yml` | 2026-02-26 |
| R-21 | March release packet blocked by required-context mismatch | GO stamped on `1ec312e3`; both required contexts completed/success | 2026-02-28 |
| R-03 | Single-owner concentration — all GO/HOLD decisions gated on @t81dev | `docs/governance/APPROVAL_DELEGATION.md` published (GOV-01) | 2026-03-05 |
| R-04 | T81Graph lang-side serialization gap blocked deterministic-surface review | Language runtime invokes `serialize_canonical()` for T81Graph; stable serialization verified | 2026-03-04 |
| R-10 | VM dispatch concentration in policy bridge increases maintenance risk | FW-02 closed: Axion opcode pre-dispatch moved outside the main VM opcode switch (`dispatch_axion_opcode_from_step`), with centralized helper routing retained | 2026-03-05 |
| **R-14** | **T81Lang test failures blocking release readiness** — 11 failing tests causing subprocess abortions and CI instability | **All 11 failing tests fixed; 100% test success rate (285/285) achieved** | **2026-03-04** |
| **R-11** | **Parser specification violations** — Operator precedence not matching T81 spec (§A.1.1) | **Parser fixed to match specification; all regression tests passing** | **2026-03-03** |
| **R-12** | **Semantic analyzer type safety gaps** — Narrowing conversions allowed when they should fail | **Semantic analyzer enhanced with numeric rank checking; proper narrowing prevention** | **2026-03-03** |
| **R-13** | **CI build failures blocking development** — Lychee link checking and documentation reference failures | **All CI issues resolved; 100% test success rate achieved; CI now green** | **2026-03-04** |
| **R-15** | **CI workflow instability** — lychee v0.23.0 panic, non-absolute `--root-dir`, missing job timeouts, CLI manual path mismatch, `CanonHash<T81String>` non-determinism | **All fixed in `bda2f089` + PR #448; 325/325 tests; canonical ci.yml restored** | **2026-03-07** |
