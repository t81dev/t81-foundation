# T81 Foundation — Critical Engineering Path & Review Queue

**Date:** November 2026
**Auditor:** Jules (Engineering Auditor & Backlog Analyst)
**Authority:** Read-only analysis and roadmap recommendation; no merge authority implied.

---

## Executive Summary

The T81 backlog initially contained 70 open pull requests at baseline `1737a7f`. With the successful resolution and merge of PR #675 into main (`1d01c62d`), the live backlog consists of 69 open pull requests spanning foundational arithmetic, repository archaeology, experimental quantum/ternary research, governance contracts, and legacy architectural transitions.
To prevent CI deadlock and restore predictable engineering velocity, work must be sequenced along the **critical engineering path**.
This document identifies the minimum set of engineering contributions necessary to maintain:
1. **Deterministic execution and arithmetic correctness**
2. **Reproducible scientific experiments**
3. **Stable artifact and evidence identity**
4. **Existing accepted governance boundaries**
5. **Contributor-facing build and test reliability**

Crucially, we separate **critical correctness defects** from **optional architectural expansion**.

---

## The Prioritized Top-Ten Review Queue

The following ten pull requests represent the critical path sequence. Merging or resolving these in order systematically clears blockers across the entire repository.

### Rank 1: PR #675 — fix(ternary): correct decode_i64 overflow saturation and add regression tests

- **Head SHA:** `d4c053c9405a078cb588e7227507c61449c7587e`
- **Domain / Role:** Codec & Arithmetic Correctness | Foundational Arithmetic Correctness
- **Technical Justification:** Resolves premature saturation in Horner prefix checks and signed overflow in decode_i64. Bounded mathematical proof and active sanitizers confirm correctness for tested int64 boundaries and cancellation regression vectors (though exhaustive verification across all unconstrained ternary vectors remains an open boundary).
- **Current Blockers:** None. Baseline CI failures (tiny-random-llama fixture, GCC 15 GoogleBenchmark) are orthogonal.
- **Recommended Immediate Action:** Recommended for maintainer review, conditional on independent final-head verification and maintainer authorization. Addresses Priority 0 gate.

### Rank 2: PR #652 — Final Publication Corrections: Baseline Maintenance Repair and ADM-2 M1 Header Isolation

- **Head SHA:** `e2e5a25b17ab37198e5fe2d53b68792963fd088f`
- **Domain / Role:** Build & Header Architecture | Compilation & Clean Build Baseline
- **Technical Justification:** Decouples experimental headers from public APIs (context.hpp, batch_inference_engine.hpp). Fixes missing <chrono> and unblocks downstream compilation without altering binary layouts.
- **Current Blockers:** Needs cross-platform build validation check.
- **Recommended Immediate Action:** Recommended for maintainer review, conditional on verification of downstream CTest matrix and header decoupling.

### Rank 3: PR #641 — fix(ci): include direct API test source in Windows CI sparse checkout

- **Head SHA:** `52c0018e9eabffe187cfab16bef442a486cc3bcb`
- **Domain / Role:** CI & Toolchain Portability | Windows CI Build Reliability
- **Technical Justification:** Resolves persistent Windows MSVC/clang-cl CMake failures caused by missing development_tests/test_real_direct_api.cpp in sparse checkout.
- **Current Blockers:** Draft status; needs marking ready for review.
- **Recommended Immediate Action:** Recommended for maintainer review, conditional on verifying sparse checkout configuration across Windows runners.

### Rank 4: PR #622 — test(ci): add regression tests for root structure validator

- **Head SHA:** `bf8147b2d4f8773848fa168e3b6cb92fdbb0f8f0`
- **Domain / Role:** Governance Enforcement | Repository Hygiene & Boundary Enforcement
- **Technical Justification:** Adds unit test coverage for scripts/ci/check_root_structure.py, ensuring no root layout violations or spurious top-level directory pollutions escape CI.
- **Current Blockers:** Requires rebase against main.
- **Recommended Immediate Action:** Recommended for revision/rebase against main, conditional on passing check_root_structure_test.

### Rank 5: PR #664 — docs(research): final evidence reconciliation for baseline assessment

- **Head SHA:** `b6dea482cbb060e38650c7d2b31ab5237a65a497`
- **Domain / Role:** Research Baseline & Evidence Ledger | Authoritative Scientific Baseline
- **Technical Justification:** Consolidates and supersedes PRs #660 and #663. Reconciles all 26 benchmark families, RFC status, and capability assertions against baseline commit e108fda.
- **Current Blockers:** Maintainer approval required on reconciled CSV ledgers.
- **Recommended Immediate Action:** Recommended for maintainer review and acceptance, conditional on maintainer sign-off of reconciled evidence matrices.

### Rank 6: PR #658 — EXP-H04-R03 Phase B Implementation: Deterministic Correctness & Reference Codecs

- **Head SHA:** `0a48054510e79e753d5152ccb3f9f130c17bbafd`
- **Domain / Role:** Codec & Matched Tradeoff Research | Verified Space-Time Codec Reference
- **Technical Justification:** Implements deterministic correctness for matched arms (B10, T243, B243-CLONE, B729) over [-364, 364]. Deterministic correctness accepted (17/17 CTests passing) with strict performance freeze.
- **Current Blockers:** Blocked by CI failures in main (resolved by PRs #652 and #641).
- **Recommended Immediate Action:** Recommended to hold for dependency on build/CI unblocking; deterministic correctness accepted under maintainer review, while benchmark/execution freeze remains strictly enforced.

### Rank 7: PR #674 — docs: T81-AIOPT-001 M2 Adversarial Workload & Experimental Design Review

- **Head SHA:** `67d3e3d0b24827801ec382069d66f5c8c9fd536a`
- **Domain / Role:** Scientific Review & AIOPT Gate | Workload Falsification & Design Boundary
- **Technical Justification:** Rigorously critiques Candidate B polynomial evaluation as experimentally degenerate, rejects learned AI selectors in favor of static cost models, and directs protocol toward bounded SpMV.
- **Current Blockers:** Maintainer review of decision recommendation.
- **Recommended Immediate Action:** Recommended for maintainer review; maintain execution freeze on AIOPT M2 under unaccepted workload critique.

### Rank 8: PR #673 — docs(research): independent M2 protocol acceptance audit

- **Head SHA:** `b8ab59ccb8183db9476d47259bd43de1ea34522a`
- **Domain / Role:** Scientific Governance & Protocol Audit | Protocol Acceptance Gate
- **Technical Justification:** Identifies seven P1 blockers in Candidate B preregistration (sparse/dense confounds, missing binary bitset controls, non-equivalent hardware setups). Formally assigns PROTOCOL_REVISION_REQUIRED.
- **Current Blockers:** Draft status.
- **Recommended Immediate Action:** Recommended for maintainer review as binding protocol revision criteria.

### Rank 9: PR #666 — docs(research): independent scientific review of T81-AIOPT-001

- **Head SHA:** `a14cf48a9ec8494f876819ec457be5478dfc08d5`
- **Domain / Role:** Scientific Integrity & Negative Findings | M1 Correctness Gate Closure
- **Technical Justification:** Demonstrates that M1 4-trit filtering over 40 lexicographically truncated patterns is domain-trivial and reducible to an O(1) lookup table. Recommends M2_REDESIGN_REQUIRED.
- **Current Blockers:** None.
- **Recommended Immediate Action:** Recommended for maintainer review to record canonical negative result on M1 catalog selection.

### Rank 10: PR #648 — Governance Authority Reconciliation Report

- **Head SHA:** `34682b77dcb72abf468683b005df6c4ad5db6104`
- **Domain / Role:** Governance Architecture | Normative Authority Map
- **Technical Justification:** Maps normative authority across RFCs, architecture explanations, and governance scripts. Clarifies frozen TISC ISA v1.9.0 and API-lock boundaries, preparing grounds for PR #649.
- **Current Blockers:** None.
- **Recommended Immediate Action:** Recommended for maintainer review as normative authority mapping reference.

---

### Separation of Governance and Acceptance States
In evaluating pull requests across this engineering and research backlog, three orthogonal states must be strictly distinguished:

1. **Scientific & Experimental Correctness:**
   Attributable verification that an experimental protocol, reference codec, or adversarial analysis mathematically and empirically behaves as asserted under controlled conditions.
   - *Example:* In PR #658 (EXP-H04-R03 Phase B), deterministic correctness and framing validation over domain `[-364, 364]` are accepted under maintainer review with 17/17 CTests and independent witness verification passing.
   - *Example:* In PR #666 and PR #674, scientific review formally accepted negative findings and adversarial falsification of trivial pattern-matching and polynomial workloads.
   - *Boundary:* Scientific acceptance confirms experimental validity within tested bounds; it does **not** assert hardware performance advantage or readiness for unconditional production integration.

2. **CI & Integration Qualification:**
   Verification that code builds cleanly, links without undefined symbols, and passes all regression suites across the entire supported platform/compiler matrix (Linux GCC/Clang, macOS Clang/GCC, Windows MSVC/clang-cl).
   - *Boundary:* Broader repository integration may remain blocked by unrelated baseline toolchain, runner, or fixture defects (such as missing `models/tiny-random-llama` or GCC 15 `-Werror=maybe-uninitialized`) even when the candidate's localized scientific correctness is fully accepted.

3. **Maintainer Merge Authorization:**
   Explicit governance approval by repository maintainers to integrate a PR into the primary branch (`main`).
   - *Boundary:* Neither passing tests nor peer-reviewed scientific correctness implies merge authorization. Merging requires explicit maintainer sign-off, adherence to frozen governance surfaces (e.g. TISC ISA v1.9.0, API-lock), and active execution freezes (such as the continued execution freeze on AIOPT M2).

## Critical Path vs. Optional Architectural Expansion

To protect project integrity and avoid merge deadlocks, proposed PRs must be strictly triaged into two streams:

### Stream A: Critical Correctness & Stability (Immediate Focus)
- **Arithmetic & Codec Fixes:** PR #675 (decode_i64 saturation), PR #658 (H04-R03 matched codec Phase B).
- **Build & Portability Repairs:** PR #652 (header isolation), PR #641 (Windows test source).
- **Governance & Evidence Baselines:** PR #622 (root validator tests), PR #664 (baseline evidence reconciliation), PR #648 (authority report).
- **Scientific Integrity Audits:** PR #674 (M2 critique), PR #673 (M2 protocol audit), PR #666 (M1 review).

### Stream B: Optional / Architectural Expansion (Hold / Superseded / Archive)
- **Large-scale Architectural Restructuring:** PR #508 (Tier 6 monad), PR #526 (DAIOS transition), PR #650 (C++ SDK 2.0 breaking API transition) -> **HOLD/ARCHIVE**.
- **Superseded Incremental Work:** PR #660, PR #663 (superseded by #664); PR #651, PR #653 (superseded by #657); PR #578, PR #579, PR #580 (superseded by #649); PR #645 (superseded by #658); PR #667, PR #668, PR #670 (superseded by #674) -> **CLOSE AS SUPERSEDED**.
- **Unproven Transactional Features:** PR #556 (SMT invariant proofs), PR #557, PR #560 (durable witness), PR #554 (Wasmtime challenge) -> **HOLD_DEPENDENCY / NEEDS_EVIDENCE**.
- **Legacy Pre-v1.9 Code:** PR #503, PR #504, PR #507, PR #527, PR #528, PR #612, PR #633, PR #634 -> **ARCHIVE_CANDIDATE**.

---

## Dependency Blockers & Resolution Sequence

### Provisional CI Failure Attribution
A primary impediment to automated PR qualification across hosted runners is a set of failures observed on GitHub Actions workflows. Based on comparing candidate run logs against baseline commit `1737a7f` (and prior baseline `e108fda`), these failures are provisionally attributed to pre-existing environment and fixture defects rather than new candidate logic, though comprehensive cross-platform qualification on `main` remains incomplete:
1. `cli_contract_test` / `run_model_artifact_review_smoke.sh`: Fails due to missing fixture `models/tiny-random-llama/model.safetensors`.
2. GCC 15 Google Benchmark: `-Werror=maybe-uninitialized` in third-party benchmark code.
3. Windows CI: Missing test source in sparse checkout.
4. Direct Experimental Header Inclusions: Fixed by PR #652.

**Blocker Scope Clarification for PRs #652 and #641:**
It is critical to note that while PR #652 (header isolation) and PR #641 (Windows test source inclusion) address essential header decoupling and Windows sparse checkout issues, **they do not alone resolve all baseline CI failures**. Specifically:
- `cli_contract_test` and `run_model_artifact_review_smoke.sh` fail due to missing fixture `models/tiny-random-llama/model.safetensors`, which neither PR provides.
- Google Benchmark builds under modern GCC (GCC 15) fail with `-Werror=maybe-uninitialized` in third-party benchmark registration code.
- macOS GCC builds fail linking `libgtest.a` due to libc++ vs libstdc++ symbol mismatches.

These remaining baseline defects must be addressed through separate targeted maintenance (e.g. adding the missing fixture or decoupling smoke tests from model weights, and adjusting benchmark compilation warnings) before broad CI turns green.

**Resolution Roadmap:**
Step 1: Priority 0 Closed — PR #675 merged into main at commit `1d01c62df45c9ebb06f01677e18331ef77e016c5`.
Step 2: Review and integrate PR #652 and PR #641 to decouple headers and repair Windows CI checkout.
Step 3: Address baseline fixture (models/tiny-random-llama) and third-party Google Benchmark GCC 15 warning issues.
Step 4: Review and integrate PR #622 (governance validator regression tests) and PR #664 (baseline evidence reconciliation).
Step 5: Process research gates (PR #658 Phase B deterministic codecs, PR #674 M2 adversarial review, PR #666 M1 review).
