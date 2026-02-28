# Active Development Tasks

**Last Updated:** February 28, 2026

This document tracks immediate, actionable tasks for the T81 project.

## 1. Current Sprint: March Governance Close + Governed Inference

### Release and Governance Closure (C2)
- [ ] **C2 Month-Close:** Execute runbook on 2026-03-31 and stamp final outcomes in `docs/records/audits/2026-03-governance-review.md`. (Prep update 2026-02-26: consolidated execution helper added at `scripts/governance/c2_month_close_check.py`, emitting `docs/status/C2_MONTH_CLOSE_CHECK_2026-03-31.md`.)
- [x] **Release Candidate Decision:** Select March release-candidate SHA and record required-context results in `docs/status/RELEASE_READINESS_PACKET_2026-03.md`. (Completed 2026-02-26: candidate `b4fdf8efdf249e391f7c93fb18cf9245926b6a38` recorded with required-context evidence; decision remains `HOLD` pending `quality gate / required` and `Analyze (cpp)` success on a post-fix main candidate.)
- [x] **Promotion Snapshot Refresh:** Re-run `python3 scripts/governance/t81lang_promotion_gate_snapshot.py` within close window and capture result. (Completed 2026-02-26; snapshot refreshed as `READY` in `docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md` and recorded in March packet/audit artifacts.)
- [x] **Status Integrity:** Re-run governance hygiene and status link-target checks and record pass/fail evidence. (Completed 2026-02-26; hygiene check and link-target sweep passed, recorded in `docs/status/RELEASE_READINESS_PACKET_2026-03.md` and `docs/records/audits/2026-03-governance-review.md`.)

### Governed llama.cpp Integration
- [x] **Fixture Pair:** Add one sanctioned model/policy fixture pair for local/CI reproducibility gate execution. (Completed 2026-02-26: CI provisioning path added in `.github/workflows/ci.yml` with sanctioned-source controls `T81_LLAMA_MODEL_URL`, `T81_LLAMA_EXPECTED_MODEL_HASH`, optional `T81_LLAMA_MODEL_SHA256`; fixture policy documented in `tests/fixtures/llama_cpp_repro/README.md`.)
- [x] **CI Gate Wiring:** Add guarded CI path for `scripts/ci/llama_cpp_repro_gate.py` when fixtures are present. (Completed 2026-02-26; optional fixture-gated steps added to `.github/workflows/ci.yml`.)
- [x] **Packaging Decision:** Decide whether `t81_llama_adapter` remains internal or is exported as a package target. (Completed 2026-02-26: remains internal/build-only; codified by `T81_EXPORT_LLAMA_ADAPTER` guard in `CMakeLists.txt` with explicit unsupported-error path.)
- [x] **Boundary Classification:** Keep llama integration explicitly classified as governed non-DCP in status/release artifacts. (Completed 2026-02-26; explicit classification added in `docs/status/RELEASE_READINESS_PACKET_2026-03.md` and `docs/status/IMPLEMENTATION_MATRIX.md`.)

### T81Lang Standard Library Stabilization (Sprint 2)

- [x] **Surface Baseline Gate:** Add machine-checkable stdlib module-surface baseline check and wire it to CI/governance checks. (Completed 2026-02-26: `scripts/governance/check_stdlib_surface_baseline.py` added and wired into `.github/workflows/ci.yml` + `check_docs_governance_hygiene.py`.)
- [x] **Stabilization Plan:** Publish governed execution plan with bounded determinism posture and acceptance gates. (Completed 2026-02-26: `docs/status/STDLIB_STABILIZATION_PLAN_2026-03.md`.)
- [x] **Module Invariant Expansion:** Add/expand conformance fixtures for under-covered stdlib behaviors (`std.math`, `std.core`) with deterministic observable outputs. (Completed 2026-02-26: added fixture suites `tests/fixtures/t81lang_std_core/*`, `tests/fixtures/t81lang_std_math/*`; consolidated into parameterized harness `tests/cpp/cli_stdlib_fixtures_test.cpp` on 2026-02-28 (`7724578e`), wired in `CMakeLists.txt`.)
- [x] **Stdlib Change Policy:** Define stdlib semver/change taxonomy (breaking/non-breaking/experimental) and required evidence per change class. (Completed 2026-02-26: `docs/governance/STDLIB_CHANGE_POLICY.md`.)
- [x] **Stdlib Promotion Snapshot:** Produce a stabilization snapshot artifact mapping each `std.*` module to status (`stable`, `bounded`, `experimental`) and evidence. (Completed 2026-02-26: `docs/status/STDLIB_PROMOTION_SNAPSHOT_2026-03.md` with governance check `scripts/governance/check_stdlib_promotion_snapshot.py`.)
- [ ] **std.io Fixture Suite:** Author `lang/stdlib/std/io.t81` fixture programs and golden `.out` files; add one-line entry to `STDLIB_MODULES` in `tests/cpp/cli_stdlib_fixtures_test.cpp`. Exit gate: `cli_stdlib_fixtures_test` (module: io) green; `check_stdlib_surface_baseline.py` passes.
- [ ] **std.sys Fixture Suite:** Author `lang/stdlib/std/sys.t81` fixture programs and golden `.out` files; add one-line entry to `STDLIB_MODULES`. Exit gate: `cli_stdlib_fixtures_test` (module: sys) green.
- [ ] **std.async Fixture Suite:** Author `lang/stdlib/std/async.t81` fixture programs and golden `.out` files; add one-line entry to `STDLIB_MODULES`. Exit gate: `cli_stdlib_fixtures_test` (module: async) green.
- [ ] **std.agent Fixture Suite:** Author `lang/stdlib/std/agent.t81` fixture programs and golden `.out` files; add one-line entry to `STDLIB_MODULES`. Exit gate: `cli_stdlib_fixtures_test` (module: agent) green; update `STDLIB_PROMOTION_SNAPSHOT_2026-03.md` evidence.

### Experimental Implementation Backlog (Post-C2 Pickup)
- [x] **Cognitive Tier 1:** Implement `SymbolicGraph::rewrite` and `is_confluent`. (Completed 2026-02-26: deterministic rewrite now updates nodes/edges with canonicalization, and confluence now checks unique nodes, valid edge endpoints, and deterministic `(from,label)->to` transitions; validated via `tiers_structure_test`.)
- [x] **Cognitive Tier 2:** Connect `ReflectiveFrame` to Axion trace events. (Completed 2026-02-26: added canonical Tier 2 reason channel `cog:tier2:reflect`, wired `ReflCap/ReflJustify/ReflTrace/ReflSeal` to emit reflective Axion event reasons, and validated via `vm_reflection_tier2_test`.)
- [x] **Cognitive Tier 3:** Implement `Recursor` evaluation loop and depth proof verification. (Completed 2026-02-26: added proof-verification APIs and deterministic evaluation loop in Tier 3 recursor, wired VM `Recurse/Contract` to enforce contraction proofs, and validated with `test_tier3_opcodes` plus a new Tier 3 failure-path check in `t81_vm_new_opcodes_test`.)
- [x] **Cognitive Tier 4:** Implement `NodeState` synchronization and gossip protocol logic. (Completed 2026-02-26: implemented bounded-coherence checks, monotonic gossip tick propagation, deterministic newest-first merge ordering with duplicate collapse, activated `t81_tier4_distributed_test`, and validated Tier 4 VM/distributed paths.)
- [x] **Cognitive Tier 5:** Implement `InfiniteCanonicalForm` lazy expansion logic. (Completed 2026-02-26: added lazy prefix seeding/expansion APIs on `InfiniteCanonicalForm`, wired VM `InfSeed/InfExpand` to deterministic lazy term growth, enabled `t81_infinite_opcodes_test`, and validated Tier 5 convergence/opcode paths.)
- [x] **Runtime JIT:** Advance `runtime/jit` from experimental research to prototype backend. (Completed 2026-02-26: added JIT trace execution/recording support for bitwise ops (`BitAnd/Or/Xor/Not/Shl/Shr/UShr`) with deterministic parity, and extended JIT equivalence coverage to include hot-loop bitwise traces and boundary logging.)
- [x] **CanonFS:** Optimize `PersistentDriver` for high-throughput tensor I/O. (Completed 2026-02-26: replaced `exists()+fopen/fwrite` with single `open(O_CREAT|O_EXCL)+write` fast path, added robust full-write handling, and enabled write-through object-cache insertion for hot rereads; validated CanonFS driver/persistent/axion-trace tests.)
- [x] **CLI Tooling:** Add `t81 trace export` for Axion logs (JSON/CSV). (Completed 2026-02-26: added `trace export` subcommand with `--format json|csv` and `-o/--out` support, parsing canonical trace lines into structured records with raw fallback; covered by `t81_cli_trace_export_test`.)
- [x] **Debugger Tooling:** Extend `t81 debug` with cognitive tier state inspection. (Completed 2026-02-26: added debugger command `t [1|2|3|4|5|all]` to inspect cognitive tier state including active tier, Tier 1 symbolic graph counts, Tier 2 reflective frame status, Tier 3 recursion depth/proofs, Tier 4 distributed tick/queues, and Tier 5 infinite-form summaries; validated via `t81_cli_debugger_test`.)

### Cognitive-Tier Spec Closure Follow-On
- [x] **Metric Enforcement:** Implement runtime hard gates for cognitive-tier complexity metrics (branching entropy, symbolic complexity, shape complexity `product(shape)*rank`). (Completed 2026-02-26: VM now enforces branch-entropy, symbolic-complexity, tensor-rank, and shape-complexity gates with deterministic promotion attempts before `TierFault`.)
- [x] **Tier Demotion Policy:** Implement conservative deterministic tier-down transition logic with explicit convergence conditions. (Completed 2026-02-26: VM now applies stability-window demotion with explicit convergence checks over recursion depth, symbolic/distributed/infinite activity, and complexity metrics.)
- [x] **Tier Declaration Breadth:** Expand `@tier(n)` static verification to cover tensor-rank/effect-surface restrictions per tier. (Completed 2026-02-26: semantic analyzer now enforces tiered call surfaces (tensor/effect builtins) and prevents lower-tier functions from calling higher-tier declared functions.)

______________________________________________________________________

## 2. Recently Completed (Feb 2026)

The highest-impact closures for this cycle are complete and tracked in:

1. `docs/records/status-history/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md` (A1 through A1G closures; archived 2026-02-28)
2. `docs/status/T81LANG_ENGINEERING_BACKLOG_2026-03.md` (BG-01 through BG-05 closures)
3. `docs/status/EXECUTION_PLAN_2026-03.md` (A1/A2/A3/B1/B2/B3/C1/C3/D1 completed; C2 in progress)

For historical implementation detail, see `docs/records/audits/2026-03-governance-review.md`.
