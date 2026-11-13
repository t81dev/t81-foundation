# PR #782: secure process capture correction

Parent: `a189ff1b9f6bace9224a23810b78c533a63e33a2`.
PR base: `88300f14c1da1066ad6db25d31be0e8e912b3ef0`.

The same Codex session authored the earlier int128 correction and this repair;
these are executed regression checks, not independent scientific validation.
Gemini authored the preceding process-capture implementation. This correction
addresses the subsequent capture-creation and Windows-coverage findings.

## Ownership and failure handling

The launcher and tests use the same move-only internal capture primitive.
POSIX uses `O_CREAT|O_EXCL|O_RDWR|O_CLOEXEC|O_NOFOLLOW`, mode 0600,
immediate unlink, checked owner/mode, and retains the descriptor through child
execution and reading. Windows uses `CREATE_NEW`, a checked protected current-user
DACL, noninheritable handles, no sharing, `FILE_FLAG_OPEN_REPARSE_POINT` and
`FILE_FLAG_DELETE_ON_CLOSE`. The resulting DACL is verified before use. Only
explicitly duplicated standard handles enter the child's handle list.

Reads rewind the retained objects; they never reopen a pathname. Existing paths
are neither truncated nor removed by failed creation. RAII handles partial creation,
launch and read failures. On POSIX an OS refusal to unlink fails closed and closes
the descriptor; an empty private created file can remain if unlink itself is denied.
No unsafe pathname-based fallback cleanup is attempted. Filename randomness is
additional collision avoidance, not the exclusivity or privacy mechanism.

## Release-active coverage

Checks throw independently of `assert`/NDEBUG. Tests cover pre-existing files,
symlink/reparse targets, private permissions, noninheritance, repeat reads of the
same object, partial-creation unwind, missing parent, and replacement-path survival
on POSIX. On Windows they also require delete-on-close cleanup and rejection while
the owned object remains open. Reparse creation is explicitly skipped only for
unsupported/privilege-denied Windows conditions, with the actual error printed.

A copied child executable and cwd contain spaces and Unicode. The Windows child
uses `wmain` and strict UTF-16-to-UTF-8 conversion, so argument comparisons observe
wide arguments rather than an ANSI CRT conversion. Cases include empty arguments,
spaces, quotes, backslashes adjoining quotes, quoted trailing backslashes, Unicode,
shell metacharacters and an injection-marker string. Exact stdout/stderr, cwd/env,
zero/nonzero exits, invalid UTF-8 rejection on Windows, and concurrent 2 MiB per
stream are checked. Disk-backed output avoids pipe-capacity deadlock; CTest applies
a 60-second timeout. There is no general child execution timeout or output-size cap.

## Executed locally

macOS ARM64; Apple clang 21.0.0 (clang-2100.1.1.101), CMake 4.3.0.
Commands run from the isolated checkout, after final source formatting:

```sh
cmake -S . -B build-review -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TUI=OFF \
  -DT81_BUILD_BENCHMARKS=OFF \
  -DFETCHCONTENT_SOURCE_DIR_ASIO=/Users/t81dev/Code/t81-foundation/build/_deps/asio-src
cmake --build build-review --target t81_process_capture_test vm_ffi_bridge_test cli_repro_hash_test t81 -j4
ctest --test-dir build-review -R '^(t81_process_capture_test|t81_cli_repro_hash_test|vm_ffi_bridge_test)$' --output-on-failure -V
/usr/bin/clang++ -std=c++23 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror -I . tests/cpp/process_capture_test.cpp tools/cli/core/process_capture.cpp -o test-capture-release
./test-capture-release
/usr/bin/clang++ -std=c++23 -O1 -g -DNDEBUG -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -I . tests/cpp/process_capture_test.cpp tools/cli/core/process_capture.cpp -o test-capture-sanitized
ASAN_OPTIONS=detect_leaks=0 ./test-capture-sanitized
```

All three CTest targets and both standalone runs passed. Symlink rejection actually
executed locally. ASan leak detection was disabled; no Windows sanitizer execution
is established by this result. Logs accompany this record. Diff whitespace and
clang-format 22 checks initially passed locally. CI then identified its version-18
spacing requirement for `struct stat info {}` in the two files. Both were
formatted with clang-format 18.1.8, changing whitespace only; the same version
passes locally. The original failed CI result is retained in PR history.

## Publication boundary and preserved limits

At publication, fresh complete Windows MSVC and clang-cl jobs at this correction's
SHA remain required. Exact SHA, job links, actual pass/fail/skip results and logs
will be retained in the PR conversation, without another source commit changing
the tested revision. POSIX execution does not authenticate Windows behavior.
No baseline-repaired claim is made before both complete Windows jobs pass.

Arithmetic selection, implementations and tests remain byte-identical to the parent.
The earlier int128 record and clean-main macOS failure comparison remain unchanged;
the previously observed FFI and fixture failures retain separate root-cause limits.
The successful focused FFI run here does not rewrite those historical observations.
No #779/#780 changes, production arithmetic edits, performance campaign, bypasses,
merges or SCG-08 advancement are included. No production-readiness or comparative
performance claim follows from these portability checks.
