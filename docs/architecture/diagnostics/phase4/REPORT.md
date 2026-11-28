# T81-ARCH-ENG-002 Phase 4: native firmware compatibility

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81-ARCH-ENG-002 Phase 4: native firmware compatibility](#t81-arch-eng-002-phase-4-native-firmware-compatibility)
  - [Result and scope](#result-and-scope)
  - [Reconciliation and input identities](#reconciliation-and-input-identities)
  - [Diagnostic firmware provenance and method](#diagnostic-firmware-provenance-and-method)
  - [Responsible returns in the instrumented counterpart](#responsible-returns-in-the-instrumented-counterpart)
  - [Fixed compatibility matrix and native outcome](#fixed-compatibility-matrix-and-native-outcome)
  - [Blockers and why no correction is promoted](#blockers-and-why-no-correction-is-promoted)
  - [Regression, artifact and platform limits](#regression-artifact-and-platform-limits)
  - [Reproduction and retention](#reproduction-and-retention)
  - [Exact changed-file inventory](#exact-changed-file-inventory)

<!-- T81-TOC:END -->


Date: 2026-11-10. **Disposition: BLOCKED.** No configuration in the bounded matrix
passes all five checks. No observational exit or shell marker is presented as
native acceptance, and no execution correction is proposed as established.

Base: **6c9eaf1a2fc8368cd06f6dddbbdcd78e6606c0ce**, freshly fetched origin/main;
PR #757 was confirmed merged at that commit. Evidence/fixture head:
**984726b9d795014990229a7565084b4427c90327**. The final report-only packaging
head is pinned in the draft PR description.

Author/executor: the same Codex assistant that implemented Phase 2 and performed
Phase 3. These are new local executions with disclosed prior context, not
independent-person validation. The maintainer's dirty checkout was preserved;
all source builds ran in isolated checkouts under `/private/tmp/t81-phase4`.
This remains bounded runtime-verification maintenance, not a research campaign.

## Result and scope

An instrumented, source-pinned EDK2 build identifies the previously unnamed
firmware returns using **module debug paths and full status values**, not image
addresses. A matched launch supplies additional real virtual devices and clears
three returns. Two remain: no TPM2 instance, and no fw_cfg direct-kernel payload.
The unchanged verifier rejects the resulting diagnostics. Its acceptance rules,
markers, failure matching, image handling and child-exit checks remain byte-for-byte
unchanged. The historical tracked debug-probe image is also unchanged.

The source-built diagnostic counterpart is not bit-identical to the installed
Homebrew firmware. Direct module/status attribution below is established for
that counterpart. Corresponding changes in the installed binary's diagnostics
corroborate the configuration dependencies, but do not establish a complete
build/symbol attestation for that binary. No identity is inferred from an
address alone.

## Reconciliation and input identities

Phase 3 demonstrated that the tracked 5,120-byte EFI is a historical debug probe,
whereas current sources can reach the shell. That distinction is preserved.
For this phase, the standard CMake EFI target and existing macOS image builder
were executed again from the Phase 4 base, both with exit 0. The same fresh image
was used for every matrix row. A compressed copy is retained as evidence only;
it does not replace or qualify the production image.

| Input | SHA-256 |
|---|---|
| Fresh current-shell image | `4eb7b30cfd60b5c8d78a92342c9645662ab8a5ded6d69d2e23e768a474c48d6c` |
| Fresh EFI | `dfdbdf134f407173476b9c6e6e1fb27f36b5a86919822d88815dfb0c73a336a0` |
| Supplied EDK2 code | `47765fe344818cbc464b1c14ae658fb4b854f5c2ceffa982411731eb4865594d` |
| Instrumented code, padded for pflash | `af8cab573d31142187d9a70a895ab31409903018bfc73e9dc28ade03ec4661cd` |
| Matching source-build vars template, padded | `b3b855c5a80310168051164986855692d1bdb06e67619856177965cd87c6774f` |

Host: macOS ARM64 / Darwin 25.5.0, QEMU 10.2.2, Python 3.14.3 for the verifier,
AppleClang 21.0.0 for hosted guest-build tools, Clang/LLD 22.1.1 for the EFI and
firmware cross-build, CMake 4.3.0. The EDK2 Python tools use system Python 3.9.6.
Exact tool output, binary hashes, submodule revisions and firmware descriptor
are retained in [provenance.json](evidence/provenance.json). The installed firmware
banner advertises edk2-stable202408; its exact binary build is not attested by
that banner or by the descriptor.

## Diagnostic firmware provenance and method

EDK2 source: `b158dad150bf02879668f72ce306445250838201` (edk2-stable202408).
Pinned gitlinks supply Brotli, OpenSSL, MbedTLS, libfdt, MipiSysT and SPDM headers.
Local IASL was built from ACPICA `7dae72155bf06b0edda9f3aea713da1d48c1c418`
(R2024_12_12); its version is 20241212. No system package was installed.

The build uses ArmVirtQemu/AARCH64, CLANGDWARF, **DEBUG**, TPM2 and TPM2 configuration
enabled, and the HTTP/IPv6/TLS/iSCSI/Cavium feature switches from the upstream
[QEMU firmware build recipe](https://github.com/qemu/qemu/blob/v10.2.2/roms/edk2-build.config).
This matches relevant recipe features, not a proof of reconstructing the supplied
binary. DEBUG error reporting is preserved and expanded with
`DEBUG_PRINT_ERROR_LEVEL=0x8000004F`; upstream DEBUG property mask is 0x2f.

The only source additions are:

- After each firmware image entrypoint returns, print its existing debug/PDB
  path and complete 64-bit status.
- At the existing ArmTrngLib error handler, print the last attempted query stage,
  SMCCC query response and mapped status.

No branch, return value, original diagnostic, driver or assertion is removed.
The exact additive patch is retained in [instrumentation.json](evidence/instrumentation.json).
The diagnostics are not applied to TernaryOS or the verifier. Complete native
continuations keep every original error and use QMP quit solely to bound capture.

The code and variable-store build outputs were copied and zero-extended to
64 MiB, as required by QEMU's pflash devices. An earlier exploratory launch with
a 2 MiB pflash backend was refused by QEMU; that startup failure remains in the
receipts and is not a pass. Correctly sized final rows use fresh private variable
copies. No existing variable store is reused to satisfy a check.

## Responsible returns in the instrumented counterpart

| Module or library | Observed raw result | Supported interpretation |
|---|---|---|
| ArmTrngLib constructor | stage 0; SMCCC query response `0xFFFFFFFFFFFFFFFF`; mapped `RETURN_UNSUPPORTED` = `0x8000000000000003` | Initial SMCCC-version query is unsupported through the selected monitor conduit. The constructor prints its existing diagnostic and then returns success. No later TRNG query was reached. |
| RngDxe | `0xA000000000000001` | **EFI_REQUEST_UNLOAD_IMAGE**, a PI-specific unload request when no RNG algorithms are available. This is not EFI_LOAD_ERROR (`0x8000000000000001`). It accounts for the numeric `00000001` diagnostic in this build. |
| QemuRamfbDxe | `0x800000000000000E` | EFI_NOT_FOUND: the `etc/ramfb` fw_cfg interface is absent in the original launch. Supplying the real ramfb device clears this return in the matched configuration. |
| Tcg2Dxe | `0x8000000000000003` | EFI_UNSUPPORTED. The full trace also says no TPM2 instance is required; this is the entrypoint branch for a none/TPM1.2 instance selection, rather than evidence of a corrupt guest EFI. No TPM backend was supplied in these runs. |
| QemuKernelLoaderFsDxe | `0x800000000000000E` | EFI_NOT_FOUND. This driver requires a fw_cfg kernel blob, whereas the tested guest is booted from its writable disk image and no direct kernel was supplied. |
| TlsDxe | `0x8000000000000015` | EFI_ABORTED. The entrypoint's TLS-context creation fails. It returns success in the compound matched configuration; the exact underlying OpenSSL/entropy cause was not independently isolated. |

The two `Not Found` returns are distinct modules; identifying either from its
load address alone would have been unjustified. Source entrypoints and the PI
status namespace resolve their meaning:
[PI status definition](https://github.com/tianocore/edk2/blob/b158dad150bf02879668f72ce306445250838201/MdePkg/Include/Pi/PiMultiPhase.h),
[RNG entrypoint](https://github.com/tianocore/edk2/blob/b158dad150bf02879668f72ce306445250838201/SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.c),
[RAM framebuffer entrypoint](https://github.com/tianocore/edk2/blob/b158dad150bf02879668f72ce306445250838201/OvmfPkg/QemuRamfbDxe/QemuRamfb.c),
[kernel filesystem entrypoint](https://github.com/tianocore/edk2/blob/b158dad150bf02879668f72ce306445250838201/OvmfPkg/QemuKernelLoaderFsDxe/QemuKernelLoaderFsDxe.c),
[TCG2 entrypoint](https://github.com/tianocore/edk2/blob/b158dad150bf02879668f72ce306445250838201/SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.c),
[TLS entrypoint](https://github.com/tianocore/edk2/blob/b158dad150bf02879668f72ce306445250838201/NetworkPkg/TlsDxe/TlsDriver.c).

These are genuine capability/initialization return statuses, including intentional
optional-driver refusal; they do not demonstrate that the TernaryOS image failed
to load or that the whole firmware stopped. The generic DXE image service prints
its error diagnostic for these nonzero error-range returns. The verifier's
existing policy still rejects that text. The ArmTrngLib sentence itself does
not match the verifier failure regex; the subsequent image-start errors do.
No exception or allowlist is introduced for optional modules.

## Fixed compatibility matrix and native outcome

All rows use the same fresh guest, one CPU, 512 MiB RAM, virt/GICv3 and the
**unchanged checked-in verifier**. Original means cortex-a57, bios code loading
and the existing device defaults. Matched means CPU max, explicit virtio RNG and
ramfb devices, nodefaults, code pflash and a fresh matching variable-store copy.
The transparent argv adapter only changes those QEMU configuration arguments;
it records and execs the real QEMU binary, without filtering logs, exits or
markers. It is an investigation fixture, not a new production launcher.

| Row | Firmware | Launch | Verifier exit | Count/disposition |
|---|---|---|---|---|
| supplied-original | Installed Homebrew code | Original | 1 | 2/5, FAILED |
| supplied-matched | Same installed code | Matched | 1 | 2/5, FAILED |
| diagnostic-original | Source-pinned instrumented code | Original | 1 | 2/5, FAILED |
| diagnostic-matched | Same instrumented code | Matched | 1 | 2/5, FAILED |

Each receipt includes the current process exit, source image/firmware/template
hashes, environment, wrapper argv, actual executed QEMU argv, QMP replies, serial,
process diagnostics and summary: [matrix.json](evidence/matrix.json). No row is
reported as successful because QMP ran, a RAM file appeared or the observer later
reached a shell. The fail-closed worker terminates/reaps QEMU after its rejection.

Complete diagnostic continuation of the final instrumented firmware establishes:

| Module | Original | Matched |
|---|---|---|
| RngDxe | PI unload request | Success |
| QemuRamfbDxe | Not Found | Success |
| TlsDxe | Aborted | Success |
| Tcg2Dxe | Unsupported | **Unsupported remains** |
| QemuKernelLoaderFsDxe | Not Found | **Not Found remains** |

The compound control establishes these observed changes; it does not isolate
every device/CPU contribution or prove an entropy-specific TLS cause. The stage-0
TRNG query remains unsupported in both launches. Both complete diagnostic runs
reach real guest-ready output and a fresh reply to a sent help command, followed
by requested QMP shutdown with exit 0. Their logs retain the firmware errors;
**these observational exits are not verification passes**.

All four exploratory/final diagnostic captures, including the failed unpadded
pflash attempt, are in [observations.json](evidence/observations.json). The final
two use the exact instrumented firmware pinned above. Earlier module-only
instrumentation is retained separately and is not substituted for the final
stage-instrumented results.

## Blockers and why no correction is promoted

No clean candidate was found within the fixed two-firmware/two-launch matrix.
The matched configuration does not supply a TPM2 instance or the direct-kernel
interface requested by two included firmware modules. QEMU on this host lists
only its TPM emulator backend; a live TPM emulator was not provisioned or tested.
A direct-kernel boot alternative was also not evaluated. This is a limitation
of the tested configuration set, not proof that no compatible configuration or
firmware exists.

Removing these modules, changing their returns, selecting a silent/RELEASE build,
lowering debug masks, providing a dummy kernel solely to satisfy a probe, or
allowlisting their diagnostics would not establish the requested pass. None was
used as a candidate. A coherent platform/firmware/boot-path combination still
needs actual native qualification through the unchanged verifier. This report
supports that narrower next step, not a speculative production patch or a
universal incompatibility claim.

The tracked historical image is not replaced. The fresh image archive is an
investigation input, not a qualified default. No execution repair is proposed
as proven, and the incomplete native acceptance status remains explicit.

## Regression, artifact and platform limits

- **STATICALLY VERIFIED:** source/status attribution scoped above; additive-only
  instrumentation; Python syntax for all four investigation fixtures; unchanged
  verifier/contract/entrypoint/production-image identities; final whitespace.
- **MOCK-EXECUTION VERIFIED:** unchanged suite passes **11/11 methods**, with
  **42 entrypoint receipts** (9 accepted, 33 rejected), plus its separate
  interruption and arithmetic probes; elapsed 34.060 seconds. These are harness
  control-flow checks, not firmware or guest execution.
- **QEMU-EXECUTION VERIFIED:** four final authoritative verifier failures and
  two complete final diagnostic continuations with raw module statuses, real
  shell/help and controlled QMP exit. The initial baseline rejection and two
  exploratory diagnostic starts are retained separately.
- **NOT VERIFIED:** any all-five-check native success; fully attested rebuild of
  the supplied Homebrew firmware; compatibility on other firmware/QEMU versions,
  Linux/Windows/WSL; live TPM backend; alternate direct-kernel boot; exact deeper
  TLS failure mechanism. No hosted CI or independent-person pass is claimed.

The offline checker passes **83 evidence consistency checks** normally and under
Python -O. These are same-author identity/scope checks, not independent scientific
validation. They verify that the unsuccessful matrix is still unsuccessful,
that module/status claims agree with retained output and that verification
sources and the tracked image match the baseline. Hashes establish identity,
not correctness or provenance authenticity.

Native build prerequisites had retained failures: BaseTools first rejected a
host macro-redefinition warning; IASL first hit an Apple linker fixup alignment
failure; two firmware builds lacked pinned metadata-header submodules and exited
14. Successful host-tool retries use a macro-warning severity adjustment and
`-no_fixup_chains`, respectively. These temporary native-tool compatibility flags
do not suppress firmware diagnostics or alter repository CI. Exact commands,
exits and all 18 raw setup/build/regression log streams are in
[build-receipts.json](evidence/build-receipts.json) and its compressed log archive.

## Reproduction and retention

The source and binary identities, exact build flags, command exits and environment
are retained. The firmware build is reproducible procedurally from its pinned
sources; bit-identical rebuilds are **not** established. Built diagnostic code,
vars, ELF/debug symbols and intermediate objects remain under
`/private/tmp/t81-phase4/edk2/Build`; firmware binaries are not vendored into the
repository. Running the native matrix again needs those binaries or a rebuilt
counterpart, with any changed hashes recorded. The exact fresh guest input is
retained as `evidence/fresh-guest.img.gz`.

1. Check out the base and create a fresh scratch root. Build the guest using the
   first three commands in `evidence/build-receipts.json`.
2. Fetch EDK2 at the pinned commit and initialize the gitlinks listed in
   `provenance.json`. Build BaseTools and the pinned IASL tool with the recorded
   successful host-tool commands. Apply the patch string from
   `evidence/instrumentation.json` to the pinned source with `git apply`.
3. Run the recorded final DEBUG firmware build command with the recorded
   PYTHON_COMMAND, CLANGDWARF_BIN and IASL_PREFIX. Keep all features/error reporting
   enabled as recorded. Copy QEMU_EFI.fd and QEMU_VARS.fd and extend each copy to
   67108864 bytes for pflash, preserving its existing bytes.
4. The host-specific fixtures use `/private/tmp/t81-phase4`. Copy the retained
   `matched_qemu.py` there and make it executable. `run_matrix.py` requires fresh
   row directories and records every unsuccessful or successful result without
   overriding the verifier. Review its fixed inputs before repeating on another
   host; it is not a portable production interface.

```sh
python3 docs/architecture/diagnostics/phase4/run_matrix.py
python3 tests/qemu/test_verification.py
python3 docs/architecture/diagnostics/phase4/check_evidence.py
python3 -O docs/architecture/diagnostics/phase4/check_evidence.py
```

The matrix needs local Unix sockets and native QEMU execution; the existing guest
image builder uses macOS temporary disk facilities. Those operations ran outside
the managed sandbox. No Linux/macOS/Windows portability conclusion follows from
one macOS ARM64 host. The observational helper's flags reproduce the source-attribution
controls; their exact executed argv is retained in the observations, rather than
being promoted to acceptance commands.

No verifier relaxation, marker fabrication, log filtering, production image
replacement, unrelated OS/ISA functionality, normative change, frozen research
artifact modification, SCG-08 Gate 2, governance activation or automatic merge.

## Exact changed-file inventory

16 additions under this investigation directory:

- `docs/architecture/diagnostics/phase4/REPORT.md`
- `docs/architecture/diagnostics/phase4/check_evidence.py`
- `docs/architecture/diagnostics/phase4/evidence/SHA256SUMS.json`
- `docs/architecture/diagnostics/phase4/evidence/build-and-setup-logs.json.gz`
- `docs/architecture/diagnostics/phase4/evidence/build-receipts.json`
- `docs/architecture/diagnostics/phase4/evidence/fresh-guest.img.gz`
- `docs/architecture/diagnostics/phase4/evidence/instrumentation.json`
- `docs/architecture/diagnostics/phase4/evidence/matrix.json`
- `docs/architecture/diagnostics/phase4/evidence/mock-processes.json`
- `docs/architecture/diagnostics/phase4/evidence/mock-tests.log`
- `docs/architecture/diagnostics/phase4/evidence/observations.json`
- `docs/architecture/diagnostics/phase4/evidence/provenance.json`
- `docs/architecture/diagnostics/phase4/evidence/validation.json`
- `docs/architecture/diagnostics/phase4/matched_qemu.py`
- `docs/architecture/diagnostics/phase4/observe.py`
- `docs/architecture/diagnostics/phase4/run_matrix.py`
