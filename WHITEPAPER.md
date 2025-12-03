# T81 Foundation Whitepaper

## Introduction

The **T81 Foundation** delivers a ternary-native, cognition-first stack built around deterministic numerics, a principled virtual machine (HanoiVM + TISC), and the C++20 compiler of **T81Lang**. This whitepaper inventories every major component implemented so far, summarizes the semantic/IR guarantees we now uphold, and highlights the tooling, CLI, and documentation that keep the stack aligned with the constitutional constraints from `AGENTS.md`.

## Repository Overview

1. **Core Numerics (`include/t81/core`, `src/`)** – implements over ninety canonical numeric types (`T81Int`, `T81Float`, `T81Fraction`, `T81Tensor`, `T81List`, `T81Set`, `T81Result`, etc.). Arithmetic and Tensor ops are tested by the extensive benchmark and unit suite under `tests/cpp`. Foundation headers avoid Unicode, follow Doxygen conventions, and are patched continuously for new machine-specific intrinsics (e.g., `T81LimbAVX2`, AVX2 helpers).
2. **Frontend (`include/t81/frontend`, `src/frontend`)** – features a lexer/parser (rejecting legacy `<...>` generics in favor of `[ ... ]`), an AST with visitor enums, and the `SemanticAnalyzer` that handles Option/Result, records, enums, vector literals, and alias instantiations. The IR generator emits `tisc::ir::Instruction`s, canonical tensors, and metadata for structural types.
3. **TISC + HanoiVM (`include/t81/tisc`, `src/tisc`)** – hosts the intermediate instruction representation, binary emitter, and runtime program format. The VM accepts metadata-rich binaries with deterministic alias/state pools, reads tensors/strings, and exposes an Axion-aware trace infrastructure. `tisc::binary_io` and `Program::type_aliases` serialize all structural info so CLI-consumed binaries remain transparent.
4. **CLI & Tools (`src/cli`, `tests/cpp/cli_*`)** – provides `t81::cli::compile`, `run_tisc`, `check_syntax` plus the `t81` executable with `compile/run/check/benchmark/weights` subcommands. The CLI honors deterministic metadata, integrates weight-model workflows, and exposes flags for quiet/verbose output. Regression tests confirm the full pipeline handles Option/Result, records, enums, and tensor literals end-to-end.
5. **Documentation & Guides (`docs/guides`, `WHITEPAPER.md`, `ROADMAP.md`, `TASKS.md`, `ANALYSIS.md`)** – describe the enum/record semantics, vector literal canonicalization, CLI toolkit, system status, and agent governance. `WHITEPAPER.md` now serves as the canonical executive summary of our repo’s capabilities.
6. **Examples/Benchmarks (`examples`, `benchmarks`)** – include Llama bridging helpers (`examples/t81_llama_bridge.hpp`), demos (`axion_demo.cpp`, `T81Genesis.cpp`), and tensor operation showcases, ensuring new CLI features have real-world touchpoints.

## Compiler & Semantic Work

1. **Structural Type System** – `record`/`enum` declarations now fully participate in the analyzer. The parser emits field/variant data, the analyzer enforces uniqueness and payload assignability, and `RecordInfo`/`EnumInfo` cache the resolved types to guide field access and pattern dispatch. Literals obey the same `is_assignable` rules used for Option/Result, so canonicalization is preserved before lowering.
2. **Option/Result Completeness** – `Some`, `None`, `Ok`, and `Err` constructors require contextual types. The analyzer enforces match exhaustiveness, consistent arm return types, and numeric widening, then emits dedicated IR instructions (`MAKE_OPTION_SOME`, `RESULT_IS_OK`) that the VM understands.
3. **Vector Literal Canonicalization** – Vector literals become `T729Tensor` handles in the IR tensor pool. Non-empty lists infer from the first numeric literal, empty lists require contextual `Vector`/`Tensor` hints, and parsing rejects mixed-typed or non-literal elements. The CLI serializes these tensors so the VM always receives canonical payloads.

## IR, CLI & VM Alignment

1. **Metadata Propagation** – Enriched `TypeAliasMetadata` carries `StructuralKind`, `FieldInfo`, and `VariantInfo` so every record/enum leaves a trail in the IR. `tisc::binary_io` serializes this metadata alongside instructions, tensors, and shape pools, enabling post-hoc analysis or tooling that doesn’t rerun the compiler.
2. **CLI Toolkit & Executable** – `t81::cli::compile`, `run_tisc`, `check_syntax`, and the `t81` CLI propagate deterministic metadata while exposing weight-model compilation (`.t81w`), temporary `.tisc` execution, and direct validation. Flags like `--weights-model`, `--output`, `--quiet`, and `--verbose` let automation scripts integrate safely.
3. **Regression Suite** – Tests: `semantic_analyzer_record_enum_test`, `semantic_analyzer_option_result_test`, `cli_structural_types_test`, `cli_record_enum_test`, `cli_option_result_test`, and `tisc_type_alias_io_test` cover semantics, CLI/VM loops, and binary IO. They ensure Option/Result matching, structural metadata, CLI compile/run paths, and serialization of metadata remain stable.
4. **High-Level Tools** – Auxiliary programs (`examples/axion_demo.cpp`, `examples/t81_llama_bridge.hpp`, `benchmarks/*`) demonstrate hooking the stack into use cases ranging from Axion workflows to Llama-powered chat sessions.

## Runtime, VM & Documentation

1. **HanoiVM & TISC** – The VM now understands canonical metadata for vectors, records, enums, and aliases. `binary_emitter` writes this data into `Program::type_aliases`, and `binary_io` serializes it so the runner can load deterministic alias tables with every execution.
2. **Guides & Whitepapers** – Added the CLI toolkit guide, structural guide, vector literal guide, system status update, and now this repository-wide whitepaper, ensuring each new feature is tied back to `spec/t81lang-spec.md`, `TASKS.md`, and `AGENTS.md`.

## Infrastructure & Community Practices

1. **Build & Tests** – `cmake` builds, `ctest` runs 83 tests (core numerics, CLI regressions, semantic passes). `WHITEPAPER.md` references this suite. We keep the repo workspace-write compliant and never force `git` resets.
2. **Agent Governance** – `AGENTS.md`, `ROADMAP.md`, and `TASKS.md` define deterministic mandates: no hidden nondeterminism, canonicalization at every boundary, and documentation for all high-level changes.

## Next Steps & Open Problems

- Advance the P1 virtualization goals (full HanoiVM memory model, Axion safety hooks).  
- Enrich CLI diagnostics with trace replay and metadata-aware replay.  
- Extend structural types to support schema versions, cross-module linking, and richer pattern matching.

Every change remains aligned with the constitutional constraint: deterministic canonicalization, no silent failures, and full traceability from parser → analyzer → IR → CLI → VM.
