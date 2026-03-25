# RFC-00C9 — EL0 Fault Evidence Query

| Field      | Value                                           |
|------------|-------------------------------------------------|
| RFC        | 00C9                                            |
| Title      | EL0 Fault Evidence Query                        |
| Status     | **Accepted**                                    |
| Depends-on | RFC-00C8 (Concurrent Fault Isolation)           |
| Supersedes | —                                               |

---

## 1. Problem Statement

RFC-00C7 introduced retained per-thread fault metadata in the freestanding EL0
scheduler:

- `FsSchedThread.fault_ec`
- `FsSchedThread.fault_far`

RFC-00C8 then proved that this metadata is populated correctly even when the
fault handler context-switches directly to a healthy sibling.

However, prior to RFC-00C9 there was no governed read path for that retained
evidence.  The only query surface was `fs_gov_find_fault(tid, ec)`, which proves
that a fault record exists in the governance ring, but it cannot answer:

- what `FAR_EL1` was recorded,
- whether the per-thread retained state matches the ring record,
- whether a later phase can inspect the fault evidence after control returns to EL1.

RFC-00C9 freezes a minimal read-only query helper for the freestanding scheduler
so post-fault validation can inspect the retained `EC` and `FAR_EL1` pair.

---

## 2. New API

```cpp
// Returns true iff tid exists and is currently Faulted with retained
// fault evidence.  On success writes:
//   *out_ec  = ESR_EL1 Exception Class (bits [31:26])
//   *out_far = FAR_EL1 captured at fault time
extern "C" bool fs_sched_get_fault(uint32_t tid,
                                    uint32_t* out_ec,
                                    uint64_t* out_far) noexcept;
```

Contract:

1. `out_ec` and `out_far` must be non-null.
2. The function returns `false` if:
   - `tid` is unknown,
   - the thread is not in `Faulted` state,
   - no retained fault evidence exists (`fault_ec == 0`).
3. The function is read-only: it must not mutate scheduler state, clear fault
   records, or affect dispatch order.

This helper is EL1-internal only.  It does not yet widen the EL0 KernelCall ABI.

---

## 3. Scheduler Semantics

`fs_sched_fault_handler()` remains the sole writer of retained fault evidence:

```cpp
cur->state     = FsSchedState::Faulted;
cur->fault_ec  = ec;
cur->fault_far = far;
```

RFC-00C9 defines that these fields are stable until the next `fs_sched_reset()`.
No other scheduler path may overwrite them for the same thread slot.

As a result, post-mortem validation performed after the scheduler returns to
EL1 may inspect the captured values deterministically.

---

## 4. Validation Requirement

Phase 18 and Phase 19 are strengthened:

- **Phase 18:** success now requires both
  - governance proof: `fs_gov_find_fault(8, 0x24)`, and
  - retained evidence proof: `fs_sched_get_fault(8) -> {ec=0x24, far=0x0}`.

- **Phase 19:** success now requires both
  - governance proof: `fs_gov_find_fault(7, 0x24)`, and
  - retained evidence proof: `fs_sched_get_fault(7) -> {ec=0x24, far=0x0}`,
  - plus the existing healthy-sibling wake proof:
    `fs_gov_find_device(6, kGovTimerDeviceWake, 30)`.

No CI gate string changes are required.

---

## 5. Why `FAR_EL1` Matters

`EC=0x24` alone distinguishes a Data Abort from an SVC, but it does not prove
which virtual address faulted.

For the current fault test binaries the expected address is `0x0`:

```asm
ldr x0, [xzr]
```

Requiring `fault_far == 0x0` closes the evidentiary gap between:

- "a fault record exists", and
- "the exact intended null-dereference test path executed."

This makes the proof surface more precise without introducing a wider ABI.

---

## 6. Backward Compatibility

RFC-00C9 is additive and read-only:

- no new EL0 syscall or KernelCall ordinal,
- no new scheduler state,
- no new governance record format,
- no change to existing CI gate strings.

Existing phases remain valid; they simply gain stronger verification.

---

## 7. Implementation Files

| File                                           | Change                                                           |
|------------------------------------------------|------------------------------------------------------------------|
| `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp` | `fs_sched_get_fault()` read-only retained-fault query            |
| `ternaryos/hal/canon_exec_loader.cpp`          | Phase 18/19 validation strengthened with `fault_ec` + `fault_far` |
| `spec/rfcs/RFC-00C9-el0-fault-evidence-query.md` | This RFC                                                       |
