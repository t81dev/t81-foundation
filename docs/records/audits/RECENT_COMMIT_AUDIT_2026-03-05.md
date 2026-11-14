# Recent Commit Audit — 2026-03-05

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Recent Commit Audit — 2026-03-05](#recent-commit-audit-—-2026-03-05)
  - [Objective](#objective)
  - [Recovery Provenance](#recovery-provenance)
  - [Reviewed Commit Window](#reviewed-commit-window)
  - [Audit Findings](#audit-findings)
  - [Remediation Applied](#remediation-applied)
  - [Conclusion](#conclusion)

<!-- T81-TOC:END -->


Status: Final
Owner: @t81dev
Scope: Post-rollback recovery and stabilization on `main`

---

## Objective

Produce a traceable audit of recent commits after rollback to a known-good baseline, verify local governance/build integrity, and record required documentation updates.

---

## Recovery Provenance

Reflog confirms rollback activity to known-good commit `293223a8` on 2026-03-04 (America/New_York):

- `2026-03-04 23:50:09 -0500` — `reset: moving to 293223a8...`
- `2026-03-04 23:12:08 -0500` — `checkout: moving from main to restore-to-commit-293223a8`

Post-rollback recovery then proceeded on `main`.

---

## Reviewed Commit Window

Audited commit range:

- Start: `c087907d` — `fix(ci): restore missing pipeline assets and recover build/governance gates`
- End: `674a7aa0` — `Advance FW-02 VM extraction and sync status backlog docs`

Chronological window (newest first):

| SHA | Date (ET) | Summary |
| :--- | :--- | :--- |
| `674a7aa0` | 2026-03-05 10:07 | Advance FW-02 extraction + status sync |
| `31861bd1` | 2026-03-05 09:59 | Harden VM dispatch + governance/status docs |
| `96c3f6a1` | 2026-03-05 09:49 | Status docs: CI recovery/system update |
| `57f1a96c` | 2026-03-05 09:16 | Restore `expected.hpp` fallback implementation |
| `b20934be` | 2026-03-05 08:39 | Format CI parent fetch-depth fix |
| `401a049e` | 2026-03-05 08:37 | Deterministic float path fix (`exp/log/sqrt`) |
| `d1261167` | 2026-03-05 08:26 | Scope clang-format CI to changed files |
| `e4daea4b` | 2026-03-05 08:25 | Formatting adjustments |
| `084fb7ec` | 2026-03-05 08:24 | Formatting adjustments |
| `a6dc44db` | 2026-03-05 08:23 | RFC link path alias + formatting fix |
| `eb287cf2` | 2026-03-05 08:22 | Lychee + AI workflow skip-path fix |
| `c087907d` | 2026-03-05 08:18 | Pipeline asset restore + gate recovery |

---

## Audit Findings

1. Governance checks were green except one actionable drift:
   - `check_public_api_semver.py` initially failed (public API lock digest mismatch).
2. Drift source was confirmed in the reviewed window:
   - `include/t81/support/expected.hpp` changed in `57f1a96c`.
   - `include/t81/types/T81Float.hpp` changed in `401a049e`.
3. Local runtime integrity remains healthy:
   - Clean local build succeeded.
   - `ctest` passed `285/285`.
4. CI posture on current head `674a7aa0`:
   - `runtime-contract`, `Format Check`, `Documentation`, `PackedTritVector Regression Check` passed.
   - `T81 Foundation CI` and `CodeQL` were in progress at audit capture.
   - `pages-build-deployment` remains non-required and failing (tracked deferred path).

---

## Remediation Applied

- Regenerated public API lock:
  - `docs/governance/PUBLIC_API_LOCK.json`
  - New digest: `21b1ef84eb3c94bba3004426f9a4570587697f538da213dd36b16616ee526950`
- Re-ran lock and governance wrapper checks:
  - `check_public_api_semver.py` — PASS
  - `check_docs_governance_hygiene.py` — PASS

---

## Conclusion

Post-rollback commit chain is coherent and recoveries are documented.
The only discovered governance drift (public API lock digest) was remediated in this audit cycle.
No additional blocker was identified from the reviewed commit window.
