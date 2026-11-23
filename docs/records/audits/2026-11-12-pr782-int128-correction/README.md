# PR #782: bounded int128 backend correction

Reviewed predecessor: `33064f9d9d1a47b0df7702a167a4110b473db263`.
PR base: `88300f14c1da1066ad6db25d31be0e8e912b3ef0`.

The user confirmed that no other Codex session was editing this PR. The handoff
and preserved review findings were posted before advancing the branch:
https://github.com/t81dev/t81-foundation/pull/782#issuecomment-6838574586.
Implementation and the earlier review are from the same assistant/session;
these passing tests are regression evidence, not independent scientific validation.

## Problem and correction

The predecessor selects the MSVC intrinsic implementation for clang-cl solely
from `_MSC_VER && _WIN64`. Its Windows clang-cl job fails while compiling core
sources because `_udiv128` is undeclared at `T81Int128.hpp:72,91`:
https://github.com/t81dev/t81-foundation/actions/runs/43321406300/job/131546781092.
This is a concrete compilation failure, not an assumed match to main's failures.

`cmake/Int128.cmake` now compiles **and links an executable** using the actual
selected compiler, C++ standard, Release configuration, and warning policy.
Volatile operands preserve signed `__int128` multiplication, division and
remainder through optimization. Syntax support alone does not select native
arithmetic. The complete compiler/link output is saved as
`build/native-int128-probe.log` and uploaded with each build job's artifacts.

When that probe fails, the backend uses two 64-bit words, 32-bit partial products
and restoring division. There are no `_udiv128` calls or 64-bit intermediate
fallbacks. Division/remainder accept full-width divisors and quotients, truncate
toward zero, and give the remainder the dividend's sign. Zero divisors and
`INT128_MIN / -1` (including the corresponding remainder operation) throw
explicit exceptions. Add/subtract/multiply retain the low 128 bits; callers must
keep signed mathematical results representable. This is not a new checked
arithmetic API and establishes no universal arithmetic claim.

The same Release-active test source is built for the selected backend and for
the forced portable backend. Seventeen Python-integer golden divmod vectors and
four product vectors cover all signs, quotients/divisors beyond 64 bits,
`INT64_MIN`, `INT128_MIN`, and `INT128_MAX`. On hosts whose native probe passes,
the forced portable test also compares 1,000 seeded cases to native arithmetic.
The portable-only exception tests do not evaluate native undefined operations.

## Executed local verification

Environment: macOS ARM64, Apple clang 21.0.0, CMake 4.3.0. This host cannot execute
MSVC or Windows clang-cl binaries. Native-probe results from this host do not
establish Windows compiler/linker capability.

From the isolated checkout:

```sh
cmake -S . -B build-review -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TUI=OFF \
  -DFETCHCONTENT_SOURCE_DIR_GOOGLEBENCHMARK=/Users/t81dev/Code/t81-foundation/build/_deps/googlebenchmark-src \
  -DFETCHCONTENT_SOURCE_DIR_ASIO=/Users/t81dev/Code/t81-foundation/build/_deps/asio-src
cmake --build build-review --target t81_int128_test t81_int128_portable_test \
  t81_bigint_properties_test t81_bigint_division_edge_properties_test \
  fuzz_bigint_libfuzzer t81_fraction_properties_test -j4
ctest --test-dir build-review --output-on-failure \
  -R '^(t81_int128(_portable)?_test|t81_bigint_properties_test|t81_bigint_division_edge_properties_test|fuzz_bigint_libfuzzer|t81_fraction_properties_test)$'
```

All six tests passed. The same configure/build/test commands were executed with
`-B build-portable -DCMAKE_CXX_FLAGS=-DT81_FORCE_PORTABLE_INT128=1` to compile the
core and arithmetic consumers with the portable backend; all six tests passed.
This forced run tests fallback behavior, not a failed native probe.

Standalone selected/native and forced portable tests passed with
`-std=c++23 -O2 -DNDEBUG -fsanitize=undefined -fno-sanitize-recover=all -I include`.
The forced portable test also passed with
`-O1 -g -DNDEBUG -DT81_FORCE_PORTABLE_INT128=1 -fsanitize=address,undefined
-fno-sanitize-recover=all`, using `ASAN_OPTIONS=detect_leaks=0`.
A temporary test copy with one deliberately wrong expected value exited 1
under `-O2 -DNDEBUG`, proving that the checks execute in Release.
Retained logs accompany this document.

## Preserved macOS baseline comparison

The retained `LastTest.log` and `LastTestsFailed.log` files were copied without
rerunning or overwriting them. `macos-baseline-comparison.json` records the exact
source revisions, clean worktree status, and SHA-256 hashes at collection.
Both the unchanged main checkout (`88300f14...`) and reviewed checkout
(`33064f9...`) report:

- `vm_ffi_bridge_test`: `FFILoadError` during the binary FFI call. Its loader
  failure's root cause is not independently established here.
- `t81_cli_contract_test_runner`: the `fs::exists(safetensors_fixture)` check
  fails at `cli_contract_test.cpp:549`.

These remain separate failures. Reproduction on main supports non-regression
for those observations; it does not establish a common environmental cause or
repair them.

## Open findings and acceptance boundary

The predecessor's missing focused Windows subprocess/Unicode tests remain an
open review finding: paths with spaces, redirection/nonzero exits, empty
arguments and malformed UTF-16 require dedicated execution coverage. The two
pre-existing duplicate-registration `assert` checks also compile out in Release.
Neither issue is weakened or silently claimed fixed by this backend correction.

**Windows baseline repaired: NOT ESTABLISHED.** Fresh complete Windows MSVC and
clang-cl build/test jobs at the corrected head are required. The PR discussion
will retain exact job links, actual capability outcomes and failures; an
in-progress, failed, or skipped job does not satisfy that requirement.

No changes to #779/#780, benchmark campaigns, check bypasses, merges, research
acceptance, or SCG-08 gate advancement are part of this correction.
