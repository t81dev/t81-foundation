> **PROVISIONAL — preserved source comparison, not acceptance or closure authorization.**

# Exact files, dependencies, overlap and review evidence

All source links are pinned to heads, not mutable branches. Own delta is merge-base of actual PR base and head; main delta is merge-base of current main and head. I/D/A/X = identical to current main / different / absent on main / deleted by head, for paths in main delta. Direct head-versus-main tree differences also include later main changes; they are not automatically a proposed rollback.

## PR 782: fix(windows): harden CLI/FFI paths and select a supported int128 backend

- Head `125a1897b7e7e068ef6d576d97c487295c7203a7`; actual base `88300f14c1da1066ad6db25d31be0e8e912b3ef0` (`main`); main merge-base `88300f14c1da1066ad6db25d31be0e8e912b3ef0`.
- Purpose: Windows portability, safe capture and compiler-selected int128. Decision: **REPAIR**.
- Dependency assessment: 782 active; overlaps 750 in CLI driver/main and CMake. Earlier arithmetic/capture authored by this same Codex session.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [779, 780].
- Evidence/limits: Changed twice during inventory; current 125a189 fixes pipeline diagnostic expectation. PR body still describes cbb324ad as current. New enum-path regression unconditionally removes a fixed temp directory (lines 85-89); review scratch ownership. Fresh Windows/CodeQL results required; earlier cbb results are historical.
- Blob counts: {'different': 11, 'absent_main': 22}. Merge simulation exit 0; tree `028f0e2d71a2ccf9f8377e210425b96c80b02c52`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 14 pass, 1 fail, 15 pending, 3 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...125a1897b7e7e068ef6d576d97c487295c7203a7).

Own changed files:

- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/.github/workflows/ci.yml))
- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/CMakeLists.txt))
- `A cmake/Int128.cmake` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/cmake/Int128.cmake))
- `A cmake/probes/native_int128.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/cmake/probes/native_int128.cpp))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/README.md))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/ctest-native.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/ctest-native.log))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/ctest-portable.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/ctest-portable.log))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/local-native-int128-probe.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/local-native-int128-probe.log))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/macos-baseline-comparison.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/macos-baseline-comparison.json))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/portable-sanitized.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/portable-sanitized.log))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/release-active-negative-check.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/release-active-negative-check.log))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/reviewed-head-LastTest.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/reviewed-head-LastTest.log))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/reviewed-head-LastTestsFailed.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/reviewed-head-LastTestsFailed.log))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/unchanged-main-LastTest.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/unchanged-main-LastTest.log))
- `A docs/records/audits/2026-11-12-pr782-int128-correction/unchanged-main-LastTestsFailed.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-12-pr782-int128-correction/unchanged-main-LastTestsFailed.log))
- `A docs/records/audits/2026-11-13-pr782-secure-capture/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-13-pr782-secure-capture/README.md))
- `A docs/records/audits/2026-11-13-pr782-secure-capture/ctest-targeted.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-13-pr782-secure-capture/ctest-targeted.log))
- `A docs/records/audits/2026-11-13-pr782-secure-capture/release.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-13-pr782-secure-capture/release.log))
- `A docs/records/audits/2026-11-13-pr782-secure-capture/sanitized.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/docs/records/audits/2026-11-13-pr782-secure-capture/sanitized.log))
- `M include/t81/types/T81Int128.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/include/t81/types/T81Int128.hpp))
- `M src/tools/CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/src/tools/CMakeLists.txt))
- `M tests/cpp/cli_pipeline_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tests/cpp/cli_pipeline_test.cpp))
- `M tests/cpp/cli_repro_hash_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tests/cpp/cli_repro_hash_test.cpp))
- `M tests/cpp/e2e_enum_metadata_determinism_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tests/cpp/e2e_enum_metadata_determinism_test.cpp))
- `M tests/cpp/ffi_bridge_testlib.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tests/cpp/ffi_bridge_testlib.cpp))
- `A tests/cpp/process_capture_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tests/cpp/process_capture_test.cpp))
- `A tests/cpp/t81_int128_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tests/cpp/t81_int128_test.cpp))
- `M tests/cpp/vm_ffi_bridge_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tests/cpp/vm_ffi_bridge_test.cpp))
- `M tools/cli/core/driver.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tools/cli/core/driver.cpp))
- `M tools/cli/core/main.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tools/cli/core/main.cpp))
- `A tools/cli/core/process_capture.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tools/cli/core/process_capture.cpp))
- `A tools/cli/core/process_capture.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tools/cli/core/process_capture.hpp))
- `A tools/cli/core/process_capture_detail.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/125a1897b7e7e068ef6d576d97c487295c7203a7/tools/cli/core/process_capture_detail.hpp))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `762e28e050c23538dd27a738cc8f0953728be70e`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `b268d028452a2daa4196c01744527782e553bb05`.
- `A cmake/Int128.cmake` — absent_main; main blob `None`; head blob `8b616256604d82a04f21fb8da2bbbe21e85d0f3e`.
- `A cmake/probes/native_int128.cpp` — absent_main; main blob `None`; head blob `1a360397f4efb34bc7df6dcd9b486cec4772ff7b`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/README.md` — absent_main; main blob `None`; head blob `119d5fe666898b741e02aa91b6a492a6bd17da01`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/ctest-native.log` — absent_main; main blob `None`; head blob `b8017bcc93aba7a70de7f4661e817a4c0e9c60ca`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/ctest-portable.log` — absent_main; main blob `None`; head blob `3c30a65aa6b5e0da0880bb2f15ddbd37d5ef5745`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/local-native-int128-probe.log` — absent_main; main blob `None`; head blob `d2c8d07ed179817be90405e3b96d53ba75a38c4b`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/macos-baseline-comparison.json` — absent_main; main blob `None`; head blob `b79258168a1db99a5213c18b9d2300b12f4dc924`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/portable-sanitized.log` — absent_main; main blob `None`; head blob `3bcd740b3b2e5ff0f54f27b80dcb0b54ccf694ff`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/release-active-negative-check.log` — absent_main; main blob `None`; head blob `b98bb8a841d39e7d19420cda89abfe3b2a25abe1`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/reviewed-head-LastTest.log` — absent_main; main blob `None`; head blob `ac8be29b7e47b43f17b49ab291aea9b9b5088dba`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/reviewed-head-LastTestsFailed.log` — absent_main; main blob `None`; head blob `cced7814d188008d098fcc22b5a65eb7ede5e350`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/unchanged-main-LastTest.log` — absent_main; main blob `None`; head blob `1c363ecfc82faf6fb1f8830f8b5a9630226542e1`.
- `A docs/records/audits/2026-11-12-pr782-int128-correction/unchanged-main-LastTestsFailed.log` — absent_main; main blob `None`; head blob `bbaa7c136c164ad22271c3697e83ba64078ee524`.
- `A docs/records/audits/2026-11-13-pr782-secure-capture/README.md` — absent_main; main blob `None`; head blob `4cd45737cd09a68d356ece7d79fb60129f790745`.
- `A docs/records/audits/2026-11-13-pr782-secure-capture/ctest-targeted.log` — absent_main; main blob `None`; head blob `0ae8651eac3fe8432bddc19fd7b5e52298ab06e7`.
- `A docs/records/audits/2026-11-13-pr782-secure-capture/release.log` — absent_main; main blob `None`; head blob `13f77cd783ea616ff2f20f44e3c31e2d739917d1`.
- `A docs/records/audits/2026-11-13-pr782-secure-capture/sanitized.log` — absent_main; main blob `None`; head blob `13f77cd783ea616ff2f20f44e3c31e2d739917d1`.
- `M include/t81/types/T81Int128.hpp` — different; main blob `25ce7982e742e21a0f229c245206cc9b4b737ba2`; head blob `0d173c2b501c3ec0cba7591db5a685c2f6ccf9ce`.
- `M src/tools/CMakeLists.txt` — different; main blob `3f00405267bc423e162ef6e66e5184f8da9b660a`; head blob `f23543ff2d1dd80c02b715e7452d61d6f29262bd`.
- `M tests/cpp/cli_pipeline_test.cpp` — different; main blob `85fa2aec81c75e0c617f512ed2e7b6019f609f14`; head blob `8c7f1d1b6e27b955527e2fa0f3610a1eb870f396`.
- `M tests/cpp/cli_repro_hash_test.cpp` — different; main blob `13dd72ffe606540a20f11c8a28e0cbe75459c69b`; head blob `6bd9e412b9e6b04f88ed06e36aee02c782a5904d`.
- `M tests/cpp/e2e_enum_metadata_determinism_test.cpp` — different; main blob `3f895a56e16a98c31c19320867b2ee0e5edaf879`; head blob `a4c7c4c2af2fab86634afff994c757f9c609e231`.
- `M tests/cpp/ffi_bridge_testlib.cpp` — different; main blob `bd07a5f1d639eb060e078ceea924ba6ccd95ee9b`; head blob `750c30d793a53e518ef1b8d74ab8702ba1557148`.
- `A tests/cpp/process_capture_test.cpp` — absent_main; main blob `None`; head blob `07ef8e1cc30f65b141d6f434a84fae717417afc0`.
- `A tests/cpp/t81_int128_test.cpp` — absent_main; main blob `None`; head blob `9dfe63559d20fa9b19c20767a33e478d94568697`.
- `M tests/cpp/vm_ffi_bridge_test.cpp` — different; main blob `b5207eab0468fa2b24baf8935e5a7e5d9203c73e`; head blob `7aad0ba43d7b0bf7e39694f8d0103035aaf31850`.
- `M tools/cli/core/driver.cpp` — different; main blob `2df57428cfb0d840d6e43d204b72bdeefe18bdfc`; head blob `e0d3f8a923b4cdc379a567c3ac5216c5ff022ae9`.
- `M tools/cli/core/main.cpp` — different; main blob `958dba8a20105a59adecd775bb6b642babedb282`; head blob `23a61abe4b7f5abd4fd44810de820a9cdf91d4dd`.
- `A tools/cli/core/process_capture.cpp` — absent_main; main blob `None`; head blob `2ce2159ddf3824867d8545a5c61136a2b60302cc`.
- `A tools/cli/core/process_capture.hpp` — absent_main; main blob `None`; head blob `0e348cd2a8f65d31880023c3f8adfc8e4e9ca5d4`.
- `A tools/cli/core/process_capture_detail.hpp` — absent_main; main blob `None`; head blob `5b58a5da0f1c463edd5476f7e429307ef650b52e`.

Overlaps with other PR own deltas:

- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`, `tools/cli/core/driver.cpp`, `tools/cli/core/main.cpp`
- #747: `.github/workflows/ci.yml`, `CMakeLists.txt`, `tools/cli/core/main.cpp`
- #649: `.github/workflows/ci.yml`, `CMakeLists.txt`
- #603: `CMakeLists.txt`, `tools/cli/core/main.cpp`
- #577: `.github/workflows/ci.yml`
- #576: `.github/workflows/ci.yml`
- #575: `.github/workflows/ci.yml`
- #574: `.github/workflows/ci.yml`
- #556: `CMakeLists.txt`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #525: `.github/workflows/ci.yml`
- #514: `.github/workflows/ci.yml`
- #513: `.github/workflows/ci.yml`
- #508: `CMakeLists.txt`
- #504: `.github/workflows/ci.yml`, `include/t81/types/T81Int128.hpp`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131867735771): in_progress / None.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131867735744): in_progress / None.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131867735717): in_progress / None.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131867735705): in_progress / None.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131867735702): in_progress / None.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131867735679): in_progress / None.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131867735610): in_progress / None.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131865752930): in_progress / None.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131865752928): in_progress / None.
- [gate / determinism slice / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131865752908): in_progress / None.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131865752868): in_progress / None.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/43419383525/job/131865752838): in_progress / None.
- [Analyze (cpp)](https://github.com/t81dev/t81-foundation/actions/runs/43419383499/job/131865251842): in_progress / None.
- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/43419383430/job/131865251664): in_progress / None.
- [test-ai-experiments](https://github.com/t81dev/t81-foundation/actions/runs/43419383465/job/131865251427): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/43419383430/job/131865251322): in_progress / None.

## PR 780: bench(core): establish BASE-PERF-00 correctness-verified Cell throughput baseline

- Head `a31b862e6bbdc7c136a74c9117aa085a13b4da2c`; actual base `d3a6cf55517a3098d7bae91b07c72694bc3472da` (`main`); main merge-base `d3a6cf55517a3098d7bae91b07c72694bc3472da`.
- Purpose: Bounded Cell CPU-time throughput baseline. Decision: **REVIEW NEXT**.
- Dependency assessment: No changed-path overlap with 779; engineering integration depends on Windows baseline 782, not SCG-08 advancement.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Prior session acceptance is scoped to bounded multiplication, accepted 352 known-defect exceptions and author-reported original provenance. Formatting-only accepted head retained; original-run authentication remains absent. No new performance campaign.
- Blob counts: {'different': 4, 'absent_main': 3}. Merge simulation exit 0; tree `0c7d24efbe203e7d36045109c892542f71dd67e2`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 29 pass, 3 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...a31b862e6bbdc7c136a74c9117aa085a13b4da2c).

Own changed files:

- `M benchmarks/runner/arith_throughput.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a31b862e6bbdc7c136a74c9117aa085a13b4da2c/benchmarks/runner/arith_throughput.cpp))
- `M benchmarks/runner/benchmark_runner.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a31b862e6bbdc7c136a74c9117aa085a13b4da2c/benchmarks/runner/benchmark_runner.cpp))
- `M benchmarks/runner/roundtrip_accuracy.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a31b862e6bbdc7c136a74c9117aa085a13b4da2c/benchmarks/runner/roundtrip_accuracy.cpp))
- `A docs/benchmarks/BASE_PERF_00_CELL_THROUGHPUT_BASELINE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a31b862e6bbdc7c136a74c9117aa085a13b4da2c/docs/benchmarks/BASE_PERF_00_CELL_THROUGHPUT_BASELINE.md))
- `A docs/benchmarks/base_perf_00_cell_throughput_baseline.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a31b862e6bbdc7c136a74c9117aa085a13b4da2c/docs/benchmarks/base_perf_00_cell_throughput_baseline.json))
- `A docs/benchmarks/base_perf_00_cell_throughput_raw.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a31b862e6bbdc7c136a74c9117aa085a13b4da2c/docs/benchmarks/base_perf_00_cell_throughput_raw.json))
- `M tests/determinism/test_primitives.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a31b862e6bbdc7c136a74c9117aa085a13b4da2c/tests/determinism/test_primitives.cpp))

All changed files versus main merge-base (including parents):

- `M benchmarks/runner/arith_throughput.cpp` — different; main blob `2add2071905ef0d4c70ea37e760842849c13191f`; head blob `8e08408b45b4631040d2217a1dc530d0c1bbd13d`.
- `M benchmarks/runner/benchmark_runner.cpp` — different; main blob `856f8852bd664c381c8a2bb1440b1e056ba4bdcf`; head blob `c91f191c0f65cd2c4f70c3afb57e7f1c77fff54d`.
- `M benchmarks/runner/roundtrip_accuracy.cpp` — different; main blob `2b00b2efebefb3a1fbb2f0c6994b5526fd4eb03f`; head blob `891555093b026f5ebccf59f93770c41435a26020`.
- `A docs/benchmarks/BASE_PERF_00_CELL_THROUGHPUT_BASELINE.md` — absent_main; main blob `None`; head blob `bd4d0772f36f9791053dba12d0f7febf0f542f47`.
- `A docs/benchmarks/base_perf_00_cell_throughput_baseline.json` — absent_main; main blob `None`; head blob `38b92f6cbdfb6c322e6f6dd30192c80d63bbe307`.
- `A docs/benchmarks/base_perf_00_cell_throughput_raw.json` — absent_main; main blob `None`; head blob `41c73a2b5a1e45ec4528253a2db1e24a13300573`.
- `M tests/determinism/test_primitives.cpp` — different; main blob `4a583951cf938f0028a3ba806d7122b85e793f7c`; head blob `66e08dd25c24ac34dd00f7241bc8464c03e3cdbd`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/43317708062/job/131547466531): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/43317708062/job/131531750016): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/43317708062/job/131531749978): completed / failure.

## PR 779: rfc(00e1),scg08: resolve independent review scientific contract findings

- Head `4d5f3586c04a69c520e613166291c0974877628d`; actual base `d3a6cf55517a3098d7bae91b07c72694bc3472da` (`main`); main merge-base `d3a6cf55517a3098d7bae91b07c72694bc3472da`.
- Purpose: RFC-00E1 oracle, conversion and product-bound corrections. Decision: **REVIEW NEXT**.
- Dependency assessment: Five shared files with 778; choose canonical correction before integrating either; no path overlap with 780.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Retain prior exact-head review/handoff; original formal review targets older 5bfc head. Current 4d5 correction addresses malformed-tryte order, OutLimbs+2 and signed bounds. This inventory does not rerun them or independently ratify the RFC.
- Blob counts: {'different': 5}. Merge simulation exit 0; tree `3a81c308f0b8172214b7753efdede48586665146`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 29 pass, 3 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...4d5f3586c04a69c520e613166291c0974877628d).

Own changed files:

- `M experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/4d5f3586c04a69c520e613166291c0974877628d/experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md))
- `M experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/4d5f3586c04a69c520e613166291c0974877628d/experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp))
- `M experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/4d5f3586c04a69c520e613166291c0974877628d/experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp))
- `M experiments/scg08_arithmetic_work/tier2_work_harness.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/4d5f3586c04a69c520e613166291c0974877628d/experiments/scg08_arithmetic_work/tier2_work_harness.hpp))
- `M spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/4d5f3586c04a69c520e613166291c0974877628d/spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md))

All changed files versus main merge-base (including parents):

- `M experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` — different; main blob `0671c097a9c73b8928cda57e689d37cf4669b68f`; head blob `64777c458739d6804d9c36310a48bdc2c37e192b`.
- `M experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp` — different; main blob `3f3ed8d8db561fa5e6d9c31c5822ad2b04a7d361`; head blob `95cf533f52e3095a77ec93f3aaa62a1456d43836`.
- `M experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp` — different; main blob `0d9e7128642faab057544c14e8dd8276ccba3f88`; head blob `4657d2cbc87df538a0f907f79f7586fd01f76a31`.
- `M experiments/scg08_arithmetic_work/tier2_work_harness.hpp` — different; main blob `610999bdc334964922c8f3ae9f66f93be7c6944f`; head blob `5321b9ba3274f00e392248f1da5a30040db58277`.
- `M spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md` — different; main blob `82bbcabab68ef0bdecd8b17e295b7bf9f8769c9a`; head blob `c883372f2cbb41783bddd8a0c1a25f9003c3f087`.

Overlaps with other PR own deltas:

- #778: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`, `experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp`, `experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp`, `experiments/scg08_arithmetic_work/tier2_work_harness.hpp`, `spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md`
- #766: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`
- #760: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`, `experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp`, `experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp`, `experiments/scg08_arithmetic_work/tier2_work_harness.hpp`

Review submissions:

- t81dev / COMMENTED / reviewed `5bfc0e2b7f531c4cdeca53ed9e1172f551683da6` / [review](https://github.com/t81dev/t81-foundation/pull/779#pullrequestreview-5873546097).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/43211539541/job/131176564479): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/43211539541/job/131162202863): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/43211539541/job/131162202806): completed / failure.

## PR 778: Correct RFC-00E1 oracle validation and arithmetic accounting contracts

- Head `56ad0d57b90487d153b35ccfdeb134fcc858adf7`; actual base `d3a6cf55517a3098d7bae91b07c72694bc3472da` (`main`); main merge-base `d3a6cf55517a3098d7bae91b07c72694bc3472da`.
- Purpose: Alternative scientific-contract correction with retained rejection evidence. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Five shared changed files with 779; neither head is ancestor of the other.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Not a duplicate of 779: 8 harness/contract paths differ between heads, with substantial executable and contract differences. Preserve unique audit logs and rejection-test machinery before choosing a canonical patch.
- Blob counts: {'absent_main': 10, 'different': 6}. Merge simulation exit 0; tree `f4a31aacf170870c69eaf37814cdd67d13b3378c`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 28 pass, 3 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...56ad0d57b90487d153b35ccfdeb134fcc858adf7).

Own changed files:

- `A docs/records/audits/2026-11-11-rfc00e1-corrections/LastTest.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/docs/records/audits/2026-11-11-rfc00e1-corrections/LastTest.log))
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/build.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/docs/records/audits/2026-11-11-rfc00e1-corrections/build.log))
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/configure-cached.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/docs/records/audits/2026-11-11-rfc00e1-corrections/configure-cached.log))
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/configure-default-interrupted.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/docs/records/audits/2026-11-11-rfc00e1-corrections/configure-default-interrupted.log))
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/configure-dns-failure.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/docs/records/audits/2026-11-11-rfc00e1-corrections/configure-dns-failure.log))
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/ctest.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/docs/records/audits/2026-11-11-rfc00e1-corrections/ctest.log))
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/development-183-trials.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/docs/records/audits/2026-11-11-rfc00e1-corrections/development-183-trials.log))
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/docs/records/audits/2026-11-11-rfc00e1-corrections/manifest.json))
- `M experiments/scg08_arithmetic_work/CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/experiments/scg08_arithmetic_work/CMakeLists.txt))
- `A experiments/scg08_arithmetic_work/CONTRACT_CORRECTIONS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/experiments/scg08_arithmetic_work/CONTRACT_CORRECTIONS.md))
- `M experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md))
- `M experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp))
- `M experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp))
- `A experiments/scg08_arithmetic_work/test_result_rejection.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/experiments/scg08_arithmetic_work/test_result_rejection.py))
- `M experiments/scg08_arithmetic_work/tier2_work_harness.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/experiments/scg08_arithmetic_work/tier2_work_harness.hpp))
- `M spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/56ad0d57b90487d153b35ccfdeb134fcc858adf7/spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md))

All changed files versus main merge-base (including parents):

- `A docs/records/audits/2026-11-11-rfc00e1-corrections/LastTest.log` — absent_main; main blob `None`; head blob `8208f53a50c10fd0aef7b2d7b423e4ddf27eb467`.
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/build.log` — absent_main; main blob `None`; head blob `9c81d74ce551c84542cc072f1d7eedfbed7911d5`.
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/configure-cached.log` — absent_main; main blob `None`; head blob `1d3820a7a9a1ac3402581b499ba4bd75b7a15b2b`.
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/configure-default-interrupted.log` — absent_main; main blob `None`; head blob `018cb36733bc06d93550ca7b2a903cda0a8058c2`.
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/configure-dns-failure.log` — absent_main; main blob `None`; head blob `4daf27bae15bdc7f488f7f121244d076fcdcfc0a`.
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/ctest.log` — absent_main; main blob `None`; head blob `f93ddadabc6ab996c3be4b73e72c650820769599`.
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/development-183-trials.log` — absent_main; main blob `None`; head blob `c590db7116fb629ae2d589242dd9399c0505c633`.
- `A docs/records/audits/2026-11-11-rfc00e1-corrections/manifest.json` — absent_main; main blob `None`; head blob `ebda348aa425936bb4a1b67f6e530d151908451d`.
- `M experiments/scg08_arithmetic_work/CMakeLists.txt` — different; main blob `dd5478a32b01bb018055555269182b1251fb8390`; head blob `5a9f2517950af14424def62fa4f140abce2eb36a`.
- `A experiments/scg08_arithmetic_work/CONTRACT_CORRECTIONS.md` — absent_main; main blob `None`; head blob `8156b7585ac80036dd527eee04a4bf4af4633a74`.
- `M experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` — different; main blob `0671c097a9c73b8928cda57e689d37cf4669b68f`; head blob `22f467232c8a126fb263e0c53ce71b092aa8d5e5`.
- `M experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp` — different; main blob `3f3ed8d8db561fa5e6d9c31c5822ad2b04a7d361`; head blob `a2f6969306ed1c59491862da137a6123f5f902fe`.
- `M experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp` — different; main blob `0d9e7128642faab057544c14e8dd8276ccba3f88`; head blob `d06bacc0c67ba9ac00ed9f3b84883934a706096c`.
- `A experiments/scg08_arithmetic_work/test_result_rejection.py` — absent_main; main blob `None`; head blob `9e355c59480acc5d5bc2ecd81111ebc955f8bd41`.
- `M experiments/scg08_arithmetic_work/tier2_work_harness.hpp` — different; main blob `610999bdc334964922c8f3ae9f66f93be7c6944f`; head blob `0684805b3a10b3e70e2cdf34eb21854f989a18ac`.
- `M spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md` — different; main blob `82bbcabab68ef0bdecd8b17e295b7bf9f8769c9a`; head blob `e7d4ac61acea465338a5a3fd261e9a77bd269e9b`.

Overlaps with other PR own deltas:

- #779: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`, `experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp`, `experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp`, `experiments/scg08_arithmetic_work/tier2_work_harness.hpp`, `spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md`
- #766: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`
- #760: `experiments/scg08_arithmetic_work/CMakeLists.txt`, `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`, `experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp`, `experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp`, `experiments/scg08_arithmetic_work/tier2_work_harness.hpp`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/43146526101/job/130974033981): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/43146526101/job/130955900094): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/43146526101/job/130955900091): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/43146526101/job/130955900083): completed / cancelled.

## PR 774: Expose architecture visualization dataset provenance and distinguish Git failures

- Head `069f285acc3307f9fe99b6d0eb706ab4f63e9840`; actual base `89223724038ff7dec349343469df4ea8389ede26` (`main`); main merge-base `89223724038ff7dec349343469df4ea8389ede26`.
- Purpose: Dashboard dataset provenance and Git-error classification. Decision: **REVIEW NEXT**.
- Dependency assessment: Shares six changed paths with stale 760 deletions; not evidence of duplication.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Bounded 7-file dashboard/validator/test correction; preserve offline-versus-live labels and historical datasets. Review source and existing tests; browser execution not repeated here.
- Blob counts: {'different': 7}. Merge simulation exit 0; tree `c94d0fed6050a854d672fe88010832741470380c`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 28 pass, 3 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...069f285acc3307f9fe99b6d0eb706ab4f63e9840).

Own changed files:

- `M docs/architecture/visualization/DASHBOARD_GUIDE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/069f285acc3307f9fe99b6d0eb706ab4f63e9840/docs/architecture/visualization/DASHBOARD_GUIDE.md))
- `M docs/architecture/visualization/dashboard/dashboard.css` ([pinned source](https://github.com/t81dev/t81-foundation/blob/069f285acc3307f9fe99b6d0eb706ab4f63e9840/docs/architecture/visualization/dashboard/dashboard.css))
- `M docs/architecture/visualization/dashboard/dashboard.js` ([pinned source](https://github.com/t81dev/t81-foundation/blob/069f285acc3307f9fe99b6d0eb706ab4f63e9840/docs/architecture/visualization/dashboard/dashboard.js))
- `M docs/architecture/visualization/dashboard/index.html` ([pinned source](https://github.com/t81dev/t81-foundation/blob/069f285acc3307f9fe99b6d0eb706ab4f63e9840/docs/architecture/visualization/dashboard/index.html))
- `M scripts/ci/validate_architecture_visualization.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/069f285acc3307f9fe99b6d0eb706ab4f63e9840/scripts/ci/validate_architecture_visualization.py))
- `M tests/ci/test_architecture_dashboard.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/069f285acc3307f9fe99b6d0eb706ab4f63e9840/tests/ci/test_architecture_dashboard.py))
- `M tests/ci/test_architecture_engineering_diagnostics.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/069f285acc3307f9fe99b6d0eb706ab4f63e9840/tests/ci/test_architecture_engineering_diagnostics.py))

All changed files versus main merge-base (including parents):

- `M docs/architecture/visualization/DASHBOARD_GUIDE.md` — different; main blob `710590c1cae045c408361dc22cabe87e75bb35d0`; head blob `7607cc2c100c6fce314499b979c4432c7e20bcbc`.
- `M docs/architecture/visualization/dashboard/dashboard.css` — different; main blob `3fa22d0ae883003c92a6913ab4d1ad4a2a8f5168`; head blob `c4cdfad0af390170129f8eaae896591f522bf7a9`.
- `M docs/architecture/visualization/dashboard/dashboard.js` — different; main blob `0200405b86461f9db58d8d6302c55af531d9638a`; head blob `4df4e883ba2973da3cb6baf1ee7b9004d3d44dbf`.
- `M docs/architecture/visualization/dashboard/index.html` — different; main blob `713e4a46a001a585988940d146934439cd8358b1`; head blob `b7024c7543dade80e57075619e0879652dd05a8a`.
- `M scripts/ci/validate_architecture_visualization.py` — different; main blob `208e90ebcc3c31100d0bed7aa62f7956f047d9d3`; head blob `9a4a2a5580c94d492a5d2c10ae75e95ddbf30bfa`.
- `M tests/ci/test_architecture_dashboard.py` — different; main blob `ef0fae927b9db40e46dc6841c3921d6e43fca793`; head blob `e79a21e5c26e44673a61788456137674fdaebab3`.
- `M tests/ci/test_architecture_engineering_diagnostics.py` — different; main blob `b63d44afbbd6714b58edc50cab46b51ba7217b73`; head blob `1f0452966993d37cdce4601aaa2e2985972ac7b1`.

Overlaps with other PR own deltas:

- #760: `docs/architecture/visualization/DASHBOARD_GUIDE.md`, `docs/architecture/visualization/dashboard/dashboard.css`, `docs/architecture/visualization/dashboard/dashboard.js`, `docs/architecture/visualization/dashboard/index.html`, `scripts/ci/validate_architecture_visualization.py`, `tests/ci/test_architecture_dashboard.py`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/43126674249/job/130898301199): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/43126674249/job/130881984745): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/43126674249/job/130881984701): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/43126674249/job/130881984577): completed / cancelled.

## PR 773: docs(research): post-merge baseline verification and SCG-08 Gate 1 readiness review

- Head `8b19423d50462f07056030788aa8730a5a48838c`; actual base `89223724038ff7dec349343469df4ea8389ede26` (`main`); main merge-base `89223724038ff7dec349343469df4ea8389ede26`.
- Purpose: Dated post-merge baseline and Gate 1 readiness report. Decision: **REVIEW NEXT**.
- Dependency assessment: References merged 770/771/772/775 and open 748/750/779; overlaps other SCG continuity claims, not files.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Both old inline findings are outdated; current text withholds acceptance and restores prerequisites. It still describes 775 as a pending draft: preserve as dated history or add a current overlay; do not present it as current status.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `4f009fbf00ef9fd30deea4481ba3296785fa9ff6`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/2 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 37 pass, 3 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...8b19423d50462f07056030788aa8730a5a48838c).

Own changed files:

- `A docs/research/continuity/POST_MERGE_BASELINE_AND_SCG08_GATE1_REVIEW_20261111.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8b19423d50462f07056030788aa8730a5a48838c/docs/research/continuity/POST_MERGE_BASELINE_AND_SCG08_GATE1_REVIEW_20261111.md))

All changed files versus main merge-base (including parents):

- `A docs/research/continuity/POST_MERGE_BASELINE_AND_SCG08_GATE1_REVIEW_20261111.md` — absent_main; main blob `None`; head blob `f4246d3412acff7ffe1c8970c8dc19705637ef9f`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `d05e9123dd129a46a0cf2fb22946309efbf75f68` / [review](https://github.com/t81dev/t81-foundation/pull/773#pullrequestreview-5860885006).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/43133248981/job/130915921140): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/43133248981/job/130906124238): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/43133248981/job/130906124036): completed / failure.

## PR 769: fix(frontend): semantic analyzer scope-lifetime stability correction

- Head `7c52be13dc0c3fb175271d63f1a8ba9aaeee170c`; actual base `36b3849aa1e6297e88de2ec1655777793ab0b59e` (`main`); main merge-base `36b3849aa1e6297e88de2ec1655777793ab0b59e`.
- Purpose: Semantic-analyzer scope lifetime repair. Decision: **SUPERSEDED**.
- Dependency assessment: Integrated through 771; main also retains later CMake timeout/check registrations.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [747].
- Evidence/limits: Simulated merge produces exactly main tree fa633c105abfe229e77eabdccd2453c620636275. Main already has header, active regression and API lock from merged 771. Closure recommendation is source-based.
- Blob counts: {'different': 1, 'identical': 3}. Merge simulation exit 0; tree `fa633c105abfe229e77eabdccd2453c620636275`; equals main: True.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 27 pass, 5 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...7c52be13dc0c3fb175271d63f1a8ba9aaeee170c).

Own changed files:

- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7c52be13dc0c3fb175271d63f1a8ba9aaeee170c/CMakeLists.txt))
- `M docs/governance/PUBLIC_API_LOCK.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7c52be13dc0c3fb175271d63f1a8ba9aaeee170c/docs/governance/PUBLIC_API_LOCK.json))
- `M include/t81/frontend/semantic_analyzer.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7c52be13dc0c3fb175271d63f1a8ba9aaeee170c/include/t81/frontend/semantic_analyzer.hpp))
- `A tests/cpp/semantic_analyzer_scope_lifetime_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7c52be13dc0c3fb175271d63f1a8ba9aaeee170c/tests/cpp/semantic_analyzer_scope_lifetime_test.cpp))

All changed files versus main merge-base (including parents):

- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `95d420cff67c4597d24d680f136a5cb191ebe627`.
- `M docs/governance/PUBLIC_API_LOCK.json` — identical; main blob `436dd6ead049adc90702fec718da53c38d63bb10`; head blob `436dd6ead049adc90702fec718da53c38d63bb10`.
- `M include/t81/frontend/semantic_analyzer.hpp` — identical; main blob `4ef5f7c6190ac1cb9ab77f08b93520bffde1c1a6`; head blob `4ef5f7c6190ac1cb9ab77f08b93520bffde1c1a6`.
- `A tests/cpp/semantic_analyzer_scope_lifetime_test.cpp` — identical; main blob `b1d874e56deada7c53aac39f1b50c11293080158`; head blob `b1d874e56deada7c53aac39f1b50c11293080158`.

Overlaps with other PR own deltas:

- #782: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`
- #747: `CMakeLists.txt`
- #649: `CMakeLists.txt`
- #603: `CMakeLists.txt`
- #556: `CMakeLists.txt`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #508: `CMakeLists.txt`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/42955651834/job/130322887874): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/42955651834/job/130311971266): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42955651834/job/130311971258): completed / cancelled.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42955651834/job/130311971252): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/42955651834/job/130311971249): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/42955651834/job/130310333870): completed / failure.

## PR 768: docs(scg08): archive cross-agent evidence handoff and reconciliation assignment

- Head `a7fca2fbc4c5c6dfdb861c37fe434c481ca5737b`; actual base `36b3849aa1e6297e88de2ec1655777793ab0b59e` (`main`); main merge-base `36b3849aa1e6297e88de2ec1655777793ab0b59e`.
- Purpose: Archive historical cross-agent SCG evidence. Decision: **REVIEW NEXT**.
- Dependency assessment: Referenced by merged 753 reconciliation and SCG reports; no implementation dependency.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: One unique archive document is absent from main and referenced by main reconciliation report. Review historical attribution and proposed Gate 2 text as evidence only; not authority.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `0e88bd63c1c01ac4917a4b019a3ebcc0c18e73ab`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 31 pass, 5 fail, 8 skip, 4 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...a7fca2fbc4c5c6dfdb861c37fe434c481ca5737b).

Own changed files:

- `A docs/research/continuity/SCG08_JULES_COMPLETE_EVIDENCE_HANDOFF_2026-11-11.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a7fca2fbc4c5c6dfdb861c37fe434c481ca5737b/docs/research/continuity/SCG08_JULES_COMPLETE_EVIDENCE_HANDOFF_2026-11-11.md))

All changed files versus main merge-base (including parents):

- `A docs/research/continuity/SCG08_JULES_COMPLETE_EVIDENCE_HANDOFF_2026-11-11.md` — absent_main; main blob `None`; head blob `3bac6961e2b119de63781779dee7147fffbd6a89`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/42954226902/job/130316143846): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/42954226902/job/130306708741): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/42954226902/job/130306708730): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42954226902/job/130306708719): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42954226902/job/130306708711): completed / cancelled.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/42954226902/job/130305727723): completed / cancelled.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/42954226902/job/130305727635): completed / failure.
- [discovery (macos-14, 3.9)](https://github.com/t81dev/t81-foundation/actions/runs/42954226904/job/130305396388): completed / cancelled.
- [discovery (macos-14, 3.13)](https://github.com/t81dev/t81-foundation/actions/runs/42954226904/job/130305396017): completed / cancelled.

## PR 767: fix(scg08): reconcile remaining validator checks atop #766

- Head `bd77c00122ff9cc63097d0ee2cd88895266878cc`; actual base `a763ab6876bf07b8ae7c529656cdd2e5d9cf3766` (`fix/scg08-gate1-provenance-and-scalar-validation-36b3849-5951848895340305516`); main merge-base `36b3849aa1e6297e88de2ec1655777793ab0b59e`.
- Purpose: Additional strict SCG evidence validator checks. Decision: **REPAIR**.
- Dependency assessment: Stacks on superseded 766; conflicts with current main validator after merged 775.
- Open heads included by ancestry: [766]; references in body (not necessarily dependencies): [766, 767].
- Evidence/limits: One own-delta file atop 766; main lacks its finite-JSON/calendar/recursive-schema logic, so not superseded. Rebase/port only surviving checks while retaining main evidence_generation_sha rules and newer negative tests.
- Blob counts: {'different': 4}. Merge simulation exit 1; tree `7b4eb7b66aa5fbe37c55cfe2e91ff373509fcb85`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...bd77c00122ff9cc63097d0ee2cd88895266878cc).

Own changed files:

- `M scripts/ci/validate_scg08_gate1_evidence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/bd77c00122ff9cc63097d0ee2cd88895266878cc/scripts/ci/validate_scg08_gate1_evidence.py))

All changed files versus main merge-base (including parents):

- `M experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` — different; main blob `0671c097a9c73b8928cda57e689d37cf4669b68f`; head blob `051f07efb16569080554cf61fe586e80c7f20d16`.
- `M experiments/scg08_arithmetic_work/evidence_schema.json` — different; main blob `8a4a641dbe3fffb0d2f40b24e5a285f82eff0433`; head blob `45ffed245f10d236537fe9150946878bbaf79928`.
- `M experiments/scg08_arithmetic_work/gate1_evidence_record.json` — different; main blob `00731bd34cfad475acde588085827698627ec60d`; head blob `9e7d662264faf8bebde50b14e612ff0387d5b3a2`.
- `M scripts/ci/validate_scg08_gate1_evidence.py` — different; main blob `31501e92a352d2b2a49f668d41f5d1a4ffc42496`; head blob `eb4e73900a8f9dd1112c1b8e4f7e0b59f9e1c59e`.

Overlaps with other PR own deltas:

- #766: `scripts/ci/validate_scg08_gate1_evidence.py`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 766: fix(scg08): repair Gate 1 evidence provenance and scalar schema validation

- Head `a763ab6876bf07b8ae7c529656cdd2e5d9cf3766`; actual base `36b3849aa1e6297e88de2ec1655777793ab0b59e` (`main`); main merge-base `36b3849aa1e6297e88de2ec1655777793ab0b59e`.
- Purpose: First evidence/source distinction and scalar-validator correction. Decision: **SUPERSEDED**.
- Dependency assessment: Four paths all represented on main after 775; 767 still has unique follow-on logic.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [765].
- Evidence/limits: Merged 775/main contains non-null publication handling, strict scalar checks, full SHA matching and newer provenance-generation distinction. Older branch would discard newer checks/provenance wording. Preserve candidate as historical evidence, not a new repair.
- Blob counts: {'different': 4}. Merge simulation exit 1; tree `5c2188ac3c1f23a0ab473d91d4fd7e8655b112bc`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/1 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 24 pass, 4 fail, 8 skip, 4 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...a763ab6876bf07b8ae7c529656cdd2e5d9cf3766).

Own changed files:

- `M experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a763ab6876bf07b8ae7c529656cdd2e5d9cf3766/experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md))
- `M experiments/scg08_arithmetic_work/evidence_schema.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a763ab6876bf07b8ae7c529656cdd2e5d9cf3766/experiments/scg08_arithmetic_work/evidence_schema.json))
- `M experiments/scg08_arithmetic_work/gate1_evidence_record.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a763ab6876bf07b8ae7c529656cdd2e5d9cf3766/experiments/scg08_arithmetic_work/gate1_evidence_record.json))
- `M scripts/ci/validate_scg08_gate1_evidence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a763ab6876bf07b8ae7c529656cdd2e5d9cf3766/scripts/ci/validate_scg08_gate1_evidence.py))

All changed files versus main merge-base (including parents):

- `M experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` — different; main blob `0671c097a9c73b8928cda57e689d37cf4669b68f`; head blob `051f07efb16569080554cf61fe586e80c7f20d16`.
- `M experiments/scg08_arithmetic_work/evidence_schema.json` — different; main blob `8a4a641dbe3fffb0d2f40b24e5a285f82eff0433`; head blob `45ffed245f10d236537fe9150946878bbaf79928`.
- `M experiments/scg08_arithmetic_work/gate1_evidence_record.json` — different; main blob `00731bd34cfad475acde588085827698627ec60d`; head blob `9e7d662264faf8bebde50b14e612ff0387d5b3a2`.
- `M scripts/ci/validate_scg08_gate1_evidence.py` — different; main blob `31501e92a352d2b2a49f668d41f5d1a4ffc42496`; head blob `2f47c502cf6c348908eb011a626b55e9f7aae2ec`.

Overlaps with other PR own deltas:

- #779: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`
- #778: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`
- #767: `scripts/ci/validate_scg08_gate1_evidence.py`
- #760: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`, `experiments/scg08_arithmetic_work/evidence_schema.json`

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `755f1e60b4f9076ef3d29ac40f0206f2097b6028` / [review](https://github.com/t81dev/t81-foundation/pull/766#pullrequestreview-5852364230).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/42941409376/job/130284515868): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42941409376/job/130266957538): completed / cancelled.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42941409376/job/130266957508): completed / cancelled.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42941409376/job/130266957500): completed / cancelled.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/42941409376/job/130266957479): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/42941409376/job/130266957474): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/42941409376/job/130264200618): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/42941409376/job/130264200555): completed / cancelled.

## PR 764: docs(continuity): reconcile research, PR inventory and SCG-08 Gate 1 status

- Head `cd9384033834323a1775ca6cc50bb86fc219fecb`; actual base `36b3849aa1e6297e88de2ec1655777793ab0b59e` (`main`); main merge-base `36b3849aa1e6297e88de2ec1655777793ab0b59e`.
- Purpose: Continuity inventory and dashboard reconciliation. Decision: **REPAIR**.
- Dependency assessment: Overlaps 657/640/601 continuity work and current merged 754/781; 774 is a separate dashboard implementation.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [765].
- Evidence/limits: 26 files include unique receipts plus shared current-state/handoff data. Merge simulation conflicts with newer main. Append a fresh dated overlay and preserve earlier receipts; do not overwrite current main status.
- Blob counts: {'different': 5, 'absent_main': 21}. Merge simulation exit 1; tree `62dac94e260333f511cb6fcba7c1e5d7f7f4426b`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 26 pass, 5 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...cd9384033834323a1775ca6cc50bb86fc219fecb).

Own changed files:

- `M docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md))
- `M docs/ai-context/CURRENT_STATE.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/CURRENT_STATE.json))
- `M docs/ai-context/START_HERE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/START_HERE.md))
- `A docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R1.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R1.md))
- `A docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R2.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R2.md))
- `A docs/ai-context/receipts/2026-11-10-r1/VALIDATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/VALIDATION.md))
- `A docs/ai-context/receipts/2026-11-10-r1/baseline-bash-reproduction.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/baseline-bash-reproduction.log))
- `A docs/ai-context/receipts/2026-11-10-r1/browser-test.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/browser-test.log))
- `A docs/ai-context/receipts/2026-11-10-r1/correction-candidate.patch` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/correction-candidate.patch))
- `A docs/ai-context/receipts/2026-11-10-r1/correction-evidence-validation.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/correction-evidence-validation.log))
- `A docs/ai-context/receipts/2026-11-10-r1/correction-negative-validation.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/correction-negative-validation.log))
- `A docs/ai-context/receipts/2026-11-10-r1/correction-schema-probe.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/correction-schema-probe.log))
- `A docs/ai-context/receipts/2026-11-10-r1/correction_schema_probe.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/correction_schema_probe.py))
- `A docs/ai-context/receipts/2026-11-10-r1/data-contract-tests.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/data-contract-tests.log))
- `A docs/ai-context/receipts/2026-11-10-r1/github-observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/github-observations.json))
- `A docs/ai-context/receipts/2026-11-10-r1/pr-inventory.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/pr-inventory.json))
- `A docs/ai-context/receipts/2026-11-10-r1/validate_reconciliation.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/validate_reconciliation.py))
- `A docs/ai-context/receipts/2026-11-10-r1/visualization-validation.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r1/visualization-validation.log))
- `A docs/ai-context/receipts/2026-11-10-r2/VALIDATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r2/VALIDATION.md))
- `A docs/ai-context/receipts/2026-11-10-r2/baseline-bash-reproduction.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r2/baseline-bash-reproduction.log))
- `A docs/ai-context/receipts/2026-11-10-r2/browser-test.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r2/browser-test.log))
- `A docs/ai-context/receipts/2026-11-10-r2/data-contract-tests.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r2/data-contract-tests.log))
- `A docs/ai-context/receipts/2026-11-10-r2/github-observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r2/github-observations.json))
- `A docs/ai-context/receipts/2026-11-10-r2/validate_followup.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/ai-context/receipts/2026-11-10-r2/validate_followup.py))
- `M docs/architecture/visualization/pull_requests_snapshot.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/architecture/visualization/pull_requests_snapshot.json))
- `M docs/architecture/visualization/research_programs.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cd9384033834323a1775ca6cc50bb86fc219fecb/docs/architecture/visualization/research_programs.json))

All changed files versus main merge-base (including parents):

- `M docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md` — different; main blob `526a9750ef88f1a7c445ae3adfd115f46bdf87a8`; head blob `012180bab65c1b92a2bbe2fb23f4b3176d9f3980`.
- `M docs/ai-context/CURRENT_STATE.json` — different; main blob `2006e2ce3d9255f2ccc96b36a11a047661303acd`; head blob `7afcd9b235d34a1a6888fb29d913d1e469e7807d`.
- `M docs/ai-context/START_HERE.md` — different; main blob `1f790fff2355f61f7639fa65486d486ff9b6d5c8`; head blob `6e9a529e24db3968520e8483e6ee078125852faa`.
- `A docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R1.md` — absent_main; main blob `None`; head blob `3133084271e4d8de7feac0741ccf72e0d7c2a5b1`.
- `A docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R2.md` — absent_main; main blob `None`; head blob `bd76557fc12ea8ac35e1f507e2546e00db3360f4`.
- `A docs/ai-context/receipts/2026-11-10-r1/VALIDATION.md` — absent_main; main blob `None`; head blob `a868cb74b0ad453acc7008651b98696e2fe29204`.
- `A docs/ai-context/receipts/2026-11-10-r1/baseline-bash-reproduction.log` — absent_main; main blob `None`; head blob `047d02a9c0e64fd84917fc2d48eae359392a24af`.
- `A docs/ai-context/receipts/2026-11-10-r1/browser-test.log` — absent_main; main blob `None`; head blob `e01eecf2d582178037f0038ed0dac1ac65b63b55`.
- `A docs/ai-context/receipts/2026-11-10-r1/correction-candidate.patch` — absent_main; main blob `None`; head blob `882620b9fdf695548948ba580ab38da7dec94edd`.
- `A docs/ai-context/receipts/2026-11-10-r1/correction-evidence-validation.log` — absent_main; main blob `None`; head blob `8beb386208cb233b56d6cd73b65246ae47198875`.
- `A docs/ai-context/receipts/2026-11-10-r1/correction-negative-validation.log` — absent_main; main blob `None`; head blob `168208a9e0119b8762f8f551db4726f28ba031a3`.
- `A docs/ai-context/receipts/2026-11-10-r1/correction-schema-probe.log` — absent_main; main blob `None`; head blob `5a30893a5153e5866db80f8833d3e23c64884646`.
- `A docs/ai-context/receipts/2026-11-10-r1/correction_schema_probe.py` — absent_main; main blob `None`; head blob `cbb5a19522589e747bbb3bb0de83a3c3eed3b3c0`.
- `A docs/ai-context/receipts/2026-11-10-r1/data-contract-tests.log` — absent_main; main blob `None`; head blob `5478a024d2d349104c9a828c9d66e08ba9caed8d`.
- `A docs/ai-context/receipts/2026-11-10-r1/github-observations.json` — absent_main; main blob `None`; head blob `88cd873d776d36a5ccfc130b94cf41ef685445ce`.
- `A docs/ai-context/receipts/2026-11-10-r1/pr-inventory.json` — absent_main; main blob `None`; head blob `b9d1194fa88a85ab7850321503e349f9c6775e01`.
- `A docs/ai-context/receipts/2026-11-10-r1/validate_reconciliation.py` — absent_main; main blob `None`; head blob `d09ec4493a2cd3d46a3b3a0d5b24871b51f25ce5`.
- `A docs/ai-context/receipts/2026-11-10-r1/visualization-validation.log` — absent_main; main blob `None`; head blob `8cc3da54038cf009bf40e078548404552b0961df`.
- `A docs/ai-context/receipts/2026-11-10-r2/VALIDATION.md` — absent_main; main blob `None`; head blob `63ac147ad885c659494838539b40e6bb010b2c5f`.
- `A docs/ai-context/receipts/2026-11-10-r2/baseline-bash-reproduction.log` — absent_main; main blob `None`; head blob `90959b5699f7a1d1db61460edb9de0dc4d8a06ec`.
- `A docs/ai-context/receipts/2026-11-10-r2/browser-test.log` — absent_main; main blob `None`; head blob `b66a6eb223ac8fc797e6441a9c9490585ba43e32`.
- `A docs/ai-context/receipts/2026-11-10-r2/data-contract-tests.log` — absent_main; main blob `None`; head blob `fa58139113a324a233501d3c947d05d23eb137b3`.
- `A docs/ai-context/receipts/2026-11-10-r2/github-observations.json` — absent_main; main blob `None`; head blob `97134cfbc907b1a28708bb1d19a406b85479c6d4`.
- `A docs/ai-context/receipts/2026-11-10-r2/validate_followup.py` — absent_main; main blob `None`; head blob `318f4970094b08036a85221d48bedc1d4d50c3e5`.
- `M docs/architecture/visualization/pull_requests_snapshot.json` — different; main blob `8d9a911ede2a466d8373724ae284c21dd166257d`; head blob `ff824864dc4f7802dc08380ee8f7e518b17a54da`.
- `M docs/architecture/visualization/research_programs.json` — different; main blob `6650f5af164fe4562799adfeafd9ac56d4c780f8`; head blob `ec040586c3882bc18ab405b2cf4a9db7709ab494`.

Overlaps with other PR own deltas:

- #760: `docs/architecture/visualization/pull_requests_snapshot.json`, `docs/architecture/visualization/research_programs.json`
- #657: `docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`, `docs/ai-context/START_HERE.md`
- #640: `docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`, `docs/ai-context/START_HERE.md`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/42936654263/job/130266729629): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42936654263/job/130253650572): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/42936654263/job/130253650536): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/42936654263/job/130253650422): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42936654263/job/130253650400): completed / cancelled.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/42936654263/job/130249154221): completed / failure.

## PR 761: docs(scg08): publish Phase 0 independent acceptance review R2

- Head `5c69f11579b96686de1520aa45062b418c7c3bdd`; actual base `8e6918fa98e19ee1476f91c97e82989b051a02e7` (`main`); main merge-base `8e6918fa98e19ee1476f91c97e82989b051a02e7`.
- Purpose: Phase 0 mathematical and acceptance report. Decision: **REPAIR**.
- Dependency assessment: SCG-08 specification and prospective acceptance decisions remain separate; no Gate 2 authorization.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [745, 753, 754].
- Evidence/limits: Current source still says NO BLOCKERS and attributes a 3-sigma criterion to charter section 5.3 while declaring unreconciled sample allocation. Reconcile claims and authority before acceptance; mathematics is a separate bounded finding.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `0bfb5eb6dc5fae3c68bbaaa9b4cdc0d587ef9112`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 31 pass, 5 fail, 8 skip, 4 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...5c69f11579b96686de1520aa45062b418c7c3bdd).

Own changed files:

- `A docs/research/continuity/SCG08_PHASE0_INDEPENDENT_ACCEPTANCE_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/5c69f11579b96686de1520aa45062b418c7c3bdd/docs/research/continuity/SCG08_PHASE0_INDEPENDENT_ACCEPTANCE_REPORT.md))

All changed files versus main merge-base (including parents):

- `A docs/research/continuity/SCG08_PHASE0_INDEPENDENT_ACCEPTANCE_REPORT.md` — absent_main; main blob `None`; head blob `ceb1fbd4539852f31182a7c0337a865bf53faada`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/42901761379/job/130154238421): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42901761379/job/130137536170): completed / cancelled.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42901761379/job/130137535875): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/42901761379/job/130137535821): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/42901761379/job/130137535768): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/42901761379/job/130131323785): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/42901761379/job/130131323433): completed / cancelled.
- [discovery (macos-14, 3.9)](https://github.com/t81dev/t81-foundation/actions/runs/42901761253/job/130129257741): completed / cancelled.
- [discovery (macos-14, 3.13)](https://github.com/t81dev/t81-foundation/actions/runs/42901761253/job/130129257569): completed / cancelled.

## PR 760: SCG-08 Phase 1 Gate 1 Implementation: Carry Propagation & Arithmetic Work Harness

- Head `dab59a63e4000f81799a6b2303bbfab26300d526`; actual base `8e6918fa98e19ee1476f91c97e82989b051a02e7` (`main`); main merge-base `8e6918fa98e19ee1476f91c97e82989b051a02e7`.
- Purpose: Original SCG arithmetic harness. Decision: **SUPERSEDED**.
- Dependency assessment: Superseded implementation by merged 763/765, with later 775 and candidate 779 corrections.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Head tree exactly equals original implementation 525554a; all seven initial added paths exist on main after 763/765. Residual delta deletes 50 unrelated newer paths. Preserve original history; do not merge rollback deletions.
- Blob counts: {'different': 8, 'deleted_head': 50, 'identical': 2}. Merge simulation exit 1; tree `f29cc84c17b1113ba7cea5731882ffce65da8af9`; equals main: False.
- Review: 1 COMMENTED; threads 3 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...dab59a63e4000f81799a6b2303bbfab26300d526).

Own changed files:

- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/CMakeLists.txt))
- `D docs/architecture/diagnostics/phase3/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/REPORT.md))
- `D docs/architecture/diagnostics/phase3/check_evidence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/check_evidence.py))
- `D docs/architecture/diagnostics/phase3/evidence/SHA256SUMS.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/SHA256SUMS.json))
- `D docs/architecture/diagnostics/phase3/evidence/build.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/build.json))
- `D docs/architecture/diagnostics/phase3/evidence/environment.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/environment.json))
- `D docs/architecture/diagnostics/phase3/evidence/fresh-image.img.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/fresh-image.img.gz))
- `D docs/architecture/diagnostics/phase3/evidence/fresh-media.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/fresh-media.json))
- `D docs/architecture/diagnostics/phase3/evidence/media.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/media.json))
- `D docs/architecture/diagnostics/phase3/evidence/mock-processes.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/mock-processes.json))
- `D docs/architecture/diagnostics/phase3/evidence/mock-tests.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/mock-tests.log))
- `D docs/architecture/diagnostics/phase3/evidence/observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/observations.json))
- `D docs/architecture/diagnostics/phase3/evidence/reverse-media.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/reverse-media.json))
- `D docs/architecture/diagnostics/phase3/evidence/reverse-substitution.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/reverse-substitution.json))
- `D docs/architecture/diagnostics/phase3/evidence/substituted-media.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/substituted-media.json))
- `D docs/architecture/diagnostics/phase3/evidence/substitution.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/substitution.json))
- `D docs/architecture/diagnostics/phase3/evidence/upstream-sources.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/upstream-sources.json))
- `D docs/architecture/diagnostics/phase3/evidence/validation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/validation.json))
- `D docs/architecture/diagnostics/phase3/evidence/verifier-runs.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/evidence/verifier-runs.json))
- `D docs/architecture/diagnostics/phase3/inspect_media.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/inspect_media.py))
- `D docs/architecture/diagnostics/phase3/observe.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/observe.py))
- `D docs/architecture/diagnostics/phase3/substitute_efi.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase3/substitute_efi.py))
- `D docs/architecture/diagnostics/phase4/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/REPORT.md))
- `D docs/architecture/diagnostics/phase4/check_evidence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/check_evidence.py))
- `D docs/architecture/diagnostics/phase4/evidence/SHA256SUMS.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/SHA256SUMS.json))
- `D docs/architecture/diagnostics/phase4/evidence/build-and-setup-logs.json.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/build-and-setup-logs.json.gz))
- `D docs/architecture/diagnostics/phase4/evidence/build-receipts.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/build-receipts.json))
- `D docs/architecture/diagnostics/phase4/evidence/fresh-guest.img.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/fresh-guest.img.gz))
- `D docs/architecture/diagnostics/phase4/evidence/instrumentation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/instrumentation.json))
- `D docs/architecture/diagnostics/phase4/evidence/matrix.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/matrix.json))
- `D docs/architecture/diagnostics/phase4/evidence/mock-processes.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/mock-processes.json))
- `D docs/architecture/diagnostics/phase4/evidence/mock-tests.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/mock-tests.log))
- `D docs/architecture/diagnostics/phase4/evidence/observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/observations.json))
- `D docs/architecture/diagnostics/phase4/evidence/provenance.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/provenance.json))
- `D docs/architecture/diagnostics/phase4/evidence/validation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/evidence/validation.json))
- `D docs/architecture/diagnostics/phase4/matched_qemu.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/matched_qemu.py))
- `D docs/architecture/diagnostics/phase4/observe.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/observe.py))
- `D docs/architecture/diagnostics/phase4/run_matrix.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/diagnostics/phase4/run_matrix.py))
- `M docs/architecture/visualization/ARCHITECTURE_VISUALIZATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/docs/architecture/visualization/ARCHITECTURE_VISUALIZATION_REPORT.md))
- `D docs/architecture/visualization/DASHBOARD_GUIDE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/DASHBOARD_GUIDE.md))
- `D docs/architecture/visualization/dashboard.css` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/dashboard.css))
- `D docs/architecture/visualization/dashboard.js` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/dashboard.js))
- `D docs/architecture/visualization/dashboard/dashboard.css` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/dashboard/dashboard.css))
- `D docs/architecture/visualization/dashboard/dashboard.js` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/dashboard/dashboard.js))
- `D docs/architecture/visualization/dashboard/index.html` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/dashboard/index.html))
- `D docs/architecture/visualization/dashboard_2d_screenshot.png` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/dashboard_2d_screenshot.png))
- `D docs/architecture/visualization/dashboard_3d_screenshot.png` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/dashboard_3d_screenshot.png))
- `D docs/architecture/visualization/index.html` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/index.html))
- `D docs/architecture/visualization/pull_requests_snapshot.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/pull_requests_snapshot.json))
- `D docs/architecture/visualization/research_programs.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/docs/architecture/visualization/research_programs.json))
- `D drivers/qemu/production_image/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/drivers/qemu/production_image/README.md))
- `A experiments/scg08_arithmetic_work/CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/experiments/scg08_arithmetic_work/CMakeLists.txt))
- `A experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md))
- `A experiments/scg08_arithmetic_work/evidence_schema.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/experiments/scg08_arithmetic_work/evidence_schema.json))
- `A experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp))
- `A experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp))
- `A experiments/scg08_arithmetic_work/tier1_markov_harness.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/experiments/scg08_arithmetic_work/tier1_markov_harness.hpp))
- `A experiments/scg08_arithmetic_work/tier2_work_harness.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/experiments/scg08_arithmetic_work/tier2_work_harness.hpp))
- `M scripts/ci/validate_architecture_visualization.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/dab59a63e4000f81799a6b2303bbfab26300d526/scripts/ci/validate_architecture_visualization.py))
- `D tests/ci/test_architecture_dashboard.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8e6918fa98e19ee1476f91c97e82989b051a02e7/tests/ci/test_architecture_dashboard.py))

All changed files versus main merge-base (including parents):

- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `1991a8485b11824e84fbc2cfdd76b6a4b3aa764b`.
- `D docs/architecture/diagnostics/phase3/REPORT.md` — deleted_head; main blob `5119be458d43523666c55b77c1330c8d06abac69`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/check_evidence.py` — deleted_head; main blob `2713a626db75c06df8f1165fa4a0ef944210d362`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/SHA256SUMS.json` — deleted_head; main blob `f48a950323203e3af492d1f903e50035781b4ff2`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/build.json` — deleted_head; main blob `586800a14fa666e10228bed1d64fb5c3069650ab`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/environment.json` — deleted_head; main blob `8e7edb41563ea4c05e141a2aa191257578c1cdfe`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/fresh-image.img.gz` — deleted_head; main blob `6dcde7d28a29605f2b1b0180593d5d43641097b2`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/fresh-media.json` — deleted_head; main blob `ff6403d0b67819b8894ebfe012950c221645f90b`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/media.json` — deleted_head; main blob `8a675e30d529ae1b7f4debcbdc26b996fa6c5426`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/mock-processes.json` — deleted_head; main blob `e02745aa825aac75371d08646befa7ad70cf063f`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/mock-tests.log` — deleted_head; main blob `3e905b264f40cb6de13ea179813961cf178fa1ec`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/observations.json` — deleted_head; main blob `22db5a4fe470e135e4fe0565513d7350b44c7b18`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/reverse-media.json` — deleted_head; main blob `1d3e81da6c777372f2175d662cf38544d815c128`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/reverse-substitution.json` — deleted_head; main blob `24bfe3f3faefdb9c9c61563c9cddffd552ab2d26`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/substituted-media.json` — deleted_head; main blob `e230ec4757bcad9bc0f3d9268001e56ab1428430`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/substitution.json` — deleted_head; main blob `74121fd55db1eb3a0954e1710da6a38d17c97d95`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/upstream-sources.json` — deleted_head; main blob `80eaa28524fbbc87553854aa84525159e2c2bdb1`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/validation.json` — deleted_head; main blob `881efe6dfb0490750a5e5fc272003c7a06ca11a9`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/evidence/verifier-runs.json` — deleted_head; main blob `0090b64fd14c148b8208d93c443aae64ea961250`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/inspect_media.py` — deleted_head; main blob `9267d6705f108a2be05e5842dee5812cdffbdf09`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/observe.py` — deleted_head; main blob `c3d4cbd67ad8737dd0380041a8255aeb88c38d8d`; head blob `None`.
- `D docs/architecture/diagnostics/phase3/substitute_efi.py` — deleted_head; main blob `19a07eb694ee95180302ac67f6456c8de2be871e`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/REPORT.md` — deleted_head; main blob `13cb63f186041423f749a9d8cdde522f58ef1f55`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/check_evidence.py` — deleted_head; main blob `5c6c7ccc2197112bc45d4cb5f75b106f89a91b92`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/SHA256SUMS.json` — deleted_head; main blob `24653f85f3e71082bdb8ae91b43cc92545234c4b`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/build-and-setup-logs.json.gz` — deleted_head; main blob `a1a80311cfa5862fb1ca047aa1fd21adab0b65cc`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/build-receipts.json` — deleted_head; main blob `162a573e4a68188a4a477e3b1bf06eeed7f639c9`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/fresh-guest.img.gz` — deleted_head; main blob `f20253f7a5f3adbeb892f5c2e5e939076b74105f`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/instrumentation.json` — deleted_head; main blob `d03f4a46ca440f83713e26324e1a567a9563310e`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/matrix.json` — deleted_head; main blob `abbce96036a30844b61372f3a3ac891f3a060fcb`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/mock-processes.json` — deleted_head; main blob `d81cb9bd1bb9bda9c4626a4a9fdf411c289a5f5e`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/mock-tests.log` — deleted_head; main blob `38c01c3788daaba1fa4fa9f12dbc45d6bcd191a7`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/observations.json` — deleted_head; main blob `ad57b1e46e3610cfc5f4ebf25bc14a2a897f86ab`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/provenance.json` — deleted_head; main blob `dc08c15d8aee9f436cf7bc1dd6dfab42ef4b4028`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/evidence/validation.json` — deleted_head; main blob `f69b2c5766d37a3f21870a91565f114d88d5f45a`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/matched_qemu.py` — deleted_head; main blob `fea3f1f89c364196b53c8618015280832dc7d80b`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/observe.py` — deleted_head; main blob `939808c854934136edbe003989efc512cd12a47c`; head blob `None`.
- `D docs/architecture/diagnostics/phase4/run_matrix.py` — deleted_head; main blob `2afacf589d11d2939af3e2a9aaca9218b1b3ef62`; head blob `None`.
- `M docs/architecture/visualization/ARCHITECTURE_VISUALIZATION_REPORT.md` — different; main blob `04d41a28b346b229c23a3b2e2250e1b60af4e3bf`; head blob `1f15858e4c39f4f235dbf7252d7e85bcbcae3281`.
- `D docs/architecture/visualization/DASHBOARD_GUIDE.md` — deleted_head; main blob `710590c1cae045c408361dc22cabe87e75bb35d0`; head blob `None`.
- `D docs/architecture/visualization/dashboard.css` — deleted_head; main blob `2d344f9d55d0d5dd57cbf694479be398bd235175`; head blob `None`.
- `D docs/architecture/visualization/dashboard.js` — deleted_head; main blob `3bbaa5ab4371776dc991cbc233c155b866cbc492`; head blob `None`.
- `D docs/architecture/visualization/dashboard/dashboard.css` — deleted_head; main blob `3fa22d0ae883003c92a6913ab4d1ad4a2a8f5168`; head blob `None`.
- `D docs/architecture/visualization/dashboard/dashboard.js` — deleted_head; main blob `0200405b86461f9db58d8d6302c55af531d9638a`; head blob `None`.
- `D docs/architecture/visualization/dashboard/index.html` — deleted_head; main blob `713e4a46a001a585988940d146934439cd8358b1`; head blob `None`.
- `D docs/architecture/visualization/dashboard_2d_screenshot.png` — deleted_head; main blob `ba647d09efcee2e0f4964efd2f0cd3000e463526`; head blob `None`.
- `D docs/architecture/visualization/dashboard_3d_screenshot.png` — deleted_head; main blob `7a8b79a3b6b8a72a9a16391e067cff0ff1430fe9`; head blob `None`.
- `D docs/architecture/visualization/index.html` — deleted_head; main blob `1580f00956c308ce4fa816c939d85b3cb2a9f934`; head blob `None`.
- `D docs/architecture/visualization/pull_requests_snapshot.json` — deleted_head; main blob `8d9a911ede2a466d8373724ae284c21dd166257d`; head blob `None`.
- `D docs/architecture/visualization/research_programs.json` — deleted_head; main blob `6650f5af164fe4562799adfeafd9ac56d4c780f8`; head blob `None`.
- `D drivers/qemu/production_image/README.md` — deleted_head; main blob `443ee399003809d135e2a4c257f513a02cd98cce`; head blob `None`.
- `A experiments/scg08_arithmetic_work/CMakeLists.txt` — different; main blob `dd5478a32b01bb018055555269182b1251fb8390`; head blob `2a9e05a7a259ebc03ca84c0f5f0af8f4e9bc5427`.
- `A experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` — different; main blob `0671c097a9c73b8928cda57e689d37cf4669b68f`; head blob `43926fb26224f5d90de191f1d0b02b71db3f0e9f`.
- `A experiments/scg08_arithmetic_work/evidence_schema.json` — different; main blob `8a4a641dbe3fffb0d2f40b24e5a285f82eff0433`; head blob `1d28a4ae8858fb79dc4270c3bc53cce68cb4c283`.
- `A experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp` — identical; main blob `3f3ed8d8db561fa5e6d9c31c5822ad2b04a7d361`; head blob `3f3ed8d8db561fa5e6d9c31c5822ad2b04a7d361`.
- `A experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp` — different; main blob `0d9e7128642faab057544c14e8dd8276ccba3f88`; head blob `9b308c60a4bf9c15ad625e900d2445e58e99ec9d`.
- `A experiments/scg08_arithmetic_work/tier1_markov_harness.hpp` — identical; main blob `82dd21da5d9915071f169c719d46393b56e2424b`; head blob `82dd21da5d9915071f169c719d46393b56e2424b`.
- `A experiments/scg08_arithmetic_work/tier2_work_harness.hpp` — different; main blob `610999bdc334964922c8f3ae9f66f93be7c6944f`; head blob `5b2883119c512a397ef830848fc10b2a9051dbb0`.
- `M scripts/ci/validate_architecture_visualization.py` — different; main blob `208e90ebcc3c31100d0bed7aa62f7956f047d9d3`; head blob `1bf1834835e8fd21d28856ecc371e917f3f8791f`.
- `D tests/ci/test_architecture_dashboard.py` — deleted_head; main blob `ef0fae927b9db40e46dc6841c3921d6e43fca793`; head blob `None`.

Overlaps with other PR own deltas:

- #782: `CMakeLists.txt`
- #779: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`, `experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp`, `experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp`, `experiments/scg08_arithmetic_work/tier2_work_harness.hpp`
- #778: `experiments/scg08_arithmetic_work/CMakeLists.txt`, `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`, `experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp`, `experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp`, `experiments/scg08_arithmetic_work/tier2_work_harness.hpp`
- #774: `docs/architecture/visualization/DASHBOARD_GUIDE.md`, `docs/architecture/visualization/dashboard/dashboard.css`, `docs/architecture/visualization/dashboard/dashboard.js`, `docs/architecture/visualization/dashboard/index.html`, `scripts/ci/validate_architecture_visualization.py`, `tests/ci/test_architecture_dashboard.py`
- #769: `CMakeLists.txt`
- #766: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`, `experiments/scg08_arithmetic_work/evidence_schema.json`
- #764: `docs/architecture/visualization/pull_requests_snapshot.json`, `docs/architecture/visualization/research_programs.json`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`
- #747: `CMakeLists.txt`
- #649: `CMakeLists.txt`
- #603: `CMakeLists.txt`
- #556: `CMakeLists.txt`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #508: `CMakeLists.txt`

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `525554a193af6a3e37f8ab43b895a2a9ca231419` / [review](https://github.com/t81dev/t81-foundation/pull/760#pullrequestreview-5848617817).

Failed or incomplete checks:


## PR 752: docs(h04-r03): separate current retention status from historical snapshot

- Head `0d775fc0556805fe097b99c417116b0672116fee`; actual base `b18075ad6de93091f9aea30707ef5237d479c9d1` (`fix/ci-backlog-portability`); main merge-base `e5c30560aa77c31184a14b03ed152a6deef5c1a3`.
- Purpose: H04 retention historical-versus-current overlay. Decision: **REVIEW NEXT**.
- Dependency assessment: Stacks on superseded 747; references merged 724/727; no measurement authorization.
- Open heads included by ancestry: [747]; references in body (not necessarily dependencies): [724, 727, 728, 747].
- Evidence/limits: Only one own file. Historical body remains an exact suffix; bounded overlay preserves freeze and retention limits. Retarget to main; inherited 747 changes are already represented.
- Blob counts: {'identical': 25, 'different': 2}. Merge simulation exit 0; tree `1ad1aa2a20120ab5c98964f1df6c5fff4c75ec4c`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...0d775fc0556805fe097b99c417116b0672116fee).

Own changed files:

- `M docs/ai-context/H04_R03_ACCEPTANCE_FINDINGS_2026-11-06.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0d775fc0556805fe097b99c417116b0672116fee/docs/ai-context/H04_R03_ACCEPTANCE_FINDINGS_2026-11-06.md))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ci.yml` — identical; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `42e7c304cca285abf7eb287848a743ceb1c017bd`.
- `M .github/workflows/cross-platform-determinism.yml` — identical; main blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`; head blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`.
- `M .github/workflows/python-wheels.yml` — identical; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`.
- `M .github/workflows/release.yml` — identical; main blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`; head blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`.
- `M .github/workflows/research-commons.yml` — identical; main blob `0891289437125428139991616ccc8f261aab2687`; head blob `0891289437125428139991616ccc8f261aab2687`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `2ff6b17c8b92d4f3f582e7e2108764149d8cb8db`.
- `M benchmarks/BM_InferenceComparison.cpp` — identical; main blob `897d16b6d238b2743f21110d675ba0928bd0f74e`; head blob `897d16b6d238b2743f21110d675ba0928bd0f74e`.
- `M docs/ai-context/H04_R03_ACCEPTANCE_FINDINGS_2026-11-06.md` — different; main blob `d71280c797ab00f2a4902115567acfdc7f540ee0`; head blob `6fd256fd6e7a4c55cc33adb75cca37ce37fb7dbc`.
- `M examples/ai-and-inference/model-load-canonfs/make_answer_fixed_demo.cpp` — identical; main blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`; head blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`.
- `M examples/ai-and-inference/model-load-canonfs/make_assess_fixed_demo.cpp` — identical; main blob `92ea01441141d62883e4d785e85f1fb411adfdfe`; head blob `92ea01441141d62883e4d785e85f1fb411adfdfe`.
- `M examples/ai-and-inference/model-load-canonfs/make_classify_fixed_demo.cpp` — identical; main blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`; head blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`.
- `M examples/ai-and-inference/model-load-canonfs/make_degraded_llama_demo.cpp` — identical; main blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`; head blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`.
- `M examples/ai-and-inference/model-load-canonfs/make_guarded_llama_demo.cpp` — identical; main blob `c407b9311ca182c826d3277ae99af9648d083f2d`; head blob `c407b9311ca182c826d3277ae99af9648d083f2d`.
- `M examples/ai-and-inference/model-load-canonfs/make_route_fixed_demo.cpp` — identical; main blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`; head blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`.
- `M include/t81/types/T81Bytes.hpp` — identical; main blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`; head blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`.
- `M include/t81/types/T81IOStream.hpp` — identical; main blob `941659293d8105829f783d7b75db704df42ed80c`; head blob `941659293d8105829f783d7b75db704df42ed80c`.
- `M scripts/ci/check_stable_baseline_compliance.py` — identical; main blob `d9a31b196ef02d0c675897b24466489a6f580d31`; head blob `d9a31b196ef02d0c675897b24466489a6f580d31`.
- `M ternaryos/dev/gicv3.cpp` — identical; main blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`; head blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`.
- `M ternaryos/kernel/kernel_abi.hpp` — identical; main blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`; head blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`.
- `A tests/ci/test_stable_baseline_compliance.py` — identical; main blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`; head blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`.
- `M tests/cpp/dpe/epoch_dag_test.cpp` — identical; main blob `0513403f5646716e745d3c1b43264298b2c7d522`; head blob `0513403f5646716e745d3c1b43264298b2c7d522`.
- `M tests/cpp/dpe/epoch_parallel_test.cpp` — identical; main blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`; head blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`.
- `M tests/cpp/dpe/thread_pool_test.cpp` — identical; main blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`; head blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`.
- `M tests/cpp/vm_decoder_test.cpp` — identical; main blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`; head blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`.
- `M tools/cli/core/main.cpp` — identical; main blob `958dba8a20105a59adecd775bb6b642babedb282`; head blob `958dba8a20105a59adecd775bb6b642babedb282`.
- `M vm/ai_backend/controlled_ai_backend.cpp` — identical; main blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`; head blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`.
- `M vm/decoder.cpp` — identical; main blob `2982780e0783fb87b56c394af943257d12357595`; head blob `2982780e0783fb87b56c394af943257d12357595`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 751: fix(skills): constrain policy validation to literal argv

- Head `b68853335264d0758124a359f705fb5fbb8dfdc3`; actual base `1522f14aa1994d03af72441f2554718c9cfc11cb` (`fix/cli-execution-policy-reconciliation`); main merge-base `e5c30560aa77c31184a14b03ed152a6deef5c1a3`.
- Purpose: Literal-argv skill policy validation boundary. Decision: **REVIEW NEXT**.
- Dependency assessment: Stacks on 750 and 747; requires 750 CLI behavior, not just the shared CMake path.
- Open heads included by ancestry: [747, 750]; references in body (not necessarily dependencies): [614, 747, 750].
- Evidence/limits: Four own files; earlier local receipts are retained in prior audit, not rerun here. Review alongside 750, then retarget and execute policy-boundary tests under current source.
- Blob counts: {'identical': 24, 'different': 5, 'absent_main': 2}. Merge simulation exit 1; tree `14f5492e14fb7062a26b6cd5139ad1844dad1c91`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b68853335264d0758124a359f705fb5fbb8dfdc3).

Own changed files:

- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b68853335264d0758124a359f705fb5fbb8dfdc3/CMakeLists.txt))
- `M skills/policy-checker/SKILL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b68853335264d0758124a359f705fb5fbb8dfdc3/skills/policy-checker/SKILL.md))
- `M skills/policy-checker/validate-policy.sh` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b68853335264d0758124a359f705fb5fbb8dfdc3/skills/policy-checker/validate-policy.sh))
- `A tests/python/test_skill_policy_boundary.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b68853335264d0758124a359f705fb5fbb8dfdc3/tests/python/test_skill_policy_boundary.py))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ci.yml` — identical; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `42e7c304cca285abf7eb287848a743ceb1c017bd`.
- `M .github/workflows/cross-platform-determinism.yml` — identical; main blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`; head blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`.
- `M .github/workflows/python-wheels.yml` — identical; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`.
- `M .github/workflows/release.yml` — identical; main blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`; head blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`.
- `M .github/workflows/research-commons.yml` — identical; main blob `0891289437125428139991616ccc8f261aab2687`; head blob `0891289437125428139991616ccc8f261aab2687`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `66c125ec0aac1d62d84881cf2b3f22c3e46673c6`.
- `M benchmarks/BM_InferenceComparison.cpp` — identical; main blob `897d16b6d238b2743f21110d675ba0928bd0f74e`; head blob `897d16b6d238b2743f21110d675ba0928bd0f74e`.
- `M examples/ai-and-inference/model-load-canonfs/make_answer_fixed_demo.cpp` — identical; main blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`; head blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`.
- `M examples/ai-and-inference/model-load-canonfs/make_assess_fixed_demo.cpp` — identical; main blob `92ea01441141d62883e4d785e85f1fb411adfdfe`; head blob `92ea01441141d62883e4d785e85f1fb411adfdfe`.
- `M examples/ai-and-inference/model-load-canonfs/make_classify_fixed_demo.cpp` — identical; main blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`; head blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`.
- `M examples/ai-and-inference/model-load-canonfs/make_degraded_llama_demo.cpp` — identical; main blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`; head blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`.
- `M examples/ai-and-inference/model-load-canonfs/make_guarded_llama_demo.cpp` — identical; main blob `c407b9311ca182c826d3277ae99af9648d083f2d`; head blob `c407b9311ca182c826d3277ae99af9648d083f2d`.
- `M examples/ai-and-inference/model-load-canonfs/make_route_fixed_demo.cpp` — identical; main blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`; head blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`.
- `M include/t81/types/T81Bytes.hpp` — identical; main blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`; head blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`.
- `M include/t81/types/T81IOStream.hpp` — identical; main blob `941659293d8105829f783d7b75db704df42ed80c`; head blob `941659293d8105829f783d7b75db704df42ed80c`.
- `M scripts/ci/check_stable_baseline_compliance.py` — identical; main blob `d9a31b196ef02d0c675897b24466489a6f580d31`; head blob `d9a31b196ef02d0c675897b24466489a6f580d31`.
- `M skills/policy-checker/SKILL.md` — different; main blob `20f175ec05b7946744d5c005b4d141684dac2752`; head blob `4b44c66e1e57b7b986154e2bade959605aa8395e`.
- `M skills/policy-checker/validate-policy.sh` — different; main blob `58aea5a0f81fbd97ed4261b85b8f06326ef8e230`; head blob `e86c25f632d3cf45b3aa00fc1228018f6487ae95`.
- `M ternaryos/dev/gicv3.cpp` — identical; main blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`; head blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`.
- `M ternaryos/kernel/kernel_abi.hpp` — identical; main blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`; head blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`.
- `A tests/ci/test_stable_baseline_compliance.py` — identical; main blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`; head blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`.
- `M tests/cpp/dpe/epoch_dag_test.cpp` — identical; main blob `0513403f5646716e745d3c1b43264298b2c7d522`; head blob `0513403f5646716e745d3c1b43264298b2c7d522`.
- `M tests/cpp/dpe/epoch_parallel_test.cpp` — identical; main blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`; head blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`.
- `M tests/cpp/dpe/thread_pool_test.cpp` — identical; main blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`; head blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`.
- `M tests/cpp/vm_decoder_test.cpp` — identical; main blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`; head blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`.
- `A tests/python/test_cli_execution_policy.py` — absent_main; main blob `None`; head blob `bdd23593ac2b614d6233137351d6295869c7c522`.
- `A tests/python/test_skill_policy_boundary.py` — absent_main; main blob `None`; head blob `1b8c37c9421b280cdf6852accb474c333685ad62`.
- `M tools/cli/core/driver.cpp` — different; main blob `2df57428cfb0d840d6e43d204b72bdeefe18bdfc`; head blob `757c5218f6422f59a14971c60f7aabbd7339e73a`.
- `M tools/cli/core/main.cpp` — different; main blob `958dba8a20105a59adecd775bb6b642babedb282`; head blob `c5f418245d0a3c7e8e0bda62c82efaffce544e8d`.
- `M vm/ai_backend/controlled_ai_backend.cpp` — identical; main blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`; head blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`.
- `M vm/decoder.cpp` — identical; main blob `2982780e0783fb87b56c394af943257d12357595`; head blob `2982780e0783fb87b56c394af943257d12357595`.

Overlaps with other PR own deltas:

- #782: `CMakeLists.txt`
- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #750: `CMakeLists.txt`
- #747: `CMakeLists.txt`
- #649: `CMakeLists.txt`
- #603: `CMakeLists.txt`
- #556: `CMakeLists.txt`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #508: `CMakeLists.txt`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 750: fix(cli): enforce explicitly requested execution policies

- Head `1522f14aa1994d03af72441f2554718c9cfc11cb`; actual base `b18075ad6de93091f9aea30707ef5237d479c9d1` (`fix/ci-backlog-portability`); main merge-base `e5c30560aa77c31184a14b03ed152a6deef5c1a3`.
- Purpose: Explicit CLI execution-policy enforcement. Decision: **REVIEW NEXT**.
- Dependency assessment: Stacks on superseded 747; functional parent of 751; coordinate with 782.
- Open heads included by ancestry: [747]; references in body (not necessarily dependencies): [613, 747].
- Evidence/limits: Four own files retain unique CLI behavior/tests. High-value policy-before-side-effect review; conflicts with current main and overlaps latest 782 driver/main changes.
- Blob counts: {'identical': 24, 'different': 3, 'absent_main': 1}. Merge simulation exit 1; tree `2cfb147e0f0ef83667c2920ff139b4f09fc5cfe6`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...1522f14aa1994d03af72441f2554718c9cfc11cb).

Own changed files:

- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/1522f14aa1994d03af72441f2554718c9cfc11cb/CMakeLists.txt))
- `A tests/python/test_cli_execution_policy.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/1522f14aa1994d03af72441f2554718c9cfc11cb/tests/python/test_cli_execution_policy.py))
- `M tools/cli/core/driver.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/1522f14aa1994d03af72441f2554718c9cfc11cb/tools/cli/core/driver.cpp))
- `M tools/cli/core/main.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/1522f14aa1994d03af72441f2554718c9cfc11cb/tools/cli/core/main.cpp))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ci.yml` — identical; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `42e7c304cca285abf7eb287848a743ceb1c017bd`.
- `M .github/workflows/cross-platform-determinism.yml` — identical; main blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`; head blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`.
- `M .github/workflows/python-wheels.yml` — identical; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`.
- `M .github/workflows/release.yml` — identical; main blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`; head blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`.
- `M .github/workflows/research-commons.yml` — identical; main blob `0891289437125428139991616ccc8f261aab2687`; head blob `0891289437125428139991616ccc8f261aab2687`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `8cba20198e72ddac245b6f96d8a367fb120f586b`.
- `M benchmarks/BM_InferenceComparison.cpp` — identical; main blob `897d16b6d238b2743f21110d675ba0928bd0f74e`; head blob `897d16b6d238b2743f21110d675ba0928bd0f74e`.
- `M examples/ai-and-inference/model-load-canonfs/make_answer_fixed_demo.cpp` — identical; main blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`; head blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`.
- `M examples/ai-and-inference/model-load-canonfs/make_assess_fixed_demo.cpp` — identical; main blob `92ea01441141d62883e4d785e85f1fb411adfdfe`; head blob `92ea01441141d62883e4d785e85f1fb411adfdfe`.
- `M examples/ai-and-inference/model-load-canonfs/make_classify_fixed_demo.cpp` — identical; main blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`; head blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`.
- `M examples/ai-and-inference/model-load-canonfs/make_degraded_llama_demo.cpp` — identical; main blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`; head blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`.
- `M examples/ai-and-inference/model-load-canonfs/make_guarded_llama_demo.cpp` — identical; main blob `c407b9311ca182c826d3277ae99af9648d083f2d`; head blob `c407b9311ca182c826d3277ae99af9648d083f2d`.
- `M examples/ai-and-inference/model-load-canonfs/make_route_fixed_demo.cpp` — identical; main blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`; head blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`.
- `M include/t81/types/T81Bytes.hpp` — identical; main blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`; head blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`.
- `M include/t81/types/T81IOStream.hpp` — identical; main blob `941659293d8105829f783d7b75db704df42ed80c`; head blob `941659293d8105829f783d7b75db704df42ed80c`.
- `M scripts/ci/check_stable_baseline_compliance.py` — identical; main blob `d9a31b196ef02d0c675897b24466489a6f580d31`; head blob `d9a31b196ef02d0c675897b24466489a6f580d31`.
- `M ternaryos/dev/gicv3.cpp` — identical; main blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`; head blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`.
- `M ternaryos/kernel/kernel_abi.hpp` — identical; main blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`; head blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`.
- `A tests/ci/test_stable_baseline_compliance.py` — identical; main blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`; head blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`.
- `M tests/cpp/dpe/epoch_dag_test.cpp` — identical; main blob `0513403f5646716e745d3c1b43264298b2c7d522`; head blob `0513403f5646716e745d3c1b43264298b2c7d522`.
- `M tests/cpp/dpe/epoch_parallel_test.cpp` — identical; main blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`; head blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`.
- `M tests/cpp/dpe/thread_pool_test.cpp` — identical; main blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`; head blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`.
- `M tests/cpp/vm_decoder_test.cpp` — identical; main blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`; head blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`.
- `A tests/python/test_cli_execution_policy.py` — absent_main; main blob `None`; head blob `bdd23593ac2b614d6233137351d6295869c7c522`.
- `M tools/cli/core/driver.cpp` — different; main blob `2df57428cfb0d840d6e43d204b72bdeefe18bdfc`; head blob `757c5218f6422f59a14971c60f7aabbd7339e73a`.
- `M tools/cli/core/main.cpp` — different; main blob `958dba8a20105a59adecd775bb6b642babedb282`; head blob `c5f418245d0a3c7e8e0bda62c82efaffce544e8d`.
- `M vm/ai_backend/controlled_ai_backend.cpp` — identical; main blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`; head blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`.
- `M vm/decoder.cpp` — identical; main blob `2982780e0783fb87b56c394af943257d12357595`; head blob `2982780e0783fb87b56c394af943257d12357595`.

Overlaps with other PR own deltas:

- #782: `CMakeLists.txt`, `tools/cli/core/driver.cpp`, `tools/cli/core/main.cpp`
- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #747: `CMakeLists.txt`, `tools/cli/core/main.cpp`
- #649: `CMakeLists.txt`
- #603: `CMakeLists.txt`, `tools/cli/core/main.cpp`
- #556: `CMakeLists.txt`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #508: `CMakeLists.txt`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 748: fix(canonfs): authorize block metadata before side effects

- Head `7a3cdf850a143758eb1355ebb8747368e6ca72cb`; actual base `b18075ad6de93091f9aea30707ef5237d479c9d1` (`fix/ci-backlog-portability`); main merge-base `e5c30560aa77c31184a14b03ed152a6deef5c1a3`.
- Purpose: CanonFS block metadata authorization before effects. Decision: **REVIEW NEXT**.
- Dependency assessment: Stacks on superseded 747; independent of 750/751; touches block_backed_driver and policy test.
- Open heads included by ancestry: [747]; references in body (not necessarily dependencies): [686, 747].
- Evidence/limits: Two own files remain distinct from main. Prior policy-boundary receipts exist; fresh source review and focused negative checks are needed after retargeting.
- Blob counts: {'identical': 25, 'different': 3}. Merge simulation exit 0; tree `1a00771d4cc819d70ff6190782a37046d14e52f6`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...7a3cdf850a143758eb1355ebb8747368e6ca72cb).

Own changed files:

- `M fs/block_backed_driver.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7a3cdf850a143758eb1355ebb8747368e6ca72cb/fs/block_backed_driver.cpp))
- `M tests/cpp/canonfs_policy_boundary_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7a3cdf850a143758eb1355ebb8747368e6ca72cb/tests/cpp/canonfs_policy_boundary_test.cpp))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ci.yml` — identical; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `42e7c304cca285abf7eb287848a743ceb1c017bd`.
- `M .github/workflows/cross-platform-determinism.yml` — identical; main blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`; head blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`.
- `M .github/workflows/python-wheels.yml` — identical; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`.
- `M .github/workflows/release.yml` — identical; main blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`; head blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`.
- `M .github/workflows/research-commons.yml` — identical; main blob `0891289437125428139991616ccc8f261aab2687`; head blob `0891289437125428139991616ccc8f261aab2687`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `2ff6b17c8b92d4f3f582e7e2108764149d8cb8db`.
- `M benchmarks/BM_InferenceComparison.cpp` — identical; main blob `897d16b6d238b2743f21110d675ba0928bd0f74e`; head blob `897d16b6d238b2743f21110d675ba0928bd0f74e`.
- `M examples/ai-and-inference/model-load-canonfs/make_answer_fixed_demo.cpp` — identical; main blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`; head blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`.
- `M examples/ai-and-inference/model-load-canonfs/make_assess_fixed_demo.cpp` — identical; main blob `92ea01441141d62883e4d785e85f1fb411adfdfe`; head blob `92ea01441141d62883e4d785e85f1fb411adfdfe`.
- `M examples/ai-and-inference/model-load-canonfs/make_classify_fixed_demo.cpp` — identical; main blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`; head blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`.
- `M examples/ai-and-inference/model-load-canonfs/make_degraded_llama_demo.cpp` — identical; main blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`; head blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`.
- `M examples/ai-and-inference/model-load-canonfs/make_guarded_llama_demo.cpp` — identical; main blob `c407b9311ca182c826d3277ae99af9648d083f2d`; head blob `c407b9311ca182c826d3277ae99af9648d083f2d`.
- `M examples/ai-and-inference/model-load-canonfs/make_route_fixed_demo.cpp` — identical; main blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`; head blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`.
- `M fs/block_backed_driver.cpp` — different; main blob `1b04d683847792169c2ca42d43adc81ea2130546`; head blob `3a3b00eb9bc08fcf7ad0a2b866598a3fbf79b4fc`.
- `M include/t81/types/T81Bytes.hpp` — identical; main blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`; head blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`.
- `M include/t81/types/T81IOStream.hpp` — identical; main blob `941659293d8105829f783d7b75db704df42ed80c`; head blob `941659293d8105829f783d7b75db704df42ed80c`.
- `M scripts/ci/check_stable_baseline_compliance.py` — identical; main blob `d9a31b196ef02d0c675897b24466489a6f580d31`; head blob `d9a31b196ef02d0c675897b24466489a6f580d31`.
- `M ternaryos/dev/gicv3.cpp` — identical; main blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`; head blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`.
- `M ternaryos/kernel/kernel_abi.hpp` — identical; main blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`; head blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`.
- `A tests/ci/test_stable_baseline_compliance.py` — identical; main blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`; head blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`.
- `M tests/cpp/canonfs_policy_boundary_test.cpp` — different; main blob `10a4b1c7dcd65c25169d54a045403c83ba870a60`; head blob `a9df4a89a86a836df8ed5f92d43ecdc396ba8571`.
- `M tests/cpp/dpe/epoch_dag_test.cpp` — identical; main blob `0513403f5646716e745d3c1b43264298b2c7d522`; head blob `0513403f5646716e745d3c1b43264298b2c7d522`.
- `M tests/cpp/dpe/epoch_parallel_test.cpp` — identical; main blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`; head blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`.
- `M tests/cpp/dpe/thread_pool_test.cpp` — identical; main blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`; head blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`.
- `M tests/cpp/vm_decoder_test.cpp` — identical; main blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`; head blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`.
- `M tools/cli/core/main.cpp` — identical; main blob `958dba8a20105a59adecd775bb6b642babedb282`; head blob `958dba8a20105a59adecd775bb6b642babedb282`.
- `M vm/ai_backend/controlled_ai_backend.cpp` — identical; main blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`; head blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`.
- `M vm/decoder.cpp` — identical; main blob `2982780e0783fb87b56c394af943257d12357595`; head blob `2982780e0783fb87b56c394af943257d12357595`.

Overlaps with other PR own deltas:

- #504: `fs/block_backed_driver.cpp`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 747: fix(ci): restore supported runners and unblock portability builds

- Head `b18075ad6de93091f9aea30707ef5237d479c9d1`; actual base `e5c30560aa77c31184a14b03ed152a6deef5c1a3` (`main`); main merge-base `e5c30560aa77c31184a14b03ed152a6deef5c1a3`.
- Purpose: CI runner/build portability repair. Decision: **SUPERSEDED**.
- Dependency assessment: Represented by merged 770/772; descendants 748/750/752 remain independently unique.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [641].
- Evidence/limits: 25 of 26 changed blobs already identical to main; remaining CMake additions are represented among newer registrations. Simulated merge is clean and exactly main tree: no surviving integration delta.
- Blob counts: {'identical': 25, 'different': 1}. Merge simulation exit 0; tree `fa633c105abfe229e77eabdccd2453c620636275`; equals main: True.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 38 pass, 3 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b18075ad6de93091f9aea30707ef5237d479c9d1).

Own changed files:

- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/.github/workflows/ci.yml))
- `M .github/workflows/cross-platform-determinism.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/.github/workflows/cross-platform-determinism.yml))
- `M .github/workflows/python-wheels.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/.github/workflows/python-wheels.yml))
- `M .github/workflows/release.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/.github/workflows/release.yml))
- `M .github/workflows/research-commons.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/.github/workflows/research-commons.yml))
- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/CMakeLists.txt))
- `M benchmarks/BM_InferenceComparison.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/benchmarks/BM_InferenceComparison.cpp))
- `M examples/ai-and-inference/model-load-canonfs/make_answer_fixed_demo.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/examples/ai-and-inference/model-load-canonfs/make_answer_fixed_demo.cpp))
- `M examples/ai-and-inference/model-load-canonfs/make_assess_fixed_demo.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/examples/ai-and-inference/model-load-canonfs/make_assess_fixed_demo.cpp))
- `M examples/ai-and-inference/model-load-canonfs/make_classify_fixed_demo.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/examples/ai-and-inference/model-load-canonfs/make_classify_fixed_demo.cpp))
- `M examples/ai-and-inference/model-load-canonfs/make_degraded_llama_demo.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/examples/ai-and-inference/model-load-canonfs/make_degraded_llama_demo.cpp))
- `M examples/ai-and-inference/model-load-canonfs/make_guarded_llama_demo.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/examples/ai-and-inference/model-load-canonfs/make_guarded_llama_demo.cpp))
- `M examples/ai-and-inference/model-load-canonfs/make_route_fixed_demo.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/examples/ai-and-inference/model-load-canonfs/make_route_fixed_demo.cpp))
- `M include/t81/types/T81Bytes.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/include/t81/types/T81Bytes.hpp))
- `M include/t81/types/T81IOStream.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/include/t81/types/T81IOStream.hpp))
- `M scripts/ci/check_stable_baseline_compliance.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/scripts/ci/check_stable_baseline_compliance.py))
- `M ternaryos/dev/gicv3.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/ternaryos/dev/gicv3.cpp))
- `M ternaryos/kernel/kernel_abi.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/ternaryos/kernel/kernel_abi.hpp))
- `A tests/ci/test_stable_baseline_compliance.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/tests/ci/test_stable_baseline_compliance.py))
- `M tests/cpp/dpe/epoch_dag_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/tests/cpp/dpe/epoch_dag_test.cpp))
- `M tests/cpp/dpe/epoch_parallel_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/tests/cpp/dpe/epoch_parallel_test.cpp))
- `M tests/cpp/dpe/thread_pool_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/tests/cpp/dpe/thread_pool_test.cpp))
- `M tests/cpp/vm_decoder_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/tests/cpp/vm_decoder_test.cpp))
- `M tools/cli/core/main.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/tools/cli/core/main.cpp))
- `M vm/ai_backend/controlled_ai_backend.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/vm/ai_backend/controlled_ai_backend.cpp))
- `M vm/decoder.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b18075ad6de93091f9aea30707ef5237d479c9d1/vm/decoder.cpp))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ci.yml` — identical; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `42e7c304cca285abf7eb287848a743ceb1c017bd`.
- `M .github/workflows/cross-platform-determinism.yml` — identical; main blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`; head blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`.
- `M .github/workflows/python-wheels.yml` — identical; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`.
- `M .github/workflows/release.yml` — identical; main blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`; head blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`.
- `M .github/workflows/research-commons.yml` — identical; main blob `0891289437125428139991616ccc8f261aab2687`; head blob `0891289437125428139991616ccc8f261aab2687`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `2ff6b17c8b92d4f3f582e7e2108764149d8cb8db`.
- `M benchmarks/BM_InferenceComparison.cpp` — identical; main blob `897d16b6d238b2743f21110d675ba0928bd0f74e`; head blob `897d16b6d238b2743f21110d675ba0928bd0f74e`.
- `M examples/ai-and-inference/model-load-canonfs/make_answer_fixed_demo.cpp` — identical; main blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`; head blob `fa550680100bfc61abdefb60a7ff2c559a7e0073`.
- `M examples/ai-and-inference/model-load-canonfs/make_assess_fixed_demo.cpp` — identical; main blob `92ea01441141d62883e4d785e85f1fb411adfdfe`; head blob `92ea01441141d62883e4d785e85f1fb411adfdfe`.
- `M examples/ai-and-inference/model-load-canonfs/make_classify_fixed_demo.cpp` — identical; main blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`; head blob `73069d6ab40a3470343a1f11bd0cd4cae512d59c`.
- `M examples/ai-and-inference/model-load-canonfs/make_degraded_llama_demo.cpp` — identical; main blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`; head blob `45a47d5cf887afd1714b56bfe44b631c0e54730c`.
- `M examples/ai-and-inference/model-load-canonfs/make_guarded_llama_demo.cpp` — identical; main blob `c407b9311ca182c826d3277ae99af9648d083f2d`; head blob `c407b9311ca182c826d3277ae99af9648d083f2d`.
- `M examples/ai-and-inference/model-load-canonfs/make_route_fixed_demo.cpp` — identical; main blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`; head blob `e244707fa5e693717f7eb0c9c1ce5f9953fd048b`.
- `M include/t81/types/T81Bytes.hpp` — identical; main blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`; head blob `16e9ad6f4d849c0ef9cef4111fc48a6588dd3f9b`.
- `M include/t81/types/T81IOStream.hpp` — identical; main blob `941659293d8105829f783d7b75db704df42ed80c`; head blob `941659293d8105829f783d7b75db704df42ed80c`.
- `M scripts/ci/check_stable_baseline_compliance.py` — identical; main blob `d9a31b196ef02d0c675897b24466489a6f580d31`; head blob `d9a31b196ef02d0c675897b24466489a6f580d31`.
- `M ternaryos/dev/gicv3.cpp` — identical; main blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`; head blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`.
- `M ternaryos/kernel/kernel_abi.hpp` — identical; main blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`; head blob `237421ea31a4e6d41f8d9ac1e33a1dbe936cb5c7`.
- `A tests/ci/test_stable_baseline_compliance.py` — identical; main blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`; head blob `53b6295e263d2660dffe68cb3b9cfd99638a6fc2`.
- `M tests/cpp/dpe/epoch_dag_test.cpp` — identical; main blob `0513403f5646716e745d3c1b43264298b2c7d522`; head blob `0513403f5646716e745d3c1b43264298b2c7d522`.
- `M tests/cpp/dpe/epoch_parallel_test.cpp` — identical; main blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`; head blob `567d2b57608f6f1efe2d91c4e0b920c88c2f14a8`.
- `M tests/cpp/dpe/thread_pool_test.cpp` — identical; main blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`; head blob `799a0d1bb53bf53e2b607a26bcfd4d47c2d092c4`.
- `M tests/cpp/vm_decoder_test.cpp` — identical; main blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`; head blob `ae03e6f8ee4e39479141e9170641affc4a5c24f8`.
- `M tools/cli/core/main.cpp` — identical; main blob `958dba8a20105a59adecd775bb6b642babedb282`; head blob `958dba8a20105a59adecd775bb6b642babedb282`.
- `M vm/ai_backend/controlled_ai_backend.cpp` — identical; main blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`; head blob `66ed3649563d5a0a2c4d77cc8ce786e6dcf5c8d1`.
- `M vm/decoder.cpp` — identical; main blob `2982780e0783fb87b56c394af943257d12357595`; head blob `2982780e0783fb87b56c394af943257d12357595`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`, `CMakeLists.txt`, `tools/cli/core/main.cpp`
- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`, `tools/cli/core/main.cpp`
- #649: `.github/workflows/ci.yml`, `.github/workflows/python-wheels.yml`, `CMakeLists.txt`
- #616: `.github/workflows/research-commons.yml`
- #603: `CMakeLists.txt`, `tools/cli/core/main.cpp`, `vm/decoder.cpp`
- #577: `.github/workflows/ci.yml`, `.github/workflows/python-wheels.yml`
- #576: `.github/workflows/ci.yml`
- #575: `.github/workflows/ci.yml`
- #574: `.github/workflows/ci.yml`
- #556: `.github/workflows/cross-platform-determinism.yml`, `CMakeLists.txt`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #525: `.github/workflows/ci.yml`, `.github/workflows/cross-platform-determinism.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/release.yml`
- #524: `.github/workflows/release.yml`
- #514: `.github/workflows/ci.yml`
- #513: `.github/workflows/ci.yml`, `.github/workflows/cross-platform-determinism.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/release.yml`
- #508: `CMakeLists.txt`, `ternaryos/dev/gicv3.cpp`
- #507: `ternaryos/dev/gicv3.cpp`
- #504: `.github/workflows/ci.yml`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/42838121136/job/129927470146): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/42838121136/job/129908634275): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/42838121136/job/129908634222): completed / failure.

## PR 732: docs(continuity): SCG-05 generic Result contract feasibility assessment

- Head `69b3b29447d827b50347fd4e2d3a140d9c174fcb`; actual base `e45c5435aa47d4ccb8f343798214eb917352c871` (`main`); main merge-base `e45c5435aa47d4ccb8f343798214eb917352c871`.
- Purpose: Generic Result feasibility assessment. Decision: **DEFER**.
- Dependency assessment: Related to proposed SDK transition 650 and authority decisions; no code implementation in this PR.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Read-only proposal, not API migration authorization. Prior inventory-count and ABI/compiler qualifications remain review limits; no container redesign should start from this report alone.
- Blob counts: {'different': 2, 'absent_main': 1}. Merge simulation exit 0; tree `6aaae4003e7577935abe93baf9675b2cb9a2cd57`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 26 pass, 10 fail, 8 skip, 4 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...69b3b29447d827b50347fd4e2d3a140d9c174fcb).

Own changed files:

- `M docs/research/continuity/LINEAGE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/69b3b29447d827b50347fd4e2d3a140d9c174fcb/docs/research/continuity/LINEAGE.md))
- `M docs/research/continuity/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/69b3b29447d827b50347fd4e2d3a140d9c174fcb/docs/research/continuity/README.md))
- `A docs/research/continuity/SCG05_GENERIC_RESULT_FEASIBILITY_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/69b3b29447d827b50347fd4e2d3a140d9c174fcb/docs/research/continuity/SCG05_GENERIC_RESULT_FEASIBILITY_REPORT.md))

All changed files versus main merge-base (including parents):

- `M docs/research/continuity/LINEAGE.md` — different; main blob `3eaa07fb45555911d94d3493c4d8c5022593370c`; head blob `88b966c68b48c76395016cbb93eda9b66e527692`.
- `M docs/research/continuity/README.md` — different; main blob `edd073802cad54c1ed68079a6a8162b59d47e9a8`; head blob `9b7457740f5ff07abe705b308148cdac732ba04c`.
- `A docs/research/continuity/SCG05_GENERIC_RESULT_FEASIBILITY_REPORT.md` — absent_main; main blob `None`; head blob `a8d34c023be65325662d7be212cf7fc3bd5fff98`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128870956822): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128863567740): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128863567531): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128863567387): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128863567380): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128863567352): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128863567304): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128863567278): completed / cancelled.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128862468746): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128862468696): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128862468693): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/42523609451/job/128862468666): completed / cancelled.
- [discovery (macos-14, 3.9)](https://github.com/t81dev/t81-foundation/actions/runs/42523609428/job/128862225461): completed / cancelled.
- [discovery (macos-14, 3.13)](https://github.com/t81dev/t81-foundation/actions/runs/42523609428/job/128862225415): completed / cancelled.

## PR 719: docs(arch003): resolve six review findings for R2 proposal

- Head `f9dd69d3de30c3816f4aae652531da97d9fac68a`; actual base `9654fd2b05f0abd1af4f0f3d14318d5213b7c8c8` (`main`); main merge-base `9654fd2b05f0abd1af4f0f3d14318d5213b7c8c8`.
- Purpose: ARCH-003 R2 deadline and receipt proposal. Decision: **REPAIR**.
- Dependency assessment: Competing proposal at same path as 718; depends on protocol/host authority for 714/716; no campaign authorized.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [718].
- Evidence/limits: Source samples host_published_boot_ns before hashing/signing/final fsync of required Stage-2 attestation. Specify terminal completion boundary and proof; old outdated threads are not treated as fresh reproductions.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `d50be040a28f91cbc3680a0a1bd98c2954de7c4c`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/3 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 10 fail, 8 skip, 2 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...f9dd69d3de30c3816f4aae652531da97d9fac68a).

Own changed files:

- `A docs/research/arch003/T81_ARCH_003_R2_PROPOSAL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f9dd69d3de30c3816f4aae652531da97d9fac68a/docs/research/arch003/T81_ARCH_003_R2_PROPOSAL.md))

All changed files versus main merge-base (including parents):

- `A docs/research/arch003/T81_ARCH_003_R2_PROPOSAL.md` — absent_main; main blob `None`; head blob `cab2b8bd50b22a58e415d1b0f33aa9f83642e520`.

Overlaps with other PR own deltas:

- #718: `docs/research/arch003/T81_ARCH_003_R2_PROPOSAL.md`

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `4c590c1e60b3b538e1a515bbc443412f4950845f` / [review](https://github.com/t81dev/t81-foundation/pull/719#pullrequestreview-5808976063).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128278160393): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128268798459): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128268798320): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128268798317): completed / cancelled.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128268798314): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128268798301): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128268798271): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128268798263): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128264052287): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128264052286): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128264052199): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/42324997301/job/128264052186): completed / cancelled.

## PR 718: docs(arch003): propose R2 external deadline authority amendment

- Head `c55b53a0d43a4d76e1aab1efe806ebfed175a99f`; actual base `9654fd2b05f0abd1af4f0f3d14318d5213b7c8c8` (`main`); main merge-base `9654fd2b05f0abd1af4f0f3d14318d5213b7c8c8`.
- Purpose: Earlier ARCH-003 external deadline proposal. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Variant of 719, related to 714/716; three old threads are outdated.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Same path as 719 but heads are not ancestor/descendant; 320 added/72 removed lines between versions. Select one after substantive clock/signature/completion comparison; no unconditional duplicate closure.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `e76606378877eb53530e812349826962d97d708a`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/3 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...c55b53a0d43a4d76e1aab1efe806ebfed175a99f).

Own changed files:

- `A docs/research/arch003/T81_ARCH_003_R2_PROPOSAL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/c55b53a0d43a4d76e1aab1efe806ebfed175a99f/docs/research/arch003/T81_ARCH_003_R2_PROPOSAL.md))

All changed files versus main merge-base (including parents):

- `A docs/research/arch003/T81_ARCH_003_R2_PROPOSAL.md` — absent_main; main blob `None`; head blob `c2f43605ed71a1a092ae34c4f48adf64c14f5afe`.

Overlaps with other PR own deltas:

- #719: `docs/research/arch003/T81_ARCH_003_R2_PROPOSAL.md`

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `ee98163e6c621f80509717be4c9cbdd87fbeb1bb` / [review](https://github.com/t81dev/t81-foundation/pull/718#pullrequestreview-5806576167).

Failed or incomplete checks:


## PR 716: ARCH-003-I2: Linux accounting closure with bounded lifecycle evidence

- Head `f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15`; actual base `1454bd63426d21686cfb5ef22dbaebc3c9cae94b` (`docs/t81-arch002-equality-protocol`); main merge-base `d134407ae1059c4d7f6a9418177e695e0e5d3b51`.
- Purpose: Bounded Linux accounting/lifecycle apparatus. Decision: **DEFER**.
- Dependency assessment: Stacks on 714; external-deadline alternatives 718/719 remain proposals.
- Open heads included by ancestry: [714]; references in body (not necessarily dependencies): [714].
- Evidence/limits: 180 own files plus three inherited protocol paths absent from main. Reported fixture accounting is not full campaign correctness, independent oracle acceptance, host qualification or exact-commit execution authorization.
- Blob counts: {'absent_main': 183}. Merge simulation exit 0; tree `98bdbbd116e1ea844ca9c8229005fb43bfbd510c`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15).

Own changed files:

- `A experiments/arch003/.gitignore` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/.gitignore))
- `A experiments/arch003/ACCOUNTING_REVIEW.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/ACCOUNTING_REVIEW.md))
- `A experiments/arch003/CHANGED_FILES.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/CHANGED_FILES.txt))
- `A experiments/arch003/CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/CMakeLists.txt))
- `A experiments/arch003/I2_CHANGED_FILES.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/I2_CHANGED_FILES.txt))
- `A experiments/arch003/IMPLEMENTATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/IMPLEMENTATION_REPORT.md))
- `A experiments/arch003/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/README.md))
- `A experiments/arch003/accounting.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/accounting.py))
- `A experiments/arch003/campaign.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/campaign.py))
- `A experiments/arch003/evidence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/evidence.py))
- `A experiments/arch003/finalization.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/finalization.py))
- `A experiments/arch003/include/arch003/adapters.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/include/arch003/adapters.hpp))
- `A experiments/arch003/include/arch003/contract.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/include/arch003/contract.hpp))
- `A experiments/arch003/include/arch003/entrypoints.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/include/arch003/entrypoints.hpp))
- `A experiments/arch003/include/arch003/evidence.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/include/arch003/evidence.hpp))
- `A experiments/arch003/include/arch003/schedule.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/include/arch003/schedule.hpp))
- `A experiments/arch003/linux_trace.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/linux_trace.py))
- `A experiments/arch003/src/adapters.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/adapters.cpp))
- `A experiments/arch003/src/admission.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/admission.cpp))
- `A experiments/arch003/src/count_bc2.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/count_bc2.cpp))
- `A experiments/arch003/src/count_byte.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/count_byte.cpp))
- `A experiments/arch003/src/count_native.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/count_native.cpp))
- `A experiments/arch003/src/count_planes.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/count_planes.cpp))
- `A experiments/arch003/src/count_pt5.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/count_pt5.cpp))
- `A experiments/arch003/src/count_tc2.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/count_tc2.cpp))
- `A experiments/arch003/src/oracle_histogram.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/oracle_histogram.cpp))
- `A experiments/arch003/src/oracle_main.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/oracle_main.cpp))
- `A experiments/arch003/src/oracle_scalar.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/oracle_scalar.cpp))
- `A experiments/arch003/src/schedule.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/schedule.cpp))
- `A experiments/arch003/src/sha256.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/sha256.cpp))
- `A experiments/arch003/src/sink.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/sink.cpp))
- `A experiments/arch003/src/wire.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/wire.cpp))
- `A experiments/arch003/src/worker.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/src/worker.cpp))
- `A experiments/arch003/tests/accounting_fixture.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/tests/accounting_fixture.cpp))
- `A experiments/arch003/tests/component_checks.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/tests/component_checks.cpp))
- `A experiments/arch003/tests/sanitizer_probe.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/tests/sanitizer_probe.cpp))
- `A experiments/arch003/tests/test_accounting.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/tests/test_accounting.py))
- `A experiments/arch003/tests/test_accounting_linux.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/tests/test_accounting_linux.py))
- `A experiments/arch003/tests/test_oracles.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/tests/test_oracles.py))
- `A experiments/arch003/tests/test_verifier.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/tests/test_verifier.py))
- `A experiments/arch003/validation/D-binaries.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-binaries.json))
- `A experiments/arch003/validation/D-build.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-build.stderr))
- `A experiments/arch003/validation/D-build.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-build.stdout))
- `A experiments/arch003/validation/D-configure.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-configure.stderr))
- `A experiments/arch003/validation/D-configure.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-configure.stdout))
- `A experiments/arch003/validation/D-ctest.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-ctest.stderr))
- `A experiments/arch003/validation/D-ctest.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-ctest.stdout))
- `A experiments/arch003/validation/D-flags.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-flags.txt))
- `A experiments/arch003/validation/D-python.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-python.stderr))
- `A experiments/arch003/validation/D-python.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/D-python.stdout))
- `A experiments/arch003/validation/R-binaries.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-binaries.json))
- `A experiments/arch003/validation/R-build.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-build.stderr))
- `A experiments/arch003/validation/R-build.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-build.stdout))
- `A experiments/arch003/validation/R-configure.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-configure.stderr))
- `A experiments/arch003/validation/R-configure.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-configure.stdout))
- `A experiments/arch003/validation/R-ctest.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-ctest.stderr))
- `A experiments/arch003/validation/R-ctest.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-ctest.stdout))
- `A experiments/arch003/validation/R-flags.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-flags.txt))
- `A experiments/arch003/validation/R-python.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-python.stderr))
- `A experiments/arch003/validation/R-python.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/R-python.stdout))
- `A experiments/arch003/validation/S-binaries.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-binaries.json))
- `A experiments/arch003/validation/S-build.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-build.stderr))
- `A experiments/arch003/validation/S-build.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-build.stdout))
- `A experiments/arch003/validation/S-configure.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-configure.stderr))
- `A experiments/arch003/validation/S-configure.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-configure.stdout))
- `A experiments/arch003/validation/S-ctest.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-ctest.stderr))
- `A experiments/arch003/validation/S-ctest.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-ctest.stdout))
- `A experiments/arch003/validation/S-flags.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-flags.txt))
- `A experiments/arch003/validation/S-python.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-python.stderr))
- `A experiments/arch003/validation/S-python.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/S-python.stdout))
- `A experiments/arch003/validation/byte-assembly-R.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/byte-assembly-R.stderr))
- `A experiments/arch003/validation/byte-assembly-R.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/byte-assembly-R.stdout))
- `A experiments/arch003/validation/cmake-version.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/cmake-version.stderr))
- `A experiments/arch003/validation/cmake-version.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/cmake-version.stdout))
- `A experiments/arch003/validation/compiler-version.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/compiler-version.stderr))
- `A experiments/arch003/validation/compiler-version.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/compiler-version.stdout))
- `A experiments/arch003/validation/components.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/components.json))
- `A experiments/arch003/validation/i2/artifact-inventory.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/artifact-inventory.json))
- `A experiments/arch003/validation/i2/first-failed/first-fixtures.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/first-fixtures.stderr))
- `A experiments/arch003/validation/i2/first-failed/first-fixtures.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/first-fixtures.stdout))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_abnormal_signal_reaped_with_peak.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_abnormal_signal_reaped_with_peak.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_actual_L_V_W_fixed_frame.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_actual_L_V_W_fixed_frame.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_actual_driver_and_linker_receipts.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_actual_driver_and_linker_receipts.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_deadline_during_seal_removes_success_root.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_deadline_during_seal_removes_success_root.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_failure_and_prefix_survive_deadline_tree_termination.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_failure_and_prefix_survive_deadline_tree_termination.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_finalization_and_seal_inside_limits.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_finalization_and_seal_inside_limits.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_metadata_cannot_spawn_worker.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_metadata_cannot_spawn_worker.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_peak_survives_free_then_exec.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_peak_survives_free_then_exec.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_pid_identity_race_mid_job.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_pid_identity_race_mid_job.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_sampling_gap_while_live.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_sampling_gap_while_live.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_short_tool_between_rss_samples.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_short_tool_between_rss_samples.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_unavailable_proc_mid_job.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_unavailable_proc_mid_job.json))
- `A experiments/arch003/validation/i2/first-failed/receipts/test_unexpected_nested_descendant.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/receipts/test_unexpected_nested_descendant.json))
- `A experiments/arch003/validation/i2/first-failed/source.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/first-failed/source.tar.gz))
- `A experiments/arch003/validation/i2/fixture-setup-failed/reviewed-fixtures.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/fixture-setup-failed/reviewed-fixtures.stderr))
- `A experiments/arch003/validation/i2/fixture-setup-failed/reviewed-fixtures.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/fixture-setup-failed/reviewed-fixtures.stdout))
- `A experiments/arch003/validation/i2/fixture-setup-failed/source-current.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/fixture-setup-failed/source-current.tar.gz))
- `A experiments/arch003/validation/i2/linux/compiler.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/compiler.stderr))
- `A experiments/arch003/validation/i2/linux/compiler.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/compiler.stdout))
- `A experiments/arch003/validation/i2/linux/fixtures.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/fixtures.stderr))
- `A experiments/arch003/validation/i2/linux/fixtures.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/fixtures.stdout))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_abnormal_signal_reaped_with_peak.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_abnormal_signal_reaped_with_peak.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_actual_L_V_W_fixed_frame.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_actual_L_V_W_fixed_frame.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_actual_driver_and_linker_receipts.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_actual_driver_and_linker_receipts.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_deadline_during_seal_removes_success_root.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_deadline_during_seal_removes_success_root.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_failure_and_prefix_survive_accounting_tree_termination.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_failure_and_prefix_survive_accounting_tree_termination.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_failure_and_prefix_survive_deadline_tree_termination.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_failure_and_prefix_survive_deadline_tree_termination.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_finalization_and_seal_inside_limits.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_finalization_and_seal_inside_limits.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_metadata_cannot_spawn_worker.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_metadata_cannot_spawn_worker.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_nonleader_exec_blocks_before_pid_transition.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_nonleader_exec_blocks_before_pid_transition.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_output_cap_during_finalization_preserves_failure.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_output_cap_during_finalization_preserves_failure.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_peak_survives_free_then_exec.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_peak_survives_free_then_exec.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_pid_identity_race_mid_job.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_pid_identity_race_mid_job.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_ptrace_options_unavailable_is_reaped.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_ptrace_options_unavailable_is_reaped.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_runtime_maintenance_thread_accounted_once.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_runtime_maintenance_thread_accounted_once.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_sampling_gap_while_live.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_sampling_gap_while_live.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_short_tool_between_rss_samples.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_short_tool_between_rss_samples.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_unavailable_exit_peak_is_null_not_zero.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_unavailable_exit_peak_is_null_not_zero.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_unavailable_proc_at_launch_is_reaped.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_unavailable_proc_at_launch_is_reaped.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_unavailable_proc_mid_job.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_unavailable_proc_mid_job.json))
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_unexpected_nested_descendant.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/i2-recorded/test_unexpected_nested_descendant.json))
- `A experiments/arch003/validation/i2/linux/identities.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/identities.stderr))
- `A experiments/arch003/validation/i2/linux/identities.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/identities.stdout))
- `A experiments/arch003/validation/i2/linux/kernel.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/kernel.stderr))
- `A experiments/arch003/validation/i2/linux/kernel.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/kernel.stdout))
- `A experiments/arch003/validation/i2/linux/linker.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/linker.stderr))
- `A experiments/arch003/validation/i2/linux/linker.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/linker.stdout))
- `A experiments/arch003/validation/i2/linux/packages.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/packages.stderr))
- `A experiments/arch003/validation/i2/linux/packages.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/packages.stdout))
- `A experiments/arch003/validation/i2/linux/python.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/python.stderr))
- `A experiments/arch003/validation/i2/linux/python.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/python.stdout))
- `A experiments/arch003/validation/i2/linux/run.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/run.json))
- `A experiments/arch003/validation/i2/linux/source-transport-cleanup.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/source-transport-cleanup.stderr))
- `A experiments/arch003/validation/i2/linux/source-transport-cleanup.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/linux/source-transport-cleanup.stdout))
- `A experiments/arch003/validation/i2/macos/D-binaries.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-binaries.json))
- `A experiments/arch003/validation/i2/macos/D-build.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-build.stderr))
- `A experiments/arch003/validation/i2/macos/D-build.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-build.stdout))
- `A experiments/arch003/validation/i2/macos/D-configure.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-configure.stderr))
- `A experiments/arch003/validation/i2/macos/D-configure.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-configure.stdout))
- `A experiments/arch003/validation/i2/macos/D-ctest.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-ctest.stderr))
- `A experiments/arch003/validation/i2/macos/D-ctest.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-ctest.stdout))
- `A experiments/arch003/validation/i2/macos/D-flags.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-flags.txt))
- `A experiments/arch003/validation/i2/macos/D-python.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-python.stderr))
- `A experiments/arch003/validation/i2/macos/D-python.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/D-python.stdout))
- `A experiments/arch003/validation/i2/macos/R-binaries.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-binaries.json))
- `A experiments/arch003/validation/i2/macos/R-build.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-build.stderr))
- `A experiments/arch003/validation/i2/macos/R-build.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-build.stdout))
- `A experiments/arch003/validation/i2/macos/R-configure.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-configure.stderr))
- `A experiments/arch003/validation/i2/macos/R-configure.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-configure.stdout))
- `A experiments/arch003/validation/i2/macos/R-ctest.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-ctest.stderr))
- `A experiments/arch003/validation/i2/macos/R-ctest.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-ctest.stdout))
- `A experiments/arch003/validation/i2/macos/R-flags.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-flags.txt))
- `A experiments/arch003/validation/i2/macos/R-python.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-python.stderr))
- `A experiments/arch003/validation/i2/macos/R-python.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/R-python.stdout))
- `A experiments/arch003/validation/i2/macos/S-binaries.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-binaries.json))
- `A experiments/arch003/validation/i2/macos/S-build.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-build.stderr))
- `A experiments/arch003/validation/i2/macos/S-build.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-build.stdout))
- `A experiments/arch003/validation/i2/macos/S-configure.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-configure.stderr))
- `A experiments/arch003/validation/i2/macos/S-configure.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-configure.stdout))
- `A experiments/arch003/validation/i2/macos/S-ctest.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-ctest.stderr))
- `A experiments/arch003/validation/i2/macos/S-ctest.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-ctest.stdout))
- `A experiments/arch003/validation/i2/macos/S-flags.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-flags.txt))
- `A experiments/arch003/validation/i2/macos/S-python.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-python.stderr))
- `A experiments/arch003/validation/i2/macos/S-python.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/S-python.stdout))
- `A experiments/arch003/validation/i2/macos/byte-assembly-R.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/byte-assembly-R.stderr))
- `A experiments/arch003/validation/i2/macos/byte-assembly-R.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/byte-assembly-R.stdout))
- `A experiments/arch003/validation/i2/macos/cmake-version.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/cmake-version.stderr))
- `A experiments/arch003/validation/i2/macos/cmake-version.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/cmake-version.stdout))
- `A experiments/arch003/validation/i2/macos/compiler-version.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/compiler-version.stderr))
- `A experiments/arch003/validation/i2/macos/compiler-version.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/compiler-version.stdout))
- `A experiments/arch003/validation/i2/macos/components.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/components.json))
- `A experiments/arch003/validation/i2/macos/symbols-histogram.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/symbols-histogram.stderr))
- `A experiments/arch003/validation/i2/macos/symbols-histogram.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/symbols-histogram.stdout))
- `A experiments/arch003/validation/i2/macos/symbols-scalar.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/symbols-scalar.stderr))
- `A experiments/arch003/validation/i2/macos/symbols-scalar.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/i2/macos/symbols-scalar.stdout))
- `A experiments/arch003/validation/symbols-histogram.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/symbols-histogram.stderr))
- `A experiments/arch003/validation/symbols-histogram.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/symbols-histogram.stdout))
- `A experiments/arch003/validation/symbols-scalar.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/symbols-scalar.stderr))
- `A experiments/arch003/validation/symbols-scalar.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/validation/symbols-scalar.stdout))
- `A experiments/arch003/verifier.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15/experiments/arch003/verifier.py))

All changed files versus main merge-base (including parents):

- `A docs/research/arch002/PROTOCOL.md` — absent_main; main blob `None`; head blob `3d86cf749841388c50a14316cb6dfe40296181c1`.
- `A docs/research/arch002/VALIDATION.md` — absent_main; main blob `None`; head blob `00443b5dc759168aeeb1b663c726366f4446dc38`.
- `A docs/research/arch002/proposal.json` — absent_main; main blob `None`; head blob `0aa53b9fdf7620f50a4dc23172410777421b1cb1`.
- `A experiments/arch003/.gitignore` — absent_main; main blob `None`; head blob `7bf087a49c9294e2fc69663e6f04e146201eff10`.
- `A experiments/arch003/ACCOUNTING_REVIEW.md` — absent_main; main blob `None`; head blob `023a3578fd44bd440cd1c6d3eb4d7cc8bf13e6fd`.
- `A experiments/arch003/CHANGED_FILES.txt` — absent_main; main blob `None`; head blob `5052506a837ef14285e299901a7b545d3aae8520`.
- `A experiments/arch003/CMakeLists.txt` — absent_main; main blob `None`; head blob `658c2b136fd132ad520e68c21656cf70f643939e`.
- `A experiments/arch003/I2_CHANGED_FILES.txt` — absent_main; main blob `None`; head blob `c518fd92f466cbff4e79fdd70723d05029912ab0`.
- `A experiments/arch003/IMPLEMENTATION_REPORT.md` — absent_main; main blob `None`; head blob `46e821fe56e9cb7a917b8bcd92a62eb5957d7586`.
- `A experiments/arch003/README.md` — absent_main; main blob `None`; head blob `2118f16031ca67f9f23ef082489eabb3061dea61`.
- `A experiments/arch003/accounting.py` — absent_main; main blob `None`; head blob `a1a6ca8fd694dd31dc6f98478964111ebd02c3b5`.
- `A experiments/arch003/campaign.py` — absent_main; main blob `None`; head blob `4695596d05b9899335309a4c63b911f693db66d4`.
- `A experiments/arch003/evidence.py` — absent_main; main blob `None`; head blob `8323220aa4fb45c086334e7cb08ef5e81ec8d9d5`.
- `A experiments/arch003/finalization.py` — absent_main; main blob `None`; head blob `8df2eb34b98f119f03a1c0ec9e45f05732ac67e4`.
- `A experiments/arch003/include/arch003/adapters.hpp` — absent_main; main blob `None`; head blob `ebb4c2d40945addc15eb24f6f7f8d2aab817a3a2`.
- `A experiments/arch003/include/arch003/contract.hpp` — absent_main; main blob `None`; head blob `a99f40215c72f76d746f9e4d7d9f3510b3431dbd`.
- `A experiments/arch003/include/arch003/entrypoints.hpp` — absent_main; main blob `None`; head blob `5b31f3edf21139331c732e452f1d367a0c1bda81`.
- `A experiments/arch003/include/arch003/evidence.hpp` — absent_main; main blob `None`; head blob `e1f8a62b136f30a2a8e12042e6f8d7a1a621bbfc`.
- `A experiments/arch003/include/arch003/schedule.hpp` — absent_main; main blob `None`; head blob `01a18adb9674fa6fd74d696987113771215c6d5a`.
- `A experiments/arch003/linux_trace.py` — absent_main; main blob `None`; head blob `3405bcc8ff388b5e6c2509050171dd9d00748e88`.
- `A experiments/arch003/src/adapters.cpp` — absent_main; main blob `None`; head blob `c2033c4307a1051fdff7aa668b811b288cf08505`.
- `A experiments/arch003/src/admission.cpp` — absent_main; main blob `None`; head blob `4fc3968d63f01fbd94fe00eda8b70ed6a9beec14`.
- `A experiments/arch003/src/count_bc2.cpp` — absent_main; main blob `None`; head blob `8faf543b0ccf54136f10aeaf74b2ee4b00ebc486`.
- `A experiments/arch003/src/count_byte.cpp` — absent_main; main blob `None`; head blob `69c0b2b12d34c8d2927ec78cbda728493513023a`.
- `A experiments/arch003/src/count_native.cpp` — absent_main; main blob `None`; head blob `2e018089d6ce50a429740c039e7484b869af9e1f`.
- `A experiments/arch003/src/count_planes.cpp` — absent_main; main blob `None`; head blob `fca47667fbbee83a72b01d40aecfc0a1c3eee7b1`.
- `A experiments/arch003/src/count_pt5.cpp` — absent_main; main blob `None`; head blob `5a365ecbfb34c1a7a4f67ee1620533c46128a3ac`.
- `A experiments/arch003/src/count_tc2.cpp` — absent_main; main blob `None`; head blob `f4dec1062ee2190319340b19326ffff8798fb80f`.
- `A experiments/arch003/src/oracle_histogram.cpp` — absent_main; main blob `None`; head blob `08755b5c0f93c3ac59510a9d02ba86626edc66b0`.
- `A experiments/arch003/src/oracle_main.cpp` — absent_main; main blob `None`; head blob `7a7acb851a565df1d8ce9ab74e0fe574b7bcaa99`.
- `A experiments/arch003/src/oracle_scalar.cpp` — absent_main; main blob `None`; head blob `fa1fec3819eeb42f408bf4a4d1436acbd05c69d8`.
- `A experiments/arch003/src/schedule.cpp` — absent_main; main blob `None`; head blob `48f3a78a1a5d076cb49002038429fefc7bcd4e33`.
- `A experiments/arch003/src/sha256.cpp` — absent_main; main blob `None`; head blob `e4223345bd71ecbc317103f19d1763309843e064`.
- `A experiments/arch003/src/sink.cpp` — absent_main; main blob `None`; head blob `c3161dc54b7b3f9fcbf553d0809a33c2ad68b9a6`.
- `A experiments/arch003/src/wire.cpp` — absent_main; main blob `None`; head blob `e9dc924f8bff955cded4ae2044f0160facbe4667`.
- `A experiments/arch003/src/worker.cpp` — absent_main; main blob `None`; head blob `0bec8a6abd359c19dbd5b69672c3d9d3b5adeb5b`.
- `A experiments/arch003/tests/accounting_fixture.cpp` — absent_main; main blob `None`; head blob `b1b8e89af975690d000ac3f2097ff39a7865a5e3`.
- `A experiments/arch003/tests/component_checks.cpp` — absent_main; main blob `None`; head blob `47f71a1dcd6fde3fd7fea4ede3ce2c7557494292`.
- `A experiments/arch003/tests/sanitizer_probe.cpp` — absent_main; main blob `None`; head blob `ad1988f4412b721f29303d92aa27e71d9e8ee00e`.
- `A experiments/arch003/tests/test_accounting.py` — absent_main; main blob `None`; head blob `719c08cc5b97048e756499edf769c1defc0117d6`.
- `A experiments/arch003/tests/test_accounting_linux.py` — absent_main; main blob `None`; head blob `288c2c46740e63167d4b99685dcc5d61e60b8c03`.
- `A experiments/arch003/tests/test_oracles.py` — absent_main; main blob `None`; head blob `bb026947e71a8e86a04f74e67c578e2beb3f0a79`.
- `A experiments/arch003/tests/test_verifier.py` — absent_main; main blob `None`; head blob `bd4d73971de8bfe50cd1615ed951a1050aea98c0`.
- `A experiments/arch003/validation/D-binaries.json` — absent_main; main blob `None`; head blob `66ca6acb4ced7e713760e7f9bdd2e02f576195f8`.
- `A experiments/arch003/validation/D-build.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/D-build.stdout` — absent_main; main blob `None`; head blob `51c94fecb5ea379b7e520db2b6016837d9407068`.
- `A experiments/arch003/validation/D-configure.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/D-configure.stdout` — absent_main; main blob `None`; head blob `cf0fd670580432cca450e0c19a232c2ffe2cb0d6`.
- `A experiments/arch003/validation/D-ctest.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/D-ctest.stdout` — absent_main; main blob `None`; head blob `7d813cde3dfacf5948f9b6567ec2689f0423aaa0`.
- `A experiments/arch003/validation/D-flags.txt` — absent_main; main blob `None`; head blob `51f8528375fbe34db25b1685bae71c10c12ac6ad`.
- `A experiments/arch003/validation/D-python.stderr` — absent_main; main blob `None`; head blob `227e462a9c23636de238354c30849960afbfb724`.
- `A experiments/arch003/validation/D-python.stdout` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/R-binaries.json` — absent_main; main blob `None`; head blob `602deae7d0a8bab0d2f74b271baf2568450b7b40`.
- `A experiments/arch003/validation/R-build.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/R-build.stdout` — absent_main; main blob `None`; head blob `51c94fecb5ea379b7e520db2b6016837d9407068`.
- `A experiments/arch003/validation/R-configure.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/R-configure.stdout` — absent_main; main blob `None`; head blob `5b3281bdbd7f2bbf3dccb11c7c1a100870bcf69f`.
- `A experiments/arch003/validation/R-ctest.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/R-ctest.stdout` — absent_main; main blob `None`; head blob `2bd0ee14d6280592dfdff48ca52a7ae875a640f0`.
- `A experiments/arch003/validation/R-flags.txt` — absent_main; main blob `None`; head blob `2468eee5408c3c0b8b91059d222317f1bdae7868`.
- `A experiments/arch003/validation/R-python.stderr` — absent_main; main blob `None`; head blob `9ea85080a7f54d081d0c00058a7791efee4560c1`.
- `A experiments/arch003/validation/R-python.stdout` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/S-binaries.json` — absent_main; main blob `None`; head blob `7afce13010f42f214894a021c96155cc7f70a669`.
- `A experiments/arch003/validation/S-build.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/S-build.stdout` — absent_main; main blob `None`; head blob `afe6a020d6931fee4101b30e43dc57b8f981a3f2`.
- `A experiments/arch003/validation/S-configure.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/S-configure.stdout` — absent_main; main blob `None`; head blob `471be0d80e23fc41036565ddf90df16b759f9e18`.
- `A experiments/arch003/validation/S-ctest.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/S-ctest.stdout` — absent_main; main blob `None`; head blob `4b1402a76d536bf36c74a5297796d1268ab1364b`.
- `A experiments/arch003/validation/S-flags.txt` — absent_main; main blob `None`; head blob `7cf1186993455a8c81b103af3585767f8c42e34e`.
- `A experiments/arch003/validation/S-python.stderr` — absent_main; main blob `None`; head blob `17ce5fe15540fce3c8d5f2cca8600a0ffb0ad779`.
- `A experiments/arch003/validation/S-python.stdout` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/byte-assembly-R.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/byte-assembly-R.stdout` — absent_main; main blob `None`; head blob `f7957bf7ff1fb271a2ffca3c3fb9350c37b4639b`.
- `A experiments/arch003/validation/cmake-version.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/cmake-version.stdout` — absent_main; main blob `None`; head blob `be81871eb67b22b02da8d5f8fab15392f88d6e11`.
- `A experiments/arch003/validation/compiler-version.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/compiler-version.stdout` — absent_main; main blob `None`; head blob `8e6ead0cd0c4c5f99db42d4eeae6c053919d8947`.
- `A experiments/arch003/validation/components.json` — absent_main; main blob `None`; head blob `8bf9d6cb6d5088e7abaff9b77fa81e6c60accd01`.
- `A experiments/arch003/validation/i2/artifact-inventory.json` — absent_main; main blob `None`; head blob `d8072face79354ae70b8946842a9e0cc727b4f87`.
- `A experiments/arch003/validation/i2/first-failed/first-fixtures.stderr` — absent_main; main blob `None`; head blob `49d93fb3638ed0853f4dd1f82f3b976954d7f072`.
- `A experiments/arch003/validation/i2/first-failed/first-fixtures.stdout` — absent_main; main blob `None`; head blob `f9444c0de8dc55db3c3f8d89b8a542445ac458bf`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_abnormal_signal_reaped_with_peak.json` — absent_main; main blob `None`; head blob `ca1aaae15450c4d931db64bbe5fed03378939b0b`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_actual_L_V_W_fixed_frame.json` — absent_main; main blob `None`; head blob `3e19029a4d9475273a3c1eba6454bc92f21659cb`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_actual_driver_and_linker_receipts.json` — absent_main; main blob `None`; head blob `41f260d3bf05df39800c1bd3847b0682ff7d1281`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_deadline_during_seal_removes_success_root.json` — absent_main; main blob `None`; head blob `9e46922017a356ad347f9e5906622625e29310d7`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_failure_and_prefix_survive_deadline_tree_termination.json` — absent_main; main blob `None`; head blob `a0477e9a7e74a5dd35f880a2fb35209077620730`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_finalization_and_seal_inside_limits.json` — absent_main; main blob `None`; head blob `8cea62da66e8fe77aff9170486d46863730188d9`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_metadata_cannot_spawn_worker.json` — absent_main; main blob `None`; head blob `c91dac5ac07827f5cf6d093782e4016815acd815`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_peak_survives_free_then_exec.json` — absent_main; main blob `None`; head blob `ee217c566f1d29e82e0838c6c8f27f92e3d81654`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_pid_identity_race_mid_job.json` — absent_main; main blob `None`; head blob `9063b299fda5abffa5764841aca7738268186c9c`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_sampling_gap_while_live.json` — absent_main; main blob `None`; head blob `2d546056430cb3b2d6aa4cf273232c388fe296a3`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_short_tool_between_rss_samples.json` — absent_main; main blob `None`; head blob `87f3e153e26027fa1a6591d291bd4307a5a612ac`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_unavailable_proc_mid_job.json` — absent_main; main blob `None`; head blob `8b76db1f8359b96845f3b79869a822d71e8566dd`.
- `A experiments/arch003/validation/i2/first-failed/receipts/test_unexpected_nested_descendant.json` — absent_main; main blob `None`; head blob `b8d01c5e3c70d118db016e8e0f84bfe7f2742f73`.
- `A experiments/arch003/validation/i2/first-failed/source.tar.gz` — absent_main; main blob `None`; head blob `9d9b1b099f2aa436a688f3f6e3c98ec48e7b3102`.
- `A experiments/arch003/validation/i2/fixture-setup-failed/reviewed-fixtures.stderr` — absent_main; main blob `None`; head blob `865837d3604e4da00e82dd22bb512414180f2b98`.
- `A experiments/arch003/validation/i2/fixture-setup-failed/reviewed-fixtures.stdout` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/fixture-setup-failed/source-current.tar.gz` — absent_main; main blob `None`; head blob `dbf8d0bae34ccb071d9cf33da2262da9ab6399f5`.
- `A experiments/arch003/validation/i2/linux/compiler.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/linux/compiler.stdout` — absent_main; main blob `None`; head blob `738e9ba2fdc9707bcc14e9c3fe2012554b3b916b`.
- `A experiments/arch003/validation/i2/linux/fixtures.stderr` — absent_main; main blob `None`; head blob `c157e9a4cd6f9f231e880f4d94feeca635e090c2`.
- `A experiments/arch003/validation/i2/linux/fixtures.stdout` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_abnormal_signal_reaped_with_peak.json` — absent_main; main blob `None`; head blob `3e3270e4f830cd1f72dde72c3585861d2fd250b3`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_actual_L_V_W_fixed_frame.json` — absent_main; main blob `None`; head blob `980f95885fb9be4b523c5777faf70b713feff9ff`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_actual_driver_and_linker_receipts.json` — absent_main; main blob `None`; head blob `46b2865b78d762cfa75f2945b79065abb5bc2deb`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_deadline_during_seal_removes_success_root.json` — absent_main; main blob `None`; head blob `2eaafaeb64a90036f7b2a35749f5e65b8f54de8a`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_failure_and_prefix_survive_accounting_tree_termination.json` — absent_main; main blob `None`; head blob `e3a648d5b8b24ee2509e9352d5aba8f114154b59`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_failure_and_prefix_survive_deadline_tree_termination.json` — absent_main; main blob `None`; head blob `180fc7795a89f3004d706d616be615f30d80f193`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_finalization_and_seal_inside_limits.json` — absent_main; main blob `None`; head blob `ce5730f762d2f354783de7e4f7cfdaaabb78b06e`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_metadata_cannot_spawn_worker.json` — absent_main; main blob `None`; head blob `692ca88f384458f38ccf9743915ef0bd72715cd6`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_nonleader_exec_blocks_before_pid_transition.json` — absent_main; main blob `None`; head blob `9029c69dbb34403858df937c7d58556d6a9fb904`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_output_cap_during_finalization_preserves_failure.json` — absent_main; main blob `None`; head blob `fe7cf48422c1d6e245c2756fd74186bb56a0a9f7`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_peak_survives_free_then_exec.json` — absent_main; main blob `None`; head blob `8e8ea1004d6ac3add4240ea5f90effece57c203f`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_pid_identity_race_mid_job.json` — absent_main; main blob `None`; head blob `03f80c6d0bb23cc511da8c811929c11a253116de`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_ptrace_options_unavailable_is_reaped.json` — absent_main; main blob `None`; head blob `25a4e16b482d6314795646b370076d9362394cb1`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_runtime_maintenance_thread_accounted_once.json` — absent_main; main blob `None`; head blob `b62dfc903c3ac0b878767ee84e61376a63f6c8bf`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_sampling_gap_while_live.json` — absent_main; main blob `None`; head blob `da86669ba5966c2dfd7597c00a5763b9dd9dd888`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_short_tool_between_rss_samples.json` — absent_main; main blob `None`; head blob `06489ac208c29c9e6363db1352b0285a5db07055`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_unavailable_exit_peak_is_null_not_zero.json` — absent_main; main blob `None`; head blob `613da6a81884e8ed42c8b4ffed483d6d45260e0d`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_unavailable_proc_at_launch_is_reaped.json` — absent_main; main blob `None`; head blob `5bf83cafb23f30f29c447b5ad2d90f4eb9c3a564`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_unavailable_proc_mid_job.json` — absent_main; main blob `None`; head blob `e3e838302dbdc03e4fcfd6982b0d947013646e10`.
- `A experiments/arch003/validation/i2/linux/i2-recorded/test_unexpected_nested_descendant.json` — absent_main; main blob `None`; head blob `89879db7972ef1b796ed5fb195cdb4cff9c8b4a4`.
- `A experiments/arch003/validation/i2/linux/identities.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/linux/identities.stdout` — absent_main; main blob `None`; head blob `7698d120c6880746a1a498149f77162db6024049`.
- `A experiments/arch003/validation/i2/linux/kernel.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/linux/kernel.stdout` — absent_main; main blob `None`; head blob `00472d28cde0929cda75bf7bb8e08eb41595c188`.
- `A experiments/arch003/validation/i2/linux/linker.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/linux/linker.stdout` — absent_main; main blob `None`; head blob `5ba484de4d58e0363264a5bb76d25cf287085490`.
- `A experiments/arch003/validation/i2/linux/packages.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/linux/packages.stdout` — absent_main; main blob `None`; head blob `80ac7528438bb3acefd72decb4fae0221dfe4980`.
- `A experiments/arch003/validation/i2/linux/python.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/linux/python.stdout` — absent_main; main blob `None`; head blob `9882f9fa90a3dd138ea29e145f5ee39290d16f97`.
- `A experiments/arch003/validation/i2/linux/run.json` — absent_main; main blob `None`; head blob `3769cdfab5b5f578fc896ffe6236bf46d2da7bd1`.
- `A experiments/arch003/validation/i2/linux/source-transport-cleanup.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/linux/source-transport-cleanup.stdout` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/D-binaries.json` — absent_main; main blob `None`; head blob `66ca6acb4ced7e713760e7f9bdd2e02f576195f8`.
- `A experiments/arch003/validation/i2/macos/D-build.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/D-build.stdout` — absent_main; main blob `None`; head blob `8da59f95ab73cc968e79af817c899568c5760407`.
- `A experiments/arch003/validation/i2/macos/D-configure.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/D-configure.stdout` — absent_main; main blob `None`; head blob `cf0fd670580432cca450e0c19a232c2ffe2cb0d6`.
- `A experiments/arch003/validation/i2/macos/D-ctest.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/D-ctest.stdout` — absent_main; main blob `None`; head blob `03f4621826311c7d0c10599910ba7729ffebdc86`.
- `A experiments/arch003/validation/i2/macos/D-flags.txt` — absent_main; main blob `None`; head blob `51f8528375fbe34db25b1685bae71c10c12ac6ad`.
- `A experiments/arch003/validation/i2/macos/D-python.stderr` — absent_main; main blob `None`; head blob `12be542ae99337bc43ae7296b523a485e0913bbd`.
- `A experiments/arch003/validation/i2/macos/D-python.stdout` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/R-binaries.json` — absent_main; main blob `None`; head blob `602deae7d0a8bab0d2f74b271baf2568450b7b40`.
- `A experiments/arch003/validation/i2/macos/R-build.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/R-build.stdout` — absent_main; main blob `None`; head blob `8da59f95ab73cc968e79af817c899568c5760407`.
- `A experiments/arch003/validation/i2/macos/R-configure.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/R-configure.stdout` — absent_main; main blob `None`; head blob `5b3281bdbd7f2bbf3dccb11c7c1a100870bcf69f`.
- `A experiments/arch003/validation/i2/macos/R-ctest.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/R-ctest.stdout` — absent_main; main blob `None`; head blob `7988378c64caf6b6f523c0e807fb81c1889df43d`.
- `A experiments/arch003/validation/i2/macos/R-flags.txt` — absent_main; main blob `None`; head blob `2468eee5408c3c0b8b91059d222317f1bdae7868`.
- `A experiments/arch003/validation/i2/macos/R-python.stderr` — absent_main; main blob `None`; head blob `18bf8e8f0b88c154eda4ea65cccf3ec98060bfcb`.
- `A experiments/arch003/validation/i2/macos/R-python.stdout` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/S-binaries.json` — absent_main; main blob `None`; head blob `7afce13010f42f214894a021c96155cc7f70a669`.
- `A experiments/arch003/validation/i2/macos/S-build.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/S-build.stdout` — absent_main; main blob `None`; head blob `42d6c1f1445dee7fd76dc2ac5a757ecab001322f`.
- `A experiments/arch003/validation/i2/macos/S-configure.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/S-configure.stdout` — absent_main; main blob `None`; head blob `471be0d80e23fc41036565ddf90df16b759f9e18`.
- `A experiments/arch003/validation/i2/macos/S-ctest.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/S-ctest.stdout` — absent_main; main blob `None`; head blob `05e64c0f7d7b59b1fcf66653f35970042fec9596`.
- `A experiments/arch003/validation/i2/macos/S-flags.txt` — absent_main; main blob `None`; head blob `7cf1186993455a8c81b103af3585767f8c42e34e`.
- `A experiments/arch003/validation/i2/macos/S-python.stderr` — absent_main; main blob `None`; head blob `4f8aa575b48c6dc707d528c8699faabcc7d19af5`.
- `A experiments/arch003/validation/i2/macos/S-python.stdout` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/byte-assembly-R.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/byte-assembly-R.stdout` — absent_main; main blob `None`; head blob `f7957bf7ff1fb271a2ffca3c3fb9350c37b4639b`.
- `A experiments/arch003/validation/i2/macos/cmake-version.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/cmake-version.stdout` — absent_main; main blob `None`; head blob `be81871eb67b22b02da8d5f8fab15392f88d6e11`.
- `A experiments/arch003/validation/i2/macos/compiler-version.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/compiler-version.stdout` — absent_main; main blob `None`; head blob `8e6ead0cd0c4c5f99db42d4eeae6c053919d8947`.
- `A experiments/arch003/validation/i2/macos/components.json` — absent_main; main blob `None`; head blob `c66296d76dbd81e035c52791f5cd9317fe8e7784`.
- `A experiments/arch003/validation/i2/macos/symbols-histogram.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/symbols-histogram.stdout` — absent_main; main blob `None`; head blob `630b0612698566487fe1d836caf1cc2f4b4a8d11`.
- `A experiments/arch003/validation/i2/macos/symbols-scalar.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/i2/macos/symbols-scalar.stdout` — absent_main; main blob `None`; head blob `7852accb7393bfd30ad62cc24926c46cbaaaa3a1`.
- `A experiments/arch003/validation/symbols-histogram.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/symbols-histogram.stdout` — absent_main; main blob `None`; head blob `630b0612698566487fe1d836caf1cc2f4b4a8d11`.
- `A experiments/arch003/validation/symbols-scalar.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/arch003/validation/symbols-scalar.stdout` — absent_main; main blob `None`; head blob `7852accb7393bfd30ad62cc24926c46cbaaaa3a1`.
- `A experiments/arch003/verifier.py` — absent_main; main blob `None`; head blob `571ac96a3908d7c61b2724beb3ff1a7642355db6`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 714: docs(research): preregister packed equality correctness protocol (ARCH-002)

- Head `1454bd63426d21686cfb5ef22dbaebc3c9cae94b`; actual base `d134407ae1059c4d7f6a9418177e695e0e5d3b51` (`main`); main merge-base `d134407ae1059c4d7f6a9418177e695e0e5d3b51`.
- Purpose: Packed equality preregistration. Decision: **DEFER**.
- Dependency assessment: Protocol parent of 716; not represented on main.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Three unique protocol/proposal/validation paths. Human protocol decisions and independently feasible controls precede any separately authorized execution.
- Blob counts: {'absent_main': 3}. Merge simulation exit 0; tree `fd889b1d82013102048be6a2132bdefac174e147`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...1454bd63426d21686cfb5ef22dbaebc3c9cae94b).

Own changed files:

- `A docs/research/arch002/PROTOCOL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/1454bd63426d21686cfb5ef22dbaebc3c9cae94b/docs/research/arch002/PROTOCOL.md))
- `A docs/research/arch002/VALIDATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/1454bd63426d21686cfb5ef22dbaebc3c9cae94b/docs/research/arch002/VALIDATION.md))
- `A docs/research/arch002/proposal.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/1454bd63426d21686cfb5ef22dbaebc3c9cae94b/docs/research/arch002/proposal.json))

All changed files versus main merge-base (including parents):

- `A docs/research/arch002/PROTOCOL.md` — absent_main; main blob `None`; head blob `3d86cf749841388c50a14316cb6dfe40296181c1`.
- `A docs/research/arch002/VALIDATION.md` — absent_main; main blob `None`; head blob `00443b5dc759168aeeb1b663c726366f4446dc38`.
- `A docs/research/arch002/proposal.json` — absent_main; main blob `None`; head blob `0aa53b9fdf7620f50a4dc23172410777421b1cb1`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 709: experiment(cpa004): execute frozen TISC profile after ExitBootServices

- Head `b803bf916a1b1e44129f3a07abf96fa6103c51fc`; actual base `ea6d031a8117341fe03013cc9d5473de1b07727d` (`main`); main merge-base `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`.
- Purpose: CPA-004 post-ExitBootServices bounded ISA experiment. Decision: **REVIEW NEXT**.
- Dependency assessment: Conceptual dependence on CPA conformance 707 and frozen profile; no new boot or benchmark campaign.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: 73 own paths include evaluator, frozen artifact and boot evidence. Review identity/oracle/admission boundaries; author-reported boots are not rerun by this inventory or a general ISA qualification.
- Blob counts: {'absent_main': 71, 'different': 2}. Merge simulation exit 0; tree `592fd08d339d99e7e82157ca0119ad3c4f3b9d7e`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b803bf916a1b1e44129f3a07abf96fa6103c51fc).

Own changed files:

- `A experiments/cpa004/COMMANDS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/COMMANDS.md))
- `A experiments/cpa004/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/README.md))
- `A experiments/cpa004/admission_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/admission_test.cpp))
- `A experiments/cpa004/contract.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/contract.json))
- `A experiments/cpa004/contract.sha256` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/contract.sha256))
- `A experiments/cpa004/evidence/acceptance/CPA003-HOSTED_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/CPA003-HOSTED_REPORT.md))
- `A experiments/cpa004/evidence/acceptance/CPA003-README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/CPA003-README.md))
- `A experiments/cpa004/evidence/acceptance/CPA003-RISK_REGISTER.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/CPA003-RISK_REGISTER.md))
- `A experiments/cpa004/evidence/acceptance/check.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/check.py))
- `A experiments/cpa004/evidence/acceptance/discrepancy-register.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/discrepancy-register.json))
- `A experiments/cpa004/evidence/acceptance/gate.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/gate.json))
- `A experiments/cpa004/evidence/acceptance/hosted-1.jsonl.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/hosted-1.jsonl.gz))
- `A experiments/cpa004/evidence/acceptance/hosted-1.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/hosted-1.stderr))
- `A experiments/cpa004/evidence/acceptance/hosted-2.jsonl.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/hosted-2.jsonl.gz))
- `A experiments/cpa004/evidence/acceptance/hosted-2.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/hosted-2.stderr))
- `A experiments/cpa004/evidence/acceptance/hosted-3.jsonl.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/hosted-3.jsonl.gz))
- `A experiments/cpa004/evidence/acceptance/hosted-3.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/hosted-3.stderr))
- `A experiments/cpa004/evidence/acceptance/hosted-build.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/hosted-build.json))
- `A experiments/cpa004/evidence/acceptance/inventory-assessment.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/inventory-assessment.json))
- `A experiments/cpa004/evidence/acceptance/oracle-freeze.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/oracle-freeze.json))
- `A experiments/cpa004/evidence/acceptance/oracle-regeneration.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/oracle-regeneration.json))
- `A experiments/cpa004/evidence/acceptance/packet-manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/packet-manifest.json))
- `A experiments/cpa004/evidence/acceptance/packet-recheck.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/packet-recheck.json))
- `A experiments/cpa004/evidence/acceptance/proposal-validation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/proposal-validation.json))
- `A experiments/cpa004/evidence/acceptance/proposal.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/proposal.json))
- `A experiments/cpa004/evidence/acceptance/recheck.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/recheck.json))
- `A experiments/cpa004/evidence/acceptance/source-pins.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/acceptance/source-pins.json))
- `A experiments/cpa004/evidence/build-artifact-hashes.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/build-artifact-hashes.json))
- `A experiments/cpa004/evidence/build-final.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/build-final.log))
- `A experiments/cpa004/evidence/build.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/build.log))
- `A experiments/cpa004/evidence/efi-headers.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/efi-headers.txt))
- `A experiments/cpa004/evidence/environment.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/environment.json))
- `A experiments/cpa004/evidence/evaluator-object.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/evaluator-object.txt))
- `A experiments/cpa004/evidence/evaluator-stack.su` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/evaluator-stack.su))
- `A experiments/cpa004/evidence/freestanding-build-commands.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/freestanding-build-commands.txt))
- `A experiments/cpa004/evidence/hosted/hosted-1.jsonl.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/hosted/hosted-1.jsonl.gz))
- `A experiments/cpa004/evidence/hosted/hosted-1.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/hosted/hosted-1.stderr))
- `A experiments/cpa004/evidence/hosted/hosted-2.jsonl.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/hosted/hosted-2.jsonl.gz))
- `A experiments/cpa004/evidence/hosted/hosted-2.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/hosted/hosted-2.stderr))
- `A experiments/cpa004/evidence/hosted/hosted-3.jsonl.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/hosted/hosted-3.jsonl.gz))
- `A experiments/cpa004/evidence/hosted/hosted-3.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/hosted/hosted-3.stderr))
- `A experiments/cpa004/evidence/hosted/summary.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/hosted/summary.json))
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-1/result.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/initial-staging-timeouts/boot-1/result.json))
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-1/serial.log.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/initial-staging-timeouts/boot-1/serial.log.gz))
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-2/result.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/initial-staging-timeouts/boot-2/result.json))
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-2/serial.log.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/initial-staging-timeouts/boot-2/serial.log.gz))
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-3/result.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/initial-staging-timeouts/boot-3/result.json))
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-3/serial.log.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/initial-staging-timeouts/boot-3/serial.log.gz))
- `A experiments/cpa004/evidence/initial-staging-timeouts/summary.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/initial-staging-timeouts/summary.json))
- `A experiments/cpa004/evidence/local-checks.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/local-checks.json))
- `A experiments/cpa004/evidence/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/manifest.json))
- `A experiments/cpa004/evidence/ndebug-failure-demonstration.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/ndebug-failure-demonstration.json))
- `A experiments/cpa004/evidence/option-off-check.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/option-off-check.json))
- `A experiments/cpa004/evidence/packet-validation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/packet-validation.json))
- `A experiments/cpa004/evidence/qemu/boot-1/result.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/qemu/boot-1/result.json))
- `A experiments/cpa004/evidence/qemu/boot-1/serial.log.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/qemu/boot-1/serial.log.gz))
- `A experiments/cpa004/evidence/qemu/boot-2/result.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/qemu/boot-2/result.json))
- `A experiments/cpa004/evidence/qemu/boot-2/serial.log.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/qemu/boot-2/serial.log.gz))
- `A experiments/cpa004/evidence/qemu/boot-3/result.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/qemu/boot-3/result.json))
- `A experiments/cpa004/evidence/qemu/boot-3/serial.log.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/qemu/boot-3/serial.log.gz))
- `A experiments/cpa004/evidence/qemu/summary.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/qemu/summary.json))
- `A experiments/cpa004/evidence/schema-validation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/evidence/schema-validation.json))
- `A experiments/cpa004/expected.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/expected.json))
- `A experiments/cpa004/hosted_observer.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/hosted_observer.cpp))
- `A experiments/cpa004/instruction-listing.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/instruction-listing.json))
- `A experiments/cpa004/observation.schema.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/observation.schema.json))
- `A experiments/cpa004/program.raw13` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/program.raw13))
- `A experiments/cpa004/run_qemu.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/run_qemu.py))
- `A experiments/cpa004/verify_evidence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/experiments/cpa004/verify_evidence.py))
- `M ternaryos/CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/ternaryos/CMakeLists.txt))
- `A ternaryos/hal/cpa004_evaluator.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/ternaryos/hal/cpa004_evaluator.cpp))
- `A ternaryos/hal/cpa004_evaluator.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/ternaryos/hal/cpa004_evaluator.hpp))
- `M ternaryos/hal/qemu_slice6_bare_kernel.c` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b803bf916a1b1e44129f3a07abf96fa6103c51fc/ternaryos/hal/qemu_slice6_bare_kernel.c))

All changed files versus main merge-base (including parents):

- `A experiments/cpa004/COMMANDS.md` — absent_main; main blob `None`; head blob `88177d0e8466c3a5ce1b6f57d9f6bdeb6f4414c0`.
- `A experiments/cpa004/README.md` — absent_main; main blob `None`; head blob `cd267d0607c13e2b5b4195699598cdcf580ac74c`.
- `A experiments/cpa004/admission_test.cpp` — absent_main; main blob `None`; head blob `aa099ca362e5e111dfb02882a139b431dd794157`.
- `A experiments/cpa004/contract.json` — absent_main; main blob `None`; head blob `97a282fe892ac248bb9bb5944d854df4e66773c6`.
- `A experiments/cpa004/contract.sha256` — absent_main; main blob `None`; head blob `67aa31382c8548b3d05ee9fbb9893f46b09a6a04`.
- `A experiments/cpa004/evidence/acceptance/CPA003-HOSTED_REPORT.md` — absent_main; main blob `None`; head blob `34119c8f63029ac7472e33d7a4bc478990771468`.
- `A experiments/cpa004/evidence/acceptance/CPA003-README.md` — absent_main; main blob `None`; head blob `0fd8f60a7f94c9760c4b45bfdccf478699c7fb05`.
- `A experiments/cpa004/evidence/acceptance/CPA003-RISK_REGISTER.md` — absent_main; main blob `None`; head blob `a2b0d503b3330d6a92baa51424e0d240bf874767`.
- `A experiments/cpa004/evidence/acceptance/check.py` — absent_main; main blob `None`; head blob `130aea6ce7d9bf58be7b176009f1d789fe7d53d6`.
- `A experiments/cpa004/evidence/acceptance/discrepancy-register.json` — absent_main; main blob `None`; head blob `48b163b90262000a29634039d15f28d70516bf27`.
- `A experiments/cpa004/evidence/acceptance/gate.json` — absent_main; main blob `None`; head blob `49e68420cb424ca8cd5ee5fb5bdd7a03ef471e23`.
- `A experiments/cpa004/evidence/acceptance/hosted-1.jsonl.gz` — absent_main; main blob `None`; head blob `c2647d42a07a57a1bf96f44f28deba85742f7cac`.
- `A experiments/cpa004/evidence/acceptance/hosted-1.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/cpa004/evidence/acceptance/hosted-2.jsonl.gz` — absent_main; main blob `None`; head blob `c2647d42a07a57a1bf96f44f28deba85742f7cac`.
- `A experiments/cpa004/evidence/acceptance/hosted-2.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/cpa004/evidence/acceptance/hosted-3.jsonl.gz` — absent_main; main blob `None`; head blob `c2647d42a07a57a1bf96f44f28deba85742f7cac`.
- `A experiments/cpa004/evidence/acceptance/hosted-3.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/cpa004/evidence/acceptance/hosted-build.json` — absent_main; main blob `None`; head blob `fc1abd5971a23a5cac678855396a378c88136dcd`.
- `A experiments/cpa004/evidence/acceptance/inventory-assessment.json` — absent_main; main blob `None`; head blob `7120fff418749973b12838977a43f7648aa56b81`.
- `A experiments/cpa004/evidence/acceptance/oracle-freeze.json` — absent_main; main blob `None`; head blob `41f6c8bfe6055b569c060f6ecb163cc8f14baaf8`.
- `A experiments/cpa004/evidence/acceptance/oracle-regeneration.json` — absent_main; main blob `None`; head blob `c7bd1856b8088bc648d7af0e772ecba4745b1e47`.
- `A experiments/cpa004/evidence/acceptance/packet-manifest.json` — absent_main; main blob `None`; head blob `8792a459482b7ff29e41d79e1096ec081efef754`.
- `A experiments/cpa004/evidence/acceptance/packet-recheck.json` — absent_main; main blob `None`; head blob `efdd7f6e99418f538e839566f3d7fd700681d62f`.
- `A experiments/cpa004/evidence/acceptance/proposal-validation.json` — absent_main; main blob `None`; head blob `daa77098a966851f8054fd3a0222096d675d151c`.
- `A experiments/cpa004/evidence/acceptance/proposal.json` — absent_main; main blob `None`; head blob `11474e63529c5783606e462a13918c5fe1e89950`.
- `A experiments/cpa004/evidence/acceptance/recheck.json` — absent_main; main blob `None`; head blob `f2099b2c6e4b51e588e31672fe045e7e778f8722`.
- `A experiments/cpa004/evidence/acceptance/source-pins.json` — absent_main; main blob `None`; head blob `ee9e565bf00a10e82ed0419e12c2302d6ba823ce`.
- `A experiments/cpa004/evidence/build-artifact-hashes.json` — absent_main; main blob `None`; head blob `b7676ef4a30a4e6333a45c9902f3f2d20abcd580`.
- `A experiments/cpa004/evidence/build-final.log` — absent_main; main blob `None`; head blob `34ef4cc627be5a91d721542a0f2b858ce3dab36d`.
- `A experiments/cpa004/evidence/build.log` — absent_main; main blob `None`; head blob `6e1e5814a4663ea949d5f4c8766b2a6912ab5f7c`.
- `A experiments/cpa004/evidence/efi-headers.txt` — absent_main; main blob `None`; head blob `7c59ca094589a3375b982867d64a7839131cd1de`.
- `A experiments/cpa004/evidence/environment.json` — absent_main; main blob `None`; head blob `7c19a90ad2d2bacc50379f8fabfc089e3608a97a`.
- `A experiments/cpa004/evidence/evaluator-object.txt` — absent_main; main blob `None`; head blob `9280f0ef638c4216d0070f88e611186e534fdb89`.
- `A experiments/cpa004/evidence/evaluator-stack.su` — absent_main; main blob `None`; head blob `7ea82de6d7f757cc1f82a93ca37a9492042bc257`.
- `A experiments/cpa004/evidence/freestanding-build-commands.txt` — absent_main; main blob `None`; head blob `d5f5720b37521a2b982012bd65e23d78c939a21a`.
- `A experiments/cpa004/evidence/hosted/hosted-1.jsonl.gz` — absent_main; main blob `None`; head blob `c2647d42a07a57a1bf96f44f28deba85742f7cac`.
- `A experiments/cpa004/evidence/hosted/hosted-1.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/cpa004/evidence/hosted/hosted-2.jsonl.gz` — absent_main; main blob `None`; head blob `c2647d42a07a57a1bf96f44f28deba85742f7cac`.
- `A experiments/cpa004/evidence/hosted/hosted-2.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/cpa004/evidence/hosted/hosted-3.jsonl.gz` — absent_main; main blob `None`; head blob `c2647d42a07a57a1bf96f44f28deba85742f7cac`.
- `A experiments/cpa004/evidence/hosted/hosted-3.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/cpa004/evidence/hosted/summary.json` — absent_main; main blob `None`; head blob `4afbdcd2da14e9e51154360983adedd7dfb9a2cb`.
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-1/result.json` — absent_main; main blob `None`; head blob `6c1419f028ee192fbc22c967cef069f742827bab`.
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-1/serial.log.gz` — absent_main; main blob `None`; head blob `cce58ee3bdfc405bd6c5ba5d5ae9d600e477e2f7`.
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-2/result.json` — absent_main; main blob `None`; head blob `5f873c7f8eea79b0abd090d481b3da234024be8b`.
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-2/serial.log.gz` — absent_main; main blob `None`; head blob `cce58ee3bdfc405bd6c5ba5d5ae9d600e477e2f7`.
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-3/result.json` — absent_main; main blob `None`; head blob `4cc91deb20cdde0d185f61f26e4ca67865235733`.
- `A experiments/cpa004/evidence/initial-staging-timeouts/boot-3/serial.log.gz` — absent_main; main blob `None`; head blob `cce58ee3bdfc405bd6c5ba5d5ae9d600e477e2f7`.
- `A experiments/cpa004/evidence/initial-staging-timeouts/summary.json` — absent_main; main blob `None`; head blob `f2712edc147fb4070c0050b2ce963c539824439f`.
- `A experiments/cpa004/evidence/local-checks.json` — absent_main; main blob `None`; head blob `090cc86fd71338dfc14a1fbdaf50130b983553c2`.
- `A experiments/cpa004/evidence/manifest.json` — absent_main; main blob `None`; head blob `e68af4ef9e10419ea4cbb635b9e475234b4787b8`.
- `A experiments/cpa004/evidence/ndebug-failure-demonstration.json` — absent_main; main blob `None`; head blob `ca13393eb1dce806fe539964d00f2f1e8813bd40`.
- `A experiments/cpa004/evidence/option-off-check.json` — absent_main; main blob `None`; head blob `28d1c1759bfa43c18c22711e005d4cbda3984b11`.
- `A experiments/cpa004/evidence/packet-validation.json` — absent_main; main blob `None`; head blob `1beaf13efbc093582da2c11427227451310310d5`.
- `A experiments/cpa004/evidence/qemu/boot-1/result.json` — absent_main; main blob `None`; head blob `b35ef0a554335c891dea51f3a586275137f1b418`.
- `A experiments/cpa004/evidence/qemu/boot-1/serial.log.gz` — absent_main; main blob `None`; head blob `417c7485f64de94fcb0abdecd6a84f11259ee716`.
- `A experiments/cpa004/evidence/qemu/boot-2/result.json` — absent_main; main blob `None`; head blob `78dd719fc3a46bcd061439a2c16b63c245f8e54f`.
- `A experiments/cpa004/evidence/qemu/boot-2/serial.log.gz` — absent_main; main blob `None`; head blob `417c7485f64de94fcb0abdecd6a84f11259ee716`.
- `A experiments/cpa004/evidence/qemu/boot-3/result.json` — absent_main; main blob `None`; head blob `c89c7414db782f9ebda12e71144c046ef8b552a9`.
- `A experiments/cpa004/evidence/qemu/boot-3/serial.log.gz` — absent_main; main blob `None`; head blob `417c7485f64de94fcb0abdecd6a84f11259ee716`.
- `A experiments/cpa004/evidence/qemu/summary.json` — absent_main; main blob `None`; head blob `87faad4cf3e424df262d4eb6b0313bfa74f459db`.
- `A experiments/cpa004/evidence/schema-validation.json` — absent_main; main blob `None`; head blob `60aaab5fa92dee06451ae8c6d14570328dab08e6`.
- `A experiments/cpa004/expected.json` — absent_main; main blob `None`; head blob `8c3965e06b57c717bb3986c87ad89c0886a8c0aa`.
- `A experiments/cpa004/hosted_observer.cpp` — absent_main; main blob `None`; head blob `35ec7c352b711db28b24c80e1e3b9e7c67234fb5`.
- `A experiments/cpa004/instruction-listing.json` — absent_main; main blob `None`; head blob `b1bc3f0792c123525a6c58e3407b10983def65f8`.
- `A experiments/cpa004/observation.schema.json` — absent_main; main blob `None`; head blob `c8f0a6f2a9b6a62dab6b1a64e1968791621435b4`.
- `A experiments/cpa004/program.raw13` — absent_main; main blob `None`; head blob `b9079f2782c852b468a39972b1a3569cb91106ec`.
- `A experiments/cpa004/run_qemu.py` — absent_main; main blob `None`; head blob `33a43a21c3071b42057877932c110da071936e57`.
- `A experiments/cpa004/verify_evidence.py` — absent_main; main blob `None`; head blob `7adb474dad9e8a0bfc5021fa69b61de1b1a0ce08`.
- `M ternaryos/CMakeLists.txt` — different; main blob `e1eee9da7af0f4abb580839fe766373c9b644f73`; head blob `591847acc4a6cb2900355be5b56c1142ab851f9b`.
- `A ternaryos/hal/cpa004_evaluator.cpp` — absent_main; main blob `None`; head blob `a15f9542ffa2461d7af6c2edf98599891e572214`.
- `A ternaryos/hal/cpa004_evaluator.hpp` — absent_main; main blob `None`; head blob `316a44b033922256dee288425babfe1324e4647c`.
- `M ternaryos/hal/qemu_slice6_bare_kernel.c` — different; main blob `0088f35d17ff2e78ffa9371b955b6119af49eb36`; head blob `3e7a2a9b416cf9ce8be8a0c374526195300f4765`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127251131875): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127237505768): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127237505619): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127237505608): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127237505558): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127237505501): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127237505485): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127237505467): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127235373473): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127235373450): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127235373439): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/42022382463/job/127235373365): completed / failure.

## PR 707: test(isa): restore trustworthy opcode matrix conformance (T81-CPA-002)

- Head `55223cbd3ee29b5391e151197fe6eee1b8889452`; actual base `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4` (`main`); main merge-base `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`.
- Purpose: Release-active ISA opcode conformance tests. Decision: **REVIEW NEXT**.
- Dependency assessment: One test path overlaps governance integration 649; no implementation expansion.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Three files only: test and two evidence records. Existing exact-head bot summary reports no findings; review defined/undefined opcode selection and source against current decoder before rerunning narrow tests.
- Blob counts: {'absent_main': 2, 'different': 1}. Merge simulation exit 0; tree `6a81c7f48ebb0135bd32736bf831551256e3e88f`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 16 pass, 15 fail, 9 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...55223cbd3ee29b5391e151197fe6eee1b8889452).

Own changed files:

- `A docs/records/audits/T81_CPA_002_OPCODE_CONFORMANCE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/55223cbd3ee29b5391e151197fe6eee1b8889452/docs/records/audits/T81_CPA_002_OPCODE_CONFORMANCE.md))
- `A docs/records/audits/T81_CPA_002_OPCODE_CONFORMANCE.results.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/55223cbd3ee29b5391e151197fe6eee1b8889452/docs/records/audits/T81_CPA_002_OPCODE_CONFORMANCE.results.json))
- `M tests/cpp/tisc_opcode_matrix_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/55223cbd3ee29b5391e151197fe6eee1b8889452/tests/cpp/tisc_opcode_matrix_test.cpp))

All changed files versus main merge-base (including parents):

- `A docs/records/audits/T81_CPA_002_OPCODE_CONFORMANCE.md` — absent_main; main blob `None`; head blob `1490c11a8a45575bcdec5de050724f3aededd686`.
- `A docs/records/audits/T81_CPA_002_OPCODE_CONFORMANCE.results.json` — absent_main; main blob `None`; head blob `c6c34d0988f93b3ad1b4400cc609382af9f3e5a4`.
- `M tests/cpp/tisc_opcode_matrix_test.cpp` — different; main blob `b3c38649c4919c8a044ed6221a6c9e4a4da818e1`; head blob `74a51d042d4304a8b14bb03a7704ccb25fbaf3d0`.

Overlaps with other PR own deltas:

- #649: `tests/cpp/tisc_opcode_matrix_test.cpp`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127035022943): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127026350818): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127026350785): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127026350728): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127026350708): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127026350707): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127026350694): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127026350679): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127024957242): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127024957201): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127024957180): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41960235462/job/127024957082): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/41960235432/job/127024629807): completed / failure.
- [format / clang-format](https://github.com/t81dev/t81-foundation/actions/runs/41960235479/job/127024629665): completed / failure.
- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/41960235432/job/127024629423): completed / failure.

## PR 699: BASE-01 B1: propose CanonFS retained-output budget amendment A1

- Head `8c9e2c7f0b1252b63b175972630bfa2147e436da`; actual base `a98b22fb65c80f4225151cbcd5c6934ae076cbe1` (`main`); main merge-base `a98b22fb65c80f4225151cbcd5c6934ae076cbe1`.
- Purpose: BASE-01 retained-output budget amendment. Decision: **DEFER**.
- Dependency assessment: Overlaps 697 contract reconciliation; related 698 qualification is blocked.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [698].
- Evidence/limits: Design-only 128 MiB CanonFS exception proposal; requires explicit amendment decision, not silent relaxation of 16 MiB limit. Preserve negative feasibility observation.
- Blob counts: {'absent_main': 1, 'different': 3}. Merge simulation exit 0; tree `6409e081c2584f4add4c78a759af0270314dede2`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...8c9e2c7f0b1252b63b175972630bfa2147e436da).

Own changed files:

- `A docs/research/base01/BASE01_B1_CANONFS_RETENTION_AMENDMENT_A1.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8c9e2c7f0b1252b63b175972630bfa2147e436da/docs/research/base01/BASE01_B1_CANONFS_RETENTION_AMENDMENT_A1.md))
- `M docs/research/base01/BASE01_B1_HOST_QUALIFICATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8c9e2c7f0b1252b63b175972630bfa2147e436da/docs/research/base01/BASE01_B1_HOST_QUALIFICATION.md))
- `M docs/research/base01/BASE01_B1_MEASUREMENT_CONTRACT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8c9e2c7f0b1252b63b175972630bfa2147e436da/docs/research/base01/BASE01_B1_MEASUREMENT_CONTRACT.md))
- `M docs/research/base01/BASE01_B1_READINESS_MATRIX.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8c9e2c7f0b1252b63b175972630bfa2147e436da/docs/research/base01/BASE01_B1_READINESS_MATRIX.md))

All changed files versus main merge-base (including parents):

- `A docs/research/base01/BASE01_B1_CANONFS_RETENTION_AMENDMENT_A1.md` — absent_main; main blob `None`; head blob `f9b03e3c11f40b8c7ef9e47da0f064f155f8dfc3`.
- `M docs/research/base01/BASE01_B1_HOST_QUALIFICATION.md` — different; main blob `e035988885657e234bbc4a356edef2661ff60227`; head blob `72eb0b0944b6addd40ddb5fd98d96c07bc9047ef`.
- `M docs/research/base01/BASE01_B1_MEASUREMENT_CONTRACT.md` — different; main blob `57ffff0f0e7f53a1f4b6cfc7625bc9e36826c451`; head blob `0b085d055c6d1a84d138933e0176da043f902354`.
- `M docs/research/base01/BASE01_B1_READINESS_MATRIX.md` — different; main blob `7d5afb9b54c104086ed7cbfcd528c80b35eacb2d`; head blob `f735a9f286c490b36eb8b1d7ea84572ea5969767`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126312696260): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126301073765): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126301073613): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126301073592): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126301073541): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126301073486): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126301073485): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126301073460): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126290941388): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126290941311): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126290941299): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41740444966/job/126290941172): completed / failure.

## PR 698: BASE-01 Physical Linux Correctness-Only Qualification Report

- Head `accebb20f4de25fddfe12e6152c356ff25cb6127`; actual base `a98b22fb65c80f4225151cbcd5c6934ae076cbe1` (`main`); main merge-base `a98b22fb65c80f4225151cbcd5c6934ae076cbe1`.
- Purpose: Physical-Linux correctness qualification record. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: BASE-01 authority/budget decisions in 697/699 remain separate.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Current body reports virtualized/BLOCKED status; three earlier findings are outdated. Independently check sealed artifact hashes and recorded procedural deviations before accepting historical packet; no physical qualification is established here.
- Blob counts: {'absent_main': 23}. Merge simulation exit 0; tree `dbc312507ac0286f4236c050b907f8741d347903`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/3 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 11 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...accebb20f4de25fddfe12e6152c356ff25cb6127).

Own changed files:

- `A docs/research/base01/BASE01_PHYSICAL_LINUX_CORRECTNESS_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/BASE01_PHYSICAL_LINUX_CORRECTNESS_REPORT.md))
- `A docs/research/base01/evidence/physical_linux_campaign/SHA256SUMS.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/SHA256SUMS.json))
- `A docs/research/base01/evidence/physical_linux_campaign/execution_logs.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/execution_logs.tar.gz))
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/lifecycle.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/foundation/lifecycle.json))
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/native-cache.txt.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/foundation/native-cache.txt.gz))
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/native-compile-commands.json.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/foundation/native-compile-commands.json.gz))
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/native-stderr.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/foundation/native-stderr.txt))
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/native.json.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/foundation/native.json.gz))
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/receipt.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/foundation/receipt.json))
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/sha256.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/foundation/sha256.json))
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/source-cache.txt.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/foundation/source-cache.txt.gz))
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/source-compile-commands.json.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/foundation/source-compile-commands.json.gz))
- `A docs/research/base01/evidence/physical_linux_campaign/manifest.expected.sha256` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/manifest.expected.sha256))
- `A docs/research/base01/evidence/physical_linux_campaign/r75/probe-cache.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/r75/probe-cache.txt))
- `A docs/research/base01/evidence/physical_linux_campaign/r75/probe-compile-commands.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/r75/probe-compile-commands.json))
- `A docs/research/base01/evidence/physical_linux_campaign/r75/raw.jsonl.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/r75/raw.jsonl.gz))
- `A docs/research/base01/evidence/physical_linux_campaign/r75/sha256.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/r75/sha256.json))
- `A docs/research/base01/evidence/physical_linux_campaign/r75/source-cache.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/r75/source-cache.txt))
- `A docs/research/base01/evidence/physical_linux_campaign/r75/source-compile-commands.json.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/r75/source-compile-commands.json.gz))
- `A docs/research/base01/evidence/physical_linux_campaign/r75/stderr.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/r75/stderr.txt))
- `A docs/research/base01/evidence/physical_linux_campaign/r75/summary.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/r75/summary.json))
- `A docs/research/base01/evidence/physical_linux_campaign/resource.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/resource.json))
- `A docs/research/base01/evidence/physical_linux_campaign/session.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/accebb20f4de25fddfe12e6152c356ff25cb6127/docs/research/base01/evidence/physical_linux_campaign/session.txt))

All changed files versus main merge-base (including parents):

- `A docs/research/base01/BASE01_PHYSICAL_LINUX_CORRECTNESS_REPORT.md` — absent_main; main blob `None`; head blob `8cdf36692f4c6c8726503e34957f560c7217331c`.
- `A docs/research/base01/evidence/physical_linux_campaign/SHA256SUMS.json` — absent_main; main blob `None`; head blob `eaf83e2be675db66b86fbc3668dd91d5323d58e1`.
- `A docs/research/base01/evidence/physical_linux_campaign/execution_logs.tar.gz` — absent_main; main blob `None`; head blob `3d972932c0391172ee526483b60803a89c6b78f0`.
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/lifecycle.json` — absent_main; main blob `None`; head blob `8cb1f852fae19bc132d89327a0c460c649782ace`.
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/native-cache.txt.gz` — absent_main; main blob `None`; head blob `f388df04b968461558b7b87483f26df37e071b90`.
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/native-compile-commands.json.gz` — absent_main; main blob `None`; head blob `0b9337e8e326b434e9612c83f708f3b2b6c656bf`.
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/native-stderr.txt` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/native.json.gz` — absent_main; main blob `None`; head blob `df58e52d7036483a9b172b161960bc19424cdba3`.
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/receipt.json` — absent_main; main blob `None`; head blob `9b0b29332ffbe8933303b02c5485d9907a6f24f0`.
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/sha256.json` — absent_main; main blob `None`; head blob `a432ee2b5eb302f043518ce6d002401188015eb7`.
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/source-cache.txt.gz` — absent_main; main blob `None`; head blob `5cb2c96cebebb431ea63fa0295ee4a65749a8882`.
- `A docs/research/base01/evidence/physical_linux_campaign/foundation/source-compile-commands.json.gz` — absent_main; main blob `None`; head blob `86074c8bc8fb320ca4dc28f9e0939a8378008db3`.
- `A docs/research/base01/evidence/physical_linux_campaign/manifest.expected.sha256` — absent_main; main blob `None`; head blob `9e679244f32702ea0e9f3ceccaa9c80f0e51de55`.
- `A docs/research/base01/evidence/physical_linux_campaign/r75/probe-cache.txt` — absent_main; main blob `None`; head blob `c4dcd9513b28a4843b0243545ff7a11665555158`.
- `A docs/research/base01/evidence/physical_linux_campaign/r75/probe-compile-commands.json` — absent_main; main blob `None`; head blob `c57eafece32bdd86febb09d5f6b30aa8342edaef`.
- `A docs/research/base01/evidence/physical_linux_campaign/r75/raw.jsonl.gz` — absent_main; main blob `None`; head blob `5d03ead2dc2b6109aa340df1f487dc21ce3bbf4d`.
- `A docs/research/base01/evidence/physical_linux_campaign/r75/sha256.json` — absent_main; main blob `None`; head blob `2fe4cb373d8a07bd71e8aa052bb7fff2c0de78ea`.
- `A docs/research/base01/evidence/physical_linux_campaign/r75/source-cache.txt` — absent_main; main blob `None`; head blob `9c8c012773fc4a6358740e737f7383e7bfda0cfb`.
- `A docs/research/base01/evidence/physical_linux_campaign/r75/source-compile-commands.json.gz` — absent_main; main blob `None`; head blob `86074c8bc8fb320ca4dc28f9e0939a8378008db3`.
- `A docs/research/base01/evidence/physical_linux_campaign/r75/stderr.txt` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A docs/research/base01/evidence/physical_linux_campaign/r75/summary.json` — absent_main; main blob `None`; head blob `cf841627adbe6d52a7e5822c4bb0aff49a6c6a1a`.
- `A docs/research/base01/evidence/physical_linux_campaign/resource.json` — absent_main; main blob `None`; head blob `3db753c074e6b62fd24e5dbe2c6e22a486e0eba1`.
- `A docs/research/base01/evidence/physical_linux_campaign/session.txt` — absent_main; main blob `None`; head blob `1790692167673d7a4e75cad65773c6b17e02391d`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `8af93319083561e5af8f30127469c1e5fad7dd6f` / [review](https://github.com/t81dev/t81-foundation/pull/698#pullrequestreview-5759418400).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126286291253): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126274689016): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126274688975): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126274688974): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126274688971): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126274688958): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126274688957): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126274688930): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126270752225): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126270752137): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126270752126): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41737253289/job/126270752125): completed / cancelled.

## PR 697: docs(base01): reconcile B1 retention and insertion contracts [skip ci]

- Head `7f42e2b8d444168f74cb3a3a25d89f002ad1eea4`; actual base `a98b22fb65c80f4225151cbcd5c6934ae076cbe1` (`main`); main merge-base `a98b22fb65c80f4225151cbcd5c6934ae076cbe1`.
- Purpose: BASE-01 retention/insertion contract discrepancy audit. Decision: **REVIEW NEXT**.
- Dependency assessment: Related alternatives 699 and qualification record 698; inspect shared contract intent before choosing one.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [694, 695].
- Evidence/limits: 17 unique audit/contract files retain concrete 100 MiB versus 16 MiB contradiction and count-doubling semantic issue. Review as evidence and proposal, not amendment activation.
- Blob counts: {'absent_main': 17}. Merge simulation exit 0; tree `34c1eab0185b428625df63e66679c3628f3d66ef`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...7f42e2b8d444168f74cb3a3a25d89f002ad1eea4).

Own changed files:

- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/AMENDMENT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/AMENDMENT.md))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/EVIDENCE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/EVIDENCE.md))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/READINESS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/READINESS.md))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/REPORT.md))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/SHA256SUMS.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/SHA256SUMS.json))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/artifact-validation.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/artifact-validation.log))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-discovery-failure.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-discovery-failure.log))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-discovery.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-discovery.log))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-h07.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-h07.json))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-retry-failure.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-retry-failure.log))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/gate-fixtures.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/gate-fixtures.log))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/pr-694.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/pr-694.json))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/pr-695.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/pr-695.json))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/schema-environment-failure.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/schema-environment-failure.log))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/source-index.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/source-index.json))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/static-checks.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/static-checks.json))
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/review.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7f42e2b8d444168f74cb3a3a25d89f002ad1eea4/docs/research/base01/b1-contract-reconciliation-2026-11-03/review.json))

All changed files versus main merge-base (including parents):

- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/AMENDMENT.md` — absent_main; main blob `None`; head blob `95f7107f842a46cc53484a6d6af760f70e17a898`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/EVIDENCE.md` — absent_main; main blob `None`; head blob `fc6e1e02ee8ad8cfc1e697494c8a70da348152fa`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/READINESS.md` — absent_main; main blob `None`; head blob `a59c6f2408decf2e83a05e22c40d59419c1ee1b6`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/REPORT.md` — absent_main; main blob `None`; head blob `213a96bd46375cddd0075c2213a4e72dec22b36f`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/SHA256SUMS.json` — absent_main; main blob `None`; head blob `48b1d41ea839b201412a2e07b74e17ce2efd093a`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/artifact-validation.log` — absent_main; main blob `None`; head blob `bea579171f8548ce0714d215902f17c1d8147e4f`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-discovery-failure.log` — absent_main; main blob `None`; head blob `1648081f662c04e151505ed26461374ed24f2762`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-discovery.log` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-h07.json` — absent_main; main blob `None`; head blob `1d23c3ca02b5537902a8b86c91df322358a83064`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/commons-retry-failure.log` — absent_main; main blob `None`; head blob `1648081f662c04e151505ed26461374ed24f2762`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/gate-fixtures.log` — absent_main; main blob `None`; head blob `87d67ebc9809eb14e0bfb9280659f4006c3fcee5`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/pr-694.json` — absent_main; main blob `None`; head blob `befce5cd4a213f02aedfd02b04c438ebd42f1678`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/pr-695.json` — absent_main; main blob `None`; head blob `c1053c35544e2a7149692c8ae7e7bc2200ae4f69`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/schema-environment-failure.log` — absent_main; main blob `None`; head blob `659a17ca21e6cbbc1050778a792937b5082d1f3c`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/source-index.json` — absent_main; main blob `None`; head blob `e455eabda3e158df972064b3cb0f2fc8d2bd0729`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/evidence/static-checks.json` — absent_main; main blob `None`; head blob `84a2301e0e469c882bfc7e0196ad5c82fcccb164`.
- `A docs/research/base01/b1-contract-reconciliation-2026-11-03/review.json` — absent_main; main blob `None`; head blob `b39bcbd789e71ffd45228463fdeee9b3f4a5f539`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 685: research: investigate H04-R03 Linux enforcement feasibility

- Head `43bc3000c2a580410a8abeae094880739c6c8fa0`; actual base `dbc8c1ac05aebf9f64dccc45968b75e891b6ff21` (`main`); main merge-base `dbc8c1ac05aebf9f64dccc45968b75e891b6ff21`.
- Purpose: H04 Linux enforcement feasibility investigation. Decision: **REVIEW NEXT**.
- Dependency assessment: Depends conceptually on H04 specification/current 724/727 retention limits and 752 historical labeling.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [682, 683].
- Evidence/limits: 18 isolated diagnostic/report paths retain FEASIBILITY BLOCKED outcome. Review frozen resource-contract mapping and negative records; no benchmark engine or Phase C authority follows.
- Blob counts: {'absent_main': 18}. Merge simulation exit 0; tree `d830bc7988d312bc5320c0ef04edf3938105f280`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 11 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...43bc3000c2a580410a8abeae094880739c6c8fa0).

Own changed files:

- `A experiments/h04_r03_matched_codec/linux_feasibility/HOST_QUALIFICATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/HOST_QUALIFICATION.md))
- `A experiments/h04_r03_matched_codec/linux_feasibility/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/REPORT.md))
- `A experiments/h04_r03_matched_codec/linux_feasibility/REPRODUCE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/REPRODUCE.md))
- `A experiments/h04_r03_matched_codec/linux_feasibility/containment_probe.c` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/containment_probe.c))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence-manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence-manifest.json))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/diagnostics.final.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence/diagnostics.final.log))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/diagnostics.first-controls-failure.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence/diagnostics.first-controls-failure.log))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/download-failure.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence/download-failure.txt))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/download-manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence/download-manifest.json))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/first-boot-failure.raw.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence/first-boot-failure.raw.log))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/host-environment.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence/host-environment.json))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/kernel-source-manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence/kernel-source-manifest.json))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/successful-boot.raw.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence/successful-boot.raw.log))
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/virtual-host-gaps.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/evidence/virtual-host-gaps.log))
- `A experiments/h04_r03_matched_codec/linux_feasibility/guest_diagnostics.sh` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/guest_diagnostics.sh))
- `A experiments/h04_r03_matched_codec/linux_feasibility/native_scratch_probe.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/native_scratch_probe.py))
- `A experiments/h04_r03_matched_codec/linux_feasibility/requirements-to-evidence.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/requirements-to-evidence.json))
- `A experiments/h04_r03_matched_codec/linux_feasibility/verify_evidence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/43bc3000c2a580410a8abeae094880739c6c8fa0/experiments/h04_r03_matched_codec/linux_feasibility/verify_evidence.py))

All changed files versus main merge-base (including parents):

- `A experiments/h04_r03_matched_codec/linux_feasibility/HOST_QUALIFICATION.md` — absent_main; main blob `None`; head blob `2af1c94584fefd97a4d942ed13ef130d8ba79d19`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/REPORT.md` — absent_main; main blob `None`; head blob `207415da0ba57b3e1ee8c19954811a8f055c6f99`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/REPRODUCE.md` — absent_main; main blob `None`; head blob `c0c67647e87828c0dfd2ea0a5ba40a164c033663`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/containment_probe.c` — absent_main; main blob `None`; head blob `e211efb4cbd1190c6fa795d340bb559cb3078421`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence-manifest.json` — absent_main; main blob `None`; head blob `947dfb183e6346544aecfcd0d646cb497ceecbac`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/diagnostics.final.log` — absent_main; main blob `None`; head blob `81bba15c39ff2686e78b4c3c2f92bf5ec0ce9c80`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/diagnostics.first-controls-failure.log` — absent_main; main blob `None`; head blob `b9be7584509d1e60030d1c4ef19b7092d40e67a9`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/download-failure.txt` — absent_main; main blob `None`; head blob `c66b72dd939e12dbeac5783317f864a4f951ef00`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/download-manifest.json` — absent_main; main blob `None`; head blob `4033879259c3552bca80d416ddbae6b4fdefe2cc`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/first-boot-failure.raw.log` — absent_main; main blob `None`; head blob `7979d78a8e855fd654fe0c7b053010df8464ca5f`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/host-environment.json` — absent_main; main blob `None`; head blob `cdba20ecea1ea99010aff341de8a09b2bec3cf54`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/kernel-source-manifest.json` — absent_main; main blob `None`; head blob `44dcd64ab3a37e5cea2361d0b22d5e4be802b46c`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/successful-boot.raw.log` — absent_main; main blob `None`; head blob `b208f1a690e2ece7446151d9644744be7b7081cc`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/evidence/virtual-host-gaps.log` — absent_main; main blob `None`; head blob `40f7f8fbd818854fefac03b599ff1923b9eeb99c`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/guest_diagnostics.sh` — absent_main; main blob `None`; head blob `ea00b194a76ab3347bb8c173729c7fc49832fa4c`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/native_scratch_probe.py` — absent_main; main blob `None`; head blob `3f637e385c355ae89c31cd13ba0e8c6217c886f2`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/requirements-to-evidence.json` — absent_main; main blob `None`; head blob `15a883c7f9d7f3eff2e3a9106bd350b429fb0fc9`.
- `A experiments/h04_r03_matched_codec/linux_feasibility/verify_evidence.py` — absent_main; main blob `None`; head blob `05708e5315234db8475045b5a03248f7dd8d92cc`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125668545876): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125658234843): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125658234751): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125658234715): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125658234640): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125658234639): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125658234627): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125658234625): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125657360885): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125657360876): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125657360863): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41556623341/job/125657360859): completed / cancelled.

## PR 674: docs: T81-AIOPT-001 M2 Adversarial Workload & Experimental Design Review

- Head `67d3e3d0b24827801ec382069d66f5c8c9fd536a`; actual base `1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263` (`main`); main merge-base `1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263`.
- Purpose: AIOPT M2 adversarial polynomial-workload proposal. Decision: **REPAIR**.
- Dependency assessment: Reviews 671/672; paired critique 673; no M2 execution authorization.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [669, 671, 672].
- Evidence/limits: Two current reported issues are supported by source: unrestricted integer evaluation points permit degeneracy, and algorithms are assigned to formats while claiming representation effect. Bound domain and disentangle algorithm selection.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `8754f8bc9bb402996b5e6d1665da55fad5e5c13c`; equals main: False.
- Review: 1 COMMENTED; threads 2 current/1 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 5 pass, 2 fail, 3 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...67d3e3d0b24827801ec382069d66f5c8c9fd536a).

Own changed files:

- `A docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-WORKLOAD-REVIEW.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/67d3e3d0b24827801ec382069d66f5c8c9fd536a/docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-WORKLOAD-REVIEW.md))

All changed files versus main merge-base (including parents):

- `A docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-WORKLOAD-REVIEW.md` — absent_main; main blob `None`; head blob `e9b1a8d043d12cc75c81a9ae9c08b0929af7259a`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `5f3ab3ee807b1ebc94cd073757196b78f73e5b3c` / [review](https://github.com/t81dev/t81-foundation/pull/674#pullrequestreview-5727004467).

Failed or incomplete checks:

- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/41291153236/job/124802528698): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/41291153236/job/124802528440): completed / failure.

## PR 673: docs(research): independent M2 protocol acceptance audit

- Head `b8ab59ccb8183db9476d47259bd43de1ea34522a`; actual base `1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263` (`main`); main merge-base `1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263`.
- Purpose: Independent M2 protocol acceptance audit. Decision: **REVIEW NEXT**.
- Dependency assessment: Audits 671/672, merged 669 and corrected H04; not a duplicate of 674.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [669, 671, 672].
- Evidence/limits: One unique audit document with seven declared acceptance blockers and reviewer-relationship disclosure. Review critique against exact referenced heads; no implementation or scientific result is established.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `ec549a26006dc1fa58a95161c3f83156c454203d`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 6 pass, 3 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b8ab59ccb8183db9476d47259bd43de1ea34522a).

Own changed files:

- `A docs/research/aiopt/T81-AIOPT-001-M2-INDEPENDENT-PROTOCOL-AUDIT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ab59ccb8183db9476d47259bd43de1ea34522a/docs/research/aiopt/T81-AIOPT-001-M2-INDEPENDENT-PROTOCOL-AUDIT.md))

All changed files versus main merge-base (including parents):

- `A docs/research/aiopt/T81-AIOPT-001-M2-INDEPENDENT-PROTOCOL-AUDIT.md` — absent_main; main blob `None`; head blob `15b75d8eea97701324303b556ea3c0e9345994cb`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41285501952/job/124799810181): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/41285501952/job/124785845272): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/41285501952/job/124785845069): completed / failure.

## PR 672: docs(research): draft M2 adaptive-representation preregistration

- Head `13e1828cc4f6cbeba1b37c18345eb68a5da98147`; actual base `1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263` (`main`); main merge-base `1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263`.
- Purpose: Adaptive M2 preregistration draft. Decision: **DEFER**.
- Dependency assessment: Follows 671 decision memo; depends on M1 acceptance/669 context.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [671].
- Evidence/limits: One design-only document; adverse critiques in 673/674 must be reconciled before protocol selection or execution.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `499fab985a6f896ee87ac6efff30e34e006e604a`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 6 pass, 3 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...13e1828cc4f6cbeba1b37c18345eb68a5da98147).

Own changed files:

- `A docs/research/aiopt/T81-AIOPT-001-M2-B-PREREGISTRATION-DRAFT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/13e1828cc4f6cbeba1b37c18345eb68a5da98147/docs/research/aiopt/T81-AIOPT-001-M2-B-PREREGISTRATION-DRAFT.md))

All changed files versus main merge-base (including parents):

- `A docs/research/aiopt/T81-AIOPT-001-M2-B-PREREGISTRATION-DRAFT.md` — absent_main; main blob `None`; head blob `a28bd47b0b369d1d9623da05d452cec6899440a6`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41286452797/job/124801736792): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/41286452797/job/124788623828): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/41286452797/job/124788623723): completed / failure.

## PR 671: docs(research): M2 adaptive-selection versus GEMV decision gate

- Head `8892d094b6ec62728eb783cd7fb735cf3d57de5f`; actual base `1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263` (`main`); main merge-base `1fe1cd7a9ca01402afcfa3d53c3305f0a1ba8263`.
- Purpose: M2 adaptive selection versus GEMV decision memo. Decision: **DEFER**.
- Dependency assessment: Follows merged 669; input to 672, challenged by 673/674.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [669].
- Evidence/limits: One design-only decision gate; choose competent discriminating controls before freezing a workload. No implementation authority inferred.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `e5ca60f0923eff2764012394e6ca62a3a7899cb7`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 6 pass, 3 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...8892d094b6ec62728eb783cd7fb735cf3d57de5f).

Own changed files:

- `A docs/research/aiopt/T81-AIOPT-001-M2-B-VS-C-DECISION-GATE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/8892d094b6ec62728eb783cd7fb735cf3d57de5f/docs/research/aiopt/T81-AIOPT-001-M2-B-VS-C-DECISION-GATE.md))

All changed files versus main merge-base (including parents):

- `A docs/research/aiopt/T81-AIOPT-001-M2-B-VS-C-DECISION-GATE.md` — absent_main; main blob `None`; head blob `4db70a7a78644534244c8019e4d7d2b4c885b5ad`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41286440468/job/124801346764): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/41286440468/job/124788585610): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/41286440468/job/124788585443): completed / failure.

## PR 670: docs(research): audit AIOPT-001 acceptance and M2 protocol

- Head `685f7d958afdb9859365b6dcf37b6a73c06b5218`; actual base `005714caced20b7ce54c37c68808d0492c7567b7` (`main`); main merge-base `005714caced20b7ce54c37c68808d0492c7567b7`.
- Purpose: AIOPT acceptance and protocol evidence audit. Decision: **REVIEW NEXT**.
- Dependency assessment: Reviews 665/666/667/668; do not merge all competing M2 variants indiscriminately.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [665, 668].
- Evidence/limits: One distinct audit; reported same-author 53,096-check reproduction is historical, not independent execution by this inventory. Preserve disclosure and protocol boundaries.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `863396e18739caf5e062ae0f797670214e312120`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 6 pass, 3 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...685f7d958afdb9859365b6dcf37b6a73c06b5218).

Own changed files:

- `A docs/research/aiopt/T81-AIOPT-001-INDEPENDENT-ACCEPTANCE-AUDIT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/685f7d958afdb9859365b6dcf37b6a73c06b5218/docs/research/aiopt/T81-AIOPT-001-INDEPENDENT-ACCEPTANCE-AUDIT.md))

All changed files versus main merge-base (including parents):

- `A docs/research/aiopt/T81-AIOPT-001-INDEPENDENT-ACCEPTANCE-AUDIT.md` — absent_main; main blob `None`; head blob `596a995519c415fc66d5509bf28fd00f2b001937`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41283979863/job/124785698015): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/41283979863/job/124781427548): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/41283979863/job/124781427385): completed / failure.

## PR 668: docs(research): propose AIOPT-001 M2 discriminating workload protocol

- Head `86838191420ecc7ccdca02e41f4e009c6375417a`; actual base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6` (`main`); main merge-base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6`.
- Purpose: Wildcard-matching M2 protocol alternative. Decision: **DEFER**.
- Dependency assessment: Depends on 665 acceptance and 666/667 review; alternative to 671/672/674.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [665, 666, 667].
- Evidence/limits: One unique protocol differs from later M2 design; no demonstrated semantic duplicate. Choose workload only after independent M1 acceptance and competent controls.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `4f84b46ddddaa376b0a21a8d72c4d63a2ce544cc`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...86838191420ecc7ccdca02e41f4e009c6375417a).

Own changed files:

- `A docs/research/aiopt/T81-AIOPT-001-M2-PROTOCOL-DRAFT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/86838191420ecc7ccdca02e41f4e009c6375417a/docs/research/aiopt/T81-AIOPT-001-M2-PROTOCOL-DRAFT.md))

All changed files versus main merge-base (including parents):

- `A docs/research/aiopt/T81-AIOPT-001-M2-PROTOCOL-DRAFT.md` — absent_main; main blob `None`; head blob `df6063058960568d4c41ebcf4190d7a4007f4e0e`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124799352508): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124783948386): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124783948372): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124783948368): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124783948363): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124783948321): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124783948320): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124783948318): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124778974526): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124778974522): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124778974515): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41282497125/job/124778974504): completed / failure.

## PR 667: docs(research): reconcile AIOPT-001 M1 evidence and M2 design gate

- Head `1edf70bbffc9168c27be5c6c19cf97388f6991a4`; actual base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6` (`main`); main merge-base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6`.
- Purpose: M1 evidence/M2 design reconciliation. Decision: **REVIEW NEXT**.
- Dependency assessment: Depends on 665/666 acceptance evidence; overlaps subject matter, not paths, with 670.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [665, 666].
- Evidence/limits: One distinct continuity document; historical correctness and design status must stay separately labeled.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `699e1f404842dd29ba74fbf91faf576e8dc0a308`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 10 fail, 8 skip, 2 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...1edf70bbffc9168c27be5c6c19cf97388f6991a4).

Own changed files:

- `A docs/research/aiopt/T81-AIOPT-001-M1-M2-RECONCILIATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/1edf70bbffc9168c27be5c6c19cf97388f6991a4/docs/research/aiopt/T81-AIOPT-001-M1-M2-RECONCILIATION.md))

All changed files versus main merge-base (including parents):

- `A docs/research/aiopt/T81-AIOPT-001-M1-M2-RECONCILIATION.md` — absent_main; main blob `None`; head blob `6db450719403ecf2eaf609f3a5739010bcf9ed69`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124808804242): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124786245743): completed / cancelled.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124786245695): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124786245694): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124786245674): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124786245661): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124786245660): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124786245626): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124779914719): completed / cancelled.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124779914615): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124779914609): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41282347932/job/124779914592): completed / failure.

## PR 666: docs(research): independent scientific review of T81-AIOPT-001

- Head `a14cf48a9ec8494f876819ec457be5478dfc08d5`; actual base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6` (`main`); main merge-base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6`.
- Purpose: Scientific review of AIOPT M1. Decision: **REVIEW NEXT**.
- Dependency assessment: Reviews M1 665 and motivates M2; separate from 670 reconciliation.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [665].
- Evidence/limits: One corrected review document; three old inline findings are outdated. Review latest bounded claims and comparators rather than counting them as still-open defects.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `8e4a4daf968487f9d80525e8e6d399750feb432a`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/3 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 6 pass, 3 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...a14cf48a9ec8494f876819ec457be5478dfc08d5).

Own changed files:

- `A docs/research/aiopt/T81-AIOPT-001-SCIENTIFIC-REVIEW.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a14cf48a9ec8494f876819ec457be5478dfc08d5/docs/research/aiopt/T81-AIOPT-001-SCIENTIFIC-REVIEW.md))

All changed files versus main merge-base (including parents):

- `A docs/research/aiopt/T81-AIOPT-001-SCIENTIFIC-REVIEW.md` — absent_main; main blob `None`; head blob `917d8bcb28562b2570cb6346731e158aec3c705c`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `bbd60d2074b4d4c40b02a8ae86026e710d81ceef` / [review](https://github.com/t81dev/t81-foundation/pull/666#pullrequestreview-5726865725).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41295240433/job/124814672213): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/41295240433/job/124814603228): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/41295240433/job/124814603055): completed / failure.

## PR 665: docs(research): establish T81-AIOPT-001 M1 correctness protocol

- Head `e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db`; actual base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6` (`main`); main merge-base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6`.
- Purpose: Reconstructed fixed-catalog M1 correctness apparatus. Decision: **REVIEW NEXT**.
- Dependency assessment: Prerequisite evidence for 666/667/670 and M2 drafts; no new experiment requested.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: 47 unique protocol/oracle/adapters/raw verification paths. Review reconstructed provenance and oracle independence; historical exhaustive checks do not prove performance advantage.
- Blob counts: {'absent_main': 47}. Merge simulation exit 0; tree `1ce54ec9bd0c63bbb10a577937217b13f45c902c`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 11 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db).

Own changed files:

- `A docs/research/aiopt/T81-AIOPT-001-M1-PROTOCOL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/docs/research/aiopt/T81-AIOPT-001-M1-PROTOCOL.md))
- `A experiments/aiopt_001_fixed_catalog/M1_VERIFICATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/M1_VERIFICATION_REPORT.md))
- `A experiments/aiopt_001_fixed_catalog/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/README.md))
- `A experiments/aiopt_001_fixed_catalog/m1.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/m1.py))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/artifacts.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/artifacts.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-assertion-line-hits.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-assertion-line-hits.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-defects.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-defects.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-independent-initial-fixture.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-independent-initial-fixture.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-independent-optimized.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-independent-optimized.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-independent.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-independent.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-instrumented-normal.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-instrumented-normal.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-instrumented-optimized.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-instrumented-optimized.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-1.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-1.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-1.stdout.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-1.stdout.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-2.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-2.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-2.stdout.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-2.stdout.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/corrected-precommit-independent-optimized.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/corrected-precommit-independent-optimized.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/corrected-precommit-independent.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/corrected-precommit-independent.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/discrepancy-record.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/discrepancy-record.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/execution.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/execution.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-active-checks-normal.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-active-checks-normal.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-active-checks-optimized.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-active-checks-optimized.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.subject.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.subject.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.subject.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.subject.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.stdout))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.subject.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.subject.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.subject.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.subject.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.stdout))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.subject.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.subject.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.subject.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.subject.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.stdout))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.subject.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.subject.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.subject.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.subject.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.stderr` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.stderr))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.stdout` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.stdout))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/initial-verify-fixture.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/initial-verify-fixture.py))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/reproduction-record.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/reproduction-record.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/review-record.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/review-record.json))
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/schema-validation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verification-evidence/schema-validation.json))
- `A experiments/aiopt_001_fixed_catalog/verify_m1.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db/experiments/aiopt_001_fixed_catalog/verify_m1.py))

All changed files versus main merge-base (including parents):

- `A docs/research/aiopt/T81-AIOPT-001-M1-PROTOCOL.md` — absent_main; main blob `None`; head blob `96313d04d7a0a469d08d1105909c6190683084ba`.
- `A experiments/aiopt_001_fixed_catalog/M1_VERIFICATION_REPORT.md` — absent_main; main blob `None`; head blob `84b6d7188fa987c516a640891246f3a8a89ff0a8`.
- `A experiments/aiopt_001_fixed_catalog/README.md` — absent_main; main blob `None`; head blob `b732892045e982b7544c20b15339d633b02edc1a`.
- `A experiments/aiopt_001_fixed_catalog/m1.py` — absent_main; main blob `None`; head blob `753824598a13aab2e502b2a98b5006e1b876097d`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/artifacts.json` — absent_main; main blob `None`; head blob `903f98f4a0b034d94ca5de661bfadadc360aefc9`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-assertion-line-hits.json` — absent_main; main blob `None`; head blob `2004c332e7062425da263b3c744714bc2f2554f7`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-defects.json` — absent_main; main blob `None`; head blob `f044d9444602ccbf18e3d19fedf115186a20a938`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-independent-initial-fixture.json` — absent_main; main blob `None`; head blob `bee6d5d9131debd04e7d5b37788131dbc5d70a2f`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-independent-optimized.json` — absent_main; main blob `None`; head blob `52bc2623fd7f3bd998d4f20bbdec3b60c0d50e34`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-independent.json` — absent_main; main blob `None`; head blob `6bff349ff94740fd94fb1c4afa39ae9d9ec87ebd`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-instrumented-normal.json` — absent_main; main blob `None`; head blob `065343df246d50438d3923350f4d14228d807ab0`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-instrumented-optimized.json` — absent_main; main blob `None`; head blob `c77f46237ea162fe28d4b880b0995b0705157a20`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-1.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-1.stdout.json` — absent_main; main blob `None`; head blob `972fd66695d035384821af40d67a968b0d07a763`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-2.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/baseline-run-2.stdout.json` — absent_main; main blob `None`; head blob `972fd66695d035384821af40d67a968b0d07a763`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/corrected-precommit-independent-optimized.json` — absent_main; main blob `None`; head blob `b2bc3b74b0c2b3abc70cb42ed3ad2d4f8a032edb`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/corrected-precommit-independent.json` — absent_main; main blob `None`; head blob `e66166c7304b7ca9c8b45b51abc82db1b597752b`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/discrepancy-record.json` — absent_main; main blob `None`; head blob `768d3584af462f6494035e381124679d32313bd3`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/execution.json` — absent_main; main blob `None`; head blob `33e408395479dcc3b53b555be95353c028a56c07`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-active-checks-normal.json` — absent_main; main blob `None`; head blob `1373b6b5a19f0cf0abc5ec687652b680705711b7`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-active-checks-optimized.json` — absent_main; main blob `None`; head blob `1373b6b5a19f0cf0abc5ec687652b680705711b7`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.subject.json` — absent_main; main blob `None`; head blob `8c3aafe654700362faf08fb3a47af27c9e2e1a9b`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.subject.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.json` — absent_main; main blob `None`; head blob `e82150ce9390ac1172ea1f97e19966f2b1e9539d`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-1.verification.stdout` — absent_main; main blob `None`; head blob `bfa5a8dfb3537fb59e723e73b4f5f7d587c712ba`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.subject.json` — absent_main; main blob `None`; head blob `8c3aafe654700362faf08fb3a47af27c9e2e1a9b`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.subject.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.json` — absent_main; main blob `None`; head blob `e82150ce9390ac1172ea1f97e19966f2b1e9539d`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-normal-2.verification.stdout` — absent_main; main blob `None`; head blob `bfa5a8dfb3537fb59e723e73b4f5f7d587c712ba`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.subject.json` — absent_main; main blob `None`; head blob `8c3aafe654700362faf08fb3a47af27c9e2e1a9b`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.subject.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.json` — absent_main; main blob `None`; head blob `2e02c25af887ea5016b928e10073b9dab75c1681`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-1.verification.stdout` — absent_main; main blob `None`; head blob `8a0e5267d1f4e7e4d45a5f80e3ea569939b7fc41`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.subject.json` — absent_main; main blob `None`; head blob `8c3aafe654700362faf08fb3a47af27c9e2e1a9b`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.subject.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.json` — absent_main; main blob `None`; head blob `2e02c25af887ea5016b928e10073b9dab75c1681`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.stderr` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/final-optimized-2.verification.stdout` — absent_main; main blob `None`; head blob `8a0e5267d1f4e7e4d45a5f80e3ea569939b7fc41`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/initial-verify-fixture.py` — absent_main; main blob `None`; head blob `54f6d3847ec431949af81636140b579d2d22e9a4`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/reproduction-record.json` — absent_main; main blob `None`; head blob `5043364b1e79c94abfb96bcf48499acd9b485015`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/review-record.json` — absent_main; main blob `None`; head blob `ac14a533c87d2c7bf9d556bcc744dd8dfc3fadde`.
- `A experiments/aiopt_001_fixed_catalog/verification-evidence/schema-validation.json` — absent_main; main blob `None`; head blob `aaf58e65dabcf1b5fb6728fb855fe59dca26b88e`.
- `A experiments/aiopt_001_fixed_catalog/verify_m1.py` — absent_main; main blob `None`; head blob `32be0d2f52c39089a4856ea2dba06416c4cd5dc3`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124818071558): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124809689158): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124809689155): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124809689148): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124809689140): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124809689119): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124809689108): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124809689107): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124805758857): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124805758820): completed / cancelled.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124805758789): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41282007834/job/124805758679): completed / failure.

## PR 664: docs(research): final evidence reconciliation for baseline assessment

- Head `b6dea482cbb060e38650c7d2b31ab5237a65a497`; actual base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6` (`main`); main merge-base `affeea65ba5de6c46bfad5d05cffa0e16bd7c2a6`.
- Purpose: Baseline scientific-evidence reconciliation. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Overlaps audit subjects 659/662 and archaeology 657, not identical files.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Eight unique ledger/report files; four earlier threads outdated. Inspect latest retained ledgers and primary source availability before treating reconciliation claims as independently validated.
- Blob counts: {'absent_main': 8}. Merge simulation exit 0; tree `08e5b8253ad23fb027e7cd7656fcbbd49571b57d`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/4 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 11 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b6dea482cbb060e38650c7d2b31ab5237a65a497).

Own changed files:

- `A docs/research/baseline-assessment/BENCHMARK_LEDGER.csv` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6dea482cbb060e38650c7d2b31ab5237a65a497/docs/research/baseline-assessment/BENCHMARK_LEDGER.csv))
- `A docs/research/baseline-assessment/CAPABILITY_MATRIX.csv` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6dea482cbb060e38650c7d2b31ab5237a65a497/docs/research/baseline-assessment/CAPABILITY_MATRIX.csv))
- `A docs/research/baseline-assessment/FINAL_RECONCILIATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6dea482cbb060e38650c7d2b31ab5237a65a497/docs/research/baseline-assessment/FINAL_RECONCILIATION.md))
- `A docs/research/baseline-assessment/GAPS_AND_PRIORITIES.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6dea482cbb060e38650c7d2b31ab5237a65a497/docs/research/baseline-assessment/GAPS_AND_PRIORITIES.md))
- `A docs/research/baseline-assessment/HYPOTHESIS_STATUS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6dea482cbb060e38650c7d2b31ab5237a65a497/docs/research/baseline-assessment/HYPOTHESIS_STATUS.md))
- `A docs/research/baseline-assessment/METHODOLOGY.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6dea482cbb060e38650c7d2b31ab5237a65a497/docs/research/baseline-assessment/METHODOLOGY.md))
- `A docs/research/baseline-assessment/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6dea482cbb060e38650c7d2b31ab5237a65a497/docs/research/baseline-assessment/README.md))
- `A docs/research/baseline-assessment/RFC_MATRIX.csv` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6dea482cbb060e38650c7d2b31ab5237a65a497/docs/research/baseline-assessment/RFC_MATRIX.csv))

All changed files versus main merge-base (including parents):

- `A docs/research/baseline-assessment/BENCHMARK_LEDGER.csv` — absent_main; main blob `None`; head blob `7f0df1d6e9fe11e01787aa28085cfea1f3c6db85`.
- `A docs/research/baseline-assessment/CAPABILITY_MATRIX.csv` — absent_main; main blob `None`; head blob `bda52c93cfa0c01dc74a6678d39cfeb822ef66f0`.
- `A docs/research/baseline-assessment/FINAL_RECONCILIATION.md` — absent_main; main blob `None`; head blob `8b47363f6a0425f73c7be0b29494ed0a230a0159`.
- `A docs/research/baseline-assessment/GAPS_AND_PRIORITIES.md` — absent_main; main blob `None`; head blob `f3ff7c01c5edc619b870e8cf1477b17a5053b27d`.
- `A docs/research/baseline-assessment/HYPOTHESIS_STATUS.md` — absent_main; main blob `None`; head blob `bf3f2cd8974f7f3be2eb80bb4aa4e10eb6921f81`.
- `A docs/research/baseline-assessment/METHODOLOGY.md` — absent_main; main blob `None`; head blob `6f4ea6919eea99c666ebdea3e6e14cf072205325`.
- `A docs/research/baseline-assessment/README.md` — absent_main; main blob `None`; head blob `47601de4cd0a885da35d1c8b8d87f8783173d5d4`.
- `A docs/research/baseline-assessment/RFC_MATRIX.csv` — absent_main; main blob `None`; head blob `159b9d34778d3a7cf9197e71764bf0820734f331`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `c117e685a7840f0f285d256e2fdde855b1996686` / [review](https://github.com/t81dev/t81-foundation/pull/664#pullrequestreview-5726637992).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124805691549): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124785388182): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124785388079): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124785387987): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124785387984): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124785387983): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124785387982): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124785387963): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124780283524): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124780283467): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124780283456): completed / cancelled.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41280244483/job/124780283412): completed / failure.

## PR 662: Extended testing results (2026-10-31): full build, ctest, and dynamic confirmation of audit findings

- Head `a1ca08d3fa59024f37858e42aeea7ec4d082c674`; actual base `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05` (`main`); main merge-base `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`.
- Purpose: Historical extended build/test audit. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Explicit follow-up to 659 at audited e108fda; no fresh test execution here.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [659].
- Evidence/limits: Single unique root-level historical report; no hosted checks. Verify source/run pin and artifact retention plus allowed publication location; not current-main build evidence.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `e4eebfce1a6fdffde3cd175865353d0f0a15fce4`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...a1ca08d3fa59024f37858e42aeea7ec4d082c674).

Own changed files:

- `A TESTING-2026-10-31.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a1ca08d3fa59024f37858e42aeea7ec4d082c674/TESTING-2026-10-31.md))

All changed files versus main merge-base (including parents):

- `A TESTING-2026-10-31.md` — absent_main; main blob `None`; head blob `34755171e526fc62b16db1c9b8e9780ded6890a5`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 659: External audit findings (2026-10-31): ai-context handoff + full-repo review

- Head `5af7b6140d40614c7e5c6b0e5b06d97cb78607ca`; actual base `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05` (`main`); main merge-base `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`.
- Purpose: Historical external repository audit. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Parent subject of 662; not superseded merely because main advanced.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [637].
- Evidence/limits: Single unique root-level report; source observations target an old pinned revision. Preserve evidence, assess claims individually against main, and verify publication governance before intake.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `91c124974ff7fecf87c26aa549494e76aeaf2c77`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...5af7b6140d40614c7e5c6b0e5b06d97cb78607ca).

Own changed files:

- `A AUDIT-FINDINGS-2026-10-31.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/5af7b6140d40614c7e5c6b0e5b06d97cb78607ca/AUDIT-FINDINGS-2026-10-31.md))

All changed files versus main merge-base (including parents):

- `A AUDIT-FINDINGS-2026-10-31.md` — absent_main; main blob `None`; head blob `f4cc049a367ce39f24cf4efd5a838deee89d7085`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 657: fix(archaeology): resolve PR #653 P1 Codex findings

- Head `db1b4beff94d48e8ce7c71dfcb449dcd9684b75a`; actual base `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05` (`main`); main merge-base `f775680aba7db200691c0f4e2a3174b0e7e51581`.
- Purpose: Archaeology classifier and research-claim correction. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Shares current AI-context paths with 640/764 and archived claim subject with 664.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [653].
- Evidence/limits: 23 unique/divergent paths include classifier/tests and archived inventory, not just handoff text. Parent 653 is closed without merge. Reconcile surviving source changes and preserve receipts before any closure.
- Blob counts: {'different': 7, 'absent_main': 16}. Merge simulation exit 1; tree `65e61b7c13bce3fc8e590ab1ba011673247bec6f`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...db1b4beff94d48e8ce7c71dfcb449dcd9684b75a).

Own changed files:

- `M docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md))
- `M docs/ai-context/BOOTSTRAP.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/BOOTSTRAP.md))
- `M docs/ai-context/DECISIONS_AND_GOVERNANCE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/DECISIONS_AND_GOVERNANCE.md))
- `M docs/ai-context/EVIDENCE_AND_FINDINGS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/EVIDENCE_AND_FINDINGS.md))
- `M docs/ai-context/RESEARCH_IDENTITY.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/RESEARCH_IDENTITY.md))
- `M docs/ai-context/START_HERE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/START_HERE.md))
- `M docs/ai-context/SYSTEM_ARCHITECTURE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/SYSTEM_ARCHITECTURE.md))
- `A docs/ai-context/archaeology/BUDGET.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/archaeology/BUDGET.jsonl))
- `A docs/ai-context/archaeology/HANDOFF.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/archaeology/HANDOFF.md))
- `A docs/ai-context/archaeology/OPEN_QUESTIONS.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/archaeology/OPEN_QUESTIONS.jsonl))
- `A docs/ai-context/archaeology/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/archaeology/README.md))
- `A docs/ai-context/archaeology/STATE.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/ai-context/archaeology/STATE.json))
- `A docs/records/inventories/repository-archaeology/CLAIMS.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/records/inventories/repository-archaeology/CLAIMS.jsonl))
- `A docs/records/inventories/repository-archaeology/COVERAGE.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/records/inventories/repository-archaeology/COVERAGE.json))
- `A docs/records/inventories/repository-archaeology/EXECUTIVE_SUMMARY.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/records/inventories/repository-archaeology/EXECUTIVE_SUMMARY.md))
- `A docs/records/inventories/repository-archaeology/FILES.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/records/inventories/repository-archaeology/FILES.jsonl))
- `A docs/records/inventories/repository-archaeology/HISTORY.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/records/inventories/repository-archaeology/HISTORY.jsonl))
- `A docs/records/inventories/repository-archaeology/METHOD.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/records/inventories/repository-archaeology/METHOD.md))
- `A docs/records/inventories/repository-archaeology/RELATIONSHIPS.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/records/inventories/repository-archaeology/RELATIONSHIPS.jsonl))
- `A docs/records/inventories/repository-archaeology/RELOCATION_CANDIDATES.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/records/inventories/repository-archaeology/RELOCATION_CANDIDATES.jsonl))
- `A docs/records/inventories/repository-archaeology/VALIDATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/docs/records/inventories/repository-archaeology/VALIDATION.md))
- `A tests/ci/archaeology_inventory_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/tests/ci/archaeology_inventory_test.py))
- `A tools/archaeology_classifier.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/db1b4beff94d48e8ce7c71dfcb449dcd9684b75a/tools/archaeology_classifier.py))

All changed files versus main merge-base (including parents):

- `M docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md` — different; main blob `526a9750ef88f1a7c445ae3adfd115f46bdf87a8`; head blob `ef85f372d8e3aa5dcd9046486fd8193ae82c8f25`.
- `M docs/ai-context/BOOTSTRAP.md` — different; main blob `8da6d42fa87744c6e08a76c03cdf75eb7d5cf8b6`; head blob `97cbb66163a1da116c682d03811a27447e60c76b`.
- `M docs/ai-context/DECISIONS_AND_GOVERNANCE.md` — different; main blob `f23190dcf40d159bcfd4ce69e41dd3d3abe14bc7`; head blob `305ca8cb9c4b7f50bfc8bfdfa2e80fdb1ce8be93`.
- `M docs/ai-context/EVIDENCE_AND_FINDINGS.md` — different; main blob `9d08156c7f4e0401fa2e03aa05d2877d991d7f04`; head blob `71d335444bd502e574ee0b14cde7b4fbc5b608a4`.
- `M docs/ai-context/RESEARCH_IDENTITY.md` — different; main blob `34a9d2b105023e6f23ae37937f470b922979446e`; head blob `0a972389a28e62205d72c8627b11575e84af835b`.
- `M docs/ai-context/START_HERE.md` — different; main blob `1f790fff2355f61f7639fa65486d486ff9b6d5c8`; head blob `90542092590a9bff6f06a63795b208878e79f974`.
- `M docs/ai-context/SYSTEM_ARCHITECTURE.md` — different; main blob `a558e8b6de3caa8aa00140638f75437d1fc441de`; head blob `df6c3b1fd5f9ff98945348649ad6bd372b1a0aaf`.
- `A docs/ai-context/archaeology/BUDGET.jsonl` — absent_main; main blob `None`; head blob `a3a6bddc066ad124c35a7d64d00b32e1f7c70604`.
- `A docs/ai-context/archaeology/HANDOFF.md` — absent_main; main blob `None`; head blob `e7efa3000dca35f363a85be8e27166a4fed15d22`.
- `A docs/ai-context/archaeology/OPEN_QUESTIONS.jsonl` — absent_main; main blob `None`; head blob `6c9c81dabb15f24ba68e4abb1f5fc96ed375ca64`.
- `A docs/ai-context/archaeology/README.md` — absent_main; main blob `None`; head blob `6cb58840d46874599cdadbfa8f32d483d0760a40`.
- `A docs/ai-context/archaeology/STATE.json` — absent_main; main blob `None`; head blob `cc9a027baf3f241c3501bb9a9da2080cfeda19a9`.
- `A docs/records/inventories/repository-archaeology/CLAIMS.jsonl` — absent_main; main blob `None`; head blob `3faaa1e50f5a4879ee9f2ddb621c1d1b75b0b8f3`.
- `A docs/records/inventories/repository-archaeology/COVERAGE.json` — absent_main; main blob `None`; head blob `ce16e296139dbd3cb88c0b3fb51ce5cc168fe19a`.
- `A docs/records/inventories/repository-archaeology/EXECUTIVE_SUMMARY.md` — absent_main; main blob `None`; head blob `75ee40997988a6dff9a97eff5e0e8181e414e4ce`.
- `A docs/records/inventories/repository-archaeology/FILES.jsonl` — absent_main; main blob `None`; head blob `1e985c6db9bbf451ee91ddd2c04ce9e910fccb22`.
- `A docs/records/inventories/repository-archaeology/HISTORY.jsonl` — absent_main; main blob `None`; head blob `c18862bd80b744d5323e99ee35809ee94397d220`.
- `A docs/records/inventories/repository-archaeology/METHOD.md` — absent_main; main blob `None`; head blob `f4ad0b74e8e509a11d04677fbd4fcecfc3578672`.
- `A docs/records/inventories/repository-archaeology/RELATIONSHIPS.jsonl` — absent_main; main blob `None`; head blob `4fdeb7816c02d81c59097d6cf5d018af51c60081`.
- `A docs/records/inventories/repository-archaeology/RELOCATION_CANDIDATES.jsonl` — absent_main; main blob `None`; head blob `0fe20675f6b03a0a649168c3a7d4566252145a73`.
- `A docs/records/inventories/repository-archaeology/VALIDATION.md` — absent_main; main blob `None`; head blob `65e78b4b2ce9be9b77fd37288722305b7d038da6`.
- `A tests/ci/archaeology_inventory_test.py` — absent_main; main blob `None`; head blob `5ea4810e0106bdbce1e72e1265628540cb0c85f4`.
- `A tools/archaeology_classifier.py` — absent_main; main blob `None`; head blob `0c17491476e9c8563bb7d05e15de318d6218ae04`.

Overlaps with other PR own deltas:

- #764: `docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`, `docs/ai-context/START_HERE.md`
- #640: `docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`, `docs/ai-context/BOOTSTRAP.md`, `docs/ai-context/DECISIONS_AND_GOVERNANCE.md`, `docs/ai-context/EVIDENCE_AND_FINDINGS.md`, `docs/ai-context/RESEARCH_IDENTITY.md`, `docs/ai-context/START_HERE.md`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 652: Final Publication Corrections: Baseline Maintenance Repair and ADM-2 M1 Header Isolation

- Head `e2e5a25b17ab37198e5fe2d53b68792963fd088f`; actual base `f775680aba7db200691c0f4e2a3174b0e7e51581` (`main`); main merge-base `f775680aba7db200691c0f4e2a3174b0e7e51581`.
- Purpose: Quantization/header and ADM-2 publication repairs. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Overlaps public API/SDK constraints in 650 and broad core correction 536.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [649].
- Evidence/limits: Six production/test paths differ from main. Older no-findings bot summary covers a predecessor, not current compatibility. Separate minimal compile repairs from API/layout effects and verify current source.
- Blob counts: {'different': 5, 'absent_main': 1}. Merge simulation exit 0; tree `9b111816faef7c982fa44877cb270e0660d95522`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 11 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...e2e5a25b17ab37198e5fe2d53b68792963fd088f).

Own changed files:

- `M codec/advanced_ternary_quantization.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e2e5a25b17ab37198e5fe2d53b68792963fd088f/codec/advanced_ternary_quantization.cpp))
- `M include/t81/axion/context.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e2e5a25b17ab37198e5fe2d53b68792963fd088f/include/t81/axion/context.hpp))
- `M include/t81/codec/advanced_ternary_quantization.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e2e5a25b17ab37198e5fe2d53b68792963fd088f/include/t81/codec/advanced_ternary_quantization.hpp))
- `M include/t81/inference/batch_inference_engine.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e2e5a25b17ab37198e5fe2d53b68792963fd088f/include/t81/inference/batch_inference_engine.hpp))
- `M inference/batch_inference_engine.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e2e5a25b17ab37198e5fe2d53b68792963fd088f/inference/batch_inference_engine.cpp))
- `A tests/cpp/inference_queue_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e2e5a25b17ab37198e5fe2d53b68792963fd088f/tests/cpp/inference_queue_test.cpp))

All changed files versus main merge-base (including parents):

- `M codec/advanced_ternary_quantization.cpp` — different; main blob `1130efe4fdbbcd07d1b95b1cccfb8d19c641802c`; head blob `0a3f130bd529d48078725a3df70b1e87fafe1ebb`.
- `M include/t81/axion/context.hpp` — different; main blob `5f3a2e8ba30cf51141108ee22bfde69659b935cd`; head blob `b311f508c9eb6ae34174e2a13564c0f7e2dbad34`.
- `M include/t81/codec/advanced_ternary_quantization.hpp` — different; main blob `dbf5ff886ebd673a8931276d34fa207c78e72329`; head blob `6466725ccc80333495819b8a28afeeb3b4383181`.
- `M include/t81/inference/batch_inference_engine.hpp` — different; main blob `934e866c5fc78f5d19e1ff68438d388c4f832487`; head blob `48c5c2d117bdad131e44a5863993975b2d94d39b`.
- `M inference/batch_inference_engine.cpp` — different; main blob `93d1269af1a089742c141365edb1ccfe3de6ea3c`; head blob `30039089cca5d0ea216714d722c70761f59e61b2`.
- `A tests/cpp/inference_queue_test.cpp` — absent_main; main blob `None`; head blob `c5199589e32984013d4a61c77e2a8820fb627e3e`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124385348347): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124375022971): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124375022960): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124375022935): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124375022914): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124375022885): completed / cancelled.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124375022865): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124375022848): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124373780769): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124373780751): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124373780729): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41146731138/job/124373780697): completed / failure.

## PR 650: docs(rfc): propose C++ SDK 2.0.0 compatibility transition (A.1)

- Head `137faac2830f9f21b66667a3cb103df79b550e0d`; actual base `f775680aba7db200691c0f4e2a3174b0e7e51581` (`main`); main merge-base `f775680aba7db200691c0f4e2a3174b0e7e51581`.
- Purpose: Proposed C++ SDK 2 compatibility transition. Decision: **DEFER**.
- Dependency assessment: Related to 649/648/580 governance; index conflicts with newer main.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [649].
- Evidence/limits: RFC proposal and index only. Explicit human decision is required before version/API-lock changes; no transition approved by existence of draft.
- Blob counts: {'absent_main': 1, 'different': 1}. Merge simulation exit 1; tree `36bc057553fb86b10dd903893dcc70311af2efd0`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...137faac2830f9f21b66667a3cb103df79b550e0d).

Own changed files:

- `A spec/rfcs/RFC-00E9-cpp-sdk-2-compatibility-transition.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/137faac2830f9f21b66667a3cb103df79b550e0d/spec/rfcs/RFC-00E9-cpp-sdk-2-compatibility-transition.md))
- `M spec/rfcs/index.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/137faac2830f9f21b66667a3cb103df79b550e0d/spec/rfcs/index.md))

All changed files versus main merge-base (including parents):

- `A spec/rfcs/RFC-00E9-cpp-sdk-2-compatibility-transition.md` — absent_main; main blob `None`; head blob `e31c690ae7e70c4603c9b9263c95aefa0d638ef1`.
- `M spec/rfcs/index.md` — different; main blob `2c05638e9bc8db73d60a4d25c2fff1ebad3e7eda`; head blob `539dc468335a0a6fa9a2d87aba2156f8cf026f3e`.

Overlaps with other PR own deltas:

- #580: `spec/rfcs/index.md`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123997799378): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123982407351): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123982406845): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123982406829): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123982406827): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123982406818): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123982406795): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123982406761): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123980799439): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123980799426): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123980799416): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/41030983622/job/123980799413): completed / failure.

## PR 649: Integrate non-API governance corrections while preserving the API-lock boundary

- Head `3a28eae9c11bbd18b09b71509fa7f74e0584bc24`; actual base `f775680aba7db200691c0f4e2a3174b0e7e51581` (`main`); main merge-base `f775680aba7db200691c0f4e2a3174b0e7e51581`.
- Purpose: Consolidated non-API governance corrections. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Overlaps all six paths of 574, seven of 575, ten of 576 and sixteen of 577; current main conflicts.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [102, 577].
- Evidence/limits: 46 paths combine workflow/validator changes and old raw audit records. Compare retained deltas from 574-577 and newer main before choosing canonical integration; do not equate shared receipts with whole-patch duplication.
- Blob counts: {'different': 17, 'absent_main': 29}. Merge simulation exit 1; tree `092705e039be5b19267c1106a5cc79c513454d8c`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 9 pass, 2 fail, 24 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...3a28eae9c11bbd18b09b71509fa7f74e0584bc24).

Own changed files:

- `M .github/workflows/ai-experiments-ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/.github/workflows/ai-experiments-ci.yml))
- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/.github/workflows/ci.yml))
- `M .github/workflows/docker.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/.github/workflows/docker.yml))
- `M .github/workflows/python-wheels.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/.github/workflows/python-wheels.yml))
- `M .github/workflows/qemu-boot-x86.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/.github/workflows/qemu-boot-x86.yml))
- `M .github/workflows/qemu-boot.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/.github/workflows/qemu-boot.yml))
- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/CMakeLists.txt))
- `M README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/README.md))
- `M docs/explanation/ARCHITECTURE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/explanation/ARCHITECTURE.md))
- `A docs/records/audits/2026-10-architecture-targets/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-architecture-targets/README.md))
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-architecture-targets/evidence.tar.gz))
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-architecture-targets/inventory.json))
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-ci-failure-propagation/README.md))
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz))
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-ci-failure-propagation/results.json))
- `A docs/records/audits/2026-10-governance-deadlock/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-governance-deadlock/README.md))
- `A docs/records/audits/2026-10-governance-deadlock/after.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-governance-deadlock/after.json))
- `A docs/records/audits/2026-10-governance-deadlock/before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-governance-deadlock/before.json))
- `A docs/records/audits/2026-10-governance-deadlock/path-only.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-governance-deadlock/path-only.json))
- `A docs/records/audits/2026-10-governance-deadlock/regressions.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-governance-deadlock/regressions.log))
- `A docs/records/audits/2026-10-governance-deadlock/target-regressions.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-governance-deadlock/target-regressions.log))
- `A docs/records/audits/2026-10-workflow-supply-chain/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-workflow-supply-chain/README.md))
- `A docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json))
- `A docs/records/audits/2026-10-workflow-supply-chain/before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-workflow-supply-chain/before.json))
- `A docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json))
- `A docs/records/audits/2026-10-workflow-supply-chain/inventory.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-workflow-supply-chain/inventory.md))
- `A docs/records/audits/2026-10-workflow-supply-chain/protection.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-workflow-supply-chain/protection.json))
- `A docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json))
- `M scripts/ci/check_architecture_coherence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/scripts/ci/check_architecture_coherence.py))
- `M scripts/ci/check_architecture_targets.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/scripts/ci/check_architecture_targets.py))
- `A scripts/ci/test_failure_propagation.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/scripts/ci/test_failure_propagation.py))
- `M scripts/governance/check_docs_governance_hygiene.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/scripts/governance/check_docs_governance_hygiene.py))
- `M scripts/governance/check_docs_structure.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/scripts/governance/check_docs_structure.py))
- `M scripts/governance/check_rfc_lifecycle_hygiene.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/scripts/governance/check_rfc_lifecycle_hygiene.py))
- `M scripts/governance/check_spec_code_alignment_baseline.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/scripts/governance/check_spec_code_alignment_baseline.py))
- `M scripts/governance/check_status_label_coherence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/scripts/governance/check_status_label_coherence.py))
- `A tests/ci/check_architecture_targets_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/ci/check_architecture_targets_test.py))
- `A tests/ci/fixtures/workflow-supply-chain/permissions.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/ci/fixtures/workflow-supply-chain/permissions.json))
- `A tests/ci/fixtures/workflow-supply-chain/pins.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/ci/fixtures/workflow-supply-chain/pins.json))
- `A tests/ci/governance_deadlock_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/ci/governance_deadlock_test.py))
- `A tests/ci/governance_layout_paths_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/ci/governance_layout_paths_test.py))
- `A tests/ci/governance_maturity_metadata_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/ci/governance_maturity_metadata_test.py))
- `A tests/ci/rfc_status_table_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/ci/rfc_status_table_test.py))
- `A tests/ci/workflow_action_smoke.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/ci/workflow_action_smoke.py))
- `A tests/ci/workflow_supply_chain_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/ci/workflow_supply_chain_test.py))
- `M tests/cpp/tisc_opcode_matrix_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/3a28eae9c11bbd18b09b71509fa7f74e0584bc24/tests/cpp/tisc_opcode_matrix_test.cpp))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `534c80263b70fb5ca14b2a375ac7d90585e2d480`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `ec07a16879a1813cf2b5ec7e0033c59799d79e6f`.
- `M .github/workflows/docker.yml` — different; main blob `c1218c1d6c96411f6893094866cc72e6ced488a6`; head blob `bc3bbc39382e2f36c6d1cda9bb08d2f8f35bacd8`.
- `M .github/workflows/python-wheels.yml` — different; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `e31ce23d4f616178c788e35a782b4f845b5fd191`.
- `M .github/workflows/qemu-boot-x86.yml` — different; main blob `bcad444157a39c3d86355298094746016a17e9cc`; head blob `34690e16f1d90dfd1d1512a02446a592465dd110`.
- `M .github/workflows/qemu-boot.yml` — different; main blob `873acefcfe4db9c5f09ceee798862e9a39ce5d68`; head blob `3ddab69d9aa403a6cb96d2e1026475d6b978dca4`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `4930495dc2730ac1f84184cc899c088447286492`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `6c40cb1862517524bf81c52207ff70aefb7203b8`.
- `M docs/explanation/ARCHITECTURE.md` — different; main blob `98d2ea3a56c36d7e0063818575c1e61bb1acc632`; head blob `c36ed9de36ee15c71755396715c08a3d3de1664a`.
- `A docs/records/audits/2026-10-architecture-targets/README.md` — absent_main; main blob `None`; head blob `0488b9f17118c4d7387a79e3c936402a62ab5da2`.
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` — absent_main; main blob `None`; head blob `8f99d63a5bfc203d302335e8794a622bb4212b47`.
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` — absent_main; main blob `None`; head blob `04ed4828d5bf5f41bdb6777a0792ba1c3a2760b2`.
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` — absent_main; main blob `None`; head blob `662c74dd580c3c57ca5715698166d7e0b0cf7949`.
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` — absent_main; main blob `None`; head blob `4adf51b1d9c5d9d34994ed0d0cf313e91f58220f`.
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` — absent_main; main blob `None`; head blob `99b189edcef423743bb44b98e128a9cb937df411`.
- `A docs/records/audits/2026-10-governance-deadlock/README.md` — absent_main; main blob `None`; head blob `22de009b00cb748565251440f62148376a8b547d`.
- `A docs/records/audits/2026-10-governance-deadlock/after.json` — absent_main; main blob `None`; head blob `ce7f470d5096a693baf064dd90909c94f55b680d`.
- `A docs/records/audits/2026-10-governance-deadlock/before.json` — absent_main; main blob `None`; head blob `f9deb13d715f0a90cb70386450e9e5e0cd40d6cf`.
- `A docs/records/audits/2026-10-governance-deadlock/path-only.json` — absent_main; main blob `None`; head blob `e25a817da6841a6777a9567581a597f0a4496609`.
- `A docs/records/audits/2026-10-governance-deadlock/regressions.log` — absent_main; main blob `None`; head blob `d848087e1ffb18d0b8ac1923c21e763903bd9b62`.
- `A docs/records/audits/2026-10-governance-deadlock/target-regressions.log` — absent_main; main blob `None`; head blob `d3c52bb530b67e4fddb030185eeb8435ec63548f`.
- `A docs/records/audits/2026-10-workflow-supply-chain/README.md` — absent_main; main blob `None`; head blob `abe1d31877fc34d5e99fe3d87049e6ce36576931`.
- `A docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json` — absent_main; main blob `None`; head blob `8e8b29e8ff8e087cee8da9633eee02785f04a8bc`.
- `A docs/records/audits/2026-10-workflow-supply-chain/before.json` — absent_main; main blob `None`; head blob `923172f37830624c5ae0e42cb3e9375bde57ffee`.
- `A docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json` — absent_main; main blob `None`; head blob `1ab55c151405fd8df56d17e043f07a8b742defe5`.
- `A docs/records/audits/2026-10-workflow-supply-chain/inventory.md` — absent_main; main blob `None`; head blob `2b807b26015f433e3da87b9e948d7543e12d6f1c`.
- `A docs/records/audits/2026-10-workflow-supply-chain/protection.json` — absent_main; main blob `None`; head blob `e6a5a86c8985fce13afee1342f85d94359fb95fa`.
- `A docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json` — absent_main; main blob `None`; head blob `2ccda18d08e33e27f314e9fe7dd76d8f6578305b`.
- `M scripts/ci/check_architecture_coherence.py` — different; main blob `0553a7959407e858b1bfd6457e465269de4ecdd7`; head blob `45f25390b0f51de4ea3604a30b076fb3a1288c96`.
- `M scripts/ci/check_architecture_targets.py` — different; main blob `3add84d9153eb32923c7e56d5abc65d094ad9d7f`; head blob `69a53706b9da99374b93810b88664a60b8b66ef2`.
- `A scripts/ci/test_failure_propagation.py` — absent_main; main blob `None`; head blob `ebb35c7710e09bcb895fdcde59633268655a52fa`.
- `M scripts/governance/check_docs_governance_hygiene.py` — different; main blob `c5f3eb430261324d0c9c499d0dc76e024ff51315`; head blob `00890185f537be449b4dd4be3e16fdaedf6cc8fa`.
- `M scripts/governance/check_docs_structure.py` — different; main blob `8637fab6dda14837e1c3a8b7f35d764bae033f86`; head blob `8d18e6d9b05c1ef4849a28c0ba66c28d07e37006`.
- `M scripts/governance/check_rfc_lifecycle_hygiene.py` — different; main blob `f2964481478241799410c9cc90e15192157c4fad`; head blob `4a37d88e9be53e77d779b244411073700e9c5ab8`.
- `M scripts/governance/check_spec_code_alignment_baseline.py` — different; main blob `28cda42078ca6eed1d906d23c41c9109d5d5dfad`; head blob `ef0038637b9f8138f4a90b6dd7344b310a2cc11b`.
- `M scripts/governance/check_status_label_coherence.py` — different; main blob `72834c4e100f5bbc29e6d18684ee5d64c805d8e5`; head blob `a72325e14ef877bb9f5fab29af893833e3dafcfa`.
- `A tests/ci/check_architecture_targets_test.py` — absent_main; main blob `None`; head blob `df1882eb3dbed0713affc323d4a443dfc064b21e`.
- `A tests/ci/fixtures/workflow-supply-chain/permissions.json` — absent_main; main blob `None`; head blob `42a9db506437e8bb4808bafead4860e6cdc2db22`.
- `A tests/ci/fixtures/workflow-supply-chain/pins.json` — absent_main; main blob `None`; head blob `bbe425537b65708a5228c177b88e33cac36381c8`.
- `A tests/ci/governance_deadlock_test.py` — absent_main; main blob `None`; head blob `5ff2e45ea91b25609710f46a97823e5637ac8987`.
- `A tests/ci/governance_layout_paths_test.py` — absent_main; main blob `None`; head blob `ed6700b6d28e1f1eb58e1103831e4f661c280665`.
- `A tests/ci/governance_maturity_metadata_test.py` — absent_main; main blob `None`; head blob `c81fc01e0252ba73c4eced50591286ce16f179c8`.
- `A tests/ci/rfc_status_table_test.py` — absent_main; main blob `None`; head blob `4f02420f1880f7b1ee0dafe972493b21f009a9d7`.
- `A tests/ci/workflow_action_smoke.py` — absent_main; main blob `None`; head blob `773a543762e53894df44fbba958b317c339f75fa`.
- `A tests/ci/workflow_supply_chain_test.py` — absent_main; main blob `None`; head blob `b28fba9fe46b5862a21bbd32dadc674f3d50f098`.
- `M tests/cpp/tisc_opcode_matrix_test.cpp` — different; main blob `b3c38649c4919c8a044ed6221a6c9e4a4da818e1`; head blob `c8b9e231148b15894c3b659c9d47b60ed3cf05ac`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`, `CMakeLists.txt`
- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`
- #747: `.github/workflows/ci.yml`, `.github/workflows/python-wheels.yml`, `CMakeLists.txt`
- #707: `tests/cpp/tisc_opcode_matrix_test.cpp`
- #603: `CMakeLists.txt`
- #577: `.github/workflows/ci.yml`, `.github/workflows/docker.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`, `docs/records/audits/2026-10-workflow-supply-chain/README.md`, `docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json`, `docs/records/audits/2026-10-workflow-supply-chain/before.json`, `docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json`, `docs/records/audits/2026-10-workflow-supply-chain/inventory.md`, `docs/records/audits/2026-10-workflow-supply-chain/protection.json`, `docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json`, `tests/ci/fixtures/workflow-supply-chain/permissions.json`, `tests/ci/fixtures/workflow-supply-chain/pins.json`, `tests/ci/workflow_action_smoke.py`, `tests/ci/workflow_supply_chain_test.py`
- #576: `.github/workflows/ci.yml`, `README.md`, `docs/records/audits/2026-10-governance-deadlock/README.md`, `docs/records/audits/2026-10-governance-deadlock/after.json`, `docs/records/audits/2026-10-governance-deadlock/before.json`, `docs/records/audits/2026-10-governance-deadlock/path-only.json`, `docs/records/audits/2026-10-governance-deadlock/regressions.log`, `docs/records/audits/2026-10-governance-deadlock/target-regressions.log`, `scripts/ci/check_architecture_coherence.py`, `tests/ci/governance_deadlock_test.py`
- #575: `.github/workflows/ci.yml`, `docs/explanation/ARCHITECTURE.md`, `docs/records/audits/2026-10-architecture-targets/README.md`, `docs/records/audits/2026-10-architecture-targets/evidence.tar.gz`, `docs/records/audits/2026-10-architecture-targets/inventory.json`, `scripts/ci/check_architecture_targets.py`, `tests/ci/check_architecture_targets_test.py`
- #574: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`, `docs/records/audits/2026-10-ci-failure-propagation/README.md`, `docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz`, `docs/records/audits/2026-10-ci-failure-propagation/results.json`, `scripts/ci/test_failure_propagation.py`
- #556: `CMakeLists.txt`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #525: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`, `.github/workflows/docker.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`
- #514: `.github/workflows/ci.yml`
- #513: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`
- #508: `CMakeLists.txt`, `README.md`
- #507: `README.md`, `scripts/governance/check_docs_structure.py`
- #504: `.github/workflows/ci.yml`, `README.md`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/41022555473/job/123953358394): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/41022555473/job/123953004065): completed / failure.

## PR 648: Governance Authority Reconciliation Report

- Head `34682b77dcb72abf468683b005df6c4ad5db6104`; actual base `f775680aba7db200691c0f4e2a3174b0e7e51581` (`main`); main merge-base `f775680aba7db200691c0f4e2a3174b0e7e51581`.
- Purpose: Governance authority decision matrix. Decision: **REPAIR**.
- Dependency assessment: Input to 649/650 and root-layout decision stack; no governance activation.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Report now lives under docs/governance, fixing the old location concern. Replay instruction still globs scripts/governance and scripts/ci while claiming 27 validators; replace with explicit intended commands.
- Blob counts: {'absent_main': 1}. Merge simulation exit 0; tree `f18723e7618abde0ab206cccd0cc7b4a0ffa4887`; equals main: False.
- Review: 1 COMMENTED; threads 2 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...34682b77dcb72abf468683b005df6c4ad5db6104).

Own changed files:

- `A docs/governance/GOVERNANCE_AUTHORITY_DECISION_MATRIX.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/34682b77dcb72abf468683b005df6c4ad5db6104/docs/governance/GOVERNANCE_AUTHORITY_DECISION_MATRIX.md))

All changed files versus main merge-base (including parents):

- `A docs/governance/GOVERNANCE_AUTHORITY_DECISION_MATRIX.md` — absent_main; main blob `None`; head blob `b95d17b6e9dfba57ec112202cf9497bc1b6def5b`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `2a890acd2ffcf705b6e687f3b06e09bf8c4889ba` / [review](https://github.com/t81dev/t81-foundation/pull/648#pullrequestreview-5704109026).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123730547329): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123719510420): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123719510336): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123719510298): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123719510244): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123719510182): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123719510164): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123719510153): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123718387208): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123718387134): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123718387108): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/40950736394/job/123718386998): completed / failure.

## PR 645: docs(research): review H04-R03 matched codec specification

- Head `6d20d381322e4859e33524f4fc18d4c6569539ce`; actual base `872ac1b4c8a303fc5417d0b21551a430d0a50200` (`main`); main merge-base `872ac1b4c8a303fc5417d0b21551a430d0a50200`.
- Purpose: Matched-codec specification correction. Decision: **SUPERSEDED**.
- Dependency assessment: H04 implementation/specification evolved on main; no new protocol content to integrate.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Current main specification has identical content after ignoring trailing spaces; branch adds only Markdown hard-break whitespace. Contract substance is already represented (later implementation commit d8e1376a7). Preserve historical review reference.
- Blob counts: {'different': 1}. Merge simulation exit 1; tree `7249fc03a93b6659e60857dc5621abe1e3c48495`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...6d20d381322e4859e33524f4fc18d4c6569539ce).

Own changed files:

- `A experiments/h04_r03_matched_codec/SPECIFICATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/6d20d381322e4859e33524f4fc18d4c6569539ce/experiments/h04_r03_matched_codec/SPECIFICATION.md))

All changed files versus main merge-base (including parents):

- `A experiments/h04_r03_matched_codec/SPECIFICATION.md` — different; main blob `e8733baa3f8c8fc11251f37601b1c2319e8983b0`; head blob `d0f1617d2e480dcc36df4b186fd16cfd909e5fb4`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123410675346): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123385244056): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123385244050): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123385244007): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123385244005): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123385243991): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123385243901): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123385243866): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123375528481): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123375528448): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123375528438): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/40848543568/job/123375528408): completed / failure.

## PR 640: docs(ai-context): reconcile H04 correction and post-merge research handoff

- Head `e0dc9649db3145e60294a2591589c321ae5d4e38`; actual base `6864a810693a6034bd28be870ee38a180ba70ba0` (`main`); main merge-base `6864a810693a6034bd28be870ee38a180ba70ba0`.
- Purpose: H04/AI-context continuity refresh. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Overlaps 657/764; main 781 supplies newer operational handoff, not automatic erasure of old evidence.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [570, 571, 572, 637].
- Evidence/limits: Six shared current handoff documents conflict with newer main. Extract any unique dated evidence rather than replacing current status; no source proof yet that whole PR is redundant.
- Blob counts: {'different': 6}. Merge simulation exit 1; tree `0566d9f9f881f1c68ed5ad9d6fd051c8deb25de3`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...e0dc9649db3145e60294a2591589c321ae5d4e38).

Own changed files:

- `M docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e0dc9649db3145e60294a2591589c321ae5d4e38/docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md))
- `M docs/ai-context/BOOTSTRAP.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e0dc9649db3145e60294a2591589c321ae5d4e38/docs/ai-context/BOOTSTRAP.md))
- `M docs/ai-context/DECISIONS_AND_GOVERNANCE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e0dc9649db3145e60294a2591589c321ae5d4e38/docs/ai-context/DECISIONS_AND_GOVERNANCE.md))
- `M docs/ai-context/EVIDENCE_AND_FINDINGS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e0dc9649db3145e60294a2591589c321ae5d4e38/docs/ai-context/EVIDENCE_AND_FINDINGS.md))
- `M docs/ai-context/RESEARCH_IDENTITY.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e0dc9649db3145e60294a2591589c321ae5d4e38/docs/ai-context/RESEARCH_IDENTITY.md))
- `M docs/ai-context/START_HERE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e0dc9649db3145e60294a2591589c321ae5d4e38/docs/ai-context/START_HERE.md))

All changed files versus main merge-base (including parents):

- `M docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md` — different; main blob `526a9750ef88f1a7c445ae3adfd115f46bdf87a8`; head blob `7a33b38e766116382c3b20a12c29afb21881afc0`.
- `M docs/ai-context/BOOTSTRAP.md` — different; main blob `8da6d42fa87744c6e08a76c03cdf75eb7d5cf8b6`; head blob `2ac28ed735464bfb57d97e15ce4df0332df734d7`.
- `M docs/ai-context/DECISIONS_AND_GOVERNANCE.md` — different; main blob `f23190dcf40d159bcfd4ce69e41dd3d3abe14bc7`; head blob `eee6ed4250b926b85dcb1ab712e52b9c0de0a1d8`.
- `M docs/ai-context/EVIDENCE_AND_FINDINGS.md` — different; main blob `9d08156c7f4e0401fa2e03aa05d2877d991d7f04`; head blob `80df31b370b7222b17175bedd31abc124399d452`.
- `M docs/ai-context/RESEARCH_IDENTITY.md` — different; main blob `34a9d2b105023e6f23ae37937f470b922979446e`; head blob `d6b35f9b2fada48bd19281ae4b518e6bdb4a21c1`.
- `M docs/ai-context/START_HERE.md` — different; main blob `1f790fff2355f61f7639fa65486d486ff9b6d5c8`; head blob `c3ebc7160fcf4b7e4c549a62a46a3495ea53cfab`.

Overlaps with other PR own deltas:

- #764: `docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`, `docs/ai-context/START_HERE.md`
- #657: `docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md`, `docs/ai-context/BOOTSTRAP.md`, `docs/ai-context/DECISIONS_AND_GOVERNANCE.md`, `docs/ai-context/EVIDENCE_AND_FINDINGS.md`, `docs/ai-context/RESEARCH_IDENTITY.md`, `docs/ai-context/START_HERE.md`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123374861037): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123351789369): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123351789363): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123351789360): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123351789338): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123351789321): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123351789276): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123351789262): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123341888220): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123341888184): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123341888138): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/40840516798/job/123341888042): completed / failure.

## PR 634: Audit and Correct EXP-T81-COMP3-01 Pre-Execution Protocol

- Head `21095d65cc0c849f5a6f2599a962cbce032b5cea`; actual base `f8ac77159ded2c4b93f4cc83abd48f6f35655468` (`main`); main merge-base `f8ac77159ded2c4b93f4cc83abd48f6f35655468`.
- Purpose: COMP3 preregistration audit/correction. Decision: **REPAIR**.
- Dependency assessment: Same path as alternative 633; original 632 closed without merge.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [632].
- Evidence/limits: Current four reported P1s require reconciliation: overlapping outcomes, interval semantics, blocked-physical-evidence claims, and missing Commons proposal. Source still has scientific acceptance predicates needing review; no execution.
- Blob counts: {'different': 1}. Merge simulation exit 1; tree `c94c77e13b00cf371316de6b275a17acb3636b12`; equals main: False.
- Review: 1 COMMENTED; threads 4 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...21095d65cc0c849f5a6f2599a962cbce032b5cea).

Own changed files:

- `A docs/research/TERNARY_ADVANTAGE_CANDIDATES.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/21095d65cc0c849f5a6f2599a962cbce032b5cea/docs/research/TERNARY_ADVANTAGE_CANDIDATES.md))

All changed files versus main merge-base (including parents):

- `A docs/research/TERNARY_ADVANTAGE_CANDIDATES.md` — different; main blob `778ece6d09ea427ca0b435d951adc0ff445bbb1a`; head blob `a10d217518a47f9a373ba3917aa1455ca30d63ce`.

Overlaps with other PR own deltas:

- #633: `docs/research/TERNARY_ADVANTAGE_CANDIDATES.md`

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `cde3bef108a179f106c9eccd752c710f8e31185d` / [review](https://github.com/t81dev/t81-foundation/pull/634#pullrequestreview-5691943703).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123267136474): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123254046104): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123254046082): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123254046049): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123254046019): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123254046014): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123254045982): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123254045935): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123251267007): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123251266949): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123251266926): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/40814019469/job/123251266757): completed / failure.

## PR 633: Correct EXP-T81-COMP3-01 Preregistration Protocol

- Head `994736a7b553f01e27b430f597da41b87d34aada`; actual base `f8ac77159ded2c4b93f4cc83abd48f6f35655468` (`main`); main merge-base `f8ac77159ded2c4b93f4cc83abd48f6f35655468`.
- Purpose: Alternative COMP3 protocol correction. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Original 632 closed without merge; conflicts with current main protocol path.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [632].
- Evidence/limits: Same protocol path as 634; substantive source variants, not proven duplicate or accepted successor. Choose one after applying current methodological findings.
- Blob counts: {'different': 1}. Merge simulation exit 1; tree `307ca3f274bb89628d13e8f0eb507e6e35190345`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...994736a7b553f01e27b430f597da41b87d34aada).

Own changed files:

- `A docs/research/TERNARY_ADVANTAGE_CANDIDATES.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/994736a7b553f01e27b430f597da41b87d34aada/docs/research/TERNARY_ADVANTAGE_CANDIDATES.md))

All changed files versus main merge-base (including parents):

- `A docs/research/TERNARY_ADVANTAGE_CANDIDATES.md` — different; main blob `778ece6d09ea427ca0b435d951adc0ff445bbb1a`; head blob `3a147d2ee50c172d495df06bc666c54ddac9c0dd`.

Overlaps with other PR own deltas:

- #634: `docs/research/TERNARY_ADVANTAGE_CANDIDATES.md`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123153815999): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123141707337): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123141707139): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123141707130): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123141707050): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123141706996): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123141706983): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123141706934): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123139859187): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123139859140): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123139859100): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/40780139966/job/123139859086): completed / failure.

## PR 628: research(i08): prepare matched measurement apparatus for independent review

- Head `a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca`; actual base `0662e0e637f47168e56ca4ddd66e8ee877473b38` (`main`); main merge-base `0662e0e637f47168e56ca4ddd66e8ee877473b38`.
- Purpose: Matched Investigation-08 apparatus and exploratory diagnostics. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Related 603 profiling and closed-unmerged 602 comparator; do not assume dependency merged.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: 59 unique paths retain raw exploratory packets; confirm matched semantics, baseline provenance and retention before interpreting results. Comments document diagnostics, not confirmatory authorization.
- Blob counts: {'absent_main': 59}. Merge simulation exit 0; tree `a414a7d8bfd1a3a4309673c1f52d64efa3fdc981`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 11 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca).

Own changed files:

- `A experiments/integrated_system_advantage/controlled_measurement/PREREGISTRATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/PREREGISTRATION.md))
- `A experiments/integrated_system_advantage/controlled_measurement/READINESS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/READINESS.md))
- `A experiments/integrated_system_advantage/controlled_measurement/analysis.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/analysis.py))
- `A experiments/integrated_system_advantage/controlled_measurement/apparatus.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/apparatus.py))
- `A experiments/integrated_system_advantage/controlled_measurement/baseline-audit.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/baseline-audit.json))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/.gitignore` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/.gitignore))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/AUTHORIZATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/AUTHORIZATION.md))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/EXECUTION_TRACE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/EXECUTION_TRACE.md))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/REPORT.md))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/check_instrumentation.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/check_instrumentation.py))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/collect.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/collect.py))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/collection.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/collection.log))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/manifest.json))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/observations.json))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/per-command-timings.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/per-command-timings.jsonl))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/prior-packet-verification.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/prior-packet-verification.log))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/raw-run.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/raw-run.tar.gz))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/recorder-checks.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/recorder-checks.log))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/verification.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/verification.log))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/verify.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/verify.py))
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/worker.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/worker.py))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/.gitignore` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/.gitignore))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/AUTHORIZATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/AUTHORIZATION.md))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/REPORT.md))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/collect.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/collect.py))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/collection.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/collection.log))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/manifest.json))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/observations.json))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/raw-profile-run.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/raw-profile-run.tar.gz))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/sample-help-metadata.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/sample-help-metadata.json))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/sample-help-stderr.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/sample-help-stderr.txt))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/sample-help-stdout.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/sample-help-stdout.txt))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/summary.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/summary.json))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/supplemental-environment.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/supplemental-environment.json))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/verification.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/verification.log))
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/verify.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/verify.py))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/.gitignore` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/.gitignore))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/AUTHORIZATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/AUTHORIZATION.md))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/REPORT.md))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/artifact-identities.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/artifact-identities.json))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/collect.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/collect.py))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/collection.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/collection.log))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/gate-scenarios.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/gate-scenarios.json))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/gate.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/gate.log))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/host-identity.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/host-identity.json))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/manifest.json))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/observations.json))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/raw-run.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/raw-run.tar.gz))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/recovery-validation.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/recovery-validation.log))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/verification.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/verification.log))
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/verify.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/verify.py))
- `A experiments/integrated_system_advantage/controlled_measurement/proposal.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/proposal.json))
- `A experiments/integrated_system_advantage/controlled_measurement/schedule.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/schedule.json))
- `A experiments/integrated_system_advantage/controlled_measurement/schedule.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/schedule.py))
- `A experiments/integrated_system_advantage/controlled_measurement/test_apparatus.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/test_apparatus.py))
- `A experiments/integrated_system_advantage/controlled_measurement/validation/attempts.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/validation/attempts.jsonl))
- `A experiments/integrated_system_advantage/controlled_measurement/validation/journal-manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/validation/journal-manifest.json))
- `A experiments/integrated_system_advantage/controlled_measurement/validation/report.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/validation/report.json))
- `A experiments/integrated_system_advantage/controlled_measurement/validation/tests.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/a9ae2af6a8abce21f2aeefb0b53fb0fe5906d2ca/experiments/integrated_system_advantage/controlled_measurement/validation/tests.log))

All changed files versus main merge-base (including parents):

- `A experiments/integrated_system_advantage/controlled_measurement/PREREGISTRATION.md` — absent_main; main blob `None`; head blob `cc6c36e8a851aac095733ff94bf32b7a062af6ac`.
- `A experiments/integrated_system_advantage/controlled_measurement/READINESS.md` — absent_main; main blob `None`; head blob `27fa0f5ae5959bd57d24a8dde7a3520488613aa7`.
- `A experiments/integrated_system_advantage/controlled_measurement/analysis.py` — absent_main; main blob `None`; head blob `7063290b43ed432803573d469c52345ce2a4cb26`.
- `A experiments/integrated_system_advantage/controlled_measurement/apparatus.py` — absent_main; main blob `None`; head blob `4d47e25f1c9a9d8ba629b2d586b2be393de58b08`.
- `A experiments/integrated_system_advantage/controlled_measurement/baseline-audit.json` — absent_main; main blob `None`; head blob `576e413009126375f7d450f0148b1344c49e5480`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/.gitignore` — absent_main; main blob `None`; head blob `b7ca9cfc5631d2e658dd0a866597b7523f7deb50`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/AUTHORIZATION.md` — absent_main; main blob `None`; head blob `7dd1a3f725e2ef8dcaaf3731d031ac28ca1395ed`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/EXECUTION_TRACE.md` — absent_main; main blob `None`; head blob `ac968fdd983a0edc91faf57e1d5ff4aba582a41d`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/REPORT.md` — absent_main; main blob `None`; head blob `ad6166e1a65a6d71c675257cc5954808e54dbec1`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/check_instrumentation.py` — absent_main; main blob `None`; head blob `e7ce7006b5ba36e6d91a57439854a64ab235c4db`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/collect.py` — absent_main; main blob `None`; head blob `46670ce1b2535c0a429e0468a5b3044dbc476003`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/collection.log` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/manifest.json` — absent_main; main blob `None`; head blob `9716179141e5e4291e5f0ee888ded5af64c9c077`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/observations.json` — absent_main; main blob `None`; head blob `a61d92273b0c523438289e08cdddffbad5637713`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/per-command-timings.jsonl` — absent_main; main blob `None`; head blob `988eff4caf0f5900bfacdf7fd804cc6271b8806f`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/prior-packet-verification.log` — absent_main; main blob `None`; head blob `778e7e15b4b85ca8c0e5816526c087f31eed35fe`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/raw-run.tar.gz` — absent_main; main blob `None`; head blob `3996148abcfcc844aed268cf7c8c1f06a2f50a1c`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/recorder-checks.log` — absent_main; main blob `None`; head blob `68eae429ed389776f67dd54e70dc458347753562`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/verification.log` — absent_main; main blob `None`; head blob `f2b9b3c9c80c48133068028ccdc0850070b565f0`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/verify.py` — absent_main; main blob `None`; head blob `f650458733a06e1210626bf243ec15064afba8bb`.
- `A experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/worker.py` — absent_main; main blob `None`; head blob `673d2f7ba7282a731e075327a60b5c7828f8a094`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/.gitignore` — absent_main; main blob `None`; head blob `0ca921d3a15cb1c549e61223f445c119a1c5c369`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/AUTHORIZATION.md` — absent_main; main blob `None`; head blob `99d4d3edeef0b7048ec2f50267960da7c2816656`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/REPORT.md` — absent_main; main blob `None`; head blob `14ae01e835d38cf6a6ae5cb663a6161e7cc17927`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/collect.py` — absent_main; main blob `None`; head blob `c35fb74959b004702809cf63d0ffb6023e2d7b75`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/collection.log` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/manifest.json` — absent_main; main blob `None`; head blob `780a91f0a74e4e1de8fc34be3ef88fa48576af8a`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/observations.json` — absent_main; main blob `None`; head blob `6b3dff9022a9c6579b6cd7c70664ad23ede13026`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/raw-profile-run.tar.gz` — absent_main; main blob `None`; head blob `e090edc2daea4d7ad0554edce92206e7b1f00ef9`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/sample-help-metadata.json` — absent_main; main blob `None`; head blob `2b0f076d762cecd716004e12fcbf799bc891884b`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/sample-help-stderr.txt` — absent_main; main blob `None`; head blob `6a8ef57a9dc7622e1fc6fe77a9ee73ca307cc278`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/sample-help-stdout.txt` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/summary.json` — absent_main; main blob `None`; head blob `72a8ab515265f74c9e31837f6da37ea962dc90b5`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/supplemental-environment.json` — absent_main; main blob `None`; head blob `11c431541ffea78de56c30f5a7aedb6029c6c669`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/verification.log` — absent_main; main blob `None`; head blob `66b1687900f7d611614fc6b48b602ae911f0e7d0`.
- `A experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/verify.py` — absent_main; main blob `None`; head blob `a75a1111860b13677975fb90788fcff76c122974`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/.gitignore` — absent_main; main blob `None`; head blob `c52e55e97391c8e5be588c00095756a553297121`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/AUTHORIZATION.md` — absent_main; main blob `None`; head blob `28ec9579f829d5c3bee0fec687cb56597ab81fee`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/REPORT.md` — absent_main; main blob `None`; head blob `511607cb37af2f2be34a74ee5b7393ce4c3f02be`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/artifact-identities.json` — absent_main; main blob `None`; head blob `8b490468349fe1a7891411ab3ef162604e89ce98`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/collect.py` — absent_main; main blob `None`; head blob `9ca87d8b3dab3eeab763064e26451c11267e1d21`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/collection.log` — absent_main; main blob `None`; head blob `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/gate-scenarios.json` — absent_main; main blob `None`; head blob `bc3255aef99940a9a044978dd9c9d50736a8f8ee`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/gate.log` — absent_main; main blob `None`; head blob `c0b956b13c596ba665d13a070264b96f5ef7a1e6`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/host-identity.json` — absent_main; main blob `None`; head blob `d5902b0902b101ce905efc9f798b923e0a3a057c`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/manifest.json` — absent_main; main blob `None`; head blob `640a6b2b91c25fcf30bc51b072c5a36e9e47b70f`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/observations.json` — absent_main; main blob `None`; head blob `47d539715f4b546e348c59db36d2d219bfe0e97e`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/raw-run.tar.gz` — absent_main; main blob `None`; head blob `cf898099a76d0dfe6ad98ce55e4b998668773331`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/recovery-validation.log` — absent_main; main blob `None`; head blob `bb832e38cf9f2e3433c9a24da84e73aac5593064`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/verification.log` — absent_main; main blob `None`; head blob `778e7e15b4b85ca8c0e5816526c087f31eed35fe`.
- `A experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/verify.py` — absent_main; main blob `None`; head blob `335c66e6b587043c42d227a54b2b69f0edb01f11`.
- `A experiments/integrated_system_advantage/controlled_measurement/proposal.json` — absent_main; main blob `None`; head blob `0ea9ad417d645fbcd1b1e434a3dab7bd864b654a`.
- `A experiments/integrated_system_advantage/controlled_measurement/schedule.json` — absent_main; main blob `None`; head blob `534d1ecba4f9cc211d47e7183080417de1bdd85f`.
- `A experiments/integrated_system_advantage/controlled_measurement/schedule.py` — absent_main; main blob `None`; head blob `7c15d47c1d9e45f339c0f385971157d282e0646d`.
- `A experiments/integrated_system_advantage/controlled_measurement/test_apparatus.py` — absent_main; main blob `None`; head blob `7a5881affe15c6411f544d3e51c9219d93dacf47`.
- `A experiments/integrated_system_advantage/controlled_measurement/validation/attempts.jsonl` — absent_main; main blob `None`; head blob `730dba276590c615ac54fa6e820ef1f9f6f15d0b`.
- `A experiments/integrated_system_advantage/controlled_measurement/validation/journal-manifest.json` — absent_main; main blob `None`; head blob `f7b77367231b8871c5740735f9d7bf1cd76df187`.
- `A experiments/integrated_system_advantage/controlled_measurement/validation/report.json` — absent_main; main blob `None`; head blob `fc1cb4689e296e1d50d9c0218aec80934e4b5195`.
- `A experiments/integrated_system_advantage/controlled_measurement/validation/tests.log` — absent_main; main blob `None`; head blob `85f977a8c6bfa7b2f04918178ae396671a6498a9`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123060495560): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123037862029): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123037861964): completed / cancelled.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123037861897): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123037861877): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123037861844): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123037861827): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123037861824): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123035393735): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123035393683): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123035393677): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/40750264712/job/123035393673): completed / failure.

## PR 617: audit: reconcile #574–#577 governance integration acceptance (no activation)

- Head `7afd3246ea587d4b4d350e807b5fd1d2334d643e`; actual base `b6a20706b3b37eebbfb5ab0293a88306a3dae29a` (`corrective/workflow-supply-chain`); main merge-base `01c85019319fd020922402ba356878e392cb2231`.
- Purpose: Governance integration replay/acceptance audit. Decision: **DEFER**.
- Dependency assessment: Contains open heads 574-577; base 577; related consolidated alternative 649.
- Open heads included by ancestry: [574, 575, 576, 577]; references in body (not necessarily dependencies): [576, 577].
- Evidence/limits: Three own paths atop governance stack; replay of old stack is not acceptance of current main or authority activation. Choose canonical corrections first.
- Blob counts: {'different': 10, 'absent_main': 29}. Merge simulation exit 1; tree `c24e73cbdb4a2be83de0ad7f219642446bb561d8`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...7afd3246ea587d4b4d350e807b5fd1d2334d643e).

Own changed files:

- `A .github/workflows/governance-integration-replay.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7afd3246ea587d4b4d350e807b5fd1d2334d643e/.github/workflows/governance-integration-replay.yml))
- `A docs/records/audits/2026-10-governance-integration/ACCEPTANCE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7afd3246ea587d4b4d350e807b5fd1d2334d643e/docs/records/audits/2026-10-governance-integration/ACCEPTANCE.md))
- `A scripts/ci/replay_governance_integration.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7afd3246ea587d4b4d350e807b5fd1d2334d643e/scripts/ci/replay_governance_integration.py))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `534c80263b70fb5ca14b2a375ac7d90585e2d480`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `ec07a16879a1813cf2b5ec7e0033c59799d79e6f`.
- `M .github/workflows/docker.yml` — different; main blob `c1218c1d6c96411f6893094866cc72e6ced488a6`; head blob `bc3bbc39382e2f36c6d1cda9bb08d2f8f35bacd8`.
- `A .github/workflows/governance-integration-replay.yml` — absent_main; main blob `None`; head blob `ef9d5869f5f2d5c53d5b2a48c319fb5c2046fd13`.
- `M .github/workflows/python-wheels.yml` — different; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `e31ce23d4f616178c788e35a782b4f845b5fd191`.
- `M .github/workflows/qemu-boot-x86.yml` — different; main blob `bcad444157a39c3d86355298094746016a17e9cc`; head blob `34690e16f1d90dfd1d1512a02446a592465dd110`.
- `M .github/workflows/qemu-boot.yml` — different; main blob `873acefcfe4db9c5f09ceee798862e9a39ce5d68`; head blob `3ddab69d9aa403a6cb96d2e1026475d6b978dca4`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `a49327d6eaf5ac128d03013935db95ec9558d49f`.
- `M docs/explanation/ARCHITECTURE.md` — different; main blob `98d2ea3a56c36d7e0063818575c1e61bb1acc632`; head blob `c36ed9de36ee15c71755396715c08a3d3de1664a`.
- `A docs/records/audits/2026-10-architecture-targets/README.md` — absent_main; main blob `None`; head blob `0488b9f17118c4d7387a79e3c936402a62ab5da2`.
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` — absent_main; main blob `None`; head blob `8f99d63a5bfc203d302335e8794a622bb4212b47`.
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` — absent_main; main blob `None`; head blob `04ed4828d5bf5f41bdb6777a0792ba1c3a2760b2`.
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` — absent_main; main blob `None`; head blob `662c74dd580c3c57ca5715698166d7e0b0cf7949`.
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` — absent_main; main blob `None`; head blob `4adf51b1d9c5d9d34994ed0d0cf313e91f58220f`.
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` — absent_main; main blob `None`; head blob `99b189edcef423743bb44b98e128a9cb937df411`.
- `A docs/records/audits/2026-10-governance-deadlock/README.md` — absent_main; main blob `None`; head blob `22de009b00cb748565251440f62148376a8b547d`.
- `A docs/records/audits/2026-10-governance-deadlock/after.json` — absent_main; main blob `None`; head blob `ce7f470d5096a693baf064dd90909c94f55b680d`.
- `A docs/records/audits/2026-10-governance-deadlock/before.json` — absent_main; main blob `None`; head blob `f9deb13d715f0a90cb70386450e9e5e0cd40d6cf`.
- `A docs/records/audits/2026-10-governance-deadlock/path-only.json` — absent_main; main blob `None`; head blob `e25a817da6841a6777a9567581a597f0a4496609`.
- `A docs/records/audits/2026-10-governance-deadlock/regressions.log` — absent_main; main blob `None`; head blob `d848087e1ffb18d0b8ac1923c21e763903bd9b62`.
- `A docs/records/audits/2026-10-governance-deadlock/target-regressions.log` — absent_main; main blob `None`; head blob `d3c52bb530b67e4fddb030185eeb8435ec63548f`.
- `A docs/records/audits/2026-10-governance-integration/ACCEPTANCE.md` — absent_main; main blob `None`; head blob `842403daa228ea008fefa8c39c107f7a367965ad`.
- `A docs/records/audits/2026-10-workflow-supply-chain/README.md` — absent_main; main blob `None`; head blob `abe1d31877fc34d5e99fe3d87049e6ce36576931`.
- `A docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json` — absent_main; main blob `None`; head blob `8e8b29e8ff8e087cee8da9633eee02785f04a8bc`.
- `A docs/records/audits/2026-10-workflow-supply-chain/before.json` — absent_main; main blob `None`; head blob `923172f37830624c5ae0e42cb3e9375bde57ffee`.
- `A docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json` — absent_main; main blob `None`; head blob `1ab55c151405fd8df56d17e043f07a8b742defe5`.
- `A docs/records/audits/2026-10-workflow-supply-chain/inventory.md` — absent_main; main blob `None`; head blob `2b807b26015f433e3da87b9e948d7543e12d6f1c`.
- `A docs/records/audits/2026-10-workflow-supply-chain/protection.json` — absent_main; main blob `None`; head blob `e6a5a86c8985fce13afee1342f85d94359fb95fa`.
- `A docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json` — absent_main; main blob `None`; head blob `2ccda18d08e33e27f314e9fe7dd76d8f6578305b`.
- `M scripts/ci/check_architecture_coherence.py` — different; main blob `0553a7959407e858b1bfd6457e465269de4ecdd7`; head blob `45f25390b0f51de4ea3604a30b076fb3a1288c96`.
- `M scripts/ci/check_architecture_targets.py` — different; main blob `3add84d9153eb32923c7e56d5abc65d094ad9d7f`; head blob `69a53706b9da99374b93810b88664a60b8b66ef2`.
- `A scripts/ci/replay_governance_integration.py` — absent_main; main blob `None`; head blob `b21a1aba889cf1a64db500e90f99405759f270ba`.
- `A scripts/ci/test_failure_propagation.py` — absent_main; main blob `None`; head blob `ebb35c7710e09bcb895fdcde59633268655a52fa`.
- `A tests/ci/check_architecture_targets_test.py` — absent_main; main blob `None`; head blob `df1882eb3dbed0713affc323d4a443dfc064b21e`.
- `A tests/ci/fixtures/workflow-supply-chain/permissions.json` — absent_main; main blob `None`; head blob `8ae3c7086a51c053f2a27ed57abbc29c79e7842d`.
- `A tests/ci/fixtures/workflow-supply-chain/pins.json` — absent_main; main blob `None`; head blob `bbe425537b65708a5228c177b88e33cac36381c8`.
- `A tests/ci/governance_deadlock_test.py` — absent_main; main blob `None`; head blob `11b7a967ddc33eabcacdab80172e2343876442df`.
- `A tests/ci/workflow_action_smoke.py` — absent_main; main blob `None`; head blob `773a543762e53894df44fbba958b317c339f75fa`.
- `A tests/ci/workflow_supply_chain_test.py` — absent_main; main blob `None`; head blob `28a407730513205659115108bd7b3453a7d341e3`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 616: fix(commons): restore PR607 source fidelity and qualify audit receipts

- Head `20ef563ea96a7cf84dad612067355e2dd4cf4d8b`; actual base `f3288b0ff55727282c7395ed14f4598add096706` (`main`); main merge-base `f3288b0ff55727282c7395ed14f4598add096706`.
- Purpose: Commons source-fidelity and receipt qualification. Decision: **REVIEW NEXT**.
- Dependency assessment: Follow-up to merged 615; overlaps 747 only in Commons workflow.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [615].
- Evidence/limits: Six paths correct historical source pin versus separate correction, and qualify reported replay. Verify exact content digests and conflict resolution while preserving audit appendices.
- Blob counts: {'different': 5, 'absent_main': 1}. Merge simulation exit 1; tree `c34db9c66d9f77d23b40b2225c393e2dc1d6fda0`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 28 pass, 12 fail, 8 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...20ef563ea96a7cf84dad612067355e2dd4cf4d8b).

Own changed files:

- `M .github/workflows/research-commons.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/20ef563ea96a7cf84dad612067355e2dd4cf4d8b/.github/workflows/research-commons.yml))
- `M docs/research/commons/PHASE1_ACCEPTANCE_AUDIT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/20ef563ea96a7cf84dad612067355e2dd4cf4d8b/docs/research/commons/PHASE1_ACCEPTANCE_AUDIT.md))
- `M docs/research/commons/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/20ef563ea96a7cf84dad612067355e2dd4cf4d8b/docs/research/commons/README.md))
- `M docs/research/commons/manifest.v1.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/20ef563ea96a7cf84dad612067355e2dd4cf4d8b/docs/research/commons/manifest.v1.json))
- `M tests/research_commons/test_commons.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/20ef563ea96a7cf84dad612067355e2dd4cf4d8b/tests/research_commons/test_commons.py))
- `A tools/research_commons/regenerate_demo_receipt.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/20ef563ea96a7cf84dad612067355e2dd4cf4d8b/tools/research_commons/regenerate_demo_receipt.py))

All changed files versus main merge-base (including parents):

- `M .github/workflows/research-commons.yml` — different; main blob `0891289437125428139991616ccc8f261aab2687`; head blob `0daad80770377b1129f3cd6cc7c6378ef4907cfd`.
- `M docs/research/commons/PHASE1_ACCEPTANCE_AUDIT.md` — different; main blob `75655e8490d6dc3b64384b308f3e93b750f66a15`; head blob `0cf54c680277be3455d5b93c7a317493b6bdf9a9`.
- `M docs/research/commons/README.md` — different; main blob `9a0b0486aac7fffd32d0004133916a3bec001d0d`; head blob `c60820f929185d054bba5c497534fa595280359a`.
- `M docs/research/commons/manifest.v1.json` — different; main blob `adf59a4751f0754a61f574848c5b7e44be72c927`; head blob `8358b11226a37be1656fea1597d66ed68f16943e`.
- `M tests/research_commons/test_commons.py` — different; main blob `e7a6ddda76b51b4c7cca4b0a31c33332b42b3753`; head blob `64e5292cb96463738c9162057bdd168f4bbc359b`.
- `A tools/research_commons/regenerate_demo_receipt.py` — absent_main; main blob `None`; head blob `2752803291626c56c25afd26734fcb101924ec09`.

Overlaps with other PR own deltas:

- #747: `.github/workflows/research-commons.yml`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122560835879): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122532456710): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122532456390): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122532456331): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122532456059): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122532455796): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122532455722): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122532455555): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122463312972): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122463312896): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122463312853): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/40572960148/job/122463312737): completed / failure.

## PR 612: audit: map agent skills to operational handlers and retain boundary failures

- Head `215fb5cba7d452f1c94a714446235619e92e7abe`; actual base `903ea2fe653df0d636e0543f9788dd8442738045` (`main`); main merge-base `fabc7aa90f70ead227ec257a1ae6833fa16ced40`.
- Purpose: Agent skill/handler compatibility evidence audit. Decision: **REVIEW NEXT**.
- Dependency assessment: Conceptual input to literal-argv skill fix 751; no production skill correction included.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [609, 610].
- Evidence/limits: 15 unique reports, matrix, receipts and reproduction paths. Review retained boundary failures; claims are operational handler scope, not a sandbox guarantee.
- Blob counts: {'absent_main': 15}. Merge simulation exit 0; tree `ec1498a63524335d14a81adeeb38fbec04ac25f3`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 7 pass, 2 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...215fb5cba7d452f1c94a714446235619e92e7abe).

Own changed files:

- `A docs/research/AGENT_SKILLS_COMPATIBILITY_MATRIX.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/AGENT_SKILLS_COMPATIBILITY_MATRIX.md))
- `A docs/research/OPERATIONAL_CAPABILITY_AUDIT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/OPERATIONAL_CAPABILITY_AUDIT.md))
- `A docs/research/OPERATIONAL_CAPABILITY_AUDIT_IMPLEMENTATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/OPERATIONAL_CAPABILITY_AUDIT_IMPLEMENTATION_REPORT.md))
- `A docs/research/evidence/operational-audit/check_output_schemas.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/check_output_schemas.py))
- `A docs/research/evidence/operational-audit/cli-observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/cli-observations.json))
- `A docs/research/evidence/operational-audit/commons-observation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/commons-observation.json))
- `A docs/research/evidence/operational-audit/ctest.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/ctest.txt))
- `A docs/research/evidence/operational-audit/docs-parity.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/docs-parity.txt))
- `A docs/research/evidence/operational-audit/probe_cli.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/probe_cli.py))
- `A docs/research/evidence/operational-audit/probe_skills.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/probe_skills.py))
- `A docs/research/evidence/operational-audit/schema-validation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/schema-validation.json))
- `A docs/research/evidence/operational-audit/stub-test.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/stub-test.txt))
- `A docs/research/evidence/operational-audit/test_evidence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/test_evidence.py))
- `A docs/research/evidence/operational-audit/wrapper-observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational-audit/wrapper-observations.json))
- `A docs/research/evidence/operational_capabilities.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/215fb5cba7d452f1c94a714446235619e92e7abe/docs/research/evidence/operational_capabilities.json))

All changed files versus main merge-base (including parents):

- `A docs/research/AGENT_SKILLS_COMPATIBILITY_MATRIX.md` — absent_main; main blob `None`; head blob `01d925c497d94aa6dcad0ee06d6f6feb172706b7`.
- `A docs/research/OPERATIONAL_CAPABILITY_AUDIT.md` — absent_main; main blob `None`; head blob `0ba487271bec309d65a6c13df119f7cb29c3fdea`.
- `A docs/research/OPERATIONAL_CAPABILITY_AUDIT_IMPLEMENTATION_REPORT.md` — absent_main; main blob `None`; head blob `a345ee8c449ad6f0b70dddc254133f4e82e48100`.
- `A docs/research/evidence/operational-audit/check_output_schemas.py` — absent_main; main blob `None`; head blob `69112c7443724a0bd972c8fcdfa88ffc4056e03a`.
- `A docs/research/evidence/operational-audit/cli-observations.json` — absent_main; main blob `None`; head blob `968df077be5758cab816c424f38333307145e652`.
- `A docs/research/evidence/operational-audit/commons-observation.json` — absent_main; main blob `None`; head blob `3e447b57303021ea2749ca50663b47744cf068ee`.
- `A docs/research/evidence/operational-audit/ctest.txt` — absent_main; main blob `None`; head blob `427db41e1a8a9cc6e96383e0c228e4829804c3c0`.
- `A docs/research/evidence/operational-audit/docs-parity.txt` — absent_main; main blob `None`; head blob `32dc640a9183a23117d17816b23decb3b7b71912`.
- `A docs/research/evidence/operational-audit/probe_cli.py` — absent_main; main blob `None`; head blob `075db4a7f16292422d92cf1a7628750f91e58c40`.
- `A docs/research/evidence/operational-audit/probe_skills.py` — absent_main; main blob `None`; head blob `f21ae91c02fc8551b270ef2ac3d323a37f2eb8b3`.
- `A docs/research/evidence/operational-audit/schema-validation.json` — absent_main; main blob `None`; head blob `e6f0d10f0a98a753ee576f31b0e9a1c813b8ee76`.
- `A docs/research/evidence/operational-audit/stub-test.txt` — absent_main; main blob `None`; head blob `735efb508fc59d09918202d6eab075d8e514262f`.
- `A docs/research/evidence/operational-audit/test_evidence.py` — absent_main; main blob `None`; head blob `52d69e9412aa42c517a26a7818f6b0d0caa4cd1e`.
- `A docs/research/evidence/operational-audit/wrapper-observations.json` — absent_main; main blob `None`; head blob `0f4d55efcf3ce0f41386e604f26a0d856711e6a7`.
- `A docs/research/evidence/operational_capabilities.json` — absent_main; main blob `None`; head blob `1c7141dc0c5604ae20bf86fa678ef59c44d1fe57`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40566829807/job/122426799954): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/40566829807/job/122410232497): completed / failure.

## PR 604: docs(research): design Investigation 09 TISC and T81Lang advantage tests

- Head `827d2accbf8ccb56afe3fe8d96dfb7251f249d7d`; actual base `c930b8c2ee7478782bf3c6d231bdd59984595ef2` (`main`); main merge-base `c930b8c2ee7478782bf3c6d231bdd59984595ef2`.
- Purpose: Investigation-09 proposal and apparatus. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Related TISC conformance 707 and ISA experimental line 709, without inferred campaign permission.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Title/body emphasize design, but own delta includes 39 paths and experiment09_e1 executable apparatus. Reconcile actual scope, authority and source pins before execution or acceptance.
- Blob counts: {'absent_main': 39}. Merge simulation exit 0; tree `a387812ff7678d4d4cd0f1608213caec486274d9`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 6 pass, 3 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...827d2accbf8ccb56afe3fe8d96dfb7251f249d7d).

Own changed files:

- `A docs/research/INVESTIGATION_09_TISC_T81LANG_ADVANTAGE_PROPOSAL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/docs/research/INVESTIGATION_09_TISC_T81LANG_ADVANTAGE_PROPOSAL.md))
- `A experiments/investigation09_e1/CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/CMakeLists.txt))
- `A experiments/investigation09_e1/ENTRY_CONTRACT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/ENTRY_CONTRACT.md))
- `A experiments/investigation09_e1/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/README.md))
- `A experiments/investigation09_e1/audit.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/audit.py))
- `A experiments/investigation09_e1/driver.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/driver.cpp))
- `A experiments/investigation09_e1/evidence-2026-10-27/SHA256SUMS.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/SHA256SUMS.json))
- `A experiments/investigation09_e1/evidence-2026-10-27/archive-contents-sha256.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/archive-contents-sha256.json))
- `A experiments/investigation09_e1/evidence-2026-10-27/audit-manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/audit-manifest.json))
- `A experiments/investigation09_e1/evidence-2026-10-27/comparison.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/comparison.json))
- `A experiments/investigation09_e1/evidence-2026-10-27/ledger.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/ledger.json))
- `A experiments/investigation09_e1/evidence-2026-10-27/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/manifest.json))
- `A experiments/investigation09_e1/evidence-2026-10-27/raw-evidence.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/raw-evidence.tar.gz))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-join-expanded.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-join-expanded.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-join-native.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-join-native.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-meet-expanded.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-meet-expanded.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-meet-native.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-meet-native.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/boolean-branch-O2-binaryen-Oz.wasm` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/boolean-branch-O2-binaryen-Oz.wasm))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/boolean.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/boolean.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/combined-branch-O2-binaryen-Oz.wasm` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/combined-branch-O2-binaryen-Oz.wasm))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/combined.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/combined.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/decision-branch-O2-binaryen-Oz.wasm` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/decision-branch-O2-binaryen-Oz.wasm))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/decision.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/decision.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/identity-branch-O2-binaryen-Oz.wasm` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/identity-branch-O2-binaryen-Oz.wasm))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/identity.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/identity.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/integer-branch-O2-binaryen-Oz.wasm` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/integer-branch-O2-binaryen-Oz.wasm))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/integer.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/integer.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/join-branch-O2-binaryen-Oz.wasm` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/join-branch-O2-binaryen-Oz.wasm))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/join.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/join.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/meet-branch-O2-binaryen-Oz.wasm` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/meet-branch-O2-binaryen-Oz.wasm))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/meet.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/meet.tisc))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/neg-branch-O2-binaryen-Oz.wasm` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/neg-branch-O2-binaryen-Oz.wasm))
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/neg.tisc` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/neg.tisc))
- `A experiments/investigation09_e1/oracle.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/oracle.py))
- `A experiments/investigation09_e1/package_evidence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/package_evidence.py))
- `A experiments/investigation09_e1/preflight/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/preflight/README.md))
- `A experiments/investigation09_e1/preflight/rejected_boolean_cast.t81` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/preflight/rejected_boolean_cast.t81))
- `A experiments/investigation09_e1/requirements.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/requirements.txt))
- `A experiments/investigation09_e1/run.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/827d2accbf8ccb56afe3fe8d96dfb7251f249d7d/experiments/investigation09_e1/run.py))

All changed files versus main merge-base (including parents):

- `A docs/research/INVESTIGATION_09_TISC_T81LANG_ADVANTAGE_PROPOSAL.md` — absent_main; main blob `None`; head blob `cb5709c85709f062ec91e10de640bd2975463994`.
- `A experiments/investigation09_e1/CMakeLists.txt` — absent_main; main blob `None`; head blob `7c051bade8a7388a75c7e3da6709a80fb08851ec`.
- `A experiments/investigation09_e1/ENTRY_CONTRACT.md` — absent_main; main blob `None`; head blob `1e57778cc378eb588f0751aa2836a2078d6c0696`.
- `A experiments/investigation09_e1/README.md` — absent_main; main blob `None`; head blob `c7f1d640b12d7fc5b3cecbead77ef947b6c91a8b`.
- `A experiments/investigation09_e1/audit.py` — absent_main; main blob `None`; head blob `0bc02e16b5d0df6f0be5235fd6a6311cc84b5f47`.
- `A experiments/investigation09_e1/driver.cpp` — absent_main; main blob `None`; head blob `4ff1577348cc7f35c3c1a4267f89f47d2f4240f3`.
- `A experiments/investigation09_e1/evidence-2026-10-27/SHA256SUMS.json` — absent_main; main blob `None`; head blob `16f837da16e1277f2490a767e6dc64363ae3b6d5`.
- `A experiments/investigation09_e1/evidence-2026-10-27/archive-contents-sha256.json` — absent_main; main blob `None`; head blob `b42b431354944da6b0c52acf7f221c1bc4ecd868`.
- `A experiments/investigation09_e1/evidence-2026-10-27/audit-manifest.json` — absent_main; main blob `None`; head blob `dace608ed53fc9f856cc14ca0c3761e39080900a`.
- `A experiments/investigation09_e1/evidence-2026-10-27/comparison.json` — absent_main; main blob `None`; head blob `e1e96074f83898d9baf1f9536b2270f4cb4e77bf`.
- `A experiments/investigation09_e1/evidence-2026-10-27/ledger.json` — absent_main; main blob `None`; head blob `48bbf7dabe423cb46c5ddb632d3e34aa8135eb36`.
- `A experiments/investigation09_e1/evidence-2026-10-27/manifest.json` — absent_main; main blob `None`; head blob `c2820046f8f0244738ef576843a5d48ff0fbc239`.
- `A experiments/investigation09_e1/evidence-2026-10-27/raw-evidence.tar.gz` — absent_main; main blob `None`; head blob `d7e0ced85cd322d5c33ad5f0f6515c1a95fd5654`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-join-expanded.tisc` — absent_main; main blob `None`; head blob `70fb8feb96dd40e72d205e2e5ee5c78c55cc700c`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-join-native.tisc` — absent_main; main blob `None`; head blob `d188796fd7bb85faa793eaa176a56617c4123849`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-meet-expanded.tisc` — absent_main; main blob `None`; head blob `e82cf39b57b9a9a1a6674a36ff10f8d69d5fb0bb`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/authored-meet-native.tisc` — absent_main; main blob `None`; head blob `24e1170f847638060d04393dc3d1dc554b4c2a38`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/boolean-branch-O2-binaryen-Oz.wasm` — absent_main; main blob `None`; head blob `d06ffef37dc35b8d5b58daace66e7959048b3923`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/boolean.tisc` — absent_main; main blob `None`; head blob `419e49013e19f65920c5b760d7afaf20f6ad8075`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/combined-branch-O2-binaryen-Oz.wasm` — absent_main; main blob `None`; head blob `69f077f3a81068f6a32becb8b9e4e7714679c6a0`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/combined.tisc` — absent_main; main blob `None`; head blob `432427534e3ba0e6934f45e54b3a5360d76b9cd2`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/decision-branch-O2-binaryen-Oz.wasm` — absent_main; main blob `None`; head blob `58a89174e6e6f5954353d8ecc08ddfaa05a8474b`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/decision.tisc` — absent_main; main blob `None`; head blob `733f2a0fd45726f4c2374b222713b6bda7dfa41c`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/identity-branch-O2-binaryen-Oz.wasm` — absent_main; main blob `None`; head blob `c5dc69dd38b115a1dc7468b2ae77b6f20c02be7c`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/identity.tisc` — absent_main; main blob `None`; head blob `f224217d3138cc12ef7983a33c1bc95a14695661`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/integer-branch-O2-binaryen-Oz.wasm` — absent_main; main blob `None`; head blob `456865c96c6fb0e3f0f40d1f071fb60efe0a43bb`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/integer.tisc` — absent_main; main blob `None`; head blob `44ae060a72e1ece0c6cda7b6c61269957fc2f25d`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/join-branch-O2-binaryen-Oz.wasm` — absent_main; main blob `None`; head blob `b8dacd24f2cb097b0200b73f2006855703c83f1e`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/join.tisc` — absent_main; main blob `None`; head blob `2798ff0bd0e5c45c18deeec5e857a44671bbafa2`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/meet-branch-O2-binaryen-Oz.wasm` — absent_main; main blob `None`; head blob `9f552a22e6f528ca2825d35f603c63f97a25efa6`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/meet.tisc` — absent_main; main blob `None`; head blob `b10ac1002cadf7fe528ffb3db31c36227072234c`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/neg-branch-O2-binaryen-Oz.wasm` — absent_main; main blob `None`; head blob `5c8dcda24bc68a2038e70c82777c4059ef129fa7`.
- `A experiments/investigation09_e1/evidence-2026-10-27/selected-artifacts/neg.tisc` — absent_main; main blob `None`; head blob `c7fff83fb2cb78bd4f80da72ef8633c908025afb`.
- `A experiments/investigation09_e1/oracle.py` — absent_main; main blob `None`; head blob `a6857d7a56d2558ef18fa779af116daa97096730`.
- `A experiments/investigation09_e1/package_evidence.py` — absent_main; main blob `None`; head blob `d37299016924ed750c119857779d3f99f46783f0`.
- `A experiments/investigation09_e1/preflight/README.md` — absent_main; main blob `None`; head blob `877f58d815efe58203bab7aee4965f2c53bc16f0`.
- `A experiments/investigation09_e1/preflight/rejected_boolean_cast.t81` — absent_main; main blob `None`; head blob `90b3355be1363a1bfd1f9b93abbc0ef8794953ae`.
- `A experiments/investigation09_e1/requirements.txt` — absent_main; main blob `None`; head blob `0e3b2a22b7b9b8b67f7caa46a36520079546f001`.
- `A experiments/investigation09_e1/run.py` — absent_main; main blob `None`; head blob `b20b621ea0da0ac75554234c19f9fb9418da8505`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40546817712/job/122340307953): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/40546817712/job/122339518516): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/40546817712/job/122339517958): completed / failure.

## PR 603: research: instrument Investigation 08 phases and attribute execution cost

- Head `0ae27310b1eade876e9b2036e3ddd811e7e86537`; actual base `7c61afa3ba0e97c3c800ca5cde567cf374d7a5cc` (`jules-8722614166180261830-8f0dac8c`); main merge-base `c930b8c2ee7478782bf3c6d231bdd59984595ef2`.
- Purpose: Investigation-08 phase-cost instrumentation. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Base ref is closed 602 branch; main shares some inherited blobs but not whole comparator tree.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [602].
- Evidence/limits: 27 own paths plus inherited comparator changes; base 602 is CLOSED UNMERGED. Identify missing comparator dependency and surviving current-main changes; no blind retarget or performance claim.
- Blob counts: {'different': 14, 'identical': 12, 'absent_main': 15}. Merge simulation exit 1; tree `79795b42da3b179e0bf5ce02029c3835e98e3cb6`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...0ae27310b1eade876e9b2036e3ddd811e7e86537).

Own changed files:

- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/CMakeLists.txt))
- `A experiments/integrated_system_advantage/PHASE_CALL_PATH.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/PHASE_CALL_PATH.md))
- `A experiments/integrated_system_advantage/PHASE_COST_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/PHASE_COST_REPORT.md))
- `A experiments/integrated_system_advantage/PHASE_METHODOLOGY.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/PHASE_METHODOLOGY.md))
- `M experiments/integrated_system_advantage/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/README.md))
- `M experiments/integrated_system_advantage/parity_probe.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/parity_probe.cpp))
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/environment.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase-evidence-2026-10-27/environment.json))
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase-evidence-2026-10-27/manifest.json))
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/observations.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase-evidence-2026-10-27/observations.json))
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/recording-control.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase-evidence-2026-10-27/recording-control.json))
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/source-identities.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase-evidence-2026-10-27/source-identities.json))
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/summary.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase-evidence-2026-10-27/summary.json))
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/validation.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase-evidence-2026-10-27/validation.tar.gz))
- `A experiments/integrated_system_advantage/phase_experiment.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase_experiment.py))
- `A experiments/integrated_system_advantage/phase_recording_control.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase_recording_control.py))
- `A experiments/integrated_system_advantage/phase_records.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/phase_records.py))
- `A experiments/integrated_system_advantage/summarize_phase_costs.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/summarize_phase_costs.py))
- `M experiments/integrated_system_advantage/test_differential_parity.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/experiments/integrated_system_advantage/test_differential_parity.py))
- `M fs/persistent_driver.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/fs/persistent_driver.cpp))
- `M tools/cli/ai/ai_cli_shared.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/tools/cli/ai/ai_cli_shared.cpp))
- `M tools/cli/core/main.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/tools/cli/core/main.cpp))
- `M tools/model/weights.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/tools/model/weights.cpp))
- `M vm/decode_state.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/vm/decode_state.cpp))
- `M vm/decoder.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/vm/decoder.cpp))
- `A vm/internal/investigation08_timing.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/vm/internal/investigation08_timing.hpp))
- `M vm/tensor_helpers.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/vm/tensor_helpers.cpp))
- `M vm/vm.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/0ae27310b1eade876e9b2036e3ddd811e7e86537/vm/vm.cpp))

All changed files versus main merge-base (including parents):

- `M .gitignore` — different; main blob `1dfd6156c8914161dafd9043cc76e9bb42fae95a`; head blob `f820be12360108c5dc9712da6e5e865e05b5c87d`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `bdbfb5e32ac5b7174e0601a63353ee8146f5d4d2`.
- `A experiments/integrated_system_advantage/CORRECTIVE_REVIEW.md` — identical; main blob `726ab004777c6e68ee3872a7bcb75a9c9311b22f`; head blob `726ab004777c6e68ee3872a7bcb75a9c9311b22f`.
- `A experiments/integrated_system_advantage/PHASE_CALL_PATH.md` — absent_main; main blob `None`; head blob `d2527dc5d6efed212267da1bf91f86f7c387c631`.
- `A experiments/integrated_system_advantage/PHASE_COST_REPORT.md` — absent_main; main blob `None`; head blob `ea19d5b721df7a971f1daaa53acb7deb68f6d27a`.
- `A experiments/integrated_system_advantage/PHASE_METHODOLOGY.md` — absent_main; main blob `None`; head blob `f7db7d2b0bd56423630721bfb6d222e1ac7888ee`.
- `M experiments/integrated_system_advantage/README.md` — different; main blob `634d9b5158ca6cf6ce0ada51f879050febc148b3`; head blob `e6e3e810685384c4c7e9d3b4c771c3972cfe9eb8`.
- `M experiments/integrated_system_advantage/ablation_runner.py` — identical; main blob `76d46992c4e3f8a45a6f4cd57dbf907ed1b36645`; head blob `76d46992c4e3f8a45a6f4cd57dbf907ed1b36645`.
- `M experiments/integrated_system_advantage/baseline_stack.py` — different; main blob `9ab2064635b74f03795f48c35c5b5ca2446bafd1`; head blob `0ff9eba560559654e4f2fb0731def6c4627914b4`.
- `A experiments/integrated_system_advantage/baseline_worker.py` — identical; main blob `62c55bb1f06545c0f4bc2dd5026c45e78f07a7ab`; head blob `62c55bb1f06545c0f4bc2dd5026c45e78f07a7ab`.
- `A experiments/integrated_system_advantage/historical/README.md` — identical; main blob `8ed4b47d9f771048ae7f35036f3493ce5218d870`; head blob `8ed4b47d9f771048ae7f35036f3493ce5218d870`.
- `A experiments/integrated_system_advantage/historical/base-597-results.json` — identical; main blob `07341c9c0109c636a3d7c48a9678fbaaa4b6aa32`; head blob `07341c9c0109c636a3d7c48a9678fbaaa4b6aa32`.
- `A experiments/integrated_system_advantage/historical/manifest.json` — identical; main blob `a6a5cf699546267e8576b64344a80ed572b21d83`; head blob `a6a5cf699546267e8576b64344a80ed572b21d83`.
- `A experiments/integrated_system_advantage/historical/original-602-results.json` — identical; main blob `6867634be4b5a429f9f61824faa9a2e01c444aac`; head blob `6867634be4b5a429f9f61824faa9a2e01c444aac`.
- `A experiments/integrated_system_advantage/historical/pre-correction-602-results.json` — identical; main blob `701179f2f34e0fac8853e2a3280899ccb42f89b2`; head blob `701179f2f34e0fac8853e2a3280899ccb42f89b2`.
- `A experiments/integrated_system_advantage/observations-2026-10-27-corrective.json` — identical; main blob `3e4188f4e974b0c76439bf2e27974a354218d1b3`; head blob `3e4188f4e974b0c76439bf2e27974a354218d1b3`.
- `A experiments/integrated_system_advantage/observations-2026-10-27-sources.tar.gz` — identical; main blob `dbfcde118219da1809e9b43e0162c15be98d4a25`; head blob `dbfcde118219da1809e9b43e0162c15be98d4a25`.
- `A experiments/integrated_system_advantage/parity_probe.cpp` — different; main blob `0726a63564022bbd67101a2fcd66e5d0160b022e`; head blob `caf9307f021e73780dfe23dc8b16b65fb89ef28b`.
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/environment.json` — absent_main; main blob `None`; head blob `395ad5587dec7507b34fc48cea54074bad68fea0`.
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/manifest.json` — absent_main; main blob `None`; head blob `da7d2fa449c38dd705eb257e25e8896151bbc795`.
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/observations.json` — absent_main; main blob `None`; head blob `8611c6f9bd1e54330f4c34073fcc8df17a1a5c90`.
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/recording-control.json` — absent_main; main blob `None`; head blob `b9ca20e8a7182cafe151dc25b8e4e726bd182ce1`.
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/source-identities.json` — absent_main; main blob `None`; head blob `6dd0ecda532c64d704e0f5e7be39b164c99c7e50`.
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/summary.json` — absent_main; main blob `None`; head blob `33d3d5d26eb553abe1c3e0bdee6bc9d4d533bc6d`.
- `A experiments/integrated_system_advantage/phase-evidence-2026-10-27/validation.tar.gz` — absent_main; main blob `None`; head blob `4ef4ece7ec5d68d9d2d050d5ed929fb736369e21`.
- `A experiments/integrated_system_advantage/phase_experiment.py` — absent_main; main blob `None`; head blob `54064248ad41175d522d72b2dc26cee9e7f039bc`.
- `A experiments/integrated_system_advantage/phase_recording_control.py` — absent_main; main blob `None`; head blob `698d21b62282bb3cb49ad4ef1b17d8623f418be4`.
- `A experiments/integrated_system_advantage/phase_records.py` — absent_main; main blob `None`; head blob `ba7147bff49e49e3da01eed50a356c0a27fb54ae`.
- `M experiments/integrated_system_advantage/results.json` — identical; main blob `972437df1b1268f34b5dee1ca77c01f33704a7c4`; head blob `972437df1b1268f34b5dee1ca77c01f33704a7c4`.
- `M experiments/integrated_system_advantage/run_experiments.py` — identical; main blob `4cf2bb3dfa586b30406878152a6442b269c1fde3`; head blob `4cf2bb3dfa586b30406878152a6442b269c1fde3`.
- `A experiments/integrated_system_advantage/summarize_phase_costs.py` — absent_main; main blob `None`; head blob `7b5085a41bbd38405d62dca591f08341e4044f1a`.
- `A experiments/integrated_system_advantage/test_differential_parity.py` — different; main blob `a664215e9c10f1dc9369761a0c1676536d5bc782`; head blob `7ddbb9a453160e35bdba8478b81b272315240d20`.
- `M fs/persistent_driver.cpp` — different; main blob `9ae4182253d5a298a75290a7ac4a99daa92ecd17`; head blob `e6bcad1d00d25febcfdf973d9a453385fcfd7328`.
- `M tools/cli/ai/ai_cli_shared.cpp` — different; main blob `4100ea9cf3bbae72e0c99600554dae52a3bb2b0f`; head blob `75c697f1086954d780efccb572a1e50ab8caa7d5`.
- `M tools/cli/core/main.cpp` — different; main blob `958dba8a20105a59adecd775bb6b642babedb282`; head blob `096afe8958efcaa86c9869bdbc16c2c8c14160be`.
- `M tools/model/weights.cpp` — different; main blob `5fc9943cc9ed03771d0df81b7b4f8e1877d517b3`; head blob `cc768ca63f55ab9dd913818c98cf74f3af0fd5b0`.
- `M vm/decode_state.cpp` — different; main blob `095378e806adc7811befee049b8b67d061b15cf1`; head blob `05ce7ec8ee3725401b68640e9f30221ba4f90654`.
- `M vm/decoder.cpp` — different; main blob `2982780e0783fb87b56c394af943257d12357595`; head blob `aaa26b072730e465abbcdaf12b323f58dd04515c`.
- `A vm/internal/investigation08_timing.hpp` — absent_main; main blob `None`; head blob `84a96527a039935b9c397a37adf5d253ad434009`.
- `M vm/tensor_helpers.cpp` — different; main blob `32554cd0e2fddc78968ec46f65cc8fdebf978030`; head blob `0a0624417c4039bef6bb1fe873b1cd39a36a04a8`.
- `M vm/vm.cpp` — different; main blob `698f5459900224cbfe59c90eec765e2504ab053b`; head blob `f9355cb0d712d4c6a771fa981f3620d9a1be5e61`.

Overlaps with other PR own deltas:

- #782: `CMakeLists.txt`, `tools/cli/core/main.cpp`
- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`, `tools/cli/core/main.cpp`
- #747: `CMakeLists.txt`, `tools/cli/core/main.cpp`, `vm/decoder.cpp`
- #649: `CMakeLists.txt`
- #556: `CMakeLists.txt`, `fs/persistent_driver.cpp`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`, `vm/vm.cpp`
- #528: `vm/vm.cpp`
- #508: `CMakeLists.txt`
- #504: `vm/tensor_helpers.cpp`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 601: docs(status): reconcile project status with current implementation and research evidence

- Head `9af0bffb29a66375ac3aaaf5012e91ffa4613e06`; actual base `c930b8c2ee7478782bf3c6d231bdd59984595ef2` (`main`); main merge-base `c930b8c2ee7478782bf3c6d231bdd59984595ef2`.
- Purpose: Status-directory evidence reconciliation. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Overlaps historical narrative subjects 640/657 and current 764; no proven wholesale supersession.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [565, 566, 597, 600].
- Evidence/limits: 32 paths include a unique historical inventory and 31 divergent status docs. Reconcile facts into dated overlays; do not replace current docs wholesale or refresh dates without validation.
- Blob counts: {'different': 31, 'absent_main': 1}. Merge simulation exit 1; tree `59a56f0ec95afe6a39ebe755c4fda77ddd1470a3`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 20 pass, 12 fail, 8 skip, 1 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...9af0bffb29a66375ac3aaaf5012e91ffa4613e06).

Own changed files:

- `M docs/status/ACTIVE_RISKS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/ACTIVE_RISKS.md))
- `M docs/status/AI_CLI_IMPLEMENTATION_MATRIX.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/AI_CLI_IMPLEMENTATION_MATRIX.md))
- `M docs/status/AI_CLI_MILESTONE_EVIDENCE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/AI_CLI_MILESTONE_EVIDENCE.md))
- `M docs/status/AI_RFC_BACKLOG.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/AI_RFC_BACKLOG.md))
- `M docs/status/AUDIT_REMEDIATION_CROSSWALK.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/AUDIT_REMEDIATION_CROSSWALK.md))
- `M docs/status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md))
- `M docs/status/CI_GATE_STATUS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/CI_GATE_STATUS.md))
- `M docs/status/CI_WORKFLOW_CONFIRMATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/CI_WORKFLOW_CONFIRMATION.md))
- `M docs/status/DECISION_LOG.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/DECISION_LOG.md))
- `M docs/status/DEPENDENCY_HEALTH.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/DEPENDENCY_HEALTH.md))
- `M docs/status/DETERMINISM_AUDIT_LOG.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/DETERMINISM_AUDIT_LOG.md))
- `M docs/status/DRIFT_DECOMPOSITION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/DRIFT_DECOMPOSITION.md))
- `M docs/status/EXTENSION_PROFILE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/EXTENSION_PROFILE.md))
- `M docs/status/FROZEN_CORE_PROFILE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/FROZEN_CORE_PROFILE.md))
- `M docs/status/GOVERNANCE_BOUNDARIES_CLARIFICATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/GOVERNANCE_BOUNDARIES_CLARIFICATION.md))
- `M docs/status/GOVERNANCE_REVIEW_CADENCE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/GOVERNANCE_REVIEW_CADENCE.md))
- `M docs/status/HARDENING_BACKLOG.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/HARDENING_BACKLOG.md))
- `M docs/status/IMPLEMENTATION_MATRIX.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/IMPLEMENTATION_MATRIX.md))
- `M docs/status/JIT_EQUIVALENCE_GAP.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/JIT_EQUIVALENCE_GAP.md))
- `M docs/status/PROJECT_CONTROL_CENTER.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/PROJECT_CONTROL_CENTER.md))
- `M docs/status/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/README.md))
- `M docs/status/RFC_0030_FLOAT_DOMAIN_PROMOTION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/RFC_0030_FLOAT_DOMAIN_PROMOTION.md))
- `M docs/status/ROADMAP.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/ROADMAP.md))
- `A docs/status/STATUS_RECONCILIATION_2026-10-27.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/STATUS_RECONCILIATION_2026-10-27.md))
- `M docs/status/STDLIB_PROMOTION_SNAPSHOT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/STDLIB_PROMOTION_SNAPSHOT.md))
- `M docs/status/SYSTEM_STATUS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/SYSTEM_STATUS.md))
- `M docs/status/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md))
- `M docs/status/T81LANG_PROMOTION_GATE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/T81LANG_PROMOTION_GATE.md))
- `M docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md))
- `M docs/status/T81LANG_TRACEABILITY_MATRIX.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/T81LANG_TRACEABILITY_MATRIX.md))
- `M docs/status/TASKS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/TASKS.md))
- `M docs/status/TERNARY_FABRIC_STRATEGIC_CONVERGENCE_NOTE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/9af0bffb29a66375ac3aaaf5012e91ffa4613e06/docs/status/TERNARY_FABRIC_STRATEGIC_CONVERGENCE_NOTE.md))

All changed files versus main merge-base (including parents):

- `M docs/status/ACTIVE_RISKS.md` — different; main blob `9816bc8c7be7335c8b0b464595d63ec06736b0c8`; head blob `1532420d02b16fc2292cb05833c5174c3da9cd98`.
- `M docs/status/AI_CLI_IMPLEMENTATION_MATRIX.md` — different; main blob `260f4e9a27327f0673de15b1fc912863fa02af09`; head blob `7839c47948cb721719a1a16faf50bc7afef992f0`.
- `M docs/status/AI_CLI_MILESTONE_EVIDENCE.md` — different; main blob `57c76652789e872cda96693528899850f508370a`; head blob `1b6be2f7e8419b1a2c2920766db716c62ed303ff`.
- `M docs/status/AI_RFC_BACKLOG.md` — different; main blob `b4edaded00c8fcd3eedbde8c60650ffa53d60216`; head blob `ef68fb02efcebd31e4194e92a8ca73de1926b9ea`.
- `M docs/status/AUDIT_REMEDIATION_CROSSWALK.md` — different; main blob `4ebcc60a2cd6a503495544910c1b38d1bfd94893`; head blob `5bdcf8ba8cfda8b78421d0dbd86a95183255702a`.
- `M docs/status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md` — different; main blob `999364be514795f84b6657f52986e85d5e02f292`; head blob `7e8ccbbb709cd3b3cb98c81afba75bd48bb11334`.
- `M docs/status/CI_GATE_STATUS.md` — different; main blob `6253c338252d8b1e4ec9dbf12693643fa700180b`; head blob `f34b5a4ac5d4d57f6b050cb9803ecb1764d3b5df`.
- `M docs/status/CI_WORKFLOW_CONFIRMATION.md` — different; main blob `49958738248c69f99d11b84acfb049af1e9d52c0`; head blob `72198cc411e5414b97cf611aa7a1f7b146f44661`.
- `M docs/status/DECISION_LOG.md` — different; main blob `51a3b11bfa99ee86f748add234a734a9e2110c15`; head blob `4b1ab1e6f07bb53709264874ad5f4bbeb32a0a15`.
- `M docs/status/DEPENDENCY_HEALTH.md` — different; main blob `4baaa4894d2916f9879b0dbd3467f9bcbf2db515`; head blob `bc09104d6e9e437369901729184f68e70d95c87e`.
- `M docs/status/DETERMINISM_AUDIT_LOG.md` — different; main blob `69d6e123aff4a4a9a4b0b4b7d7c9b178b447766d`; head blob `3e997eb5f7a84f6334a260ac2459f4f506ba3f5e`.
- `M docs/status/DRIFT_DECOMPOSITION.md` — different; main blob `75d2c4a287e7e4397674fd0ebbeeeecc78e14aa1`; head blob `a071f9221d477e62207dac475cd789d90082ebc2`.
- `M docs/status/EXTENSION_PROFILE.md` — different; main blob `bf4bada90669a36249e78135c26d5ef4113ad91c`; head blob `f911cb4a3bf59c0c5e7d28abe2f9096d8710d556`.
- `M docs/status/FROZEN_CORE_PROFILE.md` — different; main blob `e99a95e6a6135d60e3be00d003d99a416153bbdc`; head blob `a175d7db7c72a06433d04e0d2294fdcac81be811`.
- `M docs/status/GOVERNANCE_BOUNDARIES_CLARIFICATION.md` — different; main blob `01f1bb83c22cbd77362d4b94e46ee4be0da24823`; head blob `382d2a692cb54a170a064a8e432d443d3d4ebe31`.
- `M docs/status/GOVERNANCE_REVIEW_CADENCE.md` — different; main blob `b5b942dd061ef746f414cd52d780a5f237bc7d69`; head blob `a5bb3ca3f278283d369fc7a05c37a7e0d2258613`.
- `M docs/status/HARDENING_BACKLOG.md` — different; main blob `4df6a77fc63e9c28eed6f533bb88bf8d4ac2d7e6`; head blob `8e33153c377ffe6c4bba27d370ea086e7dd4e3a6`.
- `M docs/status/IMPLEMENTATION_MATRIX.md` — different; main blob `e052a7fd04f5869daebf9ed8f6e90c40eaab5822`; head blob `839803d91fad7c8e9c2e7ee206462cbe8c4431e4`.
- `M docs/status/JIT_EQUIVALENCE_GAP.md` — different; main blob `8dcfde3e47c0b7ec806324db4748456ffc5994d6`; head blob `a2eb64e1ded5044785dd8d86246ef8f072fd4a04`.
- `M docs/status/PROJECT_CONTROL_CENTER.md` — different; main blob `413942110f1b4f63115405173ec8b5f6b0f3c893`; head blob `c55a5c440d6c784a806cfd3d116d1503a140f7a9`.
- `M docs/status/README.md` — different; main blob `5aee776b1615df261c35cb2bc95ec637981bb35f`; head blob `3064055bbc4e9cffc5a11369b84651f5be378df6`.
- `M docs/status/RFC_0030_FLOAT_DOMAIN_PROMOTION.md` — different; main blob `170493b5078636ee995269245af7b314773182c9`; head blob `5360a8bf9890225b5a7d599a305118dfa5a6a3d6`.
- `M docs/status/ROADMAP.md` — different; main blob `1b3dd62c9cf835e4d415461d4681be713f5ecc9c`; head blob `8d48651591e635fbba954344f6884cf56cf31717`.
- `A docs/status/STATUS_RECONCILIATION_2026-10-27.md` — absent_main; main blob `None`; head blob `2211de700db2668635e3c7ac618e17a1ccc6f7ac`.
- `M docs/status/STDLIB_PROMOTION_SNAPSHOT.md` — different; main blob `52fc9d2372a10ee6f16e9201aba2a67c62b82b54`; head blob `0ad634021ad4190e0e3e9261a3861637ec6c7954`.
- `M docs/status/SYSTEM_STATUS.md` — different; main blob `36175df272254435ca19d15b2d544dfa90276b36`; head blob `1db6ad402e4e7f0aea70a7d6ebaa2d887a6292fa`.
- `M docs/status/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md` — different; main blob `ad994f3fdc2580b63ff627c9553b802f7824856e`; head blob `396153cc6d2a6cc0cdee645659b2aac32e427e9b`.
- `M docs/status/T81LANG_PROMOTION_GATE.md` — different; main blob `1cc5416b03ecc537c2c8c2fb29a25c1514801f25`; head blob `8dbcceb2bfccffde0cbc004c45fdbff171aa48ce`.
- `M docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md` — different; main blob `03cb3659c6909547b696d64639364388179d813c`; head blob `67c3baf892d73fd6f5a58de4830106a89608a85d`.
- `M docs/status/T81LANG_TRACEABILITY_MATRIX.md` — different; main blob `587db2f01d27660b086511e75eeeeadef4299194`; head blob `0457db5724b301f491041be371cfd5ed6ccd0442`.
- `M docs/status/TASKS.md` — different; main blob `0ae0ea9b5b42d7f68bf6646aa82e09d9e364ade2`; head blob `0e6720992f422ecdee7eaa78a8033f443507884d`.
- `M docs/status/TERNARY_FABRIC_STRATEGIC_CONVERGENCE_NOTE.md` — different; main blob `1a4a4d93afa3afd051d8cd7f8a7920efe151a59c`; head blob `749be625d1a0c43c7f92e6badca451c29d507262`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122061521354): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122050447326): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122050447289): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122050447270): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122050447256): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122050447244): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122050447201): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122050447187): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122033614529): completed / failure.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122033614518): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122033614479): completed / cancelled.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122033614407): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/40455424837/job/122027528470): completed / failure.

## PR 580: Scope A — Typed Root Governance Validator and Preservation Enforcement (Preparation Only)

- Head `353d7592a725e8e2025238a9fc247760950c8297`; actual base `71bb2c0e937dbcad73c9421e5465651b6e0d3ec3` (`corrective/root-layout-contract`); main merge-base `01c85019319fd020922402ba356878e392cb2231`.
- Purpose: Typed root contract and preservation validator preparation. Decision: **DEFER**.
- Dependency assessment: Stacks on 579, transitively 578/577/576/575/574; overlaps 649 canonicalization decision.
- Open heads included by ancestry: [574, 575, 576, 577, 578, 579]; references in body (not necessarily dependencies): [579].
- Evidence/limits: 38 own paths, 101 including inherited stack. Preparation-only authority does not activate policy; frozen contract/authorization decisions precede required-CI changes.
- Blob counts: {'different': 13, 'absent_main': 88}. Merge simulation exit 1; tree `82a995f178db5c990c08ce11b095aa724bd8bf4a`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...353d7592a725e8e2025238a9fc247760950c8297).

Own changed files:

- `A docs/governance/root-layout-contract.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/governance/root-layout-contract.json))
- `A docs/governance/root-layout-contract.schema.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/governance/root-layout-contract.schema.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/README.md))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/all-root-tests.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/all-root-tests.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/authorization-replacement-before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/authorization-replacement-before.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/cmake-limits.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/cmake-limits.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/cmake-prefix-before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/cmake-prefix-before.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/cmake_boundary_probes.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/cmake_boundary_probes.py))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/corrected-defects.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/corrected-defects.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/identities-and-preservation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/identities-and-preservation.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/inventory-required.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/inventory-required.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/inventory-standalone.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/inventory-standalone.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/nested-root-before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/nested-root-before.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/raw-tree-defects-after.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/raw-tree-defects-after.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/raw-tree-defects-before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/raw-tree-defects-before.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/regressions.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/regressions.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/reproduced-defects.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/reproduced-defects.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/root-order-after.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/root-order-after.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/root-order-before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/root-order-before.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/trust-limits.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/trust-limits.json))
- `A docs/records/audits/2026-10-root-governance-adversarial-review/trust_boundary_probes.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-adversarial-review/trust_boundary_probes.py))
- `A docs/records/audits/2026-10-root-governance-implementation/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/README.md))
- `A docs/records/audits/2026-10-root-governance-implementation/after-required.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/after-required.json))
- `A docs/records/audits/2026-10-root-governance-implementation/after-standalone.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/after-standalone.json))
- `A docs/records/audits/2026-10-root-governance-implementation/authorization-comment.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/authorization-comment.json))
- `A docs/records/audits/2026-10-root-governance-implementation/before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/before.json))
- `A docs/records/audits/2026-10-root-governance-implementation/failure-propagation-results.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/failure-propagation-results.json))
- `A docs/records/audits/2026-10-root-governance-implementation/fixture-fix-validation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/fixture-fix-validation.json))
- `A docs/records/audits/2026-10-root-governance-implementation/inventory.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/inventory.md))
- `A docs/records/audits/2026-10-root-governance-implementation/preservation-and-identities.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/preservation-and-identities.json))
- `A docs/records/audits/2026-10-root-governance-implementation/regressions-before-fixture-fix.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/regressions-before-fixture-fix.json))
- `A docs/records/audits/2026-10-root-governance-implementation/regressions.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/regressions.json))
- `A docs/records/audits/2026-10-root-governance-implementation/standalone-modes.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/docs/records/audits/2026-10-root-governance-implementation/standalone-modes.json))
- `M scripts/governance/check_root_structure.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/scripts/governance/check_root_structure.py))
- `A scripts/governance/check_root_structure_legacy.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/scripts/governance/check_root_structure_legacy.py))
- `A spec/rfcs/RFC-0057-root-layout-authority-preservation-and-contract-approval.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/spec/rfcs/RFC-0057-root-layout-authority-preservation-and-contract-approval.md))
- `M spec/rfcs/index.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/spec/rfcs/index.md))
- `A tests/ci/check_root_structure_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/353d7592a725e8e2025238a9fc247760950c8297/tests/ci/check_root_structure_test.py))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `534c80263b70fb5ca14b2a375ac7d90585e2d480`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `ec07a16879a1813cf2b5ec7e0033c59799d79e6f`.
- `M .github/workflows/docker.yml` — different; main blob `c1218c1d6c96411f6893094866cc72e6ced488a6`; head blob `bc3bbc39382e2f36c6d1cda9bb08d2f8f35bacd8`.
- `M .github/workflows/python-wheels.yml` — different; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `e31ce23d4f616178c788e35a782b4f845b5fd191`.
- `M .github/workflows/qemu-boot-x86.yml` — different; main blob `bcad444157a39c3d86355298094746016a17e9cc`; head blob `34690e16f1d90dfd1d1512a02446a592465dd110`.
- `M .github/workflows/qemu-boot.yml` — different; main blob `873acefcfe4db9c5f09ceee798862e9a39ce5d68`; head blob `3ddab69d9aa403a6cb96d2e1026475d6b978dca4`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `a49327d6eaf5ac128d03013935db95ec9558d49f`.
- `M docs/explanation/ARCHITECTURE.md` — different; main blob `98d2ea3a56c36d7e0063818575c1e61bb1acc632`; head blob `c36ed9de36ee15c71755396715c08a3d3de1664a`.
- `A docs/governance/root-layout-contract.json` — absent_main; main blob `None`; head blob `fdb216be021db6aa13242aeeb859d8f4cab8d135`.
- `A docs/governance/root-layout-contract.schema.json` — absent_main; main blob `None`; head blob `ee943658f2e80a5403c385e04882411b68c0b78f`.
- `A docs/records/audits/2026-10-architecture-targets/README.md` — absent_main; main blob `None`; head blob `0488b9f17118c4d7387a79e3c936402a62ab5da2`.
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` — absent_main; main blob `None`; head blob `8f99d63a5bfc203d302335e8794a622bb4212b47`.
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` — absent_main; main blob `None`; head blob `04ed4828d5bf5f41bdb6777a0792ba1c3a2760b2`.
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` — absent_main; main blob `None`; head blob `662c74dd580c3c57ca5715698166d7e0b0cf7949`.
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` — absent_main; main blob `None`; head blob `4adf51b1d9c5d9d34994ed0d0cf313e91f58220f`.
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` — absent_main; main blob `None`; head blob `99b189edcef423743bb44b98e128a9cb937df411`.
- `A docs/records/audits/2026-10-governance-deadlock/README.md` — absent_main; main blob `None`; head blob `22de009b00cb748565251440f62148376a8b547d`.
- `A docs/records/audits/2026-10-governance-deadlock/after.json` — absent_main; main blob `None`; head blob `ce7f470d5096a693baf064dd90909c94f55b680d`.
- `A docs/records/audits/2026-10-governance-deadlock/before.json` — absent_main; main blob `None`; head blob `f9deb13d715f0a90cb70386450e9e5e0cd40d6cf`.
- `A docs/records/audits/2026-10-governance-deadlock/path-only.json` — absent_main; main blob `None`; head blob `e25a817da6841a6777a9567581a597f0a4496609`.
- `A docs/records/audits/2026-10-governance-deadlock/regressions.log` — absent_main; main blob `None`; head blob `d848087e1ffb18d0b8ac1923c21e763903bd9b62`.
- `A docs/records/audits/2026-10-governance-deadlock/target-regressions.log` — absent_main; main blob `None`; head blob `d3c52bb530b67e4fddb030185eeb8435ec63548f`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/README.md` — absent_main; main blob `None`; head blob `79069ec26c2c2f459af54a09d99633c9b8d6722b`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/all-root-tests.json` — absent_main; main blob `None`; head blob `d62b739710de309eb5b4c2f90e905abf3c9ad78c`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/authorization-replacement-before.json` — absent_main; main blob `None`; head blob `cdebb778f74c776241175485649b13130673236e`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/cmake-limits.json` — absent_main; main blob `None`; head blob `fae1c1566d9f669ec6d3a64e7746697292be6b8c`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/cmake-prefix-before.json` — absent_main; main blob `None`; head blob `02504f546dd51afacce2489cc60c8b6ef8a51652`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/cmake_boundary_probes.py` — absent_main; main blob `None`; head blob `ddea958d2b33d244a942dd93f2e0e343079497a6`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/corrected-defects.json` — absent_main; main blob `None`; head blob `104058716ec780620854fa5e1867e0d281964828`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/identities-and-preservation.json` — absent_main; main blob `None`; head blob `4eb244dcbfc5a2a00d65b559892bc107f401c47a`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/inventory-required.json` — absent_main; main blob `None`; head blob `883035947cafbe6bcfb00b28e9035af4c15d7337`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/inventory-standalone.json` — absent_main; main blob `None`; head blob `b50b961be0f97a98a45b9963308de154ee3176d1`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/nested-root-before.json` — absent_main; main blob `None`; head blob `c8055eea9dc75ef3df1a68146840b73562765c6a`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/raw-tree-defects-after.json` — absent_main; main blob `None`; head blob `5abde482d8720d0311735f1bc0dc4a22f13356e9`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/raw-tree-defects-before.json` — absent_main; main blob `None`; head blob `dfbbad8c911634474392a1e6589a82e63fde3893`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/regressions.json` — absent_main; main blob `None`; head blob `80532cacc903b9672a616ca7f4159d8aadfd04d2`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/reproduced-defects.json` — absent_main; main blob `None`; head blob `b5c9d33a9d28ca0092d08a7edaba711c5a1bde53`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/root-order-after.json` — absent_main; main blob `None`; head blob `ef64e4b7f3dbc1d291466de6eb6f8f55352339d8`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/root-order-before.json` — absent_main; main blob `None`; head blob `60a68ef6addaa01e47f186426e3603adf6ef6c23`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/trust-limits.json` — absent_main; main blob `None`; head blob `2fa6f566afa81644172479d29a53ce167a5be3d2`.
- `A docs/records/audits/2026-10-root-governance-adversarial-review/trust_boundary_probes.py` — absent_main; main blob `None`; head blob `f028d73e9df15e358660673821141dcc5f68ce16`.
- `A docs/records/audits/2026-10-root-governance-implementation/README.md` — absent_main; main blob `None`; head blob `2a70f5a470fcef260a42ccb228555b2452fcc0a1`.
- `A docs/records/audits/2026-10-root-governance-implementation/after-required.json` — absent_main; main blob `None`; head blob `c3c3e28fa5593818e0b68948d27fe9a09595cdd1`.
- `A docs/records/audits/2026-10-root-governance-implementation/after-standalone.json` — absent_main; main blob `None`; head blob `ed5b9a38055418a5a606208ac9db8a5e7467d81b`.
- `A docs/records/audits/2026-10-root-governance-implementation/authorization-comment.json` — absent_main; main blob `None`; head blob `4d263705fe9cf27cda7cf81529ce36e246c1540f`.
- `A docs/records/audits/2026-10-root-governance-implementation/before.json` — absent_main; main blob `None`; head blob `1436d2972b0b145d75c9659d91379cdf7850cccb`.
- `A docs/records/audits/2026-10-root-governance-implementation/failure-propagation-results.json` — absent_main; main blob `None`; head blob `15e394420236d6a30954c8269a89bdd2c6cff9ec`.
- `A docs/records/audits/2026-10-root-governance-implementation/fixture-fix-validation.json` — absent_main; main blob `None`; head blob `e0cf2dd856f3c8f6ac153de0ed16e3afa6c05013`.
- `A docs/records/audits/2026-10-root-governance-implementation/inventory.md` — absent_main; main blob `None`; head blob `5d17d8677ada619c7c74ef6c57cb8850b2f2a58d`.
- `A docs/records/audits/2026-10-root-governance-implementation/preservation-and-identities.json` — absent_main; main blob `None`; head blob `d8240f657bf91c1e94a196f2485b357797ec5f68`.
- `A docs/records/audits/2026-10-root-governance-implementation/regressions-before-fixture-fix.json` — absent_main; main blob `None`; head blob `1d8c2c529d1e0c22aaedac86f981fc26bacf1c06`.
- `A docs/records/audits/2026-10-root-governance-implementation/regressions.json` — absent_main; main blob `None`; head blob `b99bc29f42a2905dd9094d3b3f7480049bbcd593`.
- `A docs/records/audits/2026-10-root-governance-implementation/standalone-modes.json` — absent_main; main blob `None`; head blob `cf4d43ef4a0b19ae1c5ec91853965d4181074028`.
- `A docs/records/audits/2026-10-root-layout-authority/README.md` — absent_main; main blob `None`; head blob `b3bb178340630cdd09f224ddd48d142e0e5eae2a`.
- `A docs/records/audits/2026-10-root-layout-authority/authority-evidence.json` — absent_main; main blob `None`; head blob `b58fde8f8ec67762cd14a304f6a8f24d2c5d7418`.
- `A docs/records/audits/2026-10-root-layout-authority/baseline.json` — absent_main; main blob `None`; head blob `1a433d90c089ba1cb4ba31c8f9dbeb0c57db1d60`.
- `A docs/records/audits/2026-10-root-layout-authority/entry-history.json` — absent_main; main blob `None`; head blob `14aa6a7e697137597f102de13ada6cfe52f1bc5f`.
- `A docs/records/audits/2026-10-root-layout-authority/inventory.json` — absent_main; main blob `None`; head blob `996e3813a685fa4cfcd8356d225126fe4813f9f0`.
- `A docs/records/audits/2026-10-root-layout-authority/inventory.md` — absent_main; main blob `None`; head blob `579cf430291276c1ee2ee54b7e79c0839ad1cbcd`.
- `A docs/records/audits/2026-10-root-layout-authority/pr-heads.json` — absent_main; main blob `None`; head blob `cea6188ff5f8bd3fa3efca60b306c2816907d4a1`.
- `A docs/records/audits/2026-10-root-layout-contract/CONTRACT_DESIGN.md` — absent_main; main blob `None`; head blob `8d105fd53c6cdb24736199e8ab6223a2acf6fcc6`.
- `A docs/records/audits/2026-10-root-layout-contract/DECISIONS.md` — absent_main; main blob `None`; head blob `9a8e782eafddd38279d30689c5da76d96a8cb29f`.
- `A docs/records/audits/2026-10-root-layout-contract/ENTRY_MATRIX.md` — absent_main; main blob `None`; head blob `02268bb58f98d5c5fc9df5e70eb8f2d7a4a9a7e9`.
- `A docs/records/audits/2026-10-root-layout-contract/README.md` — absent_main; main blob `None`; head blob `48048c36b47ec334c331ae9e5945ca84b9959b74`.
- `A docs/records/audits/2026-10-root-layout-contract/REPRODUCE.md` — absent_main; main blob `None`; head blob `212d1a7de8daa024da783b2446d26f85c9326aea`.
- `A docs/records/audits/2026-10-root-layout-contract/TEST_PLAN.md` — absent_main; main blob `None`; head blob `c8e9717754fed221a5778290ae47c6a8366dd1aa`.
- `A docs/records/audits/2026-10-root-layout-contract/baseline.json` — absent_main; main blob `None`; head blob `16bab4a711e7dd4e2a4ae92738a0d3e4b5ed6384`.
- `A docs/records/audits/2026-10-root-layout-contract/failure-propagation-results.json` — absent_main; main blob `None`; head blob `c6f1af73db85895448eb28d257cbc25282477bee`.
- `A docs/records/audits/2026-10-root-layout-contract/hosted-reviews.json` — absent_main; main blob `None`; head blob `b49bef9b20e1ef2e7d52520e73778179aa1eac43`.
- `A docs/records/audits/2026-10-root-layout-contract/inventory.json` — absent_main; main blob `None`; head blob `be2d0bd24a9061e240ac22d66313d34e3e0a4146`.
- `A docs/records/audits/2026-10-root-layout-contract/inventory.md` — absent_main; main blob `None`; head blob `6f3d3626e63e2262fbc1229530672c7f6eae6976`.
- `A docs/records/audits/2026-10-root-layout-contract/lineage-and-authority.json` — absent_main; main blob `None`; head blob `19805f000dbe7e1448495ccb13cdc305e30e4ad4`.
- `A docs/records/audits/2026-10-root-layout-contract/post-check-status.json` — absent_main; main blob `None`; head blob `4ce331da5d4ebbc4372a440e83dd3e54dc97796f`.
- `A docs/records/audits/2026-10-root-layout-contract/pr-heads.json` — absent_main; main blob `None`; head blob `9d16e4d5deebba8f06782e502ea59b086e6a3b8e`.
- `A docs/records/audits/2026-10-root-layout-contract/pre-inventory-clean.json` — absent_main; main blob `None`; head blob `4ce331da5d4ebbc4372a440e83dd3e54dc97796f`.
- `A docs/records/audits/2026-10-root-layout-contract/proposal-verification.json` — absent_main; main blob `None`; head blob `3eacc826dfed08330139870f960499ebf89cf047`.
- `A docs/records/audits/2026-10-root-layout-contract/proposed-contract.json` — absent_main; main blob `None`; head blob `f3bb97b34d2804ce585d584386a9312237540402`.
- `A docs/records/audits/2026-10-root-layout-contract/proposed-contract.schema.json` — absent_main; main blob `None`; head blob `52d20d785923f7176fa9c47b645df2a1823d06dc`.
- `A docs/records/audits/2026-10-root-layout-contract/regressions.json` — absent_main; main blob `None`; head blob `2e9c9cc4afaad4212f5826a662ba2f7c38449f21`.
- `A docs/records/audits/2026-10-root-layout-contract/verify_proposal.py` — absent_main; main blob `None`; head blob `6e97bfb20fca323a0781c546f1c37a11cbc5ebc3`.
- `A docs/records/audits/2026-10-workflow-supply-chain/README.md` — absent_main; main blob `None`; head blob `abe1d31877fc34d5e99fe3d87049e6ce36576931`.
- `A docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json` — absent_main; main blob `None`; head blob `8e8b29e8ff8e087cee8da9633eee02785f04a8bc`.
- `A docs/records/audits/2026-10-workflow-supply-chain/before.json` — absent_main; main blob `None`; head blob `923172f37830624c5ae0e42cb3e9375bde57ffee`.
- `A docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json` — absent_main; main blob `None`; head blob `1ab55c151405fd8df56d17e043f07a8b742defe5`.
- `A docs/records/audits/2026-10-workflow-supply-chain/inventory.md` — absent_main; main blob `None`; head blob `2b807b26015f433e3da87b9e948d7543e12d6f1c`.
- `A docs/records/audits/2026-10-workflow-supply-chain/protection.json` — absent_main; main blob `None`; head blob `e6a5a86c8985fce13afee1342f85d94359fb95fa`.
- `A docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json` — absent_main; main blob `None`; head blob `2ccda18d08e33e27f314e9fe7dd76d8f6578305b`.
- `M scripts/ci/check_architecture_coherence.py` — different; main blob `0553a7959407e858b1bfd6457e465269de4ecdd7`; head blob `45f25390b0f51de4ea3604a30b076fb3a1288c96`.
- `M scripts/ci/check_architecture_targets.py` — different; main blob `3add84d9153eb32923c7e56d5abc65d094ad9d7f`; head blob `69a53706b9da99374b93810b88664a60b8b66ef2`.
- `A scripts/ci/test_failure_propagation.py` — absent_main; main blob `None`; head blob `ebb35c7710e09bcb895fdcde59633268655a52fa`.
- `M scripts/governance/check_root_structure.py` — different; main blob `c9991cc6c576a66c1eb8c21e0e0e43bd1484617b`; head blob `76271e9b4aff6388c6d10613b5e01a3e341731fb`.
- `A scripts/governance/check_root_structure_legacy.py` — absent_main; main blob `None`; head blob `f66410ef17f18d9ec5282923ac5dc04b182916bb`.
- `A spec/rfcs/RFC-0057-root-layout-authority-preservation-and-contract-approval.md` — absent_main; main blob `None`; head blob `1b49d24851cc789c4a4920ee38ca3071cc586e15`.
- `M spec/rfcs/index.md` — different; main blob `2c05638e9bc8db73d60a4d25c2fff1ebad3e7eda`; head blob `51a18ee9d76073cbaf349112d27cb22ea2897e13`.
- `A tests/ci/check_architecture_targets_test.py` — absent_main; main blob `None`; head blob `df1882eb3dbed0713affc323d4a443dfc064b21e`.
- `A tests/ci/check_root_structure_test.py` — different; main blob `9f3ae67ee04c781905a1f1b37fa4146405d382e1`; head blob `2cd35745568be202ab76ebbf9a38128ed49c1e58`.
- `A tests/ci/fixtures/workflow-supply-chain/permissions.json` — absent_main; main blob `None`; head blob `8ae3c7086a51c053f2a27ed57abbc29c79e7842d`.
- `A tests/ci/fixtures/workflow-supply-chain/pins.json` — absent_main; main blob `None`; head blob `bbe425537b65708a5228c177b88e33cac36381c8`.
- `A tests/ci/governance_deadlock_test.py` — absent_main; main blob `None`; head blob `11b7a967ddc33eabcacdab80172e2343876442df`.
- `A tests/ci/workflow_action_smoke.py` — absent_main; main blob `None`; head blob `773a543762e53894df44fbba958b317c339f75fa`.
- `A tests/ci/workflow_supply_chain_test.py` — absent_main; main blob `None`; head blob `28a407730513205659115108bd7b3453a7d341e3`.

Overlaps with other PR own deltas:

- #650: `spec/rfcs/index.md`
- #528: `scripts/governance/check_root_structure.py`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 579: INVESTIGATION ONLY — proposed root-layout contract pending maintainer decisions

- Head `71bb2c0e937dbcad73c9421e5465651b6e0d3ec3`; actual base `f4ffa85bf0bcaf028b51462113bf19c2eebb0091` (`corrective/root-layout-authority`); main merge-base `01c85019319fd020922402ba356878e392cb2231`.
- Purpose: Root-layout contract decision preparation. Decision: **DEFER**.
- Dependency assessment: Stacks on 578; parent of 580; no merged policy activation.
- Open heads included by ancestry: [574, 575, 576, 577, 578]; references in body (not necessarily dependencies): [569, 574, 578].
- Evidence/limits: 20 own paths, 63 including parents. Keep proposed classification/authority matrix separate from adopted permissions.
- Blob counts: {'different': 10, 'absent_main': 53}. Merge simulation exit 1; tree `428d3f3a70d4cb87141b9e423a6e7722e4ecf433`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...71bb2c0e937dbcad73c9421e5465651b6e0d3ec3).

Own changed files:

- `A docs/records/audits/2026-10-root-layout-contract/CONTRACT_DESIGN.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/CONTRACT_DESIGN.md))
- `A docs/records/audits/2026-10-root-layout-contract/DECISIONS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/DECISIONS.md))
- `A docs/records/audits/2026-10-root-layout-contract/ENTRY_MATRIX.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/ENTRY_MATRIX.md))
- `A docs/records/audits/2026-10-root-layout-contract/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/README.md))
- `A docs/records/audits/2026-10-root-layout-contract/REPRODUCE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/REPRODUCE.md))
- `A docs/records/audits/2026-10-root-layout-contract/TEST_PLAN.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/TEST_PLAN.md))
- `A docs/records/audits/2026-10-root-layout-contract/baseline.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/baseline.json))
- `A docs/records/audits/2026-10-root-layout-contract/failure-propagation-results.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/failure-propagation-results.json))
- `A docs/records/audits/2026-10-root-layout-contract/hosted-reviews.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/hosted-reviews.json))
- `A docs/records/audits/2026-10-root-layout-contract/inventory.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/inventory.json))
- `A docs/records/audits/2026-10-root-layout-contract/inventory.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/inventory.md))
- `A docs/records/audits/2026-10-root-layout-contract/lineage-and-authority.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/lineage-and-authority.json))
- `A docs/records/audits/2026-10-root-layout-contract/post-check-status.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/post-check-status.json))
- `A docs/records/audits/2026-10-root-layout-contract/pr-heads.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/pr-heads.json))
- `A docs/records/audits/2026-10-root-layout-contract/pre-inventory-clean.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/pre-inventory-clean.json))
- `A docs/records/audits/2026-10-root-layout-contract/proposal-verification.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/proposal-verification.json))
- `A docs/records/audits/2026-10-root-layout-contract/proposed-contract.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/proposed-contract.json))
- `A docs/records/audits/2026-10-root-layout-contract/proposed-contract.schema.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/proposed-contract.schema.json))
- `A docs/records/audits/2026-10-root-layout-contract/regressions.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/regressions.json))
- `A docs/records/audits/2026-10-root-layout-contract/verify_proposal.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/71bb2c0e937dbcad73c9421e5465651b6e0d3ec3/docs/records/audits/2026-10-root-layout-contract/verify_proposal.py))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `534c80263b70fb5ca14b2a375ac7d90585e2d480`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `ec07a16879a1813cf2b5ec7e0033c59799d79e6f`.
- `M .github/workflows/docker.yml` — different; main blob `c1218c1d6c96411f6893094866cc72e6ced488a6`; head blob `bc3bbc39382e2f36c6d1cda9bb08d2f8f35bacd8`.
- `M .github/workflows/python-wheels.yml` — different; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `e31ce23d4f616178c788e35a782b4f845b5fd191`.
- `M .github/workflows/qemu-boot-x86.yml` — different; main blob `bcad444157a39c3d86355298094746016a17e9cc`; head blob `34690e16f1d90dfd1d1512a02446a592465dd110`.
- `M .github/workflows/qemu-boot.yml` — different; main blob `873acefcfe4db9c5f09ceee798862e9a39ce5d68`; head blob `3ddab69d9aa403a6cb96d2e1026475d6b978dca4`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `a49327d6eaf5ac128d03013935db95ec9558d49f`.
- `M docs/explanation/ARCHITECTURE.md` — different; main blob `98d2ea3a56c36d7e0063818575c1e61bb1acc632`; head blob `c36ed9de36ee15c71755396715c08a3d3de1664a`.
- `A docs/records/audits/2026-10-architecture-targets/README.md` — absent_main; main blob `None`; head blob `0488b9f17118c4d7387a79e3c936402a62ab5da2`.
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` — absent_main; main blob `None`; head blob `8f99d63a5bfc203d302335e8794a622bb4212b47`.
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` — absent_main; main blob `None`; head blob `04ed4828d5bf5f41bdb6777a0792ba1c3a2760b2`.
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` — absent_main; main blob `None`; head blob `662c74dd580c3c57ca5715698166d7e0b0cf7949`.
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` — absent_main; main blob `None`; head blob `4adf51b1d9c5d9d34994ed0d0cf313e91f58220f`.
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` — absent_main; main blob `None`; head blob `99b189edcef423743bb44b98e128a9cb937df411`.
- `A docs/records/audits/2026-10-governance-deadlock/README.md` — absent_main; main blob `None`; head blob `22de009b00cb748565251440f62148376a8b547d`.
- `A docs/records/audits/2026-10-governance-deadlock/after.json` — absent_main; main blob `None`; head blob `ce7f470d5096a693baf064dd90909c94f55b680d`.
- `A docs/records/audits/2026-10-governance-deadlock/before.json` — absent_main; main blob `None`; head blob `f9deb13d715f0a90cb70386450e9e5e0cd40d6cf`.
- `A docs/records/audits/2026-10-governance-deadlock/path-only.json` — absent_main; main blob `None`; head blob `e25a817da6841a6777a9567581a597f0a4496609`.
- `A docs/records/audits/2026-10-governance-deadlock/regressions.log` — absent_main; main blob `None`; head blob `d848087e1ffb18d0b8ac1923c21e763903bd9b62`.
- `A docs/records/audits/2026-10-governance-deadlock/target-regressions.log` — absent_main; main blob `None`; head blob `d3c52bb530b67e4fddb030185eeb8435ec63548f`.
- `A docs/records/audits/2026-10-root-layout-authority/README.md` — absent_main; main blob `None`; head blob `b3bb178340630cdd09f224ddd48d142e0e5eae2a`.
- `A docs/records/audits/2026-10-root-layout-authority/authority-evidence.json` — absent_main; main blob `None`; head blob `b58fde8f8ec67762cd14a304f6a8f24d2c5d7418`.
- `A docs/records/audits/2026-10-root-layout-authority/baseline.json` — absent_main; main blob `None`; head blob `1a433d90c089ba1cb4ba31c8f9dbeb0c57db1d60`.
- `A docs/records/audits/2026-10-root-layout-authority/entry-history.json` — absent_main; main blob `None`; head blob `14aa6a7e697137597f102de13ada6cfe52f1bc5f`.
- `A docs/records/audits/2026-10-root-layout-authority/inventory.json` — absent_main; main blob `None`; head blob `996e3813a685fa4cfcd8356d225126fe4813f9f0`.
- `A docs/records/audits/2026-10-root-layout-authority/inventory.md` — absent_main; main blob `None`; head blob `579cf430291276c1ee2ee54b7e79c0839ad1cbcd`.
- `A docs/records/audits/2026-10-root-layout-authority/pr-heads.json` — absent_main; main blob `None`; head blob `cea6188ff5f8bd3fa3efca60b306c2816907d4a1`.
- `A docs/records/audits/2026-10-root-layout-contract/CONTRACT_DESIGN.md` — absent_main; main blob `None`; head blob `8d105fd53c6cdb24736199e8ab6223a2acf6fcc6`.
- `A docs/records/audits/2026-10-root-layout-contract/DECISIONS.md` — absent_main; main blob `None`; head blob `9a8e782eafddd38279d30689c5da76d96a8cb29f`.
- `A docs/records/audits/2026-10-root-layout-contract/ENTRY_MATRIX.md` — absent_main; main blob `None`; head blob `02268bb58f98d5c5fc9df5e70eb8f2d7a4a9a7e9`.
- `A docs/records/audits/2026-10-root-layout-contract/README.md` — absent_main; main blob `None`; head blob `48048c36b47ec334c331ae9e5945ca84b9959b74`.
- `A docs/records/audits/2026-10-root-layout-contract/REPRODUCE.md` — absent_main; main blob `None`; head blob `212d1a7de8daa024da783b2446d26f85c9326aea`.
- `A docs/records/audits/2026-10-root-layout-contract/TEST_PLAN.md` — absent_main; main blob `None`; head blob `c8e9717754fed221a5778290ae47c6a8366dd1aa`.
- `A docs/records/audits/2026-10-root-layout-contract/baseline.json` — absent_main; main blob `None`; head blob `16bab4a711e7dd4e2a4ae92738a0d3e4b5ed6384`.
- `A docs/records/audits/2026-10-root-layout-contract/failure-propagation-results.json` — absent_main; main blob `None`; head blob `c6f1af73db85895448eb28d257cbc25282477bee`.
- `A docs/records/audits/2026-10-root-layout-contract/hosted-reviews.json` — absent_main; main blob `None`; head blob `b49bef9b20e1ef2e7d52520e73778179aa1eac43`.
- `A docs/records/audits/2026-10-root-layout-contract/inventory.json` — absent_main; main blob `None`; head blob `be2d0bd24a9061e240ac22d66313d34e3e0a4146`.
- `A docs/records/audits/2026-10-root-layout-contract/inventory.md` — absent_main; main blob `None`; head blob `6f3d3626e63e2262fbc1229530672c7f6eae6976`.
- `A docs/records/audits/2026-10-root-layout-contract/lineage-and-authority.json` — absent_main; main blob `None`; head blob `19805f000dbe7e1448495ccb13cdc305e30e4ad4`.
- `A docs/records/audits/2026-10-root-layout-contract/post-check-status.json` — absent_main; main blob `None`; head blob `4ce331da5d4ebbc4372a440e83dd3e54dc97796f`.
- `A docs/records/audits/2026-10-root-layout-contract/pr-heads.json` — absent_main; main blob `None`; head blob `9d16e4d5deebba8f06782e502ea59b086e6a3b8e`.
- `A docs/records/audits/2026-10-root-layout-contract/pre-inventory-clean.json` — absent_main; main blob `None`; head blob `4ce331da5d4ebbc4372a440e83dd3e54dc97796f`.
- `A docs/records/audits/2026-10-root-layout-contract/proposal-verification.json` — absent_main; main blob `None`; head blob `3eacc826dfed08330139870f960499ebf89cf047`.
- `A docs/records/audits/2026-10-root-layout-contract/proposed-contract.json` — absent_main; main blob `None`; head blob `f3bb97b34d2804ce585d584386a9312237540402`.
- `A docs/records/audits/2026-10-root-layout-contract/proposed-contract.schema.json` — absent_main; main blob `None`; head blob `52d20d785923f7176fa9c47b645df2a1823d06dc`.
- `A docs/records/audits/2026-10-root-layout-contract/regressions.json` — absent_main; main blob `None`; head blob `2e9c9cc4afaad4212f5826a662ba2f7c38449f21`.
- `A docs/records/audits/2026-10-root-layout-contract/verify_proposal.py` — absent_main; main blob `None`; head blob `6e97bfb20fca323a0781c546f1c37a11cbc5ebc3`.
- `A docs/records/audits/2026-10-workflow-supply-chain/README.md` — absent_main; main blob `None`; head blob `abe1d31877fc34d5e99fe3d87049e6ce36576931`.
- `A docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json` — absent_main; main blob `None`; head blob `8e8b29e8ff8e087cee8da9633eee02785f04a8bc`.
- `A docs/records/audits/2026-10-workflow-supply-chain/before.json` — absent_main; main blob `None`; head blob `923172f37830624c5ae0e42cb3e9375bde57ffee`.
- `A docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json` — absent_main; main blob `None`; head blob `1ab55c151405fd8df56d17e043f07a8b742defe5`.
- `A docs/records/audits/2026-10-workflow-supply-chain/inventory.md` — absent_main; main blob `None`; head blob `2b807b26015f433e3da87b9e948d7543e12d6f1c`.
- `A docs/records/audits/2026-10-workflow-supply-chain/protection.json` — absent_main; main blob `None`; head blob `e6a5a86c8985fce13afee1342f85d94359fb95fa`.
- `A docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json` — absent_main; main blob `None`; head blob `2ccda18d08e33e27f314e9fe7dd76d8f6578305b`.
- `M scripts/ci/check_architecture_coherence.py` — different; main blob `0553a7959407e858b1bfd6457e465269de4ecdd7`; head blob `45f25390b0f51de4ea3604a30b076fb3a1288c96`.
- `M scripts/ci/check_architecture_targets.py` — different; main blob `3add84d9153eb32923c7e56d5abc65d094ad9d7f`; head blob `69a53706b9da99374b93810b88664a60b8b66ef2`.
- `A scripts/ci/test_failure_propagation.py` — absent_main; main blob `None`; head blob `ebb35c7710e09bcb895fdcde59633268655a52fa`.
- `A tests/ci/check_architecture_targets_test.py` — absent_main; main blob `None`; head blob `df1882eb3dbed0713affc323d4a443dfc064b21e`.
- `A tests/ci/fixtures/workflow-supply-chain/permissions.json` — absent_main; main blob `None`; head blob `8ae3c7086a51c053f2a27ed57abbc29c79e7842d`.
- `A tests/ci/fixtures/workflow-supply-chain/pins.json` — absent_main; main blob `None`; head blob `bbe425537b65708a5228c177b88e33cac36381c8`.
- `A tests/ci/governance_deadlock_test.py` — absent_main; main blob `None`; head blob `11b7a967ddc33eabcacdab80172e2343876442df`.
- `A tests/ci/workflow_action_smoke.py` — absent_main; main blob `None`; head blob `773a543762e53894df44fbba958b317c339f75fa`.
- `A tests/ci/workflow_supply_chain_test.py` — absent_main; main blob `None`; head blob `28a407730513205659115108bd7b3453a7d341e3`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 578: INVESTIGATION ONLY — NO GOVERNANCE CORRECTION: root-layout authority

- Head `f4ffa85bf0bcaf028b51462113bf19c2eebb0091`; actual base `b6a20706b3b37eebbfb5ab0293a88306a3dae29a` (`corrective/workflow-supply-chain`); main merge-base `01c85019319fd020922402ba356878e392cb2231`.
- Purpose: Root-layout authority investigation. Decision: **DEFER**.
- Dependency assessment: Stacks on 577; input to 579/580.
- Open heads included by ancestry: [574, 575, 576, 577]; references in body (not necessarily dependencies): [577].
- Evidence/limits: Seven own evidence paths, 43 including parents. Outcome is investigation/authority unresolved; retain negative records, not new root permissions.
- Blob counts: {'different': 10, 'absent_main': 33}. Merge simulation exit 1; tree `2cb963ae2bf0c2a0ef1170148bfdb072d5b74c37`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...f4ffa85bf0bcaf028b51462113bf19c2eebb0091).

Own changed files:

- `A docs/records/audits/2026-10-root-layout-authority/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f4ffa85bf0bcaf028b51462113bf19c2eebb0091/docs/records/audits/2026-10-root-layout-authority/README.md))
- `A docs/records/audits/2026-10-root-layout-authority/authority-evidence.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f4ffa85bf0bcaf028b51462113bf19c2eebb0091/docs/records/audits/2026-10-root-layout-authority/authority-evidence.json))
- `A docs/records/audits/2026-10-root-layout-authority/baseline.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f4ffa85bf0bcaf028b51462113bf19c2eebb0091/docs/records/audits/2026-10-root-layout-authority/baseline.json))
- `A docs/records/audits/2026-10-root-layout-authority/entry-history.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f4ffa85bf0bcaf028b51462113bf19c2eebb0091/docs/records/audits/2026-10-root-layout-authority/entry-history.json))
- `A docs/records/audits/2026-10-root-layout-authority/inventory.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f4ffa85bf0bcaf028b51462113bf19c2eebb0091/docs/records/audits/2026-10-root-layout-authority/inventory.json))
- `A docs/records/audits/2026-10-root-layout-authority/inventory.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f4ffa85bf0bcaf028b51462113bf19c2eebb0091/docs/records/audits/2026-10-root-layout-authority/inventory.md))
- `A docs/records/audits/2026-10-root-layout-authority/pr-heads.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/f4ffa85bf0bcaf028b51462113bf19c2eebb0091/docs/records/audits/2026-10-root-layout-authority/pr-heads.json))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `534c80263b70fb5ca14b2a375ac7d90585e2d480`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `ec07a16879a1813cf2b5ec7e0033c59799d79e6f`.
- `M .github/workflows/docker.yml` — different; main blob `c1218c1d6c96411f6893094866cc72e6ced488a6`; head blob `bc3bbc39382e2f36c6d1cda9bb08d2f8f35bacd8`.
- `M .github/workflows/python-wheels.yml` — different; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `e31ce23d4f616178c788e35a782b4f845b5fd191`.
- `M .github/workflows/qemu-boot-x86.yml` — different; main blob `bcad444157a39c3d86355298094746016a17e9cc`; head blob `34690e16f1d90dfd1d1512a02446a592465dd110`.
- `M .github/workflows/qemu-boot.yml` — different; main blob `873acefcfe4db9c5f09ceee798862e9a39ce5d68`; head blob `3ddab69d9aa403a6cb96d2e1026475d6b978dca4`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `a49327d6eaf5ac128d03013935db95ec9558d49f`.
- `M docs/explanation/ARCHITECTURE.md` — different; main blob `98d2ea3a56c36d7e0063818575c1e61bb1acc632`; head blob `c36ed9de36ee15c71755396715c08a3d3de1664a`.
- `A docs/records/audits/2026-10-architecture-targets/README.md` — absent_main; main blob `None`; head blob `0488b9f17118c4d7387a79e3c936402a62ab5da2`.
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` — absent_main; main blob `None`; head blob `8f99d63a5bfc203d302335e8794a622bb4212b47`.
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` — absent_main; main blob `None`; head blob `04ed4828d5bf5f41bdb6777a0792ba1c3a2760b2`.
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` — absent_main; main blob `None`; head blob `662c74dd580c3c57ca5715698166d7e0b0cf7949`.
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` — absent_main; main blob `None`; head blob `4adf51b1d9c5d9d34994ed0d0cf313e91f58220f`.
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` — absent_main; main blob `None`; head blob `99b189edcef423743bb44b98e128a9cb937df411`.
- `A docs/records/audits/2026-10-governance-deadlock/README.md` — absent_main; main blob `None`; head blob `22de009b00cb748565251440f62148376a8b547d`.
- `A docs/records/audits/2026-10-governance-deadlock/after.json` — absent_main; main blob `None`; head blob `ce7f470d5096a693baf064dd90909c94f55b680d`.
- `A docs/records/audits/2026-10-governance-deadlock/before.json` — absent_main; main blob `None`; head blob `f9deb13d715f0a90cb70386450e9e5e0cd40d6cf`.
- `A docs/records/audits/2026-10-governance-deadlock/path-only.json` — absent_main; main blob `None`; head blob `e25a817da6841a6777a9567581a597f0a4496609`.
- `A docs/records/audits/2026-10-governance-deadlock/regressions.log` — absent_main; main blob `None`; head blob `d848087e1ffb18d0b8ac1923c21e763903bd9b62`.
- `A docs/records/audits/2026-10-governance-deadlock/target-regressions.log` — absent_main; main blob `None`; head blob `d3c52bb530b67e4fddb030185eeb8435ec63548f`.
- `A docs/records/audits/2026-10-root-layout-authority/README.md` — absent_main; main blob `None`; head blob `b3bb178340630cdd09f224ddd48d142e0e5eae2a`.
- `A docs/records/audits/2026-10-root-layout-authority/authority-evidence.json` — absent_main; main blob `None`; head blob `b58fde8f8ec67762cd14a304f6a8f24d2c5d7418`.
- `A docs/records/audits/2026-10-root-layout-authority/baseline.json` — absent_main; main blob `None`; head blob `1a433d90c089ba1cb4ba31c8f9dbeb0c57db1d60`.
- `A docs/records/audits/2026-10-root-layout-authority/entry-history.json` — absent_main; main blob `None`; head blob `14aa6a7e697137597f102de13ada6cfe52f1bc5f`.
- `A docs/records/audits/2026-10-root-layout-authority/inventory.json` — absent_main; main blob `None`; head blob `996e3813a685fa4cfcd8356d225126fe4813f9f0`.
- `A docs/records/audits/2026-10-root-layout-authority/inventory.md` — absent_main; main blob `None`; head blob `579cf430291276c1ee2ee54b7e79c0839ad1cbcd`.
- `A docs/records/audits/2026-10-root-layout-authority/pr-heads.json` — absent_main; main blob `None`; head blob `cea6188ff5f8bd3fa3efca60b306c2816907d4a1`.
- `A docs/records/audits/2026-10-workflow-supply-chain/README.md` — absent_main; main blob `None`; head blob `abe1d31877fc34d5e99fe3d87049e6ce36576931`.
- `A docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json` — absent_main; main blob `None`; head blob `8e8b29e8ff8e087cee8da9633eee02785f04a8bc`.
- `A docs/records/audits/2026-10-workflow-supply-chain/before.json` — absent_main; main blob `None`; head blob `923172f37830624c5ae0e42cb3e9375bde57ffee`.
- `A docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json` — absent_main; main blob `None`; head blob `1ab55c151405fd8df56d17e043f07a8b742defe5`.
- `A docs/records/audits/2026-10-workflow-supply-chain/inventory.md` — absent_main; main blob `None`; head blob `2b807b26015f433e3da87b9e948d7543e12d6f1c`.
- `A docs/records/audits/2026-10-workflow-supply-chain/protection.json` — absent_main; main blob `None`; head blob `e6a5a86c8985fce13afee1342f85d94359fb95fa`.
- `A docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json` — absent_main; main blob `None`; head blob `2ccda18d08e33e27f314e9fe7dd76d8f6578305b`.
- `M scripts/ci/check_architecture_coherence.py` — different; main blob `0553a7959407e858b1bfd6457e465269de4ecdd7`; head blob `45f25390b0f51de4ea3604a30b076fb3a1288c96`.
- `M scripts/ci/check_architecture_targets.py` — different; main blob `3add84d9153eb32923c7e56d5abc65d094ad9d7f`; head blob `69a53706b9da99374b93810b88664a60b8b66ef2`.
- `A scripts/ci/test_failure_propagation.py` — absent_main; main blob `None`; head blob `ebb35c7710e09bcb895fdcde59633268655a52fa`.
- `A tests/ci/check_architecture_targets_test.py` — absent_main; main blob `None`; head blob `df1882eb3dbed0713affc323d4a443dfc064b21e`.
- `A tests/ci/fixtures/workflow-supply-chain/permissions.json` — absent_main; main blob `None`; head blob `8ae3c7086a51c053f2a27ed57abbc29c79e7842d`.
- `A tests/ci/fixtures/workflow-supply-chain/pins.json` — absent_main; main blob `None`; head blob `bbe425537b65708a5228c177b88e33cac36381c8`.
- `A tests/ci/governance_deadlock_test.py` — absent_main; main blob `None`; head blob `11b7a967ddc33eabcacdab80172e2343876442df`.
- `A tests/ci/workflow_action_smoke.py` — absent_main; main blob `None`; head blob `773a543762e53894df44fbba958b317c339f75fa`.
- `A tests/ci/workflow_supply_chain_test.py` — absent_main; main blob `None`; head blob `28a407730513205659115108bd7b3453a7d341e3`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 577: ci: pin workflow actions and restrict x86 workflow token permissions

- Head `b6a20706b3b37eebbfb5ab0293a88306a3dae29a`; actual base `41fa126cc82df6ed54b095d8dd67bfe066252066` (`corrective/governance-deadlock`); main merge-base `01c85019319fd020922402ba356878e392cb2231`.
- Purpose: Pinned workflow actions and least token permissions. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Stacks on 576; parent of 578 and base of 617; overlaps dependency bumps.
- Open heads included by ancestry: [574, 575, 576]; references in body (not necessarily dependencies): [574, 576].
- Evidence/limits: Sixteen own paths; all overlap consolidated 649, but no whole-patch identity. Compare current pinned workflow behavior and unique receipt identities before any duplicate recommendation.
- Blob counts: {'different': 10, 'absent_main': 26}. Merge simulation exit 1; tree `62ef009148a861b2ca453a05e391dc6264f17ba1`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 3 pass, 2 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b6a20706b3b37eebbfb5ab0293a88306a3dae29a).

Own changed files:

- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/.github/workflows/ci.yml))
- `M .github/workflows/docker.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/.github/workflows/docker.yml))
- `M .github/workflows/python-wheels.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/.github/workflows/python-wheels.yml))
- `M .github/workflows/qemu-boot-x86.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/.github/workflows/qemu-boot-x86.yml))
- `M .github/workflows/qemu-boot.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/.github/workflows/qemu-boot.yml))
- `A docs/records/audits/2026-10-workflow-supply-chain/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/docs/records/audits/2026-10-workflow-supply-chain/README.md))
- `A docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json))
- `A docs/records/audits/2026-10-workflow-supply-chain/before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/docs/records/audits/2026-10-workflow-supply-chain/before.json))
- `A docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json))
- `A docs/records/audits/2026-10-workflow-supply-chain/inventory.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/docs/records/audits/2026-10-workflow-supply-chain/inventory.md))
- `A docs/records/audits/2026-10-workflow-supply-chain/protection.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/docs/records/audits/2026-10-workflow-supply-chain/protection.json))
- `A docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json))
- `A tests/ci/fixtures/workflow-supply-chain/permissions.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/tests/ci/fixtures/workflow-supply-chain/permissions.json))
- `A tests/ci/fixtures/workflow-supply-chain/pins.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/tests/ci/fixtures/workflow-supply-chain/pins.json))
- `A tests/ci/workflow_action_smoke.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/tests/ci/workflow_action_smoke.py))
- `A tests/ci/workflow_supply_chain_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b6a20706b3b37eebbfb5ab0293a88306a3dae29a/tests/ci/workflow_supply_chain_test.py))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `534c80263b70fb5ca14b2a375ac7d90585e2d480`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `ec07a16879a1813cf2b5ec7e0033c59799d79e6f`.
- `M .github/workflows/docker.yml` — different; main blob `c1218c1d6c96411f6893094866cc72e6ced488a6`; head blob `bc3bbc39382e2f36c6d1cda9bb08d2f8f35bacd8`.
- `M .github/workflows/python-wheels.yml` — different; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `e31ce23d4f616178c788e35a782b4f845b5fd191`.
- `M .github/workflows/qemu-boot-x86.yml` — different; main blob `bcad444157a39c3d86355298094746016a17e9cc`; head blob `34690e16f1d90dfd1d1512a02446a592465dd110`.
- `M .github/workflows/qemu-boot.yml` — different; main blob `873acefcfe4db9c5f09ceee798862e9a39ce5d68`; head blob `3ddab69d9aa403a6cb96d2e1026475d6b978dca4`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `a49327d6eaf5ac128d03013935db95ec9558d49f`.
- `M docs/explanation/ARCHITECTURE.md` — different; main blob `98d2ea3a56c36d7e0063818575c1e61bb1acc632`; head blob `c36ed9de36ee15c71755396715c08a3d3de1664a`.
- `A docs/records/audits/2026-10-architecture-targets/README.md` — absent_main; main blob `None`; head blob `0488b9f17118c4d7387a79e3c936402a62ab5da2`.
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` — absent_main; main blob `None`; head blob `8f99d63a5bfc203d302335e8794a622bb4212b47`.
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` — absent_main; main blob `None`; head blob `04ed4828d5bf5f41bdb6777a0792ba1c3a2760b2`.
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` — absent_main; main blob `None`; head blob `662c74dd580c3c57ca5715698166d7e0b0cf7949`.
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` — absent_main; main blob `None`; head blob `4adf51b1d9c5d9d34994ed0d0cf313e91f58220f`.
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` — absent_main; main blob `None`; head blob `99b189edcef423743bb44b98e128a9cb937df411`.
- `A docs/records/audits/2026-10-governance-deadlock/README.md` — absent_main; main blob `None`; head blob `22de009b00cb748565251440f62148376a8b547d`.
- `A docs/records/audits/2026-10-governance-deadlock/after.json` — absent_main; main blob `None`; head blob `ce7f470d5096a693baf064dd90909c94f55b680d`.
- `A docs/records/audits/2026-10-governance-deadlock/before.json` — absent_main; main blob `None`; head blob `f9deb13d715f0a90cb70386450e9e5e0cd40d6cf`.
- `A docs/records/audits/2026-10-governance-deadlock/path-only.json` — absent_main; main blob `None`; head blob `e25a817da6841a6777a9567581a597f0a4496609`.
- `A docs/records/audits/2026-10-governance-deadlock/regressions.log` — absent_main; main blob `None`; head blob `d848087e1ffb18d0b8ac1923c21e763903bd9b62`.
- `A docs/records/audits/2026-10-governance-deadlock/target-regressions.log` — absent_main; main blob `None`; head blob `d3c52bb530b67e4fddb030185eeb8435ec63548f`.
- `A docs/records/audits/2026-10-workflow-supply-chain/README.md` — absent_main; main blob `None`; head blob `abe1d31877fc34d5e99fe3d87049e6ce36576931`.
- `A docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json` — absent_main; main blob `None`; head blob `8e8b29e8ff8e087cee8da9633eee02785f04a8bc`.
- `A docs/records/audits/2026-10-workflow-supply-chain/before.json` — absent_main; main blob `None`; head blob `923172f37830624c5ae0e42cb3e9375bde57ffee`.
- `A docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json` — absent_main; main blob `None`; head blob `1ab55c151405fd8df56d17e043f07a8b742defe5`.
- `A docs/records/audits/2026-10-workflow-supply-chain/inventory.md` — absent_main; main blob `None`; head blob `2b807b26015f433e3da87b9e948d7543e12d6f1c`.
- `A docs/records/audits/2026-10-workflow-supply-chain/protection.json` — absent_main; main blob `None`; head blob `e6a5a86c8985fce13afee1342f85d94359fb95fa`.
- `A docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json` — absent_main; main blob `None`; head blob `2ccda18d08e33e27f314e9fe7dd76d8f6578305b`.
- `M scripts/ci/check_architecture_coherence.py` — different; main blob `0553a7959407e858b1bfd6457e465269de4ecdd7`; head blob `45f25390b0f51de4ea3604a30b076fb3a1288c96`.
- `M scripts/ci/check_architecture_targets.py` — different; main blob `3add84d9153eb32923c7e56d5abc65d094ad9d7f`; head blob `69a53706b9da99374b93810b88664a60b8b66ef2`.
- `A scripts/ci/test_failure_propagation.py` — absent_main; main blob `None`; head blob `ebb35c7710e09bcb895fdcde59633268655a52fa`.
- `A tests/ci/check_architecture_targets_test.py` — absent_main; main blob `None`; head blob `df1882eb3dbed0713affc323d4a443dfc064b21e`.
- `A tests/ci/fixtures/workflow-supply-chain/permissions.json` — absent_main; main blob `None`; head blob `8ae3c7086a51c053f2a27ed57abbc29c79e7842d`.
- `A tests/ci/fixtures/workflow-supply-chain/pins.json` — absent_main; main blob `None`; head blob `bbe425537b65708a5228c177b88e33cac36381c8`.
- `A tests/ci/governance_deadlock_test.py` — absent_main; main blob `None`; head blob `11b7a967ddc33eabcacdab80172e2343876442df`.
- `A tests/ci/workflow_action_smoke.py` — absent_main; main blob `None`; head blob `773a543762e53894df44fbba958b317c339f75fa`.
- `A tests/ci/workflow_supply_chain_test.py` — absent_main; main blob `None`; head blob `28a407730513205659115108bd7b3453a7d341e3`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`
- #747: `.github/workflows/ci.yml`, `.github/workflows/python-wheels.yml`
- #649: `.github/workflows/ci.yml`, `.github/workflows/docker.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`, `docs/records/audits/2026-10-workflow-supply-chain/README.md`, `docs/records/audits/2026-10-workflow-supply-chain/after-and-tests.json`, `docs/records/audits/2026-10-workflow-supply-chain/before.json`, `docs/records/audits/2026-10-workflow-supply-chain/buildx-advisory.json`, `docs/records/audits/2026-10-workflow-supply-chain/inventory.md`, `docs/records/audits/2026-10-workflow-supply-chain/protection.json`, `docs/records/audits/2026-10-workflow-supply-chain/upstream-verification.json`, `tests/ci/fixtures/workflow-supply-chain/permissions.json`, `tests/ci/fixtures/workflow-supply-chain/pins.json`, `tests/ci/workflow_action_smoke.py`, `tests/ci/workflow_supply_chain_test.py`
- #576: `.github/workflows/ci.yml`
- #575: `.github/workflows/ci.yml`
- #574: `.github/workflows/ci.yml`
- #525: `.github/workflows/ci.yml`, `.github/workflows/docker.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`
- #514: `.github/workflows/ci.yml`
- #513: `.github/workflows/ci.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`
- #504: `.github/workflows/ci.yml`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40276394512/job/121420231605): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/40276394512/job/121419947446): completed / failure.

## PR 576: ci: restore architecture coherence paths and README determinism boundary

- Head `41fa126cc82df6ed54b095d8dd67bfe066252066`; actual base `b06f8520d02fd68301a5ae06777beb03b73c8eb5` (`corrective/architecture-target-discovery`); main merge-base `01c85019319fd020922402ba356878e392cb2231`.
- Purpose: Architecture coherence and README boundary correction. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Stacks on 575; parent of 577; source not proven duplicate.
- Open heads included by ancestry: [574, 575]; references in body (not necessarily dependencies): [574, 575].
- Evidence/limits: Ten own paths, all overlapping 649. Current main has substantial later narrative/workflow edits; identify surviving correction hunks instead of merging old context.
- Blob counts: {'different': 6, 'absent_main': 15}. Merge simulation exit 1; tree `34d0a09989ce075ea72fc0f48ec5f1b769e24d83`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 1 pass, 2 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...41fa126cc82df6ed54b095d8dd67bfe066252066).

Own changed files:

- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/.github/workflows/ci.yml))
- `M README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/README.md))
- `A docs/records/audits/2026-10-governance-deadlock/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/docs/records/audits/2026-10-governance-deadlock/README.md))
- `A docs/records/audits/2026-10-governance-deadlock/after.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/docs/records/audits/2026-10-governance-deadlock/after.json))
- `A docs/records/audits/2026-10-governance-deadlock/before.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/docs/records/audits/2026-10-governance-deadlock/before.json))
- `A docs/records/audits/2026-10-governance-deadlock/path-only.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/docs/records/audits/2026-10-governance-deadlock/path-only.json))
- `A docs/records/audits/2026-10-governance-deadlock/regressions.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/docs/records/audits/2026-10-governance-deadlock/regressions.log))
- `A docs/records/audits/2026-10-governance-deadlock/target-regressions.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/docs/records/audits/2026-10-governance-deadlock/target-regressions.log))
- `M scripts/ci/check_architecture_coherence.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/scripts/ci/check_architecture_coherence.py))
- `A tests/ci/governance_deadlock_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/41fa126cc82df6ed54b095d8dd67bfe066252066/tests/ci/governance_deadlock_test.py))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `534c80263b70fb5ca14b2a375ac7d90585e2d480`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `54cceaec33e7bb0dfc66600283538491883afd9f`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `a49327d6eaf5ac128d03013935db95ec9558d49f`.
- `M docs/explanation/ARCHITECTURE.md` — different; main blob `98d2ea3a56c36d7e0063818575c1e61bb1acc632`; head blob `c36ed9de36ee15c71755396715c08a3d3de1664a`.
- `A docs/records/audits/2026-10-architecture-targets/README.md` — absent_main; main blob `None`; head blob `0488b9f17118c4d7387a79e3c936402a62ab5da2`.
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` — absent_main; main blob `None`; head blob `8f99d63a5bfc203d302335e8794a622bb4212b47`.
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` — absent_main; main blob `None`; head blob `04ed4828d5bf5f41bdb6777a0792ba1c3a2760b2`.
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` — absent_main; main blob `None`; head blob `662c74dd580c3c57ca5715698166d7e0b0cf7949`.
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` — absent_main; main blob `None`; head blob `4adf51b1d9c5d9d34994ed0d0cf313e91f58220f`.
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` — absent_main; main blob `None`; head blob `99b189edcef423743bb44b98e128a9cb937df411`.
- `A docs/records/audits/2026-10-governance-deadlock/README.md` — absent_main; main blob `None`; head blob `22de009b00cb748565251440f62148376a8b547d`.
- `A docs/records/audits/2026-10-governance-deadlock/after.json` — absent_main; main blob `None`; head blob `ce7f470d5096a693baf064dd90909c94f55b680d`.
- `A docs/records/audits/2026-10-governance-deadlock/before.json` — absent_main; main blob `None`; head blob `f9deb13d715f0a90cb70386450e9e5e0cd40d6cf`.
- `A docs/records/audits/2026-10-governance-deadlock/path-only.json` — absent_main; main blob `None`; head blob `e25a817da6841a6777a9567581a597f0a4496609`.
- `A docs/records/audits/2026-10-governance-deadlock/regressions.log` — absent_main; main blob `None`; head blob `d848087e1ffb18d0b8ac1923c21e763903bd9b62`.
- `A docs/records/audits/2026-10-governance-deadlock/target-regressions.log` — absent_main; main blob `None`; head blob `d3c52bb530b67e4fddb030185eeb8435ec63548f`.
- `M scripts/ci/check_architecture_coherence.py` — different; main blob `0553a7959407e858b1bfd6457e465269de4ecdd7`; head blob `45f25390b0f51de4ea3604a30b076fb3a1288c96`.
- `M scripts/ci/check_architecture_targets.py` — different; main blob `3add84d9153eb32923c7e56d5abc65d094ad9d7f`; head blob `69a53706b9da99374b93810b88664a60b8b66ef2`.
- `A scripts/ci/test_failure_propagation.py` — absent_main; main blob `None`; head blob `ebb35c7710e09bcb895fdcde59633268655a52fa`.
- `A tests/ci/check_architecture_targets_test.py` — absent_main; main blob `None`; head blob `df1882eb3dbed0713affc323d4a443dfc064b21e`.
- `A tests/ci/governance_deadlock_test.py` — absent_main; main blob `None`; head blob `11b7a967ddc33eabcacdab80172e2343876442df`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`
- #747: `.github/workflows/ci.yml`
- #649: `.github/workflows/ci.yml`, `README.md`, `docs/records/audits/2026-10-governance-deadlock/README.md`, `docs/records/audits/2026-10-governance-deadlock/after.json`, `docs/records/audits/2026-10-governance-deadlock/before.json`, `docs/records/audits/2026-10-governance-deadlock/path-only.json`, `docs/records/audits/2026-10-governance-deadlock/regressions.log`, `docs/records/audits/2026-10-governance-deadlock/target-regressions.log`, `scripts/ci/check_architecture_coherence.py`, `tests/ci/governance_deadlock_test.py`
- #577: `.github/workflows/ci.yml`
- #575: `.github/workflows/ci.yml`
- #574: `.github/workflows/ci.yml`
- #525: `.github/workflows/ci.yml`
- #514: `.github/workflows/ci.yml`
- #513: `.github/workflows/ci.yml`
- #508: `README.md`
- #507: `README.md`
- #504: `.github/workflows/ci.yml`, `README.md`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40269537756/job/121399438511): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/40269537756/job/121399148643): completed / failure.

## PR 575: ci: reconcile architecture target discovery with delegated CMake sources

- Head `b06f8520d02fd68301a5ae06777beb03b73c8eb5`; actual base `10df367102151b087ee64b1fbb737c471f4c87fb` (`corrective/ci-failure-propagation`); main merge-base `01c85019319fd020922402ba356878e392cb2231`.
- Purpose: Delegated CMake target discovery correction. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Stacks on 574; related architecture validator in 774 but distinct purpose.
- Open heads included by ancestry: [574]; references in body (not necessarily dependencies): [536, 556, 560, 572, 574].
- Evidence/limits: Seven own paths, all overlapping 649. Preserve raw target inventory and compare validator behavior under current delegated source layout.
- Blob counts: {'different': 4, 'absent_main': 8}. Merge simulation exit 0; tree `7b721f6fa354e8b70306599c1416ba734055c958`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 3 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b06f8520d02fd68301a5ae06777beb03b73c8eb5).

Own changed files:

- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b06f8520d02fd68301a5ae06777beb03b73c8eb5/.github/workflows/ci.yml))
- `M docs/explanation/ARCHITECTURE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b06f8520d02fd68301a5ae06777beb03b73c8eb5/docs/explanation/ARCHITECTURE.md))
- `A docs/records/audits/2026-10-architecture-targets/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b06f8520d02fd68301a5ae06777beb03b73c8eb5/docs/records/audits/2026-10-architecture-targets/README.md))
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b06f8520d02fd68301a5ae06777beb03b73c8eb5/docs/records/audits/2026-10-architecture-targets/evidence.tar.gz))
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b06f8520d02fd68301a5ae06777beb03b73c8eb5/docs/records/audits/2026-10-architecture-targets/inventory.json))
- `M scripts/ci/check_architecture_targets.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b06f8520d02fd68301a5ae06777beb03b73c8eb5/scripts/ci/check_architecture_targets.py))
- `A tests/ci/check_architecture_targets_test.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b06f8520d02fd68301a5ae06777beb03b73c8eb5/tests/ci/check_architecture_targets_test.py))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `534c80263b70fb5ca14b2a375ac7d90585e2d480`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `ebd0738c07e343da78023596d4112640fd7746c4`.
- `M docs/explanation/ARCHITECTURE.md` — different; main blob `98d2ea3a56c36d7e0063818575c1e61bb1acc632`; head blob `c36ed9de36ee15c71755396715c08a3d3de1664a`.
- `A docs/records/audits/2026-10-architecture-targets/README.md` — absent_main; main blob `None`; head blob `0488b9f17118c4d7387a79e3c936402a62ab5da2`.
- `A docs/records/audits/2026-10-architecture-targets/evidence.tar.gz` — absent_main; main blob `None`; head blob `8f99d63a5bfc203d302335e8794a622bb4212b47`.
- `A docs/records/audits/2026-10-architecture-targets/inventory.json` — absent_main; main blob `None`; head blob `04ed4828d5bf5f41bdb6777a0792ba1c3a2760b2`.
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` — absent_main; main blob `None`; head blob `662c74dd580c3c57ca5715698166d7e0b0cf7949`.
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` — absent_main; main blob `None`; head blob `4adf51b1d9c5d9d34994ed0d0cf313e91f58220f`.
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` — absent_main; main blob `None`; head blob `99b189edcef423743bb44b98e128a9cb937df411`.
- `M scripts/ci/check_architecture_targets.py` — different; main blob `3add84d9153eb32923c7e56d5abc65d094ad9d7f`; head blob `69a53706b9da99374b93810b88664a60b8b66ef2`.
- `A scripts/ci/test_failure_propagation.py` — absent_main; main blob `None`; head blob `ebb35c7710e09bcb895fdcde59633268655a52fa`.
- `A tests/ci/check_architecture_targets_test.py` — absent_main; main blob `None`; head blob `df1882eb3dbed0713affc323d4a443dfc064b21e`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`
- #747: `.github/workflows/ci.yml`
- #649: `.github/workflows/ci.yml`, `docs/explanation/ARCHITECTURE.md`, `docs/records/audits/2026-10-architecture-targets/README.md`, `docs/records/audits/2026-10-architecture-targets/evidence.tar.gz`, `docs/records/audits/2026-10-architecture-targets/inventory.json`, `scripts/ci/check_architecture_targets.py`, `tests/ci/check_architecture_targets_test.py`
- #577: `.github/workflows/ci.yml`
- #576: `.github/workflows/ci.yml`
- #574: `.github/workflows/ci.yml`
- #525: `.github/workflows/ci.yml`
- #514: `.github/workflows/ci.yml`
- #513: `.github/workflows/ci.yml`
- #504: `.github/workflows/ci.yml`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40267221776/job/121392399256): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/40267221776/job/121392143387): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/40267221776/job/121392143167): completed / failure.

## PR 574: ci: report validator failures through logged pipelines

- Head `10df367102151b087ee64b1fbb737c471f4c87fb`; actual base `01c85019319fd020922402ba356878e392cb2231` (`main`); main merge-base `01c85019319fd020922402ba356878e392cb2231`.
- Purpose: Logged-pipeline failure propagation. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Base of 575-580/617 stack; newer workflow edits also occur in 747/782/dependency PRs.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Six own paths, all overlapping 649. Original failure was reported reproduced; establish which producer/tee hunks remain absent on current main before selecting canonical correction.
- Blob counts: {'different': 2, 'absent_main': 4}. Merge simulation exit 0; tree `3e2455ef82331ed4e0e292b3e1c92fa5f6c53c18`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 7 pass, 3 fail, 23 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...10df367102151b087ee64b1fbb737c471f4c87fb).

Own changed files:

- `M .github/workflows/ai-experiments-ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/10df367102151b087ee64b1fbb737c471f4c87fb/.github/workflows/ai-experiments-ci.yml))
- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/10df367102151b087ee64b1fbb737c471f4c87fb/.github/workflows/ci.yml))
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/10df367102151b087ee64b1fbb737c471f4c87fb/docs/records/audits/2026-10-ci-failure-propagation/README.md))
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/10df367102151b087ee64b1fbb737c471f4c87fb/docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz))
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/10df367102151b087ee64b1fbb737c471f4c87fb/docs/records/audits/2026-10-ci-failure-propagation/results.json))
- `A scripts/ci/test_failure_propagation.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/10df367102151b087ee64b1fbb737c471f4c87fb/scripts/ci/test_failure_propagation.py))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `534c80263b70fb5ca14b2a375ac7d90585e2d480`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `ef5125e6720ea0315f10946848ef0ed3bd509453`.
- `A docs/records/audits/2026-10-ci-failure-propagation/README.md` — absent_main; main blob `None`; head blob `662c74dd580c3c57ca5715698166d7e0b0cf7949`.
- `A docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz` — absent_main; main blob `None`; head blob `4adf51b1d9c5d9d34994ed0d0cf313e91f58220f`.
- `A docs/records/audits/2026-10-ci-failure-propagation/results.json` — absent_main; main blob `None`; head blob `99b189edcef423743bb44b98e128a9cb937df411`.
- `A scripts/ci/test_failure_propagation.py` — absent_main; main blob `None`; head blob `ebb35c7710e09bcb895fdcde59633268655a52fa`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`
- #747: `.github/workflows/ci.yml`
- #649: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`, `docs/records/audits/2026-10-ci-failure-propagation/README.md`, `docs/records/audits/2026-10-ci-failure-propagation/evidence.tar.gz`, `docs/records/audits/2026-10-ci-failure-propagation/results.json`, `scripts/ci/test_failure_propagation.py`
- #577: `.github/workflows/ci.yml`
- #576: `.github/workflows/ci.yml`
- #575: `.github/workflows/ci.yml`
- #525: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`
- #514: `.github/workflows/ci.yml`
- #513: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`
- #504: `.github/workflows/ci.yml`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40261760705/job/121375681471): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/40261760705/job/121375453720): completed / failure.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/40261760705/job/121375453246): completed / failure.

## PR 573: research: test model-object authority with independent MCO-R01 oracle

- Head `b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11`; actual base `01c85019319fd020922402ba356878e392cb2231` (`main`); main merge-base `01c85019319fd020922402ba356878e392cb2231`.
- Purpose: Independent model-object authority oracle apparatus. Decision: **DEFER**.
- Dependency assessment: Research apparatus; distinct from durable-witness 557/560 and production boundary fixes.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [572].
- Evidence/limits: 55 unique paths. Finite in-memory protected-effect model is not production policy qualification or implementation advantage; independent protocol/evidence review precedes execution.
- Blob counts: {'absent_main': 55}. Merge simulation exit 0; tree `812ad1d3206c4181806c17acae16aee79b35c524`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 17 pass, 12 fail, 8 skip, 3 canceled; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11).

Own changed files:

- `A docs/research/model-computational-object/.gitattributes` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/.gitattributes))
- `A docs/research/model-computational-object/ANALYZABILITY.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/ANALYZABILITY.md))
- `A docs/research/model-computational-object/AUDIT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/AUDIT.md))
- `A docs/research/model-computational-object/CORPUS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/CORPUS.md))
- `A docs/research/model-computational-object/DECISION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/DECISION.md))
- `A docs/research/model-computational-object/EXPERIMENT_PROPOSAL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/EXPERIMENT_PROPOSAL.md))
- `A docs/research/model-computational-object/FORMAL_MODEL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/FORMAL_MODEL.md))
- `A docs/research/model-computational-object/HANDOFF.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/HANDOFF.md))
- `A docs/research/model-computational-object/MUTATION_EXPECTATIONS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/MUTATION_EXPECTATIONS.md))
- `A docs/research/model-computational-object/ORACLE_SPEC.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/ORACLE_SPEC.md))
- `A docs/research/model-computational-object/PRIOR_ART.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/PRIOR_ART.md))
- `A docs/research/model-computational-object/RESEARCH_QUESTION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/RESEARCH_QUESTION.md))
- `A docs/research/model-computational-object/THREAT_MODEL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/THREAT_MODEL.md))
- `A docs/research/model-computational-object/corpus/CASE_INDEX.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/corpus/CASE_INDEX.md))
- `A docs/research/model-computational-object/corpus/cases.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/corpus/cases.jsonl))
- `A docs/research/model-computational-object/corpus/event-permutations.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/corpus/event-permutations.jsonl))
- `A docs/research/model-computational-object/corpus/guard-assignments.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/corpus/guard-assignments.jsonl))
- `A docs/research/model-computational-object/corpus/objects.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/corpus/objects.json))
- `A docs/research/model-computational-object/review/check_packet.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/review/check_packet.py))
- `A docs/research/model-computational-object/review/make_corpus.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/review/make_corpus.py))
- `A docs/research/model-computational-object/review/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/review/manifest.json))
- `A docs/research/model-computational-object/review/request-schema-probes.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/review/request-schema-probes.json))
- `A docs/research/model-computational-object/review/requirements.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/review/requirements.txt))
- `A docs/research/model-computational-object/review/response-schema-probes.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/review/response-schema-probes.json))
- `A docs/research/model-computational-object/review/validation.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/review/validation.json))
- `A docs/research/model-computational-object/schemas/request.cd7d4620.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/schemas/request.cd7d4620.json))
- `A docs/research/model-computational-object/schemas/request.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/schemas/request.json))
- `A docs/research/model-computational-object/schemas/response.cd7d4620.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/schemas/response.cd7d4620.json))
- `A docs/research/model-computational-object/schemas/response.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/docs/research/model-computational-object/schemas/response.json))
- `A experiments/mco_r01/DISCREPANCIES.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/DISCREPANCIES.md))
- `A experiments/mco_r01/HANDOFF.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/HANDOFF.md))
- `A experiments/mco_r01/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/README.md))
- `A experiments/mco_r01/RESULTS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/RESULTS.md))
- `A experiments/mco_r01/arm_a.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/arm_a.py))
- `A experiments/mco_r01/arm_b.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/arm_b.py))
- `A experiments/mco_r01/driver.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/driver.py))
- `A experiments/mco_r01/evidence/baseline-ci.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/baseline-ci.tar.gz))
- `A experiments/mco_r01/evidence/baseline-pr-status.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/baseline-pr-status.json))
- `A experiments/mco_r01/evidence/conformance-final.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/conformance-final.tar.gz))
- `A experiments/mco_r01/evidence/conformance-first.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/conformance-first.tar.gz))
- `A experiments/mco_r01/evidence/final-commands.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/final-commands.json))
- `A experiments/mco_r01/evidence/final-proposal-trace.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/final-proposal-trace.json))
- `A experiments/mco_r01/evidence/final-results.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/final-results.json))
- `A experiments/mco_r01/evidence/first-commands.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/first-commands.json))
- `A experiments/mco_r01/evidence/first-proposal-trace.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/first-proposal-trace.json))
- `A experiments/mco_r01/evidence/first-results.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/first-results.json))
- `A experiments/mco_r01/evidence/initial-plan-projection-failure.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/initial-plan-projection-failure.json))
- `A experiments/mco_r01/evidence/local-checks.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/local-checks.json))
- `A experiments/mco_r01/evidence/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/manifest.json))
- `A experiments/mco_r01/evidence/oracle-first-phase.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/oracle-first-phase.json))
- `A experiments/mco_r01/evidence/oracle-first.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/oracle-first.json))
- `A experiments/mco_r01/evidence/oracle-first.rb` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/evidence/oracle-first.rb))
- `A experiments/mco_r01/oracle.rb` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/oracle.rb))
- `A experiments/mco_r01/run.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/run.py))
- `A experiments/mco_r01/wire.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b2a1a43452bfbb89403b5b4cb62ab83c0c0dbf11/experiments/mco_r01/wire.py))

All changed files versus main merge-base (including parents):

- `A docs/research/model-computational-object/.gitattributes` — absent_main; main blob `None`; head blob `fcadb2cf97913f58a2523f535336e725c6b59d1f`.
- `A docs/research/model-computational-object/ANALYZABILITY.md` — absent_main; main blob `None`; head blob `224314aa58b456d838c708cca54b848eeca22d18`.
- `A docs/research/model-computational-object/AUDIT.md` — absent_main; main blob `None`; head blob `c775f83e49754ed8b50332bbe9ffac25721fe1fd`.
- `A docs/research/model-computational-object/CORPUS.md` — absent_main; main blob `None`; head blob `4f0ae6331f72da215bbf5699854ec6de52d9b892`.
- `A docs/research/model-computational-object/DECISION.md` — absent_main; main blob `None`; head blob `e789847322b3241939f8197f6aec4ca318aea7fd`.
- `A docs/research/model-computational-object/EXPERIMENT_PROPOSAL.md` — absent_main; main blob `None`; head blob `9298f95b074337acb1db1c2200f8a51c9d757eeb`.
- `A docs/research/model-computational-object/FORMAL_MODEL.md` — absent_main; main blob `None`; head blob `0e77c54d5f6a7f1b7ed9082b1deec9e7ed6c7527`.
- `A docs/research/model-computational-object/HANDOFF.md` — absent_main; main blob `None`; head blob `990174b4d21f54875f94c6c44cb3183ba87a52b4`.
- `A docs/research/model-computational-object/MUTATION_EXPECTATIONS.md` — absent_main; main blob `None`; head blob `d42b1579db509e5b29e81ac6742b61c76826c7b6`.
- `A docs/research/model-computational-object/ORACLE_SPEC.md` — absent_main; main blob `None`; head blob `0745cf159485f244d60f777c2a689ba06849c7a0`.
- `A docs/research/model-computational-object/PRIOR_ART.md` — absent_main; main blob `None`; head blob `f501434e5b43829791fcf96733557534cc910966`.
- `A docs/research/model-computational-object/RESEARCH_QUESTION.md` — absent_main; main blob `None`; head blob `fdceda388e658d18db77a47ed61aed07e508f0da`.
- `A docs/research/model-computational-object/THREAT_MODEL.md` — absent_main; main blob `None`; head blob `d6635a1783207ac54270c5e1232be992ac11bab0`.
- `A docs/research/model-computational-object/corpus/CASE_INDEX.md` — absent_main; main blob `None`; head blob `f8bc622ee810aaf8b59a9eb6376a841e910c2a36`.
- `A docs/research/model-computational-object/corpus/cases.jsonl` — absent_main; main blob `None`; head blob `9056a373a07d877b759e33e4d74bef80a2cb041b`.
- `A docs/research/model-computational-object/corpus/event-permutations.jsonl` — absent_main; main blob `None`; head blob `8b6e97c995b618f9d6a2b99f4871f81cef652e83`.
- `A docs/research/model-computational-object/corpus/guard-assignments.jsonl` — absent_main; main blob `None`; head blob `cfcb5c11493638b81f691f8b293b1862585a668a`.
- `A docs/research/model-computational-object/corpus/objects.json` — absent_main; main blob `None`; head blob `8c9c9f01e4fcf5d46d964832fa0f3046e47be836`.
- `A docs/research/model-computational-object/review/check_packet.py` — absent_main; main blob `None`; head blob `ae80acb939703b4374e1e265935b2665ecc0306b`.
- `A docs/research/model-computational-object/review/make_corpus.py` — absent_main; main blob `None`; head blob `0ef522171f97c9bfaa58549ac685c230f4c618c0`.
- `A docs/research/model-computational-object/review/manifest.json` — absent_main; main blob `None`; head blob `f1d8e363903dbbde7f2fc02a4aec5926e74f5fa0`.
- `A docs/research/model-computational-object/review/request-schema-probes.json` — absent_main; main blob `None`; head blob `6c21fabf4cd1f7c21c51f9e06da380f42d287223`.
- `A docs/research/model-computational-object/review/requirements.txt` — absent_main; main blob `None`; head blob `db107d890268f20fd565a432d286150652af8245`.
- `A docs/research/model-computational-object/review/response-schema-probes.json` — absent_main; main blob `None`; head blob `6c21fabf4cd1f7c21c51f9e06da380f42d287223`.
- `A docs/research/model-computational-object/review/validation.json` — absent_main; main blob `None`; head blob `66ea3d6a04d5d551a90215b0660964bb944fd8f0`.
- `A docs/research/model-computational-object/schemas/request.cd7d4620.json` — absent_main; main blob `None`; head blob `2ac242d4f1747c3c0308b6632e80f9f841eb8408`.
- `A docs/research/model-computational-object/schemas/request.json` — absent_main; main blob `None`; head blob `262b0ab7361a80a11b6ee41fc831161972d5cfb0`.
- `A docs/research/model-computational-object/schemas/response.cd7d4620.json` — absent_main; main blob `None`; head blob `278634ff6f82532c591cb2339a0e7d43b49642a2`.
- `A docs/research/model-computational-object/schemas/response.json` — absent_main; main blob `None`; head blob `caee5c69c30391eed2f515e287e4b362d09ac0d7`.
- `A experiments/mco_r01/DISCREPANCIES.md` — absent_main; main blob `None`; head blob `6c66dd9e9aff9265966283d8c788ef817ea065e9`.
- `A experiments/mco_r01/HANDOFF.md` — absent_main; main blob `None`; head blob `8db5fa24e1f257ef144ea2d245e5f35aa6ae500f`.
- `A experiments/mco_r01/README.md` — absent_main; main blob `None`; head blob `e2611525aced3756594ec7c88dd335ca4df5e627`.
- `A experiments/mco_r01/RESULTS.md` — absent_main; main blob `None`; head blob `c59cd91f2fe3bfeee6f824918fa9b2bc7e864e83`.
- `A experiments/mco_r01/arm_a.py` — absent_main; main blob `None`; head blob `8533a791433517ec36c762cebc1c76afbd97ab1b`.
- `A experiments/mco_r01/arm_b.py` — absent_main; main blob `None`; head blob `f65631f96258d945aecff09bd20451aee15af446`.
- `A experiments/mco_r01/driver.py` — absent_main; main blob `None`; head blob `2093fdeb78a50b1b5365f7d140614e7018d3bba3`.
- `A experiments/mco_r01/evidence/baseline-ci.tar.gz` — absent_main; main blob `None`; head blob `ce445acafa817764fa05b07f087b0269b66d1fa5`.
- `A experiments/mco_r01/evidence/baseline-pr-status.json` — absent_main; main blob `None`; head blob `8d46b8aa52b3b93ea03214850908edb5c420d17c`.
- `A experiments/mco_r01/evidence/conformance-final.tar.gz` — absent_main; main blob `None`; head blob `ea29b66738743ab15db7dd531506639f7a53e300`.
- `A experiments/mco_r01/evidence/conformance-first.tar.gz` — absent_main; main blob `None`; head blob `0b771676192ffac19ddfa14a61db828a3f613476`.
- `A experiments/mco_r01/evidence/final-commands.json` — absent_main; main blob `None`; head blob `08c5041df43e8aaa4f5ac954b714ca75dd6b992f`.
- `A experiments/mco_r01/evidence/final-proposal-trace.json` — absent_main; main blob `None`; head blob `8f3b3d01a7a97482d5b0e6fbc5882faade7a2841`.
- `A experiments/mco_r01/evidence/final-results.json` — absent_main; main blob `None`; head blob `0cf038d3d9ec37b07abc0f88c269644be83ac1e4`.
- `A experiments/mco_r01/evidence/first-commands.json` — absent_main; main blob `None`; head blob `cd4d4362b091c15a02643d8674d7d54f95e75f29`.
- `A experiments/mco_r01/evidence/first-proposal-trace.json` — absent_main; main blob `None`; head blob `b4e34687cbadb64fcbe61c1c35efe2ee3b8ea511`.
- `A experiments/mco_r01/evidence/first-results.json` — absent_main; main blob `None`; head blob `c83dbaf9c28424f5e42aa040fca2c57ce962f480`.
- `A experiments/mco_r01/evidence/initial-plan-projection-failure.json` — absent_main; main blob `None`; head blob `165f716fb27ce2ebc92e71ddd4b0fa0886974921`.
- `A experiments/mco_r01/evidence/local-checks.json` — absent_main; main blob `None`; head blob `62bad99c1e5ba5ffca62b1d36e34b04584407e4e`.
- `A experiments/mco_r01/evidence/manifest.json` — absent_main; main blob `None`; head blob `3b6403ab015d3081b02a5a50854815be82e87347`.
- `A experiments/mco_r01/evidence/oracle-first-phase.json` — absent_main; main blob `None`; head blob `474d4e3928d84b671e952720a0e3a2f3b37bcc5e`.
- `A experiments/mco_r01/evidence/oracle-first.json` — absent_main; main blob `None`; head blob `b485312b9617b941c6dd17fa53639ed0ea38d35b`.
- `A experiments/mco_r01/evidence/oracle-first.rb` — absent_main; main blob `None`; head blob `9e65f244e22308aee055fb83454d26cadc230a25`.
- `A experiments/mco_r01/oracle.rb` — absent_main; main blob `None`; head blob `dbfad5cb24ad0b2f80a7f671fc868bf426cd2f5e`.
- `A experiments/mco_r01/run.py` — absent_main; main blob `None`; head blob `96c3261fac858326a4f29202dae8bc0afc9c6813`.
- `A experiments/mco_r01/wire.py` — absent_main; main blob `None`; head blob `f47b20a6bca295735b849506849ef8a6b79b7371`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121372738549): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121363037113): completed / failure.
- [build / linux-x86_64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121363037112): completed / failure.
- [build / linux-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121363037110): completed / failure.
- [build / macos-x86_64 / gcc](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121363037108): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121363037061): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121363037060): completed / failure.
- [build / macos-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121363037053): completed / failure.
- [gate / tritwise-determinism / avx2-asan](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121361648234): completed / failure.
- [build / sanitizers](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121361648221): completed / cancelled.
- [cxx-std / linux-x86_64 / clang / ON](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121361648180): completed / failure.
- [cxx-std / linux-x86_64 / clang / OFF](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121361648165): completed / failure.
- [fuzzing / frontend](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121361648146): completed / cancelled.
- [static analysis / clang-tidy](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121361648079): completed / cancelled.
- [governance-metrics](https://github.com/t81dev/t81-foundation/actions/runs/40257176366/job/121361415008): completed / failure.

## PR 560: Resolve PR #559 Review Findings and Establish Trustworthy Experimental Results

- Head `056f0d6307f59049ef9dfa42c482f727a02c2b96`; actual base `66b7905b98820816caf0c04bfbe938ea006e48e8` (`main`); main merge-base `66b7905b98820816caf0c04bfbe938ea006e48e8`.
- Purpose: Durable-witness experiment corrective patch. Decision: **REPAIR**.
- Dependency assessment: Depends on architecture-challenge arm; related 554/557 experimental authority lineage.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [559].
- Evidence/limits: Source still raises on Arm A structured policy denial and collision test uses :1:v2 rather than a real old-format collision. Repair denial evidence and regression sensitivity before result acceptance.
- Blob counts: {'different': 8}. Merge simulation exit 1; tree `ad0ef3be0d79143f7f4ffc06eee16b92c4198fae`; equals main: False.
- Review: 1 COMMENTED; threads 2 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 1 pass, 2 fail, 1 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...056f0d6307f59049ef9dfa42c482f727a02c2b96).

Own changed files:

- `M experiments/durable_witness/RESULTS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/056f0d6307f59049ef9dfa42c482f727a02c2b96/experiments/durable_witness/RESULTS.md))
- `M experiments/durable_witness/THREAT_MODEL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/056f0d6307f59049ef9dfa42c482f727a02c2b96/experiments/durable_witness/THREAT_MODEL.md))
- `M experiments/durable_witness/attacks.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/056f0d6307f59049ef9dfa42c482f727a02c2b96/experiments/durable_witness/attacks.py))
- `M experiments/durable_witness/run.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/056f0d6307f59049ef9dfa42c482f727a02c2b96/experiments/durable_witness/run.py))
- `M experiments/durable_witness/runner.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/056f0d6307f59049ef9dfa42c482f727a02c2b96/experiments/durable_witness/runner.py))
- `M experiments/durable_witness/test_durable_witness.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/056f0d6307f59049ef9dfa42c482f727a02c2b96/experiments/durable_witness/test_durable_witness.py))
- `M experiments/durable_witness/verify.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/056f0d6307f59049ef9dfa42c482f727a02c2b96/experiments/durable_witness/verify.py))
- `M experiments/durable_witness/witness.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/056f0d6307f59049ef9dfa42c482f727a02c2b96/experiments/durable_witness/witness.py))

All changed files versus main merge-base (including parents):

- `M experiments/durable_witness/RESULTS.md` — different; main blob `5df9c2ebcec15249515373626a1d1b177ce3eb52`; head blob `c3780adbc93529257d140977669e1d3e421fff8d`.
- `M experiments/durable_witness/THREAT_MODEL.md` — different; main blob `02592b10c29523794edf95b724e0cc7b9c4a6598`; head blob `77e7f3f43b9f4c9f1075479ccdfbccb02ec6906b`.
- `M experiments/durable_witness/attacks.py` — different; main blob `ef727784cc944e1a9a779ecde86b3d972125366d`; head blob `65dbdd66541a98938caf1f1f09a1caeeef57bf26`.
- `M experiments/durable_witness/run.py` — different; main blob `2f9b84790f36454908c3a20e99356b683c07c992`; head blob `72b73fc0ddb625380c23050271f28546d29bc90c`.
- `M experiments/durable_witness/runner.py` — different; main blob `bc46ffe5534124f93a0fd49918d1949b6ca96744`; head blob `87b36fada15bc5a5d8d98dad84d4f82e1e9ea4da`.
- `M experiments/durable_witness/test_durable_witness.py` — different; main blob `5eb09048696fddfa5f6ce979a85fc337a4475cc7`; head blob `c50ec6a750486ca906289ed9ee3c1020a53cb25e`.
- `M experiments/durable_witness/verify.py` — different; main blob `9dca2339b3590aff5c2461d203910ed15ffa635f`; head blob `15af852c75ff05f2b06e499902d9175fb26a2597`.
- `M experiments/durable_witness/witness.py` — different; main blob `d3a1a7d12adfe68a97263fb95670541c11543949`; head blob `94e0211e53e2324ceceefb72f684d739d22bfc0f`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `359363d72c650e6e958c99de2399267e888438f0` / [review](https://github.com/t81dev/t81-foundation/pull/560#pullrequestreview-5637853563).

Failed or incomplete checks:

- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/40086623439/job/120850776039): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/40086623439/job/120850775914): completed / failure.

## PR 557: experiment: durable authority, witnessed effects and restart replay resistance

- Head `aaa3970af36bf187782f3a5c38bd07931cc081ce`; actual base `0005ab675d2729b2795200e8c78ded43d4520b0d` (`main`); main merge-base `0005ab675d2729b2795200e8c78ded43d4520b0d`.
- Purpose: Transactional witness/receipt authority experiment. Decision: **DEFER**.
- Dependency assessment: Related 554/560 but distinct named protected effect and witness model; not duplicate.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [536, 553, 554].
- Evidence/limits: 44 unique apparatus/evidence paths. Review bounded SQLite transaction/receipt consumption and authority assumptions before any separately authorized reproduction.
- Blob counts: {'absent_main': 44}. Merge simulation exit 0; tree `e76121e16154da451292abe7b5df78a6bca0dd74`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 4 pass, 2 fail, 1 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...aaa3970af36bf187782f3a5c38bd07931cc081ce).

Own changed files:

- `A .github/workflows/accountable-transactions.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/.github/workflows/accountable-transactions.yml))
- `A experiments/accountable_transactions/CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/CMakeLists.txt))
- `A experiments/accountable_transactions/PROTOCOL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/PROTOCOL.md))
- `A experiments/accountable_transactions/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/README.md))
- `A experiments/accountable_transactions/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/REPORT.md))
- `A experiments/accountable_transactions/THREAT_MODEL.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/THREAT_MODEL.md))
- `A experiments/accountable_transactions/arm_a.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/arm_a.cpp))
- `A experiments/accountable_transactions/arm_b.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/arm_b.py))
- `A experiments/accountable_transactions/audit_public.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/audit_public.py))
- `A experiments/accountable_transactions/measure.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/measure.py))
- `A experiments/accountable_transactions/observed-macos-arm64/build.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/build.log))
- `A experiments/accountable_transactions/observed-macos-arm64/configure.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/configure.log))
- `A experiments/accountable_transactions/observed-macos-arm64/detached-import-guard.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/detached-import-guard.py))
- `A experiments/accountable_transactions/observed-macos-arm64/detached-verification.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/detached-verification.json))
- `A experiments/accountable_transactions/observed-macos-arm64/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/manifest.json))
- `A experiments/accountable_transactions/observed-macos-arm64/measurements.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/measurements.json))
- `A experiments/accountable_transactions/observed-macos-arm64/public-tests.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/public-tests.tar.gz))
- `A experiments/accountable_transactions/observed-macos-arm64/rebuild.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/rebuild.log))
- `A experiments/accountable_transactions/observed-macos-arm64/tests-raw.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/tests-raw.json))
- `A experiments/accountable_transactions/observed-macos-arm64/tests-results.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-macos-arm64/tests-results.json))
- `A experiments/accountable_transactions/observed-native/github-artifacts.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-native/github-artifacts.json))
- `A experiments/accountable_transactions/observed-native/independent-audit.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-native/independent-audit.json))
- `A experiments/accountable_transactions/observed-native/linux.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-native/linux.tar.gz))
- `A experiments/accountable_transactions/observed-native/macos.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-native/macos.tar.gz))
- `A experiments/accountable_transactions/observed-native/run.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-native/run.json))
- `A experiments/accountable_transactions/observed-native/sha256.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-native/sha256.json))
- `A experiments/accountable_transactions/observed-native/source-byte-comparison-negative-result.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-native/source-byte-comparison-negative-result.json))
- `A experiments/accountable_transactions/observed-native/windows.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-native/windows.tar.gz))
- `A experiments/accountable_transactions/observed-receipt-audit/existing-production-ci-failure.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-receipt-audit/existing-production-ci-failure.json))
- `A experiments/accountable_transactions/observed-receipt-audit/independent-audit.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-receipt-audit/independent-audit.json))
- `A experiments/accountable_transactions/observed-receipt-audit/manifest.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-receipt-audit/manifest.json))
- `A experiments/accountable_transactions/observed-receipt-audit/measurements.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-receipt-audit/measurements.json))
- `A experiments/accountable_transactions/observed-receipt-audit/missing-effect-attack.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-receipt-audit/missing-effect-attack.json))
- `A experiments/accountable_transactions/observed-receipt-audit/public-evidence.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-receipt-audit/public-evidence.tar.gz))
- `A experiments/accountable_transactions/observed-receipt-audit/shared-authority-cross-arm.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-receipt-audit/shared-authority-cross-arm.json))
- `A experiments/accountable_transactions/observed-receipt-audit/test-raw.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-receipt-audit/test-raw.json))
- `A experiments/accountable_transactions/observed-receipt-audit/test-results.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/observed-receipt-audit/test-results.json))
- `A experiments/accountable_transactions/preserve.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/preserve.py))
- `A experiments/accountable_transactions/reference-evidence/archive-audit.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/reference-evidence/archive-audit.json))
- `A experiments/accountable_transactions/reference-evidence/attacks.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/reference-evidence/attacks.json))
- `A experiments/accountable_transactions/requirements.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/requirements.txt))
- `A experiments/accountable_transactions/test_protocol.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/test_protocol.py))
- `A experiments/accountable_transactions/verify.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/verify.py))
- `A experiments/accountable_transactions/witness.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/aaa3970af36bf187782f3a5c38bd07931cc081ce/experiments/accountable_transactions/witness.py))

All changed files versus main merge-base (including parents):

- `A .github/workflows/accountable-transactions.yml` — absent_main; main blob `None`; head blob `cab68a438dbf781ae5b902aa4133c597740f9ee5`.
- `A experiments/accountable_transactions/CMakeLists.txt` — absent_main; main blob `None`; head blob `e2b3ea3cfb86cc05391d8bea5d753e427f9a1ad8`.
- `A experiments/accountable_transactions/PROTOCOL.md` — absent_main; main blob `None`; head blob `de35065a8728c439935ce5ff6bff2f266cfdfaa8`.
- `A experiments/accountable_transactions/README.md` — absent_main; main blob `None`; head blob `7134a40380148bbd44e7c33931ab337ec92f59f7`.
- `A experiments/accountable_transactions/REPORT.md` — absent_main; main blob `None`; head blob `90f4e16e592924acac2383cedf5a3c680c689184`.
- `A experiments/accountable_transactions/THREAT_MODEL.md` — absent_main; main blob `None`; head blob `41b9985b8e5980f2c9ebdd015bda25218853c186`.
- `A experiments/accountable_transactions/arm_a.cpp` — absent_main; main blob `None`; head blob `0a73441f959dcd4e8117617688178c16b4dcf5c2`.
- `A experiments/accountable_transactions/arm_b.py` — absent_main; main blob `None`; head blob `0c2e4231c0e7a9247a2a071b262b050ef29f7da6`.
- `A experiments/accountable_transactions/audit_public.py` — absent_main; main blob `None`; head blob `15493db564de57dcce61a7cfd069bab57e18109f`.
- `A experiments/accountable_transactions/measure.py` — absent_main; main blob `None`; head blob `a00d2f04c485b75c5dda014962445f5d5590b525`.
- `A experiments/accountable_transactions/observed-macos-arm64/build.log` — absent_main; main blob `None`; head blob `bf57cc62545fc4cad3ed1e3bc86fe71288425f7d`.
- `A experiments/accountable_transactions/observed-macos-arm64/configure.log` — absent_main; main blob `None`; head blob `50536dd534c9f347962b9ab29ce264ad22905b90`.
- `A experiments/accountable_transactions/observed-macos-arm64/detached-import-guard.py` — absent_main; main blob `None`; head blob `1521060ec028a7573df9e1cd4f10b2528520143e`.
- `A experiments/accountable_transactions/observed-macos-arm64/detached-verification.json` — absent_main; main blob `None`; head blob `e686e3821a64eff65ccdc36148963d7297956e6b`.
- `A experiments/accountable_transactions/observed-macos-arm64/manifest.json` — absent_main; main blob `None`; head blob `9977f16a3b08de33c5db2506cccd3ebc24acbaa9`.
- `A experiments/accountable_transactions/observed-macos-arm64/measurements.json` — absent_main; main blob `None`; head blob `dfbd17fd13d48523f7cd831bd483755d78bbe688`.
- `A experiments/accountable_transactions/observed-macos-arm64/public-tests.tar.gz` — absent_main; main blob `None`; head blob `6c5a56148bef853d116d6332dee6ef29c8657b21`.
- `A experiments/accountable_transactions/observed-macos-arm64/rebuild.log` — absent_main; main blob `None`; head blob `5437cdd11464e51413fd0ed22f2edee7036db67a`.
- `A experiments/accountable_transactions/observed-macos-arm64/tests-raw.json` — absent_main; main blob `None`; head blob `43db76ebf9dcabc2a228ebdd19c0be70aaf39f54`.
- `A experiments/accountable_transactions/observed-macos-arm64/tests-results.json` — absent_main; main blob `None`; head blob `421dd411a140cb3186f21a11ae1ab4f40d0fc053`.
- `A experiments/accountable_transactions/observed-native/github-artifacts.json` — absent_main; main blob `None`; head blob `3351a08b2385221db58c9d6ce08d0714cbd55580`.
- `A experiments/accountable_transactions/observed-native/independent-audit.json` — absent_main; main blob `None`; head blob `083747cee59b6de2ba6a7297599ede2ec918ab71`.
- `A experiments/accountable_transactions/observed-native/linux.tar.gz` — absent_main; main blob `None`; head blob `e15e4b40e453f1d5c725922bd11f28bbdc8ad76e`.
- `A experiments/accountable_transactions/observed-native/macos.tar.gz` — absent_main; main blob `None`; head blob `00d982c51366a328089db2e616bbc68038002cde`.
- `A experiments/accountable_transactions/observed-native/run.json` — absent_main; main blob `None`; head blob `a4a45ad4256f916900c9fa628de7bc961d4240e5`.
- `A experiments/accountable_transactions/observed-native/sha256.json` — absent_main; main blob `None`; head blob `dd7ac943a0fa850f709550bdeb0ebc328337493a`.
- `A experiments/accountable_transactions/observed-native/source-byte-comparison-negative-result.json` — absent_main; main blob `None`; head blob `7ed1058def10f222211f63bad6d97d57a2da592c`.
- `A experiments/accountable_transactions/observed-native/windows.tar.gz` — absent_main; main blob `None`; head blob `2f58fc00952f8256a5afe43fcb0e79a6ae593054`.
- `A experiments/accountable_transactions/observed-receipt-audit/existing-production-ci-failure.json` — absent_main; main blob `None`; head blob `7e279dd2bdbfd4754cbcc458e959a8eaf0321c06`.
- `A experiments/accountable_transactions/observed-receipt-audit/independent-audit.json` — absent_main; main blob `None`; head blob `312b7df0961ac06d078bd33748858100aba34f59`.
- `A experiments/accountable_transactions/observed-receipt-audit/manifest.json` — absent_main; main blob `None`; head blob `052e13962c946561369512254ced14762845110c`.
- `A experiments/accountable_transactions/observed-receipt-audit/measurements.json` — absent_main; main blob `None`; head blob `e51dac20b201739088e7fe8528574cd3f1dca2c7`.
- `A experiments/accountable_transactions/observed-receipt-audit/missing-effect-attack.json` — absent_main; main blob `None`; head blob `9c88a68c8eb603ab06d55875b79c1eb18094a09f`.
- `A experiments/accountable_transactions/observed-receipt-audit/public-evidence.tar.gz` — absent_main; main blob `None`; head blob `9fba87c9251273a914e4bbd2b6cdb99c51db0a53`.
- `A experiments/accountable_transactions/observed-receipt-audit/shared-authority-cross-arm.json` — absent_main; main blob `None`; head blob `c2bfa369f5bd28619edcd3b7ca92214773eaa8d2`.
- `A experiments/accountable_transactions/observed-receipt-audit/test-raw.json` — absent_main; main blob `None`; head blob `4e2c15a104ea8058228721b735d52aec18e12061`.
- `A experiments/accountable_transactions/observed-receipt-audit/test-results.json` — absent_main; main blob `None`; head blob `63b54a64032d7d162118722216b7d26e2966a037`.
- `A experiments/accountable_transactions/preserve.py` — absent_main; main blob `None`; head blob `344f5f2ec932faf25235c96e9ac6117436355d5d`.
- `A experiments/accountable_transactions/reference-evidence/archive-audit.json` — absent_main; main blob `None`; head blob `0dd6089cfee8abd7805cc2c7971e09548c880015`.
- `A experiments/accountable_transactions/reference-evidence/attacks.json` — absent_main; main blob `None`; head blob `b3351a1803808083a71446009c95b3b41c947075`.
- `A experiments/accountable_transactions/requirements.txt` — absent_main; main blob `None`; head blob `2446800f922bebd7bbec129b0d91bd15608cf639`.
- `A experiments/accountable_transactions/test_protocol.py` — absent_main; main blob `None`; head blob `22b584896e0c604b46a1d7c82bf7af4b8077ee4e`.
- `A experiments/accountable_transactions/verify.py` — absent_main; main blob `None`; head blob `d531849c101ec98ac8c0f21d450def6233655744`.
- `A experiments/accountable_transactions/witness.py` — absent_main; main blob `None`; head blob `68f00e6ea725328e8149b4f27b55c30debac6553`.

Overlaps with other PR own deltas:

- None.

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/40049808237/job/120739755058): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/40049808237/job/120739754910): completed / failure.

## PR 556: feat(canonfs,ci): portable storage mapping (RFC-00D6), SMT invariant proofs, and determinism CI fix

- Head `92e63d89b6eacdb5696d12b2da52593a8c2e526f`; actual base `872ac1b4c8a303fc5417d0b21551a430d0a50200` (`main`); main merge-base `872ac1b4c8a303fc5417d0b21551a430d0a50200`.
- Purpose: CanonFS portable mapping, SMT and CI patch. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Overlaps 748 storage boundary concept, 782 portability and workflow candidates; no duplicate proven.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [536, 554].
- Evidence/limits: 21 paths include production storage, schemas and proof assets; six blobs diverge from main. Establish surviving semantics and compatibility rather than replaying broad old changes.
- Blob counts: {'absent_main': 15, 'different': 6}. Merge simulation exit 1; tree `472a026de7924e0cc3f2471b465aeab19d9f1b2b`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...92e63d89b6eacdb5696d12b2da52593a8c2e526f).

Own changed files:

- `A .github/workflows/canonfs-portability.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/.github/workflows/canonfs-portability.yml))
- `M .github/workflows/cross-platform-determinism.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/.github/workflows/cross-platform-determinism.yml))
- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/CMakeLists.txt))
- `A experiments/canonfs_portability/.gitattributes` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/.gitattributes))
- `A experiments/canonfs_portability/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/README.md))
- `A experiments/canonfs_portability/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/REPORT.md))
- `A experiments/canonfs_portability/audit.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/audit.py))
- `A experiments/canonfs_portability/contract_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/contract_test.cpp))
- `A experiments/canonfs_portability/observed-initial/windows-job.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/observed-initial/windows-job.json))
- `A experiments/canonfs_portability/observed-initial/windows.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/observed-initial/windows.tar.gz))
- `A experiments/canonfs_portability/original-554/windows-analysis.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/original-554/windows-analysis.json))
- `A experiments/canonfs_portability/original-554/windows-failure.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/original-554/windows-failure.tar.gz))
- `A experiments/canonfs_portability/probe.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/probe.cpp))
- `A experiments/canonfs_portability/replay.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/replay.py))
- `A experiments/canonfs_portability/test_formal_z3_invariants.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/experiments/canonfs_portability/test_formal_z3_invariants.py))
- `M fs/persistent_driver.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/fs/persistent_driver.cpp))
- `M include/t81/canonfs/canon_driver.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/include/t81/canonfs/canon_driver.hpp))
- `A include/t81/canonfs/storage_mapping.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/include/t81/canonfs/storage_mapping.hpp))
- `M spec/rfcs/RFC-0054-canonfs-object-identity-and-persistence-contract.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/spec/rfcs/RFC-0054-canonfs-object-identity-and-persistence-contract.md))
- `M spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md))
- `A spec/rfcs/RFC-00D6-canonfs-portable-storage-mapping.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/92e63d89b6eacdb5696d12b2da52593a8c2e526f/spec/rfcs/RFC-00D6-canonfs-portable-storage-mapping.md))

All changed files versus main merge-base (including parents):

- `A .github/workflows/canonfs-portability.yml` — absent_main; main blob `None`; head blob `27cef100cf53ae8c2e04a21b39050616053f049d`.
- `M .github/workflows/cross-platform-determinism.yml` — different; main blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`; head blob `42bbc3fabad39923f29704b3fd803e2313352301`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `c5093c676f2bd53424ea39e8236fa9406a1af845`.
- `A experiments/canonfs_portability/.gitattributes` — absent_main; main blob `None`; head blob `1da8301e2c5f54615da7eedfd70f109dcd2d504a`.
- `A experiments/canonfs_portability/README.md` — absent_main; main blob `None`; head blob `752f417396758831e2f25fa8c4655ada3fab5b1e`.
- `A experiments/canonfs_portability/REPORT.md` — absent_main; main blob `None`; head blob `d66c5027bf5b49c655d53b8138e1cee4504be592`.
- `A experiments/canonfs_portability/audit.py` — absent_main; main blob `None`; head blob `5c89a8da34ec79c453b6bc552f7dfa0c2438aad6`.
- `A experiments/canonfs_portability/contract_test.cpp` — absent_main; main blob `None`; head blob `1f08457474f7797db3991e2ab891030b2ec9bca4`.
- `A experiments/canonfs_portability/observed-initial/windows-job.json` — absent_main; main blob `None`; head blob `be93405d4b4de720fcd33b8f08522fe33b522721`.
- `A experiments/canonfs_portability/observed-initial/windows.tar.gz` — absent_main; main blob `None`; head blob `81c35bd802778760b22caec6062afa280735c227`.
- `A experiments/canonfs_portability/original-554/windows-analysis.json` — absent_main; main blob `None`; head blob `b7b8e2acbf8e5a8b937c35442605a2d2c2367702`.
- `A experiments/canonfs_portability/original-554/windows-failure.tar.gz` — absent_main; main blob `None`; head blob `bfb773ac6ad3469c8bbb8804412b589ad16928f9`.
- `A experiments/canonfs_portability/probe.cpp` — absent_main; main blob `None`; head blob `bfde46a5a00a7d0782b6ddc1cab3917c060e8ee5`.
- `A experiments/canonfs_portability/replay.py` — absent_main; main blob `None`; head blob `bf271049525185809244ea69ca6a23445146b35f`.
- `A experiments/canonfs_portability/test_formal_z3_invariants.py` — absent_main; main blob `None`; head blob `590b2844609dda9fcd3d8d0497989864a672cd43`.
- `M fs/persistent_driver.cpp` — different; main blob `9ae4182253d5a298a75290a7ac4a99daa92ecd17`; head blob `f0eee1911ed9d0b38861991bbdd189adf5a7aff0`.
- `M include/t81/canonfs/canon_driver.hpp` — different; main blob `7f61ab90da6a3e6dfbc229f5abe82d19feafa057`; head blob `7ec8948854618ef06ba3d1b1b2aae7346b5a8700`.
- `A include/t81/canonfs/storage_mapping.hpp` — absent_main; main blob `None`; head blob `70f7e0627593f8303bb632cf9ef515ff2d662b64`.
- `M spec/rfcs/RFC-0054-canonfs-object-identity-and-persistence-contract.md` — different; main blob `a87b76ae0bd3c6bcd206bb3a145fb663c7b02b9e`; head blob `e1583680ba6f6af0680987fbd5df0ab8f4df49e3`.
- `M spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md` — different; main blob `ba6339f24d4400e920fec6553ea63901fc4202f1`; head blob `857e251065fb27e793a16b10ea3e6849fbb0be12`.
- `A spec/rfcs/RFC-00D6-canonfs-portable-storage-mapping.md` — absent_main; main blob `None`; head blob `a6fb9cc4584249ae8377baed5544ba16ea967a6b`.

Overlaps with other PR own deltas:

- #782: `CMakeLists.txt`
- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`
- #747: `.github/workflows/cross-platform-determinism.yml`, `CMakeLists.txt`
- #649: `CMakeLists.txt`
- #603: `CMakeLists.txt`, `fs/persistent_driver.cpp`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #525: `.github/workflows/cross-platform-determinism.yml`
- #513: `.github/workflows/cross-platform-determinism.yml`
- #508: `CMakeLists.txt`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:


## PR 554: experiment: independently challenge T81 against Wasmtime at the write boundary

- Head `7af837ece1325752e555b814c94aacc33afd2c96`; actual base `0005ab675d2729b2795200e8c78ded43d4520b0d` (`main`); main merge-base `0005ab675d2729b2795200e8c78ded43d4520b0d`.
- Purpose: T81 versus Wasmtime write-boundary experiment. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Related durable witness 557/560 and CanonFS mapping 556; partial integration is not whole-PR duplication.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [536, 553].
- Evidence/limits: 47/50 changed blobs already identical to main, but one absent and two differing remain. Audit residual workflow/CMake/artifact identity before closure; preserve negative Windows packet.
- Blob counts: {'absent_main': 1, 'different': 2, 'identical': 47}. Merge simulation exit 1; tree `dfc5c5c454e4c0345fd7b1f2a986711c0459283d`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 4 pass, 3 fail, 1 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...7af837ece1325752e555b814c94aacc33afd2c96).

Own changed files:

- `A .github/workflows/architecture-challenge.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/.github/workflows/architecture-challenge.yml))
- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/CMakeLists.txt))
- `A experiments/architecture_challenge/.gitattributes` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/.gitattributes))
- `A experiments/architecture_challenge/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/README.md))
- `A experiments/architecture_challenge/REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/REPORT.md))
- `A experiments/architecture_challenge/arm_a.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/arm_a.cpp))
- `A experiments/architecture_challenge/arm_b.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/arm_b.py))
- `A experiments/architecture_challenge/attacks.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/attacks.py))
- `A experiments/architecture_challenge/audit_archives.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/audit_archives.py))
- `A experiments/architecture_challenge/audit_corpus.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/audit_corpus.py))
- `A experiments/architecture_challenge/count.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/count.py))
- `A experiments/architecture_challenge/guest.wat` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/guest.wat))
- `A experiments/architecture_challenge/observed-ci/linux.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-ci/linux.tar.gz))
- `A experiments/architecture_challenge/observed-ci/macos.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-ci/macos.tar.gz))
- `A experiments/architecture_challenge/observed-ci/offline-audit.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-ci/offline-audit.json))
- `A experiments/architecture_challenge/observed-ci/sha256.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-ci/sha256.json))
- `A experiments/architecture_challenge/observed-ci/summary.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-ci/summary.json))
- `A experiments/architecture_challenge/observed-ci/windows-analysis.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-ci/windows-analysis.json))
- `A experiments/architecture_challenge/observed-ci/windows-failure.tar.gz` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-ci/windows-failure.tar.gz))
- `A experiments/architecture_challenge/observed-ci/windows-job.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-ci/windows-job.json))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/attacks.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/attacks.json))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/build.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/build.log))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/configure.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/configure.log))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/corpus.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/corpus.jsonl))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/counts.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/counts.json))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/first-ci-macos-job.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/first-ci-macos-job.json))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/first-ci-windows-job.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/first-ci-windows-job.json))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/initial-build-failure.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/initial-build-failure.log))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/measurements.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/measurements.log))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/raw.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/raw.jsonl))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/report.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/report.json))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/sha256.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/sha256.json))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/tests.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/tests.log))
- `A experiments/architecture_challenge/observed-macos-arm64-initial/verification.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64-initial/verification.jsonl))
- `A experiments/architecture_challenge/observed-macos-arm64/attacks.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/attacks.json))
- `A experiments/architecture_challenge/observed-macos-arm64/corpus.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/corpus.jsonl))
- `A experiments/architecture_challenge/observed-macos-arm64/counts-measured-source.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/counts-measured-source.json))
- `A experiments/architecture_challenge/observed-macos-arm64/counts.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/counts.json))
- `A experiments/architecture_challenge/observed-macos-arm64/final-tests.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/final-tests.log))
- `A experiments/architecture_challenge/observed-macos-arm64/measurements.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/measurements.log))
- `A experiments/architecture_challenge/observed-macos-arm64/raw.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/raw.jsonl))
- `A experiments/architecture_challenge/observed-macos-arm64/report.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/report.json))
- `A experiments/architecture_challenge/observed-macos-arm64/sha256.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/sha256.json))
- `A experiments/architecture_challenge/observed-macos-arm64/tests.log` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/tests.log))
- `A experiments/architecture_challenge/observed-macos-arm64/verification.jsonl` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/observed-macos-arm64/verification.jsonl))
- `A experiments/architecture_challenge/requirements.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/requirements.txt))
- `A experiments/architecture_challenge/run.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/run.py))
- `A experiments/architecture_challenge/test_challenge.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/test_challenge.py))
- `A experiments/architecture_challenge/verify.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/verify.py))
- `A experiments/architecture_challenge/workload-contract.json` ([pinned source](https://github.com/t81dev/t81-foundation/blob/7af837ece1325752e555b814c94aacc33afd2c96/experiments/architecture_challenge/workload-contract.json))

All changed files versus main merge-base (including parents):

- `A .github/workflows/architecture-challenge.yml` — absent_main; main blob `None`; head blob `5780fa3bc8e2174ac1bd1ded019ea0c7d35ee226`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `c8c812142bf55059c2964dacd4fb2b0c08862ad5`.
- `A experiments/architecture_challenge/.gitattributes` — identical; main blob `be7a273915157291d9f55c774701355de6da35ea`; head blob `be7a273915157291d9f55c774701355de6da35ea`.
- `A experiments/architecture_challenge/README.md` — identical; main blob `09e77eae26b960f693876de87fe84fd63aaf10b9`; head blob `09e77eae26b960f693876de87fe84fd63aaf10b9`.
- `A experiments/architecture_challenge/REPORT.md` — identical; main blob `128735e9c86b80d413e3440ea877e2511017a5cf`; head blob `128735e9c86b80d413e3440ea877e2511017a5cf`.
- `A experiments/architecture_challenge/arm_a.cpp` — identical; main blob `f2d33433cb1cbd855ad853608926e66e8b642128`; head blob `f2d33433cb1cbd855ad853608926e66e8b642128`.
- `A experiments/architecture_challenge/arm_b.py` — identical; main blob `039204aa1deed5a966da306d4fe4dcc5535027a6`; head blob `039204aa1deed5a966da306d4fe4dcc5535027a6`.
- `A experiments/architecture_challenge/attacks.py` — identical; main blob `523ac0c5ba175f738138aa9f25b041d33e72f411`; head blob `523ac0c5ba175f738138aa9f25b041d33e72f411`.
- `A experiments/architecture_challenge/audit_archives.py` — identical; main blob `21d6a3d2b696d81fe6cd90edfa770afa62dafb71`; head blob `21d6a3d2b696d81fe6cd90edfa770afa62dafb71`.
- `A experiments/architecture_challenge/audit_corpus.py` — identical; main blob `f4054c91156eb765f24a0918d7d13516b21dbe89`; head blob `f4054c91156eb765f24a0918d7d13516b21dbe89`.
- `A experiments/architecture_challenge/count.py` — identical; main blob `53f42bbae5f058c49ff6717863209ea6e6c8c913`; head blob `53f42bbae5f058c49ff6717863209ea6e6c8c913`.
- `A experiments/architecture_challenge/guest.wat` — identical; main blob `fa32c9e521b6970b2eaac34df4524eb83e4e8b7a`; head blob `fa32c9e521b6970b2eaac34df4524eb83e4e8b7a`.
- `A experiments/architecture_challenge/observed-ci/linux.tar.gz` — identical; main blob `2a0ac4d519b4b08a888eed5527e38570deec491a`; head blob `2a0ac4d519b4b08a888eed5527e38570deec491a`.
- `A experiments/architecture_challenge/observed-ci/macos.tar.gz` — identical; main blob `0924ca190e5718b0decd5537fac5ffe89418d270`; head blob `0924ca190e5718b0decd5537fac5ffe89418d270`.
- `A experiments/architecture_challenge/observed-ci/offline-audit.json` — identical; main blob `82456003b56a7fee134feac097e1d60d1426eb04`; head blob `82456003b56a7fee134feac097e1d60d1426eb04`.
- `A experiments/architecture_challenge/observed-ci/sha256.json` — identical; main blob `1996ef79fd1861762bddde22e1b2e034e73bb4f2`; head blob `1996ef79fd1861762bddde22e1b2e034e73bb4f2`.
- `A experiments/architecture_challenge/observed-ci/summary.json` — identical; main blob `cdc8eb63ea42e471fa5467afdc99bfc8d1c1382e`; head blob `cdc8eb63ea42e471fa5467afdc99bfc8d1c1382e`.
- `A experiments/architecture_challenge/observed-ci/windows-analysis.json` — identical; main blob `b7b8e2acbf8e5a8b937c35442605a2d2c2367702`; head blob `b7b8e2acbf8e5a8b937c35442605a2d2c2367702`.
- `A experiments/architecture_challenge/observed-ci/windows-failure.tar.gz` — identical; main blob `bfb773ac6ad3469c8bbb8804412b589ad16928f9`; head blob `bfb773ac6ad3469c8bbb8804412b589ad16928f9`.
- `A experiments/architecture_challenge/observed-ci/windows-job.json` — identical; main blob `ac4a125576e624440db12155f5cf5ce8c03a8807`; head blob `ac4a125576e624440db12155f5cf5ce8c03a8807`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/attacks.json` — identical; main blob `b3351a1803808083a71446009c95b3b41c947075`; head blob `b3351a1803808083a71446009c95b3b41c947075`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/build.log` — identical; main blob `8925f3f909877808b6b8578cf0ee6bfe525569b8`; head blob `8925f3f909877808b6b8578cf0ee6bfe525569b8`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/configure.log` — identical; main blob `fa372c48326b82dc5166c32e3a702a2e5aa5181b`; head blob `fa372c48326b82dc5166c32e3a702a2e5aa5181b`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/corpus.jsonl` — identical; main blob `580559cacfb765096587eeab229211c96f9cee31`; head blob `580559cacfb765096587eeab229211c96f9cee31`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/counts.json` — identical; main blob `c1f3b180daf1036c18c1c68277220f601d5eb38b`; head blob `c1f3b180daf1036c18c1c68277220f601d5eb38b`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/first-ci-macos-job.json` — identical; main blob `3dcaa8401b87657bb80f39c41b75a21c76bfc7e3`; head blob `3dcaa8401b87657bb80f39c41b75a21c76bfc7e3`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/first-ci-windows-job.json` — identical; main blob `d8a619d32d0034d9e7c42e172c306ce67eef37fa`; head blob `d8a619d32d0034d9e7c42e172c306ce67eef37fa`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/initial-build-failure.log` — different; main blob `1c1f4f51b2910bf99177c999c69a556a47a85ecb`; head blob `baeb7a02109d16c5c3cf2ec4a36ca7447ef6f056`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/measurements.log` — identical; main blob `f0a24298564d6290a2c3ceb6b9003c3d09897ac7`; head blob `f0a24298564d6290a2c3ceb6b9003c3d09897ac7`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/raw.jsonl` — identical; main blob `86541afe32dc75afa22db9709f43f088115071e0`; head blob `86541afe32dc75afa22db9709f43f088115071e0`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/report.json` — identical; main blob `d1e8363c39af0f2a034f92138a7667eb8c8e943e`; head blob `d1e8363c39af0f2a034f92138a7667eb8c8e943e`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/sha256.json` — identical; main blob `756996f7beebf9e2aeead71d1cf26a173521e79d`; head blob `756996f7beebf9e2aeead71d1cf26a173521e79d`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/tests.log` — identical; main blob `bd5cd6fcbece976f181823b1b25dd5d85eecb531`; head blob `bd5cd6fcbece976f181823b1b25dd5d85eecb531`.
- `A experiments/architecture_challenge/observed-macos-arm64-initial/verification.jsonl` — identical; main blob `d37e550987ecc9027ead07f2502c7fa53834fcea`; head blob `d37e550987ecc9027ead07f2502c7fa53834fcea`.
- `A experiments/architecture_challenge/observed-macos-arm64/attacks.json` — identical; main blob `b3351a1803808083a71446009c95b3b41c947075`; head blob `b3351a1803808083a71446009c95b3b41c947075`.
- `A experiments/architecture_challenge/observed-macos-arm64/corpus.jsonl` — identical; main blob `580559cacfb765096587eeab229211c96f9cee31`; head blob `580559cacfb765096587eeab229211c96f9cee31`.
- `A experiments/architecture_challenge/observed-macos-arm64/counts-measured-source.json` — identical; main blob `5c77f5052244a311627b535737b086251d036ec2`; head blob `5c77f5052244a311627b535737b086251d036ec2`.
- `A experiments/architecture_challenge/observed-macos-arm64/counts.json` — identical; main blob `7c9b0546b51672d270e0deb98b6a26495fab520f`; head blob `7c9b0546b51672d270e0deb98b6a26495fab520f`.
- `A experiments/architecture_challenge/observed-macos-arm64/final-tests.log` — identical; main blob `905b44323b1391352ed4bdd4f0e50c6cee9c2597`; head blob `905b44323b1391352ed4bdd4f0e50c6cee9c2597`.
- `A experiments/architecture_challenge/observed-macos-arm64/measurements.log` — identical; main blob `614673919720076414498924dbf4329deb0350cb`; head blob `614673919720076414498924dbf4329deb0350cb`.
- `A experiments/architecture_challenge/observed-macos-arm64/raw.jsonl` — identical; main blob `d1c0e7978ed279ca30e8981a92b3b1ffa7f551d6`; head blob `d1c0e7978ed279ca30e8981a92b3b1ffa7f551d6`.
- `A experiments/architecture_challenge/observed-macos-arm64/report.json` — identical; main blob `ef036ede87e766e5e0b0b1cb9ca85f18dc6de198`; head blob `ef036ede87e766e5e0b0b1cb9ca85f18dc6de198`.
- `A experiments/architecture_challenge/observed-macos-arm64/sha256.json` — identical; main blob `8e91e0012a3c370eb8ff2801486a1126832dd49b`; head blob `8e91e0012a3c370eb8ff2801486a1126832dd49b`.
- `A experiments/architecture_challenge/observed-macos-arm64/tests.log` — identical; main blob `6836323a580e299cbbeab91afa4301bfe9d5757b`; head blob `6836323a580e299cbbeab91afa4301bfe9d5757b`.
- `A experiments/architecture_challenge/observed-macos-arm64/verification.jsonl` — identical; main blob `d37e550987ecc9027ead07f2502c7fa53834fcea`; head blob `d37e550987ecc9027ead07f2502c7fa53834fcea`.
- `A experiments/architecture_challenge/requirements.txt` — identical; main blob `344a5819df76646d55bfa36feadf2d216bcdeae1`; head blob `344a5819df76646d55bfa36feadf2d216bcdeae1`.
- `A experiments/architecture_challenge/run.py` — identical; main blob `5af43f0b4e91cce2772ff370b512214ee4c879c7`; head blob `5af43f0b4e91cce2772ff370b512214ee4c879c7`.
- `A experiments/architecture_challenge/test_challenge.py` — identical; main blob `5db2ece8c97ca71ad7776b275d5710f3390ecd67`; head blob `5db2ece8c97ca71ad7776b275d5710f3390ecd67`.
- `A experiments/architecture_challenge/verify.py` — identical; main blob `5c2825d82e4ddae95544e39b23c3cc2826060a8d`; head blob `5c2825d82e4ddae95544e39b23c3cc2826060a8d`.
- `A experiments/architecture_challenge/workload-contract.json` — identical; main blob `edd1d3156bee720c45da4af2dd51785060999f8b`; head blob `edd1d3156bee720c45da4af2dd51785060999f8b`.

Overlaps with other PR own deltas:

- #782: `CMakeLists.txt`
- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`
- #747: `CMakeLists.txt`
- #649: `CMakeLists.txt`
- #603: `CMakeLists.txt`
- #556: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #508: `CMakeLists.txt`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/39877552360/job/120166738277): completed / failure.
- [compare (windows-2022)](https://github.com/t81dev/t81-foundation/actions/runs/39877552336/job/120166737847): completed / failure.
- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/39877552360/job/120166737651): completed / failure.

## PR 536: fix(core,vm): fix arithmetic UB, BigInt invariants, ControlledAIBacke…

- Head `671d3d133e6f0e1ee7e59012bc97f71462bec5d2`; actual base `872ac1b4c8a303fc5417d0b21551a430d0a50200` (`main`); main merge-base `872ac1b4c8a303fc5417d0b21551a430d0a50200`.
- Purpose: Broad core/VM arithmetic and direct-API corrections. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Overlaps 782 int128 consumers, VM/core baseline and SDK/API constraints; source is not patch-identical.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Eleven divergent core/build/test paths include historical reviewer objections. Rebase into minimal surviving fixes and rerun boundary tests; no blanket arithmetic-correctness conclusion.
- Blob counts: {'different': 10, 'absent_main': 1}. Merge simulation exit 1; tree `bcc0c2689ce0af409b3455fc40b002fd729a0e15`; equals main: False.
- Review: 2 COMMENTED; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...671d3d133e6f0e1ee7e59012bc97f71462bec5d2).

Own changed files:

- `M .gitignore` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/.gitignore))
- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/CMakeLists.txt))
- `M include/t81/types/T81BigInt.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/include/t81/types/T81BigInt.hpp))
- `M src/CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/src/CMakeLists.txt))
- `M src/direct_api/t81_direct_api.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/src/direct_api/t81_direct_api.cpp))
- `M src/direct_api/t81_direct_api_v3.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/src/direct_api/t81_direct_api_v3.cpp))
- `M tests/cpp/cli_contract_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/tests/cpp/cli_contract_test.cpp))
- `A tests/cpp/vm_value_ops_boundary_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/tests/cpp/vm_value_ops_boundary_test.cpp))
- `M vm/tier_limits.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/vm/tier_limits.cpp))
- `M vm/value_ops.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/vm/value_ops.cpp))
- `M vm/vm.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/671d3d133e6f0e1ee7e59012bc97f71462bec5d2/vm/vm.cpp))

All changed files versus main merge-base (including parents):

- `M .gitignore` — different; main blob `1dfd6156c8914161dafd9043cc76e9bb42fae95a`; head blob `4927c9e3d86ff3e949be4a1214401eda20310111`.
- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `75467d94673240da2d43d13fb397a17103aaf8e6`.
- `M include/t81/types/T81BigInt.hpp` — different; main blob `efc8f9b01d77a508040bef0d99e7b30e1429149d`; head blob `ab73862c302f653eece0e50fe0e1ac9c86ee4db5`.
- `M src/CMakeLists.txt` — different; main blob `7e834b7933f212a00974bcde10ab4ff8364c19bc`; head blob `b4a07c0fc1af7b97a99b834df0454a061dcb445a`.
- `M src/direct_api/t81_direct_api.cpp` — different; main blob `29036197d55c3fdd48af88be2aaa2d3b0f32fa4f`; head blob `1b73967c6200b2bf471534ce0408678c2a368659`.
- `M src/direct_api/t81_direct_api_v3.cpp` — different; main blob `56388ae386d2fd58645524fbbefaef80b5c462d1`; head blob `2c090fc1d88643bfbf27ce90e496bd3e352ca81c`.
- `M tests/cpp/cli_contract_test.cpp` — different; main blob `88daf9a73b6e7fb740108a563bc8143c7e666093`; head blob `671f4eeb18bd241b389aba701dae39f6222a6498`.
- `A tests/cpp/vm_value_ops_boundary_test.cpp` — absent_main; main blob `None`; head blob `798d7abfab109411614b8768de0b1bf36adb3ae9`.
- `M vm/tier_limits.cpp` — different; main blob `b7762f18810ca632122090bf102b8c9accf85a25`; head blob `f57af95b03c9218e8c9f72752f59ade3c04f9290`.
- `M vm/value_ops.cpp` — different; main blob `061ed01a5e539ff2abada9766ad3f2d6e73a6ef9`; head blob `d34ce9a00919a19e9eb300900d8b24f6838f5f95`.
- `M vm/vm.cpp` — different; main blob `698f5459900224cbfe59c90eec765e2504ab053b`; head blob `c07ba2ce287b9b665fe8c5cc1a0d83f4d1db50bc`.

Overlaps with other PR own deltas:

- #782: `CMakeLists.txt`
- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`
- #747: `CMakeLists.txt`
- #649: `CMakeLists.txt`
- #603: `CMakeLists.txt`, `vm/vm.cpp`
- #556: `CMakeLists.txt`
- #554: `CMakeLists.txt`
- #528: `vm/vm.cpp`
- #508: `CMakeLists.txt`

Review submissions:

- greptile-apps[bot] / COMMENTED / reviewed `c3820c2d67040f92213adc1c7ce6915ac3b4abef` / [review](https://github.com/t81dev/t81-foundation/pull/536#pullrequestreview-5598311408).
- t81dev / COMMENTED / reviewed `6555f0b1636f18baa77e8141118fec47d0cbdc47` / [review](https://github.com/t81dev/t81-foundation/pull/536#pullrequestreview-5644345371).

Failed or incomplete checks:


## PR 528: Implement Phase 1 Real-Model Validation Experiment

- Head `29b09ec231e2f695e7c94be70569e2546688a45f`; actual base `98738607d2bbe849d4023e6340b83c1f2d105c5f` (`main`); main merge-base `98738607d2bbe849d4023e6340b83c1f2d105c5f`.
- Purpose: Real-model validation implementation/report. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Historical model validation; broad runtime overlaps 536; no authorized model download/campaign here.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Seven paths include opcode header, VM, root validator, model script and reports. One successful check is not experiment validation. Verify current model provenance and each surviving code change before execution.
- Blob counts: {'absent_main': 3, 'different': 4}. Merge simulation exit 1; tree `27f1de5a0e5a6bf510e1a54bdcdebb7fbc721b3a`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 1 pass; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...29b09ec231e2f695e7c94be70569e2546688a45f).

Own changed files:

- `A REAL_MODEL_VALIDATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/29b09ec231e2f695e7c94be70569e2546688a45f/REAL_MODEL_VALIDATION_REPORT.md))
- `M examples/ai-and-inference/model-load-canonfs/README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/29b09ec231e2f695e7c94be70569e2546688a45f/examples/ai-and-inference/model-load-canonfs/README.md))
- `A examples/ai-and-inference/model-load-canonfs/REAL_MODEL_VALIDATION_REPORT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/29b09ec231e2f695e7c94be70569e2546688a45f/examples/ai-and-inference/model-load-canonfs/REAL_MODEL_VALIDATION_REPORT.md))
- `A examples/ai-and-inference/model-load-canonfs/test_real_hf_model.sh` ([pinned source](https://github.com/t81dev/t81-foundation/blob/29b09ec231e2f695e7c94be70569e2546688a45f/examples/ai-and-inference/model-load-canonfs/test_real_hf_model.sh))
- `M include/t81/isa/opcodes.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/29b09ec231e2f695e7c94be70569e2546688a45f/include/t81/isa/opcodes.hpp))
- `M scripts/governance/check_root_structure.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/29b09ec231e2f695e7c94be70569e2546688a45f/scripts/governance/check_root_structure.py))
- `M vm/vm.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/29b09ec231e2f695e7c94be70569e2546688a45f/vm/vm.cpp))

All changed files versus main merge-base (including parents):

- `A REAL_MODEL_VALIDATION_REPORT.md` — absent_main; main blob `None`; head blob `6d054dfe1dc6078a6395b9e9e2ca47e8c0574fb7`.
- `M examples/ai-and-inference/model-load-canonfs/README.md` — different; main blob `9e20eaacf001224ce2dfa20dbbb50c141342c3b3`; head blob `5a8097cbaf68912b0ffb3cf69bc8a9a0fb29f1c5`.
- `A examples/ai-and-inference/model-load-canonfs/REAL_MODEL_VALIDATION_REPORT.md` — absent_main; main blob `None`; head blob `6d054dfe1dc6078a6395b9e9e2ca47e8c0574fb7`.
- `A examples/ai-and-inference/model-load-canonfs/test_real_hf_model.sh` — absent_main; main blob `None`; head blob `20628c33a203134369d1bbd0ef6beb26e58a7b42`.
- `M include/t81/isa/opcodes.hpp` — different; main blob `cffbd89a3aa9cf26dfd2ae3ba13a7ac54a2e1d4f`; head blob `44f3b50e71f63dc0f846c1862c5e042873888550`.
- `M scripts/governance/check_root_structure.py` — different; main blob `c9991cc6c576a66c1eb8c21e0e0e43bd1484617b`; head blob `e2077dfd8419a8e1d27b72bda2bc712667e2cc7a`.
- `M vm/vm.cpp` — different; main blob `698f5459900224cbfe59c90eec765e2504ab053b`; head blob `baca8e488877d088da7a4aaff5968d1d6fe49ccf`.

Overlaps with other PR own deltas:

- #603: `vm/vm.cpp`
- #580: `scripts/governance/check_root_structure.py`
- #536: `vm/vm.cpp`

Review submissions:

- greptile-apps[bot] / COMMENTED / reviewed `605c66125b2dac463e45900f3d66cdf4f45418dc` / [review](https://github.com/t81dev/t81-foundation/pull/528#pullrequestreview-5447588279).

Failed or incomplete checks:


## PR 525: Bump actions/checkout from 6.0.2 to 7.0.0

- Head `e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4`; actual base `98738607d2bbe849d4023e6340b83c1f2d105c5f` (`main`); main merge-base `98738607d2bbe849d4023e6340b83c1f2d105c5f`.
- Purpose: Pinned actions/checkout major-version update. Decision: **REVIEW NEXT**.
- Dependency assessment: Overlaps 513 and governance/workflow stack; keep immutable pins and token policy.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [1924, 1941, 1946, 1971, 1977, 2043, 2044, 2194, 2224, 2226, 2236, 2248, 2286, 2301, 2305, 2327, 2356, 2357, 2414, 2439, 2446, 2454, 2458, 2459, 2460, 2461, 2462, 2463, 2464, 2467].
- Evidence/limits: 18 workflow paths; verify upstream target SHA, runtime/major compatibility and stale v6 comments before updating current workflows. Failing old CI is not a closure reason.
- Blob counts: {'different': 18}. Merge simulation exit 0; tree `eb74b7af7c4236a639af2632457e8e08e2ae007f`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 4 pass, 6 fail, 24 skip, 1 neutral; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4).

Own changed files:

- `M .github/workflows/ai-experiments-ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/ai-experiments-ci.yml))
- `M .github/workflows/ai-kms-rotation.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/ai-kms-rotation.yml))
- `M .github/workflows/bench.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/bench.yml))
- `M .github/workflows/benchmark_packed_trit_vector.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/benchmark_packed_trit_vector.yml))
- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/ci.yml))
- `M .github/workflows/codeql.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/codeql.yml))
- `M .github/workflows/cross-platform-determinism.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/cross-platform-determinism.yml))
- `M .github/workflows/docker.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/docker.yml))
- `M .github/workflows/documentation.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/documentation.yml))
- `M .github/workflows/format.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/format.yml))
- `M .github/workflows/inference-bench.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/inference-bench.yml))
- `M .github/workflows/python-wheels.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/python-wheels.yml))
- `M .github/workflows/qemu-boot-x86.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/qemu-boot-x86.yml))
- `M .github/workflows/qemu-boot.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/qemu-boot.yml))
- `M .github/workflows/release.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/release.yml))
- `M .github/workflows/repro-ledger.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/repro-ledger.yml))
- `M .github/workflows/runtime-contract.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/runtime-contract.yml))
- `M .github/workflows/t81lang-repro-hash-refresh.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4/.github/workflows/t81lang-repro-hash-refresh.yml))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `94bd628cd092bb3a2a2f9702d47fa640d629940a`.
- `M .github/workflows/ai-kms-rotation.yml` — different; main blob `c553efcf340794294c659fe7f5b8b245d6017224`; head blob `fa90cd93eb36048227bd03f2ac73e75b998e1cbd`.
- `M .github/workflows/bench.yml` — different; main blob `dcc19e495caeb7e524c87a0b0d90515b8eadadec`; head blob `c11dc968ee6d0b58a86e1e34a57eee06065d6d2d`.
- `M .github/workflows/benchmark_packed_trit_vector.yml` — different; main blob `281727af8a5e84b7208539a7c4cb743a7223ef66`; head blob `7f51720d827ba97549aa2f3e993e7fc40165fa82`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `be4146de0f33fe6c8dc286e8852333f07ec226f5`.
- `M .github/workflows/codeql.yml` — different; main blob `849cefdd0274c9ea1b7be788296fc5deabbfc4e8`; head blob `21efd95f032342aa4519c3aec8c681b60b08755a`.
- `M .github/workflows/cross-platform-determinism.yml` — different; main blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`; head blob `ee8c891b89fe39372c7536d3d81937336ef02001`.
- `M .github/workflows/docker.yml` — different; main blob `c1218c1d6c96411f6893094866cc72e6ced488a6`; head blob `b7ee9676cf227da686e4940027bd714f19efdc01`.
- `M .github/workflows/documentation.yml` — different; main blob `97440debde335bfe86384077d494c8715d022b3c`; head blob `d71e282e852d6371a96d4465fca4ad0f837b412a`.
- `M .github/workflows/format.yml` — different; main blob `2b4220fe45a0dcc8624ccabe4161ca64c73da5c0`; head blob `78d7fcf8167af0a7b75b513276a89229b08ab7e5`.
- `M .github/workflows/inference-bench.yml` — different; main blob `5ef60c8fa6977a040e103b43553d0ae01026cfde`; head blob `d4e6bd4982c1822f9f16205af83a3d3c8b4adaa3`.
- `M .github/workflows/python-wheels.yml` — different; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `b486fdb2b3506f672d35b07a6ecc49a8928cceff`.
- `M .github/workflows/qemu-boot-x86.yml` — different; main blob `bcad444157a39c3d86355298094746016a17e9cc`; head blob `39d6e1ac192856e1aba39b3f30806d16fad69143`.
- `M .github/workflows/qemu-boot.yml` — different; main blob `873acefcfe4db9c5f09ceee798862e9a39ce5d68`; head blob `8a7874832be037f9798a6dc9ce9ff334e886091b`.
- `M .github/workflows/release.yml` — different; main blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`; head blob `d99da5daf24bf3692a973a0ada5d14303f67c3f1`.
- `M .github/workflows/repro-ledger.yml` — different; main blob `72c1afb976ccaba81004f0bda3b4d95d99de1126`; head blob `fd8cea970cf8970a0b63367aef0d812c58fedb21`.
- `M .github/workflows/runtime-contract.yml` — different; main blob `940ea22fbd0d5d85226dbcbc13f23b9dd9616f39`; head blob `628af5dc91d82f45df3700e49d6abbbede1e99a5`.
- `M .github/workflows/t81lang-repro-hash-refresh.yml` — different; main blob `b4daa3553d7de6174a85929fe3aaf876813aac2d`; head blob `58cb09f58285e95d2235646de78d610f7475a841`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`
- #747: `.github/workflows/ci.yml`, `.github/workflows/cross-platform-determinism.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/release.yml`
- #649: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`, `.github/workflows/docker.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`
- #577: `.github/workflows/ci.yml`, `.github/workflows/docker.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`
- #576: `.github/workflows/ci.yml`
- #575: `.github/workflows/ci.yml`
- #574: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`
- #556: `.github/workflows/cross-platform-determinism.yml`
- #524: `.github/workflows/release.yml`
- #523: `.github/workflows/codeql.yml`
- #516: `.github/workflows/documentation.yml`
- #514: `.github/workflows/ci.yml`
- #513: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/bench.yml`, `.github/workflows/ci.yml`, `.github/workflows/cross-platform-determinism.yml`, `.github/workflows/documentation.yml`, `.github/workflows/inference-bench.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`, `.github/workflows/release.yml`, `.github/workflows/repro-ledger.yml`
- #504: `.github/workflows/ci.yml`

Review submissions:

- greptile-apps[bot] / COMMENTED / reviewed `e6e3f9d8ba7fdb253ba0398ec7c64c447669f2c4` / [review](https://github.com/t81dev/t81-foundation/pull/525#pullrequestreview-4553676326).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/28029522683/job/82966175907): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/28029522683/job/82966098835): completed / failure.
- [Analyze (cpp)](https://github.com/t81dev/t81-foundation/actions/runs/28029522367/job/82966097974): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/28029522414/job/82966097591): completed / failure.
- [test-ai-experiments](https://github.com/t81dev/t81-foundation/actions/runs/28029522402/job/82966097534): completed / failure.
- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/28029522414/job/82966097507): completed / failure.

## PR 524: Bump softprops/action-gh-release from 2.6.1 to 3.0.1

- Head `5b057e7a81c1bbe417b030d89655caba0c1c46ac`; actual base `98738607d2bbe849d4023e6340b83c1f2d105c5f` (`main`); main merge-base `98738607d2bbe849d4023e6340b83c1f2d105c5f`.
- Purpose: Pinned release action major-version update. Decision: **REVIEW NEXT**.
- Dependency assessment: Same release workflow touched by 525/747; no release execution authorized.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [764, 765, 775, 777, 781, 792, 796, 797, 798, 800, 802, 804, 805, 806].
- Evidence/limits: One release workflow path. Verify upstream SHA and v3 behavior against current release/publish contract before fresh CI.
- Blob counts: {'different': 1}. Merge simulation exit 0; tree `a10bcd0a8991f215e93c1070b173746d26ba0a51`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 3 pass, 5 fail, 24 skip, 1 neutral; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...5b057e7a81c1bbe417b030d89655caba0c1c46ac).

Own changed files:

- `M .github/workflows/release.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/5b057e7a81c1bbe417b030d89655caba0c1c46ac/.github/workflows/release.yml))

All changed files versus main merge-base (including parents):

- `M .github/workflows/release.yml` — different; main blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`; head blob `98e1d6b27558101b9df03226258b1cbba546f829`.

Overlaps with other PR own deltas:

- #747: `.github/workflows/release.yml`
- #525: `.github/workflows/release.yml`
- #513: `.github/workflows/release.yml`

Review submissions:

- greptile-apps[bot] / COMMENTED / reviewed `5b057e7a81c1bbe417b030d89655caba0c1c46ac` / [review](https://github.com/t81dev/t81-foundation/pull/524#pullrequestreview-4553672142).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/28029494463/job/82966064245): completed / failure.
- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/28029495259/job/82966004817): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/28029495259/job/82966004667): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/28029494463/job/82966001687): completed / failure.
- [Analyze (cpp)](https://github.com/t81dev/t81-foundation/actions/runs/28029494502/job/82966001612): completed / failure.

## PR 523: Bump github/codeql-action from 4.34.1 to 4.36.2

- Head `abcec70e01d4366a24a3b6b128f9bb7813b2c87c`; actual base `98738607d2bbe849d4023e6340b83c1f2d105c5f` (`main`); main merge-base `98738607d2bbe849d4023e6340b83c1f2d105c5f`.
- Purpose: Pinned CodeQL action update. Decision: **REVIEW NEXT**.
- Dependency assessment: Independent package update; same workflow also touched by checkout bump 525.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [3767, 3773, 3781, 3789, 3791, 3794, 3795, 3807, 3823, 3837, 3850, 3852, 3853, 3865, 3880, 3881, 3892, 3893, 3894, 3899, 3926, 3937, 3943, 3946, 3947, 3948, 3949].
- Evidence/limits: One CodeQL workflow path, three pinned action uses. Verify upstream SHA and config compatibility; retain security permissions.
- Blob counts: {'different': 1}. Merge simulation exit 0; tree `889d83823e583804dcd34f016127e69a474dad96`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 3 pass, 5 fail, 24 skip, 1 neutral; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...abcec70e01d4366a24a3b6b128f9bb7813b2c87c).

Own changed files:

- `M .github/workflows/codeql.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/abcec70e01d4366a24a3b6b128f9bb7813b2c87c/.github/workflows/codeql.yml))

All changed files versus main merge-base (including parents):

- `M .github/workflows/codeql.yml` — different; main blob `849cefdd0274c9ea1b7be788296fc5deabbfc4e8`; head blob `0281a8d14f860b28f445ff606feb5a32efd77919`.

Overlaps with other PR own deltas:

- #525: `.github/workflows/codeql.yml`

Review submissions:

- greptile-apps[bot] / COMMENTED / reviewed `abcec70e01d4366a24a3b6b128f9bb7813b2c87c` / [review](https://github.com/t81dev/t81-foundation/pull/523#pullrequestreview-4458988768).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/27209275062/job/80333573220): completed / failure.
- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/27209275484/job/80333506156): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/27209275484/job/80333505943): completed / failure.
- [Analyze (cpp)](https://github.com/t81dev/t81-foundation/actions/runs/27209274860/job/80333504176): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/27209275062/job/80333503472): completed / failure.

## PR 516: Bump actions/setup-node from 6.3.0 to 6.4.0

- Head `537ad15b36cc65106ecdcb20d9d141c0db69c486`; actual base `98738607d2bbe849d4023e6340b83c1f2d105c5f` (`main`); main merge-base `98738607d2bbe849d4023e6340b83c1f2d105c5f`.
- Purpose: Pinned setup-node patch/minor action update. Decision: **REVIEW NEXT**.
- Dependency assessment: Shares workflow with 525; no dependency on experimental PRs.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [1525, 1533].
- Evidence/limits: One documentation workflow path. Verify upstream SHA/runtime compatibility; no evidence it is redundant on main.
- Blob counts: {'different': 1}. Merge simulation exit 0; tree `07c1e7c6130e5d683512b073fc39ba66c2245643`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 3 pass, 5 fail, 24 skip, 1 neutral; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...537ad15b36cc65106ecdcb20d9d141c0db69c486).

Own changed files:

- `M .github/workflows/documentation.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/537ad15b36cc65106ecdcb20d9d141c0db69c486/.github/workflows/documentation.yml))

All changed files versus main merge-base (including parents):

- `M .github/workflows/documentation.yml` — different; main blob `97440debde335bfe86384077d494c8715d022b3c`; head blob `939730d79bcb30cc6863eacd6d4556f08ef78fe6`.

Overlaps with other PR own deltas:

- #525: `.github/workflows/documentation.yml`
- #513: `.github/workflows/documentation.yml`

Review submissions:

- greptile-apps[bot] / COMMENTED / reviewed `537ad15b36cc65106ecdcb20d9d141c0db69c486` / [review](https://github.com/t81dev/t81-foundation/pull/516#pullrequestreview-4148004445).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/24725054265/job/72323734749): completed / failure.
- [Analyze (cpp)](https://github.com/t81dev/t81-foundation/actions/runs/24725054281/job/72323676045): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/24725054265/job/72323675516): completed / failure.
- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/24725054273/job/72323675449): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/24725054273/job/72323675438): completed / failure.

## PR 514: Bump actions/cache from 5.0.4 to 5.0.5

- Head `61002ef3b586fcfa8c3eaf07a540608744b1fb77`; actual base `98738607d2bbe849d4023e6340b83c1f2d105c5f` (`main`); main merge-base `98738607d2bbe849d4023e6340b83c1f2d105c5f`.
- Purpose: Pinned actions/cache patch update. Decision: **REVIEW NEXT**.
- Dependency assessment: Shared CI path with 782/525/513 and governance stack; overlap is not duplicate.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [1685, 1692, 1747].
- Evidence/limits: One CI workflow path with two uses. Verify upstream SHA and cache behavior, then current-base checks.
- Blob counts: {'different': 1}. Merge simulation exit 0; tree `e32e4d1d99fa48122c663d97800658628e9e3e35`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 3 pass, 5 fail, 24 skip, 1 neutral; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...61002ef3b586fcfa8c3eaf07a540608744b1fb77).

Own changed files:

- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/61002ef3b586fcfa8c3eaf07a540608744b1fb77/.github/workflows/ci.yml))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `df36e37a4780ee05768a9ae3c140806294fe9b27`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`
- #747: `.github/workflows/ci.yml`
- #649: `.github/workflows/ci.yml`
- #577: `.github/workflows/ci.yml`
- #576: `.github/workflows/ci.yml`
- #575: `.github/workflows/ci.yml`
- #574: `.github/workflows/ci.yml`
- #525: `.github/workflows/ci.yml`
- #513: `.github/workflows/ci.yml`
- #504: `.github/workflows/ci.yml`

Review submissions:

- greptile-apps[bot] / COMMENTED / reviewed `61002ef3b586fcfa8c3eaf07a540608744b1fb77` / [review](https://github.com/t81dev/t81-foundation/pull/514#pullrequestreview-4106274057).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/24401760134/job/71274018367): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/24401760134/job/71273970292): completed / failure.
- [Analyze (cpp)](https://github.com/t81dev/t81-foundation/actions/runs/24401760046/job/71273969930): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/24401760024/job/71273969717): completed / failure.
- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/24401760024/job/71273969706): completed / failure.

## PR 513: Bump actions/upload-artifact from 7.0.0 to 7.0.1

- Head `687ced99ac87f81d1f4effc16f5675d291569234`; actual base `98738607d2bbe849d4023e6340b83c1f2d105c5f` (`main`); main merge-base `98738607d2bbe849d4023e6340b83c1f2d105c5f`.
- Purpose: Pinned artifact-upload patch update. Decision: **REVIEW NEXT**.
- Dependency assessment: Overlaps eleven paths with 525 and five with 649; preserve evidence upload semantics.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [795, 796, 797].
- Evidence/limits: Eleven workflow paths; verify upstream SHA and artifact retention/naming compatibility before current-base checks.
- Blob counts: {'different': 11}. Merge simulation exit 0; tree `d47596b04f7d20f0d88380a218a7bc90e1ea49a8`; equals main: False.
- Review: 1 COMMENTED; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 4 pass, 6 fail, 24 skip, 1 neutral; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...687ced99ac87f81d1f4effc16f5675d291569234).

Own changed files:

- `M .github/workflows/ai-experiments-ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/ai-experiments-ci.yml))
- `M .github/workflows/bench.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/bench.yml))
- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/ci.yml))
- `M .github/workflows/cross-platform-determinism.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/cross-platform-determinism.yml))
- `M .github/workflows/documentation.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/documentation.yml))
- `M .github/workflows/inference-bench.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/inference-bench.yml))
- `M .github/workflows/python-wheels.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/python-wheels.yml))
- `M .github/workflows/qemu-boot-x86.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/qemu-boot-x86.yml))
- `M .github/workflows/qemu-boot.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/qemu-boot.yml))
- `M .github/workflows/release.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/release.yml))
- `M .github/workflows/repro-ledger.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/687ced99ac87f81d1f4effc16f5675d291569234/.github/workflows/repro-ledger.yml))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ai-experiments-ci.yml` — different; main blob `7575184e28d07527e2f4ee336b964917ce76898d`; head blob `462e0a4f6146244c91743e99724607a82daf92ee`.
- `M .github/workflows/bench.yml` — different; main blob `dcc19e495caeb7e524c87a0b0d90515b8eadadec`; head blob `f88fa42169a4ac60eb151b77be11706e260b382f`.
- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `f58066f0a78306c9838f20944ecca55b9aeb9e19`.
- `M .github/workflows/cross-platform-determinism.yml` — different; main blob `9cc76f9808d96b57fddce3aaf5d84739c779da1a`; head blob `55ef0fb89719bb2953bfbe0a1568f78e4ec283f5`.
- `M .github/workflows/documentation.yml` — different; main blob `97440debde335bfe86384077d494c8715d022b3c`; head blob `390282feae1c8a4950b099a1d3eefbfc61183cec`.
- `M .github/workflows/inference-bench.yml` — different; main blob `5ef60c8fa6977a040e103b43553d0ae01026cfde`; head blob `7a0c001f6a1bec196d4dab334e06dd88ab5a205f`.
- `M .github/workflows/python-wheels.yml` — different; main blob `d41ca3f3163787ab7e9dc2d8386a03597a2cda17`; head blob `b370824a7f1bc938ca4e804d06f74251ae6e1ee3`.
- `M .github/workflows/qemu-boot-x86.yml` — different; main blob `bcad444157a39c3d86355298094746016a17e9cc`; head blob `e72c946e50b023a22fce577aab7061ca2c9ce407`.
- `M .github/workflows/qemu-boot.yml` — different; main blob `873acefcfe4db9c5f09ceee798862e9a39ce5d68`; head blob `5ead9322cd9175fd41df7cd668805b3762e6924f`.
- `M .github/workflows/release.yml` — different; main blob `9970d7fbc2eba445ed614f8c3cec2d8463a64365`; head blob `4c387a56878ced9a844810abfd187c2f33eac497`.
- `M .github/workflows/repro-ledger.yml` — different; main blob `72c1afb976ccaba81004f0bda3b4d95d99de1126`; head blob `c27b49a87dabafec5ea7dafe574d261d5c2e4acd`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`
- #747: `.github/workflows/ci.yml`, `.github/workflows/cross-platform-determinism.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/release.yml`
- #649: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`
- #577: `.github/workflows/ci.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`
- #576: `.github/workflows/ci.yml`
- #575: `.github/workflows/ci.yml`
- #574: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/ci.yml`
- #556: `.github/workflows/cross-platform-determinism.yml`
- #525: `.github/workflows/ai-experiments-ci.yml`, `.github/workflows/bench.yml`, `.github/workflows/ci.yml`, `.github/workflows/cross-platform-determinism.yml`, `.github/workflows/documentation.yml`, `.github/workflows/inference-bench.yml`, `.github/workflows/python-wheels.yml`, `.github/workflows/qemu-boot-x86.yml`, `.github/workflows/qemu-boot.yml`, `.github/workflows/release.yml`, `.github/workflows/repro-ledger.yml`
- #524: `.github/workflows/release.yml`
- #516: `.github/workflows/documentation.yml`
- #514: `.github/workflows/ci.yml`
- #504: `.github/workflows/ci.yml`

Review submissions:

- greptile-apps[bot] / COMMENTED / reviewed `687ced99ac87f81d1f4effc16f5675d291569234` / [review](https://github.com/t81dev/t81-foundation/pull/513#pullrequestreview-4106272402).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/24401748043/job/71273980945): completed / failure.
- [test-ai-experiments](https://github.com/t81dev/t81-foundation/actions/runs/24401748042/job/71273927137): completed / failure.
- [determinism / linux-x86_64](https://github.com/t81dev/t81-foundation/actions/runs/24401748071/job/71273927128): completed / failure.
- [determinism / macos-arm64](https://github.com/t81dev/t81-foundation/actions/runs/24401748071/job/71273927104): completed / failure.
- [lint / spec & docs](https://github.com/t81dev/t81-foundation/actions/runs/24401748043/job/71273927087): completed / failure.
- [Analyze (cpp)](https://github.com/t81dev/t81-foundation/actions/runs/24401748053/job/71273926880): completed / failure.

## PR 508: feat: integrate Tier 6 distributed monad into build and add tests

- Head `832b7b0824d1d7c744f2550756391e0b5d182999`; actual base `ba3e1ff0aa7588c859e3f59272444621990ca620` (`main`); main merge-base `ba3e1ff0aa7588c859e3f59272444621990ca620`.
- Purpose: Tier-6 build integration and tests. Decision: **REPAIR**.
- Dependency assessment: Adds experimental subsystem build surface; overlaps 649 CMake/README and HAL files, not a duplicate.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Source test expects synchronous labels while mesh_reflect mutates peers in-place; assert checks vanish in Release. Resolve intended semantics and make required tests active; inventory did not execute them.
- Blob counts: {'different': 4, 'absent_main': 1}. Merge simulation exit 1; tree `85af16a8e039b6ca3fb3a7a446dd90c43f972a14`; equals main: False.
- Review: 2 COMMENTED; threads 3 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 33 pass, 3 fail, 7 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...832b7b0824d1d7c744f2550756391e0b5d182999).

Own changed files:

- `M CMakeLists.txt` ([pinned source](https://github.com/t81dev/t81-foundation/blob/832b7b0824d1d7c744f2550756391e0b5d182999/CMakeLists.txt))
- `M README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/832b7b0824d1d7c744f2550756391e0b5d182999/README.md))
- `M ternaryos/dev/gicv3.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/832b7b0824d1d7c744f2550756391e0b5d182999/ternaryos/dev/gicv3.cpp))
- `M ternaryos/tests/hal_boot_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/832b7b0824d1d7c744f2550756391e0b5d182999/ternaryos/tests/hal_boot_test.cpp))
- `A tests/cpp/tier6_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/832b7b0824d1d7c744f2550756391e0b5d182999/tests/cpp/tier6_test.cpp))

All changed files versus main merge-base (including parents):

- `M CMakeLists.txt` — different; main blob `8cd9cb5a18dc44931142a71e992a628eab7c6044`; head blob `13d43605b321f41d98fe68ce0e66e3feb41051f8`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `e1d948fdc793f3b2924ef754234c3c825d4eb0d0`.
- `M ternaryos/dev/gicv3.cpp` — different; main blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`; head blob `54f7bc8e9b5515530ff0ce6d488bed97153ee314`.
- `M ternaryos/tests/hal_boot_test.cpp` — different; main blob `fdcc3b332b6ca7b12792da9a7a0255e2ff179b1f`; head blob `d1a32b0f326a866f3823edda03ad18e4e28dfb01`.
- `A tests/cpp/tier6_test.cpp` — absent_main; main blob `None`; head blob `03752001560f09f0408aa27be66431b7696c3f3d`.

Overlaps with other PR own deltas:

- #782: `CMakeLists.txt`
- #769: `CMakeLists.txt`
- #760: `CMakeLists.txt`
- #751: `CMakeLists.txt`
- #750: `CMakeLists.txt`
- #747: `CMakeLists.txt`, `ternaryos/dev/gicv3.cpp`
- #649: `CMakeLists.txt`, `README.md`
- #603: `CMakeLists.txt`
- #576: `README.md`
- #556: `CMakeLists.txt`
- #554: `CMakeLists.txt`
- #536: `CMakeLists.txt`
- #507: `README.md`, `ternaryos/dev/gicv3.cpp`, `ternaryos/tests/hal_boot_test.cpp`
- #504: `README.md`

Review submissions:

- chatgpt-codex-connector[bot] / COMMENTED / reviewed `6627908f705d15e0910d24288d06c3d6ba1b1e8b` / [review](https://github.com/t81dev/t81-foundation/pull/508#pullrequestreview-4033216014).
- greptile-apps[bot] / COMMENTED / reviewed `6627908f705d15e0910d24288d06c3d6ba1b1e8b` / [review](https://github.com/t81dev/t81-foundation/pull/508#pullrequestreview-4033222352).

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/23768420703/job/69256797230): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/23768420703/job/69254095180): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/23768420703/job/69254095129): completed / failure.

## PR 507: Documentation audit and hygiene update

- Head `b8ca8e3b25ee94cbedf55702f4c639a6294b6565`; actual base `925f4a32d932e71cc199e3d5c9becfea230f1a62` (`main`); main merge-base `925f4a32d932e71cc199e3d5c9becfea230f1a62`.
- Purpose: Documentation audit and hygiene patch. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Overlaps 504 and current continuity candidates; no wholesale supersession proved.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Twenty paths include seven unique archive assets and thirteen divergent current docs. Preserve snapshots but reconcile present architecture/roadmap language with current research charter; old snapshot divergence alone is not a defect.
- Blob counts: {'different': 13, 'absent_main': 7}. Merge simulation exit 1; tree `9ccee783ae046dae2ba226a67da55e02318096b3`; equals main: False.
- Review: 1 COMMENTED; threads 1 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: none; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b8ca8e3b25ee94cbedf55702f4c639a6294b6565).

Own changed files:

- `M README.es.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/README.es.md))
- `M README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/README.md))
- `M README.pt-BR.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/README.pt-BR.md))
- `M README.ru.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/README.ru.md))
- `M README.zh-CN.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/README.zh-CN.md))
- `M docs/BUILDABLE_NEXT_STEPS.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/BUILDABLE_NEXT_STEPS.md))
- `M docs/HANDOFF.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/HANDOFF.md))
- `M docs/ROADMAP.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/ROADMAP.md))
- `M docs/architecture/OVERVIEW.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/architecture/OVERVIEW.md))
- `M docs/explanation/T81_FOUNDATION_PROJECT_PROFILE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/explanation/T81_FOUNDATION_PROJECT_PROFILE.md))
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_A_EXECUTIVE_SUMMARY.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/records/audits/2026-03_documentation_refresh/OUTPUT_A_EXECUTIVE_SUMMARY.md))
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_B_DOCUMENT_INVENTORY_TABLE.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/records/audits/2026-03_documentation_refresh/OUTPUT_B_DOCUMENT_INVENTORY_TABLE.md))
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_C_STALE_CLAIMS_REGISTER.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/records/audits/2026-03_documentation_refresh/OUTPUT_C_STALE_CLAIMS_REGISTER.md))
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_D_CROSS_DOCUMENT_CONFLICT_REGISTER.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/records/audits/2026-03_documentation_refresh/OUTPUT_D_CROSS_DOCUMENT_CONFLICT_REGISTER.md))
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_E_PRIORITIZED_REMEDIATION_PLAN.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/records/audits/2026-03_documentation_refresh/OUTPUT_E_PRIORITIZED_REMEDIATION_PLAN.md))
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_F_UPDATED_DOCUMENTS/docs/architecture/OVERVIEW.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/records/audits/2026-03_documentation_refresh/OUTPUT_F_UPDATED_DOCUMENTS/docs/architecture/OVERVIEW.md))
- `A docs/records/audits/2026-03_documentation_refresh/README_MITIGATION.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/docs/records/audits/2026-03_documentation_refresh/README_MITIGATION.md))
- `M scripts/governance/check_docs_structure.py` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/scripts/governance/check_docs_structure.py))
- `M ternaryos/dev/gicv3.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/ternaryos/dev/gicv3.cpp))
- `M ternaryos/tests/hal_boot_test.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/b8ca8e3b25ee94cbedf55702f4c639a6294b6565/ternaryos/tests/hal_boot_test.cpp))

All changed files versus main merge-base (including parents):

- `M README.es.md` — different; main blob `c8841644c021104df4669dab7eb463e19dd036bc`; head blob `8d182e85e8b7d166aef387f03da4c2be028adcaf`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `df5c15e80819f177e881013e259c88d73f881aa6`.
- `M README.pt-BR.md` — different; main blob `eb5330a286bf77bb58852c0279980f5ec7d4e4ff`; head blob `26264e2e8dd3f97619214a0f1cfdef0269b6af35`.
- `M README.ru.md` — different; main blob `3a6d8f75da0666321fbd15615cd11fe9bde2067b`; head blob `9c945651edccb970fb211b0b9318dfc6bb5c891e`.
- `M README.zh-CN.md` — different; main blob `f724ab1b425f4b7be0627e116da2b38f2cfce170`; head blob `f0ab3b12d996474c8e82e832164bbadc384f33e7`.
- `M docs/BUILDABLE_NEXT_STEPS.md` — different; main blob `c5f0f4525050f48e78d603468f479ed590549e97`; head blob `14848ad73f9e9f19fb136ef166d7bb47f64f9668`.
- `M docs/HANDOFF.md` — different; main blob `ac7989fa47ed2cfade45b53b6466067d4187a171`; head blob `6c928d0b148cd78d756f72ff777affa5053ca695`.
- `M docs/ROADMAP.md` — different; main blob `f0f754ee551f65bd99b0686d46322c6ec0b26acc`; head blob `38bd9810e8b2e87e0f8aa027fbede686db0ac55b`.
- `M docs/architecture/OVERVIEW.md` — different; main blob `fde9f33e7db5d6e2383c876cb064f21923c3a6a3`; head blob `b1a2c73a2696ffccb194a7b09b3dc9abc28cd98f`.
- `M docs/explanation/T81_FOUNDATION_PROJECT_PROFILE.md` — different; main blob `077c5adf8cfbf3c2de26f783b7e6e989fbf357ea`; head blob `a5e21c9d5476a0e1afa03026e7453af7c94567bf`.
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_A_EXECUTIVE_SUMMARY.md` — absent_main; main blob `None`; head blob `ce0324dfe58ff118e87d9520444dde34e0fcb1ed`.
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_B_DOCUMENT_INVENTORY_TABLE.md` — absent_main; main blob `None`; head blob `6a18ce65de7aea7ac5b6a58a2f4878420ac49e9b`.
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_C_STALE_CLAIMS_REGISTER.md` — absent_main; main blob `None`; head blob `721b1e6bb96716b38c4a243405da3f3348dd19fe`.
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_D_CROSS_DOCUMENT_CONFLICT_REGISTER.md` — absent_main; main blob `None`; head blob `67f87cd533adbef9fda06bb2836d7feb41a9bced`.
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_E_PRIORITIZED_REMEDIATION_PLAN.md` — absent_main; main blob `None`; head blob `26aca2055fb1858f54d7f5fc10033a16314ed442`.
- `A docs/records/audits/2026-03_documentation_refresh/OUTPUT_F_UPDATED_DOCUMENTS/docs/architecture/OVERVIEW.md` — absent_main; main blob `None`; head blob `61372990cad3bf2b015ef9360feefbad77196193`.
- `A docs/records/audits/2026-03_documentation_refresh/README_MITIGATION.md` — absent_main; main blob `None`; head blob `af19f82b0ebf85ed043a117dbd57f21774feea54`.
- `M scripts/governance/check_docs_structure.py` — different; main blob `8637fab6dda14837e1c3a8b7f35d764bae033f86`; head blob `8d18e6d9b05c1ef4849a28c0ba66c28d07e37006`.
- `M ternaryos/dev/gicv3.cpp` — different; main blob `a3e657f2ccc243478a23212a3ceba26ebb54bf7d`; head blob `67e7b835d11c01fafc2fb53e0b9be26e4cb509fb`.
- `M ternaryos/tests/hal_boot_test.cpp` — different; main blob `fdcc3b332b6ca7b12792da9a7a0255e2ff179b1f`; head blob `327d2b58c6759403176d6113f7e9d2f87f16e573`.

Overlaps with other PR own deltas:

- #747: `ternaryos/dev/gicv3.cpp`
- #649: `README.md`, `scripts/governance/check_docs_structure.py`
- #576: `README.md`
- #508: `README.md`, `ternaryos/dev/gicv3.cpp`, `ternaryos/tests/hal_boot_test.cpp`
- #504: `README.es.md`, `README.md`, `README.pt-BR.md`, `README.ru.md`, `README.zh-CN.md`

Review submissions:

- greptile-apps[bot] / COMMENTED / reviewed `0cb24d18e955271fb7849f7b8f00aa017fef178f` / [review](https://github.com/t81dev/t81-foundation/pull/507#pullrequestreview-4031051516).

Failed or incomplete checks:


## PR 504: docs: Provide RFC compliance audit and fix status mismatch

- Head `cb208858bb9a77fcf5115592bdc74c1ca2e41317`; actual base `041309bcdfc2d85113f6a46e6349701e833e210d` (`main`); main merge-base `041309bcdfc2d85113f6a46e6349701e833e210d`.
- Purpose: RFC status/compliance audit and documentation patch. Decision: **NEEDS INVESTIGATION**.
- Dependency assessment: Overlaps 507 docs, 748 policy test, 650 RFC index and governance stack.
- Open heads included by ancestry: []; references in body (not necessarily dependencies): [].
- Evidence/limits: Twelve divergent paths include workflow, policy test and normative RFC status edits. Determine surviving authorized status corrections and preserve historical audit before narrowing or closing.
- Blob counts: {'different': 12}. Merge simulation exit 1; tree `f7f5b347e20e04057c9dc02c6ed397494fbaca62`; equals main: False.
- Review: No formal review; threads 0 current/0 outdated. Complete bodies and original review commit pins retained in JSON; unresolved does not establish current validity, and outdated does not prove corrected.
- CI: 31 pass, 4 fail, 7 skip; all latest named check URLs are in JSON.
- [Pinned source comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...cb208858bb9a77fcf5115592bdc74c1ca2e41317).

Own changed files:

- `M .github/workflows/ci.yml` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/.github/workflows/ci.yml))
- `M README.es.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/README.es.md))
- `M README.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/README.md))
- `M README.pt-BR.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/README.pt-BR.md))
- `M README.ru.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/README.ru.md))
- `M README.zh-CN.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/README.zh-CN.md))
- `M fs/block_backed_driver.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/fs/block_backed_driver.cpp))
- `M include/t81/types/T81Int128.hpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/include/t81/types/T81Int128.hpp))
- `M spec/rfcs/RFC-0055-hardware-target-profile-template.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/spec/rfcs/RFC-0055-hardware-target-profile-template.md))
- `M spec/rfcs/RFC-0056-google-axion-hardware-profile.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/spec/rfcs/RFC-0056-google-axion-hardware-profile.md))
- `M spec/rfcs/RFC_TEMPLATE_COMPLIANCE_AUDIT.md` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/spec/rfcs/RFC_TEMPLATE_COMPLIANCE_AUDIT.md))
- `M vm/tensor_helpers.cpp` ([pinned source](https://github.com/t81dev/t81-foundation/blob/cb208858bb9a77fcf5115592bdc74c1ca2e41317/vm/tensor_helpers.cpp))

All changed files versus main merge-base (including parents):

- `M .github/workflows/ci.yml` — different; main blob `42e7c304cca285abf7eb287848a743ceb1c017bd`; head blob `4ecce40b958a9b5d4c8f8f016758eef571d60dfb`.
- `M README.es.md` — different; main blob `c8841644c021104df4669dab7eb463e19dd036bc`; head blob `a3b19c529c17510332d9618eee53988927c72cc5`.
- `M README.md` — different; main blob `3d024477261f9e8bd099962314c7a6cea43287c5`; head blob `f0758cdf29cc4ef8eec9a99f55361610aa7d61dc`.
- `M README.pt-BR.md` — different; main blob `eb5330a286bf77bb58852c0279980f5ec7d4e4ff`; head blob `0750f38689977cc2d9100f5d55dd83d1ada57b26`.
- `M README.ru.md` — different; main blob `3a6d8f75da0666321fbd15615cd11fe9bde2067b`; head blob `27c9d44a0eb5d98640d3a32d05531d2f1602a462`.
- `M README.zh-CN.md` — different; main blob `f724ab1b425f4b7be0627e116da2b38f2cfce170`; head blob `0295e2d39e9cca3c509a67b19ac80cfd23a77882`.
- `M fs/block_backed_driver.cpp` — different; main blob `1b04d683847792169c2ca42d43adc81ea2130546`; head blob `28bd68e46acac4c3f7febd2b97a25f2fd2baaa1b`.
- `M include/t81/types/T81Int128.hpp` — different; main blob `25ce7982e742e21a0f229c245206cc9b4b737ba2`; head blob `5ad55f1b63c3814e4dc4fa806a49d5493a372743`.
- `M spec/rfcs/RFC-0055-hardware-target-profile-template.md` — different; main blob `a7e2c17ecc965dcaed221fa90272da556c2387c8`; head blob `a4b075a771ae3fae11b29e83f77a5ada52bf962e`.
- `M spec/rfcs/RFC-0056-google-axion-hardware-profile.md` — different; main blob `f3eb83afb0abee4a7e3e53592e826afd09f1aefd`; head blob `b25df6188de8574178aa30d0d7305343534bd724`.
- `M spec/rfcs/RFC_TEMPLATE_COMPLIANCE_AUDIT.md` — different; main blob `ae30bad146cc79a6be6e867d29c95611ff0a6337`; head blob `dc9f81e339970f6890488f733baf9436323a1fb4`.
- `M vm/tensor_helpers.cpp` — different; main blob `32554cd0e2fddc78968ec46f65cc8fdebf978030`; head blob `969ef16d5900a419551eef0b352271957012e8a7`.

Overlaps with other PR own deltas:

- #782: `.github/workflows/ci.yml`, `include/t81/types/T81Int128.hpp`
- #748: `fs/block_backed_driver.cpp`
- #747: `.github/workflows/ci.yml`
- #649: `.github/workflows/ci.yml`, `README.md`
- #603: `vm/tensor_helpers.cpp`
- #577: `.github/workflows/ci.yml`
- #576: `.github/workflows/ci.yml`, `README.md`
- #575: `.github/workflows/ci.yml`
- #574: `.github/workflows/ci.yml`
- #525: `.github/workflows/ci.yml`
- #514: `.github/workflows/ci.yml`
- #513: `.github/workflows/ci.yml`
- #508: `README.md`
- #507: `README.es.md`, `README.md`, `README.pt-BR.md`, `README.ru.md`, `README.zh-CN.md`

Review submissions:

- No formal submitted review returned; comments/receipts remain separately retained.

Failed or incomplete checks:

- [quality gate / required](https://github.com/t81dev/t81-foundation/actions/runs/23561916076/job/68611588945): completed / failure.
- [build / linux-arm64 / clang](https://github.com/t81dev/t81-foundation/actions/runs/23561916076/job/68606614899): completed / failure.
- [build / windows-x86_64 / clang-cl](https://github.com/t81dev/t81-foundation/actions/runs/23561916076/job/68606614881): completed / failure.
- [build / windows-x86_64 / msvc](https://github.com/t81dev/t81-foundation/actions/runs/23561916076/job/68606614859): completed / failure.
