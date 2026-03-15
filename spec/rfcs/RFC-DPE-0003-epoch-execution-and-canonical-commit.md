# RFC-DPE-0003: Epoch Execution and Canonical Commit

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81VM runtime, Axion kernel pager model, CanonHash81, T81Float determinism
**Created:** 2026-03-14
**Updated:** 2026-03-14
**Author:** @t81dev
**Depends on:** RFC-DPE-0002 (TISC Task Graph Primitives), RFC-0002 (Deterministic Execution Contract), RFC-00B1 (Ternary MMU), RFC-00B7 (Pager Service ABI)
**Blocks:** first multi-task epoch execution, distributed epoch verification

---

## 1. Summary

This RFC specifies the **epoch execution lifecycle** and the **canonical
commit algorithm** for deterministic parallel execution.  It defines:

- the exact canonical commit ordering rule (§2)
- the delta conflict resolution policy (§3)
- the interaction between epoch execution and the Axion pager/fault model (§4)
- CanonHash81 epoch-level verification (§5)
- epoch abort and retry semantics (§6)
- epoch state machine (§7)

This RFC is the normative counterpart to RFC-DPE-0002; together they specify
the complete deterministic parallel execution model described in RFC-DPE-0001.

---

## 2. Canonical Commit Ordering

### 2.1 Ordering rule

After all tasks in an epoch reach `Complete` state, their delta buffers are
committed to the canonical address space in the following order:

```
tasks are committed in ascending order of TaskId
```

where `TaskId = CanonHash81(canonical_serialise(TaskDesc))` (RFC-DPE-0002 §4)
and ascending order is defined as lexicographic order on the 32-byte
big-endian representation of the hash value.

This rule is:

- **deterministic** — depends only on immutable task descriptor content
- **timing-independent** — hash values are fixed at epoch submission
- **machine-independent** — CanonHash81 is a canonical algorithm with no
  host-specific variation
- **replayable** — the ordering can be reconstructed from the task descriptors
  alone

### 2.2 Commit procedure

For each task T in canonical TaskId order:

1. For each `DeltaRecord` in T's delta buffer, in address-ascending TVA order:
   a. Apply the delta to the epoch's staging area.

2. After all tasks are committed, the staging area becomes the new canonical
   state.

The staging area is a copy-on-write snapshot of the `input_snapshot` updated
by each delta application.  The original `input_snapshot` is unmodified until
the epoch completes successfully.

---

## 3. Delta Conflict Resolution

### 3.1 Non-exclusive regions

When two tasks T_a and T_b both write to the same page within a non-exclusive
output region, the value committed is the value from the task with the
**higher** TaskId in canonical order (last writer wins).

Formally: if `TaskId(T_a) < TaskId(T_b)` and both write page P, then T_b's
delta for page P is applied after T_a's delta for page P, overwriting it.

This rule is deterministic, requires no communication between tasks, and
produces consistent results across all execution orderings.

### 3.2 Exclusive regions

If a task declares `exclusive = true` on an output region (RFC-DPE-0002 §5.5)
and no other task has an overlapping declaration, the region is committed
without conflict checking.

If two tasks both declare exclusive ownership of overlapping regions and the
conflict was not caught at epoch acceptance, the epoch is immediately aborted
at commit time with `ExclusiveRegionConflict`.  This indicates a malformed
epoch graph; the epoch may not be retried without fixing the descriptor.

### 3.3 Rationale

The last-writer-in-canonical-order rule is preferred over alternatives
(e.g. first-writer, abort-on-conflict) because:

- it never requires inter-task communication during execution
- it always produces a complete committed state (no partial writes)
- the outcome is deterministic and auditable
- callers that need strict isolation should use exclusive regions, which
  catch conflicts at epoch acceptance rather than at commit time

---

## 4. Interaction with the Axion Pager / Fault Model

### 4.1 Pre-epoch page setup

Before epoch execution begins the kernel must ensure that all pages in
all declared output regions of all tasks in the epoch are mapped in the
target address space.

This is performed by the **pager service thread** (holder of
`KernelCapabilityKind::PagerService`) using `RequestPageMapping`
(RFC-00B7 §3.2) for each output region page before `SubmitEpochGraph`
returns `Ok`.

The kernel rejects epoch submission (`AddressSpaceNotPagerNeeded` or
`EpochOutputUnmapped`) if any output region page is not mapped at
submission time.

### 4.2 Page faults during task execution

Tasks execute against the epoch's immutable `input_snapshot`.  Page faults
fall into three categories:

| Fault TVA | Classification | Action |
|---|---|---|
| Within a declared input region | `MappingFault` | Task aborted; epoch enters `Aborted` state |
| Within a declared output region | Should not occur (pre-mapped in §4.1) | If occurs, treated as `MappingFault` |
| Anywhere else (scratch, stack) | `OutOfRegionFault` | Task aborted; epoch enters `Aborted` state |

`MappingFault` and `OutOfRegionFault` both abort the epoch (§6).  The
pre-epoch setup requirement in §4.1 means that output region faults should
never occur in a well-formed epoch.

### 4.3 Pager worker interaction

The pager worker's existing `is_pager_work_item_ready()` /
`resolve_completed_pager_work()` path (RFC-00B7) is not invoked during
epoch execution.  Epoch output regions are pre-mapped before execution
begins; the pager worker is not involved in per-task memory resolution.

Normal pager handoffs for non-epoch address spaces continue to be processed
by the pager worker during epoch execution without interference.

---

## 5. CanonHash81 Epoch Verification

### 5.1 Epoch hash computation

After a successful epoch commit, the VM computes an **epoch verification hash**:

```
EpochHash = CanonHash81(
    epoch_id          (uint64, little-endian)
    ∥ input_snapshot  (32-byte CanonHash81 of the input canonical state)
    ∥ committed_deltas_hash
)
```

where `committed_deltas_hash` is:

```
committed_deltas_hash = CanonHash81(
    for each task T in canonical TaskId order:
        TaskId(T) ∥ delta_hash(T)
)

delta_hash(T) = CanonHash81(
    for each DeltaRecord in T's buffer, in TVA-ascending order:
        tva (uint64, little-endian) ∥ value (page_size bytes)
)
```

### 5.2 Verification

The `EpochHash` is published as a CanonFS object of type `EpochVerification`
alongside the committed state.  A verifier replaying the epoch independently
must produce the same `EpochHash` from the same `input_snapshot` and task
descriptors.

Mismatch in `EpochHash` between two independent executions of the same epoch
indicates a determinism violation and must be treated as a hard error.

### 5.3 Interaction with T81_STRICT_DETERMINISTIC_FLOAT

All T81Float operations inside tasks must use the strict deterministic path
(RFC-0030 §3) regardless of host build flags.  The `EpochHash` includes
all floating-point outputs; any non-canonical floating-point result will
produce a divergent hash.

This means epochs that contain floating-point reductions must use the
canonical reduction tree declared in the task dependency graph
(RFC-DPE-0001 §6).

---

## 6. Epoch Abort and Retry

### 6.1 Abort triggers

An epoch enters `Aborted` state when:

- any task produces `TaskFault` (e.g. `OutOfRegionWrite`)
- any task produces `MappingFault` or `OutOfRegionFault` (§4.2)
- `ExclusiveRegionConflict` is detected at commit time
- a task's program produces a policy fault under Axion governance

### 6.2 Abort behaviour

On abort:

1. All in-progress tasks are halted.
2. All accumulated delta buffers are discarded.
3. The canonical state (`input_snapshot`) is unchanged.
4. The epoch's address space is restored to the `input_snapshot` mapping.
5. The epoch is recorded as `Aborted` in the audit log with the faulting
   task ID and fault kind.

Abort is atomic from the perspective of the canonical state — no partial
commits occur.

### 6.3 Retry

An aborted epoch may be retried after the root cause is corrected:

- a `TaskFault` (e.g. `OutOfRegionWrite`) requires fixing the task descriptor
  (widen the output region, or fix the program)
- a `MappingFault` requires fixing the `input_refs` or `input_tvas`
- an `ExclusiveRegionConflict` requires fixing the task graph

The retried submission must produce a new `EpochGraph` with a new `epoch_id`.
Retrying with the identical `EpochGraph` that faulted is not prohibited but
will fault again deterministically.

---

## 7. Epoch State Machine

```
            SubmitEpochGraph (Ok)
                    │
                    ▼
             [Accepted]
                    │  all output regions pre-mapped (§4.1)
                    ▼
             [Executing]
            /         \
     TaskFault       all tasks Complete
           │                  │
           ▼                  ▼
       [Aborted]         [Committing]
                         /         \
              ExclusiveConflict    commit complete
                        │                │
                        ▼                ▼
                    [Aborted]       [Committed]
                                         │
                                   EpochHash published
                                         │
                                         ▼
                                    [Verified]
```

State transitions are recorded in the Axion audit log with sequence numbers
and CanonHash81 signatures.

---

## 8. Governance

Epoch execution is a privileged operation requiring `PagerService` capability
(for output region pre-mapping) and optionally additional governance-gated
capabilities defined by the active Axion policy (RFC-0022).

Policy may restrict:

- maximum number of tasks per epoch
- total output region size per epoch
- task execution budget (TISC instruction count limit)
- permitted opcodes within task programs (e.g. disallow `TLOADHASH` without
  explicit policy approval)

Policy violations during epoch execution trigger an Axion policy fault,
which aborts the epoch (§6.1).

---

## 9. Compatibility

Epoch execution is an additive feature.  Existing TISC programs, Axion
service threads, and pager service threads are unaffected when no epoch is
submitted.  The pager worker continues to operate normally during epoch
execution (§4.3).

---

## 10. Implementation Plan

1. Implement epoch state machine in the Axion kernel (new
   `EpochRuntimeState` struct alongside `PagerWorkerState`)
2. Implement VM task execution mode delta buffer (RFC-DPE-0002 §5)
3. Implement canonical commit loop in `kernel_epoch.cpp`
4. Implement `EpochHash` computation using existing CanonHash81
5. Wire `EpochFault` into the Axion audit log
6. Add conformance tests (`[DPE-03]` series)

---

## 11. Open Questions

- Whether epoch output should be committed to a new CanonFS snapshot object
  (immutable) or directly to the live address space
- Whether an epoch's `EpochVerification` object should be automatically
  published to CanonFS on success (opt-in or always)
- Interaction with the existing `boot_critical` pager path during epoch
  execution — are boot-critical address spaces excluded from epoch output
  regions?

---

## 12. Acceptance Criteria

- `[DPE-03-01]` Canonical commit produces identical canonical state
  regardless of task execution order across independent runs
- `[DPE-03-02]` Last-writer-in-canonical-order conflict resolution produces
  the correct result for overlapping non-exclusive writes
- `[DPE-03-03]` Epoch abort on `TaskFault` leaves canonical state unchanged
- `[DPE-03-04]` `EpochHash` is identical across two independent executions
  of the same epoch against the same `input_snapshot`
- `[DPE-03-05]` T81Float operations inside tasks use the strict deterministic
  path; hash divergence from non-canonical float is detectable
- `[DPE-03-06]` Policy fault during task execution aborts the epoch and
  records the event in the Axion audit log

---

## 13. Acceptance Notes (2026-03-14)

`[DPE-03-01]` through `[DPE-03-04]` are proved by `t81_dpe_epoch_commit_test`
(29 assertions).  `[DPE-03-06]` is proved by `t81_ternaryos_epoch_policy_test`
(15 assertions, Slice 17).

| Criterion | Evidence | Status |
| :--- | :--- | :--- |
| `[DPE-03-01]` Deterministic commit ordering | `test_commit_ordering_is_deterministic` — both submission orders produce identical `committed_pages` and `epoch_hash` | met |
| `[DPE-03-02]` Last-writer-in-canonical-order | `test_last_writer_in_canonical_order_wins` — higher-TaskId task's page survives; lower-TaskId value overwritten | met |
| `[DPE-03-03]` Abort on TaskFault | `test_epoch_aborts_on_task_fault` — `committed_pages` empty, `epoch_hash` zero, `faulting_task_id` set | met |
| `[DPE-03-04]` EpochHash reproducibility | `test_epoch_hash_is_reproducible` — two independent DeltaBuffer sets produce identical `epoch_hash` | met |
| `[DPE-03-05]` T81Float strict path | Deferred — depends on RFC-0030 (Deterministic Math Subsystem) | open |
| `[DPE-03-06]` Policy fault audit | `test_policy_denial_aborts_epoch` — `KernelEpochPolicyGate` denial → `Aborted_PolicyFault`, `EpochAbortedPolicyFault` in audit log, `policy_faults` counter incremented, `epochs_committed` unchanged | met |

**Implementation scope (Slice 15):** Hosted commit engine in
`experimental/dpe/epoch_commit.hpp` / `epoch_commit.cpp`.  Kernel-side
`EpochRuntimeState` wiring (§10 items 1, 3, 5) delivered in Slice 16.
`[DPE-03-06]` policy gate and audit wiring delivered in Slice 17
(`kernel_epoch.hpp` / `kernel_epoch.cpp` / `kernel_runtime_support.hpp`).
`[DPE-03-05]` remains deferred pending RFC-0030.
