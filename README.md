# T81 DAIOS — Governed Artificial Intelligence Operating System

<p align="center">
  <img src="docs/assets/banner.png" alt="T81 DAIOS — Governed Artificial Intelligence Operating System" width="100%">
</p>

**A deterministic, capability-secure operating system kernel for policy-gated AI inference, bit-exact reproducibility, and immutable content-addressed storage.**

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

![Release](https://img.shields.io/badge/release-v1.9.5--Stable-blue)
![Tests](https://img.shields.io/badge/tests-393%2F393_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-Apache_2.0-blue)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/t81dev/t81-foundation)

## What is T81 DAIOS

T81 DAIOS (Deterministic Artificial Intelligence Operating System) is a capability-secure operating system built specifically for governed, deterministic AI execution and policy-gated inference.

Instead of running unmediated AI model calls, T81 DAIOS executes AI tasks as governed processes in an EL0/EL1 microkernel environment backed by **durable decision bundles** with immutable identity, complete provenance, and bit-exact replayability.

Policies are evaluated deterministically and enforced before execution or system side-effects occur. Governance is mediated at the kernel boundary via the Axion Governance Kernel reference monitor.

## Architectural Pillars

1. **Axion Governance Kernel** — Reference monitor enforcing security policies before system side-effects or execution state changes occur.
2. **CanonVFS** — Immutable, content-addressed file system guaranteeing binary cryptographic identity (`CanonHash81`).
3. **TISC Micro-Architecture** — Bit-exact ternary execution substrate ensuring deterministic process isolation.
4. **EL0/EL1 Security Boundary** — Microkernel isolation shielding kernel state from userland AI task failures.

## Why This Matters

- **Governance at the Kernel Boundary** - Policy gates state transitions and system calls before execution.
- **Identity Through Provenance** - Every AI process and decision carries an immutable cryptographic audit trail.
- **Bit-Exact Deterministic Replay** - Guaranteed reproducible state transitions across architectures bounded by the [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md).
- **EL0 Fault Containment** - Userland task faults are safely isolated from supervisor execution.
- **Content-Addressed Root FS** - CanonVFS validates executable and model object hashes prior to address-space mapping.

## Real-World Use Cases

- **Governed AI Inference** - Run policy-mediated AI tasks with mandatory access control.
- **Security & Compliance Governance** - Enforce non-bypassable policy rules on all system side-effects.
- **Immutable Auditing** - Archive content-addressed execution bundles and CanonVFS evidence objects.
- **Deterministic Process Execution** - Replay AI decision tasks with bit-identical outcomes.
- **Isolated Kernel Execution** - Execute untrusted AI workloads within sandboxed EL0 task environments.

## System Guarantees

- **Mandatory Policy Control** - Axion mediates state transitions, syscalls, and FFI boundaries fail-closed (`SecurityFault`).
- **Cryptographic Identity** - CanonVFS object hashing guarantees binary and tensor identity.
- **Bit-Exact Determinism** - State transitions ($STATE = (R, PC, SP, FLAGS, MEM, META)$) strictly match across backends.
- **Microkernel Isolation** - Hardware/QEMU ARM64 HAL separates kernel supervision from userland execution.

## Quick Start

### 1. QEMU Bare-Metal Boot Demo (Primary OS Experience)

Experience T81 DAIOS booting directly on an emulated AArch64 bare-metal microkernel target with EL0 userland isolation, Axion policy mediation, and an interactive shell.

```bash
# Install dependencies (Ubuntu example)
sudo apt-get install -y qemu-system-arm qemu-efi-aarch64 mtools parted

# Run the bare-metal boot harness
./drivers/qemu/scripts/boot_demo.sh
```

At the `t81>` prompt, try interactive system commands:
- `status` — View microkernel runtime state and active memory isolation boundaries.
- `policy` — Inspect loaded Axion governance policies.
- `help` — List active Kernelcall ABI operations.

### 2. 30-Second CanonFS + Axion Runtime Proof

Verify immutable storage and pre-side-effect policy denial on host targets:

```bash
tmp_root="$(mktemp -d)"
canon_root="$tmp_root/.t81_canonfs"

# 1. Import artifact into CanonFS
./build/t81 canonfs import \
  examples/storage-and-canonfs/canonfs-interchange/v1/model.t81w \
  --canonfs-root "$canon_root" \
  --json

canon_hash="<imported_objects[0] from step 1>"

# 2. Export artifact back out by hash
./build/t81 canonfs export \
  "$canon_hash" \
  --canonfs-root "$canon_root" \
  --out "$tmp_root/restored.t81w" \
  --json

# 3. Import under a checked-in denying policy
./build/t81 canonfs import \
  examples/storage-and-canonfs/canonfs-interchange/v1/model.t81w \
  --canonfs-root "$canon_root" \
  --policy examples/storage-and-canonfs/canonfs-interchange/v1/policy-deny-all.apl \
  --json
```

What occurs:
- Steps 1 & 2 succeed (`status: "ok"`).
- Step 3 returns `status: "error"` (`kind: "policy-failure"`), demonstrating pre-side-effect enforcement.

### 3. Native Build (Linux / macOS)

```bash
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### 4. Docker Quickstart

```bash
docker run --rm -it ghcr.io/t81dev/t81-foundation demo
```

## Core Subsystems & Maturity

| Subsystem | Role | Maturity |
|---|---|---|
| **Axion Governance Kernel** | Reference monitor mediating syscalls & state transitions | **Stable** |
| **CanonVFS / CanonFS** | Immutable, content-addressed root storage subsystem | **Stable** |
| **TISC ISA** | Standardized 209-opcode ternary instruction set (v1.9.0) | **Frozen** |
| **T81VM** | Deterministic VM interpreter & execution engine | **Stable** |
| **TernaryOS / HAL** | Bare-metal microkernel target (EL0/EL1, MMU, scheduling) | **Stable** |

The **Deterministic Core Profile (DCP)** is a verified deterministic surface bounded by the [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md).

## Project Status & Governance

T81 DAIOS architecture map and sequencing are governed by [RFC-00D2: DAIOS Target Architecture](spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md). Active risks, implementation matrices, and decision logs are tracked in [`docs/status/`](docs/status/).

- See the [DAIOS Roadmap](docs/ROADMAP.md) for planned OS milestones.
- See the [T81 Operator Guide](docs/explanation/T81_OPERATOR_GUIDE.md) for system administration.

## License

Apache 2.0
