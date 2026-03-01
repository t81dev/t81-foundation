# Active Tasks

Last Updated: 2026-02-28

Immediate, actionable items only. Structural hardening items live in `HARDENING_BACKLOG.md`.

---

## Open

### Governance

- [ ] **C2 Month-Close:** Execute runbook on 2026-03-31. Stamp final outcomes in `docs/records/audits/2026-03-governance-review.md`.
  - Preflight: `python3 scripts/governance/c2_month_close_preflight.py`
  - Check: `python3 scripts/governance/c2_month_close_check.py`
  - Runbook: `docs/records/status-history/C2_MONTH_CLOSE_RUNBOOK_2026-03-31.md`

### stdlib Fixture Suites (Sprint 2)

- [ ] **std.io:** Author `lang/stdlib/std/io.t81` fixtures + golden `.out` files. Add one-line entry to `STDLIB_MODULES` in `tests/cpp/cli_stdlib_fixtures_test.cpp`. Gate: `cli_stdlib_fixtures_test` (module: io) green; `check_stdlib_surface_baseline.py` passes.
- [ ] **std.sys:** Author `lang/stdlib/std/sys.t81` fixtures + golden `.out` files. Same pattern. Gate: `cli_stdlib_fixtures_test` (module: sys) green.
- [ ] **std.async:** Author `lang/stdlib/std/async.t81` fixtures + golden `.out` files. Gate: `cli_stdlib_fixtures_test` (module: async) green.
- [ ] **std.agent:** Author `lang/stdlib/std/agent.t81` fixtures + golden `.out` files. Update `STDLIB_PROMOTION_SNAPSHOT` evidence. Gate: `cli_stdlib_fixtures_test` (module: agent) green.

---

## Recently Closed

| Task | Completed |
| :--- | :--- |
| Release candidate GO stamp on `1ec312e3` | 2026-02-28 |
| T81Lang promotion to Beta implementation maturity | 2026-02-26 |
| Test suite deduplication (−16 files, −1,780 LOC, 285/285 passing) | 2026-02-28 |
| BG-01..05 engineering backlog closure | 2026-02-25 |
| BG-10 determinism tests (Quaternion/Prob/Qutrit/Uint) | 2026-02-28 |
| Cognitive Tier 1..5 implementation | 2026-02-26 |
| Runtime JIT prototype uplift | 2026-02-26 |
| CLI `t81 trace export` (JSON/CSV) | 2026-02-26 |
| Debugger cognitive tier inspection | 2026-02-26 |
| Phase 3 behavioral conformance expansion | 2026-02-28 |
