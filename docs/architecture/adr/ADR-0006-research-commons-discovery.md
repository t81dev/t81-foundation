# ADR-0006: Read-only Research Commons discovery

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [ADR-0006: Read-only Research Commons discovery](#adr-0006-read-only-research-commons-discovery)
  - [Context and inspection](#context-and-inspection)
  - [Decision](#decision)
  - [Consequences and boundaries](#consequences-and-boundaries)
  - [Additional integrity discrepancy](#additional-integrity-discrepancy)

<!-- T81-TOC:END -->


Date: 2026-10-27. Status: proposed implementation for human review.

## Context and inspection

Inspected README, AGENTS, handoff/roadmap/buildable steps, historical project
profile, both research charters, continuity register/lineage/evidence manifest,
RFC catalog and RFC-00D0/00D1, existing evidence validators, experiments, Python
packaging and CI conventions. Existing structured capabilities include the
continuity hypothesis register and source pins, experiment-specific results and
hash manifests, RFC-00D1/00D3 JSON schemas, and AI execution evidence validators.
Searches of scripts, tools, research docs, workflows and pyproject found no MCP
server or common research interaction interface. Existing agent/TUI/runtime APIs
are execution surfaces, not a scientific evidence registry.

GitHub PR metadata and merged repository artifacts were inspected:

- #605 merged as `827d2accbf8ccb56afe3fe8d96dfb7251f249d7d` into
  `research/investigation09-tisc-language-design`, **not main**. Its E1 packet
  under `experiments/investigation09_e1/` reports NOT SUPPORTED for the scoped
  executable-footprint discriminator. Its protocol is at
  `f1054438b805b541c4acfd294e6dba5a71711c05`; implementation baseline is
  `c930b8c2ee7478782bf3c6d231bdd59984595ef2`. No measurements rerun here.
- #606 merged as `18bd6c78b1ec2c78a500a8e290cb004a062f513e` on main:
  CAPABILITY_BASELINE.md, RFC_EVIDENCE_MATRIX.csv, TISC_T81LANG_COVERAGE.md,
  RESEARCH_GAPS.md and AUDIT_REPRODUCIBILITY.md. These are reported audit
  claims, not independently verified test runs. The packet describes the earlier
  c930b8c baseline, omits merged E1 evidence, mixes ISA versions, cites missing
  paths, and promotes some comparative claims beyond identified controls.
- #607 merged as `274421fd8e85c5b9bb4f165fce190715530f4e85` on main.
  HUMAN_AI_RESEARCH_INITIATIVE.md supplies T81-ORI collaboration intent, but its
  section 4.2 reassigns canonical H01–H08, calls H07 comparatively validated,
  and conflates H08 with H-S1. Its April date predates its October merge;
  percentages and AI methodological benefits are unvalidated assessments.

The continuity charter and original register remain authoritative for H01–H08.
ROOT_DOCUMENTATION_ALIGNMENT_AUDIT.md qualifies comparative assurance claims:
selected output parity is not equivalent computation or assurance. Existing
records remain intact; conflicts become discoverable discrepancy entries.

## Decision

Add a versioned JSON index of existing evidence. Each source has a full Git
commit, repository path, SHA-256 and immutable URL. Read Git objects rather than
mutable checkout files, including #605's separate merged history. Missing objects
fail closed and require explicit human setup; the interface never fetches.
Canonical claims, results, confidence and falsifiers are returned verbatim from
the pinned register, with supplementary evidence and unresolved conflicts linked
separately. Hashes prove byte identity, not correctness or authenticity.

Evaluate MCP as the preferred future integration transport. For this initial
foundation choose a justified equivalent: six allowlisted offline JSON CLI
operations with strict versioned requests and deterministic response envelopes.
No MCP server exists to extend; adding a server SDK, lifecycle and transport
would increase scope before the evidence contract is reviewed. The CLI is not
MCP-compatible and does not claim MCP conformance. Future MCP tools can wrap
these operations after human review without changing the evidence model.

Use Python 3.9+, Git, and a pinned JSON Schema validator; no C++ runtime changes,
network client, database, execution operation, shell interpretation or output-file
option. Separate proposal/review/reproduction/discrepancy schemas from findings.
Every request validates the manifest, references and source identities before
answering. No llms.txt: AGENTS plus the Commons README provide discovery without
another overlapping root narrative.

## Consequences and boundaries

This supports newcomer/handoff clarity and existing evidence surfaces, rather
than expanding draft runtime architecture. It does not authorize experiments,
production substitution seams, RFC acceptance, publication, PR merging, changes
to frozen/accepted artifacts or Protocol 001 activation. Human authorization is
recorded as data, never consumed as an execution capability. Self-authored tests
are implementation checks, not an independent scientific review. Coverage is a
curated initial index, not an exhaustive research census. Historical register
statuses are explicitly dated; supplementary records do not silently revise them.

Test path/object resolution, historical identities, negatives, claim boundaries,
reference failures, deterministic responses, all operations, and absence of
repository mutations. Use a separate Python CI matrix; no root CMake changes.

## Additional integrity discrepancy

Validation found that continuity/evidence/manifest.json records charter hash
`e56d1cb5...`, while actual committed charter bytes at the inspected baseline
hash to `bb073baf...`. The canonical register hash matches. The Commons indexes
both records and flags this mismatch; it does not repair or silently trust the
older packet manifest. Git lazy fetching and optional locks are disabled; the
implementation requires Git 2.46+ conservatively, using its
[documented environment contract](https://git-scm.com/docs/git/2.46.0).
