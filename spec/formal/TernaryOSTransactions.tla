---- MODULE TernaryOSTransactions ----
(***************************************************************************)
(* Formal TLA+ specification for TernaryOS v0.2 transactional isolation    *)
(* and Axion Governor capability-budget enforcement over 729-byte          *)
(* CanonStore blocks (TERNARYOS_ARCHITECTURE_v0.2.md,                      *)
(* TERNARYOS_MULTI_AGENT_CAPABILITY_BUDGET_v0.2.md).                       *)
(*                                                                         *)
(* Models concurrent agents executing TxBegin, TxRead, TxWrite (over       *)
(* balanced-ternary trits {-1, 0, +1}), TxCommit, and TxAbort with:        *)
(*   - Fail-closed capability budgets (AXHALT when budget <= 0)            *)
(*   - Snapshot isolation & atomic multi-key commit with zero orphans      *)
(*   - Deterministic canonical agent-ID conflict resolution                *)
(***************************************************************************)

EXTENDS Integers, FiniteSets

CONSTANTS
    Agents,     \* Finite set of ordered integer agent IDs, e.g. {1, 2}
    Keys,       \* Finite set of CanonStore block keys, e.g. {"k1", "k2"}
    MaxBudget   \* Maximum per-agent mutation capability budget, e.g. 2

ASSUME
    /\ IsFiniteSet(Agents)
    /\ Agents # {}
    /\ \A a \in Agents : a \in Nat /\ a >= 1
    /\ IsFiniteSet(Keys)
    /\ Keys # {}
    /\ MaxBudget \in Nat /\ MaxBudget >= 1

Trits == {-1, 0, 1}
TxStates == {"idle", "active", "committed", "aborted"}
Planes == {"deterministic", "stochastic", "hybrid"}
MaxEpoch == MaxBudget * Cardinality(Agents)

VARIABLES
    canonStore,         \* [Keys -> Trits]: committed 729-byte CanonStore blocks
    epoch,              \* 0..MaxEpoch: monotonic committed epoch counter
    txState,            \* [Agents -> TxStates]: per-agent transaction lifecycle
    txSnapshot,         \* [Agents -> [Keys -> Trits]]: isolated snapshot at TxBegin
    txSnapEpoch,        \* [Agents -> 0..MaxEpoch]: parent epoch captured at TxBegin
    txWriteSet,         \* [Agents -> SUBSET Keys]: staged dirty keys
    txStage,            \* [Agents -> [Keys -> Trits]]: isolated staging buffer
    budget,             \* [Agents -> 0..MaxBudget]: remaining capability budget
    lastCommitBudget,   \* [Agents -> 0..MaxBudget]: budget immediately prior to commit
    lastConflictWinner, \* Agents \cup {0}: canonical winner of last write-write conflict
    txPlane,            \* [Agents -> Planes]: active execution plane
    txReconciled        \* [Agents -> BOOLEAN]: TRUE iff deterministic or passed Hybrid gate

vars == <<canonStore, epoch, txState, txSnapshot, txSnapEpoch,
          txWriteSet, txStage, budget, lastCommitBudget, lastConflictWinner,
          txPlane, txReconciled>>

-----------------------------------------------------------------------------
(* Canonical conflict helpers *)

HasLowerConflictingPeer(a) ==
    \E p \in Agents :
        /\ p < a
        /\ txState[p] = "active"
        /\ (txWriteSet[p] \cap txWriteSet[a]) # {}

LowestConflictingPeer(a) ==
    CHOOSE p \in Agents :
        /\ p < a
        /\ txState[p] = "active"
        /\ (txWriteSet[p] \cap txWriteSet[a]) # {}
        /\ \A q \in Agents :
            (q < a /\ txState[q] = "active" /\ (txWriteSet[q] \cap txWriteSet[a]) # {})
            => p <= q

-----------------------------------------------------------------------------
(* Initial state: all CanonStore blocks inert (0), budgets at MaxBudget *)

Init ==
    /\ canonStore = [k \in Keys |-> 0]
    /\ epoch = 0
    /\ txState = [a \in Agents |-> "idle"]
    /\ txSnapshot = [a \in Agents |-> [k \in Keys |-> 0]]
    /\ txSnapEpoch = [a \in Agents |-> 0]
    /\ txWriteSet = [a \in Agents |-> {}]
    /\ txStage = [a \in Agents |-> [k \in Keys |-> 0]]
    /\ budget = [a \in Agents |-> MaxBudget]
    /\ lastCommitBudget = [a \in Agents |-> 0]
    /\ lastConflictWinner = 0
    /\ txPlane = [a \in Agents |-> "deterministic"]
    /\ txReconciled = [a \in Agents |-> TRUE]

-----------------------------------------------------------------------------
(* Transaction operations *)

TxBegin(a) ==
    /\ txState[a] \in {"idle", "committed", "aborted"}
    /\ txState' = [txState EXCEPT ![a] = "active"]
    /\ txSnapshot' = [txSnapshot EXCEPT ![a] = canonStore]
    /\ txSnapEpoch' = [txSnapEpoch EXCEPT ![a] = epoch]
    /\ txWriteSet' = [txWriteSet EXCEPT ![a] = {}]
    /\ txStage' = [txStage EXCEPT ![a] = canonStore]
    /\ txPlane' = [txPlane EXCEPT ![a] = "deterministic"]
    /\ txReconciled' = [txReconciled EXCEPT ![a] = TRUE]
    /\ UNCHANGED <<canonStore, epoch, budget, lastCommitBudget, lastConflictWinner>>

TxBeginStochastic(a) ==
    /\ txState[a] \in {"idle", "committed", "aborted"}
    /\ txState' = [txState EXCEPT ![a] = "active"]
    /\ txSnapshot' = [txSnapshot EXCEPT ![a] = canonStore]
    /\ txSnapEpoch' = [txSnapEpoch EXCEPT ![a] = epoch]
    /\ txWriteSet' = [txWriteSet EXCEPT ![a] = {}]
    /\ txStage' = [txStage EXCEPT ![a] = canonStore]
    /\ txPlane' = [txPlane EXCEPT ![a] = "stochastic"]
    /\ txReconciled' = [txReconciled EXCEPT ![a] = FALSE]
    /\ UNCHANGED <<canonStore, epoch, budget, lastCommitBudget, lastConflictWinner>>

TxHybridReconcile(a) ==
    /\ txState[a] = "active"
    /\ txPlane[a] = "stochastic"
    /\ txPlane' = [txPlane EXCEPT ![a] = "hybrid"]
    /\ txReconciled' = [txReconciled EXCEPT ![a] = TRUE]
    /\ UNCHANGED <<canonStore, epoch, txState, txSnapshot, txSnapEpoch,
                   txWriteSet, txStage, budget, lastCommitBudget, lastConflictWinner>>

TxRead(a, k) ==
    /\ txState[a] = "active"
    /\ LET observed == IF k \in txWriteSet[a] THEN txStage[a][k] ELSE txSnapshot[a][k]
       IN observed \in Trits
    /\ UNCHANGED vars

TxWrite(a, k, tritVal) ==
    /\ txState[a] = "active"
    /\ tritVal \in Trits
    /\ budget[a] > 0
    /\ txStage' = [txStage EXCEPT ![a][k] = tritVal]
    /\ txWriteSet' = [txWriteSet EXCEPT ![a] = @ \cup {k}]
    /\ UNCHANGED <<canonStore, epoch, txState, txSnapshot, txSnapEpoch,
                   budget, lastCommitBudget, lastConflictWinner, txPlane, txReconciled>>

TxCommit(a) ==
    /\ txState[a] = "active"
    /\ IF ~txReconciled[a] THEN
           \* ERR_PLANE_BYPASS_DENIED: stochastic plane cannot mutate CanonStore without Hybrid gate
           /\ txState' = [txState EXCEPT ![a] = "aborted"]
           /\ txWriteSet' = [txWriteSet EXCEPT ![a] = {}]
           /\ txStage' = [txStage EXCEPT ![a] = canonStore]
           /\ UNCHANGED <<canonStore, epoch, txSnapshot, txSnapEpoch,
                          budget, lastCommitBudget, lastConflictWinner, txPlane, txReconciled>>
       ELSE IF budget[a] <= 0 THEN
           \* AXHALT fail-closed: zero budget cannot commit or mutate CanonStore
           /\ txState' = [txState EXCEPT ![a] = "aborted"]
           /\ txWriteSet' = [txWriteSet EXCEPT ![a] = {}]
           /\ txStage' = [txStage EXCEPT ![a] = canonStore]
           /\ UNCHANGED <<canonStore, epoch, txSnapshot, txSnapEpoch,
                          budget, lastCommitBudget, lastConflictWinner, txPlane, txReconciled>>
       ELSE IF txSnapEpoch[a] # epoch THEN
           \* Stale parent epoch (optimistic concurrency conflict -> fail-closed abort)
           /\ txState' = [txState EXCEPT ![a] = "aborted"]
           /\ txWriteSet' = [txWriteSet EXCEPT ![a] = {}]
           /\ txStage' = [txStage EXCEPT ![a] = canonStore]
           /\ UNCHANGED <<canonStore, epoch, txSnapshot, txSnapEpoch,
                          budget, lastCommitBudget, lastConflictWinner, txPlane, txReconciled>>
       ELSE IF HasLowerConflictingPeer(a) THEN
           \* Deterministic canonical agent-ID ordering: lower ID peer wins
           /\ txState' = [txState EXCEPT ![a] = "aborted"]
           /\ txWriteSet' = [txWriteSet EXCEPT ![a] = {}]
           /\ txStage' = [txStage EXCEPT ![a] = canonStore]
           /\ lastConflictWinner' = LowestConflictingPeer(a)
           /\ UNCHANGED <<canonStore, epoch, txSnapshot, txSnapEpoch,
                          budget, lastCommitBudget, txPlane, txReconciled>>
       ELSE IF txWriteSet[a] = {} THEN
           \* Read-only commit: no CanonStore mutation and no budget charge
           /\ txState' = [txState EXCEPT ![a] = "committed"]
           /\ lastCommitBudget' = [lastCommitBudget EXCEPT ![a] = budget[a]]
           /\ UNCHANGED <<canonStore, epoch, txSnapshot, txSnapEpoch,
                          txWriteSet, txStage, budget, lastConflictWinner, txPlane, txReconciled>>
       ELSE
           \* Atomic multi-key commit to CanonStore + deterministic budget decrement
           /\ epoch < MaxEpoch
           /\ canonStore' = [k \in Keys |->
                  IF k \in txWriteSet[a] THEN txStage[a][k] ELSE canonStore[k]]
           /\ epoch' = epoch + 1
           /\ budget' = [budget EXCEPT ![a] = @ - 1]
           /\ lastCommitBudget' = [lastCommitBudget EXCEPT ![a] = budget[a]]
           /\ txState' = [txState EXCEPT ![a] = "committed"]
           /\ txWriteSet' = [txWriteSet EXCEPT ![a] = {}]
           /\ txStage' = [txStage EXCEPT ![a] = canonStore']
           /\ UNCHANGED <<txSnapshot, txSnapEpoch, lastConflictWinner, txPlane, txReconciled>>

TxAbort(a) ==
    /\ txState[a] = "active"
    /\ txState' = [txState EXCEPT ![a] = "aborted"]
    /\ txWriteSet' = [txWriteSet EXCEPT ![a] = {}]
    /\ txStage' = [txStage EXCEPT ![a] = canonStore]
    /\ UNCHANGED <<canonStore, epoch, txSnapshot, txSnapEpoch,
                   budget, lastCommitBudget, lastConflictWinner, txPlane, txReconciled>>

\* Wave 4: Level-parallel DPE worker batch commit of pairwise-disjoint write-sets
\* from active reconciled agents on the current epoch with positive capability budgets.
DpeBatchCommit(S) ==
    /\ S \subseteq Agents
    /\ Cardinality(S) >= 2
    /\ epoch + Cardinality(S) <= MaxEpoch
    /\ \A a \in S :
        /\ txState[a] = "active"
        /\ txReconciled[a]
        /\ txSnapEpoch[a] = epoch
        /\ budget[a] > 0
        /\ txWriteSet[a] # {}
    /\ \A a, b \in S : a # b => (txWriteSet[a] \cap txWriteSet[b]) = {}
    /\ canonStore' = [k \in Keys |->
           IF \E a \in S : k \in txWriteSet[a]
           THEN txStage[CHOOSE a \in S : k \in txWriteSet[a]][k]
           ELSE canonStore[k]]
    /\ epoch' = epoch + Cardinality(S)
    /\ budget' = [a \in Agents |-> IF a \in S THEN budget[a] - 1 ELSE budget[a]]
    /\ lastCommitBudget' = [a \in Agents |-> IF a \in S THEN budget[a] ELSE lastCommitBudget[a]]
    /\ txState' = [a \in Agents |-> IF a \in S THEN "committed" ELSE txState[a]]
    /\ txWriteSet' = [a \in Agents |-> IF a \in S THEN {} ELSE txWriteSet[a]]
    /\ txStage' = [a \in Agents |-> IF a \in S THEN canonStore' ELSE txStage[a]]
    /\ UNCHANGED <<txSnapshot, txSnapEpoch, lastConflictWinner, txPlane, txReconciled>>

Next ==
    \/ (\E a \in Agents :
            \/ TxBegin(a)
            \/ TxBeginStochastic(a)
            \/ TxHybridReconcile(a)
            \/ \E k \in Keys : TxRead(a, k)
            \/ \E k \in Keys, v \in Trits : TxWrite(a, k, v)
            \/ TxCommit(a)
            \/ TxAbort(a))
    \/ (\E S \in SUBSET Agents : DpeBatchCommit(S))

Spec == Init /\ [][Next]_vars

-----------------------------------------------------------------------------
(* Safety Invariants *)

TypeOK ==
    /\ canonStore \in [Keys -> Trits]
    /\ epoch \in 0..MaxEpoch
    /\ txState \in [Agents -> TxStates]
    /\ txSnapshot \in [Agents -> [Keys -> Trits]]
    /\ txSnapEpoch \in [Agents -> 0..MaxEpoch]
    /\ txWriteSet \in [Agents -> SUBSET Keys]
    /\ txStage \in [Agents -> [Keys -> Trits]]
    /\ budget \in [Agents -> 0..MaxBudget]
    /\ lastCommitBudget \in [Agents -> 0..MaxBudget]
    /\ lastConflictWinner \in Agents \cup {0}
    /\ txPlane \in [Agents -> Planes]
    /\ txReconciled \in [Agents -> BOOLEAN]

\* Three-Plane Non-Bypass Invariant: no stochastic agent can commit into
\* committed CanonStore state without traversing Hybrid reconciliation.
PlaneTransitionInvariant ==
    \A a \in Agents :
        /\ (txPlane[a] = "stochastic" => ~txReconciled[a])
        /\ (txState[a] = "committed" => txReconciled[a])

\* No agent can commit a transaction if its capability budget was <= 0 (AXHALT).
FailClosedBudgetInvariant ==
    \A a \in Agents :
        /\ budget[a] >= 0
        /\ budget[a] <= MaxBudget
        /\ (txState[a] = "committed" => lastCommitBudget[a] > 0)
        /\ (budget[a] = 0 => txWriteSet[a] = {})

\* Uncommitted writes are never visible to concurrent transactions;
\* non-active transactions leave zero orphan staged writes.
SnapshotIsolationAndAtomicity ==
    \A a \in Agents :
        /\ (txState[a] = "active" /\ txSnapEpoch[a] = epoch)
           => txSnapshot[a] = canonStore
        /\ (txState[a] # "active") => txWriteSet[a] = {}

\* Write-write conflicts resolve strictly via canonical agent ordering
\* (never randomized or non-deterministic).
DeterministicConflictResolution ==
    /\ (lastConflictWinner # 0 => lastConflictWinner \in Agents)
    /\ \A a \in Agents :
        (txState[a] = "active" /\ HasLowerConflictingPeer(a))
        => LowestConflictingPeer(a) < a

====
