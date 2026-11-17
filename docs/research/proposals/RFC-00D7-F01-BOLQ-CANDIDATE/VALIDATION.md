# Candidate preparation validation — 2026-11-17

**CANDIDATE AMENDMENT — NOT ADOPTED.** The human authorized drafting only after
Gate0. No adoption, implementation, normative/operative edit, source/schema/gate/
lock change, host action, qualifier/probe, proof/fixture/A–F execution or merge.
All reference artifacts and linux_relock_required=true remain unchanged.

## Gate0: mandatory checksum reconciliation BEFORE authoring

Working directory: /private/tmp/t81-f01-decision.
Exact pinned source:473770c7a4aaf83327eaaededd1de14bad177e4c.
Initial git rev-parse HEAD matched that commit and worktree status was clean.
The following fresh check reads SHA256SUMS and each content blob directly from
Git, not from a regenerated manifest or a prior harness-checker result. This
assistant also authored the source packet; the new byte computation is not
independent scientific/human acceptance.

Exact command:

```sh
python3 - <<'PY'
import hashlib
import subprocess
import sys
revision = '473770c7a4aaf83327eaaededd1de14bad177e4c'
directory = 'docs/research/proposals/RFC-00D7-F01-CONTRACT-DECISION/'
manifest = subprocess.check_output(['git', 'show', revision + ':' + directory + 'SHA256SUMS'])
entries = [line.split('  ', 1) for line in manifest.decode('ascii').splitlines()]
required = {'DECISION_MEMORANDUM.md', 'PREPARATION.md', 'proposal.json', 'source-pins.json'}
if len(entries) != 4 or {name for _, name in entries} != required:
    print('CORRECTIONS REQUIRED: unexpected checksum manifest scope')
    sys.exit(1)
print('revision=' + revision)
print('manifest_sha256=' + hashlib.sha256(manifest).hexdigest())
failed = False
for expected, name in entries:
    raw = subprocess.check_output(['git', 'show', revision + ':' + directory + name])
    actual = hashlib.sha256(raw).hexdigest()
    ok = actual == expected
    print(name + ': ' + ('OK' if ok else 'MISMATCH') + ' expected=' + expected + ' actual=' + actual)
    failed |= not ok
print('GATE_0=' + ('FAILED' if failed else 'PASS'))
sys.exit(1 if failed else 0)
PY
```

Captured output (no additional error output reported):

```text
revision=473770c7a4aaf83327eaaededd1de14bad177e4c
manifest_sha256=c17aa249e491e50fc90a51eb6ded995118abfe8b1d8c2b04e5cdb72883d4e5a6
DECISION_MEMORANDUM.md: OK expected=60b3d3cd763b0aa1ccdb60cfb5c7ca4426cbd928171f04fc655490172abd408c actual=60b3d3cd763b0aa1ccdb60cfb5c7ca4426cbd928171f04fc655490172abd408c
PREPARATION.md: OK expected=0086c736f2f3f0e0608946084f194d60b6753c456769cdb6b0ff80a1e165dd12 actual=0086c736f2f3f0e0608946084f194d60b6753c456769cdb6b0ff80a1e165dd12
proposal.json: OK expected=1816efebdd907ca0e366a0adffa5a25790fa8fcc4e69ab6c900e7594e8f5c213 actual=1816efebdd907ca0e366a0adffa5a25790fa8fcc4e69ab6c900e7594e8f5c213
source-pins.json: OK expected=58e192e9c798caa444f2c166cc2ad2e2819118c3fd7447b753cc05a6e0e51cd3 actual=58e192e9c798caa444f2c166cc2ad2e2819118c3fd7447b753cc05a6e0e51cd3
GATE_0=PASS
```

Exit status:0. All four entries matched. No old hash/manifest was regenerated.
Only after that result was the separate candidate worktree/branch created from
the pinned commit and the first candidate file written. A mismatch would have
ended the task with CORRECTIONS REQUIRED; that branch was not taken.

## Gate1: documentation preparation and permitted validation

Working directory: /private/tmp/t81-f01-option-b.
Created docs/f01-option-b-candidate from exact473770c7. New files only in
 docs/research/proposals/RFC-00D7-F01-BOLQ-CANDIDATE/.
Read pinned original protocol, #811 timing/resources, #812 finding and #813
O/L obligations, plus the current strict qualification receipt definition. No
source function, interpreter boundary, candidate/oracle or proof checker was
imported or called. Local existing Python is used only as a documentation/data
tool; no target/provider interpreter or artifact code is executed.

The four candidate content documents define A1–A6, P1–P4, a separate proposed
receipt type/dictionary and O01–O10/L01–L08 traceability. They preserve numeric
and deterministic contracts, qualify physical host claims only, retain sticky
known failures and incomplete evidence, and explicitly distinguish controller-
death test assertions from operation completion. There is no schema or receipt
instance and no implicit legacy limits_demonstrated:true conversion.

Validation procedure: duplicate-key JSON parsing; verify every pinned Git blob/
SHA-256 and current file; compare all five prior source/profile/plan/assessment/
memorandum trees and operative spec/protocol bytes unchanged; verify candidate
content hashes, proposal authority fields, clause/field/traceability coverage and
unchanged resource arithmetic/blocker. Run Commons shape/reference validation,
packet hashes and staged whitespace. These are source/document checks, not
observed model qualification. Results follow after checks.

Observed Gate1 local source/data/document check, cwd above, exit0:

```json
{"candidate_document_hashes_verified":5,"gate0_original_entries_unchanged":4,"linux_relock_required":true,"memo_clauses_traced":8,"model_adopted_or_qualified":false,"operative_source_profiles_schemas_locks_unchanged":true,"original_UNSUPPORTED_preserved":true,"original_obligations_traced":10,"proposed_A_clauses":6,"proposed_P_applicability_sections":4,"protocol_and_limits_unchanged":true,"source_pins_verified":40}
```

Invocation: /private/tmp/t81-pr795-parity-venv/bin/python -B - with an inline
stdlib-only JSON/Git/hash/document check. It rejected duplicate JSON keys;
compared40 Git-object bytes/blob IDs to recorded/current hashes; checked five
candidate document hashes/lengths; compared prior operative trees/spec unchanged;
rechecked old manifest/current four source files without writing them; verified
unadopted/unqualified/unimplemented flags, original UNSUPPORTED, active blocker,
A/P/O/L and dictionary field coverage,54/48 phase arithmetic,60/50 and25MiB.
The source/document check is not a receipt validator or behavioral test. No
A–F, interpreter-boundary, guard, model/oracle or proof function was invoked.

Commons command, same cwd, exit0:

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B tools/research_commons/commons.py --validate-contribution docs/research/proposals/RFC-00D7-F01-BOLQ-CANDIDATE/proposal.json
```

```json
{"execution":"unavailable","limitation":"Shape/reference checks establish neither authorization authenticity nor scientific acceptance.","status":"valid_proposal_shape"}
```

Only new candidate packet hashes were generated during drafting. The old #813
SHA256SUMS and content files were never regenerated or changed. Candidate hashes
identify partial proposed text, not an adopted protocol/model or a replacement
for the old protocol digest. Final SHA256SUMS covers every packet file except
itself, including this Gate0 record; candidate-hashes.json has no recursive
self-hash and pins the four core documents plus source-pins.json.

Final verification command: shasum -a 256 -c SHA256SUMS, working directory
/private/tmp/t81-f01-option-b/docs/research/proposals/RFC-00D7-F01-BOLQ-CANDIDATE.
All eight entries verified, exit0. Staged git diff --cached --check passed.
The packet remains below128KiB. The standard commit hook has no staged bundle
and runs no bundle test. Final worktree/PR metadata checks record only publication
identity, not model/host qualification. No normative edit, source/schema/lock
change, new executable or runtime test is supplied.
