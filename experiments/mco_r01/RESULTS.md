# MCO-R01 results

Recommendation: **REVISE**. Under the declared finite, honest-owner assumptions,
both ordinary application implementations enforce the contract and permit
independent reconstruction. No measured benefit of three-state composition,
missing computational abstraction, OS primitive, or ternary-native implementation
is established or justified by these results.

## Executed results

Starting commit: `633645afad1ac6160f2056dcd825e2590898fca2`. Only this new
experiment directory changes. The frozen packet manifest SHA-256 remains
`b191179639c6e604c3860c2743de9f284f856d185a60ab576f739706e765622b`.

Two complete runs are archived, with separate entropy traces and all commands,
exit codes, stdout/stderr, full observed journals, actual owner ledger snapshots,
receipts, mutant outputs, independent reconstruction reports, and tampered records.
The final run is authoritative for the checked-in executable source. The first
differs only in an equivalent simplification of the runner's replay-mutation
witness expression; authority/verifier sources were unchanged.

| Check | Arm A | Arm B |
| --- | ---: | ---: |
| Frozen raw-byte cases executed | 60 | 60 |
| Frozen-case disagreements | 0 | 0 |
| Abstract six-guard assignments evaluated | 729 | 729 |
| Meet pairs evaluated | 9 | 9 |
| Associativity triples, both parenthesizations evaluated | 27 | 27 |
| Reachable event schedules executed | 4 | 4 |
| Invalid full plans rejected before any prefix | 20 | 20 |
| Frozen-case protected dispatches / committed rows | 8 / 7 | 8 / 7 |
| Additional recorded proposal draws, replayed identically | 32 | 32 |
| Independent ledger/journal reconstructions | 96 | 96 |

All algebra and temporal checks agree with the independent oracle: zero failures.
The 96 reconstructions per arm comprise 60 cases, 32 fresh-owner proposal draws,
and four reachable schedules. Invalid plans are rejected tests, not successful
effect executions. The eight dispatches include the registered sink failure;
dispatch does not imply a committed row. Proposal draws and additional schedules
are excluded from the 8/7 frozen-case totals.

The Ruby reference agrees on 60/60 frozen case expectations and 24/24 plan
classifications/transitions. All 729 abstract assignments yield D=665, U=63,
P=1; these are algebra checks, not 729 reachable application states. The packet
labels 162 completed assignments, one binding prefix, and 566 abstract-only.
The finite pair/triple enumeration establishes associativity on this three-element
table; it does not prove the correctness of a general authorization system.

Both candidate process executions and recorded-trace replays exit 0; both
independent control reconstructions exit 0. Candidate behavior agrees across
both arms for every tested fixture and proposal draw. Final run
`failed_current_checks=0`. See [machine-readable results](evidence/final-results.json)
and [exact command/exit ledger](evidence/final-commands.json).

## Mutation results

All eight mutant processes exit 0 and reach the specified semantic violation.
All eight independent reconstructions exit 2 with a mismatch on that witness.
A crash or unrelated early rejection is not used as evidence of detection.

| Mutation, applied separately to each arm | Witness | Observed violation |
| --- | --- | --- |
| Permit on unknown | F06-C01 | P with G3=U instead of deferred; case has no dispatch events |
| Skip final revocation recheck | F10-C02 | One committed row despite revoked policy at A; expected zero |
| Skip replay check | F08-C05 | Two committed rows and duplicate B admitted P; expected one |
| Omit observed-model identity binding | F09-C02 | P while observed model differs from approved model; case has no dispatch events |

The two decision-only witnesses test wrong admission decisions, not invented
unauthorized row writes. The replay witness has sufficient budget so exhaustion
does not mask the missing replay check. The revocation mutant substitutes the
cached pre-revocation policy at final admission while the observer records the
actual revoked policy.

All five additional reconstruction attacks are rejected with exit 2: fabricated
permit, changed ledger artifact with a recomputed receipt hash, omitted revocation
event, erased evidence observation, and omitted entire case inventory. These
checks detect content/semantic inconsistencies under a trusted fixture/observer;
they do not authenticate evidence against a malicious host.

## External analyzability

An auditor needs frozen bytes and their manifest, authoritative context, the
recorded proposal trace, versioned verifier sources, transition records, and the
owner's actual ledger snapshot. `oracle.rb --reconstruct` recomputes permission
from those inputs without reading expected annotations, candidate authorization
helpers, or trusting a reported decision. It independently checks wire/schema
classification, binding, evidence validity and supports/refutes pairs, capability,
revocation, replay, budget/deadline, all transitions, rows, and receipts. This
also preserves the distinction between invalid, absent, refuted, and conflicting
evidence rather than collapsing them into an explanation string.

The row binds request, artifact, model, policy, output, verifier, decision/outcome,
and owner sequence. `execution_ref` hashes the canonical journal prefix through
that commit, avoiding a receipt/self-hash cycle. `receipt_ref` hashes the row.
Verifier identity hashes `wire.py`, the selected arm, and `driver.py`; archived
source fingerprints and this git commit identify the code to inspect. Fixture
and context trust must come from the frozen packet, not from an observed JSON
file's own claims. Hashes provide content binding, not attestation.

Ghidra could later examine compiled admission branches, identity comparisons,
effect call sites, and state-update paths in a compiled version. The present
Python/Ruby experiment requires source/runtime review; disassembly cannot prove
which provider, evidence registry, revocation event, or actual ledger state was
observed. Those require trusted runtime observations. No Ghidra integration is
implemented or required.

## Scientific interpretation and limits

The finite executions support the primary hypothesis under the explicitly
modeled boundary: changing an inert proposal's recommendation does not provide
capabilities, and final authority comes from the independent context and owner.
Recording proposal bytes makes this enforcement workflow replayable despite
the initial nondeterministic selection. Model explanations never grant authority.
This is evidence for the tested boundary, not a proof for arbitrary models,
malicious native code, concurrent effects, or compromised hosts.

The protected effect, replay update, and budget update occur sequentially under
one trusted owner. Both arms share that control and the wire/schema layer.
They run separately, but this is not process isolation from a malicious plugin.
The oracle uses a different language/parser and independent direct predicate,
yet shares the written semantics and finite fixtures. A common mistaken
specification remains possible. No new requirement is inferred to hide this risk.

No time/space benchmark, user study, human review-effort measurement, real-model
trial, or multi-platform run was executed. On independently measured conformance,
mutation detection, and reconstructability, the arms are equal; neither shows
a diagnostic-state advantage because both preserve the required distinctions.
Ordinary typed services, enums, libraries, and a reference monitor express and
enforce the entire experimental contract. No missing property was found.

Environment: macOS 26.5.2 ARM64; Python 3.14.3; jsonschema 4.26.0 with packet-pinned
dependencies; Ruby 2.6.10 / JSON 2.1.0. Frozen packet review also exits 0:
four meta-schemas, 22 boundary probes, 60 raw-byte classifications, 729 guard
rows, 24 plans, and 28 immutable manifest files. This structural/integrity check
is separate from the executed semantic comparisons above. No CMake/runtime
build was needed or claimed for this isolated Python/Ruby research addition.

Unresolved limits and the preserved development mismatch are recorded in
[DISCREPANCIES.md](DISCREPANCIES.md). Next work is bounded by the
[updated handoff](HANDOFF.md). No result justifies an operating-system primitive
or ternary-native implementation.

## Repository review and CI baseline

At starting head `633645af`, PR #573 is open, draft, and unmerged, with zero
submitted reviews and zero review threads. The GitHub commit-status endpoint
returns no statuses; it does not represent Actions check-run success. The
separately captured Actions rollup is **not green** (see
`evidence/baseline-pr-status.json`): 19 successes, 11 failures, three skipped,
and one in progress. Completed baseline failures include
governance-metrics, sanitizers, AVX2-ASan determinism, and six platform build jobs;
both C++ standard compatibility jobs also failed. One macOS build was still
running when captured. No human approval is claimed.

Completed job logs identify README.md's missing Determinism Surface Registry
boundary wording, and a missing `models/tiny-random-llama/model.safetensors`
source fixture. The sanitizer log reports 401/402 tests passing with the CLI
fixture-existence assertion failing. These are remote baseline observations,
not local C++ test executions or proof that every failed job has the same cause.
Three complete baseline job logs are preserved in `evidence/baseline-ci.tar.gz`.

Additional local checks: artifact hygiene and README naming exit 0. The existing
root-structure check exits 1 against the unchanged starting HEAD: it lists two
missing package files and existing top-level entries including `experiments`.
That directory was already tracked before this addition. No root structure,
README, model fixture, or runtime repair is included in this bounded research
commit. Repository-wide CI remains an unresolved integration condition.
