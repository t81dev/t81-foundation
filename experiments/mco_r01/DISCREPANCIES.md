# Discrepancy ledger

No frozen specification/corpus contradiction was found. No corpus annotation,
raw byte, identity, or expected transition was changed to obtain agreement.

1. The first independent Ruby implementation agreed on all 60 cases before
   either candidate was implemented or executed. The original source and actual
   output are retained as `evidence/oracle-first.rb`, `oracle-first.json`, and
   `oracle-first-phase.json`. This is local development provenance, not an
   independently timestamped attestation of authorship or oracle independence.
2. Extending that implementation to all 24 event plans initially produced four
   projection mismatches: case journals contain `Decision` at sequence 0, while
   the frozen event-only table numbers its first event 0. The implementation
   removed `Decision` but initially retained sequence numbers 1–4. The preserved
   `initial-plan-projection-failure.json` contains the full actual mismatch.
   Reindexing the event-only view fixed the adapter; decisions, state, ordering,
   and frozen expectations were unchanged. The runner now gates candidate
   execution on both the cases and plans. This was an implementation defect,
   not a reason to rewrite the preregistration or stop for a spec contradiction.
3. Current full runs have zero oracle disagreements, candidate disagreements,
   algebra disagreements, reconstruction failures, or undetected required
   mutants. Intentionally mutated and forged observations remain in the
   evidence archives, alongside their independent semantic rejection reports.

Unresolved scientific limits: shared honest owner; finite synthetic inputs;
single-owner atomic check/append assumed by sequential execution; trusted
context and source checkout; no hostile-host attestation; no real-model,
concurrency, timing, resource-exhaustion, or production isolation experiment.
The arms and oracle also share the written specification and fixture universe;
cross-language implementation reduces some correlated errors, not all of them.
Ruby's schema interpreter covers the pinned vocabulary only. Portability has
been executed on macOS ARM64 only. These are limits, not silently passed tests.
