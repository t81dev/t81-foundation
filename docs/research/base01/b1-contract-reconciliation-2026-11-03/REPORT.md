# BASE-01 B1 scientific contract reconciliation

**PROPOSED — NOT APPROVED. Verdict: FURTHER_PROTOCOL_DESIGN_REQUIRED.**
November 3, 2026. Documentation-only review; no measurement authorization.

The 1 MiB read cell cannot satisfy both the proposed all-results-live lifetime
and the proposed 16 MiB retained-output ceiling. Count doubling also conflicts
with the fixed first-insertion fraction unless the store lifetime changes.
These are scientific workload decisions, not implementation defects that a timer
patch can resolve. No correction option is recommended as scientifically
equivalent on the available evidence. [AMENDMENT.md](AMENDMENT.md) supplies the
smallest unconditional clarification and a concrete, conditional lifetime design
for a maintainer to assess. [READINESS.md](READINESS.md) defines acceptance gates.

## Authority, scope and independence

Author: Codex, GPT-6 (identity supplied by this session). The inspected B0, B1
design and foundation attribute their authorship to Codex/GPT-6. This review has
read those sources and their reports; it is not demonstrably independent of that
authoring lineage. A separate role label or session does not establish scientific
independence. Newly executed checks are three Python rejection/identity fixtures,
offline Commons discovery, Git/blob inspection and integer derivations. No native
workload, host qualification, timer probe, warm-up, pilot, benchmark, PMU collection
or physical experiment ran. Prior execution reports remain attributed evidence.

This review supports H07's evidence apparatus and the stable CanonFS/runtime
maintenance lane. It neither changes H01–H08 nor the Hanoi/CWEB lineage, research
direction, protected artifacts, Protocol 001, source revision or campaign scope.
The user's reported independent readiness audit was not supplied as a separately
identifiable artifact. Its two reported issues are checked directly below; no
claims about that audit's authorship, execution or completeness follow.

GitHub API observations and local ancestry establish:

| Identity | Exact revision | Meaning |
| --- | --- | --- |
| Current main at audit | `a98b22fb65c80f4225151cbcd5c6934ae076cbe1` | Review snapshot, not a replacement measured revision |
| PR #694 head | `851cfef0ef16a56f59d3e933460af1d949cacdf9` | Integrated package head |
| PR #694 merge | `52921bd08ff9a9d765d4f129791b5a98304b6b6e` | Merged 2026-11-03 20:01:47 UTC; ancestor of snapshot |
| PR #695 head | `3c615dda721bf7a864e9e3514b1ada23a1e3075c` | Acceptance-review head |
| PR #695 merge | `4f9edb9c3d434bbc89331fc1c428e375d71d3fba` | Merged 2026-11-03 20:03:40 UTC; ancestor of snapshot |
| Fixed experimental source | `cda9135d36144d4de2771af174e52ac7b30a83c9` | Unchanged by integration or this proposal |
| Phase A | `ecd66137bacbce50e605cfc1466ec7d36131ba9e` | Manifest/discovery, not measurement permission |
| Accepted B0 reference | `91e3a7d7f6082b0397f8d6ef3d7ea8a95f3a929e` | Correctness apparatus and proposed B1 plan |
| B1 design | `64c3f71ff100ea16270f1a70497ac39860c1b772` | Explicitly DESIGN ONLY |
| R75 supplement | `7ef553d221ed668f333ff4429012ee0c9378cd13` | Author-executed scoped semantic evidence |
| Foundation | `db0e7c8b976991396eed7d14ddc1e889e9c511c2` | Correctness profile, two retained reads |
| Accounting supplement | `d355511e75885a4a91299f54ceb108eda65ac925` | Fixed-fixture collector, not campaign enforcement |
| Physical runbook | `e23d19c3ea81db572c65aeb9a8380a57cb1f97ed` | Operator instructions, not an executed host report |

[PR #694](https://github.com/t81dev/t81-foundation/pull/694) and
[PR #695](https://github.com/t81dev/t81-foundation/pull/695) metadata selections
are retained in `evidence/pr-694.json` and `evidence/pr-695.json`. The latter's
integration acceptance does not supersede the underlying documents' design-only
status or qualify measurements. In particular, B1's 30-trial/5% CV text explicitly
still requires human acceptance. No separate approved B1 preregistration was found
in the inspected BASE-01 package: B0 `proposal.json` has status `proposed`, and
`human_authorization.status` is `not_requested`. Integration is not approval of
its proposed thresholds, budgets or activation scheme.

## Source-pinned contradiction report

Every reference below is at main snapshot `a98b22fb65c80f4225151cbcd5c6934ae076cbe1`
unless the URL explicitly pins the experimental source. `evidence/source-index.json`
records complete paths, immutable URLs and SHA-256 bytes for 26 inspected sources.
The runtime files used below were also compared byte-for-byte to `cda9135d…`.

| Source | Normative or implementation fact |
| --- | --- |
| [B1 contract, lines 163–202](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/docs/research/base01/BASE01_B1_MEASUREMENT_CONTRACT.md#L163-L202) | Sizes 4,096/65,536/1,048,576 bytes, fill 0x42; 100 verified allocating reads, all outputs retained after one whole-batch interval; one fresh write store per trial; first-write fraction fixed |
| [B1 contract, lines 292–297](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/docs/research/base01/BASE01_B1_MEASUREMENT_CONTRACT.md#L292-L297) | At most three authorized pilots, deterministic count doubling; stop if timer/memory requirements cannot both fit |
| [B1 contract, lines 336–386](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/docs/research/base01/BASE01_B1_MEASUREMENT_CONTRACT.md#L336-L386) | Signed permit proposed; 16 MiB retained output per child, 1 GiB RSS observation budget, 8 GiB evidence, 60 s/trial, 30 min qualification, eight hours campaign; enforcement required, not supplied |
| [B0 preregistration record](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_lane_a/proposal.json) and [B0 plan, lines 88–142](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_lane_a/README.md#L88-L142) | Proposed 100 operations, separate write/read, allocating verified read, subsequent authorization required; proposed B1 details are not an executable campaign |
| [Actual memory driver, lines 36–71](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/fs/in_memory_driver.cpp#L36-L71) | Write hashes, assigns bytes and regenerates parity even for an existing key; read hashes stored bytes and returns a vector copy |
| [Foundation lifetime contract](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_b1_foundation/RETENTION_AND_BOUNDS.md) and [native.cpp, lines 92–125](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_b1_foundation/native.cpp#L92-L125) | Only two reads per size, both retained; capacity checked; source-derived known-buffer bounds exclude allocator/runtime overhead and do not qualify B1 counts |

Let S be payload bytes and N=100 successful reads. All-live output payload alone
is N*S; capacity and container overhead can only increase its memory accounting.

| S | Minimum retained payload | Against 16 MiB ceiling |
| ---: | ---: | --- |
| 4,096 | 409,600 bytes = 0.390625 MiB | Payload fits; full accounting still required |
| 65,536 | 6,553,600 bytes = 6.25 MiB | Payload fits; full accounting still required |
| 1,048,576 | 104,857,600 bytes = 100 MiB | Fails by 88,080,384 bytes = 84 MiB; 6.25 times ceiling |

This proof does not depend on a benchmark or allocator prediction. The 1 GiB
RSS budget is a different scope and cannot waive the retained-output ceiling.
At most 16 largest-size payloads fit even if overhead were free; choosing 16
without accounting headroom is not a defensible general bound. API transient
buffers, driver data/parity, payload input, serialization and allocator caches
must also be inventoried separately; payload retention is not total process RSS.

## Correction options

These are distinct profiles. None has measured allocator/cache effects here.
All require explicit human contract acceptance, reviewed implementation and later
separate permits. No option authorizes a pilot or weakens signature/host gates.

| Assessment | A — bounded retention | B — explicit batch lifetime | C — revised ceiling |
| --- | --- | --- | --- |
| Concrete design | N=100, retain one read result, verify and release between calls | N=100 as ten intervals of ten reads; retain each chunk, verify/release outside intervals; same preloaded store | N=100 all-live, one interval; illustrative 128 MiB output allowance only for this named cell |
| Contract consistency | Must amend all-live rule; frees inside one interval also violate destruction exclusion. Outside-interval release requires 100 intervals | Must amend all-live and whole-batch interval rules and define deterministic boundaries | Preserves API/lifetime/boundary, conflicts with global 16 MiB until approved exception; 128 MiB is not justified by source evidence |
| Semantic equivalence | Same API inputs/count/bytes possible; allocation-lifetime semantics differ | Same API inputs/count/bytes/store possible; allocation-lifetime semantics differ | Closest to original workload semantics if actual capacity fits; resource envelope changes |
| Allocation behavior | Repeated release encourages reuse after every read; timed frees would add a different cost | Each interval accumulates ten new vectors; reuse across chunks; no preallocated replacement API | Accumulates 100 live allocations; no inter-read reuse; higher cache/page pressure |
| Peak retained payload | 1 MiB largest case, plus capacity/container/transient accounting | 10 MiB largest case, plus capacity/container/transient accounting | 100 MiB minimum largest case; 128 MiB needs demonstrated capacity headroom |
| Measurement comparability | 100 interval pairs and inter-call verification alter state; no pooling with original | Ten interval pairs and chunk verification alter cache/allocator state; new profile, no pooling with original | Retains original timing definition but alters resource comparability across cells/hosts; a uniform increase changes unrelated cells unnecessarily |
| Historical evidence | B0/two-read correctness survives within its original scope; cannot qualify A | Same; two-read evidence cannot qualify ten-read chunks or future Linux behavior | Same; no historical 100-live B1 evidence establishes feasibility |
| Confounder risk | Highest interval frequency, allocator reuse, verification effects | Chunk size/release schedule, extra wrapper overhead, allocator/cache carryover | Paging/working-set pressure, capacity overshoot, changed budget admission; doubling to 200 needs still more than 128 MiB |
| Governance | Approve new output lifecycle/interval profile before engineering qualification | Approve chunk size, accounting scope and interval profile before engineering qualification | Approve scientifically justified, scoped resource exception and comparability plan; general budget increase cannot be inferred |

The research objective is characterization of actual hosted APIs with reproducible
inputs and assurance, not low-memory streaming or large-live-set performance
specifically. It does not determine which lifetime defines the intended quantity.
A/B change that quantity; C preserves it at the cost of relaxing a proposed
resource control. There is no empirical or normative basis to certify one as the
intended correction. B is fully specified as a review candidate in the amendment,
not selected as an equivalent replacement. Maintainers may instead preserve
all-live semantics and keep this cell blocked pending a justified C proposal.

## First-insertion invariant

For each size-specific write trial t, index completed timed API writes j=1..N_t.
Define E_tj=1 iff that write starts with its content key absent from its driver;
otherwise E_tj=0. Require all writes successful, and define
I_t=sum(E_tj), F_t=I_t/N_t. Count data-map absence, not allocator calls, parity
regeneration, changed bytes or read-setup writes. The source's `objects_[key]`
inserts only on first absence; it still assigns/regenerates parity thereafter.

The existing base contract implies N_t=100, I_t=1, E_t1=1 and E_tj=0 for j>1,
so F_t=1/100=1%. The domain is **each write trial at each size**: one fresh driver
and one base batch. It is not a percentage pooled across sizes, reads, warm-ups,
pilots or all trials. With unchanged N in 30 trials, aggregate fraction is also
30/3000=1%, but that equality is a consequence, not an alternative acceptance rule.

Doubling to N_t=200 on the same driver gives I_t=1, F_t=1/200=0.5%. This is a
verified mathematical consequence of the proposed doubling rule and source
lifetime, **not an observed implementation change or historical measured run**.
No executable B1 loop performing 200 writes was found. The contract explicitly
says to keep the fraction fixed but does not define an admissible doubled store
lifetime. Two fresh 100-write stores give 2/200=1%, yet change state lifetime,
allocation/cache history and interval boundaries. Preserving the ratio alone
does not preserve the workload. Warm-up on disposable state cannot supply the
missing timed insertion. Read preload inserts are outside the write denominator.

Minimum correction: explicitly freeze CanonFS writes at N_t=100 and one driver
per trial; exempt them from automatic doubling. An overhead/duration failure
leaves the cell inconclusive/blocked, requiring a separately reviewed amendment.
This preserves the stated distribution without choosing an unapproved restart
schedule. Read count changes independently affect retention; the conditional
candidate freezes reads at 100 too. It does not silently redefine global pilots.

## Compatibility and historical evidence

Keep all original contracts, hashes, source pins, reports, negative/rejected
records and receipts intact. Add the amendment as a proposed supplement with
explicit links to the clauses it would supersede only after approval. The
accepted B0 record remains correctness evidence at its original counts; the
foundation's two-read evidence, accounting fixture and scoped R75 evidence retain
their original domains. None becomes measurement evidence through this review.

Any selected A/B profile must have its own protocol identity, count/lifetime
record, binary hashes, permit bindings and qualification record. Do not combine
samples across profiles or infer timing equivalence from byte-identical outputs.
C also needs an approved resource-profile identity. If prior B1 samples are later
discovered, retain them under the original contract with violations recorded;
do not retroactively qualify or relabel them. No such samples were established
by this audit. No statistical, thermal, overhead, correctness, CV or authorization
threshold is loosened in the proposed clarification.

## Explicit maintainer decisions

1. Confirm what is accepted: B0 correctness acceptance versus unapproved B1
   design/preregistration; identify the authoritative human approval record.
2. Decide the scientific output-lifetime target. Select A/B/C with justification,
   or leave the 1 MiB read cell blocked. Candidate B is not auditor-approved.
3. Accept the per-trial 1% invariant and fixed 100-write exception, or commission
   a separate restart/lifetime design. Do not preserve only an aggregate ratio.
4. Approve explicit retained-capacity/container/transient accounting and any
   scoped budget exception. Keep logical output, RSS observation, OS enforcement
   and total evidence budgets distinct.
5. Assign independent reviewers for the selected profile, R75 ledger, runner and
   security boundary; approve trust-root enrollment, permit format, revocation
   freshness and resource/watchdog scopes through human governance.
6. Assign a physical Linux host/operator and correctness-only window separately
   from later qualification-only and campaign permits. No assignment or permit
   is supplied here. The full 14-cell scope stays required until expressly amended.

## Audit verification and limits

See [EVIDENCE.md](EVIDENCE.md) for exact commands, environment, three executed
fixture tests, initial missing-dependency failure, successful offline discovery,
static derivations and source identities. These establish a source-grounded
contradiction and narrow rejection behavior. They do not establish independent
native execution, live budget enforcement, qualified timing, physical host
readiness or comparative advantage. This audit stops at reviewable artifacts.
