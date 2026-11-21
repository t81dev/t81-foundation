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

This revision directly resolves all six established review findings in one bounded specification correction:

| Finding / Topic | Issue Summary | Proposal Disposition | Section References | Unresolved Limitations |
|---|---|---|---|---|
| **Finding 1: Clock Correctness** | Linux `BOOTTIME`/`MONOTONIC_RAW` clock pairing suffers from uncoupled rate drift (`adjtime`/NTP frequency disciplining on BOOTTIME but not MONOTONIC_RAW); sleep threshold semantics must be consistent ($\ge 100\text{ ms}$). | **RESOLVED:** Replaced `CLOCK_MONOTONIC_RAW` with matching-rate `CLOCK_MONOTONIC` on Linux, ensuring both clocks track identical kernel frequency scaling and NTP discipline while `CLOCK_BOOTTIME` adds suspend duration. On Darwin, paired matching-rate `mach_continuous_time` with `mach_absolute_time`. Explicitly bounded residual NTP adjustment uncertainty ($\epsilon_{adj} \ll 100\text{ ms}$, sub-millisecond over 1,800s). Standardized uniform $\ge 100,000,000\text{ ns}$ ($\ge 100\text{ ms}$) fail-closed sleep threshold. | Section 2.3 (§2.3.1), Section 3 (§3), Section 8 (§8) | Clocks share frequency slew rate; kernel tick resolution dictates nanosecond timestamp granularity. |
| **Finding 2: Deadline Closure & Authoritative Timestamp** | Terminal sealing, signing, and publication must not occur outside the 1,800s envelope; pre-deadline timestamps cannot certify post-deadline work; need noncircular host publication evidence. | **RESOLVED:** Decoupled Stage 1 receipt sealing from Stage 2 terminal publication attestation (`host-publication-attestation.json`). Authoritative publication timestamp $T_{host\_published\_boot\_ns}$ is sampled strictly after the durable `fsync` of `host-terminal-receipt.json`, binding its SHA-256 digest $H_{receipt}$. Mandated $T_{host\_published\_boot\_ns} - T_{host\_start} \le 1,800 \times 10^9\text{ ns}$. Any publication finishing $>1,800\text{ s}$ fails closed as Status 71 (`INCONCLUSIVE_RESOURCE`). | Section 2.3 (§2.3.3, §2.3.4), Section 2.6 (§2.6), Section 2.11 (§2.11), Section 2.12 (§2.12), Section 3 (§3), Section 8 (§8) | Host-side deadline enforcement operates strictly externally via POSIX timers, kernel SIGKILL, and suspend-aware clocks. |
| **Finding 3: Receipt Authentication** | Host receipt signature specification lacked exact versioned signing payload schema, canonical serialization, explicit signature exclusion, and duplicate-key rejection. | **RESOLVED:** Defined exact versioned Ed25519 signing schema `t81.arch003.host-terminal-receipt.v1`. Mandated ASCII canonical JSON serialization (RFC 8785 subset: keys sorted lexicographically, compact separators, no float representations, no trailing newline before hashing). Explicitly excluded `host_signature_hex` from the signing payload. Mandated strict schema validation, unknown property rejection, and duplicate-key rejection. | Section 2.3 (§2.3.2, §2.3.4), Section 2.11 (§2.11) | Requires standard cryptographic implementation (Ed25519 / RFC 8032) available to host watchdog and verifier. |
| **Finding 4: Evidence Integrity & Artifact Inventory** | Host receipt must bind exact campaign identity and root digest; external verifier must inspect complete artifact inventory and reject conflicting terminal records. | **RESOLVED:** Bound `host-terminal-receipt.json` cryptographically to both `campaign_uuid` and `guest_sealed_root_sha256`. Required host acceptance verifier to independently compute SHA-256 digests for all files indexed in `artifact-index.json` (mandating presence of all expected artifacts, e.g. `observed-S.bin`) and reject missing, malformed, forged, duplicate, or conflicting terminal receipts. | Section 2.3 (§2.3.3), Section 2.4 (§2.4), Section 2.11 (§2.11), Section 2.12 (§2.12) | Offline verifier must read full guest artifact inventory bytes to compute and match file hashes. |
| **Finding 5: Scientific Verification & Contract Preservation** | Verification contract cannot substitute `manifest.status` for full independent validation; reconcile `campaign.json` manifest terminology; preserve full R1/I2 scientific matrix. | **RESOLVED:** Reaffirmed that independent verification strictly requires full re-verification of all 1,450,659 calls across Debug, Release, and Sanitizer profiles against `expected.bin` and recomputation of rolling digests; `manifest.status` / `campaign.json` status alone is strictly insufficient. Clarified that `campaign.json` is the on-disk file serialization of the campaign manifest object. Preserved 100% of R1 workloads, oracles, and controls. | Section 2.9 (§2.9), Section 2.10 (§2.10), Section 2.11 (§2.11) | Independent verification is computationally O(N) over all 1.45M observations; run offline outside campaign budget. |
| **Finding 6: Resource Accounting & Offline Audit Separation** | Detailed limits needed for 512 MiB guest cap, 16 MiB host watchdog cap, host helper process boundaries, 100 MiB output ceiling, and strict separation between in-budget operations and offline audit. | **RESOLVED:** Specified 512 MiB aggregate RSS limit for guest process tree (L, V, W, tools) and separate 16 MiB RSS cap for host Role H. Defined host helper boundaries: bounded, sequential pre-flight probes only (reaped immediately, charged to 16 MiB host budget); no concurrent host workers. Enforced total retained output $\le 100\text{ MiB}$. Explicitly separated in-budget campaign operations ($\le 1,800\text{ s}$) from post-campaign offline audit (independent verification and reviews). | Section 2.5 (§2.5), Section 2.6 (§2.6), Section 3 (§3), Section 8 (§8) | Host watchdog monitoring is strictly isolated from guest memory space and operates under strict 16 MiB RSS cap. |

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

#### 2.3.1 Suspend-Aware Platform Clocks, Matching-Rate Pairing, and Unified Sleep Detection Formula
A standard monotonic clock (`CLOCK_MONOTONIC` on Linux, `mach_absolute_time` on Darwin) pauses when the system enters ACPI S3 (suspend to RAM) or S4 (hibernate). If a host sleeps for 2 hours, `CLOCK_MONOTONIC` advances by 0 seconds, allowing an overrun campaign to appear compliant.

**Matching-Rate Platform Clock Pairs:**
To ensure accurate sleep detection without false positives caused by divergent clock rate disciplining, Role H must sample matching-rate clock pairs:
- **Linux Matching-Rate Pair:**
  - $T_{inclusive} = \text{clock\_gettime}(\text{CLOCK\_BOOTTIME})$: Monotonic time that **includes** time spent in system suspension.
  - $T_{exclusive} = \text{clock\_gettime}(\text{CLOCK\_MONOTONIC})$: Monotonic time that **excludes** suspension.
  - *Rate Coupling Justification:* Comparing `CLOCK_BOOTTIME` against `CLOCK_MONOTONIC_RAW` introduces uncoupled rate drift because `CLOCK_BOOTTIME` is subject to kernel frequency slewing (`adjtime`/NTP/PTP) whereas `CLOCK_MONOTONIC_RAW` runs off raw hardware oscillator ticks without slew. Over extended intervals, frequency drift between slewed and unslewed clocks could accumulate spurious divergence. In contrast, in the Linux kernel timekeeping subsystem, `CLOCK_BOOTTIME` and `CLOCK_MONOTONIC` are driven by identical `tk_core` timekeeper state, share identical frequency multiplier/shift adjustments, and differ strictly by `tk->total_sleep_time`. Thus, when no suspension occurs:
    $$\frac{d}{dt} T_{inclusive}(t) = \frac{d}{dt} T_{exclusive}(t)$$
- **macOS (Darwin) Matching-Rate Pair:**
  - $T_{inclusive} = \text{mach\_continuous\_time}()$: Monotonic time that **includes** system sleep time.
  - $T_{exclusive} = \text{mach\_absolute\_time}()$: Monotonic time that **excludes** system sleep time.
  - *Rate Coupling Justification:* In the XNU kernel, `mach_continuous_time` and `mach_absolute_time` are derived from the identical hardware timebase (`mach_timebase_info`), with `mach_continuous_time` adding the kernel continuous sleep offset accumulator.

**Residual Adjustment Uncertainty Analysis:**
In Linux and Darwin, neither `CLOCK_MONOTONIC` nor `CLOCK_BOOTTIME` undergoes discontinuous steps (`settimeofday` step adjustments only affect `CLOCK_REALTIME`). Kernel frequency adjustments (`adjtime`, NTP phase-locked loop) are clamped to a maximum slew rate of $\pm 500\text{ ppm}$ ($\pm 0.05\%$). Because `CLOCK_BOOTTIME` and `CLOCK_MONOTONIC` share the same timekeeper base, the differential adjustment error $\epsilon_{adj}(t) \equiv [T_{inclusive}(t) - T_{exclusive}(t)] - [T_{inclusive}(0) - T_{exclusive}(0)]$ during continuous non-sleeping execution is mathematically bounded by kernel tick rounding ($\epsilon_{adj} < 100\text{ ns} \ll 100\text{ ms}$). Even in pathological scenarios where asymmetric disciplining occurs, maximum slewing over the entire 1,800-second campaign is bounded by:
$$\max |\epsilon_{adj}| \le 500 \times 10^{-6} \times 1,800\text{ s} = 0.9\text{ seconds}$$
However, since both clocks are disciplined synchronously by the exact same NTP frequency servo in `timekeeping_advance()`, the actual rate difference between `CLOCK_BOOTTIME` and `CLOCK_MONOTONIC` is identically zero ($\epsilon_{adj} = 0$). Any observed $\Delta_{sleep}(t) \ge 100,000,000\text{ ns}$ ($100\text{ ms}$) is conclusively attributable to hardware sleep/suspension and not oscillator drift or timekeeper adjustment uncertainty.

**Unified Baseline-Adjusted Sleep Detection Formula:**
At campaign launch, Role H records initial baseline timestamps: $T_{inclusive}(0)$ and $T_{exclusive}(0)$ in nanoseconds. At any subsequent sampling point $t$, Role H evaluates:
$$\Delta_{sleep}(t) = [T_{inclusive}(t) - T_{exclusive}(t)] - [T_{inclusive}(0) - T_{exclusive}(0)]$$

- **Consistent Threshold:** If $\Delta_{sleep}(t) \ge 100,000,000\text{ ns}$ ($100\text{ ms}$), host sleep or deep suspension has occurred.
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

#### 2.3.3 Terminal Handshake, Strict Operation Ordering, and Authoritative Host Deadline Timestamp
Circularity between guest evidence sealing and host deadline certification is resolved by a strictly ordered Handshake Protocol. Every required in-budget campaign operation must conclude within the 1,800-second deadline envelope.

**Strict Operation Sequencing:**
1. **Candidate & Oracle Completion:** Guest L/V/W completes all builds, all 12 schedule families across Debug, Release, Sanitizer profiles, both oracles, negative guards, and sanitizer probes.
2. **Draft Manifest Finalization:** Guest writes `campaign.json` manifest with observed call counts, final digests, and preliminary resource accounting.
3. **Artifact Index & Evidence Sealing:** Guest traverses its evidence directory, generates `artifact-index.json`, flushes and fsyncs all files, computes $H_{root} = \text{SHA256}(\text{read\_bytes}(\text{artifact-index.json}))$, and writes `evidence.sha256` containing $H_{root}$ formatted as 64 lowercase hexadecimal ASCII characters followed by a newline (`\n`).
4. **Sealing Notification:** Guest transmits sealing-complete signal to Host via IPC pipe:
   `{"campaign_uuid": "<uuid>", "guest_sealed_root_sha256": "<hex>", "guest_elapsed_ns": <ns>}`.
5. **Pre-Exit Invariant Check:** Role H verifies intermediate host-side invariants:
   - Current suspend-inclusive elapsed time $T_{inclusive}(t) - T_{start\_boot\_ns} < 1,800 \times 10^9\text{ ns}$.
   - Baseline-adjusted sleep divergence $\Delta_{sleep}(t) < 100,000,000\text{ ns}$ ($< 100\text{ ms}$).
   - Continuous AC mains power retained throughout.
   - Heartbeat sequence continuous and intact.
6. **Pre-Exit Acknowledgment:** Role H sends pre-exit ACK `{"status": "READY_TO_REAP"}` to Guest over IPC.
7. **Guest Process Termination:** Upon receiving the pre-exit ACK, Guest L terminates all child processes, closes file descriptors, and cleanly exits (exit code 0).
8. **Authoritative Process Reaping & Post-Reap Host Timestamp:** Role H reaps the guest process group (`waitpid`), ensures no lingering guest processes remain, and immediately samples the **post-reap host completion timestamp**:
   $$T_{host\_complete\_boot\_ns} = \text{clock\_gettime}(\text{CLOCK\_BOOTTIME})$$
9. **Receipt Construction, Signing, and Durable Sealing (Stage 1 Record):** Role H constructs the canonical signing dictionary $D_{sign}$ incorporating $T_{host\_complete\_boot\_ns}$ and the elapsed time through reaping $\Delta T_{host\_total} \equiv T_{host\_complete\_boot\_ns} - T_{start\_boot\_ns}$, signs $D_{sign}$ with $SK_{host}$, writes `host-terminal-receipt.json` atomically via a temporary file, and performs durable fsync to persistent host storage.
10. **Post-Publication Timestamp Sampling & Attestation (Stage 2 Record):** Immediately following the completed `fsync` of `host-terminal-receipt.json`, Role H samples the definitive **authoritative terminal publication timestamp**:
    $$T_{host\_published\_boot\_ns} = \text{clock\_gettime}(\text{CLOCK\_BOOTTIME})$$
    Role H reads the raw bytes of the durably written `host-terminal-receipt.json` from disk, computes its cryptographic digest:
    $$H_{receipt} \equiv \text{SHA256}(\text{read\_bytes}(host\text{-}terminal\text{-}receipt.json))$$
    Role H verifies that the total campaign elapsed time through publication remains strictly within budget:
    $$\Delta T_{host\_published} \equiv T_{host\_published\_boot\_ns} - T_{start\_boot\_ns} \le 1,800 \times 10^9\text{ ns}\quad (1,800.0\text{ seconds})$$
    Role H constructs the canonical signing dictionary for `host-publication-attestation.json` binding $H_{receipt}$, `campaign_uuid`, $T_{host\_published\_boot\_ns}$, $\Delta T_{host\_published}$, and host public key, signs it with $SK_{host}$, writes `host-publication-attestation.json` atomically via temporary file, and executes a final durable `fsync`.
11. **Hard Watchdog Abort & Noncircular Deadline Enforcement:**
    - To eliminate circularity where signing an attestation could itself experience an unobserved delay, Role H arms an asynchronous POSIX kernel timer (`timer_create(CLOCK_BOOTTIME)` or `SIGALRM`) configured to deliver an uncatchable `SIGKILL` to Role H at exactly $T_{start\_boot\_ns} + 1,800 \times 10^9\text{ ns}$.
    - If durable publication of `host-publication-attestation.json` does not complete before 1,800.0s, Role H is abruptly terminated by the kernel prior to attestation publication. The absence of `host-publication-attestation.json` ensures deterministic, fail-closed campaign rejection by the offline verifier.
    - If publication completes, Role H disarms the timer, flushes remaining file descriptors, and cleanly exits (exit code 0).
    - **A timestamp signed prior to an event cannot certify that the event completed within budget; acceptance strictly requires both the sealed receipt and the host publication attestation proving post-fsync completion $\le 1,800\text{ seconds}$.**

```
Guest (L / V / W)                                             Host Watchdog (Role H)
    |                                                                   |
    |  [Phase 1: Candidate Runs, Oracles, Guards Complete]             |
    |------------------------------------------------------------------>|  (Periodic heartbeats)
    |                                                                   |
    |  1. Guest writes draft `campaign.json`.                           |
    |  2. Guest seals `artifact-index.json` & `evidence.sha256`.        |
    |  3. Guest emits Sealing-Complete signal over IPC:                 |
    |     { campaign_uuid, root_sha256, guest_elapsed_ns }             |
    |------------------------------------------------------------------>|
    |                                                                   |  [Host validates:
    |                                                                   |   - T_inclusive < 1,800s,
    |                                                                   |   - Delta_sleep < 100 ms,
    |                                                                   |   - AC power maintained,
    |                                                                   |   - heartbeats unbroken]
    |                                                                   |
    |  4. Host returns Pre-Exit ACK (READY_TO_REAP).                    |
    |<------------------------------------------------------------------|
    |                                                                   |
    |  5. Guest cleanly exits 0.                                        |
    |------------------------------------------------------------------>|
    |                                                                   |  6. Host reaps guest (`waitpid`);
    |                                                                   |  7. Host records T_host_complete;
    |                                                                   |  8. Host signs & writes `host-terminal-receipt.json`;
    |                                                                   |  9. Host performs durable fsync;
    |                                                                   | 10. Host records T_host_published_boot_ns;
    |                                                                   | 11. Host computes H_receipt = SHA256(receipt);
    |                                                                   | 12. Host asserts T_host_published <= 1,800s;
    |                                                                   | 13. Host signs & durably publishes
    |                                                                   |     `host-publication-attestation.json`;
    |                                                                   | 14. Host disarms timer & exits 0.
    v                                                                   v
```

**Contents and Fields of `host-terminal-receipt.json` (Stage 1 Record):**
```json
{
  "schema": "t81.arch003.host-terminal-receipt.v1",
  "protocol": "T81-ARCH-003-R2",
  "campaign_uuid": "...",
  "host_start_boot_ns": 1234567890000000,
  "host_sealed_boot_ns": 1234747800000000,
  "host_complete_boot_ns": 1234747890000000,
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

**Contents and Fields of `host-publication-attestation.json` (Stage 2 Record):**
```json
{
  "schema": "t81.arch003.host-publication-attestation.v1",
  "protocol": "T81-ARCH-003-R2",
  "campaign_uuid": "...",
  "host_start_boot_ns": 1234567890000000,
  "host_published_boot_ns": 1234747895000000,
  "host_elapsed_published_ns": 180005000000,
  "terminal_receipt_sha256": "...",
  "watchdog_peak_rss_kib": 4120,
  "host_public_key_hex": "...",
  "host_signature_hex": "..."
}
```

The terminal host receipt is created strictly **after** the guest has sealed its evidence root and transmitted the root digest. It binds the exact `guest_sealed_root_sha256`. The host publication attestation is created strictly **after** durable `fsync` of the receipt, binding the exact SHA-256 digest of `host-terminal-receipt.json` ($H_{receipt}$) and certifying publication within the 1,800-second envelope. Both records, alongside the complete guest evidence archive, are mandatory for campaign acceptance.

#### 2.3.4 Cryptographic Receipt Authentication, Canonical Serialization, and Strict Payload Schema
To ensure non-repudiation and prevent signature-wrapping or mutation attacks, receipt generation and verification must strictly follow the Ed25519 (RFC 8032 / PureEd25519) specification with deterministic canonical serialization:

1. **Versioned Schema Identifier:** The receipt must declare `"schema": "t81.arch003.host-terminal-receipt.v1"`.
2. **Signature-Field Exclusion:** The signature covers the entire canonical receipt payload with the signature field itself (`host_signature_hex`) excluded. The exact signing dictionary $D_{sign}$ consists of exactly 14 non-signature fields:
   - `schema` (string: `"t81.arch003.host-terminal-receipt.v1"`)
   - `protocol` (string: `"T81-ARCH-003-R2"`)
   - `campaign_uuid` (string: 36-char canonical RFC 4122 UUID)
   - `host_start_boot_ns` (integer: $0 \le n \le 2^{63}-1$)
   - `host_sealed_boot_ns` (integer: $0 \le n \le 2^{63}-1$)
   - `host_complete_boot_ns` (integer: $0 \le n \le 2^{63}-1$)
   - `host_elapsed_ns` (integer: $0 \le n \le 2^{63}-1$)
   - `max_sleep_divergence_ns` (integer: $0 \le n \le 2^{63}-1$)
   - `power_source` (string: `"AC_MAINS"`)
   - `guest_sealed_root_sha256` (string: 64 lowercase hexadecimal digits)
   - `heartbeats_received` (integer: $n \ge 0$)
   - `termination_issued` (boolean: `false`)
   - `watchdog_peak_rss_kib` (integer: $0 \le n \le 16,384$)
   - `host_public_key_hex` (string: 64 lowercase hexadecimal digits)

The complete receipt file `host-terminal-receipt.json` consists of exactly 15 fields (the 14 signing payload fields above plus `host_signature_hex`).
3. **Deterministic Canonical Serialization (RFC 8785 subset):**
   - The signing payload $M = \text{canonical}(D_{sign})$ is formatted strictly as ASCII UTF-8 bytes.
   - All dictionary keys are sorted in strict lexicographical ASCII code-point order.
   - Separators are compact: colon `:` between key and value, comma `,` between items, with **zero** whitespace (`separators=(',', ':')`).
   - String values are enclosed in double quotes `"` and must not contain unescaped control characters.
   - Integer values are formatted as standard base-10 ASCII digits without leading zeros, signs (`+`), or floating-point decimals/exponents (`.0`, `e+00`).
   - Boolean values are literal `true` or `false`.
   - The signing payload $M$ contains **no trailing newline**.
4. **Signature Generation and Encoding:**
   - The private key $SK_{host}$ signs the byte sequence $M$:
     $$\sigma = \text{Ed25519\_Sign}(SK_{host}, M)$$
   - The 64-byte signature is encoded as 128 lowercase hexadecimal characters and stored in `host_signature_hex`.
5. **Strict Schema Validation & Duplicate-Key Rejection:**
   - Parser must reject any duplicate object keys (RFC 8259 / RFC 8785 compliance).
   - Any unrecognized or unexpected keys outside the exact 15 defined fields of the complete receipt (or 14 fields of $D_{sign}$) must fail validation immediately.
   - Any type mismatch, uppercase hex characters, or integer out of range must fail validation immediately.

6. **Host Publication Attestation Schema Specification (`t81.arch003.host-publication-attestation.v1`):**
   - The Stage 2 record `host-publication-attestation.json` certifies post-`fsync` completion and binds $H_{receipt}$.
   - The signing payload $D_{pub\_sign}$ consists of exactly 9 non-signature fields:
     - `schema` (string: `"t81.arch003.host-publication-attestation.v1"`)
     - `protocol` (string: `"T81-ARCH-003-R2"`)
     - `campaign_uuid` (string: 36-char canonical RFC 4122 UUID matching `host-terminal-receipt.json`)
     - `host_start_boot_ns` (integer: $0 \le n \le 2^{63}-1$ matching receipt)
     - `host_published_boot_ns` (integer: $0 \le n \le 2^{63}-1$)
     - `host_elapsed_published_ns` (integer: $0 \le n \le 2^{63}-1$, where $host\_elapsed\_published\_ns \equiv host\_published\_boot\_ns - host\_start\_boot\_ns$)
     - `terminal_receipt_sha256` (string: 64 lowercase hexadecimal digits matching $H_{receipt}$)
     - `watchdog_peak_rss_kib` (integer: $0 \le n \le 16,384$)
     - `host_public_key_hex` (string: 64 lowercase hexadecimal digits matching receipt and $PK_{host}$)
   - The complete `host-publication-attestation.json` file contains exactly 10 fields (the 9 signing payload fields above plus `host_signature_hex`).
   - Serialized identically using ASCII canonical JSON (RFC 8785 subset, keys sorted, no trailing newline before signing).
   - Signed with $SK_{host}$; 64-byte signature stored as 128 lowercase hex characters in `host_signature_hex`.
   - Strict validation: unknown fields, duplicate keys, or signature mismatch fails closed immediately.

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
| **Digest Mismatch** | `guest_sealed_root_sha256` $\ne \text{sha256}(artifact\text{-}index.json)$ or $\ne \text{read\_text}(evidence.sha256).\text{strip}()$ | Host acceptance verifier rejects campaign | `FAIL_CORRECTNESS` (Status 65). |
| **Missing Terminal Receipt** | Guest directory present but no valid terminal receipt exists | Host acceptance verifier rejects campaign | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |
| **Missing Publication Attestation** | Terminal receipt exists but `host-publication-attestation.json` missing | Host acceptance verifier rejects campaign | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |
| **Publication Overrun (>1,800s)** | Attestation `host_elapsed_published_ns` $> 1,800 \times 10^9\text{ ns}$ | Host acceptance verifier rejects campaign | `INCONCLUSIVE_RESOURCE` (Status 71). |
| **Receipt Digest Mismatch in Attestation** | `terminal_receipt_sha256` $\ne \text{sha256}(host\text{-}terminal\text{-}receipt.json)$ | Host acceptance verifier rejects campaign | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |
| **Missing Required Artifact** | Any required artifact (e.g. `observed-S.bin`) omitted or deleted | Host acceptance verifier rejects campaign | `FAIL_EVIDENCE` (Status 67). |
| **Artifact Checksum Divergence** | Any file SHA-256 differs from `artifact-index.json` | Host acceptance verifier rejects campaign | `FAIL_EVIDENCE` (Status 67). |
| **Conflicting Terminal Records** | Duplicate terminal receipts or concurrent termination notices | Host acceptance verifier rejects campaign | `BLOCKED_RESOURCE_ACCOUNTING` (Status 70). |
| **Key Identity Mismatch** | `host_public_key_hex` $\ne PK_{host}$ from `host-authorization.json` | Host acceptance verifier rejects campaign | `BLOCKED_AUTHORIZATION` (Status 72). |

---

### 2.5 Item 5: Host-Side Monitoring Boundary, 16 MiB Watchdog Cap, Helper Processes, and Offline Audit Separation

1. **Host/Guest Accounting Separation and 512 MiB Guest Tree Cap:**
   - The 512 MiB aggregate absolute resident memory cap applies strictly to the **guest study process tree** (Owner L + Supervisor V + Worker W + compilation tools + maintenance threads).
   - Absolute aggregate RSS is the conservative R1 closure: idle baselines are recorded for context but none are subtracted. Sanitizer startup/quarantine buffers and supervisor memory count fully.
   - Role H executes in the **host management domain** (outside the VM/container cgroup/namespace). It does not allocate memory from the guest memory pool and cannot page-share with candidate workers.
2. **Host Watchdog Resource Ceiling (16 MiB Cap):**
   - Role H is implemented as a minimal standalone C binary (`arch003_watchdog`) with zero dynamic runtime dependencies.
   - Resident memory ceiling: $\text{RSS}_{H} \le 16\text{ MiB}$ ($16,777,216$ bytes / $16,384\text{ KiB}$).
   - Role H samples its own RSS via `getrusage(RUSAGE_SELF)` and records `watchdog_peak_rss_kib` in `host-terminal-receipt.json`. If $\text{RSS}_{H} > 16\text{ MiB}$, the watchdog receipt is invalid, causing fail-closed rejection.
3. **Host Helper Process Boundaries:**
   - Role H is prohibited from spawning background workers, helper pools, concurrent monitoring tasks, or persistent subprocesses.
   - Bounded, sequential host helpers are permitted solely for instantaneous pre-flight host inspection before guest launch:
     - Querying AC power status (`pmset -g batt` on Darwin or reading `/sys/class/power_supply/` on Linux).
     - Querying hardware DMI/sysctl host properties for qualification records.
   - Any such pre-flight probe must be strictly sequential, must terminate and be reaped immediately, and its memory footprint is charged against the host 16 MiB ceiling.
   - Once guest L is launched, zero host subprocesses are permitted while the campaign is running; Role H must perform all monitoring and timer checks in-process via direct POSIX/OS syscalls.
4. **Retained Output Ceiling ($\le 100\text{ MiB}$):**
   - The total disk footprint of all files retained in the evidence directory across the entire campaign must not exceed 100 MiB ($104,857,600$ bytes).
   - This budget encompasses all numeric observation streams (`observed-D.bin`, `observed-R.bin`, `observed-S.bin`), expectation files (`expected.bin`, `guard-expected.bin`), diagnostic logs, receipts, manifest `campaign.json`, and terminal receipts.
   - Any campaign exceeding 100 MiB retained output fails closed immediately as `INCONCLUSIVE_RESOURCE` (Status 71).
5. **Strict Boundary Between In-Budget Operations and Offline Audit:**
   - **In-Budget Operations ($\le 1,800.0\text{ seconds}$, $\le 512\text{ MiB}$ Guest RSS, $\le 16\text{ MiB}$ Host RSS):**
     - Host setup, guest launch, build/compilation, oracle generation, negative guards, sanitizer probes, all 12 schedule families across D/R/S profiles, supervisor verification during execution, manifest writing, evidence tree sealing (`artifact-index.json`, `evidence.sha256`), terminal receipt signing, durable write (`fsync`), terminal ACK, and guest process exit/reaping.
   - **Offline Audit (Independent Post-Campaign Operations):**
     - Independent host acceptance verification (`host_acceptance_verifier.py`), full 1.45M call replay verification, artifact SHA-256 tree audit, human review, and scientific evaluation execute strictly after campaign completion.
     - Offline audit operations are not constrained by the 1,800-second campaign budget or the 512 MiB guest RSS cap; they do not alter campaign evidence and cannot extend or modify in-budget execution.

---

### 2.6 Item 6: Inclusive 1,800-Second Campaign Envelope and Authoritative Completion Ordering

The 1,800-second ($30.0$ minute) wall-clock deadline is absolute, monotonic, suspend-inclusive, and non-resettable. It encompasses every required in-budget campaign operation from initial host setup through final process termination and durable publication:

$$\sum T = T_{host\_setup} + T_{guest\_launch} + T_{configure} + T_{compile} + T_{link} + T_{oracles} + T_{guards} + T_{probes} + T_{profiles(D,R,S)} + T_{verify} + T_{manifest} + T_{hash\_seal} + T_{pre\_exit\_ack} + T_{guest\_reap} + T_{receipt\_fsync} + T_{attestation\_fsync} \le 1,800.0\text{ seconds}$$

- **No reset** on profile boundary ($D \to R \to S$).
- **No reset** on worker transitions ($W_i \to W_{i+1}$).
- **No exclusion** of file system I/O, artifact writing, SHA-256 tree hashing, pre-exit acknowledgment, guest process reaping, final receipt signing, durable storage syncing (`fsync`), or publication attestation.
- **Strict Ordering of Finalization:**
  1. Guest seals evidence and computes `evidence.sha256` ($H_{root}$).
  2. Guest transmits sealing notification with sealed root digest to Role H.
  3. Role H verifies intermediate suspend divergence and power invariants.
  4. Role H transmits pre-exit ACK (`READY_TO_REAP`) to Guest.
  5. Guest exits cleanly (exit code 0).
  6. Role H reaps the guest process tree (`waitpid`).
  7. Role H samples post-reap timestamp $T_{host\_complete\_boot\_ns}$.
  8. Role H generates, signs, and durably flushes (`fsync`) `host-terminal-receipt.json` to disk.
  9. Role H samples definitive terminal publication timestamp $T_{host\_published\_boot\_ns}$.
  10. Role H computes $H_{receipt} = \text{SHA256}(\text{read\_bytes}(host\text{-}terminal\text{-}receipt.json))$.
  11. Role H verifies $T_{host\_published\_boot\_ns} - T_{host\_start\_boot\_ns} \le 1,800 \times 10^9\text{ ns}$.
  12. Role H signs and durably flushes (`fsync`) `host-publication-attestation.json` to disk.
  13. Role H disarms its watchdog abort timer and cleanly exits 0.
- **Authoritative Host Publication Timestamp and Final Publication Ceiling:**
  The authoritative publication timestamp $T_{host\_published\_boot\_ns}$ is sampled strictly after the durable `fsync` of `host-terminal-receipt.json`. Furthermore, the complete campaign deadline encompasses durable publication on disk: both `host-terminal-receipt.json` and `host-publication-attestation.json` must be durably written and fsynced within the 1,800-second envelope. Under no circumstances may an earlier intermediate timestamp (such as guest-side timer or post-reap timestamp) certify compliance if subsequent receipt signing, durable storage sync, or publication attestation exceeds the 1,800-second mark.
- If $T_{host\_published\_boot\_ns} - T_{host\_start\_boot\_ns} > 1,800 \times 10^9\text{ ns}$, or if publication attestation is missing or completes after 1,800 seconds, the campaign is rejected fail-closed with Status 71 (`INCONCLUSIVE_RESOURCE`); no valid passing certificate or success root may be formed.

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

### 2.9 Item 9: Preservation of Scientific Workloads, Verification Contract, and 100 MiB Evidence Envelope

The R2 amendment is strictly an operational governance and host-authority amendment. It preserves 100% of the accepted R1/I2 scientific workload, controls, and verification contract:

- **Mathematical API Contract:** Inputs are immutable $\{-1, 0, +1\}$ arrays; operation is $C = \sum [a_i = b_i]$; operand-local format tags and strict error precedence unchanged (§2).
- **Candidate Representations:** Exact five representations (BYTE, C2, PLANES, PT5, NATIVE) preserved (§3).
- **Frozen Schedule and Cardinality:** Exactly 12 families, 408,034 expected requests, 483,553 calls per profile, 1,450,659 total calls across D, R, and S profiles (§4).
- **Oracle Independence:** Dual independent oracles (Scalar Oracle and Histogram Oracle) compile in separate translation units with zero candidate linkage; both streams must agree before candidate execution (§4).
- **Full Accepted Verification Contract Preserved (§4, §8):**
  - Verification is never delegated or skipped. The independent verifier cannot rely on `manifest.status` or `campaign.json` declaration alone.
  - Acceptance strictly requires full offline re-verification of all 1,450,659 calls across Debug, Release, and Sanitizer profiles against `expected.bin` (header count, sequential case IDs, eligibility masks, rolling digests).
  - All 6 final rolling digests per profile must match the mathematical expectations exactly.
  - All 6 guard receipts (G1 wrong-result exit 1, G2 over-long exit 1) and both sanitizer probe receipts (`asan_heap_end`, `ubsan_shift`) must be independently checked.
- **Terminology Reconciliation (`campaign.json` vs `manifest`):**
  - The document reconciles terminology: the canonical file name on disk in the evidence directory is `campaign.json` (as implemented in `evidence.py` and `verifier.py`), which contains the serialized campaign manifest object adhering to schema `t81.arch002.correctness-evidence.v1`. References in documentation to "the campaign manifest" or `manifest.json` refer to this exact artifact `campaign.json`.
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
2. **Terminal Receipt Parsing and Strict Schema Verification (Stage 1 Record):**
   - Locate `host-terminal-receipt.json` on the host evidence path.
   - Parse JSON bytes strictly rejecting duplicate object keys or unescaped control characters.
   - Assert `"schema" == "t81.arch003.host-terminal-receipt.v1"` and `"protocol" == "T81-ARCH-003-R2"`.
   - Assert exact match with the defined 15-field complete receipt schema; reject any unexpected properties.
   - Assert $PK_{host}$ in `host_public_key_hex` matches trusted $PK_{host}$ from `host-authorization.json`.
   - Extract `host_signature_hex`, decode 128 hex chars to 64 bytes $\sigma_1$.
   - Reconstruct signing dictionary $D_{sign}$ by excluding `host_signature_hex` (exactly 14 fields).
   - Format canonical ASCII UTF-8 signing payload $M_1 = \text{canonical}(D_{sign})$ (keys sorted, compact separators, no trailing newline).
   - Verify $\text{Ed25519\_Verify}(PK_{host}, M_1, \sigma_1) == \text{valid}$. If verification fails, reject immediately as `BLOCKED_RESOURCE_ACCOUNTING` (Status 70).
   - Verify `termination_issued == false`.
   - Verify `host_elapsed_ns <= 1800 * 10^9`.
   - Verify `max_sleep_divergence_ns < 100_000_000`.
   - Verify `power_source == "AC_MAINS"`.
   - Verify `watchdog_peak_rss_kib <= 16384` (16 MiB).
3. **Publication Attestation Parsing and Post-Publication Deadline Verification (Stage 2 Record):**
   - Locate `host-publication-attestation.json` on the host evidence path. If missing, fail closed immediately as `BLOCKED_RESOURCE_ACCOUNTING` (Status 70).
   - Parse JSON bytes strictly rejecting duplicate object keys or unescaped control characters.
   - Assert `"schema" == "t81.arch003.host-publication-attestation.v1"` and `"protocol" == "T81-ARCH-003-R2"`.
   - Assert exact match with the defined 10-field complete attestation schema; reject any unexpected properties.
   - Assert $PK_{host}$ in attestation `host_public_key_hex` matches trusted $PK_{host}$ from `host-authorization.json`.
   - Extract `host_signature_hex`, decode 128 hex chars to 64 bytes $\sigma_2$.
   - Reconstruct signing dictionary $D_{pub\_sign}$ by excluding `host_signature_hex` (exactly 9 fields).
   - Format canonical ASCII UTF-8 signing payload $M_2 = \text{canonical}(D_{pub\_sign})$.
   - Verify $\text{Ed25519\_Verify}(PK_{host}, M_2, \sigma_2) == \text{valid}$. If verification fails, reject immediately as `BLOCKED_RESOURCE_ACCOUNTING` (Status 70).
   - **Receipt Digest Binding Check:** Compute $H_{receipt\_disk} \equiv \text{SHA256}(\text{read\_bytes}(host\text{-}terminal\text{-}receipt.json))$.
   - Assert $H_{receipt\_disk} == \text{attestation.terminal\_receipt\_sha256}$.
   - **Authoritative Inclusive Deadline Verification:**
     Assert $\text{attestation.host\_elapsed\_published\_ns} \le 1,800 \times 10^9\text{ ns}$ ($1,800.0\text{ seconds}$).
     If $\text{attestation.host\_elapsed\_published\_ns} > 1,800 \times 10^9\text{ ns}$, reject immediately fail-closed as `INCONCLUSIVE_RESOURCE` (Status 71).
4. **Evidence-Root Identity & Guest Root Binding Verification:**
   - The normative definition of the guest evidence-root digest throughout R1, I2, and R2 is strictly the SHA-256 hash of `artifact-index.json`:
     $$H_{root} \equiv \text{SHA256}(\text{read\_bytes}(artifact\text{-}index.json))$$
   - Ingest `evidence.sha256` from the guest directory; verify that its stripped text content matches $H_{root}$:
     $$\text{read\_text}(evidence.sha256).\text{strip}() == H_{root}$$
   - Ingest `guest_sealed_root_sha256` from `host-terminal-receipt.json`.
   - Assert $H_{root} == \text{guest\_sealed\_root\_sha256}$. Any disagreement between $H_{root}$, the content of `evidence.sha256`, and `guest_sealed_root_sha256` in the signed host receipt causes immediate fail-closed rejection as `FAIL_CORRECTNESS` (Status 65).
5. **Complete Artifact Inventory & Digest Verification:**
   - Ingest `artifact-index.json` from the guest directory.
   - Verify that `sha256(artifact-index.json)` matches the root hash verified above.
   - Traverse every entry in `artifact-index.json`. For every indexed path:
     - Verify the file exists on disk and is a regular file (not a symlink or directory).
     - Read file bytes and compute actual $\text{SHA256}(\text{bytes})$.
     - Assert actual size equals indexed `bytes`.
     - Assert actual hash equals indexed `sha256`.
   - Verify exhaustive inventory completeness: assert that every single required protocol artifact is present and non-empty:
     - `expected.bin` (408,034 expected records, 35,907,008 bytes)
     - `observed-D.bin` (Debug profile, 483,553 calls, 15,473,712 bytes)
     - `observed-R.bin` (Release profile, 483,553 calls, 15,473,712 bytes)
     - `observed-S.bin` (Sanitizer profile, 483,553 calls, 15,473,712 bytes)
     - `guard-expected.bin`, `guard-observed-*.bin`
     - All 6 oracle receipts, all 6 guard receipts, all 2 sanitizer probe receipts, build receipts, binary receipts, and `authorization.json`.
   - Any missing file (including specifically `observed-S.bin`) or hash mismatch fails immediately as `FAIL_EVIDENCE` (Status 67).
6. **Detection and Rejection of Conflicting Terminal Records:**
   - Assert that exactly one terminal receipt file (`host-terminal-receipt.json`) and exactly one publication attestation (`host-publication-attestation.json`) exist in the host evidence directory.
   - Scan for conflicting or duplicate terminal artifacts (e.g. `host-terminal-receipt.json` co-existing with `host-watchdog-termination.json`, duplicate receipt copies, or contradictory exit records).
   - If conflicting terminal records exist, fail closed immediately as `BLOCKED_RESOURCE_ACCOUNTING` (Status 70).
7. **Guest Manifest Verification and Terminology Reconciliation:**
   - Ingest the on-disk campaign manifest file `campaign.json` from the guest directory.
   - Validate schema `t81.arch002.correctness-evidence.v1`, asserting `manifest.status == "PASS_CORRECTNESS_CORE"` (Status 64).
   - Note: While `campaign.json` status must be PASS, this status check is strictly necessary but **not sufficient** on its own; full independent call re-verification against `expected.bin` is mandatory.
8. **Acceptance Verdict:**
   - If all checks pass: Emit `ACCEPTANCE_CONFIRMED: ARCH-003-R2 CAMPAIGN VALID`. Exit 0.
   - If any check fails, receipt or attestation is missing/malformed, or digests disagree: Emit `ACCEPTANCE_REJECTED: <REASON>`. Exit 1.

---

### 2.12 Item 12: Mandatory Specification Counterexamples

To eliminate specification ambiguity, the protocol mandates four explicit counterexamples demonstrating guaranteed deterministic rejection:

1. **Counterexample 1: Deletion or Omission of `observed-S.bin`**
   - *Scenario:* A campaign completes with all tests passing, but the artifact `observed-S.bin` is omitted from `artifact-index.json` or deleted from disk after sealing.
   - *Deterministic Outcome:* Rejected immediately with exit code 1 as `FAIL_EVIDENCE` (Status 67). Verification of the complete artifact inventory explicitly mandates that `observed-D.bin`, `observed-R.bin`, and `observed-S.bin` must all be present on disk, match their exact indexed byte lengths, and match their SHA-256 digests. No subset of profile evidence can ever be accepted.
2. **Counterexample 2: Publication Completing at 1,801 Seconds (Post-Deadline Publication Overrun)**
   - *Scenario:* Guest execution and reaping complete cleanly at 1,795 seconds, and Role H signs `host-terminal-receipt.json` bearing post-reap timestamp $T_{host\_complete\_boot\_ns} = T_{start} + 1,795\text{ s}$. However, host disk I/O, storage fsync, or publication attestation experiences a 6-second host stall, completing durable publication at $T_{host\_published\_boot\_ns} = T_{start} + 1,801.0\text{ seconds}$ ($1,801 \times 10^9\text{ ns}$).
   - *Deterministic Outcome:* Rejected immediately fail-closed as `INCONCLUSIVE_RESOURCE` (Status 71). A pre-publication timestamp cannot certify that publication completed within the 1,800s envelope.
     - *Watchdog Behavior:* The host watchdog's asynchronous 1,800s kernel timer fires at 1,800.0s, delivering SIGKILL to Role H before it can publish `host-publication-attestation.json`. Even if the attestation file were somehow written before termination, its recorded field `host_elapsed_published_ns` would equal $1,801,000,000,000\text{ ns} > 1,800 \times 10^9\text{ ns}$.
     - *Verifier Behavior:* The offline host acceptance verifier (`host_acceptance_verifier.py`) inspects `host-publication-attestation.json`. If missing, it rejects with code 1 (`BLOCKED_RESOURCE_ACCOUNTING`, Status 70). If present with `host_elapsed_published_ns > 1,800 * 10^9`, it rejects with code 1 (`INCONCLUSIVE_RESOURCE`, Status 71). No valid passing certificate can be formed.
3. **Counterexample 3: Missing, Malformed, Forged, Duplicate, or Conflicting Receipts**
   - *Scenario A (Missing Receipt):* Guest evidence is sealed, but `host-terminal-receipt.json` was not generated or was deleted.
     - *Outcome:* Rejection with exit code 1 (`BLOCKED_RESOURCE_ACCOUNTING`, Status 70).
   - *Scenario B (Malformed Schema / Duplicate Keys):* `host-terminal-receipt.json` contains duplicate keys, floats (`180.0`), unquoted numbers, or missing schema fields.
     - *Outcome:* Rejection with exit code 1 (`BLOCKED_RESOURCE_ACCOUNTING`, Status 70).
   - *Scenario C (Forged / Invalid Signature):* A single bit in the receipt payload or signature $\sigma$ is flipped, or signed with an untrusted private key.
     - *Outcome:* Ed25519 verification fails; rejection with exit code 1 (`BLOCKED_RESOURCE_ACCOUNTING`, Status 70).
   - *Scenario D (Conflicting Records):* Both `host-terminal-receipt.json` and `host-watchdog-termination.json` exist simultaneously in the evidence path.
     - *Outcome:* Rejection with exit code 1 (`BLOCKED_RESOURCE_ACCOUNTING`, Status 70).
4. **Counterexample 4: Replaced Public Key or Mismatched Authorization Identity**
   - *Scenario:* A modified runner replaces the host public key in a guest directory file or transmits a self-signed key in an attempt to authenticate an untrusted receipt.
   - *Deterministic Outcome:* Rejected immediately with exit code 1 as `BLOCKED_AUTHORIZATION` (Status 72). The host acceptance verifier loads $PK_{host}$ exclusively from the immutable pre-launch `host-authorization.json` on the host. Any mismatch against `host_public_key_hex` or any attempt to read keys from the guest directory causes immediate fatal rejection.

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
- Host timer must record dual monotonic matching-rate clocks: one suspend-inclusive
  (`CLOCK_BOOTTIME` on Linux, `mach_continuous_time` on Darwin) and one
  suspend-exclusive (`CLOCK_MONOTONIC` on Linux, `mach_absolute_time` on Darwin).
  Any baseline-adjusted divergence between these matching-rate clocks greater
  than or equal to 100,000,000 ns (100 ms) indicates host suspension or deep
  sleep and invalidates the campaign as INCONCLUSIVE_RESOURCE (Status 71)."

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
  Host helper processes are permitted solely for instantaneous pre-flight host
  inspection before guest launch, run sequentially, and are charged to H's 16 MiB cap;
  no concurrent or in-run host helper processes are permitted.
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
time (measured to the authoritative post-fsync publication timestamp T_host_published),
and <=100 MiB retained output ceiling. Offline audit (independent verification and
reviews) executes strictly after campaign finalization outside the 1,800-second
budget. Final acceptance requires a valid sealed guest evidence bundle (with
complete artifact inventory including observed-S.bin), an independently verified
terminal host receipt, and a signed host publication attestation issued by H certifying
post-fsync completion <= 1,800 seconds."
```

---

## 4. R1-to-R2 Change Matrix

| Section / Topic | Accepted R1 Protocol (`1454bd6`) | Proposed R2 Amendment | Scientific & Governance Rationale |
|---|---|---|---|
| **Process Roles (§8)** | Exactly L, V, W. "No extra monitoring process or unaccounted helper is permitted." | Adds external **Role H** on the host. Bounded exception to permit independent watchdog. | Resolves H5 qualification block; prevents unobserved guest freeze from masking deadline breach. |
| **Deadline Authority (§8)** | Solely inside guest L via `time.monotonic_ns()`. | Dual authority: guest L checks internally; host H enforces externally via suspend-aware timer. | Guarantees fail-closed termination even if guest kernel or hypervisor hangs. |
| **Clock Source (§6, §8)** | Monotonic userspace clock. | Dual matching-rate suspend-aware clocks (`CLOCK_BOOTTIME` vs `CLOCK_MONOTONIC` on Linux; `mach_continuous_time` vs `mach_absolute_time` on Darwin) and sleep detection ($\Delta_{sleep} \ge 100\text{ ms}$). | Prevents host S3/S4 sleep from hiding campaign elapsed time overruns while eliminating uncoupled clock drift. |
| **Receipt Authentication (§8)** | Internal JSON files; no signature or trust root. | Ephemeral Ed25519 host keypair; host signs terminal receipt; host verifier checks against pre-authorized key. | Real cryptographic authentication prevents forged or retrofitted watchdog receipts. |
| **Finalization Flow (§8)** | Unsequenced single-pass sealing. | Two-Stage Terminal Handshake: guest seals `artifact-index.json` & `evidence.sha256`, receives host pre-exit ACK, exits 0; host reaps guest, samples authoritative completion timestamp, signs & durably publishes `host-terminal-receipt.json` under inclusive 1,800s deadline. | Resolves circularity; ensures host deadline covers full evidence sealing, reaping, and final receipt publication. |
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
   - Sample dual matching-rate clocks (`CLOCK_BOOTTIME` and `CLOCK_MONOTONIC` on Linux; `mach_continuous_time` and `mach_absolute_time` on Darwin).
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
- [ ] **R2-CHK-02 (Suspend-Aware Clocks):** Does the amendment mandate matching-rate suspend-inclusive clocks (`CLOCK_BOOTTIME` vs `CLOCK_MONOTONIC` on Linux, `mach_continuous_time` vs `mach_absolute_time` on Darwin) and evaluate the baseline-adjusted sleep formula?
- [ ] **R2-CHK-03 (Terminal Host Receipt):** Is the terminal host receipt created after guest evidence sealing, binding the exact `artifact-index.json` SHA-256 root digest matching `evidence.sha256`?
- [ ] **R2-CHK-04 (Independent Key Identity):** Is $PK_{host}$ anchored in pre-launch authorization rather than trusting guest-controlled files?
- [ ] **R2-CHK-05 (Dual Acceptance Verification):** Does final acceptance require verification by an external host verifier without relying solely on the guest?
- [ ] **R2-CHK-06 (Memory Boundary):** Is Role H explicitly isolated from the guest 512 MiB RSS limit, and is Role H capped at $\le 16\text{ MiB}$ host RSS?
- [ ] **R2-CHK-07 (Inclusive 1,800s Deadline & Publication Attestation):** Does the 1,800-second deadline unequivocally encompass configuration, compilation, linking, all D/R/S profiles, oracles, guards, probes, evidence sealing, receipt signing, durable fsync, ACK, guest process reaping, and publication attestation without resets, measured to the authoritative post-fsync publication timestamp?
- [ ] **R2-CHK-08 (macOS Battery Hardening):** Does the proposal explicitly reject `caffeinate -s` on battery power and mandate automated AC power verification?
- [ ] **R2-CHK-09 (Fail-Closed Guarantees):** Are deadline breaches, watchdog terminations, heartbeat losses, and receipt corruptions guaranteed to fail closed with no success root?
- [ ] **R2-CHK-10 (Scientific Preservation):** Are all 12 schedule families, 408,034 expected requests, 483,553 calls/profile, representations, and oracle independence strictly preserved?
- [ ] **R2-CHK-11 (Artifact Inventory Completeness):** Does verification require checking the presence and SHA-256 digest of every indexed artifact including `observed-S.bin`, and does missing `observed-S.bin` cause deterministic rejection?
- [ ] **R2-CHK-12 (Receipt Authentication Rigor):** Does `host-terminal-receipt.json` adhere to versioned schema `t81.arch003.host-terminal-receipt.v1` with canonical JSON serialization, signature field exclusion, strict duplicate-key rejection, and pre-authorized key validation?
- [ ] **R2-CHK-13 (Offline Audit Separation):** Is the separation between in-budget operations ($\le 1,800\text{ s}$) and post-campaign offline audit (independent verification and reviews) strictly preserved?
- [ ] **R2-CHK-14 (Non-Activation Integrity):** Does the proposal refrain from claiming that implementation I2 is compliant or executing campaigns prematurely?

---

*End of Proposal. Awaiting formal maintainer review.*
