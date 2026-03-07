# Governance Review Cadence

Last Updated: 2026-03-06
Owner: @t81dev

Defines: monthly review checklist, drift review protocol, registry update
procedure, and promotion gate checklist. Prevents decay.

---

## 1. Monthly Governance Review Checklist

Run at every C-cycle close (monthly). Required before a release GO stamp.

### 1a. Required Contexts (Branch Protection)

```
gh api repos/t81dev/t81-foundation/branches/main/protection \
  --jq '.required_status_checks.contexts'

gh api repos/t81dev/t81-foundation/commits/<candidate-sha>/check-runs
```

Required contexts must be `completed` + `success`:
- [ ] `quality gate / required`
- [ ] `Analyze (cpp)`

### 1b. Determinism and Integrity Gates

- [ ] `gate / tritwise-determinism / no-simd` — pass
- [ ] `gate / tritwise-determinism / avx2-asan` — pass
- [ ] `gate / t81lang cross-arch bit-identity` — pass
- [ ] `gate / t3k cross-arch bit-identity` — pass
- [ ] `gate / determinism slice / linux-x86_64 / clang` — pass
- [ ] `formal verification / ternary logic` — pass
- [ ] `build / sanitizers` (ASAN + UBSAN) — pass
- [ ] `governance-metrics` — pass

### 1c. Governance Artifacts

- [ ] `FROZEN_CORE_PROFILE.md` — no unauthorized changes since last review
- [ ] `DRIFT_DECOMPOSITION.md` — all rows current; no undiscovered drifts
- [ ] `ACTIVE_RISKS.md` — refreshed; no open High risk without active mitigation
- [ ] `DETERMINISM_AUDIT_LOG.md` — all audits from this cycle logged
- [ ] `HARDENING_BACKLOG.md` — closed items marked; new items added from this cycle
- [ ] `EXTENSION_PROFILE.md` — no new experimental surfaces without registration
- [ ] `IMPLEMENTATION_MATRIX.md` — Last Review date current for all rows
- [ ] `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` — no stale Partial entries without owner
- [ ] `CI_GATE_STATUS.md` — no unresolved failure without explicit Deferred classification

### 1d. Structural Integrity

- [ ] Run dependency firewall check: `scripts/ci/check_core_numeric_wrapper_thinness.py`
- [ ] Verify `include/t81/` contains no experimental headers
- [ ] Verify dependency firewall waiver table is empty or all entries are explicitly justified

### 1e. Stdlib and Surface Baseline

- [ ] `python3 scripts/governance/check_stdlib_surface_baseline.py` — pass
- [ ] `python3 scripts/governance/check_stdlib_promotion_snapshot.py` — pass
- [ ] `python3 scripts/governance/t81lang_promotion_gate_snapshot.py` — pass

### 1f. Release Decision

- [ ] GO criteria met (required contexts satisfied, no Severity-2/3 incidents, no freeze exceptions)
- [ ] GO/HOLD decision stamped in release readiness packet with UTC timestamp, approver, and basis
- [ ] Non-required workflow failures classified as release-impacting or Deferred
- [ ] Decision entered in `DECISION_LOG.md`

---

## 2. Drift Review Protocol

Run monthly alongside 1c above. Takes ~15 minutes.

1. Open `DRIFT_DECOMPOSITION.md`.
2. For each **Open** row: verify the closure plan is still accurate and the target date is current.
3. For each **Monitoring** row: confirm no new failure signal.
4. Identify any new drift (spec changed, implementation changed, or gap discovered). Add a row.
5. Update state on any rows that closed since last review.
6. If any drift is found to affect a Verified surface: immediately open a Severity-2 incident.

---

## 3. Registry Update Procedure

Run when a surface changes maturity (Partial → Verified, or Verified → downgraded).

1. Update `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` — change the Status field.
2. Update `FROZEN_CORE_PROFILE.md` §4 (Verified Determinism Surfaces) to match.
3. Update `EXTENSION_PROFILE.md` if the surface moved in or out of Experimental.
4. Update `IMPLEMENTATION_MATRIX.md` row (Promotion State column).
5. Add a dated entry to `DETERMINISM_AUDIT_LOG.md`.
6. Update `DRIFT_DECOMPOSITION.md` — mark the relevant row Closed or re-open it.
7. Log the decision in `DECISION_LOG.md`.
8. If upgrading to Verified: require governance approval stamp in the release packet.

---

## 4. Promotion Gate Protocol

For a surface to be promoted from Experimental to Verified Candidate to Verified:

### Stage 1: Experimental → Verified Candidate

Required evidence:
- [ ] Architecture Decision Record (ADR) published in `docs/architecture/adr/`
- [ ] Threat model updated in `docs/governance/DETERMINISM_THREAT_MODEL.md`
- [ ] Draft registry entry in `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` (status: Planned)
- [ ] Determinism test suite passing (bit-exact across platforms)
- [ ] Incident-response controls documented and operational
- [ ] No open Severity-2/3 incidents on the surface
- [ ] Promotion evidence snapshot: `python3 scripts/governance/t81lang_promotion_gate_snapshot.py` (or equivalent)
- [ ] Surface registered in `EXTENSION_PROFILE.md` with Promotion State updated

### Stage 2: Verified Candidate → Verified (DCP eligible)

Required evidence (in addition to Stage 1):
- [ ] Governance approval stamp from @t81dev
- [ ] Release packet GO stamp on a candidate SHA with the surface included
- [ ] All Verified Candidate evidence current (no stale entries)
- [ ] Registry status set to Verified
- [ ] `FROZEN_CORE_PROFILE.md` updated to include the surface
- [ ] `EXTENSION_PROFILE.md` row removed or updated to Verified
- [ ] `DRIFT_DECOMPOSITION.md` row marked Closed

### Rollback Triggers

A Verified surface must be immediately downgraded if:
- A determinism regression is found on the surface (any commit)
- Threat-model evidence becomes stale (>90 days without review on an active surface)
- A Severity-2 or higher incident is opened on the surface

Rollback procedure: registry downgrade → `FROZEN_CORE_PROFILE.md` update →
critical defect opened → surface reverts to Partial or Experimental.

---

## 5. T81Lang Promotion Gate Criteria

Current state: Beta (implementation maturity) / Draft (spec authority)

| Criterion | Status |
| :--- | :--- |
| TG-01 — Drift decomposition has active closure tracking | Pass |
| TG-02 — Deterministic compile/repro evidence green | Pass |
| TG-03 — Core semantic conformance §§2-6 green | Pass |
| TG-04 — Open gaps bounded in engineering backlog with acceptance tests | Pass |
| TG-05 — Matrix and governance artifacts synchronized | Pass |
| TG-06 — Determinism claims bounded to registry surfaces | Pass |

Re-run: `python3 scripts/governance/t81lang_promotion_gate_snapshot.py`

---

## 6. C2 Month-Close Execution

Scheduled: **2026-03-31 (UTC)**

Quick execution steps:
1. Run preflight: `python3 scripts/governance/c2_month_close_preflight.py`
2. Run check: `python3 scripts/governance/c2_month_close_check.py`
3. Both must pass before proceeding.
4. Execute items from Monthly Governance Review Checklist (§1 above).
5. Stamp final outcome in `docs/records/audits/2026-03-governance-review.md`.
6. Update `ACTIVE_RISKS.md` to reflect any risk state changes.

Runbook reference: `docs/records/status-history/C2_MONTH_CLOSE_RUNBOOK_2026-03-31.md`

---

## 7. Governance Directives (Non-Negotiable)

1. Spec-first policy: normative behavior changes require spec update before implementation.
2. Freeze boundaries are unchanged unless a major version is bumped.
3. Determinism claims are limited to Verified registry surfaces only — no exceptions.
4. Public API contract is `include/t81/**` only — no experimental headers in public API.
5. Root-level documentation additions require explicit governance approval.
6. All GO/HOLD decisions must be stamped with UTC timestamp, approver, and basis.
7. Non-required CI failures that affect release artifacts must be classified Deferred
   with a tracking note — not silently ignored.

---

## Cross-References

- `docs/status/ACTIVE_RISKS.md`
- `docs/status/DRIFT_DECOMPOSITION.md`
- `docs/status/FROZEN_CORE_PROFILE.md`
- `docs/status/HARDENING_BACKLOG.md`
- `docs/status/EXTENSION_PROFILE.md`
- `docs/status/DECISION_LOG.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`
