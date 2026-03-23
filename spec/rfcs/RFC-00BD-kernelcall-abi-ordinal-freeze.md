# RFC-00BD: KernelCall ABI Ordinal Freeze

**Status:** accepted
**Type:** standards-track
**Applies-To:** KernelCall SVC ABI, freestanding EL0 bridge, hosted kernel call dispatch
**Created:** 2026-03-23
**Author:** @t81dev
**Depends on:** RFC-00B6 (Minimal Syscall and Capability Boundary), RFC-00BC (TernaryOS EL0 Userland Bring-Up)
**Blocks:** RFC-00BE (Freestanding Cooperative Scheduler), RFC-00BF (Freestanding KernelCall Observability), any future KernelCall kind addition

---

## Summary

This RFC freezes the `KernelCallKind` ordinal table, declares the wire-format
field layout stable at version 1, and establishes the append-only extension
rule, the wire-format version-bump policy, and the CI enforcement mechanism
that prevents silent drift between the freestanding EL0 bridge and the hosted
kernel.

The primary motivation is **semantic ground stabilization**: the freestanding
EL0 assembly stubs, CanonFS-loaded executables, observability records, and
future executable identity bindings all reference `KernelCallKind` values by
ordinal.  If those ordinals can drift, every downstream artefact becomes a
latent landmine with no compile-time protection.

---

## Motivation

RFC-00B6 defined the `KernelCall` SVC ABI and the `KernelCallKind` C++ enum.
RFC-00BC implemented the freestanding dispatch path and proved the full
Phase 5–8 boot sequence on QEMU.  The Phase 8 EL0 assembly stubs encode kind
values as literal integers:

```asm
mov  w8, #13    // SendMessage
mov  w8, #14    // ReceiveMessage
```

These constants are correct today because the `KernelCallKind` enum is
sequential from `Yield=0`.  They will silently break if any future PR inserts
a new entry before `SendMessage` — the C++ enum shifts, the assembly does not,
and the only symptom is a wrong SVC dispatch with no diagnostic.

The same problem propagates to:

- **Observability records** that store `kind` as a uint32 — replaying them
  against a kernel with a shifted table produces wrong semantics.
- **CanonFS executable identity** (RFC-00C0/C1) — a content-addressed binary
  embedding kind ordinals becomes invalid if the table changes under it.
- **RFC-00BF audit events** — the `dispatch_seq` / `kind` pair must be
  replayable against the same table version.
- **RFC-00BE scheduler** — `BlockOnIpcReceive` (kind=42) and `WaitForDevice`
  (kind=43) are wake-point primitives; the scheduler must agree with the
  dispatch table on their ordinals.

Freezing the table now costs nothing.  Discovering drift after RFC-00BE or
RFC-00BF are implemented costs a flag day.

---

## Scope

This RFC governs:

1. **`KernelCallKind` ordinals** — the uint32 encoding of each call kind on
   the wire.
2. **Wire format field layout** — the byte offsets of `magic`, `version`,
   `bytes`, `kind`, `status`, `rejection`, and the fixed response fields
   through `caller_tid` (offset 44).
3. **Wire format version constants** — `kKernelAbiWireVersion` and the magic
   bytes `kKernelAbiWireRequestMagic` / `kKernelAbiWireResponseMagic`.
4. **Assembly symbolic constant requirements** — the obligation on EL0
   assembly code to reference named constants rather than literal ordinals.

This RFC does **not** govern:

- The full `KernelCallWireRequestBlock` / `KernelCallWireResponseBlock`
  layout beyond the minimum fields listed above (those are addressed in
  RFC-00B6 §5).
- `KernelCallStatus` or `KernelCallRejection` ordinals (those are covered by
  RFC-00B6 §6 and are separately frozen; this RFC notes that constraint but
  does not re-derive it).
- The hosted `axion_kernel_call()` implementation internals.

---

## Frozen Ordinal Table

The following table is **frozen** as of the acceptance of this RFC
(commit `afbc41bf`, RFC-00BC Phase 5–8 complete).

New entries MUST be appended after the last unconditional entry.  No existing
entry may be renumbered, renamed, or removed.

| Ordinal | Name | Notes |
| ---: | :--- | :--- |
| 0 | `Yield` | |
| 1 | `SpawnThreadInCallerGroup` | |
| 2 | `SpawnThreadUnderSupervisor` | |
| 3 | `RegisterThreadEntryDescriptor` | |
| 4 | `RegisterExecutableObject` | |
| 5 | `PublishExecutableObjectFromTva` | |
| 6 | `RegisterExecutableObjectFromTva` | |
| 7 | `QueryExecutableObject` | |
| 8 | `SpawnThreadFromExecutableObject` | |
| 9 | `SpawnThreadFromEntryDescriptor` | |
| 10 | `GetThreadIdentity` | Phase 6 freestanding probe |
| 11 | `QueryThreadExecutionState` | |
| 12 | `ExitThread` | Phase 6 freestanding ExitThread |
| 13 | `SendMessage` | Phase 8 IPC sender |
| 14 | `ReceiveMessage` | Phase 8 IPC receiver |
| 15 | `ReadFaultInbox` | |
| 16 | `AcknowledgeThreadFault` | |
| 17 | `AcknowledgeSupervisorFaultGroup` | |
| 18 | `QueryProcessGroupMemory` | |
| 19 | `SetAddressSpaceBootCritical` | |
| 20 | `QueryRuntimeStatus` | |
| 21 | `QueryFaultSummary` | |
| 22 | `QuerySupervisorStatus` | |
| 23 | `QuerySupervisorRecoveryStatus` | |
| 24 | `QuerySupervisorServiceStatus` | |
| 25 | `QuerySupervisorServiceInventory` | |
| 26 | `QuerySupervisorCapabilityInventory` | |
| 27 | `QuerySupervisorDelegationSummary` | |
| 28 | `QueryCapabilityTransitionHistory` | |
| 29 | `QueryCapabilities` | |
| 30 | `QueryDelegatedCapabilities` | |
| 31 | `QueryCapabilityRecord` | |
| 32 | `GrantCapability` | |
| 33 | `RevokeCapability` | |
| 34 | `RevokeDelegatedCapabilities` | |
| 35 | `RegisterService` | |
| 36 | `SpawnThreadForService` | |
| 37 | `QueryServiceStatus` | |
| 38 | `SuspendService` | |
| 39 | `ResumeService` | |
| 40 | `MarkServiceUnhealthy` | |
| 41 | `MarkServiceHealthy` | |
| 42 | `BlockOnIpcReceive` | RFC-00B6 §5.3 blocking receive |
| 43 | `WaitForDevice` | RFC-00B5 §3.3 device park |
| 44 | `RequestPageMapping` | RFC-00B7 §3.2 pager supply |
| 45 | `WaitForPagerHandoff` | RFC-00B7 §3.3 pager park |
| 46 | `ResumePageFaultedThread` | RFC-00B7 §3.4 un-quarantine |
| 47 | `ClaimDevice` | RFC-00B6 §5.3.6 device arbitration |
| 48 | `ReleaseDevice` | RFC-00B6 §5.3.6 device arbitration |
| 49 | `QueryDevice` | RFC-00B6 §5.3.6 device arbitration |
| 50 | `SetInterruptPolicy` | RFC-00B5 §3.7 interrupt policy |
| 51 | `ClearInterruptQuarantine` | RFC-00B5 §3.7 interrupt policy |
| 52 | `QueryInterruptPolicy` | RFC-00B5 §3.7 interrupt policy |
| 53 | `SubmitEpoch` | conditional: `T81_ENABLE_DPE`; RFC-DPE-0002 §10 |

**Retirement rule:** An ordinal that corresponds to a removed feature is
**retired**, not reused.  Retired ordinals must remain in `kernel_abi.hpp`
as a comment: `// 54 = RetiredFeatureName (retired RFC-00BX, do not reuse)`.
The freestanding bridge returns `kStatusInvalidRequest` for retired ordinals.

**Conditional ordinals:** Ordinal 53 (`SubmitEpoch`) is compiled only when
`T81_ENABLE_DPE=ON`.  It is unconditionally reserved — a non-DPE build that
receives kind=53 on the wire returns `kStatusInvalidRequest`.

---

## Wire Format Constants (version 1)

The following constants are **frozen** at wire format version 1.  They are
defined in `kernel_abi_wire.hpp` and mirrored in
`userland/experimental/hal/kernelcall_abi.inc` for EL0 assembly use.

| Constant | Value | Meaning |
| :--- | :--- | :--- |
| `kKernelAbiWireRequestMagic` | `0x4B415152` | Request block identifier (`KAQR` LE) |
| `kKernelAbiWireResponseMagic` | `0x4B415250` | Response block identifier (`KARP` LE) |
| `kKernelAbiWireVersion` | `1` | Current wire format version |

**Minimum request block (12 bytes):**

| Offset | Size | Field |
| ---: | ---: | :--- |
| 0 | 4 | `magic` = `kKernelAbiWireRequestMagic` |
| 4 | 2 | `version` = `kKernelAbiWireVersion` |
| 6 | 2 | `bytes` ≥ 12 (actual caller buffer size) |
| 8 | 4 | `kind` (uint32, `KernelCallKind` ordinal) |

**Minimum response block (48 bytes), fixed fields through `caller_tid`:**

| Offset | Size | Field |
| ---: | ---: | :--- |
| 0 | 4 | `magic` = `kKernelAbiWireResponseMagic` |
| 4 | 2 | `version` |
| 6 | 2 | `bytes` |
| 8 | 4 | `status` (`KernelCallStatus`, uint32) |
| 12 | 4 | `rejection` (`KernelCallRejection`, uint32) |
| 16 | 8 | `flags` (`KernelCallWireFlags`, uint64) |
| 24 | 11 | bool fields (action_performed … service_blocked) |
| 35 | 1 | alignment pad |
| 36 | 4 | `spawned_tid` (uint32) |
| 40 | 4 | `queried_tid` (uint32) |
| 44 | 4 | `caller_tid` (uint32) |

These offsets are **frozen**.  Fields beyond offset 47 may be added in future
wire-format versions without a version bump, provided the `bytes` field
correctly advertises their presence.

---

## Stability Rules

### R1 — Append-only ordinal extension

New `KernelCallKind` entries MUST be appended after the last entry in
`kernel_abi.hpp`.  Inserting before or between existing entries is
**forbidden**.

### R2 — No reuse of retired ordinals

An ordinal that has been allocated — even if the corresponding feature is
later removed — is **permanently reserved**.  It must not be reused for a
different call.

### R3 — No silent renaming

Renaming a `KernelCallKind` entry (changing the C++ identifier while keeping
the ordinal) is permitted for internal clarity but must be accompanied by:
- A comment in `kernel_abi.hpp` noting the old name and the RFC that
  introduced the rename.
- An update to `kernelcall_abi_manifest.txt` adding the old name as an alias.
- An update to `kernelcall_abi.inc` adding a deprecated alias `.equ`.

### R4 — No assembly literal ordinals

EL0 assembly files MUST NOT embed literal `KernelCallKind` ordinal values as
unnamed immediate constants.  All kind fields must use the symbolic constants
from `userland/experimental/hal/kernelcall_abi.inc`.

Permitted:
```asm
.include "kernelcall_abi.inc"
mov  w8, #KCALL_SendMessage
```

Forbidden:
```asm
mov  w8, #13    // SendMessage — literal ordinal, violates RFC-00BD R4
```

Exception: the request `magic` field may use the `.equ` constants
`KCALL_REQ_MAGIC_LO` / `KCALL_REQ_MAGIC_HI` directly; splitting it into
two 16-bit stores using these names is not considered a literal ordinal.

---

## Wire Format Version Bump Policy

A **breaking** wire format change requires:

1. Incrementing `kKernelAbiWireVersion` in `kernel_abi_wire.hpp`.
2. A new RFC that supersedes the relevant section of RFC-00B6 and this RFC.
3. CI fixtures that encode both the old and new wire format and verify that
   the bridge correctly rejects old-version blocks (or handles them via a
   compatibility shim, if one is defined by the new RFC).

**Breaking changes** (require version bump):

- Any change to a field's byte offset within the minimum request or response
  block.
- Any change to the meaning of an existing field.
- Any change to `kKernelAbiWireRequestMagic` or `kKernelAbiWireResponseMagic`.
- Adding a mandatory field that must be present in all requests regardless of
  the `bytes` field.

**Non-breaking changes** (no version bump required):

- Appending a new `KernelCallKind` entry (ordinal extension only).
- Adding optional request fields advertised via the `bytes` field and
  `KernelCallWireFlags` bits.
- Adding response fields beyond offset 47, advertised via `bytes`.
- Adding new `KernelCallWireFlags` bits (they are already uint64; new bits
  are zero in old implementations, which is a safe default).

---

## Assembly Symbolic Constants

The file `userland/experimental/hal/kernelcall_abi.inc` is the canonical
source of `KernelCallKind` ordinals for EL0 assembly.  It must be kept in
sync with `kernel_abi.hpp` and is checked by the CI gate (see below).

The file defines three categories of symbols:

**Kind ordinals** — `KCALL_<Name>`:
```asm
.equ KCALL_Yield,                         0
.equ KCALL_GetThreadIdentity,             10
.equ KCALL_ExitThread,                    12
.equ KCALL_SendMessage,                   13
.equ KCALL_ReceiveMessage,                14
// ... (complete table)
```

**Wire format magic** — two 16-bit halves for `movk` sequences:
```asm
.equ KCALL_REQ_MAGIC_LO,   0x5152    // low 16 bits of kKernelAbiWireRequestMagic
.equ KCALL_REQ_MAGIC_HI,   0x4B41    // high 16 bits
.equ KCALL_RSP_MAGIC_LO,   0x4250    // low 16 bits of kKernelAbiWireResponseMagic
.equ KCALL_RSP_MAGIC_HI,   0x4B41    // high 16 bits
.equ KCALL_WIRE_VERSION,   1
```

**Minimum block sizes** — for `mov x1, #N` / `mov x3, #N` in SVC #1 calls:
```asm
.equ KCALL_MIN_REQ_BYTES,  12
.equ KCALL_MIN_RSP_BYTES,  48
```

---

## CI Enforcement

### Golden manifest

`spec/rfcs/kernelcall_abi_manifest.txt` is a checked-in file that records
every frozen ordinal.  The CI check `check_kernelcall_abi_freeze.py` parses
`kernel_abi.hpp`, extracts the `KernelCallKind` enum values, and verifies:

1. Every entry in the manifest is present in `kernel_abi.hpp` at the stated
   ordinal (existing entries cannot drift).
2. No entry in `kernel_abi.hpp` has a lower ordinal than the manifest's
   highest entry (no insertions before the freeze point).
3. `kKernelAbiWireVersion` and the two magic constants in `kernel_abi_wire.hpp`
   match the manifest's `[wire]` section.

New entries appended to `kernel_abi.hpp` do not fail the check; they just
need to be added to the manifest in the same PR that introduces them.

### CI gate

The check is added to the existing lint step in the QEMU boot workflow and
to any new workflow that builds code depending on `kernel_abi.hpp`.

Failure message format:
```
FAIL: KernelCallKind ordinal drift detected
  SendMessage: manifest=13, enum=14
```

Success message:
```
PASS: KernelCallKind ordinals match manifest (54 entries, wire version=1)
```

---

## Relationship to Freestanding EL0 Stubs

The four existing EL0 assembly files are updated by this RFC to replace
literal kind integers with `kernelcall_abi.inc` symbols:

| File | Literals replaced |
| :--- | :--- |
| `axion_el0_init.S` | `#10` → `#KCALL_GetThreadIdentity` |
| `el0_process_stub.S` | `#10` → `#KCALL_GetThreadIdentity` |
| `el0_ipc_test_a.S` | `#13` → `#KCALL_SendMessage` |
| `el0_ipc_test_b.S` | `#14` → `#KCALL_ReceiveMessage` |

The magic byte sequences (`mov w8, #0x5152` / `movk w8, #0x4B41`) are
replaced with `KCALL_REQ_MAGIC_LO` / `KCALL_REQ_MAGIC_HI` constants.

The version/bytes packed word (`mov w8, #0x0001` / `movk w8, #0x0010`) is
not replaced by a symbol in this RFC — the `bytes` field value is
call-specific (16 for SendMessage, 12 for ReceiveMessage) and is not a
frozen constant.  A future RFC may define per-call minimum request sizes.

---

## Acceptance Criteria

- [x] `kernelcall_abi_manifest.txt` checked in with all 54 current entries
- [x] `scripts/ci/check_kernelcall_abi_freeze.py` implemented and passing
- [x] `kernelcall_abi.inc` implemented with full kind table + wire constants
- [x] `axion_el0_init.S`, `el0_process_stub.S`, `el0_ipc_test_a.S`,
  `el0_ipc_test_b.S` updated to use symbolic constants; no literal ordinals
- [x] QEMU boot CI gate extended with ordinal freeze check
- [x] RFC-00BE (scheduler) and RFC-00BF (observability) reference this RFC
  for their `BlockOnIpcReceive` (42) and `WaitForDevice` (43) kind values

---

## Relationship to Other RFCs

- **RFC-00B6** — Defined the `KernelCall` SVC ABI and the `KernelCallKind`
  enum.  This RFC freezes the ordinals defined there.
- **RFC-00BC** — Proved the freestanding EL0 path through Phase 8.  The
  Phase 8 stubs' literal `#13`/`#14` are the direct motivation for R4.
- **RFC-00BE** — The cooperative scheduler will use `BlockOnIpcReceive`
  (kind=42) and `WaitForDevice` (kind=43) as yield points.  Both ordinals are
  frozen here before the scheduler is designed.
- **RFC-00BF** — The observability ring stores `kind` as a uint32.  Frozen
  ordinals give the ring semantic stability across versions.
- **RFC-00C0/C1** — CanonFS executable identity may bind a content-addressed
  hash to a binary that embeds ordinal constants.  A frozen table ensures the
  hash remains valid as long as the binary and the table are at the same version.
- **RFC-DPE-0002** — `SubmitEpoch` (ordinal 53) is the DPE entry point
  through the KernelCall boundary.  Its ordinal is frozen here.

---

## References

- [RFC-00B6: Minimal Syscall and Capability Boundary](RFC-00B6-minimal-syscall-capability-boundary.md)
- [RFC-00BC: TernaryOS EL0 Userland Bring-Up](RFC-00BC-ternaryos-el0-userland-bringup.md)
- [RFC-DPE-0002: TISC Task Graph Primitives](RFC-DPE-0002-tisc-task-graph-primitives.md)
- `userland/experimental/kernel/kernel_abi.hpp` — authoritative enum source
- `userland/experimental/kernel/kernel_abi_wire.hpp` — wire constants source
- `userland/experimental/hal/kernelcall_abi.inc` — assembly symbolic constants
- `spec/rfcs/kernelcall_abi_manifest.txt` — CI golden manifest
- `scripts/ci/check_kernelcall_abi_freeze.py` — CI enforcement script
