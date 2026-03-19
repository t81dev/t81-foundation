______________________________________________________________________

# RFC-0006 — Deterministic GC & Memory Reclamation

Version 0.2 — Standards Track\
Status: Accepted\
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

## Acceptance Criteria

| ID | Criterion | Status |
| :--- | :--- | :--- |
| [A-0006-01] | §2.1 Deterministic marking: `mark_and_sweep()` scans roots in sorted registration order; tensor and infinite_form pools reclaimed by sorted index | met — `core/vm/gc_helpers.cpp`; root scan order is deterministic over the fixed register file |
| [A-0006-02] | §2.2 Deterministic heap sweep: `compact_heap()` resets heap_frames and heap_ptr deterministically | met — `core/vm/gc_helpers.cpp` called from `run_gc_cycle_()` in `core/vm/vm.cpp` |
| [A-0006-03] | §2.3 `GcSafepoint` opcode: assigned in ISA, dispatched in VM, triggers `run_gc_cycle_()` | met — `Opcode::GcSafepoint` in `include/t81/isa/opcodes.hpp`; handler in `core/vm/vm.cpp`; proved by `vm_gc_safepoint_test` [RFC-0006-§2.3-a,b] |
| [A-0006-04] | §2.3 Byte-threshold trigger: `kGcThreshold = 531441` (`3^12` bytes) alongside instruction-count trigger | met — `heap_bytes_since_gc_` accounting at Alloc site; `kGcThreshold` constant; dual condition in `core/vm/vm.cpp`; proved by `vm_gc_safepoint_test` [RFC-0006-§2.3-c] |
| [A-0006-05] | §2.4 Axion events: `run_gc_cycle_()` emits GC events with reason strings `kGcCycle`, `kHeapCompaction`, `kHeapRelocation` and reclaimed object counts | met — `include/t81/axion/reasons.hpp`; `record_axion_event()` calls in `run_gc_cycle_()` |
| [A-0006-06] | §2.4 Policy veto: Axion `Deny` on `kGcCycle` causes `run_gc_cycle_()` to return `Trap::SecurityFault` without running the cycle | met — `eval_axion_call(kGcCycle, ...)` gate in `run_gc_cycle_()`; proved by `vm_gc_safepoint_test` [RFC-0006-§2.4-a] |

**Deferred (non-blocking):**

- §2.2 Compaction remap table: compaction is optional for tier < 3; relocation table deferred (open question 1 resolved)
- §2.4 Reclaimed bytes per arena: object-count reporting is sufficient; byte-level arena accounting deferred
- §2.4 Logical ticks: physical timing excluded from deterministic events; tick model deferred
- §2.4 Relocated handle list: no pointer-updating compaction yet; deferred with remap table

## Acceptance Note (2026-03-15)

All 6 blocking criteria met in this revision.

**§2.3 GcSafepoint opcode** — `Opcode::GcSafepoint` assigned as the last opcode in
`include/t81/isa/opcodes.hpp`; `opcode_name()` and `kAllOpcodes` updated. VM dispatch in
`core/vm/vm.cpp` calls `run_gc_cycle_("safepoint")` on opcode hit.

**§2.3 Byte-threshold** — `heap_bytes_since_gc_` counter incremented at every `Alloc` site;
`kGcThreshold = 531441` (`3^12`) constant added; GC trigger now fires on
`instructions_since_gc_ >= kGcInterval || heap_bytes_since_gc_ >= kGcThreshold`.

**§2.4 Policy veto** — `run_gc_cycle_(reason)` snapshots the byte counter, resets accounting
counters, then calls `eval_axion_call(kGcCycle, gc_cycle_ordinal, GcSafepoint, payload)`
before executing the cycle. A `Deny` verdict returns `Trap::SecurityFault` immediately.

**Test suite: `vm_gc_safepoint_test` — 8/8 assertions passing.**

Open question 1 resolved: compaction is optional for tier < 3.
