# S02 corrected research report

**Decision: REVISE.** The original STOP verdict and numerical claims are superseded. The corrected bounded pilot supports descriptive comparisons of the implemented heuristics; it does not establish or falsify a general ternary/stochastic/adaptive architectural advantage.

**H2 — UNTESTED**. All runs are vertex-matched. No dummy-node or capacity-matched result is claimed. H1 and H3 have descriptive pilot observations only; the unchanged rubric does not permit a confirmatory architecture verdict from this scope.

## Verified experiment and objective

There are 15 immutable problem instances (five seeds at each of N=8,12,16), 315 simulated trajectories and 1260 cost-repriced rows. Each trajectory has its initial state plus 200 post-proposal states; all 63315 states are audited with a separate Python task-energy evaluator. Every row uses its own coefficient-specific exact ground reference. Maximum observed C++/Python reference difference: 0. This is the observed local result, not a cross-platform certification.

The task Hamiltonian uses the original problem h/J throughout. Computational edges/couplings may change proposal dynamics but cannot redefine success. Adaptive arms are nonequilibrium optimization heuristics, not equilibrium samplers of the task distribution. Ground-state occupancy is not ESS or a count of independent samples. The ternary ground reference equals the binary endpoint minimum for this multi-affine Hamiltonian; tiny independent 3^N enumeration controls check that theorem. It says nothing about comparative dynamics or H2.

## Quantitative table generated from data

Same slice as the original report: N=16, T=0.5, five problem seeds. Gaps are final immutable-task energy minus the correct per-instance optimum. Cost is the declared partial software proxy, not measured runtime/energy. Costs 0.5 and 5 reprice identical trajectories.

| Arm | Mean task gap | Mean comm ops | Mean edge edits | Mean proxy cost 0.5 | Mean proxy cost 5 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Model A | 3.137028 | 606.200 | 0.000 | 806.200 | 806.200 |
| Model B | 2.475998 | 605.800 | 0.000 | 805.800 | 805.800 |
| Model C | 3.137028 | 607.200 | 0.400 | 807.400 | 809.200 |
| Model D | 2.041117 | 606.600 | 0.400 | 806.800 | 808.600 |
| Central heuristic | 14.960335 | 425.400 | 25.600 | 638.200 | 753.400 |
| Random control | 3.564428 | 604.200 | 3.200 | 805.800 | 820.200 |
| Zero-T control | 3.334169 | 606.000 | 1.200 | 806.600 | 812.000 |

## Paired descriptive observations

Negative task-gap difference favors the candidate. Pairs use the same problem and T at C_top=0.5; n=5 seeds. Repricing does not add trials. The full 45 comparisons across sizes/temperatures are in analysis.json; this display retains the original report slice rather than choosing a favorable new one.

| Candidate / baseline | Mean gap difference | Lower / equal / higher gap counts | Mean proxy-cost difference |
| --- | ---: | --- | ---: |
| Model C / Model A | 0.000000 | 0 / 5 / 0 | 1.200 |
| Model D / Model B | -0.434880 | 1 / 4 / 0 | 1.000 |
| Model C / Zero-T control | -0.197141 | 2 / 0 / 3 | 0.800 |
| Central heuristic / Model A | 11.823307 | 0 / 0 / 5 | -168.000 |
| Random control / Model A | 0.427399 | 2 / 0 / 3 | -0.400 |

These differences are observations on the corrected five-seed subset. There is no reported p>0.35, 18% capacity-matched convergence penalty, universal topology tax or statistically established absence of advantage. Failure to establish an advantage here is not equivalence and does not terminate Track S.

## Predefined sweep versus actual scope

Original predefined vertex-matched grid: 20160 priced rows. Corrected execution: 1260 rows (6.25%), leaving 18900 unexecuted. The same original subset is regenerated; it is not relabeled as the complete predefined experiment.

| Axis | Predefined | Executed | Omitted |
| --- | --- | --- | --- |
| nodes | [8, 12, 16, 24] | [8, 12, 16] | [24] |
| seeds | [1, 2, 3, 4, 5, 6, 7, 8, 9, 10] | [1, 2, 3, 4, 5] | [6, 7, 8, 9, 10] |
| temperatures | [0.1, 0.5, 1, 2] | [0.1, 0.5, 1] | [2] |
| couplings | [0.5, 1, 1.5] | [1] | [0.5, 1.5] |
| topology_costs | [0, 0.5, 1, 2, 5, 10] | [0, 0.5, 1, 5] | [2, 10] |

The original specification stated fields in [-1,1]; original code/methodology used [-0.5,0.5]. This pilot preserves the implemented interval and explicitly does not test the wider one. N24 is outside this bounded exact-reference execution. Capacity-matched task design remains absent. Temperature labels on the zero-T control do not change its acceptance policy and are not additional independent evidence.

## Unchanged independent rubric

The original frozen rubric SHA-256 is `b220e94331fa59e205929890be1f68f1a876e3d728b32c3535a3012970d87353`. Its >=20% full-cost improvement and simultaneous 95% interval criterion is retained. It is **not evaluable** after the missing capacity/quality/cost/baseline/power prerequisites; no new weaker criterion replaces it. The reviewer planning floor is at least 30 independent held-out instances per workload stratum plus a pre-unblinding power calculation; five seeds do not pass. No new confirmatory p-value is manufactured.

| Gate | Assessment | Evidence/limit |
| --- | --- | --- |
| Complete implemented dynamics and task identity | PASS FOR DOCUMENTED BOUNDED HEURISTICS | Immutable task coefficients and complete state traces independently audited; actual proposals and scheduling documented. |
| Equilibrium/ergodic sampling guarantees | NOT CLAIMED | Adaptive arms are nonequilibrium optimizers; occupancy fraction is not ESS. Fixed tiny-state Metropolis controls are checked separately. |
| Capacity/rule-matched architectural comparison | NOT MET | Vertex-matched only; H2 explicitly untested; capacity flag rejects. |
| Complete cost matching | NOT MET | Three-term proxy omits weighting/timing of full energy scans, global sorting, RNG, setup/validation and graph storage; auxiliary operation counts disclosed. |
| Strong baseline/ablation portfolio | NOT MET | Central correlation-ranking heuristic is not a strong optimized topology/inference portfolio; no trained-frozen, matched binary encoding or full factorial. |
| Independent instances/power/confirmatory statistics | NOT MET | Five seeds per size; below reviewer planning floor; no pre-unblinding power calculation. Repriced rows/temperature duplicates do not add replication. |
| Frozen >=20% full-cost materiality and simultaneous 95% interval | NOT EVALUABLE | Matched quality/full cost, strong baseline and independent-instance power prerequisites absent. Criterion unchanged, not replaced with a favorable endpoint. |
| Provenance and reproducibility | PASS FOR RECORDED LOCAL RUN | Original files preserved; all corrected rows, task coefficients, traces, source hashes and verification commands retained. |
| Physical hardware advantage | UNTESTED | No measured joules, hardware realization or area/delay data. |

## Scientific interpretation

The corrected implementation repairs demonstrated validity defects. The dataset measures task-energy outcomes, occupancy and declared proxy operations for the actual seven heuristics. It does not measure hardware advantage, total-system cost, a full baseline portfolio or H2. Local rewiring, central ranking and random rewiring can trade task quality against internal-network dynamics; those are not changes to the task itself. Centralized means a global pair-ranking heuristic here, not spectral min-cut or an exact/strong optimizer. Deterministic means zero-temperature acceptance conditional on seeded random scan/proposals, not the absence of randomness in scheduling.

**REVISE** is warranted because architectural claims remain underidentified and the completed pilot misses explicit rubric gates. CONTINUE would require a validated advantage against the required matched controls; STOP would require valid evidence sufficient to reject the declared hypothesis/domain. Neither follows from this partial five-seed proxy-cost experiment. H2 is untested, not negative evidence. No S03 or further research cycle is initiated.

## Provenance and reproduction

All original PR #582 files, results and STOP text remain byte-identical in historical/pr582/ with a superseded-evidence notice and manifest. Before-fix rejection tests and actual review comments are in evidence/before/ and evidence/pr582-*.json; after-fix controls/oracle evidence are in evidence/after/. Raw corrected states are in results.traces.jsonl.gz, and task coefficients plus all 1260 rows are in results.json. Run `python3 regenerate.py --runner /path/to/t81_s02_stochastic_topology` to regenerate, audit and render this report/analysis. SHA-256 identities and recorded commands are in evidence/generation.json. No production runtime, ISA, language or governance mechanism was modified.
