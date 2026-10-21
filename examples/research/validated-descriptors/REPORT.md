# Validated computational object descriptor experiment

**H2: Unsupported for this bounded workload. Recommendation: abandon adoption
of this descriptor; retain explicit providers.** A descriptor enables a single
caller dispatch point and earlier rejection than the original wrong-loader path.
But the guarantee comes from an added, trusted fixture validator and operation
allowlist. Both provider adapters remain; no existing provider validation is
removed. The conventional descriptor achieves the same predicates. Under the
assignment's falsifier, moving selection into a catalog while adding code does
not justify a shared computational-object abstraction.

This is a negative research result about the two PR #540 artifacts, not proof
that descriptors are always unhelpful. A future real consumer with multiple
callers or independently maintained provider contracts could justify another
experiment. No production API, README, VM opcode, subsystem, inference engine,
or dynamic provider mechanism changes.

Baseline: `main` including merged [PR #540](https://github.com/t81dev/t81-foundation/pull/540),
`b4860a8a3065a679c2f5f12896cb1990d1633470`. Work follows the CanonFS integrity
and deterministic runtime priority lanes. The user's original checkout was
`experiment/canonfs-concurrent-revocation`, with untracked `build-bench/`.
It was preserved. A separate checkout was clean at the pinned baseline.
GitHub's main ref resolved to that commit; its seven reported check runs were
completed/success before publication (format, AI experiment tests, EFI boot,
badge refresh, build, build-status report, deploy). No full-suite local pass is
claimed.

## Baseline audit and reuse decision

Read the baseline [report](../object-interface/REPORT.md),
[runner](../object-interface/runner.cpp), [comparison](../object-interface/compare.py),
and [recorded results](../object-interface/results.json). The originals are
unchanged. The executable comparison reruns all 36 baseline cases, including
its conventional control and detached provenance/output challenges.

| Baseline site | Representation / operation / authorization decision |
| --- | --- |
| `main`: kind validation | Caller restricts kind to `tisc` / `safetensors` |
| `main`: expected hash | Caller checks payload identity before storage |
| Storage write/read and alternate tag write | Axion allows intake; restored bytes verified; alternate object tag leaves identity unchanged and does not validate representation |
| `capability_denial` | Write-only grant denies protected read before providers |
| `unsupported` | Caller declines `infer`; no discoverable operations contract |
| `policy_denial` | Axion denies storage read; this does not establish execution authorization |
| `changed_after_verify` | Owned bytes survive original-source mutation; private snapshot for path-only loader |
| `representation` / `if (representation == "tisc")` | Scenario can swap declared kind; caller selects between two provider paths |
| TISC path | Parse nonempty Base-81 program, create policy-mediated VM, load and bounded run; check r42 |
| SafeTensors path | Invoke native loader, check tensor name/count/shape, return inventory summary |
| Result/provenance path | Caller selects `run`/`inspect` label and constructs links; detached checker validates meaning |
| Conventional baseline | Own representation switch, AST-checked Python arithmetic and fixture SafeTensors reader; read mask and exact policy allowlist |

Baseline rejected cases report zero **completed operations**, not zero attempted
provider calls. Wrong representation is rejected by the selected parser/loader;
the SafeTensors branch can enter the loader before rejection. We do not retrofit
A or misreport its counter as an invocation counter. B/C below instrument attempts
before provider entry and require zero on every rejection. Descriptor-only
malformations have no counterpart in A's trusted caller kind argument, so no
fabricated A descriptor-rejection results are presented.

Existing contracts reviewed for reuse:

| Surface | Reused / reason not to extend |
| --- | --- |
| [CanonFS refs, tags and permissions](../../../include/t81/canonfs/canon_types.hpp) | Reuse payload identity, persistent store and permission checks. `CanonMeta` holds key/value refs; `CanonLink` links a ref with a hint. Neither validates representation or discovers computational operations. |
| [Interchange manifest](../../../spec/rfcs/RFC-00D1-canonfs-interchange-manifest-schema.json) | Describes directory interchange entries, not provider operations. Do not expand the stable interchange contract. |
| [Model manifest](../../../docs/contracts/MODEL_MANIFEST_V1_CONTRACT.md) and [model import](../../../docs/contracts/MODEL_IMPORT_V1_CONTRACT.md) | Describe model intake/tensor inventory, not TISC or operation authorization. Reuse the existing native loader, not a parallel model manifest. |
| [Program](../../../include/t81/isa/program.hpp) / [weights](../../../include/t81/weights.hpp) | Reuse actual VM and model inspection, preserving their different semantics. |
| [Artifact families](../../../include/t81/cli/artifact_family.hpp) | Existing bounded result/bundle schemas; no representation/provider contract to reuse here. |
| [Model authorization helper](../../../kernel/axion/ai_model_loader.cpp) | A model-load boundary, not an automatic guard on native weights loading or a cross-provider discovery interface. |
| [Axion engine](../../../include/t81/axion/policy_engine.hpp) | Reuse final host authorization and the existing VM policy path; storage permission stays separate. |

## Three arms and encoding

A is the unchanged PR #540 explicit T81 runner (18 T81 cases), with its original
18-case conventional control retained as a baseline check. B is
[runner.cpp](runner.cpp): six-field decoding, reference binding, trusted adapter
lookup and exact fixture validation, operation discovery, final Axion check,
then invocation. C is `conventional()` in [compare.py](compare.py): immutable
Python bytes, SHA3-512, an exclusive-create directory store, the same encoding,
preflight order, operation discovery and separate exact authorization allowlist.

[ENCODING.md](ENCODING.md) precisely defines syntax, verified fields, trusted
assertions and distinct descriptor/payload identities. Line 5 is a singleton
supported-operation list; both current providers offer exactly one operation.
The profile deliberately accepts only the two existing canonical fixtures.
It does not claim general SafeTensors validation. This avoids duplicating a
production parser and keeps wrong data out of the existing path-only loader.
Broadening that profile would require fresh validation and tests.

B executes actual TISC in the existing VM and actually loads/inspects SafeTensors
using the existing native loader. C consumes **identical input bytes** for both
kinds. Its TISC provider interprets only the whitelisted four-instruction fixture
using standard-library arithmetic; its tensor reader checks the fixture inventory.
The conventional provider names are local catalog keys corresponding to the same
operations, not claims to implement T81VM or the native weights library.
This is a comparison of rejection/discovery predicates, not production feature
parity. Outputs are independently checked as `42\n` and `1:4\n`; arithmetic and
model inspection are never treated as equivalent computations. No performance
measurements or performance claims are made.

## Adversarial observations

**132 cases pass: 36 unchanged baseline + 96 descriptor cases.** B and C each run
24 scenarios on both payloads. Each has 4 successful cases (valid and source
mutation for both kinds) and **44 rejections, all with zero attempted provider
invocations and no output file**. Raw descriptors, input bytes, expected pins,
hashes, observed statuses and source/binary fingerprints are in
[results.json](results.json). Persistent `.blk` payloads and restored bytes are
checked independently of reported status. B/C rows must agree field for field.

| Required challenge | B and C observation, for each kind |
| --- | --- |
| Valid TISC / valid SafeTensors | `ok`; discover `run` / `inspect`; one invocation and correct output |
| Incorrect payload reference | `payload_denied` |
| Incorrect representation (registered opposing representation/provider pair) | `representation_denied` before either provider is entered |
| Unsupported requested operation | `unsupported`, with valid supported operation still discoverable |
| False provider-declared capability (`infer` also requested) | `capability_denied`; a self-consistent descriptor hash cannot add an operation |
| Payload substitution with freshly rehashed descriptor | `payload_denied` against independently pinned caller payload |
| Descriptor substitution with stale expected descriptor ref | `descriptor_denied` |
| Unsupported version / malformed encoding | `version_denied` / `malformed` |
| Source mutation after verified intake | Original output; source changed; stored/restored bytes retain original identity |
| Storage succeeds, operation denied | `operation_denied`; verified read succeeds, supported operation is known, Axion final authorization denies B; C exact allowlist denies |
| Write-only storage grant after intake | `storage_denied`; no invocation despite retained local bytes |

Additional challenges: unknown provider/constraint, missing/extra fields, invalid
hex reference, missing LF, empty field, duplicated operations, CRLF, non-ASCII,
oversized descriptor, and corrupted payload with a freshly valid content ref.
The last proves hash validity does not validate representation. All reject before
provider invocation. A retains its existing rejection behavior and provenance
checks; its wrong-hash case stores nothing, whereas B/C intentionally allow
byte intake before metadata rejection. That ordering difference is not evidence
of stronger storage admission.

## Integration cost and extension analysis

Counts are a manual structural audit, recorded in `structural_audit` in the
results. A caller dispatch decision means the useful-provider selection switch,
excluding test attack generation, guards and result-label formatting.

| Metric | A explicit T81 | B descriptor T81 | C conventional descriptor |
| --- | --- | --- | --- |
| Caller provider-selection decisions | 1 (`if/else`, two paths) | 0 (one validated function-pointer call) | 0 (one validated callable) |
| Provider-specific adapter paths | 2 | 2 | 2 |
| Extra descriptor decoding paths | 0 | 1 | 1 |
| Added representation preflight entries | 0 | 2 exact fixtures | 2 exact fixtures |
| Existing provider validators removed | 0 | 0 | 0 |
| Supported-operation discovery | Caller convention | Validated singleton | Validated singleton |
| New external dependencies | — | 0 | 0 |

New executable research code is **450 lines**: 184 C++ and 266 Python, including
all challenge generation, baseline reuse, independent checks and measurement
output. Build integration adds 18 lines and defaults OFF. Baseline executable
code is 352 lines (155 C++, 197 Python); these totals describe different testing
responsibilities, not equal provider-development costs. No production code is
removed. Fixture constructors duplicate the two original examples; two fixture
admission entries and two operation declarations are introduced in each control.
Existing TISC parsing and model validation still run on accepted inputs.

The trusted surface grows to include descriptor parsing, two independent caller
pins, compiled representation/provider/operation declarations, exact fixture
admission, and binding of the validated adapter to invocation. Authorization is
an additional experiment boundary, not a property bestowed by the descriptor.
No cryptographic provider attestation is added. Early wrong-loader rejection is
real but could be obtained by the same preflight checks in explicit branches.
One caller does not demonstrate reduced duplication across integrations.

Static third-representation exercise: A needs another caller branch and adapter.
B needs another provider function, catalog entry (including the array bound),
validated payload rule, and tests; `main` need not change. C needs another callable
and dictionary entry. The frozen two-fixture constraint profile also needs a new
profile and resolver acceptance rule in either descriptor arm. Thus common
invocation can stay stable, but extending validated representations is not
zero-touch common validation. No speculative third provider was implemented.

## Reproduction and limits

```sh
cmake -S . -B build-descriptor -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DT81_BUILD_OBJECT_INTERFACE_EXPERIMENT=ON -DT81_BUILD_DESCRIPTOR_EXPERIMENT=ON \
  -DT81_BUILD_TESTS=ON -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_BUILD_FUZZ_TESTS=OFF -DT81_BUILD_TUI=OFF -DT81_ENABLE_TERNARYOS=OFF
cmake --build build-descriptor --target t81_descriptor_comparison t81_object_comparison
ctest --test-dir build-descriptor --output-on-failure \
  -R '^(validated_descriptor_comparison|object_interface_comparison)$'
python3 examples/research/validated-descriptors/compare.py \
  --runner build-descriptor/t81_descriptor_comparison \
  --baseline-runner build-descriptor/t81_object_comparison \
  --out /tmp/t81-descriptor-evidence
```

Evidence directory must be new. Do not use Python `-O`: the unchanged baseline
uses assertions and the new driver refuses optimized mode. Network-restricted
local configure reused the existing Asio source via
`-DFETCHCONTENT_SOURCE_DIR_ASIO=/path/to/asio-src`; ordinary configure obtains
that existing dependency normally. No new dependency is required.

Validation: fresh Apple Clang 21 Release build on macOS ARM64; 6/6 targeted
CTests passed: both comparisons, CanonFS identity and policy boundary, native
weights load, and VM predispatch denial logging. C++ formatting and whitespace
checks passed. Existing duplicate-library linker warnings occurred on baseline
production test targets. Remote checks for this draft are separate from the
baseline checks listed above.

Portability review: binary streams and `Path.write_bytes` preserve LF and raw
payload bytes on Windows; paths use `std::filesystem`/`Path`; no POSIX calls,
symlinks, shell-driven orchestration, or new compiler intrinsics. New fixture
length encoding is explicitly little-endian. Existing native model loader byte
order assumptions are unchanged. Linux x86_64 and Windows x86_64 execution are
unmeasured; macOS ARM64 is measured. No cross-platform pass is inferred.

One trusted process owns a private store and model snapshot path; capability
updates and global Axion access are serialized. Expected hashes/policies and the
compiled catalog are trusted. Records are unsigned, not authenticated provenance.
Hostile replacement of the private snapshot, arbitrary program safety, concurrent
revocation, crash durability, general format recognition, inference, and a common
execution interface are outside this experiment. The exact-fixture restriction
also limits what this result can establish about future real workloads.
