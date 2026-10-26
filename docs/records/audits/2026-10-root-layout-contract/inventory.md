# Independent governance inventory

Full-history, clean exact #578 baseline. Each command was independently executed
with a 180-second timeout. No skips or timeouts. Raw stdout/stderr and exit codes
are in [inventory.json](inventory.json). The audit files were absent during this
clean inventory; pre/post status records are included.

| Command | #578 recorded exit | Reproduced exit |
| --- | ---: | ---: |
| `python3 scripts/ci/check_architecture_targets.py` | 0 | 0 |
| `python3 scripts/ci/check_tisc_freeze_integrity.py` | 0 | 0 |
| `python3 scripts/ci/check_ai_experiment_boundary.py` | 0 | 0 |
| `python3 scripts/ci/check_architecture_coherence.py` | 0 | 0 |
| `python3 scripts/governance/check_root_structure.py` | 1 | 1 |
| `python3 scripts/governance/check_readme_naming.py` | 0 | 0 |
| `python3 scripts/governance/check_translation_metadata.py` | 1 | 1 |
| `python3 scripts/governance/check_translation_staleness.py` | 1 | 1 |
| `python3 scripts/governance/check_translation_semantic_alignment.py` | 0 | 0 |
| `python3 scripts/governance/check_docs_structure.py` | 1 | 1 |
| `python3 scripts/governance/check_active_docs_link_hygiene.py` | 0 | 0 |
| `python3 scripts/governance/check_license_policy.py` | 0 | 0 |
| `python3 scripts/governance/check_repo_artifact_hygiene.py` | 0 | 0 |
| `python3 scripts/governance/check_public_api_semver.py` | 1 | 1 |
| `python3 scripts/governance/check_spec_code_alignment_baseline.py` | 1 | 1 |
| `python3 scripts/governance/check_stdlib_surface_baseline.py` | 0 | 0 |
| `python3 scripts/governance/check_stdlib_promotion_snapshot.py` | 0 | 0 |
| `python3 scripts/governance/check_status_label_coherence.py` | 1 | 1 |
| `python3 scripts/governance/check_cognitive_tier_boundary.py` | 0 | 0 |
| `python3 scripts/governance/check_overclaim_guardrails.py` | 0 | 0 |
| `python3 scripts/governance/check_rfc_lifecycle_hygiene.py` | 1 | 1 |
| `python3 scripts/governance/check_docs_governance_hygiene.py` | 1 | 1 |
| `python3 scripts/governance/check_determinism_claims.py` | 0 | 0 |
| `python3 scripts/ci/governance_metrics.py` | 0 | 0 |
| `python3 scripts/governance/check_target_name_drift.py` | 0 | 0 |
| `python3 scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0` | 0 | 0 |
| `python3 scripts/ci/audit_workflow_permissions.py --max-missing 0` | 0 | 0 |

**18 passing / 9 failing**, unchanged. Root structure still fails. No 19/8 result
or successful required gate is claimed. The aggregate hygiene wrapper also
continues failing; it is counted independently, not skipped.

Five existing regression programs pass; [regressions.json](regressions.json)
preserves the initially requested bare failure-propagation command exiting 2
because `--output-dir` is required, followed by its successful corrected
invocation (47 cases, zero failures). The other results are 18 architecture
tests, five governance tests, six supply-chain tests, and 209-opcode freeze
integrity. No new root-validator implementation tests were executed: the
proposed policy is inactive, as explained in [TEST_PLAN.md](TEST_PLAN.md).
