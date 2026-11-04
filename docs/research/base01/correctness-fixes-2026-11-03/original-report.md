# BASE-01 correctness execution report — November 3, 2026

Observed results from this session. Codex executed these checks with prior author/reviewer context; this is not independent scientific certification. No results below are inferred from historical passing logs.

## Source and environment

- Latest main fetched: `a98b22fb65c80f4225151cbcd5c6934ae076cbe1`. Isolated checkout: `/private/tmp/t81-base01-correctness-latest`.
- BASE-01 runtime source: `cda9135d36144d4de2771af174e52ac7b30a83c9`, as required by existing harnesses. Its tracked tree remained clean throughout and after execution.
- Latest-state regression tests use main plus the four-file uncommitted correction patch recorded here. Corrected source hashes are in `correction-identities.json`; this is not a claim of an unmodified-main regression pass.
- macOS 26.5.2 / Darwin 25.5.0, ARM64. `/usr/bin/c++` and `/usr/bin/cc`: Apple clang 21.0.0 (clang-2100.1.1.101), arm64-apple-darwin25.5.0. CMake 4.3.0, Ninja 1.13.2, Python 3.14.3.
- Release, C++23, `-O3 -DNDEBUG`, strict deterministic float enabled, LTO off. Benchmarks, TUI, examples, fuzz, DPE, TernaryOS and llama disabled.
- Asio `asio-1-30-2`: `12e0ce9e0500bf0f247dbd1ae894272656456079`. Initial configure failed because GitHub DNS was unavailable. Recovery used a clean cached Git checkout at exactly this tag/commit via `FETCHCONTENT_SOURCE_DIR_ASIO`; no dependency source was edited.
- The original working checkout remains at `04f7b30ec4a2171380ead964402147d485be24ec`, with the same four corrections applied for review. Baseline bytes of all four corrected files match latest main; existing untracked work was preserved.

## Results

| Check | Observed result | Evidence |
| --- | --- | --- |
| Foundation validate-only harness | PASS; 1,031 codec inputs; 3 CanonFS sizes; 6 VM builders; measurement_enabled=false; performance_samples=[] | foundation-evidence/receipt.json |
| Foundation contract suite, all native/raw options supplied | 25 passed, 0 failed/errors, 0 skipped | foundation-tests.json; logs/foundation-tests.log |
| Resource-accounting fixed-fixture suite | 27 passed, 0 failed, 0 skipped | resource.json; logs/resource-validation.log |
| Lane A validate-only harness | PASS; measurement_authorized=false | lane-a-evidence/receipt.json |
| Lane A contract/acceptance suite, native option supplied | 11 passed, 0 failed/errors, 0 skipped | lane-a-tests.json; logs/lane-a-tests.log |
| Pinned native arithmetic, workload determinism tiers, CanonFS driver, resource monitoring | 4 suite invocations passed | logs/pinned-*.log |
| R75 correctness verifier | 6 builder ledgers passed; 3 corrupted-ledger rejections; measurement-flag denial; performance_collection=false | r75-evidence/summary.json; logs/r75-validation.log |
| Initial latest-state regressions after enabling assertions | 26 passed, 1 failed (map symbol order) | latest-regressions.json; logs/latest-test_map_determinism.log |
| Final latest-state regressions after comparator correction | 27 passed, 0 failed | latest-regressions-final.json; logs/latest-final-*.log |

Counts of Python test cases, native suite invocations and R75 ledger checks are different units and are not pooled into an assertion count. Native suites include further assertions (for example codec: 24 checks, software float math: 126 checks, evidence collector: 15 checks). This was a targeted correctness/determinism run, not the entire repository CTest matrix.

## CanonFS observations

The Foundation harness executed two repeated writes and two verified reads per payload, retaining returned references/vectors. Both references matched the independent Python hashlib SHA3-512-prefix expectation, and every returned byte matched the 0x42 payload. This does not establish behavior of the protected 100-operation timed cells.

| Bytes | Expected CanonHash (first 32 SHA3-512 bytes) | Retained references / reads |
| ---: | --- | ---: |
| 4096 | `0ef03619c896d9694f13b525bde07582dcc6a6c1b04fe2d8e3caf5ee4daba40a` | 2 / 2 |
| 65536 | `9ad5626a6913267d510d36920bd68816f24f0c2ee7070a9f0c659f340647c351` | 2 / 2 |
| 1048576 | `4c1182f19f5163adc36a3c06f51467cf846c49fa21619d4137c7f3ca7fdeeea5` | 2 / 2 |

Full payload SHA-256 identities, independent expected CanonHashes and retained-byte accounting are in `foundation-evidence/receipt.json`. Raw returned bytes are retained in `foundation-evidence/native.json.gz`; `test_canonfs_independent_hash_and_bytes` also passed.

## Failures, diagnosis and corrections

1. SHA3 test failure signaling disappeared under NDEBUG. A temporary copy with a deliberately incorrect expected empty-input digest printed a mismatch and then claimed all tests passed, returning 0. Replacing assert(false) with std::exit(EXIT_FAILURE) makes the same corruption return 1. The original four SHA3 vectors pass. See logs/sha3-corrupt-before.log and logs/sha3-corrupt-after.log.
2. Float-boundary regression assertions disappeared under NDEBUG. Enabled assertions at the top of the test translation unit using the existing repository convention. A temporary unconditional-assertion sentinel returned 0 before and SIGABRT (-6) after. The genuine boundary test passes with assertions active.
3. Map determinism regression assertions disappeared under NDEBUG. Applied the same test-only correction. The sentinel returned 0 before and SIGABRT (-6) after. These deliberate sentinel failures are expected validation checks, not additional product failures.
4. With map assertions active, the real test failed at the expected alpha-first iteration order. T81Map::iter_sorted documented canonical-name ordering for T81Symbol but compared natural symbol order (interned numeric identity). Added the same symbol-name comparator already used by canonical serialization, leaving other key ordering unchanged. The existing map test and container test now pass. No test expectation or workload input was weakened.

Patch: `corrections.patch`; source identities: `correction-identities.json`; sentinel evidence: `assertion-checks.json`. The fixes are applied in both the isolated latest checkout and the original working workspace. No change was made to the pinned audited source tree.

## Limits and remaining blockers

- No remaining failure in the executed final suite. Initial configure failure and original map failure are preserved, not overwritten.
- macOS ARM64 is the only platform executed here; Linux, Windows, x86_64 and cross-host reproducibility are not established by this run.
- Foundation VM results remain labeled PROVISIONAL; the R75 verifier reproduced its source-derived ledgers but does not automatically supply independent review or promote governance status.
- The resource collector exercises fixed correctness fixtures and actual accounting APIs. Its passing result is not a hard RSS-enforcement or physical-host qualification claim.
- No protected B1 campaign, timer qualification probe, pilot, warm-up campaign, performance benchmark, PMU or energy measurement was executed. Negative CLI calls use measurement flags solely to confirm denial.
- No 100-read retention qualification, RSS containment proof, campaign permit or physical host qualification follows. No ternary superiority, production readiness or provenance authenticity is established.
- Build output contains existing Apple linker warnings about duplicate archives. AVX2/FMA capability probes fail on ARM64 as expected; the supported ARM64 build succeeds.
- No PR, merge, protocol activation or governance change occurred.

## Commands and raw evidence

`COMMANDS.md` and `commands.json` list exact recorded argv, working directories, BASE01 overrides and exit codes. `logs/` retains combined outputs and verbose compiler/link commands. Each build directory retains CMakeCache.txt and compile_commands.json. Unit tests use the documented duration-free unittest adapter, with optional native/raw inputs supplied; no skipped case is represented as passed.

Initial setup commands, preceding the recording wrapper:
```sh
git fetch origin main
git rev-parse origin/main
git worktree add --detach /private/tmp/t81-base01-correctness-latest origin/main
# That worktree attempt failed because the original .git/worktrees directory is not writable.
git clone --no-hardlinks --no-checkout /Users/t81dev/Code/t81-foundation /private/tmp/t81-base01-correctness-latest
git -C /private/tmp/t81-base01-correctness-latest checkout --detach a98b22fb65c80f4225151cbcd5c6934ae076cbe1
git -C /private/tmp/t81-base01-source-build/_deps/asio-src rev-parse HEAD 'asio-1-30-2^{commit}'
git -C /private/tmp/t81-base01-source-build/_deps/asio-src status --porcelain
git clone --no-hardlinks /private/tmp/t81-base01-source-build/_deps/asio-src /private/tmp/t81-base01-execution/asio
```

Post-change validation: `git diff --check` passed; `git -C /private/tmp/t81-base01-execution/source status --porcelain` was empty.

`SHA256SUMS.json` identifies the retained report/log/receipt/patch/driver files, CMake caches and compile commands, and relevant executables/archives. Hashes establish content identity only.
