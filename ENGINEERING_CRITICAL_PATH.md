# T81 Foundation — Critical Engineering Path & Review Queue

**Date:** November 2026
**Auditor:** Jules (Engineering Auditor & Backlog Analyst)
**Authority:** Read-only analysis and roadmap recommendation; no merge authority implied.

---

## Executive Summary

The T81 backlog contains 70 open pull requests spanning foundational arithmetic, repository archaeology, experimental quantum/ternary research, governance contracts, and legacy architectural transitions.
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
- **Technical Justification:** Eliminates integer overflow undefined behavior and premature saturation in decode_i64. Mathematical proof and active sanitizers confirm correctness for all int64 bounds and cancellation vectors.
- **Current Blockers:** None. Baseline CI failures (tiny-random-llama fixture, GCC 15 GoogleBenchmark) are orthogonal.
- **Recommended Immediate Action:** Approve and merge immediately. Closes Priority 0 gate.

### Rank 2: PR #652 — Final Publication Corrections: Baseline Maintenance Repair and ADM-2 M1 Header Isolation

- **Head SHA:** `e2e5a25b17ab37198e5fe2d53b68792963fd088f`
- **Domain / Role:** Build & Header Architecture | Compilation & Clean Build Baseline
- **Technical Justification:** Decouples experimental headers from public APIs (context.hpp, batch_inference_engine.hpp). Fixes missing <chrono> and unblocks downstream compilation without altering binary layouts.
- **Current Blockers:** Needs cross-platform build validation check.
- **Recommended Immediate Action:** Review and merge to unblock standard build pipelines.

### Rank 3: PR #641 — fix(ci): include direct API test source in Windows CI sparse checkout

- **Head SHA:** `52c0018a1eb033e9b1191a329dce446a674eec03`
- **Domain / Role:** CI & Toolchain Portability | Windows CI Build Reliability
- **Technical Justification:** Resolves persistent Windows MSVC/clang-cl CMake failures caused by missing development_tests/test_real_direct_api.cpp in sparse checkout.
- **Current Blockers:** Draft status; needs marking ready for review.
- **Recommended Immediate Action:** Mark ready and merge to restore green Windows CI.

### Rank 4: PR #622 — test(ci): add regression tests for root structure validator

- **Head SHA:** `bf8147b0aae156b71bf386c7cf6ea2b8e390c525`
- **Domain / Role:** Governance Enforcement | Repository Hygiene & Boundary Enforcement
- **Technical Justification:** Adds unit test coverage for scripts/ci/check_root_structure.py, ensuring no root layout violations or spurious top-level directory pollutions escape CI.
- **Current Blockers:** Requires rebase against main.
- **Recommended Immediate Action:** Rebase against main, review, and merge.

### Rank 5: PR #664 — docs(research): final evidence reconciliation for baseline assessment

- **Head SHA:** `b6dea48d1e2f7547bbdf527fe4fba6e267d34199`
- **Domain / Role:** Research Baseline & Evidence Ledger | Authoritative Scientific Baseline
- **Technical Justification:** Consolidates and supersedes PRs #660 and #663. Reconciles all 26 benchmark families, RFC status, and capability assertions against baseline commit e108fda.
- **Current Blockers:** Maintainer approval required on reconciled CSV ledgers.
- **Recommended Immediate Action:** Accept deliverable package and merge; close PRs #660 and #663 as superseded.

### Rank 6: PR #658 — EXP-H04-R03 Phase B Implementation: Deterministic Correctness & Reference Codecs

- **Head SHA:** `0a48054510e79e753d5152ccb3f9f130c17bbafd`
- **Domain / Role:** Codec & Matched Tradeoff Research | Verified Space-Time Codec Reference
- **Technical Justification:** Implements deterministic correctness for matched arms (B10, T243, B243-CLONE, B729) over [-364, 364]. Deterministic correctness accepted (17/17 CTests passing) with strict performance freeze.
- **Current Blockers:** Blocked by CI failures in main (resolved by PRs #652 and #641).
- **Recommended Immediate Action:** Merge once PRs #652 and #641 unblock CI; enforce strict execution/benchmark freeze.

### Rank 7: PR #674 — docs: T81-AIOPT-001 M2 Adversarial Workload & Experimental Design Review

- **Head SHA:** `67d3e3ddfe876df031df334bbab3f8858db41e73`
- **Domain / Role:** Scientific Review & AIOPT Gate | Workload Falsification & Design Boundary
- **Technical Justification:** Rigorously critiques Candidate B polynomial evaluation as experimentally degenerate, rejects learned AI selectors in favor of static cost models, and directs protocol toward bounded SpMV.
- **Current Blockers:** Maintainer review of decision recommendation.
- **Recommended Immediate Action:** Review and accept critique; maintain execution freeze on AIOPT M2.

### Rank 8: PR #673 — docs(research): independent M2 protocol acceptance audit

- **Head SHA:** `b8ab59ccb8183db9476d47259bd43de1ea34522a`
- **Domain / Role:** Scientific Governance & Protocol Audit | Protocol Acceptance Gate
- **Technical Justification:** Identifies seven P1 blockers in Candidate B preregistration (sparse/dense confounds, missing binary bitset controls, non-equivalent hardware setups). Formally assigns PROTOCOL_REVISION_REQUIRED.
- **Current Blockers:** Draft status.
- **Recommended Immediate Action:** Accept audit findings as binding gate criteria.

### Rank 9: PR #666 — docs(research): independent scientific review of T81-AIOPT-001

- **Head SHA:** `a14cf4817a78ce6c8c2576b5cf8146aa2da516cb`
- **Domain / Role:** Scientific Integrity & Negative Findings | M1 Correctness Gate Closure
- **Technical Justification:** Demonstrates that M1 4-trit filtering over 40 lexicographically truncated patterns is domain-trivial and reducible to an O(1) lookup table. Recommends M2_REDESIGN_REQUIRED.
- **Current Blockers:** None.
- **Recommended Immediate Action:** Review and merge to document canonical negative result.

### Rank 10: PR #648 — Governance Authority Reconciliation Report

- **Head SHA:** `34682b7cf4cbcfd15fc3671239c4ca4d5a7d6e81`
- **Domain / Role:** Governance Architecture | Normative Authority Map
- **Technical Justification:** Maps normative authority across RFCs, architecture explanations, and governance scripts. Clarifies frozen TISC ISA v1.9.0 and API-lock boundaries, preparing grounds for PR #649.
- **Current Blockers:** None.
- **Recommended Immediate Action:** Accept report as authoritative reference.

---

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

The primary blockage preventing automated merges is the **pre-existing baseline CI failure** on `main` (commit `1737a7f` / `e108fda`):
1. `cli_contract_test` / `run_model_artifact_review_smoke.sh`: Fails due to missing fixture `models/tiny-random-llama/model.safetensors`.
2. GCC 15 Google Benchmark: `-Werror=maybe-uninitialized` in third-party benchmark code.
3. Windows CI: Missing test source in sparse checkout.
4. Direct Experimental Header Inclusions: Fixed by PR #652.

**Resolution Roadmap:**
Step 1: Merge PR #675 (completely isolated, fixes arithmetic core).
Step 2: Merge PR #652 and PR #641 (repairs header inclusions and Windows CI).
Step 3: Add lightweight dummy safetensors fixture or decouple CLI smoke tests from model weights.
Step 4: Merge PR #622 and PR #664 (locks down repository root structure and baseline evidence).
Step 5: Process research gates (PR #658 Phase B, PR #674 M2 critique, PR #666 M1 review).
