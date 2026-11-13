# Linux x86_64 qualification checklist — not completed

**Status: NOT TESTED on a dedicated physical host.** This is a list of required
receipts, not a claim that a prospective host passes. The emulated guest used
for the counterexamples is not a substitute. The F2/F3 blockers in
[REPORT.md](REPORT.md) remain blockers even if every configuration string below
matches. Do not run a codec campaign to populate this checklist.

| Area | Required evidence before qualification | Important limit |
| --- | --- | --- |
| Identity and tenancy | Machine/CPU model, microcode, physical host/virtualization identity, exclusive allocation, boot ID, administrator attestation and exact image/kernel package identities | VM architecture strings do not prove physical exclusivity or host power control |
| Kernel | One approved pinned Linux x86_64 LTS release, boot parameters, kernel binary/config/hash, source/patch provenance and relevant security policy | Version alone does not prove syscall/controller availability or kernel build correspondence |
| Feature prerequisites | Successful isolated clone3/CLONE_INTO_CGROUP feature diagnostic; CONFIG_CGROUPS and v2 filesystem; MEMCG/pids and required controls; actual memory.peak/event APIs | Flag needs Linux >=5.7; each distribution/backport/seccomp/LSM configuration still needs checking |
| Delegation | Actual cgroup2 mount and namespace identities; process-free controller parent; separate manager/worker leaves; writable target/common ancestor for launcher; valid domain/type and no child controllers in worker leaf | Root success is not proof of an unprivileged production delegation |
| Before-start containment | Source-reviewed launch path, target directory FD identity and limits already installed, first marked child and parent observations, exec/startup continuity, no migration fallback | Cannot retroactively contain launcher allocations or reassign all inherited/shared charges |
| Privilege/isolation | Trusted launcher boundary; worker UID/capabilities, syscall/LSM policy, cgroup-control visibility/write restrictions and inherited descriptors; non-escape evidence for the approved threat model | Cgroup placement itself is not a hostile-code sandbox; do not leave an untrusted privileged worker able to undo controls |
| Exact RSS | Demonstrated continuous process-RSS mechanism with admitted mapping/ownership model, shared/fork/exec/hugepage cases and transient breach/refusal evidence | No such conforming backend is demonstrated here. RLIMIT_RSS, memcg usage, sampling and peaks cannot supply it; any stronger AS-bound candidate must prove initial/mapping coverage and unchanged-worker fit rather than silently replace RSS |
| Scratch | Complete native/interpreter/stack/TLS/allocator-internal accounting model and synchronous reservation/refusal, with aggregate peak-live bound <=64 KiB and retained failures | Cover realloc coexistence, native bypasses, instrumentation storage, threads/reentrancy and arena reuse; partial tracing is insufficient |
| Resource envelope | Unchanged 32 MiB RSS and 64 KiB scratch; preregistered maximum host allocation (16 GB) and approved exact byte conventions; include interpreter/native tables and setup footprint | A configuration that exceeds the cap is blocked; do not increase the cap to make it run |
| Swap and memory topology | /proc/swaps, swap controls including cgroup swap, RAM/NUMA allocation, page size/THP/hugepage and memory policy settings | Disabled swap does not make cgroup usage equal RSS or eliminate shared backing |
| CPU topology | >=4 physical cores, package/core/thread sibling map, chosen worker and housekeeping CPUs | Virtual CPU count or affinity mask does not prove physical core count |
| Governor/frequency | Driver and HWP/CPPC mode, approved performance governor, min/max policy, actual frequency/power/thermal-control evidence and persistent configuration identity | A governor name does not prove constant physical frequency; thermal throttling remains possible |
| Turbo | Verified vendor-specific BIOS/kernel/driver configuration and supported readback for disabled boost | Missing/masked interface is unknown, not disabled |
| Affinity | Recorded launcher/worker/all-thread masks and permitted cpuset, isolation/housekeeping/IRQ/RCU policy; verify initialization/exec placement and no migrations | taskset does not remove interrupts, NMIs or every context switch |
| SMT | Sibling map, disabled SMT or reviewed exclusive sibling allocation, actual control/readback and contention policy | An emulated one-vCPU smt/active=0 is not physical SMT evidence |
| Cache topology | Exact L1/L2/L3 sizes, types, line sizes, sharing domains and NUMA locality | sysfs topology is configuration evidence, not cache-state proof |
| Cache reset | Approved operation, target addresses/domains/hierarchy, fencing, placement between independent processes, cost placement and interference controls; physical-host demonstration | Define the protocol's scope before running. New processes, drop_caches and memory-order fences do not prove CPU cache reset |
| Retention | Exact ownership/alias model for raw/mapped/native buffers, release events, remaining references and logical-vs-resident distinction | Weak references do not prove allocator pages or physical cache lines were reclaimed |
| Reclamation | Process birth/exit identities, parent reaping, address-space lifecycle, cgroup residuals, private/shared/pinned ownership and relevant physical-page semantics | Reaped process, empty cgroup, zero requested live bytes, released arenas and physical reclamation are distinct properties |
| Compiler/build | Absolute tools, versions, target triplets, executable/source hashes, flags, linker, sysroot, libc/allocator, loader, environment and build-directory conventions | Retain complete build inputs; two equal builds in one environment do not establish cross-host reproducibility |
| Reproducible native artifacts | Byte-identical rebuilds in a separately reconstructed pinned environment, binary hashes and ELF interpreter/linked dependency closure | Retain a failure instead of replacing a differing artifact with a passing build |
| Python/dependencies | Exact Python build/ABI/GIL/allocator options, shared libraries/extensions, loaded module graph, pinned requirements/source archives/build tools and binary hashes | Native libraries can allocate outside Python's memory manager; import/build metadata is not full allocation coverage |
| Existing checksum contract | Verify the frozen xxhash 3.5.0/library 0.8.2 source pin, seed 0, known answers and independently reproduce witness cb66754878debc38 under the reviewed output convention | Prior macOS correctness validation is not Linux host qualification; no codec measurements are needed |
| Artifacts/failures | Independent parent evidence location and permissions; source/worker/process birth identity, complete schedule/order, stdout/stderr, kill/timeout/OOM causes, durable append/recovery and corruption detection | Hashes establish identity, not authentic execution; test parent/worker failures on the actual filesystem |
| Independence | Named reviewer, author relationship, exact commands/revisions, limits, failures and independent reproduction receipts | This investigation is same-agent author work, not independent reproduction |
| Authorization | Human review of unresolved contracts and host receipts, explicit bounded execution authorization and separately reviewed freeze lift | Main merges, JSON authorized flags, diagnostics and successful tests do not authorize Phase C |

No row is marked satisfied for a dedicated host by this file. Collect receipts
read-only first. Any host-wide setting changes, new instrumentation/kernel work,
resource allocation or eventual measurement campaign require their applicable
human authorization. This investigation makes none of those production changes.
