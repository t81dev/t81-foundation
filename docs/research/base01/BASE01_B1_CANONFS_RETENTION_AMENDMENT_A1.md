# BASE-01 B1 CanonFS retained-read budget — proposed amendment A1

**Status: HUMAN-DIRECTED PROTOCOL INTEGRATION CANDIDATE — NOT MERGED OR ACTIVATED.** The maintainer instructed integration after Codex's five-finding proposal-level acceptance review (conversation instruction, 2026-11-03). This records direction to prepare the normative-document changes on draft PR #699, not signed measurement authorization or proof of a qualified host. The effective published protocol remains the current default branch until separately reviewed and merged. No qualification, timed execution, warm-up, pilot, measurement, host provisioning, or measured-source change is authorized.

## Conflict and scope

The [B1 measurement contract](BASE01_B1_MEASUREMENT_CONTRACT.md) specifies 100 allocating `read_object_bytes(ref)` calls, with all returned vectors held alive until after timing, for each of three payload sizes (4,096, 65,536, 1,048,576 bytes). Its common budget specifies **16 MiB retained output per child**. The largest read cell alone retains at least `100 * 1,048,576 = 104,857,600 bytes = 100 MiB` of returned payload. Therefore the currently specified cell cannot simultaneously meet the output-lifetime and resource-ceiling requirements.

This amendment concerns only `WL-CANONFS-MEM-THROUGHPUT` retained read output. It does not modify source/input hashes, the measured API, number of operations, write first-insertion fraction, verification, other workload cells, or the 1 GiB worker peak-RSS observation budget.

## Proposed normative change (subject to approval)

Replace the common **16 MiB retained-output-per-child ceiling** with:

- **All non-CanonFS cells:** 16 MiB retained output per child, unchanged.
- **CanonFS read cells only:** 128 MiB retained-output capacity budget per child, covering all simultaneously live caller-owned returned read vectors and their backing storage. Count allocated vector **capacity**, not only logical payload length. The largest 100-read cell has a 100 MiB logical-payload floor, leaving 28 MiB for capacity slack and other retained caller output; it is **not** a proven RSS bound.
- **CanonFS write cells:** 16 MiB retained-output budget remains unless a separate, justified amendment is accepted. References must be included in capacity accounting.
- **Global worker peak-RSS observation budget:** 1 GiB, unchanged; this includes driver state, parity, allocator/runtime overhead, inputs, outputs, and all other process memory. Retained-output accounting is distinct from RSS. Do not infer that 128 MiB retained output guarantees compliance with the 1 GiB RSS budget.
- **Other existing ceilings:** unchanged, including one worker at a time, 60 seconds per trial/pilot, 8 GiB evidence, and campaign/qualification duration limits.

### Timing-preserving accounting

Before any protected worker is released, preflight a conservative retention bound against the pinned standard-library and measured-API implementation. Construct empty caller-owned output slots outside timing. During the timed interval perform only the specified 100 allocating API reads and move returned owning vectors into those slots; retain all results. **Inspect actual vector capacities only after the batch timer stops**, before releasing results. Do not insert per-read capacity branches, allocator interposition, custom allocators, preallocated read destinations, correctness checks, or serialization into the measured interval. Post-interval accounting can detect but cannot prevent an already completed excessive allocation; separate fail-closed process containment is required.

Define the checked retention ledger as the overflow-safe sum of **unique simultaneously live caller-owned output backing capacities in bytes**, outer slot array allocated capacity (`capacity * sizeof(slot)`), returned-reference/result container capacities, and any additional caller-owned retained output copies. Count each backing allocation once; record slot ABI sizes and observed capacities. Inputs, driver object/parity buffers, temporary verification buffers, and evidence serialization are **not** included in this output-retention ledger: they must be explicitly accounted for under the separate whole-process RSS/resource budget and evidence budget. Do not mislabel logical payload length or total logical driver buffers as output capacity. Reject a cell whose preflight cannot meet the applicable retained-output ceiling; a post-interval overrun invalidates the cell and preserves incomplete/denial evidence.

**Evidence boundary:** independently verify every returned byte after timing, without constructing 100 full hexadecimal strings or holding extra full-batch serialized copies in worker memory. Stream bounded evidence after the interval with separately specified output size, write limits and integrity checks. The 100 large reads would produce at least 200 MiB of hex text for one cell and approximately 6.25 GiB across 30 trials and all three sizes before metadata; the existing two-read correctness serializer is not a qualified measurement serializer. Compression alone is not a live-memory or disk-space bound.

### Resource containment and acceptance

Keep **1 GiB = 1,048,576 KiB** as a *process-lifetime peak-RSS acceptance threshold*, checked by exact-child accounting. Separately require a **human-approved** cgroup v2 memory-containment ceiling with recorded membership, `memory.max`, swap policy, ancestor limits, controller availability, reclaim/OOM events and overshoot behavior; this ceiling is **not selected by this proposal**. `memory.max` is cgroup-accounted memory, not an exact process-RSS cap, and may temporarily overshoot. Do not use `RLIMIT_RSS` as enforcement or `memory.high` throttling in a timing run; `RLIMIT_AS` constrains virtual address space, not RSS. Record separate launcher/observer memory limits and evidence-storage limits. Missing/unreadable controls, wrong membership, unexpected reclaim, OOM or control events must deny or invalidate the cell. Sampled RSS cannot prevent transient excess.

### Fixed-count qualification exception

**CanonFS read and write cells are fixed at exactly 100 operations each.** The generic pilot batch-doubling procedure in the host-qualification design must not apply to these cells. Doubling the largest reads requires at least 200 MiB retained payload and violates the proposed 128 MiB ceiling; doubling writes changes first insertion from 1/100 to 1/200. If the timer overhead or stability gate fails at 100 operations, the cell remains blocked until another explicit reviewed protocol amendment. No automatic doubling, count reduction, hidden result destruction or altered write-key semantics is permitted.

For all three CanonFS read sizes, the minimum payload retained is:

| Payload bytes | Reads | Retained logical bytes | MiB |
| ---: | ---: | ---: | ---: |
| 4,096 | 100 | 409,600 | 0.390625 |
| 65,536 | 100 | 6,553,600 | 6.25 |
| 1,048,576 | 100 | 104,857,600 | 100 |

The 128 MiB budget is a **proposal**, not a demonstrated memory requirement. Before approval, independently verify vector-capacity behavior, driver/parity coexistence, peak process footprint, the fixed-count exception, and enforceability under the qualified Linux host profile. The pinned in-memory driver returns an owning vector copy on reads; GCC 13.3 libstdc++ source suggests exact-size copy allocation, but this is not qualification of the eventual host build. If the budget cannot be supported, keep the cell blocked and propose a separately reviewed protocol revision rather than weakening output lifetime or the resource controls.

## Acceptance and integration gates

1. Maintainer explicitly accepts or rejects the 128 MiB exception, the fixed-count pilot exception, accounting placement, and the separate cgroup containment policy; record rationale, protocol version, effective commit and approving identity.
2. If accepted, update the authoritative [measurement contract](BASE01_B1_MEASUREMENT_CONTRACT.md), [readiness matrix](BASE01_B1_READINESS_MATRIX.md), and any permit schemas, retention checks, qualification criteria, and linked documents **in a separate reviewed implementation**. Do not treat this proposal alone as a change to normative limits.
3. Independently review source-derived retention accounting, actual output capacity, RSS limits and fail-closed tests. Preserve the original 100-read, 100-write and first-insertion semantics.
4. Require the dedicated physical Linux host, reviewed dynamic dispatcher, signed permits, and separate qualification/campaign authorization. No protected measurement follows from this design approval.
5. Retain the physical-host result of PR #698 as **BLOCKED — PHYSICAL HOST UNAVAILABLE**. This amendment does not retroactively qualify a VM or authorize a campaign.

**Disposition:** PROTOCOL-INTEGRATION CANDIDATE — HUMAN DIRECTION TO PROCEED RECORDED; PENDING PR REVIEW/MERGE. No experimental evidence, timing result, signed permit, or physical-host qualification is claimed. The containment ceiling and its enforcement profile still require a separate human decision.
