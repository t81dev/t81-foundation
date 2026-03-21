<p align="center">
  <img src="assets/banner.png" alt="T81 — A Ternary Computing Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation

![Release](https://img.shields.io/badge/release-v1.9.2--Stable-blue)
![Tests](https://img.shields.io/badge/tests-369%2F369_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-Apache_2.0-blue)

**AI inference you can govern and audit. Bit-exact reproducibility you can prove. Matrix multiply replaced by addition.**

T81 is a virtual machine, instruction set, and compiler stack built on balanced ternary arithmetic {−1, 0, +1}. It intercepts every AI operation before side effects, produces canonical trace hashes that are bit-identical across platforms, and replaces floating-point matmul with addition-only ternary ops — 15–60× lower power than FP16 baselines.

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

---

## Table of Contents

- [Three problems, one stack](#three-problems-one-stack)
- [Get T81](#get-t81)
- [What T81Lang looks like](#what-t81lang-looks-like)
- [Status](#status)
- [Architecture](#architecture)
- [CLI reference](#cli-reference)
- [Determinism verification](#determinism-verification)
- [Documentation](#documentation)
- [Governance](#governance)
- [The ternary advantage](#the-ternary-advantage)
- [License](#license)

---

## Three problems, one stack

### 1. AI operations need governance, not hope

When an AI agent takes an action today, there is typically no mechanism to verify *after the fact* what it computed, which policy it applied, or whether the result was altered. T81 fixes this at the instruction level.

The **Axion Governance Kernel** intercepts `AgentInvoke`, FFI calls, and every inference opcode in the TISC ISA *before any side effect occurs*. Policy is written in the Axion Policy Language (APL) and is fail-closed — a policy parse failure halts the operation. Every intercepted event is written to a **CanonFS**-anchored audit trail that can be replayed deterministically.

```apl
# secure_model.apl — allow inference only for verified model hashes
allow infer if model.hash in approved_models;
deny  infer reason "unapproved-model";
```

```sh
t81 code run inference.t81 --policy secure_model.apl
# Axion: ALLOW  infer  model=sha3:a3f7c2b1…
# Axion: DENY   infer  model=sha3:deadbeef…  reason=unapproved-model
```

### 2. "Same model, same data" should mean identical output, always

IEEE 754 floating-point is inherently platform-sensitive: rounding modes differ, denormal handling varies, FMA availability changes results. AI workloads built on it cannot be reproduced or audited with certainty.

Balanced ternary arithmetic is symmetric around zero. Rounding is truncation — no directional bias, no platform-specific drift. T81's deterministic surfaces produce **CanonHash81 trace hashes that are bit-identical** across every supported platform, verified on every CI run.

```sh
t81 determinism verify-run program.tisc
#  Run 1: a3f7c2b1e94d8f20…
#  Run 2: a3f7c2b1e94d8f20…
#  ✓  bit-exact match confirmed
```

Verified platforms: **Linux x86\_64**, **macOS ARM64**. Any divergence on a governed deterministic surface is treated as a critical defect.

### 3. Neural inference should not require a multiply unit

Ternary weights {−1, 0, +1} have no fractional component. A dot product over them is a series of conditional add/subtract operations — no multiply required. T81 ships six TISC opcodes that exploit this directly:

| Opcode | Operation |
| :--- | :--- |
| `TWMATMUL` | Ternary weight matrix multiply |
| `TQUANT` | Quantize activations to trit |
| `TATTN` | Ternary attention (Q·Kᵀ over trit weights) |
| `TWEMBED` | Weight embedding lookup |
| `TERNACCUM` | Scalar trit dot product accumulation |
| `TACT` | Activation with Axion ceiling gate |

This aligns with BitNet b1.58 / xTern class models: **15–60× energy reduction**, **4–90× throughput gain** versus FP16/FP32 baselines at comparable accuracy. T81WTN weight format stores quantized models; `t81 weights import` converts from SafeTensors or GGUF.

```sh
t81 weights import model.safetensors -o model.t81w
t81 code run inference.t81 --weights-model model.t81w --policy secure_model.apl
```

---

## Get T81

### macOS / Linux

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

Detects OS and CPU architecture, downloads the right binary, installs to `~/.local/bin`. Set `T81_INSTALL_DIR` to override.

### Windows (PowerShell)

```powershell
irm https://github.com/t81dev/t81-foundation/releases/latest/download/install.ps1 | iex
```

Installs to `%LOCALAPPDATA%\t81\bin`.

### Docker — 60 seconds, zero toolchain

```sh
docker run --rm -it ghcr.io/t81dev/t81-foundation demo
```

Pulls a ~100 MB image, runs three programs (Hello World → ternary types → determinism check), then drops into an interactive REPL. No compiler, no CMake, no configuration.

```sh
docker run --rm -it ghcr.io/t81dev/t81-foundation          # REPL only
docker run --rm -it ghcr.io/t81dev/t81-foundation <cmd>    # any t81 subcommand
```

### Prebuilt archives

Direct downloads from the [latest release](https://github.com/t81dev/t81-foundation/releases/latest):

| Platform | Archive |
| :--- | :--- |
| Linux x86\_64 | `t81-<version>-linux-x86_64.tar.gz` |
| Linux ARM64 | `t81-<version>-linux-arm64.tar.gz` |
| macOS Apple Silicon | `t81-<version>-macos-arm64.tar.gz` |
| macOS Intel | `t81-<version>-macos-x86_64.tar.gz` |
| Windows x86\_64 | `t81-<version>-windows-x86_64.zip` |

Each archive uses a standard install layout: `bin/`, `lib/`, `include/`. Place `bin/t81` on your `PATH`.

### Build from source

```sh
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure   # 369 tests
```

---

## What T81Lang looks like

```t81
fn main() -> i32 {
  let greeting: T81String = "Hello, T81!";
  let ratio:    T81Float  = 3.14159t81;
  let big:      T81BigInt = 123456789t81;
  print(greeting);
  print(ratio);
  print(big);
  return 0;
}
```

```sh
t81 code run program.t81          # compile and execute in one step
t81 code build program.t81 -o program.tisc   # compile to bytecode
t81 vm run program.tisc           # execute bytecode directly
```

With Axion governance and a weights model:

```sh
t81 code run inference.t81 \
  --policy   secure_model.apl \
  --weights-model model.t81w \
  --trace
```

Interactive exploration:

```sh
t81 repl       # line-buffered REPL; empty line executes
t81 studio     # human operator TUI (7 views, Ctrl+P palette)
t81 agent      # AI-native TUI with /compile /run /hash /allow /infer
```

---

## Status

v1.9.2 · 369/369 tests passing · Apache 2.0

The TISC ISA and core data types are **frozen** under v1.x — opcode semantics and wire formats will not change without a major version bump.

| Component | Maturity | Notes |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.9.0; `AgentInvoke`, 6 ternary-native inference opcodes, 3 FFI, 2 lattice crypto, 1 NTRU-KEM |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — bit-stable encoding |
| **T81VM** | ✅ Stable | Verified deterministic surface; bit-identical traces on Linux x86\_64 + macOS ARM64 |
| **T81Lang** | ✅ Stable | Spec v1.9.0; compiler determinism controls active |
| **Axion Governance Kernel** | ✅ Stable | Canonical reason strings, audit hooks, fail-closed policy enforcement |
| **Ternary-Native Inference** | ✅ Stable | RFC-0034 + RFC-0037; all 6 opcodes implemented and evidenced |
| **Lattice Cryptography** | ✅ Stable | RFC-0038 (ternary lattice) + RFC-0039 (NTRU-KEM) |
| **Governed FFI** | ✅ Stable | RFC-00B8 + RFC-0036; `FFIDispatcher`, `FFILibraryRegistry`, `foreign {}` syntax |
| **DPE (Parallel Execution)** | ✅ Stable | RFC-DPE-0001–0009; deterministic epoch semantics |
| **TUI Frontends** | ✅ Beta | `t81 studio` and `t81 agent` — production-usable |
| **Cognitive Tiers** | ✅ Beta | Tier4 Cognition (RFC-0021); governance-bounded |
| **TernaryOS** | ✅ Beta | Userland services; policy-bounded |
| **Axion OS** | ✅ Alpha | Governance architecture; not yet a promoted deterministic surface |

Surface classifications follow RFC-0048. Governed non-DCP and experimental surfaces are not presented as verified deterministic components.

---

## Architecture

```text
┌─────────────────────────────────────────────────────────────┐
│  Interfaces                                                 │
│  t81 studio (Human TUI)   t81 agent (AI-Native TUI)  CLI    │
├─────────────────────────────────────────────────────────────┤
│  T81Lang Compiler                                           │
│  Lexer → Parser → Typed AST → Semantic Analyzer → IRGen     │
│  agent/behavior (RFC-0015)  ·  foreign {} (RFC-0036)        │
├─────────────────────────────────────────────────────────────┤
│  Axion Governance Kernel                                    │
│  PolicyEngine · CanonFS · Audit Trail · Ethics Gate         │
├──────────────────────────────┬──────────────────────────────┤
│  T81 Virtual Machine         │  DPE Task Graph Runtime      │
│  TISC interpreter            │  EpochGraph · DeltaBuffer    │
│  (deterministic)             │  (RFC-DPE-0002)              │
├──────────────────────────────┴──────────────────────────────┤
│  TISC ISA v1.9.0  ❄️ Frozen  +  Data Types  ❄️ Frozen       │
│  Deterministic substrate — CanonHash81 bit-exact traces     │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  FFIDispatcher · FFILibraryRegistry                         │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: TernaryOS · Cognitive Tiers
```

**TISC ISA** — Frozen ternary instruction set. The immutable execution contract for the entire stack; all components above compile down to it.

**T81VM** — Deterministic TISC interpreter. Axion pre-dispatch isolation keeps governance hooks outside the hot execution path; bit-identical output is guaranteed on verified surfaces.

**Axion Governance Kernel** — Intercepts `AXREAD`, `AXSET`, `AXVERIFY`, AI opcodes, and FFI calls before any side effect. Fail-closed on policy parse failure; every event anchored to CanonFS.

**CanonFS** — Content-addressed filesystem. Code objects, model weights, and runtime artifacts are stored as immutable, hash-identified blobs, providing provenance for determinism audits.

**T81Lang** — High-level language targeting TISC bytecode. Native types: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`, `Option`, `Result`. `agent`/`behavior` declarations lower to `AgentInvoke`; `foreign {}` blocks lower to `FFICall`.

**DPE** — Task graph parallelism over the frozen ISA. Tasks declare immutable inputs; the VM commits all writes atomically at epoch end. No new opcodes required.

---

## CLI reference

```sh
# Compile and execute
t81 code build <file.t81> -o <file.tisc>
t81 code run   <file.t81|file.tisc> [--policy <apl>] [--weights-model <t81w>] [--trace]
t81 code repl
t81 code check <file.t81>

# VM inspection
t81 vm run   <file.tisc>
t81 vm debug <file.tisc>
t81 vm trace <file.tisc>

# Axion governance
t81 policy compile  <file.apl>
t81 policy validate <file.apl>
t81 axion  status
t81 axion  audit

# Determinism
t81 determinism verify-run <file.tisc>   # run twice, compare hashes
t81 determinism hash       <file.tisc>
t81 determinism certify    <file.tisc>

# Model weights
t81 weights import    <model.safetensors|model.gguf> -o model.t81w
t81 weights info      <model.t81w>
t81 weights verify    <model.t81w>
t81 weights quantize  <input> --to-gguf <out>

# TISC bytecode
t81 tisc disasm   <file.tisc>
t81 tisc validate <file.tisc>
t81 tisc stats    <file.tisc>

# Interfaces
t81 studio   # human operator TUI
t81 agent    # AI-native TUI
```

---

## Determinism verification

```sh
./scripts/ci/run_determinism_slice.sh
```

The CI cross-platform determinism gate runs on every push to `main` and on a daily schedule. Any hash divergence on a verified deterministic surface blocks the merge.

---

## Documentation

| Topic | Location |
| :--- | :--- |
| Getting Started (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Getting Started (AI) | `docs/user-guide/getting-started/ai-quickstart.md` |
| TUI Guide | `docs/user-guide/how-to/tui-guide.md` |
| ISA Specification | `spec/tisc-spec.md` |
| Axion Policy Manual | `docs/user-guide/tutorials/axion-policy-manual.md` |
| T81Lang Stdlib Reference | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Architecture Overview | `docs/architecture/OVERVIEW.md` |
| Governance Charter | `docs/governance/README.md` |
| Project Control Center | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## Governance

T81 Foundation operates under a **Continuous Governance (C2)** model. All contributions must maintain:

- **deterministic execution parity** — trace hashes match across supported platforms
- **architectural coherence** — changes to the deterministic surface require formal review
- **spec authority** — `spec/` > `docs/architecture/` > `docs/`; frozen surfaces require a major version bump

The deterministic surface registry is defined in `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Surface boundary classifications (DCP / governed non-DCP / experimental / out-of-scope) are defined in RFC-0048.

---

## The ternary advantage

While modern binary hardware is highly optimised for general-purpose computing, **balanced ternary** ({−1, 0, +1}) has structural properties that matter specifically in deterministic execution, governed AI inference, and low-complexity neural workloads.

### 1. O(1) negation — zero carry propagation

Binary two's-complement negation is a bitwise NOT followed by +1, which can trigger long carry chains. Balanced ternary negation flips +1 ↔ −1 and leaves 0 unchanged — **no carry, constant time**.

Measured: PackedCell negation reaches **~49.9 G-ops/s** on recent x86\_64 hardware, **~10.9× faster** than optimised 64-bit integer negation (verified on Linux x86\_64 and macOS ARM64).

### 2. Superior radix economy

The information-theoretic optimal radix is *e ≈ 2.718*. Ternary (base 3) is closer than binary (base 2), delivering **~1.585 bits of information per trit** (log₂3). Higher entropy per digit, more compact symmetric ranges — especially useful for weights, embeddings, and sparse tensors.

### 3. Inherent bit-exact determinism

IEEE 754 suffers from platform-specific rounding modes, associativity differences, and denormal handling. Balanced ternary is symmetric around zero: rounding is truncation with no directional bias. Every execution path produces **identical CanonHash81 trace hashes** across supported platforms.

### 4. Multiplication-free neural inference

Ternary weights {−1, 0, +1} reduce dot products to conditional add/subtract — no multiply unit required. Combined with the six TISC inference opcodes:

- 15–60× energy reduction vs FP16/FP32 baselines
- 4–90× throughput gain at comparable accuracy
- Aligns with BitNet b1.58, xTern, and 2024–2026 ternary transformer research

T81WTN weight format and `t81 weights import` make this production-ready in the stack today.

### 5. Trit-level governance hooks

Because the TISC ISA is ternary-native, Axion can intercept and audit state transitions at **trit-level granularity** before any side effect. This enables fail-closed policy enforcement, fine-grained ethics gates, and deterministic audit trails that are fundamentally more inspectable than black-box binary execution.

---

## License

Apache License 2.0.
