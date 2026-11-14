# CPA-004 acceptance report

**PASS for the frozen hosted raw13 experimental profile.** Three fresh hosted
processes and three fresh AArch64 QEMU boots agree with the separately derived
oracle at all 25 observation boundaries (initial state plus 24 instructions).
Each guest executed after successful `ExitBootServices`; none of the successful
runs timed out. Final PC=27, SP=283, halted=true; trap PCs 18, 20 and 23 were skipped.
All 81 register payloads/tags, 1,563 logical memory payloads/tags, flags and
system registers were compared at every boundary, not just the final numbers.

- Tested repository base: `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`.
- Implementation commit: `30664fe923b1f491e0722194cfd13eff74f16ccf`.
- Canonical program: 351 bytes, 27 canonical 13-byte records;
  SHA-256 `b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`.
- Frozen contract SHA-256:
  `851650fd5136fe6c8f67b924f7721f73beec54e6078b9e841261bc277170705d`.
- Executed EFI SHA-256:
  `4b03971e3207205e123e3c3a08ca1da65fd8965d3f339f3cacd092eebd765ab9`.

## Acceptance and implementation boundary

CPA-003's packet (57 files, eight pre-host frozen files) passed its hash verifier;
its oracle regeneration reproduced eight frozen hashes. Those are content
identity checks, not independent scientific validation. Before implementation,
`evidence/acceptance/check.py` separately decoded every tuple, derived all states
using Python integers and rational truncation, recomputed the exact little-endian
system hash formulas, and checked retained and three fresh hosted traces. The
original gate is retained in `evidence/acceptance/gate.json`; a later replay is
`recheck.json`. The CPA-003 manifests, source/library pins, proposal validation,
inventory and discrepancy records are copied under `evidence/acceptance/`.

The latest human request authorizes testing the implemented hosted profile while
retaining normative conflicts. This experiment accepts that limited profile; it
does not resolve the ISA prose conflicts. The same Codex agent has the previous
tasks' context: this is a separate mathematical derivation and independently
written execution switch, **not independent-person review**. No model identity
or external reviewer is inferred from role labels or commit authorship.

`ternaryos/hal/cpa004_evaluator.{hpp,cpp}` use only freestanding integer types,
fixed arrays, independent raw13 decoding, an independent execution switch and a
streaming JSON formatter. They neither call nor embed hosted T81VM execution
code, expected-state tables, allocation, STL, policy engines or filesystem code.
The only generated header contains the actual 351 input bytes. The guest emits
that buffer as hex; the supervisor checks its identity before accepting states.
The hosted observer uses the existing decoder/encoder and interpreter, with
explicit allow-all admission, empty metadata pools and no CanonFS root.

The 17 admitted assignments are Halt, LoadImm, Load, Store, Add, Mul, Div, Mod,
Cmp, TNot, TAnd, TOr, TXor, JumpIfZero, JumpIfNotZero, JumpIfNegative and Trap.
Trap is present only in skipped sentinels; successful execution does not validate
its trap transition. Direct logical memory addresses, immediate branch targets,
R[B]-tested JZ/JNZ, flag-tested JN and balanced wrapped-difference TXor match the
pinned hosted profile. Integers are checked int64 values and scalar trits only;
this is intentionally not arbitrary-program, big-integer or full-ISA support.

Admission rejects unsupported/undefined opcodes, malformed operands, changed
artifacts and excessive limits before instruction output. Execution has at most
64 steps, 81 registers and 1,563 logical memory words. Object inspection shows
14,816 bytes of static machine storage; the largest reported evaluator frame
is 128 bytes. The compile-time workspace check reserves 1 KiB of stack headroom
under 32 KiB. This is not a measurement of the whole EFI image's memory use.
The supervisor enforces a 30-second guest deadline. Instruction exhaustion has
an explicit failure status and never counts as Halt.

`ternaryos/CMakeLists.txt` adds an opt-in `T81_BUILD_CPA004_EXPERIMENT=ON` path
(OFF by default), checks the raw artifact hash and cross-links one evaluator
object. The existing bare entry calls it only after the unchanged EFI stub's
successful `ExitBootServices` branch; after output the CPU parks in WFI.
Native admission checks, the hosted observer, supervisor, frozen fixtures and
evidence/replay helpers live entirely under `experiments/cpa004/`.
No ISA, decoder, opcode assignment, VM semantics, registry or frozen
specification was changed. With the experiment macro absent, AArch64 entry
disassembly is identical to baseline (`evidence/option-off-check.json`).

## Results and retained failures

| Evidence level | Observed result |
| --- | --- |
| AArch64 freestanding compile and EFI link | PASS; ARM64 PE/COFF EFI application, no import table |
| Post-ExitBootServices guest entry | PASS, three boots; ordered bare-entry and live-evaluator markers |
| Actual canonical input and instruction execution | PASS, three identical buffers and 24-step traces |
| Oracle/hosted/guest full admitted state equivalence | PASS, all 25 boundaries in each of three runs |
| Observation schema | PASS, 25 expected + 75 guest records, jsonschema 4.23.0 |
| Debug and Release admission/limit checks | PASS; explicit checks remain active under NDEBUG |
| Deliberately wrong NDEBUG expectation | Expected exit 1, proving the check can fail |

`evidence/qemu/` contains successful per-boot commands/results and losslessly
compressed raw serial streams. Each uncompressed stream hashes to
`275b59c474731a398aada903e50ab477a1bf056abff411a8b6630ab5da68946f`.
`evidence/hosted/` retains raw full-array observations and command outcomes;
each uncompressed hosted stream hashes to
`e26c41cd709c87213904e5bfe11d3b6f20fc7124f2350e6714d9806f95cec30a`.
The supervisor terminates QEMU after `CPA004_EXIT=0`; its process return code
alone is never treated as the guest's computational halt.

The first three boot attempts were **INCONCLUSIVE**, timing out in firmware
before ExitBootServices. The staging directory lacked `TERNOS`, which the
existing EFI entry needs for its marker/report files. Adding that directory to
the supervisor staging fixed boot admission; no evaluator or EFI code change
was used to fix the failure. `evidence/initial-staging-timeouts/` preserves all
three raw logs and timeout results. Those streams hash to
`86d5512cdfeef9f9726cf2bd209f8ade3d9730843e7fef73538402ea28025c77`.

## Remaining discrepancies and limits

The pinned inventory remains 174 capability-frozen instructions, 209 registry
entries and 244 named header assignments; the old 247-value interval contains
undefined 221–223. Registry/header assignments also differ for TVecSub and
TNOT_SWAR/TAND_SWAR/TOR_SWAR. All 17 experiment assignments agree and are below
174. No inventory or specification was silently repaired here.
`spec/tisc-spec.md` describes register-indirect memory/branch targets and
FLAGS-based JZ/JNZ, whereas runtime and `spec/tisc/opcode-semantics.md` describe
the admitted direct-address/register-tested profile. TXor's “Sum” label remains
ambiguous. These material differences remain explicitly unresolved.

Measured scope: macOS ARM64 host, QEMU 10.2.2 TCG cortex-a57, existing EDK2 EFI
firmware, Clang 22.1.1 and CMake 4.3.0. This demonstrates bounded architectural
portability for one artifact; it establishes no performance, energy, physical
ternary hardware, authenticity, production readiness or complete OS claim.
Linux and Windows hosts and physical hardware were not executed. The POSIX
serial supervisor needs a POSIX environment (for example WSL on Windows).
The cross-target's Windows COFF ABI does not mean Windows was tested.
Remote main advanced to `d999f58f92080afe4c8c246147bf51bad9e7c88b`
while evidence was packaged. The experiment preserves its exact tested base;
integration with that newer tip is untested. No merge is authorized or performed.

See [COMMANDS.md](COMMANDS.md) for exact build and replay commands and
`evidence/manifest.json` for retained evidence/source hashes. EFI/firmware disks
are not checked in; their hashes and generation commands are retained.
