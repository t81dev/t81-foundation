#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
t81::vm::Trap run_until_trap(const std::vector<t81::tisc::Insn>& insns) {
  t81::tisc::Program program;
  program.insns = insns;
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result.has_value()) {
    return result.error();
  }
  return t81::vm::Trap::None;
}

void require(bool condition, const char* msg = "Assertion failed") {
  if (!condition) {
    std::cerr << "FATAL: " << msg << std::endl;
    std::abort();
  }
}

void run_expect_success(const std::vector<t81::tisc::Insn>& insns) {
  t81::vm::Trap t = run_until_trap(insns);
  if (t != t81::vm::Trap::None) {
    std::cerr << "Trap: " << (int)t << "\n";
  }
  require(t == t81::vm::Trap::None, "expected execution to succeed without trap");
}
}  // namespace

int main() {
  std::cout << "Testing Bitwise Opcodes..." << std::endl;

  // BitAnd, BitOr, BitXor
  {
    std::vector<t81::tisc::Insn> prog;
    // R1 = 0b1100 (12)
    prog.push_back({t81::tisc::Opcode::LoadImm, 1, 12, 0});
    // R2 = 0b1010 (10)
    prog.push_back({t81::tisc::Opcode::LoadImm, 2, 10, 0});

    // R3 = R1 & R2 = 0b1000 (8)
    prog.push_back({t81::tisc::Opcode::BitAnd, 3, 1, 2});
    prog.push_back({t81::tisc::Opcode::LoadImm, 4, 8, 0});
    prog.push_back({t81::tisc::Opcode::Equal, 5, 3, 4});
    prog.push_back({t81::tisc::Opcode::Assert, 5, 0, 0});

    // R3 = R1 | R2 = 0b1110 (14)
    prog.push_back({t81::tisc::Opcode::BitOr, 3, 1, 2});
    prog.push_back({t81::tisc::Opcode::LoadImm, 4, 14, 0});
    prog.push_back({t81::tisc::Opcode::Equal, 5, 3, 4});
    prog.push_back({t81::tisc::Opcode::Assert, 5, 0, 0});

    // R3 = R1 ^ R2 = 0b0110 (6)
    prog.push_back({t81::tisc::Opcode::BitXor, 3, 1, 2});
    prog.push_back({t81::tisc::Opcode::LoadImm, 4, 6, 0});
    prog.push_back({t81::tisc::Opcode::Equal, 5, 3, 4});
    prog.push_back({t81::tisc::Opcode::Assert, 5, 0, 0});

    prog.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
    run_expect_success(prog);
  }

  // BitNot
  {
    std::vector<t81::tisc::Insn> prog;
    // R1 = 0
    prog.push_back({t81::tisc::Opcode::LoadImm, 1, 0, 0});
    // R2 = ~R1 = -1
    prog.push_back({t81::tisc::Opcode::BitNot, 2, 1, 0});
    prog.push_back({t81::tisc::Opcode::LoadImm, 3, -1, 0});
    prog.push_back({t81::tisc::Opcode::Equal, 4, 2, 3});
    prog.push_back({t81::tisc::Opcode::Assert, 4, 0, 0});

    prog.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
    run_expect_success(prog);
  }

  // BitShl
  {
    std::vector<t81::tisc::Insn> prog;
    // R1 = 1
    prog.push_back({t81::tisc::Opcode::LoadImm, 1, 1, 0});
    // R2 = 1 (shift amount)
    prog.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});
    // R3 = 1 << 1 = 2
    prog.push_back({t81::tisc::Opcode::BitShl, 3, 1, 2});
    prog.push_back({t81::tisc::Opcode::LoadImm, 4, 2, 0});
    prog.push_back({t81::tisc::Opcode::Equal, 5, 3, 4});
    prog.push_back({t81::tisc::Opcode::Assert, 5, 0, 0});

    // Masking test: 1 << 65 (0x41) should be 1 << (0x41 & 0x3F) = 1 << 1 = 2
    prog.push_back({t81::tisc::Opcode::LoadImm, 2, 65, 0});
    prog.push_back({t81::tisc::Opcode::BitShl, 3, 1, 2});
    prog.push_back({t81::tisc::Opcode::Equal, 5, 3, 4});
    prog.push_back({t81::tisc::Opcode::Assert, 5, 0, 0});

    prog.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
    run_expect_success(prog);
  }

  // BitShr (Arithmetic)
  {
    std::vector<t81::tisc::Insn> prog;
    // R1 = -2 (0xFFFF...FE)
    prog.push_back({t81::tisc::Opcode::LoadImm, 1, -2, 0});
    // R2 = 1
    prog.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});
    // R3 = -2 >> 1 = -1 (0xFFFF...FF)
    prog.push_back({t81::tisc::Opcode::BitShr, 3, 1, 2});
    prog.push_back({t81::tisc::Opcode::LoadImm, 4, -1, 0});
    prog.push_back({t81::tisc::Opcode::Equal, 5, 3, 4});
    prog.push_back({t81::tisc::Opcode::Assert, 5, 0, 0});

    prog.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
    run_expect_success(prog);
  }

  // BitUShr (Logical)
  {
    std::vector<t81::tisc::Insn> prog;
    // R1 = -2 (0xFFFF...FE)
    prog.push_back({t81::tisc::Opcode::LoadImm, 1, -2, 0});
    // R2 = 1
    prog.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});
    // R3 = (u64)-2 >> 1 = 0x7FFF...FF (INT64_MAX)
    prog.push_back({t81::tisc::Opcode::BitUShr, 3, 1, 2});
    // INT64_MAX is hard to load imm if we don't have large literals or it overflows int32 operand.
    // But LoadImm operand B is int32. Wait.
    // `t81::tisc::Insn` struct defines B as `int64_t` in memory, but encoding uses 32-bit.
    // In `Insn` struct in C++, `b` is `int64_t`.
    // Let's use `LoadImm` with a large value if the struct supports it.
    // `t81::tisc::Insn` has `int64_t b`.
    // `vm.cpp` LoadImm reads `insn.b`.
    // So I can put `9223372036854775807` (INT64_MAX) in `b`.

    int64_t int64_max = 9223372036854775807LL;
    prog.push_back({t81::tisc::Opcode::LoadImm, 4, int64_max, 0});
    prog.push_back({t81::tisc::Opcode::Equal, 5, 3, 4});
    prog.push_back({t81::tisc::Opcode::Assert, 5, 0, 0});

    prog.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
    run_expect_success(prog);
  }

  std::cout << "test_bitwise PASSED\n";
  return 0;
}
