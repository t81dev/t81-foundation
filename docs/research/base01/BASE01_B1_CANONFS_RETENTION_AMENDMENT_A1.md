# BASE-01 B1 CanonFS retained-read budget — proposed amendment A1

**Status: PROPOSED / NOT APPROVED.** This is a design-only request for a human protocol decision. It does not authorize qualification, timed execution, warm-up, pilots, measurement, host provisioning, or any change to the measured source. Until explicitly accepted, the existing B1 measurement contract and its **16 MiB retained-output-per-child ceiling** remain authoritative.

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

The implementation must preflight a conservative retention bound before launching a protected cell and account for actual returned-vector capacities during execution. Reject a cell that cannot satisfy the applicable retained-output ceiling; stop on observed violations and retain incomplete/denial evidence. Do not silently truncate reads, discard vectors, free results during the timed interval, alter the batch count, move verification into the timed interval, or increase the RSS ceiling. A preflight estimate or sampled RSS alone is not proof of enforceable limits; qualification must demonstrate effective live resource controls and their failure behavior.

For all three CanonFS read sizes, the minimum payload retained is:

| Payload bytes | Reads | Retained logical bytes | MiB |
| ---: | ---: | ---: | ---: |
| 4,096 | 100 | 409,600 | 0.390625 |
| 65,536 | 100 | 6,553,600 | 6.25 |
| 1,048,576 | 100 | 104,857,600 | 100 |

The 128 MiB budget is a **proposal**, not a demonstrated memory requirement. Before approval, independently verify vector-capacity behavior, driver/parity coexistence, peak process footprint, and enforceability under the qualified Linux host profile. If the budget cannot be supported, keep the cell blocked and propose a separately reviewed protocol revision rather than weakening output lifetime or the resource controls.

## Acceptance and integration gates

1. Maintainer explicitly accepts or rejects the 128 MiB exception and records the rationale, protocol version, effective commit and approving identity.
2. If accepted, update the authoritative [measurement contract](BASE01_B1_MEASUREMENT_CONTRACT.md), [readiness matrix](BASE01_B1_READINESS_MATRIX.md), and any permit schemas, retention checks, qualification criteria, and linked documents **in a separate reviewed implementation**. Do not treat this proposal alone as a change to normative limits.
3. Independently review source-derived retention accounting, actual output capacity, RSS limits and fail-closed tests. Preserve the original 100-read, 100-write and first-insertion semantics.
4. Require the dedicated physical Linux host, reviewed dynamic dispatcher, signed permits, and separate qualification/campaign authorization. No protected measurement follows from this design approval.
5. Retain the physical-host result of PR #698 as **BLOCKED — PHYSICAL HOST UNAVAILABLE**. This amendment does not retroactively qualify a VM or authorize a campaign.

**Disposition:** PROPOSED — AWAITING HUMAN APPROVAL. No experimental evidence or timing result is claimed.
