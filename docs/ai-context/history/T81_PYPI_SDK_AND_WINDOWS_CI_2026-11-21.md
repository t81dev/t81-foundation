# T81 PyPI SDK audit and Windows CI · November 21, 2026

Codex performed these checks. The SDK observations refer to the published wheel
for tag `v2.1.0` at `9ddeeccda3170f1d12d262f757d56a0c8c903485`. The Windows
CI run tested a separate workflow change.

## Wheel audit

- Artifact `t81-2.1.0-cp312-cp312-macosx_11_0_arm64.whl`
- SHA-256 `c3904276b41fdf27468a2e1ec80dc11c91d36d4883310f739f715326cb7236ea`, verified against PyPI metadata
- Environment CPython 3.12 on macOS ARM64, isolated virtual environment

The installed SDK exposed 19 public names and no CLI entry points. The CLI and
MCP server are separate native builds. The smoke checks compiled and ran a
program, converted a `BigInt` sum with `.to_int()`, performed fixed-size tensor
addition, inspected dynamic tensor data, and round-tripped a CanonFS `RawBlock`
through memory and a reopened disk driver using the original `CanonRef`.
An `@axion_verify` program returned `7` in VM register 2 and logged the reason
`Default policy: allow all`. All ten corrected docstring examples and the
release-note snippet passed against the installed wheel. See the
[release audit](../../releases/v2.1.0.md#python-package-audit)
and [PyPI file metadata](https://pypi.org/project/t81/2.1.0/#files).

The binding links the native VM containing the `TMatMul` opcode. The audited
extension contains its diagnostic strings, and the Python tensor classes have
no direct matmul method or `@` operator. The wheel checks did not run the
opcode. [Tagged build linkage](https://github.com/t81dev/t81-foundation/blob/9ddeeccda3170f1d12d262f757d56a0c8c903485/CMakeLists.txt#L1873-L1881)
· [Tagged opcode](https://github.com/t81dev/t81-foundation/blob/9ddeeccda3170f1d12d262f757d56a0c8c903485/vm/vm.cpp#L4705)

## Hosted Windows clang-cl

[PR #838](https://github.com/t81dev/t81-foundation/pull/838) had head
`576362ee06ec6fbba115b028ed0bc3823212b4bc`. The Windows job checked out
`b615678cd278bc6c0e7bf66500a8edc224443250`, GitHub's merge commit with the
identical Git tree `382c118a972c66d20896d0c0d6beec4ca04b6fb2`.

- Compiler `C:/Program Files/Microsoft Visual Studio/18/Enterprise/VC/Tools/Llvm/x64/bin/clang-cl.exe`
- Clang `22.1.3`. MSVC toolset `14.52.37025`.
- CMake configure, Google Benchmark `STD_REGEX`, and build passed. CTest
  reported 407/407 passed in 190.66 seconds. The artifact retained the full
  text log and CMake configure diagnostics. The separate JUnit XML was absent.

[Hosted Windows job](https://github.com/t81dev/t81-foundation/actions/runs/44955702634/job/137004651754)
· [Hosted workflow run](https://github.com/t81dev/t81-foundation/actions/runs/44955702634)

Build and configure logs were retained as artifact `ci-build-windows-x86_64-clang-cl`
(ID `14727607727`).
