# Investigation 08 corrective review — October 27, 2026

Review input: PR #602 at `7972f7ecbcdec9e0f04bd1bba0103e84d3df375e`;
base `c930b8c2ee7478782bf3c6d231bdd59984595ef2`. The original automated review
covered `d4a86de729ca515228d5b668593c2d0da7b149e4`. The subsequent `fde78d40`
CI/README/radix/results commit and merge commit did **not** repair its five
comparator findings. They were reassessed against actual latest-head source,
not assumed open from old review comments.

Verdict: **APPROVE the bounded corrective implementation**. The seven-method gate
and declared smoke observations completed. No Investigation 08 closure or governance
promotion is made. Architectural performance and assurance superiority remain
**INCONCLUSIVE**, regardless of the bounded gate outcome.

## Five original findings

| Finding at review input                                                | Severity / reproduction                                                                                                                                                                             | Correction and acceptance evidence                                                                                                                                                                                                                            |
| ---------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Projection orientation (`baseline_stack.py:417–419`, also layer 1)     | High: input × transposed weights; production `native_tensor_twmatmul_direct` indexes input dimension first. Nonsymmetric valid matrices distinguish the operations even if a label stays unchanged. | All six projections index `i * hidden_dim + j`. Three valid seeded nonsymmetric weight mutations compare nine intermediate tensors and decoder candidate scores against production operations.                                                                |
| Missing activation snapping (`baseline_stack.py:438`, projections too) | High: latest-head demo selected score independently reproduced as **5490**, whereas corrected reference arithmetic gives **16**.                                                                    | Snap at both projection layers and final accumulation. Compare exact candidate vectors, not just labels.                                                                                                                                                      |
| Lossy binary CAS hashing (`baseline_stack.py:171`, `get` too)          | High: latest-head `put(b'\xff') == put(b'\xfe')` reproduced; substitution passes its old reader.                                                                                                    | Hash original bytes. Distinct references, exact round trip, corrupted existing-object refusal, and tamper rejection; cross-check binary identity and fresh-process tamper rejection with production CanonFS.                                                  |
| Incorrect model retrieval (`baseline_stack.py:551–552`)                | Medium: latest-head override-free replay of its own bundle reproduced as `valid: false`; full SHA3-512 hex is not the model CAS key.                                                                | Persist actual model/input/policy/tokenizer references; recover all four from a new runner/process with no overrides. Verify fingerprints and cross-artifact links before rerunning; reject hash-valid inconsistent evidence.                                 |
| Reused empty trial roots (`ablation_runner.py:137–140`)                | Medium: deterministic directory names with `exist_ok=True` reuse a previous invocation's populated stores; populated trials also lacked explicit priming.                                           | Unique invocation, observation and arm directories; assert empty initial state, explicitly prepopulate every populated observation. Regression exercises two invocations against the same work directory and checks disjoint roots and expected entry counts. |

The heading-API names and line numbers above refer to the review-input commit;
current code/tests are the corrective evidence. The independent initial reproduction
is preserved in the review evidence packet, not represented as a benchmark pass.

## Additional correctness and evidence repairs

- Initial-probe selection is `max_score.v1`, keeping the first sampled candidate on
  ties, not the separate greedy API's lowest-token tie rule. A wrapped candidate
  window tests this distinction. Context truncation follows the bounded reference
  history rather than summing every historical token.
- The expanded gate found an additional **BPE/tokenizer lexical mismatch**. The
  first full gate failed on a valid escaped-tokenizer scenario and generated no
  measurements. The reference's small reader removes escape backslashes without
  Unicode decoding; the comparator now preserves original tokenizer text and
  reproduces that behavior. This is a documented reference limitation, not a
  claim of complete JSON or tokenizer conformance.
- Policy syntax is parsed, duplicate model entries are retained for one-hash
  admission checks, unsupported clauses fail explicitly, and denial precedes
  object persistence. Malformed models and policies are compared against the CLI;
  not all negative tests are merely baseline-only tests any longer.
- The opt-in C++ probe compiles the production embedding/projection/attention
  operators and captures tensor values at their producing VM instructions.
  Candidate scores/IDs also come from the actual decoder and CLI provenance. It
  does not infer hidden state from the last entries in a mutable tensor pool.
  Exceptions/checks remain active with the repository's global NDEBUG policy.
- Result serialization matches the CLI byte for byte. Backend-specific provenance
  and recovery bundles intentionally have different contents and references;
  each reference is independently checked against its own bytes and linkage.
- Benchmark runs execute the full gate before trials, use fresh processes for
  both implementations, record raw seeded order/counts/roots and separate available
  phases, and refuse to overwrite an existing output. No result is hard-coded.
  T81 internal phase clocks are unavailable and recorded as null, not fabricated.

## Scope audit

Removed the unrelated Windows sparse-checkout edits, radix-comparison C++17 edit,
and root README edit. The final PR has no workflow or root README difference from
its base. Retained the Investigation 08 work-directory ignore entry. The only new
root CMake surface is `T81_BUILD_INVESTIGATION08_PROBE`, default OFF, justified by
the required intermediate differential evidence; it changes no production operation,
ISA, compiler policy, governance rule or activation record. An OFF configuration
was verified to exclude `parity_probe.cpp` from the generated build graph.

Earlier results at base, original #602 and review-input #602 are preserved byte for
byte in `historical/`, with source commits, SHA-256 identities and invalidation
reasons. Their earlier hard-coded SUPPORTED/NOT SUPPORTED strings and timings
remain historical assertions; they are not current scientific verdicts.

## Commands and actual outcomes

Environment: macOS ARM64, AppleClang 21, Python 3.14.3, CMake 4.3.0, Ninja. Source
and binary identities for observations are recorded in the machine-readable
packet. Default build policy still defines NDEBUG; the new probe uses explicit
errors, and Python tests use unittest checks rather than disabled C++ assertions.

```sh
cmake -S . -B /tmp/t81-pr601-review/build -DT81_BUILD_INVESTIGATION08_PROBE=ON -DT81_BUILD_EXAMPLES=ON
cmake --build /tmp/t81-pr601-review/build --parallel 6 --target t81 t81_make_assess_fixed_demo t81_investigation08_probe
T81_BUILD_DIR=/tmp/t81-pr601-review/build python3 experiments/integrated_system_advantage/test_differential_parity.py -f DifferentialParityTest.test_original_and_normalized_inputs
T81_BUILD_DIR=/tmp/t81-pr601-review/build python3 experiments/integrated_system_advantage/test_differential_parity.py -f DifferentialParityTest.test_tokenizer_changes_and_wrapped_ties
python3 experiments/integrated_system_advantage/run_experiments.py --build-dir /tmp/t81-pr601-review/build --work-dir /tmp/t81-pr602-review/trials --iterations 3 --seed 602 --out /tmp/t81-pr602-review/observations-corrected.json
python3 -m py_compile experiments/integrated_system_advantage/*.py
python3 scripts/ci/check_tisc_freeze_integrity.py
git diff --check
```

- Configure/build: exit 0. Duplicate-static-library linker warnings retained.
- Initial intermediate-probe attempts failed explicitly; no parity or measurement
  pass was inferred. They motivated direct instruction capture instead of assumed
  tensor-pool ordering or an unsupported tensor-print operation.
- Original/unknown/normalization/context group: exit 0, ten valid scenarios,
  161.062 seconds. Focused lexical-tokenizer/wrapped-tie group after repair:
  exit 0, 36.828 seconds.
- First full corrective gate: exit 1, seven methods in 232.349 seconds, one BPE
  mismatch; no measurement trials authorized. This failed packet was preserved
  locally as `/tmp/t81-pr602-review/observations.json`.
- Final full gate: exit 0, seven methods in 249.955 seconds; all 15 valid
  scenarios compare exact intermediate values/scores, with negative and recovery
  checks passing. The experiment runner exited 0 after six paired observations
  (three per store condition, twelve measured arm launches), with unique roots,
  expected pre-observation counts and all verification/identity checks passing.
- Python compilation, frozen-ISA integrity (209 entries), OFF-configuration probe
  exclusion and whitespace checks: exit 0. Markdown formatting checked separately.

## Scientific interpretation and remaining limits

No architectural speed ranking is justified. Pure-Python loops/hashlib and a
C++/SIMD/VM/compiler stack are not equally optimized execution conditions.
Model/policy/encoding work, persistence contents and replay interfaces also differ.
Python phase timings are observable, while T81 exposes only combined CLI wall time;
its readback and rerun use separate fresh CLI processes. A T81 rerun still consumes
original fixture files; the conventional bundle recovers persisted inputs. These
are disclosed differences, not evidence that either architecture is superior.

The corpus is bounded to this 16-wide single-row slice, declared policy subset and
specified mutations. It does not prove arbitrary transformer equivalence, every
Axion constraint, model quality, malicious-host security, concurrent storage safety,
cross-platform determinism, or full native-device efficiency. Linux/Windows/x86-64
execution, full CTest, hardware measurements and missing C–F ablations were not
performed. Three paired observations per condition are a small smoke measurement;
raw values and sample dispersion must be retained, without significance claims.

The remaining engineering steps are a competent optimized conventional control,
matched persistence/replay guarantees, comparable internal phase instrumentation,
a broader preregistered independent corpus and uncertainty protocol, and valid
ablations. These are follow-up work; this PR neither expands the production runtime
nor declares Investigation 08 complete.

## Remaining scientific limitations (not concealed comparator defects)

| Location                                           | Severity / reproduction                                                                                                                                                                         | Required action before a stronger claim                                                                                                                  |
| -------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ablation_runner.py:69`                            | Medium: T81 internal phase clocks are null while Python stages are separately timed; run the documented command and inspect both arms' phase records.                                           | Retain INCONCLUSIVE; use comparably optimized conventional execution and actual T81 phase instrumentation before attributing cost to architecture.       |
| `ablation_runner.py:74`                            | Medium: the recorded T81 replay kind explicitly consumes original fixtures, whereas the conventional worker recovers CAS inputs. The two replay timings describe different recovery guarantees. | Match persistence/recovery semantics and trust boundaries before comparing replay or assurance advantage; no runtime feature expansion is included here. |
| `run_experiments.py:40` and the observation packet | Medium: bounded corpus and small declared sample count cannot establish general model/architecture equivalence or superiority. Inspect the corpus, counts and raw observations.                 | Preregister broader independent scenarios and a suitable uncertainty protocol; retain raw data and the explicit INCONCLUSIVE disposition.                |

No remaining unexplained computational difference is accepted by the gate. A new
mismatch must stop observation generation; it must not be dismissed because a
label or aggregate timing still looks plausible.

The current raw packet is `observations-2026-10-27-corrective.json`; it was copied
verbatim from the successful runner output. `results.json` records its SHA-256
identity. `observations-2026-10-27-sources.tar.gz` preserves the exact measured
Python/C++ sources and root CMake file. Source hashes inside the raw packet match
that snapshot. The final C++ probe was subsequently clang-formatted with 18.1.8
and rebuilt; this changed formatting only. No earlier measurement was overwritten.
