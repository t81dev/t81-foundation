# PR #782: Scoped Cross-Review of Codex 128-Bit and Process-Capture Contributions

- **Branch / PR:** `fix/windows-ci-baseline-repairs` (PR #782)
- **Code Head Reviewed (unchanged):** `a27c580882445a0f9942bad5d681eab34417b8e1` (preceded by `27901e728fe63812d8c64b8c20d821a20581300e`)
- **Windows CI-Verified Baseline SHA (byte-identical 128-bit and capture files):** `125a1897b7e7e068ef6d576d97c487295c7203a7`
- **Codex Commits Reviewed:**
  - `520142114` — `fix(windows): probe int128 link capability and supply portable arithmetic`
  - `6b47cac59` — `fix(cli): retain private exclusive process capture objects`
  - `cbb324ad4` — `style(cli): apply CI clang-format 18 spacing`

## Authorship Disclosure and Scope

This record is a **cross-review within the shared PR #782 repair effort**, authored by Gemini. It is **not** third-party independent scientific or security validation.

Within PR #782 (`origin/main..27901e728fe63812d8c64b8c20d821a20581300e`), commits are partitioned as follows:

- **Gemini-authored commits:**
  - `df06696ce` — `fix(windows): repair four Windows CI baseline failure clusters`
  - `33064f9d9` — `style(test): remove trailing blank line in t81_int128_test.cpp`
  - `a189ff1b9` — `fix(windows): export ffi_bridge_testlib symbols and repair run_process_capture`
  - `d9701d827` — `fix(windows): normalize fallback module_path with generic_string`
  - `125a1897b` — `test(windows): match generic_string diagnostic path in cli_pipeline_test`
  - `27901e728` — `test(windows): use exclusive per-run temp dir in enum metadata test`
  - `a27c58088` — `test(windows): unwind owned temp dir on failure and verify with injected failure`
- **Codex-authored commits reviewed here:**
  - `520142114` (128-bit link probe and portable `int128_t`)
  - `6b47cac59`, `cbb324ad4` (exclusive `detail::CaptureFile` and secure process capture)

## Inspected Source Paths

- **128-bit arithmetic and CMake probe:**
  - `cmake/Int128.cmake`
  - `cmake/probes/native_int128.cpp`
  - `include/t81/types/T81Int128.hpp`
- **Process capture:**
  - `tools/cli/core/process_capture_detail.hpp`
  - `tools/cli/core/process_capture.cpp`

## Findings Summary

1. **128-bit (`520142114`):**
   - `cmake/Int128.cmake` uses `try_compile` with `CMAKE_TRY_COMPILE_TARGET_TYPE=EXECUTABLE` on `cmake/probes/native_int128.cpp` (`volatile` operands for signed `__int128` multiply, divide, and remainder) so compilers that parse `__int128` without supplying signed runtime helpers (such as Windows `clang-cl`) select `T81_HAS_NATIVE_INT128=0`.
   - `include/t81/types/T81Int128.hpp` defaults standalone `_MSC_VER` consumers to `T81_HAS_NATIVE_INT128=0` and implements two's-complement 128-bit `+`, `-`, `*`, full-width truncating `/`, `%`, and explicit `std::domain_error` (divide by zero) / `std::overflow_error` (`INT128_MIN / -1`).
2. **Process capture (`6b47cac59`, `cbb324ad4`):**
   - `tools/cli/core/process_capture_detail.hpp` and `tools/cli/core/process_capture.cpp` share `t81::cli::detail::CaptureFile` across launcher and tests.
   - POSIX uses `O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW` with mode `0600`, immediate `::unlink()`, `::fchmod`/`::fstat` mode and `geteuid()` verification, and `F_DUPFD_CLOEXEC` promotion when `fd <= STDERR_FILENO`.
   - Windows uses `CreateFileW` with `CREATE_NEW`, `FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OPEN_REPARSE_POINT`, no sharing, a protected single-ACE current-user DACL verified post-open via `GetSecurityInfo`, and explicit `PROC_THREAD_ATTRIBUTE_HANDLE_LIST` inheritance. `read_all()` rewinds the retained handle without reopening pathnames.

## Executed Local Checks (macOS ARM64)

Run locally in `.cache/windows-ci-repair-worktree/build` (Release):

```sh
ctest --test-dir .cache/windows-ci-repair-worktree/build \
  -R "^(t81_int128_test|t81_process_capture_test|vm_ffi_bridge_test)$" \
  --output-on-failure
```

- `279 - vm_ffi_bridge_test`: Passed (`0.04 sec`)
- `290 - t81_int128_test`: Passed (`0.01 sec`)
- `291 - t81_process_capture_test`: Passed (`0.35 sec`)
- Result: `100% tests passed, 0 tests failed out of 3`

Also run locally at code head `a27c580882445a0f9942bad5d681eab34417b8e1` (`-O3 -DNDEBUG`):

```sh
ctest --test-dir .cache/windows-ci-repair-worktree/build \
  -R "^(e2e_enum_metadata_determinism_test|t81_cli_pipeline_test|t81_cli_repro_hash_test)$" \
  --output-on-failure
```

- `243 - e2e_enum_metadata_determinism_test`, `246 - t81_cli_pipeline_test`, `307 - t81_cli_repro_hash_test`: `100% tests passed, 0 tests failed out of 3`
- `scripts/ci/t81lang_repro_gate.py`: `T81Lang gates passed: fixtures=21 hash=ea52b9edf85a7392f2fb49814253d4f2972a79ee205a8be8246f1c40356dff71`

## Inspected CI Logs (Windows x86_64)

Inspected GitHub Actions logs from `T81 Foundation CI` run `43419383525` at commit `125a1897b7e7e068ef6d576d97c487295c7203a7` (where all 128-bit and capture files are byte-identical to `27901e728fe63812d8c64b8c20d821a20581300e`):

- **`build / windows-x86_64 / msvc` (job `131867735744`, `success`):**
  - `100% tests passed out of 404`
  - `291/404 Test #291: t81_int128_test`: Passed (`0.01 sec`; `int128 backend=portable; Release-active checks; failures=0`)
  - `292/404 Test #292: t81_process_capture_test`: Passed (`0.08 sec`; `symlink/reparse rejection: executed`; `exclusive creation, private permissions, retained-object read and partial cleanup: passed`; `Unicode observed via wmain UTF-16 arguments; argument fidelity, shell literals, cwd/env, zero/nonzero exits and concurrent 2 MiB streams: passed`)
- **`build / windows-x86_64 / clang-cl` (job `131867735702`, `success`):**
  - `100% tests passed out of 404`
  - `291/404 Test #291: t81_int128_test`: Passed (`0.01 sec`; `int128 backend=portable; Release-active checks; failures=0`)
  - `292/404 Test #292: t81_process_capture_test`: Passed (`0.07 sec`; `symlink/reparse rejection: executed`; `exclusive creation, private permissions, retained-object read and partial cleanup: passed`; `Unicode observed via wmain UTF-16 arguments; argument fidelity, shell literals, cwd/env, zero/nonzero exits and concurrent 2 MiB streams: passed`)

## Preserved Limitations

- **Validation boundary:** This is a cross-review within the shared PR #782 repair effort, not third-party independent validation.
- **128-bit (`T81Int128.hpp` / `cmake/Int128.cmake`):** Portable `int128_t` implements only the operator subset used by T81 (no bitwise/shift or `<`/`<=` operators); `cmake/Int128.cmake` passes `CMAKE_BUILD_TYPE` to `try_compile` (single-config generator assumption).
- **Process capture (`process_capture.cpp`):** No global child-process timeout or output-size cap; Windows sanitizer execution is not established (Windows CI runs Release MSVC and `clang-cl`).
- **Head currency:** Windows CI (`T81 Foundation CI` `43419383525`, jobs `131867735744` MSVC and `131867735702` `clang-cl`) passed 404/404 at `125a1897b7e7e068ef6d576d97c487295c7203a7`. Subsequent code commits `27901e728` and `a27c58088` modified only `tests/cpp/e2e_enum_metadata_determinism_test.cpp` (leaving Codex's 128-bit and capture files byte-identical to `125a1897b`), and Windows CI for `a27c58088` / this audit head has not yet completed at the time of writing.
