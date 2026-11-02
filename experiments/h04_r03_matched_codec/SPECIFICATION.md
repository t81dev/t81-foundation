# H04-R03 — Matched Codec and Access Contract

**Status:** PROPOSED / SPECIFICATION ONLY — not approved for execution or measurement
**Program:** T81 Foundation, H04 Adaptive Representation Tiering, Track R
**Proposed repository path:** `experiments/h04_r03_matched_codec/SPECIFICATION.md`
**Date:** 2026-10-29
**Review:** supplied draft **SPEC NEEDS CORRECTION**; corrected proposal below remains subject to maintainer review
**Scope:** Byte-level reversible representation on conventional binary hardware; not native ternary hardware, VM, ISA, or an adaptive scheduler.

## 1. Decision question

For the identical ordered sequence of integers in the **729-state domain** `[-364,364]`, does the existing Base-243 encoding yield a reproducible **practical** storage/access advantage over (a) fixed-width packed 10-bit binary fields and (b) a domain-matched binary block integer codec, after identical framing, alignment, access guarantees, and measured conversion costs?

**Critical null:** A conventional binary encoder can emit the identical Base-243 byte stream. Consequently, an exclusive radix-based *storage* advantage over every binary encoder is not possible under an unconstrained identical byte-stream contract. R03 tests **codec design tradeoffs**, not whether a binary machine is fundamentally incapable of ternary-density encoding.

## 2. Evidence boundary and inherited facts

The corrected R02 result is conditional symbolic cost modeling plus reversible packing and actual encoded byte counts, **not** CPU time, bandwidth, energy, hardware area, or native ternary execution. R02's Tier-0 codec uses five trits per byte (243 allowed byte values), with an 8-byte little-endian element-count header. For N six-trit values, its uninterrupted payload is `ceil(6N/5)` bytes; packed 10-bit payload is `ceil(10N/8)` bytes. R02 documents that random-index access was derived but not benchmarked and that optimized binary block coding remains untested.

Source inspection baseline: remote `main` **`872ac1b4c8a303fc5417d0b21551a430d0a50200`**, verified through the GitHub branch API on 2026-10-29. The following Git blobs match that revision; source inspection is not a rerun of its tests. Source anchors:

- `experiments/h04_r02_adaptive_representation/RESULTS.md` — Git blob `fdb70b1dcfdcdb995befd9df2e96ebcd8d7f8a76`.
- `experiments/h04_r02_adaptive_representation/MATHEMATICS.md` — Git blob `e8b83506bc8f6dd0edddf4e41526f4f2e004c106`.
- `experiments/h04_r02_adaptive_representation/LIMITATIONS.md` — Git blob `738903eaba896985d27d951b7e31460b5e515343`.

R03 does not revise or erase the R02 result, its negative findings, or the H01–H08 charter. Its codec/access question is a declared supporting refinement, not a test of the canonical adaptive-policy claim: these arms contain no adaptive selection. Hanoi/CWEB tier semantics remain historical and unresolved. R02 already establishes the fixed-field size saving and the constructive absence of radix-exclusive storage benefit; neither warrants a new benchmark. Only a separately approved access/conversion tradeoff remains empirically open.

## 3. Hypotheses and decision classes

- **H04-R03-S1 (fixed-field storage):** For sufficiently large sequential blocks, Base-243 payload is smaller than packed 10-bit fields. This is an already derived mathematical property, to be checked in executable finite cases, not claimed as a new discovery.
- **H04-R03-S2 (binary control):** A binary block integer encoder over the same 729-state alphabet can match or beat Base-243 byte size in at least some declared block/alignment conditions. A binary byte-compatible clone of Base-243 **must** match it exactly and serves as an equivalence control. More strongly, the specified fixed-length B729 block code is never larger than T243 on any common block under the framing below (Section 9). Its size comparison is resolved analytically, not a new empirical hypothesis.
- **H04-R03-P1 (practical codec):** Under an explicitly selected access contract, any Base-243 byte saving relative to fixed 10-bit fields may be offset by encoding/decoding or random-access costs. The direction is **unknown** before measurement.
- **H04-R03-N1 (radix-exclusive superiority):** This experiment cannot substantiate a radix-exclusive physical or computational advantage. A same-byte binary implementation is a constructive counterexample to a universal exclusive-storage claim.

**Evidence labels:** DERIVED, IMPLEMENTED, TESTED, MEASURED, MODELED, UNSUPPORTED. Do not promote any claim between labels without new evidence.

## 4. Exact common domain and inputs

Each logical element is a signed integer `x` with `-364 <= x <= 364`. Map it bijectively to `u=x+364` in `[0,728]` for codecs requiring unsigned digits. Each input is an ordered array of N elements; no codec may drop values, reorder them, assume narrower ranges, or exploit information unavailable to the other codecs.

The core **full-domain control** uses pseudorandom sampling targeting independent uniform draws over all 729 states, with a pinned reproducible generator and unbiased bounded sampling. Uniform marginal counts alone do not establish independence. Secondary workloads test data-dependent execution and locality, not compression: all specified codec sizes depend only on N, k, A and framing, never on value frequency or correlation:

1. Uniform full-domain independent values (primary comparison).
2. Constant zero.
3. Alternating endpoints `-364,364`.
4. Boundary-concentrated values (`-364,-1,0,1,364`) with documented weights.
5. Slowly changing runs with predeclared run-length distribution.
6. Deterministic ascending/cycling through all 729 values.

Before Phase B approval, pin the generator, seed list, exact workload weights/run distributions, finite lengths and resource caps in the proposal. Before Phase C, additionally pin generated dataset and query-schedule hashes **before any timed run**. Fixed seeds are reproducibility controls, not independent process repetitions. Use arbitrary-precision arithmetic plus a separately written reference for domain/size oracles; do not import candidate helpers as the only oracle. No favorable-case cherry-picking. Separate full-domain and structured-workload timing conclusions; none establishes entropy compression. Do not use the offline R02 DP schedule as if it were a causal codec.

## 5. Codec arms and mandatory equivalence control

| ID | Arm | Encoding contract | Role |
|---|---|---|---|
| **B10** | Packed fixed 10-bit fields | Each `u` stored in exactly 10 bits, least significant bit first across fields and bytes; zero unused high bits; reject decoded fields 729–1023 | Baseline fixed binary |
| **T243** | R02-compatible Base-243 | Write `u=x+364` in exactly six ordinary base-3 digits, least significant first; concatenate digits and pack each group as sum(d_j*3^j), j=0..4; absent final digits are zero; reject bytes 243–255 and noncanonical padding | Target codec |
| **B729** | Fixed-length enumerative block integer coding | For each actual block length m, form I=sum(u_j*729^j), j=0..m-1; encode in b(m)=bit_length(729^m-1) bits, least significant bit/byte first; zero unused high bits; reject I>=729^m | Stronger equal-domain binary control |
| **B243-CLONE** | Byte-compatible binary formulation of T243 | Identical grouping of ordinary base-3 digits of u and identical framing; hence identical bytes and access language | Constructive equivalence witness, not a mandatory fourth timed implementation |

For T243, d_j=t_j+1 and x=sum(t_j*3^j), so u=sum(d_j*3^j). The unsigned u itself is not a six-balanced-trit integer when u>364. This fixes an ambiguity in the supplied draft. T243 is R02-compatible only when block resets/framing match the original stream; the B10 unsigned wire mapping here differs from R02's two's-complement mapping, although sizes coincide. Do not attribute a timing difference to radix if it is due to that mapping. A future binary control must be allowed an optimized signed-field alternative if mapping conversion materially changes the result, with its wire mapping pinned before measurement.

B729 denotes a fixed-length enumerative block integer code, **not** an adaptive range/entropy coder. The B729 implementation must use arbitrary-precision integer arithmetic where needed; do not silently overflow 64-bit integers at k=64 or 1024. For each block size, calculate bit length using exact integer comparisons, **not** floating-point logarithms. B729 is a *block integer* control, not a proof of globally optimal compression on correlated data. Do not add a general-purpose entropy coder in the initial primary comparison; any additional codec requires an amendment and separately labeled exploratory results.

**Crucial fairness:** T243 and B243-CLONE may share the same encoded language; they are not statistically independent encoding schemes. If implementations share code, report that fact; an independent implementation is preferred for cross-checking, not falsely advertised as a different mathematical codec.

## 6. Blocks, framing, alignment, and index

Use k = 1,5,64,1024 values and A = 1,8,64 bytes as a **derived size-sensitivity grid**, not 12 mandatory timing campaigns. Each codec restarts at identical block boundaries. For B=ceil(N/k), block j has m_j=min(k,N-j*k) values. N=0 has no blocks. Actual block length, not k, determines final-block capacity and canonical padding.

Freeze this single layout; no optional header/index placement remains:

- Common header: exactly 8-byte little-endian uint64 N. Codec identity, k, A and access variant are fixed externally in the approved manifest; this is experiment-scoped framing, not a self-describing file format. Reject mismatched configuration; do not infer it from data.
- RA-FIXED: no index. RA-INDEXED: immediately after the count, exactly B little-endian uint64 absolute block offsets, no sentinel. Index bytes are 8B. Reject offsets differing from the canonical positions below, overlapping blocks, out-of-bounds arithmetic or overflow.
- Let H=8+index_bytes and L_c(m) be the payload byte length in Section 9. If B>0, data starts at D=align_up(H,A). Block j occupies align_up(L_c(m_j),A) bytes, **including zero padding after the final block**. Offset j is D+sum(q<j,align_up(L_c(m_q),A)). With full preceding blocks this is D+j*align_up(L_c(k),A), even for a short final block.
- Total bytes are 8 when N=0; otherwise D+sum(j,align_up(L_c(m_j),A)). No uncounted or extra trailing bytes. The initial alignment gap and all final-block padding are included. All alignment is relative to byte zero of the encoded artifact, not allocation address. Host-buffer alignment and any allocation/copy adjustment are separately equalized and charged.
- Report payload_bytes=sum L_c(m_j), tail_padding_bits for B10/B729 and tail_padding_trits for T243 (unused positions already included in payload_bytes; do not equate trits with physical bits), alignment_bytes=(D-H)+sum(align_up(L_c(m_j),A)-L_c(m_j)), header_bytes=8, index_bytes, total_bytes. Never double-count sub-byte padding as additional bytes; any block_padding_bytes field is zero because whole-byte alignment padding is reported under alignment_bytes.

Both access variants promise the same logical API: a valid index returns exactly x_i without decoding preceding **blocks**, with work/touched bytes/scratch bounded by the selected k and independent of N. This promises addressable blocks, **not equal per-query decoding work or O(1) in k**. An implementation may extract only required digits or decode its containing block; charge every touched byte, arithmetic step, allocation and cache/setup cost. B10/T243 cannot be forced to decode an entire block merely to imitate B729; B729 may use an optimized extraction algorithm. Precomputed powers/tables belong in setup and memory accounting, with identical reuse opportunities.

- **RA-FIXED** is primary: block address arithmetic needs no persisted index for any of these fixed-size codes. Separate block-address O(1) from element decoding complexity. T243 accesses at most two payload bytes for six consecutive digits; B10 at most three; B729 access cost is an implementation question, not assumed equal.
- **RA-INDEXED** is an optional separately approved index-overhead sensitivity, not evidence that a required index makes one codec competitive. The same B-entry layout is used for this controlled variant only. Equal access guarantees do not generally require equal or unnecessary index allocations. Derive this variant's byte counts before deciding whether its timing has a use case.

Validate complete inputs and canonical framing before admitted queries. Full validation work is reported separately and included in the end-to-end cost; a previously validated immutable buffer may amortize validation identically for all arms. Do not scan/validate the whole file silently inside one arm's lookup while giving others prevalidated input. Separate first lookup, repeated lookup, warm/cold caches, and any decoded-block cache with its setup/storage cost.

Adaptive mode tags are out of scope. Any later extension requires an amendment, a causal selector, and explicit equal-information control/conversion costs.

## 7. Functional and adversarial validation (required before timing)

1. Exact round trip of every standalone value in `[-364,364]` for B10/T243/B729 at k=1; independently check the clone identity formulation. No fourth optimized clone implementation is required to establish the already-proved equivalence.
2. Exact round trips for lengths `N = 0,1,2,3,4,5,6,63,64,65,1023,1024,1025` and larger predeclared N, including partial final blocks.
3. Exact output size equality with independently derived integer formulas for each arm/block/alignment case; check B729 bit capacity `2^bits >= 729^m` and minimality `2^(bits-1) < 729^m` for each actual nonempty block length m.
4. Rejection of malformed headers, truncated payloads, nonzero padding, T243 bytes 243–255, out-of-range B729 block integers, B10 fields 729–1023, extra trailing bytes, integer overflow, and invalid block/offset index data.
5. Differential decoder checks, with at least one independently written reference implementation; verify the same logical sequence across all arms.
6. Validate random-index retrieval against the original array at first, last, block-boundary and pseudorandom indices. Include out-of-range query rejection.
7. If an independently written byte-compatible reference is approved for correctness, it must match T243 exactly; disagreement is an implementation error, not a new radix result. Do not commission a separate timed clone merely to repeat the identity proof.
8. ASan/UBSan where supported; at least one independent compiler/platform lane before claims of portability. Record skips and environmental failures.

Passing correctness is necessary but not sufficient for measurement eligibility: separate Phase C authorization remains mandatory. All correctness gates must pass for a configuration before that configuration is eligible for performance measurement. A failed or untested configuration must be labeled, not silently omitted.

## 8. Performance protocol — proposed, not yet authorized

**Measurement is prohibited until a separate approval** pins: exact source commit and dataset hashes, compiler and version, optimization flags, CPU and OS, clock/power policy where available, affinity policy, sample sizes, repetitions, timing method, random-index query schedule, warm/cold cache treatment, and environment limitations.

The scope of any later measurement must first choose a real use case and **one primary workload × N × k × A × access/cache contract**, using Section 9 to exclude already-failed size gates. The rest of the grid is derivation or descriptive diagnostics, not a search for a passing configuration. Freeze process repetition count, finite run/attempt budget, bootstrap/resampling unit, confidence level and random-access tolerances before collection; retain infrastructure failures. No adaptive stopping on favorable estimates or selective retries. Missing preregistration blocks Phase C.

When separately authorized, measure:

- Encode time and effective input elements/s.
- Sequential decode time and effective output elements/s.
- End-to-end encode + persist + read + decode time (if storage is tested, document storage device, filesystem and cache state).
- Random-index lookup latency distributions (median, p95, p99), RA-FIXED and RA-INDEXED separately.
- Effective encoded bytes and decoded logical values per second; distinguish memory bandwidth from storage I/O bandwidth.
- Peak allocated memory and temporary scratch space.

For fair benchmarking, use equal input arrays and query schedules, identical decoded int64 output buffers/observable work and validation policy, include all mapping/encoding/decoding, table/index construction and allocation in declared setup/end-to-end costs, randomized or balanced arm execution order, warmups, multiple independent repetitions, an independent correctness checksum of decoded output, and a recorded clock source. Include process-level noise, confidence intervals (bootstrap or another predeclared method), and complete raw samples. Do not use a single fastest run or aggregate across incomparable platforms. Record any architecture-specific instructions/optimizations and whether arms have comparable engineering effort. No PMU or energy claim without a documented and validated measurement source.

### Pre-registration thresholds (proposal for approval)

The thresholds below are **decision rules proposed now**, not measured outcomes or authorization to run:

- **Storage-only gate vs B10:** exact total_bytes(T243)/total_bytes(B10) <= 97/100 for a predeclared N>=1024. This is a deterministic formula check, not a statistically inferred discovery. The threshold is evaluated for the actual header/partial-block/index/alignment layout, not payload alone.
- **Strong-binary storage:** classify radix-exclusive storage as **already resolved negatively**, not a success endpoint. B243-CLONE is identical and B729 never larger under this layout. No experiment is warranted to rediscover those inequalities.
- **Sequential storage/decode tradeoff vs B10:** require the 3% exact size gate plus a paired process-level throughput-ratio interval with **lower bound >= 0.95**. The estimand (ratio of population medians of per-process elements/s), fixed sample count, interval construction and confidence level must be approved before timing; do not mistake a sample median >=0.95 for noninferiority evidence. An upper bound <0.95 rejects that throughput qualification; an interval straddling 0.95 is INCONCLUSIVE. This qualifies only the named sequential tradeoff, not encode, random-access or end-to-end superiority.
- **Comparison with B729:** report a bounded Pareto comparison over predeclared dimensions and tolerances, including its smaller/equal size. Faster T243 with extra bytes is a tradeoff, not dominance. A confirmatory practical-win label requires a separately approved minimum useful time benefit, permitted size premium, encode/scratch and random-access regressions, and simultaneous uncertainty bounds. Until those are fixed, B729 timing is descriptive, and a B10-only win does not settle the decision question against a strong control.

The 3% and 95% numbers are proposed engineering choices, not physically privileged constants or authorized targets. Human acceptance must identify the use case that makes them meaningful before implementation/measurement. No claim of universal superiority is permitted from a finite codec/platform grid, even if every measured cell wins. Multiple exploratory comparisons cannot be promoted to a confirmatory winner without a prospective multiplicity/selection rule. If no owner-approved practical decision survives the derived size results, stop without implementing or timing new codecs.

## 9. Predicted mathematical checks (not benchmark outcomes)

For a contiguous stream of N six-trit values with no block reset or alignment, R02's derived payload sizes are:

`T243 = ceil(6N/5)` bytes; `B10 = ceil(10N/8)` bytes.

For N=1000, the R02 evidence reports 1200 vs 1250 payload bytes (both plus an 8-byte count header). This is a 4% reduction relative to B10 payload, but **zero** reduction relative to a binary implementation that emits the identical Base-243 bytes.

For B729, a block of k values requires `ceil(log2(729^k))` bits before byte padding. The unaligned full-block payloads below follow from exact integer arithmetic, not a codec implementation or benchmark:

| k | B729 bits b(k) | B10 bytes | T243/clone bytes | B729 bytes |
|---|---:|---:|---:|---:|
| 1 | 10 | 2 | 2 | 2 |
| 5 | 48 | 7 | 6 | 6 |
| 64 | 609 | 80 | 77 | 77 |
| 1024 | 9739 | 1280 | 1229 | 1218 |

For an actual block m, L_B10(m)=ceil(10m/8), L_T243(m)=ceil(6m/5), L_B729(m)=ceil(b(m)/8), b(m)=bit_length(729^m-1). Because T243 is injective into L_T243 bytes, 729^m <= 256^L_T243; hence L_B729 <= L_T243 for **every m**. Common monotone alignment and identical framing/index sizes preserve that total-size inequality. A B729 implementation violating it is incorrect or using a different contract.

At A=8, full-block k=5 strides all become 8 bytes, k=64 all become 80; k=1024 gives B10/T243/B729 = 1280/1232/1224. At A=64 the full-block strides for each k are identical across arms: 64,64,128,1280 respectively. For N divisible by k, there is no T243 size advantage in these tied cells; partial final blocks must use the explicit total formula. These facts eliminate redundant size-only trials before implementation. For N=0 there are no payload/alignment bytes.

The 729-state alphabet needs at least 10 bits for one fixed field since 2^9<729<=2^10. This is a worst-case full-alphabet fixed-length bound, not an entropy bound for correlated or restricted-value traces. None of these arms compresses those traces based on their frequency distribution.

## 10. Reporting, falsifiers, and stopping rules

**Report matrix:** derive sizes for the full declared grid, and for each separately approved workload × k × A × access contract × codec include input identity, logical domain, actual payload/framing/index bytes, round-trip status, measurement eligibility, and (only after authorization) timing distributions. Show negative and null results. Preserve raw evidence and exact source revisions.

**Stop before performance testing if:** the codecs do not have identical input domain or access guarantees; B729 overflows; an implemented byte-compatible reference disagrees with T243; any correctness test fails; an essential comparator is missing; or measurement authorization and reproducibility metadata are absent.

**Stop now for storage exclusivity:** the constructive equivalence and B729 size bound already resolve that claim, without implementing a clone or running a trial. Stop before Phase B if the only remaining objective is S1/S2/N1 or if no distinct owner-approved practical access/conversion decision survives. An exact byte-count implementation check may later validate code correctness, but is not an independent new scientific result. Continue to Phase C only with its frozen decision rule and finite resource/stopping budget approved. A failed exact size gate ends that configuration’s practical-win test before timing; do not search secondary datasets for a replacement primary.

**Interpretation:**

- B10 beaten on bytes only -> storage result against a fixed-width baseline, **not** ternary-exclusive or computational superiority.
- B729 or B243-CLONE matches T243 -> no radix-exclusive storage advantage under the declared contract.
- T243 wins on a declared size/latency tradeoff -> conditional codec-engineering result for that workload/platform, not native ternary advantage.
- No reproducible practical win -> negative result for tested codecs/conditions, not a universal impossibility proof.
- Any hardware advantage claim -> out of scope; requires separate matched physical characterization.

## 11. Implementation boundaries and review gates

**Phase A (now):** specification-only PR. Independent reviewer checks domain, codec equivalence, framing, block semantics, index/access parity, falsifiers, and predeclared measurement criteria. No new executable or benchmark in this phase.

**Phase B (separate authorization):** only after the distinct practical question and finite resource budget are accepted, implement or reuse the minimum B10/T243/B729 implementations, reference decoder and correctness checks. Reuse existing R02 packing rather than reconstructing its completed byte-count experiment. The clone is a proof/reference role, not a fourth benchmark project. Record deterministic byte counts as correctness evidence. No timing yet. Avoid new VM, ISA, runtime, governance or CI framework.

**Phase C (separate authorization):** perform pre-registered timing/access measurements only on approved configurations with an adequately characterized runner.

Required review disposition: **SPEC READY**, **SPEC NEEDS CORRECTION**, or **QUESTION ALREADY RESOLVED / NO EXPERIMENT WARRANTED**. An independent reviewer may recommend stopping at the constructive byte-equivalence proof rather than consuming engineering credits.

## 12. Provenance and governance

This is a new prospective H04-R03 proposal. It does not activate CMP3, modify frozen research evidence, approve M1–M4, or authorize production measurement. The scientific objective is to falsify or bound specific representation claims with fair comparators; infrastructure work is subordinate to that objective. The current session authorizes review and one specification-only draft PR. Phase B implementation, resource use and Phase C measurements have **not been requested/authorized**. Before Phase B, complete the Commons experiment-proposal schema fields (canonical H04, actual attribution, pinned revision, independent oracle, exact methods, uncertainty/raw artifacts, resource/security budget and explicit human authorization). Keep any such proposal within this specification or a separately approved artifact; this PR adds no infrastructure.


## 13. Adversarial specification review — 2026-10-29

**Disposition of the supplied draft: SPEC NEEDS CORRECTION.** This file incorporates the minimum contract/claim corrections, but remains a draft proposal. A distinct practical codec/access question is not answered by R02; its experiment warrant remains conditional on the explicit Phase B/C gates. The storage-only/exclusivity questions are already resolved and do not justify execution.

Reviewer: Codex (GPT-6), agent; original draft author/model unknown. This session previously audited H04-R02 and inspected corrective reports, so this is not a blind review or independent laboratory replication. It did not author the supplied attachment during this review. Separate AI role labels or separate code paths do not establish scientific independence.

| Submitted section / defect | Scientific consequence | Minimum correction incorporated |
|---|---|---|
| 5: six balanced trits for unsigned u up to 728; unspecified B10/B729 bit order | Not a unique reversible wire contract | Define ordinary digits of u, t=d-1, explicit endianness, integer orientation and invalid-domain rejection |
| 3,8,9: B729 size still treated as experimental; clone benchmark mandatory | Redundant implementation campaign for derived answers | Prove B729 never larger, tabulate exact finite bounds and retain clone as constructive witness |
| 4: secondary traces described as compressibility tests | These fixed-size codecs cannot detect distribution-dependent compression | Label data-dependent execution/locality only; size depends on lengths/configuration |
| 6: ambiguous final alignment, index placement and tail handling | Byte totals and offset/access parity can change by contract choice | One exact layout, including final block padding, initial gap, zero-length case, partial blocks and canonical offsets |
| 6: equal access confused with constant work or identical indices | Could handicap direct access or hide B729 block/validation work | Equal API and bounded block work; account different touched bytes, validation/cache/setup, optional index sensitivity |
| 8: sample median and unspecified uncertainty; post-selection across grid; arbitrary practical Pareto target | No confirmatory falsifier or meaningful generic practical-win label | Require a primary use case/configuration, fixed interval/sample/budget plan, noninferiority bound and B729 utility/tolerance gates before measurement |
| 8: finite Pareto result could license universal superiority | Unsupported generalization beyond tested implementations | Permit only scoped codec results; no universal or radix-exclusive inference |

**Source evidence:** the three R02 blobs pinned in Section 2 and `packing.py` at that same main revision were inspected directly. R02 already has reversible T243/B10 encoders, byte counts, access formulas and the explicit stronger-binary follow-up question. No historical test receipts were rerun here. The Commons discovery CLI attempt failed before discovery (`ModuleNotFoundError: jsonschema`); the pinned register, manifest and schema were inspected directly. Its older indexed R02 snapshot is not substituted for the corrected main bytes. No dependency installation was needed for this review.

**Independently executed check:** Python 3 integer arithmetic only: for k in [1,5,64,1024], b=(729**k-1).bit_length(), compare ceil(b/8), ceil(6k/5), ceil(10k/8), and align_up for A in [1,8,64]. Exit 0; results are the Section 9 table. This is a derivation check, not codec implementation, correctness execution or performance measurement. Source blobs were checked with `git ls-tree` against the live main SHA. No codec/benchmark, VM, scheduler or infrastructure change was made; no historical evidence or canonical hypothesis was edited.

**Single next decision:** maintainers review this corrected specification and either select a concrete practical tradeoff/use case with finite Phase B/C preregistration or stop at the already-resolved mathematical result. Do not authorize engineering merely to recreate R02 or confirm byte-stream equivalence.

Supplied attachment SHA-256: `b8c514f76d6834aecc61e0caee312f807bf57f3da55862ab2c8d4c33b7fd42ba`. Corrections are prospective specification edits; this digest identifies the input, not acceptance or provenance authenticity.
