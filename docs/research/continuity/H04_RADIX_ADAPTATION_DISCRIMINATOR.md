# Selected next experiment: H04-R01 radix × adaptation discriminator

Status: selected, mathematically bounded specification; not implemented or benchmarked. Parent H04's original Hanoi semantics remain unresolved. This is a declared refinement, not a claim that Hanoi's tier labels are precision widths. It is separate from Research Protocol 001.

## Falsifiable question

For a fixed trace of exact bounded integer operations, does a ternary-specific advantage remain after the comparator receives the same ability to select precision? A gain over fixed binary alone is insufficient. The primary first-stage output is a crossover surface under explicit symbol-cost assumptions, not a CPU speedup or a device-efficiency certification.

Use a 2×2 design:

| | Fixed capacity | Adaptive capacity |
| --- | --- | --- |
| Binary | FB | AB |
| Balanced ternary | FT | AT |

Each arm processes exactly the same signed integers and produces the same mathematical outputs. Initial operation is addition only. Reference is independent arbitrary-precision integer arithmetic. Mode classes are explicitly declared signed binary ranges of 8/16/32/64 bits; their balanced-ternary capacities must cover those entire ranges, including the negative extreme. For a signed b-bit range, require `3^k >= 2^b + 1`, not an invented mapping from 81/243/729 to bit counts. Promote or reject overflows according to one shared rule; never silently wrap one arm while widening another.

Use four predeclared trace families: constant small, constant large, phased sizes and alternating sizes. Include exact boundary values and sums at every promotion boundary. The input generator is not a cost-free oracle available only to AT. Both adaptive arms receive identical metadata or pay for identical inspection information. Report mode-selection and transition counts even when the algebraic storage model is evaluated first.

## Smallest meaningful first-stage packet

Do not build a scheduler or VM. Compute an exact ledger over short deterministic traces:

- required binary/ternary symbol slots and equal-capacity padding;
- mode metadata, including a competitive binary bit-packed baseline and the same metadata-compression opportunities for both adaptive arms;
- selector invocations, transitions and input/output/intermediate conversions;
- crossover curves as `C3/C2` and separately named overhead parameters vary.

Primary storage counts are exact integers. Storage/area, operation work, latency and energy are separate quantities. Do not add conversion time to area or treat symbolic operation counts as measured joules. A total physical cost is evaluated only after its coefficients have consistent units and independently justified values; otherwise report the vector and conditional boundary rather than one performance score.

`probes/storage_boundary.py` already supplies a limited H01 sanity check: at M=256 and C3/C2=1.5, ternary costs 9 units versus binary 8 despite 1.5 being below log2(3). It does not implement this four-arm trace model or supply hardware coefficients.

## Controls and decision

- AB is mandatory. Compare AT against AB and report each radix's adaptive/fixed contrast; the interaction of the two factors must not be called a radix advantage by itself.
- No byte-alignment penalty may be imposed on binary while ternary receives free packing. Also report an information-theoretic binary packing bound. If a constant-time/random-access restriction is imposed, impose it on both and charge block decoding where relevant.
- Use equal domain/precision, the same trace and an independent oracle. Include conversion and control work in the ledger. Test the negative control where adaptation is disabled: adaptive and fixed selectors must then agree for each radix.
- Relabeling modes must not change modeled work. Zero-cost transitions are a sensitivity limit, not the main result. Boundary-heavy traces expose conversion and oscillation costs.
- First-stage outcomes are exact/model conditional, so there is no sampling p-value or claim of device measurement. Report the full declared parameter sweep; do not choose a favorable slice after seeing results.

Reject the proposed advantage in the tested scope if it disappears against AB, requires unequal semantics, depends solely on padding/packing restrictions, or exceeds its explicitly declared control/conversion budget. A negative result is publishable. A favorable model region is only a target for later physical or implementation evidence.

If a CPU timing phase is later authorized, first select competitive native fixed-width and variable-precision binary implementations (including optimized binary limbs/GMP where appropriate), inspect generated code and require a correct competitive ternary candidate. Fix measurement boundaries, independent process-level repetitions, uncertainty and stopping rules before confirmatory measurement. This packet makes no “equally optimized” timing claim without that work.

## Next single action and stop

Produce the four-arm exact trace ledger and independently check its domain/range and packing formulas. Stop before implementation if the tier domains, units, oracle, comparator optimization or conversion accounting cannot be specified fairly. Do not repair or benchmark the historical invalid arithmetic as a surrogate. No frozen ISA, VM, Axion, scheduler or Protocol 001 modification is needed.
