# Proposed inventory and offline materialization procedure

Future procedure only. No commands supplied here were executed and no installer,
probe, build, diagnostic or runtime service is implemented by this proposal.
Use the exact source/architecture profile pins in source-pins.json; a different
path/version/provider needs a reviewed revision, not command-line overrides.

## Immutable inventory method

Before writing, establish the exact provider and installer manifests, archive
hashes, provenance/signature review and admissible paths. After installation,
stop all writers and set the dedicated account-owned payload files/directories
read-only within the operation. This is trusted-host writer exclusion, not
immutability against a malicious owner. A stronger kernel read-only export, if
required before activation, needs a separately approved administrator arrangement;
never launch a privileged/out-of-budget remount helper to fabricate it. Inventory only those fixed roots; do not recurse
through arbitrary symlinks or /proc. No live package updater or installer may
remain. Verify input and output stability across inventory and final sealing.

Use streamed SHA-256, buffer<=128KiB; inventory regular files with no-follow
opens, fstat before/after, exact byte count and path identity checks. Reject
mutating/replaced files, unreadable members, devices/FIFOs/sockets, path traversal,
unknown hard links or files outside the approved roots. Traversal and record
limits are fixed in resources.json. No filesystem content is executed to hash it.

Each content record includes approved root ID, lossless relative path bytes
(UTF-8 plus hex representation when necessary), type, size, mode and SHA-256.
Record symlinks separately with raw target bytes, lexical and resolved destination,
every hop and bounded cycle/escape detection. Record a content/symlink-chain
manifest hash. Device/inode/mtime/ctime/uid/gid and before/after identity belong
in the operational receipt; do not confuse those machine-specific observations
with a cross-install reproducible content hash. Count every materialized path
conservatively against storage and record limits, even when hard-linked.

Inventories required:

| Surface | Required evidence | Cannot substitute |
|---|---|---|
| Interpreter | Exact /opt/t81-f01/venv/bin/python3.14 lexical chain, base executable bytes, ELF architecture/build ID, interpreter/shared-lib identity, source/build/config/patch/provider records | Version banner, CPython source digest or macOS executable |
| Stdlib | Exhaustive base/lib/python3.14 tree, library file hashes, frozen-runtime-containing executable/libpython, generated sysconfig data/config headers and install manifest | The2,405 source Lib/ entries in #810 |
| lib-dynload | Every installed extension hash/ELF identity and dependency information, mode and location | Directory name or assumed SOABI |
| Venv | Exact pyvenv.cfg byte hash, both discovery-path checks, executable symlink chain, prefix/base mapping and complete venv content | Checking one guessed config path or environment variable |
| Packages | Exact five installed distributions, complete file inventories including dist-info/METADATA/WHEEL/RECORD, installer-generated files and all transformations | Wheel RECORD or versions alone |
| Native libraries | Exact loader, libc, libgcc, libpython if linked, and recursive dependencies of executable and all installed stdlib/third-party extensions; paths, symlinks, hashes, ELF IDs and source/provider packages | Matching SONAME, tag, minimum-glibc string or unobserved loader resolution |
| Host/control | Exact instance/AMI/snapshots/account/boot/kernel/architecture/manager/control configuration and administrator receipt | Cloud product page, workflow label or host capacity |

For native closure, statically parse ELF headers, program/section tables,
DT_NEEDED/RPATH/RUNPATH/version requirements and loader configuration/cache/hwcaps
rules using a separately reviewed bounded tool. Pin the parser itself. Inspect
symlinks and every candidate selected library; reject ambiguity/unrecorded paths.
No ldd, dlopen, package/native import or test execution is implied. Undefined weak
symbols/fallbacks and CPU-feature dispatch need explicit review, not a claim that
all symbol names are eager requirements. A static candidate-resolution table
must mark its entries not_runtime_observed. Capture possible hardware-capability
alternatives rather than pretending only one filename exists.

If exact runtime selection cannot be established without native imports, retain
that uncertainty and block the associated lock/ABI claim until a separately
approved qualification step observes it. Do not fabricate a resolved-runtime
receipt from static ELF metadata. Metadata-only interpreter diagnostic maps, if
authorized, cover only the libraries it actually loaded, not rpds or other
unimported extensions. Retain the per-root scope of every manifest.

## Offline provisioning algorithm

Admission requires an exact input bundle, size/member inventory and hash list:
provider interpreter payload, five architecture-specific wheel files, #810 profile/
requirements/config bytes, bounded source-file export with Git blob identity
proofs (no full clone/.git or network fetch), pinned external installer and reviewed
inspection/cleanup tooling. No artifact is selected from a remote index, PATH,
latest alias or version-only lookup. Download/transfer is a separate approved
preparation; the operation has no network access, DNS or credentials. Stop if
inputs/signatures are unavailable or the source/host identity changed.

1. The already approved manager starts all custom work in a fresh owned aggregate
   with separate controller/helper leaves and resource/accounting origins before
   the first custom instruction. Preflight reads only; no repair of limits.
2. Verify every archive/member/type/size/hash and safe destination before release.
   Permit no absolute archive destinations, parent traversal, symlink escape,
   setuid/setgid capability, device node, startup hook or unreviewed install script.
   A provider artifact built for the fixed prefix is materialized at that prefix;
   staging/relocation must not silently rewrite compiled path assumptions.
3. If the later H2 record explicitly allows it, invoke only the accepted provider
   interpreter's standard venv facility in isolated/no-site/no-bytecode mode,
   without pip/ensurepip or copied interpreter binaries. Use symlinks to the
   pinned base executable. Normalize config to the exact four-line #810 proposed
   bytes by an explicitly reviewed data write, then hash/read back. Reject config
   alternatives, symlinked configs or changed prefix/home. No existing venv reuse.
4. The separately pinned installer stays outside the runtime venv. Its review
   must establish wheel-only behavior, target-prefix handling under CPython3.14.3
   and the lack of implicit imports/hooks. If pip is chosen, the proposed option
   contract is: isolated configuration, no index/network, explicit reviewed local
   wheel directory, --require-hashes, --no-deps, --only-binary=:all:, --no-compile,
   --no-cache-dir and disabled version checks. Tool/version/hash and exact argv
   remain required unfilled inputs; these flags alone are not installer approval.
   No installer package is installed into the runtime. No source/build fallback,
   extra, .pth/customization, resolver-added dependency or opportunistic update.
5. Install exactly the selected five wheels with the architecture hash requirements
   from #810. x86_64 and ARM64 rpds wheels are distinct; do not choose by filename
   resemblance or copy one architecture's receipt to another. Keep wheel bytes
   read-only and compare their hashes before/after. Record all generated dist-info
   and RECORD/INSTALLER transformations rather than equating wheel and installed
   manifests. Reject unexpected files. Do not compile bytecode or import packages
   as a post-install test; wheel installation is not runtime ABI qualification.
6. Stop installer/helpers; freeze and inventory all output and native closure.
   Any mutation, missing file, extra distribution, capacity exhaustion, missing required static
   identity or unsupported layout prevents a completed static-inventory receipt.
   Unobserved native runtime selection remains a separate activation/ABI blocker,
   even when the scoped static inventories are complete.
   Preserve the intermediate raw manifests, subject to evidence bounds.
7. Only if expressly approved, perform the narrowly scoped diagnostic below,
   recheck environment/source stability and finalize evidence before cleanup.
   Otherwise report diagnostic_not_authorized, not a passing contract check.

The installer is not pinned yet and no provisioning script is supplied. An
installer/hashing/cleanup implementation must receive source review and exact
identities before H2. Missing tooling is a blocker, not license to invoke an
unreviewed system pip or to implement it during the approved operation.

## Check against the accepted interpreter boundary

First compare static path/config/content evidence with #809's source predicates
and #810's exact layout. Required relationships are: real venv distinct from base;
prefix/exec-prefix and base pairs agree; lexical executable belongs to venv/bin;
resolved executable equals _base_executable; a single nonsymlinked config from
the two discovery locations has explicit include-system-site-packages=false and
home resolving to the base executable directory; purelib=platlib inside venv;
stdlib/DESTSHARED inside base; exactly the three initial standard-library paths
and nonexistent archive placeholder; no .pth, ._pth or customization hooks.
Static filesystem inspection alone cannot establish sys.prefix/sysconfig/sys.path
of a particular interpreter invocation or that the binary actually runs.

A future metadata-only diagnostic must be separately source-reviewed and hash-
pinned, have no experimental imports/dispatch, load only the stdlib-only helper
from the pinned bootstrap source with its main guard inactive, and call only
interpreter_boundary. Do not invoke supervisor, runner, bootstrap main or run.py.
Define two fresh bounded diagnostic invocations, sharing the same provisioning
allowance: isolated -I -S -B for runner=False, and -P -S -B with the exact minimal
runner environment for runner=True. The latter's required F01_QUALIFIED_CHILD
marker merely satisfies that predicate; it is not execution authorization and
must not be propagated to another process. No model functions or oracle run.
The exact diagnostic source/argv is a missing input, not supplied runnable code.

Record the exact source-export root and complete returned path/config profile, binary/config/source hashes,
exit/exception and scoped loaded-file observations. Compare with proposed bytes
and profiles; any mismatch is BLOCKED, never a reason to edit the predicate.
This helper's passing output would establish only that invocation's import/path
compatibility, not cgroup/RLIMIT/memfd race/resource enforcement, package ABI,
independent scientific proof or Linux qualification. Later qualification remains
separate even if these two diagnostics and installation succeed.

## Evidence sealing and disposal

Prepare exclusive-create bounded JSONL manifests and receipt/log files with
resource reservations before writing. Byte/record limits apply to partial files,
stdout/stderr and seal metadata. Reject duplicate/unsafe paths and unknown record
keys in later reviewed tooling. Do not truncate away a negative record to make a
manifest pass. Record original stop reason separately from cleanup/write errors.

After workers are stopped and payload frozen, produce final input/output hashes
and before/after counters, plus a hash inventory over successfully written
records. Verify readback within the budget. A missing/unreadable/incomplete seal
or exhausted evidence reservation cannot produce a complete receipt. Do not
fabricate successful persistence after OOM, kill, capacity or deadline failure.
The independent manager's stop record may remain the only evidence of death;
its absence is also a gap, not success. No custom out-of-budget observer.

Preserve successful raw evidence in the25MiB budget. Payload is quarantined with
no activation/execution right, even on a successful inventory. Failed/partial
payload is unusable; H2 must include administrator disposal of the owned volume
if bounded cleanup cannot safely remove it. Preserve accepted inputs and evidence;
never recursively delete a broader /opt tree or unrelated account/cgroup. Record
orphan/population and quarantine/disposal status separately. If it cannot be
observed, report unknown/incomplete and block reuse, lock activation and claims.
