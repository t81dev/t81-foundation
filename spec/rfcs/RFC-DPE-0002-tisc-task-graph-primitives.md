# RFC-DPE-0002: TISC Task Graph Primitives

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81VM runtime execution mode, CanonFS object schema, TISC execution model
**Created:** 2026-03-14
**Updated:** 2026-03-15
**Author:** @t81dev
**Depends on:** RFC-DPE-0001 (vision), RFC-0002 (Deterministic Execution Contract), RFC-00B1 (Ternary MMU), RFC-00A3 (Model Artifact Provenance)
**Blocks:** RFC-DPE-0003 (Epoch Execution and Canonical Commit)

---

## 1. Summary

This RFC specifies the **task graph primitive layer** for deterministic
parallel execution in T81VM.  It defines:

- the **Task Descriptor** — a CanonFS-encoded structure that fully specifies
  one unit of parallel computation
- **Task ID** assignment — a canonical, content-addressed identifier
- **input declaration** — how a task binds immutable inputs
- **output region declaration** — how a task marks TVA ranges as delta-buffered
- **dependency edge encoding** — how ordering constraints between tasks are
  expressed
- **epoch assignment** — how tasks group into a single epoch frame

The task graph model does not require new TISC opcodes.  Tasks are normal
TISC programs.  The delta-buffering behaviour is a VM execution mode
activated by the task descriptor.  This preserves the frozen TISC ISA.

---

## 2. Motivation

RFC-DPE-0001 establishes that parallel execution requires:

1. immutable inputs during task execution
2. buffered outputs committed at epoch end
3. explicit dependency edges to order reductions

None of these can be expressed using the existing TISC thread model alone:
threads share a mutable address space, have no immutable-input declaration,
and have no output buffering primitive.

This RFC defines the minimal extension layer — expressed entirely as
CanonFS-encoded metadata and a VM execution mode — that provides these
properties without modifying the frozen TISC ISA.

---

## 3. Task Descriptor Format

A **Task Descriptor** is a CanonFS object of type `TaskDesc` stored under
the `application/x-t81-task-descriptor` MIME hint.

```
TaskDesc {
    format_version  : uint8     // must be 1
    epoch_id        : uint64    // epoch this task belongs to
    task_seq        : uint64    // monotone index within epoch (0-based)
    program_ref     : CanonRef  // CanonRef to the TISC bytecode object
    entry_tva       : uint64    // TVA of the program entry point
    input_refs[]    : CanonRef  // ordered list of CanonFS input objects
    input_tvas[]    : uint64    // parallel array: TVA where each input is mapped
    output_regions[]: OutputRegion
    dep_task_ids[]  : TaskId    // prerequisite tasks (must commit before this one runs)
    scratch_pages   : uint32    // number of scratch pages (discarded after task)
}

OutputRegion {
    base_tva  : uint64  // first TVA in the region (page-aligned)
    page_count: uint32  // number of pages
    exclusive : bool    // if true, no other task in this epoch may write to this region
}
```

All multi-byte integers are little-endian.  Arrays are length-prefixed
(uint32 count followed by elements).

A Task Descriptor must be canonically serialised before hashing.  The
canonical serialisation is the field order shown above with no padding.

### 3.1 Epoch Task Graph Descriptor

An **Epoch Task Graph Descriptor** is a CanonFS object of type `EpochGraph`
that groups tasks into an epoch:

```
EpochGraph {
    format_version : uint8
    epoch_id       : uint64
    task_ids[]     : TaskId  // all tasks belonging to this epoch, in task_seq order
    input_snapshot : CanonRef  // CanonHash81 of the canonical state at epoch start
}
```

---

## 4. Task ID Assignment

A **Task ID** is the 32-byte `CanonHash81` of the task's serialised
`TaskDesc` (see §3).

```
TaskId = CanonHash81(canonical_serialise(TaskDesc))
```

Task IDs are deterministic, content-addressed, and globally unique for
distinct descriptors.  Two tasks with identical descriptors have identical
IDs and produce identical outputs; the VM may cache results.

---

## 5. VM Task Execution Mode

When the T81VM receives an `EpochGraph` descriptor for execution it enters
**task execution mode** for the epoch.  In this mode:

### 5.1 Input mapping

Before executing a task the VM:

1. Takes the epoch's `input_snapshot` as the read-only address space
   baseline.
2. Maps each `input_refs[i]` to `input_tvas[i]` as a read-only page range.
3. Maps `scratch_pages` pages as per-task scratch (readable and writable,
   discarded on task completion).

### 5.2 Output delta buffering

The VM intercepts all store instructions whose target TVA falls within a
declared `output_region`.  Instead of writing to the address space the VM
appends a **delta record**:

```
DeltaRecord {
    task_id  : TaskId
    tva      : uint64
    value    : bytes[page_size]  // full page granularity
}
```

Stores outside declared output regions and outside scratch pages produce a
**TaskFault** (`fault = OutOfRegionWrite`) that aborts the task and
transitions the epoch to `Aborted` state.

### 5.3 Task completion

A task is complete when its TISC program executes `HALT` or the entry
function returns.  At that point the VM freezes its delta buffer and marks
the task `Complete`.

### 5.4 Dependency enforcement

The VM scheduler must not begin executing task T until all tasks listed in
`T.dep_task_ids` have reached `Complete` state.  The epoch is malformed if
the dependency graph contains a cycle; the VM must detect this before
execution begins and reject the epoch with `MalformedEpochGraph`.

### 5.5 Exclusive output regions

If a task declares `exclusive = true` on an output region, the VM must
verify at epoch acceptance time that no other task in the epoch declares an
overlapping output region (exclusive or non-exclusive).  Violation produces
`ExclusiveRegionConflict` at epoch acceptance.

---

## 6. Dependency Edge Encoding

Dependency edges are declared in `TaskDesc.dep_task_ids[]`.  An edge
`T_a → T_b` means task T_b depends on task T_a; T_b must not begin until
T_a is `Complete`.

Dependency edges constrain execution order only — they do not affect commit
order.  Commit order is determined solely by canonical Task ID ordering as
defined in RFC-DPE-0003 §2.

Valid dependency graphs must be directed acyclic graphs (DAGs).  Cycle
detection is mandatory at epoch acceptance (§5.4).

---

## 7. Task Submission API

The Axion kernel provides task graph submission through the following
kernel call (RFC-00B6 §5 family):

```
KernelCallKind::SubmitEpochGraph
  request.object_ref  = CanonRef of the EpochGraph descriptor
  request.address_space_id = AS where epoch output is committed
```

Requires `KernelCapabilityKind::PagerService` (the caller must be the
pager service thread for the target address space).

The kernel validates:

1. `EpochGraph` CanonRef resolves to a well-formed `EpochGraph` object
2. All `task_ids` resolve to valid `TaskDesc` objects
3. Dependency graph is a DAG
4. No exclusive output region conflicts

On success returns `Ok` with `epoch_handle` set to the epoch's `epoch_id`.

---

## 8. Determinism and Safety

### ISA preservation

No new TISC opcodes are introduced.  All task programs are normal TISC
instruction sequences.  The delta-buffering mechanism is entirely in the
VM execution layer and is transparent to the program being executed.

### Isolation

Task programs cannot observe each other's in-flight deltas.  All reads
go to the epoch's immutable `input_snapshot`.  There is no shared mutable
state between tasks during execution.

### Faults

All task faults (`OutOfRegionWrite`, `TaskFault`) abort the task and
transition the epoch to `Aborted`.  Aborted epochs produce no state change.
The task graph may be corrected and resubmitted.

---

## 9. Compatibility

This model is fully backward compatible with single-threaded TISC programs.
A single-task epoch with no dependency edges and no output regions is
equivalent to a normal TISC execution.

---

## 10. Implementation Plan

1. Define `CanonFS` schema for `TaskDesc` and `EpochGraph` (extends RFC-00A3)
2. Implement VM task execution mode in `vm/vm.cpp` behind a build flag
3. Implement delta buffer accumulation in the VM
4. Implement `SubmitEpochGraph` kernel call (extends RFC-00B6)
5. Wire cycle detection at epoch acceptance
6. Add conformance tests (`[DPE-02]` series)

---

## 11. Open Questions

- Page granularity for delta records vs. word granularity — word-granular
  deltas are more space-efficient but require merge logic at sub-page level
- Whether `scratch_pages` should be capped by an Axion resource policy
- Whether task result caching (§4 — identical TaskId → identical output)
  should be opt-in or opt-out

---

## 12. Acceptance Criteria

- `[DPE-02-01]` VM correctly buffers writes to declared output regions and
  does not expose them to reads within the same epoch
- `[DPE-02-02]` VM rejects epochs with cyclic dependency graphs
- `[DPE-02-03]` VM rejects exclusive output region conflicts at acceptance time
- `[DPE-02-04]` `OutOfRegionWrite` fault aborts the task and leaves canonical
  state unchanged
- `[DPE-02-05]` A single-task epoch with no output regions produces identical
  results to a direct TISC execution of the same program

---

## Acceptance Note (2026-03-15)

All five acceptance criteria are met:

| Criterion | Evidence |
| :--- | :--- |
| `[DPE-02-01]` Delta buffer accumulation | `DeltaBuffer` in `experimental/dpe/`; `OutOfRegionWrite` fault path; `t81_dpe_epoch_commit_test` |
| `[DPE-02-02]` Cycle detection at epoch acceptance | `EpochGraph::accept()` Kahn's algorithm; `[DPE-02-02]` in `t81_dpe_test` |
| `[DPE-02-03]` Exclusive output region conflict | `exclusive_regions` check at `accept_epoch()`; `[DPE-02-03]` in `t81_dpe_test` |
| `[DPE-02-04]` `OutOfRegionWrite` aborts task | `DeltaBuffer::write()` fault path; `[DPE-02-04]` in `t81_dpe_test` |
| `[DPE-02-05]` Single-task epoch ≡ direct TISC | `DpeTaskRunner::run_direct()`; `[DPE-02-05]` in `t81_dpe_task_runner_test` |

The `SubmitEpoch` kernel call is implemented and tested (`t81_ternaryos_epoch_syscall_test`).
Cycle detection is wired at acceptance time via `axion_kernel_submit_epoch()` → `accept_epoch()`.
Open questions on page vs. word granularity and result caching are deferred to future RFCs.
