# Executed verification — independent integration audit

Executed 2026-10-25 America/New_York on macOS Darwin 25.5.0 ARM64,
Apple clang 21.0.0 (clang-2100.1.1.101), CMake 4.3.0, Ninja, Python 3.14.3.
Working tree: `/private/tmp/t81-h04-r02-integration`, based on main `01c85019`.
Explicit target requirement: C++17. Source/output hashes: `evidence/manifest.json`.

## Commands actually executed

```sh
cmake -S . -B /private/tmp/t81-h04-r02-clean -G Ninja -DT81_BUILD_TESTS=ON -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_TERNARYOS=OFF
```

First configure failed (exit 1): sandbox DNS prevented fetching Asio. No tests
were counted as passes. Retry used an existing, clean Asio checkout tagged
asio-1-30-2 at `12e0ce9e0500bf0f247dbd1ae894272656456079`:

```sh
cmake -S . -B /private/tmp/t81-h04-r02-clean -G Ninja -DT81_BUILD_TESTS=ON -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_TERNARYOS=OFF -DFETCHCONTENT_SOURCE_DIR_ASIO=/private/tmp/t81-accountable-build/_deps/asio-src
cmake --build /private/tmp/t81-h04-r02-clean --target t81_h04_r02_adaptive
/private/tmp/t81-h04-r02-clean/experiments/h04_r02_adaptive_representation/t81_h04_r02_adaptive --json-out experiments/h04_r02_adaptive_representation/results.json
ctest --test-dir /private/tmp/t81-h04-r02-clean -R h04_r02 --output-on-failure --no-tests=error
python3 experiments/h04_r02_adaptive_representation/oracle.py
python3 experiments/h04_r02_adaptive_representation/packing.py
python3 tests/ci/h04_r02_audit_regression_test.py --bin-path /private/tmp/t81-h04-r02-clean/experiments/h04_r02_adaptive_representation/t81_h04_r02_adaptive -v
python3 scripts/ci/run_research_regressions.py --build-dir /private/tmp/t81-h04-r02-research-clean --asio-source /private/tmp/t81-accountable-build/_deps/asio-src --jobs 4
```

Sanitizer configure/build/tests used a second fresh build directory:

```sh
cmake -S . -B /private/tmp/t81-h04-r02-sanitized -G Ninja -DT81_BUILD_TESTS=ON -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_TERNARYOS=OFF -DT81_ENABLE_ASAN=ON -DT81_ENABLE_UBSAN=ON -DFETCHCONTENT_SOURCE_DIR_ASIO=/private/tmp/t81-accountable-build/_deps/asio-src
cmake --build /private/tmp/t81-h04-r02-sanitized --target t81_h04_r02_adaptive
ctest --test-dir /private/tmp/t81-h04-r02-sanitized -R h04_r02 --output-on-failure --no-tests=error
```

Recovered R01 was extracted from `git archive def47c68 docs/research/continuity`
into `/private/tmp/t81-h04-r01-audit-source`; the deterministic gzip archive is
preserved in evidence. Its executed commands:

```sh
python3 /private/tmp/t81-h04-r01-audit-source/docs/research/continuity/probes/h04_radix_adaptation.py --output-dir /private/tmp/t81-h04-r01-audit-run
python3 /private/tmp/t81-h04-r01-audit-source/docs/research/continuity/probes/verify_h04_radix_adaptation.py --packet /private/tmp/t81-h04-r01-audit-run
python3 -m unittest discover -s /private/tmp/t81-h04-r01-audit-source/docs/research/continuity/probes -p test_h04_radix_adaptation.py -v
python3 scripts/ci/check_tisc_freeze_integrity.py
```

C++ files were formatted with repository clang-format configuration, then the
normal H04 build/CTest was rerun. Those final logs are retained alongside the
initial passing and sanitizer runs. This formatting change did not alter output.

## Outcomes and limits

| Check | Actually observed |
| --- | --- |
| Clean H04 CTest | 3/3 pass; nested Python audit 8/8 pass |
| ASan + UBSan H04 CTest | 3/3 pass, no sanitizer finding |
| Standalone Python audit | 8/8 pass |
| Exhaustive checks within audit | All 64 length-three traces over four boundary classes at each of 2 transition costs; exact rational optima and crossover |
| Block/comparator sweeps within audit | 36 configurations, shared-mode feasibility, metadata invariance and actual emitted costs |
| Packing | All 729 standalone six-trit values; all 1024 signed 10-bit values; seeded streams of length 0..150; canonicality rejection |
| Bounded research regression script | 8/8 pass (separate suite, no bypass) |
| Recovered R01 producer/verifier | Pass: 96 operations, 384 arms, 192 disabled equalities, 1596 sweep points |
| Recovered R01 tests | 19/19 pass; generated files match pinned reference bytes |
| Frozen TISC check | Pass: 209 opcodes, bitwise mappings and reserved range |

No executed test failed. The initial configure failed before tests because the
dependency could not be downloaded; the offline retry resolved it. Initial H04
build warned about C++17 extensions; explicit target features removed those
warnings. The wider research build reports duplicate-library linker warnings
for `libt81_core.a`; no test failure resulted and production link code was not
changed. Graphviz dot was unavailable to Doxygen discovery, but not required by
this build. The full repository test inventory was not built or run.

Portability review: changed C++ uses standard C++17 facilities, no platform
intrinsics, ABI assumptions or nonportable wide integers. CMake passes the exact
target path (including Windows .exe); Python uses subprocess argument arrays,
standard-library modules, temporary paths, and explicit little-endian framing.
Linux/Windows execution and x86 timings were not performed. Remote CI snapshots
are evidence of their reported states, not local cross-platform verification.
Main and the existing PRs were not uniformly green at inspection.

Reexecuting R01's own independent verifier does not independently prove every
R01 assumption. There are no new execution/energy/bandwidth measurements here.

## Original integration output SHA-256 (head 13f8377f; correction below)

- `results.json`: `b503e0c6eda24e6f282606721a8f66dcbc0f11baff05cd98c73279399e1c84e2`
- `packing_results.json`: `770b50b7836dcee0a4675de89494d370b4e2f55a47b1518d3d1848aaff02efb3`

Complete logs and source/snapshot/output hashes are in `evidence/`. To reproduce
elsewhere, replace the local Asio source path with a clean asio-1-30-2 checkout
or allow CMake to fetch that dependency. Use fresh build directories.


## PR #572 precision correction

Fresh clean and ASan/UBSan builds each pass all 3 H04 CTests; the standalone
Python audit passes 9/9, including 49 new shared-overhead scale configurations.
The first Python audit run correctly failed artifact parity after one crossover
changed at its final floating-point bits. The executable regenerated that field;
all final checks pass. Full commands, initial/final logs, reproducer and CI
snapshot details: [PRECISION_CORRECTION.md](PRECISION_CORRECTION.md).

Current `results.json` SHA-256: `5ea3ed12b16d94a9cdc86fccf808091da614d1b4f4ac6edc45e2330c7987e6e9`.
The original evidence manifest is preserved as
`history/manifest-13f8377f.json`; the current manifest records this correction.
