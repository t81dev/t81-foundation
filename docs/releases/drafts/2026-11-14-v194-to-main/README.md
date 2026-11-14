# Release scope assessment — draft, 2026-11-14

**Recommendation: runtime v2.0.0, subject to qualification and maintainer decision. Release disposition: HOLD.**
No version metadata, workflow, runtime source, tag, GitHub release, PyPI package or
container publication is changed by this packet. It is local draft material.

## Pins and scope

| Item | Exact revision |
| --- | --- |
| Published baseline v1.9.4 | 295fc67245179f1b26e5284c70520019e72bb6c3 |
| Assessed current main | 9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2 |
| Pending #780, excluded | c08c211bd3475d8b09e35648f84c6b5c0a7e31a2 |

The baseline release was published April 2, 2026. Main ends with merged #787;
#782 and #784 are also included. The interval contains 448 reachable commits,
341 first-parent entries and 1,643 changed paths, including 39 installed-header
paths. Counts are inventory, not evidence of compatibility or feature quality.
See scope-manifest.json and evidence/changed-files.tsv for exact source identities.
Any later main or #780 change requires updating this assessment before release.

## Version decision from actual contracts

The repository's normative [version policy](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/reference/VERSIONING.md)
requires MAJOR for incompatible API/behavior changes, MINOR for compatible
additions, PATCH for compatible fixes. This interval contains real incompatible
changes, beyond its numerous additive APIs and correctness repairs:

| Compatibility surface | Evidence relative to v1.9.4 | Consequence |
| --- | --- | --- |
| Persistent CanonFS | New portable h1-hex filenames and .canonfs-format marker; current driver rejects nonempty unmarked legacy stores | Existing stores require explicit, validated migration; no transparent on-disk compatibility |
| Instruction bytes | Compiled header probes: VLoad 194→240, TWMATMUL 201→194, FFICall 207→200, TOR_SWAR 215→220 | Some old instruction streams can be interpreted differently; no blanket old-bytecode compatibility claim |
| Public C++ contract | Valid canonical v1::T81Fraction<13> noexcept member-function consumer compiles at v1.9.4 and fails on main | Removal of noexcept is a source-contract change even though it repairs exception safety |
| Policy defaults | AxionVerdict default allow changes true→false; malformed/trailing policy data now rejects | Custom hooks and previously permissive failure paths need explicit review; do not weaken security to preserve old accidental behavior |

**2.0.0 is the defensible next whole-main runtime version.** Neither 1.9.5 nor
1.10.0 should advertise this complete tree as backward-compatible with v1.9.4.
A maintainer wishing to retain a 1.x line needs a separately bounded, compatible
backport or an explicit compatibility design and governance decision. Do not infer
a public API break merely from internal detail names or the number of commits.

This recommendation does not authorize opcode renumbering, a new ISA profile,
frozen-contract changes or scientific promotion. The opcode differences are
observed header and encoded-record facts; normative ISA conformance was not certified here. Runtime
SemVer, ISA/profile identity, JSON schema IDs and storage-format identity remain
separate contracts. RFC-00D6 remains a draft requiring explicit profile decisions.

## Blocking preparation before any release decision

1. Align the proposed version across root CMake, generated config/version banner,
   installed CMake package metadata and Python metadata. Current values are
   1.9.1 / 1.9.1 / 1.9.2, all inconsistent with even the last v1.9.4 tag.
2. Provide and validate the storage/bytecode/C++ upgrade instructions in the draft
   notes. No general legacy-store migrator or bytecode translator was validated.
3. Repair the inspected Dockerfile paths: root docker/ does not exist; scripts
   actually live in tools/docker/. COPY and subsequent runtime paths disagree.
4. Qualify staged release archives, SDKs and wheels, not just ordinary CI builds.
   No new platform release artifact was built or installed in this assessment.
5. Obtain exact-candidate required checks, DCP/registry/freeze review, disposition
   non-required failures, and record the human release GO/HOLD decision. See the
   repository Release Discipline and .github/RELEASE_TEMPLATE.md. DEC-015's
   historical single-use #775 exception is not a release waiver.
6. Separate nonpublishing qualification from existing publication triggers before
   testing any proposed rc tag or manual publishing workflow. Current tag/manual
   workflows can publish GitHub assets, PyPI distributions and GHCR latest.

## Packet and verification boundaries

- RELEASE_NOTES_DRAFT.md: user-facing notes and upgrade scope.
- KNOWN_LIMITATIONS.md: correctness, provenance, compatibility and research limits.
- PACKAGING_AUDIT.md: inspected workflow behavior and concrete gaps.
- PLATFORM_ARTIFACT_CHECKLIST.md: candidate qualification and platform matrix.
- evidence/: exact inventory, compatibility diff, probe source/commands/results,
  baseline release metadata, workflow input source and CI snapshot.

Executed locally on macOS ARM64 / Apple clang 21: baseline/main header opcode
comparison and actual 13-byte VLoad encoding (first byte 194→240); corrected
noexcept consumer compilation; current Cell counterexample;
current persistent-driver legacy-store rejection. The driver probe links an
existing archive built at d71b526c35592a7e5f96ed32a2bbf5807613f962; its entire
tracked source tree equals assessed main, and the archive hash is retained.
This is explicitly archive reuse, not a freshly rebuilt main library.

The first noexcept probe used the wrong namespace and failed at both revisions;
those inconclusive logs remain. The corrected probe uses t81::v1 and establishes
the reported differential. Expected rejection is not reported as arithmetic success.
No performance measurements, release workflows or publishing operations ran.

Codex authored this assessment and contributed to included Windows/capture and
#787 work. This is shared-effort engineering evidence, not independent release
certification or authentication of historical runs. Hashes establish identity.

## Latest metadata snapshot

Main and pending #780 remained at the pinned SHAs at the final refresh. Both
Windows builds and macOS ARM64 build completed successfully. Analyze (cpp) and
several builds were still running; quality gate / required had not yet appeared.
The raw API snapshot is retained in evidence/main-and-pr780-check-snapshot.json.
These are GitHub-reported checks, not locally executed platform qualification.
Existing CI benchmark jobs were observed only; this task started no campaign.
