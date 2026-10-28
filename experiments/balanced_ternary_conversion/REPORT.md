# H-CONV-01 — Balanced-Ternary Fixed-Point Conversion: Evidence Report

**Disposition:** **SUPPORTED WITHIN TESTED DOMAIN — PERFORMANCE EXPLORATORY**
**Status:** Standalone research; no production implementation change or architecture-wide claim.
**Source baseline:** `0662e0e637f47168e56ca4ddd66e8ee877473b38`.
**Preservation commit:** `e6a0d20ce9e62aaab3e548d8b55779c1a14e1a8f`.

## Research question and boundary

Can the private tensor conversion from IEEE-754 binary32 to balanced-ternary `Fixed<192,80>` avoid repeated wide division while retaining the **same canonical 48-byte representation**?

This experiment concerns the private `canonical_fixed_from_float_` path, not public `T81Fixed` constructors, `T81Float`, the T81 VM, CLI, CanonFS, governance, model execution, or native ternary hardware. See [SPEC.md](SPEC.md) for the exact bit, rounding, extraction and capacity contracts.

For finite binary32 `x`, the scoped target is `trunc_toward_zero(x * 3**80)`, encoded in 192 balanced trits. Nonfinite values produce no fixed representation. The independent oracle uses exact rational arithmetic.

## Compared methods

- **A — independent oracle:** Python exact-rational conversion and independent balanced-trit packing.
- **B — baseline:** extracted existing tensor helper; for negative binary exponents, repeatedly divide the scaled fixed-point magnitude by two.
- **C — alternative:** build the corresponding power-of-two denominator and perform one wide quotient; denominator doublings are counted. Positive-exponent behavior is retained.

The integer/fixed arithmetic bodies are extracted from recorded source snapshots. This is **not** a benchmark of the complete production binary.

## Evidence and observed results

The offline packet verifier reported:

| Evidence | Recorded result |
| --- | ---: |
| Distinct binary32 patterns in corpus | 1,778 |
| Oracle/B/C output mismatches | 0 |
| Planned native timing batches | 12 (six paired comparisons) |
| Completed native timing batches | 12 |
| Baseline B source-derived wide divisions | 87,510 |
| Alternative C source-derived wide divisions | 1,090 |
| Reduction in counted wide divisions | 98.75% |
| B median normalized time | 118.518 ms/input |
| C median normalized time | 1.622 ms/input |
| Ratio of these two reported medians | ~73.06× |

All 12 timed batches retained the same output digest. The operation counts are **source-derived high-level operations**, not instructions, cycles, energy, or an assumption that all divisions cost the same. C also performs 87,510 denominator doublings, which must not be hidden when interpreting the tradeoff.

The six pairs were measured on one host, using the fixed BC/CB schedule in `data/plan.json`. Native timings include conversion and common checksum consumption; input loading and output serialization are outside the timed interval. All completed rows, including an anomalously long B batch, were retained. The complete raw record and summary are in `raw-evidence.tar.gz` and `performance-summary.json`.

## Timing validity and limitations

The post-run `pmset -g log` capture identifies host sleep/wake activity overlapping the timing period. One baseline batch was particularly prolonged (approximately 259.45 ms/input versus a baseline median of 118.52 ms/input). **No observation was removed, adjusted, or replaced.** Consequently, the reported timing ratios are descriptive and exploratory, **not** clean compute-only performance estimates or a confirmatory speedup.

Zero mismatches across 1,778 inputs establishes agreement **on that corpus only**. It does not prove equivalence across every binary32 pattern or across other conversion contracts. Algebraic equivalence of repeated magnitude truncation and a final quotient is argued in SPEC.md under its stated capacity/rounding assumptions; it does not prove every arithmetic kernel correct. The standalone extraction changes infrastructure includes and error paths and does not reproduce all production optimizer or runtime conditions.

Neither the operation-count reduction nor native timing demonstrates an inherent advantage of ternary over binary hardware or a general T81 system advantage.

## Preservation, provenance and reproducibility

The Codex session stopped before producing its final packaging/report artifacts. The original experiment directory and raw data were recovered from the existing `research/h-conv-01` worktree. **Without rerunning the measurements**, `raw-evidence.tar.gz` and `manifest.json` were constructed afterward from the preserved files; this is a packaging operation, not original collection output. The preserved source/data files and the reconstructed archive/manifest were committed together.

Two loose capture files, `data/benchmark-stdout.txt` and `data/benchmark-stderr.txt`, are Git-ignored and therefore absent as standalone tracked files. Their original contents **are included in the tracked archive** and are used by the offline verifier. Do not treat their absence from a fresh checkout as missing measurement evidence.

From the repository root, verify without running native code or the experiment again:

```sh
python3 -B experiments/balanced_ternary_conversion/verify.py
```

The verifier checks the packet and archive SHA-256 inventories, extraction identities, source hashes, independent oracle regeneration, output equality, operation counts, chronological retention, completion and summary statistics. This command **passed in the recovery worktree** with 1,778 cases, zero mismatches, 12/12 batches and the counts above. A clean-checkout verification from `main` has since been performed and confirmed passing.

The packet contains host environment and power-event records; review them before redistribution beyond the research repository. Do not edit archived observations to repair a validation failure.

## Decision and follow-up boundary

**H-CONV-01 supports an exactness-preserving alternative for the tested private conversion domain and a substantial reduction in source-derived wide-division count.** The native performance observation is promising but exploratory and host-sleep-confounded.

Preserve this bounded result as a research finding. Do not merge it as a production conversion change or cite it as proof of ternary-vs-binary advantage. Any subsequent implementation or clean timing study requires a **separately scoped decision and protocol**; neither is authorized by this report.
