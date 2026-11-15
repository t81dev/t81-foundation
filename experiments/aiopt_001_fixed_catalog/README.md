# T81-AIOPT-001 / M1

Experimental, correctness-only implementation of the [reconstructed v0.1.1 proposal](../../docs/research/aiopt/T81-AIOPT-001-M1-PROTOCOL.md).

Run from the repository root:

```sh
python3 -B experiments/aiopt_001_fixed_catalog/m1.py
python3 -B experiments/aiopt_001_fixed_catalog/m1.py
python3 -B experiments/aiopt_001_fixed_catalog/verify_m1.py
python3 -B -O experiments/aiopt_001_fixed_catalog/verify_m1.py
```

The script has no third-party dependencies. Compare the two complete JSON outputs (including `sha256_results`) for repeatability. It enumerates 81 inputs and 40 patterns, validates 3,240 semantic predicate decisions, performs 162 adapter/oracle filter comparisons, 162 encoding round trips, and fail-closed malformed-input assertions.

The semantic oracle is separately implemented from the byte and SWAR paths within the same file. This is **implementation independence at the algorithm level**, not yet an independent reviewer or independent codebase. Do not infer M1 acceptance solely from this script.

The results JSON is printed on stdout and contains the local Python version and host platform. For review, preserve the exact source commit, two output files, the runner environment, and independent reproduction evidence.

Verification corrections validate the full catalog before payload evaluation,
normalize supported list/tuple rows once, reject unsupported/one-shot objects
with stable ContractError categories, and keep all checks active under Python -O.
The v0.1.1 proposal clarifies container types and error precedence; it does not
change the alphabet, C40, matching rule, encodings or acceptance requirements.

`verify_m1.py` independently derives vectors by base-three ordinal arithmetic,
matching indices by rank, and SWAR decoding by base-four division. It never uses
the subject oracle as the reference. It checks 3240 decisions per implementation
(9720 total), all 256 SWAR payloads, all byte values 3..255 at each position,
invalid types/catalogs/lengths, and deliberate validator mutations. It instruments
actual error-expectation and valid-filter calls. The original 604 malformed
expectations were accurate **as executed calls**, including duplicate SWAR
payload checks, not 604 unique malformed payloads. Counts are now accumulated
from completed expectations; they remain 604.

See [M1_VERIFICATION_REPORT.md](M1_VERIFICATION_REPORT.md) for provenance, preserved
baseline failures, exact executed counts and reproducibility outputs. The review
has prior context from a different earlier experiment; no independent human
acceptance or scientific reproduction is claimed. Earlier 22-record/1890-case
claims are not imported into this reconstructed experiment.

**Status:** ready for an independent maintainer's acceptance decision, subject
to the report. The protocol remains proposed; no M2, benchmarks or AI selectors.
