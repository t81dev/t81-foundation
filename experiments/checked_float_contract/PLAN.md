# Checked conversion: compatibility audit and bounded implementation plan

Status: proposal; no production implementation authorized. Baseline is merged
PR #552, main `9a2c12c56a3e7eee02f9bf1041d267f5fbd76136`. PR #536 was inspected
as still open at `3e52b6b5247b578e5f6a861a1b9a971b6e93c7cd`; it is untouched.

## Findings a future implementation must preserve

1. Production fields are balanced base-3 mantissa/direct signed exponent/sign,
   not the specification's base-81/BigInt abstraction. Raw factory/deserializer
   acceptance is wider than the proposed canonical finite subset. In particular,
   default sign-Z zero, explicit negative zero and raw special payloads must not
   silently cross a strict validated boundary.
2. Legacy input conversion has unsigned scale subtraction, host intermediate
   range dependence, finite precision extraction and incomplete rounding. Output
   conversion has its own long-double sum/powl/cast and intermediate-range risk.
   A new input algorithm alone cannot fix the inverse, callers or serialization.
3. The incompatibility is the bare return type's lack of outcome information;
   noexcept does not prohibit expected/result. Throwing would terminate, and
   returning zero/Inf/NaE cannot report a usable rounded number plus explicit loss.
4. “Ties to even” needs a total rule for same-parity base-3 exponent-boundary
   neighbors. Canonical text validation, logical numerical equality and raw byte
   identity are separate contracts. Cross-platform consistency of wrong values is
   not conformance.
5. The prior audit oracle is useful but incomplete: finite nearest-candidate
   expectations, no inverse bridge or exact/rounding mode, no signed-zero flag,
   and a decoder that accepts non-shortest exponents. Do not edit that historical
   oracle or regenerate its corpus as if it had always implemented RFC-00D5.

## Caller impact and migration gates

| Existing surface inspected | Impact of a future checked API | Migration gate |
| --- | --- | --- |
| `T81Float` int/float/double and widening constructors | Implicit convenience paths currently erase loss; widening round-trips through double and can lose information even when target M grows. | Keep signatures/behavior; introduce separately named checked factories. Widening deserves direct field arithmetic in a separate review, not this conversion scope. |
| `T81BigInt::from_float` / `to_float`, fraction bridges (`vm.cpp`, `T81Fraction.hpp`) | Existing callers may depend on sentinel/truncation behavior and 72/9 bridge precision. | Specify exactness/truncation/error at each call site; no blanket textual replacement. |
| `vm.cpp` FloatHandle pool, FAdd/FSub/FMul/FDiv | Stores native double values and performs native double arithmetic in these cases, not universally T81Float arithmetic. Flags/fault behavior also interact with open #536. | Existing ISA state transitions/fault classes/trace bytes cannot be reinterpreted by this draft. Separate freeze review before any integration. |
| `vm.cpp` conversion helpers and selected T81Float<72,9> bridges | New failures could affect control flow, returned values and trace identities. | Checked boundaries must be opt-in outside unchanged opcode contracts first. No new trap mapping invented here. |
| `isa/base81_view.cpp`, language numeric formatting, `std/math.hpp` | Parsing/printing through native double can change visible strings when rounding changes. | New named conversion profile and explicit formatting policy; preserve old fixture outputs. |
| Vector/matrix/quaternion scalar helpers and tensor helpers | Many expression-based callers cannot consume expected values directly. | Inventory policy owners; choose explicit rounded or exact semantics. Do not silently map errors to zero. No inference/performance work in this plan. |
| `T81Int`/`T81Float` serializers and CanonFS consumers | Host-native length, unchecked trit/padding/state acceptance and canonical text are distinct. Corrected values can acquire different content hashes. | No rewrite of bytes, old CanonFS objects or historical evidence. A validator must inspect raw data before lossy printing, or accept only a separately validated representation. |
| Integer provenance v1/v2 and conversion audit | Current v2 excludes all unsupported float probes; v1 observations are historical. | Keep unchanged. Re-admission needs a separately approved conversion profile plus executable numerical/native evidence; a reference-only pass does not qualify. |

The additive checked API is the recommended strategy. A legacy wrapper remains
viable **only with explicitly limited guarantees and preserved behavior**. Routing
old signatures to new results is a numerical compatibility change even if source
code still compiles. Public API additions require the applicable minor-version
and freeze approval; representation/canonicalization or ISA changes may require
the breaking-change process and cannot be smuggled in as a conversion bug fix.

## Proposed implementation sequence, acceptance and stops

| Increment | Measurable acceptance | Stop / do not broaden |
| --- | --- | --- |
| 0. Contract approval | Maintainers resolve all five RFC-00D5 §11 decisions and record selected checked result, canonical subset, limits and rounding/range policies. | Any unresolved semantic choice blocks production code. This design PR stops here. |
| 1. Pure checked input boundary | Exact binary64 bit decomposition; all 104 versioned corpus results match, including errors/flags; no host FPU dependency in numerical decisions; result metadata never hides rounding. | Stop at any oracle disagreement, unchecked integer bound, ambiguous tie or unexplained code/spec difference. Resolve reference/spec first. |
| 2. Pure checked output + validation | Exact rational-to-binary64 quotient/remainder algorithm; subnormal/normal carries, range errors, invalid source handling and strict canonical text round trips pass. All raw inputs admitted by a future object adapter have explicit validation tests. | Stop if validation requires changing legacy binary layout or silently repairing raw sign/payload/padding. Propose separate governed work. |
| 3. Independent verification | Differential exact comparisons against both neighboring outputs and complete small-format enumeration; deterministic-seed large-profile corpus spanning all binary64 exponent fields, precision edges and sign classes; sanitizer checks appropriate to chosen integer backend. | Any differing value/status/error/flag is a correctness failure. Matching hashes, approximate tolerances or platform agreement cannot waive it. |
| 4. Native evidence | Same approved version/profile/corpus on native Linux x86-64 and macOS ARM64, GCC/Clang where supported; compare actual bits/text and structured outcomes. Mark unavailable Windows/WASM NOT TESTED. | No generalized floating-point determinism claim from reference tests or one host. No emulation performance substitution. |
| 5. Explicit caller pilot | One host intake/inspection caller opts in with exact/rounded/error policy; legacy API/serialization/opcode regression artifacts remain unchanged; migration docs name identity changes. | Stop if a VM opcode/fault, public layout, old fixture or provenance identity must change. Obtain separate approval rather than widening this pilot. |

For increment 3, a recommended initial minimum is 100,000 deterministic input
encodings including every sign/exponent class and targeted half/carry values;
that future number is a coverage target, **not evidence produced by this PR**.
Resource ceilings must follow a proof of intermediate integer bounds for the
admitted M/E range; do not invent numerical success on allocation failure. Numeric
error precedence and resource failure mapping must remain distinguishable.

## Current design verification and remaining approvals

`test_reference.py` checks the static expected corpus, independent exact neighbor
certificates, complete small-format value enumeration against representative
sources, inverse binary64 rounding carries, invalid/noncanonical encodings,
error precedence, zero flags and rounding alternatives. These are **proposal
conformance tests**, not tests that production already satisfies this contract.
The existing audit/provenance data and production headers are unmodified.

Pending maintainer approval: representation subset/bounds and spec reconciliation;
checked exact/rounded/error and zero/special/range policies; the completed radix-3
tie rule; API/result/validated-object/resource policy and version/freeze process;
canonical text/legacy validation boundary; and the caller/provenance migration
sequence. This is the final decision boundary before production implementation.
