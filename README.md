# T81 — Ternary-Native Runtime for Governed, Deterministic AI Inference

<p align="center">
  <img src="docs/assets/banner.png" alt="T81 — Ternary OS for Auditable AI" width="100%">
</p>

Bit-exact reproducibility • Pre-side-effect policy enforcement • Ternary-weight inference • Immutable, hash-verified artifacts

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

![Release](https://img.shields.io/badge/release-v1.9.5--Stable-blue)
![Tests](https://img.shields.io/badge/tests-404%2F404_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-Apache_2.0-blue)

## In short

T81 is an experimental runtime and (in progress) bare-metal kernel designed for **verifiable, reproducible, and policy-gated AI inference** using balanced ternary arithmetic ({−1, 0, +1}).

It addresses three recurring problems in agentic AI systems:

- Lack of bit-exact reproducibility across hardware
- Difficulty proving exactly which model weights were executed
- Policy enforcement that is usually reactive rather than preventive

T81 approaches this by:

- Using **ternary-native weights and operations** (inspired by BitNet b1.58 and similar) → no floating-point multiplies needed
- Guaranteeing **deterministic execution** with bit-identical traces (CanonHash81) on verified platforms
- Enforcing **policies at the kernel level** before any side effect via the Axion policy engine
- Storing models/code as **immutable, content-addressed blobs** in CanonFS

**Current status (March 2026):** Runs today as a guest runtime (CLI, Docker, Python, QEMU guest). Bare-metal port is in early alpha. No native ternary hardware exists — ternary is emulated on binary CPUs.

## Why now

Agent systems are already acting on our behalf — but:

- we cannot prove exactly what weights ran  
- identical inputs can yield different outputs across platforms  
- policy is typically enforced after execution, not before  

T81 addresses these at the system level.

## Mental model

Think of T81 as:

- a kernel that intercepts AI actions before side effects  
- a runtime that produces identical results everywhere  
- a system where execution can be audited deterministically

If something cannot be reproduced or verified, it should not run.

## Quick start — Try it now

### Docker (easiest, ~60 seconds)

```bash
docker run --rm -it ghcr.io/t81dev/t81-foundation demo
```

Runs hello-world → ternary demo → determinism check → interactive REPL.

### One-line install (macOS/Linux)

```bash
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
t81 repl
```

### QEMU boot demo (most "OS-like" experience)

```bash
# Ubuntu 24.04 deps
sudo apt-get install -y qemu-system-arm qemu-efi-aarch64 mtools cmake ninja-build clang-18 lld-18

git clone https://github.com/t81dev/t81-foundation.git && cd t81-foundation
./drivers/qemu/scripts/boot_demo.sh
```
Watch: EFI → Axion kernel → `t81>` prompt.  

<p align="center">
  <img src="https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/assets/boot.gif" 
       alt="T81 QEMU AArch64 boot sequence — live t81> shell demo" 
       width="95%" style="border:1px solid #ddd; border-radius:8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);">
  <br><small>Current boot progress: EFI → bare-metal EL1 → policy engine → CanonFS mount → interactive t81> prompt</small>
</p>

<br><small>Interactive replay: <a href="https://github.com/t81dev/t81-foundation/blob/main/drivers/qemu/t81-boot.cast">t81-boot.cast (asciinema)</a></small>

At the `t81>` prompt try:

```text
status    # kernel & governance state
policy    # active rules
help
```

## Why ternary? Key technical advantages

Balanced ternary enables several structural improvements for verifiable inference:

1. **Multiplication-free dot products**  
   Ternary weights → conditional ±1 adds instead of FP multiplies  
   → Potential 15–60× energy reduction and large throughput gains vs FP16/FP32 (aligned with BitNet-style results; measured in software emulation)

2. **Zero floating-point drift**  
   Truncation-only rounding, symmetric around zero  
   → Bit-exact **CanonHash81** traces across Linux x86_64 + macOS ARM64 (CI-verified)

3. **Constant-time negation**  
   Simple digit flip (+1 ↔ −1) vs binary carry chains  
   → Measured ~46.9 G-ops/s (PackedCell) — ~10× faster than optimized 64-bit integer negation on same hardware

4. **Trit-level policy interception**  
   TISC ISA is ternary-native → Axion kernel can gate individual operations before side effects

Full benchmarks and methodology → [`benchmarks/results/`](benchmarks/results/)

## Architecture at a glance

| Component       | Purpose                                      | Status      |
|-----------------|----------------------------------------------|-------------|
| **TISC ISA**    | Frozen ternary instruction set               | ❄️ Frozen v1.9.0 |
| **T81VM**       | Deterministic interpreter + Axion hooks      | Stable      |
| **Axion**       | Fail-closed policy engine (APL)              | Stable      |
| **CanonFS**     | Immutable, hash-addressed storage            | Stable      |
| **T81Lang**     | System language → `agent`/`behavior` model   | Stable      |
| **DPE**         | Deterministic parallel executor              | Stable      |
| Bare-metal      | Native boot (no host OS)                     | 🚧 Alpha    |

See detailed diagram and component breakdown in [Architecture →](#architecture)

## Example: T81Lang + Policy

```t81
agent Inference {
  behavior run(prompt: String) -> Tensor {
    // model call gated by Axion
    return Model.forward(prompt);
  }
}
```

```apl
# secure_model.apl
allow infer if model.hash in approved_models;
deny infer reason "unapproved-model";
```

```bash
t81 code run inference.t81 --policy secure_model.apl --weights-model model.t81w --trace
```

## Status & maturity (v1.9.5)

| Surface                  | Maturity    | Notes                                      |
|--------------------------|-------------|--------------------------------------------|
| TISC ISA & core types    | ❄️ Frozen   | No breaking changes in v1.x                |
| Deterministic VM         | Stable      | Bit-identical traces on x86_64 + ARM64     |
| Axion policy & audit     | Stable      | Fail-closed, CanonFS-anchored              |
| Ternary inference opcodes| Stable      | TWMATMUL, TATTN, TQUANT, etc.              |
| QEMU boot (AArch64/x86)  | Complete    | EFI → kernel → interactive shell           |
| Bare-metal target        | Alpha       | In progress                                |
| Cognitive tiers / userland| Beta       | Experimental                               |

404/404 tests passing • Determinism CI gate active

## What T81 is **not** (yet)

- Drop-in replacement for Linux/macOS
- Optimized for legacy/binary software
- GUI-first or general-purpose desktop OS
- Running on real ternary hardware (none exists)

T81 prioritizes **verifiability, determinism, and governance** over broad compatibility. All determinism claims must be strictly bounded by the Determinism Surface Registry.

## License

Apache 2.0

---

Thanks for checking it out.  
Early feedback, issues, and contributors are very welcome.
