# F01 host provisioning readiness proposal

**PLANNING ONLY / HOST NOT SELECTED / PROVISIONING NOT AUTHORIZED / NOT QUALIFIED.**

Accepted supervisor: PR #809, `0afbc339288b44cc7536af6c227f957257bb8ff6`.
Accepted input proposal: PR #810, `0a369b8c7ce00979c970389e02e0da8d89d6ab0a`.
RFC candidate: `ec22a7588282d37ad027cff61c807bcf159ac2e0`.
Protocol SHA-256: `d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3`.
Exact Git/blob/SHA-256 pins are in source-pins.json. This is H07 supporting
planning, not a new canonical hypothesis or promotion of RFC-00D7. H01–H08,
Hanoi/CWEB history and all research/release holds remain unchanged.

Codex also authored the prior source/profiles. This proposal is not an independent
review or scientific validation. The user reports INPUT PROFILE ACCEPTABLE FOR
PROVISIONING REVIEW; that is not permission to acquire a host, supply/build an
interpreter, provision it, activate a lock, qualify Linux or run A–F.

## Proposed hosts and primary selection

| Candidate | Concrete proposed configuration | Status and admission limits |
|---|---|---|
| X1 primary | AWS EC2 m7i.large, us-east-1, Canonical Ubuntu24.04 LTS server amd64 release image, dedicated disposable instance, EBS scratch | Proposed only; exact AMI/serial/snapshots/account/AZ/instance/kernel/manager unknown |
| A1 alternate | AWS EC2 m7g.large, us-east-1, Canonical Ubuntu24.04 LTS server arm64 release image, same isolation intent | Proposed only; separate aarch64 artifact and host evidence required; no cross-host inference |
| CI context only | Repository workflows name ubuntu-24.04 and ubuntu-24.04-arm | Labels identify workflow intent, not observed cgroup delegation, namespaces, timers or a usable provisioning host |

AWS documents both selected instance sizes as2vCPU/8GiB. This is host capacity,
not a job resource allowance, exclusive CPU guarantee or performance claim.
One logical CPU and the limits below apply to custom provisioning work. ARM64
means AArch64, not ARMv9 qualification. No AWS API, account discovery, purchase,
image resolution or live host inventory occurred for this proposal.

Select X1 first to bound the initial operational matrix to one accepted x86_64
profile and one native build/loader closure. The existing wheel is already pinned;
this is a scope choice, not evidence that x86_64 is more correct, compatible or
faster. A1 is an independently approved second campaign, never an automatic
fallback or a fresh allowance after X1 fails. No two-host campaign is authorized.

Candidate OS is deliberately a full VM with an administrator-controlled system
manager, rather than an unobserved container/delegated CI environment. Ubuntu24.04
is a proposed ABI/administration baseline, not proof of any kernel, cgroup-v2,
memfd or manager feature. The exact image must be frozen before admission; do
not provision from a moving latest/current alias. For the default AWS partition,
Canonical documents owner099720109477. Verify exact owner/image/snapshot/serial
in a later approved host-selection step; no image ID is invented here. Do not
silently substitute a Marketplace owner, another release or another architecture.

The administrator must later attest kernel/boot/architecture, libc, time/mount/
cgroup namespaces, cgroup2 mounts/controller delegation, CPU mask and manager
capabilities by separately authorized read-only inventory. Resource stress,
kill/overshoot or memfd probes are qualification, not this inventory. Missing
features or an unreachable host block, rather than trigger sudo/global repairs.

## CPython provider/build strategy

Preferred strategy: obtain a separately commissioned, reviewed native CPython
3.14.3 artifact from an accountable controlled builder for the same frozen OS
image/architecture, installed into the accepted fixed layout. No such provider
artifact or builder is currently selected, available or approved. This strategy
is a proposed prerequisite, not an existing reproducible distribution claim.

Build inputs must include the verified source tarball SHA-256
`a97d5549e9ad81fe17159ed02c68774ad5d266c72f8d9a0b5a9c371fe85d902b`, exact
compiler/linker/toolchain and base image, configure/build/install recipes, complete
patch set (explicit empty list if none), generated configuration, provenance and
artifact signature/approval. Require CPython3.14.3, cp314/GIL, non-debug,
JIT/tail-call disabled, lib layout, and the exact prefix relationships in #810.
No compiler flags are guessed to stand in for an observed build recipe.

Base=/opt/t81-f01/cpython-3.14.3; venv=/opt/t81-f01/venv. The actual lexical venv
bin/python3.14 must resolve to base/bin/python3.14; home and base interpreter
paths must agree. Reject relocated/multi-prefix/lib64/framework/embedded/ZIP
layouts that do not satisfy the accepted interpreter_boundary. No broadening
of that function or relocation patch is proposed. Provider/build records and
actual content hashes must establish the relationship, not a version banner.

Rationale: a controlled native build can be specified for those exact paths and
its architecture-specific loader dependencies. Generic distro Python, a moving
container tag or a relocatable standalone tarball cannot be presumed to match.
No alternative provider is accepted merely because it reports3.14.3. The old
patch release's suitability/authenticity requires explicit human review; no
upgrade or unrecorded vendor patch is a provisioning fallback.

A builder run is a separate proposed project with its own reviewed resource and
security budget, not an unmetered phase of the60s operation below. This plan
neither supplies a build script nor authorizes compilation or builder acquisition.
Until its immutable outputs, signatures and exhaustive manifests are accepted,
provisioning readiness is BLOCKED_PROVIDER_ARTIFACT. Independent bit-identical
build reproduction is a distinct claim requiring its own receipt; simple
retrieval/hash matching is not that claim.

## Preconditions and operation boundary

Before the proposed operation, the human must approve exact host, provider
artifact, installer identity/version/hash, execution scopes, source pins and
resource/storage limits. All such unobserved identities remain null in the
templates. A digest or an authorized JSON field cannot replace the decision.
No command/action below is permission to perform it now.

An approved administrator supplies a fresh nonproduction host and a dedicated
unprivileged account, empty /opt/t81-f01 destination, read-only signed inputs,
quota-controlled scratch and an independent already-enforced bounded manager
unit. No existing interpreter, user workload, active lock or production path
may be overwritten. Administrative setup, infrastructure acquisition, input
transfer and building require their own explicit scopes/budgets before this
operation; none can be hidden in its counters or claimed performed here.

The operation itself starts before the first custom preflight helper. All its
custom processes, hashing/install/diagnostic helpers and cleanup remain in one
fresh aggregate accounting domain with empty internal cgroup and bounded leaf
placement. Do not use the experimental supervisor to provision its prerequisites.
A separately reviewed host-admin manager/launcher must supply limits and outer
termination; no new implementation is delivered here. If that prerequisite is
not demonstrable, STOP before provisioning. No custom observer escapes accounting.

Offline phases (full details in INVENTORY_AND_OFFLINE.md):

1. Verify immutable inputs, exact architecture/profile/hashes/signatures and
   the bounded source export against its pinned member manifest. Reject unsigned/unreviewed/missing provider or installer,
   metadata mismatch or any input change. Network is disabled before entry.
2. Safely materialize the approved binary payload at its final fixed prefix;
   reject archive traversal/symlink escapes/hooks and verify provider member
   inventories. No interpreter build, apt, curl, git fetch or source fallback.
3. Under a specifically authorized future interpreter-execution scope, create
   a symlink venv without pip and materialize exactly the reviewed pyvenv.cfg
   bytes. Invoke a separately pinned offline installer outside the runtime venv
   to install exactly five reviewed wheel artifacts, without resolution,
   dependency fetching, source builds, extras, hooks or bytecode generation.
4. Stop writers and set owned payload files/directories read-only; hash complete identities and
   static ELF/library closure. Distinguish candidate loader resolution from
   observed loads. Do not silently execute package code to settle that question.
5. If separately included in that future authorization, run only an audited
   metadata-only diagnostic of the pinned interpreter_boundary helper in both
   launch profiles, never bootstrap main, supervisor or run.py. Seal receipts,
   clean up workers and mark the payload quarantined. No active lock is changed.

The currently permitted task executes none of those phases. Native import or
load tests/resource probes remain separately authorized qualification activities.

## Proposed operation resources and STOP behavior

Machine-readable ceilings and phase reservations are in resources.json. One
attempt, sequential phases, no retry, fresh host or architecture fallback:

- 60s whole-operation wall,50s aggregate CPU, one logical CPU,256MiB aggregate
 memory, zero swap; one active128MiB helper and a64MiB controller leaf.
- Inner wall/CPU guard58s/48s; bounded finalization before59s/49s. Outer manager
 must cover activation, execution and shutdown within60s; the proposed profile
 runtime<=55s, activation/stop<=1s and no extensions/restarts needs host evidence.
- 25MiB total evidence output, including raw logs/manifests, summaries, receipts,
 temporary/partial evidence and final hashes. No unbounded output or compression
 trick hides a larger retained raw record. Reserve cleanup/terminal space first.
- Separately proposed provisioning storage: read-only inputs<=128MiB, installed
 payload<=512MiB, staging<=256MiB, evidence<=25MiB; combined<=921MiB, enforced
 within a1GiB quota-controlled operation volume. These are distinct provisioning
 input/payload outputs requiring approval, not a redefinition or enlargement of
 the F01 experiment's25MiB evidence cap. OS/image bytes are pre-existing approved
 infrastructure, not free custom build/download/install work inside the campaign.

Phase reservations sum50s wall/45s CPU plus10s wall/5s CPU for finalization and
cleanup. They are caps, not estimates or measurements of fit. Any missing quota,
exhausted budget, record limit, hash mutation, unsupported feature or failed
phase ends the single attempt; no automatic retries, source build, alternative
host or increase is allowed. If materialization/hash/installation cannot fit,
retain the bounded incomplete receipt and request an independently reviewed
resource/plan revision. Do not consume F01's experiment allowance.

Cleanup: stop accepting work, close input channels, terminate verified owned
process identities/groups and use the dedicated job subtree's verified cgroup
kill via the existing manager. Bound waits/reaping and require population0 and
no adopted/direct children. Never signal unrelated processes or kill the observer
from its own aggregate as normal cleanup. Manager-wide termination is last resort;
missing final records are incomplete. Keep accepted inputs read-only. Quarantine
partial payload with no reuse/execution authority; deletion/disposal of this
owned fresh volume must be explicit in the provisioning approval. If deletion
cannot fit cleanup, deny access and have the administrator dispose of the whole
disposable instance/volume under that approval. No unbounded rm traversal or
post-budget retry is permitted. Do not delete successfully retained evidence.

## Approvals and remaining blockers

| Decision | Required scope | Does not authorize |
|---|---|---|
| H0 host selection/acquisition/admin setup | Exact account/region/image/instance, spend/TTL, disposable storage, identity/readonly inventory and dedicated account/control setup | Build, install, qualify or A–F |
| H1 provider/build artifact acceptance | Accountable builder, source/patch suitability, separate build budget if needed, toolchain/recipe/image and signed immutable binaries/manifests | Target provisioning or qualification |
| H2 one bounded offline provisioning attempt | This plan hash, exact host/boot, inputs/installer, all limits, venv/installer interpreter execution and any separately reviewed metadata-only diagnostic, cleanup/disposal | Native imports, stress probes, lock activation, qualification, A–F |
| H3 independent provisioning receipt review | Raw hashes/observations/limits and scope; preserve negatives/incomplete evidence | Clear blocker or infer host qualification |
| H4 active re-lock change | Necessary target evidence, exact derived source revision, exhaustive installed lock and regenerated identities; explicit permission to change linux_relock_required | Probes or A–F |
| H5 Linux qualification | Separate exact-source/host authorization for bounded enforcement/native-compatibility probes | Contract experiments |
| H6 one A–F run | Separately accepted qualification, immutable resulting source/lock, explicit one-run record and required human RFC §4.1 passage review | Repeat run, normative promotion, production or merge |

H0–H6 name distinct decision scopes, not an assumed strictly linear schedule.
If native resolution requires runtime evidence before H4 can be accepted, request
an explicitly bounded preliminary compatibility qualification under H5 with its
own reviewed non-A–F probe source and budget. Do not clear the blocker merely to
run the supervisor as that probe. A final H5 receipt after lock integration must
name the actual new source/host identities; preliminary observations cannot be
silently recycled as final qualification or execution authority.

Source acceptance/input acceptance are already reported decisions, not H0–H6.
Human source-passage review is distinct from every software/hash check. The
sequence can require revisions when qualification finds an environment defect;
no hypothetical future SHA is used as an approval target. No fields are populated
on behalf of a human. Every integration/provisioning/lock change yields new exact
identities and requires review rather than recycling #809/#810 receipts.

Remaining blockers: no actual host/AMI/account/boot, kernel/controller/manager
inventory, provider binary/build/stdlib/native closure, installer pin or approval;
no measured resource fit, runtime ABI resolution, source-passage review or
provisioning/qualification evidence. linux_relock_required=true remains intact.
All F01 semantics, schemas, F254/256 and F288-v1 status and governance holds remain.

Official candidate information, accessed2026-11-16 (not target-host evidence):
[AWS instance specifications](https://docs.aws.amazon.com/ec2/latest/instancetypes/gp.html),
[Canonical image selection](https://ubuntu.com/aws/docs/aws-how-to/instances/find-ubuntu-images/),
[systemd255 API/version history](https://www.freedesktop.org/software/systemd/man/255/org.freedesktop.systemd1.html).
Documentation of DelegateSubgroup is not proof that the chosen image/manager
implements the required complete topology/deadline profile. No current AMI,
price, capacity, kernel version or host feature is inferred from those pages.
