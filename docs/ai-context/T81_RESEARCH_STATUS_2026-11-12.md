# T81 Foundation — Active Research Handoff (2026-11-12)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation — Active Research Handoff (2026-11-12)](#t81-foundation-—-active-research-handoff-2026-11-12)
  - [Immediate handoff](#immediate-handoff)
  - [BASE-PERF-00 candidate observations (PR #780 agent report)](#base-perf-00-candidate-observations-pr-#780-agent-report)
  - [Holding-pattern instructions for the next agent](#holding-pattern-instructions-for-the-next-agent)
  - [Disposition matrix](#disposition-matrix)

<!-- T81-TOC:END -->


**Class:** Non-normative, dated operational status; not a scientific acceptance, benchmark authorization, RFC ratification, merge approval, or claim of independent reproduction.
**Repository:** https://github.com/t81dev/t81-foundation
**Reference main for active work:** `d3a6cf55517a3098d7bae91b07c72694bc3472da` (PR #779 and #780 base; recheck current main before use).
**Observed PR metadata:** 2026-11-12. Historical research findings and earlier dated status files remain intact.
**Operational posture:** HOLDING PATTERN — await independent review of open draft PRs; do not infer acceptance from reported execution.

## Immediate handoff

1. **PR #780 — BASE-PERF-00 Cell throughput candidate:** [draft, open, unmerged](https://github.com/t81dev/t81-foundation/pull/780), head `026ee1cc84654fcb8fb127acf8c73a5b4145c291`. Seven changed files: `benchmarks/runner/arith_throughput.cpp`, `benchmark_runner.cpp`, `roundtrip_accuracy.cpp`, `tests/determinism/test_primitives.cpp`, and three evidence artifacts in `docs/benchmarks/`. Agent report claims Apple M2 physical workstation, macOS 26.5.2, Apple clang 21, Release `-O3 -DNDEBUG`, Google Benchmark 1.9.5, five repetitions and zero timed failures. **These are candidate host-local measurements, not independently accepted performance figures.**
2. **PR #779 — SCG-08 Gate 1 corrective work:** [draft, open, unmerged](https://github.com/t81dev/t81-foundation/pull/779), current observed head `4d5f3586c04a69c520e613166291c0974877628d`, one commit after the independently reviewed `5bfc0e2b7f531c4cdeca53ed9e1172f551683da6`. Source inspection of the new head identifies fixes for stored-tryte validation before decoding, `conv_limb_repacks = OutLimbs + 2`, and the exact negative product bound. **The previous review's three failures require targeted independent retesting against the new head** (including ASan and `-DNDEBUG`); source inspection alone is not acceptance.
3. **SCG-08 governance:** RFC-00E1 remains DRAFT; formal Gate 1 acceptance WITHHELD; Gate 2 STRICT NO-GO. BASE-PERF-00 is a separate authorized bounded Cell measurement, **not** SCG-08 Gate 2. Do not conflate the two workstreams or modify historical SCG-08 evidence.
4. **TCB-Core recovery:** Original numerical CLI benchmark was `./build/t81 benchmark`; current launcher is `./build/t81 internal benchmark`. The original benchmark measures software numerical arithmetic and conversion, not VM IPS or AI tokens/s. Historical archived throughput is methodologically mixed and is **not** a current accepted baseline. The benchmark operates on binary CPUs; no physical ternary hardware advantage is established.

## BASE-PERF-00 candidate observations (PR #780 agent report)

| Cell operation | Median throughput, Mops/s | Scope |
| --- | ---: | --- |
| Addition | 163.39 | 5-trit ripple carry, operand domain [-10,10] |
| Subtraction | 139.04 | 5-trit negate plus add, [-10,10] |
| Multiplication | 22.34 | 5-trit shift/add, [-10,10] |
| Division | 61.60 | `to_int + int64 division + from_int`; **not pure ternary division** |
| Round-trip conversion | 24.84 | `from_int + to_int`, [-121,121] |

Agent-reported prevalidation: 243/243 Cell round trips; exhaustive 59,049 input pairs each for addition, subtraction and division; multiplication 441/441 pairs within the timed [-10,10] domain. **Important negative finding:** multiplication raises intermediate-overflow exceptions for **352 full-domain pairs whose final product is representable**; therefore full-domain multiplication correctness is not a blanket PASS. The report also identifies 56,144 expected out-of-range multiplication pairs and 2,553 successful in-range pairs. All counts and performance statistics are subject to independent audit. Reported environment is a local Apple M2 workstation with ambient load, not a dedicated reference node.

Candidate evidence files:
- `docs/benchmarks/BASE_PERF_00_CELL_THROUGHPUT_BASELINE.md`
- `docs/benchmarks/base_perf_00_cell_throughput_baseline.json`
- `docs/benchmarks/base_perf_00_cell_throughput_raw.json`

**BASE-PERF-00 acceptance questions:** independently recalculate five medians/spreads and completed-op counts from raw JSON; reconcile `items_per_second` timing basis (CPU vs real time) with reported per-iteration latency; verify untimed oracle, optimization barriers, exception accounting, exact build/provenance, and scope of multiplication PASS. Until accepted, classify as **HOST-LOCAL BASELINE CANDIDATE**, not a standardized comparative baseline. No binary superiority claim is supported.

## Holding-pattern instructions for the next agent

- **First:** obtain Codex independent acceptance review of PR #780, pinned to head `026ee1cc84654fcb8fb127acf8c73a5b4145c291`; disposition ACCEPT / CORRECTIONS REQUIRED / BLOCKED. Do not change production `cell.hpp` or silently broaden the benchmark.
- **Second:** obtain targeted re-review of PR #779, pinned to head `4d5f3586c04a69c520e613166291c0974877628d`, focusing on the three prior defects and safety tests. No automatic merge or gate advancement.
- **After reviews:** if corrections are required, route bounded repairs to Antigravity; independently re-review new heads. If accepted, seek explicit maintainer merge decision and document the final provenance.
- **Preserve:** H01–H08 identities, SCG-07 QUALIFIED WITH LIMITATIONS, CPA/ISA/BASE historical qualifications, negative/conditional ternary-vs-binary findings, and older ai-context status snapshots. Do not rewrite the program history or treat the current holding pattern as a research stop.
- **Refresh before acting:** main SHA, PR heads/states, CI and authorizations. No claim that either PR has merged.

## Disposition matrix

| Workstream | Status |
| --- | --- |
| TCB-Core numerical benchmark recovery | EXISTING BENCHMARK RECOVERED |
| BASE-PERF-00 numerical correctness | Agent-reported bounded PASS; independent review pending |
| BASE-PERF-00 host-local throughput | MEASURED CANDIDATE; acceptance pending |
| BASE-PERF-00 reference-hardware baseline | NOT ESTABLISHED |
| Binary comparative advantage | NOT EVALUATED |
| PR #779 corrective source | FIXES PRESENT IN NEW HEAD; retest pending |
| RFC-00E1 | DRAFT |
| SCG-08 Gate 1 | WITHHELD |
| SCG-08 Gate 2 | STRICT NO-GO |

This overlay supersedes older *operational next-action* wording, not earlier evidence, decisions, or scientific scope.
