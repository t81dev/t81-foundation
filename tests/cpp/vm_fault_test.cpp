#include "t81/tisc/opcodes.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

#include <cassert>
#include <vector>

namespace {
t81::vm::Trap run_expected_trap(const std::vector<t81::tisc::Insn>& insns) {
    t81::tisc::Program program;
    program.insns = insns;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    assert(!result.has_value());
    return result.error();
}
} // namespace

int main() {
    t81::tisc::Insn load_ten;
    load_ten.opcode = t81::tisc::Opcode::LoadImm;
    load_ten.a = 0;
    load_ten.b = 10;
    t81::tisc::Insn load_zero;
    load_zero.opcode = t81::tisc::Opcode::LoadImm;
    load_zero.a = 1;
    load_zero.b = 0;
    t81::tisc::Insn div;
    div.opcode = t81::tisc::Opcode::Div;
    div.a = 0;
    div.b = 0;
    div.c = 1;
    t81::tisc::Insn halt;
    halt.opcode = t81::tisc::Opcode::Halt;
    auto trap_div_zero = run_expected_trap({load_ten, load_zero, div, halt});
    assert(trap_div_zero == t81::vm::Trap::DivideByZero);

    t81::tisc::Insn load_bad;
    load_bad.opcode = t81::tisc::Opcode::Load;
    load_bad.a = 0;
    load_bad.b = 999999;
    load_bad.c = 0;
    auto trap_invalid_mem = run_expected_trap({load_bad, halt});
    assert(trap_invalid_mem == t81::vm::Trap::InvalidMemory);

    t81::tisc::Insn pop{t81::tisc::Opcode::Pop, {0}};
    auto trap_bounds = run_expected_trap({pop, halt});
    assert(trap_bounds == t81::vm::Trap::BoundsFault);

    return 0;
}
