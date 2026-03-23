# RFC-00BC: TernaryOS EL0 Userland Bring-Up

**Status:** accepted
**Type:** standards-track
**Applies-To:** ternaryOS EL0 process model, freestanding kernel bridge, QEMU AArch64 boot lane
**Created:** 2026-03-22
**Author:** @t81dev
**Depends on:** RFC-00B0 (HAL), RFC-00B1 (Ternary MMU), RFC-00B3 (Axion Kernel Architecture), RFC-00B4 (Userland Service Contract), RFC-00B5 (Governed Event Interrupt Model), RFC-00B6 (Minimal Syscall and Capability Boundary), RFC-00B7 (Pager Service ABI)
**Blocks:** real EL0 process execution, multi-process IPC, CanonFS-hosted executables, RFC-00C0 (network stack)

---

## Summary

This RFC defines the phased implementation path from the EL0 SVC roundtrip proof
(Phase 4, commit `ffe867ff`) to a governed multi-process EL0 userland running on
the Axion freestanding kernel bridge in the QEMU AArch64 boot lane.

Each phase is self-contained, has a CI-verifiable pass condition, and depends
only on the phases before it.  The goal is a plan precise enough that each phase
can be implemented without re-examining architecture.

---

## Motivation

Phase 4 established that the EL0 → SVC → EL1 exception path works in real
hardware/emulator conditions (`BOOTAA64.EFI`, EDK2, QEMU `cortex-a57`).
However Phase 4 has three structural limitations that prevent real userland work:

1. **No address space isolation.**  `axion_el0_entry` runs in identity-mapped EFI
   memory.  EL0 code and EL1 code share the same physical-equals-virtual
   address space; a buggy EL0 process can read or corrupt EL1 state.

2. **No kernel call bridge.**  The freestanding SVC dispatcher
   (`axion_kernel_handle_svc_trap_aarch64`) handles three primitive calls
   (GetThreadIdentity, WriteSerial, ExitToEL1).  It cannot reach the governed
   `axion_kernel_call_wire_tva()` path, so capability enforcement, IPC, and
   process management are unavailable to freestanding EL0 code.

3. **No process loading.**  There is no mechanism to load an EL0 binary from
   CanonFS, register it as an executable object, and spawn it as a thread.
   The only EL0 code that runs is the hard-wired `axion_el0_entry` stub.

This RFC closes those gaps in five ordered phases.

---

## Phase Definitions

### Phase 5 — EL0 Address Space Isolation

**Goal:** EL0 processes run in their own page-table-isolated virtual address
space, separate from the EL1 kernel address space.

**What to build:**

- Before the first ERET to EL0, clone EDK2's TTBR0_EL1 root table into a
  kernel-owned copy and add four EL0-specific page mappings:
  - Init code page (4 KB, R+X for EL0, R+X for EL1 — shared `.text` page
    containing `axion_el0_entry` and EL1 SVC dispatch functions)
  - Init stack page (4 KB, R/W, no-exec — replaces BSS `s_el0_stack`)
  - Proc code page (4 KB, R/W/X — writable by EL1 for loader; executable by EL0)
  - Proc stack page (4 KB, R/W, no-exec — for Phase 7 loaded process)
- TTBR1_EL1 is **not** modified.  The EDK2 identity-map remains in TTBR0; we
  clone the root, copy the relevant L1/L2 entries, and override only the four
  leaf L3 entries.  Changing TTBR1 while EL1 IP lives in the TTBR0 range would
  require a careful trampoline that is not needed here.
- The SVC dispatcher gains TVA validation: if a pointer passed by EL0 falls
  outside the four mapped pages, the kernel returns an error rather than
  dereferencing at EL1.

**Why this comes first:** All subsequent phases depend on address space isolation
being real.  TVA validation in the Phase 6 `KernelCall` dispatcher is meaningless
without actual page table separation.  RFC-00B1 is already accepted and its
ternary radix page table implementation exists in the hosted kernel; Phase 5
brings hardware page isolation live in the freestanding bridge.

**CI gate:**
```
[axion] el0: page-isolated (TTBR0 active, EL0 stack mapped)
```
Added to `qemu-boot.yml` `Validate boot sequence` after the existing Phase 4
gate (`[axion] el0: init OK (tid=1)`).

**Key files:**

- New: `ternaryos/hal/qemu_slice6_el0_mmu.cpp` — `el0_mmu_init()`:
  clones EDK2 TTBR0, detects T0SZ/walk-depth, builds 4-page EL0 table;
  `el0_tva_valid()`: range check used by the SVC dispatcher
- New: `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp` — freestanding
  `el0_svc_kernel_call_dispatch()`: handles TVA-validated KernelCall wire requests
- Modified: `qemu_slice6_cpp_bridge.cpp` — call `el0_mmu_init()` before
  `run_el0_init()`; add proc page helpers `el0_mmu_proc_code_page()` /
  `el0_mmu_proc_stack_top()`
- Modified: `qemu_slice6_bridge_irq.cpp` — add TVA validation in SVC #3; default
  case prints ESR+ELR and redirects to EL1 (prevents infinite fault loops)

---

### Phase 6 — Freestanding Kernel Call Bridge

**Goal:** EL0 processes can invoke the full RFC-00B6 kernel call ABI through a
single `KernelCall` SVC, replacing the primitive freestanding ABI with the
governed `axion_kernel_call_wire_tva()` path.

**What to build:**

- Extend the freestanding SVC ABI to four calls:

  | SVC # | Name | Signature | Notes |
  | ---: | :--- | :--- | :--- |
  | 0 | `GetThreadIdentity` | `→ x0 = tid` | Unchanged from Phase 4 |
  | 1 | `KernelCall` | `x0=req_tva, x1=req_size, x2=rsp_tva, x3=rsp_size` | Routes to `axion_kernel_call_wire_tva()`; both TVAs validated against EL0 address space |
  | 2 | `ExitThread` | `x0=exit_code` | Replaces ExitToEL1; proper thread teardown via `ExitThread` kernel call |
  | 3 | `WriteSerial` | `x0=str_tva` | Demoted to debug-only; only valid when `AXION_DEBUG_SERIAL=1` policy is set |

- Wire SVC #1 (`KernelCall`) to a freestanding dispatcher
  (`el0_svc_kernel_call_dispatch()` in `qemu_slice6_el0_svc_bridge.cpp`) that
  validates both TVAs, parses the `KernelCallWireRequestBlock` magic/version/kind,
  and handles the request in-bridge.  The freestanding bridge does not link the
  hosted `axion_kernel_call_wire_tva()` — that symbol belongs to the hosted kernel
  which is not present in the EFI binary.  The wire-format ABI is identical, so
  the same EL0 assembly works unmodified against both the freestanding and hosted
  dispatch paths.

- Update `axion_el0_init.S` to use SVC #1 (`KernelCall`) with a
  `GetThreadIdentity` wire request rather than SVC #0, exercising the full
  bridge path, then terminate via SVC #2 (`ExitThread`).

**Why SVC #1 = KernelCall rather than a wider ABI:** A single `KernelCall` SVC
is the same design decision made in RFC-00B6 §5.1 — one narrow entry point for
all kernel operations.  Adding per-operation SVCs would replicate the proliferation
that RFC-00B6 was designed to avoid.

**Why ExitThread replaces ExitToEL1:** `ExitToEL1` was a Phase 4 hack — it
patched the trap frame directly to return to a hard-coded EL1 label.  In Phase 6
threads are real kernel objects; proper teardown requires the kernel to reclaim
the thread's resources, not a raw frame patch.

**CI gate:**
```
[axion] el0: kernel call bridge OK (KernelCall SVC wired)
```

**Key files:**

- Modified: `qemu_slice6_bridge_irq.cpp` — replace SVC #1/2 with KernelCall /
  ExitThread; route SVC #1 to `el0_svc_kernel_call_dispatch()`
- New: `ternaryos/hal/axion_el0_init.S` — use SVC #1 KernelCall
  for GetThreadIdentity, SVC #2 ExitThread to terminate
- Modified: `qemu-boot.yml` — add Phase 6 CI gate string

---

### Phase 7 — CanonFS Process Loading

**Goal:** A minimal EL0 binary stored in the CanonFS virtio-blk image is loaded
at boot, registered as an executable object, and spawned as a real EL0 thread.

**What to build:**

- Define a minimal flat binary format for EL0 executables stored in CanonFS.
  The format for Phase 7 is: a fixed 64-byte header
  (`magic="T81X"`, `entry_offset`, `code_size`, `data_size`) followed by
  code and data sections.  This is not a general ELF loader; it is the minimum
  needed to prove the load path.

- Write a loader (`canon_exec_loader.cpp`) in the freestanding bridge that:
  1. Reads LBA 3 from the CanonFS virtio-blk device (reusing the queue
     initialised by `canonfs_io_probe()`).
  2. Validates the T81X header (magic, version, `code_size` bounds,
     `entry_offset < code_size`).
  3. Copies the code section into the pre-allocated EL0 proc code page
     (`s_el0_proc_code_page`), flushes the I-cache, and ERets to EL0 with
     `el0_mmu_proc_stack_top()` as SP_EL0.
  4. The loaded process calls `GetThreadIdentity` (SVC #1 `KernelCall`),
     emits its banner via `WriteSerial` (SVC #3), and exits via `ExitThread`
     (SVC #2).  `canon_exec_load_and_run()` returns after the ERET/ExitThread
     round-trip and restores tid=1.

  `RegisterExecutableObjectFromTva` and `SpawnThreadFromExecutableObject` are
  **not called** in Phase 7 — the freestanding bridge manages the thread
  lifecycle directly.  These kernel call kinds are reserved for Phase 8+.

- The CanonFS image in `qemu-boot.yml` embeds `el0_process_stub.S` (assembled
  with `--target=aarch64-unknown-elf`, raw `.text` extracted with
  `llvm-objcopy -O binary`) as a T81X sector at LBA 3 at CI time.

**Why CanonFS before IPC:** You need at least one well-formed, dynamically loaded
EL0 process before IPC can be tested meaningfully.  Hard-wiring test binaries in
the EFI image (as Phase 4 does) does not prove the load path.

**CI gate:**
```
[axion] el0: process loaded from CanonFS (tid=2)
```

**Key files:**

- New: `ternaryos/hal/canon_exec_loader.cpp` — `canon_exec_load_and_run()`:
  reads LBA 3, validates T81X header, copies code to proc page, flushes I-cache,
  ERets to EL0, returns after ExitThread
- New: `ternaryos/hal/el0_process_stub.S` — position-independent EL0
  process stub; calls GetThreadIdentity, WriteSerial, ExitThread
- Modified: `qemu-boot.yml` — assemble `el0_process_stub.S` with clang-18, embed
  T81X sector at LBA 3 via Python; add Phase 7 CI gate; add
  `-global virtio-mmio.force-legacy=false` (QEMU 10.x defaults to legacy ver=1
  transport which is incompatible with the modern split-queue protocol used by
  `canonfs_io_probe()`)

---

### Phase 8 — EL0 IPC

**Goal:** Two concurrently-running EL0 processes exchange a message through the
`SendMessage` / `ReceiveMessage` kernel call path.

**What to build:**

- Spawn two EL0 threads from two distinct CanonFS executables (the loader from
  Phase 7 extended to load multiple objects).
- Process A calls `SendMessage` (SVC #1 `KernelCall`) targeting process B's
  thread identity.
- Process B calls `ReceiveMessage`, reads the message, and calls `ExitThread`.
- Process A polls `QueryThreadExecutionState` until B has exited, then exits.
- The kernel bridge reports the IPC round-trip in the serial log.

**Why IPC is Phase 8 and not earlier:** IPC requires:
  (a) multiple address-space-isolated processes (Phase 5),
  (b) the full kernel call bridge (Phase 6), and
  (c) a process loader to produce the second process (Phase 7).
  All three must land before IPC can be tested end-to-end.

**CI gate:**
```
[axion] el0: IPC roundtrip OK (A->B, tid=2,3)
```

**Key files:**
- New: `ternaryos/hal/el0_ipc_test_a.S` — sender stub
- New: `ternaryos/hal/el0_ipc_test_b.S` — receiver stub
- Modified: `canon_exec_loader.cpp` — load and spawn two objects
- Modified: `qemu-boot.yml` — embed both stubs; add Phase 8 CI gate

---

### Phase 9 — Network Stack (RFC-00C0)

**Goal:** Define and implement a minimal TCP/IP stack accessible to EL0 processes
through the kernel call boundary.  This phase is the subject of RFC-00C0, which
is to be authored once Phase 6 is complete (the kernel call bridge is the
prerequisite for any socket-like abstraction).

**What to build:** Deferred to RFC-00C0.  This RFC records Phase 9 as a planned
dependency so RFC-00C0 authors know the required preceding state.

**Dependencies:** Phase 6 (KernelCall SVC bridge — sockets are kernel objects
reached through `KernelCall`, not a separate SVC family).

**CI gate:** Defined by RFC-00C0.

---

## SVC ABI Version Table

| Phase | SVC # | Name | Notes |
| ---: | ---: | :--- | :--- |
| 4 | 0 | `GetThreadIdentity` | Returns `x0 = 1` (hard-coded) |
| 4 | 1 | `WriteSerial` | Direct PL011 write from kernel dispatcher |
| 4 | 2 | `ExitToEL1` | Patches trap frame ELR/SPSR; EL1 hack |
| 6 | 0 | `GetThreadIdentity` | Unchanged |
| 6 | 1 | `KernelCall` | `x0=req_tva, x1=req_sz, x2=rsp_tva, x3=rsp_sz` → `el0_svc_kernel_call_dispatch()` (freestanding bridge); wire format identical to `axion_kernel_call_wire_tva()` |
| 6 | 2 | `ExitThread` | Proper thread teardown via kernel |
| 6 | 3 | `WriteSerial` | Debug only; policy-gated |

SVC #s 4–15 are reserved for future kernel call families (fault observation,
capability delegation, memory management).  SVC #s 16+ are reserved for
userland-to-userland fast paths (not defined in this RFC).

---

## CI Gate Sequence (full boot, Phase 9 complete)

```
# Phase 1 — EFI ConOut
Axion QEMU AArch64 EDK2 slice6

# Phase 2 — bare-metal C / EL1
[axion] bare-metal EL1 kernel entry
[axion] ExitBootServices complete

# Phase 3 — C++ bridge
T81  --  Ternary OS for AI
[axion] policy engine: ready
[axion] canonfs: mounted (persistent, virtio-blk)
[axion] canonfs: I/O probe OK
[axion] kernel thread tid=1: running
[axion] event loop: priority dispatch
[axion] hw timer: GICv3 PPI30 armed (10ms)

# Phase 5 — EL0 address space (el0_mmu_init completes before first ERET)
[axion] el0: page-isolated (TTBR0 active, EL0 stack mapped)

# Phase 6 — kernel call bridge (ERET → axion_el0_entry → SVC sequence → EL1)
[axion] el0: init OK (tid=1)
[axion] el0: kernel call bridge OK (KernelCall SVC wired)

# Phase 7 — CanonFS process loading
[axion] el0: process loaded from CanonFS (tid=2)

# Phase 8 — IPC
[axion] el0: IPC roundtrip OK (A->B, tid=2,3)

# Phase 3 shell (always last)
[axion] t81sh: ready (principal=axion, tier=1)
```

Note: Phase 4's `[axion] el0: init OK (tid=1)` banner is emitted by the Phase 6
`axion_el0_init.S` stub (via WriteSerial SVC #3) and therefore appears in the
Phase 6 block, after the Phase 5 isolation banner.

---

## Design Decisions

### D1 — One KernelCall SVC instead of per-operation SVCs

RFC-00B6 §5.1 established a message-oriented kernel boundary with a single
narrow entry point.  Phase 6 carries that decision into the freestanding bridge
unchanged.  Adding per-operation SVCs (e.g., SVC #4 = SendMessage, SVC #5 =
ReceiveMessage) would bypass capability enforcement and TVA validation for each
call family.

### D2 — T81X flat binary format instead of ELF

ELF parsing requires significant code (section headers, relocations, dynamic
linking).  The Phase 7 T81X format is 64 bytes of fixed-layout header + raw
sections — the minimum to prove the load path.  ELF support is not precluded;
it is deferred until Phase 7 is proven.

### D3 — MMU activation before kernel call bridge

`axion_kernel_call_wire_tva()` validates both request and response TVA spans
before touching their contents.  Without real page tables, TVA validation is
vacuous — any EL0 pointer passes because everything is identity-mapped.  The
security property only holds once TTBR0 is live with real permissions.

### D4 — Phases 5–8 all gate on QEMU CI

Every phase has a string gate in `qemu-boot.yml`.  A phase is not considered
implemented until its CI gate passes in the QEMU boot capture workflow.  This
prevents paper implementations that compile but do not actually execute the path
on hardware/emulator.

---

## Dependency Graph

```
Phase 4 (EL0 SVC probe)           ← current, complete
    └── Phase 5 (MMU / address space)
            └── Phase 6 (KernelCall SVC bridge)
                    ├── Phase 7 (CanonFS process loading)
                    │       └── Phase 8 (IPC)
                    └── Phase 9 (RFC-00C0 network stack)
```

---

## Acceptance Criteria

- [x] **Phase 5:** `[axion] el0: page-isolated (TTBR0 active, EL0 stack mapped)`
  passes in QEMU boot CI; EL0 WriteSerial SVC with a kernel-range TVA returns an
  error rather than dereferencing
- [x] **Phase 6:** `[axion] el0: kernel call bridge OK (KernelCall SVC wired)`
  passes; `axion_el0_init.S` uses SVC #1 `KernelCall` for `GetThreadIdentity`
  and SVC #2 `ExitThread` to terminate; Phase 4 `WriteSerial` demoted to
  debug-only (SVC #3)
- [x] **Phase 7:** `[axion] el0: process loaded from CanonFS (tid=2)` passes;
  T81X binary assembled from `el0_process_stub.S` and embedded in
  `canon_store.img` at CI time; loader does not hard-code the tid
- [x] **Phase 8:** `[axion] el0: IPC roundtrip OK (A->B, tid=2,3)` passes;
  both EL0 processes exit cleanly; no kernel panics or spurious IRQs
- [ ] **Phase 9:** RFC-00C0 authored and accepted; its CI gate passes

---

## Relationship to the RFC-DPE Series

The RFC-DPE series (RFC-DPE-0001 through RFC-DPE-0009, all accepted) defines
deterministic parallel execution of TISC tasks — epoch task graphs, DAG-ordered
dispatch, bounded thread pools, audit events, and the epoch history ring.  It
operates at the TISC VM layer, not the OS EL0 process layer, and is not a
prerequisite of Phases 5–8.  However, three touch points require explicit
acknowledgement:

### DPE touch point 1 — SubmitEpoch capability requirement (Phase 6)

`SubmitEpoch` is a `KernelCallKind` reachable through the Phase 6 `KernelCall`
SVC.  Per RFC-DPE-0002 §4, the calling thread must hold
`KernelCapabilityKind::PagerService`; without it the kernel returns
`KernelCallStatus::CapabilityDenied`.

**Implication for Phase 6:** The default capability set granted to a freshly
spawned EL0 process does not include `PagerService`.  An EL0 process that
attempts `SubmitEpoch` without an explicit capability grant will receive a
deterministic denial.  Phase 6 implementers must not assume `SubmitEpoch` is
freely available from EL0; it requires a capability delegation step through the
kernel (RFC-00B6 §5.5).

### DPE touch point 2 — Audit events fire through the freestanding bridge (Phase 6)

Per RFC-DPE-0008, three `KernelAuditEventKind` variants (`EpochSubmitted`,
`EpochCommitted`, `EpochAborted`) are emitted inside `axion_kernel_submit_epoch()`
via `record_audit_event()`.  That function executes in the sequential,
single-threaded kernel call context.

**Implication for Phase 6:** The freestanding `KernelCall` SVC dispatcher must
wire through to `axion_kernel_call_wire_tva()` without bypassing the audit path.
A Phase 6 implementation that short-circuits `record_audit_event()` for
performance would silently break the DPE audit guarantee.  The correct
implementation is a straight-through call to `axion_kernel_call_wire_tva()` with
no custom pre/post hooks — the audit wiring is already inside the kernel.

### DPE touch point 3 — Epoch history ring visible to EL0 status queries (Phase 7/8)

Per RFC-DPE-0009, committed epoch records are stored in a fixed-capacity ring
buffer within `KernelRuntimeState::EpochRuntimeState` and exposed read-only
through `KernelRuntimeStatusView`.

**Implication for Phase 7/8:** EL0 processes that call the kernel's
`KernelRuntimeStatusView` query path through `KernelCall` SVC will see the epoch
history ring populated by any concurrent epoch work.  This is correct behaviour
and requires no special handling; it is noted here so Phase 7/8 implementers do
not mistake epoch history entries in the status view for unexpected state.

---

## Relationship to Other Accepted RFCs

### RFC-0045 — Deterministic Memory Model (Phase 5)

RFC-0045 defines canonical memory visibility, segment semantics, handle
identity, and aliasing constraints for all T81 execution contexts.

**Implication for Phase 5:** The modified TTBR0 installed in Phase 5 introduces
four new EL0 memory segments (init code, init stack, proc code, proc stack).  Each segment must
satisfy RFC-0045's identity and aliasing rules: EL0 code pages are read-execute,
EL0 stack pages are read-write, and neither may alias EL1 kernel segments.  Any
EL0 pointer passed to a kernel SVC is a TVA subject to RFC-0045 segment
validation before the kernel touches it.  Phase 5 implementers must not treat
the TTBR split as purely a hardware concern — it is the physical realisation of
the RFC-0045 segment boundary between kernel and user address spaces.

### RFC-0046 — Deterministic Scheduling and Execution Ordering (Phase 8)

RFC-0046 defines the constitutional scheduling rule: program order within a
thread is preserved; dependency order between threads is enforced through
explicit synchronisation; canonical commit order is determined by the kernel
scheduler, not wall-clock time.

**Implication for Phase 8:** When EL0 process A sends a message to process B,
the message transfer is a cross-thread dependency edge.  RFC-0046 requires that
B's `ReceiveMessage` observes A's `SendMessage` payload in full — no partial
reads, no torn writes.  The kernel IPC path in `axion_kernel_call_wire_tva()`
already enforces this for the hosted kernel; Phase 8 inherits that guarantee
through the `KernelCall` SVC bridge.  Phase 8 implementers must not add
any out-of-band shared memory between EL0 processes as a "fast path" — doing so
would create a synchronisation surface outside RFC-0046's ordering model.

### RFC-0048 — Deterministic Surface Definition and Governance Boundaries (Phase 6)

RFC-0048 defines the classification of deterministic surfaces: DCP
(Deterministic Core Protocol), governed non-DCP, experimental, and out-of-scope.
Every new execution path that can affect observable T81 state must be classified
before it is shipped.

**Implication for Phase 6:** The `KernelCall` SVC ABI
(`x0=req_tva, x1=req_sz, x2=rsp_tva, x3=rsp_sz → axion_kernel_call_wire_tva()`)
is a new entry point into the DCP.  It must be classified as **governed non-DCP**
(a transport layer that routes to an already-DCP path) rather than as a new DCP
surface in its own right.  The classification must be recorded in the Phase 6
implementation commit so RFC-0048's governance boundary remains unambiguous.
Phase 6 implementers must not add any side effects inside the SVC dispatcher
itself (between SVC entry and the `axion_kernel_call_wire_tva()` call) that
could alter observable T81 state — all such effects belong inside the kernel.

### RFC-0054 — CanonFS Object Identity and Persistence Contract (Phase 7)

RFC-0054 defines how CanonFS objects are identified, what persistence guarantees
the storage driver provides, and what happens to object identity across restarts.

**Implication for Phase 7:** The T81X binary loaded from CanonFS virtio-blk at
LBA 3 is a CanonFS object.  RFC-0054 guarantees that reading LBA 3 twice in the
same boot produces byte-identical content, and that the object's canonical
identity (its content hash) is stable across reboots as long as no write
intervenes.  Phase 7's loader must treat the binary as immutable once read —
any modification to the in-memory copy does not affect the CanonFS object, and
re-reading will reproduce the original bytes.  The loader must not assume the
binary is already page-aligned on disk; RFC-0054 makes no alignment guarantee
for raw block objects.

### RFC-00B2 — Device Driver Architecture (Phases 7 and 9)

RFC-00B2 defines the Phase 4 storage, display, and network device boundaries and
the guest-device ownership model.

**Implication for Phase 7:** The virtio-blk driver used to read the T81X binary
from CanonFS operates under the RFC-00B2 storage boundary.  Phase 7's loader
must claim the virtio-blk device through `ClaimDevice` (RFC-00B6 §5.9) before
reading, and release it when done.  The existing `canonfs_io_probe()` in
`qemu_slice6_cpp_bridge.cpp` already holds the device; Phase 7 must coordinate
with or extend that probe rather than opening a second concurrent claim.

**Implication for Phase 9:** The network stack (RFC-00C0) will use the RFC-00B2
network device boundary.  RFC-00C0 authors should treat RFC-00B2 §4 (guest
network device model) as the normative device contract, not the TCP/IP protocol
itself.

### RFC-00B9 — TernaryOS User Environment Standard (Phases 7 and 8)

RFC-00B9 defines `t81-init`, the session model, `t81sh`, the service registry,
and the TTY contract.  It introduces four Axion capability gates:
`BootService`, `SessionCreate`, `ServiceSpawn`, and `ShellExec`.

**Implication for Phase 7:** Spawning an EL0 process from a CanonFS executable
is the first real `ServiceSpawn` event.  Per RFC-00B9, the spawning caller must
hold the `ServiceSpawn` Axion gate; attempting to spawn without it produces a
`CapabilityDenied` rejection.  The kernel bridge in Phase 7 runs at EL1 with
the `axion` principal (tier=1), which does hold `ServiceSpawn` by default —
but Phase 7 implementers should verify this rather than assume it.

**Implication for Phase 8:** If Phase 8 IPC processes register themselves with
the RFC-00B9 service registry (`RegisterService` kernel call), they become
addressable by name.  This is optional for the Phase 8 CI gate but is the
correct architecture for any IPC that persists beyond a single boot.

---

### Advisory mentions

The following RFCs have lower direct impact on Phases 5–9 but should be
consulted if Phase 6+ work touches their surfaces:

- **RFC-0052 (Canonical Dataflow and State-Driven Execution):** Governs
  state-identity rules for data exchanged between processes.  Relevant if
  Phase 8 IPC payloads contain CanonFS object references rather than raw bytes.

- **RFC-00B8 (Governed Foreign Function Interface):** If any Phase 6+ EL0
  binary calls a native host function through the kernel FFI bridge, the
  RFC-00B8 policy gate (`ffi_policy_deny` / `required_capabilities`) applies.
  The freestanding bridge does not currently expose the FFI path to EL0; any
  change to that would require an explicit RFC-00B8 scope extension.

---

## Implementation Notes (Phases 5–8)

The following non-obvious facts were discovered during implementation and are
recorded here so future phases do not re-derive them.

### N1 — QEMU virt uses T0SZ=20 (4-level page walk), not T0SZ=25

EDK2 AAVMF 2024 on QEMU virt sets `TCR_EL1 = 0x0000000480803514`.
`T0SZ` (bits[5:0]) = `0x14` = 20, giving a 44-bit VA space and a **4-level**
hardware walk (L0 → L1 → L2 → L3).

The Phase 5 design assumed T0SZ=25 (3-level, L1 as root).  With T0SZ=20, the
root table has 32 used L0 entries; all QEMU RAM addresses (< 2^39) have
L0 index = 0, routing through `s_l1[0]` → EDK2's original L1.  A 3-level
implementation that modifies `s_l1[l1_idx=1]` (the 1–2 GB range) silently
operates on L0 index 1 (the 512 GB–1 TB region), which is never used — the
modifications are visible in memory but never walked by hardware.

**Fix**: `el0_mmu_init()` reads `TCR_EL1.T0SZ` at runtime.  For T0SZ < 25 it
clones EDK2's L1 into a kernel-owned `s_l1b`, then redirects `s_l1[0]` to
`s_l1b` so all subsequent L1/L2/L3 modifications are in owned memory.

### N2 — Proc code page must be AP_RW for the EL1 loader memcpy

`el0_mmu_init()` maps four L3 pages before installing the new TTBR0.  If the
proc code page is mapped with `AP[2:1]=11` (EL1 read-only) then
`canon_exec_load_and_run()`'s `memcpy` into that page faults at EL1 because EL1
data writes to VA 0x5cXXX000 go through TTBR0 (the address is below the TTBR1
split).  The result is a silent hang with no output — no EL1 synchronous fault
handler is installed to report it.

**Fix**: `kLeafProcCode` uses `AP_RW` (`AP[2:1]=01`, EL0+EL1 R/W, `UXN=0` so
EL0 can execute).  EL1 writes the code, flushes the I-cache, then ERets to EL0.

### N3 — QEMU 10.x virtio-mmio defaults to legacy transport (version 1)

`virtio-mmio` on QEMU 10.x has a `force-legacy=on` default.  The Version
register returns `1` (legacy) instead of `2` (modern).  The `canonfs_io_probe()`
function uses the modern split-queue registers (`0x080`/`0x090`/`0x0A0` for
desc/avail/used), which do not exist in the legacy transport — the probe silently
fails and `s_has_blk` stays `false`, skipping both the I/O probe and Phase 7.

**Fix**: Pass `-global virtio-mmio.force-legacy=false` (or `=off`) in the QEMU
command.  This is already in `.github/workflows/qemu-boot.yml`.  Local runs and
any future test scripts must include this flag.

### N4 — Virtio-mmio slot allocation is top-down

QEMU virt allocates `virtio-mmio` slots from the top of the 32-slot range
(0x0A000000 + N×0x200).  The first device added takes slot 31 (0x0A003E00);
the second takes slot 30 (0x0A003C00).  Using `-drive if=virtio` creates
**PCI** virtio devices, not MMIO — they do not appear at the MMIO base
addresses at all.  Phase 3/7 code probes `kVirtioMmioBase = 0x0A003C00`.

**Fix**: Use `-drive id=X,if=none -device virtio-blk-device,drive=X` to create
MMIO devices.  The boot disk is added first (slot 31); the CanonFS store is
added second (slot 30 = `kVirtioMmioBase`).

### N5 — Phase 8 IPC is sequential; mailbox is single-slot

The freestanding kernel has no scheduler, so "concurrency" between Process A and
Process B is simulated by running them back-to-back in EL1.  `canon_ipc_load_and_run()`
ERets to A (tid=2), A calls `SendMessage` (SVC #1, kind=13) which stores
`sender_tid` in a static `IpcMailbox` and returns, then A exits.  EL1 then
ERets to B (tid=3); B calls `ReceiveMessage` (SVC #1, kind=14), the bridge
delivers `sender_tid` at response[36:40] (`spawned_tid` field) and sets
`delivered=true`.  EL1 polls `canon_ipc_delivered()` and emits the CI gate.

This matches the `KernelCallKind` enum ordinals confirmed from `kernel_abi.hpp`:
`SendMessage=13`, `ReceiveMessage=14` (counting from `Yield=0`).

**Wire format extension**: `SendMessage` carries `ipc_dst` (target TID) at
request byte offset 12 (`bytes` field = 16).  No new SVC numbers are
introduced; both calls go through SVC #1 (KernelCall) with different `kind`
fields, consistent with the Phase 6 wire protocol design.

---

## References

- [RFC-00B0: HAL Specification](RFC-00B0-hal-spec.md)
- [RFC-00B1: Ternary MMU](RFC-00B1-ternary-mmu.md)
- [RFC-00B2: Device Driver Architecture](RFC-00B2-device-drivers.md)
- [RFC-00B3: Axion Governance Kernel Architecture](RFC-00B3-axion-kernel-architecture.md)
- [RFC-00B4: Axion Userland Service Contract](RFC-00B4-userland-service-contract.md)
- [RFC-00B5: Governed Event Interrupt Model](RFC-00B5-governed-event-interrupt-model.md)
- [RFC-00B6: Minimal Syscall and Capability Boundary](RFC-00B6-minimal-syscall-capability-boundary.md)
- [RFC-00B7: Pager Service ABI](RFC-00B7-pager-service-abi.md)
- [RFC-00B8: Governed Foreign Function Interface](RFC-00B8-governed-foreign-function-interface.md)
- [RFC-00B9: TernaryOS User Environment Standard](RFC-00B9-ternary-os-user-environment.md)
- [RFC-0045: Deterministic Memory Model](RFC-0045-deterministic-memory-model.md)
- [RFC-0046: Deterministic Scheduling and Execution Ordering](RFC-0046-deterministic-scheduling-and-execution-ordering.md)
- [RFC-0048: Deterministic Surface Definition and Governance Boundaries](RFC-0048-deterministic-surface-definition-and-governance-boundaries.md)
- [RFC-0052: Canonical Dataflow and State-Driven Execution](RFC-0052-canonical-dataflow-and-state-driven-execution.md)
- [RFC-0054: CanonFS Object Identity and Persistence Contract](RFC-0054-canonfs-object-identity-and-persistence-contract.md)
- [RFC-DPE-0001: Deterministic Parallel Execution Vision](RFC-DPE-0001-deterministic-parallel-execution-vision.md)
- [RFC-DPE-0002: TISC Task Graph Primitives](RFC-DPE-0002-tisc-task-graph-primitives.md)
- [RFC-DPE-0007: Epoch Execution Timeout](RFC-DPE-0007-epoch-execution-timeout.md)
- [RFC-DPE-0008: Epoch Audit Events](RFC-DPE-0008-epoch-audit-events.md)
- [RFC-DPE-0009: Epoch History Ring](RFC-DPE-0009-epoch-history-ring.md)
