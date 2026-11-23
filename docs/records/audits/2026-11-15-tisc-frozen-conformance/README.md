# Frozen TISC semantics: differential evidence

This directory records a disagreement between the frozen ISA and the hosted
interpreter. It does not resolve that disagreement. No VM, compiler, encoding,
normative text, or golden trace is changed by the test that produces the
evidence.

**Disposition: blocked on an explicit normative decision.** Ordinary CTest
runs `t81_vm_frozen_semantics_differential_test` with no arguments and expects
exit 0. Exit 0 means the six Load/Store/JZ/JNZ disagreements are still present.
`--require-normative` is not registered with CTest. It exits 1 while those
disagreements remain. A green characterization is not conformance.

`spec/tisc-spec.md` is the frozen ISA named by `spec/README.md` and
`docs/governance/SPEC_AUTHORITY_MODEL.md`. `spec/tisc/opcode-semantics.md` and
`spec/tisc/opcode-unified-reference.md` describe the hosted operand
convention. Both live under `spec/`. This evidence does not treat either
document as superseded.

The conflict is older than this branch. Freeze commit
`f3dd5417f845195342ed143c7bb4c9d2881d3f84` already contained the core prose and
the direct-address, register-tested interpreter. The 2026-11-15 packet below
was captured against `40695b0ba5cea3e43563b6ed2443c987d2738089`. On 2026-11-23
the branch was merged with `origin/main`
`5a005b2dea5e1eb262d4af45698f8338d16b137c`. The SHA-256 of `spec/tisc-spec.md`
and `spec/tisc/opcode-semantics.md` still matches `source-hashes.json`.
`vm/vm.cpp` has changed because other opcodes were extracted; the Load, Store,
JumpIfZero, JumpIfNotZero, and TXor handlers were re-read and re-executed.
Their behavior is unchanged.

Raw instructions are 13 bytes: opcode, then A, B, and C as little-endian
32-bit fields. The differential test round-trips every fixture through
`tisc::encode` / `tisc::decode` and checks size, opcode, and operands. Nothing
in that record selects between the two readings. The integer 5 is a legal
address and a legal register index, which is why the fixtures overlap.

## Load (`0x03`) and Store (`0x04`)

`spec/tisc-spec.md` §5.6:

- `LOAD RD, [RS]` does `R[RD] := MEM[R[RS]]`.
- `STORE [RS], RD` does `MEM[R[RS]] := R[RD]`.
- Both fault on bounds or segment violations. Store also faults on a write to
  CODE or a forbidden META region.

The hosted interpreter and `opcode-semantics.md` use the immediate fields:

- Load: `R[A] := MEM[B]`, bounds-check B, copy the memory tag, update flags.
- Store: `MEM[A] := R[B]`, bounds-check A, copy the register tag.

The core prose does not name the A/B/C bytes. The comparison below binds the
two written operands in order: destination then source for Load, address then
value for Store. That binding is stated so the oracle is reproducible. The
unambiguous split is register-indirect versus immediate addressing.

Executed fixture, with both addresses in range. `R[5]=6`, `R[1]=555`,
`MEM[5]=111`, `MEM[6]=222`.

| Case | Frozen-core reading | Hosted result |
|---|---|---|
| Load A=2, B=5 | `R[2] = MEM[R[5]] = MEM[6] = 222` | `R[2] = MEM[5] = 111` |
| Store A=5, B=1 | write 555 at `MEM[R[5]]` (address 6) | write 555 at address 5; address 6 stays 222 |

`tests/cpp/vm_load_store_test.cpp` stores and loads immediate address 5. The C
frontend emits `Load` and `Store` with an immediate address. Changing the
interpreter to §5.6 would reinterpret those bytes.

### Options

1. Amend §5.6 to immediate addressing. Existing bytes, the C frontend, and
   `vm_load_store_test` keep their current meaning. The register-indirect
   sentence is retired. This is a frozen-spec change.
2. Change the interpreter and its callers to register-indirect addressing.
   The same integer stops being an address and becomes a register index.
   Hosted compiler output, the load/store test, and historical traces break
   unless old bytes are kept under an explicit profile. Freeze enforcement
   treats an operand-order change as a break, not a silent edit.
3. Name both readings as profiles. Undecorated 13-byte records stay on the
   hosted immediate-address profile, because they have no selector. New
   artifacts would have to say which profile they use. This adds a profile
   mechanism and does not make raw bytes match §5.6.

## JumpIfZero (`0x0B`) and JumpIfNotZero (`0x19`)

`spec/tisc-spec.md` §5.7:

- Form: `JZ RS` and `JNZ RS`, one operand.
- Test `FLAGS.ZERO`, or its complement.
- When the condition holds, `PC := R[RS]`. Otherwise `PC := PC + 1`.

The hosted interpreter and `opcode-semantics.md`:

- Test `R[B] == 0` or `R[B] != 0`. Flags are not consulted.
- When taken, `PC := A`, where A is an immediate code address.
- `step()` advances PC before dispatch, so a fall-through lands on the next
  instruction.

The one-operand prose is bound to A in this evidence, so a taken core branch
uses `R[5]`. Every row below makes that prose condition true. `Cmp` of `R[7]`
and `R[8]` sets the flag. The program has seven instructions, so PC 2, PC 5,
and PC 6 are all inside it. The split is not a bounds fault.

| Case | Flags and `R[B]` | Hosted PC | §5.7 PC |
|---|---|---|---|
| JumpIfZero | `flags.zero=1`, `R[B]=0` | 5, immediate A | 6, `R[5]` |
| JumpIfZero | `flags.zero=1`, `R[B]=1` | 2, fall-through | 6, `R[5]` |
| JumpIfNotZero | `flags.zero=0`, `R[B]=1` | 5, immediate A | 6, `R[5]` |
| JumpIfNotZero | `flags.zero=0`, `R[B]=0` | 2, fall-through | 6, `R[5]` |

The predicate and the target are separate disagreements. Rows 1 and 3 take
the branch for both readings and still land on different PCs. Rows 2 and 4
fall through on the VM while the prose still takes the branch.

`tests/cpp/vm_jump_flags_test.cpp` loads 0 into the condition register and
then jumps on that register. The load also sets the zero flag, so the test
does not separate the two predicates. The C frontend emits `JumpIfZero` with
the condition register in B and patches the immediate target in A. The JIT
path in `vm/jit/jit_compiler.cpp` matches the interpreter. The LLVM lowering
in `lang/llvm/tisc_to_llvm.cpp` tests `R[C]` and branches to A. That backend
was not executed. JumpIfNegative and JumpIfPositive already test flags and
jump to immediate A, which is a third shape. This evidence does not cover
them.

### Options

1. Amend §5.7 so JZ/JNZ test `R[B]` and branch to immediate A. The
   interpreter, JIT, C frontend, and opcode tables already do this. Existing
   bytes keep working. The flag-and-register-target sentence is retired for
   these two opcodes. LLVM's use of `R[C]` remains a separate backend
   mismatch and is not fixed by that amendment.
2. Change JZ/JNZ to test flags and set PC from `R[A]`. Compiler-patched
   immediates would be read as register indexes. Programs that put the
   condition in `R[B]` would break. Old bytes need an explicit profile.
3. Keep immediate target A and test flags instead of `R[B]`. This matches
   neither document completely. It is the shape JumpIfNegative and
   JumpIfPositive already implement. Programs whose register condition and
   flags disagree change behavior. Programs where a preceding instruction
   sets both the same way, such as `LoadImm` of 0 followed by `JZ` on that
   register, keep working. The four rows above are the distinguishing cases.
4. Name profiles, with the same constraint as memory option 3. A profile has
   to say which predicate operand it uses, including whether LLVM's `R[C]`
   belongs to it.

## TXor (`0x15`)

TXor is reported and is not one of the six audit failures.

- §5.3 says tritwise XOR and gives a padding rule. It does not give a truth
  table.
- `opcode-semantics.md` says "Ternary XOR / Sum" and does not define Sum.
- `opcode-unified-reference.md` says the operation is a non-commutative
  wrapped difference.
- The interpreter clamps each scalar register to {-1, 0, 1} and stores
  `wrap(lhs - rhs)`, where a difference above 1 becomes -1 and a difference
  below -1 becomes 1. It writes an Int and updates flags.

All nine scalar pairs were executed. The VM matches wrapped difference. Six
pairs disagree with wrapped sum. Wrapped sum is only a candidate reading of
the word "Sum", not a requirement found in §5.3.

### Options

1. Authorize the implemented wrapped difference as the scalar table, and
   replace the undefined word "Sum" with that table. Say whether vectors are
   lane-wise and how padding works. No VM change.
2. Choose another table, such as wrapped sum. That changes the six
   disagreeing pairs and every caller that already depends on the VM's
   difference.
3. Leave the table unspecified and keep the exhaustive diagnostic. No
   behavior change.

## How to run

CTest must keep the no-argument command. Audit mode is manual:

```sh
cmake -S . -B /tmp/t81-tisc-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF \
  -DT81_BUILD_TUI=OFF -DT81_ENABLE_TERNARYOS=OFF -DT81_ENABLE_DPE=OFF
cmake --build /tmp/t81-tisc-build --target t81_vm_frozen_semantics_differential_test
ctest --test-dir /tmp/t81-tisc-build --output-on-failure \
  -R '^t81_vm_frozen_semantics_differential_test$'
/tmp/t81-tisc-build/t81_vm_frozen_semantics_differential_test --require-normative
```

The last command exits 1 until a normative decision changes the disagreement
and this test together. An unknown argument exits 2.

## 2026-11-23 revalidation

Linux x86_64, Ubuntu clang 18.1.3, CMake 3.28.3, Ninja, Release. The
differential test exited 0 with no arguments and 1 with `--require-normative`.
Its CTest registration passed. `t81_vm_load_store_test` and
`t81_vm_jump_flags_test` exited 0. `clang-format-18 --dry-run --Werror` passed
on the differential test. Raw output is `revalidation-2026-11-23.txt`.

Not run in this refresh: the full CTest suite, sanitizers, macOS, Windows,
ARM64, golden trace replay, the freeze checker, and the conformance `.t81`
programs. Those limits are part of the record. The 2026-11-15 logs are not
relabeled as this host's results.

## 2026-11-15 packet

`SHA256SUMS` lists this directory. The logs other than this README and
`revalidation-2026-11-23.txt` are the original macOS ARM64 packet against
`40695b0ba5cea3e43563b6ed2443c987d2738089`: Apple Clang 21.0.0, Debug
configuration inheriting the repository `-O3 -DNDEBUG`, 3/3 focused CTests,
5/5 golden traces, two conformance programs, a passing freeze check, and
audit exit 1. `history-*.txt` is that session's source history. Sanitizers
and Linux/Windows were not run then either. Hashes identify bytes. They do
not establish that the VM conforms to the frozen specification.
