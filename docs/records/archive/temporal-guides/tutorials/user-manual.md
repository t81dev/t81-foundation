# T81 User Manual

This manual teaches you how to **install**, **build**, **use**, **debug**, and **extend** programs in the T81 ecosystem — a deterministic, ternary-native runtime stack.

T81 prioritizes **bit-exact reproducibility**, **runtime governance** via Axion, and **auditable execution** over raw performance. It is especially suited for verifiable AI, cryptography, scientific computing, and high-stakes numerical workloads.

**Key invariants you can rely on:**
- Every execution produces the **identical trace** and output on supported platforms (x86_64 Linux, ARM64 Linux/macOS).
- Floating-point transcendental functions are **bit-exact** via the `dmath` backend.
- Axion policies **cannot be bypassed** — violations trigger explicit events/verdicts.

For foundational concepts → see [spec/t81-overview.md](../../../../../spec/t81-overview.md)
For normative specs → see [spec/index.md](../../../../../spec/README.md)

## Table of Contents

1. [Installation & Building](#installation--building)
2. [First Program — Hello, Balanced Ternary](#first-program--hello-balanced-ternary)
3. [Core Workflow: Edit → Compile → Run → Trace](#core-workflow-edit--compile--run--trace)
4. [Working with Data Types](#working-with-data-types)
5. [Tensors & Model Weights](#tensors--model-weights)
6. [Debugging & Inspection](#debugging--inspection)
7. [Trace Analysis & Reproducibility](#trace-analysis--reproducibility)
8. [Axion Policies & Safety](#axion-policies--safety)
9. [Advanced Usage & Integration](#advanced-usage--integration)
10. [Troubleshooting & FAQ](#troubleshooting--faq)
11. [Next Steps & Resources](#next-steps--resources)

## Installation & Building

### Prerequisites

- **CMake** ≥ 3.21
- **Clang** 18+ or **GCC** 14+ (Clang strongly preferred for determinism)
- **Python 3.10+** (for reproducibility gates & some scripts)
- Linux (x86_64/ARM64) or macOS (ARM64)

### Clone & Build

```bash
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# Release build (recommended)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Or Debug build (better for stepping / tracing)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

Run the determinism self-check immediately:

```bash
python3 scripts/ci/t81lang_repro_gate.py --t81-bin build/t81 --fixtures-dir tests/fixtures/t81lang_determinism --workdir build/repro_check --hash-out build/repro_hash.txt
```

Expected output: `T81Lang gates passed: fixtures=...` (identical hash across platforms).

Install the CLI globally (optional):

```bash
sudo cmake --install build
# or just use ./build/t81 directly
```

## First Program — Hello, Balanced Ternary

Create `hello.t81`:

```t81
// hello.t81
fn main() -> i32 {
    let msg = "Hello, Balanced Ternary!";
    print(msg);
    return 0;
}
```

Compile & run:

```bash
./build/t81 compile hello.t81 -o hello.tisc
./build/t81 run hello.tisc
```

Output:

```
Hello, Balanced Ternary!
```

Try changing the string and re-running — the trace hash should remain identical on the same machine.

## Core Workflow: Edit → Compile → Run → Trace

Typical edit-compile-debug loop:

1. Edit `.t81` source (VS Code + C++ extension works well; syntax highlighting coming soon)
2. Syntax & semantics check:

   ```bash
   t81 check hello.t81
   ```

3. Compile to TISC bytecode:

   ```bash
   t81 compile hello.t81 -o hello.tisc
   ```

4. Run:

   ```bash
   t81 run hello.tisc
   ```

5. Inspect:

   ```bash
   t81 disasm hello.tisc
   ```

## Working with Data Types

T81 provides ternary-native types with bounded determinism on verified surfaces.

Common types (see [spec/t81-data-types.md](../../../../../spec/t81-data-types.md)):

```t81
let i   : i32          = 42;
let bi  : T81BigInt    = 123456789t81;
let f   : T81Float     = 3.14159t81;
let frac: T81Fraction  = 22/7t81;
let s   : T81String    = "Hello";
```

All arithmetic is **deterministic** and **bit-exact** (except noted non-strict modes in floats).

Demo files to explore:

- `examples/data_types.t81`
- `examples/all_types_showcase.t81`
- `examples/bigint_demo.t81`

## Tensors & Model Weights

T81 includes first-class tensor support and model import tooling.

Basic tensor ops (currently via VM intrinsics or `T81Vector`):

```t81
let v: T81Vector[i32] = [1, 2, 3];
print(v);
```

Import real model weights:

```bash
t81 weights import path/to/model.safetensors -o model.t81w
t81 weights info model.t81w
```

Load in code (via handles):

```t81
// Weights are loaded into handles at runtime
let w_handle = weights::load("layer1.weight");
```

### Canonizing Tensors

For policy-gated inference (RFC-0025), tensors must be stored in the content-addressed `CanonFS` store.

```bash
t81 canonize-tensor model.t81w
```

This command:
1.  Reads the `.t81w` (or `.safetensors`) file.
2.  Serializes each tensor into the canonical `CanonObject` format.
3.  Calculates the `sha3-256` hash.
4.  Writes the object to `.t81_canonfs/objects/`.
5.  Prints the hash (e.g., `sha3-256:4158a421...`) which can be used in Axion policies (`allowed-tensor-hashes`).

See `examples/weights_load_demo.t81` and `examples/tensor_demo.t81`.

## Debugging & Inspection

- Disassemble bytecode:

  ```bash
  t81 disasm hello.tisc > hello.dis
  ```

- Step-by-step debug:

  ```bash
  t81 debug hello.tisc
  ```

  Inside the debugger:
  - `s`: Step
  - `c`: Continue
  - `r`: Show registers
  - `k`: Show stack
  - `m <addr>`: Inspect memory
  - `b <pc>`: Set breakpoint
  - `q`: Quit

- Check syntax/semantics without execution:

  ```bash
  t81 check examples/advanced_datatypes_showcase.t81
  ```

## Trace Analysis & Reproducibility

T81 execution produces deterministic traces. While direct trace generation via CLI flag is a planned feature, traces are integral to the Axion policy engine and reproducibility gates.

Visualize a trace artifact (e.g., from CI or `repro-hash` output):

```bash
t81 trace show trace.txt
```

Compare traces:

```bash
t81 trace diff trace_a.txt trace_b.txt
```

Replay for forensic analysis:

```bash
t81 trace replay hello.tisc trace.txt
```

Verify cross-build reproducibility of the compiler itself:

```bash
t81 repro-hash tests/fixtures/t81lang_determinism
```

## Axion Policies & Safety

Axion enforces runtime contracts using S-expression based policies (APL).

Define a policy (e.g., `policy.apl`):

```lisp
(policy
  (tier 1)
  (max-instructions 2000)
  (max-stack 128)
  (require-axion-event (reason "GC cycle reason=interval"))
)
```

Compile the policy:

```bash
t81 policy compile policy.apl -o policy.axionb
```

Run with enforcement:

```bash
t81 run hello.tisc --policy policy.apl
```

Violations produce structured events/verdicts in the execution log and may halt the VM.

## Advanced Usage & Integration

- **C++ interop** — see `examples/demo.cpp`, `tensor_ops.cpp`
- **System-level integration** — `examples/system-integration/`
- **Research notebooks** — `notebooks/`
- **Custom policies** — extend Axion kernel (advanced)

See how-to guides in `docs/how-to/`.

## Troubleshooting & FAQ

**Q: Build fails on macOS?**
A: Ensure Xcode command-line tools are installed; use Clang.

**Q: Trace hashes differ?**
A: Check compiler flags, use Release mode, verify determinism gate.

**Q: Float results vary?**
A: Use strict mode or avoid non-core transcendental calls.

**Q: Where is syntax highlighting?**
A: Planned; currently use C-like highlighting.

Report issues → GitHub Issues.

## Next Steps & Resources

- Explore all examples → `examples/`
- Read architecture → [docs/explanation/ARCHITECTURE.md](../../../../explanation/ARCHITECTURE.md)
- Deep dive specs → [spec/index.md](../../../../../spec/README.md)
- AI/research quickstarts → `docs/tutorials/ai-quickstart.md`, `docs/how-to/research-guide.md`
- Contribute → [CONTRIBUTING.md](../../../../../CONTRIBUTING.md)

Welcome to deterministic ternary computing.

MIT License — © t81dev 2026
