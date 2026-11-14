# Workflow Action Pinning Audit

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Workflow Action Pinning Audit](#workflow-action-pinning-audit)
  - [Summary](#summary)
  - [Tag/Major References (Migration Candidates)](#tagmajor-references-migration-candidates)
  - [SHA-Pinned References](#sha-pinned-references)
  - [Recommendation](#recommendation)

<!-- T81-TOC:END -->


## Summary

- Total `uses:` references: **49**
- Pinned to immutable SHA/digest: **49**
- Tag/major-version references: **0**
- Unclassified references: **0**

## Tag/Major References (Migration Candidates)

| Workflow | Line | Reference |
| --- | ---: | --- |
| n/a | n/a | n/a |

## SHA-Pinned References

| Workflow | Line | Reference |
| --- | ---: | --- |
| `.github/workflows/bench.yml` | 64 | `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02` |
| `.github/workflows/bench.yml` | 116 | `peter-evans/create-pull-request@c0f553fe549906ede9cf27b5156039d195d2ece0` |
| `.github/workflows/ci.yml` | 26 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/ci.yml` | 29 | `actions/setup-python@a309ff8b426b58ec0e2a45f0f869d46889d02405` |
| `.github/workflows/ci.yml` | 41 | `lycheeverse/lychee-action@a8c4c7cb88f0c7386610c35eb25108e448569cb0` |
| `.github/workflows/ci.yml` | 121 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/ci.yml` | 160 | `actions/cache@0057852bfaa89a56745cba8c7296529d2fc39830` |
| `.github/workflows/ci.yml` | 168 | `actions/cache@0057852bfaa89a56745cba8c7296529d2fc39830` |
| `.github/workflows/ci.yml` | 179 | `ilammy/msvc-dev-cmd@0b201ec74fa43914dc39ae48a89fd1d8cb592756` |
| `.github/workflows/ci.yml` | 237 | `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02` |
| `.github/workflows/ci.yml` | 247 | `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02` |
| `.github/workflows/ci.yml` | 255 | `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02` |
| `.github/workflows/ci.yml` | 297 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/ci.yml` | 338 | `actions/download-artifact@d3f86a106a0bac45b974a628896c90dbdf5c8093` |
| `.github/workflows/ci.yml` | 344 | `actions/download-artifact@d3f86a106a0bac45b974a628896c90dbdf5c8093` |
| `.github/workflows/ci.yml` | 383 | `actions/download-artifact@d3f86a106a0bac45b974a628896c90dbdf5c8093` |
| `.github/workflows/ci.yml` | 389 | `actions/download-artifact@d3f86a106a0bac45b974a628896c90dbdf5c8093` |
| `.github/workflows/ci.yml` | 427 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/ci.yml` | 451 | `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02` |
| `.github/workflows/ci.yml` | 469 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/ci.yml` | 495 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/ci.yml` | 524 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/ci.yml` | 552 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/codeql.yml` | 33 | `actions/checkout@34e114876b0b11c390a56381ad16ebd13914f8d5` |
| `.github/workflows/codeql.yml` | 36 | `github/codeql-action/init@d3ced5c96c16c4332e2a61eb6f3649d6f1b20bb8` |
| `.github/workflows/codeql.yml` | 41 | `github/codeql-action/autobuild@d3ced5c96c16c4332e2a61eb6f3649d6f1b20bb8` |
| `.github/workflows/codeql.yml` | 44 | `github/codeql-action/analyze@d3ced5c96c16c4332e2a61eb6f3649d6f1b20bb8` |
| `.github/workflows/release.yml` | 33 | `actions/checkout@34e114876b0b11c390a56381ad16ebd13914f8d5` |
| `.github/workflows/release.yml` | 51 | `anchore/sbom-action@fbfd9c6c189226748411491745178e0c2017392d` |
| `.github/workflows/release.yml` | 57 | `sigstore/cosign-installer@faadad0cce49287aee09b3a48701e75088a2c6ad` |
| `.github/workflows/release.yml` | 68 | `softprops/action-gh-release@5be0e66d93ac7ed76da52eca8bb058f665c3a5fe` |
| `.github/workflows/repro-ledger.yml` | 19 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/repro-ledger.yml` | 85 | `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02` |
| `.github/workflows/runtime-contract.yml` | 24 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/runtime-contract.yml` | 30 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/sidebar.yml` | 23 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/sidebar.yml` | 28 | `actions/setup-python@a309ff8b426b58ec0e2a45f0f869d46889d02405` |
| `.github/workflows/sidebar.yml` | 45 | `peter-evans/create-pull-request@c0f553fe549906ede9cf27b5156039d195d2ece0` |
| `.github/workflows/static.yml` | 22 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/static.yml` | 27 | `actions/setup-node@6044e13b5dc448c55e2357c09f80417699197238` |
| `.github/workflows/static.yml` | 48 | `peter-evans/create-pull-request@c0f553fe549906ede9cf27b5156039d195d2ece0` |
| `.github/workflows/t81lang-repro-hash-refresh.yml` | 17 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/t81lang-repro-hash-refresh.yml` | 45 | `peter-evans/create-pull-request@c0f553fe549906ede9cf27b5156039d195d2ece0` |
| `.github/workflows/toc.yml` | 24 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/toc.yml` | 29 | `actions/setup-python@a309ff8b426b58ec0e2a45f0f869d46889d02405` |
| `.github/workflows/toc.yml` | 48 | `peter-evans/create-pull-request@c0f553fe549906ede9cf27b5156039d195d2ece0` |
| `.github/workflows/pdf.yaml` | 20 | `actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| `.github/workflows/pdf.yaml` | 23 | `docker://marpteam/marp-cli:v4.2.3@sha256:472c9e9203391568cd777a70455ee241ee8d16a123dbbc2329277a35b6c56fc8` |
| `.github/workflows/pdf.yaml` | 36 | `actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02` |

## Recommendation

- Keep all workflow references pinned to immutable SHAs/digests.
- Use Dependabot for GitHub Actions to roll forward pinned SHAs through reviewable PRs.
- Re-run this audit after workflow edits to prevent tag regressions.

