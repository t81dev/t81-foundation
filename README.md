<p align="center">
  <img src="docs/assets/banner.png" alt="T81 — A Ternary Operating System for AI" width="100%">
  Bootable preview in QEMU · Governed ternary inference · Bit-exact across platforms
</p>


[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation

![Release](https://img.shields.io/badge/release-v1.9.2--Stable-blue)
![Tests](https://img.shields.io/badge/tests-404%2F404_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-Apache_2.0-blue)

**T81 is a ternary operating system for AI.**

Every model you load runs inside a governed, deterministic runtime. The Axion kernel intercepts every AI operation before any side effect occurs. The filesystem is content-addressed and immutable. The ISA replaces floating-point matmul with addition — no multiply unit required. Any AI expressible in ternary weights runs here: verifiably, reproducibly, and under explicit policy control.

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

---

## Demo

Boot T81 on QEMU AArch64 (EDK2 slice6) on any Linux host:

```sh
# Install deps (Ubuntu 24.04)
sudo apt-get install -y qemu-system-arm qemu-efi-aarch64 mtools cmake ninja-build clang-18 lld-18

# Clone and run
git clone https://github.com/t81dev/t81-foundation.git && cd t81-foundation
./drivers/qemu/scripts/boot_demo.sh
```

Expected terminal output:

```text
Axion QEMU AArch64 EDK2 slice6

[axion] bare-metal EL1 kernel entry
[axion] ExitBootServices complete; handing off to C++ kernel

  T81  --  Ternary OS for AI
  ===========================

[axion] policy engine: ready
[axion] canonfs: mounted (in-memory)
[axion] kernel thread tid=1: running

t81> help
  help     -- this message
  version  -- T81 build info
  status   -- kernel counters and governance state
  policy   -- Axion policy summary
t81>
```

Play the pre-recorded session locally with [asciinema](https://asciinema.org):

```sh
asciinema play drivers/qemu/t81-boot.cast
```

The full three-phase boot log is at [`drivers/qemu/sample-boot-log.txt`](drivers/qemu/sample-boot-log.txt). The [`qemu-boot`](.github/workflows/qemu-boot.yml) CI workflow validates this sequence on every push.

---

## Table of Contents

- [Demo](#demo)
- [The OS that AI was missing](#the-os-that-ai-was-missing)
- [Architecture](#architecture)
- [What T81Lang looks like](#what-t81lang-looks-like)
- [Get T81](#get-t81)
- [Status](#status)
- [Boot progress](#boot-progress)
- [CLI reference](#cli-reference)
- [Determinism verification](#determinism-verification)
- [Documentation](#documentation)
- [Governance](#governance)
- [The ternary advantage](#the-ternary-advantage)
- [License](#license)

---

## The OS that AI was missing

Binary operating systems give AI agents a process slot and a filesystem. That's it. They cannot tell you whether an inference was bit-exact, which policy authorized a model load, or whether the weights on disk are the weights that ran. T81 closes that gap — not by layering tooling on top of an existing OS, but by building the kernel, ISA, filesystem, and process model that AI-native computing requires.

### 1. A kernel that governs every AI operation before side effects

When an AI agent takes an action today, there is typically no mechanism to verify *after the fact* what it computed, which policy it applied, or whether the result was altered. T81 fixes this at the instruction level.

The **Axion kernel** intercepts `AgentInvoke`, FFI calls, and every inference opcode in the TISC ISA *before any side effect occurs*. Policy is written in the Axion Policy Language (APL) and is fail-closed — a policy parse failure halts the operation. Every intercepted event is written to a **CanonFS**-anchored audit trail that can be replayed deterministically.

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

### 2. Reproducibility as a kernel invariant, not tooling discipline

IEEE 754 floating-point is inherently platform-sensitive: rounding modes differ, denormal handling varies, FMA availability changes results. AI workloads built on it cannot be reproduced or audited with certainty.

Balanced ternary arithmetic is symmetric around zero. Rounding is truncation — no directional bias, no platform-specific drift. T81's deterministic surfaces produce **CanonHash81 trace hashes that are bit-identical** across every supported platform, verified on every CI run. This is not a property that can be bolted on; it is a consequence of the ISA design.

```sh
t81 determinism verify-run program.tisc
#  Run 1: a3f7c2b1e94d8f20…
#  Run 2: a3f7c2b1e94d8f20…
#  ✓  bit-exact match confirmed
```

Verified platforms: **Linux x86\_64**, **macOS ARM64**. Any divergence on a governed deterministic surface is treated as a critical defect.

### 3. An ISA native to ternary weights — no multiply unit required

Ternary weights {−1, 0, +1} have no fractional component. A dot product over them is a series of conditional add/subtract operations — no multiply required. T81 ships six TISC opcodes that exploit this directly:

| Opcode | Operation |
| :--- | :--- |
| `TWMATMUL` | Ternary weight matrix multiply |
| `TQUANT` | Quantize activations to trit |
| `TATTN` | Ternary attention (Q·Kᵀ over trit weights) |
| `TWEMBED` | Weight embedding lookup |
| `TERNACCUM` | Scalar trit dot product accumulation |
| `TACT` | Activation with Axion ceiling gate |

This aligns with BitNet b1.58 / xTern class models: **15–60× energy reduction**, **4–90× throughput gain** versus FP16/FP32 baselines at comparable accuracy. T81 Ternary Weight (T81WTN) format stores quantized models; `t81 weights import` converts from SafeTensors or GGUF.

```sh
t81 weights import model.safetensors -o model.t81w
t81 code run inference.t81 --weights-model model.t81w --policy secure_model.apl
```

---

## Architecture

T81 is an OS. Every component has an analogue in traditional OS design — built from scratch for ternary semantics and AI-native workloads.

| T81 component | OS analogue | Role |
| :--- | :--- | :--- |
| **TISC ISA** | Instruction set (RISC-V, ARM) | Frozen execution contract; all software compiles to it |
| **T81VM** | Kernel execution engine | Deterministic TISC interpreter; Axion fires on every opcode |
| **Axion** | Security kernel | Fail-closed policy before every side effect; audit-anchored |
| **CanonFS** | Filesystem | Content-addressed, immutable; model weights verified by hash |
| **T81Lang** | System programming language | Compiles to TISC; `agent`/`behavior` are the process model |
| **Agent / Behavior** | Process model | An agent is a process; a behavior is its `main()` |
| **Cognitive Tiers** | Privilege ring hierarchy | Tier 1 (symbolic) → Tier 5 (distributed); governance-bounded |
| **DPE** | Scheduler | Deterministic task graph; epoch-commit atomicity |

```text
┌─────────────────────────────────────────────────────────────┐
│  Interfaces                                                 │
│  t81 studio (Human TUI)   t81 agent (AI-Native TUI)  CLI    │
├─────────────────────────────────────────────────────────────┤
│  T81Lang  — system language                                 │
│  Lexer → Parser → Typed AST → Semantic Analyzer → IRGen     │
│  agent/behavior (RFC-0015)  ·  foreign {} (RFC-0036)        │
├─────────────────────────────────────────────────────────────┤
│  Axion  — kernel                                            │
│  PolicyEngine · CanonFS · Audit Trail · Ethics Gate         │
├──────────────────────────────┬──────────────────────────────┤
│  T81VM  — execution engine   │  DPE  — scheduler            │
│  TISC interpreter            │  EpochGraph · DeltaBuffer    │
│  (deterministic)             │  (RFC-DPE-0002)              │
├──────────────────────────────┴──────────────────────────────┤
│  TISC ISA v1.9.0  ❄️ Frozen  +  Data Types  ❄️ Frozen       │
│  CanonHash81 bit-exact traces across all platforms          │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: T81 Userland · Cognitive Tiers
```

**TISC ISA** — The frozen instruction set. Every piece of software compiles to it. Opcode semantics and wire formats are immutable under v1.x; divergence is a critical defect.

**T81VM** — The execution engine. Axion intercepts fire at the opcode dispatch boundary — before any side effect — keeping the governance path outside the hot interpreter loop.

**Axion** — The kernel. Intercepts `AgentInvoke`, `AXREAD`, `AXSET`, `AXVERIFY`, inference opcodes, and FFI calls before any side effect. Fail-closed on policy parse failure; every event committed to CanonFS. An agent possesses no capabilities by default — every action requires explicit policy authorization.

**CanonFS** — The filesystem. Model weights, code objects, and runtime artifacts are stored as immutable, hash-identified blobs. The Axion kernel verifies that the weights a model loads match the hash in the governing policy, eliminating model-swap attacks at the OS level.

**T81Lang** — The system programming language. Native types: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`, `Option`, `Result`. `agent`/`behavior` declarations are the process model — an agent is a first-class process; a behavior is its entry point. They lower to `AgentInvoke` in TISC. `foreign {}` blocks lower to `FFICall` (RFC-00B8).

**DPE** — The scheduler. Tasks declare immutable inputs; the VM commits all writes atomically at epoch end. Deterministic parallelism over the frozen ISA — no new opcodes required.

---

## What T81Lang looks like

T81Lang is the system programming language of T81. It compiles to TISC bytecode and gives `agent`/`behavior` declarations first-class status — an agent is a process; a behavior is its entry point.

**Basic types and arithmetic:**

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

**Agent / Behavior — the process model:**

```t81
// An agent is a named process. Its behaviors are its entry points.
// The Axion kernel policy-gates every AgentInvoke before execution.
agent Calculator {
  behavior add(a: i32, b: i32) -> i32 {
    return a + b;
  }
}

fn main() -> i32 {
  let result: i32 = Calculator.add(38, 4);
  print(result);   // 42
  return 0;
}
```

**Running and compiling:**

```sh
t81 code run program.t81                          # compile and execute
t81 code build program.t81 -o program.tisc        # compile to bytecode
t81 vm run program.tisc                           # execute bytecode directly
```

**With Axion policy and a weights model:**

```sh
t81 code run inference.t81 \
  --policy        secure_model.apl \
  --weights-model model.t81w \
  --trace
```

**Try it in your browser — no install required:**

> **[Launch the T81Lang Playground →](https://t81dev.github.io/t81-foundation/playground)**
>
> Write and run T81Lang programs directly in the browser. The full compiler + T81VM interpreter runs as WebAssembly. Eight built-in examples: Hello World, BigInt arithmetic, tensors, agent/behavior, and more.

**Interactive exploration (local):**

```sh
t81 repl       # line-buffered REPL; empty line executes
t81 studio     # human operator TUI (7 views, Ctrl+P palette)
t81 agent      # AI-native TUI with /compile /run /hash /allow /infer
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

### Python (pip)

```sh
pip install t81
```

Installs the `t81` Python package for CPython 3.9–3.13 on Linux (x86\_64, ARM64), macOS (Apple Silicon, Intel), and Windows. Provides `T81Int`, `BigInt`, `Float`, `Fraction`, `Tensor`, `HanoiVM`, `CanonFS`, and the full `compile`/`compile_and_run` API. Wheels are published to PyPI on each release via the [`python-wheels`](.github/workflows/python-wheels.yml) workflow.

```python
import t81
result = t81.compile_and_run("fn main() -> i32 { return 42; }")
```

### Build from source

```sh
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure   # 404 tests
```

---

## Status

v1.9.2 · 404/404 tests passing · Apache 2.0

The TISC ISA and core data types are **frozen** under v1.x — opcode semantics and wire formats will not change without a major version bump.

| Component | Maturity | Notes |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.9.0; `AgentInvoke`, 6 ternary-native inference opcodes, 3 FFI, 2 lattice crypto, 1 NTRU-KEM |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — bit-stable encoding |
| **T81VM** | ✅ Stable | Verified deterministic surface; bit-identical traces on Linux x86\_64 + macOS ARM64 |
| **T81Lang** | ✅ Stable | Spec v1.9.0; compiler determinism controls active |
| **Axion** | ✅ Stable | Canonical reason strings, audit hooks, fail-closed policy enforcement |
| **Ternary-Native Inference** | ✅ Stable | RFC-0034 + RFC-0037; all 6 opcodes implemented and evidenced |
| **Lattice Cryptography** | ✅ Stable | RFC-0038 (ternary lattice) + RFC-0039 (NTRU-KEM) |
| **Governed FFI** | ✅ Stable | RFC-00B8 + RFC-0036; `FFIDispatcher`, `FFILibraryRegistry`, `foreign {}` syntax |
| **DPE (Parallel Execution)** | ✅ Stable | RFC-DPE-0001–0009; deterministic epoch semantics |
| **TUI Frontends** | ✅ Beta | `t81 studio` and `t81 agent` — production-usable |
| **Cognitive Tiers** | ✅ Beta | Tier4 Cognition (RFC-0021); governance-bounded |
| **T81 Userland** | ✅ Beta | HAL + userland services; policy-bounded |
| **Native bare-metal target** | 🚧 Alpha | T81 currently runs as a guest OS layer on Linux and macOS; bare-metal execution is in active development |
| **QEMU boot sequence** | 🚧 Alpha | EFI → bare-metal → freestanding C++ bridge confirmed; `t81>` shell live on serial — [see boot progress](#boot-progress) |

Surface classifications follow RFC-0048. Governed non-DCP and experimental surfaces are not presented as verified deterministic components.

---

## Boot progress

Live recording of the current QEMU AArch64 boot sequence (serial output):

<p align="center">
  <img src="https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/assets/boot.gif" 
       alt="T81 QEMU AArch64 boot sequence — live t81> shell demo" 
       width="95%" style="border:1px solid #ddd; border-radius:8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);">
  <br><small>Current boot progress: EFI → bare-metal EL1 → policy engine → CanonFS mount → interactive t81> prompt</small>
</p>

<br><small>Interactive replay: <a href="https://github.com/t81dev/t81-foundation/blob/main/drivers/qemu/t81-boot.cast">t81-boot.cast (asciinema)</a></small>

T81 boots on QEMU AArch64 (EDK2/UEFI). The table below tracks completion toward a clean boot with a shell prompt visible on serial output — the prerequisite for a recorded boot demo in this README.

| Stage | What it covers | Done |
| :--- | :--- | :--- |
| **1. EFI/UEFI boot** | PE32+ EFI binary loads, `ExitBootServices` completes, handoff to bare-metal kernel | 95% |
| **2. Kernel entry + HAL init** | PL011 UART confirmed at EL1; freestanding C++ bridge initializes before shell | 95% |
| **3. EFI ↔ C++ kernel bridge** | Freestanding C++ (`-ffreestanding -fno-exceptions`) compiled into BOOTAA64.EFI; calls banner + shell from real QEMU | 90% |
| **4. CanonFS mount** | In-memory driver always online at boot; persistent driver activates via `T81_CANONFS_ROOT` | 80% |
| **5. Shell / interactive prompt** | Line-buffered `t81>` shell on serial; `help` / `version` / `status` / `policy` commands | 95% |
| **6. Kernel event loop** | Priority dispatch (faults → interrupts → pager → scheduler tick), WFI idle | 100% |
| | **Overall** | **~93%** |

**Current state:** The BOOTAA64.EFI binary is a three-stage image. Phase 1 (EFI) prints the ConOut banner and calls `ExitBootServices`. Phase 2 (bare-metal C) confirms EL1 PL011 MMIO access. Phase 3 (freestanding C++ bridge) prints the governance banner and runs the interactive `t81>` shell — all compiled into a single PE32+ binary with no hosted C++ runtime. The expected serial sequence on a Linux QEMU run:

```text
Axion QEMU AArch64 EDK2 slice6

[axion] bare-metal EL1 kernel entry
[axion] ExitBootServices complete; handing off to C++ kernel

  T81  --  Ternary OS for AI
  ===========================

[axion] policy engine: ready
[axion] canonfs: mounted (in-memory)
[axion] kernel thread tid=1: running

t81>
```

**Remaining to clean boot:** Virtio-blk MMIO driver for persistent CanonFS on bare-metal (so `T81_CANONFS_ROOT` has a real block device behind it in QEMU), and wiring the hosted `KernelRuntimeState` event loop (scheduler, pager, GICv3 interrupts) into the freestanding bridge path so `status` shows live counters.

Boot scripts, disk image, and captured serial output are in [`drivers/qemu/`](drivers/qemu/):

- [`drivers/qemu/scripts/launch_production.sh`](drivers/qemu/scripts/launch_production.sh) — boot the image in QEMU
- [`drivers/qemu/sample-boot-log.txt`](drivers/qemu/sample-boot-log.txt) — confirmed serial sequence from a recent run
- [`drivers/qemu/docs/QEMU_TESTING_RESULTS.md`](drivers/qemu/docs/QEMU_TESTING_RESULTS.md) — full boot test report

The [`qemu-boot`](.github/workflows/qemu-boot.yml) CI workflow builds the EFI binary, assembles a FAT32 GPT image, boots it under QEMU (TCG cortex-a57 + EDK2 AArch64) on every push that touches `userland/experimental/` or `drivers/qemu/`, validates all eight boot markers across all three phases, and commits the updated serial log back to `drivers/qemu/sample-boot-log.txt`.

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
| Inference Benchmark Results | [`benchmarks/results/inference_comparison.md`](benchmarks/results/inference_comparison.md) |

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

T81 Ternary Weight (T81WTN) format and `t81 weights import` make this production-ready in the stack today.

### 5. Trit-level governance hooks

Because the TISC ISA is ternary-native, the Axion kernel can intercept and audit state transitions at **trit-level granularity** before any side effect. This enables fail-closed policy enforcement, fine-grained ethics gates, and deterministic audit trails that are fundamentally more inspectable than black-box binary execution.

---

## License

Apache License 2.0.

---

<details>
<summary>Honest bootstrap note (March 2026)</summary>

T81 is designed as a standalone OS with its own ISA and kernel — but no native ternary hardware exists yet. The current preview runs as a guest layer on Linux/macOS/Windows via binaries, Docker, or QEMU.

This is temporary scaffolding — the same way early Linux ran on simulators before real hardware. Bare-metal boot is in Alpha; the goal is to eventually escape the host OS dependency entirely.

Thanks for reading this far.

</details>
