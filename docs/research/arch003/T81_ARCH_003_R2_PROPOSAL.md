# T81 ARCH-003-R2 — External Deadline Authority Protocol Amendment Proposal

**Document Status:** PROPOSED / REVIEW DRAFT (Non-Authoritative Proposal)
**Proposed Identifier:** T81-ARCH-003-R2
**Baseline Accepted R1 Protocol:** Git commit `1454bd63426d21686cfb5ef22dbaebc3c9cae94b` (`docs/research/arch002/PROTOCOL.md`)
**Accepted I2 Implementation Reference:** Git commit `f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15` (`experiments/arch003/`)
**Triggering Host Qualification Record:** H5 — `HOST_CLOCK_ENFORCEMENT_BLOCKED`
**Parent Hypotheses:** Canonical H03 (ternary representation / packing / access costs) and H05 (three-state decision logic / Boolean equivalence)

---

## Executive Summary and Non-Activation Statement

This document constitutes a prospective protocol amendment proposal (T81-ARCH-003-R2). It proposes a strictly bounded exception to the process-tree restrictions in Section 8 of the accepted ARCH-002-R1 protocol, enabling an external host-side deadline authority (watchdog) to enforce campaign time ceilings independently of guest VM or container clock stability.

**Governance Notice:**
- This document does **not** modify or overwrite the accepted R1 protocol or I2 implementation in place.
- This document does **not** execute any scientific campaign, benchmark, or pilot.
- This document does **not** modify or rewrite historical evidence or qualifications.
- This document does **not** merge PR #716 or activate the proposed R2 revision automatically.
- The accepted I2 implementation is **not** claimed to be R2-compliant.
- Operational activation remains blocked pending formal maintainer review and commit-scoped authorization.

---

## 1. Required Analysis

### 1.1 Item 1: Exact §8 Process Restrictions Prohibiting the H5 Watchdog

Section 8 ("Resource feasibility, risks and next gate") of accepted protocol ARCH-002-R1 (commit `1454bd63426d21686cfb5ef22dbaebc3c9cae94b`) defines the runtime process hierarchy with strict exclusivity:

1. **Explicit Exhaustive Enumeration:**
   > "Permitted runtime tree is exactly:
   > - L: one always-active campaign owner/accountant, holding the single monotonic deadline, sealed expected identity, resource records and verifier exit checks.
   > - V: at most one independently implemented supervisor/verifier at a time...
   > - W: at most one worker process at a time, with one computation thread... No concurrent oracle/candidate workers, child pools, nested workers or per-case subprocesses."

2. **Prohibition of Extra Monitoring Processes:**
   > "External metadata utilities run only with V/W absent, within the same tree/caps; live monitoring uses L's in-process OS interfaces. No extra monitoring process or unaccounted helper is permitted while the runtime tree is populated."

3. **In-Process Clock Monopoly:**
   Under R1 §8, the campaign clock is held solely by L in userspace (`clock = time.monotonic_ns`). In virtualized or containerized guest environments (such as Jules Linux environments, QEMU guests, or hypervisor slices), guest monotonic clocks are vulnerable to host scheduling pauses, vCPU descheduling, VM suspension, or hypervisor time-dilation.

4. **The Qualification Block (H5 — `HOST_CLOCK_ENFORCEMENT_BLOCKED`):**
   When the guest environment experiences a pause or freeze, L cannot sample its own clock or detect execution timeouts. However, placing a watchdog process outside L on the host system violates the literal R1 prohibition: *"No extra monitoring process or unaccounted helper is permitted while the runtime tree is populated."* Hence, qualification H5 correctly recognized that independent host-side watchdog enforcement is legally blocked by the R1 process contract.

---

### 1.2 Item 2: Specification of a Bounded Exception for Host-Side Deadline Authority (Role H)

To eliminate the circular vulnerability where a frozen guest fails to observe its own deadline breach, scientific integrity demands an external authority with independent power to terminate an overrun campaign.

The protocol amendment defines a bounded role: **Role H (Host Deadline Authority / Watchdog)**.

**Scientific Justification:**
The purpose of the §8 process restriction was to prevent unobserved computational concurrency, hidden worker threads, or unmetered background tasks that could assist the candidate comparator or corrupt accounting. A host watchdog that:
1. Has zero visibility into candidate or oracle data buffers,
2. Has zero interprocess communication with candidate or verifier algorithms,
3. Executes strictly outside the guest VM/container boundary on the physical host,
4. Possesses purely monotonic termination capability (abort authority),
preserves 100% of the isolation and concurrency guarantees intended by §8 while closing the catastrophic blind spot of guest clock freeze.

**Role H Boundary Rules:**
- **Instance Bound:** Exactly one Role H process per campaign run, launched on the host prior to guest campaign inception.
- **Authority Bound:** Role H has write access only to its own host-side execution log and host-side termination signals (SIGTERM/SIGKILL to the guest VM/container process group). It has **zero read or write access** to guest memory, candidate IPC, or verification pipes.
- **Unidirectional Enforcement:** Role H can only abort the campaign; it cannot extend, resume, pause, or alter campaign parameters.

---

### 1.3 Item 3: Cryptographic and Deterministic Binding of Host Clock, Watchdog Survival, and Evidence Bundle

To prevent retrofitted or fabricated watchdog receipts, Role H and the guest campaign L must be deterministically bound:

1. **Pre-Campaign Nonce and Identity Handshake:**
   - Host generates a 256-bit cryptographic nonce $N_{host}$ using a secure OS entropy source (`/dev/urandom` / `getentropy`).
   - Host records $T_{host\_start} = \text{clock\_gettime}(\text{CLOCK\_MONOTONIC\_RAW})$ before launching the guest.
   - Host passes $N_{host}$, $T_{host\_start}$, and campaign UUID into the guest via immutable read-only boot configuration (environment variable or kernel command line).

2. **Durable Heartbeat Ledger:**
   - Guest L writes periodic heartbeat pulses to a designated shared memory file or host-observable descriptor: `{"seq": k, "guest_ns": t_k, "state": S_k}` at intervals not exceeding 5,000 ms.
   - Host H records host-timestamped arrivals: $T_{host\_recv}(k) = \text{CLOCK\_MONOTONIC\_RAW}$.
   - If the interval $T_{host\_recv}(k) - T_{host\_recv}(k-1) > \Delta_{max\_heartbeat}$ (threshold: 15,000 ms) or if $T_{host\_now} - T_{host\_start} > 1,800 \times 10^9\text{ ns}$, Role H terminates the guest.

3. **Sealing and Cryptographic Digest Binding:**
   - Upon successful campaign completion within limits, Role H computes its terminal receipt: `host-watchdog-receipt.json`, containing:
     - `campaign_uuid`, `nonce_hex`, `host_start_ns`, `host_end_ns`, `host_elapsed_ns`, `host_clock_id` (`CLOCK_MONOTONIC_RAW`), `heartbeat_count`, `max_heartbeat_gap_ns`, `termination_issued`: false.
   - The SHA-256 digest of `host-watchdog-receipt.json` is ingested by the guest finalizer into `manifest.json` and committed into the sealed `evidence.sha256` root.
   - If Role H terminates the guest, it writes `host-watchdog-termination.json` containing the exact breach reason, host timestamp, and PID tree kill confirmation.

---

### 1.4 Item 4: Fail-Closed Semantics for Deadline Breach, Watchdog Loss, Guest Pause, Host Sleep, and Missing Evidence

The protocol operates under strict fail-closed state transitions:

| Fault Condition | Detection Mechanism | Immediate Action | Final Protocol Status |
|---|---|---|---|
| **Deadline Breach** ($T_{host} > 1,800\text{ s}$)| Role H timer event | Role H sends `SIGKILL` to guest VM/container process group | `INCONCLUSIVE_RESOURCE` (Status 71); retain completed prefix; success root forbidden. |
| **Watchdog Loss / Death** | Guest L detects missing Role H pid/heartbeat ack at finalization | Guest L aborts finalization, deletes `evidence.sha256` | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |
| **Guest Pause / Freeze** | Role H misses 3 consecutive heartbeats ($>15\text{ s}$) | Role H terminates guest | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70); campaign marked unverified. |
| **Host Sleep / Suspension** | Role H detects jump in `CLOCK_MONOTONIC_RAW` relative to steady tick ($>100\text{ ms}$) | Role H terminates guest immediately | `INCONCLUSIVE_RESOURCE` (Status 71); host qualification invalidated. |
| **Missing Watchdog Receipt** | Verifier V finds missing or malformed `host-watchdog-receipt.json` | Verifier exits with code 1; final root rejected | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |

---

### 1.5 Item 5: Host-Side Monitoring Boundary vs. 512 MiB Aggregate RSS Limit

Section 8 of ARCH-002-R1 establishes:
> "<=512 MiB aggregate absolute resident memory over the live process tree"

**Resolution of the Host Process Boundary:**
1. **Separation of Accounting Domains:**
   - The 512 MiB aggregate absolute resident memory cap applies strictly to the **guest study process tree** (Owner L + Supervisor V + Worker W + compilation tools + maintenance threads).
   - Role H executes in the **host management domain** (outside the VM/container cgroup/namespace). It does not allocate memory from the guest memory pool and cannot page-share with candidate workers.
2. **Host Watchdog Resource Ceiling:**
   - To prevent host-side resource inflation, Role H must be implemented as a bounded lightweight binary (in native C/Rust or static POSIX sh/python) with an independent resident memory ceiling: $\text{RSS}_{H} \le 16\text{ MiB}$.
   - Role H records its own peak RSS via `getrusage(RUSAGE_SELF)` in `host-watchdog-receipt.json`. If $\text{RSS}_{H} > 16\text{ MiB}$, the watchdog receipt is invalid, causing fail-closed rejection.

---

### 1.6 Item 6: Inclusive 1,800-Second Campaign Envelope

The 1,800-second ($30.0$ minute) wall-clock deadline is absolute and non-resettable. It encompasses the entirety of the campaign execution pipeline:

$$\sum T = T_{configure} + T_{compile} + T_{link} + T_{oracles} + T_{guards} + T_{probes} + T_{profiles(D,R,S)} + T_{verify} + T_{hash\_seal} \le 1,800\text{ seconds}$$

- **No reset** on profile boundary ($D \to R \to S$).
- **No reset** on worker transitions ($W_i \to W_{i+1}$).
- **No exclusion** of file system I/O, artifact writing, or SHA-256 tree hashing.
- Role H starts its timer before initiating guest setup and triggers SIGKILL the instant $T_{host\_elapsed} > 1,800.0\text{ seconds}$.

---

### 1.7 Item 7: macOS Battery Operation and Sleep Invalidation

Protocol ARCH-002-R1 §6 states:
> "AC power, Low Power Mode off, no intentional concurrent workload, sleep disabled for the bounded run."

**Critical Defect in `caffeinate -s` on Battery Power:**
Under macOS Darwin power management (IOPM / CoreOS), `caffeinate -s` creates a power assertion (`NoIdleSleepAssertion`) preventing *system idle sleep*. However:
1. When running on DC battery power, macOS battery governors will enforce *Emergency Sleep*, *Thermal Sleep*, or *Low Battery Sleep* regardless of `caffeinate -s`.
2. Furthermore, modern macOS Apple Silicon power management performs aggressive background core frequency scaling, throttling, and vCPU descheduling when running on battery, invalidating timing determinism and watchdog heartbeat cadence.

**Normative R2 Requirements for macOS:**
- **AC Power Mandatory:** Execution on DC battery power is strictly prohibited.
- **Pre-Flight Power Check:** Role H must verify AC connection via `pmset -g batt` before launching guest L:
  - If output contains `Battery Power` or lacks `'AC Power'`, Role H must fail closed immediately: `BLOCK_HOST_POWER_QUALIFICATION`.
- **Assertion Type:** On macOS, the host must use `caffeinate -d -i -m -s` (display, idle, system, and disk assertion) wrapped in a hardware AC power gate.
- **Continuous Power Sampling:** Host H polls `IOPMPowerSource` every 5 seconds. Any transition to battery power immediately aborts the campaign with `INCONCLUSIVE_RESOURCE`.

---

### 1.8 Item 8: Independent Acceptance Tests and Negative Controls

To validate the Role H watchdog mechanism before scientific authorization, the following five negative control test fixtures must be implemented and verified:

1. **Test N1 — Injected Guest Deadlock / Overrun:**
   - Guest W enters an infinite sleep loop.
   - *Expected Result:* Role H fires at exactly $T_{start} + 1,800\text{ s}$ (or configured fixture timeout $\tau = 5\text{ s}$), sends SIGKILL to guest tree, writes `host-watchdog-termination.json`. Campaign fails closed with Status 71 (`INCONCLUSIVE_RESOURCE`); no success root created.
2. **Test N2 — Heartbeat Silence (Guest Pause / Hypervisor Deschedule):**
   - Guest artificially suspends heartbeat pulse emission for $>15\text{ s}$.
   - *Expected Result:* Role H detects heartbeat timeout, reaps guest, logs timeout. Campaign fails closed with Status 70 (`BLOCKED_RESOURCE_ACCOUNTING`).
3. **Test N3 — Host Sleep / Time Jump:**
   - Host clock artificially jumps forward by $>100\text{ ms}$ between consecutive watchdog ticks.
   - *Expected Result:* Role H detects clock irregularity / sleep event, terminates guest. Campaign fails closed.
4. **Test N4 — Missing / Tampered Watchdog Receipt:**
   - Guest finishes correctly, but `host-watchdog-receipt.json` is missing, has an unmatching nonce, or mismatched SHA-256 in manifest.
   - *Expected Result:* Independent verifier V fails closed with exit code 1; sealing is aborted.
5. **Test N5 — DC Battery Disconnect:**
   - Power state simulates disconnection of AC adapter mid-run.
   - *Expected Result:* Role H detects power drop, terminates run, records `BATTERY_POWER_DETECTED`.

---

### 1.9 Item 9: Preservation of Scientific Workloads and Oracle Independence

The R2 amendment is strictly an operational governance and host-authority amendment. It preserves 100% of the underlying scientific experiment:

- **Mathematical API Contract:** Inputs are immutable $\{-1, 0, +1\}$ arrays; operation is $C = \sum [a_i = b_i]$; operand-local format tags and strict error precedence unchanged (§2).
- **Candidate Representations:** Exact five representations (BYTE, C2, PLANES, PT5, NATIVE) preserved (§3).
- **Frozen Schedule and Cardinality:** Exactly 12 families, 408,034 expected requests, 483,553 calls per profile, 1,450,659 total calls across D, R, and S profiles (§4).
- **Oracle Independence:** Dual independent oracles (Scalar Oracle and Histogram Oracle) compile in separate translation units with zero candidate linkage; both streams must agree before candidate execution (§4).
- **Output Caps:** Numeric evidence $\le 84\text{ MiB}$, commands $\le 4\text{ MiB}$, diagnostics $\le 8\text{ MiB}$, manifests $\le 2\text{ MiB}$; total retained output $\le 100\text{ MiB}$ (§8).
- **Representation Comparison:** Strongest matched controls (B-BYTE, B-PLANES, B-C2, B-PT5) and statistical decision rules (§5, §7) remain completely unchanged.

---

### 1.10 Item 10: Revision Lineage, Verifier Revision, and Separate Acceptance Gate

- **Implementation Revision:** The accepted I2 implementation (`f408f6f`) only supports L-internal Linux tracing and lacks Role H watchdog handshake hooks. A new implementation revision **ARCH-003-I3** is required to support R2.
- **Verifier Revision:** `verifier.py` (Supervisor V) must be updated to inspect and validate `host-watchdog-receipt.json`, verifying nonce matching, clock continuity, and host signature before validating normal records.
- **Separate Acceptance Gate:** Protocol R2 adoption is a prerequisite for, but does **not** grant, implementation authorization. A separate maintainer gate must approve R2, followed by a separate commit-scoped authorization for I3 implementation, host qualification, and finally campaign execution.

---

## 2. Proposed R2 Protocol Amendment Text

The following text is proposed to amend Sections 6 and 8 of `docs/research/arch002/PROTOCOL.md`:

```markdown
### Proposed Amendment to Section 6 (Host Qualification)

Add the following normative power requirement:
- "AC power is mandatory. Execution on DC battery power is prohibited. On macOS
  and Linux hosts, an automated pre-flight probe must verify connection to AC
  mains power prior to campaign launch (e.g. `pmset -g batt` confirming 'AC Power'
  or `/sys/class/power_supply/` status 'Charging'/'Full'). Disabling sleep via
  `caffeinate -s` on macOS is insufficient on battery power due to hard-coded
  kernel thermal and battery sleep assertions. Any mid-campaign transition to
  battery power aborts the run as INCONCLUSIVE_RESOURCE."

### Proposed Amendment to Section 8 (Resource Feasibility, Risks and Process Hierarchy)

Replace the process hierarchy description with the following text:
"Permitted runtime tree is exactly:

- H: exactly one external host deadline authority (watchdog) executing on the
  physical host outside the guest VM/container boundary. H enforces the absolute
  1,800-second monotonic deadline independently of guest clock stability, polls
  AC power status, monitors guest heartbeats, and possesses purely monotonic
  SIGTERM/SIGKILL abort authority over the guest container/VM process group. H
  has zero access to candidate or oracle data and consumes <=16 MiB host RSS.
- L: one always-active campaign owner/accountant inside the guest environment,
  holding the single monotonic deadline, sealed expected identity, resource
  records, guest heartbeat generator, and verifier exit checks.
- V: at most one independently implemented supervisor/verifier at a time. It
  validates worker output against the sealed oracle stream, not the worker's
  own comparison or exit alone. It must not link candidate/oracle decode logic.
- W: at most one worker process at a time, with one computation thread, serving
  an oracle producer, adapter driver, negative guard or sanitizer probe. No
  concurrent oracle/candidate workers, child pools, nested workers or per-case
  subprocesses. Batch the normal matrix within one W per profile.

External host watchdog H operates under the bounded exception that live monitoring
and abort authority may reside on the host to prevent unobserved guest freezing.
All guest descendant processes (L, V, W, compiler, and tools) remain bound by the
<=512 MiB aggregate absolute resident memory cap, <=1,800 seconds total elapsed
time, and <=100 MiB retained output ceiling."
```

---

## 3. R1-to-R2 Change Matrix

| Section / Topic | Accepted R1 Protocol (`1454bd6`) | Proposed R2 Amendment | Scientific & Governance Rationale |
|---|---|---|---|
| **Process Roles (§8)** | Exactly L, V, W. "No extra monitoring process or unaccounted helper is permitted." | Adds external **Role H** on the host. Bounded exception to permit independent watchdog. | Resolves H5 qualification block; prevents unobserved guest freeze from masking deadline breach. |
| **Deadline Authority (§8)** | Solely inside guest L via `time.monotonic_ns()`. | Dual authority: guest L checks internally; host H enforces externally with hard kill. | Guarantees fail-closed termination even if guest kernel or hypervisor hangs. |
| **Watchdog RSS (§8)** | No external watchdog permitted; all memory in live tree $\le 512\text{ MiB}$. | Guest tree remains $\le 512\text{ MiB}$. Role H on host capped at $\le 16\text{ MiB}$ host RSS. | Preserves candidate memory pressure constraints without charging host hypervisor tools to guest. |
| **macOS Power (§6)** | "AC power... sleep disabled for the bounded run." | Explicit check for AC mains; DC battery prohibited; `caffeinate` alone recognized as defective on battery. | Prevents kernel thermal sleep and clock distortion on Apple Silicon battery power. |
| **Evidence Sealing (§8)** | Manifest seals expected, observed, and compiler receipts. | Adds `host-watchdog-receipt.json` signed with pre-campaign nonce bound to manifest. | Cryptographically binds host clock observation to final evidence root. |
| **Scientific Scope (§1–5, 7)** | 12 families, 408k requests, 5 encodings, dual oracles, statistical criteria. | **Identical (Unchanged).** | 100% preservation of scientific workload, controls, and falsification rules. |

---

## 4. New Acceptance Criteria for Protocol R2

Before R2 can be declared accepted and ready for implementation authorization:

1. **CRIT-R2-01 (Formal Specification Completeness):** The proposal must provide exact, unambiguous text replacements for §6 and §8 of `PROTOCOL.md` without leaving placeholders.
2. **CRIT-R2-02 (Zero Workload Mutation):** All 12 schedule families, 408,034 expected requests, 483,553 calls per profile, 5 representation formats, and dual oracle separation must be byte-level and logically identical to R1.
3. **CRIT-R2-03 (Host Clock Isolation):** Role H must be proven mathematically and architecturally incapable of injecting data, modifying execution parameters, or communicating with candidate algorithms.
4. **CRIT-R2-04 (Negative Control Specification):** Complete specifications for negative tests N1–N5 must be provided with deterministic fail-closed exit codes.
5. **CRIT-R2-05 (Host Power Verification):** Explicit commands and exit contracts for AC mains verification on Darwin and Linux must be specified.

---

## 5. Compatibility and Provenance Analysis

- **Backward Compatibility with R1:**
  R2 is strictly an operational relaxation of the process monitoring location restriction in §8. Any valid R1 execution that was completely observable and completed within 1,800 seconds remains valid under R2 when monitored by Role H.
- **Provenance and Lineage:**
  - Base protocol: PR #714 commit `1454bd63426d21686cfb5ef22dbaebc3c9cae94b`.
  - Baseline implementation: PR #716 commit `f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15`.
  - Prior qualification finding: H5 — `HOST_CLOCK_ENFORCEMENT_BLOCKED`.
  - The amendment maintains unbroken audit provenance, citing exact commits and preserving historical failure records in `experiments/arch003/validation/`.

---

## 6. Explicit List of Implementation Changes Required (ARCH-003-I3)

The following changes will be required in the follow-on implementation revision (ARCH-003-I3):

1. **Host Watchdog Binary (`experiments/arch003/host_watchdog.py` or `.c`):**
   - Implement Role H watchdog to run on host.
   - Generate pre-campaign cryptographic nonce $N_{host}$.
   - Query host power supply state (`pmset -g batt` / `/sys/class/power_supply`).
   - Monitor heartbeat pulses; enforce 1,800-second hard deadline via POSIX signals.
   - Write `host-watchdog-receipt.json` upon completion or `host-watchdog-termination.json` upon breach.
2. **Guest Campaign Handshake (`experiments/arch003/campaign.py`):**
   - Accept host nonce and host start timestamp from environment.
   - Emit periodic heartbeat pulses during execution.
   - Ingest `host-watchdog-receipt.json` during finalization.
3. **Verifier Receipt Validation (`experiments/arch003/verifier.py`):**
   - Add verification check for `host-watchdog-receipt.json`: check nonce equivalence, monotonic continuity ($T_{end} - T_{start} \le 1,800\text{ s}$), and absence of termination flag.
4. **Negative Test Fixtures (`experiments/arch003/tests/test_watchdog.py`):**
   - Implement test suite verifying N1 (deadlock kill), N2 (heartbeat timeout), N3 (host clock jump), N4 (tampered receipt), and N5 (battery disconnect).

---

## 7. Independent Reviewer Checklist

Independent maintainers and reviewers should use the following checklist to evaluate this proposal:

- [ ] **R2-CHK-01 (Scope Limitation):** Does the amendment strictly restrict Role H to host deadline and power monitoring without granting compute or candidate IPC roles?
- [ ] **R2-CHK-02 (Memory Boundary):** Is Role H explicitly isolated from the guest 512 MiB RSS limit, and is Role H capped at $\le 16\text{ MiB}$ host RSS?
- [ ] **R2-CHK-03 (Inclusive 1,800s Deadline):** Does the 1,800-second deadline unequivocally encompass configuration, compilation, linking, all D/R/S profiles, oracles, guards, probes, and evidence sealing without resets?
- [ ] **R2-CHK-04 (macOS Battery Hardening):** Does the proposal explicitly reject `caffeinate -s` on battery power and mandate automated AC power verification?
- [ ] **R2-CHK-05 (Fail-Closed Guarantees):** Are deadline breaches, watchdog terminations, heartbeat losses, and receipt corruptions guaranteed to fail closed with no success root?
- [ ] **R2-CHK-06 (Scientific Preservation):** Are all 12 schedule families, 408,034 expected requests, 483,553 calls/profile, representations, and oracle independence strictly preserved?
- [ ] **R2-CHK-07 (Non-Activation Integrity):** Does the proposal refrain from claiming that implementation I2 is compliant or executing campaigns prematurely?

---

*End of Proposal. Awaiting formal maintainer review.*
