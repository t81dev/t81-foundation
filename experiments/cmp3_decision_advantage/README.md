# EXP-T81-COMP3-01/r1 implementation

**M1–M4 approved; r1 activated as a prospective amendment. Measurement is not
authorized.** The implementation targets the approved r1 protocol and remains
partial pending Linux/GNU-GCC and production trusted-runner validation. No
experimental performance measurements or advantage claims are provided.

Authority chain: [original frozen #635 protocol](protocol/ORIGINAL_PR635_PROTOCOL.md)
→ [approved r1 amendment](protocol/r1/PROPOSED_PROTOCOL_AMENDMENT.md)
→ this implementation. See [authority and scope](protocol/AUTHORITY.md) and
[activation record with the five SHA-256 identities](protocol/ACTIVATION.json).
The original #635 protocol bytes and the existing research-document snapshot are
unchanged. The immutable approved packet retains preparation-time draft wording;
the later maintainer authorization is recorded separately without changing hashes.
No signed production approval, credentials, delegation or eligibility is implied.

Implementation baseline: `988b18ca9ebdb4a6fb76fe0be30115470e5637f0`. Integration
preserves the newer PR #637 head `1a71733cdd266a34a174d13185c78e5b6a07c68e` in
history and restores the audited separated harness over its monolithic version.
Historical [provisional report](evidence-r1-publication/provisional/IMPLEMENTATION_REPORT.md)
and patch remain pre-approval evidence; [publication validation](evidence-r1-publication/VALIDATION.md)
records the current tests and unresolved defects. The earlier
`evidence-2026-10-29/` artifacts describe their pinned older revision only and do
not establish complete #635 conformance. No T81 runtime or governance promotion
is part of this change.

Reproduce non-performance validation with Python 3.11+ and an actual C++20 compiler:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 experiments/cmp3_decision_advantage/validate.py \
  --compiler /path/to/clang++ --objdump /path/to/llvm-objdump --out /tmp/comp3-clang
PYTHONDONTWRITEBYTECODE=1 python3 experiments/cmp3_decision_advantage/validate.py \
  --compiler /path/to/g++ --objdump /path/to/llvm-objdump --out /tmp/comp3-gcc
```

`/usr/bin/g++` on this macOS host is Apple Clang, not GNU GCC. The driver logs exact
commands, builds each translation unit separately without LTO, tests optimized and
ASan/UBSan binaries, verifies corpus/bootstrap vectors, checks explicit failure
reasons and invokes the Python regressions and assembly/object inventory. It never
runs the experimental workload. The optional `--probe-pmu` executable mode opens
and starts/stops counters without a benchmark workload and discards counter values.
On unsupported hosts it reports PMU_UNAVAILABLE. Linux failures are infrastructure
evidence, never performance data or elapsed-time substitutes.

All five implementations retain comparable no-inline dispatch wrappers and shared
external less/equal/greater payload targets. Separate untimed classifier probes
and payload checks distinguish CORRECTNESS_FAILURE from PAYLOAD_PARITY. Pattern B
classifies the closed interval [-100,100]: below=-1, inside=0, above=+1, with payload
arguments `(V,0)`. Its oracle is independent piecewise code. Threshold endpoints,
adjacent values and signed integer extremes are covered. Optimizers may merge
comparison forms; identical lowering must be retained as evidence.

Corpus randomness uses old-state XSL-RR PCG128/64, seed
`5438315F434D50335F53454544303131`, stream 1/increment 3, multiplier
`2360ED051FC65DA44385C511560B53A9`. Initialization advances zero, adds seed, advances
again. Uniform draws use `(next() >> 11) * 2^-53`; bounded draws reject values below
`2^64 mod n` before modulo. Each pattern starts a fresh generator. Each decision
draws uniform first; A/C then draw full safe-range operand offset and nonzero k;
B draws k in 1..1000 even for interval samples, then an additional bounded(201)
only for an interval sample. Branch cutoffs are the binary64 literals .475/.525
(A), .20/.30 (B), and binary64 1/3,2/3 (C). `corpus.hpp` and independent
`test_corpus.py` freeze these conversions and order; the three first-eight CSV
fixtures come from the pinned approved packet. Tests compare all 30,000 generated
pairs as well as the 24 frozen pairs and four output vectors. Reproducibility is
experiment-scoped, not a deterministic timing or hardware-counter guarantee.

`r1_analysis.py` retains uint64 counters exactly and rejects invalid domains,
duplicate/missing observations and incomplete matched series before calculations.
It implements the approved paired df=9 instruction test versus Baseline 1, nominal
one-sided 99% paired bootstrap bounds, exact rational 15% comparisons, and six fixed
regression contrasts per pattern (18 overall), two-sided Bonferroni alpha .01/18.
It reports paired 95%, 99% and family intervals and retains nonzero tiny variance.
Zero differences have p=1; nonzero constant differences use limiting p=0 and point
intervals, explicitly marked degenerate. Numerical Student-t tails are checked
against independent quantile landmarks; these checks do not prove calibration.

Bootstrap resets PCG per pattern; each of 10,000 replicates draws ten bounded(10)
indices, rejects values below 6, uses paired indices and exact ratio-of-sums
reductions, and sorts fractions. Ranks 100 and 9900 (indices 99 and 9899) give
separate nominal one-sided 99% bounds, together a central 98% interval. The full
100,000-byte index sequence has SHA-256
`8c5f235bc6d1df1c1fcdca99903abaaaa8201591d5521fe84ba7e72868770b64`.

Per-pattern precedence is BLOCKED, then demonstrated-regression NOT SUPPORTED,
then SUPPORTED if all benefit/mean guards hold, otherwise INCONCLUSIVE. Positive
sub-15% improvements do not imply NOT SUPPORTED. Overall precedence is BLOCKED >
NOT SUPPORTED > INCONCLUSIVE > SUPPORTED; all three patterns must support.
Minimum-effect evidence is reported separately. These are approved r1 rules, not
claims about the original #635 design.

`analyze_r1.py` requires a signed completion bundle. Bare CSV has no measurement
entry point; legacy `analyze.cpp` always rejects. The audit-only `--test-only`
interface accepts a separately supplied test trust store, Ed25519 role-framed
canonical records, explicit synthetic kind and test delegation. Ephemeral keys
exist only within unit-test temporary directories. Even valid test signatures
always produce measurement BLOCKED. Test-only diagnostic SUPPORTED is never an
empirical experiment result. Production verification/launch have no test override.

Remaining blockers: the intended Linux/GNU-GCC compiler, PMU, affinity/SMT,
governor/boost/readback/restoration and native resource paths have not been tested
on this host. The production T1–T8 supervisor (separate restricted UID and protected
key, immutable same-file execution, single-process restrictions, atomic capture
and completion, protected durable journals and pre/post environmental evidence)
is not implemented end to end. Synthetic signature tests do not establish those
custody/launch properties. The test journal is not a production crash-durable
ledger. Normal-exit partial-pattern diagnostic retention is also not implemented;
this patch conservatively blocks incomplete bundles. Current environmental code
still lacks the approved full readback/isolation evidence. Production keys, root,
delegation, campaign and eligibility are unconfigured; do not provision them here.

Resource polling covers collection/generation and run boundaries; approved limits
are lifetime CPU user+system 600,000,000 us and peak RSS 500,000,000 bytes, with
Linux KiB normalized to bytes. Only eventual supervisor final accounting can
validate the output/exit path. Its native wait4 interface is Linux-only and skipped
on macOS; synthetic exact-boundary checks are not kernel enforcement evidence.

The actual governance registry is
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md`; COMP3 has no promoted surface.
Historical candidate-stage distributions, the architectural falsifier and withdrawn
Investigation 10A area claims do not govern this experiment or establish hardware
advantage. Investigation 10A remains PHYSICAL EVALUATION BLOCKED. Frozen history
is preserved. No T81 runtime, ISA, CanonFS, Axion or governance activation changes
belong to this patch.
