# S02 scientific-integrity verification

**Decision: REVISE. H2: UNTESTED.** Original S02 claims/results are superseded scientific evidence. The active report and tables are regenerated from the independently audited corrected pilot.

## All five review findings reproduced and repaired

| Finding on PR #582 | Before correction: actual witness | Correction and regression |
| --- | --- | --- |
| [Cache identity](https://github.com/t81dev/t81-foundation/pull/582#discussion_r4381908354) | Same-size field change returns [-1,-1] instead of [-1,-2]. Original N8 seed2 oracle is -9.554696935154245, but stored reference is seed1's -8.37614. | Complete finite h/J/N/alphabet key; field and coupling perturbations, multiple alphabets, and all fifteen regenerated problem references verified. |
| [Changing task](https://github.com/t81dev/t81-foundation/pull/582#discussion_r4381908365) | Fixed task evaluated as -1, then 0 after edge removal, then -2 after an added computational edge. | Const task deep copy plus separate network adjacency/couplings. The same witness now remains [-1,-1,-1]; raw state trajectories are independently evaluated against immutable h/J. |
| [Biased draws](https://github.com/t81dev/t81-foundation/pull/582#discussion_r4381908375) | Both binary streams alternate. All 64 tested triples at N8/N16 force u/v to opposite parities and u/w to the same parity. | C++/Python SplitMix64, rejection-based integer reduction, separate task/noise domains; golden words, forced rejection, small-word residue counts and parity controls pass. |
| [Nonreproducible table](https://github.com/t81dev/t81-foundation/pull/582#discussion_r4381908388) | Original stored N16/T0.5 A error=2.191556, comm=599.2, rather than report 1.12/598.6; central stored error=1.42773 and cost=769, rather than 0.88/770.6. These original numbers are invalid hypothesis evidence even where internally reproduced. | analyze_results.py audits data before deriving every active table and paired effect. Independent raw-row reductions check all displayed numeric cells; --check rejects stale report/analysis. |
| [Missing capacity comparison](https://github.com/t81dev/t81-foundation/pull/582#discussion_r4381908399) | Runner never enables matching, while report claims a matched H2 outcome and 18% step penalty. | Explicit H2 UNTESTED metadata/text; capacity flag rejects dummy-node construction; no unsupported superiority/no-advantage/convergence claim. |

Actual before-run evidence: evidence/before/five-findings.json and stderr. Five methods produced six failing assertions/subtests and zero errors because the integer-sampling method has C++ and Python subtests. Failures reproduce defects; they are not counted as valid experimental results. Preserved source and report bytes are independently compared to the merge commit, not only to a self-written manifest.

## Corrected execution and controls

The recorded Release CTest suite passes **6/6 targets**, zero failures/skips:

- S02 C++ controls: 1026 active assertions, including cache coefficient identity, immutable task/network separation, invalid shapes/nonfinite inputs, unsupported matching, initial step-0 hit, edge-edit accounting and RNG vectors/rejection. Assertions remain active under NDEBUG.
- Fresh runner/Python oracle audit: all 1260 rows, 315 trajectories, 63315 states and 15 immutable problems checked. Maximum observed reference difference is 0 on this recorded platform.
- Python sampling/theorem controls: 8 methods, including tiny independent ternary enumeration and fixed-graph Metropolis invariance.
- Five original finding regressions: all five pass.
- Reporting/preservation controls: 5 methods check every table cell, repricing, explicit omissions/H2, original-file hashes and frozen-rubric bytes.
- Committed report/data reproducibility: full audit and exact regeneration of analysis/report agree.

Machine evidence: evidence/after/ctest.xml/stdout, oracle-audit.json, verification.json, source/binary/data hashes in evidence/generation.json, and exact configure/build commands. No production runtime test, hardware benchmark or native Linux/Windows certification is inferred. The current main CI snapshot is baseline evidence only; its pre-existing failures are not suppressed.

## Scope and acceptance boundary

The original full vertex grid has 20160 price cells. The corrected subset has 1260 (6.25%) and explicitly omits 18900; the generated report lists all missing values. The wider original field interval and the capacity comparison are not tested. The seven-arm centralized control is accurately labeled a ranking heuristic; the cost remains a partial logical-operation proxy. The frozen rubric's 20% full-cost/confidence threshold and planning/power requirements remain unchanged and unmet/not evaluable. Consequently these controls establish repaired measurement validity for this pilot, **not acceptance or rejection of the architectural hypotheses**. The revised decision is REVISE; no S03 or additional research cycle is started.
