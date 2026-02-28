# Release Readiness Packet (2026-03)

Status: GO (Decisioned)
Date (UTC): 2026-02-28
Owner: Release / Governance

Decision (UTC): 2026-02-26 03:10:00Z
Approver: @t81dev
Decision: HOLD

Re-evaluation (UTC): 2026-02-28T00:00:00Z
Approver: PM / @t81dev
Decision: GO
Basis: Both required branch-protection contexts are `completed` + `success` on candidate `fa343e1f`.
  Non-required failure (`build` / Jekyll Pages) classified as Deferred — pre-existing Liquid rendering
  issue in `third_party/llama.cpp` modelcard templates; no impact on C++ release artifact, determinism,
  or DCP guarantees. All determinism gates, cross-arch bit-identity gates, sanitizer builds, fuzz,
  formal verification, and governance-metrics checks passed. Release may proceed on `fa343e1f`.

## Purpose

Consolidate release-governance evidence for the March packet cycle and preserve
GO/HOLD decision-stamp continuity.

## Scope

This packet captures governance evidence, decision fields, and required-context
verification method alignment. It does not alter CI policy, runtime semantics,
or freeze boundaries.

## Decision-Stamp Continuity

- Packet cycle 2026-02 includes explicit `Decision (UTC)`, `Approver`, and
  `Decision` fields.
- Packet cycle 2026-03 retains the same required decision-stamp fields.
- GO/HOLD basis remains tied to required branch-protection context status.

## Required-Context Verification (Procedure Alignment)

- Required contexts (branch protection):
  - `quality gate / required`
  - `Analyze (cpp)`
- Verification method:
  - `gh api repos/t81dev/t81-foundation/branches/main/protection --jq '.required_status_checks.contexts'`
  - `gh api repos/t81dev/t81-foundation/commits/<sha>/check-runs`
- Decision rule:
  - GO only when all required contexts are `completed` and `success`.
  - Otherwise HOLD.

## Decision Basis

- Current packet is issued as `HOLD` pending a release-candidate SHA with
  completed required-context verification evidence.
- No freeze model changes are introduced by this packet.
- Determinism and DCP boundaries remain unchanged.

## AGI-Facing Boundary Classification (Cycle Declaration)

- `third_party/llama.cpp` integration path (`t81_llama_adapter`, CLI
  `llama-run`) is classified as: **governed non-DCP**.
- Classification rationale:
  - AGI-facing inference/policy integration surface.
  - Outside DCP-certified deterministic core.
  - Determinism claims remain bounded to practical reproducibility evidence and
    do not imply registry-Verified deterministic guarantees.
- Promotion requirement:
  - Any upgrade beyond governed non-DCP must follow
    `docs/status/GOVERNED_AGI_PROMOTION_PIPELINE.md` and corresponding
    registry/threat-model updates.

## Candidate Verification Snapshot (2026-02-26, refreshed at 13:44:01Z) — SUPERSEDED

- Candidate SHA: `b4fdf8efdf249e391f7c93fb18cf9245926b6a38` — SUPERSEDED by `fa343e1f`.
- Decision implication at time of snapshot: GO criteria not met. Packet held.

## Candidate Verification Snapshot (2026-02-28)

- Candidate SHA selected: `fa343e1f` (`origin/main`, HEAD as of 2026-02-28)
- Required contexts configured on `main` (confirmed via branch protection API):
  - `quality gate / required`
  - `Analyze (cpp)`
- Required-context verification results for `fa343e1f`:
  - `quality gate / required`: **completed / success** ✅
  - `Analyze (cpp)`: **completed / success** ✅
- Additional check-run results (non-required, informational):
  - `gate / t81lang cross-arch bit-identity`: completed / success
  - `gate / t3k cross-arch bit-identity`: completed / success
  - `gate / tritwise-determinism / no-simd`: completed / success
  - `gate / tritwise-determinism / avx2-asan`: completed / success
  - `gate / determinism slice / linux-x86_64 / clang`: completed / success
  - `build / macos-x86_64 / gcc`, `build / macos-arm64 / clang`, `build / linux-x86_64 / gcc`,
    `build / linux-x86_64 / clang`, `build / linux-arm64 / clang`: all completed / success
  - `build / sanitizers`: completed / success
  - `static analysis / clang-tidy`: completed / success
  - `fuzzing / frontend`: completed / success
  - `formal verification / ternary logic`: completed / success
  - `benchmark / linux-x86_64`, `benchmark / vm workload gate`: completed / success
  - `cxx-std / linux-x86_64 / clang / ON`, `cxx-std / linux-x86_64 / clang / OFF`: completed / success
  - `governance-metrics`, `lint / spec & docs`, `contract-sync`, `format / clang-format`: completed / success
  - `architecture / invariants (informational)`, `product / dcp integrity (informational)`: completed / success
- Non-required failure:
  - `build` (GitHub Pages / Jekyll): **completed / failure** — run `22514402961`
  - Root cause: Jekyll renders `third_party/llama.cpp/examples/model-conversion/scripts/embedding/modelcard.template`
    which contains Liquid `{{ }}` syntax, causing a Jekyll Liquid rendering error.
    This is a pre-existing Pages build issue unrelated to C++ correctness, determinism, or release quality.
  - Classification: **Deferred** — non-required workflow; does not block C++ release artifact.
    Tracking note: file an issue to suppress or exclude `third_party/` from Jekyll rendering scope.
- Decision implication:
  - Both required contexts are `completed` + `success`.
  - GO criteria are met for `fa343e1f`.
  - Non-required failure classified as deferred per release discipline.

## Open Blocking Items

1. ~~Resolve required-context evidence mismatch for `b4fdf8efdf249e391f7c93fb18cf9245926b6a38`~~
   **Resolved**: candidate SHA replaced with `fa343e1f`.
2. ~~Land workflow trigger remediation~~ **Completed 2026-02-26** (`ad6c2777`):
   `.github/workflows/codeql.yml` now includes `push` trigger on `main`.
3. ~~Capture `Analyze (cpp)` as explicit `completed` + `success`~~
   **Completed 2026-02-28**: `Analyze (cpp)` is `completed / success` on `fa343e1f`.
4. ~~Re-evaluate GO/HOLD~~ **GO criteria met** on `fa343e1f` as of 2026-02-28.
   Pending final @t81dev decision stamp to convert recommendation to decision.

## Follow-Up (Non-Blocking)

- File issue to exclude `third_party/` from Jekyll Pages rendering scope to
  resolve the `build` (Pages) non-required failure on future candidates.

## Release Manager Checklist Pointers

- `docs/product/RELEASE_DISCIPLINE.md`
- `docs/status/RELEASE_DISCIPLINE_CHECKLIST.md`
- `.github/RELEASE_TEMPLATE.md`

## Versioning Statement

This packet is a time-bound release-governance artifact and does not override
the authority model, freeze policy, or determinism registry boundaries.
