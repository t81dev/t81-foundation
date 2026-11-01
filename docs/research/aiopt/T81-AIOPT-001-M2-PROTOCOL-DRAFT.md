# T81-AIOPT-001 M2 — Discriminating Workload Protocol (DRAFT)

**Status:** DESIGN ONLY / NOT APPROVED / NO EXECUTION AUTHORIZED  
**Date:** 2026-11-01  
**Depends on:** M1 PR #665 (`e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db`); corrected scientific review PR #666 (`b2b9d3c73df96d6b4227ee0061c56bc2ee04a8fe`); reconciliation PR #667 (`1edf70bbffc9168c27be5c6c19cf97388f6991a4`). All three are proposed and unmerged at drafting.

## Research question

Under identical wildcard ternary matching semantics, can a packed ternary implementation achieve a **practically meaningful end-to-end advantage** over the strongest correct conventional binary implementation for a predeclared, nondegenerate workload? This is an empirical question, not an assertion that ternary radix intrinsically accelerates CPU arithmetic.

## Semantic contract

- Inputs are vectors `x ∈ {-1,0,+1}^L`; catalog rows are `p ∈ {-1,0,+1,*}^L` where `*` matches any valid trit.
- A row matches iff each specified position equals the corresponding input trit. Return **all** matching catalog indices in ascending catalog order; duplicates remain distinct entries. Invalid trits, malformed patterns and inconsistent lengths must fail with explicitly specified errors, before any partial output.
- Define a separate reference using plain scalar semantics. Include cases with zero, one and multiple matches; all-wildcard, no-wildcard, duplicate and adversarial catalogs. Require deterministic fixed seeds and published corpus digests.
- No benchmark until correctness agreement is independently checked across every selected representation and comparator.

## Candidate workload matrix (requires preregistration before running)

- Vector lengths `L = {16, 64, 256, 1024}`.
- Catalog sizes `M = {16, 256, 4096}`.
- Specified-position densities `D = {0.25, 0.50, 0.90, 1.00}` (the final 1.00 arm is an exact-match control).
- Input batches `B = {1, 64, 4096}`; distinguish single-query latency and amortized throughput.
- Data distributions: balanced independent trits; skewed zero-heavy; correlated positions; duplicate-pattern adversary. Predeclare exact generator and seeds; ensure representative multi-hit rates, including all-wildcard stress cases.
- This is a **candidate grid**, not permission to execute its 576 cells or assume all are feasible. Before implementation, narrow to a power-justified primary set and label exploratory cells. Record resource limits.

## Required comparators

1. Straightforward scalar byte-coded ternary baseline with short-circuit wildcard checks.
2. Optimized byte/INT8 representation using appropriate vectorized comparisons where supported.
3. Conventional bitset or per-position inverted-index candidate filtering with three state sets and wildcard semantics.
4. Packed 2-bit ternary representation with explicit invalid-code handling, including a carefully specified word-level wildcard equality predicate.
5. Lookup-table/direct-index comparator only where memory and preprocessing budgets make it feasible; do not force an infeasible exponential table.
6. Optional base-3 dense encoding as an additional **separate** representation, not a substitute for binary baselines.

All implementations must use equivalent semantic outputs, validation policy and input corpora. Let each comparator choose its best reasonable algorithm; avoid artificially forcing linear scans. Publish implementation, compiler options, source hashes, hardware and feature flags.

## Measurement and cost accounting

Predeclare **primary metric**: end-to-end time per batch, including applicable input validation, representation conversion, catalog preprocessing amortized over an explicitly fixed reuse count, dispatch, matching and output materialization. Also report separately: steady-state matching latency, preprocessing time, encode/decode, resident bytes, allocated bytes, input/catalog bandwidth and any additional index metadata. Measure both cold and warmed working sets with controlled methodology. No conflation of storage reduction with execution acceleration.

Measure on actual physical machines, not emulated hardware. Report CPU, ISA, OS, compiler, optimization flags, frequency/governor, thread affinity, thermal controls, timing calibration and instrumentation limitations. PMU counters are optional diagnostic evidence; absence must not be papered over. Randomize comparator run order; preserve raw per-run observations and rerunnable commands. Avoid invented performance forecasts.

## Hypotheses and decision thresholds (proposal, must be approved first)

- **H0:** Packed ternary has no practically meaningful end-to-end improvement over the strongest conventional comparator on the preregistered primary workload set.
- **H1:** Packed ternary improves the primary end-to-end metric by at least **10%** on a specified primary workload family without violating correctness or exceeding a separately preregistered memory budget.
- The 10% value is a **proposed decision threshold**, not a historical T81 standard. A reviewer must approve or replace it before execution.
- Specify sampling/power calculation and interval method from a pilot *before* confirmatory measurements. Control multiplicity across primary workloads; report confidence intervals and effect sizes, not p-values alone. Exploratory results cannot establish confirmatory H1.

## Outcomes

- **SUPPORTED IN TESTED SCOPE:** Confidence/effect-size evidence meets the preregistered improvement threshold against the strongest fair comparator, with correct outputs and complete cost accounting.
- **BOUNDED NEGATIVE:** Adequately powered confidence interval excludes the predeclared practical improvement on specified tested configurations.
- **INCONCLUSIVE:** Insufficient precision/power, contradictory platform outcomes, or interval spanning both negligible and meaningful improvement.
- **NOT TESTED / BLOCKED:** Unrun cells, missing controls, invalid harness, insufficient resources or governance permission. Do not turn these into negative evidence.
- Any early stop must use preregistered resource or safety rules and leave remaining cells NOT TESTED. No universal CPU or ternary-hardware closure.

## Adaptive/AI selector is a later separate question

No learned selection in M2 baseline study. A future selector would require disjoint train/test workloads, held-out distribution shifts, static and simple rule-based selectors, a clairvoyant upper bound clearly labeled unattainable, and full model training/dispatch/feature-extraction overhead. Any claim of AI-specific benefit must exceed nonlearned controls, not merely beat a poor static implementation.

## Scientific integrity and acceptance

M1 correctness remains bounded to four-trit exact C40 semantics. M2 wildcard correctness must be established afresh. The corrected H04-R02 symbolic-model results retain conditional ternary advantages but do not establish CPU speedups. The earlier CPU-kernel freeze is a program decision about tested kernels, not proof of universal impossibility.

**Required before implementation:** explicit maintainer approval of semantics, primary workload subset, comparator fairness, 10% threshold or replacement, power/statistics, cost model, preservation plan and resource budget; independent review of the protocol; resolved acceptance status of PRs #665–#667 or an explicit documented exception.

**Stop:** This file is a reviewable draft. No benchmark, experiment execution, AI selector, production integration, merge or research acceptance is authorized by its publication.
