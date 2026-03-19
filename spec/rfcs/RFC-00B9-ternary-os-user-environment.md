# RFC-00B9: TernaryOS User Environment Standard

**Status:** accepted
**Type:** standards-track
**Authors:** T81 Foundation
**Created:** 2026-03-16
**Depends-on:** RFC-00B3 (Axion Governance Kernel Architecture), RFC-00B4 (Userland Service Contract),
               RFC-00B6 (Minimal Syscall and Capability Boundary), RFC-0033 (Dual TUI Frontends)
**Relates-to:** RFC-00B0 (HAL), RFC-00B2 (Device Drivers), RFC-00B5 (Event/Interrupt Model),
               RFC-00B7 (Pager Service ABI), RFC-00B8 (Governed FFI)

---

## 1. Purpose

This RFC defines the **standard for a complete, ternary-native operating system user
environment** running on the Axion kernel.  It specifies:

- The **init service** and service graph bootstrap sequence
- The **session model**: user identity, login, TTY allocation, and session lifecycle
- The **environment shell**: `t81sh`, the ternary-native interactive shell
- The **service registry**: how user-visible services are declared, discovered, and managed
- The **display contract**: how TUI applications (RFC-0033) integrate with the kernel's
  terminal device layer (RFC-00B2)
- The **policy surface**: which Axion governance gates apply to user-environment operations
- **Acceptance criteria** for a conformant TernaryOS user environment implementation

The user environment sits above the kernel ABI (RFC-00B6) and the pager (RFC-00B7), and
below the T81Lang application layer.  It is the layer that makes TernaryOS usable as a
day-to-day operating environment, not just a bootable runtime.

---

## 2. Motivation

RFC-00B3 through RFC-00B8 define the Axion kernel, memory model, device layer, interrupt
model, syscall boundary, pager ABI, and FFI.  The kernel is now fully specified.  What is
absent is a normative definition of **what happens after boot** from the perspective of a
human user or an AI agent:

- Which service starts first and how it spawns the rest?
- How does a user log in?  What is a "session"?
- What shell or REPL is available?
- How do TUI applications (RFC-0033 `t81 studio` / `t81 agent`) attach to a live terminal?
- How are user-space services declared so they can be started on demand?
- What Axion policy gates protect login, session creation, and service spawning?

Without this RFC, TernaryOS has a kernel but no defined user-facing behaviour — equivalent
to having a POSIX kernel with no `init`, no `/etc/passwd`, and no shell.

---

## 3. Scope and Non-Goals

**In scope:**

- TernaryOS init service (`t81-init`) bootstrap contract
- Session model (login, TTY, session identity, logout)
- `t81sh` interactive shell specification
- Service registry format and activation semantics
- Display / terminal device contract for TUI applications
- Axion policy gates for environment operations
- Acceptance criteria

**Not in scope:**

- GUI / graphical display server (TernaryOS is terminal-native by design; RFC-0033 §Non-Goals)
- Network stack or remote login (deferred to a future RFC-00C-series)
- Package management
- Multi-user concurrent sessions beyond the single-TTY model (deferred)

---

## 4. Architecture Overview

```
┌───────────────────────────────────────────────────────────────┐
│  User Applications  (T81Lang programs, t81 studio, t81 agent) │
├───────────────────────────────────────────────────────────────┤
│  t81sh  ·  Service Registry  ·  Session Manager               │  ← THIS RFC
├───────────────────────────────────────────────────────────────┤
│  t81-init (PID 1 equivalent)                                  │  ← THIS RFC
├───────────────────────────────────────────────────────────────┤
│  RFC-00B6  Syscall / Capability Boundary                       │
│  RFC-00B7  Pager Service ABI                                   │
│  RFC-00B3  Axion Governance Kernel                                        │
│  RFC-00B0  HAL  ·  RFC-00B1 MMU  ·  RFC-00B2 Devices          │
└───────────────────────────────────────────────────────────────┘
```

---

## 5. Init Service (`t81-init`)

### 5.1 Role

`t81-init` is the first userland process spawned by the Axion kernel after the boot
handoff (RFC-00B0 §4).  It holds `SupervisorId = 1` and `ProcessGroupId = 1`.  All other
userland processes are descendants of `t81-init`.

`t81-init` is **not** a general-purpose daemon supervisor like `systemd`.  It performs
exactly three tasks in order:

1. **Mount** the CanonFS root (read-only) and the scratch overlay (read-write)
2. **Read** the service registry from `canonfs://etc/t81-services.json`
3. **Spawn** the `required` service set and the session manager

### 5.2 Boot Sequence

```
Axion kernel  →  t81-init (PID 1)
                     │
                     ├─ mount canonfs:// (RFC-00B2 storage driver)
                     ├─ read /etc/t81-services.json
                     ├─ spawn t81-session-mgr  (session manager)
                     └─ spawn required services  (see §7)
```

The boot sequence is **deterministic**: services are started in the order they appear in
the `required` array of `t81-services.json`, with dependency ordering resolved by
topological sort on the `depends` field.  A boot hash covering the service graph and each
service binary's CanonHash81 digest is recorded to CanonFS at
`canonfs://var/log/boot-<epoch>.canonhash`.

### 5.3 Fault Policy

If any `required` service fails to start within its `start_timeout_ms` deadline,
`t81-init` records an Axion audit event (`BootFault`) and halts.  `on_demand` services
that fail are quarantined; they may be retried by the session manager.

### 5.4 Axion Policy Gate

`t81-init` holds an implicit `BootSupervisor` capability (granted by the kernel at PID 1
spawn).  All `SpawnProcess` syscalls from `t81-init` are unconditionally allowed.
Subsequent spawns from any other process require the `ServiceSpawn` capability (§9).

---

## 6. Session Model

### 6.1 Session Identity

A **session** is a bounded execution context associated with a single authenticated
principal and a single TTY device handle.  It is represented by a `SessionId` — a
monotonically incrementing `u64` allocated by the session manager.

```
SessionRecord {
    session_id:   u64,          // monotonic; never reused
    principal:    PrincipalId,  // authenticated user identity
    tty_handle:   DeviceHandle, // RFC-00B2 terminal device
    root_pgid:    ProcessGroupId,
    start_epoch:  u64,          // TISC deterministic epoch counter
    state:        Active | Suspended | Terminated,
    canon_hash:   CanonHash81,  // hash of session configuration at login
}
```

`SessionId`s are written to CanonFS at `canonfs://var/sessions/<session_id>.json` on
creation and updated on state transitions.  This provides a tamper-evident session audit
trail.

### 6.2 Login Flow

```
TTY device ready  →  t81-login prompt
                          │
                    [principal authenticates]
                          │
                    session manager allocates SessionId
                          │
                    Axion policy gate: SessionCreate  (§9.2)
                          │
                    spawn t81sh with session environment variables
                          │
                    session active
```

Authentication is via a challenge/response against the principal store at
`canonfs://etc/principals.json`.  Credentials are stored as Argon2id hashes.  The
plaintext credential never touches CanonFS.

### 6.3 Logout and Session Termination

When `t81sh` exits (user types `exit` or `Ctrl-D`), the session manager:

1. Sends `SIGTERM` to all processes in the session's `root_pgid`
2. Waits up to `session_drain_timeout_ms` (default 5 000 ms) for clean exit
3. Sends `SIGKILL` to any remaining processes
4. Marks the `SessionRecord` as `Terminated` and writes the final CanonHash81
5. Frees the TTY device handle
6. Re-presents the `t81-login` prompt

### 6.4 Session Environment Variables

The following variables are injected into every session process:

| Variable         | Value                              |
|------------------|------------------------------------|
| `T81_SESSION_ID` | hex-encoded `SessionId`            |
| `T81_PRINCIPAL`  | authenticated principal name       |
| `T81_TTY`        | TTY device path (e.g. `/dev/tty0`) |
| `T81_EPOCH`      | TISC epoch counter at login        |
| `T81_CANON_ROOT` | CanonFS mount point                |
| `PATH`           | `/bin:/usr/bin:/usr/local/bin`     |

---

## 7. Service Registry

### 7.1 Format

Services are declared in `canonfs://etc/t81-services.json`:

```json
{
  "version": 1,
  "services": [
    {
      "id":             "t81-session-mgr",
      "binary":         "/bin/t81-session-mgr",
      "canon_hash":     "<CanonHash81 of binary>",
      "activation":     "required",
      "depends":        [],
      "capabilities":   ["SessionCreate", "TtyAllocate", "ServiceSpawn"],
      "start_timeout_ms": 2000,
      "restart_policy": "always"
    },
    {
      "id":             "t81-canonfs-daemon",
      "binary":         "/bin/t81-canonfs-daemon",
      "canon_hash":     "<CanonHash81 of binary>",
      "activation":     "required",
      "depends":        [],
      "capabilities":   ["CanonFSWrite"],
      "start_timeout_ms": 1000,
      "restart_policy": "always"
    },
    {
      "id":             "t81-studio",
      "binary":         "/usr/bin/t81",
      "args":           ["studio"],
      "canon_hash":     "<CanonHash81 of t81 binary>",
      "activation":     "on_demand",
      "depends":        ["t81-session-mgr"],
      "capabilities":   ["TtyRead", "TtyWrite"],
      "start_timeout_ms": 5000,
      "restart_policy": "never"
    }
  ]
}
```

### 7.2 Activation Modes

| Mode        | Meaning                                                         |
|-------------|-----------------------------------------------------------------|
| `required`  | Started by `t81-init` during boot; halt on failure             |
| `on_demand` | Started by the session manager when first requested            |
| `manual`    | Never started automatically; user or agent invokes explicitly  |

### 7.3 Capability Grants

Each service entry declares the capabilities it requires.  The session manager verifies
that the requesting principal holds a `CapabilityDelegate` right for each declared
capability before spawning the service (RFC-00B6 §3).  Unknown capabilities cause spawn
to be denied with an Axion `CapabilityViolation` audit event.

### 7.4 Binary Integrity

The `canon_hash` field is verified by `t81-init` and the session manager against
`CanonHash81(binary_bytes)` before every spawn.  A mismatch produces an Axion
`IntegrityViolation` audit event and the spawn is aborted.

---

## 8. `t81sh` — Ternary-Native Shell

### 8.1 Purpose

`t81sh` is the standard interactive shell for TernaryOS.  It is a deterministic,
policy-governed command interpreter that:

- Reads T81Lang expressions and statements at a prompt
- Executes them in a sandboxed T81VM instance (TISC Tier 1 by default)
- Supports pipeline composition: `cmd1 | cmd2`
- Integrates with the session manager for job control
- Logs every executed command to CanonFS at `canonfs://var/sessions/<sid>/history.jsonl`

### 8.2 Prompt Format

```
[T81_PRINCIPAL@T81_SESSION_ID tier=1]$ _
```

Example:
```
[root@00000001 tier=1]$ std.io.println("hello")
hello
[root@00000001 tier=1]$
```

### 8.3 Built-in Commands

| Command           | Description                                              |
|-------------------|----------------------------------------------------------|
| `exit [code]`     | Terminate the session                                    |
| `cd <path>`       | Change working directory                                 |
| `ls [path]`       | List CanonFS directory                                   |
| `cat <path>`      | Print CanonFS file contents                              |
| `hash <path>`     | Print CanonHash81 of a CanonFS artifact                  |
| `run <file.tisc>` | Execute a TISC bytecode file in the session VM           |
| `compile <f.t81>` | Compile a T81Lang source file to TISC bytecode           |
| `policy <cmd>`    | Inspect or modify Axion policies                         |
| `service <cmd>`   | Start/stop/status service registry entries               |
| `tier <n>`        | Elevate the session VM to Tier n (requires capability)   |
| `studio`          | Launch `t81 studio` TUI (RFC-0033) in the current TTY   |
| `agent`           | Launch `t81 agent` TUI (RFC-0033) in the current TTY    |

### 8.4 T81Lang REPL Integration

When `t81sh` detects a T81Lang expression (starts with a keyword, type name, or `fn`),
it passes the input to an embedded T81Lang REPL powered by the same frontend pipeline as
`t81 repl`.  The REPL shares the session VM state; variables declared in one line persist
for the duration of the session.

### 8.5 Pipeline Semantics

Pipelines connect the `stdout` of one process to the `stdin` of the next via an
Axion-governed IPC channel (RFC-00B5 §3).  Each stage runs in its own `ProcessGroupId`
within the session's supervisor hierarchy.  The pipeline is deterministic: all stages are
synchronised to the same TISC epoch counter at pipeline entry.

### 8.6 Command Audit Trail

Every `t81sh` command (built-in or subprocess) is appended to
`canonfs://var/sessions/<sid>/history.jsonl` as:

```json
{"epoch": 12345, "cmd": "compile main.t81", "exit_code": 0,
 "duration_ms": 42, "axion_verdict": "Allow"}
```

The history file is append-only and immutable once written (CanonFS guarantee).

---

## 9. Axion Policy Gates

The user environment introduces four new policy gates.  All are intercepted by the Axion
policy engine before the operation proceeds.

### 9.1 `BootService`

Fired by `t81-init` for each service spawn during the boot sequence.

| Field      | Value                                  |
|------------|----------------------------------------|
| `op`       | `BootService`                          |
| `subject`  | service `id` from registry             |
| `payload`  | `canon_hash` of service binary         |
| `verdict`  | `Allow` / `Deny`                       |

Default policy: `Allow` if `canon_hash` matches; `Deny` otherwise.

### 9.2 `SessionCreate`

Fired by the session manager when a principal successfully authenticates.

| Field      | Value                                      |
|------------|--------------------------------------------|
| `op`       | `SessionCreate`                            |
| `subject`  | `PrincipalId`                              |
| `payload`  | `tty_handle`, `start_epoch`                |
| `verdict`  | `Allow` / `Deny` / `Warn`                  |

`Deny` prevents the session from opening; the login prompt is re-presented.

### 9.3 `ServiceSpawn`

Fired by the session manager for any `on_demand` or `manual` service start.

| Field      | Value                                      |
|------------|--------------------------------------------|
| `op`       | `ServiceSpawn`                             |
| `subject`  | service `id`                               |
| `payload`  | requesting `SessionId`, capabilities list  |
| `verdict`  | `Allow` / `Deny`                           |

### 9.4 `ShellExec`

Fired by `t81sh` for every non-builtin command executed.

| Field      | Value                                                 |
|------------|-------------------------------------------------------|
| `op`       | `ShellExec`                                           |
| `subject`  | command string (first token)                          |
| `payload`  | full argument list, `SessionId`, current Tier         |
| `verdict`  | `Allow` / `Deny` / `Warn`                             |

A `Warn` verdict executes the command but appends a warning to the session audit log.

---

## 10. Display / TTY Contract

### 10.1 TTY Device Model

TernaryOS exposes TTY devices as RFC-00B2 character devices at `/dev/ttyN` (N = 0, 1, …).
Each TTY device handle supports:

- `read(buf, len)` — read raw bytes from the terminal input buffer
- `write(buf, len)` — write bytes to the terminal output
- `ioctl(TIOCGWINSZ)` — query terminal dimensions (columns × rows)
- `ioctl(TIOCSRAW)` / `ioctl(TIOCSNORMAL)` — switch between raw and cooked mode

### 10.2 TUI Application Integration (RFC-0033)

When `t81 studio` or `t81 agent` is launched from `t81sh`, the shell:

1. Places the TTY device into **raw mode** via `ioctl(TIOCSRAW)`
2. Transfers the TTY handle to the TUI process via a capability delegation (RFC-00B6 §3)
3. Suspends its own read loop
4. Waits for the TUI process to exit
5. Reclaims the TTY handle and restores **cooked mode** via `ioctl(TIOCSNORMAL)`

This is analogous to POSIX job control with `SIGTSTP` / `SIGCONT`, but implemented
entirely through Axion capability transfer — no signals involved.

### 10.3 Multiplexing (Future)

Single-TTY multiplexing (split-screen, tab switching between shell and TUI panes) is
deferred to a future RFC.  The current model is strictly one-foreground-process-per-TTY.

---

## 11. Principal Store

The principal store at `canonfs://etc/principals.json` defines the set of valid login
principals.  It is read-only at runtime (modifications require a privileged
`CanonFSWrite` + `PrincipalAdmin` capability pair).

```json
{
  "version": 1,
  "principals": [
    {
      "id":           1,
      "name":         "root",
      "password_hash": "<argon2id hash>",
      "capabilities": ["SessionCreate", "ServiceSpawn", "TtyAllocate",
                        "CanonFSWrite", "TierElevate", "PrincipalAdmin"],
      "default_tier": 1
    },
    {
      "id":           2,
      "name":         "agent",
      "password_hash": "<argon2id hash>",
      "capabilities": ["SessionCreate", "ServiceSpawn", "TtyAllocate"],
      "default_tier": 2
    }
  ]
}
```

The `root` principal holds all capabilities and is the only principal that can modify
the principal store or service registry.  The `agent` principal is pre-configured for
AI agent sessions (RFC-0033 `t81 agent`) with Tier 2 default.

---

## 12. Acceptance Criteria

| ID     | Criterion                                                                            |
|--------|--------------------------------------------------------------------------------------|
| AC-1   | `t81-init` boots, reads `t81-services.json`, and spawns `required` services in topological order |
| AC-2   | Boot hash is written to `canonfs://var/log/boot-<epoch>.canonhash`                   |
| AC-3   | Binary integrity check fires Axion `IntegrityViolation` and aborts spawn on mismatch |
| AC-4   | Session manager allocates a unique, monotonically increasing `SessionId` per login    |
| AC-5   | `SessionRecord` is written to CanonFS on creation and updated on state transition     |
| AC-6   | Login with invalid credentials fires `SessionCreate` Deny verdict; prompt re-presented|
| AC-7   | Logout drains the session `ProcessGroupId` within `session_drain_timeout_ms`          |
| AC-8   | `t81sh` prompt includes principal name, session ID, and current Tier                  |
| AC-9   | `t81sh` built-in commands (`compile`, `run`, `hash`, `policy`, `service`, `tier`) execute without error |
| AC-10  | Every `t81sh` command is appended to the session history JSONL in CanonFS             |
| AC-11  | `ShellExec` Axion gate fires for every non-builtin command; `Deny` blocks execution   |
| AC-12  | `studio` / `agent` built-ins transfer TTY into raw mode, launch RFC-0033 TUI, restore on exit |
| AC-13  | `on_demand` service activation triggers `ServiceSpawn` Axion gate                    |
| AC-14  | A service with an unrecognised capability in its declaration is refused at spawn time  |
| AC-15  | `t81sh` T81Lang REPL shares VM state across lines within a session                    |

---

## 13. Implementation Plan

### Phase 1 — Init and Session Manager

- Implement `t81-init` as a statically linked TISC userland binary
- Implement `t81-session-mgr` with `SessionCreate` gate and TTY allocation
- Implement `t81-login` prompt with Argon2id credential check against principal store
- Boot hash recording to CanonFS

### Phase 2 — `t81sh`

- Implement shell tokenizer, pipeline parser, and builtin dispatch
- Integrate T81Lang REPL (reuse `lang/frontend` pipeline)
- CanonFS history JSONL append
- `ShellExec` Axion gate integration

### Phase 3 — Service Registry and TUI Integration

- Service registry loader and topological sort
- On-demand service activation via `ServiceSpawn` gate
- TTY raw/cooked mode handoff for RFC-0033 TUI binaries
- Binary integrity verification at spawn time

### Phase 4 — CI and Acceptance Tests

- Add `ternaryos_user_env_test` to the CTest suite (AC-1 through AC-15)
- Snapshot-style golden-output tests for `t81sh` prompt, login flow, and command audit trail

Status 2026-03-18: met in the opt-in TernaryOS build (`-DT81_ENABLE_TERNARYOS=ON`).
The remaining work is promotion hardening for default-on/runtime integration, not an
acceptance blocker.

---

## 14. Relation to Other RFCs

| RFC       | Relationship                                                         |
|-----------|----------------------------------------------------------------------|
| RFC-00B0  | HAL provides boot handoff; `t81-init` is the first userland process  |
| RFC-00B2  | Device drivers provide TTY and CanonFS storage backing               |
| RFC-00B3  | Axion kernel provides `SupervisorId`, process spawn, capability model|
| RFC-00B4  | Userland service contract defines `Tid`, `ProcessGroupId`, lifecycle |
| RFC-00B5  | IPC event model backs `t81sh` pipeline stage communication           |
| RFC-00B6  | Syscall boundary enforces capability checks for `ServiceSpawn` etc.  |
| RFC-00B7  | Pager ABI backs CanonFS memory-mapped session history                |
| RFC-00B8  | Governed FFI allows `t81sh` scripts to call native host functions    |
| RFC-0033  | Dual TUI frontends are first-class citizens of the user environment  |

---

## 15. Changelog

| Date       | Change               |
|------------|----------------------|
| 2026-03-16 | Initial proposed draft |
