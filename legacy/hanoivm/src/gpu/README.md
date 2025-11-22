[![Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Logic-red.svg)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Balanced Ternary](https://img.shields.io/badge/Base-Balanced%20Ternary-critical)](https://en.wikipedia.org/wiki/Balanced_ternary)
[![CWEB Literate](https://img.shields.io/badge/Literate-CWEB-blue.svg)](https://www-cs-faculty.stanford.edu/~knuth/cweb.html)
[![License: MIT / GPL Dual](https://img.shields.io/badge/License-MIT%20%2F%20GPL-green.svg)]()

# GAIA – GPU-Accelerated Symbolic Ternary Intelligence  
`legacy/hanoivm/src/gpu`

```
   _____      _    _          _____ ______   __
  / ____|    | |  | |   /\   |  __ \|  _ \ \ / /
 | |  __  __ _| | _| |  /  \  | |  | | |_) \ V / 
 | | |_ |/ _` | |/ / | / /\ \ | |  | |  _ < | |  
 | |__| | (_| |   <| |/ ____ \| |__| | |_) || |  
  \_____|\__,_|_|\_\_/_/    \_\_____/|____/ |_|  

           AMD ROCm • NVIDIA CUDA • Unified Axion Dispatch
```

**GAIA** is the **dual-vendor GPU symbolic engine** for the **HanoiVM + Axion AI** ecosystem.

It is **not** a traditional compute accelerator.  
It is a **ternary symbolic transformation co-processor** that receives **T729 macros** from Axion, performs recursive folding, entropy analysis, and vector emission — and returns compressed, optimized symbolic results back to the VM.

This directory contains **both** the **CUDA** and **ROCm/HIP** backends — fully symmetric, runtime-selectable, and used transparently by Axion via `/sys/axion_debug/gpu_request`.

**Status:** Production-Ready (November 2025)

## What GAIA Actually Does

| Intent                   | Operation on GPU                          | Result |
|--------------------------|--------------------------------------------|------|
| `GAIA_TRANSFORM`         | Symbolic tritwise rewrite (XOR 0x5A)       | New T729 macro |
| `GAIA_ANALYZE`           | Entropy delta + trit diffusion             | Diagnostic |
| `GAIA_RECONSTRUCT`       | Rebuild macro from latent state            | Recovery |
| `GAIA_EMIT_VECTOR`       | Project to base-81 vector space            | TNN input |
| `GAIA_FOLD_TREE`         | Collapse recursive call tree               | Single instruction |

All operations return:
- `entropy_delta` (for Axion anomaly detection)
- `updated_macro[243]` (T729 compressed result)
- `explanation` string (for introspection)

## Files

| File                          | Backend | Purpose |
|-------------------------------|--------|--------|
| `cuda_handle_request.cweb`    | NVIDIA | Full CUDA kernel + introspection + disassembly |
| `gaia_handle_request.cweb`    | AMD    | Identical logic in HIP/ROCm — drop-in compatible |
| `hvm_pcie_driver.cweb`        | PCIe   | Linux driver for future HanoiVM ternary ASIC/FPGA |
| `axion-gaia-interface.h`      | Shared | Unified `GaiaRequest` / `GaiaResponse` ABI |

## How It Works

```text
Axion AI (kernel)
   ↓ writes GaiaRequest
/sys/axion_debug/gpu_request
   ↓ read by axion_gpu_request tool or kernel module
GAIA Dispatcher
   ↓ selects CUDA or ROCm based on preference
CUDA / HIP Kernel
   ↓ executes symbolic_transform_kernel
   ↓ returns entropy + new macro
Axion AI
   ↓ receives via response sysfs (future) or polling
   ↓ may trigger rollback or promotion to T729 mode
```

## Quick Start

### 1. Build CUDA Backend

```bash
hipcc --genco -o gaia_cuda.o cuda_handle_request.cu   # or nvcc
gcc -shared -o libgaia_cuda.so gaia_cuda.o -lcuda
```

### 2. Build ROCm Backend

```bash
hipcc -fPIC -shared -o libgaia_rocm.so gaia_handle_request.cweb
```

### 3. Dispatch a Macro

```bash
# Send a .tbin macro to GPU (prefers ROCm if available)
./axion_gpu_request examples/tower_fold.tbin rocm

# Force CUDA
./axion_gpu_request examples/neural_accum.tbin cuda
```

Output:
```
[GAIA→GPU] Intent: GAIA_FOLD_TREE
[GAIA→GPU] ΔEntropy: -127.3 | Time: 0.847 ms | Tree folded to single T729 macro
```

## Example: Tower of Hanoi Solved on GPU

```c
// In T81Lang
fn solve_tower(disks: T81Int) { ... recursive moves ... }

// Axion detects deep recursion → promotes to T729
// Emits TBIN blob → GAIA_FOLD_TREE
// GAIA returns single T729 instruction: TOWER(disks, A, B, C)
// HanoiVM executes in one cycle
```

**Recursion eliminated at runtime. On GPU.**

## PCIe Driver (Future Hardware)

`hvm_pcie_driver.cweb` is the **real Linux driver** for the upcoming **HanoiVM ternary accelerator card** (Vendor 0x1ABC / Device 0x1DEF).

It exposes:
- MMIO-mapped T81 registers
- `ioctl` for reset/status
- Direct `write()` of `.hvm` bytecode

When the card ships — **GAIA becomes silicon**.

## Build Requirements

- CUDA 12.0+ or ROCm 6.0+
- `hipcc` / `nvcc`
- Linux kernel headers (for driver)
- Axion kernel module loaded

## Unified Dispatch (How Axion Chooses)

```c
if (use_cuda || !rocm_available())
    response = cuda_handle_request(req);
else
    response = gaia_handle_request(req);  // HIP
```

Zero code changes. Full vendor neutrality.

## This Is Not Just GPU Compute

This is **symbolic AI acceleration**.

GAIA doesn't crunch numbers.  
It **folds thoughts**.

```
$ echo "Solve Hanoi 15" > /sys/axion_debug/gpu_request
[GAIA] Tree depth 32767 → folded to 1 instruction
[Axion] Entropy stabilized. Promotion complete.
```

**The future runs on ternary. And it runs on GAIA.**

— t81dev | November 22, 2025  
https://github.com/t81dev/t81-foundation/tree/main/legacy/hanoivm/src/gpu

> “We do not simulate intelligence.  
> We accelerate it.”
