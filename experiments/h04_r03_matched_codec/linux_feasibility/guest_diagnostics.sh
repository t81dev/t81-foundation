#!/bin/sh
# Run only in a disposable diagnostic guest or an explicitly prepared Linux host.
# Creates one new leaf, touches synthetic data, runs no codec arms, records no timings.
set -eu
probe_root=/tmp/h04-feasibility
mkdir -p "$probe_root"
cd "$probe_root"
printf '\nH04_FEASIBILITY_BEGIN\n'
uname -a
cat /etc/alpine-release
id
cat /proc/cmdline
cat /proc/self/status
cat /proc/swaps
cat /proc/meminfo
cat /proc/cgroups
cat /proc/self/mountinfo
cc --version
ld --version
python3 --version
apk list --installed
sha256sum containment_probe.c native_scratch_probe.py
cc -O2 -std=c11 -Wall -Wextra -Werror -frandom-seed=h04-feasibility -ffile-prefix-map=/tmp/h04-feasibility=. -Wl,--build-id=none containment_probe.c -o containment_probe
cc -O2 -std=c11 -Wall -Wextra -Werror -frandom-seed=h04-feasibility -ffile-prefix-map=/tmp/h04-feasibility=. -Wl,--build-id=none containment_probe.c -o containment_probe.rebuild
sha256sum containment_probe containment_probe.rebuild
cmp containment_probe containment_probe.rebuild
printf 'NATIVE_BUILD_IDENTICAL=1\n'
readelf -lW containment_probe
sha256sum /usr/bin/gcc /usr/bin/ld /lib/ld-musl-x86_64.so.1 "$(cc -print-prog-name=cc1)" "$(readlink -f /usr/bin/python3)"
if [ ! -f /sys/fs/cgroup/cgroup.controllers ]; then
    mount -t cgroup2 none /sys/fs/cgroup
fi
cat /sys/fs/cgroup/cgroup.controllers
printf '+memory +pids\n' > /sys/fs/cgroup/cgroup.subtree_control
cg=/sys/fs/cgroup/h04-feasibility
# A pre-existing leaf is a failure, never reused or silently reset.
mkdir "$cg"
printf '33554432\n' > "$cg/memory.max"
printf '0\n' > "$cg/memory.swap.max"
printf '1\n' > "$cg/memory.oom.group"
printf '1\n' > "$cg/pids.max"
printf 'CGROUP_BEFORE\n'
cat "$cg/memory.max" "$cg/memory.current" "$cg/memory.peak" "$cg/memory.events.local" "$cg/cgroup.type" "$cg/cgroup.procs"
./containment_probe --rss-limit
./containment_probe "$cg"
printf 'CGROUP_AFTER\n'
cat "$cg/memory.max" "$cg/memory.current" "$cg/memory.peak" "$cg/memory.events.local" "$cg/cgroup.events" "$cg/cgroup.procs"
python3 native_scratch_probe.py
rmdir "$cg"
printf 'H04_FEASIBILITY_DONE\n'
