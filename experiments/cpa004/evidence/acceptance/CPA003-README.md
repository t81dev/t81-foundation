# T81-CPA-003 — Oracle verification and artifact freeze

**Disposition: READY WITH PREREQUISITES.**
The oracle/artifact/hosted agreement is verified; memory and branch specification
authority requires explicit clarification before the future evaluator is implemented.
**Oracle assessment: VERIFIED; no proposed numerical result needed correction.**

The separate mathematical model, exact raw13 artifact and three fresh hosted
executions agree at every compared boundary. This is not freestanding execution,
implementation authorization, full ISA conformance or accepted scientific evidence.
The future evaluator remains unimplemented and unauthorized.

Verify download contents with `python3 -B verify_packet.py`; this checks hashes,
not the correctness or authenticity of the conclusions.

## Source state and reviewer relationship

- Observed current `main`: `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`.
- PR [#707](https://github.com/t81dev/t81-foundation/pull/707): **open, unmerged**;
  head `55223cbd3ee29b5391e151197fe6eee1b8889452`, base the same main revision.
  Its test-source commit is `74bc8bc824a9732c57e7225077bf15ddc6c05097`.
- CPA-001 assessment and proposal are preserved byte-for-byte under `original/`;
  the assessment used that same main baseline. Original packet files have no
  invented Git commit identity; the packet manifest pins their content hashes.
- Codex is the prior CPA-001 proposal author and CPA-002 corrective agent in the
  same conversation. This task supplies a separately implemented mathematical
  oracle and fresh hosted observation, **not independent-person scientific review**.
  Exact model identity is not established and remains null in the proposal.

`evidence/github-revisions.json` retains the remote observation. The source
archive is `/tmp/t81-cpa-803075e-fbzvi39l`; relevant files were compared with their
pinned Git blobs. `evidence/source-pins.json` records exact SHA-256 values,
repository paths, line ranges and commit-pinned GitHub links.

## Deliverables

| Requested deliverable | Packet file |
|---|---|
| Oracle assessment and expected-value verification | this report; `expected-values-check.json`; `oracle-summary.json` |
| Full instruction-level trace | [TRACE.md](TRACE.md); `oracle-trace.json`; `oracle-normalized.json` |
| Frozen binary, exact listing and digest | [program.raw13](program.raw13); `program.hex`; `program.sha256`; `instruction-listing.json`; `freeze-manifest.json` |
| Hosted commands, environment, raw observations and differences | `evidence/hosted-build.json`; `evidence/hosted-commands.json`; `evidence/hosted-*.raw.jsonl`; `hosted-comparison.json` |
| Future comparison contract and record schema | [freestanding-contract.json](freestanding-contract.json); `normalized-record.schema.json` |
| Actual Commons proposal validation | `proposal-validation.json`; `proposal.json`; `original/experiment-proposal.schema.json` |
| Risks and unresolved discrepancies | [RISK_REGISTER.md](RISK_REGISTER.md); `discrepancy-register.json`; `inventory-assessment.json` |
| Narrow next implementation task | final section below |

## Frozen program identity

Format: **27 × 13-byte records = 351 bytes**; each record is
`opcode:u8, A:i32le, B:i32le, C:i32le`. No magic header, count prefix, literal
pools, metadata container or native executable wrapper is present. Integer
literal kind defaults to Int=0 after existing decoder construction.

SHA-256:

```text
b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b
```

The exact proposal listing, rather than its expected numeric results, is the
only imported program input to `oracle.py`. Python `struct.pack('<Biii', ...)`
constructs the bytes. An independent byte reader uses explicit signed
`int.from_bytes` on operand slices and verifies all 27 records. The hosted
observer separately uses the existing C++ decoder, captures its opcode/A/B/C/tag
records, and verifies existing encoder output equals the input bytes.

All 17 used opcode assignments match the current header and frozen registry,
and all are below 174. The 174/209/244 inventory disagreement and four later
registry/header conflicts remain preserved; none changes this program's bytes.
See `inventory-assessment.json` for the actual subset-by-subset check.

**Additional material source-contract discrepancies affect this program.**
`spec/tisc-spec.md:1291–1304` describes register-indirect Load/Store, while runtime
Load uses operand B directly as an address and Store uses operand A directly.
The opcode reference at `spec/tisc/opcode-semantics.md:34–35` agrees with runtime.
Likewise `spec/tisc-spec.md:1322–1334` describes register-indirect targets and
FLAGS-based branches, whereas runtime JZ/JNZ test R[B] and branch to immediate A;
JN tests the negative flag. The opcode reference again describes runtime operands.
At PC19 the zero flag is true but R42=1, so flag-based JNZ would disagree with
the observed register-based JNZ. The reference's TXor “XOR / Sum” label is also
ambiguous; the frozen profile explicitly uses wrapped difference from source.

These are classified as **unresolved specification/implementation discrepancies**,
not numerical oracle errors, artifact-construction errors, or proven VM defects.
This packet freezes the exact implementation-profile artifact and its derived
behavior; it does not silently choose which conflicting document should prevail.
The artifact is ready for reproduction, but human clarification or explicit
acceptance of that narrowly scoped profile is a prerequisite to guest work and
any portable/normative TISC-conformance claim. See `discrepancy-register.json`.

## Mathematical derivation and hosted result

The oracle was generated and frozen by content hash **before any CPA-003 hosted
execution**. `oracle.py` does not read hosted results or the proposal's expected
numeric outputs. `freeze-manifest.json` fixes the program, source model and full
expected trace; they were not edited to match the host afterward. A fresh-directory
regeneration reproduced every frozen file hash (`evidence/oracle-regeneration.json`).

The final results derived before execution and then observed in all three runs:

| Observation | Derived and observed |
|---|---|
| Executed instructions; final PC; final SP | 24; 27; 283 |
| R43, R44, R45, R46 | 1, -1, 1, -1 |
| R49; R50 and R51 | 162; 161 and 161 |
| R53, R54 | -3, -1 |
| Memory[81], [82], [83] | 161, -1, -3 |
| Final zero, negative, positive flags | false, true, false |
| R75, R76, R77 | 24, 2110228405636142087, 1353750679943106816 |
| R78, R79, R80 | 0, 0, 0 |
| All register and memory tags | Int=0 |

The initial R75–R80 window is
`[0,2110228405636142087,1165810651130105283,0,0,1]`.
All initial flags are false. Every system-register boundary, not merely the
final signature, is included in the complete trace. The exact runtime offset
`1469598103934665603` is deliberately used for R76/R77; substituting standard
FNV's offset would be a mismatch. R76 hashes all 27 instruction records including
the three skipped Trap records. R77 includes the post-instruction PC and tier 0.

Signed division uses `q = -floor(7/2) = -3`, then
`r = -7 - q*2 = -1`; Python's signed floor remainder is not substituted.
TXor(+1,-1) is balanced difference, so 2 maps to -1. JZ and JNZ inspect register B;
JN inspects the negative flag. The path skips PCs 18,20,23, visits 24 instructions,
and Halt sets **global** halted after PC26→27. The hosted per-thread halted field
remaining false is a diagnostic distinction, not evidence that global Halt failed.

Comparison covers the fresh initial state plus all 24 post-step states: R0–R80
values/tags, flags, PC/SP, global halt and every one of the 1,563 logical memory
values/tags. Full raw arrays are retained, and their sizes/domains are checked
before sparse normalization. Unsupported internal state is not silently compared
as an architectural requirement; diagnostics retain tier, policy-loaded state,
call depth, frame count, contradictions and GC cycles.

**All three runs have zero differing fields.** Each has 25 observations,
successful explicit Halt, empty stderr and the same raw-observation SHA-256:
`e26c41cd709c87213904e5bfe11d3b6f20fc7124f2350e6714d9806f95cec30a`.
This is observed repeatability, not a universal determinism proof. The report
retains the required difference categories: oracle error, artifact-construction
error, unsupported observation, hosted implementation defect and unresolved
discrepancy. No category was assigned a numerical difference because none occurred. Source-contract
disagreements are separately classified in `discrepancy-register.json`.

## Hosted execution and reproduction

Environment: macOS / Darwin 25.5.0 ARM64, Homebrew Clang 22.1.1, CMake 4.3.0,
Python 3.14.3. The unchanged interpreter libraries were rebuilt successfully from
the pinned archive in the existing `/tmp/t81-cpa-build` directory. Ninja recovered
a build-log warning and completed a 27-action rebuild; this is not an independent
clean-build claim. Library hashes, configuration and exact compiler/link argv are
in `evidence/hosted-build.json`. Repository global O3/NDEBUG flags apply to those
libraries; the observer uses O0/g/UNDEBUG and T81_DETERMINISTIC.

The observer implements no opcode semantics or expected answers. It explicitly
calls `make_interpreter_vm(make_allow_all_engine())`, loads existing `decode`
output with empty pools/metadata, then calls the unchanged `step()` API. It rejects
a nonempty CanonFS root before constructing the VM and bounds execution to 64
instructions. It requires explicit Halt, rather than treating a successful
`run_to_halt` return at budget exhaustion as completion.

The principal executed commands were:

```sh
python3 -B /tmp/T81-CPA-003/oracle.py
cmake --build /tmp/t81-cpa-build --target t81_vm -j 4
/opt/homebrew/opt/llvm/bin/clang++ -std=c++23 -O0 -g -UNDEBUG -DT81_DETERMINISTIC -I/tmp/t81-cpa-803075e-fbzvi39l/include /tmp/T81-CPA-003/hosted_observer.cpp -L/tmp/t81-cpa-build -L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++ -lt81_vm -lt81_isa -lt81_axion -lt81_ffi -lt81_experimental -lt81_core -o /tmp/T81-CPA-003/hosted_observer
python3 -B /tmp/T81-CPA-003/run_hosted.py
python3 -B /tmp/T81-CPA-003/reproduce_oracle.py
/tmp/t81-h04-enforcement-venv/bin/python -B /tmp/T81-CPA-003/validate_packet.py
```

All returned exit 0. `run_hosted.py` retains each fresh process command and raw
stdout/stderr before reading expected oracle records. It clears T81_CANONFS_ROOT
for each process and applies a 30-second deadline. The math model runs on host
Python solely for verification; it is not a freestanding evaluator.

An independent reviewer can first run `python3 -B reproduce_oracle.py` from the
packet using only Python's standard library; it regenerates in a new scratch
directory and compares all frozen hashes without modifying this packet. For a
hosted reproduction, obtain the exact repository revision, reproduce the CMake
configuration in `evidence/hosted-build.json` with build outputs outside the
checkout, and compile `hosted_observer.cpp` against those libraries. Adjust only
source/build/compiler locations for the review environment; preserve the frozen
program and report the new platform/toolchain. The observer executable in this
packet is a macOS ARM64 validation tool, not the TISC program artifact.

## Commons and governance validation

The default Python and `/tmp/t81-commons-venv` lacked jsonschema; those failed
discovery attempts returned ModuleNotFoundError and were not claimed as validation.
An existing `/tmp/t81-h04-enforcement-venv` contains the **declared version 4.23.0**.
No package installation or undeclared-version substitution was required.

Using that environment, `get_hypothesis(H07)` and
`get_reproduction_instructions(H07)` both exited 0 through the offline Commons
CLI. Raw envelopes and commands are in `evidence/`. H07's canonical authority is
the dated register at `3d395cb604ddd49da3354d17f0505f2ed033757e`, with the original
B/C evidence tiers and qualifications preserved. Its historical PR status text
is not asserted as a new live observation. The Commons grants no execution rights;
the user's CPA-003 request separately authorizes this preparation and hosted check.

Actual `Draft202012Validator` validation checks the copied, byte-pinned Commons
schema, the original CPA-001 proposal and the scoped CPA-003 proposal. Both pass.
The proposed observation schema also validates all 100 normalized records (25
oracle plus 75 hosted), with no errors. Raw array dimensions and integer/tag
domains are additionally checked. See `proposal-validation.json` and
`evidence/schema-validation.stdout.json`.

`proposal.json` remains `status: proposed`; future freestanding authorization
is `not_requested`, with the scoped CPA-003 preparation authorization explained
separately. No accepted H07 record, H01–H08 identifier, Hanoi/CWEB history, RFC,
Protocol 001 state, production source or original checkout edit was changed.
This establishes neither comparative H07 auditability advantage nor H08 benefit.

## Future acceptance contract and next task

`freestanding-contract.json` fixes the exact bytes, admitted opcode subset and
Int-only value domain, all system-register formulas, logical segment addresses,
PC/flags/Halt behavior, source-derived trap classifications, normalized schema,
post-ExitBootServices evidence and resource/failure rules. Different internal
layouts are allowed. Integer payloads are canonical decimal strings in normalized
records to preserve full 64-bit signature values. Sparse memory overrides must
be complete and sorted; zero/Int defaults reconstitute every other word.

The positive program does not dynamically validate traps or every trit/branch
case. No guest, native ternary hardware, performance, physical-device, Windows,
Linux, full Axion/CanonFS or operating-system conformance claim follows.
Schemas and hashes establish format/content identity, not truth or authorization.
See the risk register for the remaining bounded limitations.

**Immediate prerequisite:** obtain a narrow human-reviewed clarification/acceptance
of the raw13 Load/Store, branch operands and wrapped-difference TXor profile. Do
not rewrite frozen semantics or change this artifact to force agreement.

**Recommended next implementation task, only after that prerequisite and separate authorization:** implement one
static AArch64 freestanding evaluator invocation for **this exact frozen artifact**
after successful ExitBootServices. Keep the hosted VM unchanged, implement no
heap/filesystem/compiler/AI/network services, emit the agreed instruction-boundary
records and compare all 25 observations against the frozen oracle. Require the
same input-buffer bytes, a 64-instruction/30-second limit, actual footprint
evidence for the proposed 32 KiB workspace, and fresh guest boot evidence.
Review/accept the restored conformance gate in PR #707 through the ordinary human
process; do not treat its current unmerged state or this packet as merge authority.

No freestanding implementation or merge was performed. All generated material
remains outside the repository and separate from accepted research records.
