# T81 Foundation — Research & Integration Status (2026-11-15)

**Observed `main` head:** `24d40f4d9fabf735f988f2852751c2cadd9beffb` (PR `#796` merged `2026-11-15`).
**Non-normative operational snapshot:** Records merged integration state, closed superseded PRs, and the pinned architecture assessment. No new scientific gate, release, or experiment authorization is granted by this status file.

---

## 1. Merged Integration State on `main`

| Merged PR | Main Commit | Scope Integrated |
|---|---|---|
| `#792` | `a3ae3b16b5afebff4a7ba3c0d2cffe8a69c11b1d` | Block-backed CanonFS metadata authorization (`Publish`, `Revoke`, `Repair` in `fs/block_backed_driver.cpp`) + expanded 27-case `canonfs_policy_boundary_test`. |
| `#793` | `c2ab68e582c9f773ce94a9e13bcb805016f07dd4` | Explicit CLI execution-policy validation (`tools/cli/core/execution_policy.hpp`, `driver.cpp`, `main.cpp`) + 249-process `t81_cli_execution_policy_test`, preserving `#782`. |
| `#796` | `24d40f4d9fabf735f988f2852751c2cadd9beffb` | Combined integration of `#794` (literal-argv `validate-policy.sh` + `t81_skill_policy_boundary_test`), `#795` (reconciled SCG-08 Gate 1 validator + normal/`-O` CTests & direct CI runs), `#780` (`BASE-PERF-00` Cell baseline source/records), fail-closed SIMD benchmark enforcement (`ci.yml`, `check_simd_regression.py --inventory`, `test_simd_regression.py`), and preserved readiness receipts (`docs/records/audits/2026-11-15-pr796-b48e1a4da-ci-readiness/`). |

* **Hosted CI at `#796` merge head (`f0415b0097e2f6e822ea61531e4a053857c71c71`):** `38` successful checks (including required `quality gate / required`, `Analyze (cpp)`, and `CodeQL`), `3` schedule-only experimental skips, and `1` pre-existing informational failure (`cross-compile / linux-armv9 / gcc`, failing on `complex_demo` / `asio.hpp`).

---

## 2. Superseded PR Closures (Heads & Evidence Preserved)

The following superseded PRs were closed without merge after their replacements landed on `main`, retaining their branch heads and audit evidence:

* `#794`, `#795`, `#780` — superseded by merged `#796` (`24d40f4d9fabf735f988f2852751c2cadd9beffb`).
* `#751` — superseded by `#794` (integrated via `#796`).
* `#750` — superseded by merged `#793` (`c2ab68e582c9f773ce94a9e13bcb805016f07dd4`).
* `#748` — superseded by merged `#792` (`a3ae3b16b5afebff4a7ba3c0d2cffe8a69c11b1d`).

---

## 3. Pinned Architecture Assessment & Next Lanes

* **Evidence-linked architecture assessment:** [docs/records/audits/2026-11-15-architecture-assessment/README.md](../records/audits/2026-11-15-architecture-assessment/README.md) (inspected against `c2ab68e582c9f773ce94a9e13bcb805016f07dd4`).
  * Classifies claims as `[specified]`, `[implemented]`, `[tested]`, `[measured]`, or `[proposed]`.
  * Separates software/HDL dual-bitplane representations (`[proposed]`) from CMOS differential electrical signaling (`[proposed]`, modeled area/power context).
  * Follows authoritative RFC/governance status for DCP maturity (`RFC-00D1` draft/partially implemented; `RFC-00D0` draft/design-led).
  * Establishes that tensor policy checks occur around dispatch and weight-handle access (`eval_axion_call`, `authorize_weights_tensor`, `record_axion_event` in `vm/vm.cpp`), not inner-loop MACs.
* **Proposed follow-on lanes (not yet executed/authorized):**
  1. **Axion-overhead profiling (`[proposed]`):** Use the 3-boundary `vm/vm.cpp` call-path table as the evidence base for a future Research Commons experiment proposal; do **not** introduce policy caching or AOT authorization until capability revocation, dynamic policy changes, and pre-side-effect ordering are addressed.
  2. **ARMv9 cross-compile portability:** Track `cross-compile / linux-armv9 / gcc` (`complex_demo` / `asio.hpp` include-path failure) separately so `main` stays operationally boring.
