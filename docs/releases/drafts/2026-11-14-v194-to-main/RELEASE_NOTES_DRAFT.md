# T81 Foundation 2.0.0 — draft release notes

**UNRELEASED / PROPOSED VERSION / HOLD.** Scope: v1.9.4
(295fc67245179f1b26e5284c70520019e72bb6c3) through main
9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2. These notes are not automatically
consumed by the current release workflow. Publication requires a separate decision.

T81 remains an experimental computing research platform. This version proposal
does not establish production readiness, native ternary hardware advantage or
broader verified determinism than the specifically recorded surfaces.

## Compatibility and upgrade notes

- **CanonFS persistent stores:** portable storage uses h1- plus lowercase hex
  filename components and .canonfs-format = t81-canonfs-hex-v1. Canonical content
  identifiers are unchanged by this filename codec, but old nonempty unmarked
  stores are rejected with an explicit migration error. Back up stores and
  capability metadata. A migration/export/import path must be validated before
  use; do not blindly rename files or add a marker to an old store. The existing
  canonfs repair command cleans legacy snapshot-local copies; it is not a general
  storage-format migrator. Persisted parity shards are not supported by this format.
- **Affected bytecode:** released v1.9.4 header values differ from current values
  for several extension/vector/FFI operations. For example, old byte 194 denotes
  VLoad while current 194 denotes TWMATMUL. Recompile affected artifacts from
  known source with a pinned toolchain and revalidate identity, signatures and
  policy expectations. Do not silently reinterpret old raw instruction records.
  No universal translator or old-producer detector is supplied by these notes.
  Existing golden fixtures do not cover every affected mapping.
- **C++ consumers:** rebuild against matched headers, libraries and compiler
  configuration. T81Fraction constructors/arithmetic/comparison operations can
  now propagate exceptions; code requiring their former noexcept function type
  must adapt. Installed class layouts/private containers also changed; no binary
  drop-in ABI guarantee is made. C API declaration and Python binding-source files
  are unchanged over this interval; that observation is not a complete ABI audit.
- **Policy hooks:** return an explicit allow verdict after checks. A default
  AxionVerdict now denies. Policy lexer/parser validation rejects trailing invalid
  bytes, invalid tiers and negative resource limits. Missing/invalid configured
  policy must not be treated as an empty bootstrap callback.

## Included changes

- CanonFS interchange gains bounded request/preflight helpers, a reusable engine
  and JSON renderer, together with strengthened error and policy-before-write paths.
  Existing v1 schema compatibility requires final contract tests; no schema
  rename or version expansion is approved by this packet. RFC-00D1 remains draft.
- Numeric correctness repairs cover T81Int multiplication, T81BigInt signed
  arithmetic, limb multiplication normalization and T81Fraction exception safety
  and wider conversion. These are correctness changes, not evidence of speedups
  or universally correct arithmetic across every public type/domain.
- Windows repairs cover FFI exports/calling conventions, Unicode CLI arguments,
  subprocess quoting/capture, private exclusive capture objects, and compiler
  compile-and-link capability selection for native versus portable 128-bit
  intermediates. Release-active tests retain signed boundary and negative cases.
- Frontend scope storage and symbol-map canonical ordering are corrected. Canonical
  symbol-key serialization order can differ from old runtime-local ID order;
  review persisted bytes/hashes rather than assuming old outputs remain identical.
- Research Commons, continuity records and bounded experiment corrections preserve
  source pins, negative findings, historical data and scope/provenance limitations.
  The #779/#785 contract/rejection-test work and #786's evidence are integrated via
  #787; #786's source-pinned workflow/runner remain inactive audit snapshots.
- Experimental controlled-AI/direct-API/optimization surfaces are present with
  scoped authorization evidence. Installed header existence is not a promise that
  every declaration is exported, implemented, stable, or DCP-certified.

## Pending and excluded

**#780 remains open/draft at c08c211bd3475d8b09e35648f84c6b5c0a7e31a2.** Its
BASE-PERF-00 benchmark changes and host-local measurements are not part of this
main snapshot or these release results. No benchmark figures or comparative
performance verdicts are included here. If #780 lands before release, repin the
candidate and assess its full delta; do not retroactively relabel old measurements.

## Evidence and research boundaries

Standard required CI and artifact qualification must be recorded at the final
release SHA. The retained five-test Linux/Windows experiment results apply to
source 300cdea8896d56c025f7099c0cdf0c41d7bd6237, not automatically to this release.
The later author-run local five-test result at d71b526c is tree-equal to assessed
main; it is not a fresh multi-platform packaging test or independent acceptance.

Cell full-domain multiplication remains limited; 50 * 2 can throw while the
product fits. No benchmark-domain result establishes full-domain correctness.
RFC-00E1 remains draft, formal SCG-08 Gate 1 acceptance WITHHELD, and Gate 2 STRICT
NO-GO. Canonical H01–H08 identities and Hanoi/CWEB lineage are preserved.
