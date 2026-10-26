# Independent S02 evaluation rubric — version 1

Status: frozen before examining S02 implementation or results. This is a local, timestamped review protocol, not an externally registered experiment, maintainer approval or claim that Jules has adopted it. S02's actual protocol, target workload, cost model and results were not supplied in the assignment. Criteria below apply conditionally to the stated architectural claims; missing protocol items are UNKNOWN, not observed failures. No results are inspected during this review.

## 1. Declare what could be falsified

Before unblinding, state exactly one primary architectural claim, problem distribution, size range, baseline portfolio, primary endpoint, minimum useful effect, resource envelope and rejection rule. Specify P, Q, their joint noise coupling, update order, learning state, graph constraints, precision and stopping rule. Distinguish simulation throughput, statistical efficiency, optimization quality and physical energy. A node-count advantage is not automatically an architecture advantage.

For a bounded architectural-efficiency claim, the proposed review threshold is a **20% end-to-end cost reduction**, with the upper simultaneous 95% confidence bound on candidate/best-eligible-baseline cost below 0.80, while meeting predeclared quality/bias and memory/communication constraints. This 20% is a reviewer-selected materiality threshold, not a theorem. If S02 preregisters a different justified threshold before results, report both; do not move the goalpost after inspection. A claim smaller than 20% can be reported as a measured software effect without passing this proposed architectural gate.

## 2. Mandatory validity gates

| Gate | Required record | Failure or limitation |
| --- | --- | --- |
| Complete dynamics | Full joint transition/learning law, independent/shared randomness, scheduling, graph mutation and exception rules | Marginals alone or unspecified shared state make the claim underspecified |
| Target semantics | Explicit target distribution or optimization objective, including graph/learning regularization | A stable trajectory is not proof of correct sampling or successful computation |
| Finite exact controls | Enumerate tiny transition matrices; test claimed invariant measure, communicating classes, period and probability current | Incorrect claimed invariant measure or unacknowledged cycle invalidates the relevant sampling/stability claim |
| Capacity matching | Bits of element state, weights, topology, optimizer, queues, RNG and scratch; equal representable input/output precision | Equal node counts or an uncounted graph are insufficient |
| Rule matching | Binary encoding of same finite-state rules with same random tape/schedule; optimized implementation afterward | A new algorithm must not be attributed solely to radix/architecture |
| Cost matching | All updates, proposals/rejections, rewires, global checks, synchronizations, messages/bytes, initialization, training, RNG and output validation | Free rewiring, omitted preprocessing or hidden central coordination invalidate resource superiority |
| Provenance | Exact code/config/data/environment IDs, all seeds, raw per-instance outcomes, failures and timeouts | Missing provenance limits interpretation; no producer summary alone establishes correctness |

Gates are claim-specific. A nonequilibrium optimizer need not obey detailed balance or sample a fixed target; it must stop claiming those properties. An absorbing solver can terminate legitimately; an ergodic sampler cannot silently use the same absorbing rule.

## 3. Strong baseline portfolio

1. Capacity-matched binary encoding of precisely the same ternary machine (not just one binary node per ternary node), with an explicit unused-state policy and equivalent topology/precision.
2. Optimized categorical Gibbs/Metropolis implementation; blocked or graph-colored updates where legitimate; parallel tempering/cluster methods where applicable. Give each baseline the same tuning budget and validate its applicability rather than selecting a deliberately unsuitable algorithm.
3. Adaptive binary system with the same learning, sparsity and topology mechanism; distinguish state encoding from rule/controller changes.
4. Centralized topology selection/optimization under identical observation and resource limits; charge communication, aggregation and synchronization to it. Include a centralized controller with access to the same information; label stronger global-oracle controls as upper bounds rather than fair deployable peers.
5. Optimized sparse dataflow/event-driven implementation on appropriate CPU/GPU resources. Existing stochastic/p-bit or Ising hardware becomes a required comparison only for a physical-hardware claim; cite external measurements as context, not matched measurements.

Compare to the best eligible tuned baseline selected on training/validation data, not whichever looks weakest on the test set. No post-test tuning advantage. Lack of an applicable baseline is a limitation, not automatic novelty.

## 4. Ablations and negative controls

Use the binary/ternary × deterministic/stochastic × fixed/adaptive-topology factorial where meaningful, preserving the computational task and accounting. Do not force stochastic sampling into a single deterministic trajectory and call it the same task: the deterministic control may be seeded simulation or deterministic inference with its own declared guarantee. Keep identical graph-update policies in the matched-rule comparisons.

Also compare adaptive topology to: random rewiring at the same rate; a fixed graph selected with equal training budget; topology learned during training then frozen; an oracle-selected small-instance graph (explicit upper bound); identical graph changes applied to a binary encoding. Separate learning benefit from execution benefit and allow update/training cost to amortize only over a declared reuse count.

Negative controls: independent variables with no useful coupling; a task already solved on a fixed local graph; the synchronous Gibbs wrong-target counterexample; disconnected/absorbing topology; anti-correlated/periodic update schedule; very rare topology transitions; incompatible alleged conditionals; deterministic replay of the same random/event tape; adversarial topology churn; deliberately shifted held-out instances. The actual suite must include relevant examples, not necessarily every inapplicable toy. Report a known-nonequilibrium optimizer as such rather than requiring it to pass equilibrium tests.

## 5. Statistical design and endpoints

Independent held-out problem instances are the primary replication unit; time steps, correlated samples and many seeds of one instance do not become independent instance evidence. Use paired instance-level contrasts, randomized/interleaved run order, equal seed/tuning budgets, and confidence intervals clustered/resampled by instance; nested seed variation is retained. Start with at least 30 independent held-out instances per declared workload stratum as a planning floor, then determine sample size by a pre-unblinding power/precision calculation. Thirty is not a guarantee of power or a universal sufficiency rule. With too few instances, report a pilot only.

Choose one task-appropriate primary endpoint before results:

- Sampling: wall time (or measured joules on actual hardware) to a specified estimand precision, with target correctness and bias tolerance first. Report estimator-specific effective sample size, autocorrelation and dispersed-chain diagnostics, not merely raw draws/s. Exact total variation on tiny finite instances is stronger than matching a few moments. Diagnostics on large instances do not prove mixing.
- Optimization: time/cost to a fixed externally validated objective/constraint target, or solution quality at fixed full cost, including failed/censored trials. For independent restarts with known per-trial success p and cost c, time-to-one-success at probability 0.95 is c ceil(log(0.05)/log(1-p)); p=0 yields infinity. Real p is estimated: propagate its interval and state independence/restart assumptions. Do not hide timeouts or substitute best-seed minima.
- Inference/learning: held-out proper loss/calibration and uncertainty at fixed full cost; separate training data/adaptation and evaluation. No test-set topology selection.

Use at least four dispersed chains when MCMC diagnostics apply; inspect rank-normalized/split diagnostics, multimodal transitions and estimand ESS. A diagnostic threshold is a warning gate, not a mixing theorem. Address multiple primary hypotheses with Holm familywise 0.05 control or a fixed simultaneous interval procedure; preregister the chosen procedure. If the portfolio/metric is selected on test data, account for that selection. Publish effect sizes, uncertainty, tails and all unsuccessful strata, not p-values alone. Label exploratory analyses separately. Failure to reject no difference is not equivalence; equivalence requires an explicit margin and adequately powered interval test.

## 6. What simulation can establish

A simulation may establish a mathematical counterexample, implementation correctness under a stated finite/precision model, or a resource-accounted algorithmic effect on tested instances. It cannot by itself establish lower physical joules, wiring area/delay, device noise quality, precision/temperature robustness, fabrication feasibility, scalable communication or a new complexity class. A stochastic source reproduced by a deterministic program with a supplied random tape does not invalidate the modeled randomness; it refutes claims that the simulated transition law intrinsically requires ternary hardware.

Hardware claims require matched devices/technology assumptions or actual measurements with total-system power, conversion/control/interconnect/RNG overhead, reset/training/measurement costs, accuracy, temperatures, uncertainty and runtime distribution. State what remains a modeled extrapolation. Universality is not speed, energy efficiency or useful learning.

## 7. Decision rules

- **PROCEED (bounded hypothesis only):** all relevant validity gates pass; material primary benefit survives strong controls/ablations and held-out uncertainty; identify the exact unsolved mechanism. Simulation alone permits an algorithmic hypothesis, not hardware superiority.
- **REVISE:** rules/target/cost/baselines/statistical design are missing, a claimed guarantee fails, or a real measured effect remains confounded. Preserve negative evidence and narrow the claim; do not infer a new architecture from a trajectory plot.
- **STOP (particular claim):** advantage disappears under equivalent binary encoding and best eligible controls, target bias invalidates the claimed result, or repeated adequately powered tests rule out the preregistered material effect in the declared domain. This does not prove all possible stochastic architectures useless.

Sources supporting diagnostic/algorithm cautions are documented in ARCHITECTURAL_FOUNDATIONS.md and SOURCES.md. This rubric's materiality threshold, design floor and decision gates are explicit reviewer choices. Results remain unexamined.
