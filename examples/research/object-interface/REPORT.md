# Computational object interface comparison

**Decision: Partially supported.** T81 demonstrates useful representation-independent
byte storage, identity, integrity checking, and policy/capability checks on storage
operations. This experiment does **not** demonstrate an integration advantage from
a common computational-object interface. Representation discovery, operation
support, execution, and meaningful result/provenance verification still require
separate contracts or experiment adapters. A storage reference is not a general
computational-object interface.

Baseline: `main` after merged PR #539,
`8a6dce4831bd84e23d499fa71900e959ba940875`. This work follows the CanonFS
hardening and deterministic runtime priority lanes; it changes no production
CanonFS, Axion, VM, model loader, or public contract.

## Hypothesis and current contracts

Strong hypothesis: existing public APIs let these two objects expose identity,
representation, supported operations, authorization, and verifiable invocation
without callers inventing an operation registry or translating every provider.
Falsifier: one of those steps needs a caller-owned representation/operation
convention. That falsifier is observed. The narrower shared-storage hypothesis
survives these tests; no cross-representation computational equivalence follows.

| Surface inspected | Actual contract and maturity | Evidence |
| --- | --- | --- |
| CanonFS identity/storage | Implemented and tested: `Driver` writes/reads payloads by `CanonRef`; content hash ignores `ObjectType`. `CanonExec`/`CanonTensor` enum tags do not validate formats or provide discovery. | [Driver](../../../include/t81/canonfs/canon_driver.hpp), [types](../../../include/t81/canonfs/canon_types.hpp), [identity tests](../../../tests/cpp/canonfs_identity_contract_test.cpp), persistent/in-memory drivers |
| Capability declaration | Implemented storage permission masks, not declarations of computational operations or resource requirements. No generic capability-inspection method on `Driver`. Persistent driver has bootstrap access when no grants exist; grants here are host-installed masks, not authenticated principals. | [driver tests](../../../tests/cpp/canonfs_driver_test.cpp), [persistent driver](../../../fs/persistent_driver.cpp) |
| Axion boundary | Implemented and tested fail-closed hook on persistent read/write/publish/revoke/repair; VM has its own per-instruction policy boundary. External serialization required. A store read denial is not a universal execute denial. | [hook](../../../kernel/axion/canonfs_hook.cpp), [boundary tests](../../../tests/cpp/canonfs_policy_boundary_test.cpp), [VM denial tests](../../../tests/cpp/vm_predispatch_policy_deny_logging_test.cpp) |
| Program load/run | Implemented public `Program`, Base-81 render/parse, `load_program`, bounded `run_to_halt`, VM state. Program fields include literal pools and embedded policy, but no common object-operation declaration. | [Program](../../../include/t81/isa/program.hpp), [VM API](../../../include/t81/vm/vm.hpp), [serializer](../../../isa/base81_view.cpp) |
| Model intake | Implemented, tested, narrow review-oriented loader/report and manifest contracts. SafeTensors loader imports a tensor inventory/native weights; some formats undergo quantization. This is not inference. Model metadata has no common executable capability contract. | [weights API](../../../include/t81/weights.hpp), [loader](../../../tools/model/weights.cpp), [import contract](../../../docs/contracts/MODEL_IMPORT_V1_CONTRACT.md), [manifest contract](../../../docs/contracts/MODEL_MANIFEST_V1_CONTRACT.md), [load tests](../../../tests/cpp/weights_load_test.cpp) |
| Model authorization helper | Existing `load_model_via_tloadhash` evaluates a model-load request; its caller must obey denial and perform loading. It is not automatically invoked by `load_safetensors` and is not a generic object dispatcher. | [implementation](../../../kernel/axion/ai_model_loader.cpp) |
| Provenance/result validation | Existing interchange and bounded AI artifact schemas/validators are implemented/tested for their families. Generic byte storage neither validates provenance nor proves correctness. | [artifact family](../../../include/t81/cli/artifact_family.hpp), [record validation tests](../../../tests/cpp/artifact_validate_record_test.cpp), [interchange tests](../../../tests/cpp/canonfs_interchange_engine_test.cpp) |
| Broader common object architecture | Research/specification direction, not an implemented generic contract established by these APIs. RFC-00D1 is partially implemented; RFC-00D0 remains design-led and irrelevant to this dispatch. | [README research questions](../../../README.md), [handoff](../../../docs/HANDOFF.md) |

The source/test audit establishes implemented versus tested surfaces; it does not
claim every referenced test was rerun. Local checks are listed below.

## Objects and operations

`runner.cpp construct` creates a four-instruction TISC program via the public
Base-81 renderer: load 6, load 7, multiply, halt. It parses the same verified
bytes and executes them in T81VM, observing register 42 as `42`.

The second object is a generated, padded SafeTensors header plus four I8 values
`[-1, 0, 1, -1]`, shape `[2,2]`, tensor `weight`. The existing
`load_safetensors` path actually imports native tensor data and returns one
tensor/four parameters. The captured `1:4` is an inventory summary, **not a
model computation**. Neither inference nor execution-provider replacement is
tested; no model download or new engine is involved.

| Operation | Both objects? | Existing interface or additional work |
| --- | --- | --- |
| Construct/import | Yes | Representation-specific: `Program`/render versus SafeTensors construction/loader. Common storage intake is `write_object`. |
| Establish identity | Yes | Shared CanonFS payload hash, independently recomputed with Python SHA3-512 truncated to 32 bytes. Hex evidence renders the same hash bytes, not the usual Base-81 ref string. |
| Read/export exact bytes | Yes | Shared `read_object_bytes`; Python compares restored bytes and independently reads persistent `.blk` files. |
| Inspect representation/type | Yes, differently | Caller selects loader. Program instructions and model tensor inventory are separate contracts. Wrong loader rejects these fixtures; storage accepts either type tag. No general format recognizer is added. |
| Inspect declared operations/capabilities | No common API | Neither fixture declares generic operations. Storage masks are enforced but not discoverable computational capabilities. |
| Determine operation support | Adapter only | The experiment declines an unprovided `infer` operation for both. `adapter_unsupported` is a harness decision, not a production capability-discovery result. |
| Authorize protected storage read | Yes | Same Axion hook and native permission-mask check. Denial occurs before provider invocation. Intake is allowed first; this is read denial, not zero intake side effects. |
| Invoke useful operation | Different | VM execution with its policy engine versus model import/inspection. Shared read access does not authorize every host action; model intake itself is not automatically Axion-mediated. |
| Store result/provenance bytes | Yes | Same CanonFS operations, but linking/formatting the experiment record is caller work. |
| Validate meaning/completeness | Adapter only | Detached Python checks the exact experiment record and known expected output. No universal result/provenance validator is demonstrated. |

Identical bytes imply equal payload identity here even with different type tags.
Different encodings have different content identities. A logical identity across
formats is not supplied by this experiment; equivalent computation is checked
only for the fixed arithmetic task. Model inspection and arithmetic outputs are
never compared as equivalent workloads.

## Conventional comparison

The standard-library conventional implementation is `conventional()` in
`compare.py`: immutable Python bytes, SHA3-512, an exclusive-create directory
store, explicit read/write masks, an exact policy allowlist, two separate
providers, and the same evidence format/checker. Its executable is Python source
`result = 6 * 7`, compiled/executed only after an AST check against the trusted
fixture. Its SafeTensors reader is deliberately fixture-scoped; it validates the
header/payload and reports the same inventory, not a replacement general loader.

Integration comparison: both arms require a provider branch, known operation
support, a binding from verified bytes to loader input, and caller-built
provenance. T81 reuses a much stronger existing storage/policy implementation and
a full model loader/VM; the conventional arm's policy and parsers have only this
fixture's semantics. Their production feature sets and development costs are
therefore not equal. T81 centralizes storage authorization and explicit errors;
it does not eliminate representation adapters in this workload. The same
independent output/hash/file checks succeed in both arms, so this evidence does
not establish an exclusive correctness or provenance advantage. No timing or
unrelated-workload performance ranking is made.

## Adversarial results

Each row runs for both kinds in both arms: **36 cases** total. Full per-case
hashes/statuses and environment are in [results.json](results.json). Persistent
object bytes are observed independently of the runner's status report and saved
as hex payloads in the results. `completed_operations` excludes rejected parser
attempts. Inspected representation metadata remains provider-specific.

| Case | Observed outcome in both arms | Boundary established |
| --- | --- | --- |
| Authorized | `42` for program; `1:4` for tensor intake | Provider-specific expected result, three stored payloads |
| Wrong declared representation | Parser rejects opposite representation | Adapter parser validation; type tag alone remains unchecked |
| Mismatched expected hash | Rejected, no store payloads | Caller-supplied expected identity checked before import |
| Unsupported `infer` | Adapter declines; zero provider invocations | No evidence of shared production support discovery |
| Write-only storage capability | Protected read denied; only input stored | Existing mask prevents read, not a computational-operation declaration |
| Axion denial / conventional deny policy | Protected read denied; only input stored | No result/provenance produced; both checks precede provider invocation |
| Source changed after verification | Original snapshot produces original result; source hash differs | Owned snapshot binds use to verified bytes within the trusted harness |
| Invalid provenance output link | Store accepts; detached semantic verifier rejects | Integrity-valid bytes can contain false linkage |
| Incomplete provenance | Store accepts; detached completeness verifier rejects | Content addressing does not impose a provenance schema |

Additional verifier challenges append data with the old record hash, supply an
incomplete record with a recomputed valid hash, and change expected result bytes.
All reject. Verification is distinct from successful persistence.

## Trust assumptions and limits

One trusted process owns each fresh persistent store; operations, capability
changes, and global Axion trace access are serialized. The model loader's
path-only API requires a private snapshot file. The experiment assumes no actor
can rewrite that private path; it does not solve concurrent hostile filesystem
replacement. The original mutable source is never reopened for provider use.
Existing read verification must be enabled (`T81_CANONFS_READ_VERIFY` unset or
enabled). Native capability bootstrap and trusted host authority remain intact.

The expected input reference, kind, operation, allow policy and expected result
are trusted checker inputs. Records are unsigned experiment evidence, not
authenticated attestations. An attacker able to replace the record, its expected
hash and all expected values can forge a self-consistent story. No causal proof,
crash durability, independent-process security, inference correctness, arbitrary
program safety or broad format conformance follows.
The record's policy digest identifies storage policy, not a serialized VM policy
or complete execution-environment attestation.

## Reproduce and smallest next step

```sh
cmake -S . -B build-object -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DT81_BUILD_OBJECT_INTERFACE_EXPERIMENT=ON -DT81_BUILD_TESTS=OFF \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_BUILD_FUZZ_TESTS=OFF -DT81_BUILD_TUI=OFF -DT81_ENABLE_TERNARYOS=OFF
cmake --build build-object --target t81_object_comparison
python3 examples/research/object-interface/compare.py \
  --runner build-object/t81_object_comparison --out /tmp/t81-object-evidence
```

The evidence directory must be new. Omit `--out` for temporary evidence. With
`T81_BUILD_TESTS=ON`, CTest registers `object_interface_comparison`.
Do not run Python with `-O`: the experiment intentionally uses assertions.

Local validation: fresh Apple Clang 21 Release build on macOS ARM64; all 36
comparison cases and adversarial verifier checks passed. Five selected CTests
passed: the experiment, `canonfs_identity_contract_test`,
`canonfs_policy_boundary_test`, `t81_weights_load_test`, and
`t81_vm_predispatch_policy_deny_logging_test`. C++ formatting and diff whitespace
checks passed. Existing duplicate-library linker warnings were observed on
production test targets. Python uses
standard-library APIs and explicit little-endian fixture encoding; C++ uses
existing portable APIs. Linux, Windows and other architectures are unmeasured,
not inferred from the macOS result. No new platform assumptions enter production.

Smallest justified next experiment: specify a narrow, read-only descriptor that
binds an existing payload reference to **validated** representation and a provider's
explicit operation list. Test false declarations against these same two loaders
and compare the descriptor's cost with the present explicit branches. Keep that
descriptor separate from byte identity; do not add a general execution framework
or claim shared execution until a second real execution provider is compared.
