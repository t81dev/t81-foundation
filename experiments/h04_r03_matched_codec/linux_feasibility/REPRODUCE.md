# Reproduce the isolated feasibility diagnostics

These commands run small **synthetic counterexamples**, not Phase C. They do not
execute the codec arms, generate workload/query schedules, collect elapsed
samples or bypass the execution freeze. Intentional 48 MiB/128 KiB allocations
are diagnostic falsifiers outside experiment trials, not changes to its limits.

## Inputs and resource budget

- Baseline: `dbc8c1ac05aebf9f64dccc45968b75e891b6ff21` including merged #682/#683.
- Actual host: macOS 26.5.2 ARM64, Python 3.14.3, QEMU 10.2.2.
- At most one live VM: x86_64 TCG, q35, CPU max, one vCPU, 1024 MiB RAM, no host
  filesystem sharing, no host port forwarding, diskless disposable root.
- Official Alpine 3.21.9 x86_64 virtual ISO, 67,108,864 bytes; SHA-256
  `1ceafa2ca068faf2925de977815681c182708f3c6cbfbad137f3ffc9324a03b4`.
- ISO/source/package downloads and ordinary compiler setup were explicit setup
  actions. No performance credit or dedicated-host qualification is inferred.
- Temporary host setup stayed under `/private/tmp/t81-h04-feasibility-vm`.
  Two boot attempts were made sequentially; the failed first guest was powered
  off before the second started. Both guests and the file server were stopped
  before the draft PR was opened.

The actual acquisition used Python urllib; the curl commands below reproduce the same URLs and digest checks. The publisher's HTTPS `.sha256` file was compared with the full ISO before
extraction. It is recorded in [the download manifest](evidence/download-manifest.json).
PGP verification was not performed; identity hashes are not authenticity proofs.
No prior ARM64 VM artifacts were executed or reused for the x86_64 result.

## Boot and setup

From the official URL below, download the ISO and its `.sha256`, verify the
actual bytes against the pinned value above, then extract its three boot files.
Do not silently accept updated bytes from a moving release directory.

```sh
set -eu
mkdir -p /private/tmp/t81-h04-feasibility-vm/boot /private/tmp/t81-h04-feasibility-vm/share
cd /private/tmp/t81-h04-feasibility-vm
curl --fail --location -O https://dl-cdn.alpinelinux.org/alpine/v3.21/releases/x86_64/alpine-virt-3.21.9-x86_64.iso
curl --fail --location -O https://dl-cdn.alpinelinux.org/alpine/v3.21/releases/x86_64/alpine-virt-3.21.9-x86_64.iso.sha256
shasum -a 256 -c alpine-virt-3.21.9-x86_64.iso.sha256
printf '1ceafa2ca068faf2925de977815681c182708f3c6cbfbad137f3ffc9324a03b4  alpine-virt-3.21.9-x86_64.iso\n' | shasum -a 256 -c -
tar -xf alpine-virt-3.21.9-x86_64.iso boot/vmlinuz-virt boot/initramfs-virt boot/modloop-virt
```

Copy only `containment_probe.c`, `native_scratch_probe.py` and
`guest_diagnostics.sh` from this directory into that temporary `share` directory.
The executed setup used a temporary **loopback-only** server:

```sh
python3 -m http.server 18765 --bind 127.0.0.1 --directory /private/tmp/t81-h04-feasibility-vm
```

In another terminal, the successful guest launch command was:

```sh
script -q /private/tmp/t81-h04-feasibility-vm/console.iso-boot.log \
  qemu-system-x86_64 -machine q35,accel=tcg -cpu max -m 1024 -smp 1 \
  -kernel /private/tmp/t81-h04-feasibility-vm/boot/vmlinuz-virt \
  -initrd /private/tmp/t81-h04-feasibility-vm/boot/initramfs-virt \
  -cdrom /private/tmp/t81-h04-feasibility-vm/alpine-virt-3.21.9-x86_64.iso \
  -append 'console=ttyS0 cgroup_no_v1=all' \
  -netdev user,id=n0 -device virtio-net-pci,netdev=n0 -nographic -no-reboot
```

`script` syntax is the macOS/BSD version used here; Linux recorders may have a
different command syntax. This is not permission to use a VM as the measurement
host. The full raw console is retained in [successful-boot.raw.log](evidence/successful-boot.raw.log).

Log in as root in the fresh diskless guest. The actual setup used:

```sh
ip link set eth0 up
udhcpc -i eth0 -n -q
printf 'https://dl-cdn.alpinelinux.org/alpine/v3.21/main\n' > /etc/apk/repositories
apk add --no-progress gcc musl-dev linux-headers python3
```

The exact versions observed were GCC 14.2.0-r4, binutils 2.43.1-r3, musl/musl-dev
1.2.5-r12, linux-headers 6.6-r1 and Python 3.12.16-r0. The complete 54-package
inventory and binary hashes are in the raw diagnostics. A future reproduction
must pin and archive the actual package dependency closure; a moving APK index
alone is not hermetic. If these exact inputs cannot be restored, report the
mismatch instead of claiming an exact reconstruction. Kernel source review is
pinned separately in [kernel-source-manifest.json](evidence/kernel-source-manifest.json).

## Execute the diagnostics

**Run `guest_diagnostics.sh` only in a disposable diagnostic guest or a separately
approved prepared environment.** It enables memory/pids controllers in the root
hierarchy and creates a leaf; it is not a read-only host-qualification probe.
No host-wide CPU/swap/cache settings are changed, and it provides no general
launcher for supplied commands.

Inside the guest:

```sh
mkdir -p /tmp/h04-feasibility
cd /tmp/h04-feasibility
wget -q http://10.0.2.2:18765/share/containment_probe.c
wget -q http://10.0.2.2:18765/share/native_scratch_probe.py
wget -q http://10.0.2.2:18765/share/guest_diagnostics.sh
sh guest_diagnostics.sh > diagnostics.final.log 2>&1
```

Record the shell exit status immediately, retain the entire log even on failure,
and compare the source hashes to this PR. The native compile commands are in
the script. They use `-O2 -std=c11 -Wall -Wextra -Werror`, a fixed random seed,
file-prefix mapping and no linker build ID; the final binaries are dynamically
linked. `readelf` records the interpreter. Two builds are compared with `cmp`.

The script refuses a pre-existing leaf rather than resetting history. It sets
32 MiB **cgroup memory** and zero swap with pids.max=1 for the counterexample;
these are not asserted to be exact RSS enforcement. The source's `--rss-limit`
mode sets the independent process RLIMIT_RSS to 32 MiB, touches 48 MiB and records
its surviving RSS. The clone case inherits parent-precharged shmem, records
membership before/after exec, then observes its own RSS and group usage. The
Python case uses one 128 KiB native libc allocation outside tracemalloc coverage.
There are no clocks, arbitrary worker commands, arm selectors or benchmark loops.
An alarm is used only as a failure timeout; kernel boot timestamps are ordinary
environment metadata, not Phase C timing samples.

Expected **counterexample predicates**, not fixed PID/RSS byte-for-byte values:

- All three child membership checkpoints identify the same intended leaf while
  the parent is outside, and the leaf's process list identifies that child.
- Detailed observed RSS exceeds 33,554,432 bytes in the RLIMIT_RSS case.
- Contained child's detailed RSS exceeds that value while group current/peak
  remain below it, with no OOM events, in the precharged-sharing case.
- Native requested bytes exceed 65,536 while traced peak stays below it.
- Child is reaped, but an explicitly retained shared backing page is resident
  at the mincore observation. One Python object's alias test behaves as recorded.

A different result is a failure/inconclusive reproduction of this diagnostic,
not permission to tune a favorable case. Do not infer overall feasibility even
when every predicate holds: these primarily falsify proposed weaker controls.

After collecting results, power off the guest (`poweroff -f` in this disposable
VM) and stop the host file server. Do not terminate an unrelated VM/process.
The actual run transferred the two exact diagnostic logs by base64 on the serial
console; decoded exports are retained next to the raw console. CR removal was
used only to parse serial framing; base64 validation preserved exact log bytes.

## Retained setup failures

1. Per-file netboot `.sha256` metadata returned HTTP 404. No boot assets from
   that attempt were used. The fix was the publisher-hashed virtual ISO, not a
   nonexistent SHA256SUMS file; the initial note mentioning that name is retained
   as an imperfect setup note in [download-failure.txt](evidence/download-failure.txt).
2. The ISO initramfs booted without its CD and was incorrectly asked for diskless
   DHCP. DHCP/root bootstrap was unavailable, ending in a recovery shell, which
   was powered off. See [first-boot-failure.raw.log](evidence/first-boot-failure.raw.log).
   Attaching the verified ISO and using its normal boot flow resolved setup.
3. The first native build pair was static and byte-identical, but root control
   setup enabled only memory; `pids.max` was absent and the script stopped with
   status 1. The empty owned leaf was explicitly removed, both parent controllers
   enabled, and a fresh leaf used in the successful dynamically linked rerun.
   The [failed log](evidence/diagnostics.first-controls-failure.log) is preserved;
   its static artifact identity is not substituted for the final dynamic one.

No failure was relabeled as a successful qualification.

## Offline evidence integrity

Run `python3 verify_evidence.py` from this directory (standard library only).
It verifies manifest hashes, source identities, captured markers and the recorded
counterexample predicates, and checks that F1–F5 remain classified consistently.
It does not run Linux, execute a diagnostic, inspect a prospective host, authorize
measurements or establish scientific truth. Its output must still say
`FEASIBILITY BLOCKED`, `host_qualified: false` and `phase_c_executed: false`.


## Validation performed for this PR

The final Linux guest diagnostic exited **0** and ended in
`H04_FEASIBILITY_DONE`. Both native compilations passed `-Werror`; their bytes
matched. The earlier controls failure exited **1** and remains in the evidence.
Neither successful execution nor matching builds establishes feasibility.

On the macOS authoring host, these checks passed (the Git check covers staged
source/document changes and excludes verbatim logs):

```sh
python3 -B experiments/h04_r03_matched_codec/linux_feasibility/verify_evidence.py
sh -n experiments/h04_r03_matched_codec/linux_feasibility/guest_diagnostics.sh
git diff --cached --check -- . \
  ':(exclude)experiments/h04_r03_matched_codec/linux_feasibility/evidence/*.log'
```

Python `ast.parse` accepted both new Python sources; `json.loads` accepted every
JSON file in this directory. An additional standard-library negative integrity
check copied this directory into a `TemporaryDirectory`, appended `TAMPER\n` to
the copied `evidence/diagnostics.final.log`, and called `verify(copy)` from
`verify_evidence.py`. It rejected the modified copy with
`ValueError("artifact identity mismatch: evidence/diagnostics.final.log")`.
The original artifacts were unchanged. These checks exercise syntax and artifact
consistency only. No codec tests, Phase C timing runs or host-qualification run
were executed as part of this validation.

The unrestricted `git diff --cached --check` exited **2** because verbatim
captured logs contain trailing spaces and serial-console CR characters. Those
original evidence bytes are intentionally retained. The scoped check above
passed; no source/document whitespace failures remain. An earlier unstaged
`git diff --check` passed while files were still untracked and did not inspect
the added content; it is not relied on as validation of the additions.
