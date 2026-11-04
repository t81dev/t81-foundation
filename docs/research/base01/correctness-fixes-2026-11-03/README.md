# Verified BASE-01 correctness fixes and execution evidence

Release builds could hide SHA3 mismatches and disable the float-boundary and map-determinism assertions. Enabling the map checks exposed symbol iteration ordered by interned IDs instead of the API's documented canonical names. This change preserves the four verified corrections; it does not change the scientific workloads or the audited runtime revision.

- Tested starting baseline: `a98b22fb65c80f4225151cbcd5c6934ae076cbe1`.
- Committed code tested for publication: `4bba72c6654f749229a35245fe70b3cda2274ad7`, tree `d709da0b4e733a31c42a359c04a87be136bff42c`.
- BASE-01 measured/audited source remains `cda9135d36144d4de2771af174e52ac7b30a83c9` in a separate clean checkout. The corrected main-tree header is not substituted into that historical runtime.
- The later publication commit adds only this evidence directory; it does not change tested code.
- Author/operator: Codex, with prior implementation/review context. Model identity is not independently attested here. These are observed local results, not independent scientific certification.

## Preserved failure and correction

The original [26-pass/1-fail result](original-26-pass-1-fail.json), [map failure](original-map-failure.log), and subsequent [27-pass/0-fail result](corrected-27-pass-0-fail.json) are byte-identical retained files. The [original report](original-report.md) is also unchanged; its statements about publication state describe that earlier session.

The original failure is not replaced by the fresh run. Additional original SHA3/float/map failure-path mutation checks, commands and raw logs remain in the archive.

## Publication rerun

| Check | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Foundation Python tests | 25 | 0 | 0 |
| Resource-accounting fixture tests | 27 | 0 | 0 |
| Lane A contract/acceptance tests | 11 | 0 | 0 |
| Pinned native suites | 4 | 0 | — |
| Latest-state correctness/determinism suites | 27 | 0 | — |
| New Release failure-path guards | 3 | 0 | — |

Foundation, Lane A and R75 validate-only harnesses also passed. R75 reproduced all six builder ledgers and three corrupted-ledger rejections. Each Release guard rejected a mutant containing the original broken failure handling. Those three expected mutant-check failures are retained as successful negative controls, not hidden product failures.

Foundation executed two repeated writes and two retained verified reads for each of 4,096, 65,536 and 1,048,576 bytes of 0x42. Returned bytes and both references matched the independent Python `hashlib.sha3_512(payload).digest()[:32]` oracle. Its fresh receipt has `measurement_enabled: false` and `performance_samples: []`. No 100-operation timed cell was executed.

## Environment and commands

macOS 26.5.2 / Darwin 25.5.0, ARM64; Apple clang 21.0.0 (`clang-2100.1.1.101`), Python 3.14.3, CMake 4.3.0, Ninja 1.13.2. C++23 Release, `-O3 -DNDEBUG`, strict deterministic float on, LTO off. Benchmarks, TUI, examples, fuzz, DPE, TernaryOS and llama disabled. Pinned Asio is `asio-1-30-2` / `12e0ce9e0500bf0f247dbd1ae894272656456079`, supplied from a clean local cache after the original network fetch failed.

[COMMANDS.md](COMMANDS.md) lists exact rerun arguments, working directories and raw return codes. [publication-results.json](publication-results.json) records identities and counts. The archive preserves original commands, both runs' stdout/stderr logs, per-command JSON, compiler/link commands, source/build identities, raw CanonFS bytes, independent-oracle receipts, R75 records and fixture observations. Test adapters suppress unittest duration reporting; no benchmark or performance qualification is run.

To run the new regression guards after a normal correctness-only root configure:

```sh
cmake --build /path/to/build --target t81_release_sha3_guard t81_release_float_guard t81_release_map_guard
python3 tests/python/check_release_correctness_guard.py sha3 /path/to/build/t81_release_sha3_guard
python3 tests/python/check_release_correctness_guard.py float /path/to/build/t81_release_float_guard
python3 tests/python/check_release_correctness_guard.py map /path/to/build/t81_release_map_guard
```

The three guards are registered with CTest as well. They compile with `NDEBUG` even for Debug configurations, include the actual corrected tests, and require the expected nonzero failure and diagnostic. No assertion expectations or measured workload inputs are weakened.

## Evidence identity and scope

[evidence.tar.gz](evidence.tar.gz) contains `original/` and `rerun/`. [archive-files.sha256.json](archive-files.sha256.json) identifies every archived file. [SHA256SUMS.json](SHA256SUMS.json) identifies each published sibling file. Tar metadata and gzip timestamps are normalized; retained file bytes are unchanged.

The original manifest covered 393 local files, including binaries and working build metadata. It is preserved inside the archive with `original/PUBLICATION_SCOPE.json`, which explicitly lists omitted local build artifacts and any regenerated metadata whose original bytes were unavailable. It is not presented as a manifest of a complete published build tree. Raw historical logs and the 26/1 and 27/0 records are preserved and checked against their original hashes; no historical artifact is reconstructed. Fresh build metadata is labeled under `rerun/build-metadata/`. Hashes establish content identity, not authenticity or independent validation.

## Limits

No remaining failure in these executed suites. Linux, Windows, x86_64 and cross-host reproducibility were not executed here. The MSVC-specific assertion routing in the guard is untested on Windows. Existing duplicate-archive linker warnings remain recorded. Foundation VM labels remain provisional; this patch does not promote governance or qualification status.

No physical-host qualification, B1 timed campaign, pilot, protected benchmark, PMU/energy measurement, merge, protocol activation or authorization change occurred. Commit messages include `[skip ci]` to avoid automatic benchmark-containing workflows; no CI pass is claimed. Ordinary cross-platform CI remains unexecuted in this publication task.
