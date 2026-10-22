# Native floating-point conversion audit

The PR #551 P1 finding is real. **Production conversion remains unresolved.**
This corrective increment quarantines unsupported conversion observations from
canonical provenance, preserves the failures, and adds an independent exact
reference. It does not change `T81Float`, its public API or any numeric guarantee.
The integer workload, policy gates, trace verification and tamper checks remain.

## Reproduce

No third-party Python libraries, model files or full T81 build are needed for the
audit (Python >=3.10, C++23 compiler; IEEE binary64 `double` required):

```sh
c++ -std=c++23 -O0 -DNDEBUG -DT81_DETERMINISTIC -I include \
  experiments/float_conversion_audit/probe.cpp -o /tmp/t81-float-o0
c++ -std=c++23 -O3 -DNDEBUG -DT81_DETERMINISTIC -I include \
  experiments/float_conversion_audit/probe.cpp -o /tmp/t81-float-o3
python3 experiments/float_conversion_audit/test_oracle.py
python3 experiments/float_conversion_audit/audit.py \
  --probe /tmp/t81-float-o3 --unoptimized-probe /tmp/t81-float-o0 \
  --output /tmp/t81-float-native.json
```

Run the [unchanged bounded workload and adversarial suite](../deterministic_provenance/README.md)
using the new evidence-v2 fixture. The native audit workflow executes these
commands on Linux x86-64/GCC 14 and macOS ARM64/Apple Clang, then builds only the
provenance executable and compares all eight admitted records byte for byte.
Artifacts are uploaded even on failure. A green **audit** means the reference,
exclusion and integer checks passed; the report still labels numerical conversion
**UNRESOLVED**, never a numerical-conformance success.

## Reference profile and contract conflict

`oracle.py` starts with the exact 64-bit input encoding. It decomposes exponent
and significand into a rational with Python integers; it never calls host
`float`, log, pow, rounding or T81 code. Finite encoding values are interpreted as
`sign * integer_mantissa * 3**(exponent - (M-1))`, matching the implementation's
field equation, not its conversion algorithm. For `<27,9>`:

- Mantissa is 27 balanced trits; maximum `(3**27-1)/2`, minimum normalized positive
  mantissa `(3**26+1)/2`; the highest nonzero normal trit is positive.
- Exponent range is -9841..9840; -9841 permits gradual subnormals. Special exponent
  +9841 is reserved. Every finite binary64 input fits this destination **range**;
  range is not the reason `denorm_min`, `min` or `max` should become zero.
- Both input zeros canonicalize to `+0E0`, the existing conversion choice. NaN,
  signaling/payload NaNs and either infinity return an explicit `invalid` status
  in the reference, as required by the normative invalid-input policy.
- Exact encodings return `exact`. Lossy inputs return `inexact`, with a nearest
  finite representation for diagnosis only. Search uses rational distances across
  neighboring exponent intervals, including spacing changes; ties prefer an even
  integer mantissa, then the smaller exponent. This rounding choice is explicit
  in the **audit profile**, not claimed to be a newly approved production rule.
- Inputs below the smallest nonzero subnormal or above the largest finite value
  return `underflow`/`overflow`, never silently zero/infinity. Reduced exponent
  width tests exercise these errors since finite binary64 fits `<27,9>`.
- Invalid hex encodings and unsupported audit formats raise `ValueError`.

`spec/t81-data-types.md §2.3` requires explicit deterministic errors for invalid
states and a `Result[T,E]` for precision loss, yet the current `noexcept`
`from_double(double) -> T81Float` provides neither. It emits NaE/Inf sentinels and
silently rounds. The spec also describes base-81/BigInt components, while this
class implements bounded balanced-trit fields. Normalization comments explicitly
say discarded trits are not properly rounded. There is no unambiguous, complete
existing rounding/range/error contract to patch into this API without a numerical
contract decision. The exact oracle exposes that gap; its diagnostic nearest
candidate is **not** authorization to change public semantics.

Two independent tests check the oracle: exact binary64 decomposition and brute
force enumeration of all finite encodings of a small `<4,4>` format. Coverage
includes binade boundaries, ties, negative symmetry, special inputs, invalid
encodings, destination range errors and exact/inexact distinctions. Matching a
nearest candidate is distinct from satisfying the strict lossless contract.

## Findings and before/after evidence

The production header is byte-for-byte unchanged from merged PR #551. Production
outputs **before and after this corrective PR are identical on each platform**.
The before/after change is the provenance contract: version 1 included unsupported
conversions; version 2 excludes them and rejects v1 explicitly. Original v1
`fixture.json` and `observed-arm64.json` remain preserved under
`experiments/deterministic_provenance/`; active verification uses `fixture-v2.json`.
Every new provenance run retains the actual conversion strings in an explicit
`UNSUPPORTED_NONCANONICAL` diagnostics section, outside record hashes.

The root causes are distinguishable:

1. `EffectiveM` is `size_t`, so `EffectiveM - 1 - exp_unb` performs unsigned
   subtraction. At maximum finite binary64, `exp_unb=647`; the scale becomes
   18446744073709550995 instead of -621. The subsequent power is nonfinite and
   the legacy path returns zero. This is not legitimate destination overflow.
2. For minimum binary64 values, the scale is positive: 703 for `denorm_min`, 670
   for minimum-normal on the tested ARM64 compiler. `powl(3,703)` and
   `powl(3,670)` overflow a binary64 `long double` before multiplication by the
   tiny input. Linux x86-64 extended `long double` can represent them. The
   mathematical scaled mantissa is finite in both cases. Fixing signedness alone
   does not remove this intermediate-range dependency.
3. `log`/`floor` select exponent boundaries and `llround` performs host-precision
   half-away rounding before balanced-trit normalization discards digits. Even
   at `0.5`, with finite intermediates, the production encoding lies farther from
   the exact input than the oracle's nearest candidate. A `powl` workaround alone
   is not a numerical correction. Host log/rounding boundary behavior remains
   another dependency, not proven harmless by the extreme-value reproducer.
4. On the tested native ARM64 toolchain, O0/O3 produce identical canonical strings
   for the corpus. Compiler optimization is not the observed cause there; this
   does not exclude all toolchain effects or floating-point modes.

Exact outputs, source/binary64 identifiers, independent candidates, compiler and
long-double properties are in the native JSON evidence. `represents_exact_input`,
`matches_nearest_candidate` and `lossless_contract_satisfied` are separate fields.
The report does not count matching incorrect platform outputs as numerical success.

## Decision and remaining limits

Use the user-authorized fallback: preserve numerical failures and exclude every
unsupported conversion probe from canonical provenance. No production fix is
claimed. No platform-specific expected values, weakened output/trace/policy
checks, hash-only correctness test or new workload is introduced. Version 2
strictly rejects any reintroduced floating-point record fields, even when rehashed.

Open PR #536 remains unmerged at `3e52b6b5247b578e5f6a861a1b9a971b6e93c7cd`.
Its inspected patches do not touch `T81Float.hpp::from_double`; integer helper,
BigInt-invariant and VM flag changes neither resolve these defects nor block this
quarantine. No files from that PR are overwritten or incorporated.

The next numerical change requires an approved checked-conversion contract,
integer/rational conversion with explicit rounding and range errors, and an audit
of normalization and the inverse `to_double` path (also host `powl` based). Until
then **floating-point cross-platform identity and correctness are not established**.
The integer provenance milestone can advance only within its declared boundary
when the native comparison passes. WASM and Windows remain **NOT TESTED**.
