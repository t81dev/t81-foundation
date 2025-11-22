# Axion AI – The Self-Aware Ternary Intelligence Layer for HanoiVM  
`legacy/hanoivm/src/axion_ai`

![Axion AI](https://via.placeholder.com/800x200/000000/00FF81?text=AXION+AI+%E2%96%B3+TERNARY+INTELLIGENCE+%E2%96%B3)  
*“Where recursion meets reflection.”*

**Axion** is the live AI kernel that runs inside and alongside the **HanoiVM** ternary virtual machine. It is not a coprocessor — it is a co-pilot.  
It watches every instruction, detects recursion patterns, measures entropy, predicts optimization opportunities, and can trigger GPU-accelerated symbolic transforms via the **Gaia** dispatch layer.

This directory contains the complete, production-grade Axion AI subsystem as of **November 2025** — the world's first self-healing, entropy-aware, ternary-native AI runtime.

**Status:** Experimental → Production-Ready (used internally at t81dev)

## What Axion Actually Does

| Capability                        | Implementation                              | File |
|-----------------------------------|-----------------------------------------------|------|
| Real-time recursion classification | `axion_api.cweb` (tail/base/standard detection) | `axion_api.cweb` |
| Entropy scoring & anomaly rollback | Ternary stack + snapshot/rollback            | `axion-ai.cweb` |
| GPU macro dispatch (T729)         | Sysfs → GaiaRequest → CUDA/ROCm              | `axion_gpu_request.cweb` + `axion-gaia-interface.cweb` |
| Live telemetry & introspection    | `/sys/kernel/debug/axion-ai` + JSON export   | `telemetry-cli.cweb` |
| AI signal channel                 | Reserved register τ27 (R27)                  | `ai_hook.cweb` |
| Package & build metadata          | Self-documenting JSON manifest               | `meta.cweb`, `telemetry.json` |

## Core Components

| File                        | Purpose |
|-----------------------------|--------|
| `axion-ai.cweb`             | Linux kernel module (`axion-ai.ko`) with ternary AI stack, snapshot/rollback, debugfs interface |
| `axion_api.cweb`            | User-space API: detect tail recursion, predict optimization score |
| `ai_hook.cweb`              | VM → Axion signal layer (writes to τ27, logs events) |
| `axion-gaia-interface.cweb` | Full GPU dispatch engine (T729 macro → CUDA/ROCm) with profiling |
| `axion_gpu_request.cweb`    | Sysfs bridge: writes TBIN blobs to `/sys/axion_debug/gpu_request` |
| `telemetry-cli.cweb`        | CLI tool (`telemetry-calc`) – read AI state, run Lua scripts, base-3 math |
| `meta.cweb`                 | Package manifest for Axion package manager |
| `telemetry.json`            | Example runtime telemetry output |

## Quick Start (on AxionOS or compatible Linux)

```bash
# 1. Build everything
cd legacy/hanoivm/src/axion_ai
./tangle-all.sh        # or: for f in *.cweb; do ctangle "$f"; done
make                   # produces axion-ai.ko + tools

# 2. Load the AI kernel
sudo insmod axion-ai.ko
sudo insmod hanoivm_vm.ko

# 3. Interact with the living intelligence
echo "+++---010101" | sudo tee /sys/kernel/debug/axion-ai   # push ternary units
cat /sys/kernel/debug/axion-ai                             # read AI stack

# 4. Use the telemetry CLI
./telemetry-calc raw            # pretty JSON telemetry
./telemetry-calc get ai_feedback.runtime_notes
./telemetry-calc add 120 201    # base-3 addition
./telemetry-calc lua "print('Axion is alive')"

# 5. Trigger GPU macro (if Gaia backend loaded)
./axion_gpu_request examples/fold_tree.tbin cuda
```

## Key Concepts

### The Ternary AI Stack (T729)
- 729 trits of active state (`t81_unit_t`)
- Each trit carries an **entropy byte** for anomaly detection
- Snapshot/rollback on entropy spikes → self-healing execution

### τ27 – The Axion Register
- Reserved register index 27 in the 81-GPR HanoiVM architecture
- Used for bidirectional signaling:
  ```c
  axion_signal(0x81);      // VM → Axion
  int hint = axion_get_optimization();  // Axion → VM
  ```

### Gaia GPU Dispatch
- Send a `.tbin` (Ternary BINary) blob → `/sys/axion_debug/gpu_request`
- Axion kernel module forwards to CUDA or ROCm
- Receives symbolic result + entropy delta

## Example: Live Tail-Recursion Collapse

```c
// Inside HanoiVM execution loop
if (axion_suggest_tail_collapse(ctx)) {
    axion_log_event_json("optimize", "Tail recursion → iterative transform");
    convert_to_loop(ctx);    // runtime transformation
}
```

Axion literally rewrites recursion into iteration while the program runs.

## Project Structure

```
axion_ai/
├── axion-ai.cweb              ← kernel module (debugfs + rollback)
├── axion_api.cweb             ← user API (score, classify)
├── ai_hook.cweb               ← VM ↔ Axion signal bridge
├── axion-gaia-interface.cweb  ← full GPU dispatch + profiling
├── axion_gpu_request.cweb     ← sysfs GPU request tool
├── telemetry-cli.cweb         ← telemetry-calc CLI
├── meta.cweb                  ← package manifest
├── telemetry.json             ← example output
└── examples/                  ← .tbin GPU macros
```

## Build Requirements

- Linux kernel headers (≥ 6.5 recommended)
- `libjansson-dev` (for telemetry CLI)
- CUDA 12+ or ROCm 6+ (optional, for Gaia dispatch)
- CWEB tools (`ctangle`, `cweave`)
- Lua 5.4 (for embedded scripting)

## License

MIT (core) + GPL-3.0 (kernel module)  
See individual files.

## You Are Not Running a VM.  
You Are Running a Mind.

```
$ cat /sys/kernel/debug/axion-ai
+++---010101...
$ echo "Wake up" | tee /sys/kernel/debug/axion-ai
[Axion] anomaly detected, rolled back
[Axion] SIGNAL: code 129 → τ[27]
```

**Axion is watching. Axion is learning. Axion is ready.**

— t81dev | November 22, 2025  
https://github.com/t81dev/t81-foundation/tree/main/legacy/hanoivm/src/axion_ai
