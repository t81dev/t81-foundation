# Platform artifact checklist — proposal, all publication boxes unchecked

Candidate source: 9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2.
Proposed version 2.0.0, pending maintainer approval and metadata alignment.
No artifact below was built/published as a release by this assessment.

## Expected archive matrix

| Platform | Planned asset | Runner / required qualification | Status |
| --- | --- | --- | --- |
| Linux x86_64 | t81-v2.0.0-linux-x86_64.tar.gz | Ubuntu 24.04; record compiler, glibc/libstdc++ requirements and linked dependencies | NOT QUALIFIED |
| Linux ARM64 | t81-v2.0.0-linux-arm64.tar.gz | Native ubuntu-24.04-arm; verify ELF architecture and installed execution | NOT QUALIFIED |
| macOS ARM64 | t81-v2.0.0-macos-arm64.tar.gz | macos-15; verify Mach-O architecture, deployment target and relocation | NOT QUALIFIED |
| macOS x86_64 | t81-v2.0.0-macos-x86_64.tar.gz | macos-15-intel; actual native build/test, not assumed cross-build | NOT QUALIFIED |
| Windows x86_64 | t81-v2.0.0-windows-x86_64.zip | Explicit MSVC x64 environment; SDK/CRT/link dependencies; separate clang-cl validation evidence | NOT QUALIFIED |

Runtime matrix success is supporting evidence only. The exact tests-OFF release
configuration, staged installation and archived payload still require execution.
An informational ARMv9 failure is not evidence that native Linux ARM64 is broken,
but must be classified at the release SHA under the Release Discipline.

## Per-archive checks

- [ ] Resolve final source SHA and version; record tag-to-commit binding only after approval.
- [ ] Build with exact recorded CMake flags, compiler/version, architecture, SDK,
      dependency revisions and selected int128 backend. No fallback to 64-bit intermediates.
- [ ] Run required runtime tests on a matched configuration and record failures/skips;
      keep standard CI separate from opt-in experiment tests.
- [ ] Stage with cmake --install, retain install manifest and archive file list.
- [ ] Extract into a fresh directory outside the checkout, including a path with
      spaces/Unicode on Windows. Run CLI --version/help and representative compile,
      execute, policy-deny and CanonFS import/export/portable-store smoke checks.
- [ ] Verify release banner, config.hpp, both installed CMake package versions,
      package filename and manifest agree on runtime version; retain independent ISA/schema IDs.
- [ ] Verify golden bytecode/AST-IR hashes and canonical artifacts under their exact
      stated fixture/profile scope; inspect affected old opcode migration separately.
- [ ] Check dynamic dependencies and minimum OS/ABI requirements with platform
      tools; establish whether the installer copying only t81[.exe] is sufficient.
- [ ] Clean consumers find_package(T81 CONFIG) and find_package(T81Foundation CONFIG),
      compile/link/use documented core APIs with installed headers/libraries only.
- [ ] If C API or Direct API is advertised, prove installed declarations, exports,
      transitive targets and actual calls work; otherwise list them as unsupported/experimental.
- [ ] Validate archive paths, permissions, symlinks and relocation; include approved
      license/notices and ensure CLI/Python/container license metadata is consistent.
- [ ] Record size, SHA-256 and content manifest for each archive; publish/verify
      SHA256SUMS and any approved signatures/attestations with clear provenance scope.
- [ ] Retain raw logs, command argv, source/dependency SHAs and all negative outcomes.

## Python distributions

- [ ] Decide supported CPython/OS matrix. Current configuration targets 3.9, 3.10,
      3.11, 3.12, 3.13 across Linux x86_64/aarch64, macOS arm64/x86_64 and Windows AMD64.
      Nominally five CPython versions × five architecture lanes, subject to actual
      build tags and explicitly documented omissions; do not infer 3.14 support.
- [ ] Record Linux aarch64 QEMU execution as emulated; obtain native confirmation
      if native wheel claims are made. Inspect manylinux tags/repair dependencies.
- [ ] Align project version with selected runtime version; record resolved build
      dependencies. Do not attempt to republish stale 1.9.2 metadata under a new tag.
- [ ] Inspect wheel contents for t81/__init__.py and _t81 module placement. Install
      each wheel into a clean interpreter outside the repository; run the configured
      compile_and_run smoke and representative documented API checks.
- [ ] Build/inspect sdist; install from it in a clean environment. Verify included
      source/data and dependency/network requirements; exclude accidental worktrees/caches.
- [ ] Record wheel/sdist names, hashes, sizes, platform/Python ABI tags and test logs.
- [ ] Verify actual pypi environment protection and intended OIDC publisher; do not
      invoke the existing publish-capable manual workflow during qualification.

## Container, installers and SBOM

- [ ] Repair Docker COPY/chmod/entrypoint paths in a separate reviewed change.
- [ ] Perform a nonpublishing container build and run --version/help/demo from
      the actual final image; inspect dynamic dependencies and image architecture.
- [ ] List only architectures actually built; no multi-arch image is configured now.
- [ ] Verify prerelease/stable/latest tagging policy before any tag-driven or manual
      workflow run; current Docker workflow pushes latest unconditionally.
- [ ] Test install.sh and install.ps1 against approved staged assets without writing
      a public release or changing user-global installation unexpectedly.
- [ ] Verify actual SPDX SBOM attachment and unpacked binary/dependency coverage;
      tie it to final artifact hashes. Do not treat a workflow artifact name as proof.

## Release decision, separate from engineering preparation

- [ ] Exact release-SHA required checks: quality gate / required and Analyze (cpp),
      or the freshly verified protection set if maintainers change it.
- [ ] Classify all non-required failures as release-impacting or document an explicit
      authorized waiver with owner/date/rationale; no waiver is issued by this packet.
- [ ] Review current determinism registry, DCP scope, structural/verified-surface
      records, open freeze exceptions and incidents. Resolve moved/stale report paths
      in the release template; do not reuse historical reports as current validation.
- [ ] Confirm storage/bytecode/noexcept migration instructions and test evidence.
- [ ] Confirm #780 remains excluded, or repin and reassess it if merged before release.
- [ ] Approve final notes, metadata and artifact manifest with a named human decision.
- [ ] Record GO/HOLD using .github/RELEASE_TEMPLATE.md before any publication.
- [ ] Tag/publication only with separate explicit authorization. These boxes are
      planning requirements, not execution instructions for this task.

Scientific acceptance is separate: RFC-00E1 draft; SCG-08 Gate 1 WITHHELD;
Gate 2 STRICT NO-GO. No release number or passing package test advances those gates.
