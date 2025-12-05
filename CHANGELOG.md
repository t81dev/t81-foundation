# Changelog

All notable changes to the T81 Foundation should be documented in this file. Follow the [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) style and add entries under the appropriate headings.

## [Unreleased]

- add a high-rank tensor demo and a graph demo to the `examples/` directory plus documentation so the suite of data-type demos now illustrates tensor indexing and adjacency matrix handling
- expand `docs/guides/data-types-overview.md`, `docs/guides/demo-gallery.md`, `docs/index.md`, and `README.md` with descriptions and CLI commands for the new demos to keep the guides and gallery synchronized
- ensure `scripts/run-demos.sh` compiles and runs the full demo suite (match → quaternion → high-rank tensor → graph) and document that automation with cross-references
- run the required build/test workflow (`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`, `cmake --build build --parallel`, `ctest --test-dir build --output-on-failure`) and document completion of the mandated release checks
- add `--benchmark` command to the `t81` CLI so it locates and executes `build/benchmarks/benchmark_runner`, forwarding extra args and surfacing branch/commit metadata in `docs/benchmarks.md`; document the shortcut in the README, demo gallery, and docs index
- add a ternary packing helper (`include/t81/core/packing.hpp`) plus practical 19-trit binary sizing so the benchmark now reports `Binary Bytes: 4` and the theoretical/practical density numbers reflect the 5→4 byte compression realignment
- added a packed-state `Cell` prototype (`include/t81/core/cell_packed.hpp`) plus a new `BM_NegationSpeed_PackedCell` benchmark so we can experiment with lookup-based negation (currently ~2.26 Gops/s) before rewriting the core `Cell`
- document the CLI diagnostic upgrade (errors report the source `file:line:column`) and remind release consumers to run `./build/t81 compile path/to/invalid.t81` (broken source) before shipping a new build
- added `examples/weights_load_demo.t81` plus the `docs/guides/weights-integration.md` walkthrough so the CLI → `.t81w` → `weights.load` path is illustrated with a runnable sample that reacquires handles without copying tensors.
- add a guide for the new weights integration path: CLI `t81 weights load` now persists its `.t81w` metadata through the compiler and HanoiVM, `weights.load("<tensor>")` returns a shared handle, and the VM honors that handle via the `WeightsLoad` opcode without copying the tensor.
- add `examples/axion_policy_runner.cpp` plus documentation so the policy runner CLI now prints every `AxionEvent.verdict.reason` string (stack/heap/tensor/meta transitions plus the `AxRead/AxSet guard` verdicts described in RFC-0020/RFC-0009) and auditors can replay that trace without inspecting the VM sources
- document the Axion loop trace example (run a loop-bearing demo with `./build/t81 run …` to see the emitted `(policy … (loop … (file …) (line …) (column …)))` block and tie it back to the deterministic allocator ops in `docs/guides/vm-opcodes.md`
