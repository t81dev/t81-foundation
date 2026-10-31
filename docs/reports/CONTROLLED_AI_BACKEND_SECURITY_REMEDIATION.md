# Controlled AI backend authorization remediation

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Controlled AI backend authorization remediation](#controlled-ai-backend-authorization-remediation)
  - [Reproduction before source edits](#reproduction-before-source-edits)
  - [Correction and demonstrated guarantees](#correction-and-demonstrated-guarantees)
  - [Registered tests and actual results](#registered-tests-and-actual-results)
  - [Environment and exact commands](#environment-and-exact-commands)
- [Before source edits; cached archives came from the unchanged merged #563 build.](#before-source-edits-cached-archives-came-from-the-unchanged-merged-#563-build)
  - [Compatibility, assumptions and unresolved limits](#compatibility-assumptions-and-unresolved-limits)

<!-- T81-TOC:END -->


Date: 2026-10-24 (America/New_York). Status: draft security correction.
Base: `f6d4d680709c5c6c98b305628e04346f5ef61620` (merged PR #563).

This change addresses the production findings in [draft audit PR #564](https://github.com/t81dev/t81-foundation/pull/564).
It follows the runtime stability and explicit policy-contract lanes in AGENTS.md.
It does not change Research Protocol 001, Axion evaluation semantics, the frozen
ISA, or unrelated VM execution. It does not establish consent-governed inference
or real external-effect security.

## Reproduction before source edits

Before changing any source, recompiled the original audit probe from
`4334801124042989120a0bb93278c3375ebbfa0e` against the unchanged merged backend
archive built during the #563 audit. All six documented failures reproduced:
Deny, Defer, Warn, Quarantine, actual `PolicyEngine(std::nullopt)` and null engine
all returned successful simulated inference. A second operation changed the
previous evidence record. Output: [before.txt](controlled-ai-backend-security-evidence/before.txt).
The original probe's exit 0 means **unsafe behavior reproduced**, not a passed
security test. This used cached merged-build archives, not a fresh full baseline
build. The probe source is preserved in #564 at the pinned commit above.

After the correction, the same unmodified probe returns exit 1 (its unsafe
expectations no longer hold). All six operations fail; each operation owns one
decision, caller/payload are populated, final authorization is true, and consent
remains false. [after.txt](controlled-ai-backend-security-evidence/after.txt).
The original probe's mock engine has no active policy, so its Allow case is also
rejected; the registered tests separately demonstrate a configured-policy Allow.

## Correction and demonstrated guarantees

| Boundary | Implemented behavior and evidence |
| --- | --- |
| Before dispatch | `vm/ai_backend/controlled_ai_backend.cpp:144-186,214-221` requires an active engine-owned policy and an exact Allow. Deny, Defer, Warn, Quarantine, unknown enum values, absent engines/policies and evaluation exceptions cannot enter either private dispatch routine. |
| Axion final authorization | Sets `final_authorization=true`, retains Nop rather than fabricating a VM instruction, and supplies the engine-owned policy via the const borrowed accessor in `include/t81/axion/policy_engine.hpp:26`. Actual missing-policy and required-Axion-event denials exercise the existing final-evidence checks. Engine evaluation code is unchanged. |
| Request context | Caller is the wrapper identifier `controlled_ai_backend`; existing syscall `ai.inference.inference` is retained. Payload is a versioned SHA3-512 request identity covering model reference, token count, temperature bits, input text, dispatch mode and route/target. Caller is descriptive, not an authenticated principal. No invented VM trace evidence or execution counters are supplied. |
| Consent | `AIBackendConfig.require_user_consent=true` and the constructor default block even a policy Allow with `ERR_AI_CONSENT_UNSUPPORTED`. No verified-consent contract exists. The existing false option explicitly selects policy-only operation, still subject to all authorization checks. `user_consent` stays false and consent status stays unverified in both cases. |
| Evidence | Each current operation retains the exact optional Axion verdict (kind name, numeric value and reason), request/caller/syscall context, final-authorization flag, active-policy presence, policy authorization, dispatch occurrence and failure reason. No verdict is fabricated when evaluation does not return one. Earlier evidence is not changed by later decisions. |
| JSON | Quotes, backslashes and control bytes in verdicts/model references/errors are escaped with a small shared string encoder. The registered Python test parses the output and verifies exact reason/context round trips. This is not JCS or UTF-8 validation. |
| Direct API | `src/direct_api/t81_direct_api.cpp:36-63,125-132,176,199` resets the old backend before reinitialization, rebuilds an engine from a validated cached policy on cache hits, rejects empty initial cache keys, and rejects use before successful initialization. It calls the same backend gate and propagates denial without creating result/provenance references. There is no Direct API consent opt-out. |

Both dispatch routines still synthesize responses. `inference_dispatched` is set
inside those routines, not inferred from authorization. Negative tests additionally
require no output tokens/text and no dispatch-only `ai_service` metadata.
A temporary mutation that disables the gate causes the test to fail with
`FAIL: failure must not return inference output`; the production gate was not
changed for this check. [mutation.txt](controlled-ai-backend-security-evidence/mutation.txt).

The request digest uses decimal byte lengths followed by `:` and the exact bytes
of each field, in this order: `t81.controlled-ai.request.v1`, model reference,
decimal token count, decimal uint32 temperature bit pattern, input text, effective
mode, route/target. External route is `external_simulated:` plus endpoint; local
route is `local_controlled`. This separates local and external identities even
if an endpoint text equals the local route marker. Authentication tokens and raw
prompts are not copied into evidence. The digest records context; the existing
Axion engine is not extended with new per-model or endpoint ACL semantics.

## Registered tests and actual results

The three new CTests are registered after root `enable_testing()` in
`CMakeLists.txt:1349-1361`. New C++ checks use explicit runtime failures, not
`assert`, so NDEBUG cannot disable them.

- `controlled_ai_authorization_test`: 30 non-Allow combinations (four known
  failure kinds plus unknown kind, across strict/controlled/permissive modes and
  local/external configuration), 12 missing-policy/engine combinations, a mock
  Allow with no configured policy, real final-evidence denial, standard/unknown
  exceptions, blocked required consent, real configured-policy Allow for local
  and external simulations, per-operation isolation and request/mode/target binding.
  Negative matrix cases explicitly use policy-only mode, preventing consent
  rejection from hiding a broken authorization gate. Positive tests exercise the
  real Axion engine with an explicitly configured permissive `Policy{}`; they
  demonstrate the wrapper's gate, not application-specific policy sufficiency.
- `direct_api_authorization_test`: generated tiny model and policy fixtures;
  uninitialized use, empty cache keys, actual final-authorization denial,
  consent blocking, cache-hit reinitialization, batch rejection and invalidation
  after failed model/policy initialization. No user model or fixed /tmp policy required.
- `controlled_ai_evidence_json_test`: exact Quarantine kind/numeric value/reason,
  context and model-reference round trip through JSON; no raw prompt in output.

All three and four existing Axion/CanonFS regression CTests passed: **7/7**, 0.43 s
in the final run. Output (trailing whitespace removed): [ctest.txt](controlled-ai-backend-security-evidence/ctest.txt).
Existing tests: axion_ai_hooks_test, canonfs_policy_boundary_test,
t81_vm_ai_predispatch_deny_audit_test, axion_policy_conformance_matrix_test.

A standalone optimized NDEBUG build of the corrected backend and unit test also
passed: [optimized-test.txt](controlled-ai-backend-security-evidence/optimized-test.txt). An AddressSanitizer/UndefinedBehaviorSanitizer build of that backend and
test passed with no diagnostics. [sanitized-test.txt](controlled-ai-backend-security-evidence/sanitized-test.txt).
Those standalone checks link the existing Axion/core archives: they are not a
fully sanitized or fully rebuilt Release VM test suite.

The existing AI experiment-boundary and backend-adapter contract scripts passed.
`git diff --check` passed. The focused `-Wall -Wextra -Werror -Wswitch
-Wunused-variable -Wunused-function` syntax scan passed for the changed source
and C++ regression tests. The normal CMake link still reports pre-existing
CLI/decoder duplicate-archive warnings. An additional, stricter `-Wconversion`
probe exposes six pre-existing random-seed/float conversions in this backend;
no warning suppression or unrelated numeric rewrite is included.

## Environment and exact commands

macOS ARM64; Darwin 25.5.0; Apple Clang 21.0.0 (clang-2100.1.1.101); CMake 4.3.0;
Ninja generator; Python 3.14.3. Worktree: `/private/tmp/t81-controlled-ai-security`.
Commands below were run there; output paths are audit artifacts, not runtime APIs.

```sh
# Before source edits; cached archives came from the unchanged merged #563 build.
mkdir -p /private/tmp/t81-controlled-ai-evidence
clang++ -std=c++23 -I include \
  /private/tmp/t81-pr563-audit/experiments/research_001/audit/backend_policy_probe.cpp \
  /private/tmp/t81-pr563-build/src/direct_api/libt81_direct_api.a \
  /private/tmp/t81-pr563-build/libt81_axion.a /private/tmp/t81-pr563-build/libt81_core.a \
  -o /private/tmp/t81-controlled-ai-evidence/before-probe
/private/tmp/t81-controlled-ai-evidence/before-probe

cmake -S . -B /private/tmp/t81-controlled-ai-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DT81_BUILD_TESTS=ON -DT81_BUILD_TUI=OFF \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_ENABLE_TERNARYOS=OFF -DT81_ENABLE_DPE=OFF \
  -DFETCHCONTENT_SOURCE_DIR_ASIO=/Users/t81dev/Code/t81-foundation/build/_deps/asio-src
cmake --build /private/tmp/t81-controlled-ai-build --parallel 6 --target \
  controlled_ai_authorization_test direct_api_authorization_test \
  axion_policy_conformance_matrix_test canonfs_policy_boundary_test \
  t81_vm_ai_predispatch_deny_audit_test axion_ai_hooks_test
ctest --test-dir /private/tmp/t81-controlled-ai-build --output-on-failure --verbose \
  --timeout 30 \
  -R '^(controlled_ai_authorization_test|direct_api_authorization_test|controlled_ai_evidence_json_test|axion_policy_conformance_matrix_test|canonfs_policy_boundary_test|t81_vm_ai_predispatch_deny_audit_test|axion_ai_hooks_test)$'

clang++ -std=c++23 -O2 -DNDEBUG -I include \
  tests/cpp/controlled_ai_authorization_test.cpp vm/ai_backend/controlled_ai_backend.cpp \
  /private/tmp/t81-pr563-build/libt81_axion.a /private/tmp/t81-pr563-build/libt81_core.a \
  -o /private/tmp/t81-controlled-ai-evidence/optimized-test
/private/tmp/t81-controlled-ai-evidence/optimized-test

clang++ -std=c++23 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include \
  tests/cpp/controlled_ai_authorization_test.cpp vm/ai_backend/controlled_ai_backend.cpp \
  /private/tmp/t81-controlled-ai-build/libt81_axion.a \
  /private/tmp/t81-controlled-ai-build/libt81_core.a \
  -o /private/tmp/t81-controlled-ai-evidence/sanitized-test
/private/tmp/t81-controlled-ai-evidence/sanitized-test

clang++ -std=c++23 -Wall -Wextra -Werror -Wswitch -Wunused-variable -Wunused-function \
  -I include -fsyntax-only vm/ai_backend/controlled_ai_backend.cpp \
  tests/cpp/controlled_ai_authorization_test.cpp tests/cpp/direct_api_authorization_test.cpp \
  src/direct_api/t81_direct_api.cpp
python3 scripts/ci/check_ai_experiment_boundary.py
python3 scripts/ci/check_ai_backend_adapter_contract.py --out-dir /private/tmp/t81-controlled-ai-evidence/adapter
git diff --check
```

The repository adds `-O3 -DNDEBUG` globally even with this Debug configuration;
no assertion-based security claim is made. Asio sources were reused locally to
avoid a network fetch. No production dependency or policy language was added.
The JSON test uses only Python stdlib; T81_BUILD_TESTS now explicitly requires a
Python3 interpreter to avoid silently omitting that registered test.

The after probe uses the same compile command as before with all three archive
paths replaced by `/private/tmp/t81-controlled-ai-build/...`. Mutation check:
copy the corrected backend to `/private/tmp/t81-controlled-ai-evidence/bypassed-gate.cpp`,
replace only `if (!authorize(request, evidence))` with
`if (!authorize(request, evidence) && false)`, then compile the unit test against
that copy and current Axion/core archives with `-O2 -DNDEBUG`; expected exit is 1.
The complete source remains unchanged by the temporary mutation.

## Compatibility, assumptions and unresolved limits

**Blocking limitation for consent-governed inference:** there is no verified
user-consent contract. Default backend/factory calls and all Direct API calls
now fail closed even when policy allows. Applications must not relabel a caller's
choice of `require_user_consent=false` as consent evidence. That flag is trusted
embedder configuration for policy-only use and must not be controllable by an
untrusted workload in a consent-required service. No successful consent-governed
operation is claimed or tested.

The constructor's defaulted fourth argument preserves ordinary source call sites,
but changes its binary symbol and object layout; evidence fields are additive
and also change layout. Rebuild static-library consumers. Legacy decision strings
now preserve actual kind names instead of collapsing to allow/deny. Consumers
must use `policy_authorized` for approval and must not interpret legacy
`user_consent` as policy approval. Security decisions remain retained even when
`evidence_collection=false`; that existing flag is not an authorization bypass
or a means to disable the security record.

Only macOS ARM64 was executed. New code uses supported C++20 facilities and
portable filesystem/temp-directory tests, with no platform syscall or ISA change.
Linux x86_64/ARM64 and Windows native execution remain unverified pending CI.
No real network client, GPU/model execution, provider authentication, malicious
host resistance, concurrent/reentrant use, durable evidence delivery or signed
policy provenance is demonstrated. Evidence is in-memory, can be cleared, and is
not an immutable/signed CanonFS receipt. Allocation failure or process termination
may prevent retaining it. Context records policy presence, not a portable full
policy snapshot; request digest alone proves no policy property.

The existing engine and embedder remain trusted. There is no new authenticated
caller identity, dynamic revocation mechanism, request/model allowlist or resource
budget contract. VM-derived required evidence is not fabricated: policies needing
it deny this wrapper. Direct API caches a validated policy snapshot by path;
changing the file does not revoke an active or cached instance. Disable caching
or clear/reinitialize to load changes; cache clearing alone is not revocation.
Partial external configuration continues to select local simulation, and neither
route performs real external effects. The pre-existing arbitrary-byte/invalid
UTF-8 behavior, numeric limits and time-seeded controlled randomness have not
been certified here. Direct API placeholder hashes and synthetic decisions also
remain unsuitable as cryptographic provenance or real inference results.

The earlier audit's decoder timeout is outside this wrapper correction; it was
not claimed fixed or rerun as evidence for this change. No Research Protocol 001
readiness status changes. Keep this security PR draft for independent review;
do not merge automatically.
