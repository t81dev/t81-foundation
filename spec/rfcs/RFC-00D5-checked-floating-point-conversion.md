# RFC-00D5: Checked Floating-Point Conversion

**Status:** draft — proposed requirements, not binding production behavior  
**Type:** standards-track  
**Created:** 2026-10-22  
**Applies-To:** future checked `T81Float` ↔ IEEE binary64 conversion boundaries  
**Supersedes:** nothing; does not amend ISA v1.9.0 or `t81-data-types.md` today

## 1. Purpose and authority

Specify numerical correctness before another conversion implementation is written.
MUST/SHALL in §§3–8 apply **only to the proposed checked profile if approved**.
They do not certify or alter current `from_double`, `to_double`, constructors,
serialization, arithmetic, VM opcodes, or determinism guarantees. This PR supplies
only design and isolated reference code. Maintainer decisions in §11 precede any
production implementation. The existing integer provenance boundary and v1/v2
historical evidence are unchanged.

Normative context remains `spec/t81-data-types.md §2.3`, freeze governance and
accepted RFCs. That specification describes base-81/BigInt components, prohibits
NaN/infinity as numerical values and requires explicit loss errors. The observed
bounded base-3 implementation differs. This draft proposes an **additive, named
checked profile**, rather than silently declaring the implementation the spec.

## 2. Reconstructed implementation (descriptive, not proposed requirements)

`include/t81/types/T81Float.hpp` stores `T81Int<1+E+M>`: low M trits are a signed
balanced mantissa, next E a directly signed balanced exponent, last a sign trit.
`get_exp`/`set_exp` apply **no additive bias**. `B=(3^E-1)/2` is used as an extreme
exponent, despite comments calling the field biased. The bridge's finite equation
is `value = sign * mantissa * 3^(exponent-(M-1))`.

| Surface | Actual behavior / discrepancy |
| --- | --- |
| Template admission | M≥4, E≥4, M+E+1≤2048; computing `3^E` in int64 is not safe for every syntactically admitted E. Generic template admission is not a verified format range. |
| Normalization | `normalize` moves the leading nonzero trit to M-1, copies/discards trits without full rounding, returns Inf at exponent ≥B, shifts on exponent ≤-B. It does not establish validation of every raw state. |
| Zero | `is_zero`: exponent=0, mantissa=0, irrespective of sign. `zero(false)` prints `-0E0`; `from_double(-0.0)` becomes `+0E0`. Default construction uses sign Z, which also prints as positive zero. |
| Subnormal | `is_subnormal` labels every nonzero mantissa at exponent -B, including a full-width mantissa. Header comments claiming encoded exponent zero are stale. |
| Specials | exponent B and zero mantissa is signed Inf; any nonzero mantissa at B is NaE. Printing NaE erases raw payload/sign distinctions. |
| Raw factories | `from_components` can construct unnormalized/negative mantissas; sign Z is treated as positive, and `set_exp` can discard excess exponent trits. Factory acceptance does not imply canonicality. |
| Input bridge | host log/floor/powl/llround, unsigned scale subtraction, at most 39 extracted trits; platform range and rounding defects are preserved in PR #552's audit. |
| Output bridge | sums mantissa with host long double, forms a separate `powl`, multiplies then casts to double; intermediates can overflow/underflow despite representable final products. Zero loses its sign; Inf/NaE become native Inf/NaN. |
| Text | finite sign + M high-to-low `+0-` trits + `E` + decimal exponent; special strings and short signed zeros. Printer does not validate raw normalization or padding. |
| Binary | `serialize` delegates to T81Int: native-endian uint64 byte count plus packed 2-bit trits. N/Z/P codes are 00/01/10; decoder maps 11 to Z, and deserialize checks length but not every invalid code/padding/state. This is not the proposed canonical wire boundary. |

## 3. Proposed finite representation

The supported initial profile is **4≤M≤72, 4≤E≤9**, integer parameters excluding
booleans. This includes aliases 18/9, 27/9 and 72/9 and small-format proof fixtures.
Other parameters yield `unsupported_format`, regardless of production template
admission. Range extension requires separate approval and corpus expansion.

Let `B=(3^E-1)/2`, `emin=-B`, `emax=B-1`, `U=(3^M-1)/2`,
`L=(3^(M-1)+1)/2`. The finite positive magnitudes are exactly:

- `m * 3^(e-(M-1))`, with `emin<e≤emax` and `L≤m≤U`;
- `m * 3^(emin-(M-1))`, with `1≤m≤U`, at the minimum exponent.

Negative values use a separate negative sign and positive m. Zero has the single
canonical spelling `+0E0`. There are no finite NaN/Inf values in this profile.
Minimum-exponent values retain the implementation's **subnormal-binade** label;
those with m<L have leading zero trits. These sets are disjoint and ordered, so
canonical finite values have unique fields. The gap between U at e and L at e+1
is **two** units of the finer scale, not one. Algorithms MUST account for it.

Smallest positive value is `a=3^(emin-M+1)`; greatest is
`z=U*3^(emax-M+1)`. Every finite binary64 value lies inside the 27/9 range except
zero, which has its own rule. That fact does not imply exact representability.

This chosen finite subset is a **proposal requiring approval**, not a claim that
all legacy raw encodings already obey it. Positive leading trits alone do not
validate raw sign/exponent/padding. No binary memory layout is redefined here.

## 4. Proposed interfaces and results

Two pure numerical operations are specified:

1. exact binary64 bit encoding → checked canonical finite T81 value;
2. validated canonical finite T81 value → exact binary64 bit encoding.

Binary64 uses sign bit 63, exponent bits 62..52 and fraction bits 51..0. The
reference interchange uses exactly 16 lowercase hexadecimal digits, independent
of memory endianness. Malformed hex is an adapter `invalid_encoding` error, not a
possible malformed C++ double object representation on an IEEE host.

The abstract result has exactly `status`, `value`, `error`, `flags`:

| Status | Value | Error | Meaning |
| --- | --- | --- | --- |
| `exact` | canonical text / binary64 hex | null | mathematical input value is represented exactly |
| `rounded` | canonical text / binary64 hex | null | caller explicitly selected nearest-even; exact rational loss was detected |
| `error` | null | named category | no usable output; caller state MUST remain unmodified |

`flags` is empty except for `negative_zero_normalized` on a negative binary64
zero mapped to `+0E0`. This is explicit sign normalization, not silent bit-exact
preservation. Mathematical exactness is distinct from representation identity.
Errors carry no output candidate in this initial API; the older audit's nearest
candidates were diagnostics, not success values.

Proposed C++ shape (illustrative names/types, no declaration added by this PR):

```cpp
// Conversion<T> contains T value, Exactness { Exact, Rounded }, and flags.
// ConversionError is a structured category; no errno/global status dependency.
std::expected<Conversion<T81Float<M,E>>, ConversionError>
checked_from_double(double, ConversionMode = ExactOnly);
std::expected<Conversion<double>, ConversionError>
checked_to_double(const ValidatedFloat<M,E>&, ConversionMode = ExactOnly);
```

A validator/adapter for legacy objects MUST check fields and raw encoding before
constructing `ValidatedFloat`; printing then parsing is insufficient, since it
can erase invalid raw codes and NaE payloads. Its public type shape, treatment of
default sign-Z zero and allocation/resource failure policy require approval.
Legacy binary deserialization is **not** an admitted source in this initial
reference contract. A future C++ double binding must be enabled only for IEEE
binary64 (`sizeof(double)=8`, radix 2, 53 precision bits, exponent range ending
at 1024); reject other host formats at build time rather than reinterpret them.
No byte-codec, padding migration or new storage format is
implicitly authorized. The fully specified reference sources are binary64 bits
and the strict canonical text below.

`noexcept` is compatible with returning an expected/result. The incompatibility
is specifically today's **bare T81Float/double return type plus noexcept**: it
cannot distinguish valid zero from underflow, success from inexact loss, or
multiple explicit error categories without changing observable semantics or
adding an out-of-band channel. Throwing across it terminates; sentinels cannot
encode a valid rounded value *and* its loss status. Exceptions, errno and hidden
thread-local flags are not the proposed checked protocol.

## 5. Proposed conversion rules and error precedence

In order: validate format, validate mode, validate source encoding/classification,
check destination finite range, then test exactness/round. At the first failure
return `unsupported_format`, `unsupported_mode`, `invalid_encoding`,
`noncanonical_encoding`, `nonfinite_input`, `underflow`, `overflow` or `inexact`
as applicable. For a valid finite nonzero source the two range checks are mutually
exclusive. Shape/grammar is checked before numerical conversion. Zero bypasses
nonzero range checks.

| Input | Proposed behavior |
| --- | --- |
| +binary64 zero | exact canonical +0E0, no flags |
| -binary64 zero | exact canonical +0E0, `negative_zero_normalized` flag |
| exact finite value in destination set | exact result under either mode |
| finite in range but unrepresentable, `exact_only` | `inexact` error, value null |
| finite in range but unrepresentable, `nearest_even` | globally nearest finite result, `rounded` status |
| nonzero magnitude below smallest finite nonzero destination | `underflow` error under **both** modes; no zero flush, even if zero would be nearer |
| magnitude above greatest finite destination | `overflow` error under both modes; no saturation/Inf |
| binary64 Inf / quiet or signaling NaN, any sign/payload | `nonfinite_input`, never perform arithmetic on signaling NaN |
| T81 `NaE`, `+Inf`, `-Inf` source spellings | `nonfinite_input`; these legacy sentinels are recognized but not admitted values |
| invalid or noncanonical source | explicit encoding error; no repair or normalization by the converter |

The pre-rounding range policy is intentionally stricter than IEEE flush-to-zero
or overflow-to-infinity thresholds. Near-endpoint values inside range may round
to endpoints. Exact decimal/rational construction of test sources uses reduced
positive denominators and is a **reference helper**, not another public API.

Exact binary64 decomposition: subnormal magnitude `fraction*2^-1074`; normal
magnitude `(2^52+fraction)*2^(exponent_bits-1075)`. No host floating-point operation
is needed. Target search compares integer cross-products of rational distances;
there is only **one rounding**, after normalization-scale selection. No logarithm,
intermediate FPU value, llround, double rounding or host rounding mode is allowed
to determine a proposed checked result.

## 6. Proposed nearest-even rule and alternatives

Choose the globally nearest value over the finite destination set, not a rounded
mantissa at an approximate exponent. Negative rounding is sign-symmetric.
For equidistant T81 candidates, prefer even **integer mantissa** (m mod 2=0),
not a raw bit or last trit. If parity is the same, prefer the **smaller canonical
exponent**. At adjacent exponent intervals, both endpoint mantissas can have the
same parity (both even for even M, both odd for odd M); the extra rule is required
for a total function. It selects the lower magnitude at such ties.

For binary64, prefer an even integer significand at the common precision; the
usual raw finite encoding low bit gives that choice, including carries into the
minimum normal or a new exponent. Binary subnormals have unit 2^-1074. Normals
with floor(log2(x))=k have unit 2^(k-52), with k determined by integer bit lengths
and exact comparison. Quotient/remainder comparisons decide nearest rounding;
q=2^53 carries into the next exponent; a subnormal q=2^52 becomes minimum normal.
Pre-rounding range checks still take precedence.

Exact comparisons for the small M=4 format at unit scale:

| Rational input | proposed nearest-even | nearest-away alternative | toward-zero alternative |
| --- | --- | --- | --- |
| 55/2 | 28 | 28 | 27 |
| 57/2 | 28 | 29 | 28 |
| 41 (neighbors 40,42) | 40 (both mantissas even) | 42 | 40 |
| -41 | -40 | -42 | -40 |

For M=5, 122 lies halfway between 121 at e=4 and 123 at e=5; both mantissas
are odd, so smaller exponent chooses 121. Nearest-away is sign-symmetric but has
different ties; directed/truncating modes have systematic error direction.
The proposed initial modes are **exact_only** and this fully specified
**nearest_even**, not host_default or a blanket promise to support every model.

## 7. Proposed canonical text and validation

The finite wire/test grammar is `+0E0`, or one ASCII sign, exactly M balanced
trits (`+`, `0`, `-`, most significant first), `E`, and shortest signed decimal
exponent (no leading +, leading zeroes or -0). Nonzero m must be positive and
within §3; e must be in [emin,emax]. Leading zero mantissas are permitted only at
emin. `-0E0`, long zero forms, negative mantissas and unnormalized normals yield
`noncanonical_encoding`. Wrong width/characters, whitespace, out-of-range or
reserved finite exponents yield `invalid_encoding`. The reference adapter caps
a decimal exponent token at six characters before integer parsing; longer tokens
are `invalid_encoding` and cannot request enormous powers.

Canonical decoding followed by canonical encoding SHALL preserve this **finite
text**, not host binary bytes or a binary64 round trip. Inexact binary64 bridges
cannot promise `encode(decode(x))=x` through another number system. Existing
binary serialization/identities are not rewritten, and historical text consumers
must not silently be moved to this proposed stricter profile.

## 8. Reference evidence and relationship to PR #552

`experiments/checked_float_contract/` is isolated executable **draft** evidence.
Its static corpus supplies explicit expected values/errors, independent of host
observations. Tests certify rounded expectations against exact adjacent values
and enumerate a complete small finite lattice independently of the search.
They include both directions, exact and rounded results, malformed input,
subnormal/normal boundaries, both parity-ambiguous ties, binary rounding carries,
parameter limits and explicit error precedence.

PR #552's oracle is a useful finite-magnitude nearest-candidate audit, not a full
contract: it has no checked rounding mode or binary64 inverse, loses negative-zero
provenance and accepts non-shortest exponent spellings on decode. Its finite
nearest choices agree with this proposal on the overlap, but its diagnostic
`inexact` + candidate object is not the proposed checked result. It is preserved
unchanged so old evidence is not retroactively reinterpreted.

A passing reference suite proves consistency of this proposal with its exact
examples, not production compliance or cross-platform floating-point determinism.
No floating-point probe is readmitted to integer provenance by this draft.

## 9. Compatibility and impact (no implementation changes)

See proposed [ADR-0005](../../docs/architecture/adr/ADR-0005-checked-float-conversion.md)
for option comparison and [implementation plan](../../experiments/checked_float_contract/PLAN.md).

- Existing `from_double`, `to_double`, scalar/widening constructors, layout,
  serializer and wrappers remain byte/behavior compatible in this design PR.
- Future checked APIs are additive and separately versioned. A legacy wrapper may
  keep its old algorithm and limited guarantees; it must not become an unnoticed
  adapter to corrected rounding, as that can change output and content addresses.
- VM FloatHandle pools contain doubles. FAdd/FSub/FMul/FDiv use direct double
  operations in `vm.cpp`; selected helpers use T81Float<72,9> bridges. A checked
  boundary must not change existing opcode values, results, traps, flags or traces.
  VM adoption needs a separate contract/freeze review, not automatic substitution.
- `T81BigInt`/fraction conversions, widening constructors, numeric formatting,
  `isa/base81_view.cpp`, vectors/matrices/quaternions and tensor helpers call
  existing bridges. Each caller needs an explicit exact/rounded/error policy;
  a new result-returning API cannot be dropped into those expressions silently.
- CanonFS identities and any evidence including converted text/bytes can change
  under corrected rounding. New provenance must name the conversion profile and
  parameters; immutable old objects/fixtures must not be regenerated as if equal.
- PR #536 remains independent and unmodified. Its arithmetic/VM flag changes do
  not define this conversion contract. Re-audit its eventual merge before VM work.

## 10. Bounded acceptance and stopping criteria

Design acceptance requires all corpus results and independent tests to pass,
source/authority distinctions to be explicit, both API strategies to be evaluated,
and every §11 decision to be resolved or plainly pending. It authorizes no
production rollout. Implementation acceptance and stopping thresholds are in the
plan; absence of native proof keeps all floating-point determinism claims closed.

## 11. Maintainer approval required before production work

1. Adopt/alter the finite base-3 subset, direct signed exponent and initial bounds;
   reconcile the base-81/BigInt wording with the frozen representation contract.
2. Approve exact-only default, explicit rounded success, zero-sign normalization,
   pre-rounding range errors, parity tie completion and nonfinite rejection.
3. Choose checked API/type names, validated legacy-object adapter rules (including
   default sign-Z zero and negative-zero factories), allocation failure channel,
   and compatible minor-version/freeze process. `noexcept` is not decided here.
4. Approve strict canonical text as a **new named boundary**; decide separately
   whether binary validation/codec work is necessary. No existing bytes may change
   without the applicable freeze/version/migration process.
5. Approve the caller migration list and profile-recording policy. Any future VM
   trap/semantic change requires its own governed review; existing ISA is frozen.

All five decisions are **pending**, with the preferences above offered for review.
