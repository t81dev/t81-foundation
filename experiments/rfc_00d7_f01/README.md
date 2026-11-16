# RFC-00D7-F01 synthetic harness preparation

**PREPARED SOURCE / NOT EXECUTED / NOT QUALIFIED FOR EXECUTION.**

This is an isolated non-production research instrument. It is not a model
service, VM adapter, CanonFS journal, sandbox implementation or normative RFC.
The human authorized harness preparation and static verification only. Do not
run `run.py`, `supervise.py`, candidate/oracle functions, input generators or
controls under that authorization. Do not add these to CTest, CI, imports with
side effects, or automatic test discovery. No experiment result is included.

The protocol is the clarified PR #805 document at
`c8ae2200bc37064282ffc4aaa683ed21d94d567c`; SHA-256
`0c7e7da921d3af3c6c314ec4a8bf23478f9e05695b28b808721a45475106975c`.
The contract source remains PR #804 at
`ec22a7588282d37ad027cff61c807bcf159ac2e0`.

Scope is A–F as specified in that protocol, with ceilings of 49,408 counted
states, 60 seconds wall, 50 seconds aggregate CPU, 256 MiB aggregate memory,
128 MiB for at most one runner, and 25 MiB uncompressed evidence. A–F are
sequential in one runner process, not six simultaneously allocated workers.
Subprocess creation is solely a trusted supervisor launching that one runner.
Deadline cleanup may kill that harness process; it is not fault injection into
a service or a native-worker experiment. No model weights, network access,
credentials, production modules or filesystem durability operations are used.

## Source organization and oracle independence

| Files | Role |
|---|---|
| `candidate.py` | Literal synthetic interpretations, not production behavior |
| `oracle.py` | Separately written reference equations, decoder and D3 validator |
| `domains.py` | Lazy, ascending finite cases and a literal bounded control corpus |
| `evaluate.py` | Candidate/reference comparisons and explicit ambiguity classification |
| `accounting.py` | State reservations, time guards, bounded evidence writes |
| `run.py` | Sequential control/case driver; no preparation-time invocation |
| `supervise.py` | Future Linux gate, cgroup checks and external budget supervision |
| `static_check.py` | Source/AST/schema/hash/dependency checks; imports no model modules |
| `evidence.schema.json` | Authorization, qualification, case-decoder and summary schemas |
| `pinned-d3.schema.json` | Byte-identical experimental copy of the actual pinned D3 schema |
| `interpretations.json` | Unadjudicated alternatives; no implicit policy selection |
| `dependency-lock.json`, `requirements.lock` | Preparation environment identities |
| `identities.json` | All prepared input/source byte hashes (excluding itself and the check record) |

The candidate never imports the oracle or domain generator; the oracle never
imports candidate, domains or accounting. They share immutable input values,
standard Python integer/byte primitives, and (for E) `hashlib` SHA3. Their
equations, projection, packing and reference decoding are separate. A uses
prefix counting, B a total-order oracle, C a full-history capability interpreter,
D an allocation-identity sum, E an independent literal byte vector/decoder,
and F a third-party full JSON-schema validator versus a small separate subset
evaluator. The latter handles only keywords actually present in pinned D3;
changing that schema requires new review, not silent acceptance of new keywords.

Codex authored both sides with prior review context. Source separation is
**not independent scientific validation**. Independent human review of equations,
literal controls, admissibility, domains and instrumentation is outstanding.
No assertion has been executed and no control is claimed to pass.

## Accounting and output interpretation

`Budget.visit` reserves counted states before materialization. It counts input
and pruned-order states, comparisons, observable transition allowances and
control repetitions; it does not label all Python heap objects experimental
states. Instrumented candidate event transitions, history projections, allocation sums,
logical charge events and reference-prefix advances reserve units individually.
Comparison output slots are reserved before calls. `states_reserved` is the
exact number of charged units; if a call is interrupted after reservation, it
is an upper bound on completed observable states, never a fabricated completed
state count. Internal Python/JSON-schema interpreter instructions are not
synthetic contract states. No incomplete search can pass.
In particular, intermediate oracle prefix advances in A are charged individually.
State/time/artifact guards can stop before full domain completion, even when
no counterexample exists. Missing areas and cap-stopped domains are incomplete.

Each area's ledger is ASCII with fields `case_number input_sha256 category
outcome subclaim`, maximum 128 bytes per record. The three required counters
count atomic comparison cases `(input, subclaim, rotation)`, each assigned one
category, rather than claiming independent statistical samples. Numbered control JSON holds complete
inputs. The first retained raw witness per subclaim includes input, comparison,
assumptions and an explicit `minimality: not established` label; no minimizer
has been run or provided. Collision buckets are capped at 2,048. No all-pairs
history search is allocated. Bounded raw witnesses are limited to 512 KiB per
area within the area's 4 MiB cap. Suite metadata has reserved headroom.

`summary.json` has separate suite and area counters for admissible falsifiers,
negative controls, and underdetermined cases. Negative controls never become
RFC counterexamples. Ambiguous subjects/orderings remain underdetermined.
Evidence preserves witnesses even when a subsequent cap produces an overall
`INCONCLUSIVE_LIMIT`. Never infer conformance from a missing summary, a clean
process exit, or a passing broken control. A killed runner may leave partial
ledgers; `supervisor.json` must record that incompleteness.

## Preparation-only check

Use an environment matching the checked installed-file lock:

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B experiments/rfc_00d7_f01/static_check.py
```

This entry parses ASTs, compiles them in memory without executing bytecode,
checks schema definitions without model instances, and verifies identities.
It must not import `candidate`, `oracle`, `domains`, `evaluate` or `run`.
The Commons proposal validator may separately check the documentation envelope.
Neither operation constitutes F's model-admission experiment.

## Remaining ambiguities and limitations

`interpretations.json` preserves O1/publication ordering, event-kind subject
encoding, nested LIP coordinates, outside-arena allocations, slot retention,
durability acknowledgement and textual digest encoding. The runner leaves these
under review; it does not use a label to ratify a normative repair. The trace
projection is ASCII-only and not a general JCS implementation. Schema-bound
synthetic bytes are not trained weights or a SafeTensors file.

The dependency lock covers the observed macOS ARM64 installed files, including
the `rpds-py` extension. It is not a portable Linux wheel lock. Linux re-locking,
reviewed source/wheel identities and interpreter qualification require a new
reviewed harness revision before execution. The supervisor explicitly blocks
the preparation lock. No dependency was installed for this task.

Static inspection does not establish runtime correctness, exhaustive domain
coverage, kernel limit behavior, sanitizer cleanliness or portability.
Canonical tuple order uses the literal domain ranks in `domains.py` (including
chronological action ranks), not lexical sorting of event-name strings.
The enumeration can reach caps before later subdomains. Control corpus rotations
do not repeat the entire domain. The initial prepared driver retains first
witnesses without minimization. It includes explicit accounting
and interpretation gaps; it is review material, not an execution-readiness claim.
Any defect found in future independent review must be corrected and re-pinned
before authorization, preserving this preparation record.
