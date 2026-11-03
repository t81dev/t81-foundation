# BASE-01 physical Linux onboarding: correctness only

Status: operator instructions, not an executed host report or qualification.
Use only after a maintainer assigns a dedicated physical Linux x86_64 host and
an operator. No host is assigned by this document. Do not run these commands on
the current workstation, a shared runner, VM or container as a substitute.

References: [#689 proposed host profile](https://github.com/t81dev/t81-foundation/blob/64c3f71ff100ea16270f1a70497ac39860c1b772/docs/research/base01/BASE01_B1_HOST_QUALIFICATION.md),
[#691 engineering evidence](https://github.com/t81dev/t81-foundation/tree/db0e7c8b976991396eed7d14ddc1e889e9c511c2/experiments/base01_b1_foundation),
[#692 collector evidence](https://github.com/t81dev/t81-foundation/tree/d355511e75885a4a91299f54ceb108eda65ac925/experiments/base01_resource_accounting).
Their historical receipts remain unchanged. This runbook grants no timing or
configuration-change authority and does not approve #689's proposed thresholds.

## 1. Session and evidence capture

Operator prerequisites: Bash, Git, Python 3.12+, CMake >=3.20, Ninja, a C11/C++23
GCC toolchain, util-linux tools, binutils and SHA-256 tools. Provisioning missing
packages is a separate operator action. Network access is needed for pinned Git
objects and the audited build's Asio dependency, unless verified copies exist.
Run as an ordinary user in an assigned, empty workspace. Fill the three values
below with the approved operator, asset and absolute workspace; do not reuse an
earlier evidence directory. No sudo, installation or configuration write follows.

```bash
set -euo pipefail
umask 077
export LC_ALL=C
export BASE01_OPERATOR='REPLACE_WITH_ASSIGNED_OPERATOR'
export BASE01_HOST_ASSET='REPLACE_WITH_PHYSICAL_ASSET_ID'
export BASE01_SESSION='/absolute/operator-approved/new-base01-session'
mkdir "$BASE01_SESSION"
cd "$BASE01_SESSION"
SESSION=$(pwd -P)
EVIDENCE="$SESSION/evidence"
mkdir "$EVIDENCE" "$EVIDENCE/logs"
printf 'operator=%s\nasset=%s\nscope=correctness-only\nmeasurement_authorized=false\n' \
  "$BASE01_OPERATOR" "$BASE01_HOST_ASSET" > "$EVIDENCE/session.txt"

# Every invocation retains argv, stdout, stderr and exit code. No elapsed clock.
capture() {
  local label=$1
  shift
  printf '%q ' "$@" > "$EVIDENCE/logs/$label.argv"
  printf '\n' >> "$EVIDENCE/logs/$label.argv"
  if "$@" > "$EVIDENCE/logs/$label.stdout" 2> "$EVIDENCE/logs/$label.stderr"; then
    CAPTURE_RC=0
  else
    CAPTURE_RC=$?
  fi
  printf '%s\n' "$CAPTURE_RC" > "$EVIDENCE/logs/$label.exit"
}
must() {
  capture "$@"
  if (( CAPTURE_RC != 0 )); then
    printf 'STOP: %s failed (%s); retain evidence\n' "$1" "$CAPTURE_RC" >&2
    return "$CAPTURE_RC"
  fi
}
```

Retain the exact runbook and written host/operator assignment in the evidence
directory before sealing. They are inputs to the packet, not execution permits
for dynamic qualification.

Use each log label once. Optional inventory failures remain raw failures, not
passes. On a mandatory failure, stop validation, add an operator assessment and
seal evidence using section 6. Do not work around a failure by changing source,
flags, inputs or test selection. No whole-environment dump is needed; avoid
capturing credentials. Physical inventory/serial numbers require restricted
evidence storage and an approved publication/redaction policy.

## 2. One-shot hardware and control inventory

The following commands read settings once; they perform no stability sampling,
idle-wait schedule, timer calibration, governor change or affinity assignment.

```bash
must architecture uname -m
must kernel uname -a
must distribution cat /etc/os-release
must boot cat /proc/sys/kernel/random/boot_id /proc/cmdline
must cpu lscpu --json
capture topology lscpu --extended
must cpuinfo cat /proc/cpuinfo
capture numa numactl --hardware
capture virtualization systemd-detect-virt
capture vm systemd-detect-virt --vm
capture container systemd-detect-virt --container
capture hypervisor cat /sys/hypervisor/type
capture firmware dmidecode --type 0,1,2,4,17
must memory cat /proc/meminfo /proc/swaps
must process-controls cat /proc/self/status /proc/self/cgroup /proc/self/mountinfo
capture affinity taskset -pc $$
capture interrupts cat /proc/interrupts
capture services systemctl list-units --type=service --state=running --no-pager
capture controls bash -c '
  shopt -s nullglob
  for p in /sys/devices/system/cpu/{online,offline,isolated,nohz_full,smt/active} \
    /sys/devices/system/cpu/cpu*/topology/{physical_package_id,core_id,thread_siblings_list} \
    /sys/devices/system/cpu/cpufreq/boost /sys/devices/system/cpu/intel_pstate/no_turbo \
    /sys/devices/system/cpu/cpufreq/policy*/{scaling_driver,scaling_governor,scaling_min_freq,scaling_max_freq,related_cpus} \
    /sys/kernel/mm/transparent_hugepage/enabled /proc/sys/kernel/numa_balancing \
    /sys/class/hwmon/hwmon*/name /sys/class/hwmon/hwmon*/temp*_{label,crit,max}; do
    printf "PATH %s\n" "$p"
    if test -r "$p"; then cat "$p"; else printf "UNAVAILABLE\n"; fi
  done'
capture cgroup-controls bash -c '
  p=$(awk -F: '\''$1=="0" {print $3}'\'' /proc/self/cgroup)
  test -n "$p" || { echo "UNSUPPORTED: locate cgroup v1 controls manually"; exit 2; }
  rc=0
  for name in cpuset.cpus.effective cpuset.mems.effective cpuset.cpus.partition \
    cpu.max memory.max memory.high memory.swap.max pids.max; do
    printf "CONTROL %s\n" "$name"
    cat "/sys/fs/cgroup$p/$name" || rc=2
  done
  exit "$rc"'
```

Operator gates before building:

- Confirm `x86_64`, physical asset/custodian records and out-of-band console/BMC
  or equivalent physical-host attestation. Match DMI, topology and virtualization
  readbacks. `systemd-detect-virt` commonly returns 1 for no detected environment;
  that is not by itself proof of bare metal. Missing tools, hidden hypervisor
  flags or permission-denied DMI are inconclusive. Stop if physical status is
  unverified or contradictory; do not infer it from one CPU string.
- Identify CPU model/stepping/features, microcode, firmware, SMT sibling sets,
  cache sharing, NUMA and kernel. Check required ISA features against audited
  compile commands before executing binaries; do not substitute an implementation.
- Record dedicated-host/core ownership, competing services, housekeeping/IRQ
  policy and any reservation supplied by the administrator. Reconcile effective
  cgroup paths with mountinfo; a namespace-visible root mask is not proof of host
  isolation. Record any unverified controls. No reservation is created here.
- Record #689's proposed 8 GiB physical / 4 GiB available memory gate, swap/THP/
  NUMA policy, disk capacity, frequency/boost policy and usable thermal sensors.
  A setting snapshot proves neither stability nor effective worker isolation.
  Denied/missing mandatory readbacks block qualification, even if a separately
  approved correctness run can proceed. Never label this inventory qualified.

## 3. Pinned sources, manifest and toolchain

These fetch/read/build commands are allowlisted. No PR checkout tracks a moving
branch. If the service no longer advertises an exact commit, stop and obtain a
verified source bundle; do not silently use a newer PR head.

```bash
must clone git clone --no-checkout https://github.com/t81dev/t81-foundation.git repo
for pin in cda9135d36144d4de2771af174e52ac7b30a83c9 \
  ecd66137bacbce50e605cfc1466ec7d36131ba9e \
  db0e7c8b976991396eed7d14ddc1e889e9c511c2 \
  d355511e75885a4a91299f54ceb108eda65ac925; do
  must "fetch-$pin" git -C repo fetch origin "$pin"
done
must source git -C repo worktree add --detach "$SESSION/source" cda9135d36144d4de2771af174e52ac7b30a83c9
must foundation git -C repo worktree add --detach "$SESSION/foundation" db0e7c8b976991396eed7d14ddc1e889e9c511c2
must collector git -C repo worktree add --detach "$SESSION/collector" d355511e75885a4a91299f54ceb108eda65ac925
F="$SESSION/foundation/experiments/base01_b1_foundation"
R="$SESSION/collector/experiments/base01_resource_accounting"
must manifest-source git -C repo show ecd66137bacbce50e605cfc1466ec7d36131ba9e:docs/research/base01/BASE01_MANIFEST.json
must manifest-match cmp "$EVIDENCE/logs/manifest-source.stdout" "$F/phase-a-manifest.json"
printf '%s  %s\n' 1367b021f393594e4aab3bc4b52a7b76ca94ee70a49b8f0bda62aea96d46e4ea \
  "$F/phase-a-manifest.json" > "$EVIDENCE/manifest.expected.sha256"
must manifest-hash sha256sum -c "$EVIDENCE/manifest.expected.sha256"
for tree in source foundation collector; do
  must "$tree-head" git -C "$SESSION/$tree" rev-parse HEAD
  must "$tree-bytes" git -C "$SESSION/$tree" diff --exit-code HEAD --
  must "$tree-status" git -C "$SESSION/$tree" status --porcelain=v1
done
must cc-version /usr/bin/gcc --version
must cxx-version /usr/bin/g++ --version
must target /usr/bin/g++ -dumpmachine
must compiler-hashes sha256sum /usr/bin/gcc /usr/bin/g++
must linker /usr/bin/ld --version
must libc getconf GNU_LIBC_VERSION
must tools bash -ec 'git --version; cmake --version; ninja --version; python3 --version; readelf --version'
must api-bindings python3 -B -c 'import os, resource; checks={"posix_spawn":hasattr(os,"posix_spawn"),"wait4":hasattr(os,"wait4"),"getrusage":hasattr(resource,"getrusage"),"RUSAGE_SELF":hasattr(resource,"RUSAGE_SELF"),"RUSAGE_CHILDREN":hasattr(resource,"RUSAGE_CHILDREN")}; print(checks); assert all(checks.values())'
must capacity df -Pk "$SESSION"
```

These exact commands select GCC/G++ and glibc. A musl system or other toolchain
needs its actual library/allocator identification and a reviewed command variant;
do not relabel a failed `getconf` or incompatible compiler as supported. Confirm
version outputs and clean tracked trees manually. Preserve relevant build
environment/allocator choices; if injected `CFLAGS`, `CXXFLAGS`, `LDFLAGS`,
`LD_PRELOAD` or profiling/coverage settings exist, stop for profile review.
No `-march=native`, LTO, sanitizers, coverage, fast-math or profiling is added.

## 4. Build and correctness execution allowlist

All commands below run only on the assigned host after the preceding identity
gates. No `ctest`, unqualified default project build, benchmark executable,
workflow dispatch or `time` wrapper is allowed. Asio acquisition/build output
must be retained; record its resolved Git commit and dependency bytes as well.

```bash
must source-configure cmake -S "$SESSION/source" -B "$SESSION/source-build" -G Ninja \
  -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF \
  -DT81_BUILD_TESTS=ON -DT81_USE_CXX23=ON -DT81_STRICT_DETERMINISTIC_FLOAT=ON \
  -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF \
  -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_LLAMA_CPP=OFF -DT81_TRITWISE_PROFILE=OFF \
  -DT81_ENABLE_TERNARYOS=OFF -DT81_ENABLE_DPE=OFF
must asio-head git -C "$SESSION/source-build/_deps/asio-src" rev-parse HEAD
must asio-tag git -C "$SESSION/source-build/_deps/asio-src" rev-parse 'asio-1-30-2^{commit}'
must asio-match cmp "$EVIDENCE/logs/asio-head.stdout" "$EVIDENCE/logs/asio-tag.stdout"
must source-build cmake --build "$SESSION/source-build" --verbose --parallel 2 --target \
  t81_ternary_arith_test t81_vm_workload_determinism_tiers_test \
  t81_canonfs_driver_test test_resource_monitoring
must foundation-configure cmake -S "$F" -B "$SESSION/foundation-build" -G Ninja \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF \
  -DAUDITED_SOURCE="$SESSION/source" -DAUDITED_BUILD="$SESSION/source-build"
must foundation-build cmake --build "$SESSION/foundation-build" --verbose --target foundation_check
must source-built-bytes git -C "$SESSION/source" diff --exit-code HEAD --
must binary-identities sha256sum "$SESSION/foundation-build/foundation_check" \
  "$SESSION/source-build"/libt81_{vm,axion,isa,experimental,ffi,core}.a \
  "$SESSION/source-build"/{t81_ternary_arith_test,t81_vm_workload_determinism_tiers_test,t81_canonfs_driver_test,test_resource_monitoring}
must native-sections readelf -W -h -S -l "$SESSION/foundation-build/foundation_check"
# Pause here to review actual compile/link flags and required ISA against cpuinfo.
# Resolve any mismatch before invoking an executable; do not adjust flags silently.
must native-identity "$SESSION/foundation-build/foundation_check" --identity-only
for executable in t81_ternary_arith_test t81_vm_workload_determinism_tiers_test \
  t81_canonfs_driver_test test_resource_monitoring; do
  must "$executable" "$SESSION/source-build/$executable"
done
must foundation-validation python3 -B "$F/harness.py" --validate-only \
  --source "$SESSION/source" --source-build "$SESSION/source-build" \
  --build "$SESSION/foundation-build" --output "$EVIDENCE/foundation"
export BASE01_FOUNDATION_NATIVE="$SESSION/foundation-build/foundation_check"
export BASE01_FOUNDATION_RAW="$EVIDENCE/foundation/native.json.gz"
# Preserve #691 tests without unittest elapsed-duration collection.
must foundation-contracts python3 -B - "$F" <<'PY'
import sys, unittest
from types import SimpleNamespace
from unittest.mock import patch
class Log:
    def write(self, s): sys.stdout.write(s)
    def flush(self): sys.stdout.flush()
    def writeln(self, s=""): print(s)
suite = unittest.defaultTestLoader.discover(sys.argv[1], pattern="test_*.py")
result = unittest.TextTestResult(Log(), True, 2)
result.startTestRun()
with patch.object(unittest.case, "time", SimpleNamespace(perf_counter=lambda: 0), create=True):
    suite.run(result)
result.stopTestRun()
result.printErrors()
print(dict(run=result.testsRun, failures=len(result.failures), errors=len(result.errors), skipped=len(result.skipped)))
sys.exit(0 if result.wasSuccessful() and result.testsRun == 25 and not result.skipped else 1)
PY
must resource-validation python3 -B "$R/validate.py" --validate-only \
  --cc /usr/bin/gcc --build-dir "$SESSION/resource-build" --output "$EVIDENCE/resource.json"
must resource-binary sha256sum "$SESSION/resource-build/fixture"
must structured-results python3 -B - "$EVIDENCE" <<'PY'
import json, sys
from pathlib import Path
p = Path(sys.argv[1])
foundation = json.loads((p / "foundation/receipt.json").read_text())
resource = json.loads((p / "resource.json").read_text())
assert foundation["status"] == "PASS" and foundation["measurement_enabled"] is False
assert foundation["performance_samples"] == [] and foundation["vm"]["status"] == "PROVISIONAL PASS"
assert resource["status"] == "passed" and resource["measurement_authorized"] is False
assert resource["tests"] == dict(run=27, passed=27, failed=0, skipped=0)
assert resource["performance_samples"] == [] and len(resource["raw_observations"]) == 19
assert all(r["performance_samples"] == [] for r in resource["raw_observations"])
print("Correctness receipts accepted; qualification=false; measurement=false")
PY
for tree in source foundation collector; do
  must "$tree-final-bytes" git -C "$SESSION/$tree" diff --exit-code HEAD --
  must "$tree-final-status" git -C "$SESSION/$tree" status --porcelain=v1
done
```

Keep both CMake caches, compile-command files, verbose build/link logs,
generated identity header, `resource-build/build.json`, dependency identity and
all binaries/archives in the session. The pinned harness verifies audited tracked
bytes, manifest and native source identity; the collector verifies its fixed
source/executable hashes before fixtures. New Linux binary hashes are recorded,
not expected to equal historical macOS hashes. Inspect actual ELF metadata
against the foundation receipt; discrepancies are unresolved, not format equivalence.

Acceptance for this correctness packet: four executable exits 0; #691 validation
PASS and 25 contract methods passed with no skips; #692 status `passed`, 27
methods passed, zero failures/skips. Review all diagnostics, not just the shell
status. Arithmetic must retain all 1,031 results and pass the Python oracle;
CanonFS must check 4,096/65,536/1,048,576 bytes of 0x42, independent CanonHash and
repeated identity in memory only. All six VM builders retain explicit completion,
PC/R75/output/tag assertions and PROVISIONAL status pending independent review.
`test_resource_monitoring` is VM policy regression evidence, not a POSIX oracle.
The #692 fixture is the actual resource-API check: exact PID/PPID, owned reap,
exit/signal, user/system CPU, Linux raw KiB, parent/aggregate distinction and
malformed/missing/duplicate/unsupported rejection. Expect 19 retained collector
records including intentional rejections, with synthetic corruptions labeled.
No exact CPU/RSS target or resource-rise requirement applies. CPU accounting is
not elapsed time; peak RSS is not incremental memory or energy.

## 5. Stop before dynamic qualification

A passing packet establishes native correctness on the identified host/boot,
not a qualified measurement host. Keep performance sample arrays empty and
measurement/qualification eligibility false. Do not change #691's historical
resource classification or VM status to make a combined packet appear current;
describe the supplemental #692 evidence separately in the operator assessment.

Separate, explicit qualification authorization and an approved procedure are
required for timer/backend resolution probes, empty intervals, cadenced load/
thermal/frequency monitoring, quiescence waits, stability/repeatability checks,
warm-ups, pilots and batching. Two clean-build identity comparisons can be
planned without timing; they do not replace these dynamic gates. #689's proposed
thresholds and permit design do not authorize themselves. No authorization
dispatcher, permit activation, 30-trial campaign, T81 latency/throughput result,
benchmark CI, PMU, SPICE, CMP3 or H04-R03 Phase C is allowed here.

## 6. Seal raw evidence, including failures

Write `evidence/operator-assessment.txt` identifying the operator/asset/boot,
physical-host attestation, each mandatory gate's PASS/FAIL/UNVERIFIED/UNSUPPORTED
status and evidence filenames, actual test totals/skips, dependency/allocator
identity, outstanding discrepancies, and `qualification=false`. Do not invent
passes for unavailable APIs, denied controls or skipped tests. Do not regenerate
historical receipts. Retain partial output on failure; do not automatically rerun.
API-name availability above is not execution evidence; only the bounded native
`getrusage` and parent `wait4`/`getrusage` fixture tests establish actual call paths.
Then index the entire session (source/build trees, Git objects, binaries and logs):

```bash
python3 -B - "$SESSION" <<'PY'
import hashlib, json, os, sys
from pathlib import Path
root = Path(sys.argv[1])
index = root / "evidence/SHA256SUMS.json"
if index.exists(): raise SystemExit("refuse to overwrite evidence index")
files = {}
for directory, dirs, names in os.walk(root, followlinks=False):
    for name in sorted(names + [d for d in dirs if (Path(directory)/d).is_symlink()]):
        p = Path(directory) / name
        if p == index: continue
        if p.is_symlink():
            files[str(p.relative_to(root))] = {"symlink": os.readlink(p)}
        else:
            h = hashlib.sha256()
            with p.open("rb") as f:
                for chunk in iter(lambda: f.read(1024*1024), b""): h.update(chunk)
            files[str(p.relative_to(root))] = {"sha256": h.hexdigest(), "bytes": p.stat().st_size}
with index.open("x") as f:
    json.dump({"excludes": ["evidence/SHA256SUMS.json"], "entries": files}, f, indent=2, sort_keys=True)
    f.write("\n")
PY
sha256sum "$EVIDENCE/SHA256SUMS.json"
```

Do not modify the session after sealing. Preserve the index digest outside the
session in the approved operator handoff. Hashes establish byte identity, not
truthful provenance. Keep restricted raw evidence intact; publish a separately
identified redacted copy only through the maintainer's process. A reboot or
source/toolchain/firmware/control change requires a new candidate packet.

## Minimum access needed; stop here

An assigned physical Linux x86_64 asset, identified custodian/operator, exclusive
host-use window, ordinary-user shell, writable build/evidence capacity and the
toolchain/dependency access above are the smallest outstanding prerequisites.
Read access to topology, microcode, firmware/DMI, kernel, cgroup and resource
controls is needed; an administrator can supply denied readbacks with provenance
instead of granting unrestricted root. The operator needs permission to compile
and execute only the bounded correctness commands and retain their raw records.
No governor/cpuset/SMT/IRQ/cgroup writes, reboot, kernel change, PMU privilege or
performance authorization is required or granted for this onboarding packet.
