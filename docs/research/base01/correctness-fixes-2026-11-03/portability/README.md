# PR #700 native cross-platform correctness results

**Status: PARTIAL for the broader Windows regression slice.** The four requested fixes and all three NDEBUG failure-path guards pass on the executed Linux, Windows and macOS configurations. Persistent-storage failures and the audited VM Debug build blocker remain visible; this is not an all-platform green result.

Tested code: `35f73ac9751423f9ac083c7f4b84def34d810443`. This source commit is followed only by evidence-publication commits. Starting requested head: `d953a094126d6315fe5c50280fc40deefb5899e3`. The original four corrected files are byte-identical to that head. The audited BASE-01 runtime pin remains `cda9135d36144d4de2771af174e52ac7b30a83c9`; no production source under `fs/`, `vm/`, `isa/` or `include/` changed in this portability follow-up.

## Final observed coverage

| Native platform / compiler | Configuration | Native suites passed | Native failures | Unexecuted / blocked | Guards | Broken-behavior mutants rejected |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Ubuntu 24.04 x86_64 / GCC 14.2.0 | CMake Debug | 27 | 0 | 0 | 3/3 | 3/3 |
| Ubuntu 24.04 x86_64 / GCC 14.2.0 | Release | 27 | 0 | 0 | 3/3 | 3/3 |
| Windows Server 2022 x64 / MSVC 19.44.35229 | Debug | 17 | 2 | 8 | 3/3 | 3/3 |
| Windows Server 2022 x64 / MSVC 19.44.35229 | Release | 24 | 3 | 0 | 3/3 | 3/3 |
| macOS ARM64 / Apple clang 21.0.0 | Release supplemental rerun | 27 | 0 | 0 | 3/3 | historical controls preserved |

Windows Debug's main matrix job executed 18 suites: 16 passed, two failed, nine were not built. A [separate native ISA Debug run](https://github.com/t81dev/t81-foundation/actions/runs/41748962832) then passed the independently buildable ISA IO suite, reducing unexecuted coverage to eight. The table is unique-suite coverage across those two jobs, not a fabricated single-job result. The actual MSVC Debug-configuration regression also passed in both Windows configurations.

The four affected suites are `t81_sha3_test`, `test_deterministic_float_boundaries`, `test_map_determinism`, and `t81_determinism_containers_test`. All four passed in every final configuration. The map regression asserts canonical symbol order `alpha, beta, gamma` after interning/inserting in a different order. It therefore checks names rather than incidental symbol IDs across the tested compilers.

## Demonstrated corrections

1. **MSVC Debug build flags:** the initial run stopped before tests with D8016 because global `/O2` conflicted with CMake's Debug `/RTC1`. `cmake/Compiler.cmake` now omits `/O2` only for MSVC Debug. Existing Release flags and NDEBUG policy remain unchanged. `t81_msvc_debug_configuration` compiles and executes a minimal native Debug target using the actual compiler and the repository flag module; no platform mock supplies its pass.
2. **Headless assertion guards:** the intermediate Windows Debug float/map guards timed out after 15 seconds without the expected diagnostic. The guard now routes Debug CRT assertion reports to stderr and disables interactive abort reporting. [Microsoft documents that Debug assertion reports default to a message window](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/crtsetreportmode?view=msvc-170). All final guards terminate and produce their expected diagnostic. Linux assertion guards returned `-6`; Windows returned `3`; SHA3 failure returned `1`. The checker requires nonzero termination plus the expected message, not a single portable exit/signal number. English runner diagnostics were tested; other locales/CRTs were not.
3. **Windows evidence cleanup:** the native diagnostic wrapper captured a sharing violation from `remove_all` while the test's `ifstream` was still open. The test now closes the stream after reading its contents. The unchanged evidence-schema and hook-trace checks then pass, including all 15 checks in `t81_determinism_evidence_test`. No runtime or scientific workload logic changed.

The [focused patch](portability-corrections.patch.gz) contains only these build/test corrections and their regression registration. The existing guards, mutant controls and evidence collector provide actual failure-before/pass-after coverage. Original failures remain archived.

## Remaining failures and limits

**Windows persistent CanonFS:** `canonfs_read_verify_test` and `canonfs_identity_contract_test` fail in both final Windows configurations. `t81_vm_tloadhash_decodefault_determinism_matrix_test` also fails in Release; it is unexecuted in Debug because the VM build is blocked. A native diagnostic captured canonical names containing `<` or `>` for the first two test payloads, followed by `write_success=0`, `errno=22` / `Invalid argument`. Raw hashes and messages are retained. The existing persistent driver uses canonical Base-81 text directly as a filename. No filename remapping, input replacement, storage-contract migration or fallback to the in-memory driver was introduced to make these tests pass.

**Windows Debug VM build:** `/WX` rejects C4702 at `vm/vm.cpp:2213–2222`, in code following an unconditional `break`. The audited VM source and warning policy were left intact. These eight targets remain unexecuted in Debug: `t81_vm_determinism_property_test`, `t81_vm_deterministic_fault_test`, `t81_vm_fault_family_determinism_matrix_test`, `t81_vm_tloadhash_decodefault_determinism_matrix_test`, `t81_axion_log_determinism_test`, `t81_vm_system_registers_deterministic_test`, `t81_vm_workload_determinism_tiers_test`, and `test_resource_monitoring`. A missing executable is blocked, never counted as a pass.

Only GCC 14.2.0, MSVC 19.44.35229 and the retained/supplemental Apple clang 21 environment were executed. Linux Clang, clang-cl, MinGW, other Windows versions, other architectures/locales and Debug macOS were not tested. Native binaries were not archived by these jobs; source pins, complete compiler/link commands, returned diagnostics, fixture records and raw workflow artifacts are retained. No independent scientific certification or broader performance claim follows.

## Runs, configuration and preservation

- [Initial native matrix](https://github.com/t81dev/t81-foundation/actions/runs/41745122920): source `d953a094126d6315fe5c50280fc40deefb5899e3`, controller `87d24af24e6feafd55889d77b4e784be32377f73`.
- [MSVC Debug correction and diagnostics](https://github.com/t81dev/t81-foundation/actions/runs/41746435152): source `b8e127728ab4b34dce959f6da70784a47e62567f`, controller `7c6ef50c5adde0a507910af3deb3f6c8d579f4e6`.
- [Final native matrix](https://github.com/t81dev/t81-foundation/actions/runs/41747549827): source `35f73ac9751423f9ac083c7f4b84def34d810443`, controller `7abb7439056a6dd2e3e900aff4471bb6101602d3`.
- [Independent Windows Debug ISA](https://github.com/t81dev/t81-foundation/actions/runs/41748962832): same final source, controller `85bf7f825441732566cdca8edc9934960a7ce128`.

These were real native executions on GitHub-hosted Linux and Windows guests, not simulated platform results and not physical-host qualification. No local Linux/Windows runtime was available. An isolated `validation/pr700-native-portability` branch supplied a correctness-only replacement of an existing manually dispatchable workflow; default-branch and PR workflow definitions were not changed. Every source checkout was separately pinned and checked against its expected commit. All controller snapshots, dispatch metadata, initial failures and final outcomes are retained.

Configurations disable benchmark, TUI, examples, fuzz, DPE, TernaryOS and llama targets and build only explicitly named correctness targets. The CMake Debug label on Linux still includes the repository's global `-O3 -DNDEBUG`; it is not claimed to be an unoptimized/assertions-enabled profile. Corrected MSVC Debug uses `/Od /RTC1 /MDd` without global `/O2`; `/DNDEBUG` remains. Release guards explicitly define NDEBUG and include the actual tests; the critical assertion tests explicitly enable their checks. Full effective flags and compiler/Python/CMake/Ninja versions are in [results.json](results.json) and the raw build metadata. Ordinary build-tool durations are not performance samples.

[COMMANDS.md](COMMANDS.md) renders the exact final recorded argument vectors and working directories. [history.json](history.json) separates each execution revision and distinguishes executed, failed and not-built suites. Older raw count fields named `run` included planned blocked records; this summary derives actual execution counts from recorded invocations instead of treating those placeholders as executed tests.

[evidence.tar.gz](evidence.tar.gz) preserves all 13 original GitHub artifact ZIPs, workflow/job logs, run metadata, controller source, diagnostic probes and the supplemental macOS logs. Each downloaded ZIP matched its GitHub API SHA-256 digest and its internal published file manifest (native Windows separators normalized only for ZIP-member lookup). [archive-files.sha256.json](archive-files.sha256.json) covers every archive member; [SHA256SUMS.json](SHA256SUMS.json) covers published sibling files. Archive timestamps are normalized, retained file/ZIP bytes are unchanged. Hashes establish content identity, not truth or authentic provenance.

The existing macOS report, original 26-pass/1-fail result, corrected 27-pass/0-fail result, raw archive and parent manifests remain byte-identical. Codex performed these checks with prior author/reviewer context. No audited BASE-01 runtime change, workload change, protected benchmark, timed campaign, qualification pilot, physical-host qualification, merge, governance change or protocol activation occurred. Standard benchmark-containing CI remains skipped; these explicitly authorized correctness-only jobs are reported with their actual failures and limits.
