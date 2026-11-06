# ARCH-003 current implementation report

**I2 disposition: ACCOUNTING VERIFIED within the bounded Linux ARM64 fixture
scope; independent review and campaign authorization remain pending.**

I2 continues from implementation `e38574ec935f67542ca15f5679c1e80c22292cfa`
without changing accepted protocol `1454bd63426d21686cfb5ef22dbaebc3c9cae94b`.
The new exact implementation SHA is reported in PR #716 and the final handoff.
The missing compiler-tool receipt hook described in the I1 report below is
superseded by the implementation and evidence in
[ACCOUNTING_REVIEW.md](ACCOUNTING_REVIEW.md). That document supplies mechanism,
Linux evidence, platform limits, independent-review readiness and the remaining
campaign blockers. The failed development runs remain retained.

No full campaign, campaign G1/G2 qualification, sanitizer diagnostic probe,
benchmark or timing pilot ran. No `PASS_CORRECTNESS_CORE` claim is made. The
following I1 report is preserved as the previous implementation assessment.

---

# ARCH-003-I1 implementation report

Disposition: **IMPLEMENTATION PARTIAL**. No `PASS_CORRECTNESS_CORE`, full campaign,
scientific corpus, timing pilot, benchmark, G1/G2 qualification, sanitizer probe
qualification or production modification is reported.

Base and accepted protocol: `1454bd63426d21686cfb5ef22dbaebc3c9cae94b`.
The implementation commit SHA is reported in the accompanying draft PR and final
handoff; this file is part of that commit and cannot contain its own hash.
The PR targets `docs/t81-arch002-equality-protocol` (PR #714), keeping the diff
restricted to `experiments/arch003/`. No merge is requested.

The user supplied P0's source tree/dependency order in this session and explicitly
authorized source, compilation and bounded component validation. The user also
reported Jules's acceptance of the exact protocol commit. This implementation
is authored and assessed by Codex with the protocol and P0 context. Model
identity is unestablished/null. No independent-person implementation review or
scientific validation is claimed. Canonical H03/H05 identities and historical
Hanoi/CWEB findings remain unchanged.

## Implemented source

- Standalone opt-in CMake D/R/S builds; no root CMake or CI registration.
- Operand-local tags, domain/extent admission, active-symbol-before-padding
  precedence, all five canonical encodings and six fixed count entry points.
- Separate scalar and histogram oracle encoder/decoder/validation TUs, with
  independent mathematics and no candidate linkage.
- Twelve-family descriptor enumeration, deterministic generators, canonical
  materialization, mutations, offsets, allocation modes and masks.
- Exact wire records, streaming SHA-256 pair identity, separately compiled FNV
  sink, aligned worker storage, immutable/disjoint input checks and mismatch
  prefixes. NATIVE is mandatory and correctness-only.
- Independent V parser/result join, per-role digest checks, canonical artifact
  JSON/index/root, oracle replay comparison, G1/G2 and S-probe infrastructure.
- L workflow for builds, serial probes/oracles/guards/profiles, review and
  authorization checks, sealed-identity retention, receipts and explicit
  failure/resource statuses. Its campaign compiler path is blocked below.

Exact added-file inventory: [CHANGED_FILES.txt](CHANGED_FILES.txt). All listed
paths are relative to the repository root. No pre-existing file was changed.

## Build and bounded component evidence

Environment: Darwin ARM64; `/usr/bin/clang++`, Apple clang 21.0.0
(`clang-2100.1.1.101`); Python 3.14.3; CMake 4.3.0. Exact compiler bytes, source
hashes, flags, commands, stdout/stderr, exits and binary hashes are retained in
[validation/components.json](validation/components.json) and its adjacent logs.
The CMake flags receipts distinguish the actual compiler from Homebrew Clang
also installed on this host.

| Configuration | Build | CTest | Python fixtures |
|---|---|---|---|
| D: `-O0 -g`, NDEBUG absent | compiled/linked | 3/3 | 20/20 |
| R: `-O3 -DNDEBUG` | compiled/linked | 3/3 | 20/20 |
| S: exact combined ASan+UBSan flags | compiled/linked, probes included | 3/3 bounded checks | 20/20 |

The source-hash receipt is the authority for exactly what these checks exercised.
Tests use always-active checks in R/S. S results cover the same small component
fixtures only. The prescribed heap-end and shift probes were **not run**;
compilation and small sanitizer-instrumented fixtures do not qualify the
sanitizer configuration or the full campaign.

Schedule enumeration checked exact per-family records and selected-call totals:
408,034 descriptors and 483,553 calls/profile. It constructed no scientific
payloads for that enumeration. No LARGE input was materialized. Candidate count
entry points and G1/G2 were compiled but never invoked, respecting P0's pending
oracle/verifier acceptance gate. Encoding/admission utilities were exercised.

Release BYTE disassembly contains ARM NEON `cmeq` instructions. Oracle symbol
inventories contain their respective oracle interfaces and no candidate
admission/count symbols. This is build inspection, not complete ISA, sink,
comparator or performance qualification. No timing measurement was taken.

## Oracle independence assessment

`oracle_scalar.cpp` explicitly decodes individual symbols and accumulates
scalar equality. `oracle_histogram.cpp` uses separate group/base expansion,
packing and envelope validation, and sums three diagonal histogram cells.
Neither calls candidate/production encoders, decoders, LUTs, popcount or SIMD;
neither calls the other. They share only types, descriptors, transport driver,
SHA-256 and wire framing. Candidate-side encoders are separate again.

Two five-record fixed wire streams agree byte-for-byte and match a separately
spelled-out Python reference for payloads, counts, masks and pair SHA-256. These
small fixtures do not establish agreement across the frozen matrix. Shared
schedule/transport errors remain possible; full source review and later sealed
stream agreement are still required. This is implementation separation, not
independent authorship, acceptance or replication.

## Evidence-verifier assessment

The Python fixtures reject wrong counts even with correct digests, unknown
statuses, nonzero error counts, malformed reserved fields, wrong sequence/role,
truncation, trailing output, invalid expected metadata, incomplete matrices,
unsafe paths, noncanonical JSON, missing completeness receipts and artifact
tampering. A fixed wrong-count guard-shaped byte fixture is rejected without
running either G1/G2 candidate control. Complete-bundle verification also checks
oracle/guard/probe receipt schemas and retained observation streams.

V imports no candidate/oracle mathematical code. Complete valid campaign
bundles, interrupted subprocess collection, six real guards, cap termination,
worker crashes, full replay receipts and final bundle verification have not
been exercised. Passing parser fixtures are not acceptance of that lifecycle.

## Remaining defect and untested requirements

**D1 — compiler-tool peak receipt hook is missing.** Linux driver-tree maxima
and sampled RSS are insufficient substitutes for the protocol's separate
per-process peak receipts. Campaign compilation raises
`BLOCKED_RESOURCE_ACCOUNTING` before launching its compiler. Darwin/Windows
collectors are unsupported. No campaign can pass this implementation as written.
The Linux live collector is source plus mocked boundary tests, not validated
Linux accounting. Race handling, descendant coverage and sampling feasibility
must be checked with bounded Linux fixtures before review acceptance.

The full L/V/W owner/verifier lifecycle, normal count implementations, all
scheduled payload/mutation tuples, actual offset/canary/immutability enforcement,
large streaming oracle agreement, sealed expectation replacement rejection,
interrupted-prefix retention, full manifest schemas, category accounting and
post-seal cap handling remain untested at their complete required scope.
The candidate implementations and S paths require the separately authorized
matrix, after oracle/verifier acceptance. No production conversion pipeline or
later comparator variant is qualified. Linux x86-64/ARM64 and Windows C++ builds
were not executed. macOS ARM64 compilation does not establish those platforms.

No implementation requirement was redefined to bypass the protocol. The
accounting defect is reported and blocked rather than weakening that contract.

## Resource feasibility limits

Numeric evidence arithmetic remains 82,328,144 normal bytes plus 392 guard
bytes = 82,328,536 bytes, under the 84 MiB numeric allowance. No such corpus was
generated. Largest BYTE payload pair is 32 MiB; other payload pairs and
streaming work are charged separately. The PT5 table is 59,049 bytes.

Neither 512 MiB aggregate absolute RSS nor the inclusive 1,800-second campaign
deadline has been demonstrated. Clang, ASan quarantine/runtime, Python,
hashing/conversion and verification overhead remain unmeasured. No inference
about campaign feasibility is made from component completion. No retries,
budget resets, shortened sanitizer matrix or omitted NATIVE cases are proposed.

Linux resource-unit interpretation was checked against the primary
[getrusage manual](https://www.man7.org/linux/man-pages/man2/getrusage.2.html):
child maxima are not a simultaneous tree total. This documentation check is not
validation of the implementation's collector.

## Proposed exact-commit campaign authorization

**Not recommended for this implementation SHA while D1 remains.** The draft PR
includes the exact SHA in a concrete, inactive authorization proposal. First
repair and review D1 using bounded infrastructure fixtures; pin the resulting
new commit. Accept the two oracles and V independently of candidate results.

A subsequent human decision must name the accepted protocol SHA, that exact
implementation SHA, host, Clang/runtime identities, fixed backend and a new
output directory; authorize one fresh serial D/R/S campaign under the single
512 MiB / 1,800-second / 100 MiB envelope, with all oracle streams, six guards,
two S probes and all six entry points. Preserve completed prefixes/failures;
no automatic retry, performance execution, merge or publication is included.
Authorization of this draft must not be inferred from its PR or JSON fields.
