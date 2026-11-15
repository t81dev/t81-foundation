# Independent schema/corpus design review — continuation of cd7d4620

Date: 2026-10-25 America/New_York. PR #573 stays draft on its existing branch.
All seven original documents were read before changes. The original schemas are
preserved under `schemas/*.cd7d4620.json`. This is specification/data review,
not candidate architecture implementation, an independent laboratory, or a claim
that an authorization engine passed 60 attacks.

## Findings and resolutions

| ID | Review finding | Resolution / status |
| --- | --- | --- |
| S-01 | Both original schemas are valid Draft 2020-12 schemas, but `$` in their regexes accepts a terminal LF with the selected validator. | Reproduced on request_id and ref in both schemas; add exact minLength/maxLength (32 and 71), preserve old schemas and probe evidence. |
| S-02 | Schema validity is not wire or semantic validity. Duplicate keys may be lost before validation; lone escaped surrogates, BOM, byte/depth limits and trailing values are separate concerns. | Fix a deterministic staged wire profile in ORACLE_SPEC.md; raw corpus includes malformed bytes and ambiguity. |
| S-03 | “Integer” was not distinguished from host int/float types. | Accept mathematical 1.0/8.0; reject booleans and fractional decimals. Exact Decimal parsing plus the documented validator type adaptation avoids 1e-4000 underflow. |
| A-01 | G1/G6 U values and early stops were conflated with completed guard evaluation. | Enumerate all 729 mathematical vectors; label 162 completed-domain, 1 binding-prefix and 566 abstract-only, without claiming wire coverage of all 162. |
| A-02 | Deadline equality and “inflated” in-range budgets were ambiguous. | Equality is live; capped in-range requests may commit one unit. Out-of-range request is structural failure, not a model-authorized budget increase. |
| A-03 | Oversized/deep output was both G1 invalid output and G6 resource exhaustion. | Wire bounds stop at INVALID_RESPONSE with profile detail; G6 concerns logical deadline/budget after valid binding. |
| A-04 | Changed evidence bits could silently reuse an old digest; “source” text could be confused with authority. | Store each immutable observation's exact bytes/ref; trust requirements and availability come only from context. Include corrupt-ref and self-consistent false-source cases. |
| A-05 | FinalRecheck, broker probe and actual effect attempt were not distinguished; 24 arbitrary event orders were not all reachable. | Require R<A<B; four reachable orders, twenty invalid plans that execute no prefix. Denied probes are not protected effect dispatches. |
| A-06 | A cached P could mask revocation; output-based replay keys or a zero-unit post-commit budget could hide skipped replay checks. | Final live checks; request-ref replay key; duplicate witness starts with 2 units. Revocation never erases an earlier legitimate commit. |
| A-07 | An echoed model ref alone cannot detect actual provider/model substitution. | Independently recorded observed identity/provider must match approved pins. Hosted hidden weights remain outside verified observation scope. |
| A-08 | P, Committed, duplicate rejection and sink failure could be collapsed into one result. | Separate primary/admission outcomes and state records; sink fault has P but no receipt/row/budget consumption. |
| I-01 | An “independent” oracle could share parsers, fixture generators, helpers and unchecked policy assumptions with A/B. | Independent direct predicate specification, raw-byte expectations and explicit shared-assumption ledger; current checker implements no authorization predicate. Another parser implementation remains a future confirmation requirement. |
| I-02 | Mutation success could be inferred from a build failure or a redundant guard masking the target defect. | Exact mutation scopes/witnesses and required decision/transition mismatches in MUTATION_EXPECTATIONS.md. Zero mutants executed here. |
| I-03 | A signed/hashed producer explanation might substitute for independent reconstruction. | ANALYZABILITY.md requires raw fixtures, authoritative observations, typed reasons/subjects, event records and independent sink reconciliation. Ghidra is optional future tooling. |

## Validator and actual structural review

Used an isolated venv at `/private/tmp/t81-mco-schema-review-venv` with
jsonschema 4.26.0. `review/requirements.txt` records the full installed package
versions. Draft202012Validator.check_schema accepted all four original/current
schemas. The current embedded schema JSON equals the extracted schema files.
The exact Decimal integer-type adaptation is limited to the review checker and
documented in ORACLE_SPEC.md; no production parser is modified.

The authoritative Draft references distinguish instance data from syntax and
mathematical integers from host representations:
[Core](https://json-schema.org/draft/2020-12/json-schema-core),
[Validation](https://json-schema.org/draft/2020-12/json-schema-validation).
The selected validator documents schema meta-validation and per-instance APIs:
[python-jsonschema](https://python-jsonschema.readthedocs.io/en/stable/validate/).
No remote schema or model endpoint is resolved during packet checking.

Actual results are stored in `review/validation.json`. Four meta-schema checks
and 22 schema boundary probes pass their declared expectations. All 60 case
request profiles are valid; request schemas: 59 valid, 1 invalid. Response
profiles: 50 valid, 7 explicit raw-profile failures, 3 not observed; among the
50 parsed/profile-valid responses, 42 are schema-valid and 8 schema-invalid.
Schema validation is not run on the other 10. These are expected challenge
rejections, not candidate authorization-test failures or skipped safety passes.

The 729-row table and 24-plan table have the declared counts and classifications.
Fixture hashes, byte identities, schema/error annotations and basic declared
transition arithmetic are checked. Authorization annotations are design
expectations under ORACLE_SPEC.md, not independently executed semantic results.
Candidate runs=0, mutation runs=0. No CMake/CTest or performance run was needed.

## Reproduce only this data/schema review

```sh
python3 -m venv /private/tmp/t81-mco-schema-review-venv
/private/tmp/t81-mco-schema-review-venv/bin/python -m pip install -r docs/research/model-computational-object/review/requirements.txt
python3 docs/research/model-computational-object/review/make_corpus.py
/private/tmp/t81-mco-schema-review-venv/bin/python docs/research/model-computational-object/review/check_packet.py
```

The initial install used `pip install jsonschema==4.26.0` in that isolated venv;
the full resolved versions were then frozen. Corpus generation must reproduce
the committed data bytes. Hashes/probes do not authenticate trusted observations
or prove an engine safe. The scripts contain no effect sink, provider call,
operating-system primitive, production policy adapter or inference engine.

## Recommendation and unresolved questions

**REVISE** the scientific claim; the corpus/oracle design can proceed to external
review. No missing expressiveness or enforcement property was found beyond
ordinary typed services, explicit errors, reference monitors and protected
observers. Stop claims of OS necessity or semantic-ternary superiority for this
workload. An implementation is justified only by an identified conformance or
analyzability question, not by the availability of a frozen corpus.

Unresolved: another independently implemented parser/oracle; actual A/B runs and
mutation kills; trustworthy observation of provider identity; real workload and
human-review benefit; continuous-store/host integrity; physical resource limits.
The fixture TCB and deterministic snapshot assumptions remain assumptions. The
new data do not expand H01–H08, change H04 or PR #572, or validate hidden LLM
reasoning. Reviewers should treat model interpretations as input data and the
system's authority as a separately reconstructed fact.


## Final packet checks and portability

The original/current schema probe files preserve the observed regex result;
review/validation.json records all current counts and the binary-float underflow
counterexample. The data materializer reproduces all four JSON/JSONL files and
the case index byte-for-byte. All 53 local/pinned Markdown link targets resolve.
The file manifest can be checked with the same checker using `--manifest`.
No unchecked or unreachable case is counted as an authorization pass.

The packet-local .gitattributes pins LF for immutable hashes; byte payloads are
Base64 in ASCII JSON and explicitly decoded. Review tooling uses UTF-8 and exact
decimal arithmetic and performs no shell tool dispatch. These are portability
implications reviewed in source; execution was on macOS ARM64/Python 3.14.3 only.
No Linux, Windows or independent laboratory pass is claimed. All changes remain
within docs/research/model-computational-object/; protected research/runtime
surfaces are unchanged.
