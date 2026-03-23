# RFC-00BF: Freestanding KernelCall Observability

**Status:** accepted
**Type:** standards-track
**Applies-To:** TernaryOS freestanding EL0 SVC bridge, KernelCall dispatch path
**Created:** 2026-03-23
**Author:** @t81dev
**Depends on:** RFC-00BD (KernelCall ABI Ordinal Freeze), RFC-00BE (Freestanding Cooperative Scheduler)
**Blocks:** RFC-00C0 (CanonFS Executable Identity)

---

## Summary

This RFC adds a fixed-capacity observability ring to the freestanding KernelCall
dispatch path.  Every SVC #1 dispatch that passes TVA validation records a
`FsObsRecord` — caller tid, call kind, outcome status, and an optional peer tid —
into a 32-slot ring indexed by a monotonic sequence counter.  No wall-clock
timestamps; sequence number is the sole ordering primitive.

The ring enables post-hoc verification of the execution graph:
**Thread → KernelCallKind** (call edges) and **ThreadA → ThreadB** (IPC edges).
The Phase 10 CI gate proves that the Phase 9 cooperative scheduler produced the
correct record set: one `BlockOnIpcReceive` from tid=3 and one `SendMessage`
from tid=2 to tid=3.

---

## Motivation

RFC-00BE proved the cooperative scheduler round-trip by checking a single
boolean flag (`s_sched_ipc_delivered`).  That flag answers "did IPC happen?" but
not "in what order?", "which thread called what?", or "was the response status
correct?".

RFC-00BD froze `kind` as a stable uint32 field.  A ring that stores `kind` values
is meaningful precisely because ordinals cannot drift.

RFC-00C0/C1 (CanonFS executable identity) will need to verify that a loaded binary
produced the expected KernelCall sequence.  The observability ring provides the
ground truth for that comparison.

Adding the ring now, before RFC-00C0, means RFC-00C0 can reference the ring API
rather than defining its own tracing mechanism.

---

## Scope

This RFC governs:

1. **`FsObsRecord` struct** — per-event record layout (6 × uint32, 24 bytes).
2. **Ring buffer** — 32-slot fixed-capacity ring with monotonic `seq_id`.
3. **Recording discipline** — which dispatch outcomes get recorded and where in
   the dispatch path the record is written.
4. **Reset contract** — `fs_obs_reset()` clears the ring; called by `fs_sched_reset()`.
5. **Verification API** — `fs_obs_find()` for post-run CI assertions.
6. **Phase 10 CI gate** — verifies Phase 9 records are correct.

This RFC does **not** govern:

- Wall-clock timestamps (deliberately absent; sequence ordering is sufficient).
- Per-call ring segments or per-thread rings (a single global ring is sufficient).
- Audit-trail persistence to CanonFS (that is RFC-00C0 territory).
- SVC #2 (ExitThread) recording — SVC #2 is not a KernelCall; it is handled by
  the exception vector dispatcher, not `el0_svc_kernel_call_dispatch`.

---

## Record Layout

```
FsObsRecord (24 bytes, 6 × uint32_t):

  [0:4]   seq_id    — monotonic index of this record (low 32 bits of s_obs_seq)
  [4:8]   tid       — caller tid (s_current_el0_tid at dispatch time)
  [8:12]  kind      — KernelCallKind ordinal (frozen, RFC-00BD)
  [12:16] status    — 0 = Ok, 1 = InvalidRequest
  [16:20] peer_tid  — SendMessage: ipc_dst; else 0
  [20:24] _reserved — zeroed; reserved for future RFC
```

**Immutability:** A record is written once and never modified.  The ring slot
holding the record may be overwritten after 32 additional records are emitted
(oldest-first wrap).

---

## Ring Buffer

```
Capacity:  32 slots (kObsRingCap)
Size:      768 bytes (32 × 24)
Ordering:  s_obs_seq is the sole ordering primitive
Write:     s_obs_ring[s_obs_seq % 32] ← record; ++s_obs_seq
Live set:  min(s_obs_seq, 32) most-recent records
```

The ring is a module-private static array in `qemu_slice6_el0_svc_bridge.cpp`.
It is not exported directly; access is through the `fs_obs_*` API.

---

## Recording Discipline

A record is written at the **point of outcome determination** for each dispatch
case — after the outcome (status, peer_tid) is known but before any context
switch modifies the frame.

| Kind | Recorded peer_tid | When |
| :--- | :--- | :--- |
| `GetThreadIdentity` (10) | 0 | After writing caller_tid to response |
| `SendMessage` (13) | `ipc_dst` from req[12:16] | After delivering (or not) to blocked receiver |
| `ReceiveMessage` (14) | 0 | After writing (or not writing) sender_tid to response |
| `BlockOnIpcReceive` (42) | 0 | After saving context, before redirecting frame |
| `WaitForDevice` (43) | 0 | After saving context, before redirecting frame |
| `default` (unknown kind) | 0 | After writing InvalidRequest status |

Recording is **not** performed on the `deny:` path (TVA validation failure), since
that path may not have a valid kind field.

---

## API

```cpp
// Record a dispatch outcome (called internally from el0_svc_kernel_call_dispatch).
static void fs_obs_record(uint32_t tid, uint32_t kind,
                           uint32_t status, uint32_t peer_tid) noexcept;

// Reset the ring and sequence counter.  Called from fs_sched_reset().
extern "C" void fs_obs_reset() noexcept;

// Number of records emitted since last reset (wraps at 2^64; monotonic).
extern "C" uint64_t fs_obs_count() noexcept;

// Return true iff a record exists with matching tid, kind, status=Ok, and
// peer_tid (if peer_tid != 0).  Searches the live set.
extern "C" bool fs_obs_find(uint32_t tid, uint32_t kind,
                              uint32_t peer_tid) noexcept;
```

---

## Reset Contract

`fs_obs_reset()` zeroes all 32 slots and sets `s_obs_seq = 0`.

It is called from `fs_sched_reset()` so that each cooperative scheduler session
starts with a clean ring.  Records from Phase 7 (GetThreadIdentity calls from
`el0_process_stub.S`) and Phase 8 (SendMessage/ReceiveMessage from IPC stubs)
accumulate across phases, but the Phase 9 ring is clean because `canon_sched_reset()`
calls `fs_sched_reset()` which calls `fs_obs_reset()`.

---

## Phase 10 CI Gate

After `canon_sched_load_and_run()` returns and the Phase 9 scheduler session is
complete, the loader calls `fs_obs_find()` twice:

```cpp
const bool saw_block = fs_obs_find(3u, kKindBlockOnIpcReceive, 0u);
const bool saw_send  = fs_obs_find(2u, kKindSendMessage, 3u);

if (saw_block && saw_send) {
    cel_pl011_puts("[axion] el0: obs OK (BlockOnIpcReceive tid=3, SendMessage tid=2->3)\r\n");
} else {
    cel_pl011_puts("[axion] el0: obs FAIL (expected records missing)\r\n");
}
```

This gate verifies:
1. Process B (tid=3) issued `BlockOnIpcReceive` and the dispatch succeeded.
2. Process A (tid=2) issued `SendMessage` targeting tid=3 and the dispatch succeeded.

The gate is **independent of the IPC delivery flag** — it verifies the call
graph, not just the end-to-end outcome.

---

## Execution Graph Semantics

The ring implicitly encodes two types of edges:

**Call edges** (`Thread → KernelCallKind`):
```
tid=3 → BlockOnIpcReceive
tid=2 → SendMessage
```

**IPC edges** (`ThreadA → ThreadB`, derived from peer_tid):
```
tid=2 --SendMessage--> tid=3
```

These two edge types form the execution graph that RFC-00C0 will use to verify
that a content-addressed binary produced the expected interaction pattern.

---

## Acceptance Criteria

- [x] `FsObsRecord` (24B, 6 × uint32) defined in `qemu_slice6_el0_svc_bridge.cpp`
- [x] 32-slot ring with monotonic `s_obs_seq`; oldest-first wrap on overflow
- [x] `fs_obs_record()` called at outcome point for all five handled kinds
- [x] `fs_obs_reset()` called from `fs_sched_reset()`
- [x] `fs_obs_find()` exported and used in `canon_exec_loader.cpp`
- [x] Phase 10 CI gate: `[axion] el0: obs OK (BlockOnIpcReceive tid=3, SendMessage tid=2->3)`
- [ ] RFC-00C0 references this ring as the ground-truth for executable identity
  verification

---

## Relationship to Other RFCs

- **RFC-00BD** — Frozen ordinals make ring `kind` fields semantically stable;
  `BlockOnIpcReceive=42` and `SendMessage=13` are the primary values recorded
  in Phase 10.
- **RFC-00BE** — The cooperative scheduler produces the Phase 9 call sequence
  verified by the Phase 10 gate.  The ring proves the scheduler executed
  correctly, not just that a flag was set.
- **RFC-00C0** — CanonFS executable identity will compare the ring against a
  per-binary expected call sequence manifest.  This RFC defines the ring API
  RFC-00C0 will consume.

---

## References

- [RFC-00BD: KernelCall ABI Ordinal Freeze](RFC-00BD-kernelcall-abi-ordinal-freeze.md)
- [RFC-00BE: Freestanding Cooperative Scheduler](RFC-00BE-freestanding-cooperative-scheduler.md)
- `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp` — ring implementation
- `ternaryos/hal/canon_exec_loader.cpp` — Phase 10 verification
