# Workflow Permissions Audit

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Workflow Permissions Audit](#workflow-permissions-audit)
  - [Summary](#summary)
  - [Workflow Matrix](#workflow-matrix)
  - [Recommendation](#recommendation)

<!-- T81-TOC:END -->


## Summary

- Total workflows: **11**
- Explicit `permissions` blocks: **11**
- Missing explicit `permissions`: **0**
- Workflows with write scopes: **7**

## Workflow Matrix

| Workflow | Explicit | Write Scopes |
| --- | --- | --- |
| `.github/workflows/bench.yml` | yes | `contents, pull-requests` |
| `.github/workflows/ci.yml` | yes | `none` |
| `.github/workflows/codeql.yml` | yes | `security-events` |
| `.github/workflows/pdf.yaml` | yes | `none` |
| `.github/workflows/release.yml` | yes | `contents, id-token` |
| `.github/workflows/repro-ledger.yml` | yes | `none` |
| `.github/workflows/runtime-contract.yml` | yes | `none` |
| `.github/workflows/sidebar.yml` | yes | `contents, pull-requests` |
| `.github/workflows/static.yml` | yes | `contents, pull-requests` |
| `.github/workflows/t81lang-repro-hash-refresh.yml` | yes | `contents, pull-requests` |
| `.github/workflows/toc.yml` | yes | `contents, pull-requests` |

## Recommendation

- Keep explicit `permissions` on every workflow.
- Limit `write` scopes to workflows that mutate repo state or publish releases/artifacts requiring it.
- Re-run this audit after workflow edits.
