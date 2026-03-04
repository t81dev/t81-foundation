# Active Risks

Last Updated: 2026-03-03
Owner: @t81dev
Cadence: Weekly refresh

No prose. If a risk needs an essay, escalate it.

## Open Risks

| ID | Risk | Severity | Owner | Mitigation | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| R-01 | Determinism overclaim — external summaries omit registry boundary language | High | @t81dev | `governance-metrics` CI gate enforces registry-bounded claim language | Monitoring |
| R-02 | Axion Alpha posture delays Beta promotion (partial spec coverage, evidence gaps in 1.1/1.3/1.10) | High | @t81dev | Evidence closure milestones M5–M7 active; target Beta review 2026-04-30 | Open |
| R-03 | Single-owner concentration — all GO/HOLD decisions gated on @t81dev | Medium | @t81dev | Define deputy-approval policy in `docs/governance/APPROVAL_DELEGATION.md` before 2026-04-30 | Open |
| R-04 | T81Graph lang-side serialization gap (BG-09) blocks DCP candidacy | Medium | @t81dev | Tracked in `HARDENING_BACKLOG.md`; prerequisite for graph DCP scope change | Open |
| R-05 | AGI-facing surface growth outpacing promotion evidence updates | Medium | @t81dev | Surface inventory refreshed each monthly governance cadence | Monitoring |
| R-06 | Benchmark variability — false signal in `vm workload gate` guardrail | Low | @t81dev | Alert threshold >5% regression; review guardrail if consecutive divergence detected | Monitoring |
| R-07 | Jekyll Pages build failure — persistent noise on release candidates | Low | @t81dev | Deferred; exclude `third_party/` from Jekyll scope post-C2 | Deferred |

## Closed Risks

| ID | Risk | Resolution | Closed |
| :--- | :--- | :--- | :--- |
| R-08 | CodeQL push trigger missing on `main` — required context not populated | `ad6c2777` added push trigger to `codeql.yml` | 2026-02-26 |
| R-09 | March release packet blocked by required-context mismatch | GO stamped on `1ec312e3`; both required contexts completed/success | 2026-02-28 |
| **R-10** | **T81Lang test failures blocking release readiness** — 11 failing tests causing subprocess abortions and CI instability | **All 11 failing tests fixed; 100% test success rate (285/285) achieved** | **2026-03-03** |
| **R-11** | **Parser specification violations** — Operator precedence not matching T81 spec (§A.1.1) | **Parser fixed to match specification; all regression tests passing** | **2026-03-03** |
| **R-12** | **Semantic analyzer type safety gaps** — Narrowing conversions allowed when they should fail | **Semantic analyzer enhanced with numeric rank checking; proper narrowing prevention** | **2026-03-03** |
