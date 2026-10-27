# T81 Research Commons v1

A curated, read-only discovery index for T81-ORI. Start with the
[architecture decision](../../architecture/adr/ADR-0006-research-commons-discovery.md),
[manifest](manifest.v1.json), and [agent rules](../../../AGENTS.md).
This initial foundation exposes repository evidence and uncertainty. It does not
certify findings or run experiments.

## Current integration dependency

The active index pins merged PR608 at
`fabc7aa90f70ead227ec257a1ae6833fa16ced40`. **PR610 is still an unmerged
draft**, checked 2026-10-27. Its correction at
`d1eee548d4658df37b35d0cc3781b5a77cbae96b` is indexed only as proposed evidence,
not main or an accepted finding. PR609 remains draft pending human review/merge
of PR610 and a subsequent source-pin/receipt refresh. The canonical register
already supplies the exact dated H07 B/C and H08 C/E records; PR608's stronger
summary dispositions cannot override them.

`discrepancy_history` retains PR607 identity/wording errors as superseded by
PR608, with resolution limits. PR608's remaining H07/H08 error is unresolved
until the proposed correction actually merges. `get_hypothesis` exposes this
history and the integration dependency alongside the canonical record. No new
operation or execution capability was added.

## Setup and discovery

Requires Python 3.9+, Git 2.46+ (a conservative minimum for documented `GIT_NO_LAZY_FETCH` support), and the dependency in
[requirements.txt](../../../tools/research_commons/requirements.txt). Install into
an environment outside the repository. Commands below use `python` for that
environment's interpreter; installation/fetch are explicit setup actions, outside
the discovery interface:

```sh
python -m pip install -r tools/research_commons/requirements.txt
# Needed in clones that do not contain PR605's separate merged proposal history:
git fetch origin 827d2accbf8ccb56afe3fe8d96dfb7251f249d7d
# Explicit setup for the UNMERGED correction snapshot, not an update of main:
git fetch origin d1eee548d4658df37b35d0cc3781b5a77cbae96b
python -B tools/research_commons/commons.py --validate
python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"list_research_tracks","arguments":{}}'
```

Source paths are **repository-relative at each source's revision**, not promises
that every file exists in the working tree. PR605's evidence is not on the pinned
main tree. Each source has an immutable GitHub URL, commit and SHA-256. Git objects
must be available locally; missing objects fail validation. No implicit network
fallback occurs; Git lazy fetching, interactive prompts and optional locks are disabled. Working-tree edits to an evidence document do not alter pinned
responses. New accepted evidence requires an explicitly reviewed index update;
changing the checkout branch alone does not refresh this snapshot.

## Interaction contract

[Request schema](request.schema.json), [response envelope](response.schema.json), protocol `t81.research-commons.v1`:

| Operation | Required `arguments` | Result |
| --- | --- | --- |
| `list_research_tracks` | `{}` | R, F, M, canonical IDs and evidence references |
| `list_hypotheses` | `{}` | All eight complete historical records, evidence and questions |
| `get_hypothesis` | `{"hypothesis_id":"H04"}` | Verbatim canonical record and supplementary references |
| `get_evidence` | `{"evidence_id":"hanoi-negative"}` | Pinned UTF-8 document, role, uncertainty and identity |
| `get_reproduction_instructions` | `{"hypothesis_id":"H02"}` | Curated instructions and requirements; never execution |
| `list_open_research_questions` | `{}` | Research questions, discrepancies and limitations |

Use `--request -` for one JSON request on stdin. Success writes one canonical,
sorted-key ASCII JSON object to stdout and exits 0. Failure writes a structured
`validation_failed` error to stderr, no result to stdout, and exits 2. Unknown
operations, arguments, evidence IDs, protocol versions, duplicate keys and
non-finite numbers fail. No shell or command strings are executed from requests.
The CLI is a justified MCP equivalent for this phase; **it is not an MCP server**.

Responses carry protocol, operation, baseline revision, manifest SHA-256, scope
and result. `manifest_sha256` hashes canonical JSON (sorted keys, compact
separators, ASCII), not the formatted manifest file. Every request checks all
indexed source commits, regular-file paths, hashes and references before returning
a result. Evidence responses return full curated text records; archive bytes and
arbitrary paths are not retrieval operations. Nested paths/URLs quoted inside a
historical source are preserved claims, not automatically indexed or verified
references. Follow `source_refs`/`evidence_refs` through `get_evidence`.

The original register is dated 2026-10-25. Its `result`, `confidence`, assumptions,
measurement and falsifier fields are returned unchanged. Later R02 and E1 records
are supplementary evidence; there is no computed global verdict. Evidence roles
(`reported`, `negative`, `modeled`, `disputed`, etc.) classify the indexed record,
not independent replication. The manifest indexes conflicts in PR606/607 and the
continuity hash packet so future agents cannot mistake repeated summaries for
verification. Hash identity proves bytes, not truth, authorization or authorship.

## Contribution contracts

Self-contained Draft 2020-12 schemas cover:

- [Experiment proposals](experiment-proposal.schema.json): hypothesis, attribution,
  exact revision, falsifier, matched baseline, method, measurement, artifacts,
  resources, security and human authorization.
- [Independent review drafts](independent-review.schema.json): author relationship,
  prior context, independent-execution declaration, adversarial checks and limits.
- [Reproducibility report drafts](reproducibility-report.schema.json): environment,
  exact commands, artifact hashes, failures, scope and blocked/inconclusive outcomes.
- [Discrepancy drafts](evidence-discrepancy.schema.json): conflicting claims,
  affected evidence, observed difference and proposed resolution.

All have `status: proposed`, explicit uncertainty, canonical hypothesis IDs,
pinned evidence references and authorization status. H-S1 is not H08; proposals
about integration should state the relationship to H07 without renaming it.
Noncanonical refinements belong in the research question/target fields. Schema
validation cannot prove reviewer independence, authentic authorization, adequate
statistics or a competent baseline. Contribution acceptance remains a human act.

```sh
python -B tools/research_commons/commons.py --validate-contribution docs/research/commons/examples/h04-proposal.json
python -B -m unittest discover -s tests/research_commons -v
```

The [H08 proposal](examples/h08-proposal.json) compares human-only, single-assistant
and adversarial-assistant work under matched budgets and blind independent
adjudication; authorization is `not_requested`.

The [onboarding demonstration](ONBOARDING_EXAMPLE.md) uses only these contracts
and repository evidence. See [implementation and limits](IMPLEMENTATION_REPORT.md)
for exactly what was tested.
