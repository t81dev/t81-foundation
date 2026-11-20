# T81-ARCH-ENG-002 Phase 3: UEFI boot investigation

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81-ARCH-ENG-002 Phase 3: UEFI boot investigation](#t81-arch-eng-002-phase-3-uefi-boot-investigation)
  - [Findings](#findings)
  - [Inputs and build](#inputs-and-build)
  - [Controlled real execution](#controlled-real-execution)
  - [Correction and acceptance status](#correction-and-acceptance-status)
  - [Reproduction](#reproduction)
- [Reproduce authoritative rejection; expected exit is 1 on the recorded inputs.](#reproduce-authoritative-rejection-expected-exit-is-1-on-the-recorded-inputs)
- [Read-only media inspection and unchanged regression suite.](#read-only-media-inspection-and-unchanged-regression-suite)
  - [Limits and next boundary](#limits-and-next-boundary)
  - [Changed-file inventory](#changed-file-inventory)

<!-- T81-TOC:END -->


Date: 2026-11-10. **READY FOR INDEPENDENT REVIEW — investigation and artifact-status
documentation only. No complete native verification pass or boot repair is claimed.**

Base: **cd490f5b5c0496350682542b56424dd77bea6963**, freshly fetched origin/main;
PR #756 is merged at that exact commit. Evidence/fixture/documentation head:
**e9ff215e0967a377b0c62af35d955ef5165bd0d7**. The subsequent report-only packaging
commit is identified by the final draft PR head and description.

Author: Codex assistant with the prior Phase 2 implementation context. All new
executions here were performed by this same assistant. Controlled counterfactuals
are fresh engineering evidence, not independent-person review or qualification.
The original dirty maintainer checkout was preserved; execution used an isolated
clean checkout of the base at `/private/tmp/t81-qemu-phase3`.

## Findings

There are two distinct barriers, not a demonstrated single fatal UEFI image-load
failure:

1. **The verifier stops at firmware diagnostics before testing guest readiness.**
   Both tracked and fresh images return exit 1, 2/5, VERIFICATION FAILED under
   the unchanged verifier. A diskless control emits the same early TRNG and
   image-start diagnostics. Therefore those messages do not establish failure
   of the TernaryOS EFI payload. The TRNG sentence alone does not match the
   verifier's failure regex; the subsequent `Error: Image ...` does.
2. **The tracked payload cannot provide the current shell.** Its embedded EFI
   is a historical 5,120-byte probe, identical to the staging copy, with a debug
   WFI loop and no shell-ready marker. An observational continuation using the
   original launch arguments reaches its EFI banner, post-ExitBootServices
   messages and that loop. The freshly compiled 75,264-byte EFI reaches the
   real shell and replies to `help`, while preserving the firmware errors.
   Reciprocal single-file substitutions reverse this behavior. The stale
   payload, rather than GPT layout or absent boot files, explains this tested
   difference in guest behavior.

**Root cause established within tested inputs:** stale tracked EFI content explains
missing shell behavior; an independent firmware-output/verification-contract
mismatch explains the immediate 2/5 rejection. **Not established:** which exact
firmware drivers return each error, the complete firmware compatibility repair,
or an error-free five-check native configuration. No causal claim that TRNG
failure crashes the guest is supported.

## Inputs and build

| Input | SHA-256 |
|---|---|
| Tracked raw image | `e4e0df2a3c5640367d451ef7379a87e55c8efb7624e8a4746372236f9a06089d` |
| Tracked embedded/staged EFI | `83d30ea29faa522f514bcbb1ec2c3e0056e8e4474320bce8e2876231b7b8f1da` |
| Fresh raw image | `7abf1b32a3f16f85e115e0dd8e8dd61b904651e40f2d5d51c3758a4180441718` |
| Fresh EFI | `e1067c8d95c31b31b50d7d02704ddf897a72a1738fcf0dd2082be1567127e3bc` |
| Supplied EDK2 code firmware | `47765fe344818cbc464b1c14ae658fb4b854f5c2ceffa982411731eb4865594d` |

Host: macOS ARM64 / Darwin 25.5.0; QEMU 10.2.2; Python 3.14.3; Bash 3.2.57;
CMake 4.3.0; hosted AppleClang 21.0.0; EFI compiler Homebrew Clang 22.1.1;
Homebrew LLD 22.1.1. Full versions, executable/firmware hashes and stdout/stderr
are in [environment.json](evidence/environment.json).

The standard CMake EFI target and existing macOS image builder both complete
with exit 0. Exact configure/build/image commands and full logs are in
[build.json](evidence/build.json). No production source was edited to obtain
the fresh build. The Asio override supplied to configure was unused, as the log
records; it is not an assertion that an external dependency was fetched or used.

Both GPT header and partition-array CRCs validate. Both images expose the ESP,
EFI/BOOT/BOOTAA64.EFI and TERNOS data. Extraction identifies ARM64 (`0xaa64`),
PE32+ (`0x20b`), EFI application subsystem 10 in both payloads. The old payload
has preferred image base `0x140000000` and no relocation directory; the fresh
payload has base zero and relocation data. Those differences are recorded,
but **no header-only cause is inferred**: both payloads actually execute, and
neither CPU nor pflash variations convert the old probe into a shell.

The tracked profile describes `019034a8` and a March 2026 generation date; these
are unauthenticated artifact metadata, not proof of the binary's exact source.
The fresh builder writes `git_commit=unknown` in this layout; external evidence
pins its actual source checkout and inputs. That incidental metadata limitation
is retained, not fixed opportunistically or treated as a boot cause.

## Controlled real execution

All observational launches use one CPU, 512 MiB RAM, virt/GICv3 and a disposable
image copy. The **observer is not the verifier**: it retains every firmware error,
allows diagnostic continuation for 10–20 seconds, sends `help` only after the
real ready marker, checks the reply after the pre-send output offset, and requests
QMP quit. A QEMU exit 0 below means that bounded QMP shutdown completed, not that
the verifier accepted the run. Nothing injects a ready/help marker.

| Observation | Controlled difference | Ready + fresh help | QEMU exit |
|---|---|---|---|
| tracked-original | Original verifier launch configuration, diagnostic continuation | No; debug WFI loop | 0 |
| tracked-max | CPU changed to max | No; debug WFI loop | 0 |
| tracked-nodefaults | Add nodefaults only | No; debug WFI loop | 0 |
| tracked-pflash-only | Code pflash + fresh vars instead of bios | No; debug WFI loop | 0 |
| tracked-pflash | Pflash/vars plus nodefaults | No; debug WFI loop | 0 |
| no-guest-disk | Remove boot drive and device | No; early errors persist without TernaryOS | 0 |
| fresh-original | Fresh image, original configuration | **Yes**, actual guest response | 0 |
| fresh-pflash | Fresh image, pflash/vars plus nodefaults | **Yes**, actual guest response | 0 |
| tracked-current-payload | Replace only historical EFI with current EFI in tracked-image copy | **Yes** | 0 |
| fresh-historical-payload | Replace only current EFI with historical EFI in fresh-image copy | No; debug WFI loop | 0 |

Full argv, input hashes, QMP, serial, process output and recorded dispositions for
all ten runs: [observations.json](evidence/observations.json). The substitution
fixture adjusts FAT chains/file size on disposable copies. Recursive file-content
hashes show **EFI/BOOT/BOOTAA64.EFI is the only changed file** in each direction;
all other files, including profiles, retain their content. Forward/reverse image
and payload identities are in `substitution.json`, `reverse-substitution.json`
and their corresponding media records.

The max CPU removes the particular numeric `00000001` diagnostic in this sample,
but leaves other firmware errors and the old guest loop. Nodefaults removes the
unrelated X64 option-ROM warning; it does not remove the early errors or supply
a shell. Pflash/vars does not resolve them. These are controlled observations,
not a general compatibility matrix.

The matching upstream EDK2 source version's TRNG constructor emits its diagnostic
and returns success even through its error handler; thus the text itself is not
a fatal-return assertion. See [ArmTrngLib.c at the resolved source commit](https://github.com/tianocore/edk2/blob/b158dad150bf02879668f72ce306445250838201/ArmPkg/Library/ArmTrngLib/ArmTrngLib.c#L302).
The [DXE image service's generic diagnostic](https://github.com/tianocore/edk2/blob/b158dad150bf02879668f72ce306445250838201/MdeModulePkg/Core/Dxe/Image/Image.c#L1684)
reports a failing image entrypoint without identifying that image as TernaryOS.
The tag-to-commit resolution is retained in `upstream-sources.json`. This source
inspection is **not** an independent rebuild, binary/module attribution or proof
of exactly which SMC capability check fails in the installed firmware. The
no-disk and continued-guest observations independently bound the interpretation.

## Correction and acceptance status

The production correction here is limited to **accurate artifact-status
documentation** in `drivers/qemu/production_image/README.md`. No tracked image,
bootloader, builder, launch argument, verifier or five-check definition is changed.
A one-file payload replacement is demonstrated on disposable copies, but it is
not published as a completed boot repair: the unchanged verifier still rejects
the freshly built image's firmware output. Replacing the production binary now
would not satisfy native acceptance and would hide an unresolved prerequisite.
The fresh image is retained as investigation evidence only, compressed at
[evidence/fresh-image.img.gz](evidence/fresh-image.img.gz), not selected as a new
default or promoted to a supported/qualified artifact.

| Evidence level | Actual result |
|---|---|
| STATICALLY VERIFIED | GPT CRCs, EFI identities/format/strings; single-file swap manifests; unchanged verifier bytes; Python syntax; whitespace |
| MOCK-EXECUTION VERIFIED | Unchanged suite: **11/11 methods**, **42 entrypoint receipts** (9 accepted, 33 rejected), plus its supervised interruption and arithmetic fixtures; 33.646 seconds |
| QEMU-EXECUTION VERIFIED | Ten bounded diagnostic observations; real shell/help for fresh and forward-substituted payloads; no shell for historical/reverse payloads; two final full-verifier failures |
| NOT VERIFIED | **Any all-five-check native pass**; error-free firmware configuration; full OS correctness; other firmware/QEMU/platform versions; Linux/Windows/WSL behavior |

The two final unchanged-verifier receipts are retained in
[verifier-runs.json](evidence/verifier-runs.json): **both exit 1 at 2/5** with
failure output in serial.log. No zero observer exit, marker observation, mock
success or source existence is promoted into full native acceptance.

The offline evidence checker passes **109 consistency checks**, normally and
under Python -O. That is an integrity/scope check authored with this report,
not independent scientific validation. Existing regression output, per-process
mock receipts and validation argv/exits are retained. No new production behavior
is patched, so no passing new regression is offered as proof of an execution fix.

## Reproduction

Run from the repository checkout. These host-specific investigation fixtures
use `/private/tmp/t81-phase3` and the tested Homebrew firmware paths; they are not
new general-purpose runtime interfaces. Keep the existing verification contract.

```sh
# Reproduce authoritative rejection; expected exit is 1 on the recorded inputs.
QEMU_IMG="$PWD/drivers/qemu/production_image/qemu_slice6_guest.img" \
  OUTPUT_DIR=/private/tmp/t81-phase3/reproduce VERIFICATION_TIMEOUT=40 \
  bash drivers/qemu/scripts/automated_verification.sh

# Read-only media inspection and unchanged regression suite.
python3 docs/architecture/diagnostics/phase3/inspect_media.py
python3 tests/qemu/test_verification.py
python3 docs/architecture/diagnostics/phase3/check_evidence.py
```

For the fresh build, repeat the exact three commands in `evidence/build.json`.
The image builder requires macOS disk-image facilities; it creates and detaches
a temporary volume. No Linux builder support is inferred. For an exact retained
fresh image, decompress `evidence/fresh-image.img.gz` to a temporary file and
verify its raw SHA-256 above. All observer output paths must be fresh names:

```sh
python3 docs/architecture/diagnostics/phase3/observe.py reproduce-tracked \
  --image drivers/qemu/production_image/qemu_slice6_guest.img --seconds 15
python3 docs/architecture/diagnostics/phase3/observe.py reproduce-diskless \
  --image drivers/qemu/production_image/qemu_slice6_guest.img --no-disk --seconds 10
python3 docs/architecture/diagnostics/phase3/observe.py reproduce-fresh \
  --image /private/tmp/t81-phase3/fresh-image/qemu_slice6_guest.img --seconds 20
python3 docs/architecture/diagnostics/phase3/substitute_efi.py \
  drivers/qemu/production_image/qemu_slice6_guest.img \
  /private/tmp/t81-phase3-build/ternaryos/qemu_slice6/BOOTAA64.EFI \
  /private/tmp/t81-phase3/reproduce-substituted.img
python3 docs/architecture/diagnostics/phase3/observe.py reproduce-substituted \
  --image /private/tmp/t81-phase3/reproduce-substituted.img --seconds 15
```

Observer options `--cpu max`, `--nodefaults`, `--pflash` reproduce the configuration
controls. To reverse the swap, use the fresh image as input and the tracked
staging EFI as payload. Source code and retained argv define each exact run;
the helpers do not filter errors or modify the public verifier. Unix socket and
image construction operations required execution outside the managed sandbox.

## Limits and next boundary

A firmware/launch combination that satisfies the existing error-free contract
remains unresolved. Diagnosing the installed firmware's exact failing modules
would require additional instrumented firmware or symbol/build evidence; this
report does not guess those identities. The old binary's exact source ancestry
is also not established solely from its profile. Time-limited observations do
not prove infinite nontermination, but the visible historical loop, missing shell
content and reciprocal payload controls establish the tested missing-shell cause.

The focused follow-on is a separately reviewed compatible firmware configuration
with raw evidence of **all five checks**, real ready/fresh help and QEMU exit 0.
Do not obtain it by ignoring the current errors, changing failure matching,
inserting markers or relabeling observational exits as success.

No unrelated CI repair, ISA/arithmetic/OS expansion, normative change, frozen
research artifact modification, SCG-08 Gate 2, governance activation or merge.
Content hashes establish identity, not correctness, provenance authenticity or
independent acceptance. All original failures and the unsuccessful full-native
status remain explicit.

## Changed-file inventory

22 additions relative to the base:

- `docs/architecture/diagnostics/phase3/REPORT.md`
- `docs/architecture/diagnostics/phase3/check_evidence.py`
- `docs/architecture/diagnostics/phase3/evidence/SHA256SUMS.json`
- `docs/architecture/diagnostics/phase3/evidence/build.json`
- `docs/architecture/diagnostics/phase3/evidence/environment.json`
- `docs/architecture/diagnostics/phase3/evidence/fresh-image.img.gz`
- `docs/architecture/diagnostics/phase3/evidence/fresh-media.json`
- `docs/architecture/diagnostics/phase3/evidence/media.json`
- `docs/architecture/diagnostics/phase3/evidence/mock-processes.json`
- `docs/architecture/diagnostics/phase3/evidence/mock-tests.log`
- `docs/architecture/diagnostics/phase3/evidence/observations.json`
- `docs/architecture/diagnostics/phase3/evidence/reverse-media.json`
- `docs/architecture/diagnostics/phase3/evidence/reverse-substitution.json`
- `docs/architecture/diagnostics/phase3/evidence/substituted-media.json`
- `docs/architecture/diagnostics/phase3/evidence/substitution.json`
- `docs/architecture/diagnostics/phase3/evidence/upstream-sources.json`
- `docs/architecture/diagnostics/phase3/evidence/validation.json`
- `docs/architecture/diagnostics/phase3/evidence/verifier-runs.json`
- `docs/architecture/diagnostics/phase3/inspect_media.py`
- `docs/architecture/diagnostics/phase3/observe.py`
- `docs/architecture/diagnostics/phase3/substitute_efi.py`
- `drivers/qemu/production_image/README.md`
