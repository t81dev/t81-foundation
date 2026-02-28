# T81Lang Standard Library Stabilization Plan

Date: 2026-02-26
Status: Sprint 2 In Progress (updated 2026-02-28)
Owner: Language + Governance

## 1. Goal

Stabilize the T81Lang standard library as a governed, deterministic interface
surface suitable for long-lived programs, without broadening determinism claims
beyond DCP/registry boundaries.

## 2. Why This Is Next

Core ISA/VM/governance hardening has reached a point where user-facing
durability is now the highest leverage work. For a ternary deterministic
language, stdlib stability is the bridge from runtime correctness to practical
program construction.

## 3. Scope (This Sprint)

In-scope modules (freeze candidates):

- `std.core`
- `std.math` (bounded deterministic profile)
- `std.io`
- `std.collections`
- `std.text`
- `std.bytes`
- `std.symbol`
- `std.sys`
- `std.async`
- `std.tensor`
- `std.agent`

Out-of-scope:

- introducing new nontrivial module families
- expanding non-DCP claims
- changing VM determinism profile boundaries

## 4. Stabilization Principles

1. Module surface changes require explicit governance records.
2. Determinism claims remain bounded to registry-verified surfaces.
3. Observable behavior is locked by fixture-driven CLI tests and conformance
   suites.
4. Any host-dependent math behavior is explicitly labeled and policy-bounded.
5. Experimental cognitive/agentic surfaces remain marked non-DCP unless promoted.

## 5. Acceptance Gates

1. `scripts/governance/check_stdlib_surface_baseline.py` passes.
2. `scripts/governance/check_stdlib_promotion_snapshot.py` passes.
3. CLI std fixture suites remain green:
   - `cli_std_core_fixtures_test`
   - `cli_std_math_fixtures_test`
   - `cli_std_text_fixtures_test`
   - `cli_std_bytes_fixtures_test`
   - `cli_std_collections_fixtures_test`
   - `cli_std_tensor_fixtures_test`
   - `cli_std_runtime_fixtures_test`
   - `cli_std_symbol_fixtures_test`
4. `scripts/ci/run_determinism_slice.sh build` remains green.
5. Documentation alignment is maintained between:
   - `docs/standards/standard-library.md`
   - `lang/stdlib/std/*.t81`
   - task/status artifacts
6. No broadening of DCP/verified claims in public docs.

## 6. Work Packages

1. Surface freeze manifest and CI gate
2. Behavioral invariant expansion per stdlib module group
3. Determinism-boundary labeling cleanup in stdlib docs
4. Semver/promotion criteria for stdlib module changes
5. Release-note and control-center synchronization

## 7. Exit Criteria

1. Stdlib surface baseline gate is enforced in CI and governance hygiene.
2. Module coverage is traceable from docs to fixtures/tests.
3. Open stdlib stabilization tasks in `docs/roadmaps-plans/TASKS.md` are closed
   or explicitly deferred with rationale.

## 8. Sprint 1 Closure (2026-02-26)

1. Surface baseline governance gate implemented and CI-enforced.
2. Promotion snapshot artifact implemented and governance-checked.
3. `std.core` and `std.math` fixture conformance suites added and passing.
4. Change taxonomy policy published in `docs/governance/STDLIB_CHANGE_POLICY.md`.

## 9. Sprint 2 (2026-02-27..28, In Progress)

### Completed in Sprint 2

The following fixture suites and stdlib modules shipped during the 2026-02-26..28
implementation sprint as part of the collections, surface hardening, and
determinism audit workstreams:

1. `std.polynomial` — `lang/stdlib/std/polynomial.t81` implemented; fixture
   conformance suite added (`tests/fixtures/t81lang_std_polynomial/`,
   `tests/cpp/cli_std_polynomial_fixtures_test.cpp`).
2. `std.symbolic` — `lang/stdlib/std/symbolic.t81` implemented; fixture
   conformance suite added (`tests/fixtures/t81lang_std_symbolic/`,
   `tests/cpp/cli_std_symbolic_fixtures_test.cpp`).
3. `std.symbol` — fixture conformance suite extended
   (`tests/fixtures/t81lang_std_symbol/`).
4. `std.collections` — Map/Set operation fixtures added
   (`tests/fixtures/t81lang_std_collections/03..09`); collections determinism
   test added (`tests/cpp/cli_std_collections_determinism_test.cpp`).

### Remaining in Sprint 2

The following in-scope modules (from section 3) still lack fixture conformance
suites as of 2026-02-28:

| Module | Fixture Suite | Status |
| :--- | :--- | :--- |
| `std.io` | Not yet created | Open |
| `std.text` | Not yet created | Open |
| `std.bytes` | Not yet created | Open |
| `std.sys` | Not yet created | Open |
| `std.async` | Not yet created | Open |
| `std.tensor` | Not yet created | Open |
| `std.agent` | Not yet created | Open |

Note: `std.polynomial` and `std.symbolic` were not in the original Sprint 1
in-scope list; they shipped as part of the surface hardening workstream and
their fixture suites count toward Sprint 2 closure evidence.

### Sprint 2 Exit Criteria

Same acceptance gates as section 5, applied to the remaining 7 modules above.
Sprint 2 is complete when all 7 modules have passing fixture conformance suites
and the governance gates in section 5 remain green.
