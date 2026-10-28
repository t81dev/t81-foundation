# H-CONV-01: scoped conversion semantics and falsifier

This is a standalone algorithm study under canonical H02, not a replacement
hypothesis identifier or a continuation of Investigation 08. Source revision:
`0662e0e637f47168e56ca4ddd66e8ee877473b38`; the relevant tensor/fixed/integer
sources match those inspected for diagnostic commit
`a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca`. No VM, CLI, CanonFS, governance,
model or tensor construction participates in execution of this experiment.

## Representation and exact mapping

Target: the **private tensor** `canonical_fixed_from_float_` path in
`include/t81/tensor.hpp`, returning `detail::DFixed = Fixed<192,80>`.
It is not `T81Fixed<I,F>(double)` and not `T81Float<M,E>::from_double`.

Digits are t[i] ∈ {−1,0,+1}, with i=0 the least significant position:

    q = sum(t[i] * 3**i, i=0..191)
    represented value = q * 3**(-80)
    capacity = −(3**192−1)/2 .. +(3**192−1)/2

Packed form uses two binary bits per trit, four trits per byte: N=00, Z=01,
P=10, least significant trit in the least significant two bits. Output is 48
bytes, with zero encoded as 48 bytes of 0x55. Invalid code 11 is not generated
and is not an alternative canonical output. Wider integers are zero-extended
in high trit positions; balanced signed digits do not use binary sign extension.

For finite binary32 input x, the source-implied mathematical target is:

    Q(x) = trunc_toward_zero(x * 3**80).

Decode sign s, exponent field E and fraction field f:

- E=0: magnitude x = f * 2**(−149), including zero when f=0.
- 1≤E≤254: magnitude x = (2**23+f) * 2**(E−150).
- E=255: no fixed value; return nullopt for infinity or NaN, without preserving
  NaN payload/classification in the fixed representation.

Positive and negative zero both map to the same zero. Negative finite values
are converted as positive magnitudes, then negated. Rounding is **toward zero**,
not nearest, nearest-even or floor toward negative infinity. Underflow to zero
is silent; no inexact flag is returned. For all finite inputs:

    abs(Q(x)/3**80 − x) < 3**(−80), with the result toward zero.

Subnormals are not all zero: the minimum binary32 subnormal maps to 0, whereas
the largest positive binary32 subnormal maps to raw integer 1. The grid quantum
is 3**(−80). Integers are exactly representable. A noninteger dyadic rational
cannot terminate on a finite radix-three grid: after reducing its denominator,
a nontrivial power of two cannot divide 3**80. Thus exact input extraction is
distinct from exact input representation; the required semantics quantize.

Every finite binary32 input fits: abs(x)<2**128, and 3**80<2**127, so abs(Q)<2**255,
well inside the 192-trit signed capacity (>2**303). The repeated algorithm's
positive multiplication intermediates after widening stay below 2**383 and the
negative division numerators below 2**278, within the 384-trit capacity (>2**607).
The alternative's denominator is at most 2**149. These conservative bounds avoid
width overflow in the intended integer mathematics; they are not a universal
proof that every implementation of the arithmetic kernels is correct.

## Why the existing loop has this mapping

The existing helper forms q0=m*3**80. For e<0, each Fixed division by Fixed(2)
widens, scales numerator by 3**80, divides by 2*3**80 and truncates toward zero.
For nonnegative raw integers this is floor(q/2). Consequently:

    floor(floor(a/2**j)/2) = floor(a/2**(j+1))

and k repeated divisions produce floor(a/2**k). This proves ideal-algorithm
equivalence to one quotient; it does not silently replace intermediate
rounding with a different final rounding rule. e≥0 uses exact scaling by 2**e
within the capacity bound. Negation is applied after magnitude conversion.

## A / B / C and operation accounting

A is one independent Python Fraction/integer oracle. It unpacks binary32 into
an exactly representable Python binary64 value, calls as_integer_ratio, scales
the exact rational, truncates its magnitude and independently encodes balanced
digits. It calls no extracted or production conversion. Its encoding is checked
by positional decoding and the quantization-cell error bound; explicit anchors
cover signed zero, ±1, ±1/2, minimum and maximum subnormal.

B is a faithful native extraction of the current tensor helper and its integer/
fixed arithmetic. `native.cpp::repeated` retains its finite/zero classification,
bit decomposition, repeated Fixed multiply/divide and final sign application.

C is one direct rational-scaling alternative: build the same scaled mantissa;
for e<0 form denominator 2**(−e) by k wide doublings and perform **one** wide
quotient. All denominator construction is charged. No lookup table or cached
denominator is used. For e≥0 retain B's original multiply branch unchanged.
Both return the same 48-byte packed representation with the same no-overflow
domain and use the same extracted arithmetic and native compiler.

For nonzero finite inputs with k=max(0,−e), source-derived wide-division counts
are B=k and C=1 if k>0, otherwise both zero. C additionally performs k wide
denominator doublings. Zeros and nonfinite inputs perform no division. These
are source-derived high-level operation counts, not measured instructions,
cycles, kernel iterations, energy or a cost model treating every division as
equally expensive. Avoiding repeated wide division is mathematically possible;
whether it lowers measured cost is an empirical question.

## Extraction boundaries, ambiguity and history

The packet retains original `T81Int.hpp`, `dmath_types.hpp` and `tensor.hpp`
snapshots. The local integer extraction removes the two infrastructure includes
and unused std::hash specialization, and replaces two Axion overflow trap calls
with std::overflow_error. Those failure paths are outside the capacity-bounded
test semantics; unexpected overflow stops execution rather than being ignored.
The fixed header changes only its include path. The standalone translation unit
adds standard <istream>, previously provided by an unrelated transitive include.
No project build/runtime/governance dependency remains. This preserves the
arithmetic body, not the exact production binary's optimizer context or trap API.

The specification of **global** conversion semantics is not unified: the public
`T81Fixed<I,F>(double)` constructor describes nearest rounding via llround;
the tensor's private DFixed path truncates. Earlier binary64/T81Float reports
also concern different formats and proposed exact-only/nearest-even contracts.
This study records that discrepancy without imposing a new universal contract
or changing any production surface. The scoped private helper is sufficiently
specified to test; undocumented global rounding policy is not treated as an
excuse to choose nearest rounding for it.

The retained historical Hanoi/CWEB source at
`59205d76db4aca3da7c0bdc521924b8f9bc52cc3` declares digits −1/0/+1 but its shown
addition normalizes to 0/1/2 and its multiplication is componentwise modulo 3.
It supplies no binary32-to-DFixed scaling/rounding contract. Existing continuity
evidence records failures under balanced positional arithmetic. No equivalence
between that historical code and the modern helper is assumed, and Hanoi is
neither rebuilt nor rewritten here.

## Frozen bounded test and measurement plan

One deduplicated corpus includes both signs at the predecessor/boundary/successor
of all finite exponent-field boundaries; explicit zero, integers, fractions,
extremes, subnormals, infinities and NaNs; exact-rational neighbors of grid and
half-grid boundaries; and 128 seeded finite random patterns. Labels may overlap.
The same ordered corpus is used for correctness, warmup and both native timings.
All mismatches must retain bits, expected q/bytes, actual B/C bytes and semantics.
No mismatched comparison is eligible for timing or an equivalent-algorithm claim.

After full oracle agreement, one full-corpus warmup per candidate, followed by
six paired native batches with three BC/three CB orders shuffled with an
independent fixed seed. Measure steady-clock integer nanoseconds in one native
process; conversion and a common full-output checksum are inside each interval,
while file reads, allocation of the input sequence and output serialization are
outside. No tensor construction. Report normalized ns/input and batch dispersion;
no Python/oracle timing is compared with native timing. No LTO or fast-math.

H-CONV-01 is falsified on equivalence by any unexplained mismatch. A performance
support claim additionally requires observed lower cost on this fixed matched
native workload. A finite suite cannot establish universal kernel correctness,
and one host/workload cannot establish hardware or architectural advantage.
The approximate active-work budget is 60 minutes including preservation/report;
collection subprocess timeouts reserve time for that work. Stop after reporting.
