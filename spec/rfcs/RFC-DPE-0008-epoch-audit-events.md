# RFC-DPE-0008: Epoch Audit Events

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81VM runtime, KernelAuditEventKind, axion_kernel_submit_epoch
**Created:** 2026-03-15
**Updated:** 2026-03-15
**Author:** @t81dev
**Depends on:** RFC-DPE-0003 (Epoch Execution), RFC-DPE-0007 (Epoch Timeout)
**Blocks:** epoch lifecycle observability, post-mortem audit correlation

---

## 1. Summary

Prior to this RFC only `EpochAbortedPolicyFault` emitted a `KernelAuditRecord`.
The other three epoch lifecycle transitions — acceptance/submission, successful
commit, and non-policy abort — were silent.  This leaves the canonical audit
trail incomplete: an observer cannot distinguish "epoch never submitted" from
"epoch submitted and committed" by reading the audit log alone.

This RFC adds three new `KernelAuditEventKind` variants and wires them into
`axion_kernel_submit_epoch()`:

| Event | When emitted |
| :--- | :--- |
| `EpochSubmitted` | After `accept_epoch()` succeeds — dispatched for execution |
| `EpochCommitted` | After `commit_epoch()` succeeds |
| `EpochAborted`   | On `Aborted_TaskFault`, `Aborted_ExclusiveConflict`, or `Aborted_Timeout` |

`EpochAbortedPolicyFault` (RFC-DPE-0003 §6.1) is retained unchanged as the
specific policy-gate variant.

---

## 2. KernelAuditEventKind additions

```cpp
EpochSubmitted,  ///< epoch accepted by accept_epoch() and dispatched (RFC-DPE-0008 §3.1)
EpochCommitted,  ///< epoch committed successfully via commit_epoch() (RFC-DPE-0008 §3.2)
EpochAborted,    ///< epoch aborted (TaskFault, ExclusiveConflict, Timeout) (RFC-DPE-0008 §3.3)
```

`EpochAbortedPolicyFault` is not subsumed — it remains the dedicated variant
for policy-gate denials (RFC-DPE-0003 §6.1) because policy aborts carry a
distinct operational meaning (governance enforcement, not runtime failure).

---

## 3. Emission points in axion_kernel_submit_epoch()

### 3.1 EpochSubmitted

Emitted immediately after `state.counters.epoch_submissions++`, when
`accept_epoch()` has passed and the epoch is about to begin level dispatch.
Not emitted on `Rejected_AcceptFailed` — that rejection predates submission.

### 3.2 EpochCommitted

Emitted after `state.epoch.last_committed_epoch_hash` is updated (§4 of the
existing implementation), immediately before returning
`KernelEpochResult{Ok, ...}`.

### 3.3 EpochAborted (non-policy)

Emitted on the two non-policy abort paths:
- Timeout abort (`Aborted_Timeout`): after `state.epoch.epochs_aborted++`
- Commit abort (`Aborted_TaskFault` / `Aborted_ExclusiveConflict`): after
  `state.counters.epoch_aborts++`

---

## 4. emit_epoch_audit() helper

A module-private `static void emit_epoch_audit(KernelRuntimeState&, KernelAuditEventKind)` helper:

1. Calls `record_audit_event()` (appends to `audit_log`, updates `last_audit_event`,
   increments `counters.audit_events_recorded`).
2. Increments the dedicated epoch audit counter (`epoch_audit_submissions`,
   `epoch_audit_commits`, or `epoch_audit_aborts`).
3. Updates `state.last_epoch_audit_kind` and `state.last_epoch_audit_sequence`
   from `state.last_audit_event->sequence`.

---

## 5. KernelRuntimeState additions

### 5.1 Counters

```cpp
uint64_t epoch_audit_submissions{0};  ///< EpochSubmitted events emitted
uint64_t epoch_audit_commits{0};      ///< EpochCommitted events emitted
uint64_t epoch_audit_aborts{0};       ///< EpochAborted events emitted
```

### 5.2 Retained state

```cpp
std::optional<KernelAuditEventKind> last_epoch_audit_kind{};
std::optional<uint64_t>             last_epoch_audit_sequence{};
```

---

## 6. KernelRuntimeStatusView additions

The DPE epoch section of `KernelRuntimeStatusView` gains:

```cpp
uint64_t epoch_audit_submissions{0};
uint64_t epoch_audit_commits{0};
uint64_t epoch_audit_aborts{0};
std::optional<KernelAuditEventKind> last_epoch_audit_kind{};
std::optional<uint64_t>             last_epoch_audit_sequence{};
```

---

## 7. Determinism note

`EpochAbortedPolicyFault` is already in the audit log.  The new events follow
the same `record_audit_event()` path (sequential, single-threaded kernel call
context) — they do not introduce any non-determinism.

---

## 8. Acceptance criteria

| ID | Criterion |
| :--- | :--- |
| [DPE-09-01] | A successful epoch emits exactly 1 `EpochSubmitted` + 1 `EpochCommitted`; `epoch_audit_submissions == 1`, `epoch_audit_commits == 1`. |
| [DPE-09-02] | `last_epoch_audit_kind == EpochCommitted` and `last_epoch_audit_sequence` is set after a successful epoch. |
| [DPE-09-03] | A faulted (task-fault) epoch emits `EpochSubmitted` + `EpochAborted`; `epoch_audit_aborts == 1`. |
| [DPE-09-04] | A timed-out (0 ms) epoch emits `EpochSubmitted` + `EpochAborted`; `last_epoch_audit_kind == EpochAborted`. |
| [DPE-09-05] | A policy-denied epoch emits `EpochSubmitted` + `EpochAbortedPolicyFault` (existing); `EpochAborted` is NOT emitted for policy denials. |
| [DPE-09-06] | `counters.audit_events_recorded` advances by 2 for each epoch lifecycle (submitted + committed/aborted). |
