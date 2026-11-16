# RFC-00D7-F01 preparation-only verification

Status: source prepared; A–F NOT EXECUTED; Linux NOT QUALIFIED.

The human authorized non-production harness preparation and static verification.
No candidate/oracle function, control, case generator or execution entry point
was invoked. No A–F inputs were enumerated and no result packet was created.
JSON schemas, source recipes and installed dependency metadata are preparation
artifacts, not experimental instances or findings.

## Exact identities

- Reviewed protocol before clarification: `f59f97c0eedb9dc92e56a34b76e608667b7715f5`.
- Clarified protocol commit: `c8ae2200bc37064282ffc4aaa683ed21d94d567c`.
- Clarified protocol SHA-256: `0c7e7da921d3af3c6c314ec4a8bf23478f9e05695b28b808721a45475106975c`.
- Contract source: `ec22a7588282d37ad027cff61c807bcf159ac2e0`.
- Harness sources: pinned by `identities.json` and the containing Git commit.
  This check record and the identity manifest itself are covered by Git rather
  than recursively including themselves in that manifest.

## Commands and observed outcomes

Run from the experimental worktree root:

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B experiments/rfc_00d7_f01/static_check.py
/private/tmp/t81-pr795-parity-venv/bin/python -B tools/research_commons/commons.py --validate-contribution docs/research/proposals/RFC-00D7-F01/proposal.json
git diff --cached --check
```

Static-check output (exit 0):

```json
{"artifact_hashes":4,"candidate_oracle_invocations":0,"compiled_modules":8,"cross_module_calls_checked":48,"execution_qualified":false,"experimental_states_enumerated":0,"locked_distributions":5,"model_imports":0,"preparation_only":true,"schemas_checked":2,"source_pins":9}
```

The zero-execution fields describe this checker's code path; they are not
experimental coverage counters. Compilation used Python `ast.parse` and
`compile` in memory without `exec`, model imports or writing bytecode.
Schema checks used `Draft202012Validator.check_schema` on definitions, never
F's synthetic manifests. The checker verified nine Git-object/source pins,
four proposal artifact hashes, prepared-file hashes, five installed distribution
file inventories, schema-copy identity, and import separation by AST inspection. Forty-eight cross-module call
signatures were checked against function definitions without model imports.
Commons validation returned `valid_proposal_shape`, `execution: unavailable`.
Whitespace checks cover tabs/trailing spaces in Python sources and staged diff
formatting. No Black/Ruff executable was available; no formatter was installed.
No runtime regression, sanitizer, platform, resource-probe or A–F test was run.

The environment is macOS 26.5.2 ARM64, CPython 3.14.3, using the existing named
virtual environment. A first attempt to collect dependency metadata with default
`python3` failed because `jsonschema` was not installed there; the existing
virtual environment was then used. No dependency installation occurred.

## Limits of these checks

All model/control expectations remain predictions pending execution and
independent review. Same-author source separation is not independent scientific
validation. State reservations count explicit comparison slots and instrumented transitions;
they are exact budget charges, not claimed completed-state measurements after
an interrupted call.
Hard live-memory enforcement and aggregate CPU/deadline cleanup have not been
qualified. The macOS dependency lock intentionally blocks Linux execution until
a reviewed Linux re-lock. Linux x86_64/ARM64, macOS execution and Windows execution
are all unverified; only proposed Linux execution is supported by the gate.

Do not infer readiness from syntax/schema checks or publication of this branch.
See `LINUX_QUALIFICATION.md` for the separate proposed qualification and one-run
authorization procedure. All research/release holds remain unchanged.

## Independent-review correction verification

Reviewed baseline: `71997805e156e64f3cb4d3b58bd4493ca8b71a32`. Protocol
head/hash and all source pins above remain unchanged. This is a new static
preparation check, not a rerun of any A–F experiment.

The same static-check command exited 0 with:

```json
{"artifact_hashes":4,"candidate_oracle_invocations":0,"closed_form_cap_bounds_checked":6,"compiled_modules":9,"cross_module_calls_checked":48,"declared_interpretations_checked":8,"execution_qualified":false,"experimental_states_enumerated":0,"locked_distributions":5,"model_imports":0,"preparation_only":true,"schemas_checked":2,"source_pins":9}
```

Additional AST/schema checks verify the exact interpretation registry and
required/unknown-key constraints, guarded missing/unknown-value rejection,
validation before model comparisons, D's literal `outside-arena` lookup, and
the candidate-publication read set against the memo key. The F generator's
AST is identical to its reviewed baseline. The two B event chains are checked
as literals and no permutation generator remains. No generator was imported
or invoked to establish these facts. No policy function was called: metadata
guards were inspected as ASTs and schema definitions.

Six cap bounds were checked by fixed integer arithmetic, not by generating
inputs or evaluating candidate/reference logic. `CAP_ACCOUNTING.md` distinguishes
exact full-path counts, conservative bounds and impossible exhaustive coverage.
F remains 288 inputs and is explicitly BLOCKED at its unchanged 256-state cap.
The protocol amendment document is unapplied. No cap or scientific-domain
reduction was implemented. B's full upper bound is 3,783 states, including
controls, prefix construction, memo entries, repeated case observations, actual
transitions and every constructed result row.

Commons proposal validation and staged whitespace checking passed again.
Candidate/oracle source files, pinned D3 schema, dependency lock and all
protocol-packet files are unchanged from the reviewed baseline. Metadata
reservation guards now bound mandatory control records to 4 KiB each and
runner metadata to 64 KiB inside existing artifact ceilings.

No A–F cases, controls, candidate/oracle functions, Linux qualification probes
or resource-enforcement checks were executed. State fit does not establish
wall-time fit. Source re-review is appropriate; execution remains unauthorized
and unqualified, with F requiring a separate protocol decision.

## FA01 preparation-only adoption

The user reports Gemini accepted the amendment design and authorizes source
preparation/publication, with no execution. Protocol branch commit
c5e0ca79c073a72bb41f03b76ddd68acbd7eab73 adopts the exact reviewed SHA-256
d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3.
The matching source is prepared separately, based on harness43b49541.
The corpus hash remains e356618677ad5282703b9c0a38e903140d287f24a8f62031572bad184abf0158.

Static checker output after source preparation (exit0):

```json
{"artifact_hashes":5,"candidate_oracle_invocations":0,"closed_form_cap_bounds_checked":6,"compiled_modules":10,"cross_module_calls_checked":54,"declared_interpretations_checked":8,"execution_qualified":false,"experimental_states_enumerated":0,"fa01_planned_states":254,"locked_distributions":5,"model_imports":0,"preparation_only":true,"schemas_checked":3,"source_pins":14}
```

These are in-memory compilation, AST/signature, metadata-schema and identity
checks only. No candidate, reference, proof, control, fixture-schema validation,
legacy generator or Linux resource probe was invoked. Source independence was
checked structurally, not independently scientifically validated. Both sides
remain same-author preparations. Runtime accounting checks are written but
not exercised. Complete expected F state charges are32/78/144; actual values
are not known before execution. All A–E function behavior and cap limitations
remain outside this amendment. Existing package identities are unchanged.

The archived proposal's unified diff intentionally contains single-space blank
context markers; full git whitespace checking flags those historical bytes.
The archive is preserved byte-identically; staged whitespace checks exclude
only that diff, and all remaining source/document files pass. No production,
RFC-00D3/#804, frozen ISA, governance, merge or execution change is authorized.

## Targeted readiness corrections — 2026-11-16

Reviewed source f18cafcb369989cd03e6d44c0bcd014b61cd4389 and protocol
c5e0ca79c073a72bb41f03b76ddd68acbd7eab73. The new source-only branch starts
at the actual protocol commit and replays the three existing harness commits.
Aligned point85e64a7a5c479fb5d005a6138ed165583973634f has exactly the same
Git tree as reviewed f18cafcb, before these targeted corrections. No merge,
hypothetical main commit, force-push or rewrite of the reviewed PRs occurred.

Static checker command (existing environment; exit0):

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B experiments/rfc_00d7_f01/static_check.py
```

```json
{"actual_protocol_ancestor_checked":true,"artifact_hashes":5,"candidate_oracle_invocations":0,"closed_form_cap_bounds_checked":6,"compiled_modules":10,"cross_module_calls_checked":54,"declared_interpretations_checked":8,"execution_qualified":false,"experimental_states_enumerated":0,"fa01_planned_states":254,"locked_distributions":5,"model_imports":0,"preparation_only":true,"schemas_checked":3,"source_pins":14,"terminal_guard_ast_checked":true}
```

The F experiment try-body and classification handlers match the reviewed AST
exactly; all existing model visit sites remain unchanged. AST inspection checks
that terminal writing/readback has broad exception guards, no terminal read is
unconditionally performed in the return, and prior limit/invalid classification
is retained. This is not a runtime failure-injection test. No terminal helper,
model, proof checker, fixture validator, control or generator was called.

Evidence v2 retains required fields, adding explicit missing/unverified/incomplete
trace states with nullable unavailable paths/hashes. Future authorization and
qualification require an attributable human source-passage review record. Source
recognition remains substring/regex over shared pinned text; both implementations
are same-author source and do not establish independent scientific proof.

The protocol/corpus,24 facts,32+78+144 charges,256-state F ceiling, historical
F288 obligation, A–E behavior, dependencies and all suite limits are unchanged.
All changed source identities are refreshed in identities.json. No records of
successful trace persistence or source-passage approval are fabricated. Real
terminal failure handling, summary persistence under exhausted capacity and
Linux enforcement remain unverified; process death cannot be repaired by this
helper. The macOS lock still blocks Linux. Independent review and separately
authorized qualification/execution are required before any final qualification
pinning or run. See READINESS_CORRECTIONS.md for exact ancestry and semantics.

## LQ01-R1 supervisor correction — preparation only

Baseline2f2bc47ffc9441a060f3a1cfdf4ec893c6699d0e; source design accepted by the
human for this limited implementation. No qualification or experiment entry
point was imported/invoked. No model, oracle, control, fixture, proof-checker
or generator ran. No dependency installation or Linux host probe occurred.
The same existing macOS ARM64 CPython3.14.3 preparation environment was used.

Commands:

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B experiments/rfc_00d7_f01/static_check.py
git diff --check
```

Observed checker output (exit0; final source identities checked):

```json
{"actual_protocol_ancestor_checked":true,"artifact_hashes":5,"candidate_oracle_invocations":0,"closed_form_cap_bounds_checked":6,"compiled_modules":11,"cross_module_calls_checked":54,"declared_interpretations_checked":8,"execution_qualified":false,"experimental_states_enumerated":0,"fa01_planned_states":254,"locked_distributions":5,"lq01_r1_ast_checked":true,"model_imports":0,"preparation_only":true,"protected_baseline_artifacts":17,"schemas_checked":3,"source_pins":14,"terminal_guard_ast_checked":true}
```

Compilation is AST/compile in memory, without executing resulting code objects.
Source checks inspect G0/G1 populations, budget constants, inherited CPU ordering,
GO-before-exec ordering, stdlib-only stub imports, parent death, R-only kill,
subreaping and retained direct-child wait status. They compare17 protected
non-document baseline artifacts byte-for-byte; all11 Python sources are parsed.
Both source-identity manifests, protocol history and active artifact hashes are
verified. Existing schema definitions are checked; no synthetic fixture is
validated. These source-shape checks do not prove runtime enforcement.

An initial static attempt failed because the older checker treats all literal
lookups on a variable named `values` as interpretation lookups. The manager
property dictionary was renamed `observed`, preserving that guard. Subsequent
checks passed. Source inspection also found the unchanged evidence-v2 schema
is39,106 bytes: the new gate explicitly allows65,536 bytes for that schema,
rather than the old32,768 default. Neither the schema bytes nor experimental
semantics changed. No dynamic check was used to find or repair these issues.

Whitespace checks passed. No formatter or dependency was installed. Final commit
identity and changed-file inventory are in the separate draft PR; identities.json
covers the new source, preparation record and implementation description.

Linux x86_64/ARM64 remain unqualified. macOS/Windows execution is unsupported.
The Linux re-lock blocker remains true. Real controller delegation, outer
manager deadlines, resource delivery/overshoot, GO/EOF handling, session-changing
orphans and evidence exhaustion remain unexecuted qualification obligations.
Missing records are incomplete evidence, never fabricated persistence.
