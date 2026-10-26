# S02 corrected pilot specification

Date: October 26, 2026. Status: bounded scientific-integrity correction; **REVISE**, not a new research cycle. The unchanged original specification is archived in historical/pr582/SPECIFICATION.md. The full original parameter grid is not claimed executed.

## Task and computational network are different objects

The immutable task has N spins, fields h and symmetric zero-diagonal J, and energy

`E_task(s) = -sum_i h_i*s_i - sum_{i<j} J_ij*s_i*s_j`.

Fields in this regenerated pilot follow the original implementation/methodology interval [-0.5,0.5), not the original specification's wider [-1,1]. Couplings on a clipped square-grid graph are ±1; nonedges have task J=0. All seven arms for a given problem use identical task coefficients. Reporting, exact references, task gaps, ground-state occupancy and first-hit observations always use E_task.

The mutable computational graph A and network couplings W begin from the task graph/J. Proposals use `E_network(s,A,W)`, with a positive coupling-scale fallback when a newly selected network edge has no stored nonzero W. Removing a computational edge does not remove a task constraint; adding a computational edge does not add a task constraint. Local/random rewires may set a previously zero W to a seeded random ±scale, retained for later reuse. The original task is a const deep copy.

Here exact reference means exhaustive endpoint enumeration with binary64 energy arithmetic and explicit 1e-9 comparison tolerance, not arbitrary-precision real arithmetic.

The exact-reference cache includes N, alphabet, every h value and the complete J matrix after validating finite, symmetric, well-formed coefficients. N=1..16 is supported by this bounded exact-reference runner. E_task is affine in each spin when the other spins are fixed: any zero trit can be replaced by an endpoint that does not increase energy. Thus a binary endpoint achieves a ternary minimum for this particular Hamiltonian (no onsite quadratic term). Binary 2^N enumeration computes both minima; independent 3^N enumeration verifies the theorem on tiny instances. It is not an H2 dynamics/capacity result.

## Actual transition and topology rules

One step is one seeded random single-site proposal, **not an N-site sweep**. Binary flips sign; ternary chooses either alternative with probability 1/2. The proposal is symmetric. Stochastic acceptance is **Metropolis** `min(1,exp(-delta_E_network/T))`, not the original specification's Glauber/logistic formula. The deterministic-labeled arm accepts non-increasing network energy, but its scan order remains seeded random. Temperature is fixed per stochastic run; this is not simulated annealing.

Topology attempts occur after zero-based proposal indices 10,20,...,190 (19 attempts during 200 steps). Fixed arms do nothing. Local adaptation samples a pair and, if an existing edge is instantaneously frustrated, samples a possible non-neighbor for one rewire. There is no persistent-frustration history or guaranteed improving choice of the new neighbor. Random control attempts the same kind of valid-edge rewire without consulting spins/frustration. Centralized adaptation globally ranks all candidate pairs by current W*s_i*s_j and selects the top N edges; this is a **correlation-ranking heuristic**, not spectral min-cut, exact optimization or a complete strong baseline portfolio.

Adaptive arms are nonequilibrium optimization heuristics, not claimed samplers of the immutable task Gibbs law. The complete implementation state includes spins, computational adjacency/couplings, stream states and proposal counter. No learned weight optimizer or equilibrium/mixing theorem is inferred. The exact state–topology update sequence is spin proposal, acceptance/rejection, optional rewire, then immutable-task observation.

Randomness uses the published public-domain SplitMix64 word permutation and rejection-based bounded integer sampling. Modulo reduction occurs only after rejecting the incomplete residue interval. Task generation, initialization, spin proposals, acceptance and topology use distinct fixed domain constants. This prevents task words/adaptation draws from being reused as the same spin-proposal stream. It is deterministic pseudo-randomness, not a proof of independent physical entropy or cryptographic strength. C++ and Python words/coefficients have fixed vector/equality tests. [Primary reference](https://prng.di.unimi.it/splitmix64.c).

## Scope, controls and H2

The original predefined vertex grid has seeds1–10, N8/12/16/24, T0.1/0.5/1/2, coupling scales0.5/1/1.5, costs0/0.5/1/2/5/10 and seven arms: **20160 priced cells**. The corrected pilot repeats only the originally executed subset: seeds1–5, N8/12/16, T0.1/0.5/1, coupling1, costs0/0.5/1/5, 200 single-site steps. This is 1260 priced cells, 6.25%, with 18900 unexecuted. Five seeds per size do not pass the unchanged rubric's independent-instance planning floor or power requirements. Wider field distribution and N24 are also not tested.

Arms remain binary/ternary fixed, binary/ternary local-adaptive, binary central-ranking, binary random-rewire and binary zero-temperature-acceptance local-adaptive. The fixed arms serve as no-adaptation controls. A runtime capacity flag now rejects: growing N with unused binary variables does not create the same task under matched capacity. **H2 is UNTESTED.** No capacity-matched result, 18% penalty, ternary superiority/inferiority or no-advantage conclusion is permitted.

C_top is a measurement weight, not part of the transition law. Simulate 315 trajectories once, then reprice each at four values. Zero-temperature trajectories repeated under different nominal T labels are not additional independent evidence. Report all seeds, failures and raw traces; do not treat repriced rows or time steps as independent instances.

## Metrics and cost boundary

Task gap is E_task(final)-E_task*, with a 1e-9 numerical tolerance. A below-reference gap beyond tolerance raises an error, not absolute-value masking. Occupancy is the fraction of the 200 post-proposal states within 1e-9 of the task optimum, **not ESS**. First-hit indices include the initial state at 0; nonhits serialize as null. Mean task energy excludes the initial state. Network final energy is a separately labeled diagnostic.

The retained three-term proxy is `proposal_attempts + degree_communications + C_top*edge_edits`. Edge edits count undirected additions/removals; central updates charge the symmetric difference rather than pretending every selected unchanged edge is new. Additional full-network pair checks, topology inspections, sort comparisons and random-word draws are exposed. They are not all weighted in the proxy. Full energy scans, global sorting, adjacency clearing/storage, task observation, reference computation, setup and physical communication are not a measured full-system cost. Sorting counts can depend on the C++ standard-library implementation. Do not infer architecture efficiency or hardware joules from this proxy. Offline exact oracles are evaluation machinery, not information supplied to the search policy.

The unchanged falsification rubric remains the acceptance/rejection standard. Missing matched controls, full cost, strong baselines and powered independent instances imply **REVISE** for architectural interpretation; no automatic continuation or termination of Track S/S03 is authorized.
