# Separate finding: pre-existing policy-audit exit-status masking

Status: OPEN, not repaired. Documented independently of PR #566's link changes.
Main: `f6d4d680709c5c6c98b305628e04346f5ef61620`.
PR #566: `5bfe915531f0629b3dabb15cda6c374fe86945a0`.

## Demonstrated false-green commands

In main .github/workflows/ci.yml:88 and :92 (PR #566 lines 112 and 116), these existing commands are unchanged:

```bash
python3 scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/ci/audit_workflow_permissions.py --max-missing 0 2>&1 | tee -a .github/logs/spec-and-docs.log
```

[Actual lint job 121185675149](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121185675149) logs ERROR: 7 tagged references exceed allowed maximum 0, and ERROR: 1 missing explicit-permissions declaration exceeds allowed maximum 0, but marks both steps and the job successful. Their scripts genuinely exit 1 when evaluated directly. No threshold was relaxed.

These steps use GitHub's implicit Linux bash shell (`/usr/bin/bash -e {0}`), do not enable pipefail, and do not capture the Python member of PIPESTATUS. Bash reports the last pipeline member's status: successful tee returns 0 even when Python returns 1. errexit (-e) observes that successful pipeline and continues. Multicommand governance blocks can therefore continue after earlier failed validations, as long as their tee processes succeed. Green step/job status is not evidence that these checks passed.

## Reproduction on archived exact-main workflow/script bytes

Archived main .github/workflows and both audit scripts into /private/tmp/t81-pr566-policy-main. Executed the same commands in that directory under existing Bash on macOS ARM64 / Python 3.14.3:

```bash
bash -e -c 'python3 scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0 2>&1 | tee /private/tmp/t81-pr566-failure-audit/main-pinning-masked.log'
bash -eo pipefail -c 'python3 scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0 2>&1 | tee /private/tmp/t81-pr566-failure-audit/main-pinning-strict.log'
bash -e -c 'python3 scripts/ci/audit_workflow_permissions.py --max-missing 0 2>&1 | tee /private/tmp/t81-pr566-failure-audit/main-permissions-masked.log'
bash -eo pipefail -c 'python3 scripts/ci/audit_workflow_permissions.py --max-missing 0 2>&1 | tee /private/tmp/t81-pr566-failure-audit/main-permissions-strict.log'
```

Observed exits: pinning masked=0, strict=1; permissions masked=0, strict=1. The strict form changes only propagation, retaining the same commands, output, violations and thresholds. It was used as a reproduction experiment only; no production workflow was changed. main-policy-pipeline-reproduction.json preserves exact commands, outputs and return codes.

## Other commands with the same risk

There are nine existing lint-job steps and 25 commands with tee pipelines and no explicit pipefail setup. Two policy audits above have demonstrated failing Python processes. The following inventory identifies other commands at risk; it does not assert that all of them currently fail.

### Install Python tools

```bash
pip install mdformat mdformat-gfm mdformat-frontmatter mdformat-toc 2>&1 | tee -a .github/logs/spec-and-docs.log
```

### Verify ARCHITECTURE.md target table matches CMake

```bash
python3 scripts/ci/check_architecture_targets.py 2>&1 | tee -a .github/logs/spec-and-docs.log
```

### Verify TISC v1.1.0 Freeze Integrity

```bash
python3 scripts/ci/check_tisc_freeze_integrity.py 2>&1 | tee -a .github/logs/spec-and-docs.log
```

### Enforce AI experiment sandbox boundary (RFC-00A0)

```bash
python3 scripts/ci/check_ai_experiment_boundary.py 2>&1 | tee -a .github/logs/spec-and-docs.log
```

### Enforce architecture coherence invariants

```bash
python3 scripts/ci/check_architecture_coherence.py 2>&1 | tee -a .github/logs/spec-and-docs.log
```

### Enforce workflow action pinning policy

```bash
python3 scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0 2>&1 | tee -a .github/logs/spec-and-docs.log
```

### Enforce workflow permissions policy

```bash
python3 scripts/ci/audit_workflow_permissions.py --max-missing 0 2>&1 | tee -a .github/logs/spec-and-docs.log
```

### Run governance policy structure/license/artifact checks

```bash
python3 scripts/governance/check_root_structure.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_readme_naming.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_translation_metadata.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_translation_staleness.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_translation_semantic_alignment.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_docs_structure.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_active_docs_link_hygiene.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_license_policy.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_repo_artifact_hygiene.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_public_api_semver.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_spec_code_alignment_baseline.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_stdlib_surface_baseline.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_stdlib_promotion_snapshot.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_status_label_coherence.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_cognitive_tier_boundary.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_overclaim_guardrails.py 2>&1 | tee -a .github/logs/spec-and-docs.log
python3 scripts/governance/check_rfc_lifecycle_hygiene.py 2>&1 | tee -a .github/logs/spec-and-docs.log
```

### Run governance audit wrapper

```bash
python3 scripts/governance/check_docs_governance_hygiene.py 2>&1 | tee -a .github/logs/spec-and-docs.log
```

## Limits and follow-up recommendation

PR #566 did not introduce this defect; its new DeepWiki step uses explicit bash and its own nonzero aggregation for request/status failures. Its 429 warning is a documented, endpoint-specific availability exception, distinct from accidentally masked policy failures. The lychee action independently propagates its exit with fail=true.

A separate reviewed CI correction should make Python/build failures propagate while preserving logs, e.g. explicit bash pipefail or deliberate capture of the relevant PIPESTATUS. Test that a deliberately failing audit makes the step fail while tee output remains available. Fix the existing policy violations independently; do not lower thresholds, add broad exclusions, use continue-on-error, disable checks or reinterpret a green pipeline as substantive success. Review similarly structured pipelines outside this lint job separately rather than assuming this inventory is a repository-wide audit.

This report and the reproduction do not repair or validate policy compliance. No branch protection, workflow permissions, required contexts, production checks or source contracts were changed by this audit.
