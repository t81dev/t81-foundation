# F01 Linux dependency/interpreter re-lock proposal

**PROPOSED / INPUT ARTIFACTS VERIFIED / TARGET RUNTIME MISSING / ACTIVATION BLOCKED.**
Accepted harness: `0afbc339288b44cc7536af6c227f957257bb8ff6` (PR #809).
Protocol ancestor: `c5e0ca79c073a72bb41f03b76ddd68acbd7eab73`.
Protocol SHA-256: `d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3`.
RFC candidate: `ec22a7588282d37ad027cff61c807bcf159ac2e0`.
Canonical context H07; no replacement of H01–H08 or historical Hanoi/CWEB lineage.

This packet changes only preparation documents/data inspection scripts. The
accepted harness, active macOS dependency-lock.json, requirements.lock and
source identities are untouched. linux_relock_required remains true. No package
was installed, interpreter built/run, source or wheel code imported, ELF loaded,
qualification probe or A–F/proof/fixture executed. No merge or qualification.
Codex authored the prior harness and proposals too: these fresh static checks
are not independent human/scientific validation.

## Exact dependency choices

| Distribution | Version | Verified archive SHA-256 |
|---|---|---|
| attrs, both | 26.1.0 | `c647aa4a12dfbad9333ca4e71fe62ddc36f4e63b2d260a37a8b83d2f043ac309` |
| jsonschema, both | 4.26.0 | `d489f15263b8d200f8387e64b4c3a75f06629559fb73deb8fdfb525f2dab50ce` |
| jsonschema-specifications, both | 2025.9.1 | `98802fee3a11ee76ecaca44429fda8a41bff98b00a0f2838151b113f210cc6fe` |
| referencing, both | 0.37.0 | `381329a9f99628c9069361716891d34ad94af76e461dcb0335825aecc7692231` |
| rpds-py, x86_64 | 2026.9.1 | `eac2f5dbafd585dfe31f86a23ebf0d3ba480a9d49ebc87947267b5608d4ea0cd` |
| rpds-py, ARM64/aarch64 | 2026.9.1 | `07deecbfce94c78473018bc7d10b337cc651d12df87a1eb2cb3e4024bc9c33d0` |

Separate profile-*.proposed.json and requirements-*.proposed.lock files preserve
architecture identities; the latter are future inputs, not installation commands
or active runtime locks. Four wheels are py3-none-any. Native filenames are:

- rpds_py-2026.9.1-cp314-cp314-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
- rpds_py-2026.9.1-cp314-cp314-manylinux_2_17_aarch64.manylinux2014_aarch64.whl

Fresh official release JSON and actual wheel bytes match both prior selections
and each other. All ZIP member hashes/sizes and all RECORD declarations were
checked, including the expected unhashed RECORD self-entry. No extracted code
was installed or executed. WHEEL/METADATA identities, dependency markers, tags,
member inventory and native details are in wheel-inspection.json. Hashes identify
bytes, not authenticity or provenance truth; signatures were not verified.

Metadata-only closure: jsonschema requires attrs/specifications/referencing/rpds;
specifications and referencing add no unpinned unconditional requirement for
Python3.14.3; typing-extensions applies below3.13. No extras are selected. Format
extras, dependency resolution, source builds and architecture substitutions are
not authorized. No resolver/import execution confirms this proposed closure.

## Native ABI evidence and limits

| Static field | x86_64 | ARM64/aarch64 |
|---|---|---|
| ELF | ELF64 little-endian, machine62 | ELF64 little-endian, machine183 |
| Extension | rpds.cpython-314-x86_64-linux-gnu.so | rpds.cpython-314-aarch64-linux-gnu.so |
| Native member SHA-256 | `6ccfc934fe55ba4e5fe23334a0ee537d2197b60931198746ba83ae07b94d735e` | `b6dd055a20ce1bf818e576505cbb310e8b9db5e40ff7464ac34750f74a256c38` |
| DT_NEEDED | libgcc_s.so.1, libpthread.so.0, libc.so.6, ld-linux-x86-64.so.2 | libgcc_s.so.1, libpthread.so.0, libdl.so.2, libc.so.6 |
| Observed GLIBC symbol-version range | 2.2.5–2.14 | 2.17 |
| Observed GCC symbol versions | 3.0,3.3,4.2.0 | 3.0,3.3,4.2.0 |
| RPATH/RUNPATH | none observed | none observed |

Both export PyInit_rpds and have cp314-cp314 wheel tags. Static version requirements
do not exceed the declared manylinux2.17 policy, but that is not certification
of policy compliance or loadability. The ELF record preserves undefined symbols;
weak-symbol/fallback resolution, CPython symbol exports and libc/libgcc/loader
resolution still need independent review and later separately authorized host
qualification. No ldd or native load was used. No CPU instruction disassembly,
GNU ISA property audit, glibc runtime test or ABI compatibility claim is supplied.
ARM64 means AArch64 here; no ARMv9 qualification. musl, cp314t/free-threaded, debug
ABI, PyPy, Windows and macOS are outside these profiles.

## Interpreter and standard-library identity

Official Python-3.14.3.tar.xz actual bytes:23,778,568 bytes, SHA-256
`a97d5549e9ad81fe17159ed02c68774ad5d266c72f8d9a0b5a9c371fe85d902b`.
This advances the old publisher-declared pin to a computed archive-byte check.
2,405 regular Lib/ source members are hashed in stdlib-source-inventory.json;
configure/getpath/init/pathconfig/patchlevel/build-file identities are retained.
That source inventory is **not** an installed Linux stdlib identity: it includes
source/test/bundled files and excludes generated build configuration, compiled
extensions, frozen binary content and installation transformations.

interpreter-profile.proposed.json pins CPython3.14.3, GIL-enabled cp314, non-debug,
JIT/tail-call disabled policy, exact source and proposed paths/config bytes.
No Linux binary/provider/build/toolchain, installed stdlib or native-library
manifest exists in this task. Their fields are null, deliberately blocking
activation. There is no complete reproducible binary/runtime lock yet. Version
strings, this source tarball, or the macOS executable cannot fill those fields.

The publisher marks3.14.3 superseded. It is retained because the accepted source
pins it; historical-patch suitability needs explicit human review. No implicit
upgrade, distro patch set, ABI switch or new interpreter provider is approved.
[Release](https://www.python.org/downloads/release/python-3143/).

## Reconciliation with interpreter_boundary

Proposed fixed base=/opt/t81-f01/cpython-3.14.3, venv=/opt/t81-f01/venv. The venv
bin/python3.14 must resolve to base/bin/python3.14. The proposed pyvenv.cfg bytes
are separately hashed; include-system-site-packages=false is mandatory, home
must resolve to the base executable directory, and no competing bin/pyvenv.cfg
or symlinked config is allowed. These are proposed bytes, not observed host bytes.

The allowed initial sys.path is the nonexistent base/lib/python314.zip placeholder,
base/lib/python3.14, and its lib-dynload directory. purelib and platlib must both
resolve to venv/lib/python3.14/site-packages. Actual prefix/exec-prefix and base
pairs, sysconfig paths, flags and environment still require host evidence. The
source gate rechecks them; this packet does not execute that check.

The supervisor/bootstrap and future static helper use -I -S -B; the final runner
uses -P -S -B with its exact minimal environment to preserve hash seed0. Site,
.pth/customization and ._pth overrides are prohibited, including installer-added
hooks. No pip/setuptools/extra package is admitted into the runtime profile.
Strict profile rejects system-site venvs, missing/duplicate/ambiguous config,
split lib/lib64 paths, framework/multi-prefix layouts, existing stdlib ZIPs,
embedded/custom sys.path and additional user/system package paths. These are
unsupported source-contract layouts, not reasons to weaken the accepted gate.
A path/layout change needs an explicitly reviewed profile revision.

## Missing evidence and future activation boundary

Before any active lock replacement, a separately approved preparation must:

1. Select one architecture and immutable provider/build artifact. Record exact
   binary and symlink-chain hashes, build ID, compiler/linker/configure/build
   recipe, configuration headers/sysconfig data, patches, ABI/build flags and
   provider image/package digest. Pin reproducibility inputs; distinguish a
   repeatable retrieval from an independently reproduced bit-identical build.
2. Hash every installed stdlib/lib-dynload file and frozen-runtime-containing
   binary, actual pyvenv.cfg and path relationships. Record libc, dynamic loader,
   libgcc, libpython if applicable and all resolved extension/interpreter library
   dependencies by path/hash, including OpenSSL/zlib/libffi/etc when linked.
   Wheel DT_NEEDED lists alone do not describe the interpreter's closure.
3. Independently review artifact authenticity and historical patch suitability.
   An approved offline provisioning step must use exact wheel artifacts, a
   separately pinned installer, no resolver/source fallback/extras, no bytecode
   compilation, no user/system site or startup hooks. No such installer is pinned
   or run here. Provisioning needs separate bounded authorization; it is not free
   qualification setup and must not be hidden inside the harness campaign.
4. Capture exhaustive installed-package file hashes, METADATA/WHEEL/RECORD and
   post-install transformations. Wheel RECORD is not the installed-file lock:
   rewritten RECORD/INSTALLER/direct_url or other outputs need explicit treatment.
   No empty file inventory, version-only proof or hash substitution is allowed.
5. Independently reconstruct the closure and compare identities. Publish a new
   preparation SHA with the architecture-specific active installed-file lock,
   updated source identities/static evidence and all still-required host receipts.
   Clear linux_relock_required only after the necessary evidence exists and a
   separate explicit approval grants that exact change. This packet cannot do it.
6. Obtain separately authorized qualification and then, if approved, a distinct
   one-run A–F authorization. Source acceptance and human RFC §4.1 review remain
   separate; no JSON flag, archive hash or proposal validation supplies authority.

The protocol, both evidence schemas, all A–F algorithms/domains, F254/256 and
F288-v1 deferred status, D256/S64/R128MiB/zero swap,49,408 states,60s wall,50s CPU,
256MiB aggregate and25MiB experiment artifacts and all holds remain unchanged.

Evidence sources: retained official PyPI release JSON and artifact URLs,
[CPython path initialization](https://docs.python.org/3.14/library/sys_path_init.html),
and the accepted source pins. Newer online documentation is explanatory, not a
substitute for the pinned3.14.3 source. Downloaded archives were read as inert
data only; no host or scientific qualification follows from their existence.
