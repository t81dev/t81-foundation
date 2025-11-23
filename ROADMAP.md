# T81 Foundation — Roadmap
**Current focus: ship a testable prototype (v0.2) and earn scope honestly.**

This repository is a small C++ prototype with draft specs. Many components are stubs; the “sovereign” language/VM/AI stack is aspirational. The roadmap below reflects what actually exists in-tree and what is needed next.

## Where we are
- Core math: balanced-ternary helpers, base-243 big integer with add/sub/mul/mod/gcd/divmod, base81/base243 codecs (canonical Unicode alphabet enforced) (`include/t81/bigint.hpp`, `src/bigint/*.cpp`, `src/codec/*`)
- Data structures: row-major tensor container and ops (broadcast/transpose/slice/reshape/reduce/matmul), text IO helpers (`include/t81/tensor.hpp`, `include/t81/tensor/*`, `src/io/tensor_loader.cpp`)
- IR/runtime: interpreter VM with load/store/add/sub/mul/div/mod, register/memory bounds checks, traps, flags (zero/negative), basic trace buffer, simple segment layout; encoder/decoder for TISC (`include/t81/tisc/*`, `src/tisc/encoding.cpp`, `src/vm/vm.cpp`)
- Language: parser/compiler supports literals, idents, let, if/else, return, +/−/*; emits TISC and runs on VM with basic scoped checking (`include/t81/lang/*`, `src/lang/parser.cpp`, `src/lang/compiler.cpp`)
- System stubs: CanonFS driver now content-addressed with CanonHash81 + capability enforcement and Axion hook; Axion engine still allow-all; C API for bigint string roundtrips (`src/canonfs/in_memory_driver.cpp`, `src/axion/engine.cpp`, `src/c_api/t81_c_api.cpp`)
- Tests/examples: C++ unit tests cover bigints, tensor ops/IO, codecs, VM/lang, CanonFS; examples live in `examples/`

## Milestones
| Milestone | Status | Target | Notes / Proof |
|-----------|--------|--------|---------------|
| v0.1 — Repo + math skeleton | Complete | Q2 2024 | CMake targets, base-243 bigint ops + tests (`tests/cpp/bigint_roundtrip.cpp`) |
| v0.2 — Tensor + codec utilities | In progress | Q3 2024 | Base-243/81 codecs canonical + tested; tensor ops/broadcast/IO in place; remaining: perf checks + Unicode digit alignment |
| v0.3 — TISC interpreter parity | In progress | Q4 2024 | Load/store/div/mod, traps/flags/trace present; remaining opcode coverage, META registers, spec-backed tests (`spec/tisc-spec.md`, `spec/v1.1.0-canonical.md`) |
| v0.4 — T81Lang front-end | In progress | Q1 2025 | Parser supports let/if/return/+/-/*; still need type checking/purity/effects and full grammar |
| v0.5 — Axion + CanonFS | In progress | Q2 2025 | CanonFS content-addressed + capabilities + Axion hook; Axion engine still allow-all; need policy/persistence |
| v1.0 — Deterministic stack | Planned | 2025+ | Deterministic VM, full instruction set, CI harness, docs/spec alignment |

## Near-term work (next two milestones)
- Harden tensor API: performance/overflow guards complete; add fuzz/roundtrip tests (v0.2 closure; `spec/t81-data-types.md` §3, `spec/t81vm-spec.md` §4.1 TENSOR).
- Base-81/Unicode alignment: canonical alphabet enforced in codecs/validators; remaining: align bigint textual paths and normalization rules (`spec/v1.1.0-canonical.md`, `spec/t81-data-types.md` §2) and reject legacy digit forms.
- TISC/VM: expand opcode coverage (flags/meta), add decode/jump/memory conformance tests, integrate META/trace per `spec/tisc-spec.md` §§3–5, `spec/t81vm-spec.md` §2.
- T81Lang: add proper type checking/scope enforcement, ensure both branches return, and flesh out grammar/effects (`spec/t81lang-spec.md` §§1–4).
- CanonFS/Axion: enforce capabilities without bootstrap allowance, formalize subjects, and connect Axion verdicts to VM/META (`spec/canonfs-spec.md`, `spec/axion-kernel.md`).
- Add a CI-friendly test runner that builds `tests/cpp/*` targets (current manual: `cmake -S . -B build-make -G \"Unix Makefiles\" && cmake --build build-make && ctest || ./build-make/t81_*_test`).

## Spec alignment backlog (from `spec/`)
- Data types: finalize bigint textual canonicalization and deterministic T81Float; enforce fraction reduction and positive denominators.
- CanonFS: carry CanonHash-81/capability rules into persistence layers; replace bootstrap allowances and formalize subject identity.
- TISC/VM: fill opcode set, load/store semantics, flags/meta registers, and Axion-visible faults to match `tisc-spec.md` / `t81vm-spec.md`.
- T81Lang: real parser/type checker, purity/effect annotations, and canonicalization per `t81lang-spec.md`.
- Axion: replace allow-all engine with policy enforcement and META trace hooks from `axion-kernel.md`.

## Long-term / stretch
- Deterministic JIT (LLVM) + GPU dispatch path.
- Language ergonomics: modules, REPL, richer type system.
- Formal verification for core arithmetic and VM state evolution.
- Hardware interfaces (PCIe/driver) once the software stack is stable.
