# Release Discipline Checklist (Status View)

Status: Active
Last Updated: 2026-02-25
Owner: Release / Governance

## Purpose

Provide the operational execution checklist for release managers.

## Source of Truth

Normative release policy and SemVer interpretation are defined in:

- `docs/product/RELEASE_DISCIPLINE.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`

This status checklist must remain aligned with those documents and must not
duplicate or redefine their versioning policy.

## Pre-Tag Checklist (`vX.Y.Z`)

- [ ] CI required checks green for target commit
- [ ] Determinism registry status reviewed:
      `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- [ ] DCP scope validated:
      `docs/product/DETERMINISTIC_CORE_PROFILE.md`
- [ ] Structural integrity status reviewed:
      `docs/status/STRUCTURAL_INTEGRITY_REPORT.md`
- [ ] Verified surface audit current:
      `docs/status/VERIFIED_SURFACE_AUDIT.md`
- [ ] Deterministic corpus references current:
      `docs/status/DETERMINISTIC_CORPUS_MANIFEST.md`
- [ ] No open freeze exceptions for release scope
- [ ] No open Severity 2/3 determinism incidents
- [ ] Release notes include determinism hash summary and experimental delta

## Required-Context Verification Procedure

1. Resolve release-candidate commit SHA:

```bash
git rev-parse HEAD
```

2. Query required branch-protection contexts:

```bash
gh api repos/t81dev/t81-foundation/branches/main/protection --jq '.required_status_checks.contexts'
```

3. Query check runs for release-candidate SHA:

```bash
sha=$(git rev-parse HEAD)
gh api repos/t81dev/t81-foundation/commits/$sha/check-runs \
  --jq '.check_runs[] | {name:.name,status:.status,conclusion:.conclusion,details_url:.details_url}'
```

4. Verify that every required context is present for the SHA and has:
   - `status=completed`
   - `conclusion=success`

5. Record context-level evidence in the active release-readiness packet.

## Release Decision Gate

Decision is **GO** only when all are true:

1. All required branch-protection contexts are successful for the release SHA.
2. No open `freeze-exception` blockers for release scope.
3. No unresolved Severity 2/3 determinism incidents.
4. DCP and determinism evidence artifacts are current.

If any condition fails, decision is **HOLD**.

## Non-Required Workflow Failure Rule

1. Enumerate all failed non-required workflows/jobs for release SHA.
2. Classify each as:
   - release-impacting (must be fixed before GO), or
   - non-release-impacting (waiver allowed).
3. Waiver record is mandatory for non-release-impacting failures and must
   include:
   - workflow/job name,
   - SHA,
   - rationale,
   - owner,
   - remediation target date.
4. Failures affecting DCP, verified determinism surfaces, freeze boundaries, or
   public API contract are release-impacting by default.
5. Record disposition in the release-readiness packet before final GO/HOLD
   stamp.

## Required Release Artifacts

1. Source package for tagged commit.
2. Determinism evidence summary (gate outputs and hash references).
3. Structural integrity status reference.
4. Verified surface audit reference.
5. Changelog/release notes with breaking-change visibility where applicable.

## Cross-References

- `docs/records/status-history/RELEASE_READINESS_PACKET_2026-02.md`
- `.github/RELEASE_TEMPLATE.md`

## Versioning Statement

Checklist changes must not weaken release controls defined in the source policy
documents.
