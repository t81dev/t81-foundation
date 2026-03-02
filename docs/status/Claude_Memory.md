# T81 Foundation - Claude Memory

## Project Identity
- **Name:** T81 Foundation
- **Purpose:** Deterministic ternary virtual machine + governance stack for high-stakes AI, cryptography, and scientific computing
- **Core concept:** Balanced ternary logic {-1, 0, +1}, bit-exact reproducibility, frozen immutable ISA
- **Repo:** /Users/t81dev/Code/t81-foundation

## Tech Stack
- **Language:** C++20/23 (primary), Python bindings, JS (doc indexing only)
- **Build:** CMake 3.16+, presets in CMakePresets.json; also has BUILD.bazel
- **Compilers:** AppleClang 17+, Clang 18+, GCC 14+, MSVC
- **Optional:** ASAN/UBSAN, llama.cpp adapter (T81_ENABLE_LLAMA_CPP), LLVM interface

## Key Directory Map
- `core/` — Frozen foundation: types (T81BigInt, T81Float), ISA (TISC), VM interpreter
- `kernel/axion/` — Axion Policy Kernel: governance enforcement, audit trails, CanonFS hooks
- `runtime/` — Tracing (CanonHash81), experimental Trace-JIT
- `lang/frontend/` — T81Lang: lexer → parser → semantic analyzer → TISC bytecode
- `src/` — codec (Base81/Base243), canonfs, c_api, crypto, simd (AVX2), tensor, io
- `include/t81/` — Public C++ API headers
- `spec/` — FROZEN normative specs (tisc-spec.md, t81lang-spec.md, t81vm-spec.md, axion-kernel.md)
- `docs/architecture/OVERVIEW.md` — Architecture authority doc ("North Star")
- `tooling/cli/main.cpp` — Main CLI entry point → `./build/t81` executable
- `tests/cpp/` — 300+ C++ unit/integration tests
- `experimental/` — Cognitive tiers (Tier1-5), distributed, hanoi (archived)
- `examples/` — Demos: axion, llama32, canonfs, tensor_ops, ir_roundtrip

## Architecture Pattern
```
T81Lang (source)
    ↓ lang/frontend (lexer/parser/semantic)
TISC Bytecode
    ↓ kernel/axion (policy enforcement intercept)
T81VM (core/vm) - deterministic interpreter
    ↓
Foundation: TISC ISA + ternary data types (FROZEN per major version)
```

## CMake Libraries (12 core)
t81_core → t81_axion, t81_io, t81_c_api, t81_lang_frontend, t81_isa, t81_vm, t81_jit, t81_llvm, t81_tool_model, t81_tool_cli
- Main executable: `t81` (tooling/cli/main.cpp)

## Key CMake Flags
- T81_BUILD_TESTS, T81_BUILD_EXAMPLES, T81_BUILD_BENCHMARKS
- T81_USE_CXX23, T81_STRICT_DETERMINISTIC_FLOAT, T81_TRITWISE_PROFILE
- T81_ENABLE_ASAN, T81_ENABLE_UBSAN, T81_ENABLE_LLAMA_CPP

## Important Conventions
- **Spec authority:** /spec > docs/architecture > /docs > /book (never modify spec without major version bump)
- **Determinism Gates:** CI reproducibility checks; CanonHash81 for bit-exact tracing
- **Cognitive Tiers:** Tier1 (symbolic) → Tier5 (infinite) — computation abstraction levels
- **T81W format:** Custom weight format for model serialization
- **CanonFS:** Canonical filesystem for immutable audit trails
- **Policy as Code:** Axion YAML/JSON policies intercepting TISC opcodes at runtime

## Key Files
- `CMakeLists.txt` — Main build config (~82KB, complex linkage tree)
- `spec/tisc-spec.md` — Frozen ISA spec
- `core/vm/vm.cpp` — VM interpreter
- `kernel/axion/policy_engine.cpp` — Governance enforcement
- `tooling/cli/main.cpp` — `t81` CLI entry point
- `tests/determinism/` — Reproducibility verification

## Recent Work (from git log)
- fa343e1: Merge from main
- c16dcd69: Remove test_from_chars
- dd90a21b: Merge PR #424 (dtype-closure-loop-graph)
- c93ed484: Lower T81Graph to VM native opcodes + tests

## Conformance Suite Status (2026-03-02)
- 21/24 → 22/24 passing: Fixed `policy-enforcement-allow-deny.t81` (Result.unwrap_ok TypeFault)
  - Fix: SA `visit(CallExpr)` caches object type in `_expr_type_cache` for method dispatch
- 22/24 → pending 23rd: `tier-annotation-enforcement.t81` still DecodeFaults
- 24th still pending: `type-kind-completeness.t81` (byte-string, Set/Map literals)

## TShape Opcode (Added this session)
- `include/t81/isa/opcodes.hpp` — Added `TShape` after `SetSize`
- `include/t81/isa/ir.hpp` — Added `TSHAPE` after `TSET`
- `core/isa/binary_emitter.cpp` — Added `TSHAPE` → `TShape` mapping
- `core/vm/vm.cpp` — Added TShape VM handler: `A = shape[R[C]] of tensor R[B]`

## Matrix 2D Indexing (Added this session, partially working)
- SA: `visit(VectorLiteralExpr)` handles Matrix context → returns Matrix type
- SA: `visit(IndexExpr)` handles `Matrix[T][i] → Vector[T]` and `Map[K,V][K] → V`
- SA: `visit(VariableExpr)` now caches to `_expr_type_cache` (line ~4845 sa.cpp)
- IR gen: `visit(VectorLiteralExpr)` — Matrix literal → 2D tensor pool entry + LOADI
- IR gen: `visit(IndexExpr)` — double-index `m[row][col]` → TSHAPE+MUL+ADD+TGET+F2I
- **KNOWN ISSUE**: DecodeFault in conformance program when multiple functions present
  - `canonical_types` + `tensor_rank` + `recursion` combo fails; individual pairs work
  - SUSPECTED: register clobbering across recursive calls (flat 243-register VM file, no save/restore)
  - Test programs at /tmp/test_tier_*.t81 for debugging

## Key SA/IR Architecture Notes
- `_expr_type_cache`: `unordered_map<const Expr*, Type>` populated by `evaluate_expression()`
- `evaluate_expression()` auto-caches types for all visited expressions
- `visit(VariableExpr)` now also caches explicitly (defensive fix)
- IR generator registers: global counter `_register_count` (starts at 1, skips 75-80)
- VM register file: flat 243 registers (R0..R242) shared across all function calls
- `add_tensor()` returns 1-based handle; `tensor_ptr(h)` does `h-1` for 0-based index

## Last Log


Bash Build t81 after SA fix and debug print removal
IN
cmake --build build --target t81 -j$(sysctl -n hw.logicalcpu) 2>&1 | tail -20

OUT
[1/5] Building CXX object CMakeFiles/t81_lang_frontend.dir/lang/frontend/semantic_analyzer.cpp.o
[2/5] Linking CXX static library libt81_lang_frontend.a
[3/5] Building CXX object CMakeFiles/t81_vm.dir/core/vm/vm.cpp.o
[4/5] Linking CXX static library libt81_vm.a
[5/5] Linking CXX executable t81
ld: warning: ignoring duplicate libraries: 'libt81_isa.a', 'libt81_lang_frontend.a', 'libt81_vm.a'
Bash Test policy-enforcement-allow-deny.t81
IN
./build/t81 run spec/conformance/axion-kernel/policy-enforcement-allow-deny.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-policy-enforcement-allow-deny-11789430725897606796.tisc
Program terminated normally
policy-enforcement-allow-deny.t81 now passes. Run the full conformance suite:

Bash Run all 24 conformance programs and report pass/fail
IN
for f in spec/conformance/**/*.t81; do result=$(./build/t81 run "$f" 2>&1); if echo "$result" | grep -q "terminated normally"; then echo "PASS: $f"; elif echo "$result" | grep -q "Assertion\|TypeFault\|error\|Error\|fault\|Fault"; then echo "FAIL: $f"; echo "  $(echo "$result" | tail -3)"; else echo "UNKNOWN: $f"; echo "  $(echo "$result" | tail -3)"; fi; done

OUT
PASS: spec/conformance/axion-kernel/metadata-determinism.t81
PASS: spec/conformance/axion-kernel/policy-deny-requires-reason.t81
PASS: spec/conformance/axion-kernel/policy-enforcement-allow-deny.t81
PASS: spec/conformance/axion-kernel/segment-trace-strings.t81
PASS: spec/conformance/axion-kernel/tier-supervision-invariant.t81
FAIL: spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81
          m[0][0] == 1 && m[1][1] == 4
                         ^
error: Semantic errors encountered
PASS: spec/conformance/t81-data-types/canonical-encoding.t81
PASS: spec/conformance/t81-data-types/canonical-ordering.t81
FAIL: spec/conformance/t81-data-types/type-kind-completeness.t81
  spec/conformance/t81-data-types/type-kind-completeness.t81:58:41: error: Expect ';' after expression.
spec/conformance/t81-data-types/type-kind-completeness.t81:58:41: error: Expect '}' after block. (expected '}', found '=>')
error: Parse errors encountered
PASS: spec/conformance/t81-data-types/widening-binary-interop.t81
PASS: spec/conformance/t81-data-types/widening-order.t81
PASS: spec/conformance/t81-data-types/widening-upper-chain.t81
PASS: spec/conformance/t81vm/axion-log-completeness.t81
PASS: spec/conformance/t81vm/determinism-profile.t81
PASS: spec/conformance/tisc/arithmetic-determinism.t81
PASS: spec/conformance/tisc/bitwise-determinism.t81
PASS: spec/conformance/tisc/bitwise-shift-masking.t81
PASS: spec/conformance/tisc/bounds-fault-contract.t81
PASS: spec/conformance/tisc/comparison-total-order.t81
PASS: spec/conformance/tisc/conversion-determinism.t81
PASS: spec/conformance/tisc/division-truncation.t81
PASS: spec/conformance/tisc/fraction-normalization.t81
PASS: spec/conformance/tisc/ternary-logic-canonical.t81
PASS: spec/conformance/tisc/tier-restriction.t81
22/24 — one more fixed. Two remain:

tier-annotation-enforcement.t81 — Matrix 2D indexing (m[0][0])
type-kind-completeness.t81 — Map literal => parse error
Let me look at both programs:

Read tier-annotation-enforcement.t81
Read type-kind-completeness.t81
Let me get the full compiler error messages for both programs to understand exactly what's needed:

Bash Full error output for tier-annotation-enforcement
IN
./build/t81 run spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81 2>&1

OUT
Exit code 1
warning: 'run' is a legacy alias; prefer 't81 code run'.
spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81:85:9: error: Cannot assign initializer of type 'Vector[Vector[i32]]' to constant of type 'Matrix[T81BigInt]'.
        let m: Matrix[T81BigInt] = [[1, 2], [3, 4]];
            ^
spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81:86:6: error: Expression 'm' of type 'Matrix[T81BigInt]' does not support indexing.
        m[0][0] == 1 && m[1][1] == 4
         ^
spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81:86:22: error: Expression 'm' of type 'Matrix[T81BigInt]' does not support indexing.
        m[0][0] == 1 && m[1][1] == 4
                         ^
error: Semantic errors encountered

warning: 'run' is a legacy alias; prefer 't81 code run'.
spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81:85:9: error: Cannot assign initializer of type 'Vector[Vector[i32]]' to constant of type 'Matrix[T81BigInt]'.
        let m: Matrix[T81BigInt] = [[1, 2], [3, 4]];
            ^
spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81:86:6: error: Expression 'm' of type 'Matrix[T81BigInt]' does not support indexing.
        m[0][0] == 1 && m[1][1] == 4
         ^
spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81:86:22: error: Expression 'm' of type 'Matrix[T81BigInt]' does not support indexing.
        m[0][0] == 1 && m[1][1] == 4
                         ^
error: Semantic errors encountered
Bash Full error output for type-kind-completeness
IN
./build/t81 run spec/conformance/t81-data-types/type-kind-completeness.t81 2>&1

OUT
<tool_use_error>Sibling tool call errored</tool_use_error>
Bash Full error output for type-kind-completeness
IN
./build/t81 run spec/conformance/t81-data-types/type-kind-completeness.t81 2>&1

OUT
Exit code 1
warning: 'run' is a legacy alias; prefer 't81 code run'.
spec/conformance/t81-data-types/type-kind-completeness.t81:27:27: error: Expect ';' after constant declaration. (expected ';', found '"\x74\x38\x31"')
spec/conformance/t81-data-types/type-kind-completeness.t81:57:38: error: Expect ';' after expression.
spec/conformance/t81-data-types/type-kind-completeness.t81:57:38: error: Expect '}' after block. (expected '}', found ',')
spec/conformance/t81-data-types/type-kind-completeness.t81:58:41: error: Expect ';' after expression.
spec/conformance/t81-data-types/type-kind-completeness.t81:58:41: error: Expect '}' after block. (expected '}', found '=>')
error: Parse errors encountered

warning: 'run' is a legacy alias; prefer 't81 code run'.
spec/conformance/t81-data-types/type-kind-completeness.t81:27:27: error: Expect ';' after constant declaration. (expected ';', found '"\x74\x38\x31"')
spec/conformance/t81-data-types/type-kind-completeness.t81:57:38: error: Expect ';' after expression.
spec/conformance/t81-data-types/type-kind-completeness.t81:57:38: error: Expect '}' after block. (expected '}', found ',')
spec/conformance/t81-data-types/type-kind-completeness.t81:58:41: error: Expect ';' after expression.
spec/conformance/t81-data-types/type-kind-completeness.t81:58:41: error: Expect '}' after block. (expected '}', found '=>')
error: Parse errors encountered
I need to understand the current semantic analyzer's handling of index expressions and the parser's expression parsing. Let me explore:

Grep "does not support indexing|IndexExpr|visit.*Index" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
6 lines of output
Grep "Matrix|Set\[|Map\[|ByteString|T81Bytes" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/semantic_analyzer.hpp)
1 line of output
Read semantic_analyzer.cpp (lines 4429-4478)
Now let me look at the assignment compatibility check:

Grep "Cannot assign|types_compatible|is_assignable|assign.*initializer" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
21 lines of output
Read semantic_analyzer.cpp (lines 1378-1417)
Read semantic_analyzer.cpp (lines 1417-1456)
Let me check how Matrix is represented at the IR/VM level:

Grep "Matrix|MATRIX|matrix" (in /Users/t81dev/Code/t81-foundation/include/t81/isa)
1 line of output
Grep "Kind::Matrix|Matrix.*Kind|type_to_string.*Matrix" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
6 lines of output
Read semantic_analyzer.cpp (lines 1291-1315)
Grep "Matrix|Kind::Set|Kind::Map|ByteString|b\"" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
12 lines of output
Grep "visit.*IndexExpr|IndexExpr.*visit|VECGET|VECSET|index.*matrix|matrix.*index" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
1 line of output
Read ir_generator.hpp (lines 5095-5164)
Grep "TGET|TensorHandle|MatrixHandle|tensor_ptr\|matrix_ptr" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
21 lines of output
Grep "case Opcode::TGET|Opcode::TGet" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
1 line of output
Read vm.cpp (lines 3760-3819)
Grep "VECGET|VecGet|VectorGet|VecLoad|VectorHandle" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
16 lines of output
Grep "case Opcode::VGet|VGet|VGET|case Opcode::Load" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
2 lines of output
Grep "promote_to_tensor|VectorHandle|vector_ptr|vector_get" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
21 lines of output
Read vm.cpp (lines 533-582)
Grep "visit.*VectorLiteral|VectorLiteral.*visit|VEC_PUSH|VECPUSH" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
14 lines of output
Read ir_generator.hpp (lines 5015-5094)
Grep "vector_literal_data" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
5 lines of output
Read semantic_analyzer.cpp (lines 4601-4730)
Let me look at STRVECGET opcode and how StringVectorHandle elements are accessed:

Grep "STRVECGET|StrVecGet|case.*StrVec" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
3 lines of output
Grep "case Opcode::Str|Opcode::STRLEN|Equal.*StrVec|StrVec.*Equal" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
2 lines of output
Grep "StringVectorHandle" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
28 lines of output
Read vm.cpp (lines 2721-2800)
Bash Search TISC spec for TGet definition
IN
grep -n "TGet\|T_GET\|tget\|TGET" /Users/t81dev/Code/t81-foundation/spec/tisc-spec.md | head -30

OUT
0
Bash Search TISC spec for tensor operations
IN
grep -n -i "tensor.*get\|get.*tensor\|tensor.*element\|rank" /Users/t81dev/Code/t81-foundation/spec/tisc-spec.md | head -20

OUT
0
Bash Search all spec files for tensor get operation
IN
grep -rn -i "tget\|tensor get\|tensor_get" /Users/t81dev/Code/t81-foundation/spec/ | head -20

OUT
/Users/t81dev/Code/t81-foundation/spec/tisc/opcode-unified-reference.md:164:| TGet | 107 | `0x6B` | A, B, C | `A: Dest, B: Tens, C:Idx` | - | Tensor Get Element | Gets element at `R[C]` from tensor `R[B]`. | BoundsFault | Yes | Implemented | core/vm/vm.cpp |
/Users/t81dev/Code/t81-foundation/spec/tisc/opcode-registry.md:136:| TGet | 107 (0x6B) | A, B, C | Tensor Get Element | Yes | core/vm/vm.cpp |
/Users/t81dev/Code/t81-foundation/spec/tisc/opcode-semantics.md:287:| `0x6B` | `TGET` | `A: Dest, B: Tens, C:Idx` | - | Gets element at `R[C]` from tensor `R[B]`. | BoundsFault |
/Users/t81dev/Code/t81-foundation/spec/tisc/opcode-semantics.md:402:108. `0x6B` TGET
Read vm.cpp (lines 2960-3039)
Bash Search for tensor shape/rank opcodes in TISC spec
IN
grep -n -i "tshape\|tensor.*shape\|trank\|tensor.*rank\|tsize\|tensor.*size" /Users/t81dev/Code/t81-foundation/spec/tisc/opcode-unified-reference.md | head -20

OUT
77:| TVecAdd | 33 | `0x21` | A, B, C | `A: Dest, B: T1, C: T2` | - | Tensor Vector Addition | Tensor element-wise addition. | ShapeFault | Yes | Implemented | core/vm/vm.cpp |
78:| TMatMul | 34 | `0x22` | A, B, C | `A: Dest, B: T1, C: T2` | - | Tensor Matrix Multiplication | Tensor Matrix Multiplication. | ShapeFault | Yes | Implemented | core/vm/vm.cpp |
79:| TTenDot | 35 | `0x23` | A, B, C | `A: Dest, B: T1, C: T2` | - | Tensor Dot Product | Tensor Dot Product (Contraction). | ShapeFault | Yes | Implemented | core/vm/vm.cpp |
89:| TVecMul | 79 | `0x4F` | A, B, C | `A: Dest, B: T1, C: T2` | - | Tensor Vector Multiplication | Tensor element-wise multiplication. | ShapeFault | Yes | Implemented | core/vm/vm.cpp |
155:| ChkShape | 45 | `0x2D` | A, B, C | Semantic Detail Pending | - | Check Tensor Shape | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | core/vm/vm.cpp |
161:| TRMSNorm | 77 | `0x4D` | A, B, C | `A: Dest, B: Tens, C: W` | - | Tensor RMSNorm | Applies RMSNorm with weights `R[C]`. | ShapeFault | Yes | Implemented | core/vm/vm.cpp |
162:| TRoPE | 78 | `0x4E` | A, B, C | `A: Dest, B: Tens, C: Pos` | - | Tensor RoPE | Applies Rotary Positional Embedding. | ShapeFault | Yes | Implemented | core/vm/vm.cpp |
163:| TTranspose | 80 | `0x50` | A, B, C | `A: Dest, B: Tens` | - | Tensor Transpose | Transposes tensor (2D). | ShapeFault | Yes | Implemented | core/vm/vm.cpp |
165:| TNew | 108 | `0x6C` | A, B, C | `A: Dest, B: Size` | - | Tensor New | Allocates new tensor of size `R[B]`. | BoundsFault | Yes | Implemented | core/vm/vm.cpp |
Bash Find ChkShape implementation in VM
IN
grep -n "ChkShape\|CHKSHAPE" /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp | head -10

OUT
3298:      case t81::tisc::Opcode::ChkShape: {
Read vm.cpp (lines 3299-3328)
You can add TSHAPE
Grep "add_tensor|program.*tensor|_program\.tensor" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
2 lines of output
The user says we can add TSHAPE. Let me look at the opcode infrastructure:

Globpattern: "include/t81/isa/*.hpp"
Found 10 files
Globpattern: "include/t81/isa/*.h"
No files found
Read opcodes.hpp
Read ir.hpp (lines 2-81)
Read ir.hpp (lines 81-200)
There's already Map/Set scaffolding in the opcode list. Let me check what's implemented in the VM and what the lexer supports:

Bash Check VM Map/Set implementation status
IN
grep -n "MapNew\|MapGet\|MapPut\|SetNew\|SetAdd\|SetHas\|Arrow\|FatArrow\|\"=>\"" /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp | head -20

OUT
4683:      case t81::tisc::Opcode::MapNew:
4684:      case t81::tisc::Opcode::SetNew: {
4694:      case t81::tisc::Opcode::MapPut: {
4702:        // So: dest_reg = MapPut(map_reg, key_reg, val_reg) ??
4704:        // MapPut dest, map, key ?? No, TISC is 3 operands max.
4735:      case t81::tisc::Opcode::MapGet: {
4872:      case t81::tisc::Opcode::SetAdd: {
4927:      case t81::tisc::Opcode::SetHas: {
Bash Check lexer for arrow and byte-string support
IN
grep -n "Arrow\|FatArrow\|\"=>\"\|b\"\\\\x\|ByteStr\|ByteString" /Users/t81dev/Code/t81-foundation/lang/frontend/lexer.cpp | head -20

OUT
145:      return make_token(match('>') ? TokenType::Arrow : TokenType::Minus);
152:      if (match('>')) return make_token(TokenType::FatArrow);
FatArrow is already in the lexer. Let me check the parser and b"..." byte-string support:

Bash Check parser for Set/Map literal and byte-string handling
IN
grep -n "FatArrow\|SetLiteral\|MapLiteral\|ByteStr\|b\"" /Users/t81dev/Code/t81-foundation/lang/frontend/parser.cpp | head -20

OUT
220:    case TokenType::FatArrow:
1485:      if (!check(TokenType::FatArrow) && !check(TokenType::If)) {
1493:    if (check(TokenType::FatArrow)) {
1500:  consume(TokenType::FatArrow, "Expect '=>' after match arm pattern.");
Bash Check token types for FatArrow and byte-string
IN
grep -n "TokenType::FatArrow\|ByteStr\|TokenType::Bytes" /Users/t81dev/Code/t81-foundation/include/t81/frontend/lexer.hpp 2>/dev/null || grep -n "FatArrow\|ByteStr\|T81Bytes" /Users/t81dev/Code/t81-foundation/include/t81/frontend/tokens.hpp 2>/dev/null | head -20

OUT
0
Bash Check lexer for byte-string token generation
IN
grep -rn "FatArrow\|ByteStr\|T81Bytes\|ByteString\|byte_string" /Users/t81dev/Code/t81-foundation/lang/frontend/lexer.cpp | head -20

OUT
/Users/t81dev/Code/t81-foundation/lang/frontend/lexer.cpp:152:      if (match('>')) return make_token(TokenType::FatArrow);
Bash Check lexer for byte-string support
IN
grep -n "ByteString\|byte_string\|T81Bytes\|Bytes" /Users/t81dev/Code/t81-foundation/lang/frontend/lexer.cpp | head -20

OUT
0
Bash Check lexer header for byte-string token types
IN
grep -n "Bytes\|ByteString" /Users/t81dev/Code/t81-foundation/lang/frontend/lexer.hpp | head -20

OUT
grep: /Users/t81dev/Code/t81-foundation/lang/frontend/lexer.hpp: No such file or directory
Bash Check token enum for Bytes types
IN
grep -n "Bytes\|ByteString\|byte_str" /Users/t81dev/Code/t81-foundation/include/t81/frontend/lexer.hpp 2>/dev/null; grep -n "Bytes\|byte_str" /Users/t81dev/Code/t81-foundation/include/t81/frontend/tokens.hpp 2>/dev/null | head -20

OUT
0
Globpattern: "include/t81/frontend/*.hpp"
Found 6 files
Grep "Bytes|ByteString|byte_str" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/lexer.hpp)
2 lines of output
Grep "enum class TokenType|TokenType" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/lexer.hpp)
4 lines of output
Read lexer.hpp (lines 12-141)
Now I have enough context. Let me update todos and begin implementing all three features in parallel:

Update Todos

Mark policy-enforce fix as done (SA obj type cache)

Add TSHAPE opcode + VM impl + binary_emitter

SA+IR Matrix type 2D indexing support

Lexer: b-string literal + Parser: Set/Map literal syntax

SA+IR byte-string, Set, Map, contains, map-index support

Update docs and commit all fixes
Let me look at the parser's expression handling for { and the binary emitter to understand how to add the opcode:

Bash Find parser primary expression handler and LBrace handling
IN
grep -n "LBrace\|parse_primary\|parse_expression\|SetLiteral\|MapLiteral" /Users/t81dev/Code/t81-foundation/lang/frontend/parser.cpp | head -30

OUT
204:    case TokenType::LBrace:
463:  consume(TokenType::LBrace, "Expect '{' before body.");
503:  consume(TokenType::LBrace, ("Expect '{' before " + kind + " body.").c_str());
541:  consume(TokenType::LBrace, "Expect '{' after record name.");
574:  consume(TokenType::LBrace, "Expect '{' after enum name.");
666:    consume(TokenType::LBrace, "Expect '{' after 'reflect'.");
672:    consume(TokenType::LBrace, "Expect '{' after 'distributed'.");
678:    consume(TokenType::LBrace, "Expect '{' after 'infinite'.");
687:    consume(TokenType::LBrace, "Expect '{' before train body.");
719:  if (match({TokenType::LBrace})) {
749:  consume(TokenType::LBrace, "Expect '{' after 'loop'.");
802:          check(TokenType::Assert) || check(TokenType::LBrace)) {
808:          if (check(TokenType::LBrace)) {
844:        if (check(TokenType::LBrace)) {
896:  // statement() matches LBrace and calls block(). So statement() consumed LBrace.
897:  // function() consumes LBrace and calls block().
898:  // loop() consumes LBrace and calls block().
899:  // So block() should NOT consume LBrace.
911:  consume(TokenType::LBrace, "Expect '{' before block.");
1218:  } else if (check(TokenType::LBrace)) {
1256:    } else if (check(TokenType::LBrace) && !name.lexeme.empty() &&
1270:    consume(TokenType::LBrace, "Expect '{' after '∞'.");
1321:  consume(TokenType::LBrace, "Expect '{' before match arms.");
1377:  if (match({TokenType::LBrace})) {
Bash Check binary emitter for TGET/TNEW handling
IN
grep -n "case ir::Opcode::TGET\|case ir::Opcode::TG\|case ir::Opcode::TN\|case ir::Opcode::TS\|case ir::Opcode::TSHAPE" /Users/t81dev/Code/t81-foundation/core/isa/binary_emitter.cpp | head -10

OUT
0
Bash Find binary emitter tensor opcode handling
IN
grep -n "case ir::Opcode::TGET\|case ir::Opcode::T\b\|TGet\|TNew\|TSet\|TGET\|TNEW\|TSET" /Users/t81dev/Code/t81-foundation/core/isa/binary_emitter.cpp | head -20

OUT
282:    case O::TGET:
283:      return Opcode::TGet;
284:    case O::TNEW:
285:      return Opcode::TNew;
286:    case O::TSET:
287:      return Opcode::TSet;
Read binary_emitter.cpp (lines 271-310)
Read binary_emitter.cpp (lines 306-335)
Read binary_emitter.cpp (lines 334-353)
Read parser.cpp (lines 201-225)
Read parser.cpp (lines 1211-1290)
Bash Check lexer's peek/lookahead capabilities
IN
grep -n "peek_next\|peek\|lookahead\|save\|restore\|mark" /Users/t81dev/Code/t81-foundation/include/t81/frontend/lexer.hpp | head -20

OUT
147:  Token peek_next_token();
151:  char peek() const;
152:  char peek_next() const;
Bash Look at lexer peek_next_token implementation
IN
grep -n "peek_next_token\|save\|checkpoint" /Users/t81dev/Code/t81-foundation/lang/frontend/lexer.cpp | head -10

OUT
346:Token Lexer::peek_next_token() {
347:  auto saved_current = _current;
348:  auto saved_line = _line;
349:  auto saved_line_start = _line_start;
350:  auto saved_token_start = _token_start;
352:  _current = saved_current;
353:  _line = saved_line;
354:  _line_start = saved_line_start;
355:  _token_start = saved_token_start;
Read lexer.cpp (lines 347-361)
Bash Understand parser token management
IN
grep -n "peek()\|advance()\|previous()\|_current\|_lexer" /Users/t81dev/Code/t81-foundation/lang/frontend/parser.cpp | head -20

OUT
314:    : _lexer(lexer), _source_name(std::move(source_name)) {
315:  // Prime the pump by fetching the first token. This ensures that `_current`
317:  _current = _lexer.next_token();
345:      advance();
355:  return peek().type == type;
359:Token Parser::advance() {
361:    _previous = _current;
362:    _current = _lexer.next_token();
364:  return previous();
368:bool Parser::is_at_end() { return peek().type == TokenType::Eof; }
371:Token Parser::peek() { return _current; }
374:Token Parser::previous() { return _previous; }
379:  if (check(type)) return advance();
380:  const Token found = peek();
391:  advance();
393:    if (previous().type == TokenType::Semicolon) return;
394:    switch (peek().type) {
405:    advance();
422:      const Token& anchor = struct_attrs->anchor.value_or(peek());
428:      const Token& anchor = function_attrs->anchor.value_or(peek());
Bash Check AST for Set/Map literal nodes
IN
grep -n "SetLiteral\|MapLiteral\|ByteStr\|Bytes" /Users/t81dev/Code/t81-foundation/include/t81/frontend/ast.hpp | head -20

OUT
0
Bash List all AST node types
IN
grep -n "struct.*Expr\|class.*Expr\|struct.*Stmt\|class.*Stmt" /Users/t81dev/Code/t81-foundation/include/t81/frontend/ast.hpp | head -50

OUT
17:class ExprVisitor;
18:class StmtVisitor;
20:struct Expr;
21:struct Stmt;
22:struct BinaryExpr;
23:struct UnaryExpr;
24:struct LiteralExpr;
25:struct GroupingExpr;
26:struct VariableExpr;
27:struct CallExpr;
28:struct AssignExpr;
29:struct MatchExpr;
30:struct VectorLiteralExpr;
31:struct FieldAccessExpr;
32:struct RecordLiteralExpr;
33:struct EnumLiteralExpr;
34:struct SymbolLiteralExpr;
35:struct InfiniteLiteralExpr;
36:struct IndexExpr;
37:struct BlockExpr;
38:struct IfExpr;
39:struct TypeExpr;         // Base class for type expressions
40:struct SimpleTypeExpr;   // For simple types like "T81Int"
41:struct GenericTypeExpr;  // For generic types like "Vector[T]"
42:struct InferExpr;
43:struct ExpressionStmt;
44:struct VarStmt;
45:struct LetStmt;
46:struct BlockStmt;
47:struct IfStmt;
48:struct WhileStmt;
49:struct ForStmt;
50:struct ReflectStmt;
51:struct RecurseStmt;
52:struct DistributedStmt;
53:struct InfiniteStmt;
54:struct TrainStmt;
55:struct ReturnStmt;
56:struct AssertStmt;
57:struct BreakStmt;
58:struct ContinueStmt;
59:struct FunctionStmt;
60:struct LoopStmt;
67:struct Expr {
72:struct Stmt {
79:class ExprVisitor {
104:class StmtVisitor {
132:struct BinaryExpr : Expr {
143:struct UnaryExpr : Expr {
152:struct LiteralExpr : Expr {
Bash Get RecordLiteralExpr structure
IN
grep -n -A 15 "struct RecordLiteralExpr" /Users/t81dev/Code/t81-foundation/include/t81/frontend/ast.hpp

OUT
32:struct RecordLiteralExpr;
33-struct EnumLiteralExpr;
34-struct SymbolLiteralExpr;
35-struct InfiniteLiteralExpr;
36-struct IndexExpr;
37-struct BlockExpr;
38-struct IfExpr;
39-struct TypeExpr;         // Base class for type expressions
40-struct SimpleTypeExpr;   // For simple types like "T81Int"
41-struct GenericTypeExpr;  // For generic types like "Vector[T]"
42-struct InferExpr;
43-struct ExpressionStmt;
44-struct VarStmt;
45-struct LetStmt;
46-struct BlockStmt;
47-struct IfStmt;
--
209:struct RecordLiteralExpr : Expr {
210-  RecordLiteralExpr(Token type_name, std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields)
211-      : type_name(type_name), fields(std::move(fields)) {}
212-
213-  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }
214-
215-  const Token type_name;
216-  const std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields;
217-};
218-
219-struct EnumLiteralExpr : Expr {
220-  EnumLiteralExpr(Token enum_name, Token variant, std::unique_ptr<Expr> payload)
221-      : enum_name(enum_name), variant(variant), payload(std::move(payload)) {}
222-
223-  std::any accept(ExprVisitor& visitor) const override { return visitor.visit(*this); }
224-
Read vm.cpp (lines 4684-4803)
Read vm.cpp (lines 4861-4980)
Bash Find symbol_like_text function definition
IN
grep -n "symbol_like_text" /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp | head -20

OUT
596:    auto symbol_like_text = [&](ValueTag tag,
916:      if (auto lhs = symbol_like_text(tag, lhs_val); lhs.has_value()) {
917:        auto rhs = symbol_like_text(tag, rhs_val);
1016:      if (auto symbol = symbol_like_text(tag, val_data); symbol.has_value()) {
1106:      auto msg = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
1131:      auto msg = symbol_like_text(ctx.register_tags[insn.a], ctx.registers[insn.a]);
1389:        auto symbol = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
2545:        auto symbol = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
2560:        auto symbol = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
2838:        auto lhs = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
2839:        auto rhs = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
2856:        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
2857:        auto prefix = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
2874:        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
2875:        auto suffix = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
2893:        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
2894:        auto needle = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
2910:        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
2911:        auto needle = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
2927:        auto source = symbol_like_text(ctx.register_tags[insn.a], ctx.registers[insn.a]);
Read vm.cpp (lines 597-621)
Read vm.cpp (lines 580-597)
Bash Find lexer identifier and string scanning functions
IN
grep -n "scan_token\|identifier\|string\|is_alpha\|advance()" /Users/t81dev/Code/t81-foundation/lang/frontend/lexer.cpp | head -40

OUT
9:inline std::string_view make_sv(const char* b, const char* e) noexcept {
10:  return std::string_view(b, static_cast<std::size_t>(e - b));
14:inline std::string_view make_sv(It b, It e) noexcept {
15:  return std::string_view(&*b, static_cast<std::size_t>(std::distance(b, e)));
19:const std::unordered_map<std::string_view, TokenType> KEYWORDS = {
77:bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
82:Lexer::Lexer(std::string_view source)
95:  char c = advance();
100:      advance();  // Consume 0x88
101:      advance();  // Consume 0x9E
106:  if (is_alpha(c)) return identifier();
170:      return string();
186:char Lexer::advance() {
198:  std::string_view lexeme = make_sv(_token_start, _current);
208:Token Lexer::string() {
214:    advance();
217:  if (is_at_end()) return error_token("Unterminated string.");
219:  advance();  // closing quote
226:    advance();  // consume 'x'/'X'
228:      advance();  // consume hex digit or separator
234:  while (is_digit(peek()) || peek() == '_') advance();
239:    advance();
240:    while (is_digit(peek()) || peek() == '_') advance();
247:      advance();  // Consume 'e'
248:      advance();  // Consume '+' or '-'
249:      while (is_digit(peek())) advance();
251:      advance();  // Consume 'e'
252:      while (is_digit(peek())) advance();
258:    advance();
265:    advance();
266:    advance();
267:    advance();
273:    advance();
280:Token Lexer::identifier() {
283:    if (is_alpha(next) || is_digit(next)) {
284:      advance();
290:  std::string_view text = make_sv(_token_start, _current);
299:  if (is_alpha(peek())) {
300:    while (is_alpha(peek()) || is_digit(peek())) advance();
315:        advance();
Are you almost done?
Read lexer.cpp (lines 281-330)
22/24 are now passing. The two remaining programs require substantial new language features (Matrix 2D indexing, TShape opcode, byte-string literals, Set/Map literals). I'm implementing them now — let me proceed with the code.

Starting with Matrix + TShape (the simpler of the two), then Set/Map/bytes. I'll work through all the files in sequence.

Step 1: Add TShape to the opcode enum:

Edit opcodes.hpp
Added 1 line
Now add TShape to opcode_name and update the kAllOpcodes array size (it currently uses SetSize as the max):

Edit opcodes.hpp
Added 2 lines
Step 2: Add TSHAPE to the IR opcode enum:

Edit ir.hpp
Edit failed
Read ir.hpp (lines 141-150)
Edit ir.hpp
Added 1 line
Step 3: Add TSHAPE to the binary emitter:

Edit binary_emitter.cpp
Edit failed
Read binary_emitter.cpp (lines 281-292)
Edit binary_emitter.cpp
Added 2 lines
Step 4: Implement TShape in the VM. The default: trap case is at the end — add TShape before default::

Read vm.cpp (lines 4973-4982)
Edit vm.cpp
Added 31 lines
Update Todos

Add TSHAPE opcode + VM impl + binary_emitter

SA+IR Matrix type 2D indexing support

Lexer: b-string literal + Parser: Set/Map literal syntax

SA+IR byte-string, Set, Map, contains, map-index support

Update docs and commit all fixes
Step 5: SA Matrix fixes — visit(VectorLiteralExpr) for Matrix context and visit(IndexExpr) for Matrix indexing:

Read semantic_analyzer.cpp (lines 4617-4656)
Edit semantic_analyzer.cpp
Added 10 lines
Now fix visit(IndexExpr) to handle Matrix[T][i] → Vector[T]:

Edit semantic_analyzer.cpp
Added 13 lines
Step 6: IR generator — Matrix visit(VectorLiteralExpr) and visit(IndexExpr).

First let me look at the VectorLiteralExpr visitor in the IR to find where to add Matrix handling:

Read ir_generator.hpp (lines 5015-5044)
Edit ir_generator.hpp
Added 35 lines
Now fix visit(IndexExpr) to handle Matrix double-indexing with TSHAPE+MUL+ADD+TGET:

Read ir_generator.hpp (lines 5128-5147)
Edit ir_generator.hpp
Added 83 lines
Now let's build and test the Matrix program:

Bash Build after Matrix/TShape changes
IN
cmake --build build --target t81 -j$(sysctl -n hw.logicalcpu) 2>&1 | tail -20

OUT
[29/43] Building CXX object CMakeFiles/t81_vm.dir/core/vm/memory_segments.cpp.o
[30/43] Building CXX object CMakeFiles/t81_vm.dir/core/vm/policy_trace_bridge.cpp.o
[31/43] Building CXX object CMakeFiles/t81_vm.dir/core/vm/runtime_state_helpers.cpp.o
[32/43] Building CXX object CMakeFiles/t81_vm.dir/core/vm/gc_helpers.cpp.o
[33/43] Building CXX object CMakeFiles/t81_jit.dir/runtime/jit/jit_compiler.cpp.o
/Users/t81dev/Code/t81-foundation/runtime/jit/jit_compiler.cpp:306:23: warning: unused variable 'stack' [-Wunused-variable]
  306 |           const auto& stack = state.layout.stack;
      |                       ^~~~~
1 warning generated.
[34/43] Building CXX object CMakeFiles/t81_vm.dir/core/vm/tensor_helpers.cpp.o
[35/43] Building CXX object CMakeFiles/t81_tool_cli.dir/tooling/cli/debugger.cpp.o
[36/43] Building CXX object CMakeFiles/t81_lang_frontend.dir/lang/frontend/semantic_analyzer.cpp.o
[37/43] Linking CXX static library libt81_lang_frontend.a
[38/43] Building CXX object CMakeFiles/t81_vm.dir/core/vm/vm.cpp.o
[39/43] Linking CXX static library libt81_vm.a
[40/43] Building CXX object CMakeFiles/t81_tool_cli.dir/tooling/cli/driver.cpp.o
[41/43] Building CXX object CMakeFiles/t81.dir/tooling/cli/main.cpp.o
[42/43] Linking CXX static library libt81_tool_cli.a
[43/43] Linking CXX executable t81
ld: warning: ignoring duplicate libraries: 'libt81_isa.a', 'libt81_lang_frontend.a', 'libt81_vm.a'
Bash Test tier-annotation-enforcement.t81
IN
./build/t81 run spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81 2>&1

OUT
Exit code 14
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-tier-annotation-enforcement-2530961712598961292.tisc
error: Execution trapped: DecodeFault

warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-tier-annotation-enforcement-2530961712598961292.tisc
error: Execution trapped: DecodeFault
There's a DecodeFault. Let me check how MUL and ADD in the IR map to TISC opcodes, and whether the TShape opcode might collide with an invalid value:

Bash Check how MUL/ADD IR opcodes map to TISC
IN
grep -n "case O::MUL\|case O::ADD\|case O::IMUL\|case O::IADD" /Users/t81dev/Code/t81-foundation/core/isa/binary_emitter.cpp | head -10

OUT
108:    case O::ADD:
110:    case O::MUL:
Read binary_emitter.cpp (lines 107-126)
Bash Find map_primitive_opcode implementation
IN
grep -n "map_primitive_opcode" /Users/t81dev/Code/t81-foundation/core/isa/binary_emitter.cpp | head -5

OUT
10:Opcode map_primitive_opcode(ir::Opcode ir_op, ir::PrimitiveKind kind) {
113:      return map_primitive_opcode(instr.opcode, instr.primitive);
122:      return map_primitive_opcode(instr.opcode, instr.primitive);
Read binary_emitter.cpp (lines 11-65)
Bash Check where is_valid_opcode is called in VM
IN
grep -n "is_valid_opcode\|valid.*opcode\|opcode.*valid" /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp | head -10

OUT
0
Bash Find is_valid_opcode usage in VM
IN
grep -n "is_valid_opcode\|DecodeFault.*opcode\|opcode.*decode" /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp | head -10

OUT
0
Bash Find is_valid_opcode usage across core
IN
grep -rn "is_valid_opcode" /Users/t81dev/Code/t81-foundation/core/ | head -10

OUT
/Users/t81dev/Code/t81-foundation/core/isa/encoding.cpp:33:    if (!is_valid_opcode(opcode)) {
Read encoding.cpp (lines 29-43)
Bash Check how LOADI maps to TISC
IN
grep -n "case O::LOADI" /Users/t81dev/Code/t81-foundation/core/isa/binary_emitter.cpp | head -5

OUT
158:    case O::LOADI:
Read binary_emitter.cpp (lines 159-168)
Bash Find VarExpr visitor in SA
IN
grep -n "visit.*VarExpr\|VarExpr.*visit\|evaluate_expression.*VarExpr" /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp | head -5

OUT
0
Bash Find VariableExpr visitor in SA
IN
grep -n "visit.*Variable\|VariableExpr" /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp | head -5

OUT
192:  using t81::frontend::VariableExpr;
193:  if (const auto* var = dynamic_cast<const VariableExpr*>(&expr)) {
1158:  if (auto* variable = dynamic_cast<const VariableExpr*>(&expr)) {
1587:  if (auto* variable = dynamic_cast<const VariableExpr*>(&expr)) return variable->name;
2102:  if (auto* variable = dynamic_cast<const VariableExpr*>(expr.target.get())) {
Bash Find VariableExpr visit implementation
IN
grep -n "SemanticAnalyzer::visit.*Variable" /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp | head -5

OUT
4833:std::any SemanticAnalyzer::visit(const VariableExpr& expr) {
Read semantic_analyzer.cpp (lines 4834-4853)
This session is being continued from a previous conversation that ran out of context. The summary below covers the earlier portion of the conversation.

Analysis:
Let me analyze this conversation chronologically and thoroughly.

## Session Context
This is a continuation session. The previous context had been working on T81 Foundation conformance suite fixes. The session started with a plan file showing the long-term engineering plan.

## What happened in this session:

### Fix 1: policy-enforce-allow-deny.t81 (22nd program)
The root cause was diagnosed before this session: `Result.unwrap_ok()` TypeFault at runtime because the IR generator emits `OPTION_UNWRAP` instead of `RESULT_UNWRAP_OK` for `r.unwrap()` where `r: Result[T81BigInt, T81String]`. This happened because `_semantic->type_of(fa->object.get())` returns null for VarExpr objects that were never evaluated via `evaluate_expression` in the SA.

**Fix**: In `semantic_analyzer.cpp` `visit(const CallExpr&)`, after resolving `obj_symbol`, cache the object expression's type in `_expr_type_cache`:
```cpp
if (obj_symbol) {
  if (const auto* fa = dynamic_cast<const FieldAccessExpr*>(expr.callee.get())) {
    _expr_type_cache[fa->object.get()] = obj_symbol->type;
  }
}
```

### Debug Print Removal
Two debug prints removed from `core/vm/vm.cpp`:
1. `[DBG ResultUnwrapOk]` in `ResultUnwrapOk` handler
2. `[DBG trap]` at trap return point

### Testing after fixes: 22/24 passing
Full conformance run showed:
- `tier-annotation-enforcement.t81` — Matrix 2D indexing (`m[0][0]`)  
- `type-kind-completeness.t81` — byte-string/Set/Map literal syntax

### TShape opcode addition
User explicitly said "You can add TSHAPE".

**Plan**: Add `TShape` opcode to support getting a tensor's shape dimension, enabling Matrix 2D flat-index computation.

Files modified:
1. `include/t81/isa/opcodes.hpp` - Added `TShape` after `SetSize`, updated `opcode_name`, updated `kAllOpcodes` and `is_valid_opcode` to use `TShape` instead of `SetSize`
2. `include/t81/isa/ir.hpp` - Added `TSHAPE` after `TSET`
3. `core/isa/binary_emitter.cpp` - Added `case O::TSHAPE: return Opcode::TShape;`
4. `core/vm/vm.cpp` - Added `TShape` implementation before `default:` case

### SA Matrix fixes
In `lang/frontend/semantic_analyzer.cpp`:
1. `visit(VectorLiteralExpr)` - Added Matrix[T] expected context: when expected is Matrix, visit each element as a row (Vector[T]) and return Matrix[T]
2. `visit(IndexExpr)` - Added `Matrix[T][i]` → `Vector[T]` and `Map[K,V][K]` → `V` cases

### IR Matrix fixes
In `include/t81/frontend/ir_generator.hpp`:
1. `visit(VectorLiteralExpr)` - Added Matrix[T] literal handling: collect inner vector data, build 2D tensor, store in program pool
2. `visit(IndexExpr)` - Added double-index Matrix detection (TSHAPE+MUL+ADD+TGET) and Map index handling (MapGet+OPTION_UNWRAP)

### Build + Test
Build succeeded but `tier-annotation-enforcement.t81` hit `DecodeFault` at runtime.

Currently investigating why — likely issue with `_semantic->type_of(inner->object.get())` for `VarExpr("m")`. The SA's `visit(const VariableExpr&)` returns the type but doesn't cache it in `_expr_type_cache`. So when the IR generator checks `_semantic->type_of(inner->object.get())`, it returns null and the Matrix 2D path is not taken.

## Pending issues for type-kind-completeness.t81:
- `b"\x74\x38\x31"` byte-string literal (lexer needs `ByteString` token)
- `{10, 20}` Set literal (parser needs disambiguation from block)
- `{:x => 1, :y => 2}` Map literal (parser needs `{symbol => expr}` syntax)
- `s.contains(10)` - Set `.contains()` method
- `m[:x] == 1` - Map indexing

## Current work at time of summary:
Working on why `tier-annotation-enforcement.t81` gets DecodeFault. Was checking:
1. Whether `VariableExpr` type is cached in `_expr_type_cache` (it's NOT — `visit(VariableExpr)` doesn't cache)
2. This means `_semantic->type_of(inner->object.get())` returns null, so the Matrix 2D path won't be taken

Summary:
1. Primary Request and Intent:
   The user said "proceed" (continuing from previous session where 21/24 conformance programs were passing) and is working toward all 24/24 conformance programs passing in the T81 Foundation project. The key goals are:
   - Fix `policy-enforcement-allow-deny.t81` (22nd program) — Result.unwrap_ok() TypeFault
   - Fix `tier-annotation-enforcement.t81` (23rd program) — Matrix 2D indexing (`m[0][0]`)
   - Fix `type-kind-completeness.t81` (24th program) — byte-string literals, Set/Map literals
   - User explicitly said "You can add TSHAPE" when discussing how to implement Matrix 2D indexing
   - User asked "Are you almost done?" mid-session (session is still in progress at 22/24)

2. Key Technical Concepts:
   - **T81 Foundation**: Deterministic ternary VM, T81Lang compiler, TISC ISA
   - **Conformance suite**: 24 spec programs in `spec/conformance/` that must pass `./build/t81 run`
   - **`_expr_type_cache`**: SA's `unordered_map<const Expr*, Type>` — populated by `evaluate_expression()`. `visit(VariableExpr)` does NOT cache its result here, causing `type_of(var_expr)` to return null
   - **SA method dispatch**: SA uses `resolve_symbol` (symbol table) not `evaluate_expression` for object type in `CallExpr`, causing method call objects' types to be absent from `_expr_type_cache`
   - **Matrix 2D indexing plan**: Store `[[1,2],[3,4]]` as a 2D T729DynamicTensor (shape [2,2], data [1,2,3,4]). For `m[row][col]`: emit `TSHAPE ncols, matrix, 1; MUL prod, row, ncols; ADD flat, prod, col; TGET result, matrix, flat; F2I`
   - **TShape opcode**: `TShape A, B, C` — gets dimension R[C] from tensor R[B]'s shape into R[A]
   - **Map/Set VM representation**: StringVectorHandle-based; `symbol_like_text()` only handles SymbolHandle/StringHandle (not Int)
   - **Lexer `peek_next_token()`**: Saves and restores 4 lexer state fields; provides one token of lookahead
   - **`VarExpr` type caching**: `visit(const VariableExpr&)` returns `symbol->type` but does NOT call `_expr_type_cache[&expr] = ...`, so `type_of(var_expr_ptr)` returns null in the IR generator
   - **Binary emitter**: Maps `ir::Opcode` → `tisc::Opcode` via switch statement; opcode enum uses auto-increment after explicit values 0-67

3. Files and Code Sections:
   - **`lang/frontend/semantic_analyzer.cpp`** (MODIFIED)
     - Added object type caching after `obj_symbol` resolution in `visit(const CallExpr&)` to fix `Result.unwrap_ok()` TypeFault:
       ```cpp
       // After line 2375 (auto* obj_symbol = resolve_symbol(...))
       if (obj_symbol) {
         if (const auto* fa = dynamic_cast<const FieldAccessExpr*>(expr.callee.get())) {
           _expr_type_cache[fa->object.get()] = obj_symbol->type;
         }
       }
       ```
     - Added Matrix[T] expected context in `visit(VectorLiteralExpr)`:
       ```cpp
       if (expected && expected->kind == Type::Kind::Matrix) {
         Type row_type{Type::Kind::Vector};
         if (!expected->params.empty()) row_type.params.push_back(expected->params[0]);
         for (const auto& element : expr.elements) {
           evaluate_expression(*element, &row_type);
         }
         return *expected;  // Matrix[T]
       }
       ```
     - Added Matrix/Map indexing in `visit(IndexExpr)`:
       ```cpp
       // Matrix[T][i] → Vector[T] (a row)
       if (obj_type.kind == Type::Kind::Matrix) {
         Type row_type{Type::Kind::Vector};
         if (!obj_type.params.empty()) row_type.params.push_back(obj_type.params[0]);
         return row_type;
       }
       // Map[K,V][K] → V
       if (obj_type.kind == Type::Kind::Map) {
         if (obj_type.params.size() >= 2) return obj_type.params[1];
         return Type{Type::Kind::Unknown};
       }
       ```

   - **`core/vm/vm.cpp`** (MODIFIED)
     - Removed two debug prints: `[DBG ResultUnwrapOk]` and `[DBG trap]`
     - Added `TShape` implementation before the `default:` case:
       ```cpp
       case t81::tisc::Opcode::TShape: {
         if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
           trap = Trap::DecodeFault; break;
         }
         if (auto res = promote_to_tensor(insn.b); !res) { trap = res.error(); break; }
         auto* tensor = tensor_ptr(ctx.registers[insn.b]);
         if (!tensor) { trap = Trap::DecodeFault; break; }
         if (ctx.register_tags[insn.c] != ValueTag::Int) { trap = Trap::TypeFault; break; }
         std::int64_t dim_idx = ctx.registers[insn.c];
         const auto& shape = tensor->shape();
         if (dim_idx < 0 || static_cast<std::size_t>(dim_idx) >= shape.size()) {
           trap = Trap::BoundsFault; break;
         }
         ctx.registers[insn.a] = static_cast<std::int64_t>(shape[static_cast<std::size_t>(dim_idx)]);
         ctx.register_tags[insn.a] = ValueTag::Int;
         update_flags(ctx.registers[insn.a]);
         break;
       }
       ```

   - **`include/t81/isa/opcodes.hpp`** (MODIFIED)
     - Added `TShape` after `SetSize`:
       ```cpp
       SetHas,
       SetSize,
       TShape,  // Get tensor shape dimension: A=Dest, B=Tens, C=DimIdx
       };
       ```
     - Updated `opcode_name`: added `case Opcode::TShape: return "TShape";`
     - Updated `kAllOpcodes` and `is_valid_opcode` to use `Opcode::TShape` (was `SetSize`)

   - **`include/t81/isa/ir.hpp`** (MODIFIED)
     - Added `TSHAPE` after `TSET` in `ir::Opcode` enum

   - **`core/isa/binary_emitter.cpp`** (MODIFIED)
     - Added mapping `case O::TSHAPE: return Opcode::TShape;` after `TSET` case

   - **`include/t81/frontend/ir_generator.hpp`** (MODIFIED)
     - Added Matrix 2D literal building in `visit(VectorLiteralExpr)`:
       ```cpp
       if (vector_type && vector_type->kind == Type::Kind::Matrix && !expr.elements.empty()) {
         std::vector<float> flat_data;
         int ncols = 0;
         for (const auto& row_expr : expr.elements) {
           const auto* row_data = _semantic ? _semantic->vector_literal_data(
               dynamic_cast<const VectorLiteralExpr*>(row_expr.get())) : nullptr;
           if (row_data) {
             if (ncols == 0) ncols = static_cast<int>(row_data->size());
             flat_data.insert(flat_data.end(), row_data->begin(), row_data->end());
           } else { flat_data.clear(); ncols = 0; break; }
         }
         if (!flat_data.empty() && ncols > 0) {
           int nrows = static_cast<int>(expr.elements.size());
           t81::T729DynamicTensor tensor({nrows, ncols}, flat_data);
           int handle = _program.add_tensor(std::move(tensor));
           auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
           tisc::ir::Instruction instr;
           instr.opcode = tisc::ir::Opcode::LOADI;
           instr.operands = {dest.reg, tisc::ir::Immediate{handle}};
           instr.literal_kind = tisc::LiteralKind::TensorHandle;
           emit(instr);
           record_result(&expr, dest);
           return {};
         }
       }
       ```
     - Added Matrix 2D double-index and Map index handling in `visit(IndexExpr)`:
       ```cpp
       // Matrix 2D indexing: detect (inner_IndexExpr)[col] where inner.object : Matrix[T]
       if (_semantic) {
         if (const auto* inner = dynamic_cast<const IndexExpr*>(expr.object.get())) {
           const Type* inner_obj_type = _semantic->type_of(inner->object.get());
           if (inner_obj_type && inner_obj_type->kind == Type::Kind::Matrix) {
             // ... emit TSHAPE ncols, matrix, 1; MUL prod, row, ncols; ADD flat, prod, col;
             //     TGET temp, matrix, flat; F2I dest, temp ...
           }
         }
       }
       // Map indexing: map[key] → OptionUnwrap(MapGet(map, key))
       if (_semantic) {
         const Type* obj_sem_type = _semantic->type_of(expr.object.get());
         if (obj_sem_type && obj_sem_type->kind == Type::Kind::Map) {
           // ... emit MapGet + OPTION_UNWRAP ...
         }
       }
       ```

   - **`spec/conformance/axion-kernel/policy-enforcement-allow-deny.t81`** (READ)
     - Conformance program testing Result.unwrap_ok() — now passing after SA fix
   
   - **`spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81`** (READ)
     - Lines 85-86: `let m: Matrix[T81BigInt] = [[1, 2], [3, 4]]; m[0][0] == 1 && m[1][1] == 4`
     - Still failing with `DecodeFault` after changes
   
   - **`spec/conformance/t81-data-types/type-kind-completeness.t81`** (READ)
     - Lines 27: `let raw: T81Bytes = b"\x74\x38\x31";` — ByteString literal not parsed
     - Lines 57-59: Set/Map literals `{10, 20}`, `{:x => 1, :y => 2}` — parse errors
     - Still failing with parse errors

4. Errors and fixes:
   - **`Result.unwrap_ok()` TypeFault (policy-enforcement-allow-deny.t81)**: IR emits `OPTION_UNWRAP` instead of `RESULT_UNWRAP_OK` because `_semantic->type_of(fa->object.get())` returns null for VarExpr objects in method calls. Fixed by caching `_expr_type_cache[fa->object.get()] = obj_symbol->type` in SA's `visit(CallExpr)`.
   - **22/24 now passing**: After the SA fix and debug print removal
   - **`tier-annotation-enforcement.t81` DecodeFault after Matrix changes**: Build succeeds but runtime DecodeFault. Root cause being investigated — likely `_semantic->type_of(inner->object.get())` returns null for `VarExpr("m")` since `visit(VariableExpr)` does NOT populate `_expr_type_cache`. When null, the Matrix 2D path in `visit(IndexExpr)` is not taken, falling through to the regular `TGET` path which breaks on nested tensor access.

5. Problem Solving:
   - **Solved**: policy-enforcement-allow-deny.t81 (22nd program) — now passing
   - **Solved**: TShape opcode added to ISA, binary emitter, VM
   - **In progress**: tier-annotation-enforcement.t81 — SA/IR Matrix changes are in place but runtime DecodeFault suggests `type_of(VarExpr)` returns null, bypassing the Matrix 2D index path
   - **Pending**: type-kind-completeness.t81 — needs lexer/parser/SA/IR/VM changes for byte-string, Set, Map

6. All user messages:
   - "proceed" — first message, continuing from previous session
   - "You can add TSHAPE" — explicit permission to add TShape opcode to ISA
   - "Are you almost done?" — asking for status update

7. Pending Tasks:
   - **Fix DecodeFault in tier-annotation-enforcement.t81**: The Matrix 2D index path in IR `visit(IndexExpr)` requires `_semantic->type_of(inner->object.get())` to return non-null for `VarExpr("m")`. Fix: ensure `visit(VariableExpr)` caches its type in `_expr_type_cache`, OR use a fallback lookup (e.g., check symbol table via `resolve_symbol`). The SA's `visit(VariableExpr)` currently just returns `symbol->type` without caching it.
   - **type-kind-completeness.t81** — three sub-problems:
     1. `b"\x74\x38\x31"` byte-string: add `ByteString` TokenType to `lexer.hpp`; lex `b"..."` in `lexer.cpp`; handle in SA and IR
     2. `{10, 20}` Set literal: add `peek_nth_token(n)` to lexer, detect Set pattern in parser primary(); use VectorLiteralExpr with `{` bracket; SA/IR emit SetNew+SetAdd; VM extend `symbol_like_text` to handle Int
     3. `{:x => 1, :y => 2}` Map literal: detect Map pattern (Symbol followed by =>) in parser; use RecordLiteralExpr with Map type_name OR new MapLiteralExpr; SA/IR emit MapNew+MapPut; VM MapGet returns numeric value when value string is parseable as int
     4. `s.contains(10)` — SA method dispatch for Set.contains → Bool; IR emits SetHas
     5. `m[:x] == 1` — already handled in SA (Map[K,V] indexing → V) and IR (MapGet+OPTION_UNWRAP), but VM MapGet needs to detect numeric values
   - **Update docs and commit** — after all 24/24 pass

8. Current Work:
   The last activity was investigating why `tier-annotation-enforcement.t81` gets a `DecodeFault` after adding the Matrix 2D indexing changes. The build succeeded and the program compiles but fails at runtime.

   The investigation was looking at `lang/frontend/semantic_analyzer.cpp` line 4833's `visit(const VariableExpr&)`:
   ```cpp
   std::any SemanticAnalyzer::visit(const VariableExpr& expr) {
     // ...
     auto* symbol = resolve_symbol(expr.name);
     if (!symbol) { error(...); return make_error_type(); }
     return symbol->type;  // ← does NOT call _expr_type_cache[&expr] = symbol->type
   }
   ```
   
   Since `visit(VariableExpr)` doesn't cache to `_expr_type_cache`, `_semantic->type_of(inner->object.get())` (where `inner->object` is `VarExpr("m")`) returns null in the IR generator's Matrix 2D detection code, causing the Matrix 2D path to be skipped and the normal TGET path to be used instead, which then fails on the nested TensorHandle access.

9. Optional Next Step:
   Fix the `DecodeFault` in `tier-annotation-enforcement.t81` by ensuring `_semantic->type_of(VarExpr("m"))` returns `Matrix[T81BigInt]`. The fix is in `lang/frontend/semantic_analyzer.cpp`'s `visit(const VariableExpr&)` — add caching:
   ```cpp
   std::any SemanticAnalyzer::visit(const VariableExpr& expr) {
     // ... existing code ...
     auto* symbol = resolve_symbol(expr.name);
     if (!symbol) { ... }
     _expr_type_cache[&expr] = symbol->type;  // ADD THIS
     return symbol->type;
   }
   ```
   This ensures the IR generator's `_semantic->type_of(inner->object.get())` correctly returns `Matrix[T81BigInt]` for `VarExpr("m")`, enabling the Matrix 2D double-index path (TSHAPE+MUL+ADD+TGET) to be taken.

If you need specific details from before compaction (like exact code snippets, error messages, or content you generated), read the full transcript at: /Users/t81dev/.claude/projects/-Users-t81dev-Code-t81-foundation/a52fe282-22ea-4fc7-8fa4-36328c04bc97.jsonl
Please continue the conversation from where we left off without asking the user any further questions. Continue with the last task that you were asked to work on.
Update Todos

Fix DecodeFault in tier-annotation-enforcement.t81 (cache VariableExpr type)

Fix type-kind-completeness.t81 (byte-string, Set/Map literals)

Update docs/status after 24/24
Grep "visit\(const VariableExpr&" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
5 lines of output
Read semantic_analyzer.cpp (lines 4834-4863)
Now I can see the fix needed. Line 4845 returns symbol->type without caching it. Let me add the cache:

Edit semantic_analyzer.cpp
Added 1 line
Now let's build and test:

Bash Build t81 after VariableExpr type caching fix
IN
cmake --build build --target t81 -j$(sysctl -n hw.ncpu) 2>&1 | tail -5

OUT
[1/3] Building CXX object CMakeFiles/t81_lang_frontend.dir/lang/frontend/semantic_analyzer.cpp.o
[2/3] Linking CXX static library libt81_lang_frontend.a
[3/3] Linking CXX executable t81
ld: warning: ignoring duplicate libraries: 'libt81_isa.a', 'libt81_lang_frontend.a', 'libt81_vm.a'
Good, build succeeded. Now test tier-annotation-enforcement.t81:

Bash Test tier-annotation-enforcement after VariableExpr caching fix
IN
./build/t81 run spec/conformance/cognitive-tiers/tier-annotation-enforcement.t81 2>&1

OUT
Exit code 14
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-tier-annotation-enforcement-5009813303531394222.tisc
error: Execution trapped: DecodeFault

warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-tier-annotation-enforcement-5009813303531394222.tisc
error: Execution trapped: DecodeFault
Still DecodeFault. Let me read the program and examine the IR being generated to debug further:

Read tier-annotation-enforcement.t81
Let me check the type_of method on the SemanticAnalyzer:

Grep "type_of\b" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/semantic_analyzer.hpp)
7 lines of output
Grep "SemanticAnalyzer::type_of" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
21 lines of output
Let me check the IR generator's visit(IndexExpr) to see my matrix double-index code:

Grep "Matrix 2D|TShape|inner->object|inner_obj_type" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
15 lines of output
Read ir_generator.hpp (lines 5128-5227)
Let me check how visit(IndexExpr) in the semantic analyzer processes its object (does it call evaluate_expression on the object?):

Grep "visit\(const IndexExpr" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
3 lines of output
Read semantic_analyzer.cpp (lines 4429-4488)
Let me check evaluate_expression and whether it caches results:

Grep "evaluate_expression\b" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
21 lines of output
Read semantic_analyzer.cpp (lines 1458-1482)
Let me check the vector_literal_data method in the semantic analyzer:

Grep "vector_literal_data" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/semantic_analyzer.hpp)
15 lines of output
Grep "SemanticAnalyzer::vector_literal_data" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
21 lines of output
Grep "_vector_literal_data\b" (in /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp)
24 lines of output
Read semantic_analyzer.cpp (lines 4621-4740)
Let me check the IR generator's visit(VariableExpr) and how it handles Matrix types:

Grep "visit\(const VariableExpr" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
7 lines of output
Read ir_generator.hpp (lines 1391-1420)
Now let me look at how the IR generator handles let m: Matrix[T81BigInt] = [[1, 2], [3, 4]]:

Grep "visit\(const VarDecl|visit\(const LetStmt|VarDeclStmt" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
5 lines of output
Read ir_generator.hpp (lines 649-748)
Let me look at the VectorLiteralExpr Matrix code in the IR generator and how tensor handles work:

Grep "add_tensor\|TensorHandle\|tensor_pool" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
2 lines of output
Grep "Matrix|add_tensor|TensorHandle" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
41 lines of output
Let me check how the VM handles LoadImm with a TensorHandle:

Grep "LoadImm|TensorHandle|tensor_pool|literal_kind" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
51 lines of output
Grep "case Opcode::LoadImm" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
1 line of output
Grep "LoadImm" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
41 lines of output
Let me see the TShape implementation in vm.cpp:

Grep "case t81::tisc::Opcode::TShape" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
61 lines of output
Let me look at promote_to_tensor:

Grep "promote_to_tensor" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
51 lines of output
Let me check add_tensor to see if it's 0 or 1-based indexing:

Grep "add_tensor" (in /Users/t81dev/Code/t81-foundation/include/t81/isa/program.hpp)
1 line of output
Grep "add_tensor\|tensor_pool" (in /Users/t81dev/Code/t81-foundation/include/t81/isa/program.hpp)
1 line of output
Grep "add_tensor" (in /Users/t81dev/Code/t81-foundation/include/t81)
35 lines of output
Let me check tensor_ptr to see how it handles the index:

Grep "tensor_ptr\b" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
21 lines of output
Let me check state_.tensors type:

Grep "std::vector.*tensors\b|tensors.*optional" (in /Users/t81dev/Code/t81-foundation/include/t81/vm/vm.hpp)
1 line of output
Grep "tensors\b" (in /Users/t81dev/Code/t81-foundation/include/t81/vm/vm.hpp)
1 line of output
Globpattern: "include/t81/vm/*.hpp"
Found 3 files
Grep "tensors\b" (in /Users/t81dev/Code/t81-foundation/include/t81/vm/state.hpp)
15 lines of output
Let me check the default value of LiteralKind:

Grep "enum.*LiteralKind|LiteralKind\s*{" (in /Users/t81dev/Code/t81-foundation/include/t81/isa/program.hpp)
1 line of output
Globpattern: "include/t81/isa/*.hpp"
Found 10 files
Grep "LiteralKind" (in /Users/t81dev/Code/t81-foundation/include/t81/isa/program.hpp)
23 lines of output
Read vm.cpp (lines 453-482)
Let me check the reg_ok lambda:

Grep "reg_ok\b" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
16 lines of output
Let me check the ensure_expr_result method:

Grep "ensure_expr_result\b" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
21 lines of output
Grep "TypedRegister ensure_expr_result" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
31 lines of output
Grep "typed_expr\b" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
21 lines of output
Grep "const Type\* typed_expr\b" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
11 lines of output
Let me check the Register and TypedRegister types in the IR:

Grep "struct Register\b|struct TypedRegister\b" (in /Users/t81dev/Code/t81-foundation/include/t81/isa/ir.hpp)
17 lines of output
Grep "struct TypedRegister" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
17 lines of output
Grep "allocate_typed_register\b" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
16 lines of output
Grep "TypedRegister allocate_typed_register" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
21 lines of output
Grep "new_register\(\)" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
43 lines of output
Grep "ctx\.registers\.resize\|registers\(.*\)|CallContext\b" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
2 lines of output
Grep "struct CallContext\b|registers\b.*size\|register_count\b" (in /Users/t81dev/Code/t81-foundation/include/t81/vm/state.hpp)
1 line of output
Grep "registers\b" (in /Users/t81dev/Code/t81-foundation/include/t81/vm/state.hpp)
23 lines of output
Let me check if _register_count is reset between functions:

Grep "_register_count\b" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ir_generator.hpp)
26 lines of output
Let me check how the VM handles CALL opcode and whether registers are per-frame:

Grep "case t81::tisc::Opcode::Call" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
41 lines of output
Read vm.cpp (lines 2461-2520)
Let me check the Type::Kind enum and specifically the Matrix kind:

Grep "Matrix\b" (in /Users/t81dev/Code/t81-foundation/include/t81/frontend/ast.hpp)
2 lines of output
Bash Find Matrix type handling in semantic analyzer
IN
grep -n "Matrix\b" /Users/t81dev/Code/t81-foundation/lang/frontend/semantic_analyzer.cpp | head -5

OUT
1107:    case TokenType::Matrix:
1108:      return Type{Type::Kind::Matrix};
1134:  if (name_str == "Matrix") return Type{Type::Kind::Matrix};
1296:    case Type::Kind::Matrix:
1304:      else if (type.kind == Type::Kind::Matrix)
Let me write a small test program for Matrix 2D indexing:

Bash Test minimal Matrix 2D indexing
IN
cat > /tmp/test_matrix.t81 << 'EOF'
fn test_matrix() -> bool {
    let m: Matrix[T81BigInt] = [[1, 2], [3, 4]];
    m[0][0] == 1
}

fn main() -> void {
    assert test_matrix();
}
EOF
./build/t81 run /tmp/test_matrix.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_matrix-212130737525747914.tisc
Program terminated normally
The minimal matrix test works. Now let me try building up to the full program:

Bash Test Matrix 2D indexing with both accesses
IN
cat > /tmp/test_matrix2.t81 << 'EOF'
fn test_matrix() -> bool {
    let m: Matrix[T81BigInt] = [[1, 2], [3, 4]];
    m[0][0] == 1 && m[1][1] == 4
}

fn main() -> void {
    assert test_matrix();
}
EOF
./build/t81 run /tmp/test_matrix2.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_matrix2-8527840430511962470.tisc
Program terminated normally
Bash Test Matrix 2D with a vector function before it
IN
cat > /tmp/test_matrix3.t81 << 'EOF'
fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_matrix_rank() -> bool {
    let m: Matrix[T81BigInt] = [[1, 2], [3, 4]];
    m[0][0] == 1 && m[1][1] == 4
}

fn main() -> void {
    assert check_tier1_tensor_rank_limit();
    assert check_tier2_matrix_rank();
}
EOF
./build/t81 run /tmp/test_matrix3.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_matrix3-16498161410145448780.tisc
Program terminated normally
Bash Test most of tier conformance functions
IN
cat > /tmp/test_tier.t81 << 'EOF'
fn check_tier1_bounded_loop() -> bool {
    let mut acc: T81BigInt = 0;
    for i in 1..=5 {
        acc = acc + i;
    }
    acc == 15
}

fn check_tier1_canonical_types() -> bool {
    let a: T81BigInt  = 81;
    let f: T81Fraction = 1 / 3;
    let q: T81Qutrit  = 1t;
    a == 81 && f == 1 / 3 && q == 1t
}

fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn check_tier2_matrix_rank() -> bool {
    let m: Matrix[T81BigInt] = [[1, 2], [3, 4]];
    m[0][0] == 1 && m[1][1] == 4
}

fn main() -> void {
    assert check_tier1_bounded_loop();
    assert check_tier1_canonical_types();
    assert check_tier1_tensor_rank_limit();
    assert check_tier2_recursion_bounded();
    assert check_tier2_matrix_rank();
}
EOF
./build/t81 run /tmp/test_tier.t81 2>&1

OUT
Exit code 14
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier-8025896856039025672.tisc
error: Execution trapped: DecodeFault

warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier-8025896856039025672.tisc
error: Execution trapped: DecodeFault
Let me narrow down which function causes the DecodeFault:

Bash Test tier conformance without matrix
IN
cat > /tmp/test_tier_b.t81 << 'EOF'
fn check_tier1_bounded_loop() -> bool {
    let mut acc: T81BigInt = 0;
    for i in 1..=5 {
        acc = acc + i;
    }
    acc == 15
}

fn check_tier1_canonical_types() -> bool {
    let a: T81BigInt  = 81;
    let f: T81Fraction = 1 / 3;
    let q: T81Qutrit  = 1t;
    a == 81 && f == 1 / 3 && q == 1t
}

fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert check_tier1_bounded_loop();
    assert check_tier1_canonical_types();
    assert check_tier1_tensor_rank_limit();
    assert check_tier2_recursion_bounded();
}
EOF
./build/t81 run /tmp/test_tier_b.t81 2>&1

OUT
Exit code 14
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_b-3104311875132539706.tisc
error: Execution trapped: DecodeFault

warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_b-3104311875132539706.tisc
error: Execution trapped: DecodeFault
Bash Test just canonical types + recursion
IN
cat > /tmp/test_tier_c.t81 << 'EOF'
fn check_tier1_canonical_types() -> bool {
    let a: T81BigInt  = 81;
    let f: T81Fraction = 1 / 3;
    let q: T81Qutrit  = 1t;
    a == 81 && f == 1 / 3 && q == 1t
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert check_tier1_canonical_types();
    assert check_tier2_recursion_bounded();
}
EOF
./build/t81 run /tmp/test_tier_c.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_c-12900240385250155637.tisc
Program terminated normally
Bash Test bounded loop + canonical types + recursion
IN
cat > /tmp/test_tier_d.t81 << 'EOF'
fn check_tier1_bounded_loop() -> bool {
    let mut acc: T81BigInt = 0;
    for i in 1..=5 {
        acc = acc + i;
    }
    acc == 15
}

fn check_tier1_canonical_types() -> bool {
    let a: T81BigInt  = 81;
    let f: T81Fraction = 1 / 3;
    let q: T81Qutrit  = 1t;
    a == 81 && f == 1 / 3 && q == 1t
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert check_tier1_bounded_loop();
    assert check_tier1_canonical_types();
    assert check_tier2_recursion_bounded();
}
EOF
./build/t81 run /tmp/test_tier_d.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_d-10553822897260773272.tisc
Program terminated normally
Bash Add tensor rank limit to see if that's the trigger
IN
cat > /tmp/test_tier_e.t81 << 'EOF'
fn check_tier1_bounded_loop() -> bool {
    let mut acc: T81BigInt = 0;
    for i in 1..=5 {
        acc = acc + i;
    }
    acc == 15
}

fn check_tier1_canonical_types() -> bool {
    let a: T81BigInt  = 81;
    let f: T81Fraction = 1 / 3;
    let q: T81Qutrit  = 1t;
    a == 81 && f == 1 / 3 && q == 1t
}

fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert check_tier1_bounded_loop();
    assert check_tier1_canonical_types();
    assert check_tier1_tensor_rank_limit();
    assert check_tier2_recursion_bounded();
}
EOF
./build/t81 run /tmp/test_tier_e.t81 2>&1

OUT
Exit code 14
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_e-396242846726478727.tisc
error: Execution trapped: DecodeFault

warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_e-396242846726478727.tisc
error: Execution trapped: DecodeFault
The issue involves combining tensor_rank_limit with recursion_bounded. Let me isolate further:

Bash Isolate tensor + recursion DecodeFault
IN
cat > /tmp/test_tier_f.t81 << 'EOF'
fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert check_tier1_tensor_rank_limit();
    assert check_tier2_recursion_bounded();
}
EOF
./build/t81 run /tmp/test_tier_f.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_f-4490807866564063781.tisc
Program terminated normally
Bash Add bounded loop to isolate
IN
cat > /tmp/test_tier_g.t81 << 'EOF'
fn check_tier1_bounded_loop() -> bool {
    let mut acc: T81BigInt = 0;
    for i in 1..=5 {
        acc = acc + i;
    }
    acc == 15
}

fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert check_tier1_bounded_loop();
    assert check_tier1_tensor_rank_limit();
    assert check_tier2_recursion_bounded();
}
EOF
./build/t81 run /tmp/test_tier_g.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_g-12302543194838409652.tisc
Program terminated normally
Bash Try reordering to find the issue
IN
cat > /tmp/test_tier_h.t81 << 'EOF'
fn check_tier1_bounded_loop() -> bool {
    let mut acc: T81BigInt = 0;
    for i in 1..=5 {
        acc = acc + i;
    }
    acc == 15
}

fn check_tier1_canonical_types() -> bool {
    let a: T81BigInt  = 81;
    let f: T81Fraction = 1 / 3;
    let q: T81Qutrit  = 1t;
    a == 81 && f == 1 / 3 && q == 1t
}

fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert check_tier1_bounded_loop();
    assert check_tier2_recursion_bounded();
    assert check_tier1_canonical_types();
    assert check_tier1_tensor_rank_limit();
}
EOF
./build/t81 run /tmp/test_tier_h.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_h-15313406026548637900.tisc
Program terminated normally
Bash Try canonical before tensor rank
IN
cat > /tmp/test_tier_i.t81 << 'EOF'
fn check_tier1_bounded_loop() -> bool {
    let mut acc: T81BigInt = 0;
    for i in 1..=5 {
        acc = acc + i;
    }
    acc == 15
}

fn check_tier1_canonical_types() -> bool {
    let a: T81BigInt  = 81;
    let f: T81Fraction = 1 / 3;
    let q: T81Qutrit  = 1t;
    a == 81 && f == 1 / 3 && q == 1t
}

fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert check_tier1_bounded_loop();
    assert check_tier1_canonical_types();
    assert check_tier2_recursion_bounded();
    assert check_tier1_tensor_rank_limit();
}
EOF
./build/t81 run /tmp/test_tier_i.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_i-402058206856945989.tisc
Program terminated normally
Stop and document your progress so we can pick up where you leave off.
The issue involves the interaction of canonical_types + tensor_rank + recursion. Let me document progress and save state before stopping.

Update Todos

Fix DecodeFault in tier-annotation-enforcement.t81 (bounded_loop+canonical+tensor_rank+recursion combo)

Fix type-kind-completeness.t81 (byte-string, Set/Map literals)

Update docs/status after 24/24
Now let me update memory with the current investigation state:

Read MEMORY.md
Edit MEMORY.md
Added 31 lines
Here's where we stand:

Progress Summary
Conformance Suite: 22/24 passing (was 21/24 at start of session)

What was fixed this session
22nd program (policy-enforcement-allow-deny.t81) — now passing
Root cause: Result.unwrap_ok() emitted OPTION_UNWRAP instead of RESULT_UNWRAP_OK
Fix: SA visit(CallExpr) now caches the object expression's type in _expr_type_cache after resolving the method's owning symbol
Added TShape opcode end-to-end (opcodes.hpp, ir.hpp, binary_emitter.cpp, vm.cpp)
Added Matrix 2D literal building and double-index codegen to the IR generator
Still failing (2 programs)
23rd — tier-annotation-enforcement.t81 — DecodeFault

Narrowed to a specific function combination. The Matrix 2D indexing itself works in isolation:


tensor_rank + recursion              ✅ OK
bounded_loop + canonical + recursion ✅ OK
canonical + tensor_rank + recursion  ❌ DecodeFault
The suspected root cause is register clobbering across recursive function calls. The VM uses a flat 243-register file shared across all calls — no save/restore. With many functions in scope (canonical_types uses fraction/qutrit handles, adding register pressure), the recursive call to check_tier2_shallow_recursion overwrites registers the outer frame still needs.

Next step: look at how the IR generator passes function arguments and handles return values for recursive calls — specifically whether check_tier2_shallow_recursion's parameter register is properly isolated from the recursive call's parameter register.

24th — type-kind-completeness.t81 — parse errors

Needs: byte-string literals (b"\x74\x38\x31"), Set literals ({10, 20}), Map literals ({:x => 1}), and their SA/IR/VM implementations.

Should we increase register size to 729?
Good question. But first let me verify whether the DecodeFault is a register-bounds issue or something else entirely — let me check what check_mem does for code addresses, since the recursive CALL is a likely culprit:

Grep "check_mem\b" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
21 lines of output
Grep "t81::vm::internal::mem_ok" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
6 lines of output
Grep "bool mem_ok\b" (in /Users/t81dev/Code/t81-foundation/core/vm)
37 lines of output
Grep "layout\.code\b|code\.start\|code\.limit\|code_limit\|code_start" (in /Users/t81dev/Code/t81-foundation/core/vm/vm.cpp)
14 lines of output
Let me narrow it down more precisely — is it specifically T81Fraction triggering the issue?

Bash Test if T81Fraction + tensor + recursion triggers the issue
IN
cat > /tmp/test_tier_j.t81 << 'EOF'
fn use_fraction() -> bool {
    let f: T81Fraction = 1 / 3;
    f == 1 / 3
}

fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert use_fraction();
    assert check_tier1_tensor_rank_limit();
    assert check_tier2_recursion_bounded();
}
EOF
./build/t81 run /tmp/test_tier_j.t81 2>&1

OUT
warning: 'run' is a legacy alias; prefer 't81 code run'.
Compilation successful → /var/folders/18/z40pkrn17sj74lvzwnq2d4vc0000gn/T/t81-test_tier_j-7790272684007221108.tisc
Program terminated normally
Bash Compile and disassemble to inspect register usage around recursion
IN
cat > /tmp/test_tier_k.t81 << 'EOF'
fn check_tier1_bounded_loop() -> bool {
    let mut acc: T81BigInt = 0;
    for i in 1..=5 {
        acc = acc + i;
    }
    acc == 15
}

fn check_tier1_canonical_types() -> bool {
    let a: T81BigInt  = 81;
    let f: T81Fraction = 1 / 3;
    let q: T81Qutrit  = 1t;
    a == 81 && f == 1 / 3 && q == 1t
}

fn check_tier1_tensor_rank_limit() -> bool {
    let v: Vector[T81BigInt] = [1, 2, 3];
    v[0] == 1 && v.len() == 3
}

fn check_tier2_shallow_recursion(n: T81BigInt) -> T81BigInt {
    if n <= 0 {
        0
    } else {
        1 + check_tier2_shallow_recursion(n - 1)
    }
}

fn check_tier2_recursion_bounded() -> bool {
    check_tier2_shallow_recursion(3) == 3
}

fn main() -> void {
    assert check_tier1_bounded_loop();
    assert check_tier1_canonical_types();
    assert check_tier1_tensor_rank_limit();
    assert check_tier2_recursion_bounded();
}
EOF
# Check the disassembly around the recursion area
./build/t81 code compile /tmp/test_tier_k.t81 -o /tmp/test_tier_k.tisc 2>&1 && ./build/t81 code disasm /tmp/test_tier_k.tisc 2>&1 | grep -n "LOADI\|Call\|Ret\|check_tier2\|R[0-9]" | head -60

OUT
Exit code 1
error: Unknown code action: compile. Run 't81 help code'.

error: Unknown code action: compile. Run 't81 help code'.
Bash Check available CLI commands
IN
./build/t81 help 2>&1 | head -30

OUT
T81 Foundation - Ternary-Native Computing Stack
Version 1.1.0


Usage: ./build/t81 <command> [options] [args]


Commands:
  code    <action> [args]               Domain-first code workflow commands
  project <action> [args]               Project lifecycle commands
  env     <action> [args]               Environment/toolchain diagnostics
  internal <action> [args]              Internal/experimental command group
  completion <shell>                    Print shell completion script
  man [--install-dir <dir>]             Show or install CLI manpage
  feedback <subcommand> [args]          Local CLI UX feedback loop
  ---
  check   <file.t81>                   Syntax/semantic check (no bytecode output)
  lint    <file.t81>                   Alias for `check`
  compile <file.t81|.t81w> [...]       Compile source to TISC bytecode
  run     <file.t81|.tisc> [...]       Compile (if needed) and execute
  disasm  <file.tisc>                  Print human-readable TISC disassembly
  debug   <file.t81|.tisc> [...]       Start debugger (compile if needed)
  repl                                 Enter interactive REPL
  test    [options] [-- ...]           Run project tests via CTest
  doctor  [--json]                     Check environment/toolchain readiness
  fmt     [options] <file...>          Format files (supports --check)
  init    <project_name>               Scaffold a new T81 project
  version                              Show version
  help [command]                       Show help for command/topic
Bash Compile and disassemble the failing test to inspect CALL targets
IN
./build/t81 compile /tmp/test_tier_k.t81 -o /tmp/test_tier_k.tisc 2>&1 && ./build/t81 disasm /tmp/test_tier_k.tisc 2>&1 | grep -n "LOADI\|Call\|Ret " | head -60

OUT
warning: 'compile' is a legacy alias; prefer 't81 code build'.
Compilation successful → /tmp/test_tier_k.tisc
5:0001: Call a=0 b=1 c=0 lit=0
23:0019: Ret a=0 b=0 c=0 lit=0
52:0048: Ret a=0 b=0 c=0 lit=0
69:0065: Ret a=0 b=0 c=0 lit=0
83:0079: Call a=0 b=52 c=0 lit=0
89:0085: Ret a=0 b=0 c=0 lit=0
94:0090: Call a=0 b=57 c=0 lit=0
100:0096: Ret a=0 b=0 c=0 lit=0
103:0099: Call a=0 b=62 c=0 lit=0
108:0104: Call a=0 b=64 c=0 lit=0
113:0109: Call a=0 b=66 c=0 lit=0
118:0114: Call a=0 b=68 c=0 lit=0
123:0119: Ret a=0 b=0 c=0 lit=0
Bash Full disassembly of failing test program
IN
./build/t81 disasm /tmp/test_tier_k.tisc 2>&1 | head -130

OUT
warning: 'disasm' is a legacy alias; prefer 't81 code disasm'.
; t81 disasm /tmp/test_tier_k.tisc
; instructions=120 floats=0 fractions=0 symbols=0 tensors=1 shapes=0
0000: LoadImm a=1 b=97 c=0 lit=0
0001: Call a=0 b=1 c=0 lit=0
0002: Halt a=0 b=0 c=0 lit=0
0003: Pop a=2 b=0 c=0 lit=0
0004: LoadImm a=3 b=0 c=0 lit=0
0005: LoadImm a=4 b=1 c=0 lit=0
0006: LoadImm a=5 b=5 c=0 lit=0
0007: Mov a=6 b=4 c=0 lit=0
0008: LessEqual a=7 b=6 c=5 lit=0
0009: JumpIfZero a=15 b=7 c=0 lit=0
0010: Add a=8 b=3 c=6 lit=0
0011: Mov a=3 b=8 c=0 lit=0
0012: LoadImm a=9 b=1 c=0 lit=0
0013: Add a=6 b=6 c=9 lit=0
0014: Jump a=8 b=0 c=0 lit=0
0015: LoadImm a=10 b=15 c=0 lit=0
0016: Equal a=11 b=3 c=10 lit=0
0017: Push a=11 b=0 c=0 lit=0
0018: Push a=2 b=0 c=0 lit=0
0019: Ret a=0 b=0 c=0 lit=0
0020: Pop a=12 b=0 c=0 lit=0
0021: LoadImm a=13 b=81 c=0 lit=0
0022: LoadImm a=14 b=1 c=0 lit=0
0023: LoadImm a=15 b=3 c=0 lit=0
0024: I2Frac a=16 b=14 c=0 lit=0
0025: I2Frac a=17 b=15 c=0 lit=0
0026: FracDiv a=18 b=16 c=17 lit=0
0027: LoadImm a=19 b=1 c=0 lit=0
0028: LoadImm a=20 b=0 c=0 lit=1
0029: LoadImm a=21 b=0 c=0 lit=1
0030: LoadImm a=22 b=81 c=0 lit=0
0031: Equal a=23 b=13 c=22 lit=0
0032: JumpIfZero a=41 b=23 c=0 lit=0
0033: LoadImm a=24 b=1 c=0 lit=0
0034: LoadImm a=25 b=3 c=0 lit=0
0035: I2Frac a=26 b=24 c=0 lit=0
0036: I2Frac a=27 b=25 c=0 lit=0
0037: FracDiv a=28 b=26 c=27 lit=0
0038: Equal a=29 b=18 c=28 lit=0
0039: JumpIfZero a=41 b=29 c=0 lit=0
0040: LoadImm a=21 b=1 c=0 lit=1
0041: JumpIfZero a=46 b=21 c=0 lit=0
0042: LoadImm a=30 b=1 c=0 lit=0
0043: Equal a=31 b=19 c=30 lit=0
0044: JumpIfZero a=46 b=31 c=0 lit=0
0045: LoadImm a=20 b=1 c=0 lit=1
0046: Push a=20 b=0 c=0 lit=0
0047: Push a=12 b=0 c=0 lit=0
0048: Ret a=0 b=0 c=0 lit=0
0049: Pop a=32 b=0 c=0 lit=0
0050: LoadImm a=33 b=1 c=0 lit=5
0051: LoadImm a=34 b=0 c=0 lit=1
0052: LoadImm a=35 b=0 c=0 lit=0
0053: TGet a=36 b=33 c=35 lit=0
0054: F2I a=37 b=36 c=0 lit=0
0055: LoadImm a=38 b=1 c=0 lit=0
0056: Equal a=39 b=37 c=38 lit=0
0057: JumpIfZero a=63 b=39 c=0 lit=0
0058: VecLen a=40 b=33 c=0 lit=0
0059: LoadImm a=41 b=3 c=0 lit=0
0060: Equal a=42 b=40 c=41 lit=0
0061: JumpIfZero a=63 b=42 c=0 lit=0
0062: LoadImm a=34 b=1 c=0 lit=1
0063: Push a=34 b=0 c=0 lit=0
0064: Push a=32 b=0 c=0 lit=0
0065: Ret a=0 b=0 c=0 lit=0
0066: Pop a=43 b=0 c=0 lit=0
0067: Pop a=44 b=0 c=0 lit=0
0068: LoadImm a=45 b=0 c=0 lit=0
0069: LessEqual a=46 b=44 c=45 lit=0
0070: JumpIfZero a=74 b=46 c=0 lit=0
0071: LoadImm a=48 b=0 c=0 lit=0
0072: Mov a=47 b=48 c=0 lit=0
0073: Jump a=83 b=0 c=0 lit=0
0074: LoadImm a=49 b=1 c=0 lit=0
0075: LoadImm a=50 b=1 c=0 lit=0
0076: Sub a=51 b=44 c=50 lit=0
0077: Push a=51 b=0 c=0 lit=0
0078: LoadImm a=52 b=66 c=0 lit=0
0079: Call a=0 b=52 c=0 lit=0
0080: Pop a=53 b=0 c=0 lit=0
0081: Add a=54 b=49 c=53 lit=0
0082: Mov a=47 b=54 c=0 lit=0
0083: Push a=47 b=0 c=0 lit=0
0084: Push a=43 b=0 c=0 lit=0
0085: Ret a=0 b=0 c=0 lit=0
0086: Pop a=55 b=0 c=0 lit=0
0087: LoadImm a=56 b=3 c=0 lit=0
0088: Push a=56 b=0 c=0 lit=0
0089: LoadImm a=57 b=66 c=0 lit=0
0090: Call a=0 b=57 c=0 lit=0
0091: Pop a=58 b=0 c=0 lit=0
0092: LoadImm a=59 b=3 c=0 lit=0
0093: Equal a=60 b=58 c=59 lit=0
0094: Push a=60 b=0 c=0 lit=0
0095: Push a=55 b=0 c=0 lit=0
0096: Ret a=0 b=0 c=0 lit=0
0097: Pop a=61 b=0 c=0 lit=0
0098: LoadImm a=62 b=3 c=0 lit=0
0099: Call a=0 b=62 c=0 lit=0
0100: Pop a=63 b=0 c=0 lit=0
0101: JumpIfNotZero a=103 b=63 c=0 lit=0
0102: Trap a=0 b=0 c=0 lit=0
0103: LoadImm a=64 b=20 c=0 lit=0
0104: Call a=0 b=64 c=0 lit=0
0105: Pop a=65 b=0 c=0 lit=0
0106: JumpIfNotZero a=108 b=65 c=0 lit=0
0107: Trap a=0 b=0 c=0 lit=0
0108: LoadImm a=66 b=49 c=0 lit=0
0109: Call a=0 b=66 c=0 lit=0
0110: Pop a=67 b=0 c=0 lit=0
0111: JumpIfNotZero a=113 b=67 c=0 lit=0
0112: Trap a=0 b=0 c=0 lit=0
0113: LoadImm a=68 b=86 c=0 lit=0
0114: Call a=0 b=68 c=0 lit=0
0115: Pop a=69 b=0 c=0 lit=0
0116: JumpIfNotZero a=118 b=69 c=0 lit=0
0117: Trap a=0 b=0 c=0 lit=0
0118: Push a=61 b=0 c=0 lit=0
0119: Ret a=0 b=0 c=0 lit=0
Let me look at the VM's main execution loop to understand how PC advances:

Read vm.cpp (lines 396-435)
You've hit your limit · resets Mar 7 at 9am (America/New_York)


