# T81 Foundation — Roadmap
**Current focus: ship a testable prototype (v0.2) and earn scope honestly.**

This repository is a small C++ prototype with draft specs. Many components are stubs; the “sovereign” language/VM/AI stack is aspirational. The roadmap below reflects what actually exists in-tree and what is needed next.

## Where we are
- Core math: balanced-ternary helpers, base-243 big integer with add/sub/mul/mod/gcd/divmod, base81/base243 codecs (`include/t81/bigint.hpp`, `src/bigint/*.cpp`, `src/codec/*`)
- Data structures: row-major tensor container and ops (broadcast/transpose/slice/reshape/reduce/matmul), text IO helpers (`include/t81/tensor.hpp`, `include/t81/tensor/*`, `src/io/tensor_loader.cpp`)
- IR/runtime: minimal TISC opcode set + encoder/decoder and interpreter VM; memory ops are stubbed (`include/t81/tisc/*`, `src/tisc/encoding.cpp`, `src/vm/vm.cpp`)
- Language: AST + compiler stub that only handles i64 literals and add/sub expressions (`include/t81/lang/*`, `src/lang/compiler.cpp`)
- System stubs: in-memory CanonFS driver, Axion allow-all engine, C API for bigint string roundtrips (`src/canonfs/in_memory_driver.cpp`, `src/axion/engine.cpp`, `src/c_api/t81_c_api.cpp`)
- Tests/examples: C++ unit tests cover bigints, tensor ops/IO, codec roundtrips, VM/lang integration; examples live in `examples/`

## Milestones
| Milestone | Status | Target | Notes / Proof |
|-----------|--------|--------|---------------|
| v0.1 — Repo + math skeleton | Complete | Q2 2024 | CMake targets, base-243 bigint ops + tests (`tests/cpp/bigint_roundtrip.cpp`) |
| v0.2 — Tensor + codec utilities | In progress | Q3 2024 | Tensor ops/broadcast/IO and base81/243 codecs exist; needs canonicalization and perf checks |
| v0.3 — TISC interpreter parity | In progress | Q4 2024 | Opcode coverage and memory semantics incomplete; add spec-backed tests (`spec/tisc-spec.md`) |
| v0.4 — T81Lang front-end | Planned | Q1 2025 | Parser/type-checker, more ops, file-based compiler emitting TISC |
| v0.5 — Axion + CanonFS | Planned | Q2 2025 | Replace stubs with policy engine, hashing, capability/persistence layer |
| v1.0 — Deterministic stack | Planned | 2025+ | Deterministic VM, full instruction set, CI harness, docs/spec alignment |

## Near-term work (next two milestones)
- Finalize canonical base-81/243 encodings; remove placeholder `from_ascii` path and document normalization.
- Harden tensor API: shape overflow guards, elementwise/reduction coverage, and IO fuzz/roundtrip tests.
- Fill in TISC load/store and trap behavior in the interpreter; add opcode-level conformance tests.
- Expand the compiler to parse source text (not just AST structs), add mul/branching, and ensure VM compatibility.
- Implement CanonFS hash + capability checks and wire Axion verdicts into VM/syscall paths.
- Add a CI-friendly test runner that builds `tests/cpp/*` targets.

## Spec alignment backlog (from `spec/`)
- Data types: implement canonical base-81 bigint and deterministic T81Float; enforce fraction reduction and positive denominators.
- CanonFS: use CanonHash-81 and capability enforcement; replace counter-based hashes in the in-memory driver.
- TISC/VM: fill opcode set, load/store semantics, flags/meta registers, and Axion-visible faults to match `tisc-spec.md` / `t81vm-spec.md`.
- T81Lang: real parser/type checker, purity/effect annotations, and canonicalization per `t81lang-spec.md`.
- Axion: replace allow-all engine with policy enforcement and META trace hooks from `axion-kernel.md`.

## Long-term / stretch
- Deterministic JIT (LLVM) + GPU dispatch path.
- Language ergonomics: modules, REPL, richer type system.
- Formal verification for core arithmetic and VM state evolution.
- Hardware interfaces (PCIe/driver) once the software stack is stable.
