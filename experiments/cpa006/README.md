# CPA-006 acceptance report — November 5, 2026

**PASS — BOUNDED CONFORMANCE.** T81 preserves equivalent admitted computational
states and terminal classifications across hosted T81VM and freestanding
AArch64 execution for this deliberately varied, sealed 19-case suite. All 57
fresh hosted processes and 57 fresh QEMU guests match the preexecution Python
integer oracle and each other. Replay validates 1,190 schema records, including
170 oracle records and 1,020 actual observations. This is a finite H07 engineering
prerequisite result, not general ISA conformance or a comparative advantage.

The cases exercise seven successful Halt programs, eight runtime-trap programs,
three admission rejections and one instruction-budget exhaustion. Rejections
have zero machine-state observations; traps stop at the actually attempted
instruction. No synthetic continuation or Halt was inferred. No suite run
timed out or produced a semantic disagreement.

## Baseline and source identity

- Current main fetched for this run: `ea6d031a8117341fe03013cc9d5473de1b07727d`.
- PR709: open draft, head `b803bf916a1b1e44129f3a07abf96fa6103c51fc`, base identical to main above.
- CPA004 implementation: `30664fe923b1f491e0722194cfd13eff74f16ccf` on baseline `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`.
- CPA005 integrated source tree: `70734fd5bcfe06978cea13b4d2d595d4dda71a12`.
- Review-only baseline commit: `f7e089590020508077607580da271b6570fa8f60`, exactly that tree, one parent main; no merge commit.
- CPA006 implementation tree before reporting: `cd1c72d14fdf909561b65ef479b513a6f8b51008`.
- Preexecution frozen-suite manifest SHA256: `65db0117a2071038822da6e8306ff520a576d6e73402904ad15f9f304dc78d03`.
- Original CPA003/004 351-byte program SHA256: `b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`.
- Original contract SHA256: `851650fd5136fe6c8f67b924f7721f73beec54e6078b9e841261bc277170705d`.

[Source identity](evidence/source-identity.json), [frozen manifest](frozen/manifest.json)
and [packet manifest](manifest.json) pin actual source/input/evidence content.
The original CPA003/004/005 program, expected observations and historical evidence
are unchanged. The CPA004 evaluator still admits only its original exact artifact.

Before extension, CPA005 replay checked every original packet hash and exact
reused-binary identity, then ran six fresh hosted processes and six fresh QEMU
guests across its two source snapshots. It matched all states; see
[baseline result](evidence/baseline/result.json) and raw baseline commands/logs.
This replay reused hash-verified CPA005 binaries; the CPA006 observer, libraries
and EFI images were subsequently built in a separate fresh build directory.
Original CPA005 and CPA003 reference packets are retained as compressed archives.

The focused draft targets `experiment/cpa005-integration-base`, a review-only
branch pinning the already-tested main+CPA004 tree. It keeps the unchanged,
unmerged PR709 dependency outside the CPA006 diff. Main and original PR branches
were not changed. No production activation or merge is authorized or performed.

## Finite coverage

Every case specifies exact input bytes/hash, initial state, executed sequence,
all expected transitions, terminal classification, maximum steps, permitted
observations and exclusions in [suite.json](frozen/suite.json) and its linked
`*.raw13` / `*.expected.json` files. [Protocol](PROTOCOL.md) and
[freeze receipt](evidence/freeze-receipt.json) precede implementation/execution;
[oracle regeneration](freeze_suite.py) reproduces the frozen artifacts exactly.

| Case | Expected and observed terminal | Steps | State boundaries | Hosted / guest runs |
| --- | --- | --- | --- | --- |
| C01 | halted | 10 | 11 | SUPPORTED: 3 / 3 |
| C02 | halted | 29 | 30 | SUPPORTED: 3 / 3 |
| C03 | halted | 55 | 56 | SUPPORTED: 3 / 3 |
| C04 | halted | 12 | 13 | SUPPORTED: 3 / 3 |
| C05 | halted | 11 | 12 | SUPPORTED: 3 / 3 |
| C06 | halted | 10 | 11 | SUPPORTED: 3 / 3 |
| C07-negative | runtime_trap: BoundsFault | 1 | 2 | SUPPORTED: 3 / 3 |
| C07-upper | runtime_trap: BoundsFault | 1 | 2 | SUPPORTED: 3 / 3 |
| C07-code | runtime_trap: BoundsFault | 2 | 3 | SUPPORTED: 3 / 3 |
| C08-div | runtime_trap: DivisionFault | 3 | 4 | SUPPORTED: 3 / 3 |
| C08-mod | runtime_trap: DivisionFault | 3 | 4 | SUPPORTED: 3 / 3 |
| C09 | runtime_trap: TrapInstruction | 2 | 3 | SUPPORTED: 3 / 3 |
| C10-undefined | admission_rejection: undefined_opcode | 0 | 0 | SUPPORTED: 3 / 3 |
| C10-truncated | admission_rejection: malformed | 0 | 0 | SUPPORTED: 3 / 3 |
| C10-excluded | admission_rejection: unsupported_profile | 0 | 0 | SUPPORTED: 3 / 3 |
| C11 | instruction_limit | 8 | 9 | SUPPORTED: 3 / 3 |
| C12-valid | halted | 5 | 6 | SUPPORTED: 3 / 3 |
| C12-destination | runtime_trap: DecodeFault | 1 | 2 | SUPPORTED: 3 / 3 |
| C12-source | runtime_trap: DecodeFault | 1 | 2 | SUPPORTED: 3 / 3 |

C01 tests positive, negative and zero arithmetic. C02 covers signed division and
remainder sign combinations, zero dividend and int32 extremes, including
INT32_MIN/-1 within int64. C03 covers all three unary trits and all nine binary
pairs for NOT/AND/OR/XOR. C04 tests both outcomes of JZ/JNZ/JN, including a
register/flag discriminator. C05 checks comparison outcomes and flag persistence. One C05 branch target equals
fallthrough, so its predicate is reported but not independently inferred from PC;
C04 uses distinct targets to establish both outcomes.
C06 tests first/last logical data words, zero reads and direct addressing distinct
from R30=400. C07 tests negative/one-past memory and protected code writes. C08–C12
cover zero division/modulo, executed Trap, separate malformed/undefined/excluded
admission, bounded loop exhaustion, R0/R74/R80 and negative/243 register operands.

All 81 admitted register payloads/tags, every logical memory payload/tag, flags,
PC/SP, halt, tick/system hashes, instruction, sequence/status/trap are compared at
every actual boundary. The [machine-readable matrix](conformance.json) includes
instruction and operand classes, branch predicates and whether PC distinguishes
the outcome, memory behavior,
per-profile counts, evidence completeness and exclusions. Per-case normalized
and raw observations live under `evidence/suite/<case>/`; normalized JSON and raw
streams are losslessly compressed where indicated by `.gz`. [Full raw replay](verify.py) validates hashes, actual input hex,
ordered successful ExitBootServices/bare/evaluator markers, schema records and
terminal classifications rather than trusting PASS labels.

## Isolation and adversarial evidence

CPA006 is a separate default-off sealed-input evaluator using the existing
17-opcode subset. Each EFI embeds one actual program and decodes/executes its
bytes; no case-name dispatch or precomputed answers are linked. The independent
switch uses fixed arrays, bounded integer operations, no allocation/filesystem
and no hosted interpreter. Only the minimal opt-in CMake/boot hook is outside
the experimental directory. Enabling both CPA004 and CPA006 is rejected.

[Default-off result](evidence/isolation/result.json): a fresh build with both
options omitted includes neither evaluator/input, has no experimental link
command, and matches main's entry disassembly and complete EFI code/read-only
data sections. [Review](REVIEW.md) records dependency, bounds, arithmetic,
resource, branch, trap, timeout and oracle-contamination checks. Compile-time
workspace plus 1KiB headroom is below 32KiB; the retained evaluator's largest
individual stack frame is 128 bytes, not a whole-EFI memory measurement.

All 19 native sealed cases passed ASan/UBSan under NDEBUG. Explicit Debug and
Release admission/limit controls pass. A changed instruction byte, undefined
opcode, defined/excluded opcode, truncation/null input and excessive requested
limit fail explicitly. A syntactically valid changed program executes in native
T81VM, while the original-input identity and oracle gates reject it; the sealed
guest rejects its changed buffer before execution. This distinction is retained
in `evidence/negative/mutated-hosted-result.json`. The intentionally wrong Halt expectation exits 1 under
NDEBUG; corrupting an expected payload makes raw replay exit 1 at sequence 10.
A post-execution guard now refuses a nonempty live-run output directory, verified
to fail before touching existing evidence. Compiled execution logic and frozen
expectations were unchanged by that guard.

[Commands](COMMANDS.md) and nested `commands.json` files retain exact arguments,
outputs/exits and failed controls. [Environment](evidence/environment.json):
macOS ARM64, Clang 22.1.1, CMake 4.3.0, Ninja 1.13.2, QEMU 10.2.2 TCG cortex-a57,
EDK2 firmware and jsonschema 4.23.0. [Build identities](evidence/build-artifacts.json)
pin linked libraries, observer and each EFI. Generated ESP/vars and native test
binaries are omitted after identity capture; raw observations remain.
The unchanged TISC freeze check passed. No remote CI/full-suite or CTest run is
claimed. Initial preexecution draft schema/R0 corrections, a shell wrapper issue
and an initially invalid Commons evidence reference are retained with reasons;
none was a semantic execution failure or a retrofitted expected state.

## Limits and disposition

The [discrepancy register](discrepancies.json) retains normative direct/indirect
memory/branch, register/FLAGS JZ/JNZ, TXor sum/difference and 174/209/244 inventory
conflicts. The selected implemented profile does not resolve them. C10-excluded
is rejection by the experimental admission layer, not invalidity of production
FAdd. Native registers 81..242 are outside the admitted profile, not invalid
native registers. C01's frozen metadata mentions cancellation, but no nonzero
exact cancellation occurs; that coverage is explicitly withheld without rewriting
the freeze. No frozen expected data changed after implementation or execution.

Other opcodes/tags, arbitrary precision or int64 overflow, general programs,
untested malformed metadata/branch targets, dynamic policy/authorization contexts,
concurrent evaluator calls, Linux/Windows hosts and physical hardware remain
UNSUPPORTED or unverified. The POSIX runner needs an appropriate Windows
environment or reviewed port. No performance, energy, native ternary superiority,
production-readiness or complete-OS conclusion follows.

Codex authored/reviewed this work with prior CPA005 and historical source/oracle
context. Expectations precede CPA006 outputs, but this is not blinded or
independent-person scientific replication. [Proposal](proposal.json),
[reproduction record](reproducibility.json), [review record](review.json) and
[discrepancy record](discrepancy-record.json) passed Commons shape/reference
validation and remain proposed records, not human scientific acceptance.

**Disposition: PASS — BOUNDED CONFORMANCE for the 19 frozen cases and exactly the
admitted fields and environment above.** The next step is human resolution of
the normative profile discrepancies and independent-person reproduction of this
same frozen suite before further expansion. No further experiment was executed.
