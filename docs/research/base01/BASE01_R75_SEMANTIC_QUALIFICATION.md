# BASE-01 VM R75 semantic qualification

**Disposition: R75 SEMANTICS ESTABLISHED**, within the exact six-builder,
fresh-single-context, successful-explicit-Halt domain specified below.
This establishes a software-count interpretation, not measurement readiness or
a general retired-instruction counter. No timing or performance data was collected.

Date: November 3, 2026. Author: Codex (GPT-6), also author of Lane A B0 and the
B1 design, with that prior context. This is source review and author-executed
correctness evidence, not independent scientific certification of its own code.
No coordination with another implementation or modification of PRs #687–#689.

## Fixed references and result

- Audited VM source: `cda9135d36144d4de2771af174e52ac7b30a83c9`.
- Qualification-design reference, PR #689:
  `64c3f71ff100ea16270f1a70497ac39860c1b772`.
- Historical builders/input reference, accepted B0:
  `91e3a7d7f6082b0397f8d6ef3d7ea8a95f3a929e`.
- Historical six-program input digest remains
  `86787727b57b249fb6be8285043ffd957d3efba8a2728db12f70397d4142d529`.

The [isolated probe and reproducer](../../../experiments/base01_r75_semantics/README.md)
include the historical builder definitions from the separate audited checkout.
No measured source, workload builder, frozen artifact, policy text or source pin
was changed. Additional negative fixtures are explicitly separate correctness
probes, never replacements for the six workloads. This PR does not edit #689's
matrix or silently substitute another metric. Human review may use this packet
to resolve its scoped denominator prerequisite; B1 remains unauthorized.

At successful explicit Halt, each of the six **fresh** machines publishes the
same category of count: interpreter dispatch-increment points plus post-policy
trace dispatch-increment points. All those attempts succeed in these six paths,
so final R75 equals the source-derived number of successful primary TISC opcode
dispatches, including branches and one Halt. It is not hardware instructions,
trace-entry count, outer step-call count, register-write count or a cost-weighted
operation measure. For arbitrary failure/reuse paths those equalities do not hold.

## Source-derived count and publication rules

Let `I` be the private `instruction_count_` accumulator for the current load epoch.
Let `A` count interpreter increments and `J` count trace increments after its
policy hook allows an opcode. Before wraparound, `I = A + J` following the reset.
A rejected trace hook offers a prospective count but does not increment `J`.
A rejected interpreter policy check occurs after incrementing `A`.

With no external/ordinary instruction writes to register 75, visible R75 is the
value most recently copied into that context by `sync_system_registers`; it is
**not a continuously synchronized view of I**. Publication and reset ordering
matter. Scope excludes external writes, counter overflow and multiple contexts.

| Source point | Count action | Semantic/control action | R75 publication |
| --- | --- | --- | --- |
| `load_program`, vm.cpp 369–374 | Copies prior I, then resets I to zero. | Resets State; does not clear hot spots/compiled trace map. | Fresh object starts at zero. Reused object can expose previous epoch's count until a later interpreter sync. |
| In-range interpreter dispatch, 590–620 | PC advances; I increments before recording/policy evaluation. | Opcode may subsequently execute, trap or be denied. | Not yet published; an early return can leave R75 stale. |
| Normal interpreter epilogue, 7235–7244 | No extra increment. | Synchronizes system registers, logs trace and returns trap/success. | Copies current I, including any prior trace increments. |
| Trace entry/execute, 488–516 | Policy hook receives `base_I + executed_so_far + 1`; only an allowed hook is followed by the trace increment. | Handles opcode or guard; a failed primary operation may already be counted. | No synchronization during individual traced instructions. |
| Completed/taken-branch trace return, 516–573 | Adds `ExecResult.instructions_executed` to I. | Returns from step before interpreter epilogue. | R75 stays at the last interpreter publication. |
| Guard deopt, 572–575 | Failed trace attempt is already included; fallthrough interpreter increments again. | Erases trace and retries the unexecuted opcode in the same step; retry may trap. | Published only if the retry reaches the interpreter epilogue. |
| Trace policy denial, 509–559 | Denied opcode is not added; preceding permitted attempts are added. | Returns SecurityFault. Hook's prospective count is not committed. | No interpreter sync; R75 may remain stale. |
| AxionBoundary branch, 568–569 | Would include the encountered boundary opcode after allowed hook. | Resumes at that opcode without dispatching its primary effect; step returns. | No sync at this return. This branch is not reachable from traces recordable by the pinned compiler, as explained below. |
| Explicit interpreter Halt, 1628–1630 then epilogue | Counts one interpreter dispatch. | Sets `state.halted`; advances past the real Halt and reaches epilogue. | Publishes final I. `context.halted` remains false on these successful explicit-Halt paths. |
| PC outside program, 581–588 | No increment. | Marks context/global halted without dispatching an explicit Halt. | No publication; not an acceptable completion substitute for the six builders. |
| `run_to_halt`, 7251–7275 | Bounds calls to step, not opcode dispatches. | Returns success when its loop exhausts even if not halted. | Does not force a final sync. |

Pinned sources:
[vm.cpp](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/vm/vm.cpp),
[JIT counter/guards/recorder](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/vm/jit/jit_compiler.cpp),
[R75 publication](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/vm/runtime_state_helpers.cpp),
[policy-context bridge](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/vm/policy_trace_bridge.cpp).
After default zero initialization, three sites mutate the private counter:
load reset, interpreter increment and trace-result addition. Other early-return paths may skip publication; no claim
that arbitrary faulted R75 snapshots expose the private total is made.

### Guards and Axion side exits

`ThreadedJitTrace::execute` invokes the hook before increasing its local count,
then increments before the opcode switch. Guard failure therefore includes the
failed attempt. The guard-deopt PC points back to the failed instruction; the
interpreter retries and can count it a second time. A failed primary operation
is not necessarily side-effect-free: the common flags epilogue can still run.
The counter is not a transactional-effect or retired-success counter in general.

The source has handlers for AxRead/AxSet/AxVerify/AxReport/AgentInvoke returning
AxionBoundary, but none is in `JitCompiler::record_instruction`'s supported list.
Recording one ends tracing without appending it. The sole ThreadedJitTrace
construction takes that recorded buffer; the
[trace-cache reconstruction](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/vm/jit/jit_trace_cache.cpp)
also goes through this recorder. No injection of custom traces into the factory
is used here. A deterministic probe checks all five opcodes alone and after a
supported Add prefix: no boundary opcode is retained. Thus AxionBoundary's
conditional source behavior is understood, but no reachable execution of that
branch is claimed for the pinned factory. Policy-heavy's AxReport executes in
the interpreter, once, without an AxionBoundary trace.

## Ledger for all six exact builders

[Historical builder source](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/tests/cpp/vm_workload_determinism_tiers_test.cpp).
All six paths have zero failed primary dispatches, zero guard retries, zero
policy denials and zero AxionBoundary exits. Rows count opcode dispatches,
including branches and Halt, not the number of changed storage locations.

| Builder | Interpreter A | Trace J | Successful dispatches / final R75 | step calls | Interpreter trace entries | Final PC |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| workload | 163 | 243 | 406 | 244 | 163 | 16 |
| micro | 5 | 0 | 5 | 5 | 5 | 5 |
| meso | 157 | 93 | 250 | 188 | 157 | 10 |
| mixed | 11 | 0 | 11 | 11 | 11 | 11 |
| policy-heavy | 7 | 0 | 7 | 7 | 7 | 7 |
| tensor-access | 8 | 0 | 8 | 8 | 8 | 8 |

Source derivation, checked against every observed step:

- **workload:** three initial instructions. PCs 3/4/5 execute 50 times each in
  the interpreter: recording starts at the fiftieth visit (threshold 50), and
  compilation at the branch does not replay the instructions already dispatched.
  The interpreter resets R0 at every sync, so those 50 increments do not retain
  progress in R0. Subsequently 81 complete three-op traces retain R0 between
  trace instructions/returns, allowing it to reach 81; the final branch falls
  through. PCs 6–15 each execute once in the interpreter, including Halt.
  A = 3 + 150 + 10 = 163; J = 81*3 = 243; final = 406. Visible R75 is 153 during
  all 81 trace returns, then catches up at Store and finally publishes 406.
  This is the pinned workload's real path, not a rewritten arithmetic loop.
- **meso:** same three-op loop structure, but ordinary R40 retains interpreter
  increments. Fifty interpreted iterations reach 50, then 31 traces reach 81.
  A = 3 + 150 + 4 = 157; J = 31*3 = 93; final = 250. R75 stays 153 through trace
  returns, then catches up in the interpreter tail. PCs 3/4/5 each dispatch 81
  times total; the workload builder's corresponding total is 131 each.
- **micro:** five sequential instructions, including Add, Mul and Halt. R43=84.
- **mixed:** 11 sequential instructions, including option/result/stack/store/load,
  Print and Halt. Its embedded policy is unchanged; R46=42 and printed output 42.
- **policy-heavy:** seven sequential instructions with its original embedded
  policy, AxReport once and Halt. R43=17. Policy callbacks are not extra ticks.
- **tensor-access:** eight sequential instructions, including TNew/TSet/TGet,
  Print and Halt. R44 is a FloatHandle whose referenced value is 9, not integer 9.

Each successful opcode in a trace is admitted by its hook and increments once;
none of these loop traces can guard-fail on the reached bounded integer inputs.
All four straight-line builders visit each PC once on a fresh machine and never
reach the hot threshold. Their actual policies/operations pass. The explicit
Halt is interpreted, synchronizes after all trace work and flushes the final
private accumulator. These source facts explain why the same final count
interpretation holds across all six despite different intermediate snapshots.
The original first-48-register signature does not include R75; repeated signature
agreement alone would not prove any of these counter facts.

## Completion predicate and counterexamples

Require all of the following for this scoped interpretation:

1. A fresh VM object, exactly one context, unchanged program/input identity,
   initial R75 zero, no external register mutation and no counter overflow.
2. Every call returns without trap/policy denial. `run_to_halt` success alone is
   insufficient; the correctness probe also observes all individual steps.
3. `state.halted` is true, context-0 PC equals the exact program length above,
   and the final successful interpreter trace entry is the program's explicit
   last Halt. Do **not** require `context.halted` true; that would reject these
   actual explicit-Halt runs while admitting an unrelated fall-off path.
4. Exact source-derived PC/count/publication ledger and semantic state checks
   pass, including tags/handle payloads and final count; final R75 is observed,
   not substituted from the expected constant.

Negative fixtures/probes, preserved in the raw correctness records:

| Case | Observation | Meaning |
| --- | --- | --- |
| micro, zero/four step budgets | `run_to_halt(0)` and `run_to_halt(4)` return success without halt; one subsequent step executes real Halt. | Return success is not completion; step budgets are not instruction denominators. |
| micro reload on same VM | After a completed five-op run, load publishes R75=5 before resetting private I; first new step publishes 1. | Reused-machine initial R75 is not a fresh baseline; subtracting it can give zero for a later five-op run. |
| meso then micro on same VM | Prior compiled trace at PC3 is retained. New run completes by fall-off at PC6, R75=3, not micro's PC5/R75=5; policy-context trace exit reports committed I=6. | Reusing a VM can carry code/hotness across loads and finish without a final flush. Fresh-object and explicit-Halt checks are mandatory. |
| bounded synthetic Div guard retry | Three initial loads, 50 interpreted four-op loops, five successful four-op traces, then trace Sub/failed Div and interpreter Div retry. Final fault at PC4 is logged, R75=226, not halted. | A=204, J=22; 224 successful primary operations plus two failed Div attempts. The final trace hook offers 225, deopt commits 225, interpreter retry increments/publishes 226. |
| interpreter policy denial at micro PC2 | Hook sees incremented I=3; denied Add has no primary effect and no interpreter trace entry; visible R75 remains 2. | Denied interpreter attempt is counted privately but not necessarily published. |
| trace policy denial at first meso trace | Trace entry count153; prospective step count154 denied; exit remains153 and R75=153. | Rejected trace hooks do not increment; policy-context prospective values are not committed counts. |
| direct compiled-trace policy denial | Reject first hook; ExecResult count0 for each of five supported-prefix/boundary probes. | Independently confirms rejection occurs before trace increment. |
| empty program | Halts through the PC-out-of-range path, R75=0, no explicit Halt trace. | Global halted alone is not the six-builder completion predicate. |

No alternate workload or fallback metric is used. These cases bound the valid
meaning; they do not refute the proved final count for the six fresh completed
builders. Arbitrary multi-context programs, user writes to R75, overflow, injected
traces, asynchronous engine changes and later VM revisions remain outside scope.

## Correctness apparatus and evidence

The isolated C++ probe uses public VM/JitCompiler APIs. For no-policy programs,
a wrapper delegates verdicts unchanged to the same allow-all engine and records
policy-context projections plus public R75 snapshots. Those are observations,
not private-member access. At trace entry/exit context_count equals the private
accumulator; per-op trace checks deliberately use the prospective override.
Two embedded-policy builders use their actual default policy engine unwrapped:
`load_program` replaces injected engines for policy text, so it would be invalid
to keep a pointer to the displaced engine. Their source-derived straight-line
ledger is checked against successful interpreter traces and visible R75.

For all six, a separate fresh unobserved `run_to_halt(5000)` execution is checked
against the stepped execution: all registers/tags, PC, float pool, memory,
printed output, interpreter trace contents and Axion log opcode/tag/value/verdict
content agree. This checks observation transparency, not independence from the
VM implementation. The Python ledger is source-derived and rejects deliberately
corrupted final R75, PC and halted status (three negative verifier checks).
It checks the unchanged six-program input digest, every PC path, trace transition,
count projection and publication, not just six learned golden values.

The final evidence under
[`experiments/base01_r75_semantics/evidence/final/`](../../../experiments/base01_r75_semantics/evidence/final/)
contains compressed raw JSONL, the ledger summary, compiler/CMake/compile-command
records, linked binary/archive hashes and an artifact SHA-256 index. It identifies
the implementation commit plus per-file hashes; a subsequent evidence-only commit
adds the final packet. The audited tracked-byte digest is recomputed before/after
checks and source remains unchanged. `T81_CANONFS_ROOT` is removed before launching
the probe, preventing persistent storage attachment. `--measure` rejects with 2.

Execution environment: macOS ARM64, AppleClang 21, C++23 Release, separately built
unchanged audited archives with benchmarks/TUI/DPE/llama disabled. Compiler
versions, full commands and hashes are retained; Linux/Windows execution and
cross-platform counter behavior were not dynamically reproduced here. A second
probe-only AddressSanitizer build passes, stderr empty, with byte-identical
correctness output. Audited VM libraries are uninstrumented in that run; this
is not full-VM memory-safety or sanitizer certification.

A rejected development probe is retained separately and explicitly excluded
from conclusions. It accessed an observer pointer after embedded-policy loading
replaced/destroyed the engine; the ledger rejected the missing policy callbacks.
That harness had undefined behavior. The corrected ownership rule above removes
the pointer entirely for those two programs; the final normal and probe-ASan
runs are the authoritative evidence. No failed run is rewritten as a pass.

## Reproduction, limits and remaining prerequisites

[README](../../../experiments/base01_r75_semantics/README.md) gives exact clean
source/build/run commands. Session paths were `/private/tmp/t81-base01-audited`,
`/private/tmp/t81-base01-source-build`, `/private/tmp/t81-r75-build` and
`/private/tmp/t81-r75-final-validation`; only `--validate-only` runs. Existing
pinned audited correctness archives were reused after source/configuration/hash
checks; the measured revision was not patched or instrumented. Native probe
build commands, optional probe-only ASan commands and outputs are retained.
`clang-format --dry-run --Werror` and `git diff --check` passed for this increment.
No timing loops, measurement batches, performance samples, resource campaign,
PMU, SPICE, protected experiment or authorization implementation were added.

Remaining engineering prerequisites are to carry these completion/fresh-object/
input/ledger conditions into any separately reviewed future B1 implementation,
verify them on its actual Linux build, and obtain qualified host, instrumentation
retention/timer/accounting evidence and explicit human authorization. No source
repair, new source pin, readiness promotion, protocol activation or benchmark
is authorized by this semantic finding. Broader VM semantic equivalence and the
R0/interpreter/trace difference are separate questions; no general VM certification
or automatic hardware/representation advantage follows.

The scoped R75 interpretation is established, with explicit counterexamples to
broader interpretations. No known counting case remains unresolved **within the
six-builder completion domain**. All unsupported domains remain outside this
finding; do not use it to waive the guards or silently substitute a denominator.
