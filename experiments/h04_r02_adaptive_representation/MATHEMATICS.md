# Independent derivation and cost contracts

## Finite representation

For n positional balanced trits t_i in {-1,0,1}, x=sum(t_i*3^i).
The geometric series gives maximum (3^n-1)/2 and its negative minimum.
There are 3^n digit strings. Adjacent representable intervals obtained by
prefixing -1,0,1 are disjoint and contiguous, proving uniqueness and all states.

A binary code for these states needs the smallest b with 2^b >= 3^n:
b=ceil(n*log2(3)). Signed two's-complement coverage additionally requires
2^(b-1)-1 >= (3^n-1)/2, equivalently 2^b >= 3^n+1.
For positive n these yield the same integer b, but are different conditions.
Equal domain coverage is not equal native state capacity: powers of 2 and 3
cannot coincide for positive widths.

| Trits | Exact signed maximum | Covering signed bits |
| --- | ---: | ---: |
| 6 | 364 | 10 |
| 11 | 88573 | 18 |
| 21 | 5230176601 | 34 |
| 41 | 18236498188585393201 | 66 |

Six trits have 729 states, 8 bits 256, and 10 bits 1024. Six trits cover
[-364,364]; 8 bits cover [-128,127]; 10 bits cover [-512,511]. The legacy
8/16/32/64 vs 6/11/21/41 tiers process a common int64 input domain, but have
unequal local mode boundaries. The C++ top-tier bounds are the intersection
with int64, not a claim that 41 trits have only 2^64 states. The Python oracle
uses exact mathematical bounds while rejecting inputs outside that shared domain.
A trace ledger does not implement arithmetic on these values.

## Packing and finite access contracts

Five trits have 243 states and fit in one 8-bit byte: density 8/5=1.60 bits/trit
for complete groups. A standalone six-trit value needs two bytes under this
codec, with four unused final trit positions. Its optimal fixed binary index
needs 10 bits, also two bytes if independently byte aligned.

Concatenating N six-trit values before grouping costs exactly ceil(6N/5) bytes;
packed 10-bit values cost ceil(10N/8) bytes. Both implemented codecs prepend an
equal little-endian uint64 count. Framing is not free, and total length is not
sufficient to infer count for every stream (e.g. zero vs padding). Decoders
reject invalid length, noncanonical padding, and Base-243 bytes 243..255.
Examples from actual encoder outputs are in `packing_results.json`.

A value starts at trit position 6i, in byte floor(6i/5) at offset (6i mod 5).
It touches two bytes; random access needs extracting the appropriate base-3
digits. Independent byte alignment removes the saving; aligned blocks reset
padding. The current decoder implements sequential access; the offset formula
is a derivation, not a random-access benchmark. No adaptive mode tags are
encoded by this Tier-0 codec. Separate 2-bit tags cost ceil(2N/8) bytes in both
arms if concatenated; packing them jointly is a different framing contract.

The 0.4-bit/value difference survives complete five-value groups (48 vs 50
payload bits); byte rounding makes the finite difference depend on N. Equal
headers and equal tags do not reverse it: 9.6+2=11.6 < 10+2=12. It is a
saving against the specified 10-bit code, not against every binary encoder.
An optimized binary code for the same 729^N states needs ceil(N*log2(729)) bits;
it can implement the same Base-243 codec. Thus no radix-exclusive advantage
follows. The log2(3) lower bound assumes the full independent trit alphabet;
it does not prohibit compressing constrained or redundant workload traces.

## Symbolic cost and optimization

Use an abstract common unit U, C2=1 U/binary payload bit and r=C3/C2.
Metadata m is binary control bits costed in U; transition tau and selection/
conversion kappa are assumed U costs, not measured CPU instructions or seconds.
For a feasible block-mode schedule q with N values, B=ceil(N/block_size),
D(q)=sum of absolute successive tier differences, and payload S(q):

    AB(q) = S_B(q) + B*m + tau*D(q) + N*kappa
    AT(q,r) = r*S_T(q) + B*m + tau*D(q) + N*kappa
    FB = 64*N; FT = 41*N*r

Each block has one mode covering every value, including a final partial block.
The first mode is free of a transition charge. Both adaptive arms inspect
N values with equal kappa; the fixed arms have no tags, selection or transitions.
This is an offline optimizer with full trace knowledge. Optimizer computation,
real conversion, framing and arithmetic are not measured or priced by this model.

Prefix DP keeps the minimum cost ending in each of four modes, adds block
payload, one header and per-value kappa, then tau*abs(new-old) at boundaries.
Backpointers reconstruct a feasible optimal path. Infinity represents unreachable
states; finite nonnegative parameters and positive block size are required.
The independent Python implementation solves suffix subproblems. A third test
path enumerates all feasible schedules with exact rational arithmetic.

For fixed AB path p and AT path q, crossover r=(S_B(p)+O_B(p)-O_T(q))/S_T(q).
Equal coefficients cancel overhead only if event counts are equal. For optimized
AB cost A and AT envelope min_q(r*S_T(q)+O_T(q)), the positive crossover is

    r_star = max_q ((A - O_T(q))/S_T(q)).

This follows because the minimum is below A iff at least one path is below A.
The executable reoptimizes AT during monotone bisection (80 steps), rather than
freezing its ratio-1 schedule. Tests compare the emitted root with the exact
finite maximum and include a schedule-changing counterexample. Empty traces
have no unique crossover; the executable emits the documented sentinel 0.
The payload ratio in results is the ratio of the two schedules selected at the
requested parameters, not an optimized total-cost threshold.

For 100 constant-small values, 8-bit AB=800+200+50=1050 and
AT=600*r+200+50, so r_star=4/3. Replacing Tier 0 with 10 bits yields
AB=1000+200+50=1250 and r_star=5/3. At r=1.60 AT=1210.
For legacy boundary oscillation AB=1200+200+99+50=1549;
AT=600*r+250, yielding 2.165. These are modeled costs, not timings.

For comparison, the original scaled-header implementation on constant-small
would have AT=(600+200)*r+50, so its actual total-cost threshold was 1.25.
The original published 5/3 was inconsistent even with that original model.
The corrected 4/3 follows the declared equal binary-header contract.


## Numerical precision correction (PR #572)

B*m+N*kappa is shared by all adaptive schedules and both arms. The executable
cancels this term before DP comparisons and crossover optimization, adding it
only to reported totals. Subtracting it from rounded full totals would be too
late. The Python suffix oracle instead retains the shared term in exact rational
comparisons, independently validating cancellation. See
[PRECISION_CORRECTION.md](PRECISION_CORRECTION.md) for adversarial scales and logs.
