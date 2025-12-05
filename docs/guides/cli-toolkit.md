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
- `t81::cli::check_syntax(source)` — reuses the same lex → parse → semantic → IR generation path as `compile`, but returns before persisting a `.tisc` file. Useful for CI checks that need the compilerʼs diagnostics without producing output.  
- `t81::cli::repl(weights_model?, input = std::cin)` — compiles each snippet built inside the interactive loop, runs it, and prints `Execution completed` (or any trap diagnostics) before waiting for more text. Submit an empty line to trigger compilation/execution, use `:help` to reprint these instructions, `:history` to inspect the buffered snippet (& recent runs), `:reset` to clear the pending code, `:load`/`:save` to round-trip files, `:run` to force execution before a blank line, `:model` to inspect or reload weights, `:verbose`/`:quiet` to adjust logging, `:bindings`/`:symbols` to list recorded names, and `:trace`/`:trill` to dump the latest Axion trace. Exit with `:quit`/`:exit` to tear down the session. Supplying `--weights-model` to `t81 repl` lets the REPL reuse a pretrained asset without restarting the CLI.  

Typical usage appears in `tests/cpp/cli_*_test.cpp`—the CLI builds a temporary source, writes it out, calls `compile`, and finally `run_tisc`. Each regression cleans up artifacts afterward.

## `t81` Executable Wrappers

The `t81` binary wraps the CLI surface in user-facing commands:

- `t81 compile <file>` — accepts `.t81`, `.t81w`, and `.tisc` inputs. Weight files (`.t81w`) are expanded through `t81::weights` helpers before compilation, so production weight models benefit from the same deterministic metadata flow. `--output=<path>` overrides the target `.tisc` name.  
- `t81 run <file>` — compiles `.t81` inputs to a temporary `.tisc` and runs them; `.tisc` binaries are executed directly.  
- `t81 check <file>` — runs `t81::cli::check_syntax`, which now reuses the compiler pipeline (lex → parse → semantic analysis → IR generation) without emitting bytecode. That means you get the same Option/Result/generic diagnostics as `t81 compile`, ensuring every mismatch is caught before you write a `.tisc` file.  
- `t81 repl` — enters the interactive REPL described above; it accepts the global flags (`--verbose`, `--quiet`, `--weights-model`), buffers lines until a blank line is submitted, then compiles + executes the accumulated program. `:help` reprints the command summary; `:history`, `:reset`, `:load`, `:save`, `:run`, `:model`, `:bindings`, and `:trace` provide session controls, and `:quit`/`:exit` leave the loop.  
- `t81 benchmark` and `t81 weights ...` — reuse the CLI infrastructure for stress tests and model manipulation.  

Global flags such as `--weights-model=<file>`, `--quiet`, and `--verbose` are available on every command so scripts can inject pretrained assets or tune output verbosity while preserving the structural invariants described above.

## Structural Metadata Guarantees

- Records and enums now survive the CLI boundary as serialized metadata. When the compiler sees `record Point { ... }` or `enum Flag { ... }`, `IRGenerator` emits a `TypeAliasMetadata` entry flagged with `StructuralKind::Record`/`Enum` and includes `FieldInfo`/`VariantInfo` vectors. `t81::cli::compile` stores this metadata inside the `.tisc` file (see `src/tisc/binary_io.cpp`).
- The parser now accepts `@schema(<number>)` and `@module(<path>)` annotations before a record or enum declaration; the analyzer records these values and the CLI logs them when `--verbose` is enabled so downstream tooling can report the canonical schema version and owning module path without re-parsing sources.
- Downstream tools can load the TISC file (`t81::tisc::load_program`) and inspect `program.type_aliases` to recover record layouts or enum discriminants without rerunning the analyzer. The new CLI tests (`tests/cpp/cli_structural_types_test.cpp` and `tests/cpp/cli_record_enum_test.cpp`) assert these guarantees hold end-to-end.

## Axion Trace Metadata and Guard Context

- The CLI already bundles Axion-facing metadata inside `tisc::Program`: `format_loop_metadata` copies `loop_metadata()` into `program.axion_policy_text`, `format_match_metadata` produces `(match-metadata …)` s-expressions, and `collect_enum_metadata` records canonical `(enum-id, variant-id, payload)` tuples referenced by guard-aware opcodes. This matches the contract described in [RFC-0019](../spec/rfcs/RFC-0019-axion-match-logging.md) and feeds the Axion trace described in `spec/axion-kernel.md`.
- `t81::cli::compile` dumps the match metadata string when `--verbose` is enabled, so CLI logs now show each scrutinee kind, guard presence, arm pattern, variant identifier, and declared payload type. Combined with the enum metadata summary printed later in the compile step, integration tests such as `tests/cpp/axion_enum_guard_test.cpp` can assert that the VM receives the same variant names and payload types it expects.
- Match metadata now also emits `(guard-expr "<expr>")` when a guard is present so Axion can assert the guard expression text that triggered each `EnumIsVariant`/`EnumUnwrapPayload` event. Use `match_metadata_text` to cross-check that the CLI, analyzer, and Axion interpreter all agree on the guard expression before relying on the generated guard payloads in downstream policies.
- During execution the HanoiVM records `AxionEvent` entries that the CLI exposes through `:trace`/`:trill` (REPL) or by examining `vm->state().axion_log`. Enum guards emit events like `EnumIsVariant / EnumUnwrapPayload` whose `verdict.reason` strings include the canonical `enum=<name>`, `variant=<name>`, `payload=<type>`, and whether the guard `match=pass` or `match=fail`. The Axion log entry for a blue guard looks like:

  ```
  reason: enum guard enum=Color variant=Blue payload=i32 match=pass
  reason: enum payload enum=Color variant=Blue payload=i32
  ```

- The Axion trace now also streams deterministic segment transitions such as stack/heap/tensor allocations, meta slot usage, and privileged guard decisions. Each entry emits a `verdict.reason` like `stack frame allocated stack addr=…`, `tensor slot allocated tensor addr=…`, `meta slot axion event segment=meta addr=…`, `AxRead guard segment=stack addr=42`, or `AxSet guard segment=heap addr=128`, and the regression `tests/cpp/axion_segment_trace_test.cpp` ensures those strings land in `vm->state().axion_log` so downstream Axion tooling can replay segment state word-for-word.`

- These log events carry the same variant IDs that show up in `collect_enum_metadata`, so Axion policies (see `spec/rfcs/RFC-0009-axion-policy-language.md`) can correlate guard paths with `allow-opcode`/`deny-opcode` rules and verify payload expectations deterministically.

### Guard trace example

The new `axion_guard_trace` example (`examples/axion_guard_trace.cpp`) compiles a simple `Color` enum match, runs it through the VM, and dumps both the serialized enum metadata and each `AxionEvent.verdict.reason` string. Build it with `cmake --build build --target axion_guard_trace` and run it via `./build/axion_guard_trace`. You will see output such as:

```
Enum metadata:
  enum Color (id 0)
    variant Red (id 0)
    variant Blue (id 1) payload=i32
Axion log entries:
  opcode=0 tag=0 value=0 reason="match metadata: (match-metadata (match (scrutinee Enum) (type i32) (guards false) (arms (arm (variant Red) (variant-id 0) (pattern None) (guard false) (type i32)) (arm (variant Blue) (variant-id 1) (pattern Identifier) (guard false) (payload i32) (type i32)))))"
  opcode=57 tag=0 value=0 reason="enum guard enum=Color variant=Red match=fail"
  opcode=57 tag=1 value=1 reason="enum guard enum=Color variant=Blue payload=i32 match=pass"
  opcode=58 tag=1 value=9 reason="enum payload enum=Color variant=Blue payload=i32"
```

Each `enum guard`/`enum payload` line corresponds to the `EnumIsVariant`/`EnumUnwrapPayload` opcodes recorded in the Axion log, and the metadata block verifies that the CLI transmitted the same `(enum-id, variant-id, payload)` triple that the Axion visitor sees at runtime. The new `tests/cpp/e2e_axion_trace_test.cpp` regression compiles the sample, runs it in the HanoiVM, and fails if the Axion log omits the `guard-expr` metadata or the expected guard/payload `reason` strings shown above, locking the parser/IR metadata to the trace consumer.

## Regression Tests

- `tests/cpp/cli_option_result_test.cpp`: matches on contextual `Option`/`Result` types and ensures canonical handles (`Some`, `None`, `Ok`, `Err`) commute through the CLI/VM path.  
- `tests/cpp/cli_structural_types_test.cpp`: compiles a program that declares `Point` and `Flag`, executes it, loads the resulting `.tisc`, and checks the serialized metadata contains the expected fields/variants before the binary is discarded.  
- `tests/cpp/cli_record_enum_test.cpp`: verifies match expressions over enums that embed records reach the VM without extra instrumentation.  
- `tests/cpp/cli_repl_test.cpp`: feeds a two-turn snippet (with and without a dummy `ModelFile`) into the new REPL helper, captures stdout/stderr, and asserts the REPL prints `Execution completed`, guarding the :blank-line trigger and `:quit` exit behavior.  
- The CLI tests are built via `CMakeLists.txt` and run through `ctest` when you run the standard suite.

## Notes for CLI Consumers

- When extending the CLI, remember to propagate any new structural metadata (vectors, records, enums) through `tisc::Program::type_aliases` and ensure the binaries can still be loaded by `t81::tisc::load_program`.  
- Use the helper tests as templates for new regression programs; they already cover temporary file handling, deterministic cleanup, and asserting that the metadata matches expectations.  
- Enable `--verbose` on `t81 compile` to see the structural metadata summary emitted from `t81::cli::compile`. The output lists each record/enum alias along with its schema version, module path, and declared fields/variants so automation can verify schema evolution without re-analyzing the source.  
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
