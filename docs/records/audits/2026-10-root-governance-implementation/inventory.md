# Independent governance inventories

Before uses exact #579. Required-command after keeps the original invocation.
Standalone after independently executes the same 27 commands, adding only
`--structural` to the direct root check. The aggregate hygiene wrapper still
uses its unchanged default command. This is not an activated CI result.

| Command | Before exit | Required-command after | Standalone after |
| --- | ---: | ---: | ---: |
| `python3 scripts/ci/check_architecture_targets.py` | 0 | 0 | 0 |
| `python3 scripts/ci/check_tisc_freeze_integrity.py` | 0 | 0 | 0 |
| `python3 scripts/ci/check_ai_experiment_boundary.py` | 0 | 0 | 0 |
| `python3 scripts/ci/check_architecture_coherence.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_root_structure.py` | 1 | 1 | 0 |
| `python3 scripts/governance/check_readme_naming.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_translation_metadata.py` | 1 | 1 | 1 |
| `python3 scripts/governance/check_translation_staleness.py` | 1 | 1 | 1 |
| `python3 scripts/governance/check_translation_semantic_alignment.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_docs_structure.py` | 1 | 1 | 1 |
| `python3 scripts/governance/check_active_docs_link_hygiene.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_license_policy.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_repo_artifact_hygiene.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_public_api_semver.py` | 1 | 1 | 1 |
| `python3 scripts/governance/check_spec_code_alignment_baseline.py` | 1 | 1 | 1 |
| `python3 scripts/governance/check_stdlib_surface_baseline.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_stdlib_promotion_snapshot.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_status_label_coherence.py` | 1 | 1 | 1 |
| `python3 scripts/governance/check_cognitive_tier_boundary.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_overclaim_guardrails.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_rfc_lifecycle_hygiene.py` | 1 | 1 | 1 |
| `python3 scripts/governance/check_docs_governance_hygiene.py` | 1 | 1 | 1 |
| `python3 scripts/governance/check_determinism_claims.py` | 0 | 0 | 0 |
| `python3 scripts/ci/governance_metrics.py` | 0 | 0 | 0 |
| `python3 scripts/governance/check_target_name_drift.py` | 0 | 0 | 0 |
| `python3 scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0` | 0 | 0 | 0 |
| `python3 scripts/ci/audit_workflow_permissions.py --max-missing 0` | 0 | 0 | 0 |

Totals: **18/9 → 18/9** required-command behavior; **19/8** standalone view.
Each command was executed independently with stdout/stderr, exit codes, full
history and a 240-second timeout. None timed out or was skipped. Raw files name
their exact evaluated commit. No unrelated check was fixed or suppressed.
