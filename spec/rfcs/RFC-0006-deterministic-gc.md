______________________________________________________________________

# RFC-0006 — Deterministic GC & Memory Reclamation

Version 0.1 — Draft (Standards Track)\
Status: Draft\
Author: T81VM Working Group\
Applies to: T81VM, TISC, Axion

______________________________________________________________________

# 0. Summary

This RFC defines the **Deterministic Garbage Collector (DGC)** required by
`spec/t81vm-spec.md §4.5` but not yet standardized. The DGC provides:

1. Canonical reclaiming of heap/tensor pools.
1. Axion-visible reclamation events.
1. Deterministic pause boundaries so T81Lang can reason about latency.

______________________________________________________________________

# 1. Motivation

Current prototypes either leak memory (for determinism) or run host-dependent
GCs (breaking determinism). We need a canonical algorithm that:

- preserves execution order
- produces identical pause sequences given identical traces
- exposes GC metadata to Axion and the user-level debugger

______________________________________________________________________

# 2. Design / Specification

### 2.1 Marking Model

- The collector uses **tri-color marking** but in deterministic batches.
- Roots: registers, stack slots, tensor/fraction/option/result pools.
- Mark order is sorted by handle index to remove host-dependent traversal.

### 2.2 Sweep & Compaction

- Heap is divided into canonical arenas; sweep order is deterministic.
- Compaction is optional but, when enabled, MUST update handles via a remap
  table recorded in the trace so Axion can replay mutations.

### 2.3 Trigger Policy

- GC runs:
  - at deterministic byte thresholds (`GC_THRESHOLD = 3^12 bytes`)
  - or when explicitly invoked via `GC_SAFEPOINT` opcode (reserved by this RFC).
- The VM MUST expose upcoming GC windows so T81Lang can insert voluntary
  safepoints (e.g., at loop boundaries).

### 2.4 Axion Integration

- Each GC cycle emits an Axion event containing:
  - reclaimed bytes per arena
  - time spent marking vs sweeping (logical ticks)
  - list of handles relocated
- Axion policies may veto a GC if it would exceed tier limits, forcing the VM
  into a deterministic fault.

______________________________________________________________________

# 3. Rationale

- Sorted traversal ensures deterministic object visitation regardless of
  pointer graph order.
- Safepoints let compilers and Axion coordinate GC pauses with cognitive tier
  scheduling.
- Recording relocation tables preserves referential transparency when compaction
  is enabled.

______________________________________________________________________

# 4. Backwards Compatibility

- Systems without GC can adopt DGC incrementally; until the opcode is emitted,
  semantics are identical to today’s leak-based interpreters.
- Programs that rely on the absence of GC pauses must mark regions with
  `@nogc` (future extension) or accept deterministic safepoints.

______________________________________________________________________

# 5. Security Considerations

- Deterministic GC eliminates timing side channels caused by host memory
  pressure.
- Axion visibility into reclaimed handles prevents covert channel abuse via
  “hidden” heap growth.

______________________________________________________________________

# 6. Open Questions

1. Do we require compaction for tier ≥3, or can it be optional indefinitely?
1. Should `GC_SAFEPOINT` accept hints (e.g., expected live bytes)?
1. How do we expose GC metrics to T81Lang without leaking physical timing info?

______________________________________________________________________
