---
title: "Tier 4 Cognition: Self-Referential Modeling and Cognitive Loops"
status: accepted
author: Jules
date: 2026-02-10
updated: 2026-03-15
vote: +1
---

______________________________________________________________________

## 1. Abstract

This RFC defines Tier 4 Cognition in the T81 stack. Tier 4 introduces
self-referential cognitive loops, where the system can inspect, trace, and
refine its own decision-making process through deterministic Axion-guarded
layers. All Tier 4 operations produce bit-exact, auditable `ReflectionTrace`
artifacts and are subject to recursive self-improvement safety bounds.

## 2. Motivation

Tiers 1–3 cover basic logic, arithmetic, and controlled symbolic reasoning.
Tier 4 is required for higher-order reflection: the ability for an agent to
model its own state and execution paths, detect inconsistency, and promote
itself to higher tiers when warranted — all under strict Axion supervision.
Without a formal Tier 4 specification, agents cannot safely self-correct or
make tier-selection decisions in a reproducible, auditable manner.

## 3. Proposal

### 3.1. Tier 4 Architecture

Tier 4 routines reside in `t81::cog::v1` (headers under
`include/t81/experimental/cog/tier4/`) and focus on four core components:

1. **Self-Tracing Agents** — `Tier4Loop` records internal state transitions as
   Axion-visible `ReflectionTrace` events on every `reflect()` call.
2. **Cognitive Loops** — Deterministic `observe → reflect → refine` cycles
   with safety-bounded recursive self-improvement.
3. **Tier-Aware Planners** — `TierAwarePlanner::select_tier()` maps task
   complexity (1–81 scale) and a `requires_self_reflection` flag to the
   appropriate `TierId`.
4. **Self-Model** — `SelfModel` maintains current goal, confidence score,
   a beliefs map, and a 81-entry ring-buffer observation history.

### 3.2. Core Types

#### `SelfModel` (`include/t81/experimental/cog/tier4/self_model.hpp`)

```cpp
struct SelfModel {
  std::string current_goal;
  float confidence{0.0f};
  std::map<std::string, std::string> beliefs;
  std::vector<std::string> history;  // ring-buffered at 81 entries
};
```

#### `ReflectionTrace` (`include/t81/experimental/cog/tier4/tier4_loop.hpp`)

```cpp
struct ReflectionTrace {
  std::string goal;
  float confidence;
  std::string reason;
  std::vector<std::string> history_snapshot;
  int recursion_depth{0};
  std::string improvement_type;
  float safety_score{1.0f};
};
```

#### `RecursiveImprovementBounds`

| Bound | Value | Meaning |
| :--- | :--- | :--- |
| `max_recursion_depth` | 5 | Maximum self-improvement nesting |
| `min_confidence_threshold` | 0.1 | Below this, improvement is denied |
| `max_improvement_rate` | 2.0× | Confidence may not more than double per step |
| `max_consecutive_improvements` | 3 | Hard limit on back-to-back improvements |
| `requires_human_approval` | true | `meta_learning` and `cognitive_restructuring` require external approval |

#### `Tier4Loop` key API

```cpp
class Tier4Loop {
public:
  void observe(const std::string& observation);
  ReflectionTrace reflect();
  void refine();
  bool attempt_recursive_improvement(const std::string& improvement_type);
  bool apply_deterministic_meta_learning(const std::string& pattern, uint64_t seed);
  uint64_t get_learning_state_hash() const;
  void consume_snapshot(const t81::vm::ReflectionSnapshot& snapshot);
};
```

#### `TierAwarePlanner`

```text
complexity > 54 OR requires_self_reflection  →  Tier4
complexity > 27                              →  Tier3
complexity >  9                              →  Tier2
otherwise                                    →  Tier1
```

### 3.3. Promotion Heuristics

`should_promote_to_tier4(const ReflectionTrace&)` returns `true` when any
of the following hold:

- `confidence < 0.81`
- Three or more unresolved observations without successful refinement
- `reason` contains `"inconsistency"` or `"paradox"`

`try_promote(status, axion_callback)` elevates `TierStatus::current` by one
tier on each call; Axion may veto via a `Deny` verdict (returns
`PromotionError::AxionDenied`).

### 3.4. VM Tier Enforcement

Resource limits enforced by `core/vm/tier_limits.cpp` for Tier 4:

| Resource | Tier 4 Limit |
| :--- | :--- |
| Recursion depth | 243 |
| Max tensor rank | 7 |
| Max shape complexity | 81 × 81 × 7 |
| Max symbolic complexity | 2187 (3^7) |
| Max branch entropy | 243.0 |

The `CheckTier` policy opcode in `kernel/axion/policy_engine.cpp` denies
execution if `SyscallContext::current_tier < required`.

### 3.5. Determinism Guarantees

- `reflect()` output is fully determined by `SelfModel` state at call time.
- `apply_deterministic_meta_learning(pattern, seed)` uses an explicit seed;
  `get_learning_state_hash()` returns a reproducible hash of the learning state.
- All `Tier4Loop` operations emit Axion trace events via `log_reflection()`.
- `ReflectionTrace` artifacts stored in CanonFS are bit-exact across runs
  given identical initial `SelfModel` state.

## 4. Impact

- **API:** Headers in `include/t81/experimental/cog/tier4/`; promoted into
  `include/t81/experimental/cog/` namespace.
- **Performance:** Minimal overhead during reflection cycles; Axion trace size
  grows with cognitive metadata (~81 history entries per trace).
- **Storage:** `ReflectionTrace` artifacts stored in CanonFS under the
  standard content-addressed scheme.

## 5. Alternatives

- **Opaque Reflection:** Allowing non-deterministic "black-box" reflection
  (rejected — violates RFC-0002 determinism contract).
- **Tier 3 Extension:** Folding Tier 4 into Tier 3 (rejected — Tier 4 requires
  distinct self-modeling, recursion-depth tracking, and improvement bounds not
  appropriate at Tier 3).

## 6. Unresolved Questions

- **Optimal reflection frequency for real-time applications** — The observe →
  reflect → refine cadence is application-driven; the RFC imposes no scheduling
  policy. A scheduling recommendation may be added in a future revision.
- **Compression for large cognitive traces** — History is currently ring-buffered
  at 81 entries; structured compression of `ReflectionTrace` artifacts is future
  work and does not affect correctness.

______________________________________________________________________

## Acceptance Criteria

| ID | Criterion | Status |
| :--- | :--- | :--- |
| [A-0021-01] | `Tier4Loop` implements `observe()`, `reflect()`, `refine()` cycle; `ReflectionTrace` produced by `reflect()` carries goal, confidence, reason, history_snapshot, recursion_depth, improvement_type, and safety_score | met — `tier4_loop.hpp/cpp`; verified by `tier4_test.cpp` |
| [A-0021-02] | `SelfModel` ring-buffer history capped at 81 entries; beliefs map and current_goal updated deterministically | met — `self_model.hpp`; ring-buffer eviction in `add_history()`; `tier4_test.cpp` |
| [A-0021-03] | `RecursiveImprovementBounds` enforced: `attempt_recursive_improvement()` denied when confidence < 0.1, consecutive limit ≤ 3, recursion depth ≤ 5; `requires_human_approval` blocks meta_learning and cognitive_restructuring | met — `tier4_loop.cpp`; safety-bounds tests in `tier4_test.cpp` |
| [A-0021-04] | `apply_deterministic_meta_learning(pattern, seed)` is reproducible; `get_learning_state_hash()` returns stable hash for identical state | met — `tier4_loop.hpp/cpp`; `tier4_test.cpp` |
| [A-0021-05] | `TierAwarePlanner::select_tier()` maps complexity (1–81) and `requires_self_reflection` to correct `TierId`; Tier4 selected when complexity > 54 or self-reflection required | met — `planner.hpp`; `tier4_test.cpp` |
| [A-0021-06] | `should_promote_to_tier4()` triggers on confidence < 0.81, 3+ unresolved observations, or inconsistency/paradox in reason; `try_promote()` subject to Axion veto | met — `promotion.hpp/cpp`; `tier4_test.cpp` |
| [A-0021-07] | VM tier limits enforced for Tier 4: recursion depth 243, tensor rank 7, symbolic complexity 2187; `CheckTier` Axion policy opcode denies execution below required tier | met — `tier_limits.cpp`; `policy_engine.cpp`; `tier4_vm_test.cpp` |
| [A-0021-08] | All Tier 4 `reflect()` events emit Axion audit trace via `log_reflection()`; `ReflectionTrace` artifacts are bit-exact given identical initial `SelfModel` | met — `tier4_loop.cpp`; `tier4_vm_test.cpp` |
| [A-0021-09] | `consume_snapshot()` integrates a `ReflectionSnapshot` from the VM into the active `SelfModel` without breaking determinism | met — `tier4_loop.hpp/cpp`; `tier4_vm_test.cpp` |
| [A-0021-10] | Loop closure and promotion lifecycle validated in multi-step scenarios including distributed tier semantics | met — `tier4_loop_closure_test.cpp`; `test_tier4_distributed.cpp` |

## Acceptance Note (2026-03-15)

All 10 acceptance criteria are met. The Tier 4 cognition layer is fully
implemented in `t81::cog::v1` with:

- A deterministic `observe → reflect → refine` loop (`Tier4Loop`) backed by
  a bounded `SelfModel` (81-entry ring buffer).
- `ReflectionTrace` as the canonical audit artifact — carrying goal, confidence,
  reason, history snapshot, recursion depth, improvement type, and safety score.
- Hard-bounded recursive self-improvement (`RecursiveImprovementBounds`) with
  human-approval gating for `meta_learning` and `cognitive_restructuring`.
- Deterministic meta-learning with explicit seed and state-hash reproducibility.
- `TierAwarePlanner` mapping complexity (1–81 scale) to `TierId`.
- Promotion heuristics (`should_promote_to_tier4`, `try_promote`) with Axion
  veto path.
- VM-level resource limits for Tier 4 enforced by `tier_limits.cpp` and
  `CheckTier` policy opcode.
- Test coverage across four test suites: `tier4_test`, `tier4_loop_closure_test`,
  `tier4_vm_test`, `test_tier4_distributed`.

The two open questions (reflection scheduling cadence and trace compression)
are acknowledged as future work and do not affect the correctness of the
current implementation.

This acceptance closes the one partial criterion ([A-0000-22]) in RFC-0000.
