# H04-R02 integration evidence inventory

Inventory established before implementation, 2026-10-25 America/New_York.

- Initial checkout: `corrective/pr560-witness` at `e99c34cb`; the sole untracked
  entry was `build-bench/`. Neither that branch nor its files were edited.
- Fetched main: `01c85019319fd020922402ba356878e392cb2231` (merged #569).
  #568 is merged, source head `84972dd61a231c51defb4e85289c7c879d88d598`.
- #570 open: `8a49ba1d89f1d2f43ea1bdd654cc15e5f2f79ecd`, five commits:
  `0e5de46d`, `1bf72369`, `f7015437`, `2f2446af`, `8a49ba1d`.
  The last commit has an empty tree diff. The fetched PR ref briefly pointed
  at `2f2446af`; the fetched named branch and live API establish the newer head.
- #571 open: `b2918551a676762c401f492d2cc591bab45c2488`, one commit, changes
  RESULTS/HANDOFF only. Complete diffs, commits, changed-file lists, review
  submissions and inline findings were inspected. Live records are in `evidence/`.
- #570 review findings: incorrect 10-bit feasibility inference and an arithmetic
  test that failed to exercise the executable. Later commits fix executable
  crossover arithmetic and add output checks, but retain unsupported conclusions.
- #571 review findings: confusing 8/6 with 10/6, and claiming 11.6 exceeds 12.
  The review's 1.25 threshold describes the historical scaled-header model;
  the repaired equal-header model gives 4/3 for the 8-bit constant-small arm.
- Authoritative mission and H01–H08 definitions:
  `docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md`.
  Register v1 is a historical snapshot, not a current implementation inventory.
- `def47c680f477bb1a7bb10ebb57e11a0c7b24a6f` exists in the local Git object
  database and is reachable from `research/h04-r01-trace-ledger` and
  `research/h04-r02-symmetric-modes`. It contains source, independent verifier,
  tests, data and a versioned register. Absence from main/remote ancestry does
  not establish nonexistence. A source archive is preserved in `evidence/`.
- Isolated integration branch: `research/h04-r02-independent-integration`,
  based on fetched main. Scope: research packet, tests, append-only handoffs;
  no ISA, VM, Axion, CanonFS or root README changes.
