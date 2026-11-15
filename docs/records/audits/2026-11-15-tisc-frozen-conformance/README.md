# Frozen TISC semantics: differential evidence

Pinned `origin/main`: **`40695b0ba5cea3e43563b6ed2443c987d2738089`**, freshly fetched November 15, 2026. This workstream adds a differential test and evidence only. No VM, compiler, encoding, normative text, golden trace or existing conformance fixture was changed. It is independent of PR #798 and the documentation-only context reconciliation.

**Disposition: normative/runtime reconciliation remains BLOCKED on an explicit compatibility and authority decision.** The new characterization passes while its `--require-normative` audit mode returns 1. Passing characterization is not normative conformance. No instruction implementation should be silently switched on the strength of existing CI or historical prose alone.

## Authority and history

1. `spec/tisc-spec.md` is a core normative document under `spec/README.md` and the specification authority model. §5.6 describes register-indirect LOAD/STORE; §5.7 describes FLAGS-tested conditional branches to a register-derived target. `spec/tisc/opcode-semantics.md` instead documents the hosted direct-address / register-predicate behavior. Both are under `spec/`; no inspected accepted RFC explicitly resolves this operand conflict by superseding the core specification. The opcode table cannot silently amend the core text.
2. Freeze commit **`f3dd5417f845195342ed143c7bb4c9d2881d3f84`**, committed February 23, 2026, already contains the conflicting core prose and direct-address/register-tested VM implementation (`src/vm/vm.cpp` at that revision). This is pre-existing drift at the freeze boundary, not a recent #798 regression. The release note's internal February 15, 2025 date is documentary metadata, not the Git commit date. Its encoding-freeze statements do not supply a missing operand precedence rule. Raw source and history commands are retained in `history-*.txt`.
3. Core register-indirect/FLAGS prose is present in the November 21, 2025 specification update **`6ca979eb8046ac3ef239f7cf0fccc2c2f5626b1b`**. The alternative opcode reference was introduced at **`100d813f374ae03c5e749d360d61a7f3eb604b6a`** on February 23, 2026, before the freeze commit. Source history therefore does not support treating the alternative table as a later explicit normative correction.
4. Accepted RFC-0002 requires identical bytes/state/policy/inputs to yield identical results, traces and faults. Accepted RFC-0027 calls core-spec invariants normative and their companion programs conformance evidence. Their acceptance does not establish that every named instruction has a discriminating fixture. Accepted RFC-0049 governs arithmetic laws but supplies no TXor truth table or operand-rule override. Neither authorizes rewriting these specs to match implementation.
5. The DCP profile and freeze checker preserve the ISA contract; the checker only verifies structural registry/count/range properties. Its passing result here does not resolve any discrepancy below.

## Instruction-by-instruction assessment

| Instruction | Core normative behavior | Actual hosted VM on pinned main | Compatibility and existing coverage | Smallest proposed correction |
|---|---|---|---|---|
| Load (`0x03`) | §5.6: `LOAD RD,[RS]`, `R[RD]=MEM[R[RS]]`, canonicalized, bounds/segment faults. Mapping the two written operands to A/B is the natural convention; the prose does not explicitly define byte-field binding. | `R[A]=MEM[B]`; copies the memory tag, updates flags, bounds-checks immediate B. | `vm_load_store_test.cpp` stores/loads literal address 5 and asserts round trip, enforcing direct addressing. New fixture has `R[5]=6`, `MEM[5]=111`, `MEM[6]=222`: actual 111 versus indirect oracle 222. Changing the old path would reinterpret existing bytes. | Once the authoritative profile/operand binding is approved: validate B as a register, resolve `R[B]`, bounds-check resolved address, preserve tag/fault/flag ordering. Preserve old direct-address artifacts under their explicit historical interpretation; no heuristic autodetection. No production patch applied. |
| Store (`0x04`) | §5.6: `STORE [RS],RD`, `MEM[R[RS]]=R[RD]`; bounds/segment/CODE/META restrictions. | `MEM[A]=R[B]`; copies the register tag; bounds-checks immediate A. | Same existing direct-address fixture. New fixture writes address 5 while `R[5]=6`; indirect oracle would write 6. | After the same decision: validate A as a register, resolve `R[A]`, apply existing segment checks to the resolved address, then copy value/tag. Audit aliases and trap-side-effect order. Reinterpreting historical bytes in place is incompatible. |
| JumpIfZero (`0x0B`) | §5.7: test FLAGS.ZERO; when true set PC to the target held in the sole source register, otherwise advance PC. The table-to-byte operand binding is underspecified; the new indirect oracle maps that sole target operand to A explicitly. | Tests `R[B]==0`, branches to immediate A; otherwise retains the already incremented PC. Taken target is bounds/CODE checked. | Existing VM workloads and compiler lowering use immediate patched targets and a register condition. The bounded-loop golden records JZ at PC 9, but normally correlated flags/register values do not distinguish predicate conventions. New cases distinguish both predicate and target. | After authority/compatibility decision: test `flags.zero`, validate the target register and resolved code address when taken, preserve fallthrough and trap ordering. An unconditional replacement would break hosted compiler output and historical traces. |
| JumpIfNotZero (`0x19`) | Same target convention; test the complement of FLAGS.ZERO. | Tests `R[B]!=0`, branches to immediate A. | Existing workloads and lowering use register predicates. New cases show actual PC 2 (opposing predicates) or 5 (same predicates), versus prose-oracle PC 6. | Same bounded branch correction as JZ, using `!flags.zero`, only after explicit profile/compatibility approval. No ISA expansion is implemented. |
| TXor (`0x15`) | §5.3 says tritwise “XOR definitions” and padding, but supplies no complete truth table. Alternative table says “Ternary XOR / Sum.” Neither uniquely defines addition, difference, or multi-trit behavior. | Clamps each scalar register by sign to {-1,0,1}, then computes balanced `wrap3(lhs-rhs)`, writes an Int and updates flags. This is noncommutative. | Existing packed-trit tests explicitly adopt wrapped difference from the VM, so backend agreement is not independent normative authority. The cited `ternary-logic-canonical.t81` checks TNOT/TAND/TOR/De Morgan; it never executes TXor. New test executes all nine scalar pairs: six disagree with the candidate balanced wrapped-sum table. That is a comparison, not a finding that sum is normative. | First obtain an explicit authorized truth table, scalar/vector domains, padding/tag/fault rules, and compatibility ruling. Keep the exhaustive test and historical difference results. Do not change subtraction to addition based only on the word “Sum,” and do not declare the current table normatively validated. |

Logical addressing and conditional predicates have clear prose-level differences. Exact byte-field bindings and TXor's mathematical definition still require an authoritative decision. This distinction prevents an inference from being mislabeled as settled normative text.

## Binary encoding and artifact compatibility

`isa/encoding.cpp` serializes **13-byte** raw records: opcode byte, A/B/C as three little-endian 32-bit fields. The new differential test round-trips every tested program through this API before executing it and actively checks record size, opcode and operand preservation. These counterexamples use small positive operands; no claim about wide-B truncation, negative-field decoding or all encoding domains is made.

The saved `.tisc` program API is a separate surface: `isa/binary_io.cpp` stores vector framing, opcode/A/B/C/literal-kind records (18 bytes per instruction on the inspected ABI: 1+4+8+4+1), followed by pools/metadata. It must not be confused with the raw 13-byte API. Neither inspected instruction record contains a selector distinguishing the conflicting operand interpretations. A literal-kind tag does not resolve memory/branch semantics. No raw-byte or saved-artifact migration was attempted.

There is no sound “if the operand looks like a register” repair: the same integer 5 is a valid direct address and register index, and the differential fixtures deliberately exploit that overlap. The minimal safe change today is evidence and tests. A later normative implementation correction needs explicit artifact interpretation/versioning policy and review of compiler output; this report proposes no unapproved ISA extension or automatic historical reinterpretation.

Optional LLVM/MLIR are not conformance controls here: inspected LLVM lowering tests R[C] for JZ/JNZ while the hosted interpreter tests R[B]. They were not enabled or executed. Do not generalize hosted results to those backends.

## Executed tests and limits

Local environment: macOS ARM64, Apple Clang 21.0.0, CMake 4.3.0. Fresh CMake build from this branch; Debug configuration still inherits repository `-O3 -DNDEBUG`. The new checks use `T81_TEST_CHECK` and remain active. The old `t81_isa_encoding_test` uses `assert`, so its passing executable alone does not prove its assertions ran in this configuration. The new active round-trip checks supply the relevant executed evidence.

- New differential test, existing load/store test and encoding executable: **3/3 CTests pass**. Direct `t81_vm_new_opcodes_test` also exits 0 (that executable is not registered in the inspected CTest list).
- Strict prose audit: **exit 1**, retaining six distinguishing memory/branch cases. This is the intended unresolved-conformance result, not a green normative suite.
- Existing ternary-logic and bounds conformance programs both compile and execute successfully. Their observed domains do not distinguish the five disputed contracts; the bounds fixture's negative fault cases are comments pending `@expect_fault`.
- Golden trace replay: **5/5 pass**, including the 51-entry bounded loop. Aggregate hash: `8be31748f609d27af1193cb7530125716302eed54f14ab0c1339cb576a6274eb`. This establishes compatibility for those fixtures on this host, not full ISA semantics or cross-platform parity.
- Freeze checker passes. No sanitizer configuration or Linux/Windows execution was run for this new workstream; earlier PR #798 sanitizer evidence is separate and is not relabeled as this branch's validation.

The test is a compatibility characterization with an explicit failing prose-audit mode. Its success must never be summarized as “the VM conforms to the frozen specification.” All observed behavior predates this branch. No scientific experiment, hardware measurement, benchmark campaign or research/release hold changed.

## Reproduction commands

```sh
cmake -S . -B /private/tmp/t81-tisc-build -DCMAKE_BUILD_TYPE=Debug \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF \
  -DT81_BUILD_TUI=OFF -DT81_ENABLE_TERNARYOS=OFF \
  -DFETCHCONTENT_SOURCE_DIR_ASIO=/private/tmp/t81-798-evidence/asio
cmake --build /private/tmp/t81-tisc-build --target t81_vm_frozen_semantics_differential_test t81_vm_load_store_test t81_vm_new_opcodes_test t81_isa_encoding_test t81 -j 5
ctest --test-dir /private/tmp/t81-tisc-build --output-on-failure --no-tests=error -R '^(t81_vm_(frozen_semantics_differential|load_store)_test|t81_isa_encoding_test)$'
/private/tmp/t81-tisc-build/t81_vm_frozen_semantics_differential_test --require-normative
python3 scripts/ci/check_tisc_freeze_integrity.py
python3 scripts/ci/trace_repro_gate.py --t81-bin /private/tmp/t81-tisc-build/t81 --fixtures-dir tests/fixtures/t81lang_determinism --traces-dir tests/fixtures/canonical_traces --workdir /private/tmp/t81-tisc-traces --hash-out /private/tmp/t81-tisc-traces/hash.txt
```

The Asio source directory is tag `asio-1-30-2`, commit `12e0ce9e0500bf0f247dbd1ae894272656456079`, downloaded during the preceding review. `fixture-results.json` retains exact compile/run argv for the two conformance programs. `source-hashes.json` pins inspected main sources. Raw command outputs, including the failing audit, are retained beside this report; source and tests remain the authority over this AI-authored analysis. The reviewer wrote these new tests in the same session and does not claim independent human review or historical author independence.
