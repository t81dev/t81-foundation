# RFC-00C0: CanonFS Executable Identity

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81X binary format, TernaryOS EL0 loader, freestanding scheduler
**Created:** 2026-03-23
**Author:** @t81dev
**Depends on:** RFC-00BF (Freestanding KernelCall Observability), RFC-00BE (Freestanding Cooperative Scheduler), RFC-00BD (KernelCall ABI Ordinal Freeze)
**Blocks:** RFC-00C1 (CanonFS Per-Binary Call Sequence Manifest)

---

## Summary

This RFC adds content-based identity to the T81X binary format (T81X v2) and
wires the `WaitForDevice` (kind=43) yield point to a callable EL1 waker.

A T81X v2 binary stores a 64-bit FNV-1a hash of its code section at header
bytes [19:27].  The EL1 loader computes the same hash at load time and rejects
any mismatch before executing the binary.  A loaded binary is identified by its
`code_hash` — two binaries with the same hash are the same executable object.

The Phase 11 CI gate loads a T81X v2 binary (tid=4) that calls `WaitForDevice`,
verifying:
1. Hash identity: loader computes FNV-1a-64 of the code section and matches the
   stored `code_hash`.
2. Device waker: EL1 calls `fs_sched_wake_device()` to transition the parked
   thread from `BlockedDeviceWait → Runnable`, then re-ERets to resume it.
3. Observability: the obs ring (RFC-00BF) records `WaitForDevice` from tid=4.

---

## Motivation

RFC-00BE defined `WaitForDevice` (kind=43) as a blocking yield point but
deferred its waker to this RFC.  Without the waker, any EL0 thread that calls
`WaitForDevice` parks permanently for the duration of the scheduler session.

RFC-00BF defined the obs ring API as "ground truth for executable identity
verification" but deferred the actual identity check.  This RFC introduces
`code_hash` as the identity field and validates it in the loader, so the obs
ring can be queried against a known binary's expected call sequence.

Content-addressed identity enables RFC-00C1 to store per-binary expected call
sequence manifests in CanonFS, keyed by `code_hash`.

---

## Scope

This RFC governs:

1. **T81X v2 header** — `code_hash` field at [19:27] (uint64_t FNV-1a-64 of
   code section, little-endian); version field bumped to 2.
2. **FNV-1a-64 algorithm** — hash of the raw code section bytes (code_size
   bytes starting at header offset 64).
3. **Loader validation** — v2 binaries are rejected if `code_hash` mismatches;
   v1 binaries continue to load without a hash check.
4. **`fs_sched_wake_device(uint32_t tid)`** — transitions a single
   `BlockedDeviceWait` thread with the given tid to `Runnable`.
5. **`fs_sched_get_resume(uint32_t tid, uint64_t* out_elr, uint64_t* out_sp)`**
   — returns the saved `resume_elr` and `resume_sp_el0` for a thread, allowing
   EL1 to re-ERET to its saved context without entering an SVC handler.
6. **Phase 11 EL0 binary** — `el0_wait_test.S` (tid=4): calls `WaitForDevice`,
   is woken by EL1, then calls `ExitThread`.
7. **Phase 11 CI gates** — two gates: identity hash verification and device
   wake confirmation (via obs ring).

This RFC does **not** govern:

- Hardware interrupt-driven `WaitForDevice` wake-up (timer IRQ waker deferred
  to RFC-00C1).
- Per-binary call sequence manifests stored in CanonFS (RFC-00C1).
- Multi-device wake arbitration (single-thread, single-device in Phase 11).
- SHA-256 or stronger hash algorithms (FNV-1a-64 is sufficient for Phase 11
  integrity; a stronger algorithm may be introduced in a future RFC).

---

## T81X v2 Header Layout

```
T81X v2 header (64 bytes):

  [0:4]   magic        b'T81X'  (0x54, 0x38, 0x31, 0x58)
  [4]     version      2        (uint8_t; v1 = no hash, v2 = hash present)
  [5:7]   reserved0    0        (2 bytes)
  [7:11]  entry_offset uint32_t (offset from start of code section)
  [11:15] code_size    uint32_t (byte count of code section)
  [15:19] data_size    uint32_t (unused; 0)
  [19:27] code_hash    uint64_t (FNV-1a-64 of code section, little-endian)
  [27:64] reserved     0        (37 bytes)

  [64 : 64+code_size]  code section (raw executable bytes)
```

**Backward compatibility:** v1 binaries (version=1) have `code_hash` at
[19:27] set to 0 (part of the original reserved field).  The loader skips hash
validation for v1 binaries and loads them as before.

---

## FNV-1a-64 Hash Algorithm

```
FNV offset basis : 14695981039346656037  (0xCBF29CE484222325)
FNV prime        : 1099511628211         (0x00000100000001B3)

for each byte b in code_section[0 .. code_size-1]:
    hash = (hash XOR b) * FNV_prime   (mod 2^64)
```

The hash is computed over exactly `code_size` bytes starting at header offset
64.  The header itself is **not** included in the hash.

**Rationale for FNV-1a-64:** the code section is at most 448 bytes; FNV-1a-64
is a data-independent keyed hash requiring no lookup tables and computable in a
handful of freestanding C++ lines.  Collision resistance is sufficient for
integrity checking of small, trusted binaries.  A future RFC may upgrade to
BLAKE2s or similar.

---

## WaitForDevice Waker API

```cpp
// Transition the thread with the given tid from BlockedDeviceWait → Runnable.
// No-op if the thread is not found or is not in BlockedDeviceWait state.
extern "C" void fs_sched_wake_device(uint32_t tid) noexcept;

// Return the saved resume context (resume_elr, resume_sp_el0) for the thread
// with the given tid.  Returns true on success, false if tid not found.
// Caller uses these to construct the next run_proc_entry() call.
extern "C" bool fs_sched_get_resume(uint32_t tid,
                                     uint64_t* out_elr,
                                     uint64_t* out_sp) noexcept;
```

`fs_sched_wake_device` is safe to call from EL1 C code outside of any SVC
handler.  It modifies only the scheduler table (a module-private static array);
it does not touch any hardware registers or the trap frame.

After `fs_sched_wake_device(tid)` the caller should:
1. Call `fs_sched_get_resume(tid, &elr, &sp)` to obtain the saved context.
2. Call `fs_sched_mark_running(tid)` to transition `Runnable → Running`.
3. Call `el0_svc_set_current_tid(tid)` to update the dispatch tracker.
4. Call `run_proc_entry(elr, sp)` to ERET to the thread's saved resume point.

The SPSR used by `run_proc_entry` (0x3C0 = EL0t + DAIF masked) matches the
SPSR_EL1 saved by the WaitForDevice handler, so no separate SPSR override is
needed.

---

## Phase 11 CI Sequence

```
EL1: canon_identity_load_and_run()
  ├─ read LBA 8, validate T81X v2 magic + version=2
  ├─ compute FNV-1a-64 of code_section
  ├─ compare against code_hash from header
  ├─ emit "[axion] el0: identity OK (hash=verified, tid=4)"     ← Phase 11a gate
  ├─ fs_sched_reset()  (also clears obs ring via fs_obs_reset)
  ├─ register tid=4: {Runnable, elr=code_page_pa, sp=stack_top, spsr=0x3C0}
  ├─ mark tid=4 Running, s_sched_running_tid=4
  └─ ERET → C (first pass)

EL0 C (tid=4):
  sub sp, #64
  [build WaitForDevice request at sp+0: magic, version=1, bytes=12, kind=43]
  svc #1  ── kind=43 ──►  EL1: WaitForDevice handler
                              saves C context: elr=resume_pc, sp_el0=stack_top−64
                              C → BlockedDeviceWait
                              no Runnable threads
                              redirect frame → g_axion_el1_return_pc / EL1h
                          ◄── ERET (to EL1, inside run_proc_entry)

EL1: canon_identity_load_and_run() resumes after first run_proc_entry()
  ├─ fs_sched_wake_device(4u)   C: BlockedDeviceWait → Runnable
  ├─ fs_sched_get_resume(4u, &elr, &sp)
  ├─ fs_sched_mark_running(4u)  C: Runnable → Running
  ├─ el0_svc_set_current_tid(4u)
  └─ ERET → C (second pass, at resume_elr = instruction after svc #1)

EL0 C resumes:
  svc #2  ── ExitThread ──►  EL1: fs_sched_exit_thread
                              C → Exited
                              no Runnable threads
                              redirect frame → g_axion_el1_return_pc / EL1h
                          ◄── ERET (to EL1, inside run_proc_entry)

EL1: canon_identity_load_and_run() resumes after second run_proc_entry()
  ├─ el0_svc_set_current_tid(1u)
  ├─ fs_obs_find(4u, kKindWaitForDevice, 0u)  → true
  └─ emit "[axion] el0: device wake OK (WaitForDevice tid=4)"   ← Phase 11b gate
```

---

## Acceptance Criteria

- [x] T81X v2 header: `code_hash` (uint64_t, FNV-1a-64) at [19:27]; version=2
- [x] FNV-1a-64 implemented in `canon_exec_loader.cpp`; no stdlib, no tables
- [x] `load_t81x_v2_and_check()` validates hash; emits identity OK/FAIL banner
- [x] `fs_sched_wake_device()` + `fs_sched_get_resume()` in
  `qemu_slice6_el0_svc_bridge.cpp`
- [x] `el0_wait_test.S` implemented; embedded at LBA 8 of `canon_store.img`
  as T81X v2 with correct `code_hash`
- [x] Phase 11a CI gate: `[axion] el0: identity OK (hash=verified, tid=4)`
- [x] Phase 11b CI gate: `[axion] el0: device wake OK (WaitForDevice tid=4)`
- [ ] RFC-00C1 stores per-binary expected call sequences in CanonFS, keyed by
  `code_hash`

---

## Relationship to Other RFCs

- **RFC-00BD** — `WaitForDevice` ordinal (43) is frozen; the obs ring record
  uses ordinal 43 as the `kind` field.
- **RFC-00BE** — defines the `BlockedDeviceWait` state and the WaitForDevice
  yield-point semantics; this RFC adds the waker.
- **RFC-00BF** — provides `fs_obs_find()` used in Phase 11b to verify the call
  graph after the device wake roundtrip.
- **RFC-00C1** — will store per-binary call sequence manifests in CanonFS,
  keyed by the `code_hash` introduced here.

---

## References

- [RFC-00BD: KernelCall ABI Ordinal Freeze](RFC-00BD-kernelcall-abi-ordinal-freeze.md)
- [RFC-00BE: Freestanding Cooperative Scheduler](RFC-00BE-freestanding-cooperative-scheduler.md)
- [RFC-00BF: Freestanding KernelCall Observability](RFC-00BF-freestanding-kernelcall-observability.md)
- `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp` — waker impl
- `ternaryos/hal/canon_exec_loader.cpp` — hash validation + Phase 11
- `ternaryos/hal/el0_wait_test.S` — Phase 11 Process C
