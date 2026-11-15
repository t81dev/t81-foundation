# Windows CanonFS and VM Debug correctness repairs

Both requested defects are repaired in source `e09540caeb826df7eaeb9e876be5beb745784921`, based on PR #700 reference `edaa3fd67fdc08b4979faaf0bf8754866fab80c5`. Publication adds evidence only. This separate PR is stacked on #700's branch so its diff contains only these repairs and their evidence; neither PR is merged.

## Corrections and compatibility

The persistent driver now uses `h1-` followed by 64 lowercase hexadecimal digits encoding the original 32 CanonHash bytes, with `.blk` or `.cap` appended. The resulting component is 71 ASCII characters and contains no Windows-reserved characters, reserved DOS basename, or trailing dot/space. The codec is reversible and distinguishes identities on case-insensitive filesystems. Canonical Base-81 serialization, the payload-only hash function (SHA3-512 truncated to 32 bytes in this implementation), public references, and stored payload bytes are unchanged. This remains the actual persistent driver. Snapshot inventories decode portable stems before reporting canonical content identifiers; physical snapshot paths/manifests are storage-format dependent.

**Explicit migration boundary:** new empty roots receive `.canonfs-format` containing exactly `t81-canonfs-hex-v1\n`. Nonempty unmarked stores, unknown/partial/extended markers, mixed legacy object/capability names, and unsupported persisted parity shards fail closed. Old readers/writers must not open a new-format root. Preserve the old root; any migration requires an old reader and new writer into a separate empty root, independent byte/hash/reference checks, and explicit recreation/verification of authorized capability masks. No automatic migration or migration utility is supplied. Do not restore legacy snapshot paths directly into the new live store. See [the storage specification](../../../../spec/supplemental/canonfs-spec.md).

The VM correction deletes twelve lines: checks/counter updates after an unconditional `SecurityFault`/`break`, and their now-unused local counter. The existing protected `WriteCode` denial and Axion event remain. The application pass, warning policy, `/WX`, and other execution paths are unchanged. A new always-active test exercises mixed batches with valid and invalid code targets, verifies `SecurityFault`, unchanged register/memory/meta-write count on denial, and successful allowed register/memory/Noop batches.

## Observed execution

| Native platform / compiler | Configuration | Before: original 27 suites | Final: original suites | New regressions | Interchange | NDEBUG guards | Broken guards rejected |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Ubuntu 24.04 x86_64 / GCC 14.2.0 | Debug | 27/27 | 27/27 | 2/2 | 1/1 | 3/3 | 3/3 |
| Ubuntu 24.04 x86_64 / GCC 14.2.0 | Release | 27/27 | 27/27 | 2/2 | 1/1 | 3/3 | 3/3 |
| Windows Server 2022 x64 / MSVC 19.44.35229 | Debug | 17 passed, 2 failed, 8 blocked (union of two jobs) | 27/27 | 2/2 | 1/1 | 3/3 | 3/3 |
| Windows Server 2022 x64 / MSVC 19.44.35229 | Release | 24 passed, 3 failed | 27/27 | 2/2 | 1/1 | 3/3 | 3/3 |
| macOS 26.5.2 ARM64 / Apple clang 21.0.0 | Release | 27/27 | 27/27 | 2/2 | 1/1 | 3/3 | Not rerun here |

Final reported configurations have **zero test failures and zero blocked suites**. Native run status and fixture identity assertions must also pass; a missing executable is never a pass.

The narrowly scoped `.gitattributes` rule preserves CanonFS interchange fixture bytes on Windows checkouts, and that existing test now closes both input streams before deleting its fixture tree. Neither change alters a production call path.

The two new tests are `canonfs_storage_format_test` and `t81_vm_metarefine_writecode_denial_test`. Their checks execute under NDEBUG via `T81_TEST_CHECK`. Storage coverage includes a golden byte-to-key vector, all 256 byte values, malformed/uppercase keys, both originally failing Windows payloads, repeated writes, in-memory/persistent identity equality, independently generated Python SHA3 expectations, byte equality after reopening, portable capability persistence, and legacy/mixed/incomplete format rejection. Existing corruption tests now locate payloads through the storage codec without changing their payloads or verification expectations.

The original 27-suite target set is retained verbatim in the archived controller. All three Release failure guards reject broken behavior; all three deliberately regressed guard mutants are rejected. Windows also passes the native MSVC Debug configuration regression. Linux CMake Debug retains repository-wide `-O3 -DNDEBUG`; it is not an unoptimized/assertions-enabled configuration. MSVC Debug uses `/Od /RTC1 /MDd`, retains `/DNDEBUG`, and compiles the repaired VM under `/W4 /WX`. Full flags and compiler versions are retained in raw build records and compile-command metadata.

Both payloads' canonical Base-81 text was compared across the original failed Windows probes, repaired native Windows Debug/Release probes, and repaired macOS CLI output. Every comparison agrees. Independent Python `hashlib.sha3_512(payload).digest()[:32]` expectations match the actual stored filenames and byte payloads. These observations and exact UTF-8 identity bytes are archived as `execution/identity-observations.json`.

On macOS ARM64, actual CLI put/get/snapshot additionally verifies returned-byte equality and that the manifest pairs a portable physical path with the original canonical Base-81 content identifier. Full CLI/snapshot behavior on Windows, Linux Clang, MinGW, clang-cl, other locales/Windows versions, Windows/Linux ARM64, and macOS Debug are untested. Existing legacy-store migration is intentionally a manual compatibility boundary, not validated automated migration.

The first repaired-code run passed the original 27 suites and both new tests on all four Linux/Windows configurations. An additional Windows interchange test failed its byte-for-byte JSON golden check. A second run using exact pinned fixture bytes passed that check, then failed deleting its fixture directory because two input streams remained open. Both failures are preserved unchanged. The final source pins only the CanonFS fixture subtree against Git line-ending conversion and closes those two test streams. The final native workflow uses normal platform checkouts (no global line-ending override), independently checks every one of the 22 fixture files against its Git blob, and passes the entire interchange test in Debug and Release. `execution/failure-history.json` and all initial/second raw ZIPs retain these intermediate failures.

## Reproduction and raw evidence

- [Native run, initial controller](https://github.com/t81dev/t81-foundation/actions/runs/41762988241), controller `40085b73392a12e3cb44c17f03c0163b3323825b`.
- [Native run, exact fixture bytes](https://github.com/t81dev/t81-foundation/actions/runs/41764565466), controller `4080f780d41382c4ba8b922c9799ab39f720f674`.
- [Final native run](https://github.com/t81dev/t81-foundation/actions/runs/41765234128), controller `7186a35084c90c0f9e36d8faa48fb6c516c2ea73`.
- The first two runs check out source `ca97b9cbdc2c76a6ab5b830a2eb4df0ceae5566c`; the final run checks out `e09540caeb826df7eaeb9e876be5beb745784921` with the fixture/test-handle corrections. Every source checkout is separate from the controller and tracked-clean before/after execution. These are real GitHub-hosted Linux/Windows executions, not emulation or physical-host qualification.

[COMMANDS.md](COMMANDS.md) records exact commands/argument vectors and working directories, including compiler-version commands. [results.json](results.json) preserves structured outcomes. [evidence.tar.gz](evidence.tar.gz) contains original raw artifact ZIPs, per-command stdout/stderr, build and compiler/link commands, CMake caches, source pins, workflow/job logs, all three controllers, supplemental macOS scripts/output, and the original Windows failure ZIPs from PR #700. Original ZIPs match GitHub's published SHA-256 digests and internal file manifests. [archive-files.sha256.json](archive-files.sha256.json) covers archive members; [SHA256SUMS.json](SHA256SUMS.json) covers published sibling files. Hashes identify content, not truth or authentic provenance. Native executables are not archived.

The original macOS 26-pass/1-fail result, corrected 27-pass/0-fail result, publication records, and PR #700 portability archive remain byte-identical. Before repair, Windows Release passed 24/27 with three persistent-path failures; Windows Debug's main job passed 16, failed two, and could not build nine. A separate passing ISA run yields 17 passed/two failed/eight blocked across those original Debug jobs; that union is not a fabricated single-job result. The old failing source `35f73ac9751423f9ac083c7f4b84def34d810443` has identical runtime/test/config bytes to the requested reference `edaa3fd...`.

Codex authored these corrections and executed these checks with prior context. Passing correctness tests do not establish independent scientific validation or a performance result. Historical BASE-01 source commit `cda9135d36144d4de2771af174e52ac7b30a83c9` and its artifact identities are unchanged. No protected benchmark, timing probe, campaign, qualification pilot, physical-host qualification, protocol activation, governance change, merge, or measured workload change was performed. Workflow changes are confined to the isolated validation branch; the repair PR does not change CI workflow definitions.
