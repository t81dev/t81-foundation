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

## What T81 is

T81 is a **deterministic, policy-gated runtime for auditable AI inference**.

It is built around four ideas:

- **Deterministic execution** with bit-identical traces on governed platforms
- **Policy enforcement before side effects** via the Axion policy engine
- **Immutable, content-addressed artifacts** via CanonFS
- **Ternary-native execution paths** for efficient, reproducible inference work

In longer form, T81 is also an in-progress guest OS and bare-metal kernel effort.
But the most useful way to understand it today is as a runtime you can use to:

- run governed inference workloads
- prove which artifacts executed
- reproduce results across verified environments
- keep policy and evidence attached to execution from the start

Determinism claims in this README are bounded by the
[Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md),
which defines the verified platforms, toolchains, and excluded host-dependent
surfaces for those guarantees.

## Why it exists

T81 is aimed at three recurring problems in agentic and model-driven systems:

- identical inputs producing different outputs across platforms
- weak evidence about which weights and artifacts actually ran
- policy enforcement that happens after execution rather than before it

The project’s position is simple:

> If something cannot be reproduced, governed, and audited, it should not be trusted to act.

## What works today

As of March 2026, T81 is usable today as a:

- CLI runtime
- Docker-delivered demo/runtime environment
- Python-integrated execution environment
- QEMU guest with interactive shell and CanonFS-backed runtime surfaces
- CanonFS interchange seed with import/export CLI, schema artifacts,
  provenance records, and contract-tested JSON surfaces

Still experimental:

- bare-metal/native hardware bring-up
- broader OS/userland ambitions beyond the current guest/runtime path
- real ternary hardware targets, which do not exist yet

No native ternary hardware exists today. T81’s ternary execution model runs on conventional binary CPUs.

## Choose your path

- **Try it in 60 seconds:** use Docker and get a working demo plus REPL
- **Use the runtime locally:** install the CLI and run T81 code, policies, and CanonFS flows
- **See the OS direction:** boot the QEMU demo and interact with the `t81>` shell
- **Inspect the architecture:** read the [RFC catalog](spec/rfcs/index.md), [handoff guide](docs/HANDOFF.md), and subsystem docs

If you want the shortest serious maintainer path, read [docs/HANDOFF.md](docs/HANDOFF.md) and then pick a task from [docs/BUILDABLE_NEXT_STEPS.md](docs/BUILDABLE_NEXT_STEPS.md).

## Quick start

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

### End-to-end runtime example

```bash
t81 canonfs import model.t81w --json
t81 code run inference.t81 --weights-model model.t81w --policy secure_model.apl --trace
```

This is the core T81 workflow today:

- import immutable artifacts into CanonFS
- run under Axion policy
- produce deterministic execution evidence
- keep execution tied to content-addressed inputs

The current RFC-00D1 seed is stable enough to build examples and adjacent
tooling against at the JSON contract level:

- `t81.canonfs-import.v1`
- `t81.canonfs-export.v1`
- `t81.canonfs-import-provenance.v1`
- `t81.canonfs-export-provenance.v1`
- `t81.canonfs-interchange-manifest.v1`
- `host-file` / `host-directory`

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

If you want the quickest accurate mental model, start with the runtime and treat the QEMU/bare-metal work as the longer-term systems direction.

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
| **RFC-00D1 seed** | CanonFS import/export + schema contract    | Stable seed |
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

T81 prioritizes **verifiability, determinism, and governance** over broad compatibility.

## License

Apache 2.0

---

Thanks for checking it out.  
Early feedback, issues, and contributors are very welcome.
