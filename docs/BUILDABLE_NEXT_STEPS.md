# T81 Buildable Next Steps

These are the best next tasks for a contributor who wants to ship something real instead of only studying the architecture.

## Priority board

Use this board when deciding what to do next.

### Now

- RFC-00D1 hardening:
  tighten CanonFS interchange denial reasons, provenance reporting, and
  negative-path coverage before expanding the subsystem
- RFC-00D1 CLI/schema alignment:
  keep `canonfs import` / `canonfs export` JSON, exit codes, and docs aligned
  with the core interchange contract
- CI and portability boringness:
  remove concrete toolchain/setup footguns, especially in build, demo, and
  smoke-test paths contributors are likely to try early

### Next

- RFC-00D1 contract promotion review:
  separate stable v1 interchange contract from draft implementation choices and
  write promotion blockers down explicitly
- Public-story cleanup:
  keep the runtime-first story, handoff docs, and contributor roadmap aligned
  with the strongest usable surfaces
- AI CLI truth-in-labeling:
  keep `t81 ai ...`, `weights ...`, and `internal llama-run` documentation
  aligned with what currently executes versus what is still scaffolded
- Reusable core extraction:
  move behavior that currently lives only in CLI glue into reusable core
  modules when it reduces maintainer memory load

### Later

- RFC-00D0 resolver prototype:
  implement deterministic service descriptor loading and resolution without
  claiming a full TCP/IP stack
- Additional subsystem breadth only after the current runtime, CanonFS, and CI
  lanes are easier for a new contributor to pick up than they are today

## Suggested issue list

Work these in order unless an active regression or CI failure is blocking the
repo.

1. Tighten CanonFS import denial reasons and add matching negative tests.
2. Tighten CanonFS export denial reasons and add matching negative tests.
3. Add RFC-00D1 CLI contract coverage for failure cases and JSON error shapes.
4. Keep interchange JSON payloads, schema IDs, and docs aligned.
5. Add or refresh one golden CanonFS interchange example under `examples/`.
   Current seed example: `examples/canonfs-interchange/`.
6. Move reusable interchange behavior out of CLI-only code where practical.
7. Add fail-fast checks for remaining QEMU/EFI toolchain and smoke-path footguns.
8. Trim the highest-friction contributor-path docs so they stay short and current.
9. Review workflow overlap and consolidate only where it lowers maintenance cost.
10. Review RFC-00D1 for partial contract promotion after behavior stops moving.
11. Replace synthetic `t81 ai inference run` payload generation with one real native execution lane.
    Current state:
    the strict deterministic `t81_reference_vm` lane is now real and runs a
    bounded native Llama-shaped probe with tokenizer-aware candidate
    selection, row-wise logits scoring, and a bounded decode trace with
    `--max-tokens` support, `termination_reason`, `final_decode_state`, and an
    explicit bounded native decode-state shape. That decode trace now
    separates decode-state carry from candidate-window seeding so the payload
    is clearer about what is true state versus current heuristic selection, and
    later candidate windows are now seeded from a model-computed carry
    signature taken from the real logits sample.
    Later trace steps now use the previously selected/generated token as the
    actual embed input and can blend a bounded recent token-history context,
    and step `0` can now start with bounded prompt-token context when the
    tokenizer yields multiple prompt tokens. Multi-token prompts can also seed
    the initial candidate window from full prompt-token history, and the
    bounded context window now preserves the original prompt anchor token.
    The lane now also exports real intermediate hidden-tensor evidence from
    live VM state and reimports a bounded carried hidden tensor into later
    decode probes through the compiled tensor-pool path and now blend that
    carried tensor with the current `attn0` path, so bounded hidden
    state carry is now real. Repeated weak or recovery-grade
    bounded decode steps now stop intentionally with
    `termination_reason: "stability_recovery_exhausted"` instead of silently
    extending the trace. Checked-in runnable examples now prove `ready`,
    `guarded`, and `degraded`. The bounded decode state now also carries an
    explicit `forward_state` object derived from the hidden projection.
    That state now records `forward_state_generation`, persists a bounded slice
    of prior carried rows with decay, and has a checked-in 3-step probe via
    `examples/model-load-canonfs/run_forward_state_ai_probe.sh`.
    That 3-step path now visibly shifts the third decode step into a
    history-heavy forward-state mode. The remaining limitation is narrower
    now: general KV-cache or arbitrary intermediate-tensor import is still
    left to do.

## Pick one lane

- If you want the best chance of shipping code quickly, start with RFC-00D1.
- If you want design work more than code, start with RFC-00D0.
- If you want project leverage more than subsystem depth, start with public-story cleanup or CI boringness work.
- If you want the highest-value AI product step, start with the `t81 ai` implementation matrix and replace one scaffolded inference command with a real execution path.

## 1. RFC-00D1 policy-profile depth

Why this matters:

- RFC-00D1 already has code, schemas, tests, and CI validation
- the next value is better behavior, not more scaffolding

Concrete work:

- replace the current narrow built-in profiles with richer interchange policy decisions
- add explicit policy-profile docs and examples
- extend import/export denial reasons and provenance details

Key files:

- [interchange_ops.hpp](/Users/t81dev/Code/t81-foundation/include/t81/canonfs/interchange_ops.hpp)
- [canonfs_interchange_ops.cpp](/Users/t81dev/Code/t81-foundation/fs/canonfs_interchange_ops.cpp)
- [driver.cpp](/Users/t81dev/Code/t81-foundation/tools/cli/driver.cpp)
- [canonfs_interchange_test.cpp](/Users/t81dev/Code/t81-foundation/tests/cpp/canonfs_interchange_test.cpp)
- [cli_contract_test.cpp](/Users/t81dev/Code/t81-foundation/tests/cpp/cli_contract_test.cpp)

Definition of done:

- policy behavior is more expressive than allow/deny-by-profile
- tests cover new denial/reporting cases
- CLI and core API remain aligned
- schema artifacts and emitted JSON stay aligned

## 2. RFC-00D1 contract promotion review

Why this matters:

- the subsystem is implemented enough that some parts may be ready to stop moving

Concrete work:

- review which RFC-00D1 surfaces are stable enough to promote from `draft`
- separate stable interchange contract from still-fluid implementation choices
- tighten docs around supported v1 source/target kinds and explicit deferrals

Key files:

- [RFC-00D1-canonfs-foreign-filesystem-interchange.md](/Users/t81dev/Code/t81-foundation/spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)
- [spec/rfcs/index.md](/Users/t81dev/Code/t81-foundation/spec/rfcs/index.md)
- [docs/HANDOFF.md](/Users/t81dev/Code/t81-foundation/docs/HANDOFF.md)

Definition of done:

- a contributor can tell which RFC-00D1 surfaces are stable
- promotion blockers are written down explicitly

Suggested first read:

- [RFC-00D1-canonfs-foreign-filesystem-interchange.md](/Users/t81dev/Code/t81-foundation/spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)
- [docs/HANDOFF.md](/Users/t81dev/Code/t81-foundation/docs/HANDOFF.md)

## 3. RFC-00D0 resolver prototype

Why this matters:

- it is the narrowest useful networking step
- it exercises base-81 identity, CanonFS manifests, and evidence without requiring a full TCP/IP stack immediately

Concrete work:

- implement service descriptor loading and deterministic resolution
- keep it separate from actual packet-stack work
- emit stable evidence for resolve decisions

Key files:

- [RFC-00D0-base81-aware-tcp-ip-stack.md](/Users/t81dev/Code/t81-foundation/spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md)
- [spec/rfcs/index.md](/Users/t81dev/Code/t81-foundation/spec/rfcs/index.md)
- likely new code under `fs/`, `tools/`, or a future `net/` module

Definition of done:

- service descriptors can be loaded and resolved deterministically
- evidence shows original T81 identity plus resolved transport target
- no claim is made yet that the full TCP/IP stack exists

Suggested first read:

- [RFC-00D0-base81-aware-tcp-ip-stack.md](/Users/t81dev/Code/t81-foundation/spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md)
- [spec/rfcs/index.md](/Users/t81dev/Code/t81-foundation/spec/rfcs/index.md)

## 4. Public-story cleanup

Why this matters:

- adoption is currently limited more by legibility than by lack of architecture

Concrete work:

- keep the top-level docs synchronized with actual maturity
- avoid overlapping or contradictory roadmap/status messages
- keep newcomer-facing docs short and current

Key files:

- [README.md](/Users/t81dev/Code/t81-foundation/README.md)
- [docs/README.md](/Users/t81dev/Code/t81-foundation/docs/README.md)
- [docs/HANDOFF.md](/Users/t81dev/Code/t81-foundation/docs/HANDOFF.md)
- [docs/ROADMAP.md](/Users/t81dev/Code/t81-foundation/docs/ROADMAP.md)

Definition of done:

- a new engineer can identify the current product surface quickly
- the handoff set stays small and current

## 5. CI boringness work

Why this matters:

- T81 loses momentum quickly if contributors spend their first day fighting formatting, Windows portability, or sparse-checkout drift

Concrete work:

- keep portability helpers centralized
- keep workflow assumptions current
- fix CI regressions quickly and narrowly

Key files:

- [CMakeLists.txt](/Users/t81dev/Code/t81-foundation/CMakeLists.txt)
- [.github/workflows/ci.yml](/Users/t81dev/Code/t81-foundation/.github/workflows/ci.yml)
- [.github/workflows/qemu-boot-x86.yml](/Users/t81dev/Code/t81-foundation/.github/workflows/qemu-boot-x86.yml)

Definition of done:

- CI failures are mostly about real behavior, not project hygiene drift

Current workflow audit:

- Keep these lanes distinct:
  `ci.yml` as the main integration lane, `cross-platform-determinism.yml` as the
  multi-OS hash-verification lane, `qemu-boot.yml` / `qemu-boot-x86.yml` as
  architecture-specific boot capture lanes, and `python-wheels.yml` /
  `release.yml` as packaging lanes.
- First safe consolidation candidate:
  fold `deterministic-profile-enforcement.yml` into a named job inside
  `ci.yml`. It runs on the same push / pull_request cadence as the main CI
  lane and overlaps with deterministic contract enforcement already represented
  there.
- Second safe consolidation candidate:
  review whether `runtime-contract.yml` should remain standalone or become a
  narrow `ci.yml` job. Its trigger shape mirrors the main CI path, so the bar
  for staying separate should be a materially different ownership or failure
  policy.
- Third safe consolidation candidate:
  reduce benchmark workflow duplication across `bench.yml`,
  `benchmark_packed_trit_vector.yml`, `inference-bench.yml`, and
  `repro-ledger.yml` by centralizing shared benchmark setup and keeping only one
  PR-sensitive regression gate. The rest should stay manual or scheduled unless
  they protect a current product contract.

Recommended order:

1. Consolidate deterministic-profile enforcement into `ci.yml`.
2. Decide whether runtime-contract remains a standalone contract lane.
3. Centralize shared benchmark workflow setup before merging or deleting any
   benchmark workflows.
