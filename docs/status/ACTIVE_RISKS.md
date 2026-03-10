# Active Risks

Last Updated: 2026-03-10
Owner: @t81dev
Cadence: Weekly refresh

No prose. If a risk needs an essay, escalate it.

## Open Risks

| ID | Risk | Severity | Owner | Mitigation | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| R-05 | AGI-facing surface growth outpacing promotion evidence updates | Medium | @t81dev | Surface inventory refreshed each monthly governance cadence | Monitoring |
| R-06 | Documentation maintenance burden after reorganization | Low | @t81dev | Content-based structure reduces maintenance overhead; automated link checking in CI | Monitoring |
| R-07 | Benchmark variability — false signal in `vm workload gate` guardrail | Low | @t81dev | Alert threshold >5% regression; review guardrail if consecutive divergence detected | Monitoring |
| R-09 | Test failures impacting release readiness — 5 tests failing (98.5% success rate) | Medium | @t81dev | **✅ RESOLVED** - All test failures fixed; 325/325 tests passing (includes `t81_determinism_containers_test` fix via `T81String::serialize_canonical()`) | **Closed** |

## Closed Risks

| ID | Risk | Resolution | Closed |
| :--- | :--- | :--- | :--- |
| **R-01** | **Determinism overclaim — external summaries omit registry boundary language** | `check_determinism_claims.py` CI gate implemented and wired to enforce registry-bounded claims; all external summaries appended with boundary link | **2026-03-10** |
| **R-16** | **VM OOB register-index crashes** — SymLoad, ReflCap, ReflJustify accessed `register_tags[insn.b]` without `reg_ok()` guard, discovered by fuzz_vm | Added `!reg_ok(insn.b)` guard to all three dispatch cases; `85a0b438` | **2026-03-10** |
| **R-17** | **binary_io OOM on corrupt/empty .tisc** — length-prefix read without sanity check allowed attacker-controlled allocation up to 2⁶⁴ elements (→ OOM-kill, exit 137) | `read_checked_size()` helper added; throws on EOF or count > 16M; `85a0b438` | **2026-03-10** |
| **R-02** | **Axion Alpha posture delays Beta promotion** | **Resolved through successful 2026-03-10 Beta candidacy review and promotion** | **2026-03-10** |
| R-07 | CodeQL push trigger missing on `main` — required context not populated | `ad6c2777` added push trigger to `codeql.yml` | 2026-02-26 |
| R-09 | March release packet blocked by required-context mismatch | GO stamped on `1ec312e3`; both required contexts completed/success | 2026-02-28 |
| R-03 | Single-owner concentration — all GO/HOLD decisions gated on @t81dev | `docs/governance/APPROVAL_DELEGATION.md` published (GOV-01) | 2026-03-05 |
| R-04 | T81Graph lang-side serialization gap (BG-09) blocks DCP candidacy | Language runtime invokes `serialize_canonical()` for T81Graph; stable serialization verified | 2026-03-04 |
| R-10 | VM dispatch concentration in policy bridge increases maintenance risk | FW-02 closed: Axion opcode pre-dispatch moved outside the main VM opcode switch (`dispatch_axion_opcode_from_step`), with centralized helper routing retained | 2026-03-05 |
| **R-14** | **T81Lang test failures blocking release readiness** — 11 failing tests causing subprocess abortions and CI instability | **All 11 failing tests fixed; 100% test success rate (285/285) achieved** | **2026-03-04** |
| **R-11** | **Parser specification violations** — Operator precedence not matching T81 spec (§A.1.1) | **Parser fixed to match specification; all regression tests passing** | **2026-03-03** |
| **R-12** | **Semantic analyzer type safety gaps** — Narrowing conversions allowed when they should fail | **Semantic analyzer enhanced with numeric rank checking; proper narrowing prevention** | **2026-03-03** |
| **R-13** | **CI build failures blocking development** — Lychee link checking and documentation reference failures | **All CI issues resolved; 100% test success rate achieved; CI now green** | **2026-03-04** |
| **R-15** | **CI workflow instability** — lychee v0.23.0 panic, non-absolute `--root-dir`, missing job timeouts, CLI manual path mismatch, `CanonHash<T81String>` non-determinism | **All fixed in `bda2f089` + PR #448; 325/325 tests; canonical ci.yml restored** | **2026-03-07** |
