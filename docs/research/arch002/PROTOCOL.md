# T81-ARCH-002-R1 — Packed-trit equality correctness protocol closure

**Recommendation: ACCEPTANCE-READY for independent protocol review.** Status:
proposed; date: 2026-11-05. Acceptance and commit-scoped correctness-only
implementation/execution authorization remain separate human decisions. No
kernel, oracle corpus, correctness experiment, pilot, benchmark or merge is
authorized or reported here. Section 7 remains performance DESIGN ONLY.

R1 amendment baseline: `182634e0d5e4ac3a80413791d07a27dd7e82bc27` (PR #714).
The reusable-source inventory and its links retain the original source baseline
`d134407ae1059c4d7f6a9418177e695e0e5d3b51`; these are different revision roles.
The decision ledger is in section 9 of this existing document.

Parent: canonical **H03**, with H05 Boolean-equivalence context; ARCH-002 does
not replace either hypothesis. This is a synthetic representation/kernel-cost
discriminator, not evidence of application relevance, native ternary hardware,
or processor superiority. A conventional encoder can emit identical bytes.
Codex authored ARCH-001 and this protocol with the same context; model identity
is unestablished/null. No blinded or independent-person replication is claimed.
Jules's ISA-002 work, Axion, governance and frozen CPA evidence are excluded.
See [proposal](proposal.json) and [documentation validation](VALIDATION.md).

## 1. Reuse and non-duplication gate

All paths/line anchors in this table refer to the original reusable-source
baseline `d134407ae1059c4d7f6a9418177e695e0e5d3b51`. Existing
implementations are evidence and reusable interfaces, not independently
validated behavior in this task. Do not edit them or weaken their tests.

| Exact reusable source/function | Existing question; boundary for ARCH-002 |
|---|---|
| [ComputeTritVector::from_trits/from_packed/to_trits](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/include/t81/experimental/packed_trit_vector.hpp#L203), stable aliases in `include/t81/packed_trit_vector.hpp` | Canonical compute packing, validation and conversion; no equality-count API found. Use public aliases in future adapters. |
| [pack_pt5/unpack_pt5](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/codec/trit_packing.cpp#L70); `PackedTritVector::from_packed/to_trits` | Reuse valid PT5 encoders. Decoder accepts extra bytes and does not enforce unused-digit canonicality: apply the experiment's strict envelope outside production code. |
| [T81::EncodeTrit/DecodeTrit/UnpackDigits](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/include/t81/native.hpp#L33) | Different two-bit fields and positional arithmetic. Its decoder aliases code 11 to +1; this is not compute-vector decoding. Native adapter is correctness-only here. |
| `include/t81/support/bitopts.hpp::popcount64`, `include/t81/simd/simd.hpp` feature helpers | Eligible scalar population count and compile-time backend information; not proof of runtime feature access or optimized code generation. |
| [test_backend_equivalence](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/tests/cpp/test_tritwise_backend_equivalence.cpp#L34); `tests/cpp/test_packed_trit_vector.cpp::{test_phase2a_from_packed_validation,test_trailing_byte_masking,test_cross_representation_consistency}` | Reuse boundary cases and byte-identity ideas; independently specify equality oracle. Existing tests concern logic/conversion, not this count. Do not rerun all kernels to establish feasibility. |
| [BM_MaskApply_*, BM_PatternMatch_*, BM_CanonPipeline_*, BM_TensorLane_*](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/benchmarks/BM_TritwiseWorkloads.cpp#L24); `BM_PackedTritVector.cpp::{BM_ComputePack,BM_ComputeUnpack,BM_Kernel_TAnd_*}` | Min/max/negation pipelines, conversion and allocation costs; not a scalar count of corresponding equal symbols in two immutable arrays. Reuse infrastructure only after auditing timing boundaries and effective checks. |
| `experiments/h04_r03_matched_codec/SPECIFICATION.md`, §§1–5; H04-R02 inventory/results | Related codec/access work over 729-state elements; constructive binary-clone control already defeats exclusive density claims. Does not close this three-state equality/reduction question. No adaptive policy is tested here. |
| [AIOPT M2 adversarial assessment, §§1–2](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md#L16) | Related wildcard catalog filtering and mandatory bit-sliced controls. This experiment has no wildcards, fixed catalog, match-list output or repeated indexed queries. Preserve the critique; equality is not uniquely ternary algebra. |
| `experiments/radix_comparison/`, `experiments/packed_ternary_matvec/`, `docs/research/INVESTIGATIONS_07_08_CORRECTIVE_ASSESSMENT.md` | Preserve scalar-arithmetic, MatVec and matched-overlay negative/qualified findings. No instruction expansion or reopened frozen optimization campaign. |
| `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json` (H03/H05); `docs/research/commons/manifest.v1.json` | Canonical identities and curated evidence, not a live exhaustive experiment registry. H05 compares equivalent Boolean functions; H03 charges packing/access/conversion. |

Inspection used tracked-main path/content searches for `count_equal`,
`equality_count`, `equal_positions`, `hamming`, `popcount`, equality/reduction
phrases and ARCH-001/002; inspected the benchmark functions above, experiment
specifications, register and Commons records. Remote PR searches for equality
and ARCH-002 found related work but no matching protocol closing this question.
**No equivalent completed equality-count experiment was identified in that
scope.** This is not an all-history/all-unpublished-branches assertion. If an
independent reviewer locates an equivalent accepted protocol/results packet
before implementation, **STOP**, link it, and reuse it rather than execute a
duplicate. [VALIDATION.md](VALIDATION.md) retains search/identity checks.

## 2. Mathematical operation and experimental API contract

Inputs are two immutable, ordered arrays `a,b` of the same length N over
`{-1,0,+1}`. Elements are independent symbols, not positional integer digits.
Output is `C = sum([a[i] == b[i]], i=0..N-1)` as unsigned **uint64**, with
`0 <= C <= N`. Empty arrays return exactly zero. No approximate equality,
wildcards, reorderings, saturation, carry propagation or early “any equal” exit.

The request is exactly `Request{a,b}`, where each operand is
`Operand{format:uint8, n:uint64, payload:immutable byte span}`. Span extent is
the actual accessible byte count, not an advertised N-sized allocation.
Formats have stable tags BYTE=0, C2=1, PT5=2, NATIVE=3, PLANES=4; 5..255 are
unknown. No shared request-level format field exists. Thus mixed-format input
is representable and rejected before decoding or conversion. No implicit
transcoding occurs in the count API.

The selected entry point has a fixed registered format (section 4). It receives
only Request, not seeds, distributions, case IDs, expected counts, input hashes
or pair summaries. All normal valid calls use its registered format. Payloads
are disjoint immutable buffers; identical values do not imply shared addresses.
For N=0 both spans have extent zero and null data pointers; no dereference or
pointer arithmetic is permitted. Valid N is 0..2^24 inclusive: a resource/domain
limit, not the mathematical or uint64 count limit.

Return exactly `(status,count)`, with the following immutable uint8 status IDs.
Count is uint64 only for OK and null for every error; never release a partial
count. Diagnostic text/positions are not governed comparison outputs.

| ID | Status | First applicable condition, in this order |
|---:|---|---|
| 0 | OK | All checks pass; return the mathematical count |
| 1 | ERR_FORMAT | Either tag unknown, tags differ, or their common tag differs from this entry point's registered format |
| 2 | ERR_LENGTH | N_a != N_b |
| 3 | ERR_LIMIT | Equal N exceeds 2^24 |
| 4 | ERR_PAYLOAD_LENGTH | Either span is not exactly the size required by section 3 |
| 5 | ERR_SYMBOL | Invalid active byte, field, PT5 byte or plane overlap |
| 6 | ERR_PADDING | Noncanonical unused digits/bits |

At each preflight level check A then B. Validate active symbols of all A then
all B before padding of A then B. In particular, B's active error outranks A's
padding error. Padding-only code 11 in NATIVE is ERR_PADDING, not ERR_SYMBOL;
an entire PT5 byte 243..255 is ERR_SYMBOL even in a partial group. Format,
length and limit rejection must not inspect payload contents. Perform checked
size arithmetic only after the domain check. No access beyond either span is
permitted; SIMD tails may not rely on accessible guard bytes.

Each checked result is consumed once by the opaque, separately compiled common
sink defined in section 4.3. Its status/count frame and uint64 rolling digest
are fixed there. Full per-case comparison is mandatory; a matching digest alone
is insufficient. No output-only pointer observation or unused result counts as
consumption. Disable interprocedural/LTO optimization across the call boundary,
retain full optimization inside kernels, and inspect the qualified assembly
before any later timing to rule out count reuse/dead-code elimination.

## 3. Canonical representations and transformations

All formats encode increasing logical indices from low fields/bits to high.
Sizes exclude the identical section-2 Request descriptor (two operand-local
format tags, lengths and spans); record its actual size separately. No
ISA/container/wire format is introduced by the payload representation rules.

| Format | Canonical payload and validation |
|---|---|
| BYTE | N bytes: `FF=-1`, `00=0`, `01=+1`; reject every other byte. |
| C2 (ComputeTritVector) | `ceil(N/4)` bytes; slot j at bits `2j..2j+1`: `00=0,01=+1,11=-1`; reject active `10`. Unused fields must be `00`. |
| PT5 | `ceil(N/5)` bytes; `b=sum((t[j]+1)*3^j,j=0..4)`, 0..242. Reject 243..255 as ERR_SYMBOL, including a partial final byte. Missing trailing trits must be zero, hence their base-3 digits must be 1. Exact re-encoding of the decoded N symbols must reproduce the payload. |
| NATIVE (mandatory correctness adapter) | `32*ceil(N/128)` bytes of 32-byte blocks; four fields/byte, `00=-1,01=0,10=+1`; active `11` rejected by this experiment even though production DecodeTrit aliases it. Unused fields through the final block encode zero (`01`, whole zero-padding bytes `55`). Do not call native arithmetic. |
| PLANES | Positive plane followed by negative plane, each `ceil(N/8)` bytes. Bit i%8 of byte i/8 marks +1 or -1; neither means zero; both set on an active position is ERR_SYMBOL. All unused bits in both final bytes are zero. |

The independent BYTE-to-format transform follows these equations, validates
source symbols and emits canonical padding. Reverse transformation decodes
exactly N active symbols and validates the entire envelope. Cross-format
conversion goes through mathematical symbols, not reinterpret_cast or a shared
lookup. Existing production converters may be reused as candidates, **not as
the sole oracle**. The strict PT5/native envelope is experimental admission,
not a correction, relaxation or promotion of existing production semantics.

Payloads are N, ceil(N/4), ceil(N/5), 32*ceil(N/128), and 2*ceil(N/8) bytes
respectively. PT5's asymptotic 20% reduction from C2 is arithmetic, not a speed
result. Headers, tables, scratch, alignment and live buffers are additional.

## 4. Frozen correctness campaign

Nothing in this section may execute until the separate authorization in section
8. All counts below are schedule arithmetic, not generated or executed cases.
The six core entry points are fixed before implementation acceptance:

| Implementation ID | Entry point | Registered format tag |
|---:|---|---:|
| 0 | T-C2 | 1 |
| 1 | B-BYTE | 0 |
| 2 | B-PLANES | 4 |
| 3 | B-C2 | 1 |
| 4 | B-PT5 | 2 |
| 5 | NATIVE-ADAPTER | 3 |

NATIVE is mandatory for the entire correctness matrix, including invalids and
sanitizers. It is a strict encoding/count adapter, never positional arithmetic
or a performance arm. Its absence blocks core correctness acceptance. The four
conventional controls and their qualification requirements in section 5 remain
mandatory. The initial core campaign has exactly one source-frozen entry point
per row. Additional control/backend variants remain eligible for later strong
comparator qualification, but must first pass this entire matrix under separate
commit-scoped correctness authorization; they cannot be silently added to this
budget or compared after reduced coverage. No performance comparator requirement
is waived by a core correctness pass.
Core count coverage does not qualify an unexecuted end-to-end converter:
section-5 conversion pipelines must additionally match the sealed valid inputs
in every profile before later performance qualification, under separate approval
if that work would exceed this frozen campaign.

Two source-independent mathematical oracles must be accepted before candidate
execution: explicit scalar decoding/equality accumulation, and separately
implemented decoding plus the three diagonal cells of a 3x3 joint histogram.
Neither calls candidate/production encoders, decoders, SIMD, popcount or LUTs;
the two decoders/encoders may not share implementation logic. Both validate the
full envelope and agree on status/count and canonical input bytes for every
scheduled request, including streaming large cases. BYTE-to-F-to-BYTE checks
for each of the five F are part of valid-case preparation, including exact
re-encoding/padding identity. No extra F-by-G conversion Cartesian product is
implied. If a production converter will be used by a later end-to-end arm, its
bytes must match these sealed inputs before it can qualify; it is not an oracle.
The two oracle streams are compared and sealed before any candidate execution;
disagreement stops with FAIL_ORACLE. Expected truth is never repaired to match
candidate output. Streaming/compact byte buffers are required, not per-element
Python integer lists. Oracle independence is a source-review requirement, not
a claim that different role labels establish independent-person validation.

### 4.1 Schedule and ordering

Let L be the ascending list
`[0,1,2,3,4,5,7,8,9,15,16,17,31,32,33,63,64,65,127,128,129,
255,256,257,1023,1024,1025,4097]` (28 lengths). Let
`K=[4,5,8,16,32,64,128,256,1024]` and P(N) be the sorted unique in-range
members of `{0,N-1} union {k-1,k: k in K}`; P(0) is empty.
Format order is BYTE,C2,PT5,NATIVE,PLANES. Operand order is A,B. Ordered symbol
pairs are lexicographic over -1,0,+1. Ascending numeric order applies to every
remaining index/mutation value. These orders, not unordered container iteration,
define the case sequence. For each logical pair in families 0..5 expand formats
innermost in that order. Else expand the exact format-specific requests below.
Emit one expected record per expanded request; assign consecutive case_seq from
0. Emit observations in increasing case_seq, then implementation ID, using the
eligible implementation mask in the expected record. Normal calls go only to
entry points registered for that format; family 10 explicitly tests misrouting.
The always-active harness verifies disjoint nonempty span ranges and exact
input immutability against the sealed pair hash before/after calls. Hash only
actual accessible span bytes, never an advertised invalid N-sized range. This
verification is charged to correctness resources and lies outside later timed
regions as already required by section 5.

| Family ID | Frozen expansion (outer loops written first) | Expected records | Calls/profile |
|---:|---|---:|---:|
| 0 ANCHOR | Empty pair once, then all nine N=1 ordered symbol pairs | 50 | 60 |
| 1 EXHAUSTIVE | N=1..5; A ordinal 0..3^N-1; B ordinal 0..3^N-1 | 332145 | 398574 |
| 2 BOUNDARY | N in L; the 14 patterns below, then each p in P(N) with the two boundary mutations below | 4630 | 5556 |
| 3 SEEDED | N in L; distribution U then S; j=0..31, phase 0 with endpoint 0/1 respectively | 8960 | 10752 |
| 4 LARGE | Endpoint 0..3 from the generator definition below, phase 0, j=0 only | 20 | 24 |
| 5 OFFSET | N in L; o=0..31; allocation mode E then C; one cycle pair with last-position mismatch when N>0 | 8960 | 10752 |
| 6 ADMISSION | Formats in order; operand A then B; byte/mask v=0..255 as below | 2560 | 3072 |
| 7 NATIVE_FIELDS | N=128, NATIVE only; operand A then B; field i=0..127 changed to active code 11 | 256 | 256 |
| 8 PADDING | Formats C2,PT5,NATIVE,PLANES; partial N ascending; operand A then B; unused position ascending; wrong code ascending as below | 49012 | 49048 |
| 9 ENVELOPE | Unequal lengths, over-limit descriptors, then truncated/extra spans as below | 580 | 696 |
| 10 FORMAT | Known mixed tags; unknown-tag triples; then known-format misroutes as below | 778 | 4662 |
| 11 PRECEDENCE | Format order; fault-pair order, then the same-level/opposite-operand cases below | 83 | 101 |
| **Total** | No other implicit products or repetitions | **408034** | **483553** |

For family 1, ordinal v encodes symbol i as `(floor(v/3^i) mod 3)-1`.
There are 66,429 logical ordered pairs across N=1..5; N=5 contributes 59,049.
This includes all valid PT5 group pairs and all valid C2 group pairs.
For family 2 the 14 patterns are nine constant ordered pairs; three cycling
pairs A[i]=(-1,0,+1)[i mod 3], B[i]=(-1,0,+1)[(i+k) mod 3], k=0,1,2;
then two alternating pairs A[i]=(-1,+1)[i mod 2], B[i]=(-1,+1)[(i+k) mod 2],
k=0,1. For each p, first use A all zero and B all +1 except B[p]=0
(one match); then A all zero and B all zero except B[p]=+1 (one mismatch).
Families deliberately overlap as anchors/coverage checks, not independent
replications. N=0 executes the named empty patterns without reading memory.

Every family except 5 uses offsets (0,0) and mode E. Family 5 uses byte offsets
`(o,(7*o+3) mod 32)` relative to 32-byte-aligned allocation bases, so every
offset 0..31 occurs in each operand, in 32 pairs rather than 32^2 pairs. Its A
is the three-symbol cycle and B=A except its last symbol is advanced once
cyclically. E has exactly offset+payload_bytes allocated, no accessible trailing
byte; the span starts at base+offset. For nonempty spans the always-active
harness verifies the actual payload address modulo 32 equals its declared
offset before calling the entry point. C adds a 32-byte prefix and 32-byte suffix
of A5 hex, keeping the same payload address modulo 32; check all guards and
payload immutability afterward. N=0 uses null, empty spans in both modes.
ASan E catches allocation-end overreads; C catches guard writes. Prefix reads
within an allocation are not all detected by these tools; this is scoped
coverage, not a proof that every possible out-of-span read is observed.

Family 6 starts with separately allocated canonical all-zero pairs. BYTE uses
N=1, C2 N=4, PT5 N=5, NATIVE N=4: replace the selected operand's first byte by
v, retaining canonical remaining NATIVE padding. PLANES uses N=8 and sets both
plane bytes of the selected operand to v. Valid mutations must return their
oracle count; these are not all error cases. C2's active forbidden fields and
all PLANES overlaps are covered here, so no additional field-product is implied.

Family 8 starts with canonical all-zero pairs. C2: N=1..3, unused field j=N..3,
wrong two-bit code 01,10,11. PT5: N=1..4, unused digit j=N..4, replace digit 1
by 0 then 2 while leaving all other digits 1. NATIVE: N=1..127, unused field
j=N..127, replace zero code 01 by 00,10,11. PLANES: N=1..7, unused position
j=N..7, replace its plane state 00 by 01,10,11 (positive bit first in this
state notation). All are ERR_PADDING; no product of multiple unused positions.

Family 9 loops formats first within each subfamily: (a) unequal (N_a,N_b)
ordered as (0,1),(1,0),(4,5),(5,4), with each operand's own canonical zero
payload; (b) equal over-limit N=2^24+1 then UINT64_MAX with empty spans and no
advertised-size allocation; (c) N in L, operand A then B, truncate one last
byte when the required span is nonempty, then append one 00 byte. Peer spans
are canonical. Empty-span truncation produces no case; extra N=0 bytes do.

Family 10 uses N_a=N_b=0 and empty spans: (a) known tag pairs (f,g), f then g
ascending, f!=g; (b) for each unknown u=5..255 use (u,BYTE),(BYTE,u),(u,u) in
that order. These 773 requests run on all six entry points. (c) for known f
ascending, use (f,f) only on entry points whose registered tag differs from f;
one record holds that complement mask. All return ERR_FORMAT. There is no
unknown-by-unknown Cartesian product.

Family 11 starts with zero pairs at N=1 in each format. Faults in priority order
are F (A tag=255), L (N_b=N_a+1), M (N_a=2^24+1 and N_b equal unless L),
B (append one 00 byte to A), S (invalid active B), P (invalid unused A).
Use every pair of distinct applicable faults in lexicographic rank order: 10
pairs for BYTE, 15 for other formats. Build base small spans for the descriptor
N=1; M never allocates its advertised payload. Apply M then L, then tag/byte
mutations. Expected status is the higher-priority fault; incidental short spans
under M cannot outrank M. S uses BYTE byte 02, C2 field-0 code 10, PT5 byte
243, NATIVE field-0 code 11, PLANES overlapping active bit 0. P uses C2 field
1 code 01, PT5 digit 1 changed from 1 to 0, NATIVE field 1 changed to 00,
PLANES unused positive bit 1 set. Then, at canonical descriptor N=1, add:
both operands active-invalid for every format; and, for each padding format,
active-invalid A/padding-invalid B followed by both operands padding-invalid.
The S+P pair already checks padding A versus active-invalid B. BYTE has no P.

There are exactly 483,553 core-entry-point calls per profile, 1,450,659 across
three profiles. Oracle preparation/replay and the explicitly separate
guard/probe invocations in section 4.2 are additional charged work, not extra
core-entry-point cases.
Each profile replays identical sealed requests; lengths, distributions, offsets,
mutations and profiles are not multiplied in any other way. A cap breach or
unavailable required path yields an incomplete result, never case dropping.

### 4.2 Builds, sanitizers and harness-negative controls

After authorization, freeze compiler path/version/hash, target flags, backend,
source/binary identities and complete commands before data generation. Use the
same Clang-family toolchain on a given host for all profiles. Common flags are
`-std=c++20 -Wall -Wextra -Werror -fno-lto`; no warning suppression, fast-math,
cross-boundary LTO or sanitizer ignorelist is permitted.

| Profile ID | Required compile/link profile, in execution order |
|---:|---|
| 0 D | Debug: `-O0 -g`, NDEBUG absent |
| 1 R | Release: `-O3 -DNDEBUG` |
| 2 S | Combined ASan+UBSan: `-O1 -g -DNDEBUG -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all`; instrument at compilation and link with the compiler driver and both runtimes |

Build all three profiles serially and record their binary identities before
corpus generation. Next perform the two S diagnostic probes described below.
Then execute profiles D, R, S in that order. In D, execute the scalar oracle
producer then histogram producer over all 408,034 records and seal their agreed
stream; in R and S, execute each producer again over the identical schedule and
compare its full stream with that seal before that profile's G1/G2 and normal
candidate run. Thus there are six oracle streams (2,448,204 expected-record
evaluations), retained once by agreed byte identity with six explicit receipts.
No candidate runs if an oracle replay disagrees or a required probe is absent.

All 12 families, all six entry points and both allocation modes are required;
expected inputs/oracle seals are shared unchanged across D/R/S, never regenerated
with different seeds. The oracle producer executables,
admission, converters, scalar/tail/SIMD code and common sink used in S must all
be instrumented; prebuilt uninstrumented study libraries cannot count as S
coverage. Owner/verifier parsing is separately reviewed always-active code,
not a claim of C++ sanitizer coverage for their interpreter or the OS. Instrument
the two independent oracle producers in S and compare their streams to the
same seal before S candidates. Retain a single agreed expected stream by
content identity, plus each producer's source/profile/hash/exit/comparison receipt.

S uses `ASAN_OPTIONS=halt_on_error=1:detect_leaks=0:symbolize=0` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=0:symbolize=0`. LSan, TSan, MSan
and complete detection of all undefined behavior are outside this claim.
Required coverage is the compiler's address instrumentation and undefined-check
group throughout study-owned C++ code, with the two diagnostic probes below.
Use-after-return and use-after-scope submodes are not specifically qualified by
this revision; toolchain defaults must be recorded, not presented as matched
cross-platform coverage. A sanitizer pass is not a universal memory-safety proof.
Disabling LSan is explicit scope, not a leak-free result; disabling symbolization
avoids an extra diagnostic child and does not disable instrumentation. No
suppression/recovery or omitted study path is allowed. Before any S corpus
execution require two isolated probes: a volatile one-byte heap-end overwrite
must exit nonzero with `AddressSanitizer: heap-buffer-overflow`; a volatile
unsigned shift by that type's bit width must exit nonzero with `runtime error:`
and `shift exponent`. Preserve raw diagnostics. Compiler flag acceptance alone
is insufficient. Probes are expected toolchain failures, not scientific cases.

The exact Clang/runtime/OS tuple must support both sanitizers, their diagnostics
and the required allocation/accounting hooks. Otherwise BLOCKED_TOOLCHAIN (or
BLOCKED_RESOURCE_ACCOUNTING); retain labeled D/R subsets but no complete pass.
Do not substitute GCC/MSVC, disable a sanitizer, shorten S to boundary cases, or
move S to a different host while calling the original host complete. A separate
host may earn its own fully labeled D/R/S result only under its own authorization
and single shared budget. macOS/M2 or Linux availability is not assumed from a
feature macro. ASan virtual mappings are not RSS; do not use an address-space
ulimit as a substitute for RSS accounting.

References for coverage/flags, not evidence of local availability:
[Clang ASan](https://clang.llvm.org/docs/AddressSanitizer.html) and
[Clang UBSan](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html).

In EACH profile, before the normal matrix, perform G1 then G2 on the fixed
C2 Request N_a=N_b=1, payloads 00, expected count 1, implementation 0:

- G1: mutate its computed count to 2 before local comparison and sink; the
  worker emits that result and exits nonzero (1); the independent verifier
  also rejects and exits 1.
- G2: retain the same mutation and suppress the worker's own comparison; the
  worker emits count 2 and exits 0. The independent verifier still compares
  against the sealed anchor, rejects and exits 1. It must not trust child exit 0.

The owner in section 8 must observe those exact exits and the wrong-count
record. An unexpected verifier/worker exit, missing output or accepted count 2
is FAIL_GUARD; no normal candidates run. Expected negative probes do not fail a
campaign when correctly rejected. These checks remain active under NDEBUG.
No new performance timing data are produced in D/R/S; resource clocks are
accounting only. All three builds, probes, oracle work, six guards and normal
runs share the one section-8 budget; no per-profile or sanitizer budget reset.

### 4.3 Stable evidence serialization and verification

Use uint8 tags and explicit little-endian unsigned integers, never native struct
layout, addresses or locale-sensitive strings. Normal binary files start with
16 bytes: ASCII `A2R1` (4), kind (1: E=0 expected/O=1 observed), profile (1:
255 for expected, otherwise 0/1/2), probe (1: 0 normal, 1 G1, 2 G2, 255 shared
guard expectation), reserved zero (1), actual record_count uint64 (8). The
verifier writes/seals file headers, including actual prefix counts on failure.
Candidate-worker stdout contains only the following fixed-size observation
frames; oracle-worker stdout contains expected records instead. Neither stream
has a header, header seeking or diagnostic text. V constructs the file header
when retaining a stream, and records the raw-record-stream SHA-256 separately
from the header-inclusive file SHA-256.

Expected record, exactly 88 bytes in field order:
`case_seq:u64, family:u8, format_a:u8, format_b:u8, expected_status:u8,
N_a:u64, N_b:u64, bytes_a:u64, bytes_b:u64, expected_count:u64,
offset_a:u8, offset_b:u8, allocation_mode:u8, implementation_mask:u8,
pair_sha256:32 raw bytes`.
Mask bit k selects implementation k; only bits 0..5 may be set and the mask
must be nonempty. Allocation mode E=0/C=1. Error expected_count bytes are zero
but mean null, not a successful zero. The frozen schedule determines every
generation/mutation tuple from case_seq; no worker may choose tuples at runtime.
Each oracle applies the registered-format rule for every selected mask member
and must confirm their expected status/count is uniform before emitting the
single expected record. This is true by construction for the misroute masks;
the mask never supplies algorithmic expected truth to a kernel.

The pair hash is SHA-256 of the concatenation
`ASCII("ARCH002-R1-PAIR") || 00 || format_a:u8 || N_a:u64 || bytes_a:u64 ||
exact A span bytes || format_b:u8 || N_b:u64 || bytes_b:u64 || exact B span bytes`.
All integers here are little-endian. It binds tags, lengths and exact mutated
or canonical payloads; no input arrays need be retained. Offsets/allocation mode
are bound by the enclosing expected-file hash. The two oracle producers must
agree on complete expected records, not only aggregate counts.

Observed record, exactly 32 bytes in field order:
`case_seq:u64, implementation_id:u8, status:u8, count_valid:u8,
reserved8_zero:u8, count:u64, rolling_digest_after:u64, reserved32_zero:u32`.
Count_valid is exactly 1 for OK and 0 for errors; error count bytes are zero.
No unknown statuses, omitted/duplicated rows, unexpected implementations,
reserved bits, trailing bytes or premature EOF can pass. Kernel error output
therefore still has the abstract value (status,null).

Maintain a separate uint64 digest for EACH implementation in EACH profile,
reset also for each guard. Initial D is hexadecimal `cbf29ce484222325`.
For every invocation fold the nine-byte frame `status:u8 || count:u64` (zero
count bytes on error), in byte order: for each byte x set
`D=((D xor x)*0x100000001b3) mod 2^64`. Fold once, including errors, and emit
the updated D in the observation. The opaque common sink performs exactly this
work; its equivalent status/count work stays inside any later section-5 timed
boundary, while file hashing/serialization stays outside it. This is an FNV-1a
consumption checksum, not cryptographic integrity or sufficient correctness proof.

The independent verifier recomputes the digest from observed bytes, joins each
record to the sealed expected case and eligible implementation, compares status,
null/range/count, verifies order/completeness, and recomputes expected progress.
A well-framed wrong count is FAIL_CORRECTNESS, not a passing checksum. A bad
frame/order/digest is FAIL_EVIDENCE. Both prevent acceptance. The owner retains
the sealed expected hash and refuses any post-seal replacement.

Files are expected.bin and observed-D.bin/observed-R.bin/observed-S.bin. Retain
one expected stream when oracle byte streams agree, with both stream hashes
and comparison receipts; do not store duplicate full copies. On disagreement
retain the shared-prefix identity and both first divergent records and stop.
Guard expectation uses one expected record with case_seq=UINT64_MAX, family=255,
mask=1, and the fixed C2 anchor; observed guards use that sequence, implementation
0, and files guard-D-G1.bin through guard-S-G2.bin. Normal numbering never uses
UINT64_MAX. Guard rejections and worker/verifier exits are explicit receipts,
not ordinary scientific failures or ordinary passing cases.

Canonical JSON J is the Commons convention: sorted ASCII property names,
compact separators, ensure_ascii=true with lowercase Unicode escapes, no NaN,
infinity or floating numbers, no insignificant whitespace. Each JSON file is
UTF-8 J followed by one LF. Uint64 quantities in JSON are 16-digit lowercase
hex strings; measured elapsed-ns/RSS/exit fields use exact decimal integer values
within 0..2^53-1 (or null when unavailable), never rounded floats. JSON includes
authorization reference, protocol/proposal SHA-256, exact source/binary/compiler
identities/flags, role/profile/roster, expected/actual counts, final per-role
digests, guard receipts, sanitizer diagnostics, cap events and completeness.

campaign.json has exactly the following required properties (no additional
properties). All receipt paths are relative artifact paths listed in the index.
Null means not produced/unavailable; it may never stand for a passing check.

| Property | Exact value/type |
|---|---|
| schema | `t81.arch002.correctness-evidence.v1` |
| protocol_revision | `T81-ARCH-002-R1` |
| protocol_source_revision, implementation_source_revision | Exact 40-lowercase-hex Git commit identities |
| protocol_sha256, proposal_sha256 | 64-lowercase-hex hashes of accepted document bytes |
| authorization_receipt, host_receipt, binary_receipt, failure_receipt | Artifact path or null |
| build_receipts | Three path-or-null slots in D,R,S order |
| status_id, status | Corresponding numeric/text campaign status from the table below |
| complete | Boolean; true only for PASS_CORRECTNESS_CORE |
| expectations_sealed | Boolean; true only after the initial two oracle streams agree and the owner records the immutable expected-file hash |
| expected_records, calls_per_profile | Planned uint32 constants 408034 and 483553 |
| expected_file, guard_expected_file | `expected.bin` / `guard-expected.bin`, or null if not produced |
| actual_expected_records | Actual retained expected-record count, uint32 |
| observed_files | Three path-or-null slots in D,R,S order |
| actual_calls | Three uint32 counts of retained valid observation frames in D,R,S order |
| final_digests | Three arrays of six fixed 16-lowercase-hex values, implementation-ID order; D0 for a role with no calls |
| oracle_receipts | Six path-or-null slots: D scalar,histogram; R scalar,histogram; S scalar,histogram |
| guard_receipts | Six path-or-null slots: D G1,G2; R G1,G2; S G1,G2 |
| sanitizer_probe_receipts | Two path-or-null slots: ASan heap-end, UBSan shift |
| resources | Object with exactly rss_peak_bound_bytes, elapsed_ns, retained_bytes (exact nonnegative JSON integers <=2^53-1, or null), and accounting_complete (Boolean) |

Each oracle receipt has exactly profile (0/1/2), kind (`scalar`/`histogram`),
binary_sha256, raw_stream_sha256 (64 lowercase hex each), records (uint32),
exit_code (integer), and agreed (Boolean). A passing receipt requires exit 0,
408034 records, byte agreement and the same raw stream hash as expected.bin
after its 16-byte header. Each guard receipt has exactly profile, kind
(`G1`/`G2`), worker_exit, verifier_exit (integers), expected_count and
observed_count (16-hex uint64 values), rejected (Boolean), and observed_file
(path). Passing guard checks require counts 1/2, rejected=true, verifier_exit=1,
and worker_exit=1 for G1 or 0 for G2. Probe receipts have exactly kind
(`asan_heap_end`/`ubsan_shift`), binary_sha256, exit_code, diagnostic_file (path)
and detected (Boolean); require nonzero exit and the prescribed diagnostic.
An interrupted/malformed receipt is retained as a diagnostic artifact and its
manifest slot stays null, preventing a fabricated completed receipt.

Build/binary/host/failure receipts retain exact commands, flags, source hashes,
environment, complete exits and raw diagnostics; their tool-specific content
is opaque evidence, not another numeric result format. The verifier checks the
fixed manifest and numeric/guard/oracle schemas before declaring completeness.

Stable correctness campaign status IDs are 64 PASS_CORRECTNESS_CORE,
65 FAIL_CORRECTNESS, 66 FAIL_GUARD, 67 FAIL_EVIDENCE, 68 FAIL_ORACLE,
69 BLOCKED_TOOLCHAIN, 70 BLOCKED_RESOURCE_ACCOUNTING, 71 INCONCLUSIVE_RESOURCE,
72 BLOCKED_AUTHORIZATION. Owner exits 0 only for a complete core pass, 1 for
failures 65..68, 2 for blocking statuses, 3 for resource-incomplete evidence.
These do not rename section-7 performance outcomes. A confirmed mismatch remains
a failure even if a later cap breach occurs; it cannot be relabeled inconclusive.

artifact-index.json is exactly a JSON array of objects with exactly the keys
path (string), bytes (nonnegative integer <=2^53-1), sha256 (64 lowercase hex).
It lists every retained artifact other than itself and evidence.sha256,
sorted by unique ASCII relative path (slash
separators, no absolute paths or dot/dot-dot components). Include the campaign
JSON, expected/observed streams, commands, qualification and negative receipts.
SHA-256 hashes exact file bytes, including JSON LF and binary headers. The root
is SHA256 of the canonical artifact-index.json bytes; evidence.sha256 is its
64 lowercase hex digits followed by LF. This excludes a self-hash cycle.
Semantic streams must agree across completed profiles for their fixed case/role
schedule. Complete bundle roots may differ with host/build/resource receipts;
reproducibility means recomputing the root from the retained exact bytes, not
claiming identical host receipts or authenticity/authorization from a hash.

The normal streams occupy exactly 82,328,144 bytes including four headers;
the shared guard expectation plus six observed guard files add 392 bytes.
These are static record-layout/cardinality calculations, not produced evidence.

Fixed generator for later authorized implementation: SplitMix64 with uint64
wrap, initial state S; on each draw add `9E3779B97F4A7C15`, then
`z=(z xor (z>>30))*BF58476D1CE4E5B9`, then
`z=(z xor (z>>27))*94D049BB133111EB`, return `z xor (z>>31)`.
Draw bounded r in `[0,m)` by rejecting u>=`2^64-(2^64 mod m)`, then r=u mod m.
Generate interleaved a[i], b[i] from successive accepted draws:

- Uniform: m=3; r=0,1,2 maps to -1,0,+1.
- Skewed: m=20; r=0..17 maps to 0, r=18 to -1, r=19 to +1 (0.90/0.05/0.05).
- `S=0x4152434830303200 + phase*2^32 + endpoint*2^24 + j`, modulo 2^64.
  Endpoint 0=(2^20,U), 1=(2^20,S), 2=(2^24,U), 3=(2^24,S).
  Phases: 0 correctness, 1 control-selection, 2 variance pilot, 3 confirmation,
  4 resident secondary, 5 batch calibration. For boundary correctness use endpoint 0/1 by
  distribution and j as above; generator prefixes are intentionally reusable.

Oracle results and canonical input hashes are sealed before timed execution;
the seed/phase receipt is not delivered to kernels. Inputs are immutable per
invocation. Do not infer actual count from the distributions' expected match
probabilities (1/3 and 0.815). Retain every mismatch; stop scientific acceptance
on the first confirmed error. Repeated passing runs are not independent-person
validation.

## 5. Comparator qualification and cost boundaries

| Arm | Qualification and role |
|---|---|
| T-C2 candidate | One fixed checked C2 equality/count implementation, using the public representation. No PT5-vs-C2 candidate selection after seeing results. Backend/dispatch fixed in source before any pilot; no candidate tuning during selection. |
| B-BYTE | Optimized byte equality + widening reduction and validation; native NEON on M2, auto-vectorized loop also eligible. A scalar-only byte baseline is insufficient. |
| B-PLANES | `valid_mask & ~((Pa xor Pb) | (Na xor Nb))`, population count and uint64 sum; validate no active P&N overlap. Permit scalar hardware popcount and NEON byte-count/reduction variants, with justified ISA availability. |
| B-C2 | Conventional two-bit symbol equality/reduction over the **same C2 bytes**, independently implemented; permit word/SIMD masks, nibble LUTs and popcount. This same-byte control prevents attributing library tuning to ternary exclusivity. |
| B-PT5 | Conventional base-243 symbol groups; eligible 243x243 table of equality counts for full groups (59,049 one-byte entries), with scalar partial-group handling, validation, table construction and memory charged. This is the same logical alphabet; do not exclude it as “ternary.” |
| NATIVE | Mandatory correctness/encoding adapter in section 4; excluded from all performance arms/endpoints and from positional arithmetic claims. |

All four controls are mandatory on M2. Before pilot, each must pass §4 and have
source/assembly records confirming intended ISA and non-eliminated work. Permit
all above implementations to optimize the same operation, including fused
validation; prohibit expected-count metadata, pair preprocessing outside the
charged boundary, or layout-only reinterpretation. If a control cannot be
qualified, performance is blocked, not measured against a weaker replacement.

**Primary: single-use byte-input end-to-end.** Each timed invocation receives
the same pre-generated immutable BYTE arrays. Time starts before validation,
representation allocation/conversion and table initialization, and ends after
counting, common sink consumption, scratch/payload destruction and deallocation.
BYTE may operate directly, with no artificial conversion/allocation. Packed
arms must materialize their declared encoding; record its digest outside timing
on an equivalent verification invocation. No metadata about pair equality is
created outside timing. Runtime-generated lookup tables are constructed inside
each invocation; compile-time tables are permitted but charge their binary size
and record whether they are compile-time or constructed. No cold disk/process
launch is timed. Library one-time initialization belongs inside the first call;
any persistent cache must be reset between invocations or be a declared static
table. Correctness output must stay identical if any table/cache is absent.

**Secondary: resident-packed.** Start with validated canonical arrays, supplied
in each arm's declared format, no pair metadata. Time the same checked function
(including revalidation), dispatch, count and sink; storage conversion/table
preparation stays separately reported, never hidden. No confirmatory speed
decision from this mode; it requires its own later approved schedule. In both
modes separately itemize bytes, allocations, conversion, validation, table and
dispatch costs without subtracting “overhead” from the primary elapsed time.
Optional instrumented attribution is not mixed with clean timing.

Repeated calls use an opaque separately compiled boundary, so immutable inputs
cannot justify compiler-hoisted counts. No multiplication of an already computed
answer. Hashing input arrays and diagnostic serialization are outside timing;
the common count/status sink is inside. Reset/pre-touch the same source arrays
before each timed batch; no cache-flush claims. Report working sets, not assumed
L1/DRAM residency. Include read/write traffic and cache behavior as explanatory
measurements only when counter access is actually available.

## 6. Host qualification and fixed future execution conditions

Primary target is user-specified 2022 MacBook Air M2, 8 GB RAM, 256 GB SSD.
This is a target identity, not a newly collected hardware receipt. Checklist
before any performance authorization:

- Capture model identifier, architecture/CPU features (e.g. `sysctl`), OS/kernel,
  core topology, available RAM, native/Rosetta/emulation status and virtualization.
- Record compiler path/version/hash, target triple, exact flags/link commands,
  executable hash, enabled backend and disassembly. No `-ffast-math` relevance
  is assumed; operation is exact integer. Feature macros alone are not a host
  capability receipt. Never execute unavailable instructions.
- One computational worker thread, with the bounded owner/verifier processes
  in section 8; BLAS/OpenMP thread pools disabled. Record
  scheduling/QoS and migration evidence where available; do not claim physical
  core pinning on macOS without evidence. Do not pool P/E-core or host identities
  as independent architectural measurements.
- AC power, Low Power Mode off, no intentional concurrent workload, sleep
  disabled for the bounded run. Record battery/power and available thermal
  pressure state before/after each block. Do not require privileged monitoring.
- Record process baseline/peak RSS, OS pressure, swap-in/out counters and free
  storage before/after. No compressed-memory/swap-dependent timing. OS thermal
  warnings, pressure above nominal, sleep, swap activity, lost output or cap
  breach invalidate the campaign for confirmation; preserve it as inconclusive,
  not a deletable outlier. If required states cannot be observed, performance
  qualification is blocked. No automatic retry campaign.
- Record timer source/resolution; use a monotonic elapsed timer. Record PMU/
  hardware-counter permissions. Lack of counters permits qualified elapsed-time
  results but no cycle/cache-miss/energy claim. CPU features do not measure speed.

Jules's Linux container remains **UNKNOWN** until CPU, ISA/OS-enabled state,
container/VM/emulator, vCPU quotas/affinity/steal, physical-host visibility,
compiler, memory and PMU access are recorded. It may supply labeled correctness
results; no native M2 timing or pooled primary inference follows. Any Linux/x86
timing study requires a separate preregistration with a new comparison family.

## 7. Performance preregistration — DESIGN ONLY

Four primary endpoints are the two fixed sizes x two fixed distributions in §4,
on qualified native M2 only, using §5 end-to-end costs. Let T_B and T_C be positive
batch times per operation for a pilot-selected control and T-C2. Within paired
block j define `D_ej=ln(T_B/T_C)`. Primary estimand is
`theta_e=E[D_ej]` over the frozen generator/block-order schedule and qualified
run conditions; exp(theta_e) is a geometric-mean paired time ratio. This is not
ratio of separate arithmetic means, a best run or a pooled endpoint average.

**Threshold clarification:** “at least 10% improvement” means **10% lower
latency**, T_C/T_B<=0.90, so `theta0=ln(10/9)`. A 1.10 speedup is only 9.09%
lower latency and would weaken ARCH-001's stated time criterion. Success requires
each endpoint's simultaneous lower bound strictly above theta0 (conservative
at the exact boundary). No density-only or resident-only success substitutes.

**Pilot separation:** after correctness and separate performance approval:

1. Phase 1: j=0..7, all qualified control variants and fixed candidate; same
   inputs within an endpoint/block. For each endpoint select the control with
   lowest geometric mean end-to-end time. Exact ties use B-BYTE, B-PLANES, B-C2,
   B-PT5, then lexical variant ID. Freeze control IDs, source hashes, candidate
   dispatch and timing repetition counts; publish the full pilot. No pilot
   observations enter confidence intervals. This comparison is against the
   pilot-selected control, not a proof it is globally fastest.
2. Phase 2: j=0..15, selected control/candidate only, for variance planning.
   Freeze n once, before any phase-3 outcomes. Review log-ratio independence/
   stationarity and a normal-model diagnostic receipt. If lag-1 absolute sample
   autocorrelation exceeds 0.30, Shapiro-Wilk p<0.01, drift/host qualification fails,
   or sample SD is zero, mark pilot inconclusive and stop; do not switch analysis
   methods or tune the kernel to rescue it. Passing diagnostics does not prove
   the assumptions; the confidence/power claims remain conditional on them.
3. Phase 3: j=0..n-1, fresh phase-3 seeds only. Both arms get identical logical
   arrays. Never inspect significance to stop.

In phases 1,2,3, randomize each block with a separate SplitMix stream initialized
to `0x4152434830324F52 + phase*2^32 + j`. Fisher-Yates-shuffle endpoint IDs
`[0,1,2,3]` with bounded rejection, visiting indices 3 down to 1. Then, in
endpoint-ID order, phase 1 shuffles the full frozen arm/variant roster (initial
lexical ID order); phases 2/3 draw r in [0,2), using candidate/control order for
r=0 and control/candidate for r=1. Execute in the shuffled endpoint order with
those local arm orders. Calibration uses lexical arm order and no inference.
No data-generation stream is shared with ordering.

The diagnostic lag-1 statistic is Pearson correlation of consecutive pairs
`(D[0:n-1],D[1:n])`; zero variance is invalid. Define drift as an absolute
difference greater than `ln(1.05)` between the mean of the first floor(n/2)
ratios and the mean of the remaining ratios, in time order. Use these exact
diagnostics and thresholds in both phase 2 and confirmation. No discretionary
visual rejection or undocumented stationarity test may replace them.

The 64-block cap applies to confirmation, as in ARCH-001; the fixed 8+16 pilot
blocks and calibration are additional but share the single 30-minute/resource
cap. No new campaign or extra pilot blocks may be added automatically.

Before phase-1 collection, calibrate power-of-two repetition count R_e using
phase-5, j=0 data: double R from 1 until the shortest batch across the candidate
and all preregistered qualified control variants lasts >=20 ms, cap R at 2^20.
Freeze that common R_e for all arms in selection and all subsequent phases.
Record calibration separately; it cannot select a winner or enter inference.
Each repetition performs a complete single-use invocation; no answer
reuse or amortized packing. If cap cannot reach 20 ms, stop inconclusive. Use
one untimed full invocation per arm followed by the source pre-touch before each
timed batch; count this work toward resource caps. Each block consists of all
four endpoint pairs. Timer/allocator states are part of the observed workload.

**Coherent power target:** 90% power cannot be promised at theta=theta0: that
is the null boundary. Plan at **20% lower latency**, `theta1=ln(1.25)`, retaining
the 10% decision threshold and 64-block cap. This explicit alternative is a
planning assumption, not an expected benefit. To target at least 90% probability
of passing all four endpoints, require per-endpoint modeled power >=0.975;
the union bound limits total miss probability to 0.10 without endpoint independence.

Under the stated independent normal log-ratio model, with phase-2 SD s_e and
df=15, use conservative variance planning:
`sigmaU_e=sqrt(15*s_e^2 / chi2_quantile(0.0125,15))`.
Select the smallest common integer n in [20,64] for which every endpoint has
`P(noncentral_t(n-1, sqrt(n)*(theta1-theta0)/sigmaU_e)
   > t_quantile(0.99375,n-1)) >= 0.975`.
If none exists, **INCONCLUSIVE_POWER_CAP**, no confirmation. Conditional pilot
variance uncertainty prevents an unconditional 90% guarantee; report this.
Freeze the numerical package/version and independent verification of quantiles
before pilot (e.g. SciPy t/chi2/nct with survival functions); do not estimate n
from confirmatory data. No numerical power calculation or pilot ran here.

At confirmation compute the two-sided interval
`[L_e,U_e]=mean(D_e) +/- t_quantile(0.99375,n-1)*sd(D_e)/sqrt(n)`.
These four 98.75% intervals give >=95% simultaneous coverage under the model
by Bonferroni. **PASS_COST_IN_SCOPE** requires all L_e>theta0, correctness
and every host/resource gate. The same intervals govern every threshold decision;
do not choose a different one-sided or two-sided family after seeing data.
If an upper bound <=theta0, label that endpoint **NOT_SUPPORTED_AT_THRESHOLD**;
otherwise a failed lower-bound criterion is **INCONCLUSIVE_PRECISION**. Do not
call an inconclusive result proof of equality. Report all endpoints/intervals.
Confirmation diagnostics failing the same phase-2 thresholds make the campaign
inconclusive; no alternative bootstrap or selective run deletion afterward.

References for the design (not newly executed statistics):
[NIST Bonferroni method](https://itl.nist.gov/div898/handbook/prc/section4/prc473.htm),
[NIST t statistic](https://itl.nist.gov/div898/software/dataplot/refman1/auxillar/t_test.htm),
[SciPy noncentral-t functions](https://docs.scipy.org/doc/scipy/reference/generated/scipy.stats.nct.html).
An independent statistical reviewer must approve the model/planning receipt
before performance authorization. Correctness implementation does not depend
on a favorable power result.

## 8. Resource feasibility, risks and next gate

The old "one process/thread" wording means one computational worker, not the
absence of independent observation. Permitted runtime tree is exactly:

- L: one always-active campaign owner/accountant, holding the single monotonic
  deadline, sealed expected identity, resource records and verifier exit checks.
- V: at most one independently implemented supervisor/verifier at a time. It
  validates worker output against the sealed oracle stream, not the worker's
  own comparison or exit alone. It must not link candidate/oracle decode logic.
- W: at most one worker process at a time, with one computation thread, serving
  an oracle producer, adapter driver, negative guard or sanitizer probe. No
  concurrent oracle/candidate workers, child pools, nested workers or per-case
  subprocesses. Batch the normal matrix within one W per profile.

The third role L is needed to observe V's required nonzero guard exits without
terminating/resetting the campaign clock. V/W are reaped before replacement.
In compilation mode L may instead own one compiler driver and at most one tool
child; no V/W execution overlaps compilation, and use one build job. All such
descendants count. Diagnostic symbolizer children are disabled in S. Runtime
maintenance threads created by sanitizer/OS libraries must be recorded and may
not partition study input or run adapter/oracle work; their memory is charged.
External metadata utilities run only with V/W absent, within the same tree/caps;
live monitoring uses L's in-process OS interfaces. No extra monitoring process
or unaccounted helper is permitted while the runtime tree is populated.

Correctness D/R/S are one campaign: <=512 MiB aggregate absolute resident memory
over the live process tree, <=1,800 seconds from L start through all builds,
oracle preparation, guards/probes, profiles, output verification and sealing,
and <=100 MiB total retained output. No phase/profile/restart may reset limits.
Absolute aggregate RSS is the conservative R1 closure of the old additional-RSS
cap: record idle baselines for context but subtract none, including sanitizer
startup/quarantine and supervisor buffers. Count shared resident pages in each
process, conservatively. Use RSS, not ASan's virtual address mapping size.

L samples live-tree RSS at intervals no greater than 100 ms and obtains OS
per-process peak-RSS receipts at each reap. For each job, add L's campaign peak
and its child-role peaks to give a conservative aggregate bound; take the
maximum across sequential jobs, not a sum of every historical worker. Stop/kill
on observed excess; a later peak receipt showing excess also invalidates budget
conformance. This is bounded monitoring plus retrospective rejection, not a
claim of perfect instantaneous containment. Missing descendant/peak accounting
is BLOCKED_RESOURCE_ACCOUNTING. On deadline/output/RSS breach terminate the
process tree, retain the exact completed prefix and reason, and mark
INCONCLUSIVE_RESOURCE unless a confirmed failure already governs. No auto-retry,
extra campaign, omitted NATIVE case or shortened sanitizer replay is permitted.
The collector receipt names the OS API, raw units, conversion to bytes,
sampling interval and descendant coverage. No raw-unit assumption or missing
process may be silently treated as zero resident memory.

A separately authorized future pilot-plus-confirmation campaign gets its own
single identical resource envelope; section 7's calibration, 8+16 pilot blocks,
confirmation cap, statistical rules and timing boundaries are unchanged. D/R/S
are correctness configurations, not three performance campaigns. Neither this
revision nor a resource-incomplete run grants execution or re-execution rights.

At N=2^24 the BYTE pair uses 32 MiB; one C2/PLANES or NATIVE working pair about
8 MiB, PT5 about 6.4 MiB. Retain one arm's working pair at a time, <=64 MiB
scratch, <=64 MiB additional source/verification copies, <=1 MiB tables and
<=128 MiB runtime/analysis planning reserve. The previous major-buffer estimate
below 300 MiB is arithmetic, not measured RSS; supervisor/compiler/sanitizer
overheads must actually fit the aggregate cap or the campaign is incomplete.
Never retain all input arrays or per-element Python objects. Regenerate by the
frozen schedule and sealed pair hashes. Preserve every confirmed mismatch as
bounded operand slices/indices plus the full record and source identities.

Correctness output sub-budgets are <=84 MiB numeric streams (82,328,144 normal
bytes plus 392 guard bytes), <=4 MiB host/commands,
<=8 MiB diagnostics/assembly/mismatch slices,
and <=2 MiB manifests/source diffs, leaving 2 MiB global headroom. File sizes and
duplicates count toward the 100 MiB total; never silently truncate an artifact.
The original performance allocation (<=20 MiB numeric, <=10 MiB host/commands,
<=10 MiB diagnostics, <=10 MiB manifests/diffs) remains for section 7 only.
The distinct correctness allocation pays for complete per-call evidence rather
than silently replacing it with an aggregate checksum. No SSD I/O belongs to
later timed regions; elapsed resource accounting includes evidence writing.

| Risk / falsifier | Required disposition |
|---|---|
| Duplicate accepted equality-count packet discovered | STOP and reuse; do not silently rename the experiment. |
| Wrong count, acceptance of invalid code/padding, ineffective Release control | FAIL_CORRECTNESS; no timing authorization. |
| Strong byte/bit-plane/same-byte control missing | BLOCK comparator qualification, not a weak-baseline pass. |
| Same bytes/algorithm can be conventionally encoded | Report implementation/packing effects only; no exclusive ternary claim. |
| Win disappears after encoding/validation/allocation | End-to-end threshold not supported; resident result remains secondary. |
| Model, power cap, timer, thermal, swap, budget or evidence gate fails | Preserve all outputs as inconclusive; no auto-retry, relaxed threshold or removed sample. |
| Generalization to CPA, ISA, application benefit or physical energy | Unsupported by this protocol; requires a distinct authorized question. |

**Next authorization gate:** independent maintainer review of this protocol,
non-duplication finding and admission rules, followed by explicit commit-scoped
authorization for isolated correctness-only adapters, oracle, test harness and
the cases in §4 under §8 caps. This draft grants none. No production changes,
timed calls, pilot-selection runs, performance CI, ISA-002 work or historical
artifact edits belong to that implementation. After correctness review, a
separate host/comparator/statistical qualification and explicit performance
authorization are required. Do not merge this PR automatically.

## 9. R1 decision ledger and acceptance boundary

These are protocol resolutions of the six supplied ARCH-003 readiness
ambiguities, not experimental findings or independent acceptance. The R1 editor
is Codex, with the original draft and supplied assessment context; no model
identity or independent reviewer identity is inferred. Original H03/H05 findings,
historical negative records, frozen artifacts and source links are unchanged.

| Decision | Resolution | Reason / coverage and acceptance effect |
|---|---|---|
| R1-01 Process/accounting | L owner + independent V verifier + one W compute worker; serial jobs; aggregate absolute RSS and one clock | V must really exit on a suppressed-child-check guard while L retains accounting. Replaces an impossible literal single-process requirement; does not expand compute concurrency. Charges all overhead and all three correctness profiles to existing time/output ceilings. |
| R1-02 Admission | Two operand-local format tags and explicit registered-format binding; stable status IDs and total precedence | Mixed formats are representable without conversion. Invalid domain/length checks precede payload inspection; B symbols outrank A padding. |
| R1-03 NATIVE | Mandatory strict correctness-only adapter in all families/profiles | Resolves optional wording versus all-format obligations by retaining coverage. No NATIVE speed comparison or production decoder change; unavailable NATIVE blocks a complete pass. |
| R1-04 Scheduling | Twelve enumerated families, six core entry points, three profiles; 32 paired offsets only in family 5 | Exactly 408,034 requests and 483,553 calls/profile; prevents an accidental lengths x seeds x offsets x mutations x variants explosion without hiding exclusions. Additional variants require their own full qualification. |
| R1-05 Evidence | 88-byte expectations, 32-byte observations, fixed enum/order/FNV frame and SHA-256 artifact index | Makes truncation, missing rows, null/count confusion and hash reconstruction rules explicit. Both oracle streams agree before candidates; no hash substitutes for correctness or authentic authorization. |
| R1-06 Sanitizers | Full combined ASan+UBSan S replay with active NDEBUG guards and two diagnostic probes | No boundary-only sanitizer subset or silent toolchain fallback. All D/R/S work shares one budget; LSan/TSan/MSan and undetectable bounds accesses are excluded explicitly. |

R1 does not change section 7's statistical performance criteria. Independent
maintainer acceptance of this revision, review of the future oracle/comparator
implementation and explicit commit-scoped correctness-only authorization remain
outstanding. Compiler/sanitizer availability and budget feasibility are future
qualification facts, not documentation-validation passes. A complete core pass
would establish only this fixed schedule/roster/host's correctness, not broader
kernel, platform, architectural, performance or physical claims.
