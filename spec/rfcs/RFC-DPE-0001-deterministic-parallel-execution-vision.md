# RFC-DPE-0001: Deterministic Parallel Execution — Vision and Motivation

**Status:** accepted
**Type:** informational
**Applies-To:** T81VM runtime, Axion kernel scheduler, TISC execution model, CanonFS
**Created:** 2026-03-14
**Updated:** 2026-03-14
**Author:** @t81dev
**Depends on:** RFC-0002 (Deterministic Execution Contract), RFC-00B1 (Ternary MMU), RFC-00B3 (Axion Governance Kernel Architecture)
**Superseded-By:** —
**See also:** RFC-DPE-0002 (TISC Task Graph Primitives), RFC-DPE-0003 (Epoch Execution and Canonical Commit)

---

## 1. Summary

This document describes the motivation and high-level architecture for
deterministic parallel execution in the T81 ecosystem.  It establishes the
vocabulary and principles used by the normative companion RFCs.

**This RFC is informational.**  It does not define wire formats, opcode
assignments, or binding algorithmic rules.  Those are specified in
RFC-DPE-0002 (task graph primitives) and RFC-DPE-0003 (epoch execution and
canonical commit).

The core invariant this model must preserve:

```
same program + same canonical state → identical results everywhere
```

---

## 2. Motivation

The T81 architecture defines determinism as a first-class execution invariant.
Naive parallelism breaks that invariant through:

- thread scheduling order
- shared writable memory races
- instruction reordering across cores
- non-canonical reduction ordering
- timing differences across processors or machines

Traditional approaches (mutexes, locks, barriers) suppress data races but do
not guarantee deterministic outcomes across machines or across time.

A structured deterministic parallel model is required to support:

- multi-core execution on the T81VM
- scalable AI inference with reproducible outputs
- deterministic distributed scientific computation
- verifiable audit trails for high-assurance environments

---

## 3. Design Principles

Any conforming implementation of deterministic parallel execution must satisfy
the following principles.  The normative definitions of compliance are in
RFC-DPE-0002 and RFC-DPE-0003.

### P1 — Deterministic State Evolution

All visible machine state must evolve according to a canonical ordering
independent of runtime scheduling.

### P2 — Immutable Input State

Parallel tasks operate on immutable inputs taken from the epoch's canonical
snapshot.  No task may observe another task's in-flight deltas.

### P3 — Canonical Commit Ordering

Results produced in parallel are merged into global state in a
predetermined deterministic order defined by RFC-DPE-0003.

### P4 — Replayability

Every epoch's execution and commit must be fully replayable from the
epoch's input snapshot and task graph descriptor.

### P5 — Governance Compatibility

Parallel execution must remain compatible with Axion policy enforcement.
Task creation, resource budgets, and privileged opcode access are all
subject to the existing capability model.

---

## 4. Deterministic Task Graph Model (Overview)

Parallel work is expressed as a **task graph** rather than arbitrary threads.

Each node (task) is a deterministic computation unit defined by:

- a TISC instruction sequence (its program)
- a set of immutable input references (CanonFS refs or epoch snapshot ranges)
- a declared set of output regions (TVA ranges whose writes are delta-buffered)
- a set of dependency edges to predecessor tasks

Tasks without dependency conflicts may execute concurrently on any number of
cores.  The scheduler is free to order execution in any way that respects
dependency constraints; final results are independent of scheduling order
because all inputs are immutable and outputs are committed canonically.

The normative task descriptor format is defined in RFC-DPE-0002 §3.

---

## 5. Epoch-Based Execution (Overview)

Parallel execution occurs within **epochs**.  An epoch is a deterministic
computation frame:

```
Epoch N
  snapshot canonical state (read-only baseline)
  dispatch task graph (tasks run against snapshot)
  buffer all task output deltas
  commit deltas to canonical state in deterministic order
Epoch N+1 (sees committed result of Epoch N)
```

No task may mutate global canonical state during execution.  All writes go to
per-task delta buffers and are merged at commit time under the ordering rule
defined in RFC-DPE-0003 §2.

---

## 6. Deterministic Reduction Trees

Parallel reductions must follow canonical reduction trees to preserve
bit-exact results regardless of execution order.

Example — canonical sum of four T81Float values:

```
(a + b) + (c + d)     ← canonical pairing by index
```

not

```
((a + c) + b) + d     ← scheduling-order pairing (non-canonical)
```

The canonical tree structure is determined by the task graph topology.
Implementations must not reorder reduction steps beyond what the declared
dependency edges permit.

This requirement interacts with `T81_STRICT_DETERMINISTIC_FLOAT`; see
RFC-DPE-0003 §5.

---

## 7. Governance Integration

Axion policy enforcement applies to all epoch and task lifecycle events:

- task graph submission (requires capability)
- resource budgets (max parallel task count, memory footprint per epoch)
- privileged opcode access within tasks
- model weight loading (`TLOADHASH` policy remains active inside tasks)
- epoch commit (delta size limits)

Policy evaluation must itself be deterministic and bounded; policies may not
introduce non-determinism into the commit ordering.

---

## 8. Distributed Execution (Informational)

The deterministic task graph model extends naturally to distributed clusters
when each node receives an identical epoch snapshot and task graph descriptor.
Independent nodes may compute disjoint task subsets; final outputs can be
verified through deterministic replay of the epoch.

The replication mechanism for distributing the epoch snapshot and task graph
is out of scope for the current RFC series.  Distributed execution is noted
here as a long-term architectural direction.

---

## 9. Known Gaps (Addressed by Companion RFCs)

The following issues are intentionally left open in this document and
resolved normatively in RFC-DPE-0002 and RFC-DPE-0003:

| Gap | Resolved in |
|---|---|
| Task descriptor wire format and task ID assignment | RFC-DPE-0002 §3–4 |
| TISC-level output region declaration | RFC-DPE-0002 §5 |
| Dependency edge encoding | RFC-DPE-0002 §6 |
| Canonical commit ordering rule (exact algorithm) | RFC-DPE-0003 §2 |
| Delta conflict resolution | RFC-DPE-0003 §3 |
| Interaction with Axion pager/fault model | RFC-DPE-0003 §4 |
| CanonHash81 epoch-level verification | RFC-DPE-0003 §5 |
| Epoch abort and retry semantics | RFC-DPE-0003 §6 |

---

## 10. Open Questions (Future RFCs)

- Deterministic GPU acceleration (requires separate RFC)
- Optimal epoch sizing and adaptive epoch splitting
- Distributed task graph synchronization protocol
- VM trace compression for epoch replay storage
