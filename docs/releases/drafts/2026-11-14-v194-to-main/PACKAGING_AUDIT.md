# Packaging workflow inspection — pinned main 9256b6ec

**Source inspection, not an executed release qualification.** Source hashes and
immutable URLs are in scope-manifest.json. No release/wheel/container workflow was
invoked. All findings below require bounded preparation, not an unapproved tag.

## Publication triggers and approval boundary

| Workflow | Trigger | Configured output | What was observed |
| --- | --- | --- | --- |
| .github/workflows/release.yml | push tag v*.*.* | Five CLI/SDK archives, installers, SBOM, GitHub release | Tag drives creation directly; no explicit required-check query, draft flag, prerelease flag or release-approval environment |
| .github/workflows/python-wheels.yml | same tag or workflow_dispatch | Wheels + sdist, then PyPI | Publish job has no tag-only condition; manual dispatch can reach publish. pypi environment/OIDC is declared; configured approval protection was not verified |
| .github/workflows/docker.yml | same tag or workflow_dispatch | GHCR version / major.minor / latest | push:true; latest is unconditional. No platforms matrix; the configured runner supplies the build architecture |

A proposed v2.0.0-rc.1 also matches the broad tag glob. The release action omits a
prerelease setting and Docker requests latest. **Do not create a trial rc tag or
use wheel/Docker dispatch to test packaging.** A separate nonpublishing qualification
path should build/upload artifacts without GitHub/PyPI/GHCR publication. Existing
runtime CI success does not substitute for that path.

The release workflow uses generate_release_notes:true; it does not read this
packet or .github/RELEASE_TEMPLATE.md. Maintainers must connect approved notes and
the release decision record to any eventual publication. This packet grants no
release authority and does not change the workflows.

## Concrete source findings

| Finding | Evidence | Required treatment before an affected artifact is offered |
| --- | --- | --- |
| Version mismatch | CMakeLists.txt and include/t81/config.hpp = 1.9.1; pyproject.toml = 1.9.2; previous published tag = v1.9.4 | Select version, align metadata and verify archive banner / CMake package / Python distribution agree |
| Broken Docker source path | Dockerfile COPY docker/ /t81/docker/; tracked scripts exist only under tools/docker/; chmod/entrypoint expect /t81/tools/docker | Repair in a separate reviewed change; run a nonpublishing build and smoke the actual image |
| Release layout differs from standard CI | Release config sets TESTS/EXAMPLES/BENCHMARKS OFF, leaves TUI/fuzz/TernaryOS defaults, builds all default targets | Test exactly those flags or intentionally define a narrower shipped profile; ordinary CI cannot establish this package build |
| Windows compiler environment unspecified | Release uses Ninja without an MSVC environment action or explicit compiler, unlike ordinary CI | Select/record compiler, SDK and CRT; obtain actual Windows packaging result. Failure was not locally reproduced on Windows |
| SDK support boundaries unclear | include/t81 is installed wholesale; t81_direct_api is built but not in T81_INSTALL_TARGETS; C API declaration is ffi/c_api/t81_c_api.h outside installed include/t81 | Validate clean consumers; export/install intended surfaces or explicitly exclude them from supported SDK claims |
| Mixed license display | Root LICENSE / Python metadata / Docker label say Apache 2.0; CLI banner says MIT and GPL-3.0 | Maintainer reconciles notices and display with actual licensing; this is not an automated license change |
| No archive checksum verification in installers | install.sh/install.ps1 download archives over HTTPS and copy only bin/t81[.exe] | Publish exact asset hashes and decide verification policy; test whether copying one binary is sufficient for actual dynamic dependencies |
| No candidate artifact test after staging | release.yml builds/installs/archives and then publishes | Extract to a clean location and run installed CLI/SDK checks; do not assume build-tree tests prove relocated use |

The Windows lane's current state is a source-level qualification gap, not an
assertion of an observed Windows release failure. Source inspections also show
unconditional development/test targets under subdirectories despite TESTS=OFF;
confirm the real install/build manifest instead of assuming those flags remove
all development dependencies.

## Archives and install layout

Expected names are t81-<tag>-<platform>.tar.gz (four Unix platforms) and
.zip (Windows). dist/ mirrors CMake install with bin, lib and include, plus CMake
exports under lib/cmake/T81 and lib/cmake/T81Foundation. The TUI target is not
listed in the explicit install target set; verify what is actually shipped.

Both CMake package version files use SameMajorVersion. That is metadata matching,
not proof of ABI compatibility. Installed consumer tests must cover compiler
backend selection, transitive dependencies and package relocation. The CMake
native-int128 probe's result must not be assumed to propagate correctly to every
standalone/installed consumer without testing.

Install scripts extract/copy only t81, not SDK libraries, data, helper executables
or runtime DLLs. Inspect linked dependencies before claiming a one-file install.
Linux artifacts are built on Ubuntu 24.04, not a promised manylinux baseline.
macOS deployment target is not explicitly set in release.yml. Windows ARM64 is
not a configured artifact. These are qualification targets for the checklist.

## Wheels and sdist

pyproject.toml requests CPython 3.9–3.13, five OS/architecture lanes, no 32-bit/musl.
Linux aarch64 runs via QEMU on x86_64; distinguish that from native ARM64 execution.
The manylinux_2_28 images and GCC toolset 13 differ from ordinary Ubuntu GCC/Clang
CI. The build backend requirements scikit-build-core>=0.10 and pybind11>=2.13 are
not locked; retain resolved versions and build provenance.

The wheel's pure package source is ffi/python while its compiled module installs
to t81/. wheel.packages mapping and Windows extension placement require actual
wheel-content/import checks; no wheel failure or success is asserted from this
source inspection alone. The configured smoke test checks compile_and_run; add
coverage for documented APIs from an installed wheel outside the repository.
No CPython 3.14 wheel is configured. The sdist file set/size and offline dependency
requirements need inspection; source availability is not an installability proof.

## SBOM and artifact provenance

The pinned Anchore action declares SPDX JSON and upload-release-assets:true by
default. The release workflow supplies artifact-name:sbom.spdx.json and scans the
artifacts directory, but does not set output-file. Verify the actual attached
asset and its coverage of unpacked executables/libraries; the final release files
list alone does not establish attachment or content completeness. The pinned
upstream action specification is retained in evidence/anchore-pinned-action.yml.

The inspected v1.9.4 release has five platform archives, installers, dummy model
fixtures, a generator and sanitizer log; its asset list does not contain an SBOM.
That historical observation is not a current-pipeline result. Old assets must not
be relabeled as builds of this candidate.

Current workflows produce no explicit SHA256SUMS/attestation stage, and the binary
CI upload retention is one day (wheels/sdist seven). Archive qualification records
and hashes before expiry. Hashes prove content identity, not source-to-binary
truth or author authenticity. No release-provenance assertion is authenticated by
this assessment alone.
