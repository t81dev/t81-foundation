# Research Commons implementation and limitations

Date: 2026-10-27. Implementation author: Codex; exact underlying model identity
not recorded. This is implementation verification, not independent research
validation. No research experiment was executed by this assignment.

## Delivered boundary

[ADR-0006](../../architecture/adr/ADR-0006-research-commons-discovery.md) precedes
the implementation. The initial index contains 35 pinned source documents, three
tracks and eight canonical hypotheses, with dated historical dispositions,
supplementary evidence, reproduction pointers, explicit uncertainty, governance
and ten open research/discrepancy/limitation entries. Documents and accepted
outcomes remain in their existing locations. No protected ISA, language, VM,
Axion, CanonFS, frozen fixture or historical charter/register was edited.

The Python CLI implements all six requested read operations using immutable Git
objects and strict JSON requests. MCP was evaluated; the ADR chooses an offline
JSON CLI equivalent to avoid a new server dependency/lifecycle before contract
review. There is no MCP transport, server registration, write API, experiment
execution, external API client, auto-fetch, merge operation or governance activation.
Git 2.46+ is required; lazy fetching, optional locks and prompts are disabled.

Self-contained JSON Schemas cover discovery manifest, requests, response envelopes,
experiment proposals, independent-review drafts, reproduction-report drafts and
evidence-discrepancy drafts. Four illustrative contribution records exercise the
schemas. The [fresh-agent walkthrough](ONBOARDING_EXAMPLE.md) discovers canonical
H04, retrieves supplementary model negatives and scope limitations, and prepares
an unexecuted, unauthorized experiment proposal with matched adaptive binary control.
This walkthrough is not represented as an actual independent-agent review.

## Evidence reconciliation

The manifest's main baseline is
`274421fd8e85c5b9bb4f165fce190715530f4e85` (#607). The original active checkout
was #605's experiment branch at `4778e94fec498dfd431977c982dc2c1374966869`;
implementation used a separate worktree based on main, preserving the original
branch and its untracked build-bench directory.

- PR605 is merged at `827d2accbf8ccb56afe3fe8d96dfb7251f249d7d` into the
  Investigation09 proposal branch, not main. Its scoped E1 negative, ledgers,
  hash manifest, entry contract and reproduction README resolve at that commit.
- PR606 is merged at `18bd6c78b1ec2c78a500a8e290cb004a062f513e`. Its five
  audit artifacts remain reported/disputed evidence. A literal comma-split scan
  of source-like CSV paths found 46 absent candidates at the inspected main
  baseline, including `src/policy/axion_policy.cpp` and `lang/parser.cpp`.
  This is a pointer check, not a full audit of every capability. Reported 408-test
  passes have not been independently established here. Encoding/API and ISA
  version wording need reconciliation; 2-bit versus 8-bit raw packing is 4x,
  whereas log2(3) describes entropy per trit.
- PR607's section4.2 redefines canonical IDs and overstates comparative assurance;
  the index preserves the original register and exposes the disagreement. H08
  remains AI-assisted research methodology, distinct from Investigation08/H-S1.
- R02's blanket software conclusion exceeds its symbolic model's limits.
- The older continuity manifest's charter hash is `e56d1cb5...`, but actual pinned
  committed bytes hash to `bb073baf...`. Both are indexed and the discrepancy
  remains open. No protected artifact was repaired to force validation.

These are surfaced disagreements, not amendments to accepted research outcomes.

## Verification actually performed

Environment: macOS 26.5.2, ARM64; Python 3.14.3; Git 2.53.0. Temporary venv:
`/private/tmp/t81-commons-venv`. Installed packages: jsonschema 4.23.0,
attrs 26.1.0, jsonschema-specifications 2025.9.1, referencing 0.37.0,
rpds-py 2026.9.1. Only the direct schema dependency is pinned in requirements;
these are the exact local resolved versions, not a cross-platform lockfile.

Executed from the implementation worktree:

```sh
/private/tmp/t81-commons-venv/bin/python -B tools/research_commons/commons.py --validate
/private/tmp/t81-commons-venv/bin/python -B -m unittest discover -s tests/research_commons -v
python3 scripts/governance/check_root_structure.py
python3 scripts/ci/check_tisc_freeze_integrity.py
git diff --check
```

Results: 35 source identities resolve; **16 tests pass**; root-structure check
passes (81 entries); freeze-integrity check passes (209 registered opcodes);
whitespace check passes. The freeze checker uses its own historical v1.1.0 label;
that output does not resolve the documentation's version inconsistency.

Tests cover all published source paths at their revisions, canonical verbatim
H01–H08 identities, negatives, no unsupported status promotion, strict requests,
missing references/files/commits, hash/URL/path tampering, duplicate IDs/keys,
proposal requirements, all contribution examples, working-tree substitution resistance
and deterministic responses.
All six operations run with socket construction blocked and Git calls limited
to version/ls-tree/cat-file; full worktree hashes, Git HEAD/refs/status and index
hash remain equal before and after. This is evidence about tested paths, not a
security sandbox against arbitrary modifications to the implementation or a
malicious host. Git metadata/configuration is trusted.

The [demonstration receipt](demo-observation.json) retains actual implementation-
author API request/response hashes for all six operations plus proposal-shape
validation. A test recomputes those hashes. Canonical manifest SHA-256:
`81ea5ba5e39d47ffe69bb92e6137c3b0170c176eb8874ac8153a9d5ddd4b7b69`.
No receipt claims experiment execution, genuine reviewer independence or
scientific acceptance.

## Portability assessment and unresolved limits

Reviewed Linux, macOS, Windows and ARM64 implications: Git object reads avoid
checkout newline conversion; repository paths use explicit POSIX syntax while
host paths use pathlib; subprocess receives argument arrays without shell syntax;
no architecture-specific native T81 dependency is imported. JSON responses omit
absolute host paths and clocks. Git regular-file mode checks reject symlink,
directory and submodule retrieval. Python syntax uses the project's 3.9 minimum.

A dedicated CI workflow targets Linux x86-64, Linux ARM64, macOS ARM64 and Windows,
with Python 3.9/3.13. Those platforms/versions are **not locally executed** here;
CI results remain pending. No C++ configure/build/full CTest run, old experiment
rerun, external link crawl, live security certification or benchmark replication
is claimed. No native code changed, so the narrow verification slice is Python
contract/schema checks plus existing governance/freeze checks.

The index is curated, not exhaustive. A valid source reference proves available
bytes, not that every nested citation in a report resolves. Scientific adequacy,
authentic human authorization, matched-control quality and reviewer independence
need human review and independent work. Historical register dispositions remain
dated. New evidence/index updates require review; missing #605 objects require
explicit setup outside the read-only interface. llms.txt was omitted because it
would duplicate AGENTS and this discovery guide. A future MCP wrapper can reuse
the contract after its scope and dependencies receive review.
