# RFC-00E9: Proposed C++ SDK 2.0.0 Compatibility Transition

- **RFC-ID:** RFC-00E9
- **Title:** Proposed C++ SDK 2.0.0 Compatibility Transition
- **Status:** draft
- **Type:** standards-track
- **Applies-To:** C++ SDK under `include/t81/**`, package versioning, API baseline and release migration
- **Created:** 2026-10-30
- **Updated:** 2026-10-30
- **Author:** Codex, at maintainer request; human review pending
- **Supersedes:** None
- **Superseded-By:** None
- **Discussion:** A.1 compatibility adjudication; protected integration [PR #649](https://github.com/t81dev/t81-foundation/pull/649)

## Summary

This draft requests one scoped decision: retain the existing C++ interfaces and
prepare an explicitly breaking SDK 2.0.0 release, rather than claim that the
accumulated interfaces are compatible SDK 1.9.1 evolution. The API lock is
historically inconsistent with committed headers, and independently identifiable
source breaks and protected layout changes already exist on the integration base.

**Authorization is RFC preparation and draft publication only.** This document is
proposed text, not an approved exception, version change, release authorization,
API-lock update, RFC acceptance or permission to merge. Proposed requirements
below take effect only through explicit acceptance and separately authorized
implementation. Merging a document whose status remains `draft` would not accept
the transition.

PR #649 must remain at `3a28eae9c11bbd18b09b71509fa7f74e0584bc24`.
This draft changes no implementation, SDK metadata, lock, existing RFC status,
workflow, protection or enforcement setting. RFC-00E9 is a proposed identifier
subject to maintainer assignment. RFC-00E8 and ADM-2 are unpublished proposals;
neither is authority or a prerequisite here.

## Motivation and Existing Authority

The change fits the existing deterministic-runtime/CLI stability and CanonFS
maintenance lanes: it makes a public contract reviewable without adding a
subsystem. Retaining the current implementation avoids a broad rollback of
functional additions and security corrections. It does not make SDK 2.0.0 an
established requirement before approval.

Authority is pinned to the reviewed base, not to this proposal:

- [ADR-0003, Accepted](https://github.com/t81dev/t81-foundation/blob/f775680aba7db200691c0f4e2a3174b0e7e51581/docs/architecture/adr/ADR-0003-public-api-boundary.md):
  `include/t81/**` is the public boundary; redefining it requires a superseding
  ADR and migration guidance.
- [Freeze Enforcement, sections 1.D, 2 and 3](https://github.com/t81dev/t81-foundation/blob/f775680aba7db200691c0f4e2a3174b0e7e51581/docs/governance/FREEZE_ENFORCEMENT.md):
  public layouts, signatures, symbols and header organization are protected;
  source/binary breaks require a major version and the breaking-change procedure.
- [Release Discipline, sections 1-3A](https://github.com/t81dev/t81-foundation/blob/f775680aba7db200691c0f4e2a3174b0e7e51581/docs/product/RELEASE_DISCIPLINE.md):
  compatible additions require a minor version; patches are bug fixes without
  compatibility or deterministic-contract expansion. Release prerequisites apply.
- Accepted [RFC-0034](RFC-0034-t81-native-ai-inference.md),
  [RFC-0040](RFC-0040-swar-formalization.md),
  [RFC-0041](RFC-0041-simd-formalization.md),
  [RFC-0044](RFC-0044-stable-packed-trit-vector-interface.md),
  [RFC-0048](RFC-0048-deterministic-surface-definition-and-governance-boundaries.md)
  and [RFC-00B8](RFC-00B8-governed-foreign-function-interface.md) supply relevant
  inference, packed-trit, classification and FFI contracts. Determinism class and
  C++ compatibility are different questions. Experimental labeling does not
  automatically remove a header from ADR-0003's boundary.

An unchanged compilation result or object size does not prove source, binary or
behavioral compatibility. A digest mismatch alone proves none of those breaks.
Concrete evidence below independently rules out the blanket 1.9.1 claim. Even
compatible feature additions are not patch-only changes under existing policy.

## Evidence and Historical Baseline

### Pinned identities

| Role | Commit / tree |
| --- | --- |
| Reviewed `main` and #649 base | `f775680aba7db200691c0f4e2a3174b0e7e51581` |
| Frozen #649 head | `3a28eae9c11bbd18b09b71509fa7f74e0584bc24` |
| Frozen tree | `6eb71024c4b1c2dc0aec058c7e66e7d29a46b0ec` |
| Commit writing preserved lock digest (L) | `bc3cd3879398c340a880edfdeb07c02639ec5883` |
| Historical `v1.9.0` tag target (T) | `6100a5ce5b283e231bf6cda9ca261240717e7afd` |

The base-to-frozen diff changes no public headers, API validator or API lock.
All compatibility changes discussed here predate #649. L and T are explicit
comparison snapshots, not assumed coherent or accepted SDK baselines.

### Lock inconsistency

Authoritative paths are `scripts/governance/check_public_api_semver.py`,
`docs/governance/PUBLIC_API_LOCK.json` and `include/t81/config.hpp` at the pinned
revisions. The validator algorithm is unchanged between L and the frozen head.
It sorts `.hpp` paths and hashes each path, NUL, SHA-256 of file bytes, NUL.
It includes comments, inline bodies, experimental headers and version macros.
It checks byte identity and version equality, not SemVer compatibility.

The preserved lock says version `1.9.0`, with surface digest
`fa406791328c8ce64c03f25aa794b20b069d0b68a1755213585f90aee2d02c93`.
Its file SHA-256 at both L and the frozen head is
`36227edf0ac6eeab1e0d2e4702e0b9ff9663f212eed7698436b7c768ba106740`.

| Committed snapshot | Header count | Calculated surface SHA-256 |
| --- | ---: | --- |
| L | 224 | `6e364b850f1804029a4e272ce41c08d8dd107e9780e6322742c57ba68f87ec94` |
| Reviewed main and frozen head | 252 | `f398c5a4f0bd10249a5f0c20c8f5b507e6a908a734e7617fe52c9696bf53abe2` |

Neither matches the preserved lock. This establishes committed-tree/lock
inconsistency at its writer, not the identity of its generating workspace.
No reconstruction of unknown generated/untracked files or checkout state is
attempted or required by this proposal. Preserve the old record; do not rewrite
it as a passing baseline. Hashes identify content, not approval or authenticity.

T also illustrates version ambiguity: its tracked configuration says `1.3.6`,
while its lock says `1.3.2`. The tag name alone cannot supply a trusted 1.9.0
SDK ABI baseline. The `1.9.1` configuration bump is attributable to
`bb74edaa7178444c99639ce21517baca0c497b94`.

### Reproduction without lock generation

Run from a clone containing the pinned objects. This reads Git bytes in memory;
it does not extract files or invoke `--write-lock`:

```sh
python3 -B - <<'PY'
import hashlib, io, subprocess, tarfile
refs = [
    "bc3cd3879398c340a880edfdeb07c02639ec5883",
    "f775680aba7db200691c0f4e2a3174b0e7e51581",
    "3a28eae9c11bbd18b09b71509fa7f74e0584bc24",
]
for ref in refs:
    data = subprocess.check_output(["git", "archive", ref, "include/t81"])
    with tarfile.open(fileobj=io.BytesIO(data)) as archive:
        files = sorted((m for m in archive if m.isfile()
                        and m.name.endswith(".hpp")), key=lambda m: m.name)
        digest = hashlib.sha256()
        for member in files:
            content = archive.extractfile(member).read()
            digest.update(member.name.encode("utf-8") + b"\0")
            digest.update(hashlib.sha256(content).digest() + b"\0")
    lock = subprocess.check_output([
        "git", "show", ref + ":docs/governance/PUBLIC_API_LOCK.json"])
    print(ref, len(files), digest.hexdigest(), hashlib.sha256(lock).hexdigest())
PY
git diff --name-only f775680aba7db200691c0f4e2a3174b0e7e51581 \
  3a28eae9c11bbd18b09b71509fa7f74e0584bc24 -- include/t81 \
  docs/governance/PUBLIC_API_LOCK.json scripts/governance/check_public_api_semver.py
```

The archive calculation covers regular tracked headers at these revisions;
the filesystem validator also sees untracked `.hpp` files. A future release
must verify their absence rather than assume Git and workspace scope agree.

## Compatibility Findings and Attribution

Paths below resolve at the pinned main/frozen revisions. Historical changes are
identified by full commit IDs in the attribution table. Confirmed source breaks
are distinct from layout-policy violations and unexecuted platform ABI checks.

| Surface and source path | Verified change | Classification and limit |
| --- | --- | --- |
| `ValueTag`, `include/t81/vm/state.hpp` | L to current: insert `IntVectorHandle=20`; `SymbolicGraphHandle` 20 to 21, `Tier2FrameHandle` 21 to 22, `InfiniteHandle` 22 to 23 | Numeric interpretation/behavior changes; names and underlying byte width survive. Numeric assertions break. Cross-version tag interpretation is incompatible; this is not evidence of a TISC wire encoding change. |
| `FFICallResult`, `include/t81/ffi/ffi.hpp` | T to L: add `vector<string>`; L to current: add `vector<int64_t>` before `void*`, shifting pointer index 4 to 5 to 6 | Confirmed source break for indexed access; exact variant type and exhaustive visitors change. Binary interoperability is unverified and must not be promised, even if sizes happen to match. |
| `State`, `include/t81/vm/state.hpp` | Add `weights_tensor_trits` (T to L) and `int_vectors` (L to current) | Protected layout changes; inserted storage affects subsequent members. Named-member consumers may compile; aggregate decomposition/positional consumers can break. No platform ABI certification. |
| `ThreadContext`, same path | Append `activation_quarantined` (T to L) | Protected layout and activation-state behavior change; decomposition arity changes. Possible padding reuse would not prove binary compatibility. |
| `SyscallContext`, `include/t81/axion/context.hpp` | Append `final_authorization` | Protected layout and authorization-context change; decomposition arity changes. Actual platform binary compatibility unverified. |
| `compile`, `include/t81/cli/driver.hpp` | Add defaulted sixth parameter `bool announce_success` | Confirmed source break for the old exact function-pointer type; ordinary calls can survive. Changed signature requires an old-symbol adapter to promise old ABI; none is established here. |
| `run_tisc` / `debug_tisc`, same path | T to L: append weights-model parameters | Exact function types change despite defaults; include in migration when comparing with T. Old binary compatibility unverified. |
| `include/t81/monitoring/performance_monitor.hpp` | `shared_mutex` to `mutex` for collector metrics, profiler sessions and resource-manager usage | Private synchronization members affect public class layout. Protected layout changes; public method signatures remain. Binary and concurrency/performance equivalence unverified. No class named `PerformanceMonitor` is established by this filename. |
| `include/t81/types/T81Int128.hpp` and `T81BigInt.hpp` | Remove standalone global `int128_t`; relocate to `t81::v1::detail`, retain BigInt-facing alias | Confirmed standalone source break on tested Clang path. Windows/MSVC branches need separate verification. Detail namespace is not a new recommended public alias. |
| `ShellSession`, `include/t81/axion/shell/shell_session.hpp` | Add `ShellBackend` polymorphic base, virtual methods/destructor and members | Protected layout/polymorphism change; calls can survive while type traits and special-member behavior change. Old binary interoperability unverified. |

Additional relevant history must remain visible: `T81String::operator<=>` was
removed at L and restored later; `isa::ir::Opcode` gained `TTRANSPOSE`, shifting
following implicit values; `AxionVerdict::allow` defaults to false instead of
true; policy parsing/final authorization tightened; native float formatting
and HostFloat tensor dispatch changed. These are source-history, numeric or
behavioral changes, not evidence that every current behavior is a regression.
Security corrections must be retained. Current `T81BigInt`/canonical numeric
and packed-trit aliases are not removed by this proposal. Aliases do not isolate
the ABI of their underlying types.

L to main has 28 added and 25 modified `.hpp` paths, with no deletions. New
CanonFS, decoder, controlled-backend, direct-API, support and experimental-AI
declarations are additions, not independently certified implementations or
patch-only evolution. This RFC does not claim an exhaustive cross-platform ABI
audit of those declarations.

| Commit | Relevant attribution |
| --- | --- |
| `cc752934f464c9b29054d5fbb9cd9261a0fe6e75` | Thread activation quarantine field; CLI model parameters |
| `827c45532abec0046c957b4b37fb58ba2355b0c6` | State weight-trit cache; string-list variant alternative |
| `4e1e437b4fc910b2fd24aa66e7f60f48cb1403e8` | FFI byte-span/string-list bridge result structs |
| `bc3cd3879398c340a880edfdeb07c02639ec5883` | Preserved lock digest; historical string comparison removal |
| `3188efce41b6da013ec3db256604aa1e1f589021` | Integer alias/include relocation |
| `ffe867ffa8b63166b49cf2d6c8f2e9095e908281` | Integer-list variant, value tag and State storage; float/tensor changes |
| `38b3383ea4a7577232f5c557cc63ca819346c27a` | IR transpose insertion |
| `d3e1abca7e1449ac288bafad92cbd9219bf9152f` | Shell backend interface |
| `818822dceaf7dbb752f8e90c1350607a59f0744a` | Compile announcement parameter |
| `9da7900babf65339b473480c79a49875bf01200f` | Monitoring mutex substitutions |
| `dec74cfa4d7be321e0c28adb276a8dc2d278c502` | Final authorization context and fail-closed verdict default |

The preceding A.1 investigation ran declaration-based Clang syntax probes:
old five-parameter compile function-pointer assignment, `get<5>` pointer
assignment, and standalone `int128_t` compiled against L and failed against main.
Environment: macOS ARM64, Python 3.14.3, Homebrew Clang 22.1.1; command
`clang++ -std=c++23 -x c++ -fsyntax-only -`. These were extracted-declaration
probes, not installed-SDK or mixed-binary tests. The main/frozen-equivalent API
validator exited 1 with both version and digest mismatch. These are author-run
records from the same Codex conversation, not independent scientific validation.
Earlier Linux, hosted determinism and CodeQL results are reported context;
they are not rerun or newly certified by this documentation proposal.

## Proposal: Exact SDK Boundary and Consumer Contract

### Scope retained from ADR-0003

Retain the public C++ boundary **all headers under `include/t81/**`**, including
public classes' private storage, existing public aliases and transitively exposed
types. Preserve the validator's current `.hpp` scope; inventory all header
extensions as a separate completeness check, without silently excluding files.
CMake currently installs this entire directory. No stable allowlist, four-domain
architecture, new namespace hierarchy or superseding ADR is proposed here.

Establish SDK 2.0.0 as a new compatibility baseline after the approval sequence
below. It deliberately does not promise source or binary compatibility with
earlier SDKs. From that baseline, apply existing major/minor/patch and freeze
rules. Keep existing `t81::v1` aliases/namespaces; the namespace spelling is not
a guarantee of SDK 1.x binary interoperability and is not renamed to `v2` here.

The SDK version does not change the frozen TISC ISA version, opcode registry,
wire semantics, canonical encodings, schema versions or determinism profiles.
The existing shared `T81_VERSION` metadata also drives CMake package versions;
future implementation must explicitly document it as the SDK/package version.
It must not mechanically replace ISA version text or independently versioned
artifact contracts. This is clarification of the existing release surface,
not implementation of separate compatibility-domain infrastructure.

Experimental implementations keep their existing classifications and limits.
RFC-0044's stable packed-trit surface remains protected despite its experimental
implementation namespace. No public exposure is retroactively exempted from
compatibility review. `ffi/c_api/` is outside this C++ boundary and current lock;
its opaque handles do not establish a certified C ABI. No C ABI expansion or
promotion is authorized, and existing C façade tests must continue passing.

### Supported consumers and compilers

Proposed supported consumers are source-built C++ applications using installed
public headers and the exported `T81::` targets from the `T81` or `T81Foundation`
CMake packages. In-repository consumers remain regression obligations. All
consumers rebuild against a matched SDK header/library set.

Use C++23 by default (`T81_USE_CXX23=ON`). Preserve the existing opt-in C++20
lane only for the subset actually passing its checks; do not promise blanket
C++20 support. Compiler acceptance is feature/test based, including a matching
standard library; README's GCC 13+ example is not proof that every configuration
works. Before release, publish exact tested GCC/libstdc++, Clang/libc++ and
MSVC/STL versions, targets and flags for the existing Linux, macOS, Windows,
x86-64 and ARM64 support lanes. Untested combinations remain unverified.

No compatibility is promised across compiler ABIs, standard-library vendors or
ABI settings, architectures, incompatible build flags, or mixed SDK majors.
This scopes testing; it does not relax existing supported-platform obligations.
CMake's existing `SameMajorVersion` package behavior should be retained and
tested so SDK 2 does not satisfy an SDK 1 version request.

### Migration and previous major

Consumers must rebuild, update exact CLI function types, use documented variant
alternatives rather than historical indices, and update tag-dependent logic.
Do not reinterpret old tag-bearing objects as current objects. Recreate VM and
policy contexts using current constructors/initializers; audit aggregate
decomposition and positional initialization. Review shell inheritance/lifetime
assumptions and monitoring integration. Stop using the standalone global
integer alias; use a documented public numeric type suited to the application.
Review policy-denial and formatting changes separately from build fixes.

No in-memory object or variant bytes become a portable serialization format.
Existing normative serialization, bytecode and artifact identities retain their
own accepted contracts. Migration must not renumber frozen wire values or weaken
authorization to emulate old behavior.

Propose marking SDK 1.x **Deprecated** upon the eventual 2.0.0 release, retaining
historical tags, locks and evidence unchanged. No maintained 1.x branch, support
term, backport commitment or retrospective patch compatibility is implied.
Maintainers may instead select Maintenance Mode only with an owner, scope and
support term. Neither disposition activates through this draft.

## Determinism / Safety Considerations

Preserve existing TISC encodings and semantics, canonical serialization and
content identity, deterministic profiles, pre-side-effect authorization,
fail-closed defaults and required-evidence evaluation. A major SDK version is
not permission to change these contracts. Any newly discovered divergence must
follow its existing incident/freeze process; this RFC supplies no blanket waiver.

No VM-state refactor, Pimpl, security rollback, subsystem addition, general
governance repair or CI activation is in scope. The historical lock discrepancy
must remain visible after any authorized new baseline is recorded.

## Implementation Plan (Future Authorization Required)

1. Maintainer resolves the decisions below and accepts a scoped breaking-change
   RFC through the existing process. RFC acceptance is design approval, not
   implementation, merge or release authorization.
2. On a separately authorized branch, retain current interfaces and apply only
   SDK/package version metadata consistency, normative SDK transition wording,
   implementation-matrix update, migration guide, `BREAKING CHANGES` changelog
   entry and approved previous-major disposition. Inventory metadata consumers;
   update applicable files, not unrelated independently versioned specifications.
3. Add focused consumer/package regression checks and retain security/ISA tests.
   No production interface redesign is necessary merely to acknowledge a break.
4. Freeze the proposed public surface and name an exact full baseline commit B
   containing approved headers, version metadata, generated-header bytes and
   build settings. B cannot be the present 1.9.1 frozen head. Record B's tree,
   sorted path/hash inventory, toolchain, configure command and workspace status.
5. Only with explicit authorization, generate the API lock using the procedure
   below. Commit the resulting lock and evidence on the implementation branch.
6. Review and test the resulting integration commit, then obtain separate merge
   and release approvals. No merge, tag, release or enforcement activation is
   implied by passing checks or accepting this draft.

### Future lock procedure and exact revision requirements

Use the unchanged validator; do not use its output as compatibility analysis.
Preserve the old lock bytes in Git history and link the discrepancy from the
new baseline record. Do not hard-code the current 1.9.1 digest as the future
2.0.0 digest: version macros participate in the hash.

For B, require a clean disposable checkout with pinned dependencies/toolchain,
no untracked headers, and byte-preserving checkout settings. Record platform,
compiler, standard library, Git/Python/CMake versions, options and dependency
revisions. Configure in a separate build directory, then verify that generating
`config.hpp` leaves the approved header inventory and bytes unchanged. If it
changes bytes, commit/review them and choose a new B before proceeding.

At B the retained old lock is expected to fail. After explicit approval, run
`python3 -B scripts/governance/check_public_api_semver.py --write-lock` exactly
once in that checkout. Immediately verify that only
`docs/governance/PUBLIC_API_LOCK.json` changed and that its version and digest
match independent computation from B. The baseline receipt records B externally
to the three-field lock format; do not invent an incompatible lock schema.

Let L2 be the commit adding that lock and receipt. Record L2 separately from B;
its public-header tree must equal B's. Independently check out L2 and run the
validator without write mode. Repeat inventory/digest verification from Git
objects and a second clean checkout, including supported platform checkout
settings. Any header drift invalidates the baseline and requires review before
another authorized generation, not repeated regeneration until CI passes.
The receipt cannot include its own final commit hash; report L2 in the PR/CI
record after committing. Lock approval proves neither compatibility nor release
readiness; all acceptance obligations still apply.

### Relationship to PR #649 and ADM-2

Keep #649's branch/head and preserved lock untouched. This documentation branch
can be reviewed against main independently. Future implementation can be a
separate companion or successor candidate authorized by the maintainer. A
combined tree has a new SHA and needs its own validation. The frozen head's
old-lock failure cannot be relabeled as passing after another tree is fixed.

ADM-2 M1 is not an established dependency for inventory, baseline selection or
this scoped major transition. A later domain carve-out would require its own
approval and, where applicable, superseding ADR before domain-specific locks.

## Alternatives and Tradeoffs

- Keep 1.9.1: would require restoring a specifically approved historical source
  and layout contract, with adapters/reversions and behavioral review. Simply
  refreshing the lock is insufficient. Security bugs must not be restored.
- Separate compatibility domains: potentially useful later, but adds policy,
  inventory, packaging and tooling decisions unnecessary for this narrow change.
- Defer: retain the current lock and hold integration/release. This remains the
  correct state until an approved resolution exists.

The proposed major transition minimizes implementation breadth while making
consumer rebuilding and migration explicit. It does not recover a missing
historical baseline or certify untested binary behavior.

## Open Questions and Maintainer Approval Record

All decisions are pending. A record must identify approver, date, exact revision,
decision and conditions; a JSON field, AI summary or successful test is not
authorization.

| Decision | Proposed disposition | Required human decision |
| --- | --- | --- |
| RFC identity/status | RFC-00E9, draft | Confirm identifier; explicitly accept/reject through RFC process |
| Retain current SDK breaks | SDK/package 2.0.0 | Approve breaking-change scope and version; no approval inferred |
| Boundary | Preserve all `include/t81/**` | Confirm transitive exposure and no retroactive experimental exclusions |
| Supported consumers/toolchains | Matched C++23 SDK builds; tested C++20 subset | Approve exact tested release matrix and declared limitations |
| SDK 1.x | Deprecated at eventual release | Approve disposition, or fund owned Maintenance Mode |
| Baseline B | Exact future approved commit, not yet selected | Approve header inventory, generated bytes, configuration and migration evidence |
| Lock generation | One scoped generation after B approval | Explicitly authorize the write and review its resulting delta |
| Integration | Separate combined candidate | Authorize implementation and later merge independently; keep #649 frozen |
| Release | No release authorization | Approve final readiness packet, tag and publication separately |

## Acceptance Criteria

### For acceptance of the design

- Maintainers resolve the decisions needed for the scoped transition and record
  approval of the exact RFC revision. Remaining release-specific selections are
  assigned to the implementation/release gate, not silently presumed complete.
- Review verifies cited source changes and acknowledges the historical lock
  inconsistency without claiming to recover its generating workspace.
- Migration, support scope and preserved ISA/determinism/security contracts are
  explicit. No four-domain implementation or broad governance repair is bundled.

### For future implementation and release

- Version/configuration/package metadata is consistent; applicable spec and
  implementation-matrix updates, migration notes, changelog and previous-major
  disposition satisfy the existing breaking-change procedure.
- B and L2 are recorded with reproducible inventories/digests. The validator
  passes read-only on L2; the aggregate governance check passes on the actual
  combined integration candidate, without suppressions or altered enforcement.
- Installed CMake consumers cover both package names, compile/link against
  supported exported targets, reject cross-major package matching and exercise
  intended alias/include usage. Old exact CLI function types and variant-index
  assumptions are documented migration failures rather than falsely passing
  compatibility tests.
- Focused current-behavior checks include `vm_ffi_bridge_test`,
  `t81_cli_contract_test`, `t81_core_numeric_compat_test`,
  `t81_v1_canonical_numeric_contract_test`, `t81_c_api_bigint_test`, relevant
  CanonFS authorization/CLI regressions and assertion-enabled
  `tisc_opcode_matrix_test`. Verify target names against the implementation
  revision; retain exact configure/build/CTest commands and nonzero test counts.
- Relevant determinism and required CI checks pass at the actual combined SHA
  across existing supported platform lanes. Retain raw results, dependency pins,
  compiler/library versions, failures and unexecuted limits. Documentation-only
  checks or previous frozen receipts do not establish this new candidate's result.
- Do not require old-binary interoperability for an explicitly breaking major;
  do not claim it. Protected-layout impacts remain scoped to tested ABIs, and
  source probes or equal object sizes do not replace mixed-binary evidence.
- Existing release prerequisites and separate human merge/release approvals
  remain mandatory. This draft itself closes no CI failure and activates nothing.
