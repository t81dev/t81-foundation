# Tracked historical QEMU boot probe

The tracked `qemu_slice6_guest.img` contains a historical 5,120-byte
`EFI/BOOT/BOOTAA64.EFI`, matching the adjacent staging copy. Its embedded profile
reports revision `019034a8` and a March 2026 build date. These metadata fields are
not an attestation. Actual QEMU observation reaches a debug WFI loop; this payload
does not implement the current shell-ready/help behavior.

Do not use this image as evidence that the current shell has been built or that
all verification checks pass. Build the current `t81_ternaryos_qemu_slice6_efi`
target and construct a fresh image with
`ternaryos/scripts/build_qemu_slice6_artifact.sh` where that builder is supported,
then select it explicitly with `QEMU_IMG`.

A refresh alone is insufficient for the tested environment: the supplied EDK2
firmware also emits early errors with **no guest disk attached**, and the current
verifier rejects those errors for both historical and fresh images. No five-check
native pass is claimed. The tracked binary and its hash remain unchanged.

See the [Phase 3 investigation](../../../docs/architecture/diagnostics/phase3/REPORT.md)
for reciprocal payload controls, input identities, all raw diagnostics, fresh
image archive, exact build/reproduction commands and remaining limitations.
