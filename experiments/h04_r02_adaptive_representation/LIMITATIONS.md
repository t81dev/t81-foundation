# Remaining limits

- C++ ledgers model precision assignment on int64 values, not ternary arithmetic.
  Python rejects outside-int64 inputs even though 41 trits cover a larger range.
- Four native tier boundaries differ; the 10-bit control is a sensitivity check,
  not exact equality of native state capacity or globally optimal binary coding.
- Offline DP sees the full trace; per-value selection costs are stipulated.
  Real encoder, conversion and causal decision costs remain unmeasured.
- Base-243 codec measurements are byte counts and round trips, not latency,
  throughput, energy, or native ternary device measurements. Compression of
  redundant traces or optimal binary block coding may change storage comparisons.
- Tests were executed on macOS ARM64. Linux and Windows runtime behavior is not
  certified by source inspection; CI status is recorded separately.
- H04-R01 commit def47c68 is recovered locally and archived. Reexecution of its
  producer/verifier/tests is reported separately from independent re-derivation
  of all R01 assumptions or reproduction in another laboratory.
- Historical arithmetic/matvec benchmark ratios are not timings for this codec.
  No result here justifies ending Track R or asserting physical impossibility.
