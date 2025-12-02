#include "t81/tisc/program.hpp"
#include "t81/tisc/opcodes.hpp"
#include "t81/vm/vm.hpp"

#include <cassert>
#include <vector>

namespace {

std::unique_ptr<t81::vm::IVirtualMachine> run_program(const std::vector<t81::tisc::Insn>& insns) {
    t81::tisc::Program program;
    program.insns = insns;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    assert(result.has_value());
    return vm;
}

t81::vm::Trap run_expected_trap(const std::vector<t81::tisc::Insn>& insns) {
    t81::tisc::Program program;
    program.insns = insns;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    assert(!result.has_value());
    return result.error();
}

}  // namespace

int main() {
    t81::tisc::Insn stack_alloc{};
    stack_alloc.opcode = t81::tisc::Opcode::StackAlloc;
    stack_alloc.a = 0;
    stack_alloc.b = 16;

    t81::tisc::Insn stack_alloc2{};
    stack_alloc2.opcode = t81::tisc::Opcode::StackAlloc;
    stack_alloc2.a = 1;
    stack_alloc2.b = 32;

    t81::tisc::Insn stack_free{};
    stack_free.opcode = t81::tisc::Opcode::StackFree;
    stack_free.a = 1;
    stack_free.b = 32;

    t81::tisc::Insn stack_free0{};
    stack_free0.opcode = t81::tisc::Opcode::StackFree;
    stack_free0.a = 0;
    stack_free0.b = 16;

    t81::tisc::Insn halt{};
    halt.opcode = t81::tisc::Opcode::Halt;

    {
        auto vm = run_program({stack_alloc, stack_free0, halt});
        assert(vm->state().stack_frames.empty());
        assert(vm->state().sp == vm->state().layout.stack_limit);
        assert(vm->state().registers[0] >= static_cast<std::int64_t>(vm->state().layout.code_limit));
    }

    {
        auto vm = run_program({stack_alloc, stack_alloc2, stack_free, stack_free0, halt});
        assert(vm->state().stack_frames.empty());
        assert(vm->state().sp == vm->state().layout.stack_limit);
    }

    {
        auto trap = run_expected_trap({stack_alloc, stack_alloc2, stack_free0, halt});
        assert(trap == t81::vm::Trap::IllegalInstruction);
    }

    t81::tisc::Insn stack_overflow{};
    stack_overflow.opcode = t81::tisc::Opcode::StackAlloc;
    stack_overflow.a = 2;
    stack_overflow.b = 512;
    {
        auto trap = run_expected_trap({stack_overflow, halt});
        assert(trap == t81::vm::Trap::BoundsFault);
    }

    t81::tisc::Insn heap_alloc{};
    heap_alloc.opcode = t81::tisc::Opcode::HeapAlloc;
    heap_alloc.a = 3;
    heap_alloc.b = 32;

    t81::tisc::Insn heap_free{};
    heap_free.opcode = t81::tisc::Opcode::HeapFree;
    heap_free.a = 3;
    heap_free.b = 32;

    {
        auto vm = run_program({heap_alloc, heap_free, halt});
        assert(vm->state().heap_frames.empty());
        assert(vm->state().heap_ptr == vm->state().layout.stack_limit);
    }

    {
        auto trap = run_expected_trap({heap_alloc, heap_alloc, heap_free, halt});
        assert(trap == t81::vm::Trap::IllegalInstruction);
    }

    t81::tisc::Insn heap_big{};
    heap_big.opcode = t81::tisc::Opcode::HeapAlloc;
    heap_big.a = 4;
    heap_big.b = 1024;
    {
        auto trap = run_expected_trap({heap_big, halt});
        assert(trap == t81::vm::Trap::BoundsFault);
    }

    return 0;
}
