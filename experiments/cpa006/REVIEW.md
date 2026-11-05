# CPA006 bounded review and limitations

Codex authored the suite, Python reference, observer and experimental evaluator,
with prior CPA005 and historical-source context. Python expectations were
frozen before new evaluator implementation or execution. This is separate
mathematical derivation and execution logic, not independent-person or blinded
scientific replication. Hash receipts establish content identity, not authentic
authorization, reviewer independence or an externally trusted freeze timestamp.

The evaluator is derived from the separately written CPA004 switch, but CPA004
code, exact-artifact contract and historical observations remain unchanged.
CPA006 changes only a separate sealed experiment and the minimal default-off
CMake/entry hook. It embeds actual inputs, no expected traces or final answers.
Initial layout constants derive from n and the pinned hosted layout; live system
hashes derive from bytes/state. Each image admits only its exact generated input
with structural checks and executes the same switch regardless of test name.
No dispatch uses case identifiers or expected outputs. Only valid/excluded
opcode admission classification examines the opcode domain.

Admission scans actual raw13 records before byte equality/execution. The input
length is bounded at 832 bytes; the selected image has at most 64 records.
Runtime operand checks precede memory/register indexing. Admitted registers are
R0..R80; invalid negative/243 operands reach DecodeFault in both profiles.
Memory is n+1536 words; data begins at n, excluding code; bounds failures emit
only the actually attempted trap boundary. No array allocation depends on
untrusted runtime input. Static workspace plus 1KiB headroom is compile-checked
below 32KiB for every image. The retained evaluator stack report has maximum
individual frame 128 bytes; this is not total EFI memory/stack measurement.
Global evaluator state is single-invocation, not a concurrent/reentrant API.

Signed LE32 decoding avoids implementation-defined narrowing. Add/multiply
bounds precede signed arithmetic; division guards zero and INT64_MIN/-1; scalar
trit guards bound NOT/XOR; decimal formatting avoids negating INT64_MIN. Exact
Python integers/sign-magnitude truncation and modular XOR are different reference
mechanisms. Native ASan/UBSan plus NDEBUG passed all 19 sealed cases. Unreachable
int64 overflow guards were inspected, not exhaustively established; arbitrary
precision and overflow execution remain outside the profile.

JZ/JNZ test R[B] and JN tests the negative flag. C04 exercises both outcomes for
each and includes JNZ taken while the zero flag is true. C06 observes memory
30 independently of R30=400. C05 checks equal/less/greater flags and preservation
through Store, branches and Halt. Complete trit tables are covered, but the suite
does not generalize beyond scalar trits or tested operand domains.

Trap and Halt differ: trap leaves halted=false and records the attempted
instruction with the live post-step PC/tick/system hashes; no fabricated
continuation follows it. Admission rejection has zero state records. C11's eight
actual taken branches end in instruction_limit without a synthetic ninth
observation or Halt. The native VM's normal instruction stepping is used; the
external budget is explicit, not a newly claimed native policy limit. The loop
is below the native JIT hotspot threshold; no JIT behavior is tested or added.

Defined FAdd 36 is rejected by experimental admission in the observer and guest.
That is shared experimental admission behavior, not native full-ISA invalidity.
Undefined 255 and truncated raw13 are separately tested. Valid modified bytes
are not intrinsically invalid ISA: the guest's sealed-input gate rejects them,
and the evidence parser checks exact identity. A modified expectation is
rejected by replay. All these checks use explicit returns/exceptions and remain
active under NDEBUG; the deliberately wrong Halt expectation fails with exit 1.

The guest supervisor requires ordered real boot-path markers, actual input hex,
one complete terminal record, every state/schema match, and no timeout. It
records actual QEMU arguments and supervisor return status but never treats
that return status or firmware startup as computational Halt. All 57 fresh
suite guests completed; no failed or timed-out run was relabeled PASS.

The native observer necessarily links unchanged hosted logic; the freestanding
object does not link or call that observer or VM. Import-table and object-symbol
inspection is retained. Default-off fresh-build EFI `.text`/`.rdata` and entry
disassembly match the pinned main control; neither evaluator nor generated input
is included. No new runtime dependency enters production paths.

No production ISA/decoder/VM/specification or historical evidence file changed.
The normative operand/branch/XOR/inventory conflicts remain unresolved. The
frozen C01 metadata mentions cancellation but its instructions do not exercise
nonzero exact cancellation: the conformance report explicitly withholds that
coverage. No frozen expected state was adjusted after implementation/execution.

macOS ARM64 / QEMU TCG AArch64 was executed. The EFI cross-target uses Windows
COFF ABI, which is not Windows-host execution. The POSIX supervisor is suitable
for a POSIX host in principle; Linux was not executed here and native Windows
requires a separately reviewed port or POSIX environment such as WSL. Physical
ARM64, other host architectures, arbitrary programs, other tags/opcodes and
performance/energy/ternary superiority are untested. No remote CI, full suite,
production-readiness or complete-OS claim follows from this finite result.
