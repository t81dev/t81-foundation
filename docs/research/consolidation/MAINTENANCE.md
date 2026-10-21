# Maintenance burden and one bounded cleanup

Inventory baseline: `6977f49b3a2eba54d9776361a6026971ee529bfd`.
Counts include tracked source, docs, fixtures and public trust material within
these directories; they exclude caches and the rest of the repository. Exact
result hashes and machine-readable counts are in
[repository-snapshot.json](repository-snapshot.json).

| Directory | Files | Bytes | JSON bytes | C++/Python source lines |
| --- | ---: | ---: | ---: | ---: |
| `examples/governance/accountable-computation` | 69 | 374,283 | 259,453 | 1,070 |
| `examples/research/object-interface` | 4 | 60,639 | 28,736 | 352 |
| `examples/research/validated-descriptors` | 5 | 211,752 | 170,837 | 450 |
| Total | 78 | 646,674 | 459,026 | 1,872 |

These are physical source lines including blank lines/comments, not production
implementation LOC or transitive dependencies. The Python tests under `tests/`
are additional and remain intact. The JSON is about 71% of these directories'
bytes; it is not a repository-wide storage crisis.

## Duplication audit and disposition

| Finding | Evidence / maintenance consequence | Decision |
| --- | --- | --- |
| Repeated configure/build/test instructions | Four opt-in flags, repeated OFF toggles and separate target/test lists across five reports | Add one thin regression entry point; historical measurement instructions remain frozen. |
| Hash and fixture construction repeated in adapters | `run.py`, `compare.py`, native runners and descriptor fixture constructors each express SHA3-512-truncated identity / bounded input conventions | Retain now. A shared runtime fixture library would change source fingerprints and risk coupling comparators; no measured maintenance win justifies it. |
| Producer/verifier apparently duplicate arithmetic, policy and content-address calculations | `verify.py` imports no producer; `verify_concurrent.py` imports no native producer or measurement code | Keep intentional independence. Do not centralize the acceptance predicates merely to lower LOC. |
| Producer orchestration overlaps | Temporary fresh stores, subprocesses, environment/fingerprint capture and evidence JSON in `run.py`, `measure_concurrent.py`, both comparison drivers | Thin command consolidation is sufficient. A generic research runner would be another abstraction with versioning and exception-handling costs. |
| Repeated #540 evidence inside #541 | `validated-descriptors/results.json` is 170,837 bytes; its `baseline_results` equals the complete #540 recorded JSON (28,736 bytes as its standalone encoding), including fingerprints | Preserve immutable historical record. Any future run format may link a content hash to a separately retained baseline; do not retroactively rewrite signed/fingerprinted evidence. |
| Largest generated results | `concurrent-results.json` 115,524 bytes, `hardening-results.json` 50,204, original `results.json` 37,729, `concurrent-summary.json` 15,563 | Keep. Raw samples, traces, coverage and environment explain conclusions; summaries alone cannot replace them. Avoid re-recording merely for this consolidation. |
| Four build flags | `T81_BUILD_ACCOUNTABLE_EXPERIMENT`, `T81_BUILD_CONCURRENT_EXPERIMENT`, `T81_BUILD_OBJECT_INTERFACE_EXPERIMENT`, `T81_BUILD_DESCRIPTOR_EXPERIMENT` | No flag is proven redundant: they gate different prerequisites/targets; descriptor explicitly depends on object-interface. Keep default OFF and existing names for reproducibility. The duplication is users repeating their configuration. |
| Broader AI flag is distinct | `T81_ENABLE_AI_EXPERIMENTS` builds experimental AI CLI compatibility, not these four research suites | Do not collapse unrelated flags or infer research coverage from AI CI. No current workflow names the four dedicated options. |

## Preserve, freeze, and archive policy

Retain the production #538 boundary tests and the current A/D execution/verifier
attacks as active regressions. Retain #539 schedule handshakes, post-revocation
new requests, actual filesystem/block-image tampering and store-reuse refusal.
Retain #540's two real-provider fixtures, wrong-representation/storage/unsupported
operation checks and false/incomplete provenance challenges. Retain #541's
false-capability, payload/descriptor substitution, encoding/version, source
mutation and separate operation-denial tests as an opt-in negative research result.
Do not replace any of these with a prose claim.

Freeze the original #537 report/results/18 receipts and the #538 hardening
results/24 receipts, public keys and caller input/policy files. Preserve #539's
results/summary/traces and #540/#541's raw payload/descriptor evidence and source
fingerprints. A public key is required verification material; there is no committed
private signing key in the reviewed experiment fixture directories. Do not rotate
keys in historical receipts or expect regenerated Ed25519 envelopes/timings to
match byte for byte. Compare the documented semantic predicates.

The original vulnerable B/C binaries must be rebuilt at
`d8ad35254a1e35ca7374a4f57f7a870d568521fa`; contemporary B/C labels with a repaired
binary would falsify the historical experiment. Original source remains reachable
by the full head and merge SHAs in the ledger. Historical root-structure/publication
limitations are documented; this assignment does not repair unrelated workflows.

#540/#541 and the snapshot concurrency reference should remain opt-in, frozen
research rather than grow into public APIs. Logical archiving here means stopping
feature development while preserving executable sources and exact links. A path
move, code deletion or Git-history rewrite buys little at this scale and would
break links/fingerprints. No physical archival move is made. Production hardening
and its regression tests stay active; failed historical assertions stay visible.

## Single implemented improvement

[run_research_regressions.py](../../../scripts/ci/run_research_regressions.py)
configures the existing options, builds four existing experiment targets and four
native regression targets, checks that all eight test names are registered, and
runs exactly those CTests. It fails on missing tools/tests or any configure,
build/test error. The script rejects Python optimized mode because the unchanged
#540 harness uses assertions. It does not parse or share verifier predicates,
introduce a build option, alter production semantics or add another hypothesis.

The code is argv-based Python standard library using `Path`, CMake and CTest;
there is no shell command expansion or platform-specific executable suffix logic.
Ninja and a configured C++23 compiler are explicit prerequisites, alongside the
existing OpenSSL/Python requirements. Windows compiler setup is still the caller's
responsibility. Existing Asio can be supplied for offline configure. The chosen
build cache is reconfigured to this documented opt-in slice; use a dedicated build
directory rather than an unrelated working build.

This is a reduction from reconstructing several configure/build/test commands to
one entry point. It does not remove source lines from historical experiments and
does add a small maintained script. That tradeoff is justified by lower setup and
missing-test risk, not an unmeasured claim of faster onboarding.

## Validation of this consolidation

The new command completed a fresh Release build and **8/8 named CTests passed**
on macOS ARM64 using Apple Clang 21 and the existing pinned Asio source. This runs
current A/D at two repetitions, concurrency at three (192 initial operations),
the 36-case object comparison and the 132-case descriptor comparison, plus the
four native regressions. Cases overlap; do not sum those as independent findings.
The original 30-repetition results were reviewed and preserved, not regenerated.
Linux/Windows execution remains unmeasured; the 30-day objective addresses it.

All 42 checked-in signed receipts were independently checked: 36 accepted and
the six historical unauthorized B receipts rejected as required. The detached
schedule checker reproduced the recorded 64-case/1,920-operation summary. Recorded
source fingerprints match the exact research-head SHAs, including historical
sources that differ from current main. Documentation links, whitespace, preserved
result hashes, merged ancestry and unchanged historical trees also passed. No all-repository test
pass, cross-platform experimental pass or default-CI research coverage is claimed.
