# EXP-T81-COMP3-01/r1 publication validation

M1–M4 and prospective r1 activation are approved by the maintainer's explicit
publication instruction, bound to the five exact packet hashes in
[ACTIVATION.json](../protocol/ACTIVATION.json). Measurement is not authorized.
The unsigned packet record remains immutable; no production signing credentials,
trust anchor, signed approval, delegation or eligibility were provisioned.

## Integration and preservation

Implementation baseline: `988b18ca9ebdb4a6fb76fe0be30115470e5637f0`. Verified provisional
patch SHA-256: `1123bd8f4d52ce212b69ce796cc6c806d87cfbf11b3bfd4342c8323a2d2889f3`;
report SHA-256: `35f11074821b80ca3b595365dce92944cbb998d29ee11964a147d44ef57929f1`.
Both are retained unchanged under `provisional/`, with original source identities.

PR #637 had advanced to `1a71733cdd266a34a174d13185c78e5b6a07c68e`, replacing the
separated harness with monolithic code and removing its prior validation files.
Integration preserves that commit as the parent and restores the verified repaired
separated harness and historical evidence. No force push or merge commit is needed.
Publication changes beyond the provisional patch are limited to the approved
packet/authority record, approval-status and measurement-gate messages, immutable
identity tests, and this validation evidence. Methodology and thresholds are unchanged.

The #635 original blob `0fbe0b4da1614867bf51b1c3eaf200f82d789a65` is preserved
byte-for-byte in `protocol/ORIGINAL_PR635_PROTOCOL.md`; its SHA-256 is
`990f54a57d7ad34cfcd93c45147e81440ac8f2430eae3271d5f676d6947081f1`.
The existing `docs/research/TERNARY_ADVANTAGE_CANDIDATES.md` is unchanged (SHA-256
`2066d1c9f3f3f1831b7317296e551895881f30016b0a9c85547f8c6d537e4c81`).
All five copied r1 documents match the approved hashes. No frozen history,
T81 runtime, ISA, CanonFS, Axion or unrelated governance files were modified.

## Executed results

| Compiler / suite | PASS | FAIL | SKIP |
|---|---:|---:|---:|
| Homebrew Clang 22.1.1 driver checks | 49 | 0 | 1 |
| Apple Clang 21.0.0 driver checks | 49 | 0 | 1 |
| r1 Python suite, each driver (41 methods) | 40 | 0 | 1 |
| Identity/preservation suite, each driver (5 methods) | 5 | 0 | 0 |

Driver counts include the nested Python commands, so these rows are not additive.
Each driver has one documented unavailable Linux-environment group skip; each
Python suite skips Linux-only native integer wait4/rusage. GCC is unavailable,
not a passing or substituted compiler job. Compared with the provisional 48-check
driver, the additional passing check is the five-method protocol identity suite.
The Python suite retains the final provisional coverage of 40 passing methods
plus one skip, including signed CSV/manifest order rejection.

Both compilers built all eight C++ translation units separately with no LTO under
strict warnings, then built and ran ASan+UBSan configurations. No compiler warnings
or sanitizer findings were observed. Four PCG vectors, 30,000 generated corpus
pairs and 24 pinned first-eight pairs pass independent comparison. All five modes
pass mandatory/policy endpoints and extremes, observable payload and classifier
checks; injected CORRECTNESS_FAILURE and PAYLOAD_PARITY remain distinct.

Statistical tests pass all 11 counterexamples (including 70/100 instructions and
11/10 branch misses), seven exact bootstrap fixtures, the full 100,000-index
vector, exact 15% boundary/binary64-collapse cases, zero/tiny variance and Student-t
landmarks, all 18 isolated regression contrasts and 64 aggregate states. Integer
grammar/overflow/duplicates/missing rows, final exit/signal and CPU/RSS boundaries,
signature tampering/roles/revocation/replay, test trust isolation, complete-CSV
failed-collector and signed CSV-order mismatch cases pass. Test signatures always
remain synthetic diagnostics and cannot qualify measurement evidence.

The original 27-finding reconciliation mapping is retained in the historical
provisional report; current approval status is superseded by the authority record.
Findings 1–3 now have an explicit approved prospective authority chain. Findings
15–17, 20 and 25 retain the resource/supervision/Linux limitations below; no new
claim of full conformance is made. Findings 26–27 remain scope/history qualifications,
without edits to frozen governance or historical protocol prose.

## Commands and assembly

```sh
PYTHONDONTWRITEBYTECODE=1 python3 experiments/cmp3_decision_advantage/validate.py --compiler /opt/homebrew/opt/llvm/bin/clang++ --objdump /opt/homebrew/opt/llvm/bin/llvm-objdump --out /private/tmp/t81-comp3-r1-publication/clang22
PYTHONDONTWRITEBYTECODE=1 python3 experiments/cmp3_decision_advantage/validate.py --compiler /usr/bin/clang++ --objdump /opt/homebrew/opt/llvm/bin/llvm-objdump --out /private/tmp/t81-comp3-r1-publication/apple-clang21
```

Exact expanded commands and versions are in `clang22.log` and `apple-clang21.log`.
Optimized flags: `-std=c++20 -O3 -fomit-frame-pointer -fno-lto -Wall -Wextra
-Wpedantic -Wconversion -Wshadow -Werror`; assembly adds `-S -fverbose-asm`.
Sanitizers append `-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined
-fno-sanitize-recover=all`. Host: macOS 26.5.2/Darwin 25.5.0 ARM64, Python 3.14,
OpenSSL 3.6.2. Apple Clang is `clang-2100.1.1.101`. `/usr/bin/g++` is Apple Clang.

Per-basic-block assembly and object inventories are archived beside the logs,
with generated `.s` files. The actual `.o`/executables remain in the external
validation output directories. Both compilers produce A/C wrapper totals
8/7/8/7/9 for candidate/B1/B2/B3/B4 and policy totals 9/7/10/7/10. Each shared
payload has 3 instructions including ret. A/C candidate and B2 lower identically;
Pattern B remains asymmetric. These are static inventories, never dynamic PMU
estimates. Mach-O linker-hint labels do not create artificial basic blocks.

## Unresolved measurement blockers

**MEASUREMENT BLOCKED — PMU_UNAVAILABLE.** The no-workload infrastructure probe
returns exit 6 and PMU_UNAVAILABLE on this host. No COMP3 experimental workload,
performance measurements, elapsed-time substitute or estimated hardware counters
were collected. Linux GCC/Clang execution, PMU instructions/branch misses, affinity
readbacks, SMT isolation, governor/boost states and restoration, and native Linux
resource accounting remain untested. macOS Clang passes do not replace Linux/GCC.

The production T1–T8 supervisor/verifier remains incomplete and disabled: protected
separate UID/key custody, same-file execution/dependency binding, single-process
restrictions, authoritative final output/exit accounting, atomic capture/terminal
publication, crash-durable protected journals and full pre/post environment
evidence are not implemented end to end. Synthetic cryptographic tests do not
validate those properties. Current Linux controls lack full approved readback,
unsupported/denied distinctions and isolation evidence; PMU-open error classification
needs intended-host review. Incomplete bundles block conservatively rather than
retaining unaffected-pattern diagnostics. No production keys or trust were
provisioned. Separate signed production approval/eligibility and explicit measurement
authorization remain mandatory.

Publication is authorized; measurement eligibility is BLOCKED and measurement is
NOT AUTHORIZED. This report does not establish scientific acceptance or a ternary
advantage. Stop after publication of these validated repairs.

The staged whitespace check passes for executable source, documentation and all
other evidence. The preserved `provisional/PROVISIONAL.patch` alone is excluded:
Git flags its single-space unified-diff context lines as trailing whitespace.
Those bytes are required to preserve the verified patch identity; they were not
rewritten as a formatting fix.
