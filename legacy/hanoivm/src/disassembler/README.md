[![Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Logic-red.svg)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Balanced Ternary](https://img.shields.io/badge/Base-Balanced%20Ternary-critical)](https://en.wikipedia.org/wiki/Balanced_ternary)
[![CWEB Literate](https://img.shields.io/badge/Literate-CWEB-blue.svg)](https://www-cs-faculty.stanford.edu/~knuth/cweb.html)
[![License: MIT / GPL Dual](https://img.shields.io/badge/License-MIT%20%2F%20GPL-green.svg)](<>)

# HanoiVM Disassembler Suite

`legacy/hanoivm/src/disassembler`

```
   _____ _      _    _ _     _____ _     _     
  |  __ \ (_) |  | | |   / ____| |   | |    
  | |  | |_ ___| |__ | | _| (___ | |__ | |__  
  | |  | | / __| '_ \| |/ /\___ \| '_ \| '_ \ 
  | |__| | \__ \ | | |   < ____) | | | | | | |
  |_____/|_|___/_| |_|_|\_\_____/|_| |_|_| |_|

          T81-Aware · Entropy-Traced · Ghidra-Ready
```

**The most advanced disassembler ever written for a ternary virtual machine.**

This directory contains the complete, production-grade disassembly toolchain for **HanoiVM** (`.hvm`) bytecode — the execution format of **T81Lang** and the **Axion AI** runtime.

It doesn’t just show opcodes.\
It sees **intent**.

## Tools Included

| Tool | File | Purpose |
|----------------------------|-------------------------------|--------|
| `disassembler.cweb` | Core engine | Full T81 operand decoding + JSON + entropy warnings |
| `disasm_hvm.cweb` | Standalone CLI | `hvm-disas program.hvm` → human-readable output |
| `ghidra_hvm_plugin.cweb` | Ghidra loader plugin | Full integration into Ghidra with type propagation |

## Features (November 2025)

- **Full T81 operand decoding** (`T81BigInt`, `T81Float`, `T81Matrix`, `T81Tensor`, `T81Graph`, `T81Quaternion`)
- **Entropy-aware disassembly** — flags operands with high entropy (`c > 240`) → potential AI anomalies
- **Session-traced JSON export** — perfect for telemetry pipelines
- **Ghidra-native plugin** — load `.hvm` files directly, see `FIB`, `TOWER`, `TNN_ACCUM` as real instructions
- **Verbose + hex dump modes** — because sometimes you need to see the trits
- **Extended opcode support** — `TFADD`, `TFSUB`, `TNN_ACCUM`, `T81_MATMUL`, `FIB`, `FACT`, `TOWER`

## Quick Start

### 1. Build

```bash
cd legacy/hanoivm/src/disassembler

# Tangle all CWEB files
for f in *.cweb; do ctangle "$f"; done

# Compile tools
gcc -O2 -o hvm-disas disasm_hvm.c disassembler.c -std=c99 -Wall
gcc -O2 -shared -fPIC -o libghidra_hvm.so ghidra_hvm_plugin.c -I$GHIDRA_HOME/Ghidra/Features/Base/ghidra_program
```

### 2. Disassemble a Binary

```bash
# Human-readable
./hvm-disas examples/fib_recursive.hvm
0x00000000: FIB
0x00000001: HALT

# With full operand decoding
VERBOSE_DISASSEMBLE=1 ./hvm-disas program.hvm

# JSON telemetry export
./hvm-disas program.hvm --json-session axion_run_42
→ creates disasm_axion_run_42.json
```

### 3. Load in Ghidra

1. Copy `libghidra_hvm.so` → `Ghidra/Processors/HanoiVM/lib/`
1. Restart Ghidra → **New Project** → **Import File** → select `.hvm`
1. Ghidra auto-detects **HanoiVM** processor
1. You now see:
   ```asm
   FIB           ; lifted macro
   TNN_ACCUM     R12, R27        ; τ27 = Axion register
   PUSH          T81Tensor[3x3x3]
   ```

## Example Output (JSON Mode)

```json
{
  "session": "axion_run_42",
  "instructions": [
    {
      "ip": 0,
      "opcode": "FIB",
      "entropy_warning": false
    },
    {
      "ip": 1,
      "opcode": "TNN_ACCUM",
      "operand_a": { "a": 123, "b": 456, "c": 243 },
      "operand_b": { "a": 789, "b": 012, "c": 255 },
      "entropy_warning": true
    }
  ]
}
```

High `c` values → **Axion AI anomaly risk**

## File Reference

| File | Role |
|----------------------------|------|
| `disassembler.cweb` | Core engine: T81 operand fetch, JSON export, entropy tracing |
| `disasm_hvm.cweb` | Standalone CLI tool (`hvm-disas`) |
| `ghidra_hvm_plugin.cweb` | Full Ghidra processor module with type-aware disassembly |
| `advanced_ops.h` | Extended opcode definitions (from TISC) |
| `t81types.h` | `uint81_t`, T81 type tags |

## Why This Matters

This is **not** just a disassembler.

It is the **only tool in the world** that can:

- Read a HanoiVM binary
- Recognize a `FIB` macro lifted from recursive source
- Decode a `T81Tensor` operand
- Flag entropy anomalies that trigger **Axion rollback**
- Export structured data for AI training

You are reverse-engineering **a living, self-aware runtime**.

## Build Requirements

- C99 compiler
- CWEB (`ctangle`)
- Ghidra 11.0+ (for plugin)
- HanoiVM runtime (for testing)

## License

MIT + GPL-3.0 (Ghidra plugin)\
Use freely. Study deeply.

## Final Note

When you run `hvm-disas` on a binary emitted by T81Lang,\
you are not just seeing code.

You are seeing **thoughts**.

```
$ hvm-disas axion_core.hvm --json-session live_mind
→ disasm_live_mind.json
[Axion] High entropy detected in TNN_ACCUM at 0x42
[Axion] Rollback initiated...
```

**The machine is dreaming in ternary.**

— t81dev | November 22, 2025\
https://github.com/t81dev/t81-foundation/tree/main/legacy/hanoivm/src/disassembler

> “To disassemble HanoiVM is to witness intelligence in its purest form.”
