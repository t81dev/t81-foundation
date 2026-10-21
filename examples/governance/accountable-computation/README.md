# First falsifiable accountable computation experiment

This is a bounded precursor to the inventory protocol, using the stable VM,
CanonFS, and Axion surfaces in the current priority lanes. It creates a purchase
order **result artifact**, never a real purchase order. It tests H1 locally:
content identity, authorization before result-object creation, and independently
checkable evidence. It introduces no public computational-object interface.

## Reproduce

Requirements: CMake with C++23 support (4.3.0 tested), Ninja, a C++23 compiler,
Python >=3.10 with
SHA3, OpenSSL >=3 with Ed25519 `pkeyutl -rawin`, Git, and network access for the
repository's pinned Asio test dependency. No Python packages are installed. The experiment is opt-in so default builds and
CI do not acquire a Python/OpenSSL requirement.
From a clean checkout of this PR:

```sh
cmake -S . -B /tmp/t81-accountable-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON \
  -DT81_BUILD_ACCOUNTABLE_EXPERIMENT=ON \
  -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_EXAMPLES=OFF \
  -DT81_BUILD_TUI=OFF -DT81_ENABLE_TERNARYOS=OFF \
  -DT81_BUILD_FUZZ_TESTS=OFF
cmake --build /tmp/t81-accountable-build --target \
  t81_accountable_experiment canonfs_axion_trace_test canonfs_read_verify_test \
  canonfs_identity_contract_test t81_vm_predispatch_policy_deny_logging_test -j 6
ctest --test-dir /tmp/t81-accountable-build --output-on-failure \
  -R '^(accountable_computation_experiment|canonfs_axion_trace_test|canonfs_read_verify_test|canonfs_identity_contract_test|t81_vm_predispatch_policy_deny_logging_test)$'
python3 examples/governance/accountable-computation/run.py \
  --runner /tmp/t81-accountable-build/t81_accountable_experiment \
  --output /tmp/t81-accountable-results --repetitions 30
```

Choose fresh build/results paths if they already exist. An offline CMake build
can point `FETCHCONTENT_SOURCE_DIR_ASIO` to a separately obtained checkout at
`asio-1-30-2`. The measurement output includes all individual receipts, artifacts,
caller trust inputs, public key, and `results.json`. The ephemeral signing private
key is deleted at the end. These keys attest a local test run; they are not a
production identity or trust root.

Verify a receipt using only `verify.py`, OpenSSL, the separately pinned public
key, and the caller's input/policy files (copy them to another machine if desired):

```sh
python3 examples/governance/accountable-computation/verify.py \
  /tmp/t81-accountable-results/C-authorized-0/receipt.json \
  --public-key /tmp/t81-accountable-results/public.pem \
  --input /tmp/t81-accountable-results/C-authorized-0/input \
  --policy /tmp/t81-accountable-results/C-authorized-0/policy
```

The checked-in `observed/` directory contains the first signed receipt from every
arm/scenario. To verify those without rerunning the producer:

```sh
python3 examples/governance/accountable-computation/verify.py \
  examples/governance/accountable-computation/observed/C-authorized.json \
  --public-key examples/governance/accountable-computation/observed/public.pem \
  --input examples/governance/accountable-computation/observed/input \
  --policy examples/governance/accountable-computation/observed/allow.apl
```

Use `changed-input` for changed_bytes, `deny.apl` for denied/revoked, and
`invalid.apl` for invalid_policy. Other cases use input/allow.apl.
For newly generated revoked cases pin the `revoked` policy file. The B-invalid-policy, B-denied, and B-revoked receipts
**must fail verification**: they record actual unauthorized object creation.
The CTest suite passes when it detects these known research failures, checks the
other cases, and rejects evidence attacks. A green CTest is not a passing H1 gate.

## Arms and scope

| Arm | Implementation | Added integration |
| --- | --- | --- |
| A | Python hashlib SHA3-512 truncated to 256 bits; bounded integer product; exact policy allowlist; exclusive file creation | Conventional explicit orchestration and structured receipt |
| B | CanonFS in-memory input import/read; four-instruction TISC program through real interpreter/PolicyEngine; persistent CanonFS output through existing Axion policy hook | Experiment harness connects existing APIs and collects observations |
| C | Same T81 path | Validate policy, request final evaluation at the driver callback, require explicit Allow |

The input bytes are `25 1000\n`; result bytes are `25000\n`. Integers are limited
to [1,1000] so the multiplication cannot encounter the arithmetic-overflow defects
under review in PR #536. Identity is the first 32 bytes of SHA3-512 (not SHA3-256),
matching CanonHash81. CanonFS output names use the existing Base81 encoding; the
conventional arm uses hexadecimal. The detached verifier independently checks both.

The defined prohibited side effect is creating a result `.blk` file in a fresh,
otherwise empty output store. Input intake, arithmetic, directory creation, and
harness audit files are outside that boundary. The harness reads every `.blk`
after execution; denial requires zero result objects. It also checks the returned
status and records policy-callback counts. It does not infer non-execution solely
from a denial string. A policy-denied request may compute its proposed result.

Authorization is a single-use local commit permission, represented by an allow
policy or an unmet Axion event requirement. No spending-rule interpreter, agent,
capability delegation, or concurrency subsystem is added. Revocation is an explicit,
synchronous barrier after calculation and before opening the output driver. It is
not a concurrent TOCTOU or prompt-injection experiment. Invalid policy is supplied
as bytes from a policy file, exercising the existing parser failure behavior.

Both implementations test the same input, identity predicate, output bytes,
allow/deny semantics, revocation schedule, side-effect observation, signature
format, and independent verifier. The existing hook skips required-event checks at its Nop context. C uses the
existing PolicyEngine with `next_opcode=Halt` to request final policy evaluation
at the commit boundary. Halt is the engine's existing final-evaluation marker;
no additional VM instruction executes at that storage boundary. This adapter
convention is an integration limitation, not a new policy API.
C also rejects invalid policy before constructing a driver and accepts only an
explicit Allow verdict. A has an exact allowlist for this small policy
vocabulary; it does not implement the Axion language. Its behavior for arbitrary
Axion documents is outside the comparison. End-to-end timings include different
runtime/startup paths and **must not be used to rank performance**.

## Evidence and replay contract

Receipts sign the entire `record` with Ed25519, including method, input, policy,
expected identity, returned status, and observed object bytes/names. The restricted
canonical encoding is Python JSON with sorted keys, ASCII escaping, compact
separators, and no floating point fields. This is an experiment format, not a
claim of RFC 8785 or RFC-00D4 conformance. The verifier does not import the producer,
load T81, contact a server, or accept a key URI from untrusted evidence. It checks
caller-pinned inputs/policy/key, recomputes hashes, applies the finite policy
vocabulary, replays integer multiplication, and checks content-addressed output.

A signature authenticates the harness's observation, not a proof of runtime
execution, time ordering, or absence of unreported writes. Direct filesystem
inspection provides the tested side-effect evidence; a malicious harness, signer,
host, or out-of-band writer is outside this experiment's trust boundary. Denial
receipts are checkable records of denial, never authorization for an action.

Replays require the same bytes, method, active policy at the barrier, clean stores,
trusted executable, single process/thread, and no concurrent mutation. Repetitions
compare complete records (excluding signatures and timings). Cross-OS/architecture
identity remains inconclusive until actually run there. No platform-specific code
is introduced: paths use pathlib/std::filesystem, and cryptography uses OpenSSL;
Windows executable paths must include `.exe`. Linux/x86_64 and Windows have not
been measured in this delivery. The checked-in observation is macOS/ARM64 only.

See [REPORT.md](REPORT.md) for protocol conflicts, reviewed PRs, observations,
falsification gates, limitations, and the next experiment.
