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

## Evidence integration checkpoint — 2026-10-27 (PR610 not merged)

**Dependency remains open.** GitHub metadata checked during this update reports
PR610 open/draft, `merged: false`, head
`d1eee548d4658df37b35d0cc3781b5a77cbae96b`. Its API-generated potential merge
SHA is not treated as an actual merged revision. No correction from PR610 is
claimed as part of main, and PR609 remains draft. Completing the corrected-main
snapshot requires human review/merge of PR610, verification of its actual merge
commit, a subsequent pin/history/receipt update, and rerun validation. Commons
never performs that merge or fetches evidence on demand.

The original #609 work at `7183ae0de2a6d069fa1c422af335294909736eff` is
preserved. Merged main at `fabc7aa90f70ead227ec257a1ae6833fa16ced40`
(PR608) was merged into the same branch without conflict. The manifest now pins
that actual main baseline and contains **39** source identities. PR607's original
initiative at `274421fd8e85c5b9bb4f165fce190715530f4e85` remains available as
`initiative`; PR608 is `initiative-608`; PR610's actual head objects are available
only under explicit `*-610-proposed` IDs with role `proposal` and UNMERGED
uncertainty. The PR608 reconciliation report and PR610 append-only proposal report
are also separately indexed. E1 remains pinned to its separately merged
`827d2accbf8ccb56afe3fe8d96dfb7251f249d7d` history.

The index retains three discrepancy-history records, including superseded PR607
ID/wording assertions with the limits of PR608's reconciliation. PR608's remaining
H07/H08 discrepancy stays unresolved because its correction is only proposed.
Eleven current questions remain. `get_hypothesis` exposes the history and pending
integration state using the existing operation. The six-operation interface,
request protocol, execution prohibition and offline Git-only reader are retained.

Canonical H01–H08 records are returned verbatim. The register SHA-256 remains
`af8ca65dcef136f294eb33010b5a6621875a52db4f3e60b94a4e28db5318a1c1`.
H07 retains B/C evidence, no new independent security certification, the original
confidence/next action and 2026-10-25 disposition. H08 retains C/E evidence,
`Researcher intent authoritative; comparative benefit untested.`, unavailable
earlier prompt provenance and the prohibition on treating repeated model
summaries as independent verification. No charter/register bytes were edited.

### Commands and actual local results for this update

Same macOS ARM64 / Python 3.14.3 / Git 2.53.0 environment as the initial report.
Explicit operator setup outside the interface fetched main and the PR610 branch;
CI setup explicitly fetches the pinned PR610 object in addition to the existing
PR605 object. Installing or fetching is never a discovery operation.

```sh
git fetch origin main docs/pr608-evidence-correction
git merge --no-edit origin/main
/private/tmp/t81-commons-venv/bin/python -B tools/research_commons/commons.py --validate
/private/tmp/t81-commons-venv/bin/python -B -m unittest discover -s tests/research_commons -v
/private/tmp/t81-commons-venv/bin/python -B tools/research_commons/commons.py --validate-contribution docs/research/commons/examples/h08-proposal.json
python3 scripts/ci/check_tisc_freeze_integrity.py
python3 scripts/governance/check_root_structure.py
python3 scripts/governance/check_overclaim_guardrails.py
python3 scripts/governance/check_active_docs_link_hygiene.py
python3 scripts/governance/check_determinism_claims.py
python3 scripts/governance/check_docs_structure.py
git diff --check
```

- **PASS:** All 39 actual source Git objects, hashes, regular-file paths and
  evidence references validate. Missing or promoted proposal references fail
  closed. Active canonical manifest SHA-256:
  `a3ee2d288ac7b00176beae32f854c373194b11556d83293396bbcd7c02d688bf`.
- **PASS:** 19 Commons tests, including deterministic responses, retained Hanoi
  and E1 negatives, verbatim H07/H08 limits, historical/pending source separation,
  no-network/no-mutation checks and fresh-process H08 onboarding. Initial
  demonstration bytes remain in `demo-observation.initial.json`, checked against
  the original commit. The active receipt was regenerated from actual responses.
- **PASS:** H08 proposal shape/reference validation with `not_requested` human
  authorization and `execution: unavailable`. The fresh client finds H08 via
  Track M, checks its actual register evidence, retrieves the unmerged correction
  as a proposal, and prepares/validates a scratch proposal. Controls include
  human-only, single-assistant and adversarial-assistant arms, matched budgets,
  blind independent adjudicators and a held-out confirmatory corpus. This is
  fresh-process client verification, **not independent-model scientific review**.
- **PASS:** Freeze registry checker (209 values), root structure, overclaim
  guardrails, active-docs link hygiene, determinism claim boundaries, 43 local
  Markdown file targets and whitespace. Registry integrity is not executable opcode conformance. No C++ or historical
  experiment rerun was needed for this index/interface-data change.
- **FAIL, pre-existing:** Docs-structure checker still expects the five absent
  top-level `book/book-{en,cn,es,pt,ru}` directories. This is unchanged from main
  and was also documented in PR610; unrelated subsystem/checker repair is excluded.

### Cross-platform CI review (original #609 head)

The following observations are for original commit `7183ae0d`, not a claim about
this new update's remote tests. [Commons run 40558170198](https://github.com/t81dev/t81-foundation/actions/runs/40558170198)
reports six successful jobs: Linux x86-64, Linux ARM64 and Windows, each on Python
3.9 and 3.13. Both macOS-14 jobs are queued; their environment and results are
unverified. No Commons failure was reported at this checkpoint.

[Foundation run 40558169976](https://github.com/t81dev/t81-foundation/actions/runs/40558169976)
has successful governance-metrics and spec/docs lint jobs, skipped optional
experimental jobs, and remaining queued jobs. [Determinism run 40558170231](https://github.com/t81dev/t81-foundation/actions/runs/40558170231)
has Linux/macOS builds in progress; [CodeQL run 40558169931](https://github.com/t81dev/t81-foundation/actions/runs/40558169931)
is in progress. [Format run 40558169951](https://github.com/t81dev/t81-foundation/actions/runs/40558169951)
passed. Pending or skipped jobs establish no new correctness or portability
result. Results for the pushed update must be assessed separately.

External clients can inspect canonical H08 without adopting superseded summary
assertions, as the fresh-process test demonstrates. Full integration of the
**corrected initiative on main** remains blocked by unmerged PR610. Neither the
proposed correction nor successful implementation tests establish H08 comparative
methodology benefit or authorize experiment execution or governance activation.
