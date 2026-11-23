# AXION-AI-001 HD01 — Documentation Preparation Record

Status: NON-NORMATIVE / NO AUTHORITY MODEL SELECTED OR ADOPTED
Date: 2026-11-17
Baseline: `cb9216f42f63e95e58709a71e08d38139c71e48b`

## Attribution, prior reading and authorization

Codex authored this memorandum with prior ADR-authoring context. This is
preparation for independent review, not independent scientific validation.
The human assignment authorizes HD01 documentation and a draft PR only.
ADR-0007 and its complete validation record were read before authoring; their
source identities are retained below. Their original source baseline remains
56db3a18909a029042f7ceded2e19d96aec57cdc, while this memorandum pins the merged
main snapshot above. Historical validation results were read, not rerun.

All authority models and record/verification/conformance proposals remain
non-normative. No model, roots, keys, threshold, algorithm suite or trust profile
is selected/adopted. HD01 remains OPEN; HD02-HD06 remain distinct human gates.
Human-readable candidate field lists are not active schemas or interfaces.

## Exact documentation-only scope

Two new files, under docs/research/proposals/AXION-AI-001-HD01/:

- MEMORANDUM.md
- PREPARATION.md

No prior tracked file is modified, removed or renamed. In particular ADR-0007,
its original validation record, RFC-00D3, provenance gate/keyring, implementation,
tests, active schemas/locks, governance gates and qualification artifacts retain
the baseline bytes. An isolated worktree preserves the user's unrelated untracked
artifacts. No research/architecture index or accepted finding is rewritten.

The memorandum covers separate approval/admission/effect authority, exact asset
and runtime identities, custodian/worker boundaries, revocation and rollback,
three competing models, fifteen future adversarial cases and HD02-HD06 links.
Manual editorial inspection checked that all three candidates remain unselected,
that future cases have observable falsifiers, and that uncertainty, context
shrinkage and cumulative accounting agree with the ADR. This is author inspection,
not a proof, executed oracle or independent-review result.

## Source identities and observations

The nine repository references below were read as pinned Git blobs and hashed
with SHA-256. Digests establish identity, not approval, truth or authentication.
The prior ADR digest is the identity retained by its original validation record;
this preparation does not rerun that record's checker or its historical checks.

| Reference | Baseline repository path | SHA-256 |
|---|---|---|
| ADR | `docs/architecture/adr/ADR-0007-axion-ai-001-governance-broker.md` | `5c176eb6766f81cdb6ae1a0716d3f6ab60bab2c9a0377478ab6bfad53b591e2d` |
| ADR-VALIDATION | `docs/records/audits/2026-11-17-axion-ai-001-adr-001/VALIDATION.md` | `26fe74592076053bcd5998df805b315a6a93e0e04e50d3076d9c1978a9fe111c` |
| D3 | `spec/rfcs/RFC-00D3-model-manifest-schema.json` | `a7c2cf69c9fc3b2030075cca2ea902fc5c28c53ebd28dc80b65ac4f5c3b8cbe8` |
| PROVENANCE | `scripts/ci/check_ai_model_provenance_gate.py` | `8aaa2ddc1c130baae2ac58159751677a3b72b1ba27b58ba4ed4e34d82d4ecba9` |
| KEYRING | `scripts/ci/ai_model_provenance_keyring.json` | `cb698a9b70ffdc2d01835fca94add39fa3deb335fd020c793a0f5546a10394da` |
| KERNEL | `ternaryos/kernel/kernel_abi.hpp` | `23bd9c8d1a6529fb4d46d8f910325f40ebd7b94ef40a0b0cced01eb3722f2903` |
| AXION | `kernel/axion/axion_api.cpp` | `a9735db4ef38af8811bbf91389f07eda887b1ef6ca1378675953abb7c87aefca` |
| LLAMA | `tools/model/llama_cpp_adapter.cpp` | `e4b97ae1de4cea24338f0f9d23e218225a9e3cbb8493f6d237a319a1b5491043` |
| CANONFS | `fs/block_backed_driver.cpp` | `b5bceb6eac7cdf2c0cf6abd34b23485794da9da9698c5d006c2f853fe14751b4` |

## External documentary precedents

Read on 2026-11-17 using web page retrieval only:

- https://theupdateframework.github.io/specification/latest/ — observed TUF
  specification 1.0.37, last modified 2026-10-08. Limited documentary precedent
  for roles/delegation, thresholds and version/freshness defenses in software
  updates, not adopted T81 authority or a tested runtime guarantee.
- https://github.com/secure-systems-lab/dsse/blob/master/protocol.md redirected to
  https://github.com/dsse-lf/dsse/blob/master/protocol.md — observed DSSE protocol
  1.0.2, dated 2024-05-10. Limited precedent for authenticated payload type/bytes,
  not signer authorization or a selected signature profile.
- https://in-toto.io/docs/specs/ was consulted for the specification index; no
  in-toto conformance or additional authority guarantee is claimed.

These are mutable latest/master/index pages, not immutable external source pins.
The observed versions are retained; any normative adoption or implementation
proposal needs exact external revisions and reviewed profiles. No packages or
external code were downloaded, installed or executed.

## Preparation commands and execution exclusions

Working directory: `/private/tmp/t81-axion-ai-001-hd01`.

Executed source/document operations:

- `git fetch origin main`: exit 0; observed main
  cb9216f42f63e95e58709a71e08d38139c71e48b, parent
  56db3a18909a029042f7ceded2e19d96aec57cdc.
- `git show origin/main:docs/architecture/adr/ADR-0007-axion-ai-001-governance-broker.md`
  and the corresponding `docs/records/audits/2026-11-17-axion-ai-001-adr-001/VALIDATION.md`:
  exit 0. Longer output was read in additional bounded portions.
- `git show` / `git grep` / text reads inspected the source references. A bounded
  Python standard-library text/hash operation read their Git blobs, produced the
  nine hashes above, listed B01-B15, and resolved reference definitions and the
  local PREPARATION.md link. It imported no repository code, invoked no gate,
  evaluated no proposed conformance case and performed no signature verification.
  Exit 0; all nine sources and the local link resolved.

Reproducible source-identity command pattern (substitute a listed path):

```sh
git show cb9216f42f63e95e58709a71e08d38139c71e48b:docs/architecture/adr/ADR-0007-axion-ai-001-governance-broker.md | shasum -a 256
```

Only documentation scope/whitespace inspection and Git publication follow these
reads. No test suite, existing ADR checker, schema validator, provenance gate,
crypto fixture, state enumeration, conformance scenario, benchmark, interpreter
build, model inference, host/qualification probe or F01 experiment was run. No
implementation code or schema was authored. The pre-commit hook was inspected:
it invokes bundle tests only for staged .t81-bundle files; this candidate stages
none, so its guarded test command has no eligible inputs.

Staged inspection: `git diff --cached --name-status` lists only the two new
Markdown files above (status A); `git diff --cached --check` exits 0 with no
output. No baseline tracked file is modified. These are Git documentation
inspections, not execution of a test or proposed verification procedure.

The future B01-B15 evidence packets do not exist as executed HD01 results.
Neither document claims dynamic verification, qualified hosts, measured costs,
successful signature cases or independently authenticated human approval keys.

F01 remains BOLQ NOT ADOPTED; residual risks NOT ACCEPTED; M1–M6 STOP;
linux_relock_required=true. Its operative protocol SHA-256 and qualification
semantics remain unchanged. No merge or operational permission is granted.
