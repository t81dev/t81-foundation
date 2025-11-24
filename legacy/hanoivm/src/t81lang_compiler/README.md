# T81Lang Compiler

![T81Lang Logo](https://via.placeholder.com/400x200/000000/81FF81?text=T81Lang) *(Placeholder: Replace with actual ternary-inspired logo)*

**T81Lang** is an innovative, statically-typed programming language designed for symbolic, recursive, and AI-native computation. It leverages base-81 arithmetic, ternary logic, and advanced data types like tensors and graphs to enable ultra-dense, efficient encoding of complex algorithms. The language compiles down to bytecode for the **HanoiVM** virtual machine via the **TISC** (Ternary Instruction Set Compiler) intermediate representation.

This repository hosts the **legacy HanoiVM-based compiler** for T81Lang, located at `legacy/hanoivm/src/t81lang_compiler`. It implements a full pipeline using literate programming (CWEB) for transparency and maintainability. The compiler supports pattern-matching optimizations, macro injection for recursive patterns (e.g., Fibonacci, Tower of Hanoi), and entropy-guided code generation.

**Status:** Experimental / Legacy (Active development has moved to modern backends like LLVM in the main branch).

## Features

- **Base-81 Literals**: Native support for `123t81` (integers) and `3.14t81` (floats) with ternary singletons (`+`, `-`, `0`, `1`, `2`).
- **Rich Type System**: Primitives like `T81BigInt`, `T81Float`; composites like `T81Matrix`, `T81Tensor`, `T81Graph`, `T81Socket`.
- **Recursive & AI Primitives**: Stdlib macros for `fib`, `fact`, `tower`, `tnn` (Ternary Neural Network accumulate), and `matmul`.
- **Pattern-Matching Backend**: Detects Hanoi/Tower sequences in IR and lifts to optimized opcodes.
- **HanoiVM Target**: Emits compact `.hvm` bytecode for the stack-based ternary VM.
- **Literate Programming**: All core modules in `.cweb` format for self-documenting code.
- **Extensibility**: Optional LLVM backend skeleton for native compilation.

## Architecture Overview

The compiler pipeline transforms T81Lang source (`.t81`) into executable HanoiVM bytecode (`.hvm`):

```
.t81 Source
  ↓ (Lexer: base-81 tokenization)
Tokens
  ↓ (Parser: recursive descent AST)
AST
  ↓ (Semantics: type checking, symbol tables)
Typed AST
  ↓ (IR Gen: 3-address code)
IR (.ir)
  ↓ (TISC: macro injection, pattern recognition)
TISC IR
  ↓ (Emitter: binary bytecode)
.hvm Bytecode → HanoiVM Execution
```

Key optimizations:

- **Entropy Scoring**: AI-guided selection via `tyrnary_entropy.h`.
- **Symbolic Macros**: Lifts common patterns (e.g., recursion) to single opcodes like `TISC_OP_TOWER`.

## Repository Structure

This legacy compiler is self-contained in `legacy/hanoivm/src/t81lang_compiler`. Core files (all `.cweb` for literate programming):

| File | Purpose |
|------|---------|
| `t81lang_lexer.cweb` | Tokenizes source with `t81` suffix detection for literals. |
| `t81lang_grammar.cweb` | BNF grammar specification (used for validation). |
| `t81lang_parser.cweb` | Builds AST from tokens (functions, statements, expressions). |
| `t81lang_semantic.cweb` | Symbol tables, scoping, type checking (e.g., `T81BigInt` compatibility). |
| `t81lang_irgen.cweb` | Generates 3-address IR (load/add/jump). |
| `t81lang_compiler.cweb` | Main driver: orchestrates pipeline with CLI flags. |
| `tisc_ir.cweb` | Defines TISC IR format and opcodes (e.g., `TISC_OP_FIB`). |
| `tisc_stdlib.cweb` | Stdlib macros: `fib`, `tower`, `tnn`, `matmul`, debug. |
| `tisc_backend.cweb` | Transforms IR to TISC with pattern detection (Hanoi sequences). |
| `tisc_compiler.cweb` | Compiles HVM disassembly to TISC (legacy bridge). |
| `emit_hvm.cweb` | Emits binary `.hvm` from IR (HanoiVM format with magic header). |

Supporting files:

- `slang_grammer.ebnf`: Extended BNF grammar.
- `t81_compile.py`: Modern Python frontend (tokenizer + AST → TISC/JSON).

For LLVM backend (experimental, in parent dir):

- `t81_codegen.cweb`, `T81InstrInfo.td`, `T81RegisterInfo.td` (81 GPRs: R0–R80).

## Quick Start

### Prerequisites

- **C Compiler**: GCC/Clang (C99+).
- **CWEB Tools**: Install `cweb` (TeX Live or `tlmgr install cweb`) for tangling/weaving `.cweb` → `.c`/`.tex`.
- **Make**: For building.
- **HanoiVM Runtime**: Clone from `legacy/hanoivm` and build the interpreter (`hanoivm_vm.cweb`).

### Build Instructions

1. **Tangle CWEB Files** (generate `.c` from `.cweb`):

   ```bash
   cd legacy/hanoivm/src/t81lang_compiler
   cweb t81lang_compiler.cweb  # Or tangle all *.cweb
   ```

1. **Compile** (links lexer/parser/etc.):

   ```bash
   make  # Assumes provided Makefile (see below)
   # Or manually:
   gcc -o t81c *.c -std=c99 -Wall -O2
   ```

1. **Example Makefile** (save as `Makefile`):

   ```makefile
   CWEB = cweb
   CC = gcc
   CFLAGS = -std=c99 -Wall -O2 -I.
   SOURCES = $(wildcard *.c)  # Auto-detect tangled .c
   TARGET = t81c

   all: tangle $(TARGET)

   tangle:
   	$(CWEB) *.cweb  # Tangle all

   $(TARGET): $(SOURCES)
   	$(CC) $(CFLAGS) -o $@ $^

   clean:
   	rm -f *.c *.tex $(TARGET)
   ```

1. **Run**:

   ```bash
   ./t81c hello.t81 --emit-ir --emit-hvm
   # Outputs: output.ir + program.hvm
   ./hanoivm program.hvm  # Run on VM
   ```

### Usage

```bash
Usage: ./t81c <source.t81> [flags]

Flags:
  --emit-ir       Dump IR to output.ir
  --emit-hvm      Emit binary .hvm (default: no)
  --no-analysis   Skip semantic checks
```

## Example: Hello World in T81Lang

`examples/hello.t81`:

```t81
fn main() -> T81String {
    let msg: T81String = "Hello, T81Lang! (Base-81 Power)";
    return msg;
}
```

Compile & Run:

```bash
./t81c hello.t81 --emit-hvm
./hanoivm program.hvm  # Outputs: Hello, T81Lang! (Base-81 Power)
```

Advanced Example (Recursive Fibonacci):

```t81
fn fib(n: T81BigInt) -> T81BigInt {
    if n <= 1t81 {
        return n;
    } else {
        return fib(n - 1t81) + fib(n - 2t81);
    }
}

fn main() -> T81BigInt {
    return fib(10t81);  // Lifts to TISC_OP_FIB macro
}
```

## Development

- **Tangling/Weaving**: Use `cweave file.cweb` for PDF docs; `ctangle` for code.
- **Testing**: Run `make test` (integrates `ir_test_sample()` from `t81lang_irgen.cweb`).
- **Extending Opcodes**: Add to `tisc_ir.cweb` and update `tisc_stdlib.cweb`.
- **Debugging**: Enable `VERBOSE_ASM=1` for assemblers; use `TISC_OP_DEBUG` macros.

## Limitations (Legacy Branch)

- No full LLVM integration (see main branch for `t81-llvm`).
- Basic control flow (no loops in VM yet; expands to jumps).
- HanoiVM is stack-only (no registers; future: 81 GPRs via LLVM).

## Contributing

1. Fork & clone: `git clone https://github.com/t81dev/t81-foundation`.
1. Tangle `.cweb` files, make changes, commit tangled `.c` + `.cweb`.
1. Test: `./t81c examples/*.t81 --emit-hvm && ./hanoivm program.hvm`.
1. PR to `main` (not legacy).

Follow literate style: Document in `@* Sections *@`.

## License

GPL-3.0 (see `LICENSE` in root).\
T81Lang is free software; contributions welcome!

## Acknowledgments

Inspired by Knuth's CWEB, Hanoi Tower recursion, and ternary computing.\
Built with ❤️ by the T81Dev community.

______________________________________________________________________

*Version: 0.1.0 (Legacy HanoiVM Edition)* | *Date: November 22, 2025*\
[Docs](https://t81lang.org) | [Issues](https://github.com/t81dev/t81-foundation/issues) | [Chat](https://x.com/t81dev)
