# CPA-003 hosted observation report

Result: all three fresh processes returned exit 0, each with 25 initial/post-step
observations, explicit global Halt after 24 instructions, and zero normalized
differences from the frozen pre-host oracle. No negative trap path executed.

Source: `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`. The original checkout
was not used for compilation because it contains pre-existing edits. The
unchanged clean archive and rebuilt libraries are pinned by source/library hashes
in `evidence/hosted-build.json`. Relevant source content was checked against Git
objects in `evidence/source-pins.json`.

The numerical agreement is an implementation-profile result. Normative memory
and branch prose disagrees with this profile; see `discrepancy-register.json`.
Overall disposition remains READY WITH PREREQUISITES.

## Exact build and process command records

The following is retained command metadata with actual exit status, not a
verbatim full compiler build transcript. The raw instruction-boundary stdout and
empty stderr files are retained separately without rewriting the observer output.

```json
{
  "build_directory": "/tmp/t81-cpa-build",
  "build_exit_code": 0,
  "build_note": "Reused baseline CMake directory; Ninja reported premature end of file/recovering in its build log and then rebuilt 27 actions to successful libt81_vm.a link. No clean-build reproducibility claim; no production source edits.",
  "cmake_cache_sha256": "9b5aae24b16f9c548931983f6ef4a5f2e76f2fc00873d471358685b8495ea5c9",
  "configure_command_from_CPA001": [
    "cmake",
    "-S",
    "/tmp/t81-cpa-803075e-fbzvi39l",
    "-B",
    "/tmp/t81-cpa-build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang",
    "-DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++",
    "-DT81_BUILD_TESTS=OFF",
    "-DT81_BUILD_TUI=OFF",
    "-DT81_BUILD_EXAMPLES=OFF",
    "-DT81_BUILD_BENCHMARKS=OFF",
    "-DT81_BUILD_FUZZ_TESTS=OFF",
    "-DT81_ENABLE_TERNARYOS=ON",
    "-DT81_ENABLE_DPE=OFF"
  ],
  "executed_build_command": [
    "cmake",
    "--build",
    "/tmp/t81-cpa-build",
    "--target",
    "t81_vm",
    "-j",
    "4"
  ],
  "libraries": {
    "libt81_axion.a": "8eccfb23eb676fef0f78b1bc6a9681764267b7b013fd9ba314d042e806652185",
    "libt81_core.a": "e1531098b8120d03a1c0540ae53db24ae9694bc30dd1fd6673cd635bff922e0f",
    "libt81_experimental.a": "b7af9f7515c6a410fedfae557c257a5fd22e5274aa49449fb40e9982b5e49060",
    "libt81_ffi.a": "8b3cd725b4fab4c6c89252a55689038926b1521be780d49fee1c9ac898e7f441",
    "libt81_isa.a": "1e4daeccdb0465e7e40ec47d65bfb98fe2c4cf386d8995ddcb69e4155ac8d3b4",
    "libt81_vm.a": "cbc78eca10e3bc858d3b78ac125778e1a3d01741b6ec2f7795137abc29eca78f"
  },
  "library_compile_configuration": "CMake Debug plus repository global O3/NDEBUG; T81_STRICT_DETERMINISTIC_FLOAT=ON supplies T81_DETERMINISTIC, DPE=OFF",
  "observer_binary_sha256": "223f3144593fc4594b85e3b2a158862e6037afe3c2a607d378c4db4e2d605ec6",
  "observer_compile_exit_code": 0,
  "observer_compile_link_command": [
    "/opt/homebrew/opt/llvm/bin/clang++",
    "-std=c++23",
    "-O0",
    "-g",
    "-UNDEBUG",
    "-DT81_DETERMINISTIC",
    "-I/tmp/t81-cpa-803075e-fbzvi39l/include",
    "/tmp/T81-CPA-003/hosted_observer.cpp",
    "-L/tmp/t81-cpa-build",
    "-L/opt/homebrew/opt/llvm/lib/c++",
    "-Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++",
    "-lt81_vm",
    "-lt81_isa",
    "-lt81_axion",
    "-lt81_ffi",
    "-lt81_experimental",
    "-lt81_core",
    "-o",
    "/tmp/T81-CPA-003/hosted_observer"
  ],
  "observer_source_sha256": "1680eac553345b0619eb51747470b537e2e7ff0c610ea5afb891a796bc532b5a",
  "source_archive": "/tmp/t81-cpa-803075e-fbzvi39l",
  "source_revision": "803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4",
  "toolchain": "Homebrew clang 22.1.1; C++23; Darwin 25.5.0 arm64; CMake 4.3.0"
}
```

```json
[
  {
    "command": [
      "/private/tmp/T81-CPA-003/hosted_observer",
      "/private/tmp/T81-CPA-003/program.raw13"
    ],
    "cwd": "/Users/t81dev/Code/t81-foundation",
    "environment_override": {
      "T81_CANONFS_ROOT": null
    },
    "exit_code": 0,
    "stdout_file": "evidence/hosted-1.raw.jsonl",
    "stderr_file": "evidence/hosted-1.stderr.txt"
  },
  {
    "command": [
      "/private/tmp/T81-CPA-003/hosted_observer",
      "/private/tmp/T81-CPA-003/program.raw13"
    ],
    "cwd": "/Users/t81dev/Code/t81-foundation",
    "environment_override": {
      "T81_CANONFS_ROOT": null
    },
    "exit_code": 0,
    "stdout_file": "evidence/hosted-2.raw.jsonl",
    "stderr_file": "evidence/hosted-2.stderr.txt"
  },
  {
    "command": [
      "/private/tmp/T81-CPA-003/hosted_observer",
      "/private/tmp/T81-CPA-003/program.raw13"
    ],
    "cwd": "/Users/t81dev/Code/t81-foundation",
    "environment_override": {
      "T81_CANONFS_ROOT": null
    },
    "exit_code": 0,
    "stdout_file": "evidence/hosted-3.raw.jsonl",
    "stderr_file": "evidence/hosted-3.stderr.txt"
  }
]
```

## Interpretation

The observer contains no execution switch or expected result values. It reads
`program.raw13`, uses existing decode/encode, explicitly constructs an allow-all
engine and calls the existing make_interpreter_vm/load_program/step path. It
records full signed integer payloads before the comparison code projects them
into canonical decimal strings and sparse memory. Array lengths/domains are
checked separately, so normalization cannot hide a missing tag suffix.

No VM instruction semantics, opcode assignments, metadata, memory defaults or
production policy code were modified. No CanonFS driver was requested by the
observer environment. Three repetitions establish only this observed
repeatability, not cross-platform determinism or native ternary performance.

`hosted-comparison.json` holds per-run controls and an empty numerical differences
list. Source/specification differences remain classified as unresolved
discrepancies rather than being mistaken for observed VM defects or silently
resolved by the oracle.
