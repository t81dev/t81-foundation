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
2. Axion-visible reclamation events.
3. Deterministic pause boundaries so T81Lang can reason about latency.

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
2. Should `GC_SAFEPOINT` accept hints (e.g., expected live bytes)?
3. How do we expose GC metrics to T81Lang without leaking physical timing info?

______________________________________________________________________

## Implementation Status Note (2026-03-15)

The core DGC algorithm is substantially implemented. The following table maps each
RFC section to its current implementation status.

| Section | Requirement | Status |
| :--- | :--- | :--- |
| §2.1 Marking | Two-color mark over registers + memory + reflection_snapshots | **IMPLEMENTED** — `mark_and_sweep()` in `core/vm/gc_helpers.cpp`; roots scanned in deterministic registration order |
| §2.1 Marking | Tri-color marking | **PARTIAL** — implementation uses two-color (mark bit); tri-color state machine not used; reclaim results are deterministic regardless |
| §2.1 Mark order | Sorted by handle index | **IMPLEMENTED** — tensor and infinite_form pools reclaimed by sorted index; root scan order is deterministic over the fixed register file |
| §2.2 Sweep | Deterministic heap sweep | **IMPLEMENTED** — `compact_heap()` in `core/vm/gc_helpers.cpp`; resets heap_frames and heap_ptr; called from `run_gc_cycle_()` in `core/vm/vm.cpp` |
| §2.2 Compaction | Optional compaction with remap table | **NOT IMPLEMENTED** — compaction resets the heap frame list; handle remap table (needed for Axion replay of mutations) not recorded |
| §2.3 Trigger | Byte-threshold `GC_THRESHOLD = 3^12 bytes` | **NOT IMPLEMENTED** — current trigger is instruction-count (`kGcInterval = 64`) in `core/vm/vm.cpp`; byte accounting deferred |
| §2.3 Trigger | `GC_SAFEPOINT` opcode | **NOT IMPLEMENTED** — opcode reserved by this RFC; not yet assigned in `include/t81/isa/opcodes.hpp`; explicit-safepoint execution path absent from VM |
| §2.4 Axion | Axion event emitted per GC cycle | **IMPLEMENTED** — `run_gc_cycle_()` emits events with reason strings `kGcCycle`, `kHeapCompaction`, `kHeapRelocation` from `include/t81/axion/reasons.hpp`; reclaimed object counts included |
| §2.4 Axion | Reclaimed bytes per arena | **NOT IMPLEMENTED** — `GcReclaimCounts` reports object counts (tensors, infinite_forms), not byte sizes; arena byte accounting absent |
| §2.4 Axion | Logical ticks (mark vs sweep time) | **NOT IMPLEMENTED** — no tick accounting; physical timing deliberately excluded from deterministic events |
| §2.4 Axion | Relocated handle list | **NOT IMPLEMENTED** — no relocation table; compaction is destructive reset, not pointer-updating |
| §2.4 Axion | Policy veto path | **NOT IMPLEMENTED** — Axion events are emitted but the GC cycle is not gated on policy verdict; no deny/fault path |

**What is not yet implemented that blocks Accepted status:**

1. `GC_SAFEPOINT` opcode — must be assigned in `include/t81/isa/opcodes.hpp` and handled in the VM dispatch loop
2. Byte-threshold trigger — `kGcInterval` instruction-count trigger is sufficient for correctness but violates the `3^12`-byte canonical trigger in §2.3
3. Axion veto — GC cycles must be deniable by policy to satisfy §2.4's tier-limit enforcement
4. Relocation table — required for §2.2's "Axion can replay mutations" guarantee

**Graduation path:** Assign `GcSafepoint` opcode; implement byte-threshold accounting alongside the instruction-count trigger; add a policy verdict gate to `run_gc_cycle_()`; defer relocation table to the first program that uses explicit compaction (open question 1 resolution: compaction is optional for tier < 3).
