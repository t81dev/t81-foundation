# T81 ARCH-003-R2 — External Deadline Authority Protocol Amendment Proposal

**Document Status:** PROPOSED / REVIEW DRAFT (Non-Authoritative Proposal)
**Proposed Identifier:** T81-ARCH-003-R2
**Baseline Accepted R1 Protocol:** Git commit `1454bd63426d21686cfb5ef22dbaebc3c9cae94b` (`docs/research/arch002/PROTOCOL.md`)
**Accepted I2 Implementation Reference:** Git commit `f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15` (`experiments/arch003/`)
**Triggering Host Qualification Record:** H5 — `HOST_CLOCK_ENFORCEMENT_BLOCKED`
**Parent Hypotheses:** Canonical H03 (ternary representation / packing / access costs) and H05 (three-state decision logic / Boolean equivalence)

---

## Executive Summary and Non-Activation Statement

This document constitutes a prospective protocol amendment proposal (T81-ARCH-003-R2) for independent review. It amends Section 6 (Host Qualification) and Section 8 (Resource Feasibility, Risks and Process Hierarchy) of the accepted ARCH-002-R1 protocol (`1454bd63426d21686cfb5ef22dbaebc3c9cae94b`). It specifies an independently enforced host-side deadline authority (Role H watchdog) capable of terminating overrun, frozen, or suspended campaigns from outside the guest VM or container.

**Governance Notice:**
- This proposal does **not** modify or overwrite the accepted R1 protocol or I2 implementation in place.
- This proposal does **not** execute any scientific campaign, benchmark, or pilot.
- This proposal does **not** modify or rewrite historical evidence or qualifications.
- This proposal does **not** merge PR #716 or activate the proposed R2 revision automatically.
- The accepted I2 implementation is **not** claimed to be R2-compliant.
- Operational activation remains strictly blocked pending formal maintainer review and commit-scoped authorization.

---

## 1. Codex Review Findings and Residual Defects Disposition

This revision directly resolves all prior Codex findings and addresses the four residual specification defects:

| Finding / Defect | Issue Summary | Proposal Disposition | Section References | Unresolved Limitations |
|---|---|---|---|---|
| **Defect 1** | Terminal Host Receipt binding after guest sealing. Need an independently authenticated host receipt created after guest sealing and final verification, bound to exact sealed evidence digest, required for acceptance, covered by the 1,800s deadline. | **RESOLVED:** Defined `host-terminal-receipt.json` created by Role H on the host after guest emits its sealed evidence digest (`evidence.sha256`). Bound cryptographically to the root digest, campaign UUID, total suspend-inclusive elapsed time, and signed with the host Ed25519 private key. Acceptance requires both valid guest bundle and matching host terminal receipt within the 1,800s deadline. | Section 2.3 (§2.3.3), Section 2.6 (§2.6), Section 2.11 (§2.11) | Host and guest clocks are independent; receipt binds host observation to guest root digest rather than asserting identical internal guest tick counts. |
| **Defect 2** | Independently trusted Ed25519 public-key identity. A guest-controlled public key file cannot authenticate a host receipt; trust roots must be host-controlled. | **RESOLVED:** Specified that `PK_host` is established in the immutable human authorization record (`authorization.json` signed prior to launch) and held directly by the independent host-side acceptance verifier. A guest cannot substitute `PK_host`. Any mismatch between the host's actual key and the pre-authorized key fails closed immediately. | Section 2.3 (§2.3.2), Section 2.4 (§2.4), Section 2.11 (§2.11) | Requires maintainer or CI runner to pin the host authority public key in the pre-launch campaign manifest. |
| **Defect 3** | Reconciled sleep-detection equations with uniform units, thresholds, and dispositions. | **RESOLVED:** Standardized all sleep detection across §§2.3, 2.4, 2.8, 3, 4, 8 to the exact baseline-adjusted difference formula: $\Delta_{sleep}(t) = [T_{inclusive}(t) - T_{exclusive}(t)] - [T_{inclusive}(0) - T_{exclusive}(0)] \ge 100,000,000\text{ ns}$ (100 ms). Units are consistently nanoseconds; threshold is strictly 100 ms; disposition is uniformly Status 71 (`INCONCLUSIVE_RESOURCE`). | Section 2.3 (§2.3.1), Section 2.4 (§2.4), Section 2.8 (§2.8), Section 3 (§3) | Clock resolution depends on OS kernel; nanosecond timestamps reflect timer API precision. |
| **Defect 4** | Independent final acceptance mechanism detecting missing or conflicting terminal receipts without relying exclusively on guest verifier. | **RESOLVED:** Specified an independent external host-side acceptance verifier (`host_acceptance_verifier.py`) that executes in the host environment after guest termination. It independently inspects both the guest evidence directory (`evidence.sha256`) and `host-terminal-receipt.json`, verifying signatures against trusted `PK_host`, checking deadlines, and detecting missing, partial, or conflicting receipts. | Section 2.11 (§2.11), Section 7 (§7) | Requires Python 3 with `ed25519` or `cryptography` library installed in the host verification environment. |

---

## 2. Required Technical Analysis and Specification

### 2.1 Item 1: Exact §8 Process Restrictions Prohibiting the H5 Watchdog

Section 8 ("Resource feasibility, risks and next gate") of accepted protocol ARCH-002-R1 defines the runtime process tree with strict exclusivity:

1. **Exhaustive Role Enumeration:**
   > "Permitted runtime tree is exactly:
   > - L: one always-active campaign owner/accountant, holding the single monotonic deadline, sealed expected identity, resource records and verifier exit checks.
   > - V: at most one independently implemented supervisor/verifier at a time...
   > - W: at most one worker process at a time, with one computation thread... No concurrent oracle/candidate workers, child pools, nested workers or per-case subprocesses."

2. **Prohibition of Extra Monitoring Processes:**
   > "External metadata utilities run only with V/W absent, within the same tree/caps; live monitoring uses L's in-process OS interfaces. No extra monitoring process or unaccounted helper is permitted while the runtime tree is populated."

3. **In-Process Clock Monopoly and the H5 Qualification Block:**
   Under R1 §8, the campaign clock is held exclusively by L in userspace via `time.monotonic_ns()`. When L executes within a container or virtualized guest, guest monotonic clocks are vulnerable to host scheduling pauses, vCPU descheduling, VM suspension, or hypervisor time dilation. During a guest pause or freeze, L cannot sample its clock or observe deadline expiration. However, running a watchdog process outside L on the host system directly violates the literal R1 prohibition: *"No extra monitoring process or unaccounted helper is permitted while the runtime tree is populated."* Qualification H5 correctly found: `HOST_CLOCK_ENFORCEMENT_BLOCKED`.

---

### 2.2 Item 2: Bounded Exception for Host-Side Deadline Authority (Role H)

To eliminate the vulnerability where a frozen guest fails to observe its own deadline breach, a bounded exception is established for **Role H (Host Deadline Authority / Watchdog)**.

**Scientific Justification:**
The intent of the §8 process restriction was to prohibit unobserved computational concurrency, hidden worker threads, or unmetered background tasks that could assist the candidate comparator or corrupt accounting. A host watchdog that:
1. Has zero visibility into candidate or oracle data buffers,
2. Has zero interprocess communication with candidate or verifier algorithms,
3. Executes strictly outside the guest VM/container boundary on the physical host,
4. Possesses purely monotonic termination capability (abort authority),
preserves 100% of the isolation and concurrency guarantees intended by §8 while closing the catastrophic blind spot of guest clock freeze.

**Role H Boundary Rules:**
- **Instance Bound:** Exactly one Role H process per campaign, launched on the host prior to guest campaign inception.
- **Authority Bound:** Role H has write access only to its host-side execution ledger and host-side termination signals (SIGTERM/SIGKILL to the guest VM/container process group). It has **zero read or write access** to guest memory, candidate IPC, or verification pipes.
- **Unidirectional Enforcement:** Role H can only abort the campaign; it cannot extend, resume, pause, or alter campaign parameters.

---

### 2.3 Item 3: Suspend-Aware Clocks, Cryptographic Authentication, and Terminal Handshake

#### 2.3.1 Suspend-Aware Platform Clocks and Unified Sleep Detection Formula
A standard monotonic clock (`CLOCK_MONOTONIC` or `CLOCK_MONOTONIC_RAW` on Linux) pauses when the system enters ACPI S3 (suspend to RAM) or S4 (hibernate). If a host sleeps for 2 hours, `CLOCK_MONOTONIC_RAW` advances by 0 seconds, allowing an overrun campaign to appear compliant.

To guarantee suspend-awareness, Role H must sample dual clocks on each tick:
- **Linux:**
  - $T_{inclusive} = \text{clock\_gettime}(\text{CLOCK\_BOOTTIME})$: Monotonic time that **includes** time spent in system suspension.
  - $T_{exclusive} = \text{clock\_gettime}(\text{CLOCK\_MONOTONIC\_RAW})$: Monotonic time that **excludes** suspension.
- **macOS (Darwin):**
  - $T_{inclusive} = \text{mach\_continuous\_time}()$: Monotonic time that **includes** system sleep time.
  - $T_{exclusive} = \text{mach\_absolute\_time}()$: Monotonic time that **excludes** system sleep time.

**Unified Baseline-Adjusted Sleep Detection Formula:**
At campaign launch, Role H records initial baseline timestamps: $T_{inclusive}(0)$ and $T_{exclusive}(0)$ in nanoseconds. At any subsequent sampling point $t$, Role H evaluates:
$$\Delta_{sleep}(t) = [T_{inclusive}(t) - T_{exclusive}(t)] - [T_{inclusive}(0) - T_{exclusive}(0)]$$

- **Threshold:** If $\Delta_{sleep}(t) \ge 100,000,000\text{ ns}$ ($100\text{ ms}$), host sleep or deep suspension has occurred.
- **Immediate Action:** Role H sends SIGKILL to the guest container/VM process group.
- **Protocol Disposition:** Fails closed as `INCONCLUSIVE_RESOURCE` (Status 71); host qualification is invalidated; no success root may be formed.

Total campaign elapsed time is governed strictly by the suspend-inclusive clock:
$$\Delta T_{campaign} = T_{inclusive}(now) - T_{inclusive}(start) \le 1,800 \times 10^9\text{ ns}\quad (1,800.0\text{ seconds})$$

#### 2.3.2 Independently Trusted Ed25519 Public-Key Identity
To prevent guest manipulation of public keys or receipt forgery:
1. **Host Asymmetric Keypair:** Prior to guest launch, Role H generates an ephemeral Ed25519 keypair: $(SK_{host}, PK_{host})$.
2. **Key Custody:** $SK_{host}$ remains strictly within Role H host process memory, with memory locked (`mlock`) and zeroed on process exit (`explicit_bzero`). It is never exposed to the guest.
3. **Independent Trust Anchor:** The public key $PK_{host}$ is committed directly to the pre-launch host authorization manifest (`host-authorization.json`) on the host filesystem and provided as an environment variable or read-only volume to the guest.
4. **Independent Verifier Verification:** The external host-side acceptance verifier loads $PK_{host}$ directly from `host-authorization.json` (not from the guest directory) when validating receipts.

#### 2.3.3 Terminal Handshake and Independently Authenticated Terminal Host Receipt
Circularity between guest evidence sealing and host deadline certification is resolved by a strictly ordered Handshake Protocol:

```
Guest (L / V)                                                 Host Watchdog (Role H)
    |                                                                   |
    |  [Phase 1: Candidate Runs, Oracles, Guards Complete]             |
    |------------------------------------------------------------------>|  (Heartbeat pulses: seq=k)
    |                                                                   |
    |  1. Guest writes draft manifest, finishes observed verifications. |
    |  2. Guest computes final `evidence.sha256` root digest.           |
    |  3. Guest emits Sealing-Complete signal to Host via IPC pipe:     |
    |     { campaign_uuid, root_sha256, guest_elapsed_ns }             |
    |------------------------------------------------------------------>|
    |                                                                   |  [Host validates:
    |                                                                   |   - T_inclusive <= 1,800s,
    |                                                                   |   - Delta_sleep < 100 ms,
    |                                                                   |   - AC power maintained,
    |                                                                   |   - heartbeats unbroken]
    |                                                                   |
    |                                                                   |  4. Host creates & signs
    |                                                                   |     `host-terminal-receipt.json`
    |                                                                   |     binding `root_sha256`.
    |                                                                   |  5. Host writes receipt to host
    |                                                                   |     evidence destination.
    |  6. Host returns Terminal-Ack to guest.                           |
    |<------------------------------------------------------------------|
    |                                                                   |
    |  7. Guest performs clean shutdown and exits 0.                    |
    |------------------------------------------------------------------>|
    |                                                                   |  8. Host reaps guest; closes
    |                                                                   |     execution session.
    v                                                                   v
```

**Contents of `host-terminal-receipt.json`:**
```json
{
  "protocol": "T81-ARCH-003-R2",
  "campaign_uuid": "...",
  "host_start_boot_ns": 1234567890000000,
  "host_end_boot_ns": 1234747890000000,
  "host_elapsed_ns": 180000000000,
  "max_sleep_divergence_ns": 1420000,
  "power_source": "AC_MAINS",
  "guest_sealed_root_sha256": "...",
  "heartbeats_received": 180,
  "termination_issued": false,
  "watchdog_peak_rss_kib": 4096,
  "host_public_key_hex": "...",
  "host_signature_hex": "..."
}
```

The terminal host receipt is created strictly **after** the guest has sealed its evidence root and transmitted the root digest. It binds the exact `root_sha256`. Both the guest evidence archive and `host-terminal-receipt.json` are required for final campaign acceptance.

---

### 2.4 Item 4: Fail-Closed Semantics for All Boundary Faults

| Fault Condition | Exact Detection Rule | Immediate Action | Final Protocol Status |
|---|---|---|---|
| **Deadline Breach** | $T_{inclusive}(t) - T_{inclusive}(0) > 1,800 \times 10^9\text{ ns}$ | Role H sends `SIGKILL` to guest VM/container process group | `INCONCLUSIVE_RESOURCE` (Status 71); retain completed prefix; success root forbidden. |
| **Host Sleep / Suspension** | $\Delta_{sleep}(t) \ge 100,000,000\text{ ns}$ | Role H sends `SIGKILL` to guest VM/container process group | `INCONCLUSIVE_RESOURCE` (Status 71); host qualification invalidated. |
| **Watchdog Loss / Crash** | Guest detects closed IPC pipe or heartbeat ACK failure | Guest L aborts finalization, deletes partial `evidence.sha256` | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |
| **Guest Freeze / Pause** | No heartbeat received for $> 15 \times 10^9\text{ ns}$ | Role H sends `SIGKILL` to guest process group | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70); campaign marked unverified. |
| **AC Power Drop** | `pmset` / sysfs reports battery power | Role H sends `SIGKILL` to guest process group | `INCONCLUSIVE_RESOURCE` (Status 71); battery run rejected. |
| **Terminal Receipt Tampering** | Ed25519 signature fails verification against $PK_{host}$ | Host acceptance verifier rejects campaign | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |
| **Digest Mismatch** | `guest_sealed_root_sha256` $\ne$ `sha256(evidence.sha256)` | Host acceptance verifier rejects campaign | `FAIL_CORRECTNESS` (Status 65). |
| **Missing Terminal Receipt** | Guest directory present but no valid terminal receipt exists | Host acceptance verifier rejects campaign | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |

---

### 2.5 Item 5: Host-Side Monitoring Boundary vs. 512 MiB Aggregate RSS Limit

1. **Host/Guest Accounting Separation:**
   - The 512 MiB aggregate absolute resident memory cap applies strictly to the **guest study process tree** (Owner L + Supervisor V + Worker W + compilation tools + maintenance threads).
   - Role H executes in the **host management domain** (outside the VM/container cgroup/namespace). It does not allocate memory from the guest memory pool and cannot page-share with candidate workers.
2. **Host Watchdog Resource Ceiling:**
   - Role H is implemented as a minimal standalone C binary (`arch003_watchdog`) with zero dynamic runtime dependencies.
   - Resident memory ceiling: $\text{RSS}_{H} \le 16\text{ MiB}$ ($16,777,216$ bytes).
   - Role H samples its own RSS via `getrusage(RUSAGE_SELF)` and includes `watchdog_peak_rss_kib` in `host-terminal-receipt.json`. If $\text{RSS}_{H} > 16\text{ MiB}$, the watchdog receipt is invalid, causing fail-closed rejection.

---

### 2.6 Item 6: Inclusive 1,800-Second Campaign Envelope

The 1,800-second ($30.0$ minute) wall-clock deadline is absolute, monotonic, suspend-inclusive, and non-resettable. It encompasses the entirety of the campaign execution pipeline:

$$\sum T = T_{configure} + T_{compile} + T_{link} + T_{oracles} + T_{guards} + T_{probes} + T_{profiles(D,R,S)} + T_{verify} + T_{manifest} + T_{hash\_seal} + T_{terminal\_ack} \le 1,800\text{ seconds}$$

- **No reset** on profile boundary ($D \to R \to S$).
- **No reset** on worker transitions ($W_i \to W_{i+1}$).
- **No exclusion** of file system I/O, artifact writing, SHA-256 tree hashing, or terminal receipt signing.
- Role H starts its timer before initiating guest setup and triggers SIGKILL the instant $T_{inclusive\_elapsed} > 1,800 \times 10^9\text{ ns}$.

---

### 2.7 Item 7: macOS Battery Operation and Sleep Invalidation

1. **Defect in `caffeinate -s` on Battery Power:**
   Under macOS Darwin IOPM, `caffeinate -s` creates a userspace power assertion (`NoIdleSleepAssertion`) preventing *system idle sleep*. However, when running on DC battery power, macOS kernel power governors enforce *Emergency Sleep*, *Thermal Sleep*, and *Low Battery Sleep* regardless of userspace assertions. Furthermore, Apple Silicon power management performs aggressive background core frequency scaling, throttling, and vCPU descheduling when on battery, destroying timing determinism.
2. **Normative R2 Requirements for macOS:**
   - **AC Power Mandatory:** Execution on DC battery power is strictly prohibited.
   - **Pre-Flight Power Check:** Role H must verify AC connection via `pmset -g batt` before launching guest L. If output contains `Battery Power` or lacks `'AC Power'`, Role H must fail closed immediately: `BLOCK_HOST_POWER_QUALIFICATION`.
   - **Assertion Type:** On macOS, the host must use `caffeinate -d -i -m -s` (display, idle, system, and disk assertion) wrapped in a hardware AC power gate.
   - **Continuous Power Sampling:** Role H polls `IOPMPowerSource` every 5 seconds. Any transition to battery power immediately aborts the campaign with `INCONCLUSIVE_RESOURCE`.

---

### 2.8 Item 8: Independent Acceptance Tests and Real Suspension Negative Controls

To validate the Role H watchdog mechanism before scientific authorization, five negative control fixtures must pass:

1. **Test N1 — Injected Guest Deadlock / Overrun:**
   - Guest W enters an infinite sleep loop.
   - *Expected Result:* Role H fires at exactly $T_{start} + 1,800\text{ s}$ (or fixture timeout $\tau = 5\text{ s}$), sends SIGKILL to guest tree, writes signed `host-watchdog-termination.json`. Campaign fails closed with Status 71 (`INCONCLUSIVE_RESOURCE`); no success root created.
2. **Test N2 — Heartbeat Silence (Guest Pause / Hypervisor Deschedule):**
   - Guest artificially suspends heartbeat pulse emission for $>15\text{ s}$.
   - *Expected Result:* Role H detects heartbeat timeout, reaps guest, logs timeout. Campaign fails closed with Status 70 (`BLOCKED_RESOURCE_ACCOUNTING`).
3. **Test N3 — Real Host Suspension / Sleep Fixture:**
   - On Linux host, fixture invokes `rtcwake -m mem -s 3` to put the physical host into ACPI S3 sleep for 3 seconds while a bounded task runs.
   - On macOS host, fixture programmatically disables power assertion and signals sleep via IOPM.
   - *Expected Result:* Upon wake, Role H detects $\Delta_{sleep} \ge 3,000,000,000\text{ ns} > 100,000,000\text{ ns}$, immediately aborts the campaign, and logs `HOST_SUSPENSION_DETECTED`. Campaign fails closed with Status 71 (`INCONCLUSIVE_RESOURCE`).
4. **Test N4 — Tampered Terminal Receipt and Signature:**
   - Guest finishes correctly, but `host-terminal-receipt.json` has a tampered byte or an invalid Ed25519 signature.
   - *Expected Result:* Host acceptance verifier fails signature verification, exits with code 1; acceptance is rejected with Status 70.
5. **Test N5 — DC Battery Disconnect:**
   - Power state simulates disconnection of AC adapter mid-run.
   - *Expected Result:* Role H detects power drop, terminates run, records `BATTERY_POWER_DETECTED`.

---

### 2.9 Item 9: Preservation of Scientific Workloads and 100 MiB Evidence Envelope

The R2 amendment is strictly an operational governance and host-authority amendment. It preserves 100% of the underlying scientific experiment:

- **Mathematical API Contract:** Inputs are immutable $\{-1, 0, +1\}$ arrays; operation is $C = \sum [a_i = b_i]$; operand-local format tags and strict error precedence unchanged (§2).
- **Candidate Representations:** Exact five representations (BYTE, C2, PLANES, PT5, NATIVE) preserved (§3).
- **Frozen Schedule and Cardinality:** Exactly 12 families, 408,034 expected requests, 483,553 calls per profile, 1,450,659 total calls across D, R, and S profiles (§4).
- **Oracle Independence:** Dual independent oracles (Scalar Oracle and Histogram Oracle) compile in separate translation units with zero candidate linkage; both streams must agree before candidate execution (§4).
- **Output Budget Reconciled with 100 MiB Cap:**
  - Numeric streams: $\le 84\text{ MiB}$ (normal 82,328,144 bytes + guard 392 bytes).
  - Commands / Host receipts: $\le 4\text{ MiB}$ (`host-terminal-receipt.json` consumes $<4\text{ KiB}$).
  - Diagnostics / Assembly: $\le 8\text{ MiB}$.
  - Manifests / Source diffs: $\le 2\text{ MiB}$.
  - Headroom: $\ge 2\text{ MiB}$.
  - Total retained output: $\le 100\text{ MiB}$ (§8).
- **Representation Comparison:** Strongest matched controls (B-BYTE, B-PLANES, B-C2, B-PT5) and statistical decision rules (§5, §7) remain completely unchanged.

---

### 2.10 Item 10: Revision Lineage, Verifier Revision, and Separate Acceptance Gate

- **Implementation Revision (ARCH-003-I3):** The accepted I2 implementation (`f408f6f`) only supports L-internal Linux tracing and lacks Role H watchdog handshake hooks. A new implementation revision **ARCH-003-I3** is required to support R2.
- **Verifier Revision:** `verifier.py` (Supervisor V) is updated to support the sealing handshake.
- **Separate Acceptance Gate:** Protocol R2 adoption is a prerequisite for, but does **not** grant, implementation authorization. A separate maintainer gate must approve R2, followed by a separate commit-scoped authorization for I3 implementation, host qualification, and finally campaign execution.

---

### 2.11 Item 11: Independent Final Acceptance Mechanism (Host Acceptance Verifier)

The guest supervisor `verifier.py` cannot be the sole acceptance judge of external host authority compliance. An independent host-side acceptance verifier (`experiments/arch003/host_acceptance_verifier.py`) is specified to run on the host after guest termination:

**Independent Acceptance Algorithm:**
1. **Host Public Key Ingestion:** Ingest $PK_{host}$ from the immutable host authorization record (`host-authorization.json`), strictly ignoring any public key found in the guest directory.
2. **Terminal Receipt Verification:**
   - Locate `host-terminal-receipt.json` on the host evidence path.
   - Verify the Ed25519 signature over the canonical JSON of `host-terminal-receipt.json` using $PK_{host}$.
   - Verify `termination_issued == false`.
   - Verify `host_elapsed_ns <= 1800 * 10^9`.
   - Verify `max_sleep_divergence_ns < 100_000_000`.
   - Verify `power_source == "AC_MAINS"`.
3. **Guest Root Binding Verification:**
   - Ingest `evidence.sha256` from the guest evidence directory.
   - Compute $H_{root} = \text{SHA256}(\text{read\_bytes}(\text{evidence.sha256}))$.
   - Assert $H_{root} == \text{guest\_sealed\_root\_sha256}$ from `host-terminal-receipt.json`.
4. **Guest Manifest Verification:**
   - Ingest `manifest.json` from the guest directory.
   - Assert `manifest.status == "PASS_CORRECTNESS_CORE"`.
5. **Acceptance Verdict:**
   - If all checks pass: Emit `ACCEPTANCE_CONFIRMED: ARCH-003-R2 CAMPAIGN VALID`. Exit 0.
   - If any check fails, receipt is missing, or digests disagree: Emit `ACCEPTANCE_REJECTED: <REASON>`. Exit 1.

---

## 3. Proposed R2 Protocol Amendment Text

The following text is proposed to amend Sections 6 and 8 of `docs/research/arch002/PROTOCOL.md`:

```markdown
### Proposed Amendment to Section 6 (Host Qualification)

Add the following normative power and timer requirements:
- "AC mains power is mandatory. Execution on DC battery power is prohibited. On macOS
  and Linux hosts, an automated pre-flight probe must verify connection to AC
  power prior to campaign launch (e.g. `pmset -g batt` confirming 'AC Power'
  or `/sys/class/power_supply/` status 'Charging'/'Full'). Disabling sleep via
  `caffeinate -s` on macOS is insufficient on battery power due to hard-coded
  kernel thermal and battery sleep assertions. Any mid-campaign transition to
  battery power aborts the run as INCONCLUSIVE_RESOURCE.
- Host timer must record dual monotonic clocks: one suspend-inclusive
  (`CLOCK_BOOTTIME` on Linux, `mach_continuous_time` on Darwin) and one
  suspend-exclusive (`CLOCK_MONOTONIC_RAW` on Linux, `mach_absolute_time` on Darwin).
  Any baseline-adjusted divergence between these clocks exceeding 100,000,000 ns
  (100 ms) indicates host suspension or deep sleep and invalidates the campaign
  as INCONCLUSIVE_RESOURCE."

### Proposed Amendment to Section 8 (Resource Feasibility, Risks and Process Hierarchy)

Replace the process hierarchy description with the following text:
"Permitted runtime tree is exactly:

- H: exactly one external host deadline authority (watchdog) executing on the
  physical host outside the guest VM/container boundary. H enforces the absolute
  1,800-second monotonic, suspend-inclusive deadline (`CLOCK_BOOTTIME` /
  `mach_continuous_time`) independently of guest clock stability, polls AC power
  status, monitors guest heartbeats, and possesses purely monotonic SIGTERM/SIGKILL
  abort authority over the guest container/VM process group. H holds an ephemeral
  Ed25519 private key, executes a Terminal Handshake with the guest after evidence
  sealing, issues a signed terminal host receipt binding the guest evidence root
  digest, has zero access to candidate or oracle data, and consumes <=16 MiB host RSS.
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
time, and <=100 MiB retained output ceiling. Final acceptance requires both a
valid sealed guest evidence bundle and an independently verified terminal host
receipt signed by H."
```

---

## 4. R1-to-R2 Change Matrix

| Section / Topic | Accepted R1 Protocol (`1454bd6`) | Proposed R2 Amendment | Scientific & Governance Rationale |
|---|---|---|---|
| **Process Roles (§8)** | Exactly L, V, W. "No extra monitoring process or unaccounted helper is permitted." | Adds external **Role H** on the host. Bounded exception to permit independent watchdog. | Resolves H5 qualification block; prevents unobserved guest freeze from masking deadline breach. |
| **Deadline Authority (§8)** | Solely inside guest L via `time.monotonic_ns()`. | Dual authority: guest L checks internally; host H enforces externally via suspend-aware timer. | Guarantees fail-closed termination even if guest kernel or hypervisor hangs. |
| **Clock Source (§6, §8)** | Monotonic userspace clock. | Dual suspend-aware clocks (`CLOCK_BOOTTIME` / `mach_continuous_time`) and sleep detection ($\Delta_{sleep} \ge 100\text{ ms}$). | Prevents host S3/S4 sleep from hiding campaign elapsed time overruns. |
| **Receipt Authentication (§8)** | Internal JSON files; no signature or trust root. | Ephemeral Ed25519 host keypair; host signs terminal receipt; host verifier checks against pre-authorized key. | Real cryptographic authentication prevents forged or retrofitted watchdog receipts. |
| **Finalization Flow (§8)** | Unsequenced single-pass sealing. | Terminal Handshake: guest seals `evidence.sha256`, host creates signed `host-terminal-receipt.json` binding the root. | Resolves circularity; ensures host deadline covers full evidence sealing. |
| **Acceptance Verification (§8)** | Relies solely on guest `verifier.py`. | Dual acceptance: guest verifier + independent external `host_acceptance_verifier.py`. | Prevents compromised guest from self-certifying host compliance. |
| **Watchdog RSS (§8)** | No external watchdog permitted; all memory in live tree $\le 512\text{ MiB}$. | Guest tree remains $\le 512\text{ MiB}$. Role H on host capped at $\le 16\text{ MiB}$ host RSS. | Preserves candidate memory pressure constraints without charging host hypervisor tools to guest. |
| **macOS Power (§6)** | "AC power... sleep disabled for the bounded run." | Explicit check for AC mains; DC battery prohibited; `caffeinate` alone recognized as defective on battery. | Prevents kernel thermal sleep and clock distortion on Apple Silicon battery power. |
| **Scientific Scope (§1–5, 7)** | 12 families, 408k requests, 5 encodings, dual oracles, statistical criteria. | **Identical (Unchanged).** | 100% preservation of scientific workload, controls, and falsification rules. |

---

## 5. New Acceptance Criteria for Protocol R2

Before R2 can be declared accepted and ready for implementation authorization:

1. **CRIT-R2-01 (Formal Specification Completeness):** The proposal must provide exact, unambiguous text replacements for §6 and §8 of `PROTOCOL.md` without leaving placeholders.
2. **CRIT-R2-02 (Zero Workload Mutation):** All 12 schedule families, 408,034 expected requests, 483,553 calls per profile, 5 representation formats, and dual oracle separation must be byte-level and logically identical to R1.
3. **CRIT-R2-03 (Host Clock Isolation):** Role H must be proven mathematically and architecturally incapable of injecting data, modifying execution parameters, or communicating with candidate algorithms.
4. **CRIT-R2-04 (Negative Control Specification):** Complete specifications for negative tests N1–N5 (including real suspension N3) must be provided with deterministic fail-closed exit codes.
5. **CRIT-R2-05 (Host Power Verification):** Explicit commands and exit contracts for AC mains verification on Darwin and Linux must be specified.
6. **CRIT-R2-06 (Cryptographic Non-Repudiation):** Verification of Role H receipts must rely on Ed25519 signature checks against a pre-authorized host public key.
7. **CRIT-R2-07 (Dual Acceptance Artifacts):** Final acceptance must strictly require both valid guest artifacts and a matching, signed terminal host receipt.

---

## 6. Compatibility and Provenance Analysis

- **Backward Compatibility with R1:**
  R2 is strictly an operational refinement of the process monitoring location restriction in §8. Any valid R1 execution that was completely observable and completed within 1,800 seconds remains valid under R2 when monitored by Role H.
- **Provenance and Lineage:**
  - Base protocol: PR #714 commit `1454bd63426d21686cfb5ef22dbaebc3c9cae94b`.
  - Baseline implementation: PR #716 commit `f408f6fb27bd7516fb0b32f3645f9d53fbbd4b15`.
  - Triggering qualification finding: H5 — `HOST_CLOCK_ENFORCEMENT_BLOCKED`.
  - The amendment maintains unbroken audit provenance, citing exact commits and preserving historical failure records in `experiments/arch003/validation/`.

---

## 7. Explicit List of Implementation Changes Required (ARCH-003-I3)

The following changes will be required in the follow-on implementation revision (ARCH-003-I3):

1. **Host Watchdog Native Binary (`experiments/arch003/host_watchdog.c`):**
   - Implement Role H watchdog in C without external runtime dependencies.
   - Generate ephemeral Ed25519 keypair; export `host_authority.pub`.
   - Sample dual clocks (`CLOCK_BOOTTIME` and `CLOCK_MONOTONIC_RAW` on Linux; `mach_continuous_time` on Darwin).
   - Evaluate $\Delta_{sleep}(t)$; abort if $\ge 100,000,000\text{ ns}$.
   - Query AC power state (`pmset -g batt` / `/sys/class/power_supply`).
   - Monitor heartbeat pulses; enforce 1,800-second hard deadline via POSIX signals.
   - Execute Terminal Handshake: receive `root_sha256`, generate and sign `host-terminal-receipt.json`.
2. **Guest Campaign Handshake (`experiments/arch003/campaign.py`):**
   - Emit periodic heartbeat pulses during execution.
   - Transmit `root_sha256` after evidence sealing; await terminal ACK before exiting.
3. **Independent Host Acceptance Verifier (`experiments/arch003/host_acceptance_verifier.py`):**
   - Ingest trusted $PK_{host}$ from host authorization record.
   - Verify signature and fields of `host-terminal-receipt.json`.
   - Validate digest match against guest `evidence.sha256`.
4. **Negative Test Fixtures (`experiments/arch003/tests/test_watchdog.py`):**
   - Implement test suite verifying N1 (deadlock kill), N2 (heartbeat timeout), N3 (real host suspension / sleep jump), N4 (tampered receipt/signature), and N5 (battery disconnect).

---

## 8. Independent Reviewer Checklist

Independent maintainers and reviewers should use the following checklist to evaluate this proposal:

- [ ] **R2-CHK-01 (Scope Limitation):** Does the amendment strictly restrict Role H to host deadline and power monitoring without granting compute or candidate IPC roles?
- [ ] **R2-CHK-02 (Suspend-Aware Clocks):** Does the amendment mandate suspend-inclusive clocks (`CLOCK_BOOTTIME` / `mach_continuous_time`) and evaluate the baseline-adjusted sleep formula?
- [ ] **R2-CHK-03 (Terminal Host Receipt):** Is the terminal host receipt created after guest evidence sealing, binding the exact `evidence.sha256` digest?
- [ ] **R2-CHK-04 (Independent Key Identity):** Is $PK_{host}$ anchored in pre-launch authorization rather than trusting guest-controlled files?
- [ ] **R2-CHK-05 (Dual Acceptance Verification):** Does final acceptance require verification by an external host verifier without relying solely on the guest?
- [ ] **R2-CHK-06 (Memory Boundary):** Is Role H explicitly isolated from the guest 512 MiB RSS limit, and is Role H capped at $\le 16\text{ MiB}$ host RSS?
- [ ] **R2-CHK-07 (Inclusive 1,800s Deadline):** Does the 1,800-second deadline unequivocally encompass configuration, compilation, linking, all D/R/S profiles, oracles, guards, probes, evidence sealing, and terminal receipt creation without resets?
- [ ] **R2-CHK-08 (macOS Battery Hardening):** Does the proposal explicitly reject `caffeinate -s` on battery power and mandate automated AC power verification?
- [ ] **R2-CHK-09 (Fail-Closed Guarantees):** Are deadline breaches, watchdog terminations, heartbeat losses, and receipt corruptions guaranteed to fail closed with no success root?
- [ ] **R2-CHK-10 (Scientific Preservation):** Are all 12 schedule families, 408,034 expected requests, 483,553 calls/profile, representations, and oracle independence strictly preserved?
- [ ] **R2-CHK-11 (Non-Activation Integrity):** Does the proposal refrain from claiming that implementation I2 is compliant or executing campaigns prematurely?

---

*End of Proposal. Awaiting formal maintainer review.*
