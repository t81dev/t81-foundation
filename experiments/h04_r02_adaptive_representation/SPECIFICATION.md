# H04-R02 corrected experimental contract

Parent H04: Adaptive Representation Tiering, Track R. Original findings and
contracts are retained by Git and explicitly superseded where inconsistent.
This is a symbolic representation-cost experiment, not VM arithmetic execution.

Compare FB=64-bit, AB=8/16/32/64-bit, FT=41-trit and AT=6/11/21/41-trit
schedules on identical int64 traces. Equal trace coverage does not imply equal
native tier capacity. A sensitivity control replaces AB's first mode with
10 bits; it does not claim to be the optimal binary encoding.

Both adaptive arms pay one binary mode header per block, an equal per-value
selection/conversion coefficient, and an equal per-level transition coefficient.
A block uses one covering tier. Initial tier selection has no transition charge.
Four modes need two bits per block in a concrete fixed tag encoding; other
metadata values are symbolic sensitivity/ablation parameters.

C++ minimizes the declared cost with prefix DP. Python independently solves
suffix DP, and a separate rational enumerator checks small-domain minima and
crossover envelopes. Codec packing is a distinct Tier-0 stream experiment with
an equal 8-byte count header and no adaptive tags.

Acceptance: all encodings round-trip, ledgers match declared units, optimization
agrees with exhaustive feasible schedules, and finite packing is compared under
the same framing/alignment contract. A favorable model point is conditional;
a negative point does not falsify all H04 implementations. No changes to frozen
ISA, runtime, Axion or CanonFS are part of this experiment.

Full equations and assumptions: [MATHEMATICS.md](MATHEMATICS.md).
