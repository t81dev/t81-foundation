# Investigation 09 E1: three-state ISA footprint discriminator

**Disposition: NOT SUPPORTED for the preregistered workload and recorded entry contract.**

The combined compiler-generated saved TISC executable is **1,240 bytes**.
The smallest conforming conventional Wasm module is **157 bytes**, from branch
source compiled with Clang O2 and Binaryen Oz. The ratio is **7.8981**; the
preregistered maximum was **0.95**. The complete TISC artifact therefore fails
the footprint gate. This establishes no execution-time, energy, silicon-area,
native ternary hardware, or general architectural superiority claim in either
direction. No timing measurements were collected.

## Preregistration and source identities

- [Unchanged protocol](../../docs/research/INVESTIGATION_09_TISC_T81LANG_ADVANTAGE_PROPOSAL.md): PR #604, commit `f1054438b805b541c4acfd294e6dba5a71711c05` (unmerged when execution began).
- Production implementation baseline: `c930b8c2ee7478782bf3c6d231bdd59984595ef2`.
- Initial experiment source and independent oracle: `410049655db27626d07cb2c0d0a67ba8e90470eb`.
- Independent replay/decoder and corrected adapter counters: `26b10e7f28262965d2b502a91c446837c08fd5f4`.
- [Entry contract](ENTRY_CONTRACT.md): fixed before comparative observations. No source-input specialization, threshold changes, ISA/compiler/VM fixes, or new governance mechanisms.

The experiment branch incorporates the proposal and leaves its pending branch
unchanged. Canonical H01–H08, H04-R01/R02 and historical results are unchanged.
This is a bounded I09-H1 experiment, not completion of Investigation 09.

## What executed

The driver invokes the production lexer, parser, semantic analyzer, attached IR
generator, binary emitter, full saved-program serializer/loader, and interpreter.
It retains the caller/callee, literal pools and function metadata. Runtime inputs
enter through the documented four-binding host checkpoint; no bytecode is patched.
This adapter is needed because the experiment is not a new language input facility.

The common abstract contract takes four signed scalar arguments
`(selector,a,b,c)` and returns a bounded integer encoding of the ordered results.
The decision entry encodes its actual `min(a,b)`, `-c` and final maximum together;
the independent oracle decodes and checks all three values. Both implementations
include the encoding work. This observation ABI is overhead, not an advantage.
Three-state kernels operate only on {-1,0,1}; inputs remain dynamic.

There are 213 module/input cases, including four deliberately faulting boolean
cases. Across the compiled TISC, three native C controls, and 18 Wasm variants per
module, plus the hand-authored TISC controls, **4,722 comparisons pass**. This
includes **88 expected fault observations**: division faults, explicit Wasm
unreachable traps, and isolated native trap signals on the same required RHS
paths. All remaining observations return the oracle value. Empty external-effect
order is required and observed; this experiment does not compare per-instruction
policy tracing or establish equal assurance cost.

The full three-state input domain comprises 3 negation, 9 meet, 9 join and 27
decision inputs, both in the combined module and individual kernel modules.
Negative controls cover boolean AND/OR with runtime division on the RHS, bounded
integer addition/comparison on [-3,3], and identity. Faulting RHS tests verify
actual short circuit, rather than only checking a boolean truth table.

Conventional controls use signed i64 arithmetic in Wasm32 modules, preserving the
physical scalar word width and mathematical domain. Branch, conditional-select,
and full-domain table sources are each built at **O2/O3/Oz**, with and without
**Binaryen Oz**: **144 actual Wasm artifacts**, all validated and executed by
Wasmtime in independent worker processes with fresh instances per case. All
imports are empty. Clang's default exported memory is retained and included;
smaller alternative linker/export configurations were not searched after results.
Native O3 libraries are additional semantic controls, not the denominator.

All 8 compiler-generated and 4 hand-authored TISC artifacts were independently
loaded and replayed in **249 fresh processes**, with an empty temporary CanonFS
store for every replay. Outputs/faults match the initial observations. The audit
also checks consumed arithmetic/logic flags and the exact boolean RHS division
execution decision. No warm session or unrelated benchmark is involved.

## Executable footprint and instruction ledger

All sizes below are actual complete files. No metadata was subtracted to obtain
the primary result. The Wasm minimum includes both compiler-only and
Binaryen-optimized artifacts across all preregistered source forms/levels.

| Module                      | Saved compiler TISC bytes | TISC instructions | Compact stream bytes (diagnostic) | Smallest conforming Wasm bytes |
| --------------------------- | ------------------------: | ----------------: | --------------------------------: | -----------------------------: |
| Combined four kernels       |                     1,240 |                64 |                               832 |                            157 |
| Negation                    |                       538 |                25 |                               325 |                             57 |
| Meet                        |                       538 |                25 |                               325 |                             65 |
| Join                        |                       538 |                25 |                               325 |                             65 |
| Decision plus intermediates |                       754 |                37 |                               481 |                             92 |
| Boolean short circuit       |                     1,060 |                54 |                               702 |                            115 |
| Bounded integer add/compare |                       736 |                36 |                               468 |                             67 |
| Identity                    |                       484 |                22 |                               286 |                             54 |

The actual saved instruction record uses **18 bytes** here: opcode u8,
A i32, B i64, C i32, literal-kind u8. The separate `encode` API uses 13-byte
records; it is not the saved executable format. Combined TISC file bytes
reconcile as `64*18 + 88 = 1,240`, including count/pools/metadata framing.
An in-memory `Insn` occupies 24 bytes on this platform; this is a third quantity.

The best Wasm file reconciles as 8 header bytes plus type (11), function (4),
memory (5), export (18), and code (111) sections, including section headers.
The independent byte decoder counts 64 Wasm instructions **including structured
end/else markers**; that happens to equal the TISC instruction count. These are
different operational units and do not establish equal dispatch or execution cost.
The combined TISC path executes 26–44 instructions per successful invocation,
including its four initializer instructions.

## Compiler versus ISA attribution

| Meet or join implementation                  | Complete TISC bytes | Instructions | Compact stream bytes |
| -------------------------------------------- | ------------------: | -----------: | -------------------: |
| Existing compiler                            |                 538 |           25 |                  325 |
| Hand-authored existing native min/max opcode |                 502 |           23 |                  299 |
| Hand-authored compare/branch expansion       |                 574 |           27 |                  351 |

The direct opcode saves four saved instruction records (72 bytes) against the
expanded control in the same VM and calling convention. That is bounded evidence
of instruction expressiveness. The hand-authored form also omits the compiler's
two unreachable trailing return instructions, saving another 36 bytes relative
to compiler output. No compiler optimization was implemented. Even the smaller
hand-authored file exceeds the corresponding 65-byte conventional module.

The combined compiler source also emits repeated small constants and allocates
new registers. Saved records, calling/return code, metadata, and lack of the
conventional optimizer's transformations contribute to this observed footprint.
This is an implementation/encoding result. It does not identify a fundamental
ternary computational disadvantage or justify ISA changes.

## State and support accounting

The [ledger](evidence-2026-10-27/ledger.json) distinguishes these quantities:

| Quantity                                      | TISC combined path                    | Best Wasm combined module                                        |
| --------------------------------------------- | ------------------------------------- | ---------------------------------------------------------------- |
| Common logical ABI live payload               | 32 bytes: four 64-bit input words     | Same 32-byte entry contract                                      |
| Decision intermediate payload                 | 24 bytes: meet, negation, result      | Same three 64-bit semantic values                                |
| Abstract dataflow peak in the recorded model  | 32 bytes                              | 32 bytes                                                         |
| Actual allocated register payload / tags      | 1,944 / 243 bytes                     | Native register allocation not observed                          |
| VM flat memory payload / tags                 | 12,800 / 1,600 bytes (includes stack) | Exported linear memory 65,536 bytes; no memory.grow instructions |
| Additional Wasm nonparameter locals           | Not the TISC register model           | 0 in best artifact; parameters can be overwritten                |
| Flags                                         | 3 bytes                               | Native condition/operand-stack storage not observed              |
| Trace vector allocated element storage        | 768–1,536 bytes                       | No equivalent trace required by this pure-computation contract   |
| Policy-event vector allocated element storage | 13,824 bytes                          | No equivalent policy mechanism claimed                           |

The logical model excludes immutable code constants, duplicate physical copies,
call bookkeeping and runtime metadata; these are charged in artifact or physical
state ledgers. It models required semantic values, not physical backend liveness
or process RSS. TISC retains far more allocated slots than the logical graph.
The state ledger does not establish a total-memory advantage or full native
liveness equivalence. Even if logical equality were disputed, the failed size
gate alone precludes the preregistered advantage.

State/context object sizes are 2,136/2,408 bytes; these contain some already
listed fields and vector headers and must not be added to all rows indiscriminately.
Vector storage excludes allocations inside event strings. Native/JIT code,
operand stack, allocator overhead and process RSS were not measured. No claim
about deployment footprint follows from these partial physical observations.

Measured support files are recorded separately: TISC adapter with statically
linked runtime **2,345,648 bytes**; Wasmtime shared library **23,517,920 bytes**;
Python executable launcher **52,448 bytes**; Binaryen disassembler tool
**10,071,015 bytes**. Sizes/hashes identify those files, not their entire dynamic
dependency closure. The optimizer is build support, not execution runtime.
They are excluded from the portable artifact-size metric on both sides. No
support-size superiority claim is made across these different runtime designs.

## Provenance, corrections and limitations

Execution platform: Apple M2 ARM64, 8 physical cores, 8 GiB RAM; macOS 26.5.2
(25F84). Production C++ build: Apple Clang 21.0.0, C++23, Release with effective
O3/NDEBUG; conventional frontend/linker: Homebrew Clang/LLD 22.1.1. Engine:
Wasmtime Python package **50.0.1**, including its hashed ARM64 shared library;
optimizer/disassembler: npm Binaryen **132.0.0**, with registry integrity pin.
CMake 4.3.0, Ninja 1.13.2. Tool versions, hashes, effective build/link commands,
full outputs, workload matrices, source hashes and exact artifacts are retained.

No timing uncertainty is estimated: these are exact sizes of pinned bytes.
Results are sensitive to compiler/runtime versions, the recorded observation
ABI and module framing. Linux/x86-64 and Windows were not executed. CMake and
the scripts isolate platform choices; native library flags differ on macOS
versus other platforms, and Windows tool availability/flags require validation.
Do not generalize the numerical values beyond this measured ARM64 configuration.
Saved TISC serialization portability is not independently established here.

The initial malformed boolean adapter attempted a bool-to-i32 cast; it was
rejected during preflight and corrected to ordinary integer-returning branches.
The rejected source/diagnostics are preserved in [preflight](preflight/README.md).
This was a fixture correction before the measured comparison, not a compiler fix.

An adapter counter initially omitted the faulting instruction from four TISC
fault cases. Original raw observations remain intact; `independent-replay.json`
supersedes that field and adds the state ledger. The independent decoder/replay
checks the counts against the trace. Initial authored runs used the default
store with no store instructions; independent replays correct the isolation
condition. Neither correction changes output, fault, artifact bytes, or verdict.

Invalid inputs are rejected by the shared experiment host wrapper before either
engine entry. This is explicitly a trusted typed-input comparison plus a separate
boundary wrapper check, not standalone untrusted-input validation inside each
module. Direct VM injection can bypass language typing; it is not advertised as
a production calling convention. Per-instruction policy guarantees, provenance
authenticity, recovery from hostile storage, and architecture performance are
outside this E1 contract.

## Evidence and independent reproduction

[Raw evidence archive](evidence-2026-10-27/raw-evidence.tar.gz) contains every
generated source, exhaustive oracle matrix, TISC/Wasm/native artifact, Wasm
disassembly, raw parity observation, initial hash manifest, independent replay,
command output, effective build command, and environment record. Compression is
lossless; original raw JSON is retained inside, including superseded observations.
The [SHA-256 manifest](evidence-2026-10-27/SHA256SUMS.json) verifies the archive,
standalone ledgers/manifests, and selected artifacts. All artifacts remain
experimental data, not installed product binaries.

Extract to an empty directory to inspect or replay the original byte artifacts:

```sh
mkdir -p /tmp/t81-i09-original
 tar -xzf experiments/investigation09_e1/evidence-2026-10-27/raw-evidence.tar.gz -C /tmp/t81-i09-original
```

To rebuild and rerun (requires Clang with a working wasm32 linker, Node/npm,
Python and CMake/Ninja), use an isolated tools directory. The recorded dependency
pins must remain fixed; if the necessary tools cannot be obtained, report BLOCKED.
Installations require network access and are not part of the workload measurement.

```sh
python3 -m venv /tmp/t81-i09-tools
/tmp/t81-i09-tools/bin/pip install wasmtime==50.0.1
npm install --prefix /tmp/t81-i09-binaryen --save-exact binaryen@132.0.0
cmake -S experiments/investigation09_e1 -B /tmp/t81-i09-build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/t81-i09-build --target i09_tisc_driver -j 4
/tmp/t81-i09-tools/bin/python experiments/investigation09_e1/run.py --driver /tmp/t81-i09-build/i09_tisc_driver --wasm-opt /tmp/t81-i09-binaryen/node_modules/.bin/wasm-opt --clang clang --out /tmp/t81-i09-reproduction
/tmp/t81-i09-tools/bin/python experiments/investigation09_e1/audit.py /tmp/t81-i09-reproduction --driver /tmp/t81-i09-build/i09_tisc_driver --wasm-dis /tmp/t81-i09-binaryen/node_modules/.bin/wasm-dis
```

The runner requires a nonexistent output directory so prior results cannot be
overwritten. It checks all parity gates before calculating the footprint ratio.
The auditor independently decodes/reconciles file bytes and replays TISC cases.
A reproduction on a different toolchain must retain its own manifest, bytes and
results instead of overwriting this packet. Native dynamic-library UUIDs/hashes
can vary; such binaries are controls and not the portable footprint denominator.

Validation actually performed: isolated CMake configure/build of the driver;
4,722 differential comparisons; 249 independent TISC replays; complete Wasm
byte/section decoding; expected-fault classification and short-circuit/flag checks;
freeze-integrity registry check. No full repository CI run is claimed.
