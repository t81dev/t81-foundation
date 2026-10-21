# CanonFS production policy hardening

Baseline: merged PR #537, `634e0dc69a69cb148f81159ac8c7fc3225582149`.
Historical experiment: `d8ad35254a1e35ca7374a4f57f7a870d568521fa`.

The research question is whether the existing policy architecture can enforce its
pre-side-effect boundary without another architectural layer. The patch stays in
the current CanonFS hardening/runtime stability lanes. It extends the existing
context with explicit final authorization, rather than adding another policy
engine, adapter service, or subsystem. Measurement conclusions appear below and
are restricted to the tested serialized boundary.

## Root cause and contract correction

The original hook converted parse failure into an absent engine and then returned
an allowing verdict. The parser also ignored trailing input; numeric conversion
could throw, negative resource limits could become enormous unsigned limits, and
out-of-range tier values could narrow. Separately, PolicyEngine evaluated required
match/segment/Axion/alignment events only at `Halt`, while the storage callback
supplied `Nop`. Both interpreted and compiled paths had that final-check convention.
Only Deny was rejected by the old hook; other non-Allow verdicts were accepted.

Arm C's synthetic Halt is not the production design. The minimal contract change
is **`SyscallContext::final_authorization`** (default false). It requires an active
policy and checks all required evidence immediately, without changing the opcode
or normal VM completion semantics. Cached loop matches are discarded for each
final authorization request. This is source-compatible for existing callers;
consumers of the C++ struct must rebuild because its binary layout changes. No ISA
mapping or serialized artifact identity is changed.

The production hook now:

- installs a rejecting callback for missing/malformed/throwing policy input;
- requests final authorization with the honest default Nop context;
- accepts only explicit Allow, and denies evaluation exceptions;
- checks the current operation's canonical attempted-event strings, rather than
  historical audit events from another call or driver;
- reports one proposed metadata mutation for non-read operations, zero for reads,
  so `(max-meta-writes 0)` is an explicit write denial. This is a per-operation
  proposal count, not a cumulative VM/session quota.

The canonical audit strings remain unchanged. An event records an **attempt**, not
successful completion or an external grant. Policies cannot require a future read
as a precondition for an earlier write. Two pre-existing trace tests previously
relied on that unenforced condition; they now install valid per-operation write
and read policies while retaining their trace and persistence checks.

All three driver implementations deny a throwing callback before protected
mutation; the block-backed noexcept path no longer terminates on that error.
A default-constructed CanonFS `AxionVerdict` now denies. Low-level drivers still
allow an **explicitly unconfigured** callback in bootstrap mode, as the existing
optional-hook and capability-bootstrap contracts specify. This is not configured
policy authorization. Installing the text hook with empty policy always denies;
clearing a hook is a trusted host operation, not revocation.

## Entry-point audit

The complete search for `make_axion_policy_hook` and `set_axion_hook` found:

| Surface | Affected path / responsibility |
| --- | --- |
| `kernel/axion/canonfs_hook.cpp` | Shared text-policy factory; every installed callback now performs final authorization. Outside this implementation, current direct users of the factory are the trace demo, experiment, and C++ tests; it is not automatically installed on every raw driver. |
| `fs/persistent_driver.cpp` | Write, read, capability publish/revoke, and repair invoke `axion_allow`; writes check before exclusive `_open`/`open` and cache insertion, even for an already existing object. |
| `fs/in_memory_driver.cpp` | Same five operations check before object/parity/capability insertion or removal. |
| `fs/block_backed_driver.cpp` | Write/read/publish/revoke/repair invoke the callback before the CanonStore operation. Exception handling covers its noexcept helper. |
| `vm/jit/jit_trace_cache.cpp`, `include/t81/jit/jit_trace_cache.hpp` | Public hook pass-through to the supplied driver; custom callbacks now deny on exception/default verdict. |
| `include/t81/canonfs/{canon_driver,axion_hook,canon_types}.hpp` | Public driver/factory/verdict contracts, now documented explicitly. |
| `examples/storage-and-canonfs/canonfs_trace_demo.cpp` | Existing factory consumer; ordinary `(policy (tier 1))` allow behavior is retained. |
| `examples/governance/accountable-computation/runner.cpp` | D uses only the production factory plus a delegating call counter. B/C are measured with the original binary; C's experimental logic is not used by D. |
| C++ trace/persistent tests, `jit_canonfs_cache_test.cpp` | Direct hook consumers; updated trace tests enforce per-operation conditions rather than relying on missing enforcement. |

Raw driver creation also occurs in `fs/canonfs_interchange_ops.cpp`, VM setup in
`vm/vm.cpp`, `ternaryos/kernel/kernel_lifecycle.cpp`, the Python bindings in
`lang/python/t81_python.cpp`, CLI code in `tools/cli/core/{main,driver,canonize_tensor}.cpp`,
and `tools/cli/ai/ai_cli_shared.cpp`. Benchmarks also construct raw drivers.
These do not install this text-policy hook automatically. Interchange has its own
profile/evaluator boundary, and VM dispatch has its own policy checks. Their raw
storage access remains trusted/bootstrap access, not a newly claimed end-to-end
policy guarantee. This patch does not pretend that all of those callers acquire
governance simply because the optional hook is fixed.

## Tests and observable effects

`canonfs_policy_boundary_test` snapshots all regular files and bytes under a real
persistent store before and after rejected writes. It tests a fresh distinct
result as well as the existing-object fast path. It covers valid allow, explicit
callback and zero-write-budget denials, default verdict, empty/malformed/trailing
policy, numeric conversion/range errors, missing required events, synchronous
revocation after proposal, evaluation exception, engine bytecode error, and
unconfigured bootstrap mode. It also verifies historical events cannot satisfy a
new request, exercises final authorization with Nop in both interpreted/compiled
engines, and tests missing policy and cached-loop evidence.

The same test invokes the actual in-memory driver on a throwing callback and the
actual block-backed driver against a device write counter: evaluation failure must
produce zero device writes. CanonStore is compiled into that test so the test
works with the TernaryOS build option disabled. The production driver/CanonStore
implementations are not replaced by mocks.

All **49 tests in the selected VM/Axion/CanonFS slice passed**, including the
new production-boundary test and the A/D experiment/adversarial verifier suite.
The original-commit experiment CTest also passed, reproducing its expected B
failures. All 24 new and 18 historical signed sample receipts independently
produced their expected verifier outcomes. C++ formatting and diff whitespace
checks passed. The status-document freshness command passed for October 21.

The first 48-test run exposed an existing out-of-tree fixture-path assumption in
canonfs_interchange_test (it searched ancestors of the build directory for golden
examples). CMake now gives that test the repository working directory; its golden
files and assertions are unchanged. The complete selected slice then passed.
Reproduce the broader slice after the builds below:

```sh
cmake --build /tmp/t81-policy-build --target \
  $(cat examples/governance/accountable-computation/hardening-test-targets.txt) -j 6
ctest --test-dir /tmp/t81-policy-build --output-on-failure \
  -R "^($(paste -sd '|' examples/governance/accountable-computation/hardening-test-targets.txt)|accountable_computation_experiment)$"
```

The target list contains the 48 native tests; the opt-in experiment adds one.
Existing macOS linking still emits duplicate-static-library warnings. No complete
repository build or all-tests-green result is claimed.

## Reproduce the historical control and production arm

Validated environment: macOS 26.5.2 ARM64, Apple Clang 21.0.0, CMake 4.3.0,
Ninja 1.13.2, Python 3.14.3, OpenSSL 3.6.2. The dependency and compiler settings
remain the PR #537 settings: Release C++23, strict deterministic float enabled,
`-O3 -DNDEBUG -Wall -Wextra -Wpedantic`, no TUI/benchmarks/TernaryOS guest build.
Asio is the existing test dependency at `asio-1-30-2`; no packages/frameworks added.

From this checkout, use fresh directories:

```sh
git worktree add --detach /tmp/t81-policy-baseline-src \
  d8ad35254a1e35ca7374a4f57f7a870d568521fa
cmake -S /tmp/t81-policy-baseline-src -B /tmp/t81-policy-baseline-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON \
  -DT81_BUILD_ACCOUNTABLE_EXPERIMENT=ON -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_TUI=OFF \
  -DT81_ENABLE_TERNARYOS=OFF -DT81_BUILD_FUZZ_TESTS=OFF
cmake --build /tmp/t81-policy-baseline-build --target t81_accountable_experiment -j 6
ctest --test-dir /tmp/t81-policy-baseline-build --output-on-failure \
  -R '^accountable_computation_experiment$'

cmake -S . -B /tmp/t81-policy-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON \
  -DT81_BUILD_ACCOUNTABLE_EXPERIMENT=ON -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_TUI=OFF \
  -DT81_ENABLE_TERNARYOS=OFF -DT81_BUILD_FUZZ_TESTS=OFF
cmake --build /tmp/t81-policy-build --target \
  t81_accountable_experiment canonfs_policy_boundary_test \
  canonfs_axion_trace_test canonfs_persistent_driver_test canonfs_read_verify_test -j 6
ctest --test-dir /tmp/t81-policy-build --output-on-failure \
  -R '^(accountable_computation_experiment|canonfs_policy_boundary_test|canonfs_axion_trace_test|canonfs_persistent_driver_test|canonfs_read_verify_test)$'
python3 examples/governance/accountable-computation/run.py \
  --runner /tmp/t81-policy-build/t81_accountable_experiment \
  --baseline-runner /tmp/t81-policy-baseline-build/t81_accountable_experiment \
  --production --output /tmp/t81-policy-measured --repetitions 30
```

The local historical build used an isolated `git archive` of that exact commit;
its runner source was compared byte-for-byte with `git show`. The pinned Asio
source downloaded for the earlier experiment was reused through
`-DFETCHCONTENT_SOURCE_DIR_ASIO=...` for offline configuration. Neither the
untracked `build-bench/` directory nor its binaries were used.

The original six scenario tuples, input bytes/digests, result bytes, allow/deny
fixtures, synchronous revocation barrier, signature format, and verifier predicates
are unchanged. B/C use the original native binary. D uses the patched binary and
calls the corrected production factory; it neither builds an Arm C engine nor
sets Halt/final flags in the harness. The verifier's only behavioral extension is
accepting the D arm label. The current CLI requires an explicit production
comparison and historical binary to avoid relabeling patched B as historical B.

The original `REPORT.md`, `results.json`, and `observed/` remain untouched.
`hardening-results.json` contains all timing samples for this repeat plus hashes
of both native binaries and harness sources. `hardening-observed/` contains one
signed receipt per arm/scenario and caller-pinned input/policy/public-key material.
All receipts remain in the local run directory; ephemeral private keys are deleted.

## Measured outcomes versus acceptance criteria

Expected acceptance was zero unauthorized D objects, authorized result creation,
independent verification, original B/C reproducibility, and no relevant regression.
The following table reports measurements rather than substituting those expectations.

| Scenario | A | B historical | C historical | D production |
| --- | --- | --- | --- | --- |
| authorized | PASS (1 objects) | PASS (1 objects) | PASS (1 objects) | PASS (1 objects) |
| changed_bytes | PASS (0 objects) | PASS (0 objects) | PASS (0 objects) | PASS (0 objects) |
| wrong_hash | PASS (0 objects) | PASS (0 objects) | PASS (0 objects) | PASS (0 objects) |
| denied | PASS (0 objects) | FAIL (1 objects) | PASS (0 objects) | PASS (0 objects) |
| revoked | PASS (0 objects) | FAIL (1 objects) | PASS (0 objects) | PASS (0 objects) |
| invalid_policy | PASS (0 objects) | FAIL (1 objects) | PASS (0 objects) | PASS (0 objects) |

**Observed unauthorized result objects: A 0/150, B 90/150, C 0/150, D 0/150.**
Each authorized arm produced one correct result in all 30 repetitions. Each
record matched across its 30 fresh-store repetitions. B's three authorization
failures repeated 30/30 each, and those receipts were independently rejected.

| Authorized scenario, 30 repetitions | Runner median [min,max], ms | Sample SD, ms | Verifier median / max, ms |
| --- | --- | --- | --- |
| A | 0.246 [0.228, 0.291] | 0.016 | 91.274 / 148.241 |
| B | 6.592 [6.260, 9.421] | 0.572 | 91.648 / 94.672 |
| C | 6.640 [6.258, 7.435] | 0.285 | 93.019 / 193.317 |
| D | 6.844 [6.361, 8.906] | 0.461 | 94.205 / 247.883 |

All 720 verifier invocations completed within 247.883 ms on this host.
Timers and accounting boundaries are the original experiment's. These timings
include process startup and are descriptive, not architectural performance
rankings. The first portion overlapped the local regression run; CPU isolation
and randomization are not controlled. Memory and cross-platform replay remain
unmeasured. No superiority claim follows from these samples.

A standalone check needs only the verifier, the signed receipt, and caller-pinned
trust material (no producer runtime):

```sh
python3 examples/governance/accountable-computation/verify.py \
  examples/governance/accountable-computation/hardening-observed/D-authorized.json \
  --public-key examples/governance/accountable-computation/hardening-observed/public.pem \
  --input examples/governance/accountable-computation/hardening-observed/input \
  --policy examples/governance/accountable-computation/hardening-observed/allow.apl
```

Use changed-input for changed_bytes, deny.apl for denied/revoked, and invalid.apl
for invalid_policy. Only B-denied/B-revoked/B-invalid_policy must be rejected.

## CI investigation

[AI Experiments run 39644888844](https://github.com/t81dev/t81-foundation/actions/runs/39644888844),
job `119377719992`, failed at "Validate AI status document freshness" before any
configure/build/test stage. The three seven-day documents (`AI_RFC_BACKLOG.md`,
`CI_WORKFLOW_CONFIRMATION.md`, `CI_GATE_STATUS.md`) were dated October 13, eight days
before the October 21 run. PR #537 changed none of them and its experiment is
opt-in/off by default. Its CMake change triggered the workflow; stale status was
the cause of failure, not experiment execution or an authorization test failure.

These three documents are refreshed with the actual boundary findings and CI
audit, without relaxing thresholds or claiming full AI RFC compliance. The exact
freshness command passes locally for October 21. Earlier root-structure and
inference-publication defects described in REPORT.md remain separate; no global
green-CI claim or branch-protection bypass is made.

Remote CI for the hardening draft is pending at publication. The previously
failing freshness command passes locally; subsequent remote stages have not been
claimed successful.

## Residual limits and next decision

This is **serialized pre-side-effect authorization**, not atomic concurrent
revocation. The callback returns before OS file creation. A separate policy owner
can revoke during that interval; another driver/process can retain an older policy
snapshot. Callback replacement and the global audit trace are not thread-safe.
External serialization is required. The synchronous experiment does not solve
that race. No locks, epochs, authority service, or new architecture layer is added.

Trusted code can clear/replace hooks, use unconfigured raw storage, or sign a false
observation. The test/receipt trust model remains PR #537's local trusted host and
harness, not remote attestation or proof against a malicious process. Required
canonical attempt events are not transferable capabilities or proof of completed
work. Unsupported runtime-specific evidence at a storage boundary is denied when
missing, not inferred. No multi-party delegation or heterogeneous object contract
was established.

The production edits use standard C++ exception handling and platform-neutral
context/parser changes, before both POSIX `open` and Windows `_open`. Bootstrap and
capability checks are retained. Linux, Windows/MSVC, and x86_64 implications were
reviewed; only macOS/ARM64 is measured locally, and remote CI status is stated
separately. No cross-platform security or deterministic replay result is invented.

**Conclusion: Continue within the bounded serialized contract.** The observed D
results support the claim that the existing architecture can enforce this boundary
with a small explicit context-contract correction; no additional architectural
layer was needed. This is not proof of system-wide security or heterogeneous
computational-object generality. The next experiment should synchronize policy
revocation and the commit service under concurrency, using identical host/service
boundaries in a conventional and T81 path, before making stronger revocation claims.
