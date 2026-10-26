# Corrected executable results

Current authority: generated `results.json`, checked against the executable and
independent oracle by the audit suite. Default r=1, m=2, tau=1, kappa=0.5,
block_size=1, legacy 8-bit Tier 0. Costs are abstract U; slots are bits or trits.

| Workload | FB | AB (slots) | FT | AT (slots) | Payload ratio | Optimized total crossover |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| adversarial_transition | 6400 | 3397 (3000) | 4100 | 2372 (1975) | 1.518987342 | 1.518987342 |
| alternating | 6400 | 4147 (3600) | 4100 | 2897 (2350) | 1.531914894 | 1.531914894 |
| boundary_oscillation | 6400 | 1549 (1200) | 4100 | 850 (600) | 2.000000000 | 2.165000000 |
| constant_large | 6400 | 6650 (6400) | 4100 | 4350 (4100) | 1.560975610 | 1.560975610 |
| constant_small | 6400 | 1050 (800) | 4100 | 850 (600) | 1.333333333 | 1.333333333 |
| phased | 6400 | 3253 (3000) | 4100 | 2228 (1975) | 1.518987342 | 1.518987342 |
| signed_extremes | 6400 | 3405 (3056) | 4100 | 2359 (2010) | 1.520398010 | 1.520398010 |
| sparse_large | 6400 | 1171 (912) | 4100 | 929 (670) | 1.361194030 | 1.361194030 |

With the executable 10-bit Tier-0 control, `boundary_oscillation` instead has
AB=1250 and AT=850 at r=1, total crossover 5/3. At r=1.60 AT=1210 < AB=1250.
This contradicts universal no-storage-benefit statements in both prior PRs.
An 8-bit encoding of constant-small remains a better payload choice than
10 bits; the 10-bit arm is a capacity sensitivity control, not universally best.
The six-trit codec has a smaller full domain than 10 bits and is not an equal
native-capacity comparison. See [MATHEMATICS.md](MATHEMATICS.md).

Ratios here do not measure energy, latency, memory bandwidth or physical ternary
cell cost. `packing_results.json` records actual Python encoder byte lengths and
round trips, with an equal 8-byte count header and no adaptive tags.
