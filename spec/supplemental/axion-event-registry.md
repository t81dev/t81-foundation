# Axion AI Hook Event Registry

**Normative ref:** RFC-0032 §6.4
**Status:** accepted
**Last updated:** 2026-03-15

This document is the single authoritative registry of AI hook event identifiers
emitted by the `AIHookEngine` (RFC-0032 §4.1).  Implementations MUST emit
these identifiers verbatim.  Consumers (CI audit runners, conformance test
harnesses, policy dashboards) MUST match against these exact strings.

---

## Event identifier format

```
<event_name> [key=value ...]
```

- `event_name`: lowercase snake_case identifier (this registry).
- Key-value pairs: space-separated, `key=value`, no quoting.
- Events are emitted as entries in `AIHookEngine::ai_trace()` (a
  `std::vector<std::string>`).
- The trace is hashed by `EvidenceCollector::record_trace()` using FNV-1a
  64-bit for determinism verification.

---

## Registry

### `model_load`

Emitted by `load_model_via_tloadhash()` when the Axion TLOADHASH gate
evaluates a model hash.

| Field | Type | Description |
|-------|------|-------------|
| `hash` | hex string | The model hash under evaluation |
| `verdict` | `allow` \| `deny` | Policy decision |

**Example (allow):**
```
model_load hash=abc123def456 verdict=allow
```

**Example (deny):**
```
model_load hash=abc123def456 verdict=deny
```

**Normative source:** RFC-0032 §5.2 (TLOADHASH promotion gate)

---

### `attn_guard`

Emitted by `AIHookEngine` before the ATTN opcode is executed.  Always emitted
regardless of tier verdict; the subsequent `ai_exec_gate` event carries the
final policy decision.

| Field | Type | Description |
|-------|------|-------------|
| `shape` | string | Tensor shape descriptor, e.g. `1x1x1` or `4x4x4` |
| `tier` | uint | Cognitive tier at time of guard evaluation |

**Example:**
```
attn_guard shape=1x1x1 tier=2
```

**Normative source:** RFC-0032 §8.1 (ATTN tier gate); RFC-0026 §5.15

---

### `qmatmul_guard`

Emitted by `AIHookEngine` before the QMATMUL opcode is executed.  QMATMUL has
no tier gate; this event always precedes `ai_exec_gate backend=t81vm policy=allow`.

| Field | Type | Description |
|-------|------|-------------|
| `shape` | string | Tensor shape descriptor |
| `wt_hash` | hex string | Weight hash for audit (may be `0` if unspecified) |

**Example:**
```
qmatmul_guard shape=4x4 wt_hash=0
```

**Normative source:** RFC-0032 §8.1

---

### `ai_exec_gate`

Emitted by `AIHookEngine` after each AI opcode pre-execution hook completes.
Records the backend target and the final Axion policy verdict.

| Field | Type | Description |
|-------|------|-------------|
| `backend` | string | Execution backend — always `t81vm` for conformant implementations |
| `policy` | `allow` \| `deny` | Final policy verdict |

**Example (allowed):**
```
ai_exec_gate backend=t81vm policy=allow
```

**Example (denied — ATTN at Tier0):**
```
ai_exec_gate backend=t81vm policy=deny
```

**Normative source:** RFC-0032 §8.1, RFC-0026 §5.15

---

## Event ordering contract

For each AI opcode invocation, events MUST appear in the trace in this order:

1. `<opcode>_guard` (e.g. `attn_guard`, `qmatmul_guard`)
2. `ai_exec_gate`

The `model_load` event is independent of opcode execution and appears only
during TLOADHASH evaluation.

---

## Conformance test cross-references

| Event | C++ test file | Test ID |
|-------|---------------|---------|
| `model_load` | `tests/cpp/axion_ai_hooks_test.cpp` | C04-05, C04-06 |
| `attn_guard` | `tests/cpp/axion_ai_hooks_test.cpp` | C04-03 |
| `ai_exec_gate` | `tests/cpp/axion_ai_hooks_test.cpp` | C04-07 |
| `qmatmul_guard` | `tests/cpp/axion_ai_hooks_test.cpp` | C04-08 |
| Trace reproducibility | `tests/determinism/evidence_collector.cpp` | C06-05 |

---

---

## FFI Events (RFC-00B8)

FFI audit events are emitted by `FFIDispatcher` into `FFICallResult::audit_events`
and flow into the Axion audit trail via the governed FFI path.  They use the same
`key=value` format as AI hook events.

---

### `ffi_call`

Emitted by `FFIDispatcher::generate_audit_events_()` for every completed FFI
dispatch (including non-success outcomes that reach the execute path).

| Field | Type | Description |
|-------|------|-------------|
| `fn` | string | Registered function name |
| `lib` | string | Library name |
| `type` | `deterministic` \| `governed` \| `quarantined` | FFI classification |
| `status` | `success` \| `policy_denied` \| `type_mismatch` \| `resource_exhausted` \| `external_error` \| `quarantine_required` | Result status |
| `ns` | uint | Execution time in nanoseconds |

**Example (success):**
```
ffi_call fn=getpid lib=libc type=governed status=success ns=142
```

**Normative source:** RFC-00B8 §7 (Q1 resolution — governance cost)

---

### `ffi_quarantine`

Emitted when a `@quarantined` function call is blocked by the fail-closed
quarantine gate before policy evaluation.

| Field | Type | Description |
|-------|------|-------------|
| `fn` | string | Quarantined function name |
| `lib` | string | Library name |

**Example:**
```
ffi_quarantine fn=dangerous_op lib=libdangerous
```

**Normative source:** RFC-00B8 §7 (Q6 resolution — sandbox boundary)

---

### `ffi_policy_deny`

Emitted when the Axion policy engine denies an FFI call.

| Field | Type | Description |
|-------|------|-------------|
| `fn` | string | Function name |
| `reason` | string | Policy denial reason |

**Example:**
```
ffi_policy_deny fn=write_file reason=Policy denied: write operations restricted
```

**Normative source:** RFC-00B8 §3.4 (governance gate)

---

### `ffi_capability`

Emitted once per required capability when `FFIFunction::required_capabilities`
is non-empty.  Capabilities are surfaced to the policy engine as `trace_reasons`;
this event makes them observable in the audit trail.

| Field | Type | Description |
|-------|------|-------------|
| `fn` | string | Function name |
| `cap` | string | Required capability name |

**Example (two required capabilities):**
```
ffi_capability fn=privileged_op cap=filesystem_write
ffi_capability fn=privileged_op cap=network_access
```

**Normative source:** RFC-00B8 §7 (Q6 resolution — capability gate)

---

### `ffi_error`

Emitted alongside `ffi_call` when status is not `success` and an error message
is available.

| Field | Type | Description |
|-------|------|-------------|
| `fn` | string | Function name |
| `msg` | string | Error message |

**Example:**
```
ffi_error fn=broken_op msg=Exception in FFI call: segfault
```

**Normative source:** RFC-00B8 §3.4 (error propagation)

---

## Event ordering contract (FFI)

For each FFI call, events appear in this order:

1. `ffi_quarantine` — if `@quarantined` and blocked (no further events)
2. `ffi_policy_deny` — if policy denies (no further events)
3. `ffi_capability` × N — one per required capability (if any)
4. `ffi_call` — always present if execution was attempted
5. `ffi_error` — present only on non-success outcomes with an error message

---

## Change log

| Date | Change |
|------|--------|
| 2026-03-22 | FFI events added (RFC-00B8 §7 Q6 sandbox resolution) |
| 2026-03-15 | Initial registry created (RFC-0032 §6.4 closure) |
