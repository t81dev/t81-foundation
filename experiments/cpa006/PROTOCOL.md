# CPA-006 frozen bounded conformance protocol

Status: proposed experimental profile, execution authorized by the user's
CPA-006 mission; merge, ISA acceptance and production activation are not
authorized. Canonical research parent is H07, an engineering prerequisite.

Baseline replay passed before suite construction: CPA-005's complete packaged
hash manifest and CPA-003/004 retained checks, six fresh hosted processes and
six fresh QEMU guests on frozen and integrated CPA-005 snapshots. Exact
hash-verified binaries were reused for this baseline replay; it is not another
fresh-build claim. New CPA-006 binaries will be built separately. See
`evidence/baseline/result.json` and raw command/run records.

Main: ea6d031a8117341fe03013cc9d5473de1b07727d; PR709 head:
b803bf916a1b1e44129f3a07abf96fa6103c51fc; CPA005 integrated tree:
70734fd5bcfe06978cea13b4d2d595d4dda71a12. CPA003/004 canonical program SHA256:
b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b;
contract: 851650fd5136fe6c8f67b924f7721f73beec54e6078b9e841261bc277170705d.
Original CPA003/004/005 bytes, expected states and code remain unchanged.

The finite suite in `frozen/suite.json` specifies 19 cases covering C01–C12,
including separate invalid operand, memory and decode cases. Every case pins
exact bytes, instruction count, SHA256, initial state (or no state for admission
rejection), executed PC sequence, terminal classification, step bound, permitted
observations and exclusions. `*.expected.json` contains every expected state.
The manifest pins all artifacts and the observation schema. `freeze_suite.py`
is a Python integer reference that neither invokes nor reads either evaluator's
outputs. Freeze regeneration must match byte for byte. Expected data must never
be changed to accommodate observed disagreement.

A preexecution draft corrected copied schema SP constraints and the source-
documented R0 LoadImm behavior before evaluator implementation or any CPA006
execution. The original draft and reasons are retained. The final freeze is
recorded in `evidence/freeze-receipt.json` before implementation/execution.

Selected profile: raw13 integer records, the existing 17 opcode assignments,
scalar trits, registers R0..R80, at most 64 records/steps and n+1536 logical
memory words (n<=64). SP=n+256. Fresh memory and tags are zero/Int; flags false;
system registers use the inspected hosted hash/tick formulas. Logical code is
not writable as data. Arithmetic is exact within int64; division truncates
toward zero and remainder has the dividend's sign. TXor is balanced wrapped
difference. Memory addresses and branch targets are immediate; JZ/JNZ test R[B],
JN the negative flag. Both branch outcomes and a flag/register discriminator
are frozen. The direct-memory discriminator sets R30=400 while accessing 30.

CPA004's exact-artifact contract is not widened. CPA006 is a separate sealed
suite experiment: each guest image embeds one actual frozen input, compares
runtime input bytes against that image's admitted input, decodes and executes
independently, and emits the actual buffer. It does not link expected states.
No arbitrary-program public API or new opcode is introduced. All new evaluator,
oracle, adapter and supervisor code lives under experiments/cpa006. Only the
minimal default-off build/boot hook is outside that directory; CPA004 cannot
be enabled simultaneously with CPA006.

Invalid opcodes/truncated raw13 fail admission without an initial state;
defined but excluded opcodes fail experimental profile admission explicitly
(they are not claimed to be invalid production instructions). Invalid register
or memory operands in otherwise admitted raw13 records reach runtime and trap;
initial state and the actually attempted trapping instruction are observed.
No observation follows a trap. Host/native trap strings and classifications
are retained. Budget exhaustion is a supervisor/harness boundary after the
last actual instruction, not a Halt, trap or fabricated extra VM step.
Timeout is INCONCLUSIVE and cannot satisfy an expected terminal condition.

For each case run three fresh hosted processes and three independent QEMU
boots, each with fresh ESP and firmware variables. Compare every frame and
terminal record with the pre-frozen oracle and with the other profile. Check
emitted input identity and ordered ExitBootServices/bare/evaluator markers.
Raw stdout/serial, stderr, command arguments, exits, timeout flags and failures
are immutable evidence. A supervisor stop proves no computational state by
itself. A missing result is not PASS.

Adversarial controls: corrupt a retained expected payload and require comparison
failure; modify one byte of an admitted image's input and require artifact
rejection; reject undefined and defined/excluded opcodes and truncated input;
exercise bounded loop exhaustion and excessive requested limit; compile checks
with explicit -DNDEBUG and include a deliberate failing expectation. Run
ASan/UBSan on the native evaluator harness. Compare default-off EFI code/data
with the unchanged baseline and inspect evaluator dependencies.

Resource scope: local four-job builds; at most three simultaneous 512 MiB TCG
guests, each with 30-second boot/execution deadline; fixed evaluator storage
under 32 KiB plus 1 KiB compile-time stack headroom. No dynamic allocation,
filesystem, network or hosted interpreter in the evaluator. No benchmark or
hardware performance measurement. The expected suite has only eight loop visits,
below the hosted JIT hotspot threshold of 50; no JIT work is added.

SUPPORT requires all three runs of each profile, byte identity, terminal and
all admitted state matches. Missing observations produce PARTIAL; environment
prevention BLOCKED; reproducible disagreement FAIL. No ISA/production repair
is permitted to force agreement. Registers 81..242, arbitrary precision/tags,
int64-overflow execution, generalized resource/policy contexts and all other
opcodes are UNSUPPORTED in this experiment. Finite coverage never implies
general ISA conformance. Normative memory/branch/TXor and inventory discrepancies
remain unresolved and will be retained in the discrepancy register.

Authorship: Codex agent, with prior CPA005 context and historical expected/source
access. Oracle code is separate from native/freestanding execution; this is not
blinded validation or independent-person scientific replication.
