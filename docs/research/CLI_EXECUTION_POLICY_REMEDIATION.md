# CLI execution policy boundary remediation

This maintenance correction addresses OP-SEC-02 from [draft PR #612](https://github.com/t81dev/t81-foundation/pull/612), audit head `215fb5cba7d452f1c94a714446235619e92e7abe`. That PR remains the original audit record; this change does not edit it or repair its agent-skill findings.

## Source and execution boundary

- Remediation base: `903ea2fe653df0d636e0543f9788dd8442738045`.
- Implementation and regression source: `0482235f0886bce73b88c0fc82cddf07bb78da16`.
- Red comparison: retained audit binary built from `fabc7aa90f70ead227ec257a1ae6833fa16ced40`, SHA-256 `d6a88ee8b445d09a1c93592c7b75323a590839b17d9427873a208050b5e38c0d`. Its relevant CLI, VM, policy parser, and policy engine files are identical to the remediation base (verified with `git diff` in the receipts).
- Corrected binary SHA-256: `279c12e8c982ad625a69e624ccdacc4d16c8d61ae7ef27c9e6e35cee65c35e07`.

The shared parser consumes `--policy` into `Args::policy`, including occurrences after the action or input. `run_vm_command` now initializes its policy path from that field. Both `vm run` and `vm trace` pass it to `run_tisc`. `code run` normalizes to `run` while retaining that field, compiles `.t81` input if needed, then calls the same driver for `.tisc` execution.

The driver now returns 1 on an open/read/parse failure, including an empty file. It validates the captured policy bytes, attaches those same bytes to the program, loads the VM, and checks that an explicit policy was installed before calling `run_to_halt`. Exceptions also propagate to the existing CLI error handler rather than reaching execution. It does not reopen the policy file between validation and installation. A policy denial still follows the existing VM path and returns `SecurityFault`, exit 13. Axion parsing, evaluation, and normative semantics are unchanged.

With no explicit policy, embedded policy and existing VM defaults are preserved. This does not imply every operation is permitted without a policy. Source compilation can occur before requested-policy validation. Policies that allow earlier instructions can deny later; the existing diagnostic `compute executed: no` must not be generalized to mean no earlier instruction ever ran. The tests below use a zero-instruction budget to deny the first dispatch specifically.

## Reproduction and retained results

Observed 2026-10-27 on macOS 26.5.2 ARM64, AppleClang 21.0.0.21000101, Python 3.14.3, CMake 4.3.0, Ninja 1.13.2. This is local implementation regression evidence, not independent scientific replication or a universal security proof.

The isolated checkout is `/private/tmp/t81-cli-policy-fix`; build directory is `/private/tmp/t81-cli-policy-build`. Configuration used an existing local Asio source checkout to avoid fetching it. Commands run from the isolated checkout:

```sh
cmake -S . -B /private/tmp/t81-cli-policy-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_BUILD_TUI=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_TERNARYOS=OFF \
  -DFETCHCONTENT_SOURCE_DIR_ASIO=/Users/t81dev/Code/t81-foundation/build/_deps/asio-src
cmake --build /private/tmp/t81-cli-policy-build --parallel 4 --target \
  t81 t81_vm_predispatch_policy_deny_logging_test \
  t81_vm_policy_parse_fail_closed_test axion_policy_allow_deny_determinism_test \
  t81_cli_contract_test
python3 tests/python/test_cli_execution_policy.py \
  --t81-bin /private/tmp/t81-operational-build/t81 \
  --receipt /private/tmp/t81-policy-before.json
python3 tests/python/test_cli_execution_policy.py \
  --t81-bin /private/tmp/t81-cli-policy-build/t81 \
  --receipt /private/tmp/t81-policy-after.json
ctest --test-dir /private/tmp/t81-cli-policy-build --output-on-failure \
  -R '^(t81_cli_execution_policy_test|t81_vm_predispatch_policy_deny_logging_test|t81_vm_policy_parse_fail_closed_test|axion_policy_allow_deny_determinism_test)$'
```

[Before](evidence/cli_execution_policy/before.json) and [after](evidence/cli_execution_policy/after.json) retain every process's argv array, working directory, exit code, stdout, stderr, successful trace contents, fixture contents and hashes, binary hash, test hash, and environment. Temporary paths are historical observations, recreated by the test rather than permanent fixture locations. The [verification receipt](evidence/cli_execution_policy/verification.json) retains additional exact commands and outputs. The [initial build log](evidence/cli_execution_policy/initial-build.log) includes the existing duplicate-library linker warning.

| Check | Actual result |
| --- | --- |
| New CLI matrix against retained audit binary | Exit 1; 73 processes, 42 failing cases, 138 failed assertions |
| Same matrix against corrected binary | Exit 0; 73 processes, zero failed assertions |
| Focused CTest selection above | Exit 0; 4/4 tests pass |
| Existing CLI contract test | CTest exit 8; stops at missing `models/tiny-random-llama-exported.safetensors`, `cli_contract_test.cpp:491` |
| CLI documentation parity | Exit 0; 24 topics |
| Root structure, active doc links, overclaim guardrails, determinism claims | Each exits 0 |
| TISC freeze integrity script | Exit 0; 209 registry entries; registry integrity, not executable opcode conformance |
| Documentation structure | Exit 1; missing `book/book-en`, `book/book-cn`, `book/book-es`, `book/book-pt`, `book/book-ru` |
| Changed C++ lines, clang-format 18.1.8 | Exit 0 |
| Optional whole-file formatting of the two CLI files | Exit 1 on both base and corrected files due to pre-existing formatting; unrelated lines left unchanged |

The matrix covers `vm run`, `vm trace`, and `code run` for both source and bytecode. For each, it exercises the domain-level shared-parser placement, action-local placement, and trailing placement. Valid policies preserve sentinel output `47` and successful termination/trace creation. Missing, malformed, and empty policy files exit 1 with no program stdout. The valid deny-all fixture `(policy (max-instructions 0))` exits 13 with `count=1 limit=0`, no sentinel, and no trace file. The VM's existing pre-dispatch implementation checks that verdict before instruction dispatch; its existing deny-logging regression also passes. Unsupported placement before the top-level domain and a missing policy option value exit 1. The test uses argv arrays and paths containing spaces and is registered outside the POSIX-only CTest block.

## Remaining limits and human acceptance

The complete CLI contract suite did not finish because its model fixture is absent. Documentation structure remains red for the five directories above. Neither missing model data nor unrelated formatting/book structure is repaired here. Read-error and application-failure guards are inspected production branches; partial device reads and VM installation fault injection were not separately exercised. Missing-file, empty-file, parse-rejection, and first-instruction denial behavior were exercised through the real CLI. Other CLI actions, host sandboxing, and general Axion correctness are outside this correction's validation scope.

Linux x86-64, Linux ARM64, Windows, and hosted macOS CI results are not yet available for this branch at report preparation. Portability is supported by the existing C++ interfaces and Python subprocess argument arrays; it remains an unverified expectation until CI runs. Human acceptance requires review of the production boundary and receipts, review of CI outcomes on the published draft, and explicit disposition of the broader check failures. Preserve #612 as historical evidence. Do not merge automatically.
