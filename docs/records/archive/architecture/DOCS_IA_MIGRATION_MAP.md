# Docs IA Migration Map

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Docs IA Migration Map](#docs-ia-migration-map)
  - [Phase 0 Snapshot (pre-move)](#phase-0-snapshot-pre-move)
  - [Move Map](#move-map)
  - [Notes / Deferred Follow-ups](#notes--deferred-follow-ups)

<!-- T81-TOC:END -->


This record captures the `/docs` IA hardening migration that separates:

- Canonical reference docs
- Audit/record artifacts
- Site build machinery

Authority hierarchy remains unchanged:
`/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.

## Phase 0 Snapshot (pre-move)

- Site machinery identified under:
  - `docs/_includes/`
  - `docs/_layouts/`
  - `docs/_config.yml`, `docs/_config.yaml`
  - `docs/assets/`
  - `docs/search/`
- Canonical docs retained in place:
  - `docs/architecture/`
  - `docs/governance/`
  - `docs/status/`
  - `docs/spec/`
  - `docs/guides/`
- Records identified:
  - `docs/audits/`
  - `docs/archive/`
  - inventory/tree/manifest artifacts now grouped under `docs/records/inventories/`

## Move Map

| Old Path | New Path | Category | Link-Update Notes |
| --- | --- | --- | --- |
| `docs/_includes/` | `docs/site/_includes/` | site | Updated markdown/script references to layouts/includes paths. |
| `docs/_layouts/` | `docs/site/_layouts/` | site | Updated references in assets README and inventories. |
| `docs/_config.yml` | `docs/site/_config.yml` | site | Updated references in inventories and grep checks. |
| `docs/_config.yaml` | `docs/site/_config.yaml` | site | Updated references in inventories and grep checks. |
| `docs/assets/` | `docs/site/assets/` | site | Updated references in docs and inventories. |
| `docs/search/` | `docs/site/search/` | site | Updated `package.json` (`docs:search`) and docs references. |
| `docs/audits/` | `docs/records/audits/` | records | Updated audit references in docs + scripts. |
| `docs/archive/` | `docs/records/archive/` | records | Updated archive references in docs and inventories. |
| `docs/records/audits/repo_inventory.tsv` | `docs/records/inventories/repo_inventory.tsv` | records | Moved inventory artifacts out of audit report directory. |
| `docs/records/audits/repo_tree.txt` | `docs/records/inventories/repo_tree.txt` | records | Updated audit README regeneration paths. |
| `docs/records/audits/repo_tree_expanded.txt` | `docs/records/inventories/repo_tree_expanded.txt` | records | Updated audit README regeneration paths. |
| `docs/records/audits/docs_refresh_manifest.tsv` | `docs/records/inventories/docs_refresh_manifest.tsv` | records | Grouped with inventories/manifests. |

## Notes / Deferred Follow-ups

- Tutorials/how-to/reference/explanation surfaces remain as-is in this migration.
- Any deeper canonical taxonomy consolidation is intentionally deferred to a
  dedicated follow-up, to avoid changing technical claims in this pass.
