> Historical entry below is preserved verbatim and superseded by the appended
> independent-audit entry. Current findings: [RESULTS.md](RESULTS.md).

# H04-R02 Research Session Handoff

**Document ID:** `experiments/h04_r02_adaptive_representation/HANDOFF.md`
**Date:** October 2026
**Status:** Complete / Append-Only Continuity Entry

---

## 1. Summary of Accomplishments

1. **Foundational Continuity Established:**
   - Verified research continuity across `docs/research/continuity/`.
   - Confirmed commit `def47c68` / `research/h04-r01-trace-ledger` is unavailable in the git object database, matching `HYPOTHESIS_REGISTER_v1.json` status ("selected specification, not implemented").

2. **Fair 4-Arm Comparison Framework Implemented:**
   - Created `experiments/h04_r02_adaptive_representation/` implementing **FB**, **AB**, **FT**, **AT** with standardized capacity domains and symmetric metadata/transition accounting.

3. **Independent Optimization Oracle Built:**
   - Implemented dynamic programming reference optimizer in `oracle.py` and validated C++ evaluator outputs across all 8 workload trace families.

4. **Evidence-Bounded Classification Assigned:**
   - **Classification B — No ternary-specific advantage:** Apparent benefits disappear after comparator optimization (introducing Adaptive Binary **AB**) and accounting for conversion, metadata, and transition costs. Over $96\%$ of observed storage reduction is due to adaptive mode tiering, not the balanced ternary radix.

---

## 2. Recommended Next Research Question

**Next Single Action:**
Transition research focus from Track R (ternary arithmetic / representation efficiency) to **Track F (Deterministic & Policy-Gated Execution in AI Runtimes / Candidate 3)** or **Track M (AI Methodology & Evidence Provenance)**.

Do not attempt further software CPU ternary representation or arithmetic optimizations, as empirical results across `experiments/radix_comparison/`, `experiments/packed_ternary_matvec/`, and `experiments/h04_r02_adaptive_representation/` conclusively demonstrate that software ternary emulation offers no performance or storage advantage over optimized adaptive binary on modern binary hardware.


## 2026-10-25 — Independent integration audit (America/New_York)

**Pins:** main 01c85019; #570 8a49ba1d; #571 b2918551; recovered R01 def47c68.
The previous absence-of-history statement and universal research-closure advice
are withdrawn. H04-R01 exists locally and is preserved as a pinned source archive.

Corrected metadata, implemented DP, shared block modes, 10-bit control, independent
suffix oracle, exact exhaustive crossover checks, and reversible Base-243 encoder
are in this integration branch. Classification D applies to the original
methodological inference; corrected results are conditional modeled evidence.
They do not close H04. The 1.60<5/3 storage window survives equal metadata under
its specified comparator; no execution or device advantage was measured.

See VERIFICATION.md for actual executed checks and limitations. Recommend merging
the reviewed integration branch after CI, then closing #570/#571 as superseded.
No existing PR was merged or closed. Next action: predeclare and run the matched
codec/access experiment in RESULTS.md; stop if equal-domain binary coding or
access/conversion overhead removes the claimed benefit.


## 2026-10-25 — PR #572 precision follow-up

Reproduced the automated review's large shared-overhead defect on 13f8377f.
C++ now cancels schedule-independent overhead before optimization/crossover;
Python independently retains it in exact rational comparisons. The 49 added
adversarial cases and clean/sanitized suites pass; see PRECISION_CORRECTION.md.
One default crossover changed only in its final floating-point bits. Scientific
classification is unchanged. Existing PRs remain open pending fresh review/CI.
