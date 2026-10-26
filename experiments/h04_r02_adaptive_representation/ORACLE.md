# Oracle independence and limits

`oracle.py` derives feasible modes from integer range inequalities, computes
optimal suffix schedules, and independently reconstructs event counts. C++
uses prefix DP with backpointers. They share the declared model, so agreement
alone cannot validate its scientific assumptions.

`tests/ci/h04_r02_audit_regression_test.py` enumerates every feasible block-mode
schedule for short traces with exact rational costs, without importing either
implementation's tier-selection or optimization helpers. It independently
checks the optimum and max-over-path crossover formula. Tests also read actual
C++ JSON, compare all default ledger fields and the checked-in result artifact,
exercise cost/metadata/block/10-bit controls, and check malformed codec inputs.
The oracle-printing CTest alone is only a smoke test; the audit CTest performs
the substantive comparisons. Equal minima may have multiple optimal schedules;
small-domain checks require optimal cost and block feasibility rather than one
tie-breaking convention. Default ledger sequence parity is checked directly.

Theoretical range and packing assumptions are separately derived in
[MATHEMATICS.md](MATHEMATICS.md). No second model process or independent lab is
claimed: these are separate code paths executed in one audit session.
