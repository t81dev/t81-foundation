# RFC-00B7: Pager Service ABI

**Status:** accepted
**Type:** standards-track
**Applies-To:** Axion kernel ABI, pager service model, capability boundary, fault lifecycle
**Created:** 2026-03-14
**Updated:** 2026-03-14
**Author:** @t81dev
**Depends on:** RFC-00B1 (Ternary MMU), RFC-00B3 (Axion Kernel Architecture), RFC-00B6 (Minimal Syscall and Capability Boundary)
**Blocks:** first real userland pager service, DPE epoch output region pre-mapping (RFC-DPE-0003 §4)

---

## 1. Summary

This RFC defines the public pager service ABI for Axion: the three kernel
calls that allow a privileged userland thread holding the `PagerService`
capability to drive the complete fault→handoff→service→resume lifecycle for a
pager-faulted victim thread.

The three calls are:

| §   | Call                       | Role |
|-----|---------------------------|------|
| 3.2 | `RequestPageMapping`       | Map the faulting TVA after an `Unmapped` fault |
| 3.3 | `WaitForPagerHandoff`      | Park until a pager handoff is dispatched       |
| 3.4 | `ResumePageFaultedThread`  | Un-quarantine victim after mapping is present  |

The `PagerService` capability (§3.1) authorises a process group to exercise
all three calls.

---

## 2. Motivation

### 2.1 Internal pager worker gap

The Axion kernel has a kernel-owned pager worker that tracks `pager_needed`
address spaces and resolves them when a mapping appears.  That internal worker
is sufficient for boot-critical auto-resolution, but it is not sufficient for
general userland page-fault handling because:

- it has no way to learn *what* backing store to load from
- it has no notification channel to a userland service
- it cannot correlate fault TVA semantics (code, data, stack, etc.)

A userland pager service thread must be able to observe fault handoffs, supply
mappings from its own backing store, and resume the faulted thread — all
through the kernel's capability-checked boundary.

### 2.2 DPE epoch output region pre-mapping

RFC-DPE-0003 §4.1 requires that all output region pages in a submitted epoch
graph are pre-mapped before execution begins.  The mechanism is
`RequestPageMapping` called by the pager service thread owning the target
address space.

### 2.3 Relationship to the existing internal pager worker

`RequestPageMapping` and `ResumePageFaultedThread` are additive to the
internal pager worker.  When a pager service thread supplies a mapping via
`RequestPageMapping`, the internal worker may detect it on its next tick and
clear `pager_needed` through the normal resolution path — or the pager service
thread may take explicit ownership of the resume by calling
`ResumePageFaultedThread` before the worker ticks.  Both paths are safe;
double-resolution is prevented by the `pager_needed` gate on
`RequestPageMapping` and the `quarantined` gate on `ResumePageFaultedThread`.

---

## 3. Proposal

### 3.1 PagerService Capability

```
KernelCapabilityKind::PagerService
```

A process group holding `PagerService` is authorised to:

- call `RequestPageMapping` on any address space it owns or that the kernel
  authorises it to service
- call `WaitForPagerHandoff` to block on the next pager handoff dispatch
- call `ResumePageFaultedThread` on any thread in a quarantined pager-fault
  state

`PagerService` is a kernel-seeded or delegated capability.  It is not granted
automatically to every process group.  Calls from threads whose process group
does not hold `PagerService` are rejected with `CapabilityDenied` /
`MissingCapability`.

---

### 3.2 RequestPageMapping

**Purpose:** Install a page mapping for a pager-needed address space.

**Request:**

```
KernelCallKind::RequestPageMapping
  request.address_space_id  = target address space (required)
```

**Preconditions:**

1. Caller holds `PagerService` capability.
2. `address_space_id` is present and resolves to a known address space.
3. `as_state.pager_needed == true` — the address space has an outstanding
   unmapped fault.
4. `as_state.last_pager_fault` is present — the fault record exists with a
   valid TVA and access mode.

**Action:**

The kernel derives page permissions from the fault access mode:

| `MmuAccessMode` | Permissions |
|---|---|
| `Read`    | `{readable=true, writable=false, executable=false}` |
| `Write`   | `{readable=true, writable=true,  executable=false}` |
| `Execute` | `{readable=true, writable=false, executable=true}`  |

Then calls `mmu_map(page_table, allocator, last_pager_fault.tva, as_id,
perms)`.  The mapping is immediately visible in the page table.

Increments `pager_service_mappings` counter.

**Result on success:**

```
status               = Ok
action_performed     = true
pager_mapping_supplied = true
address_space_id     = as_id
```

**Rejection cases:**

| Condition | Status | Rejection |
|---|---|---|
| No `PagerService` capability | `CapabilityDenied` | `MissingCapability` |
| `address_space_id` absent | `InvalidRequest` | `MissingAddressSpace` |
| AS not found | `NotFound` | `MissingAddressSpace` |
| `pager_needed == false` | `InvalidRequest` | `AddressSpaceNotPagerNeeded` |
| `last_pager_fault` absent | `InvalidRequest` | `MissingPagerFault` |

**Notes:**

- `RequestPageMapping` does not clear `pager_needed`.  The internal pager
  worker clears it on its next tick when it detects the mapping via
  `is_pager_work_item_ready()`.
- The call is idempotent with respect to the physical page table — `mmu_map`
  is a no-op if the TVA is already mapped — but the `pager_needed` gate
  prevents a second call once the internal worker has cleared the flag.
- For DPE use (RFC-DPE-0003 §4.1), this call is issued once per output region
  page before `SubmitEpochGraph` returns `Ok`.

---

### 3.3 WaitForPagerHandoff

**Purpose:** Park the calling `PagerService` thread until the kernel dispatches
the next pager handoff notification.

**Request:**

```
KernelCallKind::WaitForPagerHandoff
```

**Preconditions:**

1. Caller holds `PagerService` capability.
2. The caller is not already sleeping.

**Action:**

The kernel calls `scheduler.sleep(caller.tid)` and inserts the TID into
`pager_handoff_waiting_tids`.

When `dispatch_pending_pager_handoff()` next fires in the kernel loop, it
sends a synthetic IPC message to every thread in `pager_handoff_waiting_tids`:

```
CanonMessage {
    sender  = kKernelTid,
    payload = address_space_id,   // the AS entering pager_needed state
    tag     = "pager-handoff-wake",
}
```

Then calls `scheduler.wake(tid)` for each waiting thread, increments
`pager_handoff_wakes`, and clears the set.

**Result on park:**

```
status           = Ok
action_performed = true
thread_sleeping  = true
```

If the scheduler cannot sleep the caller (already sleeping): `RetryLater`.

**Usage pattern:**

```
loop:
    WaitForPagerHandoff                 // parks; woken by kernel dispatch
    msg = ReceiveMessage                // read synthetic IPC → extract as_id
    RequestPageMapping(as_id)           // supply the mapping
    ResumePageFaultedThread(victim_tid) // un-quarantine victim
```

---

### 3.4 ResumePageFaultedThread

**Purpose:** Un-quarantine a victim thread whose pager fault TVA has been
mapped by a prior `RequestPageMapping` call.

**Request:**

```
KernelCallKind::ResumePageFaultedThread
  request.target_tid  = TID of the quarantined victim (required)
```

**Preconditions:**

1. Caller holds `PagerService` capability.
2. `target_tid` is present and resolves to a known thread.
3. Target thread is quarantined (`thread_state.quarantined == true`).
4. Target thread fault inbox is non-empty.
5. Front fault in inbox has `fault == MmuFault::Unmapped` (pager-resolvable).
6. The fault TVA translates successfully — `mmu_translate(page_table, tva)`
   returns a physical address, confirming the mapping was installed.

**Action (`axion_kernel_resume_pager_faulted_thread`):**

1. Pops the `Unmapped` fault record from the thread's `fault_inbox`.
2. Increments `thread_fault_acknowledgements` and `pager_service_resumptions`.
3. Emits `KernelAuditEventKind::ThreadFaultAcknowledged`.
4. If `fault_inbox` is now empty:
   - Decrements `group_state.pending_fault_count`.
   - If `pending_fault_count == 0`: clears `acknowledgement_pending`, `faulted`,
     and `blocked` on the process group.  Pager fault resolution is
     self-contained; no separate supervisor-level ACK is required for the
     common pager-only fault path.
   - Calls `maybe_recover_thread` — sets `quarantined = false`,
     calls `scheduler.wake(tid)`, increments `thread_fault_recoveries`.

**Result on success:**

```
status               = Ok
action_performed     = true
pager_thread_resumed = true
rejection            = None
```

**Rejection cases:**

| Condition | Status | Rejection |
|---|---|---|
| No `PagerService` capability | `CapabilityDenied` | `MissingCapability` |
| `target_tid` absent | `InvalidRequest` | `MissingTargetThread` |
| Thread not found | `NotFound` | `MissingTargetThread` |
| Thread not quarantined, or inbox empty, or front fault ≠ `Unmapped` | `InvalidRequest` | `TargetNotQuarantined` |
| TVA not yet mapped | `InvalidRequest` | `PagerFaultNotResolved` |

**Notes:**

- The `PagerFaultNotResolved` rejection means `RequestPageMapping` has not
  been called yet (or the mapping install failed).  The pager service thread
  must call `RequestPageMapping` first.
- The call is idempotent in the sense that a second call on an already-resumed
  (non-quarantined) thread returns `TargetNotQuarantined` cleanly.
- If the victim's group has additional non-pager faults in other thread inboxes,
  only the front `Unmapped` fault is drained; `pending_fault_count` is
  decremented accordingly.  Recovery proceeds only when the inbox is empty
  and no other pending faults remain.

---

### 3.5 Complete Fault → Handoff → Service → Resume Lifecycle

```
Victim thread hits unmapped TVA
        │
        ▼
Kernel delivers MmuFault::Unmapped to victim's fault_inbox
Victim thread quarantined (scheduler.sleep)
as_state.pager_needed = true
as_state.last_pager_fault = { tva, access_mode }
        │
        ▼
Kernel pager handoff queue (dispatch_pending_pager_handoff)
→ synthetic IPC to pager_handoff_waiting_tids
→ scheduler.wake for each waiting PagerService thread
        │
        ▼
PagerService thread wakes (WaitForPagerHandoff completes)
ReceiveMessage → extract address_space_id from IPC payload
        │
        ▼
RequestPageMapping(address_space_id)
→ mmu_map(page_table, allocator, fault_tva, as_id, perms)
→ pager_service_mappings++
        │
        ▼
ResumePageFaultedThread(victim_tid)
→ validate: quarantined + Unmapped front fault + TVA mapped
→ pop fault from inbox
→ clear group acknowledgement_pending / faulted / blocked (if last fault)
→ maybe_recover_thread: quarantined=false, scheduler.wake(victim_tid)
→ pager_service_resumptions++
        │
        ▼
Victim thread rescheduled; resumes at faulting instruction
```

The internal pager worker continues to operate normally during this flow for
non-service address spaces.  When a `PagerService` thread supplies a mapping
and calls `ResumePageFaultedThread`, the internal worker's next
`is_pager_work_item_ready()` tick on that same AS will find `pager_needed`
still set (it is cleared by the worker tick, not by these ABI calls) but the
TVA is now mapped, so resolution proceeds normally.

---

## 4. Determinism and Safety

**Determinism:**

- `RequestPageMapping` derives permissions from the recorded `last_pager_fault`
  access mode, not from a caller-supplied value.  Permission assignment is
  deterministic given the fault record.
- `WaitForPagerHandoff` parks the thread deterministically; wakeup is driven
  by the kernel's FIFO pager handoff queue, preserving handoff ordering.
- `ResumePageFaultedThread` drains exactly the front `Unmapped` fault from the
  inbox and applies the standard `maybe_recover_thread` path.  All counter
  increments and audit events are deterministic.

**Safety:**

- All three calls require `PagerService` capability.  Unprivileged threads
  cannot observe fault state or map pages via this path.
- `RequestPageMapping` cannot be used to map an arbitrary TVA; it maps exactly
  the `last_pager_fault.tva` recorded by the kernel.
- `ResumePageFaultedThread` validates the TVA is mapped before un-quarantining;
  a thread cannot be resumed with an unmapped fault still outstanding.
- The `TargetNotQuarantined` and `PagerFaultNotResolved` rejections ensure the
  call is idempotent and safe to retry.

**Governance:**

The `PagerService` capability is governed by the standard Axion capability
model (RFC-00B6 §5.4).  Policy may restrict which process groups are granted
`PagerService`, and the Axion policy engine intercepts capability checks at the
kernel boundary (RFC-0022).

---

## 5. Compatibility

- The three calls are additive to the existing kernel ABI.  Existing TISC
  programs and service threads are unaffected when no thread holds
  `PagerService`.
- The internal boot-critical auto-resolution path (kernel-owned) remains
  independent and continues to work for boot-critical address spaces
  regardless of whether a `PagerService` thread is registered.
- `WaitForPagerHandoff` and the internal pager worker handoff path are
  independent.  The internal worker still dispatches handoffs through its own
  FIFO queue; `WaitForPagerHandoff` is a parallel notification channel, not a
  replacement for the internal dispatch.

---

## 6. Implementation Plan

All three calls are implemented and tested as of 2026-03-14:

1. ✅ `PagerService` capability added to `KernelCapabilityKind` (kernel_abi.hpp)
2. ✅ `RequestPageMapping` handler in `kernel_abi.cpp`
3. ✅ `WaitForPagerHandoff` handler in `kernel_abi.cpp`; wake path in `kernel_pager.cpp`
4. ✅ `axion_kernel_resume_pager_faulted_thread` in `kernel_faults.cpp`
5. ✅ `ResumePageFaultedThread` handler in `kernel_abi.cpp`
6. ✅ Counters: `pager_service_mappings`, `pager_handoff_wakes`, `pager_service_resumptions`
7. ✅ `KernelRuntimeStatusView` exposes all three counters
8. ✅ Conformance tests: `[AC-22o]` (34 assertions), `[AC-22p]` (32 assertions), `[AC-22q]` (37 assertions)

---

## 7. Open Questions

- Whether a single process group may hold `PagerService` for multiple address
  spaces simultaneously, or whether `PagerService` should carry an optional
  `address_space_scope` like `FaultObserve`/`FaultAcknowledge`.  Current
  implementation has no scope restriction on `PagerService`; any holder may
  target any address space that is `pager_needed`.
- Whether `WaitForPagerHandoff` should carry an optional filter
  (e.g. `address_space_id`) so a pager service thread can block only on
  handoffs for its own address space.
- Whether `ResumePageFaultedThread` should also clear `pager_needed` on the
  target address space, removing the dependency on the internal pager worker
  tick to complete cleanup.

---

## 8. Acceptance Criteria

- `[AC-22o-01]` `RequestPageMapping` requires `PagerService` capability;
  missing capability returns `CapabilityDenied` / `MissingCapability`.
- `[AC-22o-02]` `RequestPageMapping` on a non-existent AS returns `NotFound` /
  `MissingAddressSpace`.
- `[AC-22o-03]` `RequestPageMapping` on an AS that is not `pager_needed`
  returns `InvalidRequest` / `AddressSpaceNotPagerNeeded`.
- `[AC-22o-04]` `RequestPageMapping` on a `pager_needed` AS installs the
  mapping at `last_pager_fault.tva`; `mmu_translate_checked` returns
  `MmuFault::None` for that TVA immediately after.
- `[AC-22o-05]` `pager_service_mappings` counter advances by exactly 1 per
  successful call; rejected calls do not advance it.
- `[AC-22o-06]` `KernelRuntimeStatusView.pager_service_mappings` reflects
  the counter value.
- `[AC-22p-01]` `WaitForPagerHandoff` requires `PagerService` capability.
- `[AC-22p-02]` `WaitForPagerHandoff` parks the caller; `thread_sleeping` is
  set in the result.
- `[AC-22p-03]` When a pager handoff is dispatched, every thread in
  `pager_handoff_waiting_tids` receives a synthetic IPC with
  `tag = "pager-handoff-wake"` and `payload = address_space_id`.
- `[AC-22p-04]` `pager_handoff_wakes` counter advances once per dispatch.
- `[AC-22p-05]` After wake, `RequestPageMapping` succeeds and the pager worker
  resolves the address space on its next tick.
- `[AC-22q-01]` `ResumePageFaultedThread` requires `PagerService` capability.
- `[AC-22q-02]` `ResumePageFaultedThread` without `target_tid` returns
  `InvalidRequest` / `MissingTargetThread`.
- `[AC-22q-03]` `ResumePageFaultedThread` with a nonexistent TID returns
  `NotFound` / `MissingTargetThread`.
- `[AC-22q-04]` `ResumePageFaultedThread` before `RequestPageMapping` returns
  `InvalidRequest` / `PagerFaultNotResolved`; victim remains quarantined.
- `[AC-22q-05]` After `RequestPageMapping`, `ResumePageFaultedThread`
  succeeds; victim is un-quarantined with an empty fault inbox.
- `[AC-22q-06]` `pager_service_resumptions` advances by exactly 1 per
  successful call.
- `[AC-22q-07]` A second call on a non-quarantined thread returns
  `InvalidRequest` / `TargetNotQuarantined`; counter does not advance.
- `[AC-22q-08]` `KernelRuntimeStatusView.pager_service_resumptions` reflects
  the counter value.
