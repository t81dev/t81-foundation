# T81 CLI Toolkit

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 CLI Toolkit](#t81-cli-toolkit)
  - [Commands](#commands)
  - [`t81` Executable Wrappers](#`t81`-executable-wrappers)
  - [Structural Metadata Guarantees](#structural-metadata-guarantees)
  - [Regression Tests](#regression-tests)
  - [Notes for CLI Consumers](#notes-for-cli-consumers)
  - [Advanced Examples](#advanced-examples)

<!-- T81-TOC:END -->












This guide summarizes the current command-line surface of `t81::cli` after the recent frontend work. It focuses on the operations you can rely on today, the invariants the CLI enforces, and the regression tests that keep the toolchain honest.

## Commands

- `t81::cli::compile(source, tisc_path)` — compiles a `.t81` source file into the TISC binary. Always produce deterministic metadata: vector literals are canonicalized into tensor handles, `Option/Result` constructors are validated against contextual types, and every `record`/`enum` declaration emits `TypeAliasMetadata` entries preserving field layouts and variant payloads.  
- `t81::cli::run_tisc(tisc_path)` — runs a compiled TISC program through the HanoiVM. Because the CLI carries canonical tensor handles and structural metadata, the VM can rely on deterministic memory layouts during execution and diagnostics.  
- `t81::cli::check_syntax(source)` — performs the same parse + semantic passes as `compile` but stops before IR generation, returning zero only if no diagnostics are emitted. Useful for CI checks that need fast validation without writing output files.

Typical usage appears in `tests/cpp/cli_*_test.cpp`—the CLI builds a temporary source, writes it out, calls `compile`, and finally `run_tisc`. Each regression cleans up artifacts afterward.

## `t81` Executable Wrappers

The `t81` binary wraps the CLI surface in user-facing commands:

- `t81 compile <file>` — accepts `.t81`, `.t81w`, and `.tisc` inputs. Weight files (`.t81w`) are expanded through `t81::weights` helpers before compilation, so production weight models benefit from the same deterministic metadata flow. `--output=<path>` overrides the target `.tisc` name.  
- `t81 run <file>` — compiles `.t81` inputs to a temporary `.tisc` and runs them; `.tisc` binaries are executed directly.  
- `t81 check <file>` — runs `t81::cli::check_syntax`, helpful for quick parsing/semantic validation.  
- `t81 benchmark` and `t81 weights ...` — reuse the CLI infrastructure for stress tests and model manipulation.  

Global flags such as `--weights-model=<file>`, `--quiet`, and `--verbose` are available on every command so scripts can inject pretrained assets or tune output verbosity while preserving the structural invariants described above.

## Structural Metadata Guarantees

- Records and enums now survive the CLI boundary as serialized metadata. When the compiler sees `record Point { ... }` or `enum Flag { ... }`, `IRGenerator` emits a `TypeAliasMetadata` entry flagged with `StructuralKind::Record`/`Enum` and includes `FieldInfo`/`VariantInfo` vectors. `t81::cli::compile` stores this metadata inside the `.tisc` file (see `src/tisc/binary_io.cpp`).  
- Downstream tools can load the TISC file (`t81::tisc::load_program`) and inspect `program.type_aliases` to recover record layouts or enum discriminants without rerunning the analyzer. The new CLI tests (`tests/cpp/cli_structural_types_test.cpp` and `tests/cpp/cli_record_enum_test.cpp`) assert these guarantees hold end-to-end.

## Regression Tests

- `tests/cpp/cli_option_result_test.cpp`: matches on contextual `Option`/`Result` types and ensures canonical handles (`Some`, `None`, `Ok`, `Err`) commute through the CLI/VM path.  
- `tests/cpp/cli_structural_types_test.cpp`: compiles a program that declares `Point` and `Flag`, executes it, loads the resulting `.tisc`, and checks the serialized metadata contains the expected fields/variants before the binary is discarded.  
- `tests/cpp/cli_record_enum_test.cpp`: verifies match expressions over enums that embed records reach the VM without extra instrumentation.  
- The CLI tests are built via `CMakeLists.txt` and run through `ctest` when you run the standard suite.

## Notes for CLI Consumers

- When extending the CLI, remember to propagate any new structural metadata (vectors, records, enums) through `tisc::Program::type_aliases` and ensure the binaries can still be loaded by `t81::tisc::load_program`.  
- Use the helper tests as templates for new regression programs; they already cover temporary file handling, deterministic cleanup, and asserting that the metadata matches expectations.  
- If you introduce new AST nodes with backend implications, wire them through the analyzer and IR generator as shown in `include/t81/frontend/ir_generator.hpp` and document them in a guide like this one so downstream integrators know what the CLI guarantees.

## Advanced Examples

1. **Inspect structural metadata from a compiled `.tisc` file**

   ```cpp
   auto program = t81::tisc::load_program("record-enum.tisc");
   for (const auto& alias : program.type_aliases) {
       if (alias.kind == t81::tisc::StructuralKind::Record) {
           std::cout << "Record " << alias.name << " fields:";
           for (const auto& field : alias.fields) {
               std::cout << " " << field.name << ':' << field.type;
           }
           std::cout << '\n';
       }
       if (alias.kind == t81::tisc::StructuralKind::Enum) {
           std::cout << "Enum " << alias.name << " variants:";
           for (const auto& variant : alias.variants) {
               std::cout << " " << variant.name;
               if (variant.payload) {
                   std::cout << '(' << *variant.payload << ')';
               }
           }
           std::cout << '\n';
       }
   }
   ```

2. **Compile a program, run it, and propagate diagnostics**

   ```cpp
   auto src = make_temp_path("example", ".t81");
   auto tisc = src;
   tisc.replace_extension(".tisc");
   write_source(src, R"(fn main() -> i32 { return 0; })");
   if (t81::cli::compile(src, tisc) != 0) {
       throw std::runtime_error("Compiler reported errors");
   }
   if (t81::cli::run_tisc(tisc) != 0) {
       throw std::runtime_error("VM reported an error");
   }
   ```

These examples demonstrate how to consume the CLI's outputs programmatically while preserving the deterministic invariants described above.
