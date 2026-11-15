# T81 Foundation — Research & Integration Status (2026-11-15)

**Freshly pinned `origin/main`:** `40695b0ba5cea3e43563b6ed2443c987d2738089` (2026-11-15).
**Earlier November 15 observation, retained below:** `24d40f4d9fabf735f988f2852751c2cadd9beffb` (PR `#796`). The reconciliation addendum supersedes current-state implications of that earlier snapshot.
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


## 4. Fresh-main reconciliation (November 15)

This refresh is documentation-only. [PR observations](receipts/2026-11-15-reconciliation-observations.json) and [complete PR #799 CI jobs](receipts/2026-11-15-pr799-all-jobs.json) are retained. `CURRENT_STATE.json` now includes the fresh `2026-11-15-reconciled` operational overlay, selected for action by `next_gate`. The `selected_view` field remains the dated `2026-11-02-reconciled` view because the existing contract test pins that historical selection; it is not current PR state. Earlier views remain unchanged. The fresh inventory covers only the listed PRs, not the entire backlog.

| Integrated PR | Merge commit in pinned main | Qualification |
|---|---|---|
| #782 | `fce139671d2e462b1c7f957139e1664b3b244e1f` | Windows CI baseline repairs integrated; older pending-merge status is historical. |
| #787 | `9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2` | #779/#785 integration lane; both source PRs are closed without merge. Their individual heads remain evidence. |
| #797 | `a78b1b686b97c0e8c41fa0357ca987f7a8006d10` | Prior November 15 status and architecture assessment integrated. |
| #799 | `40695b0ba5cea3e43563b6ed2443c987d2738089` | Asio target is defined for examples-only builds as well as test builds. |

#792/#793/#796 remain integrated at the commits above. #780/#794/#795 remain closed without merge, with their work integrated through #796; do not label those source PRs independently merged. #798 remains open at `618ff9058a71a38e47692780f41ceed0b59ca50a`, with rank-regression and divide-comment follow-up only; final readiness remains **BLOCKED** pending exact-head review and CI. No merge is authorized here.

**ARMv9 qualification correction:** The earlier informational `complex_demo` / `asio.hpp` failure is preserved historical evidence. PR #799's qualification run `43746367704`, job `132945852419`, now reports **SUCCESS** for `cross-compile / linux-armv9 / gcc (informational)`. This supersedes the earlier failure as the latest observed qualification of that correction. It is a cross-build with `aarch64-linux-gnu-g++`, not native ARMv9 execution, hardware validation or a blanket main-head guarantee. The associated seven hosted Linux/macOS/Windows matrix jobs and required quality gate report success in that PR run. This documentation refresh did not rerun those suites or establish a full current-main CI certificate.

**Hosted runtime versus experimental OS:** Main hosts `vm/vm.cpp`, `kernel/axion/`, `fs/`, `lang/frontend/`, and `isa/math/t81_soft_math/`. Axion policy mediation is in-process and depends on the host OS. Main also has experimental TernaryOS boot/HAL code; `ternaryos/hal/cpa004_evaluator.cpp` is absent at the pinned SHA. CPA branch/QEMU results are historical, bounded evidence and do not establish full hosted/freestanding TISC parity, OS containment or production readiness. The architecture index now identifies actual source paths and uses named tests instead of unstable CTest numbers.

**TISC conformance:** Load/Store addressing and JZ/JNZ predicate/target conflicts remain open; TXor prose lacks a decisive truth table. A separate differential-test workstream preserves the distinctions before any normative correction. Frozen opcode ADD/SUB/MUL/NEG remain arbitrary precision; internal wrapping helpers currently have no production callers. No ISA behavior or specification is changed by this status refresh.

**Holds remain binding:** H01–H08 and historical Hanoi/CWEB negative findings are unchanged. SCG-07 remains QUALIFIED WITH LIMITATIONS; RFC-00E1 remains DRAFT; SCG-08 formal Gate 1 acceptance remains WITHHELD and Gate 2 STRICT NO-GO. H04-R03 Phase C, AIOPT M2 and other measurement/execution holds remain; CPA/ISA/BASE qualifications stay scoped to their records. Integration and CI success grant no release publication, version/tag, governance activation, hardware-superiority claim, new experiment or resource authorization. The prior architecture-overhead lane remains proposed only.
