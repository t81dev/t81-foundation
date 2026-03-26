# T81 Buildable Next Steps

These are the best next tasks for a contributor who wants to ship something real instead of only studying the architecture.

## Pick one lane

- If you want the best chance of shipping code quickly, start with RFC-00D1.
- If you want design work more than code, start with RFC-00D0.
- If you want project leverage more than subsystem depth, start with public-story cleanup or CI boringness work.

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
