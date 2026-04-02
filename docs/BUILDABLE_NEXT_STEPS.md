# T81 Buildable Next Steps

These are the best next tasks for a contributor who wants to ship something real instead of only studying the architecture.

## Priority board

Use this board when deciding what to do next.

### Now

- RFC-00D1 policy-profile depth:
  build on the now-documented built-in profile surface by improving policy
  documents, examples, and any remaining profile-shaped denial clarity without
  broadening interchange format scope
- CI and portability boringness:
  remove concrete toolchain/setup footguns, especially in build, demo, and
  smoke-test paths contributors are likely to try early

### Next

- RFC-00D1 contract promotion follow-through:
  keep the current v1 candidate surface explicit in docs/examples and only
  promote more once the remaining RFC-scoped blockers actually stop moving
- Public-story cleanup:
  keep the runtime-first story, handoff docs, and contributor roadmap aligned
  with the strongest usable surfaces
- Bounded family stabilization:
  keep the admitted AI OS-object family baseline, identity invariant,
  admission gate, negative-path enforcement, and maintainer status note
  aligned; do not add a fourth composition unless there is a concrete need
- Claim discipline:
  keep public and external-facing descriptions aligned with the currently
  validated bounded family and the documented Canonical Identity Invariant;
  use the deterministic AI OS-object reference memo to avoid overclaiming
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
   Current seed example: `examples/storage-and-canonfs/canonfs-interchange/`.
6. Move reusable interchange behavior out of CLI-only code where practical.
7. Add fail-fast checks for remaining QEMU/EFI toolchain and smoke-path footguns.
8. Trim the highest-friction contributor-path docs so they stay short and current.
9. Review workflow overlap and consolidate only where it lowers maintenance cost.
10. Keep public claims aligned with the validated bounded AI OS-object family.
    Current reference:
    `docs/explanation/DETERMINISTIC_AI_OS_OBJECT_SUBSTRATE_REFERENCE_MEMO.md`
    should be the standard check before broadening external language about
    DAIOS, AI operating systems, or generalized deterministic AI claims.
11. Keep the admitted bounded AI OS-object family boring.
    Current reference surfaces:
    `docs/status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md`,
    `docs/reference/AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md`,
    and `docs/reference/AI_OS_OBJECT_CHAIN_CATALOG.md`.
    The next work on this lane should be stabilization and misuse-path
    hardening, not a fourth composition unless a concrete consumer justifies it.
12. (DONE) Review RFC-00D1 for partial contract promotion after behavior stops moving.
    Current state:
    RFC-00D1 now explicitly names the current CanonFS interchange v1 candidate
    contract instead of leaving contributors to infer it from code and tests.
    The RFC now calls out:
    - the stable CLI operations, schema ids, source/target kinds, and
      policy-profile names
    - the stable structured error entry fields `kind`, `code`, `reason`, and
      `message`
    - the explicit result linkage fields `provenance_schema` and
      `manifest_schema`
    - the remaining promotion blockers that still keep the broader RFC in
      `draft`

    The practical result is that a contributor can now tell which interchange
    fields are the current v1 candidate surface and which questions are still
    intentionally deferred.
13. (DONE) Promote the bounded native `t81 ai inference run` lane into a reusable runtime state path.
    Current state:
    the strict deterministic `t81_reference_vm` lane is already real. It now
    runs a bounded native Llama-shaped probe with tokenizer-aware candidate
    selection, row-wise logits scoring, explicit readiness posture, checked-in
    `ready` / `guarded` / `degraded` examples, and a bounded decode path with
    real carried hidden-tensor, q/k, forward-state, and combined
    architecture-state evidence. That lane now reaches a bounded 4-step
    horizon, has a distinct deep fourth-step architecture-state mode, and
    emits explicit horizon metadata such as `bounded_horizon_steps`,
    `bounded_horizon_remaining`, `bounded_horizon_utilization`, and
    `termination_reason: "deep_architecture_state_horizon_reached"`.
    This means the old “replace the synthetic payload” task is effectively
    complete for the strict deterministic reference-VM path.
    It also now has:
    - a minimal public runtime decode surface (`t81::vm::Decoder`)
    - explicit public decode-state transition helpers
    - a public native probe API plus shared request builders
    - CLI convergence onto the same public probe/runtime path
    - direct decoder/public-probe tests plus the existing CLI contract slice

    The practical result is that the old extraction/convergence task is done
    enough that it should not stay on the active board unless a new concrete
    consumer appears.

    Additional progress now completed on this lane:
    1. (DONE) Promote the carried hidden/qk/forward/architecture-state handoff
       from compiled-literal tensor patching to a reusable
       intermediate-state object in the public VM path.
    2. (DONE) Add another non-CLI consumer of the public decode/probe path by
       exercising the shared request-builder and intermediate-state handoff
       directly in VM-side tests, alongside the public `Decoder`.

    3. (DONE) Add a narrow true greedy decode runtime path for the single
       supported `llama-dense-v1` architecture, reusing the public
       decode/probe surfaces and `IntermediateDecodeState` continuation
       contract rather than creating a parallel stack.

    If this lane is revisited later, the next worthwhile work is:
    1. Only after that, consider step-5-plus decode horizons for the same
       narrow architecture, or another equally narrow follow-on decode mode.
    2. Keep `benchmark run` and `policy test` out of scope for this lane until
       their current scaffolded behavior is explicitly being replaced.

14. (DONE) Harden the CanonFS interchange contract surface.
    Current state:
    the RFC-00D1 CanonFS interchange lane now emits explicit structured error
    reasons from core import/export operations, carries those same reasons
    through the CLI JSON surface, and validates the resulting contract shapes in
    both dedicated interchange tests and the existing CLI contract slice.
    It also now has:
    - explicit stable `reason` values in interchange error objects
    - deterministic `kind` / `code` mapping derived from those reasons
    - aligned `canonfs import` / `canonfs export` JSON rendering in core and CLI
    - provenance/schema fields made explicit in result documents
    - negative coverage for malformed manifest, invalid schema, missing source,
      missing object, policy denial, and hash mismatch
    - a checked-in golden example at
      `examples/storage-and-canonfs/canonfs-interchange/`

    The practical result is that CanonFS interchange is now much closer to a
    contributor-facing contract surface than an implementation detail.

    If this lane is revisited next, the highest-value work is:
    1. review which RFC-00D1 interchange fields and schemas are stable enough
       to promote explicitly as v1 contract surface
    2. tighten policy-profile docs/examples rather than adding new interchange
       formats or broader subsystem scope

15. (DONE) Make the built-in RFC-00D1 policy-profile surface explicit.
    Current state:
    the four shipped built-in interchange policy profiles now have a reusable
    core description, user-visible CLI help text, example coverage, and direct
    core plus CLI tests. The repo now says plainly what `permissive`,
    `import-only`, `export-only`, and `deny-all` do instead of leaving those
    semantics implicit in code and denials.

    It now has:
    - reusable core profile metadata for name, import/export allowance, and
      summary
    - `t81 canonfs` help text that explains the built-in profiles directly
    - golden-example docs covering the current narrow profile semantics
    - direct tests for all four built-in profiles, including `deny-all`

    The practical result is that the built-in profile surface is now a real,
    contributor-facing contract instead of maintainer memory.

    Additional progress now completed on this lane:
    1. (DONE) Add a small checked-in policy-document example that shows how an
       explicit policy file can narrow an otherwise allowed built-in profile
       into a real `partial` import/export result.

       Current state:
       the CanonFS interchange example now includes an `alpha`-only checked-in
       policy file plus CLI contract coverage that demonstrates:
       - built-in profiles and explicit policy files are conjunctive rather
         than competing control planes
       - directory import can succeed as `partial` when one path is admitted
         and another is denied by policy
       - directory export can likewise materialize a subset of manifest entries
         while still emitting explicit `policy_denied` evidence

       The practical result is that a contributor can now learn the current
       `ok` / `partial` / `error` interchange posture from checked-in examples
       instead of inferring it from implementation details.

    If this lane is revisited next, the highest-value work is:
    1. add one more equally small policy-document example only if it clarifies a
       distinct policy-file interaction beyond the new partial-result case
    2. keep policy denial reporting explicit without expanding interchange
       formats or adding new built-in profile families

## Pick one lane

- If you want the best chance of shipping code quickly, start with RFC-00D1.
- If you want design work more than code, start with RFC-00D0.
- If you want project leverage more than subsystem depth, start with public-story cleanup or CI boringness work.
- If you want the highest-value AI product step later, start from a concrete new
  consumer of the public `t81::vm` decode/probe path rather than reopening the
  just-finished CLI extraction work.

## 1. RFC-00D1 policy-profile depth

Why this matters:

- RFC-00D1 already has code, schemas, tests, and CI validation
- the next value is better behavior, not more scaffolding

Concrete work:

- add one or two small policy-document examples that build on the current
  built-in profiles, with at least one checked-in example now covering a real
  `partial` import/export result
- keep explicit policy-profile docs and examples aligned with CLI/core behavior
- extend import/export denial reasons and provenance details

Key files:

- [interchange_ops.hpp](../include/t81/canonfs/interchange_ops.hpp)
- [canonfs_interchange_ops.cpp](../fs/canonfs_interchange_ops.cpp)
- [examples/storage-and-canonfs/canonfs-interchange/README.md](../examples/storage-and-canonfs/canonfs-interchange/README.md)
- [driver.cpp](../tools/cli/driver.cpp)
- [canonfs_interchange_test.cpp](../tests/cpp/canonfs_interchange_test.cpp)
- [cli_contract_test.cpp](../tests/cpp/cli_contract_test.cpp)

Definition of done:

- a contributor can understand the built-in profiles and at least one policy
  document example, including a real `partial` result, without reading the
  implementation
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

- [RFC-00D1-canonfs-foreign-filesystem-interchange.md](../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)
- [spec/rfcs/index.md](../spec/rfcs/index.md)
- [docs/HANDOFF.md](HANDOFF.md)

Definition of done:

- a contributor can tell which RFC-00D1 surfaces are stable
- promotion blockers are written down explicitly

Suggested first read:

- [RFC-00D1-canonfs-foreign-filesystem-interchange.md](../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)
- [docs/HANDOFF.md](HANDOFF.md)

## 3. RFC-00D0 resolver prototype

Why this matters:

- it is the narrowest useful networking step
- it exercises base-81 identity, CanonFS manifests, and evidence without requiring a full TCP/IP stack immediately

Concrete work:

- implement service descriptor loading and deterministic resolution
- keep it separate from actual packet-stack work
- emit stable evidence for resolve decisions

Key files:

- [RFC-00D0-base81-aware-tcp-ip-stack.md](../spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md)
- [spec/rfcs/index.md](../spec/rfcs/index.md)
- likely new code under `fs/`, `tools/`, or a future `net/` module

Definition of done:

- service descriptors can be loaded and resolved deterministically
- evidence shows original T81 identity plus resolved transport target
- no claim is made yet that the full TCP/IP stack exists

Suggested first read:

- [RFC-00D0-base81-aware-tcp-ip-stack.md](../spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md)
- [spec/rfcs/index.md](../spec/rfcs/index.md)

## 4. Public-story cleanup

Why this matters:

- adoption is currently limited more by legibility than by lack of architecture

Concrete work:

- keep the top-level docs synchronized with actual maturity
- avoid overlapping or contradictory roadmap/status messages
- keep newcomer-facing docs short and current

Key files:

- [README.md](../README.md)
- [docs/README.md](README.md)
- [docs/HANDOFF.md](HANDOFF.md)
- [docs/ROADMAP.md](ROADMAP.md)

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

- [CMakeLists.txt](../CMakeLists.txt)
- [.github/workflows/ci.yml](../.github/workflows/ci.yml)
- [.github/workflows/qemu-boot-x86.yml](../.github/workflows/qemu-boot-x86.yml)

Definition of done:

- CI failures are mostly about real behavior, not project hygiene drift

Current workflow audit:

- Keep these lanes distinct:
  `ci.yml` as the main integration lane, `cross-platform-determinism.yml` as the
  multi-OS hash-verification lane, `qemu-boot.yml` / `qemu-boot-x86.yml` as
  architecture-specific boot capture lanes, and `python-wheels.yml` /
  `release.yml` as packaging lanes.
- First safe consolidation candidate:
  (DONE) fold `deterministic-profile-enforcement.yml` into a named job inside
  `ci.yml`.

  Current state:
  the deterministic-profile static/file checks now live in the main CI
  workflow as `product / deterministic profile enforcement`, while the
  duplicate standalone workflow and duplicate rebuild/test path are gone.
- Second safe consolidation candidate:
  (DONE) review whether `runtime-contract.yml` should remain standalone or
  become a narrow `ci.yml` job.
- Third safe consolidation candidate:
  (IN PROGRESS) reduce benchmark workflow duplication across `bench.yml`,
  `benchmark_packed_trit_vector.yml`, `inference-bench.yml`, and
  `repro-ledger.yml` by centralizing shared benchmark setup and keeping only one
  PR-sensitive regression gate. The rest should stay manual or scheduled unless
  they protect a current product contract.

Recommended order:

1. (DONE) Consolidate deterministic-profile enforcement into `ci.yml`.
2. (DONE) Keep `runtime-contract.yml` standalone for now.
   Current state:
   this lane has materially different coupling from the main CI path:
   - it checks out the external `t81-vm` repository rather than only this repo
   - it enforces cross-repo contract/tag/pin alignment, not just in-repo build
     or test behavior
   - it carries a distinct `workflow_dispatch` approval input for intentional
     contract-marker drift (`allow_contract_change`)
   - its nightly cadence and approval semantics are part of the contract lane,
     not just generic CI repetition

   The practical result is that folding it into `ci.yml` right now would mix a
   cross-repo approval gate into the main integration lane without obviously
   reducing maintainer load.
3. (IN PROGRESS) Centralize shared benchmark workflow setup before merging or deleting any
   benchmark workflows.
   Current state:
   the benchmark-specific workflows now share a reusable benchmark-runner
   bootstrap helper under `scripts/ci/`, so the remaining work is no longer
   basic CMake duplication. The next decision is which single PR-sensitive
   benchmark regression gate should remain prominent once workflow purposes are
   compared side by side.
   Additional progress now completed:
   - `bench.yml` remains the primary benchmark-sensitive lane because it owns
     the current VM workload guardrail
   - `benchmark_packed_trit_vector.yml` has been narrowed to scheduled/manual
     specialist coverage instead of remaining a second push / pull_request
     benchmark workflow
   - `inference-bench.yml` has been narrowed to scheduled/manual plus release
     publishing because it refreshes a benchmark report rather than protecting a
     single narrow PR regression contract
