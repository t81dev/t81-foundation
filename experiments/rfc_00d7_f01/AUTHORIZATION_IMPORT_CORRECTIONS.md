# PR #809 targeted authorization/import correction

Source preparation only; no supervisor/bootstrap execution, probes, fixture or
proof checking, A–F execution, package installation, qualification or merge.
Review baseline: `b0ab7f31eee897634aa0a19d84c7ac504dc6331c`.
PR base remains `2f2bc47ffc9441a060f3a1cfdf4ec893c6699d0e`.
Codex authored both versions. These source checks are not independent review or
scientific proof; this candidate remains for independent implementation review.

## Byte-level authorization contract

The original path is now an input to exactly one bounded capture, never the
bootstrap or final runner interface. Capture opens with NOFOLLOW/NONBLOCK,
requires a regular file of1–8192 bytes, reads at offset0 with an8193-byte bound,
and checks device/inode/size/mtime/ctime before/after and against the path's
lstat. Symlinks, detectable capture-time mutation/substitution, short reads or
writes fail closed. No claim is made against a malicious privileged host.

The parent creates a private Linux memfd before the fork so the waiting bootstrap
inherits its descriptor. After READY and before G1, the parent copies the raw
bytes, applies F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL, and compares the
sealed readback to the captured bytes. Missing memfd/sealing support or any
sealing/readback failure blocks GO. The at-most8KiB backing and all raw copies
remain within the existing cgroup/accounting ceilings; no state allowance grows.
The stub does not read the unsealed object or enter experimental code before GO.

G1 checks seals, regular-file type and bounded exact length, reads with pread at
0, parses those raw bytes with duplicate-key rejection, and validates the same
existing authorization schema, identities, scope and interpretation policy.
It returns the raw bytes and their SHA-256/size/device/inode binding. No JSON
reserialization occurs between validation and binding.

**Window1, G1 to GO:** GO is constructed from G1's binding, with the unchanged
256-byte packet ceiling. It never hashes or reads the original path again.
There is no mutable pathname used as downstream authority. Changing the original
input after capture cannot change the validated snapshot or later runner input;
it is deliberately no longer consulted. This is snapshot semantics, not a claim
that subsequent changes to an unused path are monitored or rejected.

**Window2, bootstrap to final consumption:** the bootstrap requires the GO key
set and true GO flag, rechecks the inherited descriptor's four seals, exact
length, hash and device/inode against GO, and passes that same descriptor through
exec. The final run.py repeats those checks against the binding passed by the
bootstrap, parses the verified bytes in memory, closes the descriptor and uses
that object for the unchanged experiment loop. There is no pathname fallback.
A substituted descriptor, missing seals, truncated/oversized data or mismatched
identity/hash fails before any model imports. Neither a matching JSON object nor
a matching reserialization can substitute for the original validated byte string.

The existing supporting authorization-record.json now contains the exact
captured byte string, not a reserialized copy. Supervisor supporting evidence
records its binding. Existing result/evidence schemas are unchanged, as are
exclusive writes, artifact reservations and incomplete-finalization semantics.
Hashes and inherited descriptors establish content identity, not human approval
or truth. Receipt authenticity remains the operator's responsibility.

## Interpreter/import boundary

The supervisor and bootstrap use -I -S -B. The final runner uses -P -S -B with
the existing minimal environment (PATH, hash seed0, no bytecode, no user site,
qualified-child marker); only CPython's optional LC_CTYPE coercion addition is
allowed. -P removes automatic script/current-directory insertion; -S prevents
site, .pth and customization startup. -I is not used for the final runner because
it ignores PYTHONHASHSEED; preserving hash seed0 remains explicit. No PYTHONPATH,
PYTHONHOME, user-base or launcher environment is accepted for final entry.

The stdlib-only interpreter_boundary helper runs in G1, the gated stub after GO,
and final run.py before model/dependency imports. It requires Linux CPython,
no site initialization, safe-path mode, assertions enabled, and the appropriate
isolated or exact runner environment. It verifies:

- sys.prefix/exec_prefix agree on a real venv distinct from base_prefix, with a
  matching base_exec_prefix; lexical sys.executable belongs to that venv/bin and
  resolves to sys._base_executable.
- Both CPython configuration search candidates, beside the lexical executable
  and in its parent, are considered. Exactly one must exist, be nonsymlinked and
  identify prefix/pyvenv.cfg. Duplicate/oversized/malformed configuration fails;
  include-system-site-packages must explicitly equal false, and home must match
  the resolved base executable's directory.
- purelib and platlib resolve to the same existing directory within the venv;
  stdlib and DESTSHARED belong to the resolved base prefix.
- The entire initial sys.path matches the strict reviewed Linux three-entry
  stdlib archive-placeholder/stdlib/dynamic-library profile. Existing archives,
  additional directories, system/user site paths or other layouts are rejected.
- Interpreter/base executable/library-directory *._pth overrides and .pth,
  sitecustomize.py or usercustomize.py in every permitted import directory are
  rejected. Unknown layouts are unsupported, never silently accepted.

The final runner then installs only the checked project root, checked stdlib
paths and checked venv library as its search path. It imports the experimental
modules only after interpreter and sealed-authorization checks. The future
metadata-check subprocess also uses -I -S -B; its source has an explicit Linux
no-site setup branch to add only the checked venv library before jsonschema.
The preparation validation on macOS does not enter that branch or load the
bootstrap helpers. No helper or startup profile was exercised in this task.

These checks cover the actual runtime path relationships rather than checking
one guessed pyvenv.cfg. They intentionally support a narrow profile and do not
qualify every CPython installation, embedded interpreter or distribution layout.
They do not defend against later privileged modification of the interpreter,
stdlib, locked packages or source tree. Their immutability, identity and ABI
remain trusted-host qualification prerequisites. No Linux interpreter or wheel
re-lock is performed or implied here.

Reference behavior consulted as documentation, not host evidence:
[CPython3.14 path initialization](https://docs.python.org/3.14/library/sys_path_init.html)
and [command-line flags](https://docs.python.org/3.14/using/cmdline.html).
The current dependency lock still pins preparation CPython3.14.3 and still blocks
Linux. Newer online documentation is not a replacement interpreter pin.

## Explicit protected-file exception and invariants

run.py is an authorized exception to the earlier protected-file allowlist. Its
changes are limited to sealed-FD CLI/authorization consumption, import-boundary
setup and moving existing model imports after those checks. Its run_area AST
and main body from schema loading through the execution/evidence loop match the
original baseline exactly. No alternative execution entry point is added; its
old authorization-path CLI is removed rather than retained as a fallback.
The environment marker is still not an authentication/security boundary against
an operator who can invoke or edit the source directly.

Static checks now compare16 protected non-document artifacts byte-for-byte
rather than17, because run.py is explicitly removed from that count. The run.py
exception and its AST checks are reported separately. All24 facts, F00–F07/four
rotations, F32+78+144=254/256, F288-v1 deferred status, candidates, oracles, area
domains, accounting, both evidence schemas, protocol/hash and source pins remain
unchanged. D/S/R topology, memory/swap/CPU/deadline and artifact ceilings and
all governance/research holds remain unchanged.

identities.json covers every prepared source/document except itself and the
historical/static validation record. lq01-r1-source.json now explicitly includes
run.py and the protected-file exception. Git identifies both manifests and the
validation record. These identity records are not qualification receipts.

Remaining host-dependent risks: memfd/seal availability and descriptor inheritance,
actual CPython prefix/path/config behavior, locked import/extension ABI integrity,
cgroup delegation/kill/orphan behavior, startup/GO failure paths, capacity and
deadline overshoot remain untested. Linux x86_64 and ARM64 are unqualified;
macOS/Windows execution is unsupported. Source acceptance, human RFC §4.1 review,
Linux qualification and one-run authorization remain separate decisions.
