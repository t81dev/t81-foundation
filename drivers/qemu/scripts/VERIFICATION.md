# Bounded QEMU verification

Run either entry point from any working directory:

```sh
QEMU_IMG=/absolute/path/qemu_slice6_guest.img \
  OUTPUT_DIR=/absolute/path/results \
  bash /path/to/t81-foundation/ternaryos/automated_verification.sh
```

`drivers/qemu/scripts/verify_production.sh` defaults to the image under
`drivers/qemu/production_image/`. The automated entry points default to
`build/qemu_test_debug/qemu_slice6_guest.img` relative to the repository, preserving
the previous development-image choice. Set `QEMU_IMG` to select a newly built
image. Explicit relative input/output paths resolve against the invoking working
directory. Missing or empty images and missing executables fail.

Requirements: POSIX host, Bash, Python 3.9+, QEMU AArch64 with QMP, and AArch64 EDK2
firmware. `QEMU_BIN` selects the executable; `BIOS` selects firmware instead of
searching standard Linux/macOS locations. `VERIFICATION_TIMEOUT` bounds QEMU
execution, default 60 seconds, maximum 600. The worker uses private local Unix
sockets, TCG/cortex-a57, GICv3, one CPU, and 512 MiB RAM. Native Windows is not a
supported invocation path; WSL has not been verified here.

The worker copies the selected image and runs these checks in one QEMU session:

| Check | Required observation | Limit |
|---|---|---|
| `monitor_test` | QMP handshake and `query-status` reports running | Host monitor, not guest correctness |
| `memory_analysis` | QMP `pmemsave` returns and creates exactly 4096 bytes at guest RAM address 0x40000000 | RAM sampling only; no memory-layout inference |
| `boot_sequence_test` | Serial emits the exact t81sh ready marker from `qemu_slice6_cpp_bridge.cpp` | Guest reaches this marker |
| `hardware_test` | QMP reports CPU 0 as the sole CPU and an inserted hd0 | QEMU configuration, not guest driver qualification |
| `integration_test` | Sending `help` yields the known shell-help line in newly appended serial output | One guest command/response |

The final success disposition additionally requires a QMP `quit` reply, zero
QEMU exit, nonempty QMP/serial evidence, and no explicit failure markers in the
captured streams. This is deliberately conservative: `error`, `fail`, `failed`,
`failure`, `fatal`, `panic`, `assertion`, and the cross mark reject a run. A benign
message containing those words may therefore require separate investigation;
these messages are never silently ignored to obtain a pass. The diagnostic QEMU
stderr/stdout log may legitimately be empty.

`OUTPUT_DIR` is a **parent directory**. Each attempt creates a private, fresh
`ternaryos-verification-*` child directory; it never clears or reuses previous
reports. An unset or empty value uses the system temporary directory. The worker
prints the exact attempt path and retains command argv, image copy, RAM sample,
QMP transcript, serial and process diagnostics, and the summary. Retained image
copies consume disk space; remove completed attempt directories when no longer
needed. Do not infer success from a historical report or file presence. Check the
current invocation's exit status: 0 for all checks plus final exit/evidence;
124 for deadline expiry; 128+signal for interruption; child failures preserve
positive exit status (signals map to 128+signal); other verification failures
return 1. A partial count never makes the attempt successful.

The two automated shell entry points and the generated deployment verifier use
the same worker. The old monitor/memory inspection scripts remain diagnostic
utilities and are not acceptance authorities. This repair does not certify the
full deployment installer, boot-image builder, OS, arithmetic, or architecture.

Run isolated regressions without QEMU:

```sh
python3 tests/qemu/test_verification.py
python3 tests/qemu/reproduce_baseline.py
```

The second command needs Git history containing the pinned Phase 1 merge and
prints actual local Bash behavior; Bash 3.2 does not reproduce the newer-shell
arithmetic termination. Mock QMP/serial success establishes harness control flow
only. It does not establish guest execution or qualification.
