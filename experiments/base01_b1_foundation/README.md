# BASE-01 B1 engineering foundation — correctness only

This standalone directory implements retained-output correctness checks and
static/fixture observers. It has **no timer, warm-up, pilot, trial, campaign,
benchmark, signed-permit verifier or executable authorization mechanism**.
Unknown CLI modes are rejected before source access or worker launch. Native
`--identity-only` returns a compile-bound fingerprint without running workloads;
`--validate-only` is the only workload-executing entry point.

Codex authored this implementation with prior Lane A context. This is
self-authored correctness evidence, not independent scientific certification.
Jules' R75 review/integration assignment is separate: it was not duplicated.
VM assertions remain **PROVISIONAL** and cannot be promoted by a CLI option or
environment variable. Review completion has not been authoritatively established
for this task; PR #690 was observed open/draft at its referenced head.

References are immutable; no merge is assumed:

| Reference | Pin |
| --- | --- |
| Audited source | `cda9135d36144d4de2771af174e52ac7b30a83c9` |
| Phase A manifest snapshot | `ecd66137bacbce50e605cfc1466ec7d36131ba9e` |
| Manifest SHA-256 | `1367b021f393594e4aab3bc4b52a7b76ca94ee70a49b8f0bda62aea96d46e4ea` |
| Accepted B0 oracle/reference | `91e3a7d7f6082b0397f8d6ef3d7ea8a95f3a929e` |
| B1 design, PR #689 | `64c3f71ff100ea16270f1a70497ac39860c1b772` |
| Provisional semantic reference, PR #690 | `7ef553d221ed668f333ff4429012ee0c9378cd13` |

`phase-a-manifest.json` is an unchanged byte copy of the historical manifest.
The copied `digest`, `canonical` and `verify_codec` functions in `oracle.py` are
verbatim from accepted B0. Its arbitrary-precision positional check is independent
of the native codec. The harness neither imports the old runner nor edits any
Phase A/B0 artifact. Canonical H01–H08, Hanoi/CWEB history and frozen exclusions
are unchanged. This is supporting correctness apparatus, not a new comparative
research result or a claim of native ternary advantage.

## Reproduce on the validated platform

The actual correctness execution was macOS ARM64 / Apple clang 21 / C++23 Release.
Linux execution remains unverified; Windows execution is rejected. The static
ELF64 parser is tested using explicitly synthetic fixtures; that is not a Linux
execution claim. Python uses only the standard library; no dependency install
is needed for this harness.

From this branch's checkout, use separate absolute paths:

```sh
git worktree add --detach /tmp/foundation-source cda9135d36144d4de2771af174e52ac7b30a83c9
cmake -S /tmp/foundation-source -B /tmp/foundation-source-build -G Ninja \
  -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_FUZZ_TESTS=OFF \
  -DT81_ENABLE_TERNARYOS=OFF -DT81_ENABLE_DPE=OFF
cmake --build /tmp/foundation-source-build --target t81_vm_workload_determinism_tiers_test -j 4
cmake -S experiments/base01_b1_foundation -B /tmp/foundation-native -G Ninja \
  -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DAUDITED_SOURCE=/tmp/foundation-source -DAUDITED_BUILD=/tmp/foundation-source-build
cmake --build /tmp/foundation-native
python3 -B experiments/base01_b1_foundation/harness.py --validate-only \
  --source /tmp/foundation-source --source-build /tmp/foundation-source-build \
  --build /tmp/foundation-native --output /tmp/foundation-evidence-new
BASE01_FOUNDATION_NATIVE=/tmp/foundation-native/foundation_check \
  BASE01_FOUNDATION_RAW=/tmp/foundation-evidence-new/native.json.gz \
  python3 -B -m unittest discover -s experiments/base01_b1_foundation -p 'test_*.py' -v
```

Use the same compiler and standard library in both builds; select the appropriate
compiler when eventually reproducing Linux. The audited target's normal build
fetches its pinned Asio dependency. Do not invoke a broad CTest, root benchmark
build, benchmark runner or existing benchmark-containing workflow. This CMake
project is separate from root CMake and links only the six audited correctness
archives. Source/manifest bytes and build configuration are checked before work.

CMake binds the native executable to SHA-256 hashes of its four native source/
build files. The launcher checks `--identity-only` before launching a workload,
rejecting stale binaries, then checks the same identity in its result. Code is
split into non-LTO translation units so encode/decode outputs escape their calls.
This is stale-build detection under trusted code, not executable authenticity
against an adversarial operator replacing the program or compiler.

## What validation executes

- Exactly 1,031 corpus inputs; allocating encode and pre-encoded decode are
  separate stages. All outputs remain available through verification/serialization.
  Native results are checked by the unchanged independent Python oracle.
- Exactly three CanonFS payloads: 4,096 / 65,536 / 1,048,576 bytes, fill 0x42.
  Two repeated writes retain both references; two verified reads retain all bytes.
  Python computes `hashlib.sha3_512(payload).digest()[:32]` independently and checks
  both returned hashes and every read byte. No project hash routine supplies the
  expected value. SHA-256 separately identifies the payload and artifacts.
- The six unchanged historical VM builders run on fresh single-context machines.
  Assertions cover initial R75=0, success, global halt, successful explicit Halt
  trace, no trap/denial, exact terminal PC/R75 and semantic outputs/tags. No new
  R75 model or review is implemented. The reference remains provisional.
- RSS normalization and selector/lifecycle validators use fixtures and actual
  platform constant bindings, **not getrusage samples**. Actual spawned/reaped
  worker PID, parent PID and exit status are recorded without time/resource values.
- Binary/linked-archive stat bytes and SHA-256 are static identity observations.
  Supported ELF64-LE/Mach-O64-LE section/segment metadata preserves memory versus
  file extents. Archives, fat/32-bit/big-endian/unknown formats are explicitly
  unsupported for section parsing; their content identity is still recorded.

The worker uses only `make_in_memory_driver` for storage operations. Its VM
factory is protected by rejecting a CanonFS-root environment variable; the
launcher removes it and sets read verification to 1. Normal filesystem writes
are limited to build/validation evidence, not CanonFS persistent/block workloads.

## Result and failure contract

Successful receipts use `schema: base01.b1-foundation.validation.v1`, status PASS,
mode correctness-only, exact source/manifest/runner/native identities, per-file
hashes, input identities, retained-output bounds, PROVISIONAL VM results, worker
lifecycle, static artifact metadata, platform/compiler and per-workload engineering
classification. Qualification/measurement eligibility is always false;
`measurement_enabled` is false and `performance_samples` is exactly empty.
Resource records explicitly say no collector exists and samples are empty.

Existing output directories are rejected; no historical receipt is overwritten.
Failures exit nonzero with a structured FAIL/REJECTED diagnostic. Partial worker
stdout/stderr and lifecycle records remain for diagnosis, without a success
receipt or readiness claim. Accepted stdout is bounded before JSON loading;
see [retention/bounds](RETENTION_AND_BOUNDS.md) for the distinction between
logical admission limits and hard process/resource enforcement.

The full test command above passed 25 tests, 0 failures, 0 skips. With optional
native/raw environment paths absent, corresponding tests explicitly skip; do not
call such a run full validation. The suite covers denial before any work,
source/manifest tampering, stale native binding, oracle/output corruption,
provisional VM scope, malformed/unsupported object files, units and selector/
lifecycle misuse. Fixture values are synthetic correctness inputs, not measured
CPU/RSS/duration data. No real resource collection API is called.

See [readiness and retained evidence](READINESS.md) and the
[future authorization interface proposal](AUTHORIZATION_INTERFACE.md).
No frozen protocol, root workflow, existing experimental artifact or PR
#687–#690 is modified. No measurement authorization or merge is implied.
