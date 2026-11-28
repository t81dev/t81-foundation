# RFC-00E1 corrective review packet

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [RFC-00E1 corrective review packet](#rfc-00e1-corrective-review-packet)
  - [Finding-to-file matrix](#finding-to-file-matrix)
  - [Accounting events and expected counts](#accounting-events-and-expected-counts)
    - [Addition](#addition)
    - [Multiplication](#multiplication)
  - [Oracle enforcement and limitations](#oracle-enforcement-and-limitations)
  - [Verification and evidence limits](#verification-and-evidence-limits)
  - [Maintainer scientific choices left open](#maintainer-scientific-choices-left-open)

<!-- T81-TOC:END -->


Disposition: **READY FOR INDEPENDENT REVIEW** (implementation corrections only).
RFC-00E1 remains **DRAFT**. Formal Gate 1 acceptance remains **WITHHELD**.
Gate 2 remains **STRICT NO-GO**. No merge, ratification or campaign is authorized
by this packet.

Reviewed and corrective-branch baseline:
`d3a6cf55517a3098d7bae91b07c72694bc3472da`.
The publication commit containing this packet is the final source head (reported
in the draft PR); it is not the historical evidence-generation revision.

Author: Codex, correcting implementation agent in the user's session; exact
underlying model identity is unknown. The user supplied the review findings.
These checks and derivations were performed by the same correcting agent, with
that prior context. They are not an independent scientific review or ratification.
Canonical H02, H01–H08 identities and historical Hanoi/CWEB lineage are unchanged.

## Finding-to-file matrix

Paths below are relative to `experiments/scg08_arithmetic_work/` unless stated.

| Finding | Correction | Files |
| --- | --- | --- |
| P1 oracle | Result-level five-field reports for both representations; `all_passed()` gates acceptance; operand round trips; output capacity, sign and signed-domain classification; mutation rejection | `exact_oracle_adapter.hpp`, `test_exp_scg08_01.cpp`, `test_result_rejection.py`, `CMakeLists.txt`; RFC §4 |
| P1 accounting consistency | Explicit mixed abstraction kernel boundary, event definitions, exclusions, partial normalization and logical memory diagnostics; weight 3 stipulated | `tier2_work_harness.hpp`; RFC §3 |
| P1 counters | Two binary limb additions, selected carry predicates, ternary scalar carry combination, copies excluded, all explicit borrow decisions, result-zero normalization and sign comparison | `tier2_work_harness.hpp`, `test_exp_scg08_01.cpp` |
| P1 conversion | Exclude and disclose multiplication-by-one decode, tryte encoding, carry construction, repacking, oracle conversion and setup; no claim of zero cost | RFC §3.1; this packet |
| P2 binary multiplication | Four helper additions plus four caller additions per visited product; correct stale sparse-case explanation | RFC §3.3 (formerly §3.2), `test_exp_scg08_01.cpp` |
| Evidence continuity | Prominent prospective correction notice; preserve historical report body, JSON, observations, thresholds and trials | `GATE1_IMPLEMENTATION_REPORT.md`; new raw logs under `docs/records/audits/2026-11-11-rfc00e1-corrections/` |

## Accounting events and expected counts

RFC §3 contains the event definitions. The weighted sum remains
`A + 3*M + L + C + P`. A is a ternary addc macro invocation or selected scalar
value addition/subtraction, M a selected product, L a named table access, C a
selected predicate/decision, P a normalization pass. This is not a primitive
host instruction census. Copies, bitwise operations, indexing and loop control
are excluded. Conversion is excluded even inside a kernel call. Logical limb
transfer counts exclude internal memory traffic. Normalization diagnostics omit
division/modulo and are not full cost visibility.

The following counts were derived from expression arity and control-flow paths,
not fitted from printed counters. Assertions then check those expectations.

### Addition

For ternary addition with n input limbs, m output limbs and k nonzero incoming
carries inside the input loop: A=n+2k, L=38(n+k), C=m. The two events for each
incoming carry are the second addc and `c1+c2`. Extension assignment adds no A.
The named lookup total is 9+(8+7+5)+(8+1)=38. This excludes local sum/carry-array
accesses and addc's map-ID arithmetic and indexing.

For same-sign binary addition: A=2n. C=1+2n+s+1, where s is the number of
output limbs visited by the zero scan. The fixed terms are the sign comparison
and final all-zero decision. Each input limb evaluates the carry selector and
one selected carry comparison. Extension limbs only copy/clear a carry.

| Fixture | A | L | C | W_ops |
| --- | ---: | ---: | ---: | ---: |
| Ternary two-limb zero/sparse/dense add, k=0 | 2 | 76 | 2 | 80 |
| Ternary two-limb carry absorption, k=1 | 4 | 114 | 2 | 120 |
| Ternary one-limb max + 1, two outputs | 1 | 38 | 2 | 41 |
| Binary one input/two outputs, zero + zero (s=2) | 2 | 0 | 6 | 8 |
| Binary one input/two outputs, 1+0 or (2^63-1) doubled (s=1) | 2 | 0 | 5 | 7 |
| Binary two inputs/three outputs, [2^64-1,0]+[1,0] (s=2) | 4 | 0 | 8 | 12 |
| Binary one input/two outputs, (2^64-1)+1 (s=2) | 2 | 0 | 6 | 8 |
| Binary +1 plus -1 (equal magnitudes) | 0 | 0 | 6 | 6 |
| Binary [0,1] minus [1,0], three outputs | 4 | 0 | 16 | 20 |
| Binary [0,0,1] minus [2^64-1,2^64-1,0], four outputs | 6 | 0 | 19 | 25 |

Cancellation C=1 sign +1 equal magnitude limb +1 c==0 +2 zero scans +1 final.
For unequal magnitudes in the last two rows, the common prefix is 1 sign +2
magnitude decisions +1 c==0 +2 `c>0` selections =6. Each subtraction limb does
one `smaller+borrow` and one `larger-sub`. Its decisions are the overflow
predicate, `!b1`, conditional second comparison, combined borrow choice: four
when b1=false, three when b1=true. Thus the two-limb case is C=6+8+2=16;
the three-limb case is C=6+(4+3+4)+2=19. Reusing `sub` removes an unnecessary
repeated addition from the predicate without changing the result.

### Multiplication

`mul64`: expand four 32x32 products. The four distinct additions are
`u1*v0+(w0>>32)`, `w1+u0*v1`, `w2+(w1>>32)`, `u1*v1+w2`.
Thus A=4, M=4, W=16, independent of data. For all-ones words,
`(2^64-1)^2 = (2^64-2)*2^64+1`: hi=2^64-2, lo=1.
Each caller product adds four A; each executed extension-carry step adds one.

Binary one-limb multiplication, two output limbs:

- Zero: M=A=0; C=1 sign +1 outer zero +2 zero scan +1 final =5; W=5.
- 1*1: M=4, A=4 helper+4 caller=8; C=1 sign +1 outer +1 bound
  +2 carry comparisons +1 extension zero check +1 scan +1 final=8; W=28.
  There is **no** extension addition because carry=0.
- `(2^63-1)^2`: M=4, A=8+1 extension=9; C=8+1 extension comparison=9;
  W=30. The low result word is 1, so the zero scan stops at the first word.

For ternary multiplication with n=54 decoded input trits and z_a,z_b nonzero
trits, all products fitting output capacity:
M=A=z_a*z_b; C=n+z_a*n+z_a*z_b; P=1.
Hence W=4*z_a*z_b+C+1. Zero yields W=55; 1*1 yields W=114.
For `2^63-1`, repeated integer division with balanced remainder yields 29
nonzero positions:
`0,1,3,4,6,7,8,10,12,14,16,18,19,20,21,22,23,25,26,29,30,31,34,35,36,37,38,39,40`.
The largest product index is 80, within the 81-trit output. Therefore M=A=841,
C=54+29*54+841=2461, W=5826. This derivation used integer arithmetic separately
from harness instrumentation; it is still authored by the correcting agent.
Zero normalization has exactly 81 trits: 4*81 additions/subtractions, 81 sign
comparisons and one pass. Division and modulo are outside these diagnostics.

## Oracle enforcement and limitations

Every arithmetic result in band and deterministic accounting fixtures is checked
against the base-10^9 oracle. Seven fixed pairs in each of five bands retain the
existing bounded coverage. Additional cases exercise extension copy versus actual
carry arithmetic, cancellation, two borrow paths and a full-word helper edge.

Overflow means **outside the original signed B-bit domain**, not failure of a
wider accumulator. It compares the mathematical result's classification with
the decoded result and reported classification. There is no separate kernel
status flag to validate. An out-of-domain result is accepted if exactly retained.
Binary digits are uint64_t magnitude words with a canonical nonnegative zero sign.
Ternary checks inspect decoded digits; packed tryte storage before decoding is
not independently validated. No claim of an internal transient overflow proof
is made. The oracle itself is a separate representation/algorithm, not an
independently certified implementation.

The portable Python subprocess driver deliberately corrupts numeric results,
canonical digits (while the permissive decoder still returns the expected value),
sign, capacity, overflow classification, binary zero sign, and binary value.
Each must exit 1 with the exact mandatory diagnostic. The eighth subprocess
checks the budget guard at counter 1001 without executing arithmetic. In-process
checks also exercise binary capacity rejection and a valid out-of-domain result
followed by an incorrect overflow label. `all_passed()` is the shared acceptance
path, replacing the old literal-false negative test.

## Verification and evidence limits

Environment: macOS ARM64 (`Darwin arm64`), Apple clang 21.0.0
`clang-2100.1.1.101`, target `arm64-apple-darwin25.5.0`, Python 3.14.3.
Final targeted CMake build and all four SCG08 CTests PASS, including eight
subprocess rejections and 94 existing schema-negative cases. NDEBUG is active;
SCG08_REQUIRE and oracle acceptance checks remain active.

Exact final commands, from `/private/tmp/t81-rfc00e1`:

```sh
cmake -S . -B /private/tmp/t81-rfc00e1-min-build -DT81_BUILD_EXPERIMENTS=ON -DT81_BUILD_TESTS=ON -DT81_BUILD_TUI=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_ENABLE_TERNARYOS=OFF -DFETCHCONTENT_SOURCE_DIR_ASIO=/Users/t81dev/Code/t81-foundation/build/_deps/asio-src -DCMAKE_BUILD_TYPE=Release
cmake --build /private/tmp/t81-rfc00e1-min-build --target t81_exp_scg08_01_test -j 2
ctest --test-dir /private/tmp/t81-rfc00e1-min-build -R scg08 --output-on-failure
```

The clean local Asio checkout is `12e0ce9e0500bf0f247dbd1ae894272656456079`,
verified against dereferenced `asio-1-30-2`. Default configuration was interrupted
while acquiring dependencies; minimal configuration without the cache failed
because github.com could not resolve. The cached configuration and build passed;
no source or dependency pin was changed. Commons CLI discovery was attempted
read-only but blocked by missing `jsonschema`; the canonical charters/register
and Commons onboarding instructions were read directly. Optional Draft7 schema
parity likewise did not run. These failures are retained rather than called passes.

Development commands also compiled and ran the C++ file twice directly:

```sh
clang++ -std=c++20 -O2 -DNDEBUG -I include experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp -o /private/tmp/t81-rfc00e1-test
/private/tmp/t81-rfc00e1-test
python3 experiments/scg08_arithmetic_work/test_result_rejection.py /private/tmp/t81-rfc00e1-test
```

Two standalone parent runs consumed 179 and 183 trials; two CTest parent runs
consumed 184 each: **730 total parent trials during this session**, below 1000.
Subprocess mutation tests execute no arithmetic; budget negatives only attempt
the guard sentinel. The final suite's 184 is new corrective validation, distinct
from the historical 179. The first development log was superseded locally; the
183-trial development log and final raw CTest log are retained. No benchmarks,
large-scale sampling, hardware performance measurements or production arithmetic
changes were made. Smoke-suite durations in CTest are execution logs, not timing
research. Historical evidence JSON is byte-identical to baseline.

No Linux, Windows or x86_64 execution was performed. The former POSIX fork/wait
checks were replaced with Python subprocesses and the C++ uses standard integer
operations without new ISA assumptions. Windows/Linux and other toolchains
remain unverified; portable source inspection is not cross-platform validation.

## Maintainer scientific choices left open

- Whether this mixed macro/scalar kernel metric is useful for future comparisons,
  and what multiplication weights or sensitivity analyses to authorize.
- Whether to authorize a separately versioned complete conversion/normalization/
  memory accounting model. Current costs are excluded, not established negligible.
- Independent review of the result validators and width reasoning, and any formal
  Gate 1 signature. No historical acceptance threshold or width table changes here.
- Separate dated authorization before any Gate 2 work. H02 timing, elementary
  operations-per-bit and weighted W_ops remain distinct evaluation models.
