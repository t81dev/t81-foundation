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

## 1. Codex Review Findings Disposition Summary

This revision directly resolves all three P1 findings identified during initial Codex review of PR #718 (commit `ee98163e6c621f80509717be4c9cbdd87fbeb1bb`):

| Finding | Codex Issue Summary | Proposal Disposition | Section & Line References | Unresolved Limitations |
|---|---|---|---|---|
| **Finding 1** | Suspend-insensitive clock assumptions (`CLOCK_MONOTONIC_RAW` stops during sleep on Linux); artificial clock jumps do not validate real kernel suspension behavior. | **RESOLVED:** Mandated dual-clock accounting using platform-specific suspend-aware clocks (`CLOCK_BOOTTIME` on Linux, `mach_continuous_time` on Darwin) alongside monotonic elapsed clocks. Added explicit host sleep detection via clock divergence ($\Delta \ge 100\text{ ms}$). Defined real suspension test fixture N3 using Linux kernel suspend/wake (`rtcwake -m mem`) or Darwin IOPM assertions. | Section 2.3 (§2.3), Section 2.4 (§2.4), Section 2.8 (§2.8, Test N3) | Testing requires elevated privileges (`CAP_SYS_ADMIN` or root for `rtcwake`) in dedicated physical CI; mock fallback allowed only for component tests. |
| **Finding 2** | Finalization circularity: guest cannot emit a sealed evidence root containing a receipt that certifies finalization success before finalization finishes. | **RESOLVED:** Replaced single-pass sealing with a strictly ordered Two-Phase Finalization and Handshake Protocol (Commit-Request $\to$ Seal $\to$ Host Completion Ack $\to$ Terminal Sealing $\to$ Verifier Closure). Host deadline explicitly covers all phases including verification, manifest writing, hashing, evidence sealing, and final host completion acknowledgment. | Section 2.3 (§2.3.3), Section 2.4 (§2.4), Section 2.6 (§2.6) | If guest dies during the final 2-byte ack commit, host retains termination record; campaign fails closed without false success. |
| **Finding 3** | Nonce and SHA-256 alone do not authenticate receipts across hostile or unprivileged boundaries; key custody, trust roots, and tamper controls were missing. | **RESOLVED:** Established asymmetric Ed25519 digital signature trust root with host-retained private key custody (or kernel-enforced out-of-band Unix domain socket IPC with SO_PEERCRED descriptor passing). Defined explicit public key distribution to guest verifier, strict signature verification in `verifier.py`, and fail-closed rejection on invalid signature or channel corruption. | Section 2.3 (§2.3.1, §2.3.4), Section 2.4 (§2.4), Section 7 (§7) | Host key custody requires secure host filesystem permissions (`0400` root-owned); key generation must occur fresh per campaign. |

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

### 2.3 Item 3: Suspend-Aware Clocks, Cryptographic Authentication, and Two-Phase Sealing

#### 2.3.1 Suspend-Aware Platform Clocks vs. Monotonic Clocks
A standard monotonic clock (`CLOCK_MONOTONIC` or `CLOCK_MONOTONIC_RAW` on Linux) pauses when the system enters ACPI S3 (suspend to RAM) or S4 (hibernate). If a host sleeps for 2 hours, `CLOCK_MONOTONIC_RAW` advances by 0 seconds, allowing an overrun campaign to appear compliant.

To guarantee suspend-awareness, Role H must sample dual clocks on each tick:
- **Linux:**
  - $T_{boot} = \text{clock\_gettime}(\text{CLOCK\_BOOTTIME})$: Monotonic time that **includes** time spent in system suspension.
  - $T_{mono} = \text{clock\_gettime}(\text{CLOCK\_MONOTONIC\_RAW})$: Monotonic time that **excludes** suspension.
  - **Host Sleep Detection Rule:** If $(T_{boot} - T_{mono}) - (T_{boot, 0} - T_{mono, 0}) \ge 100\text{ ms}$, host sleep/suspension occurred.
- **macOS (Darwin):**
  - $T_{cont} = \text{mach\_continuous\_time}()$: Monotonic time that **includes** system sleep time.
  - $T_{abs} = \text{mach\_absolute\_time}()$: Monotonic time that **excludes** system sleep time.
  - **Host Sleep Detection Rule:** If $(T_{cont} - T_{abs}) - (T_{cont, 0} - T_{abs, 0}) \ge 100\text{ ms}$, host sleep/suspension occurred.

Elapsed campaign time is governed strictly by the suspend-inclusive clock:
$$\Delta T_{campaign} = T_{suspend\_inclusive}(now) - T_{suspend\_inclusive}(start) \le 1,800.0\text{ seconds}$$

#### 2.3.2 Cryptographic Trust Root and Host Key Custody
To eliminate shared-nonce vulnerabilities:
1. **Host Asymmetric Keypair:** Prior to guest launch, Role H generates an ephemeral Ed25519 keypair: $(SK_{host}, PK_{host})$.
2. **Key Custody:** $SK_{host}$ remains strictly within Role H host process memory, with page memory locked (`mlock`) and zeroed on exit. It is **never** transmitted to the guest.
3. **Public Key Injection:** $PK_{host}$ (32 bytes raw) is written to a read-only configuration file mounted into the guest at `/etc/arch003/host_authority.pub`.
4. **Receipt Signature:** All receipts emitted by Role H (`host-watchdog-receipt.json`, `host-completion-ack.json`, `host-watchdog-termination.json`) are signed with $SK_{host}$ using Ed25519:
   $$\sigma = \text{Ed25519\_Sign}(SK_{host}, \text{Canonical\_JSON}(Receipt))$$

#### 2.3.3 Two-Phase Finalization and Handshake Protocol (Resolving Circularity)
A guest cannot include a host-signed completion receipt inside an evidence archive that must be hashed before the host knows whether hashing succeeded. Circularity is resolved by a two-phase commit:

```
Guest (L/V)                                                Host Watchdog (Role H)
    |                                                                |
    |  [Phase 1: Candidate D/R/S, Oracles, Guards Complete]         |
    |--------------------------------------------------------------->|  (Heartbeat pulses: seq=k)
    |                                                                |
    |  1. Assemble draft manifest: `manifest.draft.json`            |
    |  2. Compute SHA-256 of all raw artifacts (observed, etc.)      |
    |  3. Emit Commit-Request: `finalize-request.json`              |
    |     { campaign_uuid, draft_manifest_sha256, elapsed_guest_ns }|
    |--------------------------------------------------------------->|
    |                                                                |  [Host validates:
    |                                                                |   T_boot <= 1,800s,
    |                                                                |   heartbeats continuous,
    |                                                                |   no sleep/power drop]
    |                                                                |
    |  4. Role H signs and returns `host-completion-ack.json`        |
    |     { campaign_uuid, ack_seq, host_boot_ns, host_sig }        |
    |<---------------------------------------------------------------|
    |                                                                |
    |  [Phase 2: Final Evidence Root Sealing]                        |
    |  5. Guest writes `manifest.json` embedding the exact           |
    |     `host-completion-ack.json` contents & signature.           |
    |  6. Guest computes final `evidence.sha256` root.               |
    |  7. Verifier V executes final verification over root & ack:    |
    |     - Verifies Ed25519 signature of ack using PK_host.        |
    |     - Confirms all 100 MiB output caps and byte digests.       |
    |  8. Emits final 2-byte completion signal to host: `0x54 0x38`   |
    |--------------------------------------------------------------->|
    |                                                                |  [Host logs terminal sealed receipt:
    |                                                                |   `host-watchdog-receipt.json`]
    v                                                                v
```

If the guest fails, crashes, or exceeds limits at any point before step 8, Role H fires SIGKILL and records `host-watchdog-termination.json`. No valid success receipt is ever left without full sealing and verification.

---

### 2.4 Item 4: Fail-Closed Semantics for All Boundary Faults

| Fault Condition | Detection Mechanism | Immediate Action | Final Protocol Status |
|---|---|---|---|
| **Deadline Breach** ($T_{boot} > 1,800\text{ s}$)| Role H suspend-aware timer | Role H sends `SIGKILL` to guest VM/container process group | `INCONCLUSIVE_RESOURCE` (Status 71); retain completed prefix; success root forbidden. |
| **Watchdog Loss / Crash** | Guest L detects closed IPC pipe or heartbeat acknowledgment failure | Guest L aborts finalization immediately, unlinks any partial `evidence.sha256` | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |
| **Guest Freeze / Deschedule** | Role H misses 3 consecutive heartbeats ($>15\text{ s}$ on $T_{boot}$) | Role H terminates guest | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70); campaign marked unverified. |
| **Host Sleep / Suspension** | Clock divergence: $(T_{boot} - T_{mono}) \ge 100\text{ ms}$ | Role H terminates guest immediately | `INCONCLUSIVE_RESOURCE` (Status 71); host qualification invalidated. |
| **Watchdog Receipt Tampering** | Verifier V fails Ed25519 signature check against `PK_host` | Verifier V exits with code 1; sealing is aborted | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |
| **Out-of-Order Receipt** | `host-completion-ack.json` present before Commit-Request | Verifier V rejects bundle as unsequenced | `FAIL_GOVERNANCE` (Status 67). |

---

### 2.5 Item 5: Host-Side Monitoring Boundary vs. 512 MiB Aggregate RSS Limit

1. **Host/Guest Accounting Separation:**
   - The 512 MiB aggregate absolute resident memory cap applies strictly to the **guest study process tree** (Owner L + Supervisor V + Worker W + compilation tools + maintenance threads).
   - Role H executes in the **host management domain** (outside the VM/container cgroup/namespace). It does not allocate memory from the guest memory pool and cannot page-share with candidate workers.
2. **Host Watchdog Resource Ceiling:**
   - Role H is implemented as a minimal standalone C binary (`arch003_watchdog`) with zero dynamic runtime dependencies.
   - Resident memory ceiling: $\text{RSS}_{H} \le 16\text{ MiB}$.
   - Role H samples its own RSS via `getrusage(RUSAGE_SELF)` and includes `watchdog_peak_rss_kib` in `host-watchdog-receipt.json`. If $\text{RSS}_{H} > 16\text{ MiB}$, the watchdog receipt is invalid, causing fail-closed rejection.

---

### 2.6 Item 6: Inclusive 1,800-Second Campaign Envelope

The 1,800-second ($30.0$ minute) wall-clock deadline is absolute, monotonic, suspend-inclusive, and non-resettable. It encompasses the entirety of the campaign execution pipeline:

$$\sum T = T_{configure} + T_{compile} + T_{link} + T_{oracles} + T_{guards} + T_{probes} + T_{profiles(D,R,S)} + T_{verify} + T_{manifest} + T_{hash\_seal} + T_{ack} \le 1,800\text{ seconds}$$

- **No reset** on profile boundary ($D \to R \to S$).
- **No reset** on worker transitions ($W_i \to W_{i+1}$).
- **No exclusion** of file system I/O, artifact writing, or SHA-256 tree hashing.
- Role H starts its timer before initiating guest setup and triggers SIGKILL the instant $T_{boot\_elapsed} > 1,800.0\text{ seconds}$.

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
   - *Expected Result:* Upon wake, Role H detects $(T_{boot} - T_{mono}) \ge 3,000\text{ ms} > 100\text{ ms}$, immediately aborts the campaign, and logs `HOST_SUSPENSION_DETECTED`. Campaign fails closed with Status 71 (`INCONCLUSIVE_RESOURCE`).
4. **Test N4 — Missing / Tampered Watchdog Receipt and Signature:**
   - Guest finishes correctly, but `host-completion-ack.json` has a tampered byte or an invalid Ed25519 signature.
   - *Expected Result:* Independent verifier V fails signature verification, exits with code 1; sealing is aborted with Status 70.
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
  - Commands / Host receipts: $\le 4\text{ MiB}$ (Role H receipts consume $<4\text{ KiB}$ total).
  - Diagnostics / Assembly: $\le 8\text{ MiB}$.
  - Manifests / Source diffs: $\le 2\text{ MiB}$.
  - Headroom: $\ge 2\text{ MiB}$.
  - Total retained output: $\le 100\text{ MiB}$ (§8).
- **Representation Comparison:** Strongest matched controls (B-BYTE, B-PLANES, B-C2, B-PT5) and statistical decision rules (§5, §7) remain completely unchanged.

---

### 2.10 Item 10: Revision Lineage, Verifier Revision, and Separate Acceptance Gate

- **Implementation Revision (ARCH-003-I3):** The accepted I2 implementation (`f408f6f`) only supports L-internal Linux tracing and lacks Role H watchdog handshake hooks. A new implementation revision **ARCH-003-I3** is required to support R2.
- **Verifier Revision:** `verifier.py` (Supervisor V) must be updated to inspect and validate `host-completion-ack.json` and its Ed25519 signature before validating normal records.
- **Separate Acceptance Gate:** Protocol R2 adoption is a prerequisite for, but does **not** grant, implementation authorization. A separate maintainer gate must approve R2, followed by a separate commit-scoped authorization for I3 implementation, host qualification, and finally campaign execution.

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
  Any divergence between these clocks exceeding 100 ms indicates host suspension
  or deep sleep and invalidates the campaign as INCONCLUSIVE_RESOURCE."

### Proposed Amendment to Section 8 (Resource Feasibility, Risks and Process Hierarchy)

Replace the process hierarchy description with the following text:
"Permitted runtime tree is exactly:

- H: exactly one external host deadline authority (watchdog) executing on the
  physical host outside the guest VM/container boundary. H enforces the absolute
  1,800-second monotonic, suspend-inclusive deadline (`CLOCK_BOOTTIME` /
  `mach_continuous_time`) independently of guest clock stability, polls AC power
  status, monitors guest heartbeats, and possesses purely monotonic SIGTERM/SIGKILL
  abort authority over the guest container/VM process group. H holds an ephemeral
  Ed25519 private key, executes a Two-Phase Commit handshake with the guest
  during evidence sealing, has zero access to candidate or oracle data, and
  consumes <=16 MiB host RSS.
- L: one always-active campaign owner/accountant inside the guest environment,
  holding the single monotonic deadline, sealed expected identity, resource
  records, guest heartbeat generator, and verifier exit checks.
- V: at most one independently implemented supervisor/verifier at a time. It
  validates worker output against the sealed oracle stream, not the worker's
  own comparison or exit alone. It must not link candidate/oracle decode logic.
  It validates the host completion acknowledgment signature before final sealing.
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

## 4. R1-to-R2 Change Matrix

| Section / Topic | Accepted R1 Protocol (`1454bd6`) | Proposed R2 Amendment | Scientific & Governance Rationale |
|---|---|---|---|
| **Process Roles (§8)** | Exactly L, V, W. "No extra monitoring process or unaccounted helper is permitted." | Adds external **Role H** on the host. Bounded exception to permit independent watchdog. | Resolves H5 qualification block; prevents unobserved guest freeze from masking deadline breach. |
| **Deadline Authority (§8)** | Solely inside guest L via `time.monotonic_ns()`. | Dual authority: guest L checks internally; host H enforces externally via suspend-aware timer. | Guarantees fail-closed termination even if guest kernel or hypervisor hangs. |
| **Clock Source (§6, §8)** | Monotonic userspace clock. | Dual suspend-aware clocks (`CLOCK_BOOTTIME` / `mach_continuous_time`) and sleep detection ($\Delta \ge 100\text{ ms}$). | Prevents host S3/S4 sleep from hiding campaign elapsed time overruns. |
| **Receipt Authentication (§8)** | Internal JSON files; no signature or trust root. | Ephemeral Ed25519 host keypair; host signs completion ack; verifier validates public key. | Real cryptographic authentication prevents forged or retrofitted watchdog receipts. |
| **Finalization Flow (§8)** | Unsequenced single-pass sealing. | Two-Phase Commit Handshake (Commit-Request $\to$ Ack $\to$ Seal $\to$ Ack Verification). | Resolves finalization circularity; ensures host deadline covers full evidence sealing. |
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
6. **CRIT-R2-06 (Cryptographic Non-Repudiation):** Verification of Role H receipts must rely on Ed25519 signature checks with independent public key distribution.

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
   - Query AC power state (`pmset -g batt` / `/sys/class/power_supply`).
   - Monitor heartbeat pulses; enforce 1,800-second hard deadline via POSIX signals.
   - Execute Two-Phase Commit handshake: sign `host-completion-ack.json` upon valid Commit-Request.
2. **Guest Campaign Handshake (`experiments/arch003/campaign.py`):**
   - Ingest public key `host_authority.pub`.
   - Emit periodic heartbeat pulses during execution.
   - Emit `finalize-request.json` after candidate runs; await and ingest `host-completion-ack.json`.
3. **Verifier Receipt Validation (`experiments/arch003/verifier.py`):**
   - Ingest `host_authority.pub` and verify the Ed25519 signature on `host-completion-ack.json`.
   - Check monotonic continuity ($T_{end} - T_{start} \le 1,800\text{ s}$), and absence of termination or sleep flags.
4. **Negative Test Fixtures (`experiments/arch003/tests/test_watchdog.py`):**
   - Implement test suite verifying N1 (deadlock kill), N2 (heartbeat timeout), N3 (real host suspension / sleep jump), N4 (tampered receipt/signature), and N5 (battery disconnect).

---

## 8. Independent Reviewer Checklist

Independent maintainers and reviewers should use the following checklist to evaluate this proposal:

- [ ] **R2-CHK-01 (Scope Limitation):** Does the amendment strictly restrict Role H to host deadline and power monitoring without granting compute or candidate IPC roles?
- [ ] **R2-CHK-02 (Suspend-Aware Clocks):** Does the amendment mandate suspend-inclusive clocks (`CLOCK_BOOTTIME` / `mach_continuous_time`) and reject suspend-blind timers?
- [ ] **R2-CHK-03 (Two-Phase Finalization):** Is the finalization circularity resolved such that evidence sealing cannot succeed prior to host completion acknowledgment?
- [ ] **R2-CHK-04 (Cryptographic Trust Root):** Are watchdog receipts authenticated via asymmetric signatures (Ed25519) with host key custody rather than shared nonces?
- [ ] **R2-CHK-05 (Memory Boundary):** Is Role H explicitly isolated from the guest 512 MiB RSS limit, and is Role H capped at $\le 16\text{ MiB}$ host RSS?
- [ ] **R2-CHK-06 (Inclusive 1,800s Deadline):** Does the 1,800-second deadline unequivocally encompass configuration, compilation, linking, all D/R/S profiles, oracles, guards, probes, and evidence sealing without resets?
- [ ] **R2-CHK-07 (macOS Battery Hardening):** Does the proposal explicitly reject `caffeinate -s` on battery power and mandate automated AC power verification?
- [ ] **R2-CHK-08 (Fail-Closed Guarantees):** Are deadline breaches, watchdog terminations, heartbeat losses, and receipt corruptions guaranteed to fail closed with no success root?
- [ ] **R2-CHK-09 (Scientific Preservation):** Are all 12 schedule families, 408,034 expected requests, 483,553 calls/profile, representations, and oracle independence strictly preserved?
- [ ] **R2-CHK-10 (Non-Activation Integrity):** Does the proposal refrain from claiming that implementation I2 is compliant or executing campaigns prematurely?

---

*End of Proposal. Awaiting formal maintainer review.*
