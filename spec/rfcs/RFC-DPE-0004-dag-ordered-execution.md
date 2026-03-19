# RFC-DPE-0004: DAG-Ordered Multi-Task Epoch Execution

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81VM runtime, DpeTaskRunner, axion_kernel_submit_epoch
**Created:** 2026-03-14
**Updated:** 2026-03-14
**Author:** @t81dev
**Depends on:** RFC-DPE-0002 (TISC Task Graph Primitives), RFC-DPE-0003 (Epoch Execution and Canonical Commit)
**Blocks:** distributed epoch verification, multi-stage AI inference pipelines

---

## 1. Summary

RFC-DPE-0002 defines the task dependency graph and validates it as a DAG via
`accept_epoch()`, but the execution engine (RFC-DPE-0003) runs tasks in
array order and ignores `dep_task_ids`.  This RFC specifies:

- the topological execution order rule (§2)
- predecessor delta propagation — how output pages from one task become
  input pages for dependent tasks (§3)
- the `DpeTaskInputSnapshot` type (§4)
- interaction with the canonical commit algorithm (§5)
- acceptance criteria (§6)

---

## 2. Topological Execution Order

### 2.1 Rule

Tasks in an epoch **must execute in topological order** of their dependency
graph: for any edge T_dep → T_succ (T_dep must precede T_succ), T_dep
completes before T_succ begins.

The topological order is derived from `TaskDescriptor::dep_task_ids`, which
contain the **program-identity TaskIds** of predecessor tasks
(RFC-DPE-0002 §4).  `topological_sort_epoch()` implements Kahn's algorithm
on the program-identity index and returns task indices in a valid execution
order.

### 2.2 Determinism

The topological sort must be deterministic.  When multiple tasks have no
remaining unsatisfied dependencies (tie), they are ordered by their
**canonical TaskId** (ascending lexicographic on the 32-byte hash), matching
the canonical commit ordering rule from RFC-DPE-0003 §2.1.

### 2.3 Independent tasks

Tasks with no `dep_task_ids` entries (independent tasks) may appear anywhere
in the topological order relative to other independent tasks.  The
deterministic tie-breaking rule in §2.2 resolves their relative position.

---

## 3. Predecessor Delta Propagation

### 3.1 Input snapshot construction

Before task T executes, the execution engine constructs a
`DpeTaskInputSnapshot` for T:

```
input_snapshot(T) = ∅
for each dep_id in T.dep_task_ids:
    dep_task = resolve(dep_id, epoch)  // program-identity lookup
    for each DeltaRecord R in dep_task.delta_records:
        input_snapshot(T)[R.tva] ← R.value
```

If multiple predecessors of T write to the same TVA, the value from the
predecessor with the **higher TaskId** (canonical order) wins — consistent
with RFC-DPE-0003 §3.1.

### 3.2 Input snapshot loading

The execution engine writes each page in `input_snapshot(T)` into the VM's
memory at the corresponding word position before T's program runs.  This is
performed via `IVirtualMachine::set_memory_word()`.

Word position derivation (hosted simulation):

```
word_start(tva) = tva   // for single-page output regions where tva == base_tva
```

Multi-page region support (tva ≠ base_tva for page p > 0) is deferred to a
future slice; the current implementation covers single-page output regions.

### 3.3 Scope of input propagation

Only direct predecessor deltas are propagated — not transitive predecessors.
Transitive inputs are already encoded in predecessor commit results (a
predecessor that itself received inputs will have those values reflected in
its delta records).

---

## 4. DpeTaskInputSnapshot

```cpp
struct DpeTaskInputSnapshot {
  // key = word start in State::memory (= tva for single-page regions)
  std::map<uint64_t, std::array<std::byte, kDpePageSize>> pages{};
};
```

`DpeTaskRunner::run_direct()` accepts an optional `DpeTaskInputSnapshot`.
When non-empty, each page is written into VM memory (via
`IVirtualMachine::set_memory_word()`) after `load_program()` and before the
output-region pre-snapshot is taken.

---

## 5. Interaction with Canonical Commit

DAG-ordered execution does not change the canonical commit algorithm.
RFC-DPE-0003 §2 commits deltas in TaskId-ascending order, independent of
execution order.  The EpochHash therefore remains deterministic regardless
of the DAG shape.

Tasks that received input snapshots will have output deltas that reflect
those inputs.  These deltas are committed in the same canonical order as
tasks without dependencies.

---

## 6. Acceptance Criteria

- `[DPE-04-01]` A two-task chain T0 → T1: T0 writes value V to page P;
  T1's input snapshot contains V at page P before T1 runs.
- `[DPE-04-02]` T1 reads V from its input memory, transforms it to V', and
  commits V' to the canonical state.  The final committed page at TVA P
  contains V', not V.
- `[DPE-04-03]` The same two-task chain submitted with tasks in reversed
  array order (T1 first, T0 second) produces identical committed state —
  proving execution order follows the DAG, not array order.
- `[DPE-04-04]` An independent task (no deps) is unaffected by the
  topological ordering change; its committed result is identical to the
  pre-DPE-0004 single-task case.

---

## 7. Open Questions

- Multi-page region TVA→word_start derivation (currently deferred; requires
  base_tva encoding in DeltaRecord or a separate mapping table).
- Parallel task execution within a topological level (currently sequential;
  future slice).

---

## 8. Acceptance Notes (2026-03-14)

`[DPE-04-01]` through `[DPE-04-04]` proved by `t81_dpe_epoch_dag_test`
(Slice 18).

| Criterion | Evidence | Status |
| :--- | :--- | :--- |
| `[DPE-04-01]` Input snapshot received by dependent task | `test_dag_predecessor_value_flows_to_successor` — T1 reads T0's written value from pre-populated input memory | met |
| `[DPE-04-02]` Dependent task transforms and commits correct value | `test_dag_predecessor_value_flows_to_successor` — committed page reflects T1's transformed value (42 → 43) | met |
| `[DPE-04-03]` Reversed array order produces identical committed state | `test_dag_array_order_irrelevant` — T1-first, T0-second array order produces same committed page as T0-first | met |
| `[DPE-04-04]` Independent task unaffected | `test_independent_task_unaffected_by_topo_sort` — single independent task produces correct result | met |
