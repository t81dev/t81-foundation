# Docs Triage Report (2026-02-26)

Status: Active
Scope: `docs/**` full file triage (markdown + support artifacts).
Method: content scan + metadata extraction + inbound-link heuristic (docs-local), with README safety override.

## Recommendation Totals

| Recommendation | Count |
| :--- | ---: |
| `keep` | 113 |
| `keep-archived` | 25 |
| `update` | 133 |

## Per-Directory Summary

| Directory | keep | keep-archived | update | move-to-archive |
| :--- | ---: | ---: | ---: | ---: |
| `.DS_Store` | 1 | 0 | 0 | 0 |
| `.bundle` | 1 | 0 | 0 | 0 |
| `README.md` | 1 | 0 | 0 | 0 |
| `architecture` | 18 | 0 | 16 | 0 |
| `benchmarks` | 1 | 0 | 2 | 0 |
| `contributing` | 1 | 0 | 1 | 0 |
| `explanation` | 4 | 0 | 5 | 0 |
| `governance` | 7 | 0 | 14 | 0 |
| `guides` | 13 | 0 | 15 | 0 |
| `how-to` | 3 | 0 | 4 | 0 |
| `index.md` | 0 | 0 | 1 | 0 |
| `migration` | 1 | 0 | 1 | 0 |
| `navigation.md` | 1 | 0 | 0 | 0 |
| `policies` | 2 | 0 | 5 | 0 |
| `product` | 3 | 0 | 1 | 0 |
| `proposals` | 1 | 0 | 3 | 0 |
| `records` | 14 | 25 | 10 | 0 |
| `reference` | 6 | 0 | 9 | 0 |
| `releases` | 1 | 0 | 1 | 0 |
| `repo_audit_report.md` | 0 | 0 | 1 | 0 |
| `research` | 1 | 0 | 3 | 0 |
| `rfcs` | 1 | 0 | 1 | 0 |
| `roadmaps-plans` | 3 | 0 | 12 | 0 |
| `site` | 17 | 0 | 0 | 0 |
| `spec` | 5 | 0 | 1 | 0 |
| `standards` | 1 | 0 | 2 | 0 |
| `status` | 3 | 0 | 23 | 0 |
| `tutorials` | 3 | 0 | 2 | 0 |

## Move-to-Archive Candidates

No automatic move-to-archive candidates after README safety override.

## Immediate Actions Suggested

1. Refresh inventory snapshots because docs tree changed:
   - `docs/records/inventories/repo_tree.txt`
   - `docs/records/inventories/repo_tree_expanded.txt`
   - `docs/records/inventories/repo_inventory.tsv`
2. Review `update` candidates with zero inbound docs links for either cross-linking or archival.

## Artifacts

- Full matrix: `docs/records/inventories/docs_triage_matrix_2026-02-26.tsv`

## Constraints

- No deletions were recommended automatically; deletion requires manual governance decision.
