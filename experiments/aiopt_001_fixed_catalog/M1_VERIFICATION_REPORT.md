# T81-AIOPT-001-M1-VERIFY correctness review

## Executive summary

**Recommendation: READY_FOR_INDEPENDENT_ACCEPTANCE.** Corrected code passes the
separate arithmetic reference and malformed-input/regression checks locally.
This recommends an independent maintainer decision; it does not declare formal
scientific acceptance, accept the proposed protocol, or authorize M2.

Examined remote HEAD/reference commit:
`b83414f0b04563cecae1c3741ceba7692dd81a11`.
Validated corrected implementation/verifier/protocol source commit:
`32d7f9a885d527772227d0d13d108f5de3483497`.
The subsequent evidence/report commit changes no executable source. Its exact
publication SHA is supplied in the delivery summary and containing Git history.

Four concrete defect categories were corrected: consumed catalog iterators,
Boolean/float catalog validation, unsupported-object/error-boundary handling,
and correctness assertions disappearing under Python -O. Valid-domain behavior
and its canonical digest are unchanged. The original **604** malformed-expectation
count was correct as executed calls; it was never 604 unique malformed states.

## Provenance and independence boundary

The named branch was fetched from origin, its actual HEAD identified, and the
reference commit confirmed as an ancestor (indeed identical to examined HEAD).
PR #665 was open/draft, not merged, with that head when inspected. Review used
an isolated detached worktree at /private/tmp/t81-aiopt-m1-verify, preserving the
original workspace and its unrelated untracked work.

Native macOS 26.5.2 / Darwin ARM64; Python 3.14.3, executable
/opt/homebrew/opt/python@3.14/bin/python3.14; standard library only for subject and
verifier. No compiler, C++ runtime, SIMD hardware, performance instrumentation or
third-party testing library is involved. Exact argv, working directory, return
codes, Python/platform, source SHA and file hashes are in
[execution.json](verification-evidence/execution.json).

This agent authored an earlier **different** min/max-based M1 package in the same
conversation. It has that prior context and inspected the target before writing
the separate reference. The target branch's author/model identity is not
independently authenticated here. Code/reference independence and fresh local
processes are demonstrated; independent-person or independent scientific
replication is **not** claimed. The schema record consequently sets
independent_execution=false conservatively, while documenting the local runs.

This PR's protocol is a **newly reconstructed prospective proposal**. It was not
recovered or previously accepted. Its equality matching, byte codes 0/1/2 and
invalid SWAR code 11 differ from the earlier min/max package; no earlier result,
22-record/1890-case corpus, or acceptance claim was imported into this review.

## Protocol-to-implementation compliance

| Contract | Evidence / current conclusion |
| --- | --- |
| T={-1,0,+1}, T^4=81 | Arithmetic ordinal generation and uniqueness check; all inputs covered. |
| First 40 lexicographic patterns | Derived without itertools.product; C[0]=(-1,-1,-1,-1), C[39]=(0,0,0,-1), first excluded vector=(0,0,0,0). Exact catalog agreement checked. |
| Equality matching and ordered indices | 3240 decisions for each of oracle, byte and SWAR (9720 checks), plus 81 complete outputs per implementation (243). All match independent rank-derived results. |
| Byte map -1→0,0→1,+1→2 | All 81 encodings/round trips, uniqueness, invalid byte values 3..255 at every position and invalid element types checked. |
| SWAR least-significant pair first; 11 invalid | Base-four division independently classifies all 256 payloads: 81 valid, 175 invalid. Both decoding and filtering checked for each. |
| Exact error categories and precedence | Full catalog validated before payload; supported-container length before elements. Invalid semantic trits, payload types, catalog size/order/contents/rows tested. v0.1.1 clarifies Python API types prospectively. |
| No partial match list on error | Malformed last catalog rows are rejected even when input matches the first row; invalid payload+catalog combinations fail as INVALID_CATALOG. |
| Deterministic repeatability | Two fresh normal processes and two fresh -O processes for both subject and verifier; complete JSON is identical within each mode, canonical digest identical across all. |
| Acceptance boundary | Local verification passed; protocol/maintainer/scientific acceptance still outstanding. No M2 permission. |

## Independent mathematical methodology

For ordinal k=0..80, repeated division by weights 27,9,3,1 yields base-three
digits; subtract one from each to obtain the semantic vector in lexicographic
order. Uniqueness of radix-three expansion fixes its ordinal, so its sole catalog
match is [k] if k<40, otherwise []. This avoids both the subject oracle's nested
comparison loop and the byte/SWAR predicates; production helper predicates and
catalog enumeration are not reused as the reference.

Expected byte codes use a literal {-1:0,0:1,1:2} map. Expected SWAR payload is the
sum of code_i*4^i. Independent decoding uses repeated divmod(payload,4), rejects
a remainder of three, and subtracts one from valid digits; it does not use the
subject's bit shifts, masks, XOR, encoder or decoder. The finite domain, catalog,
outputs and encodings are checked against the subject only after derivation.
No wildcard or generalized catalog semantics are introduced.

## Exact executed counts

[verify_m1.py](verify_m1.py) records counts only when a check executes. Each final
verifier process runs **13274 checks, 13274 passed, 0 failed, 0 skipped**:

| Check group | Executed checks per process |
| --- | ---: |
| Predicate decisions (3240 ×3 implementations) | 9720 |
| Complete filter outputs | 243 |
| Encoding agreement | 162 |
| Round trips | 162 |
| All 256 SWAR payloads ×decode/filter | 512 |
| Invalid byte values 3..255 ×4 positions ×2 APIs | 2024 |
| Invalid byte element types/ranges | 56 |
| Invalid semantic trits/types | 84 |
| Invalid SWAR scalar types/ranges | 22 |
| Wrong vector lengths | 30 |
| Unsupported/hostile object types | 61 |
| Invalid catalogs | 45 |
| Error precedence | 47 |
| Supported catalog containers / byte containers | 6 / 4 |
| Reference outcomes / mathematical facts / catalog agreement | 81 / 6 / 1 |
| Encoding uniqueness | 2 |
| Harness execution/digest/mutation integrity | 6 |

Four final verifier processes total **53096 passed checks, 0 failures, 0 skips**.
Four additional standalone subject runs pass. These are repeated executed checks,
not claims of 53096 unique mathematical cases or independent experimental samples.
The verifier also executes the subject internally for instrumentation and uses
intentional wrong-result/wrong-error mutations which must be rejected.

Malformed count audit: wrapping actual expect_error executions yields
INVALID_ENCODING=544, INVALID_LENGTH=15, INVALID_TRIT=36, INVALID_CATALOG=9,
total **604**. The 544 includes 24 invalid-byte expectations, 512 position-based
SWAR expectations and 8 SWAR type/range expectations. The 512 intentionally
revisit payloads invalid at more than one position; there are 175 distinct invalid
8-bit payloads. The independent verifier separately checks each of all 256 once
per API. Executed baseline source-line tracing confirms **604 category-assertion
hits in normal mode, zero with -O**, despite 604 error-expectation calls in both
modes. Thus the field was accurate as calls but not as executed assertions under
-O. [Line-hit evidence](verification-evidence/baseline-assertion-line-hits.json)
retains this distinction. After correction, counts accumulate from completed
expectations whose category guards remain active.

Independent instrumentation also observes **4171 completed require calls** in a
subject run in each mode: 3567 in run (3240 predicate +162 roundtrip +162 adapter
+3 structural checks) and 604 error-category checks. Instrumentation verifies 81
valid byte and 81 valid SWAR filter calls with -O as well as normally. These
figures come from actual calls, not the subject summary or documentation.

## Defects and narrow corrections

| Finding | Baseline evidence | Correction / regression |
| --- | --- | --- |
| Catalog iteration consumed | A generator of exact C40 passed tuple(catalog) validation, then yielded no rows; all filters returned [] for catalog member zero. | Unsupported one-shot catalogs now raise INVALID_CATALOG. Supported list/tuple rows are validated and snapshotted once; generator regressions cover all three paths. |
| Cross-type equality admits invalid catalog | Boolean zeros accepted; float trits accepted by oracle/byte but SWAR raised TypeError during shifting. | Strict integer-domain and row/shape/order/content validation of the entire catalog; Boolean/float and bad-last-row regressions. |
| Unsupported Python shapes escape contract | None catalog/input raised TypeError; protocol did not define container/precedence edge cases. | Prospective v0.1.1 API clarification plus guarded categories for unsupported containers, malformed rows and hostile objects; explicit precedence regressions. Valid mathematical semantics unchanged. |
| -O removes checks, leaves claimed totals | Valid byte/SWAR filter calls dropped from 81 each to zero; roundtrip/predicate assert expressions also vanished. Wrong error categories could pass. Summary still claimed the same comparisons/digest. | Always-active require; malformed count comes from completed expect_error returns; mutation tests ensure wrong valid output/error category is rejected under -O. |

Raw [baseline defects](verification-evidence/baseline-defects.json), original
normal/optimized instrumentation and both baseline subject outputs are preserved.
The current regression suite yielded 78 failures against the original source,
82 under -O. These include prospective type-boundary clarifications and multiple
checks per defect, not 78/82 independent mathematical defects. Final failures are
zero in the same suite, without changing equality semantics or expected digest.

Reviewer test correction: the first verifier draft mistakenly classified integer
4 as an invalid SWAR object, conflicting with the exhaustive valid-payload check.
That fixture was corrected (4 is valid) before implementation corrections were
accepted. [Initial verifier](verification-evidence/initial-verify-fixture.py) and
its initial result are retained; those initial counts are not final evidence.
Additional byte-type/hostile-object and -O mutation regressions explain the later
suite size. No failing output or prior result was silently discarded.

## Deterministic reproduction and commands

Every command runs from repository root; no timing is collected:

```sh
python3 -B experiments/aiopt_001_fixed_catalog/m1.py
python3 -B experiments/aiopt_001_fixed_catalog/m1.py
python3 -B -O experiments/aiopt_001_fixed_catalog/m1.py
python3 -B -O experiments/aiopt_001_fixed_catalog/m1.py
python3 -B experiments/aiopt_001_fixed_catalog/verify_m1.py --output experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.json
python3 -B experiments/aiopt_001_fixed_catalog/verify_m1.py --output experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.json
python3 -B -O experiments/aiopt_001_fixed_catalog/verify_m1.py --output experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.json
python3 -B -O experiments/aiopt_001_fixed_catalog/verify_m1.py --output experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.json
```

The actual run order interleaved subject/verifier per mode/repetition; exact argv,
exit codes and cwd are recorded in execution.json. Complete stdout/stderr and
machine-readable observations are retained per process. Canonical serialized
valid observations match the baseline and independently derived results:

`21bd3c7f69e35d32cd5d5b847f34e0e1f2c5218e76d1ffb0db7327bde47966c9`.

[Review](verification-evidence/review-record.json),
[reproducibility](verification-evidence/reproduction-record.json), and
[discrepancy](verification-evidence/discrepancy-record.json) drafts follow the
Commons schemas. Standalone schema checks passed; PR sources were not added to
or represented as validated by the Commons index. The reproduction schema lists
unique commands; execution.json retains repeated invocations. A duplicate-command
schema error was corrected without dropping the repeated raw executions.
Hashes identify bytes, not truth, authentic authorization or independent review.

## Changes, limitations and recommendation

Only the allowed protocol file and experiment directory changed. The existing
m1.py and README are corrected; verify_m1.py, this report and verification-evidence
are added. [Artifact inventory](verification-evidence/artifacts.json) lists exact
paths, hashes and sizes (excluding its own bytes to avoid a self-hash cycle).
Earlier workspace content, runtime, T81Lang/TISC/T81VM/Axion/CanonFS, canonical
hypotheses and governance are untouched. No benchmark, optimization, AI selection,
representation-selection experiment, new PR or merge was performed.

Limits: one native macOS ARM64/Python 3.14.3 environment; other Python versions,
OSes and architectures unexecuted. No cross-host or independent-person scientific
replication. API guarantees cover the explicitly clarified built-in containers,
not arbitrary user-defined objects/iterables or concurrent mutation. Finite exact
matching success says nothing about performance, hardware efficiency, inference
or broad H08 benefit. The protocol remains proposed and reconstructed.

**READY_FOR_INDEPENDENT_ACCEPTANCE** recommends an independent maintainer review
of the corrected source, clarification and preserved evidence. It grants no
formal scientific acceptance and no authorization for M2. Stop here.
