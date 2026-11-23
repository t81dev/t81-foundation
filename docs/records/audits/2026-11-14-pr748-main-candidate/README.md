# PR #748 reconciliation against refreshed main

Disposition: **surviving bounded fix; local candidate prepared; cross-platform integration evidence pending**.
No original branch change, push, PR retarget/closure or merge was performed.

| Source | Exact revision |
| --- | --- |
| Refreshed main / candidate base | 9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2 |
| Original #748 head | 7a3cdf850a143758eb1355ebb8747368e6ca72cb |
| Original #748 parent / #747 head | b18075ad6de93091f9aea30707ef5237d479c9d1 |
| #686 predecessor | ac7397aa5f8751d34adcb6ce2d38e3141e65b36c |
| Original policy-only commit | 8010a082bf454b291fced8cd14d1bace4f7ff1a2 |
| Tested candidate code commit | 8e236dba8a1a61568ef4d916afab55f7b5ce45bf |

Candidate branch: `candidate/pr748-main-policy-metadata`. The evidence-only commit
following that code commit adds this audit; production/test sources stay identical.
GitHub metadata and source/patch comparisons are retained under evidence/.

## Surviving unique changes, dependency and conflict result

All #748 policy changes still survive uniquely against main: hook checks in block
capability publication, capability revocation and repair, plus ref-aware helper
and Release-active populated-store regressions. Main has the lower-level read/write
hook guards but lacks these three metadata checks.

The two #748 changed blobs are identical to closed/unmerged #686:

- fs/block_backed_driver.cpp: `3a3b00eb9bc08fcf7ad0a2b866598a3fbf79b4fc`.
- tests/cpp/canonfs_policy_boundary_test.cpp: `a9df4a89a86a836df8ed5f92d43ecdc396ba8571`.

This establishes source lineage/duplication with the predecessor, not evidence that
main already contains the fix. #747 is closed/unmerged and was #748's CI parent.
The candidate does not carry that old parent or its workflow changes. Main already
provides the relevant Driver/hook/CanonStore and test registration infrastructure;
fresh compilation verifies this bounded dependency set. Original parent and main
are byte-identical for both changed files, canon_driver.hpp and canon_store.cpp.
The original two-file patch passes git apply --check on main without conflict.
No substantive merge conflict was resolved; no full old-branch merge was attempted.

Other changes to the shared policy-boundary test and block driver overlap this
same source surface and should be checked before later integration. Main's newer
Windows/CI work is retained as-is; this task neither reverts it nor claims every
old #747 commit is an ancestor or separately revalidated on every target.

## Candidate scope

Production driver is **byte-for-byte #748's version** (blob above). The existing
policy-boundary test adds three denial modes: default verdict, zero metadata budget
and missing required event, plus capacity-query counting before the callback and
after denial. Matrix: three operations × nine modes = **21 negative + 6 positive**.
The imported seed/survivor checks, populated bytes, ref/op verification, writes,
reads, flush counts and successful effects remain active. clang-format **18.1.8**
was applied only to the edited test. Shared test_runtime_check.hpp is unchanged.

Only two code/test files differ from main. No public API, serialization, arithmetic,
capability semantics, workflow, scientific gate or benchmark change is included.

## Authorization and affected side effects

Source inspection plus the executed matrix support this bounded statement:
**with a configured hook, each of the three affected method invocations consults
that hook before its protected store/index/device effects**. Denial and callback
exceptions return CapabilityError. Empty hook explicitly retains bootstrap mode.

| Operation | Callback arguments | Effects reached only afterward |
| --- | --- | --- |
| publish_capability | Publish, grant.target | Capability serialization/packing, put/dedup/index lookup, capacity query, payload write, caps_ insertion, root/overflow index writes and flush |
| revoke_capability | Revoke, supplied ref | caps_ erase, store lookup/removal and zero-write, index erase, root/overflow index writes and flush |
| parity_repair_subtree | Repair, supplied ref | Index clear/next-LBA reset, capacity queries, root/overflow/fallback reads and index rebuild |

The guard is the first statement in each affected method. Thus the inspection
covers also paths not individually injected by this matrix, such as deduplicated
publication and fallback index scanning. Those paths are not falsely reported as
separate executed regressions. Positive matrix controls exercise actual I/O;
negative cases check op/ref, unchanged populated bytes and no read/write/flush or
capacity query, then prove seeded objects/capability index entries survive.
Private caps_ internals are assessed by source ordering, not directly inspected by
the device instrumentation. Matrix success is not an exhaustive proof of all paths.

**Do not expand that statement to all effects or all driver call paths:**

- Constructor recovery with default rebuild_on_open=true scans/rebuilds before a
  hook can be installed. It is existing trusted initialization, not repaired or
  claimed authorized by this candidate. Tests deliberately construct with false.
- A production policy hook appends audit records and advances its audit slot even
  for denied attempts. That observational effect is intentional and is not proof
  of a successful protected mutation; no global-state-unchanged claim is made.
- Repair rebuilds the whole index despite the subtree-shaped API. Supplied ref
  is operation context, not per-descendant authorization or a subtree restriction.
- Empty hook is explicitly unconfigured/ungoverned bootstrap, not an Allow verdict
  from a configured policy. Configured missing/invalid policy still rejects.
- Existing read/write calls still pass the helper's empty ref; this candidate does
  not add target-aware read/write policy or principal/capability enforcement.
- Block read/write do not consult caps_ for permissions; full capability enforcement
  is outside scope. The map remains in-memory. Existing ignored remove/flush failures,
  crash atomicity and recovery semantics are not repaired or injected here.
- Hooks/raw driver holders are trusted host code. External serialization remains
  required; callback replacement/reentrancy, concurrent revocation and malicious
  devices/hosts are not made transactional or sandboxed.

## Executed verification — fresh local builds

Environment: macOS ARM64, Apple clang 21, CMake 4.3; exact version output, configure
argv, actual compiler commands and logs retained. Builds are fresh isolated trees,
not reused T81 archives. Local Asio and installed GTest override dependency discovery;
no claim of authenticated dependency provenance. TUI/examples/benchmarks/fuzz and
TernaryOS aggregate build are OFF; this is hosted CanonFS scope, not bare-metal proof.

1. **Negative control:** unfixed main production source + #748's original regression
   test, compiled Release/-DNDEBUG, exited **1** at missing callback-count check
   (first publication denial). This is main plus a test injection, not a claim that
   unchanged main's existing tests detect the defect. It does not separately execute
   every operation's baseline failure before the process exits.
2. **Final candidate Release:** rebuilt after test formatting; selected CTests
   canonfs_policy_boundary_test, canonfs_persistent_driver_test,
   canonfs_axion_trace_test **3/3 pass**, matrix **21 negative + 6 positive**.
3. **Final candidate ASan+UBSan:** fresh RelWithDebInfo/-DNDEBUG build with both
   sanitizer flags on changed driver and test, same CTests **3/3 pass**, same matrix.
   No sanitizer diagnostic in these runs; not a universal memory-safety proof.
4. CTest show-only JSON proves exactly three selected registrations. Actual Ninja
   commands prove -DNDEBUG remains enabled; T81_TEST_CHECK uses explicit failure
   exit and is unchanged. Baseline exit 1 demonstrates the assertion path executes.
5. clang-format-18 dry run and **runtime/test-source-scoped** git diff --check pass.
   The full evidence packet diff check reports raw capture whitespace (patch context,
   CTest JSON, CMake caches and probe logs); that exit 2 is retained separately and
   is not reported as a clean full-packet check. Raw records are not normalized.
   Initial formatting diagnostics are retained; test layout was corrected before
   final rebuilds/reruns.

The original #748 body reports earlier 3/3 results and freeze checks. Those are
retained as author-reported metadata, not relabeled as these fresh runs. This task
performed no full-suite, Linux/Windows/macOS-x86_64, hosted ARM64-Linux, QEMU or hardware
qualification and started no remote CI or performance campaign. Cross-platform
results/required checks remain prerequisites before claiming integration readiness.

Codex prepared this derivative candidate and added tests in shared session context,
including prior related Windows/capture work. This is newly executed local evidence,
not independent security/scientific acceptance or authentication of original runs.
