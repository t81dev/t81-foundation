# FA01 qualification-readiness source corrections

Prepared 2026-11-16. No experiment or qualification has run. This is a targeted
source correction and ancestry audit, not a new scientific result.

Reviewed protocol: `c5e0ca79c073a72bb41f03b76ddd68acbd7eab73`.
Reviewed harness: `f18cafcb369989cd03e6d44c0bcd014b61cd4389`.
Accepted protocol SHA-256 remains
`d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3`.

## Terminal evidence correction

`fa01.run` now calls a terminal-I/O helper that catches ordinary exceptions from
serialization, size/reservation checks, writing and bounded readback. A trace
failure cannot replace an already classified `INCONCLUSIVE_LIMIT` or
`INVALID_RUN` with an I/O exception or a different stop reason. The original
classification and stop text are additionally retained as terminal-prior fields.
Favorable completion is downgraded to inconclusive if a complete, byte-verified
trace is unavailable. All previously written ledger/control/boundary files are
left untouched; there is no retry, deletion, truncation or overwrite.

The result distinguishes:

| Status | Meaning | Advertised hash |
|---|---|---|
| `verified_complete` | Write/close and bounded readback match; 24 nodes recorded | actual matching bytes |
| `verified_incomplete` | Matching written bytes, fewer than 24 nodes | actual matching bytes |
| `unverified` | A file exists after failure; it may be partial or pre-existing | null |
| `missing` | No trace file was observed after the failed attempt | null |
| `unknown` | Presence/readback could not be established | null |

Verification here means file-byte identity, **not** proof validity, durable disk
persistence or successful experiment completion. `proof_status`, phase coverage
and aggregate verdict remain separate. Missing/unverified hashes are null,
never invented. Existing evidence-schema v2 fields stay required, with explicit
nullable failure states and additional status/error/prior-classification fields.
The schema permits a favorable F aggregate only with complete verified trace
evidence and completed domain coverage.

The helper does not promise recovery from process death, a hard kill, exhausted
host memory, or failure to persist the later suite summary. The outer supervisor
must continue to treat absent terminal records as incomplete. No new storage
durability API, emergency-journal test or evidence budget is introduced. The
written helper and its exception paths have **not** executed during this task.

## Actual ancestry alignment, without a merge

`c5e0ca79` and `d6e0aa8dffab8cb9ceafde11645df8c6c512b346` contain identical
documentation trees. The former descends from `c8ae2200`; the latter was a
cherry-pick on harness `43b49541`, so `c5e0ca79` was not an ancestor of the
reviewed harness despite its plan naming that exact protocol commit.

A new branch was created on the **actual reviewed protocol commit**, not on an
imagined main merge. Only the three actual harness source commits were replayed:

| Original source commit | Aligned source commit |
|---|---|
| `71997805e156e64f3cb4d3b58bd4493ca8b71a32` | `e907c7e2392d7a77829d4e0bd25a3006aaa18adf` |
| `43b49541ddea76ba504031765229e455ef80c4ea` | `a154bb3e6c7394469379f5663035899f930a8a42` |
| `f18cafcb369989cd03e6d44c0bcd014b61cd4389` | `85e64a7a5c479fb5d005a6138ed165583973634f` |

The tree at aligned `85e64a7a` is exactly identical to the tree at reviewed
`f18cafcb` (`git diff --exit-code` returned 0). Targeted corrections follow
that point. The new branch has `c5e0ca79` as an actual ancestor. No merge commit,
force-push, rewrite of either reviewed PR, or protected-branch update occurred.
Historical protocol/corpus/proposal files are preserved byte-for-byte.

After any eventual human-reviewed protocol merge, use the **actual** fetched
main/merge commit and verify the accepted protocol hash and historical files.
Only then plan any follow-up replay onto that actual main. If the protocol is
squashed, its new Git identity must be recorded even when content is unchanged.
No future merge SHA is predicted, no post-merge operation is performed here,
and this source alignment authorizes no merge of PR #806 or #807.

## Shared source-extraction assumption and human review

Both proof implementations consume the same pinned RFC-00D7 §4.1 wording.
The candidate recognizes it through substring tests in split section boundaries;
the reference recognizes it through regexes in a matched section. These are
different recognition routines over the **same textual assumption**, not two
independent semantic authorities or independent scientific proofs. Both were
written by the same assistant with shared prior context.

Before qualification pinning, an explicit human must read the complete pinned
§4.1 passage and judge whether the same-root/own-member necessary-condition
reading is supported. The reviewer must consider wrapper projections, schema
version selection, missing/null behavior and any ambiguity in the meaning of
Stage 2. A source hash proves identity, not normative meaning. Literal/regex
matches cannot replace that review. An unresolved or disputed reading remains
an unresolved assumption; it must not become a favorable proof premise.

Future authorization and qualification records now require a closed `f_source_review` record containing
human reviewer identity, attributable record reference, pinned source hash,
section, the reading examined and acknowledgement of the recognition limit.
The summary must retain it as `source_passage_review`. This required shape is
not authentication of a human act; the operator must verify the actual review.
No such review record is manufactured in this preparation. The source-extraction
assumption is also recorded in `fa01-plan.json`.

## Preserved invariants and limits

Candidate/reference proof source, all 24 facts, F00–F07 bytes, four rotations,
the 32+78+144=254 counted-state path, 256-state F cap, F288-v1 history, A–E
source behavior and all suite budgets are unchanged. Terminal metadata/I/O is
outside the candidate/reference state machine, as before; it consumes existing
artifact/time/memory allowances and introduces no extra model states or budget.
The v1 evidence schema and v2 success requirements remain; v2 gains truthful
failure representations and the mandatory human passage-review record.

Static checks compare the entire F experiment body/handlers with the reviewed
AST, inspect the exception guards and schema fields, verify actual ancestry and
the tree-equivalent alignment point, and check source/file/dependency identities.
No helper, proof checker, fixture, control, generator or Linux resource probe is
executed. Dynamic exception behavior, real evidence persistence and kernel limit
qualification remain unverified. The unchanged macOS dependency lock still
blocks Linux execution. Separate independent review and execution authorization
remain required.
