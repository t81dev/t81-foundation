# C++ SDK 1.x to 2.x migration notes

**Status:** non-normative note, recorded 2026-11-23.  
**Companion:** draft [RFC-00E9](../../spec/rfcs/RFC-00E9-cpp-sdk-2-compatibility-transition.md).  
**Does not:** accept that RFC, change `include/t81/**`, change
`docs/governance/PUBLIC_API_LOCK.json`, mark SDK 1.x deprecated or in
maintenance, or authorize a merge or release.

This note collects consumer-facing compatibility text that is already published
and the historical checklist from the draft RFC. It is not the freeze
procedure's normative migration guide. `docs/status/IMPLEMENTATION_MATRIX.md`
and the changelog are unchanged.

## Published v2.0.0 consumer notes

[Release v2.0.0](../releases/v2.0.0.md) states all of the following. This note
does not add requirements beyond that text:

- Project packaging and the public API version metadata were aligned to 2.0.0
  in `CMakeLists.txt`, `pyproject.toml`, `include/t81/config.hpp`, and
  `PUBLIC_API_LOCK.json`.
- CanonFS portable storage uses `h1-` lowercase-hex filename components and
  `.canonfs-format = t81-canonfs-hex-v1`. Unmarked nonempty legacy stores are
  rejected with an explicit migration error. Back up stores and capability
  metadata before migrating them.
- Extension, vector, and FFI opcode assignments, and `T81Fraction`
  exception-safety and class layouts, changed relative to `v1.9.4`. Recompile
  bytecode artifacts and C++ consumers against a matched header and library set.
- `AxionVerdict` defaults to deny. Policy hooks return an explicit allow verdict
  after their checks.

Later release notes do not replace those statements.
[Release v2.1.0](../releases/v2.1.0.md) records a package and lock sync to
2.1.0. [Release v2.2.0](../releases/v2.2.0.md) is a draft Python SDK note and
does not contain a C++ ABI section.

## Historical source checklist

The draft RFC records source and layout findings against pinned commits from
2026-10-30, including `ValueTag` numeric values, `FFICallResult` alternative
order, VM and syscall context members, exact `compile` / `run_tisc` /
`debug_tisc` function types, monitoring mutex members, the standalone global
`int128_t` alias, and `ShellSession` polymorphism. Those findings were not
re-probed with a compiler for this note.

Until a maintainer accepts a revised checklist, a consumer moving off those
older headers should:

- rebuild against one matched SDK header and library set;
- stop using historical variant indices and old exact function-pointer types;
- stop interpreting old tag numbers, old aggregate positions, or old in-memory
  object bytes as current objects;
- construct VM and policy contexts with current initializers;
- review shell inheritance, monitoring integration, and the removal of the
  standalone global integer alias;
- review policy-denial and formatting behavior separately from compile fixes.

In-memory objects and variant bytes are not a portable serialization format.
Frozen TISC encodings, canonical serialization, content identity, and
authorization behavior keep their own contracts. Do not renumber frozen wire
values or weaken authorization to imitate an older SDK.

## What was rechecked on 2026-11-23

Read-only Git and validator checks, on Linux with Python 3.12.3, against main
`5a005b2dea5e1eb262d4af45698f8338d16b137c`:

- `include/t81/config.hpp` says 2.2.0. `PUBLIC_API_LOCK.json` says 2.2.0 and
  digest `06b7a31088bc85dd6efa4c8dff97c41777314204df124840594a720577b746e3`.
  `python3 -B scripts/governance/check_public_api_semver.py` passed. The tree
  had 253 headers and no untracked `include/t81/**/*.hpp` files.
- The same digest matches tag `v2.2.0` at
  `7aa734ea48c8133f620398cdad1767b561ff69b2`. Component-order digests also match
  the locks at `v2.0.0` (`7d4582ce5472bb78420940d35a3aa03b4487101f`, 2.0.0) and
  `v2.1.0` (`9ddeeccda3170f1d12d262f757d56a0c8c903485`, 2.1.0).
- Tag `v2.0.1` (`f3b41b5832bc2b2a897eda10d8cc26731e98ec3f`) changes only the
  patch macros in `config.hpp` relative to `v2.0.0`. Its lock remains 2.0.0.
  The component-order digest is
  `90e451bf4cafe90378032f6a6ad4bb6aa5a89754a4ae049a850551e7c08c0be8`, which does
  not match that lock. Later tags put the version and lock back on one number.
- Tag `v1.9.4` (`295fc67245179f1b26e5284c70520019e72bb6c3`) has config macros
  1.9.1, lock version 1.9.0, and 233 headers. Neither path ordering matches the
  lock. This note does not treat that tag as a verified ABI baseline.
- From the October reviewed main
  `f775680aba7db200691c0f4e2a3174b0e7e51581` to the measured main tip, the
  public-header path delta is `canonfs/storage_key.hpp` (added) and
  modifications to `config.hpp`, `frontend/semantic_analyzer.hpp`,
  `ternary/arith.hpp`, `types/T81BigInt.hpp`, `types/T81Bytes.hpp`,
  `types/T81Fraction.hpp`, `types/T81IOStream.hpp`, `types/T81Int.hpp`,
  `types/T81Int128.hpp`, `types/T81Limb.hpp`, and `types/T81Map.hpp`. The delta
  is an inventory, not a new break classification.
- A passing lock does not prove that an older binary, an older source file, or
  another compiler ABI still works.

No Clang syntax probe from the October record was repeated. No mixed-binary,
cross-compiler, or installed-package test was run for this note. No supported
compiler/standard-library matrix is published here.

## Still undecided

Maintainers still decide whether RFC-00E9 is accepted, rejected, or superseded
by the tags that already shipped; whether RFC-00E9 remains the right identifier
next to the unindexed RFC-00E1 draft file; the tested toolchain matrix; whether
SDK 1.x is Deprecated or in owned Maintenance Mode; whether `v2.0.1` needs an
erratum; and whether the freeze procedure still requires a normative migration
guide, an implementation-matrix update, and a changelog `BREAKING CHANGES`
section. None of those decisions is made by this file.
