# Corrected methodology

The eight legacy 100-value trace generators are preserved: constant_small,
constant_large, phased, alternating, boundary_oscillation, signed_extremes,
sparse_large and adversarial_transition. The default first trace actually
contains -100..-1 at length 100, within its declared small-value family.

Costs use symbolic common U: weighted payload + B*m + tau*level_delta + N*kappa
for adaptive arms. Binary metadata is never scaled by trit ratio. Both arms use
the same block partition, selector information and cost coefficients. Domains
are int64 at input; local mode ranges retain a known capacity confounder,
explicitly tested with a 10-bit first binary mode. DP is offline, not causal.

The optimizer does not compute arithmetic outputs, perform runtime conversions,
or measure its own planning work in the ledger. Packing alignment/framing is
accounted separately by actual codecs. No inference about decode latency is
made from symbol-slot counts. See [MATHEMATICS.md](MATHEMATICS.md),
[ORACLE.md](ORACLE.md) and [VERIFICATION.md](VERIFICATION.md).
