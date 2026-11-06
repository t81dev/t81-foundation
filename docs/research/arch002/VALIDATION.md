# ARCH-002 documentation validation

Baseline: `d134407ae1059c4d7f6a9418177e695e0e5d3b51`. Assessment date: 2026-11-05. These are documentation
checks, not correctness experiments or independent scientific validation.

## Results

- Proposal syntax and Draft 2020-12 schema with format checking: PASS.
- Commons contribution shape/reference check: exit 0, `valid_proposal_shape`,
  `execution: unavailable`; no authorization authenticity or scientific acceptance.
- All six discovery operation types passed (including both H03 and H05).
- 6 commit-pinned Markdown source links/line anchors resolve locally;
  relative protocol/proposal/validation links resolve.
- Static consistency checks cover the shared 0.99375 t quantile, 0.975 per-endpoint
  planning power, latency threshold, strict admission, caps and phase separation.
  They do not establish statistical assumptions or validate a future harness.
- No kernel/oracle implementation, scientific corpus generation, timing pilot,
  benchmark, simulation, production change, CMake build or CTest was executed.
- Workflow files were inspected; the documentation commit uses `[skip ci]` to
  avoid automatically requesting performance execution. No CI success is claimed.

## Tool/environment and repeatable checks

- Python `3.14.3`, `Darwin arm64`.
- jsonschema `4.23.0` from temporary
  `PYTHONPATH=/tmp/t81-h02-r01-schema`; no dependency files changed.
- Exact source reads: `git show <baseline>:<path>`; SHA-256 below.
- Schema validation: `Draft202012Validator(schema, format_checker=FormatChecker()).validate(proposal)`.
- Contribution validation:

```sh
PYTHONPATH=/tmp/t81-h02-r01-schema python3 -B tools/research_commons/commons.py --validate-contribution docs/research/arch002/proposal.json
```

The temporary dependency path is local tooling, not a required production path;
a reviewer can use any environment with the recorded validator.

## Non-duplication audit

Tracked-main searches were scoped to source/tests/benchmarks/experiments and
research indexes. Matching masks, catalog filtering, codec and arithmetic work
was then inspected as listed in PROTOCOL section 1. Remote GitHub PR searches
`repo:t81dev/t81-foundation "equality"` and `repo:t81dev/t81-foundation "ARCH-002"`
returned related work and no ARCH-002 PR respectively. Search results are not a
proof about all unpublished or unindexed work. No equivalent completed packet
was found; discovery of one before implementation triggers STOP/reuse.

Command: `git grep -n -i -E count_equal|equality_count|equal_positions|hamming d134407ae1059c4d7f6a9418177e695e0e5d3b51 -- include benchmarks experiments tests :!*.json :!*.txt :!*.log :!*.gz`

Exit 1; no matches.

Command: `git grep -n -i -E ARCH-002|ARCH-001 d134407ae1059c4d7f6a9418177e695e0e5d3b51 -- docs/research experiments benchmarks`

Exit 1; no matches.

## Discovery receipts

Operation | Exit | SHA-256 of exact response
---|---:|---
`list_research_tracks` | 0 | `e71052118759ee614d09f5212a79f9a54e359eb8d06752bf70ea251e7c69d13c`
`list_hypotheses` | 0 | `c93c6abcdf2e2730724133864e5bdd64d0ed19235f519b734da8923cb50b7f8e`
`get_hypothesis` | 0 | `d0206aab2c9819bfdd39efb96c738351666059d2c4b800a0512ab19ddde4e0d5`
`get_hypothesis` | 0 | `1c85aa6a81d283183a43798cf929e6c608acc1ed9fc0294cab03aabdfc7403c0`
`get_evidence` | 0 | `ac354e8682acc430f6515727976d675f58deec367c0ae778313ed2afca62d37e`
`get_reproduction_instructions` | 0 | `51edbba586c90820c0b689641a05ae9b8bb58292b12d94759ae64db22156e095`
`list_open_research_questions` | 0 | `4ba2a1d2e9d6a570cdbd30b7dc66452b5682219bddd763e0c618ad51c9ac5a26`

Responses use the Commons historical manifest, not live repository status.

## Inspected source identities

Path | SHA-256 at baseline
---|---
| `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json` | `af8ca65dcef136f294eb33010b5a6621875a52db4f3e60b94a4e28db5318a1c1` |
| `include/t81/experimental/packed_trit_vector.hpp` | `6da8852191272edeaa85d310bbb26be43b9660ba656e180db4727d9a796cb67a` |
| `include/t81/packed_trit_vector.hpp` | `f2695aa4d6ab21874732abefa1f23f0eb65dd956e61c0dd7570741c620bc7ac6` |
| `codec/trit_packing.cpp` | `0675981cf277cb66e4ae1f7c2eb5939c0c89c3d546408e76310d7e7524037a4a` |
| `include/t81/native.hpp` | `f982788052247bcdfacfe368e0d719f29bdc396a20bc1695a8f4264ba32f7952` |
| `include/t81/support/bitopts.hpp` | `403ce1f5ff55d0a533ed86a23021c83d91b28a5124d5a35b25e148029ce388fc` |
| `tests/cpp/test_tritwise_backend_equivalence.cpp` | `e3c40bf11cde8181160b8825e13ef43836cf956e876238c348b2bcf6377b5ebc` |
| `tests/cpp/test_packed_trit_vector.cpp` | `33aa93749ba805fc3e6e57011dbb25ae1b9605ff704ba9fed571eabfa4f5f4a2` |
| `benchmarks/BM_TritwiseWorkloads.cpp` | `09426f5882b0b3893f9eddf409dcd454fb0b36894d56be1c4d1190d4bfe31929` |
| `benchmarks/BM_PackedTritVector.cpp` | `f2363c506dc28d65675a605be733f1fe783a47f6900c7af7aa0277c22c72e30f` |
| `experiments/h04_r03_matched_codec/SPECIFICATION.md` | `27d6748476f2ae662d96fccecf831380aae9847203bf6cd93a76beb742469b22` |
| `docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md` | `a620a0494d6272fe6f0ce3287b1c9997aad77765a226caa2b8563bdcb48523eb` |
| `docs/research/commons/manifest.v1.json` | `c6e2642365399b85a98e53727d1ec5a8240c3cb51ea86416565543a0658de911` |

## Final document-only checks

Final strict JSON parsing/schema and local Markdown-link checks: PASS. The
staged diff passes `git diff --cached --check` and contains exactly the three
new files in this directory. Static checks confirm the final shared t quantile,
latency threshold, pilot cap definition and explicit drift/order rules.
No experimental execution or remote CI result is claimed.
